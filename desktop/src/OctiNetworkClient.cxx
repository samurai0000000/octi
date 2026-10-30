/*
 * OctiNetworkClient.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include "OctiNetworkClient.hxx"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <thread>

namespace octi {

namespace {

typedef nlohmann::json Json;

std::string toLowerAscii(const std::string &s)
{
    std::string out = s;
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(out[i])));
    }
    return out;
}

std::string toUpperAscii(const std::string &s)
{
    std::string out = s;
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(out[i])));
    }
    return out;
}

std::string urlEncode(const std::string &val)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(val.size() * 3);
    for (std::size_t i = 0; i < val.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(val[i]);
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back('%');
            out.push_back(hex[(c >> 4) & 0x0F]);
            out.push_back(hex[c & 0x0F]);
        }
    }
    return out;
}

LinkedIdentityInfo parseLinkedIdentity(const Json &j)
{
    LinkedIdentityInfo info;
    if (!j.is_object()) {
        return info;
    }
    info.provider = j.value("provider", "");
    info.providerEmail = j.value("provider_email", "");
    info.displayHint = j.value("display_hint", "");
    info.linkedAt = j.value("linked_at", static_cast<std::int64_t>(0));
    info.lastLoginAt = j.value("last_login_at", static_cast<std::int64_t>(0));
    return info;
}

std::vector<LinkedIdentityInfo> parseLinkedIdentities(const Json &arr)
{
    std::vector<LinkedIdentityInfo> out;
    if (!arr.is_array()) {
        return out;
    }
    for (std::size_t i = 0; i < arr.size(); ++i) {
        out.push_back(parseLinkedIdentity(arr[i]));
    }
    return out;
}

ArchivedGameSummary parseArchivedGameSummary(const Json &j)
{
    ArchivedGameSummary g;
    if (!j.is_object()) {
        return g;
    }
    g.gameId = j.value("game_id", j.value("room_id", ""));
    g.mode = j.value("mode", "2P");
    g.bases = j.value("bases", 3);
    g.superProngs = j.value("super_prongs", false);
    g.edgeless = j.value("edgeless", false);
    if (j.contains("time_control") && j["time_control"].is_string()) {
        g.timeControl = j["time_control"].get<std::string>();
    }
    if (j.contains("players") && j["players"].is_array()) {
        const Json &pl = j["players"];
        for (std::size_t i = 0; i < pl.size(); ++i) {
            if (!pl[i].is_object()) {
                continue;
            }
            ArchivedGamePlayer p;
            p.seat = pl[i].value("seat", static_cast<int>(i) + 1);
            p.handle = pl[i].value("handle", "");
            if (pl[i].contains("player_uuid") &&
                pl[i]["player_uuid"].is_string()) {
                p.playerUuid = pl[i]["player_uuid"].get<std::string>();
            }
            g.players.push_back(p);
        }
    }
    g.result = j.value("result", "");
    g.winnerSeat = j.value("winner_seat", 0);
    g.termination = j.value("termination", "");
    g.plyCount = j.value("ply_count", 0);
    g.startedAt = j.value("started_at", static_cast<std::int64_t>(0));
    g.endedAt = j.value("ended_at", static_cast<std::int64_t>(0));
    g.finalFen = j.value("final_fen", j.value("fen", ""));
    g.octiTranscript = j.value("octi_transcript", "");
    return g;
}

PlayerProfileInfo parsePlayerProfile(const Json &j)
{
    PlayerProfileInfo p;
    if (!j.is_object()) {
        return p;
    }
    p.uuid = j.value("uuid", "");
    p.handle = j.value("handle", "");
    p.displayName = j.value("display_name", "");
    p.avatarUrl = j.value("avatar_url", "");
    p.elo2p = j.value("elo_2p", 1200);
    p.elo4p = j.value("elo_4p", 1200);
    p.eloOfk = j.value("elo_ofk", 1200);
    p.gamesPlayed = j.value("games_played", 0);
    p.wins = j.value("wins", 0);
    p.losses = j.value("losses", 0);
    p.draws = j.value("draws", 0);
    p.createdAt = j.value("created_at", static_cast<std::int64_t>(0));
    p.lastSeenAt = j.value("last_seen_at", static_cast<std::int64_t>(0));
    if (j.contains("linked_identities")) {
        p.identities = parseLinkedIdentities(j["linked_identities"]);
    }
    if (j.contains("recent_games") && j["recent_games"].is_array()) {
        const Json &rg = j["recent_games"];
        for (std::size_t i = 0; i < rg.size(); ++i) {
            p.recentGames.push_back(parseArchivedGameSummary(rg[i]));
        }
    }
    return p;
}

TableSummaryInfo parseTableSummary(const Json &j)
{
    TableSummaryInfo t;
    if (!j.is_object()) {
        return t;
    }
    t.roomId = j.value("room_id", "");
    t.mode = j.value("mode", "2P");
    t.bases = j.value("bases", 3);
    t.superProngs = j.value("super_prongs", false);
    t.edgeless = j.value("edgeless", false);
    if (j.contains("time_control") && j["time_control"].is_object()) {
        const Json &tc = j["time_control"];
        t.timeControlEnabled = tc.value("enabled", false);
        t.initialMs = tc.value("initial_ms", static_cast<std::int64_t>(0));
        t.incrementMs = tc.value("increment_ms", static_cast<std::int64_t>(0));
        t.disconnectGraceMs =
            tc.value("disconnect_grace_ms", static_cast<std::int64_t>(90000));
    }
    t.rated = j.value("rated", true);
    t.allowTakeback = j.value("allow_takeback", false);
    t.observerPolicy = j.value("observer_policy", "OPEN");
    t.voicePolicy = j.value("voice_policy", "PLAYERS_ONLY");
    t.phase = j.value("phase", "WAITING_FOR_PLAYERS");
    t.hasPasscode = j.value("has_passcode", false);
    t.hostHandle = j.value("host_handle", "");
    t.seatsTotal = j.value("seats_total", 2);
    t.seatsFilled = j.value("seats_filled", 0);
    if (j.contains("players") && j["players"].is_array()) {
        for (std::size_t i = 0; i < j["players"].size(); ++i) {
            if (j["players"][i].is_string()) {
                t.players.push_back(j["players"][i].get<std::string>());
            }
        }
    }
    t.observerCount = j.value("observer_count", 0);
    return t;
}

GameViewData parseGameView(const Json &j)
{
    GameViewData v;
    if (!j.is_object()) {
        return v;
    }
    v.live = j.value("live", true);
    v.roomId = j.value("room_id", j.value("game_id", ""));
    v.mode = j.value("mode", "2P");
    v.bases = j.value("bases", 3);
    v.superProngs = j.value("super_prongs", false);
    v.edgeless = j.value("edgeless", false);
    if (j.contains("time_control") && j["time_control"].is_object()) {
        const Json &tc = j["time_control"];
        v.timeControlEnabled = tc.value("enabled", false);
        v.initialMs = tc.value("initial_ms", static_cast<std::int64_t>(0));
        v.incrementMs = tc.value("increment_ms", static_cast<std::int64_t>(0));
        v.disconnectGraceMs =
            tc.value("disconnect_grace_ms", static_cast<std::int64_t>(90000));
    }
    v.rated = j.value("rated", true);
    v.allowTakeback = j.value("allow_takeback", false);
    v.observerPolicy = j.value("observer_policy", "OPEN");
    v.voicePolicy = j.value("voice_policy", "PLAYERS_ONLY");
    v.phase = j.value("phase", "WAITING_FOR_PLAYERS");
    v.paused = j.value("paused", false);
    v.hasPasscode = j.value("has_passcode", false);
    v.hostUuid = j.value("host_uuid", "");
    v.viewerSeat = j.value("viewer_seat", 0);
    if (j.contains("seats") && j["seats"].is_array()) {
        const Json &arr = j["seats"];
        for (std::size_t i = 0; i < arr.size(); ++i) {
            if (!arr[i].is_object()) {
                continue;
            }
            GameSeatInfo s;
            s.seat = arr[i].value("seat", static_cast<int>(i) + 1);
            if (arr[i].contains("player_uuid") &&
                arr[i]["player_uuid"].is_string()) {
                s.playerUuid = arr[i]["player_uuid"].get<std::string>();
            }
            s.handle = arr[i].value("handle", "");
            s.rating = arr[i].value("rating", 1200);
            s.connected = arr[i].value("connected", true);
            s.disconnectedForMs = arr[i].value(
                "disconnected_for_ms", static_cast<std::int64_t>(0));
            s.forfeitClaimable = arr[i].value("forfeit_claimable", false);
            s.clockMs =
                arr[i].value("clock_ms", static_cast<std::int64_t>(-1));
            s.eliminated = arr[i].value("eliminated", false);
            v.seats.push_back(s);
        }
    }
    if (j.contains("observers") && j["observers"].is_array()) {
        for (std::size_t i = 0; i < j["observers"].size(); ++i) {
            if (j["observers"][i].is_string()) {
                v.observers.push_back(j["observers"][i].get<std::string>());
            }
        }
    }
    v.observerCount =
        j.value("observer_count", static_cast<int>(v.observers.size()));
    v.fen = j.value("fen", "");
    v.turnSeat = j.value("turn_seat", 0);
    v.plyCount = j.value("ply_count", 0);
    if (j.contains("moves") && j["moves"].is_array()) {
        for (std::size_t i = 0; i < j["moves"].size(); ++i) {
            if (j["moves"][i].is_string()) {
                v.moves.push_back(j["moves"][i].get<std::string>());
            }
        }
    }
    v.result = j.value("result", "");
    v.termination = j.value("termination", "");
    v.winnerSeat = j.value("winner_seat", 0);
    if (j.contains("proposals") && j["proposals"].is_array()) {
        const Json &pr = j["proposals"];
        for (std::size_t i = 0; i < pr.size(); ++i) {
            if (!pr[i].is_object()) {
                continue;
            }
            GameProposalInfo p;
            p.type = pr[i].value("type", "");
            p.proposerUuid = pr[i].value("proposer_uuid", "");
            if (pr[i].contains("accepted_by") &&
                pr[i]["accepted_by"].is_array()) {
                for (std::size_t k = 0; k < pr[i]["accepted_by"].size(); ++k) {
                    if (pr[i]["accepted_by"][k].is_string()) {
                        p.acceptedBy.push_back(
                            pr[i]["accepted_by"][k].get<std::string>());
                    }
                }
            }
            if (pr[i].contains("awaiting") && pr[i]["awaiting"].is_array()) {
                for (std::size_t k = 0; k < pr[i]["awaiting"].size(); ++k) {
                    if (pr[i]["awaiting"][k].is_string()) {
                        p.awaiting.push_back(
                            pr[i]["awaiting"][k].get<std::string>());
                    }
                }
            }
            p.expiresAtMs = pr[i].value(
                "expires_at_ms", static_cast<std::int64_t>(0));
            v.proposals.push_back(p);
        }
    }
    if (j.contains("voice_peers") && j["voice_peers"].is_array()) {
        const Json &vp = j["voice_peers"];
        for (std::size_t i = 0; i < vp.size(); ++i) {
            if (!vp[i].is_object()) {
                continue;
            }
            VoicePeerInfo peer;
            peer.playerUuid = vp[i].value("player_uuid", "");
            peer.micMuted = vp[i].value("mic_muted", false);
            v.voicePeers.push_back(peer);
        }
    }
    v.seq = j.value("seq", static_cast<std::int64_t>(0));
    v.archived = j.value("archived", false);
    v.rematchRoomId = j.value("rematch_room_id", "");
    v.createdAtMs = j.value("created_at_ms", static_cast<std::int64_t>(0));
    return v;
}

} // namespace

/* ================================================================== */
/* OAuthLoopbackListener                                              */
/* ================================================================== */

struct OAuthLoopbackListener::Impl {
    httplib::Server server;
    std::thread listenThread;
    mutable std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> running{false};
    std::atomic<int> port{0};
    bool received = false;
    std::string provider;
    std::string token;
    std::function<void(const std::string &, const std::string &)> callback;

    ~Impl()
    {
        stop();
    }

    bool start(
        std::function<void(const std::string &, const std::string &)> cb,
        std::string &errorOut)
    {
        stop();
        {
            std::lock_guard<std::mutex> lock(mutex);
            received = false;
            provider.clear();
            token.clear();
            callback = std::move(cb);
        }

        auto handleOAuthRedirect =
            [this](const httplib::Request &req, httplib::Response &res) {
                std::string prov = "google";
                if (req.has_param("provider")) {
                    prov = req.get_param_value("provider");
                }
                std::string tok;
                if (req.has_param("token")) {
                    tok = req.get_param_value("token");
                } else if (req.has_param("id_token")) {
                    tok = req.get_param_value("id_token");
                } else if (req.has_param("access_token")) {
                    tok = req.get_param_value("access_token");
                }

                if (tok.empty()) {
                    res.status = 400;
                    res.set_content(
                        "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                        "<title>OCTI Sign-In Incomplete</title></head>"
                        "<body><h2>OCTI Sign-In Incomplete: missing token</h2>"
                        "</body></html>",
                        "text/html; charset=utf-8");
                    return;
                }

                std::function<void(const std::string &, const std::string &)>
                    cbCopy;
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    received = true;
                    provider = prov;
                    token = tok;
                    cbCopy = callback;
                }
                cv.notify_all();
                if (cbCopy) {
                    cbCopy(prov, tok);
                }

                res.status = 200;
                res.set_content(
                    "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
                    "<title>Sign-in complete</title></head>"
                    "<body style=\"background:#0B0F17;color:#F8FAFC;"
                    "font-family:sans-serif;text-align:center;padding:48px;\">"
                    "<h2 style=\"color:#C8A253;\">Sign-in complete — "
                    "you may return to OCTI</h2>"
                    "<p>Your desktop session has been authenticated.</p>"
                    "</body></html>",
                    "text/html; charset=utf-8");
            };

        server.Get("/callback", handleOAuthRedirect);
        server.Get("/oauth/callback", handleOAuthRedirect);

        int bound = server.bind_to_any_port("127.0.0.1");
        if (bound <= 0) {
            errorOut = "failed to bind OAuth loopback listener on 127.0.0.1:0";
            return false;
        }
        port.store(bound);
        running.store(true);
        listenThread = std::thread([this]() {
            server.listen_after_bind();
            running.store(false);
        });
        server.wait_until_ready();
        return true;
    }

    void stop()
    {
        if (server.is_running()) {
            server.stop();
        }
        running.store(false);
        cv.notify_all();
        if (listenThread.joinable()) {
            listenThread.join();
        }
        port.store(0);
    }
};

OAuthLoopbackListener::OAuthLoopbackListener()
    : _impl(new Impl())
{
}

OAuthLoopbackListener::~OAuthLoopbackListener()
{
    stop();
}

bool OAuthLoopbackListener::start(
    std::function<void(const std::string &provider,
                       const std::string &token)> onCallback,
    std::string &errorOut)
{
    return _impl->start(std::move(onCallback), errorOut);
}

void OAuthLoopbackListener::stop()
{
    _impl->stop();
}

bool OAuthLoopbackListener::isRunning() const
{
    return _impl->running.load();
}

int OAuthLoopbackListener::boundPort() const
{
    return _impl->port.load();
}

std::string OAuthLoopbackListener::callbackUrl() const
{
    int p = boundPort();
    if (p <= 0) {
        return std::string();
    }
    return "http://127.0.0.1:" + std::to_string(p) + "/callback";
}

bool OAuthLoopbackListener::waitForCallback(
    int timeoutMs,
    std::string &outProvider,
    std::string &outToken)
{
    std::unique_lock<std::mutex> lock(_impl->mutex);
    if (!_impl->received && timeoutMs > 0) {
        _impl->cv.wait_for(
            lock, std::chrono::milliseconds(timeoutMs),
            [this]() { return _impl->received || !_impl->running.load(); });
    }
    if (_impl->received) {
        outProvider = _impl->provider;
        outToken = _impl->token;
        return true;
    }
    return false;
}

/* ================================================================== */
/* OctiNetworkClient::parseSseBuffer                                  */
/* ================================================================== */

std::vector<SseEvent> OctiNetworkClient::parseSseBuffer(std::string &buffer)
{
    std::vector<SseEvent> events;

    for (;;) {
        std::size_t sepPos = std::string::npos;
        std::size_t sepLen = 0;
        for (std::size_t i = 0; i + 1 < buffer.size(); ++i) {
            if (buffer[i] == '\n' && buffer[i + 1] == '\n') {
                sepPos = i;
                sepLen = 2;
                break;
            }
            if (i + 3 < buffer.size() && buffer[i] == '\r' &&
                buffer[i + 1] == '\n' && buffer[i + 2] == '\r' &&
                buffer[i + 3] == '\n') {
                sepPos = i;
                sepLen = 4;
                break;
            }
        }
        if (sepPos == std::string::npos) {
            break;
        }

        std::string rawBlock = buffer.substr(0, sepPos);
        buffer.erase(0, sepPos + sepLen);

        SseEvent ev;
        bool hasEventOrData = false;
        std::istringstream iss(rawBlock);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty() || line[0] == ':') {
                continue;
            }
            if (line.compare(0, 3, "id:") == 0) {
                std::string val = line.substr(3);
                while (!val.empty() && val.front() == ' ') {
                    val.erase(val.begin());
                }
                char *endPtr = nullptr;
                unsigned long long parsed =
                    std::strtoull(val.c_str(), &endPtr, 10);
                if (endPtr != val.c_str()) {
                    ev.id = static_cast<std::uint64_t>(parsed);
                }
            } else if (line.compare(0, 6, "event:") == 0) {
                std::string val = line.substr(6);
                while (!val.empty() && val.front() == ' ') {
                    val.erase(val.begin());
                }
                ev.event = val;
                hasEventOrData = true;
            } else if (line.compare(0, 5, "data:") == 0) {
                std::string val = line.substr(5);
                if (!val.empty() && val.front() == ' ') {
                    val.erase(val.begin());
                }
                if (!ev.dataJson.empty()) {
                    ev.dataJson.push_back('\n');
                }
                ev.dataJson += val;
                hasEventOrData = true;
            }
        }

        if (hasEventOrData && !ev.event.empty()) {
            events.push_back(std::move(ev));
        }
    }

    return events;
}

/* ================================================================== */
/* OctiNetworkClient::Impl                                            */
/* ================================================================== */

struct OctiNetworkClient::Impl {
    struct RawResponse {
        bool transportOk = false;
        int status = 0;
        std::string body;
        std::string contentType;
        std::string transportError;
        int retryAfterSec = 0;
        Json json;
    };

    struct SseRunner {
        std::mutex mutex;
        std::condition_variable cv;
        std::shared_ptr<httplib::Client> activeClient;
        std::thread worker;
        std::atomic<bool> stopRequested{false};
        std::atomic<bool> active{false};
        std::atomic<std::uint64_t> lastId{0};

        ~SseRunner()
        {
            stop();
        }

        void stop()
        {
            stopRequested.store(true);
            std::shared_ptr<httplib::Client> cli;
            {
                std::lock_guard<std::mutex> lock(mutex);
                cli = activeClient;
            }
            if (cli) {
                cli->stop();
            }
            cv.notify_all();
            if (worker.joinable()) {
                worker.join();
            }
            {
                std::lock_guard<std::mutex> lock(mutex);
                activeClient.reset();
            }
            active.store(false);
        }
    };

    std::string host;
    int port;
    mutable std::mutex tokenMutex;
    std::string sessionToken;

    SseRunner lobbyStream;
    SseRunner gameStream;

    Impl(const std::string &h, int p)
        : host(h), port(p)
    {
    }

    ~Impl()
    {
        lobbyStream.stop();
        gameStream.stop();
    }

    std::string currentToken() const
    {
        std::lock_guard<std::mutex> lock(tokenMutex);
        return sessionToken;
    }

    httplib::Headers authHeaders(
        const httplib::Headers &extra = httplib::Headers()) const
    {
        httplib::Headers h = extra;
        h.emplace("X-Octi-Protocol-Version",
                  std::to_string(OCTI_PROTOCOL_VERSION));
        std::string tok = currentToken();
        if (!tok.empty()) {
            h.emplace("Authorization", "Bearer " + tok);
        }
        return h;
    }

    std::unique_ptr<httplib::Client> makeClient(int readTimeoutSec = 10) const
    {
        std::unique_ptr<httplib::Client> cli(
            (port == 443)
                ? new httplib::Client("https://" + host)
                : new httplib::Client(host, port));
        cli->set_connection_timeout(5, 0);
        cli->set_read_timeout(readTimeoutSec, 0);
        cli->set_write_timeout(10, 0);
        return cli;
    }

    static RawResponse wrapResult(const httplib::Result &res)
    {
        RawResponse r;
        if (!res) {
            r.transportOk = false;
            r.transportError = httplib::to_string(res.error());
            return r;
        }
        r.transportOk = true;
        r.status = res->status;
        r.body = res->body;
        r.contentType = res->get_header_value("Content-Type");
        if (res->has_header("Retry-After")) {
            r.retryAfterSec =
                std::max(0, std::atoi(res->get_header_value("Retry-After").c_str()));
        }
        if (!r.body.empty() && (r.body[0] == '{' || r.body[0] == '[')) {
            r.json = Json::parse(r.body, nullptr, false);
            if (r.json.is_discarded()) {
                r.json = Json();
            }
        }
        return r;
    }

    template <typename Fn>
    RawResponse executeWithRetry(Fn &&requestFn) const
    {
        httplib::Result res;
        for (int attempt = 1; attempt <= 5; ++attempt) {
            std::unique_ptr<httplib::Client> cli = makeClient();
            res = requestFn(cli.get());
            if (res) {
                return wrapResult(res);
            }
            if (attempt < 5) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(100 * attempt));
            }
        }
        if (host == OCTI_SERVER_HOST && port == OCTI_SERVER_PORT) {
            std::unique_ptr<httplib::Client> fbCli(
                new httplib::Client(OCTI_SERVER_FALLBACK_HOST, OCTI_SERVER_FALLBACK_PORT));
            fbCli->set_connection_timeout(5, 0);
            fbCli->set_read_timeout(10, 0);
            fbCli->set_write_timeout(10, 0);
            httplib::Result fbRes = requestFn(fbCli.get());
            if (fbRes) {
                return wrapResult(fbRes);
            }
        }
        return wrapResult(res);
    }

    RawResponse doGet(const std::string &path,
                      const httplib::Headers &extra = httplib::Headers()) const
    {
        return executeWithRetry([&](httplib::Client *cli) {
            return cli->Get(path, authHeaders(extra));
        });
    }

    RawResponse doPost(const std::string &path, const Json &body) const
    {
        std::string payload = body.dump();
        return executeWithRetry([&](httplib::Client *cli) {
            return cli->Post(path, authHeaders(), payload, "application/json");
        });
    }

    RawResponse doPatch(const std::string &path, const Json &body) const
    {
        std::string payload = body.dump();
        return executeWithRetry([&](httplib::Client *cli) {
            return cli->Patch(path, authHeaders(), payload, "application/json");
        });
    }

    RawResponse doDelete(const std::string &path) const
    {
        return executeWithRetry([&](httplib::Client *cli) {
            return cli->Delete(path, authHeaders());
        });
    }

    template <typename T>
    static void populateError(const RawResponse &raw, ApiResult<T> &out)
    {
        out.ok = false;
        out.httpStatus = raw.status;
        out.retryAfterSec = raw.retryAfterSec;
        if (!raw.transportOk) {
            out.errorCode = "TRANSPORT_ERROR";
            out.errorMessage = raw.transportError;
            return;
        }
        if (raw.json.is_object()) {
            out.errorCode = raw.json.value("error", "HTTP_ERROR");
            out.errorMessage = raw.json.value("message", "");
        } else {
            out.errorCode = "HTTP_" + std::to_string(raw.status);
            out.errorMessage = raw.body;
        }
    }

    void launchSseStream(
        SseRunner &runner,
        const std::string &basePath,
        std::uint64_t sinceSeq,
        std::function<void(const SseEvent &)> onEvent)
    {
        runner.stop();
        runner.stopRequested.store(false);
        runner.lastId.store(sinceSeq);
        runner.active.store(true);

        runner.worker = std::thread(
            [this, &runner, basePath, onEvent]() {
                while (!runner.stopRequested.load()) {
                    std::uint64_t curSeq = runner.lastId.load();
                    std::string path = basePath +
                                       (basePath.find('?') == std::string::npos
                                            ? "?stream=1&since_seq="
                                            : "&stream=1&since_seq=") +
                                       std::to_string(curSeq);

                    std::shared_ptr<httplib::Client> cli(
                        (host == OCTI_SERVER_HOST && port == 443) || port == 443
                            ? new httplib::Client("https://" + host)
                            : new httplib::Client(host, port));
                    cli->set_connection_timeout(5, 0);
                    cli->set_read_timeout(30, 0);
                    cli->set_write_timeout(5, 0);

                    {
                        std::lock_guard<std::mutex> lock(runner.mutex);
                        if (runner.stopRequested.load()) {
                            break;
                        }
                        runner.activeClient = cli;
                    }

                    httplib::Headers hdrs = authHeaders();
                    hdrs.emplace("Accept", "text/event-stream");
                    if (curSeq > 0) {
                        hdrs.emplace("Last-Event-ID", std::to_string(curSeq));
                    }

                    int httpStatus = 0;
                    bool terminalEventSeen = false;
                    std::string chunkBuffer;

                    httplib::Result res = cli->Get(
                        path, hdrs,
                        [&runner, &httpStatus](const httplib::Response &resp) {
                            httpStatus = resp.status;
                            return !runner.stopRequested.load() &&
                                   resp.status == 200;
                        },
                        [&runner, &chunkBuffer, &terminalEventSeen, &onEvent](
                            const char *data, std::size_t len) {
                            if (runner.stopRequested.load()) {
                                return false;
                            }
                            chunkBuffer.append(data, len);
                            std::vector<SseEvent> parsed =
                                OctiNetworkClient::parseSseBuffer(chunkBuffer);
                            for (std::size_t i = 0; i < parsed.size(); ++i) {
                                if (parsed[i].id > runner.lastId.load()) {
                                    runner.lastId.store(parsed[i].id);
                                }
                                if (parsed[i].event == "closed") {
                                    terminalEventSeen = true;
                                }
                                if (onEvent) {
                                    onEvent(parsed[i]);
                                }
                            }
                            return !runner.stopRequested.load();
                        });

                    {
                        std::lock_guard<std::mutex> lock(runner.mutex);
                        runner.activeClient.reset();
                    }

                    if (runner.stopRequested.load() || terminalEventSeen) {
                        break;
                    }
                    if (httpStatus >= 400 && httpStatus < 500 &&
                        httpStatus != 429) {
                        break;
                    }

                    std::unique_lock<std::mutex> lock(runner.mutex);
                    runner.cv.wait_for(
                        lock, std::chrono::milliseconds(150),
                        [&runner]() { return runner.stopRequested.load(); });
                }
                runner.active.store(false);
            });
    }
};

/* ================================================================== */
/* OctiNetworkClient                                                  */
/* ================================================================== */

OctiNetworkClient::OctiNetworkClient()
    : _impl(new Impl(OCTI_SERVER_HOST, OCTI_SERVER_PORT)),
      _host(OCTI_SERVER_HOST),
      _port(OCTI_SERVER_PORT)
{
}

OctiNetworkClient::OctiNetworkClient(const std::string &host, int port)
    : _impl(new Impl(host, port)),
      _host(host),
      _port(port)
{
}

OctiNetworkClient::~OctiNetworkClient()
{
    stopEventStreams();
}

std::unique_ptr<OctiNetworkClient> OctiNetworkClient::createForLoopbackTest(
    int loopbackPort)
{
    return std::unique_ptr<OctiNetworkClient>(
        new OctiNetworkClient("127.0.0.1", loopbackPort));
}

const std::string &OctiNetworkClient::host() const
{
    return _host;
}

int OctiNetworkClient::port() const
{
    return _port;
}

void OctiNetworkClient::setEndpoint(const std::string &host, int port)
{
    _host = host;
    _port = port;
    _impl->host = host;
    _impl->port = port;
}

std::string OctiNetworkClient::buildUrl(const std::string &path) const
{
    std::string prefix;
    if (_port == 443 || (_host == "octi.selfso.com" && _port == 443)) {
        prefix = "https://" + _host;
    } else {
        prefix = "http://" + _host + ":" + std::to_string(_port);
    }
    if (path.empty()) {
        return prefix;
    }
    if (path.front() != '/') {
        return prefix + "/" + path;
    }
    return prefix + path;
}

void OctiNetworkClient::setSessionToken(const std::string &token)
{
    std::lock_guard<std::mutex> lock(_impl->tokenMutex);
    _impl->sessionToken = token;
}

std::string OctiNetworkClient::sessionToken() const
{
    return _impl->currentToken();
}

void OctiNetworkClient::clearSessionToken()
{
    std::lock_guard<std::mutex> lock(_impl->tokenMutex);
    _impl->sessionToken.clear();
}

/* ---- 1..10: Health, Auth, Profile & Leaderboard ---- */

ApiResult<HealthInfo> OctiNetworkClient::fetchHealth()
{
    ApiResult<HealthInfo> res;
    Impl::RawResponse raw = _impl->doGet("/api/v1/health");
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data.status = raw.json.value("status", "");
    res.data.version = raw.json.value("version", "");
    res.data.uptimeSec =
        raw.json.value("uptime_sec", static_cast<std::int64_t>(0));
    res.data.activeTables =
        raw.json.value("active_tables", static_cast<std::int64_t>(0));
    res.data.onlinePlayers =
        raw.json.value("online_players", static_cast<std::int64_t>(0));
    res.data.protocolVersion =
        raw.json.value("protocol_version", OCTI_PROTOCOL_VERSION);
    res.data.minProtocolVersion =
        raw.json.value("min_protocol_version", OCTI_MIN_PROTOCOL_VERSION);
    return res;
}

ApiResult<std::vector<AuthProviderInfo>> OctiNetworkClient::fetchAuthProviders()
{
    ApiResult<std::vector<AuthProviderInfo>> res;
    Impl::RawResponse raw = _impl->doGet("/api/v1/auth/providers");
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    if (raw.json.contains("providers") && raw.json["providers"].is_array()) {
        const Json &arr = raw.json["providers"];
        for (std::size_t i = 0; i < arr.size(); ++i) {
            if (!arr[i].is_object()) {
                continue;
            }
            AuthProviderInfo info;
            info.name = arr[i].value("name", "");
            info.displayName = arr[i].value("display_name", "");
            if (arr[i].contains("client_ids") &&
                arr[i]["client_ids"].is_array()) {
                for (std::size_t k = 0; k < arr[i]["client_ids"].size(); ++k) {
                    if (arr[i]["client_ids"][k].is_string()) {
                        info.clientIds.push_back(
                            arr[i]["client_ids"][k].get<std::string>());
                    }
                }
            }
            res.data.push_back(info);
        }
    }
    return res;
}

ApiResult<SsoLoginData> OctiNetworkClient::loginWithSso(
    const std::string &provider,
    const std::string &token,
    const std::string &desiredHandle,
    bool isAccessToken)
{
    ApiResult<SsoLoginData> res;
    Json body;
    body["provider"] = provider;
    body["client_agent"] = "octi-desktop";
    if (isAccessToken) {
        body["access_token"] = token;
    } else {
        body["id_token"] = token;
    }

    Impl::RawResponse raw = _impl->doPost("/api/v1/auth/sso", body);
    if (!raw.transportOk || (raw.status != 200 && raw.status != 201) ||
        !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }

    res.ok = true;
    res.httpStatus = raw.status;
    res.data.sessionToken = raw.json.value("session_token", "");
    res.data.expiresAt =
        raw.json.value("expires_at", static_cast<std::int64_t>(0));
    res.data.isNewAccount =
        (raw.status == 201) || raw.json.value("created", false);
    if (raw.json.contains("profile")) {
        res.data.profile = parsePlayerProfile(raw.json["profile"]);
    }
    if (raw.json.contains("linked_identities")) {
        res.data.identities =
            parseLinkedIdentities(raw.json["linked_identities"]);
        res.data.profile.identities = res.data.identities;
    }

    if (!res.data.sessionToken.empty()) {
        setSessionToken(res.data.sessionToken);
    }

    if (!desiredHandle.empty() && !res.data.sessionToken.empty()) {
        ApiResult<PlayerProfileInfo> patched = updateMyProfile(desiredHandle);
        if (patched.ok) {
            res.data.profile = patched.data;
            res.data.profile.identities = res.data.identities;
        }
    }

    return res;
}

ApiResult<LinkIdentityData> OctiNetworkClient::linkIdentity(
    const std::string &provider,
    const std::string &token,
    bool isAccessToken)
{
    ApiResult<LinkIdentityData> res;
    Json body;
    body["provider"] = provider;
    if (isAccessToken) {
        body["access_token"] = token;
    } else {
        body["id_token"] = token;
    }

    Impl::RawResponse raw = _impl->doPost("/api/v1/auth/link", body);
    if (!raw.transportOk) {
        Impl::populateError(raw, res);
        return res;
    }

    if (raw.status == 409 && raw.json.is_object()) {
        Impl::populateError(raw, res);
        res.data.conflictAlreadyLinked = true;
        if (raw.json.contains("existing_profile") &&
            raw.json["existing_profile"].is_object()) {
            res.data.existingProfile =
                parsePlayerProfile(raw.json["existing_profile"]);
        }
        return res;
    }

    if (raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }

    res.ok = true;
    res.httpStatus = raw.status;
    res.data.status = raw.json.value("status", "linked");
    if (raw.json.contains("linked_identities")) {
        res.data.identities =
            parseLinkedIdentities(raw.json["linked_identities"]);
    }
    return res;
}

ApiResult<UnlinkIdentityData> OctiNetworkClient::unlinkIdentity(
    const std::string &provider)
{
    ApiResult<UnlinkIdentityData> res;
    Impl::RawResponse raw =
        _impl->doDelete("/api/v1/auth/link/" + urlEncode(provider));
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data.status = raw.json.value("status", "unlinked");
    if (raw.json.contains("linked_identities")) {
        res.data.identities =
            parseLinkedIdentities(raw.json["linked_identities"]);
    }
    return res;
}

ApiResult<bool> OctiNetworkClient::logout()
{
    ApiResult<bool> res;
    Impl::RawResponse raw = _impl->doPost("/api/v1/auth/logout", Json::object());
    if (!raw.transportOk || raw.status != 200) {
        Impl::populateError(raw, res);
        return res;
    }
    clearSessionToken();
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = true;
    return res;
}

ApiResult<PlayerProfileInfo> OctiNetworkClient::fetchMyProfile()
{
    ApiResult<PlayerProfileInfo> res;
    Impl::RawResponse raw = _impl->doGet("/api/v1/profile/me");
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = parsePlayerProfile(raw.json);
    return res;
}

ApiResult<PlayerProfileInfo> OctiNetworkClient::updateMyProfile(
    const std::string &handle,
    const std::string &avatarUrl,
    const std::string &displayName)
{
    ApiResult<PlayerProfileInfo> res;
    Json body = Json::object();
    if (!handle.empty()) {
        body["handle"] = handle;
    }
    if (!avatarUrl.empty()) {
        body["avatar_url"] = avatarUrl;
    }
    if (!displayName.empty()) {
        body["display_name"] = displayName;
    }
    Impl::RawResponse raw = _impl->doPatch("/api/v1/profile/me", body);
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = parsePlayerProfile(raw.json);
    return res;
}

ApiResult<PlayerProfileInfo> OctiNetworkClient::fetchPlayerProfile(
    const std::string &playerUuid)
{
    ApiResult<PlayerProfileInfo> res;
    Impl::RawResponse raw =
        _impl->doGet("/api/v1/players/" + urlEncode(playerUuid));
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = parsePlayerProfile(raw.json);
    return res;
}

ApiResult<std::vector<LeaderboardRow>> OctiNetworkClient::fetchLeaderboard(
    const std::string &mode,
    int limit)
{
    ApiResult<std::vector<LeaderboardRow>> res;
    std::string path = "/api/v1/leaderboard?mode=" + urlEncode(mode) +
                       "&limit=" + std::to_string(limit);
    Impl::RawResponse raw = _impl->doGet(path);
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    if (raw.json.contains("entries") && raw.json["entries"].is_array()) {
        const Json &arr = raw.json["entries"];
        for (std::size_t i = 0; i < arr.size(); ++i) {
            if (!arr[i].is_object()) {
                continue;
            }
            LeaderboardRow row;
            row.rank = arr[i].value("rank", static_cast<int>(i) + 1);
            row.uuid = arr[i].value("uuid", "");
            row.handle = arr[i].value("handle", "");
            row.displayName = arr[i].value("display_name", "");
            row.elo = arr[i].value("elo", 1200);
            row.gamesPlayed = arr[i].value("games_played", 0);
            row.wins = arr[i].value("wins", 0);
            row.losses = arr[i].value("losses", 0);
            row.draws = arr[i].value("draws", 0);
            res.data.push_back(row);
        }
    }
    return res;
}

/* ---- 11..22: Lobby, Tables & Matchmaking ---- */

ApiResult<LobbyStateData> OctiNetworkClient::fetchLobbyState()
{
    ApiResult<LobbyStateData> res;
    Impl::RawResponse raw = _impl->doGet("/api/v1/lobby/state");
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    if (raw.json.contains("players") && raw.json["players"].is_array()) {
        const Json &pl = raw.json["players"];
        for (std::size_t i = 0; i < pl.size(); ++i) {
            if (!pl[i].is_object()) {
                continue;
            }
            LobbyPlayerInfo p;
            p.playerUuid = pl[i].value("player_uuid", "");
            p.handle = pl[i].value("handle", "");
            p.status = pl[i].value("status", "IN_LOBBY");
            p.roomId = pl[i].value("room_id", "");
            res.data.players.push_back(p);
        }
    }
    if (raw.json.contains("tables") && raw.json["tables"].is_array()) {
        const Json &tb = raw.json["tables"];
        for (std::size_t i = 0; i < tb.size(); ++i) {
            res.data.tables.push_back(parseTableSummary(tb[i]));
        }
    }
    if (raw.json.contains("recent_chat") && raw.json["recent_chat"].is_array()) {
        const Json &ch = raw.json["recent_chat"];
        for (std::size_t i = 0; i < ch.size(); ++i) {
            if (!ch[i].is_object()) {
                continue;
            }
            LobbyChatEntry c;
            c.seq = ch[i].value("seq", static_cast<std::int64_t>(0));
            c.atMs = ch[i].value("at_ms", static_cast<std::int64_t>(0));
            c.playerUuid = ch[i].value("player_uuid", "");
            c.handle = ch[i].value("handle", "");
            c.text = ch[i].value("text", "");
            res.data.recentChat.push_back(c);
        }
    }
    res.data.latestSeq =
        raw.json.value("latest_seq", static_cast<std::int64_t>(0));
    return res;
}

ApiResult<std::int64_t> OctiNetworkClient::postLobbyChat(
    const std::string &text)
{
    ApiResult<std::int64_t> res;
    Json body;
    body["text"] = text;
    Impl::RawResponse raw = _impl->doPost("/api/v1/lobby/chat", body);
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = raw.json.value("seq", static_cast<std::int64_t>(0));
    return res;
}

ApiResult<std::vector<TableSummaryInfo>> OctiNetworkClient::listTables(
    bool includeFinished)
{
    ApiResult<std::vector<TableSummaryInfo>> res;
    std::string path =
        includeFinished ? "/api/v1/tables?include_finished=1" : "/api/v1/tables";
    Impl::RawResponse raw = _impl->doGet(path);
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    if (raw.json.contains("tables") && raw.json["tables"].is_array()) {
        const Json &arr = raw.json["tables"];
        for (std::size_t i = 0; i < arr.size(); ++i) {
            res.data.push_back(parseTableSummary(arr[i]));
        }
    }
    return res;
}

ApiResult<CreateTableResponse> OctiNetworkClient::createTable(
    const CreateTableRequest &req)
{
    ApiResult<CreateTableResponse> res;
    Json body;
    body["mode"] = req.mode;
    body["bases"] = req.bases;
    body["super_prongs"] = req.superProngs;
    body["edgeless"] = req.edgeless;
    body["rated"] = req.rated;
    body["allow_takeback"] = req.allowTakeback;
    body["observer_policy"] = req.observerPolicy;
    body["voice_policy"] = req.voicePolicy;
    if (!req.passcode.empty()) {
        body["passcode"] = req.passcode;
    }
    if (req.timeControlEnabled && req.initialMs > 0) {
        Json tc;
        tc["initial_ms"] = req.initialMs;
        tc["increment_ms"] = req.incrementMs;
        tc["disconnect_grace_ms"] = req.disconnectGraceMs;
        body["time_control"] = tc;
    } else {
        body["time_control"] = "untimed";
    }

    Impl::RawResponse raw = _impl->doPost("/api/v1/tables", body);
    if (!raw.transportOk || (raw.status != 201 && raw.status != 200) ||
        !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data.roomId = raw.json.value("room_id", "");
    res.data.seat = raw.json.value("seat", 1);
    return res;
}

ApiResult<JoinTableResponse> OctiNetworkClient::joinTable(
    const std::string &roomId,
    int seat,
    const std::string &passcode)
{
    ApiResult<JoinTableResponse> res;
    Json body;
    body["seat"] = seat;
    if (!passcode.empty()) {
        body["passcode"] = passcode;
    }
    Impl::RawResponse raw = _impl->doPost(
        "/api/v1/tables/" + urlEncode(roomId) + "/join", body);
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data.roomId = raw.json.value("room_id", roomId);
    res.data.seat = raw.json.value("seat", seat);
    res.data.gameStarted = raw.json.value("game_started", false);
    return res;
}

ApiResult<bool> OctiNetworkClient::leaveTable(const std::string &roomId)
{
    ApiResult<bool> res;
    Impl::RawResponse raw = _impl->doPost(
        "/api/v1/tables/" + urlEncode(roomId) + "/leave", Json::object());
    if (!raw.transportOk || raw.status != 200) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = true;
    return res;
}

ApiResult<bool> OctiNetworkClient::kickSeat(
    const std::string &roomId,
    const std::string &targetPlayerUuid)
{
    ApiResult<bool> res;
    Json body;
    body["target"] = targetPlayerUuid;
    Impl::RawResponse raw = _impl->doPost(
        "/api/v1/tables/" + urlEncode(roomId) + "/kick", body);
    if (!raw.transportOk || raw.status != 200) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = true;
    return res;
}

ApiResult<bool> OctiNetworkClient::kickSeat(
    const std::string &roomId,
    int seat)
{
    ApiResult<GameViewData> gv = fetchGameView(roomId);
    if (!gv.ok) {
        ApiResult<bool> res;
        res.ok = false;
        res.httpStatus = gv.httpStatus;
        res.errorCode = gv.errorCode;
        res.errorMessage = gv.errorMessage;
        return res;
    }
    for (std::size_t i = 0; i < gv.data.seats.size(); ++i) {
        if (gv.data.seats[i].seat == seat &&
            !gv.data.seats[i].playerUuid.empty()) {
            return kickSeat(roomId, gv.data.seats[i].playerUuid);
        }
    }
    return kickSeat(roomId, std::string());
}

ApiResult<bool> OctiNetworkClient::observeTable(
    const std::string &roomId,
    const std::string &passcode)
{
    ApiResult<bool> res;
    Json body = Json::object();
    if (!passcode.empty()) {
        body["passcode"] = passcode;
    }
    Impl::RawResponse raw = _impl->doPost(
        "/api/v1/tables/" + urlEncode(roomId) + "/observe", body);
    if (!raw.transportOk || raw.status != 200) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = true;
    return res;
}

ApiResult<bool> OctiNetworkClient::unobserveTable(const std::string &roomId)
{
    ApiResult<bool> res;
    Impl::RawResponse raw = _impl->doPost(
        "/api/v1/tables/" + urlEncode(roomId) + "/unobserve", Json::object());
    if (!raw.transportOk || raw.status != 200) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = true;
    return res;
}

ApiResult<bool> OctiNetworkClient::enqueueMatchmaking(
    const MatchmakingRequest &req)
{
    ApiResult<bool> res;
    Json body;
    body["mode"] = req.mode;
    body["bases"] = req.bases;
    body["super_prongs"] = req.superProngs;
    body["edgeless"] = req.edgeless;
    if (req.timeControlEnabled && req.initialMs > 0) {
        Json tc;
        tc["initial_ms"] = req.initialMs;
        tc["increment_ms"] = req.incrementMs;
        body["time_control"] = tc;
    } else {
        body["time_control"] = "untimed";
    }
    Impl::RawResponse raw =
        _impl->doPost("/api/v1/matchmaking/enqueue", body);
    if (!raw.transportOk || raw.status != 200) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = true;
    return res;
}

ApiResult<bool> OctiNetworkClient::dequeueMatchmaking()
{
    ApiResult<bool> res;
    Impl::RawResponse raw =
        _impl->doPost("/api/v1/matchmaking/dequeue", Json::object());
    if (!raw.transportOk || raw.status != 200) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = true;
    return res;
}

ApiResult<MatchmakingStatusData> OctiNetworkClient::fetchMatchmakingStatus()
{
    ApiResult<MatchmakingStatusData> res;
    Impl::RawResponse raw = _impl->doGet("/api/v1/matchmaking/status");
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data.queued = raw.json.value("queued", false);
    res.data.waitedMs =
        raw.json.value("waited_ms", static_cast<std::int64_t>(0));
    res.data.window = raw.json.value("window", 0);
    res.data.rating = raw.json.value("rating", 1200);
    res.data.matchedRoomId = raw.json.value("matched_room_id", "");
    return res;
}

/* ---- 23..32: Live Game, Chat, Voice Signaling & Moderation ---- */

ApiResult<GameViewData> OctiNetworkClient::fetchGameView(
    const std::string &roomId)
{
    ApiResult<GameViewData> res;
    Impl::RawResponse raw =
        _impl->doGet("/api/v1/games/" + urlEncode(roomId));
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = parseGameView(raw.json);
    return res;
}

ApiResult<MoveSubmitData> OctiNetworkClient::submitMove(
    const std::string &roomId,
    const std::string &moveStr)
{
    ApiResult<MoveSubmitData> res;
    Json body;
    body["move"] = moveStr;
    Impl::RawResponse raw = _impl->doPost(
        "/api/v1/games/" + urlEncode(roomId) + "/move", body);
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data.move = raw.json.value("move", moveStr);
    res.data.clockMs =
        raw.json.value("clock_ms", static_cast<std::int64_t>(-1));
    res.data.elapsedMs =
        raw.json.value("elapsed_ms", static_cast<std::int64_t>(0));
    res.data.gameOver = raw.json.value("game_over", false);
    res.data.seq = raw.json.value("seq", static_cast<std::int64_t>(0));
    return res;
}

ApiResult<bool> OctiNetworkClient::resignGame(const std::string &roomId)
{
    ApiResult<bool> res;
    Impl::RawResponse raw = _impl->doPost(
        "/api/v1/games/" + urlEncode(roomId) + "/resign", Json::object());
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = raw.json.value("game_over", true);
    return res;
}

ApiResult<GameActionData> OctiNetworkClient::submitGameAction(
    const std::string &roomId,
    const std::string &action)
{
    ApiResult<GameActionData> res;
    Json body;
    body["action"] = action;
    Impl::RawResponse raw = _impl->doPost(
        "/api/v1/games/" + urlEncode(roomId) + "/action", body);
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data.action = raw.json.value("action", action);
    res.data.roomId = raw.json.value("room_id", roomId);
    res.data.gameOver = raw.json.value("game_over", false);
    res.data.seq = raw.json.value("seq", static_cast<std::int64_t>(0));
    return res;
}

ApiResult<std::int64_t> OctiNetworkClient::postGameChat(
    const std::string &roomId,
    const std::string &scope,
    const std::string &text)
{
    ApiResult<std::int64_t> res;
    Json body;
    body["scope"] = toLowerAscii(scope.empty() ? "all" : scope);
    body["text"] = text;
    Impl::RawResponse raw = _impl->doPost(
        "/api/v1/games/" + urlEncode(roomId) + "/chat", body);
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = raw.json.value("seq", static_cast<std::int64_t>(0));
    return res;
}

ApiResult<std::int64_t> OctiNetworkClient::submitVoiceSignal(
    const std::string &roomId,
    const std::string &toPlayerUuid,
    const std::string &signalType,
    const std::string &payloadJson)
{
    ApiResult<std::int64_t> res;
    Json body;
    body["target"] = toPlayerUuid;
    body["type"] = signalType;
    Json parsedPayload = Json::parse(payloadJson, nullptr, false);
    if (!parsedPayload.is_discarded()) {
        body["payload"] = parsedPayload;
    } else {
        body["payload"] = payloadJson;
    }
    Impl::RawResponse raw = _impl->doPost(
        "/api/v1/games/" + urlEncode(roomId) + "/voice-signal", body);
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = raw.json.value("seq", static_cast<std::int64_t>(0));
    return res;
}

ApiResult<std::int64_t> OctiNetworkClient::submitVoiceMeta(
    const std::string &roomId,
    const std::string &eventType,
    const std::string &targetPlayerUuid)
{
    (void)targetPlayerUuid;
    ApiResult<std::int64_t> res;
    std::string action = toUpperAscii(eventType);
    if (action == "SPEAKING_START") {
        action = "MIC_UNMUTED";
    } else if (action == "SPEAKING_STOP") {
        action = "MIC_MUTED";
    }
    Json body;
    body["action"] = action;
    Impl::RawResponse raw = _impl->doPost(
        "/api/v1/games/" + urlEncode(roomId) + "/voice-meta", body);
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = raw.json.value("seq", static_cast<std::int64_t>(0));
    return res;
}

ApiResult<bool> OctiNetworkClient::banObserver(
    const std::string &roomId,
    const std::string &targetPlayerUuid,
    const std::string &reason)
{
    ApiResult<bool> res;
    Json body;
    body["target"] = targetPlayerUuid;
    if (!reason.empty()) {
        body["reason"] = reason;
    }
    Impl::RawResponse raw = _impl->doPost(
        "/api/v1/games/" + urlEncode(roomId) + "/ban", body);
    if (!raw.transportOk || raw.status != 200) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = true;
    return res;
}

ApiResult<bool> OctiNetworkClient::muteParticipant(
    const std::string &roomId,
    const std::string &targetPlayerUuid,
    bool muteChat,
    bool muteVoice)
{
    ApiResult<bool> res;
    Json body;
    body["target"] = targetPlayerUuid;
    body["mute_chat"] = muteChat;
    body["mute_voice"] = muteVoice;
    Impl::RawResponse raw = _impl->doPost(
        "/api/v1/games/" + urlEncode(roomId) + "/mute", body);
    if (!raw.transportOk || raw.status != 200) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = true;
    return res;
}

ApiResult<RoomSettingsData> OctiNetworkClient::updateRoomSettings(
    const std::string &roomId,
    const std::string &observerPolicy,
    const std::string &voicePolicy)
{
    ApiResult<RoomSettingsData> res;
    Json body = Json::object();
    if (!observerPolicy.empty()) {
        body["observer_policy"] = observerPolicy;
    }
    if (!voicePolicy.empty()) {
        body["voice_policy"] = voicePolicy;
    }
    Impl::RawResponse raw = _impl->doPost(
        "/api/v1/games/" + urlEncode(roomId) + "/settings", body);
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data.observerPolicy =
        raw.json.value("observer_policy", observerPolicy);
    res.data.voicePolicy = raw.json.value("voice_policy", voicePolicy);
    res.data.seq = raw.json.value("seq", static_cast<std::int64_t>(0));
    return res;
}

/* ---- 33..35: Game Archive & .octi Transcript ---- */

ApiResult<ArchivedGamesPage> OctiNetworkClient::listArchivedGames(
    const std::string &mode,
    const std::string &playerUuid,
    int limit,
    int offset)
{
    ApiResult<ArchivedGamesPage> res;
    std::string path = "/api/v1/archive/games?limit=" +
                       std::to_string(limit) +
                       "&offset=" + std::to_string(offset);
    if (!mode.empty()) {
        path += "&mode=" + urlEncode(mode);
    }
    if (!playerUuid.empty()) {
        path += "&player=" + urlEncode(playerUuid);
    }
    Impl::RawResponse raw = _impl->doGet(path);
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data.total = raw.json.value("total", static_cast<std::int64_t>(0));
    res.data.limit = raw.json.value("limit", limit);
    res.data.offset = raw.json.value("offset", offset);
    if (raw.json.contains("games") && raw.json["games"].is_array()) {
        const Json &arr = raw.json["games"];
        for (std::size_t i = 0; i < arr.size(); ++i) {
            res.data.games.push_back(parseArchivedGameSummary(arr[i]));
        }
    }
    return res;
}

ApiResult<ArchivedGameSummary> OctiNetworkClient::fetchArchivedGame(
    const std::string &gameId)
{
    ApiResult<ArchivedGameSummary> res;
    Impl::RawResponse raw =
        _impl->doGet("/api/v1/archive/games/" + urlEncode(gameId));
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = parseArchivedGameSummary(raw.json);
    return res;
}

ApiResult<std::string> OctiNetworkClient::downloadOctiTranscript(
    const std::string &gameId)
{
    ApiResult<std::string> res;
    Impl::RawResponse raw =
        _impl->doGet("/api/v1/archive/games/" + urlEncode(gameId) + "/transcript");
    if (!raw.transportOk || raw.status != 200) {
        raw = _impl->doGet("/api/v1/archive/games/" + urlEncode(gameId) + "/octi");
        if (!raw.transportOk || raw.status != 200) {
            Impl::populateError(raw, res);
            return res;
        }
    }
    res.ok = true;
    res.httpStatus = raw.status;
    res.data = raw.body;
    return res;
}

ApiResult<UploadTranscriptResponse> OctiNetworkClient::uploadTrainingTranscript(
    const UploadTranscriptRequest &req)
{
    ApiResult<UploadTranscriptResponse> res;
    Json body;
    Json parsed = Json::parse(req.transcriptJson, nullptr, false);
    if (!parsed.is_discarded() && parsed.is_object()) {
        body["transcript"] = parsed;
    } else {
        body["transcript"] = req.transcriptJson;
    }
    if (!req.clientId.empty()) {
        body["clientId"] = req.clientId;
    }

    Impl::RawResponse raw = _impl->doPost("/api/v1/archive/upload", body);
    if (!raw.transportOk || raw.status != 200) {
        Impl::populateError(raw, res);
        return res;
    }

    res.ok = true;
    res.httpStatus = raw.status;
    if (raw.json.is_object()) {
        if (raw.json.contains("game_id") && raw.json["game_id"].is_string()) {
            res.data.gameId = raw.json["game_id"].get<std::string>();
        } else if (raw.json.contains("gameId") && raw.json["gameId"].is_string()) {
            res.data.gameId = raw.json["gameId"].get<std::string>();
        }
        if (raw.json.contains("ply_count") && raw.json["ply_count"].is_number_integer()) {
            res.data.plyCount = raw.json["ply_count"].get<int>();
        } else if (raw.json.contains("plyCount") && raw.json["plyCount"].is_number_integer()) {
            res.data.plyCount = raw.json["plyCount"].get<int>();
        }
    }
    return res;
}

ApiResult<int> OctiNetworkClient::deleteInstallArchiveData(
    const std::string &installId)
{
    ApiResult<int> res;
    Impl::RawResponse raw =
        _impl->doDelete("/api/v1/archive/install/" + urlEncode(installId));
    if (!raw.transportOk || raw.status != 200) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    if (raw.json.is_object() && raw.json.contains("deleted") &&
        raw.json["deleted"].is_number_integer()) {
        res.data = raw.json["deleted"].get<int>();
    }
    return res;
}

/* ---- Poll & Server-Sent Events (SSE) Live Streams ---- */

ApiResult<std::vector<SseEvent>> OctiNetworkClient::pollLobbyEvents(
    std::uint64_t sinceSeq,
    int limit)
{
    ApiResult<std::vector<SseEvent>> res;
    std::string path = "/api/v1/lobby/events?since_seq=" +
                       std::to_string(sinceSeq) +
                       "&limit=" + std::to_string(limit);
    Impl::RawResponse raw = _impl->doGet(path);
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    if (raw.json.contains("events") && raw.json["events"].is_array()) {
        const Json &arr = raw.json["events"];
        for (std::size_t i = 0; i < arr.size(); ++i) {
            if (!arr[i].is_object()) {
                continue;
            }
            SseEvent ev;
            ev.id = arr[i].value("seq", static_cast<std::uint64_t>(0));
            ev.event = arr[i].value("type", "");
            ev.dataJson = arr[i].dump();
            res.data.push_back(std::move(ev));
        }
    }
    return res;
}

ApiResult<std::vector<SseEvent>> OctiNetworkClient::pollGameEvents(
    const std::string &roomId,
    std::uint64_t sinceSeq,
    int limit)
{
    ApiResult<std::vector<SseEvent>> res;
    std::string path = "/api/v1/games/" + urlEncode(roomId) +
                       "/events?since_seq=" + std::to_string(sinceSeq) +
                       "&limit=" + std::to_string(limit);
    Impl::RawResponse raw = _impl->doGet(path);
    if (!raw.transportOk || raw.status != 200 || !raw.json.is_object()) {
        Impl::populateError(raw, res);
        return res;
    }
    res.ok = true;
    res.httpStatus = raw.status;
    if (raw.json.contains("events") && raw.json["events"].is_array()) {
        const Json &arr = raw.json["events"];
        for (std::size_t i = 0; i < arr.size(); ++i) {
            if (!arr[i].is_object()) {
                continue;
            }
            SseEvent ev;
            ev.id = arr[i].value("seq", static_cast<std::uint64_t>(0));
            ev.event = arr[i].value("type", "");
            ev.dataJson = arr[i].dump();
            res.data.push_back(std::move(ev));
        }
    }
    return res;
}

void OctiNetworkClient::startLobbyEventStream(
    std::uint64_t sinceSeq,
    std::function<void(const SseEvent &)> onEvent)
{
    _impl->launchSseStream(_impl->lobbyStream, "/api/v1/lobby/events",
                           sinceSeq, std::move(onEvent));
}

void OctiNetworkClient::stopLobbyEventStream()
{
    _impl->lobbyStream.stop();
}

bool OctiNetworkClient::isLobbyEventStreamActive() const
{
    return _impl->lobbyStream.active.load();
}

std::uint64_t OctiNetworkClient::lastLobbyEventId() const
{
    return _impl->lobbyStream.lastId.load();
}

void OctiNetworkClient::startGameEventStream(
    const std::string &roomId,
    std::uint64_t sinceSeq,
    std::function<void(const SseEvent &)> onEvent)
{
    _impl->launchSseStream(
        _impl->gameStream,
        "/api/v1/games/" + urlEncode(roomId) + "/events",
        sinceSeq,
        std::move(onEvent));
}

void OctiNetworkClient::stopGameEventStream()
{
    _impl->gameStream.stop();
}

bool OctiNetworkClient::isGameEventStreamActive() const
{
    return _impl->gameStream.active.load();
}

std::uint64_t OctiNetworkClient::lastGameEventId() const
{
    return _impl->gameStream.lastId.load();
}

void OctiNetworkClient::stopEventStreams()
{
    stopLobbyEventStream();
    stopGameEventStream();
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
