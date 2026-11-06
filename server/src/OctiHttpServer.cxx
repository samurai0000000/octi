/*
 * OctiHttpServer.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiHttpServer.hxx"

#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif
#include "OctiCore.hxx"
#include "OctiBoard.hxx"
#include "OctiCrypto.hxx"
#include "OctiExceptions.hxx"
#include "OctiGame.hxx"
#include "WebUi.hxx"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <sstream>
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
        if (!(std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '@')) {
            return false;
        }
    }
    return true;
}

const std::string kOauthStateSecret = "octi_oauth_state_hmac_secret_2026";

std::string hmacSha256(const std::string &data, const std::string &key)
{
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int mdLen = 0;
    HMAC(EVP_sha256(), key.data(), key.size(),
         reinterpret_cast<const unsigned char *>(data.data()), data.size(),
         md, &mdLen);
    return std::string(reinterpret_cast<const char *>(md), mdLen);
}

std::string urlEncode(const std::string &value)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' ||
            c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::uppercase << std::setw(2)
                    << int(static_cast<unsigned char>(c));
        }
    }
    return escaped.str();
}

struct InitOidcRegistry {
    InitOidcRegistry()
    {
        (void)OidcVerifier::providerRegistry();
    }
} g_initOidcRegistry;

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

std::string detectClientType(const Request &req)
{
    if (req.has_header("X-Octi-Client-Type")) {
        std::string h = lower(req.get_header_value("X-Octi-Client-Type"));
        size_t first = h.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            size_t last = h.find_last_not_of(" \t\r\n");
            h = h.substr(first, (last - first + 1));
        } else {
            h.clear();
        }
        if (h == "web" || h == "android" || h == "desktop") {
            return h;
        }
    }
    if (req.has_param("client_type")) {
        std::string p = lower(req.get_param_value("client_type"));
        size_t first = p.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            size_t last = p.find_last_not_of(" \t\r\n");
            p = p.substr(first, (last - first + 1));
        } else {
            p.clear();
        }
        if (p == "web" || p == "android" || p == "desktop") {
            return p;
        }
    }
    if (req.has_header("User-Agent")) {
        std::string ua = lower(req.get_header_value("User-Agent"));
        if (ua.find("octi-android") != std::string::npos ||
            ua.find("okhttp") != std::string::npos) {
            return "android";
        }
        if (ua.find("octi-desktop") != std::string::npos) {
            return "desktop";
        }
        if (ua.find("mozilla") != std::string::npos ||
            ua.find("chrome") != std::string::npos ||
            ua.find("safari") != std::string::npos ||
            ua.find("firefox") != std::string::npos) {
            return "web";
        }
    }
    return "";
}

std::string clientRemoteIp(const Request &req)
{
    if (req.has_header("X-Forwarded-For")) {
        std::string xff = req.get_header_value("X-Forwarded-For");
        size_t comma = xff.find(',');
        if (comma != std::string::npos) {
            xff = xff.substr(0, comma);
        }
        size_t first = xff.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            size_t last = xff.find_last_not_of(" \t\r\n");
            std::string ip = xff.substr(first, (last - first + 1));
            if (!ip.empty()) {
                return ip;
            }
        }
    }
    if (req.has_header("X-Real-IP")) {
        std::string xri = req.get_header_value("X-Real-IP");
        size_t first = xri.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            size_t last = xri.find_last_not_of(" \t\r\n");
            std::string ip = xri.substr(first, (last - first + 1));
            if (!ip.empty()) {
                return ip;
            }
        }
    }
    return req.remote_addr;
}

bool isLoopbackIp(const std::string &ip)
{
    return ip.empty() ||
           ip == "127.0.0.1" ||
           ip == "::1" ||
           ip == "::ffff:127.0.0.1" ||
           ip == "localhost" ||
           ip == "unknown";
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
    case RoomCode::ROBOTS_NOT_ALLOWED:
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
    j["location"] = p.location;
    j["bio"] = p.bio;
    j["elo_2p"] = p.elo2p;
    j["elo_4p"] = p.elo4p;
    j["elo_ofk"] = p.eloOfk;
    j["games_played"] = p.gamesPlayed;
    j["wins"] = p.wins;
    j["losses"] = p.losses;
    j["draws"] = p.draws;
    j["is_admin"] = p.isAdmin;
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
        j["email"] = ids[i].providerEmail;
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
    j["allow_robots"] = c.allowRobots;
    j["observer_policy"] = toString(c.observerPolicy);
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
        !fieldBool(b, "allow_robots", false, cfg.allowRobots, err) ||
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

std::string generateLeaseId()
{
    unsigned char b[16];
    if (RAND_bytes(b, sizeof(b)) != 1) {
        return std::string(32, '0');
    }
    static const char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(32);
    for (size_t i = 0; i < sizeof(b); ++i) {
        out.push_back(hex[(b[i] >> 4) & 0x0F]);
        out.push_back(hex[b[i] & 0x0F]);
    }
    return out;
}

struct StreamState {
    std::shared_ptr<SlotGuard> slot;
    PollFn fetch;
    int64_t cursor = 0;
    std::string token;          // empty for anonymous streams
    int64_t playerId = 0;
    std::string clientType;
    int64_t startedMs = 0;
    int64_t lastWriteMs = 0;
    int64_t lastAuthMs = 0;
    bool first = true;
    bool gapSent = false;
    bool isRobot = false;
    int64_t robotId = 0;
    std::string leaseId;
    std::shared_ptr<void> robotStreamGuard;
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
    struct ActiveRobotLease {
        std::string leaseId;
        int64_t connectedAtMs = 0;
        int64_t lastSeenMs = 0;
        std::string clientIp;
        std::string clientType = "robot";
        int activeStreams = 0;
    };

    struct RobotStreamGuard {
        std::mutex &mutex;
        std::map<int64_t, ActiveRobotLease> &leases;
        int64_t robotId;
        RobotStreamGuard(std::mutex &m, std::map<int64_t, ActiveRobotLease> &l, int64_t rId)
            : mutex(m), leases(l), robotId(rId)
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto it = leases.find(robotId);
            if (it != leases.end()) {
                ++it->second.activeStreams;
            }
        }
        ~RobotStreamGuard()
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto it = leases.find(robotId);
            if (it != leases.end() && it->second.activeStreams > 0) {
                --it->second.activeStreams;
            }
        }
    };

    struct Auth {
        int64_t playerId = 0;
        std::string token;
        std::string clientType;
        bool isRobot = false;
        RobotRecord robotRecord;
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
    void maybeBindClientInstallId(const Request &req, int64_t playerId,
                                  const std::string &bodyInstallId = "");
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
    Json tableJson(const TableSummary &t);
    Json seatJson(const SeatView &s);
    Json roomViewJson(const RoomView &v);
    Json roomEventJson(const RoomEvent &e);
    Json lobbyEventJson(const LobbyEvent &e);
    Json gameSummaryJson(const GameRecord &g);
    Json buildRobotProfilePayload(const RobotRecord &r);

    /* streaming */
    bool wantsStream(const Request &req) const;
    void respondEvents(const Request &req, Response &res, const Auth &auth,
                       PollFn fetch);
    bool streamStep(StreamState &st, httplib::DataSink &sink);
    void waitWake(int ms);

    /* handlers */
    void handleHealth(const Request &req, Response &res);
    void handleAvatarGet(const Request &req, Response &res);
    void handlePrivacy(const Request &req, Response &res);
    void handleDataDeletion(const Request &req, Response &res);
    void handleAuthAuthorize(const Request &req, Response &res);
    void handleAuthCallback(const Request &req, Response &res);
    void handleProviders(const Request &req, Response &res);
    void handleSso(const Request &req, Response &res);
    void handleLink(const Request &req, Response &res);
    void handleUnlink(const Request &req, Response &res);
    void handleAuthListIdentities(const Request &req, Response &res);
    void handleAuthUnlinkProvider(const Request &req, Response &res);
    void handleLogout(const Request &req, Response &res);
    void handleProfileGet(const Request &req, Response &res);
    void handleProfilePatch(const Request &req, Response &res);
    void handlePlayer(const Request &req, Response &res);
    void handleUsersList(const Request &req, Response &res);
    void handleUserGet(const Request &req, Response &res);
    void handleRobotsList(const Request &req, Response &res);
    void handleRobotsDirectory(const Request &req, Response &res);
    void handleRobotProfileGet(const Request &req, Response &res);
    void handleRobotCreate(const Request &req, Response &res);
    void handleRobotPatch(const Request &req, Response &res);
    void handleRobotDelete(const Request &req, Response &res);
    void handleRobotsConnect(const Request &req, Response &res);
    void handleRobotsDisconnect(const Request &req, Response &res);
    void handleRobotsExportConf(const Request &req, Response &res);
    void handleLeaderboard(const Request &req, Response &res);
    void handleLobbyState(const Request &req, Response &res);
    void handleLobbyChat(const Request &req, Response &res);
    void handleLobbyEvents(const Request &req, Response &res);
    void handleTables(const Request &req, Response &res);
    void handleTableCreate(const Request &req, Response &res);
    void handleTableJoin(const Request &req, Response &res);
    void handleTableLeave(const Request &req, Response &res);
    void handleTablePurge(const Request &req, Response &res);
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
    void handleBan(const Request &req, Response &res);
    void handleMute(const Request &req, Response &res);
    void handleSettings(const Request &req, Response &res);
    void handleGameEvents(const Request &req, Response &res);
    void handleArchiveUpload(const Request &req, Response &res);
    void handleArchiveDeleteInstall(const Request &req, Response &res);
    void handleArchivePurgeGame(const Request &req, Response &res);
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
    void handleFavicon(const Request &req, Response &res);
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

    std::mutex _robotLeaseMutex;
    std::map<int64_t, ActiveRobotLease> _activeRobotLeases;
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
    int64_t lastReconcile = 0;
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
            if (lastReconcile == 0 || (now - lastReconcile >= cfg.reconcileIntervalSec)) {
                store.reconcileAnonymousGames();
                lastReconcile = now;
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
    if (playerId == 0) {
        return std::string();
    }
    if (isRobotActorId(playerId)) {
        int64_t robotId = robotIdFromActorId(playerId);
        {
            std::lock_guard<std::mutex> lock(cacheMutex);
            std::map<int64_t, std::string>::const_iterator it =
                uuidById.find(playerId);
            if (it != uuidById.end()) {
                return it->second;
            }
        }
        RobotRecord r;
        if (!store.getRobotById(robotId, r)) {
            return std::string();
        }
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (uuidById.size() >= kMaxCacheEntries) {
            uuidById.clear();
            idByUuid.clear();
        }
        uuidById[playerId] = r.robotUuid;
        idByUuid[r.robotUuid] = playerId;
        return r.robotUuid;
    }
    if (playerId < 0) {
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
    if (store.getProfileByUuid(uuid, p)) {
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
    RobotRecord r;
    if (store.getRobotByUuid(uuid, r)) {
        int64_t actorId = makeRobotActorId(r.robotId);
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (uuidById.size() >= kMaxCacheEntries) {
            uuidById.clear();
            idByUuid.clear();
        }
        uuidById[actorId] = r.robotUuid;
        idByUuid[r.robotUuid] = actorId;
        playerId = actorId;
        return true;
    }
    return false;
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

void OctiHttpServer::Impl::maybeBindClientInstallId(const Request &req,
                                                    int64_t playerId,
                                                    const std::string &bodyInstallId)
{
    if (playerId <= 0) {
        return;
    }
    std::string candidateId;
    if (req.has_header("X-Octi-Install-Id")) {
        std::string h = req.get_header_value("X-Octi-Install-Id");
        if (isSafeId(h)) {
            candidateId = h;
        }
    }
    if (candidateId.empty() && !bodyInstallId.empty() && isSafeId(bodyInstallId)) {
        candidateId = bodyInstallId;
    }
    if (!candidateId.empty()) {
        store.bindInstallToPlayer(candidateId, playerId);
    }
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
    if (token.rfind("octi_bot_", 0) == 0) {
        RobotTokenValidation val = store.validateRobotToken(token);
        if (!val.valid || val.robot.revoked) {
            res.set_header("WWW-Authenticate", "Bearer error=\"invalid_token\"");
            sendError(res, 401, "UNAUTHORIZED", "invalid or revoked robot token");
            return false;
        }
        RobotRecord robot = val.robot;
        if (req.has_header("X-Octi-Robot-Lease")) {
            std::string leaseHeader = req.get_header_value("X-Octi-Robot-Lease");
            std::lock_guard<std::mutex> lock(_robotLeaseMutex);
            auto it = _activeRobotLeases.find(robot.robotId);
            if (it != _activeRobotLeases.end() && !it->second.leaseId.empty() &&
                it->second.leaseId != leaseHeader) {
                sendError(res, 409, "ROBOT_TOKEN_SUPERSEDED",
                          "robot connection lease has been superseded by a newer connection");
                return false;
            }
        }

        const std::string &path = req.path;
        if (path.rfind("/api/v1/auth", 0) == 0) {
            sendError(res, 403, "FORBIDDEN_ROBOT",
                      "robots cannot access human authentication endpoints");
            return false;
        }
        if (path == "/api/v1/robots" || path.rfind("/api/v1/robots/", 0) == 0) {
            if (path != "/api/v1/robots/connect" &&
                path != "/api/v1/robots/disconnect" &&
                path != "/api/v1/robots/directory" &&
                path.rfind("/api/v1/robots/profile/", 0) != 0) {
                sendError(res, 403, "FORBIDDEN_ROBOT",
                          "robots cannot manage robot registrations");
                return false;
            }
        }
        if (path.rfind("/api/v1/profile", 0) == 0 && req.method == "PATCH") {
            sendError(res, 403, "FORBIDDEN_ROBOT",
                      "robots cannot update human profiles");
            return false;
        }

        auth.isRobot = true;
        auth.robotRecord = robot;
        auth.playerId = makeRobotActorId(robot.robotId);
        auth.token = token;
        auth.clientType = "robot";

        {
            std::lock_guard<std::mutex> lock(_robotLeaseMutex);
            auto it = _activeRobotLeases.find(robot.robotId);
            if (it != _activeRobotLeases.end()) {
                it->second.lastSeenMs = steadyMs();
            }
        }
        rooms.recordClientHeartbeat(auth.playerId, "robot", true);
        return true;
    }

    SessionValidation sv = store.validateSession(token);
    if (!sv.valid) {
        res.set_header("WWW-Authenticate", "Bearer error=\"invalid_token\"");
        sendError(res, 401, "UNAUTHORIZED", "invalid or expired session");
        return false;
    }
    auth.isRobot = false;
    auth.playerId = sv.playerId;
    auth.token = token;
    auth.clientType = detectClientType(req);
    maybeBindClientInstallId(req, auth.playerId);
    return true;
}

/* No header: anonymous viewer (player id 0). A bad header is still a 401. */
bool OctiHttpServer::Impl::optionalAuth(const Request &req, Response &res,
                                        Auth &auth)
{
    if (!req.has_header("Authorization")) {
        auth = Auth();
        auth.clientType = detectClientType(req);
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
    if (provider == "facebook" && accessToken.empty() && !idToken.empty()) {
        int dots = 0;
        for (size_t i = 0; i < idToken.size(); ++i) {
            if (idToken[i] == '.') {
                ++dots;
            }
        }
        if (dots < 2) {
            accessToken = idToken;
            idToken.clear();
        }
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
    j["occupied"] = (s.playerId != 0);
    j["player_uuid"] = s.playerId != 0 ? Json(uuidOf(s.playerId)) : Json();
    j["handle"] = s.handle;
    j["rating"] = s.rating;
    j["connected"] = s.connected;
    j["disconnected_for_ms"] = s.disconnectedForMs;
    j["forfeit_claimable"] = s.forfeitClaimable;
    j["clock_ms"] = s.clockMs;
    j["eliminated"] = s.eliminated;
    j["is_robot"] = s.isRobot;
    if (s.isRobot) {
        j["robot_uuid"] = s.robotUuid;
        j["robot_description"] = s.robotDescription;
        j["owner_player_id"] = s.ownerPlayerId;
        j["owner_uuid"] = s.ownerUuid;
        j["owner_handle"] = s.ownerHandle;
        j["avatar_url"] = s.avatarUrl;
        j["robot_games_played"] = s.robotGamesPlayed;
        j["robot_wins"] = s.robotWins;
        j["robot_losses"] = s.robotLosses;
        j["robot_draws"] = s.robotDraws;
    }
    return j;
}

Json OctiHttpServer::Impl::roomViewJson(const RoomView &v)
{
    Json j = configJson(v.config);
    j["ok"] = true;
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
    j["observer_handles"] = v.observerHandles;
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
    j["seq"] = v.latestSeq;
    j["archived"] = v.archived;
    j["rematch_room_id"] = v.rematchRoomId;
    j["created_at_ms"] = v.createdAtMs;
    j["last_move_at_ms"] = v.lastMoveAtMs;
    j["idle_ms"] = v.idleMs;
    j["ttl_remaining_ms"] = v.ttlRemainingMs;
    return j;
}

Json OctiHttpServer::Impl::roomEventJson(const RoomEvent &e)
{
    Json j;
    j["seq"] = e.seq;
    j["type"] = toString(e.type);
    j["at_ms"] = e.atMs;
    j["is_robot"] = e.isRobot;
    if (e.isRobot) {
        j["robot_uuid"] = e.robotUuid;
        j["robot_description"] = e.robotDescription;
        j["owner_player_id"] = e.ownerPlayerId;
        j["owner_uuid"] = e.ownerUuid;
        j["owner_handle"] = e.ownerHandle;
        j["avatar_url"] = e.avatarUrl;
    }
    if (e.playerId != 0) {
        j["player_uuid"] = uuidOf(e.playerId);
        j["handle"] = e.handle;
    }
    if (e.targetPlayerId != 0) {
        j["target_uuid"] = uuidOf(e.targetPlayerId);
        if (!e.targetHandle.empty()) {
            j["target_handle"] = e.targetHandle;
        }
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
    case RoomEventType::OBSERVER_BANNED:
        j["reason"] = e.text;
        break;
    case RoomEventType::USER_MUTED:
        j["mute_chat"] = e.detail.find("mute_chat=1") != std::string::npos;
        break;
    case RoomEventType::POLICY_UPDATED: {
        const std::string op = "observer_policy=";
        if (e.detail.compare(0, op.size(), op) == 0) {
            size_t sep = e.detail.find(';');
            j["observer_policy"] = sep != std::string::npos
                                       ? e.detail.substr(op.size(), sep - op.size())
                                       : e.detail.substr(op.size());
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

Json OctiHttpServer::Impl::tableJson(const TableSummary &t)
{
    Json j = configJson(t.config);
    j["room_id"] = t.roomId;
    j["phase"] = toString(t.phase);
    j["has_passcode"] = t.hasPasscode;
    j["host_uuid"] = t.hostPlayerId != 0 ? uuidOf(t.hostPlayerId) : "";
    j["host_handle"] = t.hostHandle;
    j["created_at_ms"] = t.createdAtMs;
    j["seats_total"] = t.seatsTotal;
    j["seats_filled"] = t.seatsFilled;
    Json seatsArr = Json::array();
    for (size_t i = 0; i < t.seats.size(); ++i) {
        const TableSeatSummary &s = t.seats[i];
        Json sj;
        sj["seat"] = s.seat;
        sj["occupied"] = (s.playerId != 0);
        sj["player_uuid"] = s.playerId != 0 ? uuidOf(s.playerId) : "";
        sj["handle"] = s.handle;
        sj["rating"] = s.rating;
        sj["connected"] = s.connected;
        sj["is_robot"] = s.isRobot;
        if (s.isRobot) {
            sj["robot_uuid"] = s.robotUuid;
            sj["robot_description"] = s.robotDescription;
            sj["owner_player_id"] = s.ownerPlayerId;
            sj["owner_uuid"] = s.ownerUuid;
            sj["owner_handle"] = s.ownerHandle;
            sj["avatar_url"] = s.avatarUrl;
            sj["robot_games_played"] = s.robotGamesPlayed;
            sj["robot_wins"] = s.robotWins;
            sj["robot_losses"] = s.robotLosses;
            sj["robot_draws"] = s.robotDraws;
        }
        seatsArr.push_back(sj);
    }
    if (seatsArr.empty() && t.seatsTotal > 0) {
        for (int i = 1; i <= t.seatsTotal; ++i) {
            Json sj;
            sj["seat"] = i;
            sj["occupied"] = false;
            sj["player_uuid"] = "";
            sj["handle"] = "";
            sj["rating"] = 1500;
            sj["connected"] = false;
            sj["is_robot"] = false;
            seatsArr.push_back(sj);
        }
    }
    j["seats"] = seatsArr;
    j["players"] = t.playerHandles;
    j["observer_count"] = t.observerCount;
    j["observer_handles"] = t.observerHandles;
    j["last_move_at_ms"] = t.lastMoveAtMs;
    j["idle_ms"] = t.idleMs;
    j["ttl_remaining_ms"] = t.ttlRemainingMs;
    return j;
}

Json OctiHttpServer::Impl::lobbyEventJson(const LobbyEvent &e)
{
    Json j;
    j["seq"] = e.seq;
    j["type"] = toString(e.type);
    int64_t tsMs = (e.createdAtMs > 0) ? e.createdAtMs : e.atMs;
    j["at_ms"] = tsMs;
    j["created_at_ms"] = tsMs;
    j["is_robot"] = e.isRobot;
    if (e.isRobot) {
        j["robot_uuid"] = e.robotUuid;
        j["robot_description"] = e.robotDescription;
        j["owner_player_id"] = e.ownerPlayerId;
        j["owner_uuid"] = e.ownerUuid;
        j["owner_handle"] = e.ownerHandle;
        j["avatar_url"] = e.avatarUrl;
    }
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
    const int64_t robotIds[4] = {g.p1RobotId, g.p2RobotId, g.p3RobotId,
                                 g.p4RobotId};
    const std::string *handles[4] = {&g.p1Handle, &g.p2Handle, &g.p3Handle,
                                     &g.p4Handle};
    for (int i = 0; i < 4; ++i) {
        if (handles[i]->empty()) {
            continue;
        }
        Json p;
        p["seat"] = i + 1;
        p["handle"] = *handles[i];
        if (robotIds[i] > 0) {
            p["is_robot"] = true;
            RobotRecord rRec;
            if (store.getRobotById(robotIds[i], rRec)) {
                p["robot_uuid"] = rRec.robotUuid;
            }
        } else if (ids[i] != 0) {
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
        if (st.isRobot) {
            RobotTokenValidation val = store.validateRobotToken(st.token);
            if (!val.valid || val.robot.revoked) {
                putText(sink, "event: closed\ndata: {\"error\":\"UNAUTHORIZED\"}\n\n");
                sink.done();
                return true;
            }
            if (!st.leaseId.empty()) {
                std::lock_guard<std::mutex> lock(_robotLeaseMutex);
                auto it = _activeRobotLeases.find(st.robotId);
                if (it != _activeRobotLeases.end() && !it->second.leaseId.empty() &&
                    it->second.leaseId != st.leaseId) {
                    putText(sink, "event: closed\ndata: {\"error\":\"ROBOT_TOKEN_SUPERSEDED\"}\n\n");
                    sink.done();
                    return true;
                }
                if (it != _activeRobotLeases.end()) {
                    it->second.lastSeenMs = steadyMs();
                }
            }
        } else {
            SessionValidation sv = store.validateSession(st.token);
            if (!sv.valid || sv.playerId != st.playerId) {
                putText(sink, "event: closed\ndata: {\"error\":\"UNAUTHORIZED\"}\n\n");
                sink.done();
                return true;
            }
        }
        rooms.recordClientHeartbeat(st.playerId, st.clientType, false);
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
    st->clientType = auth.clientType;
    if (auth.isRobot) {
        st->isRobot = true;
        st->robotId = auth.robotRecord.robotId;
        if (req.has_header("X-Octi-Robot-Lease")) {
            st->leaseId = req.get_header_value("X-Octi-Robot-Lease");
        } else {
            std::lock_guard<std::mutex> lock(_robotLeaseMutex);
            auto it = _activeRobotLeases.find(st->robotId);
            if (it != _activeRobotLeases.end()) {
                st->leaseId = it->second.leaseId;
            }
        }
        st->robotStreamGuard = std::make_shared<RobotStreamGuard>(
            _robotLeaseMutex, _activeRobotLeases, st->robotId);
    }
    if (auth.playerId != 0) {
        rooms.recordClientHeartbeat(auth.playerId, auth.clientType, false);
    }

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

void OctiHttpServer::Impl::handleAvatarGet(const Request &req, Response &res)
{
    auto it = req.path_params.find("file");
    if (it == req.path_params.end() || it->second.empty()) {
        sendError(res, 404, "NOT_FOUND", "Avatar not found");
        return;
    }
    std::string bytes;
    std::string mime;
    if (!store.readAvatarFile(it->second, bytes, mime)) {
        sendError(res, 404, "NOT_FOUND", "Avatar not found");
        return;
    }
    res.status = 200;
    res.set_header("Cache-Control", "public, max-age=86400");
    res.set_content(bytes, mime.c_str());
}

void OctiHttpServer::Impl::handlePrivacy(const Request &, Response &res)
{
    res.status = 200;
    res.set_header("Content-Type", "text/html; charset=utf-8");
    res.set_header("Cache-Control", "public, max-age=86400");
    res.set_content(
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<meta charset=\"utf-8\">\n"
        "<title>OCTI - Privacy &amp; Data Deletion Policy</title>\n"
        "<style>body{font-family:sans-serif;max-width:800px;margin:40px auto;line-height:1.6;padding:0 20px;color:#333;}\n"
        "h1,h2{color:#111;}code{background:#f4f4f4;padding:2px 6px;border-radius:4px;}</style>\n"
        "</head>\n"
        "<body>\n"
        "<h1>OCTI Privacy &amp; Data Deletion Policy</h1>\n"
        "<p>Last updated: 2026</p>\n"
        "<h2>1. Data Collection &amp; Use</h2>\n"
        "<p>OCTI collects minimal public profile information through single sign-on (Google, Apple, Microsoft, Facebook, LINE), "
        "including your display name, email, avatar URL, and provider-specific subject identifier. Passwords are never collected or stored.</p>\n"
        "<h2>2. Game Telemetry &amp; Analytics</h2>\n"
        "<p>Online and uploaded game transcripts (board moves, game results, ELO ratings) are archived for leaderboards, replays, and AI research under CC-BY-4.0.</p>\n"
        "<h2>3. Data Deletion &amp; Account Unlinking</h2>\n"
        "<p>Players can unlink single sign-on identities via the in-app Account Settings tab or delete client-uploaded transcripts by calling the telemetry erasure endpoint with their install ID. "
        "To request complete deletion of your account and associated profile data, contact samurai@selfso.com.</p>\n"
        "</body>\n"
        "</html>\n",
        "text/html; charset=utf-8");
}

void OctiHttpServer::Impl::handleDataDeletion(const Request &req, Response &res)
{
    std::string code = AccountStore::sha256Hex(std::to_string(epochSec()) + ":" + req.body).substr(0, 12);
    Json j;
    j["url"] = "https://octi.selfso.com/privacy/?confirmation_code=" + code;
    j["confirmation_code"] = code;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleAuthAuthorize(const Request &req, Response &res)
{
    std::string provider = req.get_param_value("provider");
    if (provider != "google" && provider != "facebook") {
        sendError(res, 400, "UNSUPPORTED_PROVIDER", "unsupported provider");
        return;
    }

    if (!verifier.isProviderEnabled(provider)) {
        sendError(res, 503, "SSO_PROVIDER_NOT_CONFIGURED", "SSO provider is not enabled");
        return;
    }

    std::map<std::string, std::vector<std::string>>::const_iterator it =
        cfg.providerClientIds.find(provider);
    if (it == cfg.providerClientIds.end() || it->second.empty() || it->second[0].empty()) {
        sendError(res, 503, "SSO_PROVIDER_NOT_CONFIGURED", "SSO provider client_id not configured");
        return;
    }
    const std::string &clientId = it->second[0];

    std::string redirectUri = "/#account";
    if (req.has_param("redirect_uri")) {
        redirectUri = req.get_param_value("redirect_uri");
    }

    bool redirectValid = false;
    if (!redirectUri.empty() && redirectUri[0] == '/' && (redirectUri.size() == 1 || redirectUri[1] != '/')) {
        redirectValid = true;
    } else if (redirectUri == "octi://auth/callback" || redirectUri == "octi://callback") {
        redirectValid = true;
    } else if (redirectUri.rfind("http://127.0.0.1:", 0) == 0 || redirectUri.rfind("http://localhost:", 0) == 0) {
        size_t colon = redirectUri.find(':', 7);
        if (colon != std::string::npos) {
            size_t slash = redirectUri.find('/', colon);
            if (slash != std::string::npos) {
                std::string portStr = redirectUri.substr(colon + 1, slash - (colon + 1));
                int64_t portNum = 0;
                if (parseUnsignedStrict(portStr, 65535, portNum) && portNum >= 1) {
                    std::string pathPart = redirectUri.substr(slash);
                    if (pathPart == "/callback" || pathPart == "/oauth/callback") {
                        redirectValid = true;
                    }
                }
            }
        }
    }
    if (!redirectValid) {
        sendError(res, 400, "INVALID_REDIRECT_URI", "invalid redirect_uri");
        return;
    }

    int64_t now = epochSec();
    std::string host = req.get_header_value("Host");
    if (host.empty()) {
        host = cfg.bindAddress + ":" + std::to_string(boundPort.load());
    }
    std::string proto = req.get_header_value("X-Forwarded-Proto");
    if (proto.empty()) {
        proto = (host.find("selfso.com") != std::string::npos) ? "https" : "http";
    }
    std::string callbackUrl = proto + "://" + host + "/api/v1/auth/callback";

    Json stateObj;
    stateObj["provider"] = provider;
    stateObj["redirect_uri"] = redirectUri;
    stateObj["callback_url"] = callbackUrl;
    stateObj["nonce"] = AccountStore::sha256Hex(std::to_string(steadyMs()) + ":" + redirectUri);
    stateObj["exp"] = now + 600;

    std::string statePayload = stateObj.dump();
    std::string stateB64 = base64UrlEncode(statePayload);
    std::string sig = hmacSha256(stateB64, kOauthStateSecret);
    std::string sigB64 = base64UrlEncode(sig);
    std::string stateParam = stateB64 + "." + sigB64;

    std::string authUrl;
    if (provider == "google") {
        authUrl = "https://accounts.google.com/o/oauth2/v2/auth?client_id=" +
                  urlEncode(clientId) +
                  "&response_type=code&scope=openid%20email%20profile&redirect_uri=" +
                  urlEncode(callbackUrl) + "&state=" + urlEncode(stateParam);
    } else {
        authUrl = "https://www.facebook.com/v19.0/dialog/oauth?client_id=" +
                  urlEncode(clientId) +
                  "&response_type=code&scope=public_profile,email&redirect_uri=" +
                  urlEncode(callbackUrl) + "&state=" + urlEncode(stateParam);
    }

    res.status = 302;
    res.set_header("Location", authUrl);
    res.set_header("Cache-Control", "no-store");
}

void OctiHttpServer::Impl::handleAuthCallback(const Request &req, Response &res)
{
    std::string stateParam = req.get_param_value("state");
    size_t dot = stateParam.find('.');
    if (dot == std::string::npos) {
        sendError(res, 400, "INVALID_OAUTH_STATE", "invalid state parameter");
        return;
    }

    std::string stateB64 = stateParam.substr(0, dot);
    std::string sigB64 = stateParam.substr(dot + 1);
    std::string sig;
    if (!base64UrlDecode(sigB64, sig)) {
        sendError(res, 400, "INVALID_OAUTH_STATE", "malformed state signature");
        return;
    }
    std::string expectedSig = hmacSha256(stateB64, kOauthStateSecret);
    if (sig != expectedSig) {
        sendError(res, 400, "INVALID_OAUTH_STATE", "state signature mismatch");
        return;
    }

    std::string payloadJson;
    if (!base64UrlDecode(stateB64, payloadJson)) {
        sendError(res, 400, "INVALID_OAUTH_STATE", "malformed state payload");
        return;
    }
    Json stateObj = Json::parse(payloadJson, nullptr, false);
    if (stateObj.is_discarded() || !stateObj.is_object()) {
        sendError(res, 400, "INVALID_OAUTH_STATE", "invalid state JSON");
        return;
    }

    int64_t exp = stateObj.value("exp", 0LL);
    if (exp < epochSec()) {
        sendError(res, 400, "INVALID_OAUTH_STATE", "state has expired");
        return;
    }

    std::string provider = stateObj.value("provider", "");
    std::string redirectUri = stateObj.value("redirect_uri", "/#account");
    std::string callbackUrl = stateObj.value("callback_url", "https://octi.selfso.com/api/v1/auth/callback");

    std::string idToken = req.get_param_value("id_token");
    std::string accessToken = req.get_param_value("access_token");
    std::string code = req.get_param_value("code");

    if (!code.empty() && idToken.empty() && accessToken.empty()) {
        int dots = 0;
        for (size_t i = 0; i < code.size(); ++i) {
            if (code[i] == '.') {
                ++dots;
            }
        }
        if (dots >= 2) {
            idToken = code;
        } else {
            std::string clientId;
            std::map<std::string, std::vector<std::string>>::const_iterator cit =
                cfg.providerClientIds.find(provider);
            if (cit != cfg.providerClientIds.end() && !cit->second.empty()) {
                clientId = cit->second[0];
            }
            std::string clientSecret;
            std::map<std::string, std::string>::const_iterator sit =
                cfg.providerClientSecrets.find(provider);
            if (sit != cfg.providerClientSecrets.end()) {
                clientSecret = sit->second;
            }
            if (!clientId.empty() && !clientSecret.empty()) {
                if (provider == "google") {
                    httplib::SSLClient gcli("oauth2.googleapis.com", 443);
                    gcli.enable_server_certificate_verification(true);
                    gcli.set_connection_timeout(5, 0);
                    gcli.set_read_timeout(5, 0);
                    httplib::Params params{
                        {"code", code},
                        {"client_id", clientId},
                        {"client_secret", clientSecret},
                        {"redirect_uri", callbackUrl},
                        {"grant_type", "authorization_code"}
                    };
                    if (auto gres = gcli.Post("/token", params)) {
                        Json tj = Json::parse(gres->body, nullptr, false);
                        if (tj.is_object() && tj.contains("id_token") && tj["id_token"].is_string()) {
                            idToken = tj["id_token"].get<std::string>();
                        }
                    }
                } else if (provider == "facebook") {
                    httplib::SSLClient fbcli("graph.facebook.com", 443);
                    fbcli.enable_server_certificate_verification(true);
                    fbcli.set_connection_timeout(5, 0);
                    fbcli.set_read_timeout(5, 0);
                    std::string fbPath = "/v19.0/oauth/access_token?client_id=" + urlEncode(clientId) +
                                         "&redirect_uri=" + urlEncode(callbackUrl) +
                                         "&client_secret=" + urlEncode(clientSecret) +
                                         "&code=" + urlEncode(code);
                    if (auto fbres = fbcli.Get(fbPath.c_str())) {
                        Json tj = Json::parse(fbres->body, nullptr, false);
                        if (tj.is_object() && tj.contains("access_token") && tj["access_token"].is_string()) {
                            accessToken = tj["access_token"].get<std::string>();
                        }
                    }
                }
            }
        }
    }

    if (provider == "facebook" && accessToken.empty() && !idToken.empty()) {
        int dots = 0;
        for (size_t i = 0; i < idToken.size(); ++i) {
            if (idToken[i] == '.') {
                ++dots;
            }
        }
        if (dots < 2) {
            accessToken = idToken;
            idToken.clear();
        }
    }

    VerifyResult vr;
    if (!accessToken.empty()) {
        if (provider != "facebook") {
            sendError(res, 400, "INVALID_ARGUMENT", "access_token is only accepted for facebook");
            return;
        }
        vr = verifier.verifyFacebookAccessToken(accessToken);
    } else if (!idToken.empty()) {
        vr = verifier.verifyIdToken(provider, idToken);
    } else if (!code.empty()) {
        vr = verifier.verifyIdToken(provider, code);
    } else {
        sendError(res, 400, "INVALID_ARGUMENT", "missing id_token or access_token");
        return;
    }

    if (!vr.ok()) {
        sendError(res, statusForVerify(vr.status), verifyStatusName(vr.status), vr.detail);
        return;
    }

    LoginResult lr = store.loginWithSso(vr.claims, "octi-oauth-broker/1.0");
    if (!lr.ok) {
        sendError(res, 500, "INTERNAL_ERROR", "sign-in failed");
        return;
    }
    rooms.enterLobby(lr.profile.playerId);
    rooms.recordClientHeartbeat(lr.profile.playerId, "web", true);

    std::string targetLoc;
    std::string isNewStr = lr.created ? "true" : "false";
    if (redirectUri.rfind("http://127.0.0.1:", 0) == 0 ||
        redirectUri.rfind("http://localhost:", 0) == 0 ||
        redirectUri.rfind("octi://", 0) == 0) {
        std::string rawTok = !idToken.empty() ? idToken : (!accessToken.empty() ? accessToken : code);
        targetLoc = redirectUri + (redirectUri.find('?') != std::string::npos ? "&" : "?") +
                    "id_token=" + urlEncode(rawTok) +
                    "&auth_token=" + urlEncode(lr.sessionToken) +
                    "&provider=" + urlEncode(provider) +
                    "&is_new_account=" + isNewStr +
                    "&handle=" + urlEncode(lr.profile.handle) +
                    "&player_uuid=" + urlEncode(lr.profile.uuid);
    } else {
        targetLoc = "/#auth_token=" + urlEncode(lr.sessionToken) +
                    "&provider=" + urlEncode(provider) +
                    "&is_new_account=" + isNewStr;
    }

    res.status = 302;
    res.set_header("Location", targetLoc);
    res.set_header("Cache-Control", "no-store");
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
    std::string bodyInstallId;
    if (body.contains("install_id") && body["install_id"].is_string()) {
        bodyInstallId = body["install_id"].get<std::string>();
    }
    maybeBindClientInstallId(req, lr.profile.playerId, bodyInstallId);
    rooms.enterLobby(lr.profile.playerId);
    std::string cType = detectClientType(req);
    if (cType.empty()) {
        std::string lAgent = lower(agent);
        if (lAgent.find("android") != std::string::npos) {
            cType = "android";
        } else if (lAgent.find("desktop") != std::string::npos) {
            cType = "desktop";
        } else if (lAgent.find("web") != std::string::npos ||
                   lAgent.find("mozilla") != std::string::npos) {
            cType = "web";
        }
    }
    rooms.recordClientHeartbeat(lr.profile.playerId, cType, true);
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
    bool forceRelink = false;
    std::string err;
    fieldBool(body, "force_relink", false, forceRelink, err);
    LinkResult lr = store.linkIdentity(auth.playerId, claims, forceRelink);
    switch (lr.status) {
    case LinkStatus::LINKED:
    case LinkStatus::ALREADY_LINKED_SELF: {
        std::string bodyInstallId;
        if (body.contains("install_id") && body["install_id"].is_string()) {
            bodyInstallId = body["install_id"].get<std::string>();
        }
        maybeBindClientInstallId(req, auth.playerId, bodyInstallId);
        std::vector<LinkedIdentity> ids;
        store.listLinkedIdentities(auth.playerId, ids);
        Json j;
        j["ok"] = true;
        j["linked"] = lr.status == LinkStatus::LINKED;
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

void OctiHttpServer::Impl::handleAuthListIdentities(const Request &req,
                                                   Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    std::vector<LinkedIdentity> ids;
    if (!store.listLinkedIdentities(auth.playerId, ids)) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to list identities");
        return;
    }
    Json j;
    j["ok"] = true;
    j["identities"] = identitiesJson(ids);
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleAuthUnlinkProvider(const Request &req,
                                                   Response &res)
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
        j["ok"] = true;
        j["status"] = "unlinked";
        j["linked_identities"] = identitiesJson(ids);
        sendJson(res, 200, j);
        return;
    }
    case UnlinkStatus::LAST_IDENTITY_FORBIDDEN:
        sendError(res, 409, "cannot_unlink_last_identity",
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
    if (!auth.clientType.empty()) {
        rooms.clearClientPresence(auth.playerId, auth.clientType);
    }
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
    if (auth.isRobot) {
        Json j;
        j["uuid"] = auth.robotRecord.robotUuid;
        j["handle"] = auth.robotRecord.robotName;
        j["display_name"] = auth.robotRecord.robotName;
        j["avatar_url"] = auth.robotRecord.ownerAvatarUrl;
        j["bio"] = auth.robotRecord.description;
        j["is_robot"] = true;
        j["robot_uuid"] = auth.robotRecord.robotUuid;
        j["robot_name"] = auth.robotRecord.robotName;
        j["description"] = auth.robotRecord.description;
        j["owner_uuid"] = auth.robotRecord.ownerUuid;
        j["owner_handle"] = auth.robotRecord.ownerHandle;
        j["owner_avatar_url"] = auth.robotRecord.ownerAvatarUrl;
        j["games_played"] = auth.robotRecord.gamesPlayed;
        j["wins"] = auth.robotRecord.wins;
        j["losses"] = auth.robotRecord.losses;
        j["draws"] = auth.robotRecord.draws;
        j["created_at"] = auth.robotRecord.createdAt;
        j["linked_identities"] = Json::array();
        sendJson(res, 200, j);
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
    if (fieldPresent(body, "bio")) {
        up.setBio = true;
        if (!fieldString(body, "bio", true, up.bio, err)) {
            sendError(res, 400, "INVALID_ARGUMENT", err);
            return;
        }
    }
    if (fieldPresent(body, "location")) {
        up.setLocation = true;
        if (!fieldString(body, "location", true, up.location, err)) {
            sendError(res, 400, "INVALID_ARGUMENT", err);
            return;
        }
    }
    if (!up.setHandle && !up.setDisplayName && !up.setAvatarUrl && !up.setBio &&
        !up.setLocation) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "nothing to update: send handle, display_name, avatar_url, bio or location");
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
        std::vector<LinkedIdentity> ids;
        store.listLinkedIdentities(auth.playerId, ids);
        Json j = profileJson(p, true);
        j["linked_identities"] = identitiesJson(ids);
        sendJson(res, 200, j);
        return;
    }
    case UpdateStatus::NOT_FOUND:
        sendError(res, 401, "UNAUTHORIZED", "unknown player");
        return;
    case UpdateStatus::INVALID_HANDLE:
        sendError(res, 400, "INVALID_HANDLE",
                  "Handle must be 3 to 24 characters using only letters (A-Z, a-z), digits (0-9), underscores (_), or hyphens (-).");
        return;
    case UpdateStatus::HANDLE_TAKEN:
        sendError(res, 409, "HANDLE_TAKEN",
                  "That handle is already taken by another player. Please choose a different handle.");
        return;
    case UpdateStatus::HANDLE_RATE_LIMITED:
        sendError(res, 429, "HANDLE_CHANGE_RATE_LIMITED",
                  "Handle can only be changed once every 24 hours after initial setup. Please try again later.");
        return;
    case UpdateStatus::INVALID_DISPLAY_NAME:
        sendError(res, 400, "INVALID_DISPLAY_NAME",
                  "Display name must be 1 to 64 printable characters and cannot be blank.");
        return;
    case UpdateStatus::INVALID_AVATAR_URL:
        sendError(res, 400, "INVALID_AVATAR_URL",
                  "Avatar must be an uploaded image file or a valid https:// URL (up to 512 characters, no spaces).");
        return;
    case UpdateStatus::INVALID_BIO:
        sendError(res, 400, "INVALID_BIO",
                  "Bio must be 500 characters or fewer.");
        return;
    case UpdateStatus::INVALID_LOCATION:
        sendError(res, 400, "INVALID_LOCATION",
                  "Location must be 64 printable characters or fewer on a single line.");
        return;
    case UpdateStatus::ERROR:
        break;
    }
    sendError(res, 500, "INTERNAL_ERROR", "profile update failed");
}

void OctiHttpServer::Impl::handlePlayer(const Request &req, Response &res)
{
    handleUserGet(req, res);
}

void OctiHttpServer::Impl::handleUsersList(const Request &req, Response &res)
{
    UserDirectoryFilter filter;
    if (req.has_param("q")) {
        filter.query = req.get_param_value("q");
    }
    if (req.has_param("sort")) {
        filter.sortField = req.get_param_value("sort");
    }
    if (filter.sortField.empty()) {
        filter.sortField = "elo_2p";
    }
    if (req.has_param("order")) {
        std::string ord = lower(req.get_param_value("order"));
        if (ord == "asc") {
            filter.sortAsc = true;
        } else if (ord == "desc") {
            filter.sortAsc = false;
        }
    }
    int64_t limit = 50;
    if (req.has_param("limit")) {
        if (!parseUnsignedClamped(req.get_param_value("limit"), 100, limit) || limit < 1) {
            sendError(res, 400, "INVALID_ARGUMENT", "limit must be a positive integer");
            return;
        }
    }
    filter.limit = static_cast<int>(std::max<int64_t>(1, std::min<int64_t>(limit, 100)));

    int64_t offset = 0;
    if (req.has_param("offset")) {
        if (!parseUnsignedClamped(req.get_param_value("offset"), INT64_MAX, offset) || offset < 0) {
            sendError(res, 400, "INVALID_ARGUMENT", "offset must be a non-negative integer");
            return;
        }
    }
    filter.offset = static_cast<int>(offset);

    std::vector<UserDirectoryEntry> entries;
    int64_t total = 0;
    if (!store.listUsers(filter, entries, &total)) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to list users");
        return;
    }

    Json usersArr = Json::array();
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto &e = entries[i];
        Json uj;
        uj["uuid"] = e.profile.uuid;
        uj["handle"] = e.profile.handle;
        uj["display_name"] = e.profile.displayName;
        uj["avatar_url"] = e.profile.avatarUrl;
        uj["location"] = e.profile.location;
        uj["bio"] = e.profile.bio;
        uj["elo_2p"] = e.profile.elo2p;
        uj["elo_4p"] = e.profile.elo4p;
        uj["elo_ofk"] = e.profile.eloOfk;
        uj["games_played"] = e.profile.gamesPlayed;
        uj["wins"] = e.profile.wins;
        uj["losses"] = e.profile.losses;
        uj["draws"] = e.profile.draws;
        uj["active_robots_count"] = e.activeRobotsCount;
        uj["created_at"] = e.profile.createdAt;
        uj["last_seen_at"] = e.profile.lastSeenAt;
        usersArr.push_back(uj);
    }

    Json j;
    j["total"] = total;
    j["limit"] = filter.limit;
    j["offset"] = filter.offset;
    j["users"] = usersArr;
    sendJson(res, 200, j);
}

Json OctiHttpServer::Impl::buildRobotProfilePayload(const RobotRecord &r)
{
    Json rj;
    rj["is_robot"] = true;
    rj["uuid"] = r.robotUuid;
    rj["robot_uuid"] = r.robotUuid;
    rj["name"] = r.robotName;
    rj["handle"] = r.robotName;
    rj["robot_name"] = r.robotName;
    rj["display_name"] = r.robotName;
    rj["description"] = r.description;
    rj["bio"] = r.description;
    rj["avatar_url"] = r.ownerAvatarUrl;
    rj["owner_uuid"] = r.ownerUuid;
    rj["owner_handle"] = r.ownerHandle;
    rj["owner_avatar_url"] = r.ownerAvatarUrl;
    rj["elo"] = 1200;
    rj["games_played"] = r.gamesPlayed;
    rj["wins"] = r.wins;
    rj["losses"] = r.losses;
    rj["draws"] = r.draws;
    double wr = (r.gamesPlayed > 0)
                    ? std::round((static_cast<double>(r.wins) / r.gamesPlayed) *
                                 1000.0) /
                          10.0
                    : 0.0;
    rj["win_rate"] = wr;
    rj["revoked"] = r.revoked;
    rj["token_deleted"] = r.tokenDeleted;
    bool retired = (r.revoked || r.tokenDeleted);
    rj["retired"] = retired;
    rj["status"] = (!retired) ? "active" : "retired";
    rj["created_at"] = r.createdAt;
    rj["last_seen_at"] = 0;
    rj["robots"] = Json::array();

    ArchiveFilter af;
    af.robotId = r.robotId;
    af.robotUuid = r.robotUuid;
    af.limit = 50;
    af.offset = 0;
    std::vector<GameRecord> games;
    store.listArchivedGames(af, games, nullptr);

    Json gamesArr = Json::array();
    for (size_t i = 0; i < games.size(); ++i) {
        const auto &g = games[i];
        Json gj = gameSummaryJson(g);
        gj["p1_handle"] = g.p1Handle;
        gj["p2_handle"] = g.p2Handle;
        gj["p3_handle"] = g.p3Handle;
        gj["p4_handle"] = g.p4Handle;

        int mySeat = 0;
        if (r.robotId > 0) {
            if (g.p1RobotId == r.robotId) mySeat = 1;
            else if (g.p2RobotId == r.robotId) mySeat = 2;
            else if (g.p3RobotId == r.robotId) mySeat = 3;
            else if (g.p4RobotId == r.robotId) mySeat = 4;
        }
        if (mySeat == 0 && !r.robotName.empty()) {
            std::string rn = lower(r.robotName);
            if (lower(g.p1Handle) == rn) mySeat = 1;
            else if (lower(g.p2Handle) == rn) mySeat = 2;
            else if (lower(g.p3Handle) == rn) mySeat = 3;
            else if (lower(g.p4Handle) == rn) mySeat = 4;
        }

        std::string outcome = "DRAW";
        if (g.result == "DRAW" || (g.winnerSeat == 0 && g.winner == "NONE")) {
            outcome = "DRAW";
        } else if (g.result == "TEAM_13_WIN") {
            outcome = (mySeat == 1 || mySeat == 3) ? "WIN" : "LOSS";
        } else if (g.result == "TEAM_24_WIN") {
            outcome = (mySeat == 2 || mySeat == 4) ? "WIN" : "LOSS";
        } else if (g.winnerSeat > 0) {
            outcome = (mySeat == g.winnerSeat) ? "WIN" : "LOSS";
        } else if (g.winner == "A") {
            outcome = (mySeat == 1) ? "WIN" : "LOSS";
        } else if (g.winner == "B") {
            outcome = (mySeat == 2) ? "WIN" : "LOSS";
        } else if (g.winner == "C") {
            outcome = (mySeat == 3) ? "WIN" : "LOSS";
        } else if (g.winner == "D") {
            outcome = (mySeat == 4) ? "WIN" : "LOSS";
        }
        gj["player_outcome"] = outcome;
        gamesArr.push_back(gj);
    }
    rj["recent_games"] = gamesArr;
    return rj;
}

void OctiHttpServer::Impl::handleUserGet(const Request &req, Response &res)
{
    std::string id;
    std::unordered_map<std::string, std::string>::const_iterator it =
        req.path_params.find("id");
    if (it == req.path_params.end()) {
        it = req.path_params.find("uuid");
    }
    if (it == req.path_params.end() || !isSafeId(it->second)) {
        sendError(res, 404, "NOT_FOUND", "no such resource");
        return;
    }
    id = it->second;

    PlayerProfile prof;
    bool found = store.getProfileByUuid(id, prof);
    if (!found) {
        std::string h = id;
        if (!h.empty() && h[0] == '@') {
            h = h.substr(1);
        }
        found = store.getProfileByHandle(h, prof);
    }
    if (!found) {
        RobotRecord robotRec;
        bool rFound = store.getRobotByUuid(id, robotRec);
        if (!rFound) {
            std::string h = id;
            if (!h.empty() && h[0] == '@') {
                h = h.substr(1);
            }
            rFound = store.getRobotByName(h, robotRec);
        }
        if (rFound && robotRec.gamesPlayed > 0) {
            sendJson(res, 200, buildRobotProfilePayload(robotRec));
            return;
        }
    }
    if (!found || prof.banned) {
        sendError(res, 404, "NOT_FOUND", "no such user");
        return;
    }

    std::vector<RobotRecord> robots;
    store.listPublicRobotsByOwner(prof.playerId, robots);
    Json robotsArr = Json::array();
    for (size_t i = 0; i < robots.size(); ++i) {
        const auto &r = robots[i];
        Json rj;
        rj["robot_uuid"] = r.robotUuid;
        rj["robot_name"] = r.robotName;
        rj["description"] = r.description;
        rj["games_played"] = r.gamesPlayed;
        rj["wins"] = r.wins;
        rj["losses"] = r.losses;
        rj["draws"] = r.draws;
        rj["created_at"] = r.createdAt;
        robotsArr.push_back(rj);
    }

    ArchiveFilter af;
    af.playerUuid = prof.uuid;
    af.limit = 50;
    af.offset = 0;
    std::vector<GameRecord> games;
    store.listArchivedGames(af, games, nullptr);
    Json gamesArr = Json::array();
    for (size_t i = 0; i < games.size(); ++i) {
        const auto &g = games[i];
        Json gj = gameSummaryJson(g);
        gj["p1_handle"] = g.p1Handle;
        gj["p2_handle"] = g.p2Handle;
        gj["p3_handle"] = g.p3Handle;
        gj["p4_handle"] = g.p4Handle;

        int mySeat = 0;
        if (prof.playerId > 0 && prof.playerId == g.p1PlayerId) mySeat = 1;
        else if (prof.playerId > 0 && prof.playerId == g.p2PlayerId) mySeat = 2;
        else if (prof.playerId > 0 && prof.playerId == g.p3PlayerId) mySeat = 3;
        else if (prof.playerId > 0 && prof.playerId == g.p4PlayerId) mySeat = 4;
        else if (!prof.handle.empty() && prof.handle == g.p1Handle) mySeat = 1;
        else if (!prof.handle.empty() && prof.handle == g.p2Handle) mySeat = 2;
        else if (!prof.handle.empty() && prof.handle == g.p3Handle) mySeat = 3;
        else if (!prof.handle.empty() && prof.handle == g.p4Handle) mySeat = 4;

        std::string outcome = "DRAW";
        if (g.result == "DRAW" || (g.winnerSeat == 0 && g.winner == "NONE")) {
            outcome = "DRAW";
        } else if (g.result == "TEAM_13_WIN") {
            outcome = (mySeat == 1 || mySeat == 3) ? "WIN" : "LOSS";
        } else if (g.result == "TEAM_24_WIN") {
            outcome = (mySeat == 2 || mySeat == 4) ? "WIN" : "LOSS";
        } else if (g.winnerSeat > 0) {
            outcome = (mySeat == g.winnerSeat) ? "WIN" : "LOSS";
        } else if (g.winner == "A") {
            outcome = (mySeat == 1) ? "WIN" : "LOSS";
        } else if (g.winner == "B") {
            outcome = (mySeat == 2) ? "WIN" : "LOSS";
        } else if (g.winner == "C") {
            outcome = (mySeat == 3) ? "WIN" : "LOSS";
        } else if (g.winner == "D") {
            outcome = (mySeat == 4) ? "WIN" : "LOSS";
        }
        gj["player_outcome"] = outcome;
        gamesArr.push_back(gj);
    }

    Json j = profileJson(prof, true);
    j["profile"] = profileJson(prof, true);
    j["robots"] = robotsArr;
    j["recent_games"] = gamesArr;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleRobotsDirectory(const Request &req, Response &res)
{
    RobotDirectoryFilter filter;
    if (req.has_param("q")) {
        filter.query = req.get_param_value("q");
    }
    if (req.has_param("sort")) {
        filter.sortField = req.get_param_value("sort");
    } else {
        filter.sortField = "wins";
    }
    if (req.has_param("order")) {
        std::string ord = lower(req.get_param_value("order"));
        filter.sortAsc = (ord == "asc");
    } else {
        filter.sortAsc = false;
    }
    int limit = 50;
    if (req.has_param("limit")) {
        try {
            limit = std::stoi(req.get_param_value("limit"));
        } catch (...) {
            limit = 50;
        }
    }
    if (limit < 1) limit = 1;
    if (limit > 100) limit = 100;
    filter.limit = limit;

    int offset = 0;
    if (req.has_param("offset")) {
        try {
            offset = std::stoi(req.get_param_value("offset"));
        } catch (...) {
            offset = 0;
        }
    }
    if (offset < 0) offset = 0;
    filter.offset = offset;

    std::vector<RobotRecord> list;
    int64_t total = 0;
    if (!store.listPublicRobotsDirectory(filter, list, &total)) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to list robots directory");
        return;
    }

    Json arr = Json::array();
    for (size_t i = 0; i < list.size(); ++i) {
        const auto &r = list[i];
        Json rj;
        rj["robot_uuid"] = r.robotUuid;
        rj["name"] = r.robotName;
        rj["robot_name"] = r.robotName;
        rj["display_name"] = r.robotName;
        rj["description"] = r.description;
        rj["bio"] = r.description;
        rj["is_robot"] = true;
        rj["elo"] = 1200;
        rj["games_played"] = r.gamesPlayed;
        rj["wins"] = r.wins;
        rj["losses"] = r.losses;
        rj["draws"] = r.draws;
        double wr = (r.gamesPlayed > 0)
                        ? std::round((static_cast<double>(r.wins) /
                                      r.gamesPlayed) *
                                     1000.0) /
                              10.0
                        : 0.0;
        rj["win_rate"] = wr;
        rj["revoked"] = r.revoked;
        rj["token_deleted"] = r.tokenDeleted;
        bool retired = (r.revoked || r.tokenDeleted);
        rj["retired"] = retired;
        rj["status"] = (!retired) ? "active" : "retired";
        rj["created_at"] = r.createdAt;
        rj["last_seen_at"] = 0;
        rj["owner_uuid"] = r.ownerUuid;
        rj["owner_handle"] = r.ownerHandle;
        rj["owner_avatar_url"] = r.ownerAvatarUrl;
        arr.push_back(rj);
    }

    Json resp;
    resp["total"] = total;
    resp["limit"] = limit;
    resp["offset"] = offset;
    resp["robots"] = arr;
    sendJson(res, 200, resp);
}

void OctiHttpServer::Impl::handleRobotProfileGet(const Request &req, Response &res)
{
    std::string id;
    std::unordered_map<std::string, std::string>::const_iterator it =
        req.path_params.find("id");
    if (it == req.path_params.end()) {
        it = req.path_params.find("uuid");
    }
    if (it == req.path_params.end() || !isSafeId(it->second)) {
        sendError(res, 404, "ERR_NOT_FOUND", "robot profile not found");
        return;
    }
    id = it->second;

    RobotRecord robotRec;
    bool found = store.getRobotByUuid(id, robotRec);
    if (!found) {
        std::string h = id;
        if (!h.empty() && h[0] == '@') {
            h = h.substr(1);
        }
        found = store.getRobotByName(h, robotRec);
    }
    if (!found || robotRec.gamesPlayed <= 0) {
        sendError(res, 404, "ERR_NOT_FOUND", "robot profile not found");
        return;
    }

    sendJson(res, 200, buildRobotProfilePayload(robotRec));
}

void OctiHttpServer::Impl::handleRobotsList(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    std::vector<RobotRecord> robots;
    if (!store.listRobots(auth.playerId, robots)) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to list robots");
        return;
    }
    int activeCount = 0;
    Json arr = Json::array();
    for (size_t i = 0; i < robots.size(); ++i) {
        const auto &r = robots[i];
        if (!r.revoked) {
            ++activeCount;
        }
        Json rj;
        rj["robot_uuid"] = r.robotUuid;
        rj["robot_name"] = r.robotName;
        rj["description"] = r.description;
        rj["owner_uuid"] = r.ownerUuid;
        rj["owner_handle"] = r.ownerHandle;
        rj["robot_token"] = r.token;
        rj["token_prefix"] = r.tokenPrefix;
        rj["created_at"] = r.createdAt;
        rj["games_played"] = r.gamesPlayed;
        rj["wins"] = r.wins;
        rj["losses"] = r.losses;
        rj["draws"] = r.draws;
        rj["revoked"] = r.revoked;
        rj["revoked_at"] = r.revokedAt;
        arr.push_back(rj);
    }
    Json j;
    j["max_robots"] = kMaxRobotsPerPlayer;
    j["active_count"] = activeCount;
    j["robots"] = arr;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleRobotCreate(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    Json body;
    if (!readJsonBody(req, res, body, false)) {
        return;
    }
    std::string name;
    std::string err;
    if (fieldPresent(body, "robot_name")) {
        fieldString(body, "robot_name", false, name, err);
    } else if (fieldPresent(body, "name")) {
        fieldString(body, "name", false, name, err);
    }
    if (name.empty()) {
        sendError(res, 400, "INVALID_ROBOT_NAME", "missing robot_name");
        return;
    }
    std::string desc;
    fieldString(body, "description", false, desc, err);

    CreateRobotResult cr = store.createRobot(auth.playerId, name, desc);
    switch (cr.status) {
    case RobotStatus::OK: {
        Json j;
        j["robot_uuid"] = cr.robot.robotUuid;
        j["robot_name"] = cr.robot.robotName;
        j["description"] = cr.robot.description;
        j["owner_uuid"] = cr.robot.ownerUuid;
        j["owner_handle"] = cr.robot.ownerHandle;
        j["robot_token"] = cr.rawToken;
        j["token_prefix"] = cr.robot.tokenPrefix;
        j["created_at"] = cr.robot.createdAt;
        j["revoked"] = cr.robot.revoked;
        sendJson(res, 201, j);
        return;
    }
    case RobotStatus::INVALID_ROBOT_NAME:
        sendError(res, 400, "INVALID_ROBOT_NAME",
                  cr.errorMessage.empty() ? "invalid robot name" : cr.errorMessage);
        return;
    case RobotStatus::INVALID_ROBOT_DESCRIPTION:
        sendError(res, 400, "INVALID_ROBOT_DESCRIPTION",
                  cr.errorMessage.empty() ? "invalid robot description" : cr.errorMessage);
        return;
    case RobotStatus::ROBOT_NAME_TAKEN:
        sendError(res, 409, "ROBOT_NAME_TAKEN", "robot name already taken");
        return;
    case RobotStatus::ROBOT_LIMIT_REACHED:
        sendError(res, 409, "ROBOT_LIMIT_REACHED",
                  "robot limit reached (max " + std::to_string(kMaxRobotsPerPlayer) + ")");
        return;
    default:
        break;
    }
    sendError(res, 500, "INTERNAL_ERROR", "failed to create robot");
}

void OctiHttpServer::Impl::handleRobotPatch(const Request &req, Response &res)
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
    std::string desc;
    std::string err;
    if (!fieldString(body, "description", true, desc, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    RobotStatus st = store.updateRobotDescription(auth.playerId, id, desc);
    switch (st) {
    case RobotStatus::OK: {
        Json j;
        j["updated"] = true;
        j["robot_uuid"] = id;
        j["description"] = desc;
        sendJson(res, 200, j);
        return;
    }
    case RobotStatus::INVALID_ROBOT_DESCRIPTION:
        sendError(res, 400, "INVALID_ROBOT_DESCRIPTION", "invalid robot description");
        return;
    case RobotStatus::NOT_FOUND:
        sendError(res, 404, "NOT_FOUND", "robot not found");
        return;
    default:
        break;
    }
    sendError(res, 500, "INTERNAL_ERROR", "failed to update robot");
}

void OctiHttpServer::Impl::handleRobotDelete(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    bool permanent = req.has_param("permanent") &&
                     (req.get_param_value("permanent") == "1" ||
                      req.get_param_value("permanent") == "true");
    if (!permanent && req.has_param("delete") &&
        (req.get_param_value("delete") == "1" ||
         req.get_param_value("delete") == "true")) {
        permanent = true;
    }

    if (permanent) {
        RobotStatus st = store.deleteRobot(auth.playerId, id);
        switch (st) {
        case RobotStatus::OK: {
            Json j;
            j["ok"] = true;
            j["deleted"] = true;
            j["id"] = id;
            j["robot_uuid"] = id;
            sendJson(res, 200, j);
            return;
        }
        case RobotStatus::NOT_FOUND:
            sendError(res, 404, "NOT_FOUND", "robot not found");
            return;
        default:
            break;
        }
        sendError(res, 500, "INTERNAL_ERROR", "failed to delete robot");
        return;
    }

    RobotStatus st = store.revokeRobot(auth.playerId, id);
    switch (st) {
    case RobotStatus::OK: {
        Json j;
        j["revoked"] = true;
        j["id"] = id;
        j["robot_uuid"] = id;
        sendJson(res, 200, j);
        return;
    }
    case RobotStatus::NOT_FOUND:
        sendError(res, 404, "NOT_FOUND", "robot not found");
        return;
    case RobotStatus::ALREADY_REVOKED:
        sendError(res, 409, "ALREADY_REVOKED", "robot is already revoked");
        return;
    default:
        break;
    }
    sendError(res, 500, "INTERNAL_ERROR", "failed to revoke robot");
}

void OctiHttpServer::Impl::handleRobotsConnect(const Request &req, Response &res)
{
    std::string token;
    Json body;
    bool bodyRead = readJsonBody(req, res, body, true);
    if (!bodyRead) {
        return;
    }
    if (!extractBearer(req, token)) {
        std::string err;
        if (fieldPresent(body, "robot_token")) {
            fieldString(body, "robot_token", false, token, err);
        } else if (fieldPresent(body, "token")) {
            fieldString(body, "token", false, token, err);
        }
    }
    if (token.empty()) {
        res.set_header("WWW-Authenticate", "Bearer");
        sendError(res, 401, "UNAUTHORIZED", "missing robot token");
        return;
    }
    RobotTokenValidation val = store.validateRobotToken(token);
    if (!val.valid || val.robot.revoked) {
        res.set_header("WWW-Authenticate", "Bearer error=\"invalid_token\"");
        sendError(res, 401, "UNAUTHORIZED", "invalid or revoked robot token");
        return;
    }
    RobotRecord robotRec = val.robot;

    bool ifUnused = false;
    std::string err;
    fieldBool(body, "if_unused", false, ifUnused, err);

    int64_t actorId = makeRobotActorId(robotRec.robotId);
    const int64_t now = steadyMs();

    std::string leaseId;
    {
        std::lock_guard<std::mutex> lock(_robotLeaseMutex);
        auto it = _activeRobotLeases.find(robotRec.robotId);
        if (ifUnused && it != _activeRobotLeases.end() && !it->second.leaseId.empty()) {
            if (it->second.activeStreams > 0 || (now - it->second.lastSeenMs < 30000)) {
                sendError(res, 409, "ROBOT_TOKEN_IN_USE",
                          "robot token is currently active on another connection");
                return;
            }
        }
        leaseId = generateLeaseId();
        ActiveRobotLease &lease = _activeRobotLeases[robotRec.robotId];
        lease.leaseId = leaseId;
        lease.connectedAtMs = now;
        lease.lastSeenMs = now;
        lease.clientIp = req.remote_addr;
        lease.clientType = "robot";
    }

    rooms.enterLobby(actorId);
    rooms.recordClientHeartbeat(actorId, "robot", true);

    std::string activeRoomId;
    bool hasActiveRoom = rooms.findActiveRoomForActor(actorId, activeRoomId);

    Json j;
    j["ok"] = true;
    j["lease_id"] = leaseId;
    j["robot_id"] = robotRec.robotId;
    j["robot_uuid"] = robotRec.robotUuid;
    j["robot_name"] = robotRec.robotName;
    j["description"] = robotRec.description;
    j["owner_uuid"] = robotRec.ownerUuid;
    j["owner_handle"] = robotRec.ownerHandle;
    j["owner_avatar_url"] = robotRec.ownerAvatarUrl;
    j["active_room_id"] = hasActiveRoom ? activeRoomId : "";
    j["in_game"] = hasActiveRoom;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleRobotsDisconnect(const Request &req, Response &res)
{
    std::string token;
    Json body;
    bool bodyRead = readJsonBody(req, res, body, true);
    if (!bodyRead) {
        return;
    }
    if (!extractBearer(req, token)) {
        std::string err;
        if (fieldPresent(body, "robot_token")) {
            fieldString(body, "robot_token", false, token, err);
        } else if (fieldPresent(body, "token")) {
            fieldString(body, "token", false, token, err);
        }
    }
    if (token.empty()) {
        res.set_header("WWW-Authenticate", "Bearer");
        sendError(res, 401, "UNAUTHORIZED", "missing robot token");
        return;
    }
    RobotTokenValidation val = store.validateRobotToken(token);
    if (!val.valid || val.robot.revoked) {
        res.set_header("WWW-Authenticate", "Bearer error=\"invalid_token\"");
        sendError(res, 401, "UNAUTHORIZED", "invalid or revoked robot token");
        return;
    }
    RobotRecord robotRec = val.robot;

    std::string leaseHeader;
    if (req.has_header("X-Octi-Robot-Lease")) {
        leaseHeader = req.get_header_value("X-Octi-Robot-Lease");
    } else if (fieldPresent(body, "lease_id")) {
        std::string err;
        fieldString(body, "lease_id", false, leaseHeader, err);
    }

    {
        std::lock_guard<std::mutex> lock(_robotLeaseMutex);
        auto it = _activeRobotLeases.find(robotRec.robotId);
        if (it != _activeRobotLeases.end()) {
            if (leaseHeader.empty() || it->second.leaseId == leaseHeader) {
                _activeRobotLeases.erase(it);
            }
        }
    }

    int64_t actorId = makeRobotActorId(robotRec.robotId);
    rooms.clearClientPresence(actorId, "robot");
    rooms.leaveLobby(actorId);

    Json j;
    j["ok"] = true;
    sendJson(res, 200, j);
}

void OctiHttpServer::Impl::handleRobotsExportConf(const Request &req, Response &res)
{
    Auth auth;
    if (!requireAuth(req, res, auth)) {
        return;
    }
    if (auth.isRobot || auth.playerId <= 0) {
        sendError(res, 403, "FORBIDDEN_ROBOT", "robots cannot export token configs");
        return;
    }
    PlayerProfile prof;
    if (!store.getProfileById(auth.playerId, prof)) {
        sendError(res, 401, "UNAUTHORIZED", "unknown player");
        return;
    }
    std::vector<RobotRecord> robots;
    if (!store.listRobots(auth.playerId, robots)) {
        sendError(res, 500, "INTERNAL_ERROR", "failed to list robots");
        return;
    }
    std::ostringstream ss;
    ss << "# octi tokens.conf\n";
    ss << "# Owner: " << prof.handle << " (" << prof.uuid << ")\n";
    ss << "# Format: <robot_name>=<robot_token>\n\n";
    for (size_t i = 0; i < robots.size(); ++i) {
        const auto &r = robots[i];
        if (!r.revoked && !r.token.empty()) {
            ss << r.robotName << "=" << r.token << "\n";
        }
    }
    std::string conf = ss.str();
    res.status = 200;
    res.set_header("Cache-Control", "no-store");
    res.set_header("Content-Disposition", "attachment; filename=\"tokens.conf\"");
    res.set_content(conf, "text/plain; charset=utf-8");
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
        e["avatar_url"] = rows[i].avatarUrl;
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
    if (!optionalAuth(req, res, auth)) {
        return;
    }
    if (auth.playerId == 0) {
        std::string remoteIp = clientRemoteIp(req);
        if (!isLoopbackIp(remoteIp) && !auth.clientType.empty()) {
            int64_t viewerId = rooms.ensureAnonymousObserver(remoteIp);
            rooms.enterLobby(viewerId);
            rooms.recordClientHeartbeat(viewerId, auth.clientType, false);
        }
    } else {
        rooms.enterLobby(auth.playerId);
        rooms.recordClientHeartbeat(auth.playerId, auth.clientType, false);
    }
    LobbySnapshot snap = rooms.lobbyState();
    Json j;
    Json players = Json::array();
    for (size_t i = 0; i < snap.players.size(); ++i) {
        PlayerProfile pProf;
        std::string avatarUrl;
        if (snap.players[i].isRobot) {
            avatarUrl = snap.players[i].avatarUrl;
        } else if (snap.players[i].playerId > 0 && store.getProfileById(snap.players[i].playerId, pProf)) {
            avatarUrl = pProf.avatarUrl;
        }
        Json clientsArr = Json::array();
        for (size_t ci = 0; ci < snap.players[i].clients.size(); ++ci) {
            clientsArr.push_back(snap.players[i].clients[ci]);
        }
        Json pj = {
            {"player_uuid", uuidOf(snap.players[i].playerId)},
            {"handle", snap.players[i].handle},
            {"avatar_url", avatarUrl},
            {"status", toString(snap.players[i].status)},
            {"room_id", snap.players[i].roomId},
            {"clients", clientsArr},
            {"online_sec", snap.players[i].onlineSec},
            {"idle_sec", snap.players[i].idleSec},
            {"is_robot", snap.players[i].isRobot}
        };
        if (snap.players[i].isRobot) {
            pj["robot_uuid"] = snap.players[i].robotUuid;
            pj["robot_description"] = snap.players[i].robotDescription;
            pj["owner_player_id"] = snap.players[i].ownerPlayerId;
            pj["owner_uuid"] = snap.players[i].ownerUuid;
            pj["owner_handle"] = snap.players[i].ownerHandle;
            pj["robot_games_played"] = snap.players[i].robotGamesPlayed;
            pj["robot_wins"] = snap.players[i].robotWins;
            pj["robot_losses"] = snap.players[i].robotLosses;
            pj["robot_draws"] = snap.players[i].robotDraws;
        }
        players.push_back(pj);
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
    if (!optionalAuth(req, res, auth)) {
        return;
    }
    std::string remoteIp = clientRemoteIp(req);
    const int64_t viewer = (auth.playerId > 0)
                               ? auth.playerId
                               : rooms.ensureAnonymousObserver(remoteIp);
    if (auth.playerId == 0 && !isLoopbackIp(remoteIp)) {
        rooms.recordClientHeartbeat(viewer, auth.clientType, false);
    }
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
    int64_t seat = 1;
    if (!fieldInt(body, "seat", false, seat, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    if (seat < 0 || seat > 4) {
        sendError(res, 400, "INVALID_ARGUMENT", "seat must be 0..4");
        return;
    }
    RoomResult r = rooms.createTable(auth.playerId, cfgIn, static_cast<int>(seat));
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

void OctiHttpServer::Impl::handleTablePurge(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    bool isHost = !auth.isRobot && rooms.isTableHost(id, auth.playerId);
    bool isAdmin = !auth.isRobot && store.isPlayerAdmin(auth.playerId);
    if (!isHost && !isAdmin) {
        sendError(res, 403, "ERR_FORBIDDEN",
                  "only the table host or an administrator may delete a table");
        return;
    }
    RoomResult r = rooms.purgeRoom(id);
    if (!r.ok()) {
        if (r.code == RoomCode::NOT_FOUND) {
            sendError(res, 404, "NOT_FOUND", "room not found");
        } else {
            sendError(res, 400, "BAD_REQUEST", r.message);
        }
        return;
    }
    Json j;
    j["ok"] = true;
    j["status"] = "purged";
    j["room_id"] = id;
    sendJson(res, 200, j);
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
    if (!pathId(req, "id", res, id) || !optionalAuth(req, res, auth)) {
        return;
    }
    int64_t viewerId = (auth.playerId > 0)
                           ? auth.playerId
                           : rooms.ensureAnonymousObserver(clientRemoteIp(req));
    RoomResult r = rooms.observeRoom(id, viewerId);
    if (!r.ok()) {
        sendRoomError(res, r);
        return;
    }
    RoomView view;
    rooms.getRoomView(id, viewerId, view);
    sendJson(res, 200, roomViewJson(view));
}

void OctiHttpServer::Impl::handleTableUnobserve(const Request &req,
                                                Response &res)
{
    Auth auth;
    std::string id;
    if (!pathId(req, "id", res, id) || !optionalAuth(req, res, auth)) {
        return;
    }
    int64_t viewerId = (auth.playerId > 0)
                           ? auth.playerId
                           : rooms.ensureAnonymousObserver(clientRemoteIp(req));
    finishRoom(res, rooms.stopObserving(id, viewerId), Json::object());
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
    rs.allowRobots = c.allowRobots;
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
    const int64_t viewerId = (auth.playerId > 0)
                                 ? auth.playerId
                                 : rooms.ensureAnonymousObserver(clientRemoteIp(req));
    RoomView view;
    RoomResult r = rooms.getRoomView(id, viewerId, view);
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
    GameRoomManager::MoveTelemetry telemetry;
    if (body.contains("ai") && body["ai"].is_object()) {
        const auto &ai = body["ai"];
        telemetry.present = true;
        if (ai.contains("depth") && ai["depth"].is_number_integer()) {
            telemetry.depth = ai["depth"].get<int>();
        }
        if (ai.contains("nodes") && ai["nodes"].is_number_integer()) {
            telemetry.nodes = ai["nodes"].get<int64_t>();
        }
        if (ai.contains("score") && ai["score"].is_number_integer()) {
            telemetry.score = ai["score"].get<int>();
        }
        if (ai.contains("elapsed_ms") && ai["elapsed_ms"].is_number_integer()) {
            telemetry.elapsedMs = ai["elapsed_ms"].get<int64_t>();
        }
    }
    RoomResult r = rooms.submitMove(id, auth.playerId, move, telemetry);
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

    if (action == "DRAW_OFFER") {
        action = "offer_draw";
    } else if (action == "ACCEPT_DRAW") {
        action = "accept_draw";
    } else if (action == "DECLINE_DRAW") {
        action = "decline_draw";
    } else if (action == "PAUSE_REQUEST") {
        action = "request_pause";
    } else if (action == "ACCEPT_PAUSE") {
        action = "accept_pause";
    } else if (action == "DECLINE_PAUSE") {
        action = "decline_pause";
    } else if (action == "RESUME") {
        action = "resume";
    } else if (action == "UNDO_REQUEST") {
        action = "request_undo";
    } else if (action == "ACCEPT_UNDO") {
        action = "accept_undo";
    } else if (action == "DECLINE_UNDO") {
        action = "decline_undo";
    } else if (action == "REMATCH_REQUEST") {
        action = "request_rematch";
    } else if (action == "ACCEPT_REMATCH") {
        action = "accept_rematch";
    } else if (action == "DECLINE_REMATCH") {
        action = "decline_rematch";
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
    } else if (action == "claim_forfeit") {
        r = rooms.claimDisconnectForfeit(id, pid);
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
 * Sets the chat mute state of the target.
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
    if (!fieldPresent(body, "mute_chat")) {
        sendError(res, 400, "INVALID_ARGUMENT", "send mute_chat");
        return;
    }
    bool muteChat = false;
    std::string err;
    if (!fieldBool(body, "mute_chat", false, muteChat, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    finishRoom(res,
               rooms.muteUser(id, auth.playerId, target, muteChat),
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
    std::string err;
    if (!fieldString(body, "observer_policy", false, observerText, err)) {
        sendError(res, 400, "INVALID_ARGUMENT", err);
        return;
    }
    if (observerText.empty()) {
        sendError(res, 400, "INVALID_ARGUMENT", "send observer_policy");
        return;
    }
    std::optional<ObserverPolicy> op;
    ObserverPolicy parsed = ObserverPolicy::OPEN;
    if (!parseObserverPolicy(observerText, parsed)) {
        sendError(res, 400, "INVALID_ARGUMENT",
                  "observer_policy must be OPEN, SILENT_OBSERVERS or "
                  "NO_OBSERVERS");
        return;
    }
    op = parsed;
    RoomResult r = rooms.updateRoomPolicies(id, auth.playerId, op);
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
    finishRoom(res, r, j);
}

void OctiHttpServer::Impl::handleGameEvents(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!pathId(req, "id", res, id) || !optionalAuth(req, res, auth)) {
        return;
    }
    const int64_t viewer = (auth.playerId > 0)
                               ? auth.playerId
                               : rooms.ensureAnonymousObserver(clientRemoteIp(req));
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

static int countPods(const OctiBoard &b, OctiPlayer player)
{
    int count = 0;
    for (int x = 0; x < b.getBoardWidth(); ++x) {
        for (int y = 0; y < b.getBoardHeight(); ++y) {
            const OctiSquare &sq = b.squareAt(x, y);
            for (int i = 0; i < sq.getNumPods(); ++i) {
                if (podTypeToPlayer(sq.getPodAt(i).getType()) == player) {
                    ++count;
                }
            }
        }
    }
    return count;
}

static std::string classifyRuleTermination(const OctiBoard &b, const OctiGame &g)
{
    OctiPlayer w = g.getWinner();
    if (g.getMode() == GAME_MODE_2) {
        const int xs[3] = {2, 4, 6};
        int y = (w == PLAYER_A) ? 6 : 2;
        int captured = 0;
        for (int i = 0; i < 3; ++i) {
            if (b.squareAt(xs[i], y).isCapturedBy(w)) {
                ++captured;
            }
        }
        int needed = (g.getBases() == GAME_BASES_1) ? 1 : 3;
        if (captured >= needed) {
            return "BASE_CAPTURE";
        }
        OctiPlayer opp = (w == PLAYER_A) ? PLAYER_B : PLAYER_A;
        return (countPods(b, opp) == 0 && g.getFreePods(opp) == 0) ? "TOTAL_CAPTURE" : "STALEMATE";
    }
    if (g.getMode() == GAME_MODE_OFK) {
        int y = (w == PLAYER_A) ? 5 : 1;
        for (int x = 1; x <= 4; ++x) {
            if (b.squareAt(x, y).isCapturedBy(w)) {
                return "BASE_CAPTURE";
            }
        }
        OctiPlayer opp = (w == PLAYER_A) ? PLAYER_B : PLAYER_A;
        return countPods(b, opp) == 0 ? "TOTAL_CAPTURE" : "STALEMATE";
    }
    bool bases;
    if (w == PLAYER_A || w == PLAYER_C) {
        bases = b.squareAt(2, 6).isCapturedByTeamOf(PLAYER_A) &&
                b.squareAt(6, 2).isCapturedByTeamOf(PLAYER_A);
    } else {
        bases = b.squareAt(2, 2).isCapturedByTeamOf(PLAYER_B) &&
                b.squareAt(6, 6).isCapturedByTeamOf(PLAYER_B);
    }
    if (bases) {
        return "BASE_CAPTURE";
    }
    int bdPods = countPods(b, PLAYER_B) + countPods(b, PLAYER_D);
    int acPods = countPods(b, PLAYER_A) + countPods(b, PLAYER_C);
    int bdFree = (g.isPlayerEliminated(PLAYER_B) ? 0 : g.getFreePods(PLAYER_B)) +
                 (g.isPlayerEliminated(PLAYER_D) ? 0 : g.getFreePods(PLAYER_D));
    int acFree = (g.isPlayerEliminated(PLAYER_A) ? 0 : g.getFreePods(PLAYER_A)) +
                 (g.isPlayerEliminated(PLAYER_C) ? 0 : g.getFreePods(PLAYER_C));
    if ((w == PLAYER_A || w == PLAYER_C) && bdPods == 0 && bdFree == 0) {
        return "TOTAL_CAPTURE";
    }
    if ((w == PLAYER_B || w == PLAYER_D) && acPods == 0 && acFree == 0) {
        return "TOTAL_CAPTURE";
    }
    return "STALEMATE";
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
    if (modeStr == "2P" || modeStr == "2") {
        mode = GAME_MODE_2;
        modeStr = "2P";
    } else if (modeStr == "4P" || modeStr == "4") {
        mode = GAME_MODE_4;
        modeStr = "4P";
    } else if (modeStr == "OFK") {
        mode = GAME_MODE_OFK;
    } else {
        sendError(res, 400, "MALFORMED_UPLOAD", "invalid mode");
        return;
    }

    int basesVal = 1;
    OctiGameBases bases = GAME_BASES_1;
    bool superProngs = false;
    bool edgeless = false;

    if (mode == GAME_MODE_OFK) {
        basesVal = 1;
        bases = GAME_BASES_1;
        superProngs = false;
        edgeless = false;
    } else if (mode == GAME_MODE_4) {
        basesVal = 3;
        int claimedBases = transcriptObj.value("bases", 3);
        bases = (claimedBases == 1) ? GAME_BASES_1 : GAME_BASES_3;
        if (transcriptObj.contains("superProngs") && transcriptObj["superProngs"].is_boolean()) {
            superProngs = transcriptObj["superProngs"].get<bool>();
        }
        if (transcriptObj.contains("edgeless") && transcriptObj["edgeless"].is_boolean()) {
            edgeless = transcriptObj["edgeless"].get<bool>();
        }
    } else {
        if (!transcriptObj.contains("bases") || !transcriptObj["bases"].is_number_integer()) {
            sendError(res, 400, "MALFORMED_UPLOAD", "missing bases");
            return;
        }
        basesVal = transcriptObj["bases"].get<int>();
        if (basesVal != 1 && basesVal != 3) {
            sendError(res, 400, "MALFORMED_UPLOAD", "bases must be 1 or 3");
            return;
        }
        bases = (basesVal == 3) ? GAME_BASES_3 : GAME_BASES_1;

        if (!transcriptObj.contains("superProngs") || !transcriptObj["superProngs"].is_boolean() ||
            !transcriptObj.contains("edgeless") || !transcriptObj["edgeless"].is_boolean()) {
            sendError(res, 400, "MALFORMED_UPLOAD", "missing superProngs or edgeless");
            return;
        }
        superProngs = transcriptObj["superProngs"].get<bool>();
        edgeless = transcriptObj["edgeless"].get<bool>();
    }

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
    std::string normalizedClaimedWinner = claimedWinner;
    if (mode == GAME_MODE_4) {
        if (claimedWinner == "TEAM_13_WIN" || claimedWinner == "A_WIN" || claimedWinner == "C_WIN") {
            normalizedClaimedWinner = "A";
        } else if (claimedWinner == "TEAM_24_WIN" || claimedWinner == "B_WIN" || claimedWinner == "D_WIN") {
            normalizedClaimedWinner = "B";
        }
    }
    if (normalizedClaimedWinner != "A" && normalizedClaimedWinner != "B" &&
        normalizedClaimedWinner != "C" && normalizedClaimedWinner != "D" &&
        normalizedClaimedWinner != "NONE") {
        sendError(res, 400, "MALFORMED_UPLOAD", "invalid winner value");
        return;
    }
    std::string claimedFinalFen;
    if (transcriptObj.contains("finalFen") && transcriptObj["finalFen"].is_string()) {
        claimedFinalFen = transcriptObj["finalFen"].get<std::string>();
    } else if (transcriptObj.contains("final_fen") && transcriptObj["final_fen"].is_string()) {
        claimedFinalFen = transcriptObj["final_fen"].get<std::string>();
    } else {
        sendError(res, 400, "MALFORMED_UPLOAD", "missing finalFen");
        return;
    }

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
    std::string seatHandle[5] = {"", "Player", "Player", "", ""};
    int64_t seatPlayerId[5] = {0, 0, 0, 0, 0};
    bool seatIsAi[5] = {false, false, false, false, false};
    auto canonicalSeatOf = [](const Json &p) -> int {
        if (!p.is_object()) {
            return 0;
        }
        std::string pStr = p.value("player", "");
        if (pStr == "A") return 1;
        if (pStr == "B") return 2;
        if (pStr == "C") return 3;
        if (pStr == "D") return 4;
        int seat = p.value("seat", 0);
        return (seat >= 1 && seat <= 4) ? seat : 0;
    };
    auto canonicalSideOf = [](int seat) -> const char * {
        switch (seat) {
        case 1: return "A";
        case 2: return "B";
        case 3: return "C";
        case 4: return "D";
        default: return "";
        }
    };
    if (transcriptObj.contains("players") && transcriptObj["players"].is_array()) {
        for (const auto &p : transcriptObj["players"]) {
            if (p.is_object()) {
                int seat = canonicalSeatOf(p);
                std::string h = p.value("handle", "");
                std::string t = p.value("type", "");
                int64_t pid = p.value("playerId", p.value("id", static_cast<int64_t>(0)));
                bool isAi = (t == "AI" || h.rfind("AI_", 0) == 0);
                if (seat >= 1 && seat <= 4) {
                    if (!h.empty()) seatHandle[seat] = h;
                    if (pid > 0) seatPlayerId[seat] = pid;
                    seatIsAi[seat] = isAi;
                }
                if (t == "AI" || p.value("aiDifficulty", 0) > 0) {
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
    bool winnerMatches = false;
    if (mode == GAME_MODE_4) {
        if ((replayedWinner == "A" || replayedWinner == "C") &&
            (normalizedClaimedWinner == "A" || normalizedClaimedWinner == "C")) {
            winnerMatches = true;
        } else if ((replayedWinner == "B" || replayedWinner == "D") &&
                   (normalizedClaimedWinner == "B" || normalizedClaimedWinner == "D")) {
            winnerMatches = true;
        } else if (replayedWinner == "NONE" && normalizedClaimedWinner == "NONE") {
            winnerMatches = true;
        }
    } else {
        winnerMatches = (claimedWinner == replayedWinner);
    }
    if (!winnerMatches) {
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
    auto normalizePlayersArr = [&](Json &arr) {
        if (!arr.is_array()) {
            return;
        }
        for (auto &pEntry : arr) {
            if (!pEntry.is_object()) {
                continue;
            }
            int s = canonicalSeatOf(pEntry);
            if (s >= 1 && s <= 4) {
                pEntry["seat"] = s;
                const char *side = canonicalSideOf(s);
                if (side[0] != '\0') {
                    pEntry["player"] = side;
                }
            }
        }
        std::vector<Json> sorted(arr.begin(), arr.end());
        std::stable_sort(sorted.begin(), sorted.end(),
                         [](const Json &a, const Json &b) {
                             int sa = a.is_object() ? a.value("seat", 99) : 99;
                             int sb = b.is_object() ? b.value("seat", 99) : 99;
                             return sa < sb;
                         });
        arr = Json::array();
        for (auto &item : sorted) {
            arr.push_back(std::move(item));
        }
    };
    if (storedJson.contains("players")) {
        normalizePlayersArr(storedJson["players"]);
    }
    if (storedJson.contains("game") && storedJson["game"].is_object() &&
        storedJson["game"].contains("players")) {
        normalizePlayersArr(storedJson["game"]["players"]);
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
    rec.p1PlayerId = seatPlayerId[1];
    rec.p2PlayerId = seatPlayerId[2];
    rec.p1Handle = seatHandle[1];
    rec.p2Handle = seatHandle[2];
    if (mode == GAME_MODE_4) {
        rec.p3PlayerId = seatPlayerId[3];
        rec.p3Handle = seatHandle[3].empty() ? "Player" : seatHandle[3];
        rec.p4PlayerId = seatPlayerId[4];
        rec.p4Handle = seatHandle[4].empty() ? "Player" : seatHandle[4];
    } else {
        rec.p3PlayerId = 0;
        rec.p4PlayerId = 0;
        rec.p3Handle = "";
        rec.p4Handle = "";
    }
    if (mode == GAME_MODE_4) {
        if (replayedWinner == "A" || replayedWinner == "C") {
            rec.result = "TEAM_13_WIN";
            rec.winnerSeat = (replayedWinner == "C") ? 3 : 1;
        } else if (replayedWinner == "B" || replayedWinner == "D") {
            rec.result = "TEAM_24_WIN";
            rec.winnerSeat = (replayedWinner == "D") ? 4 : 2;
        } else {
            rec.result = "DRAW";
            rec.winnerSeat = 0;
        }
    } else {
        rec.result = (replayedWinner == "A") ? "P1_WIN" : (replayedWinner == "B" ? "P2_WIN" : "DRAW");
        if (replayedWinner == "A") rec.winnerSeat = 1;
        else if (replayedWinner == "B") rec.winnerSeat = 2;
        else rec.winnerSeat = 0;
    }

    std::string term = transcriptObj.value("termination", "");
    if (term == "FORFEIT") term = "RESIGNATION";

    if (statusStr == "RESIGNED") {
        if (term.empty()) {
            term = "RESIGNATION";
        }
    } else {
        std::string ruleTerm = classifyRuleTermination(board, *game);
        if (term.empty()) {
            term = ruleTerm;
        } else if (term == "BASE_CAPTURE" || term == "BASE_OCCUPATION" ||
                   term == "TOTAL_CAPTURE" || term == "ELIMINATION" ||
                   term == "STALEMATE") {
            if (ruleTerm == "BASE_CAPTURE" && (term == "BASE_CAPTURE" || term == "BASE_OCCUPATION")) {
                // Keep client's preferred BASE_CAPTURE / BASE_OCCUPATION synonym
            } else if (ruleTerm == "TOTAL_CAPTURE" && (term == "TOTAL_CAPTURE" || term == "ELIMINATION")) {
                // Keep client's preferred TOTAL_CAPTURE / ELIMINATION synonym
            } else {
                term = ruleTerm;
            }
        }
    }
    if (term != "BASE_CAPTURE" && term != "BASE_OCCUPATION" &&
        term != "TOTAL_CAPTURE" && term != "ELIMINATION" &&
        term != "STALEMATE" && term != "RESIGNATION" &&
        term != "TIMEOUT" && term != "DISCONNECT_FORFEIT" &&
        term != "AGREED_DRAW") {
        term = (statusStr == "RESIGNED" ? "RESIGNATION" : classifyRuleTermination(board, *game));
    }
    rec.termination = term;
    storedJson["termination"] = term;

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

    PlayerProfile authedPlayer;
    bool hasAuthedPlayer = false;
    if (req.has_header("Authorization")) {
        std::string token;
        if (extractBearer(req, token)) {
            SessionValidation sv = store.validateSession(token);
            if (sv.valid && sv.playerId > 0) {
                if (store.getProfileById(sv.playerId, authedPlayer)) {
                    hasAuthedPlayer = true;
                }
            }
        }
    }
    if (!hasAuthedPlayer && !installId.empty() && isSafeId(installId)) {
        if (store.getPlayerByInstallId(installId, authedPlayer)) {
            hasAuthedPlayer = true;
        }
    }
    if (hasAuthedPlayer && authedPlayer.playerId > 0) {
        if (!installId.empty() && isSafeId(installId)) {
            store.bindInstallToPlayer(installId, authedPlayer.playerId);
        }
        int maxSeats = (mode == GAME_MODE_4) ? 4 : 2;
        int humanSeat = 0;
        for (int s = 1; s <= maxSeats; ++s) {
            if (seatPlayerId[s] == authedPlayer.playerId ||
                (seatHandle[s] == authedPlayer.handle && authedPlayer.handle != "Player")) {
                humanSeat = s;
                break;
            }
        }
        if (humanSeat == 0) {
            for (int s = 1; s <= maxSeats; ++s) {
                if (!seatIsAi[s] && seatHandle[s].rfind("AI_", 0) != 0) {
                    humanSeat = s;
                    break;
                }
            }
        }
        if (humanSeat == 0) humanSeat = 1;

        seatPlayerId[humanSeat] = authedPlayer.playerId;
        seatHandle[humanSeat] = authedPlayer.handle;

        rec.p1PlayerId = seatPlayerId[1];
        rec.p1Handle = seatHandle[1];
        rec.p2PlayerId = seatPlayerId[2];
        rec.p2Handle = seatHandle[2];
        if (mode == GAME_MODE_4) {
            rec.p3PlayerId = seatPlayerId[3];
            rec.p3Handle = seatHandle[3];
            rec.p4PlayerId = seatPlayerId[4];
            rec.p4Handle = seatHandle[4];
        }

        auto updateHumanEntry = [&](Json &arr) {
            if (!arr.is_array()) {
                return;
            }
            for (auto &pEntry : arr) {
                if (!pEntry.is_object()) continue;
                int s = canonicalSeatOf(pEntry);
                std::string t = pEntry.value("type", "");
                std::string h = pEntry.value("handle", "");
                if (s == humanSeat || (s == 0 && (t == "HUMAN" || h.rfind("AI_", 0) != 0))) {
                    pEntry["handle"] = authedPlayer.handle;
                    pEntry["player_uuid"] = authedPlayer.uuid;
                }
            }
        };
        if (storedJson.contains("players")) {
            updateHumanEntry(storedJson["players"]);
        }
        if (storedJson.contains("game") && storedJson["game"].is_object() &&
            storedJson["game"].contains("players")) {
            updateHumanEntry(storedJson["game"]["players"]);
        }
        rec.octiTranscript = storedJson.dump();
    }

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

void OctiHttpServer::Impl::handleArchivePurgeGame(const Request &req, Response &res)
{
    Auth auth;
    std::string id;
    if (!requireAuth(req, res, auth) || !pathId(req, "id", res, id)) {
        return;
    }
    if (auth.isRobot || !store.isPlayerAdmin(auth.playerId)) {
        sendError(res, 403, "FORBIDDEN", "admin privileges required");
        return;
    }
    std::string err;
    if (!store.purgeArchivedGame(id, err)) {
        if (err == "game_not_found") {
            sendError(res, 404, "NOT_FOUND", "archived game not found");
        } else {
            sendError(res, 500, "INTERNAL_ERROR", "failed to purge archived game: " + err);
        }
        return;
    }
    rooms.refreshCachedPlayerStats();
    Json j;
    j["ok"] = true;
    j["status"] = "purged";
    j["game_id"] = id;
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

void OctiHttpServer::Impl::handleFavicon(const Request &, Response &res)
{
    res.status = 204;
    res.set_header("Cache-Control", "public, max-age=86400");
}

void OctiHttpServer::Impl::handleIndex(const Request &, Response &res)
{
    res.status = 200;
    res.set_header("Cache-Control", "no-cache");
    res.set_header("Content-Security-Policy",
                   "default-src 'none'; script-src 'unsafe-inline'; "
                   "worker-src 'self' blob:; "
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
        {"GET", "/favicon.ico", &Impl::handleFavicon},
        {"GET", "/health", &Impl::handleHealth},
        {"GET", "/privacy", &Impl::handlePrivacy},
        {"POST", "/privacy", &Impl::handleDataDeletion},
        {"GET", "/api/v1/health", &Impl::handleHealth},
        {"GET", "/api/v1/avatars/:file", &Impl::handleAvatarGet},
        {"GET", "/avatar/:file", &Impl::handleAvatarGet},
        {"GET", "/api/v1/auth/data-deletion", &Impl::handleDataDeletion},
        {"POST", "/api/v1/auth/data-deletion", &Impl::handleDataDeletion},
        {"GET", "/api/v1/auth/authorize", &Impl::handleAuthAuthorize},
        {"GET", "/api/v1/auth/callback", &Impl::handleAuthCallback},
        {"GET", "/api/v1/auth/providers", &Impl::handleProviders},
        {"POST", "/api/v1/auth/sso", &Impl::handleSso},
        {"POST", "/api/v1/auth/link", &Impl::handleLink},
        {"DELETE", "/api/v1/auth/link/:provider", &Impl::handleUnlink},
        {"GET", "/auth/identities", &Impl::handleAuthListIdentities},
        {"GET", "/api/v1/auth/identities", &Impl::handleAuthListIdentities},
        {"DELETE", "/auth/identities/:provider", &Impl::handleAuthUnlinkProvider},
        {"DELETE", "/api/v1/auth/identities/:provider", &Impl::handleAuthUnlinkProvider},
        {"POST", "/auth/logout", &Impl::handleLogout},
        {"POST", "/api/v1/auth/logout", &Impl::handleLogout},
        {"GET", "/api/v1/profile/me", &Impl::handleProfileGet},
        {"GET", "/api/v1/profile", &Impl::handleProfileGet},
        {"PATCH", "/api/v1/profile/me", &Impl::handleProfilePatch},
        {"PATCH", "/api/v1/profile", &Impl::handleProfilePatch},
        {"GET", "/api/v1/profile/:id", &Impl::handleUserGet},
        {"GET", "/api/v1/users", &Impl::handleUsersList},
        {"GET", "/api/v1/users/:id", &Impl::handleUserGet},
        {"GET", "/api/v1/robots", &Impl::handleRobotsList},
        {"GET", "/api/v1/robots/export-conf", &Impl::handleRobotsExportConf},
        {"GET", "/api/v1/robots/directory", &Impl::handleRobotsDirectory},
        {"GET", "/api/v1/robots/profile/:id", &Impl::handleRobotProfileGet},
        {"POST", "/api/v1/robots/connect", &Impl::handleRobotsConnect},
        {"POST", "/api/v1/robots/disconnect", &Impl::handleRobotsDisconnect},
        {"POST", "/api/v1/robots", &Impl::handleRobotCreate},
        {"PATCH", "/api/v1/robots/:id", &Impl::handleRobotPatch},
        {"DELETE", "/api/v1/robots/:id", &Impl::handleRobotDelete},
        {"POST", "/api/v1/robots/:id/revoke", &Impl::handleRobotDelete},
        {"GET", "/api/v1/players/:uuid", &Impl::handlePlayer},
        {"GET", "/api/v1/leaderboard", &Impl::handleLeaderboard},
        {"GET", "/api/v1/lobby/state", &Impl::handleLobbyState},
        {"POST", "/api/v1/lobby/chat", &Impl::handleLobbyChat},
        {"GET", "/api/v1/lobby/events", &Impl::handleLobbyEvents},
        {"GET", "/api/v1/tables", &Impl::handleTables},
        {"GET", "/api/v1/tables/:id", &Impl::handleGameGet},
        {"POST", "/api/v1/tables", &Impl::handleTableCreate},
        {"POST", "/api/v1/tables/:id/join", &Impl::handleTableJoin},
        {"POST", "/api/v1/tables/:id/leave", &Impl::handleTableLeave},
        {"DELETE", "/api/v1/tables/:id", &Impl::handleTablePurge},
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
        {"POST", "/api/v1/games/:id/ban", &Impl::handleBan},
        {"POST", "/api/v1/games/:id/mute", &Impl::handleMute},
        {"POST", "/api/v1/games/:id/settings", &Impl::handleSettings},
        {"GET", "/api/v1/games/:id/events", &Impl::handleGameEvents},
        {"POST", "/api/v1/archive/upload", &Impl::handleArchiveUpload},
        {"DELETE", "/api/v1/archive/install/:installId", &Impl::handleArchiveDeleteInstall},
        {"DELETE", "/api/v1/archive/games/:id", &Impl::handleArchivePurgeGame},
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
     * Nothing but JSON is ever read: a request that announces a body of
     * any other type (audio, octet-stream, multipart...) is refused before
     * a single body byte is consumed.
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
                rq.method == "OPTIONS" ||
                rq.path == "/api/v1/auth/data-deletion" ||
                rq.path == "/privacy") {
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
