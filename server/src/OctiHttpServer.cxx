/*
 * OctiHttpServer.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiHttpServer.hxx"

#include "OctiCore.hxx"
#include "OctiBoard.hxx"
#include "OctiCrypto.hxx"
#include "OctiExceptions.hxx"
#include "OctiGame.hxx"
#include "WebUi.hxx"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <thread>

namespace octi {

namespace {

typedef nlohmann::json Json;
typedef httplib::Request Request;
typedef httplib::Response Response;

const size_t kMaxBearerBytes = 256;
const size_t kMaxIdBytes = 64;
const size_t kMaxClientAgentBytes = 64;
const size_t kMaxCacheEntries = 20000;
const size_t kMaxQueryLimit = 500;

/* ------------------------------------------------------------------ */
/* Small helpers                                                       */
/* ------------------------------------------------------------------ */

int64_t steadyMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

int64_t epochSec()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string dumpJson(const Json &j)
{
    return j.dump(-1, ' ', false, Json::error_handler_t::replace);
}

std::string lower(const std::string &s)
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(out[i])));
    }
    return out;
}

std::string upper(const std::string &s)
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(out[i])));
    }
    return out;
}

bool hasControlChars(const std::string &s)
{
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x20 || c == 0x7f) {
            return true;
        }
    }
    return false;
}

/* Digits only (no sign), at most 18 of them; values above max clamp. */
bool parseUnsignedClamped(const std::string &s, int64_t maxValue,
                          int64_t &out)
{
    if (s.empty() || s.size() > 18) {
        return false;
    }
    int64_t v = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') {
            return false;
        }
        v = v * 10 + (s[i] - '0');
    }
    out = std::min(v, maxValue);
    return true;
}

/* Like parseUnsignedClamped but values above max are rejected. */
bool parseUnsignedStrict(const std::string &s, int64_t maxValue, int64_t &out)
{
    int64_t v = 0;
    if (s.empty() || s.size() > 18 || !parseUnsignedClamped(s, INT64_MAX, v) ||
        v > maxValue) {
        return false;
    }
    out = v;
    return true;
}

bool isSafeId(const std::string &s)
{
    if (s.empty() || s.size() > kMaxIdBytes) {
        return false;
    }
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (!(std::isalnum(c) || c == '-' || c == '_' || c == '.')) {
            return false;
        }
    }
    return true;
}

std::string cleanAgent(const std::string &in)
{
    std::string out;
    for (size_t i = 0; i < in.size() && out.size() < kMaxClientAgentBytes;
         ++i) {
        unsigned char c = static_cast<unsigned char>(in[i]);
        if (c >= 0x20 && c < 0x7F) {
            out += static_cast<char>(c);
        }
    }
    return out;
}

const char *modeName(OctiGameMode m)
{
    switch (m) {
    case GAME_MODE_4: return "4P";
    case GAME_MODE_OFK: return "OFK";
    default: break;
    }
    return "2P";
}

bool parseModeName(const std::string &s, OctiGameMode &out)
{
    std::string u = upper(s);
    if (u == "2P") {
        out = GAME_MODE_2;
        return true;
    }
    if (u == "4P") {
        out = GAME_MODE_4;
        return true;
    }
    if (u == "OFK") {
        out = GAME_MODE_OFK;
        return true;
    }
    return false;
}

/* ------------------------------------------------------------------ */
/* JSON field access with type checking                                */
/* ------------------------------------------------------------------ */

bool fieldString(const Json &b, const char *key, bool required,
                 std::string &out, std::string &err)
{
    Json::const_iterator it = b.find(key);
    if (it == b.end() || it->is_null()) {
        if (required) {
            err = std::string("missing field '") + key + "'";
            return false;
        }
        return true;
    }
    if (!it->is_string()) {
        err = std::string("field '") + key + "' must be a string";
        return false;
    }
    out = it->get<std::string>();
    return true;
}

bool fieldBool(const Json &b, const char *key, bool required, bool &out,
               std::string &err)
{
    Json::const_iterator it = b.find(key);
    if (it == b.end() || it->is_null()) {
        if (required) {
            err = std::string("missing field '") + key + "'";
            return false;
        }
        return true;
    }
    if (!it->is_boolean()) {
        err = std::string("field '") + key + "' must be a boolean";
        return false;
    }
    out = it->get<bool>();
    return true;
}

bool jsonInt(const Json &v, int64_t &out)
{
    if (v.is_number_unsigned()) {
        uint64_t u = v.get<uint64_t>();
        if (u > static_cast<uint64_t>(INT64_MAX)) {
            return false;
        }
        out = static_cast<int64_t>(u);
        return true;
    }
    if (v.is_number_integer()) {
        out = v.get<int64_t>();
        return true;
    }
    return false;
}

bool fieldInt(const Json &b, const char *key, bool required, int64_t &out,
              std::string &err)
{
    Json::const_iterator it = b.find(key);
    if (it == b.end() || it->is_null()) {
        if (required) {
            err = std::string("missing field '") + key + "'";
            return false;
        }
        return true;
    }
    if (!jsonInt(*it, out)) {
        err = std::string("field '") + key + "' must be an integer";
        return false;
    }
    return true;
}

bool fieldPresent(const Json &b, const char *key)
{
    Json::const_iterator it = b.find(key);
    return it != b.end() && !it->is_null();
}

/* ------------------------------------------------------------------ */
/* Responses                                                           */
/* ------------------------------------------------------------------ */

void sendJson(Response &res, int status, const Json &body)
{
    res.status = status;
    res.set_header("Cache-Control", "no-store");
    res.set_content(dumpJson(body), "application/json");
}

void sendError(Response &res, int status, const std::string &code,
               const std::string &message)
{
    Json j;
    j["error"] = code;
    j["message"] = message;
    sendJson(res, status, j);
}

int statusFor(RoomCode c)
{
    switch (c) {
    case RoomCode::OK:
        return 200;
    case RoomCode::UNKNOWN_PLAYER:
        return 401;
    case RoomCode::NOT_FOUND:
        return 404;
    case RoomCode::INVALID_ARGUMENT:
    case RoomCode::ILLEGAL_SYNTAX:
    case RoomCode::ILLEGAL_MOVE:
        return 400;
    case RoomCode::FORBIDDEN_BANNED:
    case RoomCode::FORBIDDEN_NOT_SEATED:
    case RoomCode::FORBIDDEN_NOT_PARTICIPANT:
    case RoomCode::FORBIDDEN_MUTED:
    case RoomCode::FORBIDDEN_POLICY:
    case RoomCode::FORBIDDEN_SEATED_TARGET:
    case RoomCode::FORBIDDEN_PASSCODE:
    case RoomCode::FORBIDDEN_NOT_HOST:
    case RoomCode::NOT_YOUR_TURN:
        return 403;
    case RoomCode::INVALID_STATE:
    case RoomCode::SEAT_TAKEN:
    case RoomCode::ALREADY_SEATED:
    case RoomCode::CONFLICT:
        return 409;
    case RoomCode::PAYLOAD_TOO_LARGE:
        return 413;
    case RoomCode::RATE_LIMITED:
        return 429;
    case RoomCode::LIMIT_REACHED:
        return 503;
    case RoomCode::INTERNAL_ERROR:
        break;
    }
    return 500;
}

void sendRoomError(Response &res, const RoomResult &r)
{
    sendError(res, statusFor(r.code), toString(r.code), r.message);
    if (r.code == RoomCode::RATE_LIMITED) {
        res.set_header("Retry-After", "5");
    }
}

int statusForVerify(VerifyStatus s)
{
    switch (s) {
    case VerifyStatus::OK:
        return 200;
    case VerifyStatus::MALFORMED:
    case VerifyStatus::UNSUPPORTED_PROVIDER:
    case VerifyStatus::PROVIDER_DISABLED:
        return 400;
    case VerifyStatus::KEY_SET_UNAVAILABLE:
    case VerifyStatus::REMOTE_FAILURE:
        return 503;
    default:
        break;
    }
    return 401;
}

/* ------------------------------------------------------------------ */
/* JSON views                                                          */
/* ------------------------------------------------------------------ */

Json profileJson(const PlayerProfile &p, bool includeSeen)
{
    Json j;
    j["uuid"] = p.uuid;
    j["handle"] = p.handle;
    j["display_name"] = p.displayName;
    j["avatar_url"] = p.avatarUrl;
    j["elo_2p"] = p.elo2p;
    j["elo_4p"] = p.elo4p;
    j["elo_ofk"] = p.eloOfk;
    j["games_played"] = p.gamesPlayed;
    j["wins"] = p.wins;
    j["losses"] = p.losses;
    j["draws"] = p.draws;
    j["created_at"] = p.createdAt;
    if (includeSeen) {
        j["last_seen_at"] = p.lastSeenAt;
    }
    return j;
}

Json identitiesJson(const std::vector<LinkedIdentity> &ids)
{
    Json arr = Json::array();
    for (size_t i = 0; i < ids.size(); ++i) {
        Json j;
        j["provider"] = ids[i].provider;
        j["provider_email"] = ids[i].providerEmail;
        j["display_hint"] = ids[i].displayHint;
        j["linked_at"] = ids[i].linkedAt;
        j["last_login_at"] = ids[i].lastLoginAt;
        arr.push_back(j);
    }
    return arr;
}

Json configJson(const GameTableConfig &c)
{
    Json j;
    j["mode"] = modeName(c.mode);
    j["bases"] = static_cast<int>(c.bases);
    j["super_prongs"] = c.useSuperProngs;
    j["edgeless"] = c.useEdgelessBoard;
    Json tc;
    tc["enabled"] = c.timeControl.enabled;
    tc["initial_ms"] = c.timeControl.initialMs;
    tc["increment_ms"] = c.timeControl.incrementMs;
    tc["disconnect_grace_ms"] = c.timeControl.disconnectGraceMs;
    j["time_control"] = tc;
    j["rated"] = c.rated;
    j["allow_takeback"] = c.allowTakeback;
    j["observer_policy"] = toString(c.observerPolicy);
    j["voice_policy"] = toString(c.voicePolicy);
    return j;
}

Json tableJson(const TableSummary &t)
{
    Json j = configJson(t.config);
    j["room_id"] = t.roomId;
    j["phase"] = toString(t.phase);
    j["has_passcode"] = t.hasPasscode;
    j["host_handle"] = t.hostHandle;
    j["seats_total"] = t.seatsTotal;
    j["seats_filled"] = t.seatsFilled;
    j["players"] = t.playerHandles;
    j["observer_count"] = t.observerCount;
    return j;
}

/* ------------------------------------------------------------------ */
/* Request parsing                                                     */
/* ------------------------------------------------------------------ */

bool isJsonContentType(const std::string &contentType)
{
    std::string ct = lower(contentType);
    const std::string want = "application/json";
    if (ct.compare(0, want.size(), want) != 0) {
        return false;
    }
    return ct.size() == want.size() || ct[want.size()] == ';' ||
           ct[want.size()] == ' ';
}

bool parseTimeControl(const Json &b, TimeControlConfig &tc, std::string &err)
{
    tc = TimeControlConfig();
    Json::const_iterator it = b.find("time_control");
    if (it == b.end() || it->is_null()) {
        return true;
    }
    if (it->is_string()) {
        std::string s = it->get<std::string>();
        if (s.empty() || s == "untimed") {
            return true;
        }
        size_t plus = s.find('+');
        int64_t base = 0;
        int64_t inc = 0;
        if (plus == std::string::npos ||
            !parseUnsignedStrict(s.substr(0, plus), 14400, base) ||
            !parseUnsignedStrict(s.substr(plus + 1), 14400, inc) || base < 1) {
            err = "time_control must be 'untimed', '<seconds>+<increment "
                  "seconds>' or an object";
            return false;
        }
        tc.enabled = true;
        tc.initialMs = base * 1000;
        tc.incrementMs = inc * 1000;
        return true;
    }
    if (it->is_object()) {
        int64_t initial = 0;
        int64_t increment = 0;
        int64_t grace = tc.disconnectGraceMs;
        if (!fieldInt(*it, "initial_ms", true, initial, err) ||
            !fieldInt(*it, "increment_ms", false, increment, err) ||
            !fieldInt(*it, "disconnect_grace_ms", false, grace, err)) {
            return false;
        }
        tc.enabled = true;
        tc.initialMs = initial;
        tc.incrementMs = increment;
        tc.disconnectGraceMs = grace;
        return true;
    }
    err = "time_control must be a string or an object";
    return false;
}

bool parseTableConfig(const Json &b, GameTableConfig &cfg, std::string &err)
{
    cfg = GameTableConfig();
    std::string modeText;
    if (!fieldString(b, "mode", false, modeText, err)) {
        return false;
    }
    if (!modeText.empty() && !parseModeName(modeText, cfg.mode)) {
        err = "mode must be one of 2P, 4P, OFK";
        return false;
    }
    int64_t bases = static_cast<int64_t>(cfg.bases);
    if (!fieldInt(b, "bases", false, bases, err)) {
        return false;
    }
    if (bases != 1 && bases != 3) {
        err = "bases must be 1 or 3";
        return false;
    }
    cfg.bases = bases == 1 ? GAME_BASES_1 : GAME_BASES_3;
    if (!fieldBool(b, "super_prongs", false, cfg.useSuperProngs, err) ||
        !fieldBool(b, "edgeless", false, cfg.useEdgelessBoard, err) ||
        !fieldBool(b, "rated", false, cfg.rated, err) ||
        !fieldBool(b, "allow_takeback", false, cfg.allowTakeback, err) ||
        !parseTimeControl(b, cfg.timeControl, err)) {
        return false;
    }
    std::string text;
    if (!fieldString(b, "observer_policy", false, text, err)) {
        return false;
    }
    if (!text.empty() && !parseObserverPolicy(text, cfg.observerPolicy)) {
        err = "observer_policy must be OPEN, SILENT_OBSERVERS or NO_OBSERVERS";
        return false;
    }
    text.clear();
    if (!fieldString(b, "voice_policy", false, text, err)) {
        return false;
    }
    if (!text.empty() && !parseVoicePolicy(text, cfg.voicePolicy)) {
        err = "voice_policy must be DISABLED, PLAYERS_ONLY or ALL_ALLOWED";
        return false;
    }
    return fieldString(b, "passcode", false, cfg.passcode, err);
}

/* ------------------------------------------------------------------ */
/* Event streams                                                       */
/* ------------------------------------------------------------------ */

struct Frame {
    int64_t seq = 0;
    std::string type;
    std::string json;           // compact single-line JSON
};

struct PollBatch {
    std::vector<Frame> frames;
    int64_t latestSeq = 0;
    int64_t nextSeq = 0;        // cursor to resume from
    bool gap = false;
    bool hasMore = false;
    std::string errorCode;
    std::string message;
};

typedef std::function<int(int64_t since, size_t limit, PollBatch &out)>
    PollFn;

/* Holds one SSE slot for as long as the response (and its copies) live. */
struct SlotGuard {
    explicit SlotGuard(std::atomic<int> &counter) : _counter(counter) {}
    ~SlotGuard() { --_counter; }
    SlotGuard(const SlotGuard &) = delete;
    SlotGuard &operator=(const SlotGuard &) = delete;
    std::atomic<int> &_counter;
};

struct StreamState {
    std::shared_ptr<SlotGuard> slot;
    PollFn fetch;
    int64_t cursor = 0;
    std::string token;          // empty for anonymous streams
    int64_t playerId = 0;
    int64_t startedMs = 0;
    int64_t lastWriteMs = 0;
    int64_t lastAuthMs = 0;
    bool first = true;
    bool gapSent = false;
};

std::string sseFrame(const Frame &f)
{
    return "id: " + std::to_string(f.seq) + "\nevent: " + f.type +
           "\ndata: " + f.json + "\n\n";
}

bool putText(httplib::DataSink &sink, const std::string &text)
{
    return sink.write(text.data(), text.size());
}

} // namespace

/* ------------------------------------------------------------------ */
/* Implementation                                                      */
/* ------------------------------------------------------------------ */

struct OctiHttpServer::Impl {
    struct Auth {
        int64_t playerId = 0;
        std::string token;
    };

    Impl(AccountStore &s, OidcVerifier &v, GameRoomManager &r,
         const HttpServerConfig &c);

    /* lifecycle */
    bool start(std::string &error);
    void stop();
    void tickLoop();
    void registerRoutes();

    /* identity cache */
    std::string uuidOf(int64_t playerId);
    bool playerIdOfUuid(const std::string &uuid, int64_t &playerId);
    bool resolveTarget(const Json &body, const char *key, Response &res,
                       int64_t &playerId);

    /* auth + parsing */
    static bool extractBearer(const Request &req, std::string &token);
    bool requireAuth(const Request &req, Response &res, Auth &auth);
    bool optionalAuth(const Request &req, Response &res, Auth &auth);
    bool readJsonBody(const Request &req, Response &res, Json &out,
                      bool allowEmpty);
    bool pathId(const Request &req, const char *name, Response &res,
                std::string &out);
    bool verifyCredential(const Json &body, Response &res,
                          VerifiedClaims &claims);
    bool allowCredentialAttempt(const Request &req, Response &res);
    bool allowUploadAttempt(const Request &req, Response &res);
    bool allowExportAttempt(const Request &req, Response &res);

    /* json views needing the cache */
    Json seatJson(const SeatView &s);
    Json roomViewJson(const RoomView &v);
    Json roomEventJson(const RoomEvent &e);
    Json lobbyEventJson(const LobbyEvent &e);
    Json gameSummaryJson(const GameRecord &g);

    /* streaming */
    bool wantsStream(const Request &req) const;
    void respondEvents(const Request &req, Response &res, const Auth &auth,
                       PollFn fetch);
    bool streamStep(StreamState &st, httplib::DataSink &sink);
    void waitWake(int ms);

    /* handlers */
    void handleHealth(const Request &req, Response &res);
    void handleProviders(const Request &req, Response &res);
    void handleSso(const Request &req, Response &res);
    void handleLink(const Request &req, Response &res);
    void handleUnlink(const Request &req, Response &res);
    void handleLogout(const Request &req, Response &res);
    void handleProfileGet(const Request &req, Response &res);
    void handleProfilePatch(const Request &req, Response &res);
    void handlePlayer(const Request &req, Response &res);
    void handleLeaderboard(const Request &req, Response &res);
    void handleLobbyState(const Request &req, Response &res);
    void handleLobbyChat(const Request &req, Response &res);
    void handleLobbyEvents(const Request &req, Response &res);
    void handleTables(const Request &req, Response &res);
    void handleTableCreate(const Request &req, Response &res);
    void handleTableJoin(const Request &req, Response &res);
    void handleTableLeave(const Request &req, Response &res);
    void handleTableKick(const Request &req, Response &res);
    void handleTableObserve(const Request &req, Response &res);
    void handleTableUnobserve(const Request &req, Response &res);
    void handleEnqueue(const Request &req, Response &res);
    void handleDequeue(const Request &req, Response &res);
    void handleMatchStatus(const Request &req, Response &res);
    void handleGameGet(const Request &req, Response &res);
    void handleMove(const Request &req, Response &res);
    void handleResign(const Request &req, Response &res);
    void handleAction(const Request &req, Response &res);
    void handleChat(const Request &req, Response &res);
    void handleVoiceSignal(const Request &req, Response &res);
    void handleVoiceMeta(const Request &req, Response &res);
    void handleBan(const Request &req, Response &res);
    void handleMute(const Request &req, Response &res);
    void handleSettings(const Request &req, Response &res);
    void handleGameEvents(const Request &req, Response &res);
    void handleArchiveUpload(const Request &req, Response &res);
    void handleArchiveDeleteInstall(const Request &req, Response &res);
    void handleArchiveList(const Request &req, Response &res);
    void handleArchiveGet(const Request &req, Response &res);
    void handleArchiveTranscript(const Request &req, Response &res);
    void handleArchiveOcti(const Request &req, Response &res);
    void handleArchivePlies(const Request &req, Response &res);
    void handleArchiveStats(const Request &req, Response &res);
    void handleArchiveExport(const Request &req, Response &res);
    void handleTelemetryCrash(const Request &req, Response &res);
    void handleTelemetryCrashes(const Request &req, Response &res);
    void handleTelemetryCrashGet(const Request &req, Response &res);
    bool pathCrashId(const Request &req, Response &res, std::string &out);
    void handleIndex(const Request &req, Response &res);

    void finishRoom(Response &res, const RoomResult &r, Json body);

    AccountStore &store;
    OidcVerifier &verifier;
    GameRoomManager &rooms;
    HttpServerConfig cfg;
    httplib::Server server;

    std::thread listenThread;
    std::thread tickThread;
    std::mutex lifecycleMutex;
    bool started = false;
    bool stopped = false;
    std::atomic<bool> stopping;
    std::atomic<int> boundPort;
    std::atomic<uint64_t> ticks;
    std::atomic<int> sseActive;
    int64_t startedAtSec = 0;

    std::mutex wakeMutex;
    std::condition_variable wakeCv;

    struct RateWindow {
        int64_t startMs = 0;
        int count = 0;
    };
    std::mutex rateMutex;
    std::map<std::string, RateWindow> credentialAttempts;
    std::map<std::string, RateWindow> uploadAttempts;
    std::map<std::string, RateWindow> exportAttempts;

    std::mutex cacheMutex;
    std::map<int64_t, std::string> uuidById;
    std::map<std::string, int64_t> idByUuid;
};

OctiHttpServer::Impl::Impl(AccountStore &s, OidcVerifier &v,
                           GameRoomManager &r, const HttpServerConfig &c)
    : store(s), verifier(v), rooms(r), cfg(c), stopping(false), boundPort(0),
      ticks(0), sseActive(0)
{
    if (cfg.maxWorkerThreads < 4) {
        cfg.maxWorkerThreads = 4;
    }
    if (cfg.workerThreads < 2) {
        cfg.workerThreads = 2;
    }
    if (cfg.workerThreads > cfg.maxWorkerThreads) {
        cfg.workerThreads = cfg.maxWorkerThreads;
    }
    int sseCeiling = static_cast<int>(cfg.maxWorkerThreads / 2);
    if (cfg.maxSseStreams > sseCeiling) {
        cfg.maxSseStreams = sseCeiling;
    }
    if (cfg.maxSseStreams < 0) {
        cfg.maxSseStreams = 0;
    }
    if (cfg.ssePollMs < 10) {
        cfg.ssePollMs = 10;
    }
    if (cfg.tickIntervalMs < 10) {
        cfg.tickIntervalMs = 10;
    }
    if (cfg.sseBatchLimit < 1) {
        cfg.sseBatchLimit = 1;
    }
    if (cfg.sseBatchLimit > kMaxQueryLimit) {
        cfg.sseBatchLimit = kMaxQueryLimit;
    }
}

/* ------------------------------------------------------------------ */
/* Lifecycle                                                           */
/* ------------------------------------------------------------------ */

bool OctiHttpServer::Impl::start(std::string &error)
{
    std::lock_guard<std::mutex> lock(lifecycleMutex);
    if (started || stopped) {
        error = "server already started or stopped";
        return false;
    }
    if (cfg.port < 0 || cfg.port > 65535) {
        error = "port out of range";
        return false;
    }
    if (cfg.bindAddress.empty()) {
        error = "empty bind address";
        return false;
    }

    /*
     * httplib defaults to SO_REUSEPORT on Linux, which lets a second
     * process bind the very same port and silently share (or steal) the
     * traffic. SO_REUSEADDR alone still allows a prompt restart.
     */
    server.set_socket_options([](socket_t sock) {
        httplib::set_socket_opt(sock, SOL_SOCKET, SO_REUSEADDR, 1);
    });
    server.set_payload_max_length(cfg.maxBodyBytes);
    server.set_read_timeout(cfg.readTimeoutSec, 0);
    server.set_write_timeout(cfg.writeTimeoutSec, 0);
    const size_t baseThreads = cfg.workerThreads;
    const size_t maxThreads = cfg.maxWorkerThreads;
    const size_t maxQueued = cfg.maxQueuedRequests;
    server.new_task_queue = [baseThreads, maxThreads, maxQueued] {
        return new httplib::ThreadPool(baseThreads, maxThreads, maxQueued);
    };
    registerRoutes();

    int bound = 0;
    errno = 0;
    if (cfg.port == 0) {
        bound = server.bind_to_any_port(cfg.bindAddress);
    } else if (server.bind_to_port(cfg.bindAddress, cfg.port)) {
        bound = cfg.port;
    }
    if (bound <= 0) {
        error = "cannot bind " + cfg.bindAddress + ":" +
                std::to_string(cfg.port) +
                (errno != 0 ? std::string(" (") + std::strerror(errno) + ")"
                            : std::string());
        return false;
    }
    boundPort = bound;
    startedAtSec = epochSec();
    stopping = false;

    listenThread = std::thread([this]() { server.listen_after_bind(); });
    server.wait_until_ready();
    if (!server.is_running()) {
        if (listenThread.joinable()) {
            listenThread.join();
        }
        boundPort = 0;
        error = "listener failed to start";
        return false;
    }
    tickThread = std::thread([this]() { tickLoop(); });
    started = true;
    return true;
}

void OctiHttpServer::Impl::stop()
{
    std::lock_guard<std::mutex> lock(lifecycleMutex);
    if (!started || stopped) {
        return;
    }
    stopped = true;
    {
        std::lock_guard<std::mutex> wake(wakeMutex);
        stopping = true;
    }
    wakeCv.notify_all();
    server.stop();
    if (listenThread.joinable()) {
        listenThread.join();
    }
    if (tickThread.joinable()) {
        tickThread.join();
    }
}

void OctiHttpServer::Impl::waitWake(int ms)
{
    std::unique_lock<std::mutex> lock(wakeMutex);
    wakeCv.wait_for(lock, std::chrono::milliseconds(ms),
                    [this]() { return stopping.load(); });
}

void OctiHttpServer::Impl::tickLoop()
{
    int64_t lastPurge = 0;
    while (!stopping.load()) {
        waitWake(cfg.tickIntervalMs);
        if (stopping.load()) {
            break;
        }
        try {
            rooms.tick();
            int64_t now = epochSec();
            if (now - lastPurge >= cfg.sessionPurgeIntervalSec) {
                store.purgeExpiredSessions(now);
                lastPurge = now;
            }
        } catch (const std::exception &ex) {
            std::cerr << "octi_server: maintenance tick failed: " << ex.what()
                      << "\n";
        }
        ++ticks;
    }
}

/* ------------------------------------------------------------------ */
/* Identity cache                                                      */
/* ------------------------------------------------------------------ */

std::string OctiHttpServer::Impl::uuidOf(int64_t playerId)
{
    if (playerId <= 0) {
        return std::string();
    }
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        std::map<int64_t, std::string>::const_iterator it =
            uuidById.find(playerId);
        if (it != uuidById.end()) {
            return it->second;
        }
    }
    PlayerProfile p;
    if (!store.getProfileById(playerId, p)) {
        return std::string();
    }
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (uuidById.size() >= kMaxCacheEntries) {
        uuidById.clear();
        idByUuid.clear();
    }
    uuidById[playerId] = p.uuid;
    idByUuid[p.uuid] = playerId;
    return p.uuid;
}

bool OctiHttpServer::Impl::playerIdOfUuid(const std::string &uuid,
                                          int64_t &playerId)
{
    if (!isSafeId(uuid)) {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        std::map<std::string, int64_t>::const_iterator it =
            idByUuid.find(uuid);
        if (it != idByUuid.end()) {
            playerId = it->second;
            return true;
        }
    }
    PlayerProfile p;
    if (!store.getProfileByUuid(uuid, p)) {
        return false;
    }
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (uuidById.size() >= kMaxCacheEntries) {
        uuidById.clear();
        idByUuid.clear();
    }
    uuidById[p.playerId] = p.uuid;
    idByUuid[p.uuid] = p.playerId;
    playerId = p.playerId;
    return true;
}

/* Reads body[key] (a player uuid) and resolves it; writes 400/404 on error. */
bool OctiHttpServer::Impl::resolveTarget(const Json &body, const char *key,
                                         Response &res, int64_t &playerId)
{
    std::string uuid;
    std::string err;
    if (!fieldString(body, key, true, uuid, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return false;
    }
    if (!playerIdOfUuid(uuid, playerId)) {
        sendError(res, 404, "UNKNOWN_TARGET", "no such player");
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* Auth and request parsing                                            */
/* ------------------------------------------------------------------ */

bool OctiHttpServer::Impl::extractBearer(const Request &req,
                                         std::string &token)
{
    if (!req.has_header("Authorization")) {
        return false;
    }
    std::string h = req.get_header_value("Authorization");
    const std::string scheme = "bearer ";
    if (h.size() <= scheme.size() ||
        lower(h.substr(0, scheme.size())) != scheme) {
        return false;
    }
    size_t b = scheme.size();
    while (b < h.size() && h[b] == ' ') {
        ++b;
    }
    std::string t = h.substr(b);
    if (t.empty() || t.size() > kMaxBearerBytes) {
        return false;
    }
    for (size_t i = 0; i < t.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(t[i]);
        if (c <= 0x20 || c >= 0x7F) {
            return false;
        }
    }
    token = t;
    return true;
}

bool OctiHttpServer::Impl::requireAuth(const Request &req, Response &res,
                                       Auth &auth)
{
    std::string token;
    if (!extractBearer(req, token)) {
        res.set_header("WWW-Authenticate", "Bearer");
        sendError(res, 401, "UNAUTHORIZED",
                  "missing or malformed Authorization: Bearer header");
        return false;
    }
    SessionValidation sv = store.validateSession(token);
    if (!sv.valid) {
        res.set_header("WWW-Authenticate", "Bearer error=\"invalid_token\"");
        sendError(res, 401, "UNAUTHORIZED", "invalid or expired session");
        return false;
    }
    auth.playerId = sv.playerId;
    auth.token = token;
    return true;
}

/* No header: anonymous viewer (player id 0). A bad header is still a 401. */
bool OctiHttpServer::Impl::optionalAuth(const Request &req, Response &res,
                                        Auth &auth)
{
    if (!req.has_header("Authorization")) {
        auth = Auth();
        return true;
    }
    return requireAuth(req, res, auth);
}

bool OctiHttpServer::Impl::readJsonBody(const Request &req, Response &res,
                                        Json &out, bool allowEmpty)
{
    if (req.body.empty()) {
        if (allowEmpty) {
            out = Json::object();
            return true;
        }
        sendError(res, 400, "MALFORMED_JSON", "a JSON request body is required");
        return false;
    }
    if (!isJsonContentType(req.get_header_value("Content-Type"))) {
        sendError(res, 415, "UNSUPPORTED_MEDIA_TYPE",
                  "Content-Type must be application/json");
        return false;
    }
    out = Json::parse(req.body, nullptr, false);
    if (out.is_discarded() || !out.is_object()) {
        sendError(res, 400, "MALFORMED_JSON",
                  "request body is not a valid JSON object");
        return false;
    }
    return true;
}

bool OctiHttpServer::Impl::pathId(const Request &req, const char *name,
                                  Response &res, std::string &out)
{
    std::unordered_map<std::string, std::string>::const_iterator it =
        req.path_params.find(name);
    if (it == req.path_params.end() || !isSafeId(it->second)) {
        sendError(res, 404, "NOT_FOUND", "no such resource");
        return false;
    }
    out = it->second;
    return true;
}

/* Verifies the SSO credential in body; on failure the response is written. */
bool OctiHttpServer::Impl::verifyCredential(const Json &body, Response &res,
                                            VerifiedClaims &claims)
{
    std::string provider;
    std::string idToken;
    std::string accessToken;
    std::string err;
    if (!fieldString(body, "provider", true, provider, err) ||
        !fieldString(body, "id_token", false, idToken, err) ||
        !fieldString(body, "access_token", false, accessToken, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return false;
    }
    if (!OidcVerifier::isSupportedProvider(provider)) {
        sendError(res, 400, "UNSUPPORTED_PROVIDER", "unsupported provider");
        return false;
    }
    if (!idToken.empty() && !accessToken.empty()) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "send either id_token or access_token, not both");
        return false;
    }
    VerifyResult vr;
    if (!accessToken.empty()) {
        if (provider != "facebook") {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "access_token is only accepted for facebook");
            return false;
        }
        vr = verifier.verifyFacebookAccessToken(accessToken);
    } else if (!idToken.empty()) {
        vr = verifier.verifyIdToken(provider, idToken);
    } else {
        sendError(res, 400, "INVALID_ARGUMENT", "missing field 'id_token'");
        return false;
    }
    if (!vr.ok()) {
        sendError(res, statusForVerify(vr.status),
                  verifyStatusName(vr.status), vr.detail);
        return false;
    }
    claims = vr.claims;
    return true;
}

/* ------------------------------------------------------------------ */
/* JSON views needing the identity cache                               */
/* ------------------------------------------------------------------ */

Json OctiHttpServer::Impl::seatJson(const SeatView &s)
{
    Json j;
    j["seat"] = s.seat;
    j["player_uuid"] = s.playerId != 0 ? Json(uuidOf(s.playerId)) : Json();
    j["handle"] = s.handle;
    j["rating"] = s.rating;
    j["connected"] = s.connected;
    j["disconnected_for_ms"] = s.disconnectedForMs;
    j["forfeit_claimable"] = s.forfeitClaimable;
    j["clock_ms"] = s.clockMs;
    j["eliminated"] = s.eliminated;
    return j;
}

Json OctiHttpServer::Impl::roomViewJson(const RoomView &v)
{
    Json j = configJson(v.config);
    j["live"] = true;
    j["room_id"] = v.roomId;
    j["phase"] = toString(v.phase);
    j["paused"] = v.paused;
    j["has_passcode"] = v.hasPasscode;
    j["host_uuid"] = uuidOf(v.hostPlayerId);
    j["viewer_seat"] = v.viewerSeat;
    Json seats = Json::array();
    for (size_t i = 0; i < v.seats.size(); ++i) {
        seats.push_back(seatJson(v.seats[i]));
    }
    j["seats"] = seats;
    Json obs = Json::array();
    for (size_t i = 0; i < v.observerIds.size(); ++i) {
        obs.push_back(uuidOf(v.observerIds[i]));
    }
    j["observers"] = obs;
    j["observer_count"] = static_cast<int>(v.observerIds.size());
    j["fen"] = v.fen;
    j["turn_seat"] = v.turnSeat;
    j["ply_count"] = v.plyCount;
    j["moves"] = v.moves;
    j["result"] = v.result;
    j["termination"] = v.termination;
    j["winner_seat"] = v.winnerSeat;
    Json props = Json::array();
    for (size_t i = 0; i < v.proposals.size(); ++i) {
        const ProposalView &p = v.proposals[i];
        Json pj;
        pj["type"] = toString(p.type);
        pj["proposer_uuid"] = uuidOf(p.proposerPlayerId);
        Json acc = Json::array();
        for (size_t k = 0; k < p.acceptedBy.size(); ++k) {
            acc.push_back(uuidOf(p.acceptedBy[k]));
        }
        pj["accepted_by"] = acc;
        Json wait = Json::array();
        for (size_t k = 0; k < p.awaiting.size(); ++k) {
            wait.push_back(uuidOf(p.awaiting[k]));
        }
        pj["awaiting"] = wait;
        pj["expires_at_ms"] = p.expiresAtMs;
        props.push_back(pj);
    }
    j["proposals"] = props;
    Json peers = Json::array();
    for (size_t i = 0; i < v.voicePeers.size(); ++i) {
        Json pj;
        pj["player_uuid"] = uuidOf(v.voicePeers[i].playerId);
        pj["mic_muted"] = v.voicePeers[i].micMuted;
        peers.push_back(pj);
    }
    j["voice_peers"] = peers;
    j["seq"] = v.latestSeq;
    j["archived"] = v.archived;
    j["rematch_room_id"] = v.rematchRoomId;
    j["created_at_ms"] = v.createdAtMs;
    return j;
}

Json OctiHttpServer::Impl::roomEventJson(const RoomEvent &e)
{
    Json j;
    j["seq"] = e.seq;
    j["type"] = toString(e.type);
    j["at_ms"] = e.atMs;
    if (e.playerId != 0) {
        j["player_uuid"] = uuidOf(e.playerId);
        j["handle"] = e.handle;
    }
    if (e.targetPlayerId != 0) {
        j["target_uuid"] = uuidOf(e.targetPlayerId);
    }
    j["seat"] = e.seat;
    switch (e.type) {
    case RoomEventType::MOVE:
        j["ply"] = e.ply;
        j["move"] = e.text;
        j["clock_ms"] = e.clockMs;
        j["elapsed_ms"] = e.elapsedMs;
        break;
    case RoomEventType::CHAT:
        j["scope"] = toString(e.scope);
        j["text"] = e.text;
        break;
    case RoomEventType::VOICE_SIGNAL: {
        j["signal_type"] = e.detail;
        Json payload = Json::parse(e.text, nullptr, false);
        if (!payload.is_discarded()) {
            j["payload"] = payload;
        }
        break;
    }
    case RoomEventType::VOICE_META:
        j["action"] = e.detail;
        break;
    case RoomEventType::OBSERVER_BANNED:
        j["reason"] = e.text;
        break;
    case RoomEventType::USER_MUTED:
        j["mute_chat"] = e.detail.find("mute_chat=1") != std::string::npos;
        j["mute_voice"] = e.detail.find("mute_voice=1") != std::string::npos;
        break;
    case RoomEventType::POLICY_UPDATED: {
        size_t sep = e.detail.find(';');
        const std::string op = "observer_policy=";
        const std::string vp = "voice_policy=";
        if (sep != std::string::npos && e.detail.compare(0, op.size(), op) == 0 &&
            e.detail.compare(sep + 1, vp.size(), vp) == 0) {
            j["observer_policy"] = e.detail.substr(op.size(), sep - op.size());
            j["voice_policy"] = e.detail.substr(sep + 1 + vp.size());
        }
        break;
    }
    case RoomEventType::PROPOSAL_OFFERED:
    case RoomEventType::PROPOSAL_ACCEPTED:
    case RoomEventType::PROPOSAL_DECLINED:
    case RoomEventType::PROPOSAL_EXPIRED:
        j["proposal"] = e.detail;
        if (!e.text.empty()) {
            j["note"] = e.text;
        }
        break;
    case RoomEventType::UNDO_APPLIED:
        j["ply"] = e.ply;
        j["move"] = e.text;
        break;
    case RoomEventType::REMATCH_CREATED:
        j["room_id"] = e.text;
        break;
    case RoomEventType::GAME_OVER:
        j["termination"] = e.detail;
        j["result"] = e.text;
        j["winner_seat"] = e.seat;
        break;
    default:
        if (!e.detail.empty()) {
            j["detail"] = e.detail;
        }
        if (!e.text.empty()) {
            j["text"] = e.text;
        }
        break;
    }
    return j;
}

Json OctiHttpServer::Impl::lobbyEventJson(const LobbyEvent &e)
{
    Json j;
    j["seq"] = e.seq;
    j["type"] = toString(e.type);
    j["at_ms"] = e.atMs;
    if (e.playerId != 0) {
        j["player_uuid"] = uuidOf(e.playerId);
        j["handle"] = e.handle;
    }
    switch (e.type) {
    case LobbyEventType::LOBBY_CHAT:
        j["text"] = e.text;
        break;
    case LobbyEventType::PRESENCE:
        j["status"] = toString(e.status);
        j["room_id"] = e.roomId;
        break;
    default:
        j["room_id"] = e.roomId;
        break;
    }
    return j;
}

Json OctiHttpServer::Impl::gameSummaryJson(const GameRecord &g)
{
    Json j;
    j["game_id"] = g.gameId;
    j["mode"] = g.gameMode;
    j["bases"] = g.gameBases;
    j["super_prongs"] = g.superProngs;
    j["edgeless"] = g.edgeless;
    j["time_control"] = g.timeControl;
    Json players = Json::array();
    const int64_t ids[4] = {g.p1PlayerId, g.p2PlayerId, g.p3PlayerId,
                            g.p4PlayerId};
    const std::string *handles[4] = {&g.p1Handle, &g.p2Handle, &g.p3Handle,
                                     &g.p4Handle};
    for (int i = 0; i < 4; ++i) {
        if (handles[i]->empty()) {
            continue;
        }
        Json p;
        p["seat"] = i + 1;
        p["handle"] = *handles[i];
        if (ids[i] != 0) {
            p["player_uuid"] = uuidOf(ids[i]);
        }
        players.push_back(p);
    }
    j["players"] = players;
    j["result"] = g.result;
    j["winner_seat"] = g.winnerSeat;
    j["termination"] = g.termination;
    j["ply_count"] = g.plyCount;
    j["started_at"] = g.startedAt;
    j["ended_at"] = g.endedAt;
    j["source"] = g.source;
    j["uploaded_at"] = g.uploadedAt;
    j["winner"] = g.winner;
    j["status"] = g.status;
    j["ai_difficulty"] = g.aiDifficulty;
    j["app_version"] = g.appVersion;
    j["platform"] = g.platform;
    return j;
}

/* ------------------------------------------------------------------ */
/* Event streaming (JSON poll and SSE)                                 */
/* ------------------------------------------------------------------ */

bool OctiHttpServer::Impl::wantsStream(const Request &req) const
{
    if (req.has_param("stream")) {
        std::string v = lower(req.get_param_value("stream"));
        if (v == "1" || v == "true") {
            return true;
        }
    }
    return lower(req.get_header_value("Accept")).find("text/event-stream") !=
           std::string::npos;
}

bool OctiHttpServer::Impl::streamStep(StreamState &st,
                                      httplib::DataSink &sink)
{
    const int64_t now = steadyMs();
    if (stopping.load()) {
        putText(sink, "event: reconnect\ndata: {}\n\n");
        sink.done();
        return true;
    }
    if (st.first) {
        st.first = false;
        st.startedMs = now;
        st.lastWriteMs = now;
        st.lastAuthMs = now;
        if (!putText(sink, "retry: 3000\n\n")) {
            return false;
        }
    }
    if (now - st.startedMs >= cfg.sseMaxDurationMs) {
        putText(sink, "event: reconnect\ndata: {}\n\n");
        sink.done();
        return true;
    }
    if (st.playerId != 0 && now - st.lastAuthMs >= cfg.sseSessionRecheckMs) {
        st.lastAuthMs = now;
        SessionValidation sv = store.validateSession(st.token);
        if (!sv.valid || sv.playerId != st.playerId) {
            putText(sink, "event: closed\ndata: {\"error\":\"UNAUTHORIZED\"}\n\n");
            sink.done();
            return true;
        }
    }

    PollBatch batch;
    int status = st.fetch(st.cursor, cfg.sseBatchLimit, batch);
    if (status != 200) {
        Json j;
        j["error"] = batch.errorCode;
        j["message"] = batch.message;
        putText(sink, "event: closed\ndata: " + dumpJson(j) + "\n\n");
        sink.done();
        return true;
    }
    if (batch.gap && !st.gapSent) {
        st.gapSent = true;
        if (!putText(sink, "event: gap\ndata: {}\n\n")) {
            return false;
        }
    }
    bool wrote = false;
    for (size_t i = 0; i < batch.frames.size(); ++i) {
        if (!putText(sink, sseFrame(batch.frames[i]))) {
            return false;
        }
        wrote = true;
    }
    st.cursor = std::max(st.cursor, batch.nextSeq);
    if (wrote) {
        st.lastWriteMs = steadyMs();
        return true;
    }
    waitWake(cfg.ssePollMs);
    if (stopping.load()) {
        putText(sink, "event: reconnect\ndata: {}\n\n");
        sink.done();
        return true;
    }
    const int64_t after = steadyMs();
    if (after - st.lastWriteMs >= cfg.sseHeartbeatMs) {
        if (!putText(sink, ": hb\n\n")) {
            return false;
        }
        st.lastWriteMs = after;
    }
    return true;
}

void OctiHttpServer::Impl::respondEvents(const Request &req, Response &res,
                                         const Auth &auth, PollFn fetch)
{
    int64_t since = 0;
    if (req.has_param("since_seq") &&
        !parseUnsignedClamped(req.get_param_value("since_seq"), INT64_MAX / 2,
                              since)) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "since_seq must be a non-negative integer");
        return;
    }
    int64_t limit = 100;
    if (req.has_param("limit") &&
        (!parseUnsignedClamped(req.get_param_value("limit"),
                               static_cast<int64_t>(kMaxQueryLimit), limit) ||
         limit < 1)) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "limit must be a positive integer");
        return;
    }
    const bool stream = wantsStream(req);
    if (stream && req.has_header("Last-Event-ID")) {
        int64_t last = 0;
        if (parseUnsignedClamped(req.get_header_value("Last-Event-ID"),
                                 INT64_MAX / 2, last)) {
            since = std::max(since, last);
        }
    }

    if (!stream) {
        PollBatch batch;
        int status = fetch(since, static_cast<size_t>(limit), batch);
        if (status != 200) {
            sendError(res, status, batch.errorCode, batch.message);
            return;
        }
        Json j;
        Json events = Json::array();
        for (size_t i = 0; i < batch.frames.size(); ++i) {
            events.push_back(Json::parse(batch.frames[i].json));
        }
        j["events"] = events;
        j["latest_seq"] = batch.latestSeq;
        j["next_seq"] = batch.nextSeq;
        j["gap"] = batch.gap;
        j["has_more"] = batch.hasMore;
        sendJson(res, 200, j);
        return;
    }

    /* Validate access once before committing to a stream. */
    {
        PollBatch probe;
        int status = fetch(since, 1, probe);
        if (status != 200) {
            sendError(res, status, probe.errorCode, probe.message);
            return;
        }
    }
    if (sseActive.fetch_add(1) >= cfg.maxSseStreams) {
        --sseActive;
        res.set_header("Retry-After", "5");
        sendError(res, 503, "TOO_MANY_STREAMS",
                  "too many open event streams; use polling");
        return;
    }
    std::shared_ptr<StreamState> st(new StreamState());
    st->slot.reset(new SlotGuard(sseActive));
    st->fetch = fetch;
    st->cursor = since;
    st->token = auth.token;
    st->playerId = auth.playerId;

    res.status = 200;
    res.set_header("Cache-Control", "no-store");
    res.set_header("X-Accel-Buffering", "no");
    res.set_chunked_content_provider(
        "text/event-stream", [this, st](size_t, httplib::DataSink &sink) {
            return streamStep(*st, sink);
        });
}

/* ------------------------------------------------------------------ */
/* Handlers: service, auth, profile                                    */
/* ------------------------------------------------------------------ */

void OctiHttpServer::Impl::handleHealth(const Request &, Response &res)
{
    Json j;
    j["status"] = "ok";
    j["version"] = OctiCore::versionString();
    j["protocol_version"] = cfg.protocolVersion;
    j["min_protocol_version"] = cfg.minProtocolVersion;
    j["uptime_sec"] = std::max<int64_t>(0, epochSec() - startedAtSec);
    j["active_tables"] = static_cast<int64_t>(rooms.roomCount());
    j["online_players"] = static_cast<int64_t>(rooms.onlinePlayerCount());
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleProviders(const Request &, Response &res)
{
    Json arr = Json::array();
    const std::vector<ProviderDescriptor> &reg =
        OidcVerifier::providerRegistry();
    for (size_t i = 0; i < reg.size(); ++i) {
        if (!verifier.isProviderEnabled(reg[i].name)) {
            continue;
        }
        Json p;
        p["name"] = reg[i].name;
        p["display_name"] = reg[i].displayName;
        Json ids = Json::array();
        std::map<std::string, std::vector<std::string> >::const_iterator it =
            cfg.providerClientIds.find(reg[i].name);
        if (it != cfg.providerClientIds.end()) {
            for (size_t k = 0; k < it->second.size(); ++k) {
                ids.push_back(it->second[k]);
            }
        }
        p["client_ids"] = ids;
        arr.push_back(p);
    }
    Json j;
    j["providers"] = arr;
    sendJson(res, 200, j);
}

/*
 * Fixed-window throttle per remote address for the two endpoints that make
 * the server verify a provider credential. The table is bounded: when it is
 * full, expired windows go first, then the oldest one.
 */
bool OctiHttpServer::Impl::allowCredentialAttempt(const Request &req,
                                                  Response &res)
{
    if (cfg.ssoRateLimitMax <= 0 || cfg.ssoRateLimitWindowMs <= 0) {
        return true;
    }
    const size_t kMaxTrackedAddresses = 4096;
    const int64_t now = steadyMs();
    int64_t retryAfterMs = 0;
    {
        std::lock_guard<std::mutex> lock(rateMutex);
        if (credentialAttempts.size() >= kMaxTrackedAddresses &&
            credentialAttempts.find(req.remote_addr) ==
                credentialAttempts.end()) {
            for (std::map<std::string, RateWindow>::iterator it =
                     credentialAttempts.begin();
                 it != credentialAttempts.end();) {
                if (now - it->second.startMs >= cfg.ssoRateLimitWindowMs) {
                    it = credentialAttempts.erase(it);
                } else {
                    ++it;
                }
            }
            if (credentialAttempts.size() >= kMaxTrackedAddresses) {
                std::map<std::string, RateWindow>::iterator oldest =
                    credentialAttempts.begin();
                for (std::map<std::string, RateWindow>::iterator it =
                         credentialAttempts.begin();
                     it != credentialAttempts.end(); ++it) {
                    if (it->second.startMs < oldest->second.startMs) {
                        oldest = it;
                    }
                }
                credentialAttempts.erase(oldest);
            }
        }
        RateWindow &w = credentialAttempts[req.remote_addr];
        if (w.count == 0 || now - w.startMs >= cfg.ssoRateLimitWindowMs) {
            w.startMs = now;
            w.count = 0;
        }
        if (w.count < cfg.ssoRateLimitMax) {
            ++w.count;
            return true;
        }
        retryAfterMs = cfg.ssoRateLimitWindowMs - (now - w.startMs);
    }
    sendError(res, 429, "RATE_LIMITED",
              "too many sign-in attempts; retry later");
    res.set_header("Retry-After",
                   std::to_string(std::max<int64_t>(1, (retryAfterMs + 999) / 1000)));
    return false;
}

bool OctiHttpServer::Impl::allowUploadAttempt(const Request &req, Response &res)
{
    if (cfg.uploadRateLimitMax <= 0 || cfg.uploadRateLimitWindowMs <= 0) {
        return true;
    }
    const size_t kMaxTrackedAddresses = 4096;
    const int64_t now = steadyMs();
    int64_t retryAfterMs = 0;
    {
        std::lock_guard<std::mutex> lock(rateMutex);
        if (uploadAttempts.size() >= kMaxTrackedAddresses &&
            uploadAttempts.find(req.remote_addr) == uploadAttempts.end()) {
            for (auto it = uploadAttempts.begin(); it != uploadAttempts.end();) {
                if (now - it->second.startMs >= cfg.uploadRateLimitWindowMs) {
                    it = uploadAttempts.erase(it);
                } else {
                    ++it;
                }
            }
            if (uploadAttempts.size() >= kMaxTrackedAddresses) {
                auto oldest = uploadAttempts.begin();
                for (auto it = uploadAttempts.begin(); it != uploadAttempts.end(); ++it) {
                    if (it->second.startMs < oldest->second.startMs) {
                        oldest = it;
                    }
                }
                uploadAttempts.erase(oldest);
            }
        }
        RateWindow &w = uploadAttempts[req.remote_addr];
        if (w.count == 0 || now - w.startMs >= cfg.uploadRateLimitWindowMs) {
            w.startMs = now;
            w.count = 0;
        }
        if (w.count < cfg.uploadRateLimitMax) {
            ++w.count;
            return true;
        }
        retryAfterMs = cfg.uploadRateLimitWindowMs - (now - w.startMs);
    }
    sendError(res, 429, "RATE_LIMITED",
              "too many upload attempts; retry later");
    res.set_header("Retry-After",
                   std::to_string(std::max<int64_t>(1, (retryAfterMs + 999) / 1000)));
    return false;
}

bool OctiHttpServer::Impl::allowExportAttempt(const Request &req, Response &res)
{
    if (cfg.exportRateLimitMax <= 0 || cfg.exportRateLimitWindowMs <= 0) {
        return true;
    }
    const size_t kMaxTrackedAddresses = 4096;
    const int64_t now = steadyMs();
    int64_t retryAfterMs = 0;
    {
        std::lock_guard<std::mutex> lock(rateMutex);
        if (exportAttempts.size() >= kMaxTrackedAddresses &&
            exportAttempts.find(req.remote_addr) == exportAttempts.end()) {
            for (auto it = exportAttempts.begin(); it != exportAttempts.end();) {
                if (now - it->second.startMs >= cfg.exportRateLimitWindowMs) {
                    it = exportAttempts.erase(it);
                } else {
                    ++it;
                }
            }
            if (exportAttempts.size() >= kMaxTrackedAddresses) {
                auto oldest = exportAttempts.begin();
                for (auto it = exportAttempts.begin(); it != exportAttempts.end(); ++it) {
                    if (it->second.startMs < oldest->second.startMs) {
                        oldest = it;
                    }
                }
                exportAttempts.erase(oldest);
            }
        }
        RateWindow &w = exportAttempts[req.remote_addr];
        if (w.count == 0 || now - w.startMs >= cfg.exportRateLimitWindowMs) {
            w.startMs = now;
            w.count = 0;
        }
        if (w.count < cfg.exportRateLimitMax) {
            ++w.count;
            return true;
        }
        retryAfterMs = cfg.exportRateLimitWindowMs - (now - w.startMs);
    }
    sendError(res, 429, "RATE_LIMITED",
              "too many export attempts; retry later");
    res.set_header("Retry-After",
                   std::to_string(std::max<int64_t>(1, (retryAfterMs + 999) / 1000)));
    return false;
}

void OctiHttpServer::Impl::handleSso(const Request &req, Response &res)
{
    if (!allowCredentialAttempt(req, res)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    std::string agent;
    std::string err;
    if (!fieldString(body, "client_agent", false, agent, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    VerifiedClaims claims;
    if (!verifyCredential(body, res, claims)) {
        return;
    }
    LoginResult lr = store.loginWithSso(claims, cleanAgent(agent));
    if (!lr.ok) {
        sendError(res, 500, "INTERNAL_ERROR", "sign-in failed");
        return;
    }
    rooms.enterLobby(lr.profile.playerId);
    Json j;
    j["session_token"] = lr.sessionToken;
    j["expires_at"] = lr.sessionExpiresAt;
    j["created"] = lr.created;
    j["profile"] = profileJson(lr.profile, true);
    j["linked_identities"] = identitiesJson(lr.identities);
    sendJson(res, lr.created ? 201 : 200, j);
}

void OctiHttpServer::Impl::handleLink(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth) || !allowCredentialAttempt(req, res)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    VerifiedClaims claims;
    if (!verifyCredential(body, res, claims)) {
        return;
    }
    LinkResult lr = store.linkIdentity(auth.playerId, claims);
    switch (lr.status) {
    case LinkStatus::LINKED:
    case LinkStatus::ALREADY_LINKED_SELF: {
        std::vector<LinkedIdentity> ids;
        store.listLinkedIdentities(auth.playerId, ids);
        Json j;
        j["status"] = lr.status == LinkStatus::LINKED ? "linked"
                                                       : "already_linked";
        j["linked_identities"] = identitiesJson(ids);
        sendJson(res, 200, j);
        return;
    }
    case LinkStatus::CONFLICT_ALREADY_LINKED: {
        Json j;
        j["error"] = "CONFLICT_ALREADY_LINKED";
        j["message"] = "this identity is already linked to another profile";
        Json other;
        other["uuid"] = lr.conflictProfile.uuid;
        other["handle"] = lr.conflictProfile.handle;
        other["elo_2p"] = lr.conflictProfile.elo2p;
        other["games_played"] = lr.conflictProfile.gamesPlayed;
        j["existing_profile"] = other;
        sendJson(res, 409, j);
        return;
    }
    case LinkStatus::PLAYER_NOT_FOUND:
        sendError(res, 401, "UNAUTHORIZED", "unknown player");
        return;
    case LinkStatus::INVALID_CLAIMS:
        sendError(res, 400, "INVALID_ARGUMENT", "invalid identity claims");
        return;
    case LinkStatus::ERROR:
        break;
    }
    sendError(res, 500, "INTERNAL_ERROR", "link failed");
}

void OctiHttpServer::Impl::handleUnlink(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    std::unordered_map<std::string, std::string>::const_iterator it =
        req.path_params.find("provider");
    if (it == req.path_params.end() ||
        !OidcVerifier::isSupportedProvider(it->second)) {
        sendError(res, 400, "UNSUPPORTED_PROVIDER", "unsupported provider");
        return;
    }
    switch (store.unlinkIdentity(auth.playerId, it->second)) {
    case UnlinkStatus::UNLINKED: {
        std::vector<LinkedIdentity> ids;
        store.listLinkedIdentities(auth.playerId, ids);
        Json j;
        j["status"] = "unlinked";
        j["linked_identities"] = identitiesJson(ids);
        sendJson(res, 200, j);
        return;
    }
    case UnlinkStatus::LAST_IDENTITY_FORBIDDEN:
        sendError(res, 400, "LAST_IDENTITY_FORBIDDEN",
                  "cannot unlink the only remaining sign-in identity");
        return;
    case UnlinkStatus::NOT_LINKED:
        sendError(res, 404, "NOT_LINKED", "provider is not linked");
        return;
    case UnlinkStatus::ERROR:
        break;
    }
    sendError(res, 500, "INTERNAL_ERROR", "unlink failed");
}

void OctiHttpServer::Impl::handleLogout(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    store.revokeSession(auth.token);
    /*
     * Only the last signed-in device takes the player out of the lobby. A
     * failed count (-1) keeps the player online rather than dropping a
     * player who may still be playing from another device.
     */
    if (store.countActiveSessions(auth.playerId) == 0) {
        rooms.leaveLobby(auth.playerId);
    }
    Json j;
    j["status"] = "logged_out";
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleProfileGet(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    PlayerProfile p;
    if (!store.getProfileById(auth.playerId, p)) {
        sendError(res, 401, "UNAUTHORIZED", "unknown player");
        return;
    }
    std::vector<LinkedIdentity> ids;
    store.listLinkedIdentities(auth.playerId, ids);
    Json j = profileJson(p, true);
    j["linked_identities"] = identitiesJson(ids);
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleProfilePatch(const Request &req,
                                              Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    ProfileUpdate up;
    std::string err;
    if (fieldPresent(body, "handle")) {
        up.setHandle = true;
        if (!fieldString(body, "handle", true, up.handle, err)) {
            sendError(res, 400, "INVALID_ARGUMENT", err);
            return;
        }
    }
    if (fieldPresent(body, "display_name")) {
        up.setDisplayName = true;
        if (!fieldString(body, "display_name", true, up.displayName, err)) {
            sendError(res, 400, "INVALID_ARGUMENT", err);
            return;
        }
    }
    if (fieldPresent(body, "avatar_url")) {
        up.setAvatarUrl = true;
        if (!fieldString(body, "avatar_url", true, up.avatarUrl, err)) {
            sendError(res, 400, "INVALID_ARGUMENT", err);
            return;
        }
    }
    if (!up.setHandle && !up.setDisplayName && !up.setAvatarUrl) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "nothing to update: send handle, display_name or avatar_url");
        return;
    }
    switch (store.updateProfile(auth.playerId, up)) {
    case UpdateStatus::OK: {
        PlayerProfile p;
        if (!store.getProfileById(auth.playerId, p)) {
            sendError(res, 500, "INTERNAL_ERROR", "profile lookup failed");
            return;
        }
        rooms.enterLobby(auth.playerId);    // refresh the cached handle
        sendJson(res, 200, profileJson(p, true));
        return;
    }
    case UpdateStatus::NOT_FOUND:
        sendError(res, 401, "UNAUTHORIZED", "unknown player");
        return;
    case UpdateStatus::INVALID_HANDLE:
        sendError(res, 400, "INVALID_HANDLE",
                  "handle must be 3 to 24 characters");
        return;
    case UpdateStatus::HANDLE_TAKEN:
        sendError(res, 409, "HANDLE_TAKEN", "handle is already taken");
        return;
    case UpdateStatus::INVALID_DISPLAY_NAME:
        sendError(res, 400, "INVALID_DISPLAY_NAME", "invalid display_name");
        return;
    case UpdateStatus::INVALID_AVATAR_URL:
        sendError(res, 400, "INVALID_AVATAR_URL", "invalid avatar_url");
        return;
    case UpdateStatus::ERROR:
        break;
    }
    sendError(res, 500, "INTERNAL_ERROR", "profile update failed");
}

void OctiHttpServer::Impl::handlePlayer(const Request &req, Response &res)
{
    std::string uuid;
    if (!pathId(req, "uuid", res, uuid)) {
        return;
    }
    PlayerProfile p;
    if (!store.getProfileByUuid(uuid, p)) {
        sendError(res, 404, "NOT_FOUND", "no such player");
        return;
    }
    Json j = profileJson(p, false);
    ArchiveFilter f;
    f.playerUuid = uuid;
    f.limit = 10;
    std::vector<GameRecord> games;
    Json recent = Json::array();
    if (store.listArchivedGames(f, games)) {
        for (size_t i = 0; i < games.size(); ++i) {
            recent.push_back(gameSummaryJson(games[i]));
        }
    }
    j["recent_games"] = recent;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleLeaderboard(const Request &req,
                                             Response &res)
{
    std::string mode = "2P";
    if (req.has_param("mode")) {
        mode = upper(req.get_param_value("mode"));
    }
    OctiGameMode parsed = GAME_MODE_2;
    if (!parseModeName(mode, parsed)) {
        sendError(res, 400, "INVALID_ARGUMENT", "mode must be 2P, 4P or OFK");
        return;
    }
    int64_t limit = 50;
    if (req.has_param("limit") &&
        (!parseUnsignedClamped(req.get_param_value("limit"),
                               AccountStore::kMaxLeaderboardLimit, limit) ||
         limit < 1)) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "limit must be a positive integer");
        return;
    }
    std::vector<LeaderboardEntry> rows;
    if (!store.leaderboard(mode, static_cast<int>(limit), rows)) {
        sendError(res, 500, "INTERNAL_ERROR", "leaderboard query failed");
        return;
    }
    Json arr = Json::array();
    for (size_t i = 0; i < rows.size(); ++i) {
        Json e;
        e["rank"] = rows[i].rank;
        e["uuid"] = rows[i].uuid;
        e["handle"] = rows[i].handle;
        e["display_name"] = rows[i].displayName;
        e["elo"] = rows[i].elo;
        e["games_played"] = rows[i].gamesPlayed;
        e["wins"] = rows[i].wins;
        e["losses"] = rows[i].losses;
        e["draws"] = rows[i].draws;
        arr.push_back(e);
    }
    Json j;
    j["mode"] = mode;
    j["entries"] = arr;
    sendJson(res, 200, j);
}

/* ------------------------------------------------------------------ */
/* Handlers: lobby, tables, matchmaking                                */
/* ------------------------------------------------------------------ */

void OctiHttpServer::Impl::finishRoom(Response &res, const RoomResult &r,
                                      Json body)
{
    if (!r.ok()) {
        sendRoomError(res, r);
        return;
    }
    body["ok"] = true;
    if (r.seq != 0) {
        body["seq"] = r.seq;
    }
    sendJson(res, 200, body);
}

void OctiHttpServer::Impl::handleLobbyState(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    rooms.enterLobby(auth.playerId);
    LobbySnapshot snap = rooms.lobbyState();
    Json j;
    Json players = Json::array();
    for (size_t i = 0; i < snap.players.size(); ++i) {
        Json p;
        p["player_uuid"] = uuidOf(snap.players[i].playerId);
        p["handle"] = snap.players[i].handle;
        p["status"] = toString(snap.players[i].status);
        p["room_id"] = snap.players[i].roomId;
        players.push_back(p);
    }
    j["players"] = players;
    Json tables = Json::array();
    for (size_t i = 0; i < snap.tables.size(); ++i) {
        tables.push_back(tableJson(snap.tables[i]));
    }
    j["tables"] = tables;
    Json chat = Json::array();
    for (size_t i = 0; i < snap.recentChat.size(); ++i) {
        chat.push_back(lobbyEventJson(snap.recentChat[i]));
    }
    j["recent_chat"] = chat;
    j["latest_seq"] = snap.latestSeq;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleLobbyChat(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    std::string text;
    std::string err;
    if (!fieldString(body, "text", true, text, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    finishRoom(res, rooms.postLobbyChat(auth.playerId, text), Json::object());
}

void OctiHttpServer::Impl::handleLobbyEvents(const Request &req,
                                             Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    const int64_t viewer = auth.playerId;
    PollFn fetch = [this, viewer](int64_t since, size_t limit,
                                  PollBatch &out) -> int {
        LobbyPoll poll = rooms.pollLobbyEvents(viewer, since, limit);
        out.latestSeq = poll.latestSeq;
        out.gap = poll.gap;
        out.hasMore = poll.hasMore;
        for (size_t i = 0; i < poll.events.size(); ++i) {
            Frame f;
            f.seq = poll.events[i].seq;
            f.type = toString(poll.events[i].type);
            f.json = dumpJson(lobbyEventJson(poll.events[i]));
            out.frames.push_back(f);
        }
        out.nextSeq = poll.hasMore && !out.frames.empty()
                          ? out.frames.back().seq
                          : std::max(since, poll.latestSeq);
        return 200;
    };
    respondEvents(req, res, auth, fetch);
}

void OctiHttpServer::Impl::handleTables(const Request &req, Response &res)
{
    bool includeFinished = false;
    if (req.has_param("include_finished")) {
        std::string v = lower(req.get_param_value("include_finished"));
        includeFinished = (v == "1" || v == "true");
    }
    std::vector<TableSummary> tables = rooms.listTables(includeFinished);
    Json arr = Json::array();
    for (size_t i = 0; i < tables.size(); ++i) {
        arr.push_back(tableJson(tables[i]));
    }
    Json j;
    j["tables"] = arr;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleTableCreate(const Request &req,
                                             Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, true)) {
        return;
    }
    GameTableConfig cfgIn;
    std::string err;
    if (!parseTableConfig(body, cfgIn, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    RoomResult r = rooms.createTable(auth.playerId, cfgIn);
    if (!r.ok()) {
        sendRoomError(res, r);
        return;
    }
    Json j;
    j["ok"] = true;
    j["room_id"] = r.roomId;
    j["seat"] = r.seat;
    sendJson(res, 201, j);
}

void OctiHttpServer::Impl::handleTableJoin(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, true)) {
        return;
    }
    int64_t seat = 0;
    std::string passcode;
    std::string err;
    if (!fieldInt(body, "seat", false, seat, err) ||
        !fieldString(body, "passcode", false, passcode, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    if (seat < 0 || seat > 4) {
        sendError(res, 400, "INVALID_ARGUMENT", "seat must be 0..4");
        return;
    }
    RoomResult r =
        rooms.joinTable(id, auth.playerId, static_cast<int>(seat), passcode);
    Json j;
    j["room_id"] = r.roomId;
    j["seat"] = r.seat;
    j["game_started"] = r.gameStarted;
    finishRoom(res, r, j);
}

void OctiHttpServer::Impl::handleTableLeave(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    finishRoom(res, rooms.leaveTable(id, auth.playerId), Json::object());
}

void OctiHttpServer::Impl::handleTableKick(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    Json body;
    int64_t target = 0;
    if (!readJsonBody(req, res, body, false) ||
        !resolveTarget(body, "target", res, target)) {
        return;
    }
    finishRoom(res, rooms.kickSeat(id, auth.playerId, target), Json::object());
}

void OctiHttpServer::Impl::handleTableObserve(const Request &req,
                                              Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    RoomResult r = rooms.observeRoom(id, auth.playerId);
    Json j;
    j["room_id"] = r.roomId;
    finishRoom(res, r, j);
}

void OctiHttpServer::Impl::handleTableUnobserve(const Request &req,
                                                Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    finishRoom(res, rooms.stopObserving(id, auth.playerId), Json::object());
}

void OctiHttpServer::Impl::handleEnqueue(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, true)) {
        return;
    }
    GameTableConfig c;
    std::string err;
    if (!parseTableConfig(body, c, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    MatchRuleset rs;
    rs.mode = c.mode;
    rs.bases = c.bases;
    rs.useSuperProngs = c.useSuperProngs;
    rs.useEdgelessBoard = c.useEdgelessBoard;
    rs.initialMs = c.timeControl.enabled ? c.timeControl.initialMs : 0;
    rs.incrementMs = c.timeControl.enabled ? c.timeControl.incrementMs : 0;
    Json j;
    j["queued"] = true;
    finishRoom(res, rooms.enqueueMatchmaking(auth.playerId, rs), j);
}

void OctiHttpServer::Impl::handleDequeue(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    Json j;
    j["queued"] = false;
    finishRoom(res, rooms.dequeueMatchmaking(auth.playerId), j);
}

void OctiHttpServer::Impl::handleMatchStatus(const Request &req,
                                             Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    MatchmakingStatus st = rooms.matchmakingStatus(auth.playerId);
    Json j;
    j["queued"] = st.queued;
    j["waited_ms"] = st.waitedMs;
    j["window"] = st.window;
    j["rating"] = st.rating;
    j["matched_room_id"] = st.matchedRoomId;
    sendJson(res, 200, j);
}

/* ------------------------------------------------------------------ */
/* Handlers: games                                                     */
/* ------------------------------------------------------------------ */

void OctiHttpServer::Impl::handleGameGet(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!pathId(req, "id", res, id) || !optionalAuth(req, res, auth)) {
        return;
    }
    RoomView view;
    RoomResult r = rooms.getRoomView(id, auth.playerId, view);
    if (r.code == RoomCode::NOT_FOUND) {
        GameRecord rec;
        if (store.getArchivedGame(id, rec)) {
            Json j = gameSummaryJson(rec);
            j["live"] = false;
            j["room_id"] = rec.gameId;
            j["phase"] = "FINISHED";
            j["fen"] = rec.finalFen;
            j["archived"] = true;
            sendJson(res, 200, j);
            return;
        }
    }
    if (!r.ok()) {
        sendRoomError(res, r);
        return;
    }
    sendJson(res, 200, roomViewJson(view));
}

void OctiHttpServer::Impl::handleMove(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    Json body;
    std::string move;
    std::string err;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    if (!fieldString(body, "move", true, move, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    RoomResult r = rooms.submitMove(id, auth.playerId, move);
    Json j;
    j["move"] = r.move;
    j["clock_ms"] = r.clockMs;
    j["elapsed_ms"] = r.elapsedMs;
    j["game_over"] = r.gameOver;
    finishRoom(res, r, j);
}

void OctiHttpServer::Impl::handleResign(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    RoomResult r = rooms.resign(id, auth.playerId);
    Json j;
    j["game_over"] = r.gameOver;
    finishRoom(res, r, j);
}

void OctiHttpServer::Impl::handleAction(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    Json body;
    std::string action;
    std::string err;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    if (!fieldString(body, "action", true, action, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    const int64_t pid = auth.playerId;
    RoomResult r;
    if (action == "offer_draw") {
        r = rooms.submitProposal(id, pid, ProposalType::DRAW_OFFER);
    } else if (action == "accept_draw") {
        r = rooms.respondProposal(id, pid, ProposalType::DRAW_OFFER, true);
    } else if (action == "decline_draw") {
        r = rooms.respondProposal(id, pid, ProposalType::DRAW_OFFER, false);
    } else if (action == "request_pause") {
        r = rooms.submitProposal(id, pid, ProposalType::PAUSE_REQUEST);
    } else if (action == "accept_pause") {
        r = rooms.respondProposal(id, pid, ProposalType::PAUSE_REQUEST, true);
    } else if (action == "decline_pause") {
        r = rooms.respondProposal(id, pid, ProposalType::PAUSE_REQUEST, false);
    } else if (action == "resume") {
        r = rooms.resumeGame(id, pid);
    } else if (action == "request_undo") {
        r = rooms.submitProposal(id, pid, ProposalType::UNDO_REQUEST);
    } else if (action == "accept_undo") {
        r = rooms.respondProposal(id, pid, ProposalType::UNDO_REQUEST, true);
    } else if (action == "decline_undo") {
        r = rooms.respondProposal(id, pid, ProposalType::UNDO_REQUEST, false);
    } else if (action == "request_rematch") {
        r = rooms.submitProposal(id, pid, ProposalType::REMATCH_REQUEST);
    } else if (action == "accept_rematch") {
        r = rooms.respondProposal(id, pid, ProposalType::REMATCH_REQUEST, true);
    } else if (action == "decline_rematch") {
        r = rooms.respondProposal(id, pid, ProposalType::REMATCH_REQUEST, false);
    } else {
        sendError(res, 400, "INVALID_ARGUMENT", "unknown action");
        return;
    }
    Json j;
    j["action"] = action;
    j["room_id"] = r.roomId.empty() ? id : r.roomId;
    j["game_over"] = r.gameOver;
    finishRoom(res, r, j);
}

void OctiHttpServer::Impl::handleChat(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    Json body;
    std::string text;
    std::string scopeText = "all";
    std::string err;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    if (!fieldString(body, "text", true, text, err) ||
        !fieldString(body, "scope", false, scopeText, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    ChatScope scope = ChatScope::ALL;
    if (!parseChatScope(scopeText, scope)) {
        sendError(res, 400, "INVALID_ARGUMENT", "scope must be 'all' or 'team'");
        return;
    }
    finishRoom(res, rooms.postBoardChat(id, auth.playerId, scope, text),
               Json::object());
}

void OctiHttpServer::Impl::handleVoiceSignal(const Request &req,
                                             Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    int64_t target = 0;
    if (!resolveTarget(body, "target", res, target)) {
        return;
    }
    std::string typeText;
    std::string err;
    if (!fieldString(body, "type", true, typeText, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    VoiceSignalType type = VoiceSignalType::SDP_OFFER;
    if (!parseVoiceSignalType(typeText, type)) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "type must be SDP_OFFER, SDP_ANSWER or ICE_CANDIDATE");
        return;
    }
    Json::const_iterator payload = body.find("payload");
    if (payload == body.end() || payload->is_null()) {
        sendError(res, 400, "INVALID_ARGUMENT", "missing field 'payload'");
        return;
    }
    /* Signaling only: the payload is an opaque JSON blob, at most 4 KiB. */
    finishRoom(res,
               rooms.submitVoiceSignal(id, auth.playerId, target, type,
                                       payload->dump(-1, ' ', false,
                                                     Json::error_handler_t::replace)),
               Json::object());
}

void OctiHttpServer::Impl::handleVoiceMeta(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    Json body;
    std::string actionText;
    std::string err;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    if (!fieldString(body, "action", true, actionText, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    VoiceMetaAction action = VoiceMetaAction::JOINED_VOICE;
    if (!parseVoiceMetaAction(actionText, action)) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "action must be JOINED_VOICE, LEFT_VOICE, MIC_MUTED or "
                  "MIC_UNMUTED");
        return;
    }
    finishRoom(res, rooms.submitVoiceMeta(id, auth.playerId, action),
               Json::object());
}

void OctiHttpServer::Impl::handleBan(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    Json body;
    int64_t target = 0;
    std::string reason;
    std::string err;
    if (!readJsonBody(req, res, body, false) ||
        !resolveTarget(body, "target", res, target)) {
        return;
    }
    if (!fieldString(body, "reason", false, reason, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    RoomResult r = rooms.banUser(id, auth.playerId, target, reason);
    Json j;
    if (r.ok() && !r.message.empty()) {
        j["note"] = r.message;
    }
    finishRoom(res, r, j);
}

/*
 * Sets the complete mute state of the target: an omitted mute_chat or
 * mute_voice means false, so {"mute_chat":false} also lifts a voice mute.
 */
void OctiHttpServer::Impl::handleMute(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    Json body;
    int64_t target = 0;
    if (!readJsonBody(req, res, body, false) ||
        !resolveTarget(body, "target", res, target)) {
        return;
    }
    if (!fieldPresent(body, "mute_chat") && !fieldPresent(body, "mute_voice")) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "send mute_chat and/or mute_voice");
        return;
    }
    bool muteChat = false;
    bool muteVoice = false;
    std::string err;
    if (!fieldBool(body, "mute_chat", false, muteChat, err) ||
        !fieldBool(body, "mute_voice", false, muteVoice, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    finishRoom(res,
               rooms.muteUser(id, auth.playerId, target, muteChat, muteVoice),
               Json::object());
}

void OctiHttpServer::Impl::handleSettings(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    std::string observerText;
    std::string voiceText;
    std::string err;
    if (!fieldString(body, "observer_policy", false, observerText, err) ||
        !fieldString(body, "voice_policy", false, voiceText, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    if (observerText.empty() && voiceText.empty()) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "send observer_policy and/or voice_policy");
        return;
    }
    std::optional<ObserverPolicy> op;
    std::optional<VoicePolicy> vp;
    if (!observerText.empty()) {
        ObserverPolicy parsed = ObserverPolicy::OPEN;
        if (!parseObserverPolicy(observerText, parsed)) {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "observer_policy must be OPEN, SILENT_OBSERVERS or "
                      "NO_OBSERVERS");
            return;
        }
        op = parsed;
    }
    if (!voiceText.empty()) {
        VoicePolicy parsed = VoicePolicy::DISABLED;
        if (!parseVoicePolicy(voiceText, parsed)) {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "voice_policy must be DISABLED, PLAYERS_ONLY or "
                      "ALL_ALLOWED");
            return;
        }
        vp = parsed;
    }
    /*
     * The manager merges the omitted field with the room's current value
     * under its own lock, so two concurrent requests that each change a
     * different policy cannot overwrite one another.
     */
    RoomResult r = rooms.updateRoomPolicies(id, auth.playerId, op, vp);
    if (!r.ok()) {
        sendRoomError(res, r);
        return;
    }
    RoomView view;
    RoomResult cur = rooms.getRoomView(id, auth.playerId, view);
    if (!cur.ok()) {
        sendRoomError(res, cur);
        return;
    }
    Json j;
    j["observer_policy"] = toString(view.config.observerPolicy);
    j["voice_policy"] = toString(view.config.voicePolicy);
    finishRoom(res, r, j);
}

void OctiHttpServer::Impl::handleGameEvents(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!pathId(req, "id", res, id) || !optionalAuth(req, res, auth)) {
        return;
    }
    const int64_t viewer = auth.playerId;
    PollFn fetch = [this, id, viewer](int64_t since, size_t limit,
                                      PollBatch &out) -> int {
        RoomPoll poll;
        RoomResult r = rooms.pollRoomEvents(id, viewer, since, limit, poll);
        if (!r.ok()) {
            out.errorCode = toString(r.code);
            out.message = r.message;
            return statusFor(r.code);
        }
        out.latestSeq = poll.latestSeq;
        out.gap = poll.gap;
        out.hasMore = poll.hasMore;
        for (size_t i = 0; i < poll.events.size(); ++i) {
            Frame f;
            f.seq = poll.events[i].seq;
            f.type = toString(poll.events[i].type);
            f.json = dumpJson(roomEventJson(poll.events[i]));
            out.frames.push_back(f);
        }
        out.nextSeq = poll.hasMore && !out.frames.empty()
                          ? out.frames.back().seq
                          : std::max(since, poll.latestSeq);
        return 200;
    };
    respondEvents(req, res, auth, fetch);
}

/* ------------------------------------------------------------------ */
/* Handlers: archive and UI                                            */
/* ------------------------------------------------------------------ */

static std::string sanitizeTranscriptJson(const std::string &rawJsonStr, bool pretty = false)
{
    try {
        auto j = nlohmann::json::parse(rawJsonStr);
        if (j.is_object()) {
            if (j.contains("client") && j["client"].is_object()) {
                j["client"].erase("extended");
            }
            j["license"] = "CC-BY-4.0";
            j["attribution"] = "Octi game data, Charles Chiou (selfso.com)";
            return pretty ? j.dump(2) : j.dump();
        }
    } catch (...) {}
    return rawJsonStr;
}

void OctiHttpServer::Impl::handleArchiveUpload(const Request &req, Response &res)
{
    if (!allowUploadAttempt(req, res)) {
        return;
    }
    if (req.body.empty() || req.body.size() > 65536) {
        sendError(res, req.body.size() > 65536 ? 413 : 400,
                  req.body.size() > 65536 ? "PAYLOAD_TOO_LARGE" : "MALFORMED_UPLOAD",
                  "invalid payload size");
        return;
    }
    Json root = Json::parse(req.body, nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        sendError(res, 400, "MALFORMED_UPLOAD", "invalid JSON payload");
        return;
    }

    Json transcriptObj;
    std::string clientId;

    if (root.contains("transcript")) {
        if (root["transcript"].is_string()) {
            std::string tStr = root["transcript"].get<std::string>();
            if (tStr.size() > 49152) {
                sendError(res, 400, "MALFORMED_UPLOAD", "transcript exceeds 48 KiB");
                return;
            }
            transcriptObj = Json::parse(tStr, nullptr, false);
            if (transcriptObj.is_discarded() || !transcriptObj.is_object()) {
                sendError(res, 400, "MALFORMED_UPLOAD", "transcript is not valid JSON");
                return;
            }
        } else if (root["transcript"].is_object()) {
            transcriptObj = root["transcript"];
        } else {
            sendError(res, 400, "MALFORMED_UPLOAD", "transcript must be a string or object");
            return;
        }
        if (root.contains("clientId")) {
            if (!root["clientId"].is_string() || root["clientId"].get<std::string>().size() > 128) {
                sendError(res, 400, "MALFORMED_UPLOAD", "invalid clientId");
                return;
            }
            clientId = root["clientId"].get<std::string>();
        }
    } else if (root.contains("format")) {
        transcriptObj = root;
    } else {
        sendError(res, 400, "MALFORMED_UPLOAD", "missing transcript or format field");
        return;
    }

    if (transcriptObj.dump().size() > 49152) {
        sendError(res, 400, "MALFORMED_UPLOAD", "transcript exceeds 48 KiB");
        return;
    }

    if (!transcriptObj.contains("format") || !transcriptObj["format"].is_string() ||
        transcriptObj["format"].get<std::string>() != "octi.transcript.v3") {
        sendError(res, 400, "MALFORMED_UPLOAD", "unsupported transcript format");
        return;
    }

    if (!transcriptObj.contains("mode") || !transcriptObj["mode"].is_string()) {
        sendError(res, 400, "MALFORMED_UPLOAD", "missing mode");
        return;
    }
    std::string modeStr = transcriptObj["mode"].get<std::string>();
    OctiGameMode mode;
    if (modeStr == "2P") {
        mode = GAME_MODE_2;
    } else if (modeStr == "4P") {
        mode = GAME_MODE_4;
    } else if (modeStr == "OFK") {
        mode = GAME_MODE_OFK;
    } else {
        sendError(res, 400, "MALFORMED_UPLOAD", "invalid mode");
        return;
    }

    if (!transcriptObj.contains("bases") || !transcriptObj["bases"].is_number_integer()) {
        sendError(res, 400, "MALFORMED_UPLOAD", "missing bases");
        return;
    }
    int basesVal = transcriptObj["bases"].get<int>();
    if (basesVal != 1 && basesVal != 3) {
        sendError(res, 400, "MALFORMED_UPLOAD", "bases must be 1 or 3");
        return;
    }
    OctiGameBases bases = (basesVal == 3) ? GAME_BASES_3 : GAME_BASES_1;

    if (!transcriptObj.contains("superProngs") || !transcriptObj["superProngs"].is_boolean() ||
        !transcriptObj.contains("edgeless") || !transcriptObj["edgeless"].is_boolean()) {
        sendError(res, 400, "MALFORMED_UPLOAD", "missing superProngs or edgeless");
        return;
    }
    bool superProngs = transcriptObj["superProngs"].get<bool>();
    bool edgeless = transcriptObj["edgeless"].get<bool>();

    std::string startPlayerStr = transcriptObj.value("startPlayer", "A");
    OctiPlayer startPlayer = PLAYER_A;
    if (startPlayerStr == "A") startPlayer = PLAYER_A;
    else if (startPlayerStr == "B") startPlayer = PLAYER_B;
    else if (startPlayerStr == "C") startPlayer = PLAYER_C;
    else if (startPlayerStr == "D") startPlayer = PLAYER_D;
    else {
        sendError(res, 400, "MALFORMED_UPLOAD", "invalid startPlayer");
        return;
    }

    if (!transcriptObj.contains("status") || !transcriptObj["status"].is_string()) {
        sendError(res, 400, "MALFORMED_UPLOAD", "missing status");
        return;
    }
    std::string statusStr = transcriptObj["status"].get<std::string>();
    if (statusStr == "IN_PROGRESS" || statusStr == "NOT_STARTED") {
        sendError(res, 400, "GAME_NOT_COMPLETED", "game is not completed");
        return;
    }
    if (statusStr != "GAME_OVER" && statusStr != "RESIGNED") {
        sendError(res, 400, "MALFORMED_UPLOAD", "invalid status");
        return;
    }

    if (!transcriptObj.contains("winner") || !transcriptObj["winner"].is_string()) {
        sendError(res, 400, "MALFORMED_UPLOAD", "missing winner");
        return;
    }
    std::string claimedWinner = transcriptObj["winner"].get<std::string>();
    if (claimedWinner != "A" && claimedWinner != "B" && claimedWinner != "C" &&
        claimedWinner != "D" && claimedWinner != "NONE") {
        sendError(res, 400, "MALFORMED_UPLOAD", "invalid winner value");
        return;
    }
    if (!transcriptObj.contains("finalFen") || !transcriptObj["finalFen"].is_string()) {
        sendError(res, 400, "MALFORMED_UPLOAD", "missing finalFen");
        return;
    }
    std::string claimedFinalFen = transcriptObj["finalFen"].get<std::string>();

    std::string appVersion;
    std::string platform;
    std::string installId = clientId;
    if (transcriptObj.contains("client")) {
        if (!transcriptObj["client"].is_object()) {
            sendError(res, 400, "MALFORMED_UPLOAD", "client must be an object");
            return;
        }
        const auto &clientObj = transcriptObj["client"];
        for (auto it = clientObj.begin(); it != clientObj.end(); ++it) {
            const std::string &k = it.key();
            if (k != "appVersion" && k != "buildType" && k != "platform" && k != "extended") {
                if (!it.value().is_string() && !it.value().is_number()) {
                    sendError(res, 400, "MALFORMED_UPLOAD", "invalid unknown client field type");
                    return;
                }
                if (it.value().is_string() && it.value().get<std::string>().size() > 128) {
                    sendError(res, 400, "MALFORMED_UPLOAD", "oversized unknown client field");
                    return;
                }
            }
        }
        if (clientObj.contains("appVersion")) {
            if (!clientObj["appVersion"].is_string() || clientObj["appVersion"].get<std::string>().size() > 128) {
                sendError(res, 400, "MALFORMED_UPLOAD", "invalid appVersion");
                return;
            }
            appVersion = clientObj["appVersion"].get<std::string>();
            if (hasControlChars(appVersion)) {
                sendError(res, 400, "MALFORMED_UPLOAD", "appVersion has control chars");
                return;
            }
        }
        if (clientObj.contains("platform")) {
            if (!clientObj["platform"].is_string() || clientObj["platform"].get<std::string>().size() > 128) {
                sendError(res, 400, "MALFORMED_UPLOAD", "invalid platform");
                return;
            }
            platform = clientObj["platform"].get<std::string>();
            if (hasControlChars(platform)) {
                sendError(res, 400, "MALFORMED_UPLOAD", "platform has control chars");
                return;
            }
        }
        if (clientObj.contains("extended")) {
            if (!clientObj["extended"].is_object()) {
                sendError(res, 400, "MALFORMED_UPLOAD", "extended must be an object");
                return;
            }
            const auto &ext = clientObj["extended"];
            for (auto it = ext.begin(); it != ext.end(); ++it) {
                const std::string &k = it.key();
                if (k != "startedAt" && k != "endedAt" && k != "utcOffsetMin" &&
                    k != "locale" && k != "osVersion" && k != "installId") {
                    if (it.value().is_string() && it.value().get<std::string>().size() > 128) {
                        sendError(res, 400, "MALFORMED_UPLOAD", "oversized extended field");
                        return;
                    }
                }
            }
            if (ext.contains("installId") && ext["installId"].is_string()) {
                std::string iid = ext["installId"].get<std::string>();
                if (iid.size() > 128 || hasControlChars(iid)) {
                    sendError(res, 400, "MALFORMED_UPLOAD", "invalid installId in extended");
                    return;
                }
                if (!iid.empty()) {
                    installId = iid;
                }
            }
            if (ext.contains("locale") && ext["locale"].is_string()) {
                if (ext["locale"].get<std::string>().size() > 128 || hasControlChars(ext["locale"].get<std::string>())) {
                    sendError(res, 400, "MALFORMED_UPLOAD", "invalid locale");
                    return;
                }
            }
            if (ext.contains("osVersion") && ext["osVersion"].is_string()) {
                if (ext["osVersion"].get<std::string>().size() > 128 || hasControlChars(ext["osVersion"].get<std::string>())) {
                    sendError(res, 400, "MALFORMED_UPLOAD", "invalid osVersion");
                    return;
                }
            }
        }
    }

    if (!transcriptObj.contains("plies") || !transcriptObj["plies"].is_array()) {
        sendError(res, 400, "MALFORMED_UPLOAD", "missing plies array");
        return;
    }
    const auto &plies = transcriptObj["plies"];
    if (plies.empty()) {
        sendError(res, 400, "GAME_NOT_COMPLETED", "plies array cannot be empty");
        return;
    }
    if (plies.size() > 1000) {
        sendError(res, 400, "MALFORMED_UPLOAD", "too many plies (> 1000)");
        return;
    }

    int detectedAiDifficulty = 0;
    std::string player1Handle = "Player";
    std::string player2Handle = "Player";
    if (transcriptObj.contains("players") && transcriptObj["players"].is_array()) {
        for (const auto &p : transcriptObj["players"]) {
            if (p.is_object()) {
                int seat = p.value("seat", 0);
                std::string h = p.value("handle", "");
                if (seat == 1 && !h.empty()) player1Handle = h;
                if (seat == 2 && !h.empty()) player2Handle = h;
                if (p.value("type", "") == "AI" || p.value("aiDifficulty", 0) > 0) {
                    int d = p.value("aiDifficulty", 0);
                    if (d >= 1 && d <= 5) detectedAiDifficulty = d;
                }
            }
        }
    }

    for (size_t i = 0; i < plies.size(); ++i) {
        const auto &p = plies[i];
        if (!p.is_object()) {
            sendError(res, 400, "MALFORMED_UPLOAD", "ply must be an object");
            return;
        }
        if (!p.contains("move") || !p["move"].is_string()) {
            sendError(res, 400, "MALFORMED_UPLOAD", "ply missing move");
            return;
        }
        std::string mv = p["move"].get<std::string>();
        if (mv.empty() || mv.size() > 128 || hasControlChars(mv)) {
            sendError(res, 400, "MALFORMED_UPLOAD", "invalid move in ply");
            return;
        }
        if (p.contains("elapsedMs")) {
            if (!p["elapsedMs"].is_number_integer() || p["elapsedMs"].get<int64_t>() < 0) {
                sendError(res, 400, "MALFORMED_UPLOAD", "negative elapsedMs");
                return;
            }
        }
        if (p.contains("clockAfterMs")) {
            if (!p["clockAfterMs"].is_number_integer() || p["clockAfterMs"].get<int64_t>() < -1) {
                sendError(res, 400, "MALFORMED_UPLOAD", "invalid clockAfterMs");
                return;
            }
        }
        if (p.contains("ai") && p["ai"].is_object()) {
            const auto &ai = p["ai"];
            if (ai.contains("difficulty") && ai["difficulty"].is_number_integer()) {
                int d = ai["difficulty"].get<int>();
                if (d < 0 || d > 5) {
                    sendError(res, 400, "MALFORMED_UPLOAD", "invalid ai difficulty");
                    return;
                }
                if (d > 0 && detectedAiDifficulty == 0) detectedAiDifficulty = d;
            }
            if (ai.contains("budgetMs") && (!ai["budgetMs"].is_number_integer() || ai["budgetMs"].get<int64_t>() < 0)) {
                sendError(res, 400, "MALFORMED_UPLOAD", "negative budgetMs");
                return;
            }
            if (ai.contains("depthReached") && (!ai["depthReached"].is_number_integer() || ai["depthReached"].get<int>() < 0)) {
                sendError(res, 400, "MALFORMED_UPLOAD", "negative depthReached");
                return;
            }
            if (ai.contains("nodesVisited") && (!ai["nodesVisited"].is_number_unsigned() && !ai["nodesVisited"].is_number_integer())) {
                sendError(res, 400, "MALFORMED_UPLOAD", "invalid nodesVisited");
                return;
            }
        }
    }

    OctiBoard board(mode, superProngs, edgeless, bases);
    OctiGame *game = board.getGame();
    if (!game) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to initialize game engine");
        return;
    }
    game->start(startPlayer);

    for (size_t i = 0; i < plies.size(); ++i) {
        const auto &p = plies[i];
        std::string plyPlayerStr = p.value("player", "");
        std::string mv = p["move"].get<std::string>();

        OctiPlayer expectedPlayer = game->getTurn();
        std::string expectedPlayerStr;
        switch (expectedPlayer) {
        case PLAYER_A: expectedPlayerStr = "A"; break;
        case PLAYER_B: expectedPlayerStr = "B"; break;
        case PLAYER_C: expectedPlayerStr = "C"; break;
        case PLAYER_D: expectedPlayerStr = "D"; break;
        default: break;
        }
        if (!plyPlayerStr.empty() && plyPlayerStr != expectedPlayerStr) {
            sendError(res, 400, "INVALID_GAME", "turn mismatch in ply " + std::to_string(i + 1));
            return;
        }

        try {
            game->move(expectedPlayer, mv);
        } catch (const IllegalMoveException &e) {
            sendError(res, 400, "INVALID_GAME", std::string("illegal move: ") + e.what());
            return;
        } catch (const IllegalMoveSyntaxException &e) {
            sendError(res, 400, "INVALID_GAME", std::string("illegal syntax: ") + e.what());
            return;
        } catch (const std::exception &e) {
            sendError(res, 400, "INVALID_GAME", std::string("move failed: ") + e.what());
            return;
        }
    }

    if (statusStr == "GAME_OVER") {
        if (!game->isGameOver()) {
            sendError(res, 400, "GAME_NOT_COMPLETED", "game did not reach game over state");
            return;
        }
    } else if (statusStr == "RESIGNED") {
        if (claimedWinner == "NONE") {
            sendError(res, 400, "INVALID_GAME", "resigned game must have a winner");
            return;
        }
        OctiPlayer resWinner = PLAYER_UNDEF;
        if (claimedWinner == "A") resWinner = PLAYER_A;
        else if (claimedWinner == "B") resWinner = PLAYER_B;
        else if (claimedWinner == "C") resWinner = PLAYER_C;
        else if (claimedWinner == "D") resWinner = PLAYER_D;
        game->setResignedWinner(resWinner);
    }

    std::string replayedWinner;
    switch (game->getWinner()) {
    case PLAYER_A: replayedWinner = "A"; break;
    case PLAYER_B: replayedWinner = "B"; break;
    case PLAYER_C: replayedWinner = "C"; break;
    case PLAYER_D: replayedWinner = "D"; break;
    default: replayedWinner = "NONE"; break;
    }
    if (claimedWinner != replayedWinner) {
        sendError(res, 400, "INVALID_GAME", "claimed winner does not match replay");
        return;
    }

    std::string replayedFinalFen = board.toPositionString();
    if (claimedFinalFen != replayedFinalFen) {
        sendError(res, 400, "INVALID_GAME", "claimed finalFen does not match replay");
        return;
    }

    std::string serverGameId = board.computeCanonicalGameId();

    Json storedJson = transcriptObj;
    if (storedJson.contains("client") && storedJson["client"].is_object()) {
        storedJson["client"].erase("extended");
    }
    storedJson["gameId"] = serverGameId;
    storedJson["finalFen"] = replayedFinalFen;
    storedJson["license"] = "CC-BY-4.0";
    storedJson["attribution"] = "Octi game data, Charles Chiou (selfso.com)";
    storedJson["winner"] = replayedWinner;
    storedJson["status"] = statusStr;

    int64_t now = epochSec();
    int64_t startedAt = now;
    int64_t endedAt = now;
    if (transcriptObj.contains("client") && transcriptObj["client"].is_object() &&
        transcriptObj["client"].contains("extended") && transcriptObj["client"]["extended"].is_object()) {
        const auto &ext = transcriptObj["client"]["extended"];
        if (ext.contains("startedAt") && ext["startedAt"].is_number_integer() && ext["startedAt"].get<int64_t>() > 0) {
            startedAt = ext["startedAt"].get<int64_t>();
        }
        if (ext.contains("endedAt") && ext["endedAt"].is_number_integer() && ext["endedAt"].get<int64_t>() > 0) {
            endedAt = ext["endedAt"].get<int64_t>();
        }
    }
    if (endedAt < startedAt) {
        endedAt = startedAt;
    }

    GameRecord rec;
    rec.gameId = serverGameId;
    rec.gameMode = modeStr;
    rec.gameBases = basesVal;
    rec.superProngs = superProngs;
    rec.edgeless = edgeless;
    rec.timeControl = transcriptObj.value("timeControl", "untimed");
    rec.p1Handle = player1Handle;
    rec.p2Handle = player2Handle;
    rec.result = (replayedWinner == "A") ? "P1_WIN" : (replayedWinner == "B" ? "P2_WIN" : "DRAW");
    if (replayedWinner == "A") rec.winnerSeat = 1;
    else if (replayedWinner == "B") rec.winnerSeat = 2;
    else if (replayedWinner == "C") rec.winnerSeat = 3;
    else if (replayedWinner == "D") rec.winnerSeat = 4;
    else rec.winnerSeat = 0;
    rec.termination = transcriptObj.value("termination", statusStr == "RESIGNED" ? "RESIGNATION" : "BASE_OCCUPATION");
    rec.plyCount = static_cast<int>(plies.size());
    rec.octiTranscript = storedJson.dump();
    rec.finalFen = replayedFinalFen;
    rec.startedAt = startedAt;
    rec.endedAt = endedAt;
    rec.source = "client_upload";
    rec.uploadedAt = now;
    rec.winner = replayedWinner;
    rec.status = statusStr;
    rec.aiDifficulty = detectedAiDifficulty;
    rec.appVersion = appVersion;
    rec.platform = platform;
    rec.installId = installId;

    std::string archiveErr;
    ArchiveStatus st = store.archiveGame(rec, archiveErr);
    if (st == ArchiveStatus::DUPLICATE_ID) {
        sendError(res, 409, "DUPLICATE_ID", "game already archived");
        return;
    }
    if (st != ArchiveStatus::OK) {
        sendError(res, 500, "INTERNAL_ERROR", "archive failed: " + archiveErr);
        return;
    }

    Json resp;
    resp["ok"] = true;
    resp["game_id"] = rec.gameId;
    resp["ply_count"] = rec.plyCount;
    sendJson(res, 200, resp);
}

void OctiHttpServer::Impl::handleArchiveDeleteInstall(const Request &req, Response &res)
{
    std::string installId;
    if (!pathId(req, "installId", res, installId)) {
        return;
    }
    if (installId.empty() || installId.size() > 64 || !isSafeId(installId)) {
        sendError(res, 400, "INVALID_ARGUMENT", "invalid installId");
        return;
    }
    int deleted = store.deleteGamesByInstallId(installId);
    if (deleted < 0) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to delete games");
        return;
    }
    Json j;
    j["ok"] = true;
    j["deleted"] = deleted;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleArchiveList(const Request &req,
                                             Response &res)
{
    ArchiveFilter f;
    if (req.has_param("mode")) {
        OctiGameMode m = GAME_MODE_2;
        std::string mode = upper(req.get_param_value("mode"));
        if (!parseModeName(mode, m)) {
            sendError(res, 400, "INVALID_ARGUMENT",
                      "mode must be 2P, 4P or OFK");
            return;
        }
        f.mode = mode;
    }
    if (req.has_param("player")) {
        f.playerUuid = req.get_param_value("player");
        if (!isSafeId(f.playerUuid)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid player id");
            return;
        }
    }
    if (req.has_param("source")) {
        std::string src = lower(req.get_param_value("source"));
        if (src != "online" && src != "client_upload") {
            sendError(res, 400, "INVALID_ARGUMENT", "source must be online or client_upload");
            return;
        }
        f.source = src;
    }
    if (req.has_param("status")) {
        std::string st = upper(req.get_param_value("status"));
        if (st != "GAME_OVER" && st != "RESIGNED") {
            sendError(res, 400, "INVALID_ARGUMENT", "status must be GAME_OVER or RESIGNED");
            return;
        }
        f.status = st;
    }
    if (req.has_param("winner")) {
        std::string w = upper(req.get_param_value("winner"));
        if (w != "A" && w != "B" && w != "C" && w != "D" && w != "NONE") {
            sendError(res, 400, "INVALID_ARGUMENT", "winner must be A, B, C, D, or NONE");
            return;
        }
        f.winner = w;
    }
    if (req.has_param("aiDifficulty") || req.has_param("ai_difficulty")) {
        std::string dStr = req.has_param("aiDifficulty") ? req.get_param_value("aiDifficulty") : req.get_param_value("ai_difficulty");
        int64_t diff = 0;
        if (!parseUnsignedClamped(dStr, 5, diff) || diff < 1 || diff > 5) {
            sendError(res, 400, "INVALID_ARGUMENT", "aiDifficulty must be 1..5");
            return;
        }
        f.aiDifficulty = static_cast<int>(diff);
    }
    if (req.has_param("appVersion") || req.has_param("app_version")) {
        std::string av = req.has_param("appVersion") ? req.get_param_value("appVersion") : req.get_param_value("app_version");
        if (av.size() > 128 || hasControlChars(av)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid appVersion");
            return;
        }
        f.appVersion = av;
    }
    if (req.has_param("platform")) {
        std::string plat = req.get_param_value("platform");
        if (plat.size() > 128 || hasControlChars(plat)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid platform");
            return;
        }
        f.platform = plat;
    }
    if (req.has_param("from")) {
        int64_t fromSec = 0;
        if (!parseUnsignedClamped(req.get_param_value("from"), INT64_MAX, fromSec)) {
            sendError(res, 400, "INVALID_ARGUMENT", "from must be a non-negative integer");
            return;
        }
        f.fromSec = fromSec;
    }
    if (req.has_param("to")) {
        int64_t toSec = 0;
        if (!parseUnsignedClamped(req.get_param_value("to"), INT64_MAX, toSec)) {
            sendError(res, 400, "INVALID_ARGUMENT", "to must be a non-negative integer");
            return;
        }
        f.toSec = toSec;
    }
    if (req.has_param("sort")) {
        std::string sVal = req.get_param_value("sort");
        std::string field = sVal;
        std::string dir = "";
        size_t colon = sVal.find(':');
        if (colon != std::string::npos) {
            field = sVal.substr(0, colon);
            dir = lower(sVal.substr(colon + 1));
        }
        if (field != "ended_at" && field != "uploaded_at" && field != "ply_count") {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid sort field");
            return;
        }
        f.sortField = field;
        if (dir == "asc") {
            f.sortAsc = true;
        } else if (dir == "desc") {
            f.sortAsc = false;
        }
    }
    if (req.has_param("order")) {
        std::string ord = lower(req.get_param_value("order"));
        if (ord == "asc") {
            f.sortAsc = true;
        } else if (ord == "desc") {
            f.sortAsc = false;
        } else {
            sendError(res, 400, "INVALID_ARGUMENT", "order must be asc or desc");
            return;
        }
    }

    int64_t limit = 20;
    int64_t offset = 0;
    if ((req.has_param("limit") &&
         !parseUnsignedClamped(req.get_param_value("limit"), INT32_MAX,
                               limit)) ||
        (req.has_param("offset") &&
         !parseUnsignedClamped(req.get_param_value("offset"), INT32_MAX,
                               offset))) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "limit and offset must be non-negative integers");
        return;
    }
    limit = std::max<int64_t>(1, std::min<int64_t>(limit,
                                                    AccountStore::kMaxArchiveLimit));
    f.limit = static_cast<int>(limit);
    f.offset = static_cast<int>(offset);
    std::vector<GameRecord> games;
    int64_t total = 0;
    if (!store.listArchivedGames(f, games, &total)) {
        sendError(res, 500, "INTERNAL_ERROR", "archive query failed");
        return;
    }
    Json arr = Json::array();
    for (size_t i = 0; i < games.size(); ++i) {
        arr.push_back(gameSummaryJson(games[i]));
    }
    Json j;
    j["total"] = total;
    j["limit"] = limit;
    j["offset"] = offset;
    j["games"] = arr;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleArchiveGet(const Request &req, Response &res)
{
    std::string id;
    if (!pathId(req, "id", res, id)) {
        return;
    }
    GameRecord rec;
    if (!store.getArchivedGame(id, rec)) {
        sendError(res, 404, "NOT_FOUND", "no such archived game");
        return;
    }
    Json j = gameSummaryJson(rec);
    j["final_fen"] = rec.finalFen;
    std::string sanitized = sanitizeTranscriptJson(rec.octiTranscript, true);
    j["transcript_json"] = sanitized;
    j["octi_transcript"] = sanitized;
    j["license"] = "CC-BY-4.0";
    j["attribution"] = "Octi game data, Charles Chiou (selfso.com)";
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleArchiveTranscript(const Request &req,
                                                   Response &res)
{
    std::string id;
    if (!pathId(req, "id", res, id)) {
        return;
    }
    std::string transcript;
    if (!store.getArchivedTranscript(id, transcript)) {
        sendError(res, 404, "NOT_FOUND", "no such archived game");
        return;
    }
    std::string sanitized = sanitizeTranscriptJson(transcript, true);
    res.status = 200;
    res.set_header("Cache-Control", "no-store");
    res.set_header("Content-Disposition",
                   "attachment; filename=\"" + id + ".json\"");
    res.set_content(sanitized, "application/json; charset=utf-8");
}

void OctiHttpServer::Impl::handleArchiveOcti(const Request &req,
                                             Response &res)
{
    handleArchiveTranscript(req, res);
}

void OctiHttpServer::Impl::handleArchivePlies(const Request &req, Response &res)
{
    std::string id;
    if (!pathId(req, "id", res, id)) {
        return;
    }
    std::string transcript;
    if (!store.getArchivedTranscript(id, transcript)) {
        sendError(res, 404, "NOT_FOUND", "no such archived game");
        return;
    }

    Json tObj;
    try {
        tObj = Json::parse(transcript);
        if (!tObj.is_object() || tObj.value("format", "") != "octi.transcript.v3") {
            sendError(res, 500, "INTERNAL_ERROR", "corrupt transcript");
            return;
        }
    } catch (...) {
        sendError(res, 500, "INTERNAL_ERROR", "corrupt transcript json");
        return;
    }

    std::string modeStr = tObj.value("mode", "2P");
    OctiGameMode mode = GAME_MODE_2;
    if (modeStr == "4P") mode = GAME_MODE_4;
    else if (modeStr == "OFK") mode = GAME_MODE_OFK;

    int basesVal = tObj.value("bases", 1);
    OctiGameBases bases = (basesVal == 3) ? GAME_BASES_3 : GAME_BASES_1;
    bool superProngs = tObj.value("superProngs", false);
    bool edgeless = tObj.value("edgeless", false);
    std::string startPlayerStr = tObj.value("startPlayer", "A");
    OctiPlayer startPlayer = PLAYER_A;
    if (startPlayerStr == "B") startPlayer = PLAYER_B;
    else if (startPlayerStr == "C") startPlayer = PLAYER_C;
    else if (startPlayerStr == "D") startPlayer = PLAYER_D;

    OctiBoard board(mode, superProngs, edgeless, bases);
    std::string initialFen = board.toPositionString();
    OctiGame *game = board.getGame();
    if (!game) {
        sendError(res, 500, "INTERNAL_ERROR", "engine initialization failed");
        return;
    }
    game->start(startPlayer);

    Json pliesArr = Json::array();
    if (tObj.contains("plies") && tObj["plies"].is_array()) {
        const auto &rawPlies = tObj["plies"];
        for (size_t i = 0; i < rawPlies.size(); ++i) {
            const auto &p = rawPlies[i];
            if (!p.is_object() || !p.contains("move") || !p["move"].is_string()) {
                continue;
            }
            std::string mv = p["move"].get<std::string>();
            OctiPlayer turn = game->getTurn();
            std::string playerStr;
            switch (turn) {
            case PLAYER_A: playerStr = "A"; break;
            case PLAYER_B: playerStr = "B"; break;
            case PLAYER_C: playerStr = "C"; break;
            case PLAYER_D: playerStr = "D"; break;
            default: break;
            }
            try {
                game->move(turn, mv);
            } catch (const std::exception &ex) {
                sendError(res, 500, "INTERNAL_ERROR", std::string("replay error at ply ") + std::to_string(i + 1) + ": " + ex.what());
                return;
            }

            Json plyOut;
            plyOut["ply"] = i + 1;
            plyOut["plyIndex"] = i + 1;
            plyOut["player"] = playerStr;
            plyOut["move"] = mv;
            plyOut["elapsedMs"] = p.value("elapsedMs", 0LL);
            if (p.contains("clockAfterMs") && p["clockAfterMs"].get<int64_t>() >= 0) {
                plyOut["clockAfterMs"] = p["clockAfterMs"].get<int64_t>();
            }
            std::string fenCur = board.toPositionString();
            plyOut["fen"] = fenCur;
            plyOut["fenAfter"] = fenCur;
            if (p.contains("ai") && p["ai"].is_object()) {
                plyOut["ai"] = p["ai"];
            }
            pliesArr.push_back(plyOut);
        }
    }

    Json resp;
    resp["game_id"] = id;
    resp["mode"] = modeStr;
    resp["initialFen"] = initialFen;
    resp["initial_fen"] = initialFen;
    resp["plies"] = pliesArr;
    sendJson(res, 200, resp);
}

void OctiHttpServer::Impl::handleArchiveStats(const Request &req, Response &res)
{
    (void)req;
    ArchiveStats stats;
    if (!store.getArchiveStats(stats)) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to calculate archive stats");
        return;
    }
    Json j;
    j["total_games"] = stats.totalGames;
    j["by_source"] = stats.bySource;
    j["by_mode"] = stats.byMode;
    Json byDiff = Json::object();
    for (const auto &kv : stats.byAiDifficulty) {
        byDiff[std::to_string(kv.first)] = kv.second;
    }
    j["by_ai_difficulty"] = byDiff;
    j["by_winner"] = stats.byWinner;

    Json aiArr = Json::array();
    for (const auto &ds : stats.aiDifficultyStats) {
        Json d;
        d["difficulty"] = ds.difficulty;
        d["total_games"] = ds.totalGames;
        d["ai_wins"] = ds.aiWins;
        d["win_rate"] = ds.winRate;
        aiArr.push_back(d);
    }
    j["ai_win_rates"] = aiArr;
    j["average_plies"] = stats.averagePlies;
    j["uploads_by_day"] = stats.uploadsByDay;
    j["license"] = "CC-BY-4.0";
    j["attribution"] = "Octi game data, Charles Chiou (selfso.com)";
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleArchiveExport(const Request &req, Response &res)
{
    if (!allowExportAttempt(req, res)) {
        return;
    }

    ArchiveFilter f;
    f.includeTranscript = true;
    f.maxLimit = AccountStore::kMaxExportLimit;

    if (req.has_param("mode")) {
        OctiGameMode m = GAME_MODE_2;
        std::string mode = upper(req.get_param_value("mode"));
        if (!parseModeName(mode, m)) {
            sendError(res, 400, "INVALID_ARGUMENT", "mode must be 2P, 4P or OFK");
            return;
        }
        f.mode = mode;
    }
    if (req.has_param("player")) {
        f.playerUuid = req.get_param_value("player");
        if (!isSafeId(f.playerUuid)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid player id");
            return;
        }
    }
    if (req.has_param("source")) {
        std::string src = lower(req.get_param_value("source"));
        if (src != "online" && src != "client_upload") {
            sendError(res, 400, "INVALID_ARGUMENT", "source must be online or client_upload");
            return;
        }
        f.source = src;
    }
    if (req.has_param("status")) {
        std::string st = upper(req.get_param_value("status"));
        if (st != "GAME_OVER" && st != "RESIGNED") {
            sendError(res, 400, "INVALID_ARGUMENT", "status must be GAME_OVER or RESIGNED");
            return;
        }
        f.status = st;
    }
    if (req.has_param("winner")) {
        std::string w = upper(req.get_param_value("winner"));
        if (w != "A" && w != "B" && w != "C" && w != "D" && w != "NONE") {
            sendError(res, 400, "INVALID_ARGUMENT", "winner must be A, B, C, D, or NONE");
            return;
        }
        f.winner = w;
    }
    if (req.has_param("aiDifficulty") || req.has_param("ai_difficulty")) {
        std::string dStr = req.has_param("aiDifficulty") ? req.get_param_value("aiDifficulty") : req.get_param_value("ai_difficulty");
        int64_t diff = 0;
        if (!parseUnsignedClamped(dStr, 5, diff) || diff < 1 || diff > 5) {
            sendError(res, 400, "INVALID_ARGUMENT", "aiDifficulty must be 1..5");
            return;
        }
        f.aiDifficulty = static_cast<int>(diff);
    }
    if (req.has_param("appVersion") || req.has_param("app_version")) {
        std::string av = req.has_param("appVersion") ? req.get_param_value("appVersion") : req.get_param_value("app_version");
        if (av.size() > 128 || hasControlChars(av)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid appVersion");
            return;
        }
        f.appVersion = av;
    }
    if (req.has_param("platform")) {
        std::string plat = req.get_param_value("platform");
        if (plat.size() > 128 || hasControlChars(plat)) {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid platform");
            return;
        }
        f.platform = plat;
    }
    if (req.has_param("from")) {
        int64_t fromSec = 0;
        if (!parseUnsignedClamped(req.get_param_value("from"), INT64_MAX, fromSec)) {
            sendError(res, 400, "INVALID_ARGUMENT", "from must be a non-negative integer");
            return;
        }
        f.fromSec = fromSec;
    }
    if (req.has_param("to")) {
        int64_t toSec = 0;
        if (!parseUnsignedClamped(req.get_param_value("to"), INT64_MAX, toSec)) {
            sendError(res, 400, "INVALID_ARGUMENT", "to must be a non-negative integer");
            return;
        }
        f.toSec = toSec;
    }
    if (req.has_param("sort")) {
        std::string sVal = req.get_param_value("sort");
        std::string field = sVal;
        std::string dir = "";
        size_t colon = sVal.find(':');
        if (colon != std::string::npos) {
            field = sVal.substr(0, colon);
            dir = lower(sVal.substr(colon + 1));
        }
        if (field != "ended_at" && field != "uploaded_at" && field != "ply_count") {
            sendError(res, 400, "INVALID_ARGUMENT", "invalid sort field");
            return;
        }
        f.sortField = field;
        if (dir == "asc") {
            f.sortAsc = true;
        } else if (dir == "desc") {
            f.sortAsc = false;
        }
    }
    if (req.has_param("order")) {
        std::string ord = lower(req.get_param_value("order"));
        if (ord == "asc") {
            f.sortAsc = true;
        } else if (ord == "desc") {
            f.sortAsc = false;
        } else {
            sendError(res, 400, "INVALID_ARGUMENT", "order must be asc or desc");
            return;
        }
    }

    int64_t limit = 1000;
    int64_t offset = 0;
    if ((req.has_param("limit") &&
         !parseUnsignedClamped(req.get_param_value("limit"), INT32_MAX,
                               limit)) ||
        (req.has_param("offset") &&
         !parseUnsignedClamped(req.get_param_value("offset"), INT32_MAX,
                               offset))) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "limit and offset must be non-negative integers");
        return;
    }
    limit = std::max<int64_t>(1, std::min<int64_t>(limit, AccountStore::kMaxExportLimit));
    f.limit = static_cast<int>(limit);
    f.offset = static_cast<int>(offset);

    std::vector<GameRecord> games;
    if (!store.listArchivedGames(f, games)) {
        sendError(res, 500, "INTERNAL_ERROR", "archive export query failed");
        return;
    }

    std::string ndjson;
    for (const auto &g : games) {
        std::string line = sanitizeTranscriptJson(g.octiTranscript, false);
        if (!line.empty()) {
            ndjson += line;
            ndjson += "\n";
        }
    }

    res.status = 200;
    res.set_header("Cache-Control", "no-store");
    res.set_header("Content-Disposition", "attachment; filename=\"octi-archive-export.ndjson\"");
    res.set_content(ndjson, "application/x-ndjson; charset=utf-8");
}

void OctiHttpServer::Impl::handleIndex(const Request &, Response &res)
{
    res.status = 200;
    res.set_header("Cache-Control", "no-cache");
    res.set_header("Content-Security-Policy",
                   "default-src 'none'; script-src 'unsafe-inline'; "
                   "style-src 'unsafe-inline'; connect-src 'self'; "
                   "img-src 'self' data:; base-uri 'none'; "
                   "form-action 'none'; frame-ancestors 'none'");
    res.set_content(webUiHtml(), "text/html; charset=utf-8");
}

void OctiHttpServer::Impl::handleTelemetryCrash(const Request &req, Response &res)
{
    if (!allowUploadAttempt(req, res)) {
        return;
    }
    if (req.body.empty() || req.body.size() > 65536) {
        sendError(res, req.body.size() > 65536 ? 413 : 400,
                  req.body.size() > 65536 ? "PAYLOAD_TOO_LARGE" : "MALFORMED_UPLOAD",
                  "invalid payload size");
        return;
    }
    Json root = Json::parse(req.body, nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        sendError(res, 400, "INVALID_ARGUMENT", "invalid JSON payload");
        return;
    }

    if (!root.contains("platform") || !root["platform"].is_string() ||
        root["platform"].get<std::string>().empty() ||
        root["platform"].get<std::string>().size() > 64) {
        sendError(res, 400, "INVALID_ARGUMENT", "missing or invalid platform");
        return;
    }
    std::string platform = root["platform"].get<std::string>();

    if (!root.contains("exception_type") || !root["exception_type"].is_string() ||
        root["exception_type"].get<std::string>().empty() ||
        root["exception_type"].get<std::string>().size() > 256) {
        sendError(res, 400, "INVALID_ARGUMENT", "missing or invalid exception_type");
        return;
    }
    std::string exceptionType = root["exception_type"].get<std::string>();

    int64_t createdAt = 0;
    if (root.contains("created_at") && root["created_at"].is_number_integer()) {
        createdAt = root["created_at"].get<int64_t>();
    }
    if (createdAt <= 0) {
        createdAt = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    std::string appVersion;
    if (root.contains("app_version") && root["app_version"].is_string()) {
        appVersion = root["app_version"].get<std::string>();
        if (appVersion.size() > 64) {
            sendError(res, 400, "INVALID_ARGUMENT", "app_version exceeds 64 chars");
            return;
        }
    }

    std::string osVersion;
    if (root.contains("os_version") && root["os_version"].is_string()) {
        osVersion = root["os_version"].get<std::string>();
        if (osVersion.size() > 128) {
            sendError(res, 400, "INVALID_ARGUMENT", "os_version exceeds 128 chars");
            return;
        }
    }

    std::string exceptionMessage;
    if (root.contains("exception_message") && root["exception_message"].is_string()) {
        exceptionMessage = root["exception_message"].get<std::string>();
        if (exceptionMessage.size() > 2048) {
            sendError(res, 400, "INVALID_ARGUMENT", "exception_message exceeds 2048 chars");
            return;
        }
    }

    std::string threadName;
    if (root.contains("thread_name") && root["thread_name"].is_string()) {
        threadName = root["thread_name"].get<std::string>();
        if (threadName.size() > 128) {
            sendError(res, 400, "INVALID_ARGUMENT", "thread_name exceeds 128 chars");
            return;
        }
    }

    std::string stackTrace;
    if (root.contains("stack_trace") && root["stack_trace"].is_string()) {
        stackTrace = root["stack_trace"].get<std::string>();
        if (stackTrace.size() > 32768) {
            sendError(res, 400, "INVALID_ARGUMENT", "stack_trace exceeds 32768 chars");
            return;
        }
    }

    std::string contextJson = "{}";
    if (root.contains("context")) {
        if (root["context"].is_object()) {
            contextJson = root["context"].dump();
        } else if (root["context"].is_string()) {
            contextJson = root["context"].get<std::string>();
        } else if (!root["context"].is_null()) {
            contextJson = root["context"].dump();
        }
        if (contextJson.size() > 16384) {
            sendError(res, 400, "INVALID_ARGUMENT", "context exceeds 16384 chars");
            return;
        }
    }

    std::string installId;
    if (root.contains("install_id") && root["install_id"].is_string()) {
        installId = root["install_id"].get<std::string>();
        if (installId.size() > 128) {
            sendError(res, 400, "INVALID_ARGUMENT", "install_id exceeds 128 chars");
            return;
        }
    }

    std::string crashId;
    if (root.contains("crash_id") && root["crash_id"].is_string()) {
        crashId = root["crash_id"].get<std::string>();
    }
    if (crashId.empty()) {
        std::string seed = platform + "|" + appVersion + "|" + exceptionType + "|" +
                           exceptionMessage + "|" + stackTrace + "|" + std::to_string(createdAt);
        crashId = AccountStore::sha256Hex(seed);
    } else if (crashId.size() > 128) {
        sendError(res, 400, "INVALID_ARGUMENT", "crash_id exceeds 128 chars");
        return;
    }

    CrashReportRecord rec;
    rec.crashId = crashId;
    rec.createdAt = createdAt;
    rec.uploadedAt = 0;
    rec.platform = platform;
    rec.appVersion = appVersion;
    rec.osVersion = osVersion;
    rec.exceptionType = exceptionType;
    rec.exceptionMessage = exceptionMessage;
    rec.threadName = threadName;
    rec.stackTrace = stackTrace;
    rec.contextJson = contextJson.empty() ? "{}" : contextJson;
    rec.installId = installId;

    std::string err;
    ArchiveStatus st = store.recordCrashReport(rec, err);
    if (st == ArchiveStatus::STORED) {
        Json j;
        j["crash_id"] = rec.crashId;
        j["deduplicated"] = false;
        sendJson(res, 201, j);
    } else if (st == ArchiveStatus::DUPLICATE) {
        Json j;
        j["crash_id"] = rec.crashId;
        j["deduplicated"] = true;
        sendJson(res, 200, j);
    } else {
        sendError(res, 400, "INVALID_ARGUMENT", err.empty() ? "invalid crash report" : err);
    }
}

void OctiHttpServer::Impl::handleTelemetryCrashes(const Request &req, Response &res)
{
    CrashReportFilter f;
    if (req.has_param("platform")) {
        f.platform = req.get_param_value("platform");
    }
    if (req.has_param("app_version")) {
        f.appVersion = req.get_param_value("app_version");
    } else if (req.has_param("appVersion")) {
        f.appVersion = req.get_param_value("appVersion");
    }
    if (req.has_param("exception_type")) {
        f.exceptionType = req.get_param_value("exception_type");
    } else if (req.has_param("exceptionType")) {
        f.exceptionType = req.get_param_value("exceptionType");
    }

    int64_t limit = 20;
    int64_t offset = 0;
    if ((req.has_param("limit") &&
         !parseUnsignedClamped(req.get_param_value("limit"), INT32_MAX, limit)) ||
        (req.has_param("offset") &&
         !parseUnsignedClamped(req.get_param_value("offset"), INT32_MAX, offset))) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "limit and offset must be non-negative integers");
        return;
    }
    limit = std::max<int64_t>(1, std::min<int64_t>(limit, 100));
    f.limit = static_cast<int>(limit);
    f.offset = static_cast<int>(offset);

    std::vector<CrashReportRecord> records;
    int64_t total = 0;
    if (!store.listCrashReports(f, records, &total)) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to list crash reports");
        return;
    }

    Json arr = Json::array();
    for (const auto &r : records) {
        Json item;
        item["crash_id"] = r.crashId;
        item["created_at"] = r.createdAt;
        item["uploaded_at"] = r.uploadedAt;
        item["platform"] = r.platform;
        item["app_version"] = r.appVersion;
        item["os_version"] = r.osVersion;
        item["exception_type"] = r.exceptionType;
        item["exception_message"] = r.exceptionMessage;
        item["thread_name"] = r.threadName;
        arr.push_back(item);
    }
    Json j;
    j["total"] = total;
    j["crashes"] = arr;
    sendJson(res, 200, j);
}

bool OctiHttpServer::Impl::pathCrashId(const Request &req, Response &res,
                                       std::string &out)
{
    std::string val;
    auto it = req.path_params.find("id");
    if (it != req.path_params.end()) {
        val = it->second;
    } else if (req.matches.size() > 1) {
        val = req.matches[1].str();
    }
    if (val.empty() || val.size() > 128) {
        sendError(res, 404, "NOT_FOUND", "no such resource");
        return false;
    }
    for (char c : val) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (!std::isalnum(uc) && uc != '-' && uc != '_') {
            sendError(res, 404, "NOT_FOUND", "no such resource");
            return false;
        }
    }
    out = val;
    return true;
}

void OctiHttpServer::Impl::handleTelemetryCrashGet(const Request &req, Response &res)
{
    std::string id;
    if (!pathCrashId(req, res, id)) {
        return;
    }
    CrashReportRecord rec;
    if (!store.getCrashReport(id, rec)) {
        sendError(res, 404, "NOT_FOUND", "no such crash report");
        return;
    }
    Json j;
    j["crash_id"] = rec.crashId;
    j["created_at"] = rec.createdAt;
    j["uploaded_at"] = rec.uploadedAt;
    j["platform"] = rec.platform;
    j["app_version"] = rec.appVersion;
    j["os_version"] = rec.osVersion;
    j["exception_type"] = rec.exceptionType;
    j["exception_message"] = rec.exceptionMessage;
    j["thread_name"] = rec.threadName;
    j["stack_trace"] = rec.stackTrace;
    Json ctx = Json::parse(rec.contextJson, nullptr, false);
    if (!ctx.is_discarded()) {
        j["context"] = ctx;
    } else {
        j["context"] = Json::object();
    }
    j["context_json"] = rec.contextJson;
    sendJson(res, 200, j);
}

/* ------------------------------------------------------------------ */
/* Routing                                                             */
/* ------------------------------------------------------------------ */

void OctiHttpServer::Impl::registerRoutes()
{
    typedef void (Impl::*Handler)(const Request &, Response &);
    struct Route {
        const char *method;
        const char *pattern;
        Handler handler;
    };
    static const Route routes[] = {
        {"GET", "/", &Impl::handleIndex},
        {"GET", "/api/v1/health", &Impl::handleHealth},
        {"GET", "/api/v1/auth/providers", &Impl::handleProviders},
        {"POST", "/api/v1/auth/sso", &Impl::handleSso},
        {"POST", "/api/v1/auth/link", &Impl::handleLink},
        {"DELETE", "/api/v1/auth/link/:provider", &Impl::handleUnlink},
        {"POST", "/api/v1/auth/logout", &Impl::handleLogout},
        {"GET", "/api/v1/profile/me", &Impl::handleProfileGet},
        {"PATCH", "/api/v1/profile/me", &Impl::handleProfilePatch},
        {"GET", "/api/v1/players/:uuid", &Impl::handlePlayer},
        {"GET", "/api/v1/leaderboard", &Impl::handleLeaderboard},
        {"GET", "/api/v1/lobby/state", &Impl::handleLobbyState},
        {"POST", "/api/v1/lobby/chat", &Impl::handleLobbyChat},
        {"GET", "/api/v1/lobby/events", &Impl::handleLobbyEvents},
        {"GET", "/api/v1/tables", &Impl::handleTables},
        {"POST", "/api/v1/tables", &Impl::handleTableCreate},
        {"POST", "/api/v1/tables/:id/join", &Impl::handleTableJoin},
        {"POST", "/api/v1/tables/:id/leave", &Impl::handleTableLeave},
        {"POST", "/api/v1/tables/:id/kick", &Impl::handleTableKick},
        {"POST", "/api/v1/tables/:id/observe", &Impl::handleTableObserve},
        {"POST", "/api/v1/tables/:id/unobserve", &Impl::handleTableUnobserve},
        {"POST", "/api/v1/matchmaking/enqueue", &Impl::handleEnqueue},
        {"POST", "/api/v1/matchmaking/dequeue", &Impl::handleDequeue},
        {"GET", "/api/v1/matchmaking/status", &Impl::handleMatchStatus},
        {"GET", "/api/v1/games/:id", &Impl::handleGameGet},
        {"POST", "/api/v1/games/:id/move", &Impl::handleMove},
        {"POST", "/api/v1/games/:id/resign", &Impl::handleResign},
        {"POST", "/api/v1/games/:id/action", &Impl::handleAction},
        {"POST", "/api/v1/games/:id/chat", &Impl::handleChat},
        {"POST", "/api/v1/games/:id/voice-signal", &Impl::handleVoiceSignal},
        {"POST", "/api/v1/games/:id/voice-meta", &Impl::handleVoiceMeta},
        {"POST", "/api/v1/games/:id/ban", &Impl::handleBan},
        {"POST", "/api/v1/games/:id/mute", &Impl::handleMute},
        {"POST", "/api/v1/games/:id/settings", &Impl::handleSettings},
        {"GET", "/api/v1/games/:id/events", &Impl::handleGameEvents},
        {"POST", "/api/v1/archive/upload", &Impl::handleArchiveUpload},
        {"DELETE", "/api/v1/archive/install/:installId", &Impl::handleArchiveDeleteInstall},
        {"GET", "/api/v1/archive/games", &Impl::handleArchiveList},
        {"GET", "/api/v1/archive/games/:id", &Impl::handleArchiveGet},
        {"GET", "/api/v1/archive/games/:id/transcript", &Impl::handleArchiveTranscript},
        {"GET", "/api/v1/archive/games/:id/octi", &Impl::handleArchiveOcti},
        {"GET", "/api/v1/archive/games/:id/plies", &Impl::handleArchivePlies},
        {"GET", "/api/v1/archive/stats", &Impl::handleArchiveStats},
        {"GET", "/api/v1/archive/export", &Impl::handleArchiveExport},
        {"POST", "/api/v1/telemetry/crash", &Impl::handleTelemetryCrash},
        {"GET", "/api/v1/telemetry/crashes", &Impl::handleTelemetryCrashes},
        {"GET", "/api/v1/telemetry/crashes/:id", &Impl::handleTelemetryCrashGet},
    };

    server.set_default_headers({{"X-Content-Type-Options", "nosniff"},
                                {"Referrer-Policy", "no-referrer"},
                                {"X-Frame-Options", "DENY"}});

    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); ++i) {
        Handler h = routes[i].handler;
        httplib::Server::Handler fn = [this, h](const Request &rq,
                                                Response &rs) {
            (this->*h)(rq, rs);
        };
        const std::string method = routes[i].method;
        if (method == "GET") {
            server.Get(routes[i].pattern, fn);
        } else if (method == "POST") {
            server.Post(routes[i].pattern, fn);
        } else if (method == "PATCH") {
            server.Patch(routes[i].pattern, fn);
        } else {
            server.Delete(routes[i].pattern, fn);
        }
    }

    /*
     * Voice stays peer-to-peer and nothing but JSON is ever read: a request
     * that announces a body of any other type (audio, octet-stream,
     * multipart...) is refused before a single body byte is consumed.
     */
    server.set_pre_routing_handler(
        [this](const Request &rq, Response &rs) -> httplib::Server::HandlerResponse {
            if (rq.path != "/api/v1/health" &&
                (rq.path == "/api/v1" || rq.path.rfind("/api/v1/", 0) == 0)) {
                int clientProto = 1;
                if (rq.has_header("X-Octi-Protocol-Version")) {
                    std::string val = rq.get_header_value("X-Octi-Protocol-Version");
                    if (val.empty()) {
                        sendError(rs, 400, "INVALID_PROTOCOL_VERSION",
                                  "invalid X-Octi-Protocol-Version header");
                        return httplib::Server::HandlerResponse::Handled;
                    }
                    for (size_t i = 0; i < val.size(); ++i) {
                        if (val[i] < '0' || val[i] > '9') {
                            sendError(rs, 400, "INVALID_PROTOCOL_VERSION",
                                      "invalid X-Octi-Protocol-Version header");
                            return httplib::Server::HandlerResponse::Handled;
                        }
                    }
                    try {
                        long long v = std::stoll(val);
                        if (v < 1 || v > 1000000) {
                            sendError(rs, 400, "INVALID_PROTOCOL_VERSION",
                                      "invalid X-Octi-Protocol-Version header");
                            return httplib::Server::HandlerResponse::Handled;
                        }
                        clientProto = static_cast<int>(v);
                    } catch (...) {
                        sendError(rs, 400, "INVALID_PROTOCOL_VERSION",
                                  "invalid X-Octi-Protocol-Version header");
                        return httplib::Server::HandlerResponse::Handled;
                    }
                }
                if (clientProto < cfg.minProtocolVersion) {
                    Json j;
                    j["error"] = "UPGRADE_REQUIRED";
                    j["message"] =
                        "client protocol version is outdated; please upgrade OCTI";
                    j["protocol_version"] = cfg.protocolVersion;
                    j["min_protocol_version"] = cfg.minProtocolVersion;
                    sendJson(rs, 426, j);
                    return httplib::Server::HandlerResponse::Handled;
                }
            }

            if (rq.method == "GET" || rq.method == "HEAD" ||
                rq.method == "OPTIONS") {
                return httplib::Server::HandlerResponse::Unhandled;
            }
            const bool hasBody = rq.get_header_value("Content-Length") != "" ||
                                 rq.has_header("Transfer-Encoding");
            const bool empty = rq.get_header_value("Content-Length") == "0" &&
                               !rq.has_header("Transfer-Encoding");
            if (hasBody && !empty &&
                !isJsonContentType(rq.get_header_value("Content-Type"))) {
                sendError(rs, 415, "UNSUPPORTED_MEDIA_TYPE",
                          "only application/json request bodies are accepted");
                return httplib::Server::HandlerResponse::Handled;
            }
            return httplib::Server::HandlerResponse::Unhandled;
        });

    server.set_error_handler(
        [](const Request &, Response &rs) -> httplib::Server::HandlerResponse {
            if (!rs.body.empty() || rs.content_provider_) {
                return httplib::Server::HandlerResponse::Unhandled;
            }
            std::string code = "ERROR";
            switch (rs.status) {
            case 404: code = "NOT_FOUND"; break;
            case 405: code = "METHOD_NOT_ALLOWED"; break;
            case 413: code = "PAYLOAD_TOO_LARGE"; break;
            case 408: code = "REQUEST_TIMEOUT"; break;
            case 400: code = "BAD_REQUEST"; break;
            default:
                if (rs.status >= 500) {
                    code = "INTERNAL_ERROR";
                }
                break;
            }
            sendError(rs, rs.status, code,
                      rs.status == 413 ? "request body is too large"
                                       : "request failed");
            return httplib::Server::HandlerResponse::Handled;
        });

    server.set_exception_handler(
        [](const Request &, Response &rs, std::exception_ptr) {
            sendError(rs, 500, "INTERNAL_ERROR", "internal server error");
        });
}

/* ------------------------------------------------------------------ */
/* Public facade                                                       */
/* ------------------------------------------------------------------ */

OctiHttpServer::OctiHttpServer(AccountStore &store, OidcVerifier &verifier,
                               GameRoomManager &rooms,
                               const HttpServerConfig &config)
    : _impl(new Impl(store, verifier, rooms, config))
{
}

OctiHttpServer::~OctiHttpServer()
{
    stop();
}

bool OctiHttpServer::start(std::string &error)
{
    return _impl->start(error);
}

void OctiHttpServer::stop()
{
    _impl->stop();
}

bool OctiHttpServer::isRunning() const
{
    return _impl->server.is_running();
}

int OctiHttpServer::port() const
{
    return _impl->boundPort.load();
}

const std::string &OctiHttpServer::bindAddress() const
{
    return _impl->cfg.bindAddress;
}

uint64_t OctiHttpServer::tickCount() const
{
    return _impl->ticks.load();
}

int OctiHttpServer::activeStreams() const
{
    return _impl->sseActive.load();
}

} // namespace octi

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
