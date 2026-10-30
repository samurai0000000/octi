/*
 * TestOctiHttpServerIntegration.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

/*
 * Live loopback-socket qualification of OctiHttpServer. Every test starts a
 * real OctiHttpServer on an ephemeral 127.0.0.1 port in front of a real
 * AccountStore (SQLite in a private temp directory), OidcVerifier and
 * GameRoomManager, and talks to it with httplib::Client over TCP. SSO
 * tokens are genuine RS256 / ES256 JWTs signed by keys generated here and
 * injected into the verifier with setKeySet(); no real identity provider is
 * ever contacted. The only stand-in is the Graph API fetcher of the one
 * Facebook access_token test, which sits at the remote boundary.
 */

#include "AccountStore.hxx"
#include "EloCalculator.hxx"
#include "GameRoomManager.hxx"
#include "OctiBoard.hxx"
#include "OctiCore.hxx"
#include "OctiGame.hxx"
#include "OctiHttpServer.hxx"
#include "OidcVerifier.hxx"
#include "ServerApp.hxx"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

#include <arpa/inet.h>
#include <dirent.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness.h>

using namespace octi;

typedef nlohmann::json Json;

namespace {

/* ------------------------------------------------------------------ */
/* OpenSSL helpers: real keys, real signatures                         */
/* ------------------------------------------------------------------ */

struct PkeyDeleter {
    void operator()(EVP_PKEY *p) const { EVP_PKEY_free(p); }
};
typedef std::unique_ptr<EVP_PKEY, PkeyDeleter> Pkey;

struct BnDeleter {
    void operator()(BIGNUM *p) const { BN_free(p); }
};
typedef std::unique_ptr<BIGNUM, BnDeleter> Bn;

std::string bnParam(EVP_PKEY *key, const char *name, int padTo)
{
    BIGNUM *raw = nullptr;
    if (EVP_PKEY_get_bn_param(key, name, &raw) != 1) {
        return std::string();
    }
    Bn bn(raw);
    int len = padTo > 0 ? padTo : BN_num_bytes(bn.get());
    std::string out(static_cast<size_t>(len), '\0');
    if (padTo > 0) {
        BN_bn2binpad(bn.get(), reinterpret_cast<unsigned char *>(&out[0]), len);
    } else {
        BN_bn2bin(bn.get(), reinterpret_cast<unsigned char *>(&out[0]));
    }
    return out;
}

std::string digestSign(EVP_PKEY *key, const std::string &input)
{
    std::unique_ptr<EVP_MD_CTX, void (*)(EVP_MD_CTX *)> md(EVP_MD_CTX_new(),
                                                            EVP_MD_CTX_free);
    size_t len = 0;
    if (!md || EVP_DigestSignInit(md.get(), nullptr, EVP_sha256(), nullptr,
                                  key) != 1 ||
        EVP_DigestSign(md.get(), nullptr, &len,
                       reinterpret_cast<const unsigned char *>(input.data()),
                       input.size()) != 1) {
        return std::string();
    }
    std::string sig(len, '\0');
    if (EVP_DigestSign(md.get(), reinterpret_cast<unsigned char *>(&sig[0]),
                       &len,
                       reinterpret_cast<const unsigned char *>(input.data()),
                       input.size()) != 1) {
        return std::string();
    }
    sig.resize(len);
    return sig;
}

/* ES256 JWS signature is raw r || s (64 bytes), not ASN.1 DER. */
std::string signEs256(EVP_PKEY *key, const std::string &input)
{
    std::string der = digestSign(key, input);
    const unsigned char *p = reinterpret_cast<const unsigned char *>(der.data());
    ECDSA_SIG *sig = d2i_ECDSA_SIG(nullptr, &p, static_cast<long>(der.size()));
    if (!sig) {
        return std::string();
    }
    const BIGNUM *r = nullptr;
    const BIGNUM *s = nullptr;
    ECDSA_SIG_get0(sig, &r, &s);
    std::string raw(64, '\0');
    BN_bn2binpad(r, reinterpret_cast<unsigned char *>(&raw[0]), 32);
    BN_bn2binpad(s, reinterpret_cast<unsigned char *>(&raw[32]), 32);
    ECDSA_SIG_free(sig);
    return raw;
}

struct TestKeys {
    Pkey rsa;
    Pkey ec;
    TestKeys()
        : rsa(EVP_PKEY_Q_keygen(nullptr, nullptr, "RSA", size_t(2048))),
          ec(EVP_PKEY_Q_keygen(nullptr, nullptr, "EC", "P-256"))
    {
    }
};

TestKeys &keys()
{
    static TestKeys k;
    return k;
}

std::string jwks()
{
    Json rsa;
    rsa["kty"] = "RSA";
    rsa["use"] = "sig";
    rsa["kid"] = "rsa-1";
    rsa["alg"] = "RS256";
    rsa["n"] = base64UrlEncode(bnParam(keys().rsa.get(), OSSL_PKEY_PARAM_RSA_N, 0));
    rsa["e"] = base64UrlEncode(bnParam(keys().rsa.get(), OSSL_PKEY_PARAM_RSA_E, 0));
    Json ec;
    ec["kty"] = "EC";
    ec["use"] = "sig";
    ec["kid"] = "ec-1";
    ec["crv"] = "P-256";
    ec["x"] = base64UrlEncode(bnParam(keys().ec.get(), OSSL_PKEY_PARAM_EC_PUB_X, 32));
    ec["y"] = base64UrlEncode(bnParam(keys().ec.get(), OSSL_PKEY_PARAM_EC_PUB_Y, 32));
    Json doc;
    doc["keys"] = Json::array();
    doc["keys"].push_back(rsa);
    doc["keys"].push_back(ec);
    return doc.dump();
}

int64_t realNow()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string issuerFor(const std::string &provider)
{
    if (provider == "google") {
        return "https://accounts.google.com";
    }
    if (provider == "apple") {
        return "https://appleid.apple.com";
    }
    if (provider == "microsoft") {
        return "https://login.microsoftonline.com/"
               "9188040d-6c67-4c5b-b112-36a304b66dad/v2.0";
    }
    if (provider == "facebook") {
        return "https://www.facebook.com";
    }
    return "https://access.line.me";
}

/* LINE uses ES256, everyone else RS256: both algorithms stay exercised. */
std::string makeToken(const std::string &provider, const std::string &sub,
                      const std::string &name,
                      const std::function<void(Json &, Json &)> &mutate =
                          std::function<void(Json &, Json &)>())
{
    Json header;
    header["alg"] = provider == "line" ? "ES256" : "RS256";
    header["typ"] = "JWT";
    header["kid"] = provider == "line" ? "ec-1" : "rsa-1";
    Json claims;
    claims["iss"] = issuerFor(provider);
    claims["aud"] = "client-" + provider;
    claims["sub"] = sub;
    claims["iat"] = realNow() - 10;
    claims["exp"] = realNow() + 3600;
    claims["email"] = "user@example.com";
    claims["name"] = name;
    claims["picture"] = "https://example.com/avatar.png";
    if (mutate) {
        mutate(header, claims);
    }
    std::string input =
        base64UrlEncode(header.dump()) + "." + base64UrlEncode(claims.dump());
    std::string alg = header.value("alg", "RS256");
    std::string sig = alg == "ES256" ? signEs256(keys().ec.get(), input)
                                     : digestSign(keys().rsa.get(), input);
    return input + "." + base64UrlEncode(sig);
}

/* ------------------------------------------------------------------ */
/* HTTP client                                                         */
/* ------------------------------------------------------------------ */

struct Reply {
    int status = -1;
    std::string body;
    std::string contentType;
    std::string error;
    httplib::Headers headers;
    Json json;

    std::string header(const std::string &k) const
    {
        httplib::Headers::const_iterator it = headers.find(k);
        return it == headers.end() ? std::string() : it->second;
    }
};

class Http {
public:
    explicit Http(int port) : _port(port) {}

    Reply get(const std::string &path, const std::string &token = "",
              const httplib::Headers &extra = httplib::Headers()) const
    {
        httplib::Client cli = client();
        return wrap(cli.Get(path, headersFor(token, extra)));
    }

    Reply post(const std::string &path, const std::string &token,
               const Json &body,
               const httplib::Headers &extra = httplib::Headers()) const
    {
        return raw("POST", path, token, body.dump(), "application/json", extra);
    }

    Reply patch(const std::string &path, const std::string &token,
                const Json &body,
                const httplib::Headers &extra = httplib::Headers()) const
    {
        return raw("PATCH", path, token, body.dump(), "application/json", extra);
    }

    Reply del(const std::string &path, const std::string &token,
              const httplib::Headers &extra = httplib::Headers()) const
    {
        httplib::Client cli = client();
        return wrap(cli.Delete(path, headersFor(token, extra)));
    }

    Reply raw(const std::string &method, const std::string &path,
              const std::string &token, const std::string &body,
              const std::string &contentType,
              const httplib::Headers &extra = httplib::Headers()) const
    {
        httplib::Client cli = client();
        httplib::Headers h = headersFor(token, extra);
        if (method == "PATCH") {
            return wrap(cli.Patch(path, h, body, contentType));
        }
        return wrap(cli.Post(path, h, body, contentType));
    }

    int port() const { return _port; }

private:
    httplib::Client client() const
    {
        httplib::Client cli("127.0.0.1", _port);
        cli.set_connection_timeout(5, 0);
        cli.set_read_timeout(15, 0);
        cli.set_write_timeout(15, 0);
        return cli;
    }

    static httplib::Headers headersFor(const std::string &token,
                                       const httplib::Headers &extra)
    {
        httplib::Headers h = extra;
        if (!token.empty()) {
            h.insert(std::make_pair("Authorization", "Bearer " + token));
        }
        return h;
    }

    static Reply wrap(const httplib::Result &res)
    {
        Reply r;
        if (!res) {
            r.error = httplib::to_string(res.error());
            return r;
        }
        r.status = res->status;
        r.body = res->body;
        r.headers = res->headers;
        r.contentType = res->get_header_value("Content-Type");
        if (!r.body.empty() && (r.body[0] == '{' || r.body[0] == '[')) {
            r.json = Json::parse(r.body, nullptr, false);
            if (r.json.is_discarded()) {
                r.json = Json();
            }
        }
        return r;
    }

    int _port;
};

/* The reply expression is evaluated exactly once (it performs a request). */
#define EXPECT_STATUS(code, reply)                                            \
    do {                                                                      \
        const Reply &expectReply_ = (reply);                                  \
        LONGS_EQUAL_TEXT((code), expectReply_.status,                         \
                         (expectReply_.error + expectReply_.body).c_str());   \
    } while (0)

#define EXPECT_ERROR(code, name, reply)                                       \
    do {                                                                      \
        const Reply &expectErr_ = (reply);                                    \
        LONGS_EQUAL_TEXT((code), expectErr_.status,                           \
                         (expectErr_.error + expectErr_.body).c_str());       \
        STRCMP_EQUAL((name), expectErr_.json.value("error", "").c_str());     \
    } while (0)

/* ------------------------------------------------------------------ */
/* Environment                                                         */
/* ------------------------------------------------------------------ */

HttpServerConfig testConfig()
{
    HttpServerConfig c;
    c.bindAddress = "127.0.0.1";
    c.port = 0;
    c.tickIntervalMs = 50;
    c.ssePollMs = 25;
    c.sseHeartbeatMs = 200;
    c.sseSessionRecheckMs = 100;
    c.uploadRateLimitMax = 100;
    c.exportRateLimitMax = 100;
    for (int i = 0; i < 5; ++i) {
        static const char *names[] = {"google", "apple", "microsoft",
                                      "facebook", "line"};
        c.providerClientIds[names[i]].push_back(std::string("client-") +
                                                names[i]);
    }
    return c;
}

struct Env {
    std::string dir;
    std::string dbPath;
    AccountStore store;
    OidcVerifier verifier;
    std::unique_ptr<GameRoomManager> rooms;
    std::unique_ptr<OctiHttpServer> server;

    Env(const HttpServerConfig &cfg, bool lineEnabled, const RoomLimits *limits)
    {
        char tmpl[] = "/tmp/octi_http_XXXXXX";
        if (mkdtemp(tmpl) == nullptr) {
            FAIL("mkdtemp failed");
        }
        dir = tmpl;
        dbPath = dir + "/octi.db";
        std::string err;
        if (!store.openOrCreate(dbPath, err)) {
            FAIL(err.c_str());
        }
        static const char *names[] = {"google", "apple", "microsoft",
                                      "facebook", "line"};
        for (int i = 0; i < 5; ++i) {
            std::string n = names[i];
            if (n == "line" && !lineEnabled) {
                continue;
            }
            verifier.setClientId(n, "client-" + n);
            if (!verifier.setKeySet(n, jwks())) {
                FAIL("setKeySet failed");
            }
        }
        rooms.reset(new GameRoomManager(store));
        if (limits != nullptr) {
            rooms->setLimits(*limits);
        }
        server.reset(new OctiHttpServer(store, verifier, *rooms, cfg));
        if (!server->start(err)) {
            FAIL(err.c_str());
        }
    }

    ~Env()
    {
        server.reset();
        rooms.reset();
        store.close();
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    Http http() const { return Http(server->port()); }
};

int threadCount()
{
    int n = 0;
    DIR *d = opendir("/proc/self/task");
    if (d == nullptr) {
        return -1;
    }
    while (struct dirent *e = readdir(d)) {
        if (e->d_name[0] != '.') {
            ++n;
        }
    }
    closedir(d);
    return n;
}

/* ------------------------------------------------------------------ */
/* Server-Sent Events client                                           */
/* ------------------------------------------------------------------ */

class SseClient {
public:
    SseClient() : _status(-1), _finished(false) {}
    ~SseClient() { stop(); }

    void start(int port, const std::string &path, const std::string &token,
               const httplib::Headers &extra = httplib::Headers())
    {
        _client.reset(new httplib::Client("127.0.0.1", port));
        _client->set_connection_timeout(5, 0);
        _client->set_read_timeout(30, 0);
        httplib::Headers h = extra;
        h.insert(std::make_pair("Accept", "text/event-stream"));
        if (!token.empty()) {
            h.insert(std::make_pair("Authorization", "Bearer " + token));
        }
        httplib::Client *cli = _client.get();
        _thread = std::thread([this, cli, path, h]() {
            httplib::Result res = cli->Get(
                path, h,
                [this](const httplib::Response &r) {
                    std::lock_guard<std::mutex> lock(_mutex);
                    _status = r.status;
                    _contentType = r.get_header_value("Content-Type");
                    return true;
                },
                [this](const char *data, size_t len) {
                    std::lock_guard<std::mutex> lock(_mutex);
                    _data.append(data, len);
                    return true;
                });
            std::lock_guard<std::mutex> lock(_mutex);
            _finished = true;
        });
    }

    bool waitFor(const std::string &needle, int timeoutMs)
    {
        const int64_t deadline = nowMs() + timeoutMs;
        while (nowMs() < deadline) {
            if (data().find(needle) != std::string::npos) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return data().find(needle) != std::string::npos;
    }

    bool waitFinished(int timeoutMs)
    {
        const int64_t deadline = nowMs() + timeoutMs;
        while (nowMs() < deadline) {
            if (finished()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return finished();
    }

    void stop()
    {
        if (_client) {
            _client->stop();
        }
        if (_thread.joinable()) {
            _thread.join();
        }
    }

    std::string data() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _data;
    }

    int status() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _status;
    }

    std::string contentType() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _contentType;
    }

    bool finished() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _finished;
    }

    /* Parsed "data:" payloads of every frame whose event name is type. */
    std::vector<Json> events(const std::string &type) const
    {
        std::vector<Json> out;
        std::string all = data();
        size_t pos = 0;
        while (pos < all.size()) {
            size_t end = all.find("\n\n", pos);
            if (end == std::string::npos) {
                break;
            }
            std::string frame = all.substr(pos, end - pos);
            pos = end + 2;
            if (frame.find("event: " + type + "\n") == std::string::npos) {
                continue;
            }
            size_t d = frame.find("data: ");
            if (d != std::string::npos) {
                Json j = Json::parse(frame.substr(d + 6), nullptr, false);
                if (!j.is_discarded()) {
                    out.push_back(j);
                }
            }
        }
        return out;
    }

private:
    static int64_t nowMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
    }

    std::unique_ptr<httplib::Client> _client;
    std::thread _thread;
    mutable std::mutex _mutex;
    std::string _data;
    std::string _contentType;
    int _status;
    bool _finished;
};

/* A plain listening TCP socket that occupies a loopback port. */
class PortHolder {
public:
    PortHolder() : _fd(-1), _port(0)
    {
        _fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (_fd < 0) {
            return;
        }
        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        socklen_t len = sizeof(addr);
        if (::bind(_fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0 ||
            ::listen(_fd, 4) != 0 ||
            ::getsockname(_fd, reinterpret_cast<struct sockaddr *>(&addr), &len) != 0) {
            ::close(_fd);
            _fd = -1;
            return;
        }
        _port = ntohs(addr.sin_port);
    }
    ~PortHolder()
    {
        if (_fd >= 0) {
            ::close(_fd);
        }
    }
    PortHolder(const PortHolder &) = delete;
    PortHolder &operator=(const PortHolder &) = delete;
    int port() const { return _port; }

private:
    int _fd;
    int _port;
};

struct User {
    std::string token;
    std::string uuid;
    std::string handle;
};

} // namespace

/* ------------------------------------------------------------------ */
/* Fixture                                                             */
/* ------------------------------------------------------------------ */

TEST_GROUP(ServerFoundationHttpTest)
{
};

TEST_GROUP(OctiHttpServerIntegrationTest)
{
    std::unique_ptr<Env> env;

    void setup() { startEnv(testConfig(), true, nullptr); }

    void teardown() { env.reset(); }

    void startEnv(const HttpServerConfig &cfg, bool lineEnabled,
                  const RoomLimits *limits)
    {
        env.reset();
        env.reset(new Env(cfg, lineEnabled, limits));
    }

    Http http() const { return env->http(); }

    /* ---- identities ---- */

    User signIn(const std::string &provider, const std::string &sub,
                const std::string &name)
    {
        Reply r = http().post(
            "/api/v1/auth/sso", "",
            Json{{"provider", provider},
                 {"id_token", makeToken(provider, sub, name)},
                 {"client_agent", "octi-test/1.0"}});
        CHECK_TEXT(r.status == 200 || r.status == 201,
                   (r.error + r.body).c_str());
        User u;
        u.token = r.json.value("session_token", "");
        u.uuid = r.json["profile"].value("uuid", "");
        u.handle = r.json["profile"].value("handle", "");
        CHECK_FALSE(u.token.empty());
        CHECK_FALSE(u.uuid.empty());
        return u;
    }

    /* ---- tables and games ---- */

    std::string createTable(const User &host, Json cfg = Json::object())
    {
        Reply r = http().post("/api/v1/tables", host.token, cfg);
        EXPECT_STATUS(201, r);
        CHECK_TRUE(r.json.value("ok", false));
        return r.json.value("room_id", "");
    }

    /* Host creates a table, every other user takes the next seat. */
    std::string startGame(const std::vector<User> &users,
                          Json cfg = Json::object())
    {
        std::string room = createTable(users[0], cfg);
        for (size_t i = 1; i < users.size(); ++i) {
            Reply j = http().post("/api/v1/tables/" + room + "/join",
                                  users[i].token, Json::object());
            EXPECT_STATUS(200, j);
            CHECK_EQUAL(static_cast<int>(i) + 1, j.json.value("seat", 0));
            CHECK_EQUAL(i + 1 == users.size(), j.json.value("game_started", false));
        }
        return room;
    }

    Json view(const std::string &room, const std::string &token = "")
    {
        Reply r = http().get("/api/v1/games/" + room, token);
        EXPECT_STATUS(200, r);
        return r.json;
    }

    Json events(const std::string &room, const std::string &token,
                int64_t since = 0)
    {
        Reply r = http().get("/api/v1/games/" + room +
                                 "/events?since_seq=" + std::to_string(since),
                             token);
        EXPECT_STATUS(200, r);
        return r.json;
    }

    static std::vector<Json> ofType(const Json &evs, const std::string &type)
    {
        std::vector<Json> out;
        for (size_t i = 0; i < evs["events"].size(); ++i) {
            if (evs["events"][i].value("type", "") == type) {
                out.push_back(evs["events"][i]);
            }
        }
        return out;
    }

    Reply act(const std::string &room, const User &u, const std::string &action)
    {
        return http().post("/api/v1/games/" + room + "/action", u.token,
                           Json{{"action", action}});
    }

    /*
     * Plays plies legal moves chosen from a local engine that mirrors the
     * room, submitting each through the HTTP API as the seat to move.
     * Returns the number of plies played.
     */
    int playPlies(const std::string &room, const std::vector<User> &users,
                  OctiGameMode mode, int plies, Json cfg = Json::object())
    {
        OctiBoard local(mode, cfg.value("super_prongs", false),
                        cfg.value("edgeless", false),
                        cfg.value("bases", 3) == 1 ? GAME_BASES_1
                                                   : GAME_BASES_3);
        OctiGame *g = local.getGame();
        g->start(PLAYER_A);
        int played = 0;
        for (int i = 0; i < plies && !g->isGameOver(); ++i) {
            std::vector<OctiMove> legal = g->generateLegalMoves();
            CHECK_TRUE(!legal.empty());
            OctiMove mv = legal[static_cast<size_t>(i * 7) % legal.size()];
            std::string text = mv.asString();
            int seat = static_cast<int>(g->getTurn());
            Reply r = http().post("/api/v1/games/" + room + "/move",
                                  users[static_cast<size_t>(seat - 1)].token,
                                  Json{{"move", text}});
            EXPECT_STATUS(200, r);
            STRCMP_EQUAL(text.c_str(), r.json.value("move", "").c_str());
            g->move(static_cast<OctiPlayer>(seat), text);
            ++played;
        }
        return played;
    }

    /* A draw ends a game quickly: both players sign off on it. */
    std::string playDrawGame(const User &a, const User &b)
    {
        std::vector<User> users;
        users.push_back(a);
        users.push_back(b);
        std::string room = startGame(users);
        EXPECT_STATUS(200, act(room, a, "offer_draw"));
        Reply r = act(room, b, "accept_draw");
        EXPECT_STATUS(200, r);
        CHECK_TRUE(r.json.value("game_over", false));
        return room;
    }

    static std::string repeat(char c, size_t n) { return std::string(n, c); }
};

/* ================================================================== */
/* Foundation: CLI options and lifecycle                               */
/* ================================================================== */

TEST(OctiHttpServerIntegrationTest, HealthProvidersAndUnknownRoute)
{
    Reply h = http().get("/api/v1/health");
    EXPECT_STATUS(200, h);
    STRCMP_EQUAL("ok", h.json.value("status", "").c_str());
    CHECK_FALSE(h.json.value("version", "").empty());
    CHECK_EQUAL(0, h.json.value("active_tables", -1));
    CHECK_EQUAL(0, h.json.value("online_players", -1));
    CHECK_TRUE(h.json.value("uptime_sec", -1) >= 0);
    STRCMP_EQUAL("nosniff", h.header("X-Content-Type-Options").c_str());

    Reply p = http().get("/api/v1/auth/providers");
    EXPECT_STATUS(200, p);
    std::set<std::string> names;
    for (size_t i = 0; i < p.json["providers"].size(); ++i) {
        const Json &e = p.json["providers"][i];
        names.insert(e.value("name", ""));
        STRCMP_EQUAL(("client-" + e.value("name", "")).c_str(),
                     e["client_ids"][0].get<std::string>().c_str());
        CHECK_FALSE(e.value("display_name", "").empty());
    }
    std::set<std::string> want = {"google", "apple", "microsoft", "facebook",
                                  "line"};
    CHECK_TRUE(names == want);

    Reply nf = http().get("/api/v1/nope");
    EXPECT_ERROR(404, "NOT_FOUND", nf);
    Reply bad = http().get("/api/v1/games/..%2Fetc");
    EXPECT_STATUS(404, bad);
}

TEST(OctiHttpServerIntegrationTest, ProvidersListOnlyConfiguredOnes)
{
    startEnv(testConfig(), false, nullptr);
    Reply p = http().get("/api/v1/auth/providers");
    EXPECT_STATUS(200, p);
    CHECK_EQUAL(4, static_cast<int>(p.json["providers"].size()));
    for (size_t i = 0; i < p.json["providers"].size(); ++i) {
        CHECK_TRUE(p.json["providers"][i].value("name", "") != "line");
    }
    /* A disabled provider cannot be used to sign in. */
    Reply r = http().post("/api/v1/auth/sso", "",
                          Json{{"provider", "line"},
                               {"id_token", makeToken("line", "x", "Nobody")}});
    EXPECT_ERROR(400, "PROVIDER_DISABLED", r);
}

TEST(OctiHttpServerIntegrationTest, StartOnBusyPortFailsAndStopIsIdempotent)
{
    HttpServerConfig busy = testConfig();
    busy.port = env->server->port();
    OctiHttpServer second(env->store, env->verifier, *env->rooms, busy);
    std::string err;
    CHECK_FALSE(second.start(err));
    CHECK_TRUE(err.find("cannot bind") != std::string::npos);
    CHECK_FALSE(second.isRunning());
    CHECK_EQUAL(0, second.port());
    second.stop();
    second.stop();

    /* The first server is unaffected. */
    EXPECT_STATUS(200, http().get("/api/v1/health"));

    int port = env->server->port();
    CHECK_TRUE(port > 0);
    CHECK_TRUE(env->server->isRunning());
    env->server->stop();
    env->server->stop();
    CHECK_FALSE(env->server->isRunning());
    Reply after = http().get("/api/v1/health");
    CHECK_EQUAL(-1, after.status);      /* connection refused */
    CHECK_FALSE(env->server->start(err));   /* no restart of a stopped server */
}

TEST(OctiHttpServerIntegrationTest, MaintenanceTickRunsOnWorkerThread)
{
    uint64_t before = env->server->tickCount();
    const int64_t deadline =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count() +
        3000;
    while (env->server->tickCount() < before + 3 &&
           std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
                   .count() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK_TRUE(env->server->tickCount() >= before + 3);
}

TEST(OctiHttpServerIntegrationTest, CleanShutdownLeavesNoThreadsBehind)
{
    env.reset();
    const int baseline = threadCount();
    CHECK_TRUE(baseline > 0);
    startEnv(testConfig(), true, nullptr);
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    std::vector<User> users = {a, b};
    std::string room = startGame(users);
    SseClient sse;
    sse.start(env->server->port(), "/api/v1/games/" + room + "/events", a.token);
    CHECK_TRUE(sse.waitFor("retry: 3000", 3000));
    CHECK_EQUAL(1, env->server->activeStreams());
    CHECK_TRUE(threadCount() > baseline);

    env->server->stop();                 /* with a stream still open */
    CHECK_TRUE(sse.waitFinished(5000));
    CHECK_TRUE(sse.data().find("event: reconnect") != std::string::npos);
    CHECK_EQUAL(0, env->server->activeStreams());
    sse.stop();
    env.reset();

    int after = threadCount();
    for (int i = 0; i < 200 && after != baseline; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        after = threadCount();
    }
    CHECK_EQUAL(baseline, after);
}

/* ================================================================== */
/* Nominal                                                             */
/* ================================================================== */

TEST(OctiHttpServerIntegrationTest, NominalFullScenario)
{
    /* ---- Client A: Google sign-in, then link LINE and Microsoft ---- */
    Reply s1 = http().post("/api/v1/auth/sso", "",
                           Json{{"provider", "google"},
                                {"id_token", makeToken("google", "sub-a",
                                                       "Alice Anders")}});
    EXPECT_STATUS(201, s1);
    CHECK_TRUE(s1.json.value("created", false));
    User a;
    a.token = s1.json.value("session_token", "");
    a.uuid = s1.json["profile"].value("uuid", "");
    a.handle = s1.json["profile"].value("handle", "");
    CHECK_TRUE(a.token.size() >= 40);
    CHECK_EQUAL(1200, s1.json["profile"].value("elo_2p", 0));
    CHECK_EQUAL(1, static_cast<int>(s1.json["linked_identities"].size()));
    /* No server-side secrets leak into the sign-in answer. */
    CHECK_TRUE(s1.body.find("sub-a") == std::string::npos);

    Reply l1 = http().post("/api/v1/auth/link", a.token,
                           Json{{"provider", "line"},
                                {"id_token", makeToken("line", "line-sub-a",
                                                       "Alice Line")}});
    EXPECT_STATUS(200, l1);
    STRCMP_EQUAL("linked", l1.json.value("status", "").c_str());
    Reply l2 = http().post("/api/v1/auth/link", a.token,
                           Json{{"provider", "microsoft"},
                                {"id_token", makeToken("microsoft", "ms-sub-a",
                                                       "Alice MS")}});
    EXPECT_STATUS(200, l2);
    CHECK_EQUAL(3, static_cast<int>(l2.json["linked_identities"].size()));
    /* Linking the same identity twice is idempotent. */
    Reply l3 = http().post("/api/v1/auth/link", a.token,
                           Json{{"provider", "microsoft"},
                                {"id_token", makeToken("microsoft", "ms-sub-a",
                                                       "Alice MS")}});
    EXPECT_STATUS(200, l3);
    STRCMP_EQUAL("already_linked", l3.json.value("status", "").c_str());

    Reply me = http().get("/api/v1/profile/me", a.token);
    EXPECT_STATUS(200, me);
    std::set<std::string> providers;
    for (size_t i = 0; i < me.json["linked_identities"].size(); ++i) {
        providers.insert(me.json["linked_identities"][i].value("provider", ""));
    }
    std::set<std::string> want = {"google", "line", "microsoft"};
    CHECK_TRUE(providers == want);

    /* Signing in through a linked provider reaches the same profile. */
    Reply again = http().post("/api/v1/auth/sso", "",
                              Json{{"provider", "line"},
                                   {"id_token", makeToken("line", "line-sub-a",
                                                          "Alice Line")}});
    EXPECT_STATUS(200, again);
    CHECK_FALSE(again.json.value("created", true));
    STRCMP_EQUAL(a.uuid.c_str(), again.json["profile"].value("uuid", "").c_str());

    /* ---- B (Apple) and observer C (Facebook) ---- */
    User b = signIn("apple", "sub-b", "Bob Brown");
    User c = signIn("facebook", "sub-c", "Carol Clark");
    CHECK_TRUE(a.uuid != b.uuid && b.uuid != c.uuid && a.uuid != c.uuid);

    /* ---- lobby chat and presence ---- */
    Reply chat = http().post("/api/v1/lobby/chat", a.token,
                             Json{{"text", "hello lobby"}});
    EXPECT_STATUS(200, chat);
    CHECK_TRUE(chat.json.value("seq", 0) > 0);
    Reply lev = http().get("/api/v1/lobby/events?since_seq=0", b.token);
    EXPECT_STATUS(200, lev);
    bool sawChat = false;
    for (size_t i = 0; i < lev.json["events"].size(); ++i) {
        const Json &e = lev.json["events"][i];
        if (e.value("type", "") == "LOBBY_CHAT") {
            STRCMP_EQUAL("hello lobby", e.value("text", "").c_str());
            STRCMP_EQUAL(a.handle.c_str(), e.value("handle", "").c_str());
            STRCMP_EQUAL(a.uuid.c_str(), e.value("player_uuid", "").c_str());
            sawChat = true;
        }
    }
    CHECK_TRUE(sawChat);
    Reply lobby = http().get("/api/v1/lobby/state", c.token);
    EXPECT_STATUS(200, lobby);
    CHECK_EQUAL(3, static_cast<int>(lobby.json["players"].size()));
    for (size_t i = 0; i < lobby.json["players"].size(); ++i) {
        STRCMP_EQUAL("IN_LOBBY",
                     lobby.json["players"][i].value("status", "").c_str());
    }
    CHECK_EQUAL(1, static_cast<int>(lobby.json["recent_chat"].size()));

    /* ---- table, observer, second seat ---- */
    std::string room = createTable(a, Json{{"mode", "2P"}, {"rated", true}});
    Reply tables = http().get("/api/v1/tables");
    EXPECT_STATUS(200, tables);
    CHECK_EQUAL(1, static_cast<int>(tables.json["tables"].size()));
    STRCMP_EQUAL("WAITING_FOR_PLAYERS",
                 tables.json["tables"][0].value("phase", "").c_str());
    CHECK_EQUAL(1, tables.json["tables"][0].value("seats_filled", 0));

    EXPECT_STATUS(200, http().post("/api/v1/tables/" + room + "/observe",
                                   c.token, Json::object()));
    Reply join = http().post("/api/v1/tables/" + room + "/join", b.token,
                             Json{{"seat", 0}});
    EXPECT_STATUS(200, join);
    CHECK_TRUE(join.json.value("game_started", false));
    CHECK_EQUAL(2, join.json.value("seat", 0));

    Reply lobby2 = http().get("/api/v1/lobby/state", c.token);
    std::map<std::string, std::string> status;
    for (size_t i = 0; i < lobby2.json["players"].size(); ++i) {
        status[lobby2.json["players"][i].value("player_uuid", "")] =
            lobby2.json["players"][i].value("status", "");
    }
    STRCMP_EQUAL("PLAYING", status[a.uuid].c_str());
    STRCMP_EQUAL("PLAYING", status[b.uuid].c_str());
    STRCMP_EQUAL("OBSERVING", status[c.uuid].c_str());

    Json anon = view(room);
    STRCMP_EQUAL("IN_PROGRESS", anon.value("phase", "").c_str());
    CHECK_EQUAL(1, anon.value("turn_seat", 0));
    CHECK_EQUAL(0, anon.value("viewer_seat", -1));
    CHECK_EQUAL(1, anon.value("observer_count", 0));
    STRCMP_EQUAL(a.uuid.c_str(), anon["seats"][0].value("player_uuid", "").c_str());
    STRCMP_EQUAL(b.handle.c_str(), anon["seats"][1].value("handle", "").c_str());
    CHECK_EQUAL(1, view(room, a.token).value("viewer_seat", 0));
    CHECK_EQUAL(2, view(room, b.token).value("viewer_seat", 0));

    /* ---- board text chat (players and observer) ---- */
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/chat", a.token,
                                   Json{{"text", "gl hf"}, {"scope", "all"}}));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/chat", c.token,
                                   Json{{"text", "spam one"}}));
    Json evs = events(room, b.token);
    std::vector<Json> chats = ofType(evs, "CHAT");
    CHECK_EQUAL(2, static_cast<int>(chats.size()));
    STRCMP_EQUAL("gl hf", chats[0].value("text", "").c_str());
    STRCMP_EQUAL("spam one", chats[1].value("text", "").c_str());

    /* ---- peer-to-peer voice: signaling and meta only ---- */
    Reply sig = http().post("/api/v1/games/" + room + "/voice-signal", a.token,
                            Json{{"target", b.uuid},
                                 {"type", "SDP_OFFER"},
                                 {"payload", Json{{"sdp", "v=0 offer"}}}});
    EXPECT_STATUS(200, sig);
    std::vector<Json> toB = ofType(events(room, b.token), "VOICE_SIGNAL");
    CHECK_EQUAL(1, static_cast<int>(toB.size()));
    STRCMP_EQUAL("SDP_OFFER", toB[0].value("signal_type", "").c_str());
    STRCMP_EQUAL("v=0 offer", toB[0]["payload"].value("sdp", "").c_str());
    STRCMP_EQUAL(a.uuid.c_str(), toB[0].value("player_uuid", "").c_str());
    /* The relayed signal is private to its target. */
    CHECK_EQUAL(0, static_cast<int>(ofType(events(room, c.token), "VOICE_SIGNAL").size()));
    CHECK_EQUAL(0, static_cast<int>(ofType(events(room, ""), "VOICE_SIGNAL").size()));
    CHECK_EQUAL(0, static_cast<int>(ofType(events(room, a.token), "VOICE_SIGNAL").size()));

    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/voice-meta",
                                   a.token, Json{{"action", "JOINED_VOICE"}}));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/voice-meta",
                                   b.token, Json{{"action", "JOINED_VOICE"}}));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/voice-meta",
                                   a.token, Json{{"action", "MIC_MUTED"}}));
    Json afterVoice = view(room);
    CHECK_EQUAL(2, static_cast<int>(afterVoice["voice_peers"].size()));
    bool aMuted = false;
    for (size_t i = 0; i < afterVoice["voice_peers"].size(); ++i) {
        if (afterVoice["voice_peers"][i].value("player_uuid", "") == a.uuid) {
            aMuted = afterVoice["voice_peers"][i].value("mic_muted", false);
        }
    }
    CHECK_TRUE(aMuted);
    std::vector<Json> meta = ofType(events(room, ""), "VOICE_META");
    CHECK_EQUAL(3, static_cast<int>(meta.size()));
    STRCMP_EQUAL("MIC_MUTED", meta[2].value("action", "").c_str());
    /* PLAYERS_ONLY: the observer may not use voice. */
    Reply obsVoice = http().post("/api/v1/games/" + room + "/voice-meta",
                                 c.token, Json{{"action", "JOINED_VOICE"}});
    EXPECT_ERROR(403, "FORBIDDEN_POLICY", obsVoice);

    /* ---- moves ---- */
    Reply outOfTurn = http().post("/api/v1/games/" + room + "/move", b.token,
                                  Json{{"move", "37+e"}});
    EXPECT_ERROR(403, "NOT_YOUR_TURN", outOfTurn);
    std::vector<User> players = {a, b};
    CHECK_EQUAL(4, playPlies(room, players, GAME_MODE_2, 4));
    CHECK_EQUAL(4, view(room).value("ply_count", 0));
    CHECK_EQUAL(4, static_cast<int>(view(room)["moves"].size()));

    /* ---- pause / resume freezes play ---- */
    EXPECT_STATUS(200, act(room, a, "request_pause"));
    Reply pauseAccept = act(room, b, "accept_pause");
    EXPECT_STATUS(200, pauseAccept);
    CHECK_TRUE(view(room).value("paused", false));
    Reply movePaused = http().post("/api/v1/games/" + room + "/move", a.token,
                                   Json{{"move", "33+b"}});
    EXPECT_ERROR(409, "INVALID_STATE", movePaused);
    EXPECT_STATUS(200, act(room, a, "resume"));
    CHECK_FALSE(view(room).value("paused", true));
    std::vector<Json> pauseEvents = ofType(events(room, ""), "GAME_PAUSED");
    CHECK_EQUAL(1, static_cast<int>(pauseEvents.size()));
    CHECK_EQUAL(1, static_cast<int>(ofType(events(room, ""), "GAME_RESUMED").size()));

    /* ---- observer nuisance, ban, purge ---- */
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/chat", c.token,
                                   Json{{"text", "spam two"}}));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/chat", c.token,
                                   Json{{"text", "spam three"}}));
    int cChats = 0;
    std::vector<Json> beforeBan = ofType(events(room, a.token), "CHAT");
    for (size_t i = 0; i < beforeBan.size(); ++i) {
        if (beforeBan[i].value("handle", "") == c.handle) {
            ++cChats;
        }
    }
    CHECK_EQUAL(3, cChats);

    Reply ban = http().post("/api/v1/games/" + room + "/ban", a.token,
                            Json{{"target", c.uuid}, {"reason", "spamming"}});
    EXPECT_STATUS(200, ban);
    Json afterBan = events(room, a.token);
    int cLeft = 0;
    int aChats = 0;
    for (size_t i = 0; i < afterBan["events"].size(); ++i) {
        const Json &e = afterBan["events"][i];
        if (e.value("type", "") == "CHAT") {
            (e.value("handle", "") == c.handle ? cLeft : aChats)++;
        }
    }
    CHECK_EQUAL(0, cLeft);                      /* C's messages are purged */
    CHECK_EQUAL(1, aChats);                     /* A's are kept */
    std::vector<Json> banned = ofType(afterBan, "OBSERVER_BANNED");
    CHECK_EQUAL(1, static_cast<int>(banned.size()));
    STRCMP_EQUAL(c.uuid.c_str(), banned[0].value("target_uuid", "").c_str());
    STRCMP_EQUAL("spamming", banned[0].value("reason", "").c_str());

    EXPECT_ERROR(403, "FORBIDDEN_BANNED",
                 http().get("/api/v1/games/" + room, c.token));
    EXPECT_ERROR(403, "FORBIDDEN_BANNED",
                 http().get("/api/v1/games/" + room + "/events", c.token));
    EXPECT_ERROR(403, "FORBIDDEN_BANNED",
                 http().post("/api/v1/games/" + room + "/chat", c.token,
                             Json{{"text", "still here?"}}));
    EXPECT_ERROR(403, "FORBIDDEN_BANNED",
                 http().post("/api/v1/tables/" + room + "/observe", c.token,
                             Json::object()));

    /* ---- finish the game, ratings and archive ---- */
    Reply resign = http().post("/api/v1/games/" + room + "/resign", b.token,
                               Json::object());
    EXPECT_STATUS(200, resign);
    CHECK_TRUE(resign.json.value("game_over", false));
    Json done = view(room);
    STRCMP_EQUAL("FINISHED", done.value("phase", "").c_str());
    STRCMP_EQUAL("P1_WIN", done.value("result", "").c_str());
    STRCMP_EQUAL("RESIGNATION", done.value("termination", "").c_str());
    CHECK_TRUE(done.value("archived", false));
    CHECK_EQUAL(0, static_cast<int>(http().get("/api/v1/tables").json["tables"].size()));

    Reply lb = http().get("/api/v1/leaderboard?mode=2P&limit=10");
    EXPECT_STATUS(200, lb);
    CHECK_EQUAL(3, static_cast<int>(lb.json["entries"].size()));
    EloCalculator::DuelUpdate up =
        EloCalculator::updateDuel(1200, 1200, EloOutcome::FIRST_WINS);
    STRCMP_EQUAL(a.uuid.c_str(), lb.json["entries"][0].value("uuid", "").c_str());
    CHECK_EQUAL(up.newRating1, lb.json["entries"][0].value("elo", 0));
    CHECK_TRUE(lb.json["entries"][0].value("elo", 0) > 1200);
    CHECK_EQUAL(1, lb.json["entries"][0].value("wins", 0));
    CHECK_EQUAL(1, lb.json["entries"][0].value("rank", 0));
    bool sawLoser = false;
    for (size_t i = 0; i < lb.json["entries"].size(); ++i) {
        if (lb.json["entries"][i].value("uuid", "") == b.uuid) {
            CHECK_EQUAL(up.newRating2, lb.json["entries"][i].value("elo", 0));
            CHECK_EQUAL(1, lb.json["entries"][i].value("losses", 0));
            sawLoser = true;
        }
    }
    CHECK_TRUE(sawLoser);

    Reply player = http().get("/api/v1/players/" + a.uuid);
    EXPECT_STATUS(200, player);
    CHECK_EQUAL(1, player.json.value("games_played", 0));
    CHECK_EQUAL(1, static_cast<int>(player.json["recent_games"].size()));
    CHECK_TRUE(player.body.find("email") == std::string::npos);
    CHECK_TRUE(player.body.find("player_id") == std::string::npos);
    CHECK_TRUE(player.body.find("linked_identities") == std::string::npos);

    Reply list = http().get("/api/v1/archive/games");
    EXPECT_STATUS(200, list);
    CHECK_EQUAL(1, list.json.value("total", 0));
    const std::string listedId = list.json["games"][0].value("game_id", "");
    STRCMP_EQUAL(room.c_str(), listedId.c_str());
    STRCMP_EQUAL("2P", list.json["games"][0].value("mode", "").c_str());
    CHECK_EQUAL(4, list.json["games"][0].value("ply_count", 0));
    Reply detail = http().get("/api/v1/archive/games/" + room);
    EXPECT_STATUS(200, detail);
    CHECK_FALSE(detail.json.value("octi_transcript", "").empty());
    CHECK_FALSE(detail.json.value("final_fen", "").empty());
    STRCMP_EQUAL(a.handle.c_str(),
                 detail.json["players"][0].value("handle", "").c_str());

    Reply octi = http().get("/api/v1/archive/games/" + room + "/transcript");
    EXPECT_STATUS(200, octi);
    STRCMP_EQUAL("application/json; charset=utf-8", octi.contentType.c_str());
    STRCMP_EQUAL(detail.json.value("octi_transcript", "").c_str(),
                 octi.body.c_str());
    CHECK_TRUE(octi.header("Content-Disposition").find(room + ".json") !=
               std::string::npos);
    /* The downloaded transcript replays to the archived final position. */
    OctiBoard replay;
    replay.deserializeGame(octi.body);
    STRCMP_EQUAL(detail.json.value("final_fen", "").c_str(),
                 replay.toPositionString().c_str());
    CHECK_EQUAL(detail.json.value("ply_count", -1), replay.getGame()->getNumMoves());

    /* ---- the embedded UI ---- */
    Reply ui = http().get("/");
    EXPECT_STATUS(200, ui);
    CHECK_TRUE(ui.contentType.find("text/html") == 0);
    CHECK_TRUE(ui.body.find("<!DOCTYPE html>") != std::string::npos);
    CHECK_TRUE(ui.body.find("<title>OCTI") != std::string::npos);
    CHECK_TRUE(ui.body.find("<canvas id=\"board-canvas\">") != std::string::npos);
    CHECK_TRUE(ui.body.find("THEME_PALETTES") != std::string::npos);
    CHECK_TRUE(ui.body.find("renderBoardCanvas") != std::string::npos);
    CHECK_TRUE(ui.body.find("drawOctagonPod") != std::string::npos);
    CHECK_TRUE(ui.body.find("drawProngs") != std::string::npos);
    CHECK_TRUE(ui.body.find("LAST_MOVE_DISPLAY_DURATION_MS = 5000") != std::string::npos);
    CHECK_TRUE(ui.body.find("theme-select") != std::string::npos);
    CHECK_TRUE(ui.body.find("btn-rotate") != std::string::npos);
    CHECK_TRUE(ui.body.find("res-card-a") != std::string::npos);
    CHECK_TRUE(ui.body.find("res-card-b") != std::string::npos);
    CHECK_TRUE(ui.body.find("stack-inspector-body") != std::string::npos);
    CHECK_TRUE(ui.body.find("Archive Replay") != std::string::npos);
    CHECK_TRUE(ui.body.find("EventSource") != std::string::npos);
    CHECK_TRUE(ui.body.find("/api/v1/leaderboard") != std::string::npos);
    CHECK_TRUE(ui.body.find("/api/v1/archive/games") != std::string::npos);
    CHECK_FALSE(ui.header("Content-Security-Policy").empty());
    /* Self-contained: no external resource of any kind. */
    CHECK_TRUE(ui.body.find("http://") == std::string::npos);
    CHECK_TRUE(ui.body.find("https://") == std::string::npos);
    CHECK_TRUE(ui.body.find("src=\"//") == std::string::npos);
    CHECK_TRUE(ui.body.find("<link") == std::string::npos);
    CHECK_TRUE(ui.body.find("<iframe") == std::string::npos);
}

TEST(OctiHttpServerIntegrationTest, ProfilePatchAndPublicProfile)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");

    Reply p = http().patch("/api/v1/profile/me", a.token,
                           Json{{"handle", "alice_wins"},
                                {"display_name", "Alice the Great"}});
    EXPECT_STATUS(200, p);
    STRCMP_EQUAL("alice_wins", p.json.value("handle", "").c_str());
    STRCMP_EQUAL("Alice the Great", p.json.value("display_name", "").c_str());
    Reply pub = http().get("/api/v1/players/" + a.uuid);
    EXPECT_STATUS(200, pub);
    STRCMP_EQUAL("alice_wins", pub.json.value("handle", "").c_str());

    EXPECT_ERROR(409, "HANDLE_TAKEN",
                 http().patch("/api/v1/profile/me", b.token,
                              Json{{"handle", "alice_wins"}}));
    EXPECT_ERROR(400, "INVALID_HANDLE",
                 http().patch("/api/v1/profile/me", b.token,
                              Json{{"handle", "ab"}}));
    EXPECT_ERROR(400, "INVALID_HANDLE",
                 http().patch("/api/v1/profile/me", b.token,
                              Json{{"handle", repeat('x', 25)}}));
    EXPECT_STATUS(200, http().patch("/api/v1/profile/me", b.token,
                                    Json{{"handle", repeat('y', 24)}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().patch("/api/v1/profile/me", b.token, Json::object()));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().patch("/api/v1/profile/me", b.token,
                              Json{{"handle", 12345}}));
    EXPECT_ERROR(404, "NOT_FOUND", http().get("/api/v1/players/not-a-player"));
}

TEST(OctiHttpServerIntegrationTest, FacebookAccessTokenSignInThroughGraphBoundary)
{
    /* The Graph API is the remote boundary: only its fetcher is a stand-in. */
    struct Graph {
        std::vector<std::string> urls;
    } graph;
    env->verifier.setFacebookAppSecret("app-secret");
    env->verifier.setHttpFetcher(
        [&graph](const std::string &url, HttpResponse &resp) {
            graph.urls.push_back(url);
            resp.status = 200;
            if (url.find("/debug_token?") != std::string::npos) {
                Json d;
                d["data"]["is_valid"] = true;
                d["data"]["app_id"] = "client-facebook";
                d["data"]["user_id"] = "fb-user-77";
                d["data"]["expires_at"] = 0;
                resp.body = d.dump();
            } else {
                Json m;
                m["id"] = "fb-user-77";
                m["name"] = "Fay Book";
                m["email"] = "fay@example.com";
                resp.body = m.dump();
            }
            return true;
        });
    Reply r = http().post("/api/v1/auth/sso", "",
                          Json{{"provider", "facebook"},
                               {"access_token", "EAAB-graph-token"}});
    EXPECT_STATUS(201, r);
    STRCMP_EQUAL("Fay Book", r.json["profile"].value("display_name", "").c_str());
    CHECK_EQUAL(2, static_cast<int>(graph.urls.size()));
    CHECK_TRUE(graph.urls[0].find("debug_token") != std::string::npos);

    /* access_token is a Facebook-only credential; both fields is ambiguous. */
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post("/api/v1/auth/sso", "",
                             Json{{"provider", "google"},
                                  {"access_token", "x"}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post("/api/v1/auth/sso", "",
                             Json{{"provider", "facebook"},
                                  {"access_token", "x"},
                                  {"id_token", "y"}}));

    /* Graph says the token is invalid: rejected, no session. */
    env->verifier.setHttpFetcher(
        [](const std::string &url, HttpResponse &resp) {
            resp.status = 200;
            resp.body = url.find("/debug_token?") != std::string::npos
                            ? Json{{"data", {{"is_valid", false}}}}.dump()
                            : "{}";
            return true;
        });
    EXPECT_ERROR(401, "TOKEN_REJECTED",
                 http().post("/api/v1/auth/sso", "",
                             Json{{"provider", "facebook"},
                                  {"access_token", "revoked"}}));
    /* Graph unreachable: 503, not a crash and not a 401. */
    env->verifier.setHttpFetcher(
        [](const std::string &, HttpResponse &) { return false; });
    EXPECT_ERROR(503, "REMOTE_FAILURE",
                 http().post("/api/v1/auth/sso", "",
                             Json{{"provider", "facebook"},
                                  {"access_token", "any"}}));
}

TEST(OctiHttpServerIntegrationTest, MatchmakingPairsTwoPlayersOverHttp)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");

    Reply qa = http().post("/api/v1/matchmaking/enqueue", a.token,
                           Json{{"mode", "2P"}});
    EXPECT_STATUS(200, qa);
    CHECK_TRUE(qa.json.value("queued", false));
    EXPECT_ERROR(409, "CONFLICT",
                 http().post("/api/v1/matchmaking/enqueue", a.token,
                             Json{{"mode", "2P"}}));
    Reply st = http().get("/api/v1/matchmaking/status", a.token);
    EXPECT_STATUS(200, st);
    CHECK_TRUE(st.json.value("queued", false));
    CHECK_TRUE(st.json.value("matched_room_id", "x").empty());

    EXPECT_STATUS(200, http().post("/api/v1/matchmaking/enqueue", b.token,
                                   Json{{"mode", "2P"}}));
    std::string room;
    for (int i = 0; i < 200 && room.empty(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        room = http().get("/api/v1/matchmaking/status", a.token)
                   .json.value("matched_room_id", "");
    }
    CHECK_FALSE(room.empty());
    const std::string bRoom = http().get("/api/v1/matchmaking/status", b.token)
                                  .json.value("matched_room_id", "");
    STRCMP_EQUAL(room.c_str(), bRoom.c_str());
    Json v = view(room);
    STRCMP_EQUAL("IN_PROGRESS", v.value("phase", "").c_str());
    CHECK_TRUE(v.value("rated", false));
    std::set<std::string> seated = {v["seats"][0].value("player_uuid", ""),
                                    v["seats"][1].value("player_uuid", "")};
    std::set<std::string> expected = {a.uuid, b.uuid};
    CHECK_TRUE(seated == expected);
    CHECK_FALSE(http().get("/api/v1/matchmaking/status", a.token)
                    .json.value("queued", true));
    /* Not queued any more. */
    EXPECT_ERROR(409, "INVALID_STATE",
                 http().post("/api/v1/matchmaking/dequeue", a.token,
                             Json::object()));
}

TEST(OctiHttpServerIntegrationTest, DequeueLeavesTheQueue)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    EXPECT_STATUS(200, http().post("/api/v1/matchmaking/enqueue", a.token,
                                   Json{{"mode", "OFK"}}));
    EXPECT_STATUS(200, http().post("/api/v1/matchmaking/dequeue", a.token,
                                   Json::object()));
    CHECK_FALSE(http().get("/api/v1/matchmaking/status", a.token)
                    .json.value("queued", true));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post("/api/v1/matchmaking/enqueue", a.token,
                             Json{{"mode", "5P"}}));
}

TEST(OctiHttpServerIntegrationTest, OfkAndFourPlayerTablesPlayOverHttp)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    User c = signIn("microsoft", "sub-c", "Carol Clark");
    User d = signIn("line", "sub-d", "Dave Dunn");

    /* OCTI for Kids: single base, no superprongs. */
    std::vector<User> two = {a, b};
    Json ofk = Json{{"mode", "OFK"}, {"time_control", "300+2"}};
    std::string kids = startGame(two, ofk);
    Json kv = view(kids);
    STRCMP_EQUAL("OFK", kv.value("mode", "").c_str());
    CHECK_EQUAL(1, kv.value("bases", 0));
    CHECK_TRUE(kv["time_control"].value("enabled", false));
    CHECK_EQUAL(300000, kv["time_control"].value("initial_ms", 0));
    CHECK_EQUAL(2000, kv["time_control"].value("increment_ms", 0));
    CHECK_TRUE(kv["seats"][0].value("clock_ms", -1) > 0);
    CHECK_EQUAL(3, playPlies(kids, two, GAME_MODE_OFK, 3, Json{{"bases", 1}}));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + kids + "/resign", b.token,
                                   Json::object()));
    STRCMP_EQUAL("OFK", http().get("/api/v1/archive/games?mode=OFK")
                             .json["games"][0].value("mode", "").c_str());
    Reply ofkBoard = http().get("/api/v1/leaderboard?mode=OFK");
    EXPECT_STATUS(200, ofkBoard);
    STRCMP_EQUAL(a.uuid.c_str(), ofkBoard.json["entries"][0].value("uuid", "").c_str());
    CHECK_TRUE(ofkBoard.json["entries"][0].value("elo", 0) > 1200);
    /* OFK ratings are separate from the 2P ones. */
    CHECK_EQUAL(1200, http().get("/api/v1/leaderboard?mode=2P")
                          .json["entries"][0].value("elo", 0));

    /* 2v2: seats 1+3 against 2+4; team chat only reaches teammates. */
    std::vector<User> four = {a, b, c, d};
    std::string quad = startGame(four, Json{{"mode", "4P"}});
    Json qv = view(quad);
    STRCMP_EQUAL("4P", qv.value("mode", "").c_str());
    CHECK_EQUAL(4, static_cast<int>(qv["seats"].size()));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + quad + "/chat", a.token,
                                   Json{{"text", "plan: go left"}, {"scope", "team"}}));
    CHECK_EQUAL(1, static_cast<int>(ofType(events(quad, c.token), "CHAT").size()));
    CHECK_EQUAL(1, static_cast<int>(ofType(events(quad, a.token), "CHAT").size()));
    CHECK_EQUAL(0, static_cast<int>(ofType(events(quad, b.token), "CHAT").size()));
    CHECK_EQUAL(0, static_cast<int>(ofType(events(quad, d.token), "CHAT").size()));
    CHECK_EQUAL(0, static_cast<int>(ofType(events(quad, ""), "CHAT").size()));
    /* Team scope is meaningless in a 2-player game. */
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post("/api/v1/games/" + kids + "/chat", a.token,
                             Json{{"text", "x"}, {"scope", "team"}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post("/api/v1/games/" + quad + "/chat", a.token,
                             Json{{"text", "x"}, {"scope", "everyone"}}));
    CHECK_EQUAL(2, playPlies(quad, four, GAME_MODE_4, 2));
}

TEST(OctiHttpServerIntegrationTest, DrawAndRematchSwapSeats)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    std::string room = playDrawGame(a, b);
    Json v = view(room);
    STRCMP_EQUAL("DRAW", v.value("result", "").c_str());
    STRCMP_EQUAL("AGREED_DRAW", v.value("termination", "").c_str());

    EXPECT_STATUS(200, act(room, a, "request_rematch"));
    Reply acc = act(room, b, "accept_rematch");
    EXPECT_STATUS(200, acc);
    std::string next = view(room).value("rematch_room_id", "");
    CHECK_FALSE(next.empty());
    CHECK_TRUE(next != room);
    Json nv = view(next);
    STRCMP_EQUAL("IN_PROGRESS", nv.value("phase", "").c_str());
    STRCMP_EQUAL(b.uuid.c_str(), nv["seats"][0].value("player_uuid", "").c_str());
    STRCMP_EQUAL(a.uuid.c_str(), nv["seats"][1].value("player_uuid", "").c_str());
}

TEST(OctiHttpServerIntegrationTest, TakebackOnCasualTable)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    std::vector<User> users = {a, b};
    /* Rated tables never allow takeback. */
    std::string rated = startGame(users, Json{{"allow_takeback", true}});
    EXPECT_ERROR(403, "FORBIDDEN_POLICY", act(rated, a, "request_undo"));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + rated + "/resign",
                                   b.token, Json::object()));

    std::string room = startGame(
        users, Json{{"rated", false}, {"allow_takeback", true}});
    CHECK_EQUAL(2, playPlies(room, users, GAME_MODE_2, 2));
    EXPECT_STATUS(200, act(room, a, "request_undo"));
    EXPECT_STATUS(200, act(room, b, "accept_undo"));
    CHECK_EQUAL(1, view(room).value("ply_count", 0));
    CHECK_EQUAL(1, static_cast<int>(ofType(events(room, ""), "UNDO_APPLIED").size()));
}

/* ================================================================== */
/* Boundary                                                            */
/* ================================================================== */

TEST(OctiHttpServerIntegrationTest, ArchivePaginationBoundaries)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    std::set<std::string> ids;
    for (int i = 0; i < 3; ++i) {
        ids.insert(playDrawGame(a, b));
    }
    CHECK_EQUAL(3, static_cast<int>(ids.size()));

    Reply p1 = http().get("/api/v1/archive/games?limit=2&offset=0");
    EXPECT_STATUS(200, p1);
    CHECK_EQUAL(3, p1.json.value("total", 0));
    CHECK_EQUAL(2, p1.json.value("limit", 0));
    CHECK_EQUAL(2, static_cast<int>(p1.json["games"].size()));
    Reply p2 = http().get("/api/v1/archive/games?limit=2&offset=2");
    CHECK_EQUAL(1, static_cast<int>(p2.json["games"].size()));
    std::set<std::string> seen;
    for (size_t i = 0; i < p1.json["games"].size(); ++i) {
        seen.insert(p1.json["games"][i].value("game_id", ""));
    }
    seen.insert(p2.json["games"][0].value("game_id", ""));
    CHECK_TRUE(seen == ids);                    /* pages tile with no overlap */

    Reply beyond = http().get("/api/v1/archive/games?limit=2&offset=3");
    EXPECT_STATUS(200, beyond);
    CHECK_EQUAL(0, static_cast<int>(beyond.json["games"].size()));
    CHECK_EQUAL(3, beyond.json.value("total", 0));
    Reply zero = http().get("/api/v1/archive/games?limit=0");
    EXPECT_STATUS(200, zero);
    CHECK_EQUAL(1, static_cast<int>(zero.json["games"].size()));
    CHECK_EQUAL(1, zero.json.value("limit", 0));
    Reply huge = http().get("/api/v1/archive/games?limit=100000");
    EXPECT_STATUS(200, huge);
    CHECK_EQUAL(100, huge.json.value("limit", 0));
    CHECK_EQUAL(3, static_cast<int>(huge.json["games"].size()));

    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().get("/api/v1/archive/games?limit=abc"));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().get("/api/v1/archive/games?limit=-1"));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().get("/api/v1/archive/games?offset=x"));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().get("/api/v1/archive/games?mode=3P"));
    CHECK_EQUAL(0, http().get("/api/v1/archive/games?mode=4P").json.value("total", -1));
    CHECK_EQUAL(3, http().get("/api/v1/archive/games?mode=2p").json.value("total", -1));
    CHECK_EQUAL(3, http().get("/api/v1/archive/games?player=" + a.uuid)
                       .json.value("total", -1));
    CHECK_EQUAL(0, http().get("/api/v1/archive/games?player=no-such-player")
                       .json.value("total", -1));
    EXPECT_ERROR(404, "NOT_FOUND", http().get("/api/v1/archive/games/g-00000000-0000"));
    EXPECT_ERROR(404, "NOT_FOUND",
                 http().get("/api/v1/archive/games/g-00000000-0000/octi"));
}

TEST(OctiHttpServerIntegrationTest, ChatLimitsAtTheByteBoundary)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    std::vector<User> users = {a, b};
    std::string room = startGame(users);

    const std::string max = repeat('m', GameRoomManager::kMaxChatBytes);
    const std::string over = repeat('m', GameRoomManager::kMaxChatBytes + 1);
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/chat", a.token,
                                   Json{{"text", max}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post("/api/v1/games/" + room + "/chat", a.token,
                             Json{{"text", over}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post("/api/v1/games/" + room + "/chat", a.token,
                             Json{{"text", "   "}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post("/api/v1/games/" + room + "/chat", a.token,
                             Json{{"text", 7}}));
    EXPECT_STATUS(200, http().post("/api/v1/lobby/chat", a.token, Json{{"text", max}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post("/api/v1/lobby/chat", a.token, Json{{"text", over}}));
}

TEST(OctiHttpServerIntegrationTest, LobbyChatRateLimitReturns429)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    for (int i = 0; i < 5; ++i) {
        EXPECT_STATUS(200, http().post("/api/v1/lobby/chat", a.token,
                                       Json{{"text", "msg " + std::to_string(i)}}));
    }
    Reply limited = http().post("/api/v1/lobby/chat", a.token, Json{{"text", "sixth"}});
    EXPECT_ERROR(429, "RATE_LIMITED", limited);
    CHECK_FALSE(limited.header("Retry-After").empty());
}

TEST(OctiHttpServerIntegrationTest, VoiceSignalPayloadLimitIsFourKiB)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    std::vector<User> users = {a, b};
    std::string room = startGame(users);
    const std::string path = "/api/v1/games/" + room + "/voice-signal";

    /* {"sdp":"..."} is 10 bytes of framing around the filler. */
    Json atLimit = Json{{"sdp", repeat('x', GameRoomManager::kMaxVoicePayloadBytes - 10)}};
    CHECK_EQUAL(4096, static_cast<int>(atLimit.dump().size()));
    Json overLimit = Json{{"sdp", repeat('x', GameRoomManager::kMaxVoicePayloadBytes - 9)}};
    CHECK_EQUAL(4097, static_cast<int>(overLimit.dump().size()));

    EXPECT_STATUS(200, http().post(path, a.token,
                                   Json{{"target", b.uuid}, {"type", "SDP_OFFER"},
                                        {"payload", atLimit}}));
    EXPECT_ERROR(413, "PAYLOAD_TOO_LARGE",
                 http().post(path, a.token,
                             Json{{"target", b.uuid}, {"type", "SDP_OFFER"},
                                  {"payload", overLimit}}));
    /* Only the accepted signal was relayed. */
    std::vector<Json> got = ofType(events(room, b.token), "VOICE_SIGNAL");
    CHECK_EQUAL(1, static_cast<int>(got.size()));
    CHECK_EQUAL(4096, static_cast<int>(got[0]["payload"].dump().size()));
}

TEST(OctiHttpServerIntegrationTest, RequestBodyLimitIsEnforcedExactly)
{
    HttpServerConfig cfg = testConfig();
    cfg.maxBodyBytes = 8192;
    startEnv(cfg, true, nullptr);
    User a = signIn("google", "sub-a", "Alice Anders");

    /* Exactly at the limit the request reaches the handler (and fails there
     * on the 500-byte chat limit); one byte more is refused outright. */
    std::string atLimit = "{\"text\":\"";
    atLimit += repeat('z', 8192 - atLimit.size() - 2);
    atLimit += "\"}";
    CHECK_EQUAL(8192, static_cast<int>(atLimit.size()));
    Reply ok = http().raw("POST", "/api/v1/lobby/chat", a.token, atLimit,
                          "application/json");
    EXPECT_ERROR(400, "INVALID_ARGUMENT", ok);

    std::string over = atLimit;
    over.insert(over.size() - 2, "z");
    CHECK_EQUAL(8193, static_cast<int>(over.size()));
    Reply big = http().raw("POST", "/api/v1/lobby/chat", a.token, over,
                           "application/json");
    EXPECT_ERROR(413, "PAYLOAD_TOO_LARGE", big);

    /* A much larger body is refused too and the server stays healthy. */
    Reply huge = http().raw("POST", "/api/v1/auth/sso", "",
                            std::string(200000, 'q'), "application/json");
    EXPECT_STATUS(413, huge);
    EXPECT_STATUS(200, http().get("/api/v1/health"));
}

TEST(OctiHttpServerIntegrationTest, AudioBodiesAreNeverAcceptedOrRelayed)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    std::vector<User> users = {a, b};
    std::string room = startGame(users);
    const std::string path = "/api/v1/games/" + room + "/voice-signal";
    std::string pcm(2000, '\x7f');
    pcm[3] = '\0';

    const char *types[] = {"audio/ogg", "audio/webm", "audio/L16",
                           "application/octet-stream", "multipart/form-data; boundary=x",
                           "text/plain"};
    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); ++i) {
        Reply r = http().raw("POST", path + "?target=" + b.uuid, a.token, pcm,
                             types[i]);
        EXPECT_ERROR(415, "UNSUPPORTED_MEDIA_TYPE", r);
    }
    /* Even a JSON-looking body with the wrong type is refused. */
    Reply wrongType = http().raw(
        "POST", path, a.token,
        Json{{"target", b.uuid}, {"type", "SDP_OFFER"}, {"payload", {{"a", 1}}}}.dump(),
        "text/plain");
    EXPECT_ERROR(415, "UNSUPPORTED_MEDIA_TYPE", wrongType);
    /* A JSON envelope that is not valid JSON never becomes a relayed signal. */
    Reply garbage = http().raw("POST", path, a.token, pcm, "application/json");
    EXPECT_ERROR(400, "MALFORMED_JSON", garbage);
    CHECK_EQUAL(0, static_cast<int>(ofType(events(room, b.token), "VOICE_SIGNAL").size()));
    /* The charset parameter is fine. */
    EXPECT_STATUS(200, http().raw("POST", path, a.token,
                                  Json{{"target", b.uuid}, {"type", "ICE_CANDIDATE"},
                                       {"payload", {{"candidate", "candidate:1 1 UDP 1 192.0.2.1 9 typ host"}}}}
                                      .dump(),
                                  "application/json; charset=utf-8"));
    CHECK_EQUAL(1, static_cast<int>(ofType(events(room, b.token), "VOICE_SIGNAL").size()));
}

TEST(OctiHttpServerIntegrationTest, IncrementalEventPollingAndPagingBoundaries)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    std::vector<User> users = {a, b};
    std::string room = startGame(users);
    for (int i = 0; i < 3; ++i) {
        EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/chat", a.token,
                                       Json{{"text", "m" + std::to_string(i)}}));
    }
    Json all = events(room, "");
    int64_t latest = all.value("latest_seq", 0);
    CHECK_EQUAL(latest, all.value("next_seq", 0));
    CHECK_FALSE(all.value("has_more", true));
    CHECK_FALSE(all.value("gap", true));

    /* Nothing new after the cursor. */
    Json none = events(room, "", all.value("next_seq", 0));
    CHECK_EQUAL(0, static_cast<int>(none["events"].size()));
    CHECK_EQUAL(latest, none.value("next_seq", 0));

    /* Paging: limit=2 returns two events, more are flagged, the cursor is the
     * last delivered sequence, and following it reaches the rest. */
    Reply page = http().get("/api/v1/games/" + room + "/events?since_seq=0&limit=2");
    EXPECT_STATUS(200, page);
    CHECK_EQUAL(2, static_cast<int>(page.json["events"].size()));
    CHECK_TRUE(page.json.value("has_more", false));
    CHECK_EQUAL(page.json["events"][1].value("seq", 0), page.json.value("next_seq", -1));
    std::set<int64_t> seqs;
    int64_t cursor = 0;
    for (int guard = 0; guard < 50; ++guard) {
        Reply r = http().get("/api/v1/games/" + room +
                             "/events?limit=2&since_seq=" + std::to_string(cursor));
        EXPECT_STATUS(200, r);
        for (size_t i = 0; i < r.json["events"].size(); ++i) {
            seqs.insert(r.json["events"][i].value("seq", 0));
        }
        cursor = r.json.value("next_seq", 0);
        if (!r.json.value("has_more", false)) {
            break;
        }
    }
    CHECK_EQUAL(static_cast<int>(all["events"].size()), static_cast<int>(seqs.size()));

    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/chat", b.token,
                                   Json{{"text", "new one"}}));
    Json inc = events(room, "", cursor);
    CHECK_EQUAL(1, static_cast<int>(inc["events"].size()));
    STRCMP_EQUAL("new one", inc["events"][0].value("text", "").c_str());

    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/games/" + room + "/events?since_seq=-5"));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/games/" + room + "/events?limit=0"));
    EXPECT_ERROR(404, "NOT_FOUND", http().get("/api/v1/games/g-20000101-9999/events"));
}

/* ================================================================== */
/* Fault injection                                                     */
/* ================================================================== */

TEST(OctiHttpServerIntegrationTest, MissingOrInvalidBearerIsUnauthorized)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    const char *protectedGets[] = {"/api/v1/profile/me", "/api/v1/lobby/state",
                                   "/api/v1/lobby/events",
                                   "/api/v1/matchmaking/status"};
    for (size_t i = 0; i < sizeof(protectedGets) / sizeof(protectedGets[0]); ++i) {
        Reply none = http().get(protectedGets[i]);
        EXPECT_ERROR(401, "UNAUTHORIZED", none);
        STRCMP_EQUAL("Bearer", none.header("WWW-Authenticate").c_str());
        EXPECT_STATUS(401, http().get(protectedGets[i], "not-a-real-token"));
    }
    httplib::Headers basic;
    basic.insert(std::make_pair("Authorization", "Basic YTpi"));
    EXPECT_STATUS(401, http().get("/api/v1/profile/me", "", basic));
    httplib::Headers empty;
    empty.insert(std::make_pair("Authorization", "Bearer"));
    EXPECT_STATUS(401, http().get("/api/v1/profile/me", "", empty));
    httplib::Headers spaced;
    spaced.insert(std::make_pair("Authorization", "Bearer two tokens"));
    EXPECT_STATUS(401, http().get("/api/v1/profile/me", "", spaced));

    const char *protectedPosts[] = {"/api/v1/auth/link", "/api/v1/auth/logout",
                                    "/api/v1/lobby/chat", "/api/v1/tables",
                                    "/api/v1/matchmaking/enqueue",
                                    "/api/v1/matchmaking/dequeue",
                                    "/api/v1/tables/g-1/join",
                                    "/api/v1/games/g-1/move",
                                    "/api/v1/games/g-1/resign",
                                    "/api/v1/games/g-1/action",
                                    "/api/v1/games/g-1/chat",
                                    "/api/v1/games/g-1/voice-signal",
                                    "/api/v1/games/g-1/voice-meta",
                                    "/api/v1/games/g-1/ban",
                                    "/api/v1/games/g-1/mute",
                                    "/api/v1/games/g-1/settings"};
    for (size_t i = 0; i < sizeof(protectedPosts) / sizeof(protectedPosts[0]); ++i) {
        EXPECT_ERROR(401, "UNAUTHORIZED",
                     http().post(protectedPosts[i], "", Json::object()));
        EXPECT_STATUS(401, http().post(protectedPosts[i], "bogus", Json::object()));
    }
    EXPECT_STATUS(401, http().patch("/api/v1/profile/me", "", Json{{"handle", "abcd"}}));
    EXPECT_STATUS(401, http().del("/api/v1/auth/link/google", ""));

    /* A valid token works until it is revoked by logout. */
    EXPECT_STATUS(200, http().get("/api/v1/profile/me", a.token));
    EXPECT_STATUS(200, http().post("/api/v1/auth/logout", a.token, Json::object()));
    EXPECT_ERROR(401, "UNAUTHORIZED", http().get("/api/v1/profile/me", a.token));
    EXPECT_STATUS(401, http().post("/api/v1/auth/logout", a.token, Json::object()));
    /* Signing in again mints a fresh, working session. */
    User again = signIn("google", "sub-a", "Alice Anders");
    CHECK_TRUE(again.token != a.token);
    EXPECT_STATUS(200, http().get("/api/v1/profile/me", again.token));
}

TEST(OctiHttpServerIntegrationTest, BadSsoCredentialsAreRejected)
{
    HttpServerConfig cfg = testConfig();
    startEnv(cfg, true, nullptr);

    /* Tampered signature. */
    std::string good = makeToken("google", "sub-a", "Alice");
    std::string tampered = good;
    tampered[tampered.size() - 3] = tampered[tampered.size() - 3] == 'A' ? 'B' : 'A';
    Reply t = http().post("/api/v1/auth/sso", "",
                          Json{{"provider", "google"}, {"id_token", tampered}});
    EXPECT_ERROR(401, "BAD_SIGNATURE", t);
    CHECK_TRUE(t.json.value("session_token", "").empty());

    /* Wrong audience, wrong issuer, expired. */
    EXPECT_ERROR(401, "BAD_AUDIENCE",
                 http().post("/api/v1/auth/sso", "",
                             Json{{"provider", "google"},
                                  {"id_token", makeToken("google", "s", "A",
                                                         [](Json &, Json &c) {
                                                             c["aud"] = "someone-else";
                                                         })}}));
    EXPECT_ERROR(401, "BAD_ISSUER",
                 http().post("/api/v1/auth/sso", "",
                             Json{{"provider", "google"},
                                  {"id_token", makeToken("google", "s", "A",
                                                         [](Json &, Json &c) {
                                                             c["iss"] = "https://evil.example";
                                                         })}}));
    EXPECT_ERROR(401, "EXPIRED",
                 http().post("/api/v1/auth/sso", "",
                             Json{{"provider", "google"},
                                  {"id_token", makeToken("google", "s", "A",
                                                         [](Json &, Json &c) {
                                                             c["exp"] = realNow() - 7200;
                                                         })}}));
    /* A token for another provider's issuer never passes. */
    EXPECT_ERROR(401, "BAD_ISSUER",
                 http().post("/api/v1/auth/sso", "",
                             Json{{"provider", "apple"},
                                  {"id_token", makeToken("google", "s", "A")}}));
    /* alg=none forgery. */
    Json hdr = Json{{"alg", "none"}, {"typ", "JWT"}};
    Json claims = Json{{"iss", issuerFor("google")}, {"aud", "client-google"},
                       {"sub", "forged"}, {"exp", realNow() + 3600}};
    std::string forged = base64UrlEncode(hdr.dump()) + "." +
                         base64UrlEncode(claims.dump()) + ".";
    EXPECT_STATUS(400, http().post("/api/v1/auth/sso", "",
                                   Json{{"provider", "google"}, {"id_token", forged}}));
    forged += "AAAA";
    EXPECT_ERROR(401, "UNSUPPORTED_ALG",
                 http().post("/api/v1/auth/sso", "",
                             Json{{"provider", "google"}, {"id_token", forged}}));
    /* Malformed or missing pieces. */
    EXPECT_ERROR(400, "MALFORMED",
                 http().post("/api/v1/auth/sso", "",
                             Json{{"provider", "google"}, {"id_token", "a.b"}}));
    EXPECT_ERROR(400, "UNSUPPORTED_PROVIDER",
                 http().post("/api/v1/auth/sso", "",
                             Json{{"provider", "twitter"}, {"id_token", good}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post("/api/v1/auth/sso", "", Json{{"provider", "google"}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post("/api/v1/auth/sso", "", Json{{"id_token", good}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post("/api/v1/auth/sso", "",
                             Json{{"provider", 5}, {"id_token", good}}));
    /* None of the above created an account. */
    CHECK_EQUAL(0, http().get("/api/v1/health").json.value("online_players", -1));
    CHECK_EQUAL(0, static_cast<int>(http().get("/api/v1/leaderboard")
                                        .json["entries"].size()));
}

TEST(OctiHttpServerIntegrationTest, LinkConflictReturns409WithExistingProfile)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");

    /* B tries to claim A's Google identity. */
    Reply r = http().post("/api/v1/auth/link", b.token,
                          Json{{"provider", "google"},
                               {"id_token", makeToken("google", "sub-a", "Alice Anders")}});
    EXPECT_ERROR(409, "CONFLICT_ALREADY_LINKED", r);
    STRCMP_EQUAL(a.uuid.c_str(), r.json["existing_profile"].value("uuid", "").c_str());
    STRCMP_EQUAL(a.handle.c_str(), r.json["existing_profile"].value("handle", "").c_str());
    CHECK_EQUAL(1200, r.json["existing_profile"].value("elo_2p", 0));
    CHECK_EQUAL(0, r.json["existing_profile"].value("games_played", -1));
    /* The conflicting profile is shown but nothing was merged or moved. */
    CHECK_TRUE(r.body.find("sub-a") == std::string::npos);
    Reply bMe = http().get("/api/v1/profile/me", b.token);
    CHECK_EQUAL(1, static_cast<int>(bMe.json["linked_identities"].size()));
    STRCMP_EQUAL("apple", bMe.json["linked_identities"][0].value("provider", "").c_str());
    Reply aMe = http().get("/api/v1/profile/me", a.token);
    CHECK_EQUAL(1, static_cast<int>(aMe.json["linked_identities"].size()));
    STRCMP_EQUAL(a.uuid.c_str(), aMe.json.value("uuid", "").c_str());
}

TEST(OctiHttpServerIntegrationTest, UnlinkRules)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    EXPECT_STATUS(200, http().post("/api/v1/auth/link", a.token,
                                   Json{{"provider", "line"},
                                        {"id_token", makeToken("line", "l-a", "Alice")}}));
    /* The last remaining identity can never be removed. */
    Reply first = http().del("/api/v1/auth/link/line", a.token);
    EXPECT_STATUS(200, first);
    CHECK_EQUAL(1, static_cast<int>(first.json["linked_identities"].size()));
    Reply last = http().del("/api/v1/auth/link/google", a.token);
    EXPECT_ERROR(400, "LAST_IDENTITY_FORBIDDEN", last);
    CHECK_EQUAL(1, static_cast<int>(http().get("/api/v1/profile/me", a.token)
                                        .json["linked_identities"].size()));
    EXPECT_ERROR(404, "NOT_LINKED", http().del("/api/v1/auth/link/apple", a.token));
    EXPECT_ERROR(400, "UNSUPPORTED_PROVIDER", http().del("/api/v1/auth/link/twitter", a.token));
    /* Removed identity can be linked again. */
    EXPECT_STATUS(200, http().post("/api/v1/auth/link", a.token,
                                   Json{{"provider", "line"},
                                        {"id_token", makeToken("line", "l-a", "Alice")}}));
}

TEST(OctiHttpServerIntegrationTest, MalformedJsonBodiesAreBadRequests)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    const char *posts[] = {"/api/v1/auth/sso", "/api/v1/auth/link",
                           "/api/v1/lobby/chat", "/api/v1/games/g-1/move",
                           "/api/v1/games/g-1/action", "/api/v1/games/g-1/ban"};
    const char *bodies[] = {"{not json", "[1,2,3]", "\"text\"", "{\"text\":", "42"};
    for (size_t i = 0; i < sizeof(posts) / sizeof(posts[0]); ++i) {
        for (size_t k = 0; k < sizeof(bodies) / sizeof(bodies[0]); ++k) {
            Reply r = http().raw("POST", posts[i], a.token, bodies[k],
                                 "application/json");
            if (std::string(posts[i]) == "/api/v1/auth/sso") {
                EXPECT_ERROR(400, "MALFORMED_JSON", r);
            } else {
                EXPECT_ERROR(400, "MALFORMED_JSON", r);
            }
        }
        /* An empty body where a JSON document is required. */
        Reply empty = http().raw("POST", posts[i], a.token, "", "application/json");
        EXPECT_ERROR(400, "MALFORMED_JSON", empty);
    }
    EXPECT_ERROR(400, "MALFORMED_JSON",
                 http().raw("PATCH", "/api/v1/profile/me", a.token, "{oops",
                            "application/json"));
}

TEST(OctiHttpServerIntegrationTest, MoveFaultsLeaveTheBoardUntouched)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    std::vector<User> users = {a, b};
    std::string room = startGame(users);
    const std::string path = "/api/v1/games/" + room + "/move";
    const std::string fenBefore = view(room).value("fen", "");

    EXPECT_ERROR(403, "NOT_YOUR_TURN", http().post(path, b.token, Json{{"move", "37+e"}}));
    EXPECT_ERROR(400, "ILLEGAL_SYNTAX", http().post(path, a.token, Json{{"move", "zz"}}));
    EXPECT_ERROR(400, "ILLEGAL_SYNTAX", http().post(path, a.token, Json{{"move", ""}}));
    EXPECT_ERROR(400, "ILLEGAL_SYNTAX", http().post(path, a.token, Json{{"move", "33\n"}}));
    /* Well-formed but illegal: there is no pod on 11. */
    Reply illegal = http().post(path, a.token, Json{{"move", "11+a"}});
    EXPECT_ERROR(400, "ILLEGAL_MOVE", illegal);
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post(path, a.token, Json{{"move", repeat('1', 513)}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().post(path, a.token, Json{{"move", 33}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().post(path, a.token, Json::object()));
    /* A player who is not seated cannot move at all. */
    User c = signIn("facebook", "sub-c", "Carol Clark");
    EXPECT_ERROR(403, "FORBIDDEN_NOT_SEATED",
                 http().post(path, c.token, Json{{"move", "33+b"}}));
    EXPECT_ERROR(404, "NOT_FOUND",
                 http().post("/api/v1/games/g-20000101-9999/move", a.token,
                             Json{{"move", "33+b"}}));

    Json v = view(room);
    CHECK_EQUAL(0, v.value("ply_count", -1));
    CHECK_EQUAL(1, v.value("turn_seat", 0));
    STRCMP_EQUAL(fenBefore.c_str(), v.value("fen", "").c_str());
    CHECK_EQUAL(0, static_cast<int>(ofType(events(room, ""), "MOVE").size()));
    /* The turn is still A's and a legal move goes through. */
    CHECK_EQUAL(1, playPlies(room, users, GAME_MODE_2, 1));
}

TEST(OctiHttpServerIntegrationTest, ModerationRulesAndMutes)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    User c = signIn("facebook", "sub-c", "Carol Clark");
    User d = signIn("microsoft", "sub-d", "Dave Dunn");
    std::vector<User> users = {a, b};
    std::string room = startGame(users, Json{{"voice_policy", "ALL_ALLOWED"}});
    const std::string base = "/api/v1/games/" + room;
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + room + "/observe", c.token, Json::object()));

    /* An observer cannot ban; a player cannot ban or mute the opponent. */
    EXPECT_ERROR(403, "FORBIDDEN_NOT_SEATED",
                 http().post(base + "/ban", c.token, Json{{"target", d.uuid}}));
    EXPECT_ERROR(403, "FORBIDDEN_SEATED_TARGET",
                 http().post(base + "/ban", a.token, Json{{"target", b.uuid}}));
    EXPECT_ERROR(403, "FORBIDDEN_SEATED_TARGET",
                 http().post(base + "/mute", a.token,
                             Json{{"target", b.uuid}, {"mute_chat", true}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post(base + "/ban", a.token, Json{{"target", a.uuid}}));
    EXPECT_ERROR(404, "UNKNOWN_TARGET",
                 http().post(base + "/ban", a.token, Json{{"target", "no-such-uuid"}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().post(base + "/ban", a.token, Json::object()));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post(base + "/mute", a.token, Json{{"target", c.uuid}}));
    /* Banning a non-participant is allowed (pre-emptive ban). */
    EXPECT_STATUS(200, http().post(base + "/ban", a.token, Json{{"target", d.uuid}}));
    EXPECT_ERROR(403, "FORBIDDEN_BANNED",
                 http().post("/api/v1/tables/" + room + "/observe", d.token, Json::object()));

    /* Chat mute: C is silenced, then restored. */
    EXPECT_STATUS(200, http().post(base + "/chat", c.token, Json{{"text", "before"}}));
    EXPECT_STATUS(200, http().post(base + "/mute", a.token,
                                   Json{{"target", c.uuid}, {"mute_chat", true}}));
    EXPECT_ERROR(403, "FORBIDDEN_MUTED",
                 http().post(base + "/chat", c.token, Json{{"text", "muted now"}}));
    std::vector<Json> muteEvents = ofType(events(room, ""), "USER_MUTED");
    CHECK_EQUAL(1, static_cast<int>(muteEvents.size()));
    CHECK_TRUE(muteEvents[0].value("mute_chat", false));
    CHECK_FALSE(muteEvents[0].value("mute_voice", true));
    EXPECT_STATUS(200, http().post(base + "/mute", b.token,
                                   Json{{"target", c.uuid}, {"mute_chat", false}}));
    EXPECT_STATUS(200, http().post(base + "/chat", c.token, Json{{"text", "unmuted"}}));

    /* Voice mute: an observer allowed by ALL_ALLOWED loses voice. */
    EXPECT_STATUS(200, http().post(base + "/voice-meta", c.token,
                                   Json{{"action", "JOINED_VOICE"}}));
    EXPECT_STATUS(200, http().post(base + "/mute", a.token,
                                   Json{{"target", c.uuid}, {"mute_voice", true}}));
    EXPECT_ERROR(403, "FORBIDDEN_MUTED",
                 http().post(base + "/voice-meta", c.token,
                             Json{{"action", "MIC_UNMUTED"}}));
    EXPECT_ERROR(403, "FORBIDDEN_MUTED",
                 http().post(base + "/voice-signal", c.token,
                             Json{{"target", a.uuid}, {"type", "SDP_OFFER"},
                                  {"payload", {{"sdp", "x"}}}}));
    for (size_t i = 0; i < view(room)["voice_peers"].size(); ++i) {
        CHECK_TRUE(view(room)["voice_peers"][i].value("player_uuid", "") != c.uuid);
    }

    /* Voice signaling target rules. */
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post(base + "/voice-signal", a.token,
                             Json{{"target", a.uuid}, {"type", "SDP_OFFER"},
                                  {"payload", {{"sdp", "x"}}}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post(base + "/voice-signal", a.token,
                             Json{{"target", d.uuid}, {"type", "SDP_OFFER"},
                                  {"payload", {{"sdp", "x"}}}}));
    EXPECT_ERROR(404, "UNKNOWN_TARGET",
                 http().post(base + "/voice-signal", a.token,
                             Json{{"target", "nobody"}, {"type", "SDP_OFFER"},
                                  {"payload", {{"sdp", "x"}}}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post(base + "/voice-signal", a.token,
                             Json{{"target", b.uuid}, {"type", "VIDEO_OFFER"},
                                  {"payload", {{"sdp", "x"}}}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post(base + "/voice-signal", a.token,
                             Json{{"target", b.uuid}, {"type", "SDP_OFFER"}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post(base + "/voice-meta", a.token,
                             Json{{"action", "SHOUT"}}));
    EXPECT_ERROR(409, "INVALID_STATE",
                 http().post(base + "/voice-meta", b.token,
                             Json{{"action", "MIC_MUTED"}}));
}

TEST(OctiHttpServerIntegrationTest, SettingsPoliciesGateObserversAndVoice)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    User c = signIn("facebook", "sub-c", "Carol Clark");
    std::vector<User> users = {a, b};
    std::string room = startGame(users);
    const std::string base = "/api/v1/games/" + room;
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + room + "/observe", c.token, Json::object()));
    EXPECT_STATUS(200, http().post(base + "/chat", c.token, Json{{"text", "hi"}}));

    /* Only seated players may change policies. */
    EXPECT_ERROR(403, "FORBIDDEN_NOT_SEATED",
                 http().post(base + "/settings", c.token,
                             Json{{"observer_policy", "OPEN"}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().post(base + "/settings", a.token, Json::object()));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post(base + "/settings", a.token,
                             Json{{"observer_policy", "WHATEVER"}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post(base + "/settings", a.token,
                             Json{{"voice_policy", "LOUD"}}));

    /* SILENT_OBSERVERS: the observer may watch but no longer chat. */
    Reply s = http().post(base + "/settings", a.token,
                          Json{{"observer_policy", "SILENT_OBSERVERS"}});
    EXPECT_STATUS(200, s);
    STRCMP_EQUAL("SILENT_OBSERVERS", s.json.value("observer_policy", "").c_str());
    STRCMP_EQUAL("PLAYERS_ONLY", s.json.value("voice_policy", "").c_str());
    EXPECT_ERROR(403, "FORBIDDEN_POLICY",
                 http().post(base + "/chat", c.token, Json{{"text", "psst"}}));
    EXPECT_STATUS(200, http().get(base, c.token));
    EXPECT_STATUS(200, http().post(base + "/chat", b.token, Json{{"text", "players still chat"}}));

    /* DISABLED voice: nobody, seated players included, may signal. */
    EXPECT_STATUS(200, http().post(base + "/settings", b.token,
                                   Json{{"voice_policy", "DISABLED"}}));
    EXPECT_ERROR(403, "FORBIDDEN_POLICY",
                 http().post(base + "/voice-meta", a.token,
                             Json{{"action", "JOINED_VOICE"}}));
    EXPECT_ERROR(403, "FORBIDDEN_POLICY",
                 http().post(base + "/voice-signal", a.token,
                             Json{{"target", b.uuid}, {"type", "SDP_OFFER"},
                                  {"payload", {{"sdp", "x"}}}}));
    std::vector<Json> pol = ofType(events(room, ""), "POLICY_UPDATED");
    CHECK_EQUAL(2, static_cast<int>(pol.size()));
    STRCMP_EQUAL("SILENT_OBSERVERS", pol[0].value("observer_policy", "").c_str());
    STRCMP_EQUAL("DISABLED", pol[1].value("voice_policy", "").c_str());
    STRCMP_EQUAL("SILENT_OBSERVERS", pol[1].value("observer_policy", "").c_str());

    /* NO_OBSERVERS evicts the observer and hides the in-progress game. */
    EXPECT_STATUS(200, http().post(base + "/settings", a.token,
                                   Json{{"observer_policy", "NO_OBSERVERS"}}));
    EXPECT_ERROR(403, "FORBIDDEN_POLICY", http().get(base, c.token));
    EXPECT_ERROR(403, "FORBIDDEN_POLICY", http().get(base));
    EXPECT_ERROR(403, "FORBIDDEN_POLICY", http().get(base + "/events"));
    EXPECT_STATUS(200, http().get(base, a.token));
    EXPECT_STATUS(200, http().get(base + "/events", b.token));
}

TEST(OctiHttpServerIntegrationTest, ConcurrentPartialSettingsNeverOverwriteEachOther)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    std::vector<User> users = {a, b};
    std::string room = startGame(users);
    const std::string path = "/api/v1/games/" + room + "/settings";
    const Http h = http();
    const int rounds = 40;
    std::atomic<int> badStatus(0);

    /*
     * Thread one only ever sends observer_policy, thread two only ever
     * sends voice_policy. A read-modify-write in the handler would let one
     * thread write back the stale value of the other thread's field, so the
     * final state would not be (last observer value, last voice value).
     */
    std::thread observerThread([&]() {
        for (int i = 0; i < rounds; ++i) {
            const char *v = i % 2 == 0 ? "SILENT_OBSERVERS" : "OPEN";
            Reply r = h.post(path, a.token, Json{{"observer_policy", v}});
            if (r.status != 200) {
                ++badStatus;
            }
        }
    });
    std::thread voiceThread([&]() {
        for (int i = 0; i < rounds; ++i) {
            const char *v = i % 2 == 0 ? "ALL_ALLOWED" : "DISABLED";
            Reply r = h.post(path, b.token, Json{{"voice_policy", v}});
            if (r.status != 200) {
                ++badStatus;
            }
        }
    });
    observerThread.join();
    voiceThread.join();
    CHECK_EQUAL(0, badStatus.load());

    Json v = view(room, a.token);
    STRCMP_EQUAL("OPEN", v.value("observer_policy", "").c_str());
    STRCMP_EQUAL("DISABLED", v.value("voice_policy", "").c_str());
}

TEST(OctiHttpServerIntegrationTest, SettingsResponseReportsTheMergedPolicies)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    std::vector<User> users = {a, b};
    std::string room = startGame(users, Json{{"voice_policy", "ALL_ALLOWED"}});
    const std::string path = "/api/v1/games/" + room + "/settings";
    Reply r = http().post(path, a.token, Json{{"observer_policy", "NO_OBSERVERS"}});
    EXPECT_STATUS(200, r);
    STRCMP_EQUAL("NO_OBSERVERS", r.json.value("observer_policy", "").c_str());
    STRCMP_EQUAL("ALL_ALLOWED", r.json.value("voice_policy", "").c_str());
}

TEST(OctiHttpServerIntegrationTest, LogoutKeepsPlayerOnlineWhileAnotherDeviceIsSignedIn)
{
    User phone = signIn("google", "sub-a", "Alice Anders");
    User laptop = signIn("google", "sub-a", "Alice Anders");
    CHECK_TRUE(phone.uuid == laptop.uuid);
    CHECK_TRUE(phone.token != laptop.token);
    Reply h0 = http().get("/api/v1/health");
    EXPECT_STATUS(200, h0);
    CHECK_EQUAL(1, h0.json.value("online_players", -1));

    EXPECT_STATUS(200, http().post("/api/v1/auth/logout", phone.token, Json::object()));
    EXPECT_ERROR(401, "UNAUTHORIZED", http().get("/api/v1/profile/me", phone.token));
    EXPECT_STATUS(200, http().get("/api/v1/profile/me", laptop.token));
    CHECK_EQUAL(1, http().get("/api/v1/health").json.value("online_players", -1));

    EXPECT_STATUS(200, http().post("/api/v1/auth/logout", laptop.token, Json::object()));
    CHECK_EQUAL(0, http().get("/api/v1/health").json.value("online_players", -1));
}

TEST(OctiHttpServerIntegrationTest, CredentialAttemptsAreRateLimitedPerAddress)
{
    HttpServerConfig cfg = testConfig();
    cfg.ssoRateLimitMax = 3;
    cfg.ssoRateLimitWindowMs = 60 * 1000;
    startEnv(cfg, true, nullptr);

    User a = signIn("google", "sub-a", "Alice Anders");
    /* A rejected credential spends an attempt as well. */
    EXPECT_STATUS(400, http().post("/api/v1/auth/sso", "",
                                   Json{{"provider", "google"}, {"id_token", "not-a-jwt"}}));
    EXPECT_STATUS(200, http().post("/api/v1/auth/link", a.token,
                                   Json{{"provider", "apple"},
                                        {"id_token", makeToken("apple", "sub-a2", "Alice Anders")}}));
    Reply limited = http().post("/api/v1/auth/sso", "",
                                Json{{"provider", "google"},
                                     {"id_token", makeToken("google", "sub-b", "Bob Brown")}});
    EXPECT_ERROR(429, "RATE_LIMITED", limited);
    CHECK_FALSE(limited.header("Retry-After").empty());
    CHECK_TRUE(std::atoi(limited.header("Retry-After").c_str()) >= 1);
    CHECK_TRUE(std::atoi(limited.header("Retry-After").c_str()) <= 60);
    EXPECT_ERROR(429, "RATE_LIMITED",
                 http().post("/api/v1/auth/link", a.token,
                             Json{{"provider", "line"},
                                  {"id_token", makeToken("line", "sub-a3", "Alice Anders")}}));

    /* Everything else is untouched by the credential throttle. */
    EXPECT_STATUS(200, http().get("/api/v1/profile/me", a.token));
    EXPECT_STATUS(200, http().get("/api/v1/health"));
}

TEST(OctiHttpServerIntegrationTest, CredentialRateLimitCanBeDisabledAndWindowReopens)
{
    HttpServerConfig cfg = testConfig();
    cfg.ssoRateLimitMax = 0;
    startEnv(cfg, true, nullptr);
    for (int i = 0; i < 40; ++i) {
        signIn("google", "sub-bulk", "Bulk User");
    }

    cfg = testConfig();
    cfg.ssoRateLimitMax = 2;
    cfg.ssoRateLimitWindowMs = 300;
    startEnv(cfg, true, nullptr);
    signIn("google", "sub-a", "Alice Anders");
    signIn("google", "sub-a", "Alice Anders");
    EXPECT_ERROR(429, "RATE_LIMITED",
                 http().post("/api/v1/auth/sso", "",
                             Json{{"provider", "google"},
                                  {"id_token", makeToken("google", "sub-a", "Alice Anders")}}));
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    signIn("google", "sub-a", "Alice Anders");
}

TEST(OctiHttpServerIntegrationTest, BanBindsIdentifiedViewersAndNoObserversClosesAnonymousReads)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    User c = signIn("facebook", "sub-c", "Carol Clark");
    std::vector<User> users = {a, b};
    std::string room = startGame(users);
    const std::string base = "/api/v1/games/" + room;
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + room + "/observe", c.token, Json::object()));
    EXPECT_STATUS(200, http().post(base + "/ban", a.token, Json{{"target", c.uuid}}));

    /* The ban follows the identity ... */
    EXPECT_ERROR(403, "FORBIDDEN_BANNED", http().get(base, c.token));
    EXPECT_ERROR(403, "FORBIDDEN_BANNED", http().get(base + "/events", c.token));
    /* ... while an open table stays publicly readable without identity. */
    EXPECT_STATUS(200, http().get(base));
    /* A table that wants no outside viewers says so, and that holds for
     * anonymous readers as well. */
    EXPECT_STATUS(200, http().post(base + "/settings", a.token,
                                   Json{{"observer_policy", "NO_OBSERVERS"}}));
    EXPECT_ERROR(403, "FORBIDDEN_POLICY", http().get(base));
    EXPECT_ERROR(403, "FORBIDDEN_POLICY", http().get(base + "/events"));
}

TEST(OctiHttpServerIntegrationTest, PasscodeAndSeatRules)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    User c = signIn("facebook", "sub-c", "Carol Clark");
    std::string room = createTable(a, Json{{"passcode", "sesame"}});
    Reply pub = http().get("/api/v1/tables");
    CHECK_TRUE(pub.json["tables"][0].value("has_passcode", false));
    CHECK_TRUE(pub.body.find("sesame") == std::string::npos);
    CHECK_TRUE(http().get("/api/v1/games/" + room).body.find("sesame") == std::string::npos);

    const std::string join = "/api/v1/tables/" + room + "/join";
    EXPECT_ERROR(403, "FORBIDDEN_PASSCODE", http().post(join, b.token, Json::object()));
    EXPECT_ERROR(403, "FORBIDDEN_PASSCODE", http().post(join, b.token, Json{{"passcode", "nope"}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().post(join, b.token, Json{{"passcode", "sesame"}, {"seat", 7}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().post(join, b.token, Json{{"passcode", "sesame"}, {"seat", -1}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().post(join, b.token, Json{{"passcode", "sesame"}, {"seat", "1"}}));
    EXPECT_ERROR(409, "SEAT_TAKEN", http().post(join, b.token, Json{{"passcode", "sesame"}, {"seat", 1}}));
    EXPECT_ERROR(409, "ALREADY_SEATED", http().post("/api/v1/tables", a.token, Json::object()));
    EXPECT_STATUS(200, http().post(join, b.token, Json{{"passcode", "sesame"}, {"seat", 2}}));
    EXPECT_ERROR(409, "INVALID_STATE", http().post(join, c.token, Json{{"passcode", "sesame"}}));
    EXPECT_ERROR(404, "NOT_FOUND", http().post("/api/v1/tables/g-20000101-9999/join", c.token, Json::object()));

    /* Table configuration is validated. */
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().post("/api/v1/tables", c.token, Json{{"mode", "9P"}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().post("/api/v1/tables", c.token, Json{{"bases", 2}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().post("/api/v1/tables", c.token, Json{{"rated", "yes"}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().post("/api/v1/tables", c.token, Json{{"voice_policy", "LOUD"}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().post("/api/v1/tables", c.token, Json{{"time_control", "fast"}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().post("/api/v1/tables", c.token, Json{{"time_control", {{"initial_ms", 0}}}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().post("/api/v1/tables", c.token, Json{{"time_control", 5}}));
    EXPECT_STATUS(201, http().post("/api/v1/tables", c.token, Json{{"time_control", {{"initial_ms", 60000}, {"increment_ms", 1000}}}}));
}

TEST(OctiHttpServerIntegrationTest, HostCanKickASeatBeforeTheGameStarts)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    User c = signIn("facebook", "sub-c", "Carol Clark");
    std::string room = createTable(a);
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + room + "/observe", c.token, Json::object()));
    EXPECT_ERROR(403, "FORBIDDEN_NOT_SEATED",
                 http().post("/api/v1/tables/" + room + "/kick", c.token, Json{{"target", a.uuid}}));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post("/api/v1/tables/" + room + "/kick", a.token, Json{{"target", c.uuid}}));
    /* B takes seat 2... which would start a 2P game, so use a 4P table. */
    std::string quad = createTable(c, Json{{"mode", "4P"}});
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + quad + "/join", b.token, Json::object()));
    EXPECT_ERROR(403, "FORBIDDEN_NOT_HOST",
                 http().post("/api/v1/tables/" + quad + "/kick", b.token, Json{{"target", c.uuid}}));
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + quad + "/kick", c.token, Json{{"target", b.uuid}}));
    Reply afterKick = http().get("/api/v1/games/" + quad);
    EXPECT_STATUS(200, afterKick);
    CHECK_TRUE(afterKick.json["seats"][1]["player_uuid"].is_null());
    CHECK_EQUAL(1, http().get("/api/v1/tables").json["tables"][1].value("seats_filled", 0));
    /* Leaving a waiting table. */
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + room + "/leave", a.token, Json::object()));
    EXPECT_ERROR(403, "FORBIDDEN_NOT_SEATED",
                 http().post("/api/v1/tables/" + quad + "/leave", a.token, Json::object()));
}

TEST(OctiHttpServerIntegrationTest, FinishedGameRemainsReadableFromTheArchiveAfterReaping)
{
    RoomLimits limits;
    limits.finishedRetentionMs = 100;
    startEnv(testConfig(), true, &limits);
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    std::string room = playDrawGame(a, b);
    /* Wait for the maintenance tick to reap the in-memory room. */
    bool reaped = false;
    for (int i = 0; i < 200 && !reaped; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        reaped = env->rooms->roomCount() == 0;
    }
    CHECK_TRUE(reaped);
    Reply v = http().get("/api/v1/games/" + room);
    EXPECT_STATUS(200, v);
    CHECK_FALSE(v.json.value("live", true));
    STRCMP_EQUAL("FINISHED", v.json.value("phase", "").c_str());
    STRCMP_EQUAL("DRAW", v.json.value("result", "").c_str());
    CHECK_FALSE(v.json.value("fen", "").empty());
    /* Live-only endpoints report the room as gone. */
    EXPECT_ERROR(404, "NOT_FOUND", http().get("/api/v1/games/" + room + "/events"));
}

/* ================================================================== */
/* Server-Sent Events                                                  */
/* ================================================================== */

TEST(OctiHttpServerIntegrationTest, SseStreamDeliversRoomEventsLive)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    User c = signIn("facebook", "sub-c", "Carol Clark");
    std::vector<User> users = {a, b};
    std::string room = startGame(users);
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + room + "/observe", c.token, Json::object()));

    SseClient sse;
    sse.start(env->server->port(), "/api/v1/games/" + room + "/events?stream=1", c.token);
    CHECK_TRUE(sse.waitFor("retry: 3000", 3000));
    CHECK_EQUAL(200, sse.status());
    CHECK_TRUE(sse.contentType().find("text/event-stream") == 0);
    /* The backlog (room created ... game started ... observer joined). */
    CHECK_TRUE(sse.waitFor("event: GAME_STARTED", 3000));

    /* Live: nothing has been sent for the move yet; make it happen now. */
    CHECK_TRUE(sse.data().find("event: MOVE") == std::string::npos);
    CHECK_EQUAL(1, playPlies(room, users, GAME_MODE_2, 1));
    CHECK_TRUE(sse.waitFor("event: MOVE", 3000));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/chat", b.token,
                                   Json{{"text", "stream me"}}));
    CHECK_TRUE(sse.waitFor("stream me", 3000));
    std::vector<Json> mv = sse.events("MOVE");
    CHECK_EQUAL(1, static_cast<int>(mv.size()));
    CHECK_EQUAL(1, mv[0].value("ply", 0));
    STRCMP_EQUAL(a.uuid.c_str(), mv[0].value("player_uuid", "").c_str());
    /* Frames carry resumable ids. */
    CHECK_TRUE(sse.data().find("id: " + std::to_string(mv[0].value("seq", 0)) + "\n") !=
               std::string::npos);
    /* A private voice signal to somebody else never reaches this stream. */
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/voice-signal", a.token,
                                   Json{{"target", b.uuid}, {"type", "SDP_OFFER"},
                                        {"payload", {{"sdp", "secret-offer"}}}}));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/chat", a.token,
                                   Json{{"text", "marker after signal"}}));
    CHECK_TRUE(sse.waitFor("marker after signal", 3000));
    CHECK_TRUE(sse.data().find("secret-offer") == std::string::npos);
    CHECK_EQUAL(1, env->server->activeStreams());

    /* Dropping the client frees its slot. */
    sse.stop();
    bool freed = false;
    for (int i = 0; i < 300 && !freed; ++i) {
        freed = env->server->activeStreams() == 0;
        if (!freed) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    CHECK_TRUE(freed);
}

TEST(OctiHttpServerIntegrationTest, SseResumesFromLastEventIdAndTargetsVoiceSignals)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    std::vector<User> users = {a, b};
    std::string room = startGame(users);
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/chat", a.token, Json{{"text", "old news"}}));
    int64_t latest = events(room, "").value("latest_seq", 0);

    httplib::Headers resume;
    resume.insert(std::make_pair("Last-Event-ID", std::to_string(latest)));
    SseClient sse;
    sse.start(env->server->port(), "/api/v1/games/" + room + "/events", b.token, resume);
    CHECK_TRUE(sse.waitFor("retry: 3000", 3000));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/voice-signal", a.token,
                                   Json{{"target", b.uuid}, {"type", "ICE_CANDIDATE"},
                                        {"payload", {{"candidate", "c1"}}}}));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/chat", a.token, Json{{"text", "fresh news"}}));
    CHECK_TRUE(sse.waitFor("fresh news", 3000));
    CHECK_TRUE(sse.data().find("old news") == std::string::npos);
    std::vector<Json> sig = sse.events("VOICE_SIGNAL");
    CHECK_EQUAL(1, static_cast<int>(sig.size()));
    STRCMP_EQUAL("ICE_CANDIDATE", sig[0].value("signal_type", "").c_str());
    STRCMP_EQUAL("c1", sig[0]["payload"].value("candidate", "").c_str());
    sse.stop();
}

TEST(OctiHttpServerIntegrationTest, SseRefusesBannedViewersAndUnknownRooms)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    User c = signIn("facebook", "sub-c", "Carol Clark");
    std::vector<User> users = {a, b};
    std::string room = startGame(users);
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + room + "/observe", c.token, Json::object()));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/ban", a.token, Json{{"target", c.uuid}}));

    Reply banned = http().get("/api/v1/games/" + room + "/events?stream=1", c.token);
    EXPECT_ERROR(403, "FORBIDDEN_BANNED", banned);
    Reply missing = http().get("/api/v1/games/g-20000101-9999/events?stream=1");
    EXPECT_ERROR(404, "NOT_FOUND", missing);
    Reply badAuth = http().get("/api/v1/games/" + room + "/events?stream=1", "bogus");
    EXPECT_ERROR(401, "UNAUTHORIZED", badAuth);
    CHECK_EQUAL(0, env->server->activeStreams());
}

TEST(OctiHttpServerIntegrationTest, SseBansAnActiveViewerMidStream)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    User c = signIn("facebook", "sub-c", "Carol Clark");
    std::vector<User> users = {a, b};
    std::string room = startGame(users);
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + room + "/observe", c.token, Json::object()));
    SseClient sse;
    sse.start(env->server->port(), "/api/v1/games/" + room + "/events?stream=1", c.token);
    CHECK_TRUE(sse.waitFor("event: OBSERVER_JOINED", 3000));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/ban", a.token, Json{{"target", c.uuid}}));
    CHECK_TRUE(sse.waitFor("event: closed", 3000));
    CHECK_TRUE(sse.waitFinished(3000));
    std::vector<Json> closed = sse.events("closed");
    CHECK_EQUAL(1, static_cast<int>(closed.size()));
    STRCMP_EQUAL("FORBIDDEN_BANNED", closed[0].value("error", "").c_str());
    sse.stop();
}

TEST(OctiHttpServerIntegrationTest, SseEndsWhenTheSessionIsRevoked)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    SseClient sse;
    sse.start(env->server->port(), "/api/v1/lobby/events?stream=1", a.token);
    CHECK_TRUE(sse.waitFor("retry: 3000", 3000));
    EXPECT_STATUS(200, http().post("/api/v1/auth/logout", a.token, Json::object()));
    CHECK_TRUE(sse.waitFor("event: closed", 3000));
    CHECK_TRUE(sse.waitFinished(3000));
    CHECK_TRUE(sse.data().find("UNAUTHORIZED") != std::string::npos);
    sse.stop();
    CHECK_EQUAL(0, env->server->activeStreams());
}

TEST(OctiHttpServerIntegrationTest, LobbySseDeliversChatAndPrivateMatchNotices)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    SseClient sse;
    sse.start(env->server->port(), "/api/v1/lobby/events", a.token);
    CHECK_TRUE(sse.waitFor("retry: 3000", 3000));
    EXPECT_STATUS(200, http().post("/api/v1/lobby/chat", b.token, Json{{"text", "anyone for a game?"}}));
    CHECK_TRUE(sse.waitFor("anyone for a game?", 3000));
    std::vector<Json> chat = sse.events("LOBBY_CHAT");
    CHECK_FALSE(chat.empty());
    STRCMP_EQUAL(b.handle.c_str(), chat.back().value("handle", "").c_str());

    /* A match found for A and B is announced privately to each of them. */
    EXPECT_STATUS(200, http().post("/api/v1/matchmaking/enqueue", a.token, Json{{"mode", "2P"}}));
    EXPECT_STATUS(200, http().post("/api/v1/matchmaking/enqueue", b.token, Json{{"mode", "2P"}}));
    CHECK_TRUE(sse.waitFor("event: MATCH_FOUND", 5000));
    sse.stop();
    User c = signIn("facebook", "sub-c", "Carol Clark");
    Reply cEvents = http().get("/api/v1/lobby/events?since_seq=0", c.token);
    for (size_t i = 0; i < cEvents.json["events"].size(); ++i) {
        CHECK_TRUE(cEvents.json["events"][i].value("type", "") != "MATCH_FOUND");
    }
}

TEST(OctiHttpServerIntegrationTest, StreamCapIsEnforcedAndSlotsAreReleased)
{
    HttpServerConfig cfg = testConfig();
    cfg.maxWorkerThreads = 8;
    cfg.workerThreads = 4;
    cfg.maxSseStreams = 2;
    startEnv(cfg, true, nullptr);
    User a = signIn("google", "sub-a", "Alice Anders");

    SseClient one;
    SseClient two;
    one.start(env->server->port(), "/api/v1/lobby/events?stream=1", a.token);
    two.start(env->server->port(), "/api/v1/lobby/events?stream=1", a.token);
    CHECK_TRUE(one.waitFor("retry: 3000", 3000));
    CHECK_TRUE(two.waitFor("retry: 3000", 3000));
    CHECK_EQUAL(2, env->server->activeStreams());
    Reply third = http().get("/api/v1/lobby/events?stream=1", a.token);
    EXPECT_ERROR(503, "TOO_MANY_STREAMS", third);
    CHECK_FALSE(third.header("Retry-After").empty());
    /* Plain polling is unaffected by the cap. */
    EXPECT_STATUS(200, http().get("/api/v1/lobby/events", a.token));
    CHECK_EQUAL(2, env->server->activeStreams());

    one.stop();
    two.stop();
    bool freed = false;
    for (int i = 0; i < 300 && !freed; ++i) {
        freed = env->server->activeStreams() == 0;
        if (!freed) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    CHECK_TRUE(freed);
    SseClient again;
    again.start(env->server->port(), "/api/v1/lobby/events?stream=1", a.token);
    CHECK_TRUE(again.waitFor("retry: 3000", 3000));
    again.stop();
}

TEST(OctiHttpServerIntegrationTest, SseStreamIsBoundedInTimeAndAsksToReconnect)
{
    HttpServerConfig cfg = testConfig();
    cfg.sseMaxDurationMs = 300;
    startEnv(cfg, true, nullptr);
    User a = signIn("google", "sub-a", "Alice Anders");
    SseClient sse;
    sse.start(env->server->port(), "/api/v1/lobby/events?stream=1", a.token);
    CHECK_TRUE(sse.waitFor("event: reconnect", 3000));
    CHECK_TRUE(sse.waitFinished(3000));
    sse.stop();
    /* Heartbeats keep an idle stream alive until then. */
    cfg.sseMaxDurationMs = 5000;
    startEnv(cfg, true, nullptr);
    User b = signIn("google", "sub-b", "Bob Brown");
    SseClient idle;
    idle.start(env->server->port(), "/api/v1/lobby/events?stream=1", b.token);
    CHECK_TRUE(idle.waitFor(": hb", 3000));
    idle.stop();
}

TEST(OctiHttpServerIntegrationTest, ProtocolV1GoldenContract)
{
    /* 1. Health endpoint exposes protocol_version and min_protocol_version */
    Reply h = http().get("/api/v1/health");
    EXPECT_STATUS(200, h);
    STRCMP_EQUAL("ok", h.json.value("status", "").c_str());
    STRCMP_EQUAL(OctiCore::versionString(), h.json.value("version", "").c_str());
    CHECK_EQUAL(1, h.json.value("protocol_version", 0));
    CHECK_EQUAL(1, h.json.value("min_protocol_version", 0));
    CHECK_TRUE(h.json.value("uptime_sec", -1) >= 0);
    CHECK_TRUE(h.json.value("active_tables", -1) >= 0);
    CHECK_TRUE(h.json.value("online_players", -1) >= 0);

    /* 2. Endpoints work seamlessly when X-Octi-Protocol-Version is omitted (default=1) */
    User a = signIn("google", "sub-golden-a", "Alice Golden");
    Reply prof1 = http().get("/api/v1/profile/me", a.token);
    EXPECT_STATUS(200, prof1);
    STRCMP_EQUAL(a.uuid.c_str(), prof1.json.value("uuid", "").c_str());

    Reply mm1 = http().get("/api/v1/matchmaking/status", a.token);
    EXPECT_STATUS(200, mm1);
    CHECK_FALSE(mm1.json.value("queued", true));

    Reply tbls1 = http().get("/api/v1/tables", a.token);
    EXPECT_STATUS(200, tbls1);
    CHECK_TRUE(tbls1.json["tables"].is_array());

    /* 3. Endpoints work seamlessly when X-Octi-Protocol-Version: 1 is explicitly attached */
    httplib::Headers proto1;
    proto1.insert(std::make_pair("X-Octi-Protocol-Version", "1"));

    Reply prof2 = http().get("/api/v1/profile/me", a.token, proto1);
    EXPECT_STATUS(200, prof2);
    STRCMP_EQUAL(a.uuid.c_str(), prof2.json.value("uuid", "").c_str());

    Reply tblCreate = http().post(
        "/api/v1/tables", a.token,
        Json{{"name", "Golden Table"}, {"mode", "2P"}}, proto1);
    EXPECT_STATUS(201, tblCreate);
    CHECK_TRUE(tblCreate.json.value("ok", false));
    std::string roomId = tblCreate.json.value("room_id", "");
    CHECK_FALSE(roomId.empty());

    Reply tblView = http().get("/api/v1/games/" + roomId, a.token, proto1);
    EXPECT_STATUS(200, tblView);
    STRCMP_EQUAL("WAITING_FOR_PLAYERS", tblView.json.value("phase", "").c_str());

    /* Player B signs in and joins with protocol header */
    Reply ssoB = http().post(
        "/api/v1/auth/sso", "",
        Json{{"provider", "apple"},
             {"id_token", makeToken("apple", "sub-golden-b", "Bob Golden")}},
        proto1);
    EXPECT_STATUS(201, ssoB);
    User b;
    b.token = ssoB.json.value("session_token", "");
    b.uuid = ssoB.json["profile"].value("uuid", "");

    Reply joinB = http().post("/api/v1/tables/" + roomId + "/join", b.token,
                              Json::object(), proto1);
    EXPECT_STATUS(200, joinB);
    CHECK_TRUE(joinB.json.value("game_started", false));

    /* Submit a legal move with protocol header */
    Reply mv = http().post("/api/v1/games/" + roomId + "/move", a.token,
                           Json{{"move", "33+b"}}, proto1);
    EXPECT_STATUS(200, mv);
    STRCMP_EQUAL("33+b", mv.json.value("move", "").c_str());

    /* Poll events with protocol header */
    Reply evs = http().get("/api/v1/games/" + roomId + "/events?since_seq=0",
                           a.token, proto1);
    EXPECT_STATUS(200, evs);
    CHECK_TRUE(evs.json["events"].is_array());
    CHECK_TRUE(evs.json["events"].size() >= 1);
}

TEST(OctiHttpServerIntegrationTest, ProtocolVersionGateRejectsOutdatedClientsWith426)
{
    HttpServerConfig cfg = testConfig();
    cfg.protocolVersion = 2;
    cfg.minProtocolVersion = 2;
    startEnv(cfg, true, nullptr);

    /* 1. Health endpoint is bypassed and reports configured versions */
    Reply h = http().get("/api/v1/health");
    EXPECT_STATUS(200, h);
    CHECK_EQUAL(2, h.json.value("protocol_version", 0));
    CHECK_EQUAL(2, h.json.value("min_protocol_version", 0));

    /* Health with outdated protocol header is still allowed */
    httplib::Headers v1Header;
    v1Header.insert(std::make_pair("X-Octi-Protocol-Version", "1"));
    Reply h1 = http().get("/api/v1/health", "", v1Header);
    EXPECT_STATUS(200, h1);

    /* 2. Requests without X-Octi-Protocol-Version default to 1, rejected with 426 */
    Reply rNoHeader = http().get("/api/v1/tables");
    EXPECT_STATUS(426, rNoHeader);
    STRCMP_EQUAL("UPGRADE_REQUIRED", rNoHeader.json.value("error", "").c_str());
    STRCMP_EQUAL("client protocol version is outdated; please upgrade OCTI",
                 rNoHeader.json.value("message", "").c_str());
    CHECK_EQUAL(2, rNoHeader.json.value("protocol_version", 0));
    CHECK_EQUAL(2, rNoHeader.json.value("min_protocol_version", 0));

    /* 3. Requests with X-Octi-Protocol-Version: 1 rejected with 426 */
    Reply rV1 = http().get("/api/v1/tables", "", v1Header);
    EXPECT_STATUS(426, rV1);
    STRCMP_EQUAL("UPGRADE_REQUIRED", rV1.json.value("error", "").c_str());
    CHECK_EQUAL(2, rV1.json.value("min_protocol_version", 0));

    Reply rAuthV1 = http().post(
        "/api/v1/auth/sso", "",
        Json{{"provider", "google"},
             {"id_token", makeToken("google", "sub-gate-a", "Alice Gate")}},
        v1Header);
    EXPECT_STATUS(426, rAuthV1);

    Reply rMmV1 = http().get("/api/v1/matchmaking/status", "", v1Header);
    EXPECT_STATUS(426, rMmV1);

    /* 4. Requests with X-Octi-Protocol-Version: 2 are accepted */
    httplib::Headers v2Header;
    v2Header.insert(std::make_pair("X-Octi-Protocol-Version", "2"));
    Reply rV2 = http().get("/api/v1/tables", "", v2Header);
    EXPECT_STATUS(200, rV2);

    Reply rAuthV2 = http().post(
        "/api/v1/auth/sso", "",
        Json{{"provider", "google"},
             {"id_token", makeToken("google", "sub-gate-a", "Alice Gate")}},
        v2Header);
    EXPECT_STATUS(201, rAuthV2);

    /* Higher protocol version (e.g. 3 >= min 2) is accepted */
    httplib::Headers v3Header;
    v3Header.insert(std::make_pair("X-Octi-Protocol-Version", "3"));
    Reply rV3 = http().get("/api/v1/tables", "", v3Header);
    EXPECT_STATUS(200, rV3);

    /* 5. Malformed X-Octi-Protocol-Version headers return 400 INVALID_PROTOCOL_VERSION */
    const char *badHeaders[] = {"abc", "0", "-1", "-100", "1.5", "99999999999999999999", ""};
    for (size_t i = 0; i < sizeof(badHeaders) / sizeof(badHeaders[0]); ++i) {
        httplib::Headers badH;
        badH.insert(std::make_pair("X-Octi-Protocol-Version", badHeaders[i]));
        Reply rBad = http().get("/api/v1/tables", "", badH);
        EXPECT_STATUS(400, rBad);
        STRCMP_EQUAL("INVALID_PROTOCOL_VERSION",
                     rBad.json.value("error", "").c_str());
    }
}

/* ------------------------------------------------------------------ */
/* Envelope 2: Archive upload, legality replay, privacy, export & DoS */
/* ------------------------------------------------------------------ */

namespace {

std::string makeWonTranscript(const std::string &installId = "11111111-1111-1111-1111-111111111111",
                             int aiDiff = 3,
                             const std::string &platform = "linux-x86_64")
{
    octi::OctiBoard board(octi::GAME_MODE_2, false, false, octi::GAME_BASES_1);
    octi::OctiGame *g = board.getGame();
    g->start(octi::PLAYER_A);
    const char *moves[] = {
        "73+a", "77+e", "73a-74", "77e-76", "74a-75", "57+h", "75a-77"
    };
    for (size_t i = 0; i < 7; ++i) {
        octi::OctiPlayer p = (i % 2 == 0) ? octi::PLAYER_A : octi::PLAYER_B;
        g->move(p, moves[i]);
    }
    octi::OctiGameMetadata meta;
    meta.status = "GAME_OVER";
    meta.termination = "BASE_OCCUPATION";
    meta.appVersion = "2.1.1";
    meta.platform = platform;
    meta.buildType = "release";
    meta.extended.present = true;
    meta.extended.installId = installId;
    meta.extended.locale = "en-US";
    meta.extended.osVersion = "Linux 6.6";
    meta.extended.startedAt = 1730000000;
    meta.extended.endedAt = 1730000100;
    meta.extended.utcOffsetMin = 480;
    meta.players.push_back({1, octi::PLAYER_A, "HUMAN", "Player", 0, 0});
    meta.players.push_back({2, octi::PLAYER_B, "AI", "AI_L" + std::to_string(aiDiff), aiDiff, 2000});

    std::vector<octi::OctiPlyMetadata> plyMeta(7);
    for (size_t i = 0; i < 7; ++i) {
        plyMeta[i].elapsedMs = 1200 + i * 100;
        plyMeta[i].clockAfterMs = -1;
        if (i % 2 == 1) {
            plyMeta[i].isAiMove = true;
            plyMeta[i].aiDifficulty = aiDiff;
            plyMeta[i].aiDepthReached = 4;
            plyMeta[i].aiNodesVisited = 5000;
            plyMeta[i].aiBestScore = -150;
            plyMeta[i].aiBudgetMs = 2000;
        }
    }
    g->setAllPlyMetadata(plyMeta);
    return board.serializeJson(meta);
}

std::string makeResignedTranscript(const std::vector<std::string> &moves,
                                  const std::string &installId,
                                  int aiDiff,
                                  const std::string &platform = "android-arm64")
{
    octi::OctiBoard board(octi::GAME_MODE_2, false, false, octi::GAME_BASES_3);
    octi::OctiGame *g = board.getGame();
    g->start(octi::PLAYER_A);
    for (size_t i = 0; i < moves.size(); ++i) {
        octi::OctiPlayer p = (i % 2 == 0) ? octi::PLAYER_A : octi::PLAYER_B;
        g->move(p, moves[i]);
    }
    g->resign(octi::PLAYER_A);

    octi::OctiGameMetadata meta;
    meta.status = "RESIGNED";
    meta.termination = "RESIGNATION";
    meta.appVersion = "2.1.1";
    meta.platform = platform;
    meta.buildType = "release";
    meta.extended.present = true;
    meta.extended.installId = installId;
    meta.extended.locale = "zh-TW";
    meta.extended.osVersion = "Android 14";
    meta.extended.startedAt = 1730000200;
    meta.extended.endedAt = 1730000300;
    meta.extended.utcOffsetMin = 480;
    meta.players.push_back({1, octi::PLAYER_A, "HUMAN", "Player", 0, 0});
    meta.players.push_back({2, octi::PLAYER_B, "AI", "AI_L" + std::to_string(aiDiff), aiDiff, 1000});

    std::vector<octi::OctiPlyMetadata> plyMeta(moves.size());
    for (size_t i = 0; i < moves.size(); ++i) {
        plyMeta[i].elapsedMs = 500 + i * 50;
        plyMeta[i].clockAfterMs = -1;
        if (i % 2 == 1) {
            plyMeta[i].isAiMove = true;
            plyMeta[i].aiDifficulty = aiDiff;
            plyMeta[i].aiDepthReached = 2;
            plyMeta[i].aiNodesVisited = 800;
            plyMeta[i].aiBestScore = 50;
            plyMeta[i].aiBudgetMs = 1000;
        }
    }
    g->setAllPlyMetadata(plyMeta);
    return board.serializeJson(meta);
}

} // namespace

TEST(OctiHttpServerIntegrationTest, ArchiveUploadNominalWonAndResigned)
{
    std::string wonTr = makeWonTranscript("inst-won-01", 3, "linux-x86_64");
    Reply rWon = http().post("/api/v1/archive/upload", "", Json{{"transcript", wonTr}});
    EXPECT_STATUS(200, rWon);
    CHECK_TRUE(rWon.json.value("ok", false));
    std::string wonId = rWon.json.value("game_id", "");
    CHECK_FALSE(wonId.empty());
    CHECK_EQUAL(7, rWon.json.value("ply_count", 0));

    /* Verify retrieval through /api/v1/archive/games/:id */
    Reply rGet = http().get("/api/v1/archive/games/" + wonId);
    EXPECT_STATUS(200, rGet);
    STRCMP_EQUAL(wonId.c_str(), rGet.json.value("game_id", "").c_str());
    STRCMP_EQUAL("2P", rGet.json.value("mode", "").c_str());
    STRCMP_EQUAL("client_upload", rGet.json.value("source", "").c_str());
    STRCMP_EQUAL("A", rGet.json.value("winner", "").c_str());
    STRCMP_EQUAL("GAME_OVER", rGet.json.value("status", "").c_str());
    CHECK_EQUAL(3, rGet.json.value("ai_difficulty", 0));
    STRCMP_EQUAL("2.1.1", rGet.json.value("app_version", "").c_str());
    STRCMP_EQUAL("linux-x86_64", rGet.json.value("platform", "").c_str());
    STRCMP_EQUAL("CC-BY-4.0", rGet.json.value("license", "").c_str());
    STRCMP_EQUAL("Octi game data, Charles Chiou (selfso.com)",
                 rGet.json.value("attribution", "").c_str());

    /* Privacy check: installId and client.extended never leak in public response */
    CHECK_TRUE(rGet.body.find("inst-won-01") == std::string::npos);
    CHECK_TRUE(rGet.body.find("\"extended\"") == std::string::npos);

    /* Verify /api/v1/archive/games/:id/transcript */
    Reply rTr = http().get("/api/v1/archive/games/" + wonId + "/transcript");
    EXPECT_STATUS(200, rTr);
    STRCMP_EQUAL("application/json; charset=utf-8", rTr.contentType.c_str());
    CHECK_TRUE(rTr.header("Content-Disposition").find(wonId + ".json") != std::string::npos);
    CHECK_TRUE(rTr.body.find("inst-won-01") == std::string::npos);
    CHECK_TRUE(rTr.body.find("\"extended\"") == std::string::npos);

    /* Downloaded transcript must parse as JSON v3.0 and replay through the engine */
    octi::OctiBoard replayWon;
    replayWon.deserializeGame(rTr.body);
    STRCMP_EQUAL(rGet.json.value("final_fen", "").c_str(),
                 replayWon.toPositionString().c_str());
    CHECK_EQUAL(7, replayWon.getGame()->getNumMoves());

    /* Verify legacy alias /octi also serves JSON */
    Reply rOcti = http().get("/api/v1/archive/games/" + wonId + "/octi");
    EXPECT_STATUS(200, rOcti);
    STRCMP_EQUAL("application/json; charset=utf-8", rOcti.contentType.c_str());
    STRCMP_EQUAL(rTr.body.c_str(), rOcti.body.c_str());

    /* Upload resigned game */
    std::string resTr = makeResignedTranscript({"33+a", "37+e"}, "inst-res-02", 1, "android-arm64");
    Reply rRes = http().post("/api/v1/archive/upload", "", Json{{"transcript", resTr}});
    EXPECT_STATUS(200, rRes);
    std::string resId = rRes.json.value("game_id", "");
    CHECK_FALSE(resId.empty());
    CHECK_EQUAL(2, rRes.json.value("ply_count", 0));

    Reply rGetRes = http().get("/api/v1/archive/games/" + resId);
    EXPECT_STATUS(200, rGetRes);
    STRCMP_EQUAL("RESIGNED", rGetRes.json.value("status", "").c_str());
    STRCMP_EQUAL("B", rGetRes.json.value("winner", "").c_str());
    CHECK_EQUAL(1, rGetRes.json.value("ai_difficulty", 0));
    STRCMP_EQUAL("android-arm64", rGetRes.json.value("platform", "").c_str());
    CHECK_TRUE(rGetRes.body.find("inst-res-02") == std::string::npos);

    /* Verify /api/v1/archive/games/:id/plies endpoint */
    Reply rPlies = http().get("/api/v1/archive/games/" + resId + "/plies");
    EXPECT_STATUS(200, rPlies);
    STRCMP_EQUAL(resId.c_str(), rPlies.json.value("game_id", "").c_str());
    STRCMP_EQUAL("2P", rPlies.json.value("mode", "").c_str());
    CHECK_FALSE(rPlies.json.value("initialFen", "").empty());
    CHECK_EQUAL(2, static_cast<int>(rPlies.json["plies"].size()));
    CHECK_EQUAL(1, rPlies.json["plies"][0].value("plyIndex", 0));
    STRCMP_EQUAL("A", rPlies.json["plies"][0].value("player", "").c_str());
    STRCMP_EQUAL("33+a", rPlies.json["plies"][0].value("move", "").c_str());
    CHECK_FALSE(rPlies.json["plies"][0].value("fenAfter", "").empty());
    CHECK_EQUAL(2, rPlies.json["plies"][1].value("plyIndex", 0));
    STRCMP_EQUAL("B", rPlies.json["plies"][1].value("player", "").c_str());
    STRCMP_EQUAL("37+e", rPlies.json["plies"][1].value("move", "").c_str());
    CHECK_FALSE(rPlies.json["plies"][1].value("fenAfter", "").empty());
}

TEST(OctiHttpServerIntegrationTest, ArchiveUploadRejectsUncompletedAndTamperedGames)
{
    /* 1. In-progress game rejected with 400 GAME_NOT_COMPLETED */
    Json jInProg = Json::parse(makeWonTranscript());
    jInProg["status"] = "IN_PROGRESS";
    Reply rInProg = http().post("/api/v1/archive/upload", "", Json{{"transcript", jInProg.dump()}});
    EXPECT_STATUS(400, rInProg);
    STRCMP_EQUAL("GAME_NOT_COMPLETED", rInProg.json.value("error", "").c_str());

    /* 2. False claim of GAME_OVER when engine replay shows game not over */
    Json jPremature = Json::parse(makeResignedTranscript({"33+a", "37+e"}, "inst-1", 1));
    jPremature["status"] = "GAME_OVER";
    Reply rPremature = http().post("/api/v1/archive/upload", "", Json{{"transcript", jPremature.dump()}});
    EXPECT_STATUS(400, rPremature);
    STRCMP_EQUAL("GAME_NOT_COMPLETED", rPremature.json.value("error", "").c_str());

    /* 3. Empty plies rejected with 400 GAME_NOT_COMPLETED */
    Json jEmpty = Json::parse(makeWonTranscript());
    jEmpty["plies"] = Json::array();
    Reply rEmpty = http().post("/api/v1/archive/upload", "", Json{{"transcript", jEmpty.dump()}});
    EXPECT_STATUS(400, rEmpty);
    STRCMP_EQUAL("GAME_NOT_COMPLETED", rEmpty.json.value("error", "").c_str());

    /* 4. Tampered winner rejected with 400 INVALID_GAME */
    Json jTamperWinner = Json::parse(makeWonTranscript());
    jTamperWinner["winner"] = "B";
    Reply rTamperWinner = http().post("/api/v1/archive/upload", "", Json{{"transcript", jTamperWinner.dump()}});
    EXPECT_STATUS(400, rTamperWinner);
    STRCMP_EQUAL("INVALID_GAME", rTamperWinner.json.value("error", "").c_str());

    /* 5. Tampered finalFen rejected with 400 INVALID_GAME */
    Json jTamperFen = Json::parse(makeWonTranscript());
    jTamperFen["finalFen"] = "2:1:1:1:P:A:-:0:4,0,24,1:4,0,24,1:A11,B11";
    Reply rTamperFen = http().post("/api/v1/archive/upload", "", Json{{"transcript", jTamperFen.dump()}});
    EXPECT_STATUS(400, rTamperFen);
    STRCMP_EQUAL("INVALID_GAME", rTamperFen.json.value("error", "").c_str());

    /* 6. Illegal move syntax rejected with 400 INVALID_GAME */
    Json jBadSyntax = Json::parse(makeWonTranscript());
    jBadSyntax["plies"][0]["move"] = "invalid_move_text";
    Reply rBadSyntax = http().post("/api/v1/archive/upload", "", Json{{"transcript", jBadSyntax.dump()}});
    EXPECT_STATUS(400, rBadSyntax);
    STRCMP_EQUAL("INVALID_GAME", rBadSyntax.json.value("error", "").c_str());

    /* 7. Illegal move semantics (e.g. jumping to illegal square) rejected with 400 INVALID_GAME */
    Json jBadSemantics = Json::parse(makeWonTranscript());
    jBadSemantics["plies"][0]["move"] = "73a-79";  // jump 6 squares without pods
    Reply rBadSemantics = http().post("/api/v1/archive/upload", "", Json{{"transcript", jBadSemantics.dump()}});
    EXPECT_STATUS(400, rBadSemantics);
    STRCMP_EQUAL("INVALID_GAME", rBadSemantics.json.value("error", "").c_str());

    /* 8. Turn order mismatch rejected with 400 INVALID_GAME */
    Json jWrongTurn = Json::parse(makeWonTranscript());
    jWrongTurn["plies"][0]["player"] = "B";
    Reply rWrongTurn = http().post("/api/v1/archive/upload", "", Json{{"transcript", jWrongTurn.dump()}});
    EXPECT_STATUS(400, rWrongTurn);
    STRCMP_EQUAL("INVALID_GAME", rWrongTurn.json.value("error", "").c_str());
}

TEST(OctiHttpServerIntegrationTest, ArchiveUploadRejectsMalformedPayloadsAndEnforcesCaps)
{
    /* 1. Non-JSON body */
    Reply rNonJson = http().raw("POST", "/api/v1/archive/upload", "", "random garbage string", "application/json");
    EXPECT_STATUS(400, rNonJson);
    STRCMP_EQUAL("MALFORMED_UPLOAD", rNonJson.json.value("error", "").c_str());

    /* 2. Missing format field */
    Json jNoFmt = Json::parse(makeWonTranscript());
    jNoFmt.erase("format");
    Reply rNoFmt = http().post("/api/v1/archive/upload", "", Json{{"transcript", jNoFmt.dump()}});
    EXPECT_STATUS(400, rNoFmt);
    STRCMP_EQUAL("MALFORMED_UPLOAD", rNoFmt.json.value("error", "").c_str());

    /* 3. Wrong format string */
    Json jWrongFmt = Json::parse(makeWonTranscript());
    jWrongFmt["format"] = "octi.transcript.v2";
    Reply rWrongFmt = http().post("/api/v1/archive/upload", "", Json{{"transcript", jWrongFmt.dump()}});
    EXPECT_STATUS(400, rWrongFmt);
    STRCMP_EQUAL("MALFORMED_UPLOAD", rWrongFmt.json.value("error", "").c_str());

    /* 4. Negative elapsedMs */
    Json jNegElapsed = Json::parse(makeWonTranscript());
    jNegElapsed["plies"][0]["elapsedMs"] = -10;
    Reply rNegElapsed = http().post("/api/v1/archive/upload", "", Json{{"transcript", jNegElapsed.dump()}});
    EXPECT_STATUS(400, rNegElapsed);
    STRCMP_EQUAL("MALFORMED_UPLOAD", rNegElapsed.json.value("error", "").c_str());

    /* 5. Negative AI budgetMs */
    Json jNegBudget = Json::parse(makeWonTranscript());
    jNegBudget["plies"][1]["ai"]["budgetMs"] = -500;
    Reply rNegBudget = http().post("/api/v1/archive/upload", "", Json{{"transcript", jNegBudget.dump()}});
    EXPECT_STATUS(400, rNegBudget);
    STRCMP_EQUAL("MALFORMED_UPLOAD", rNegBudget.json.value("error", "").c_str());

    /* 6. Invalid type inside client block */
    Json jUnknownClient = Json::parse(makeWonTranscript());
    jUnknownClient["client"]["extraBogus"] = Json::array({"not", "allowed"});
    Reply rUnknownClient = http().post("/api/v1/archive/upload", "", Json{{"transcript", jUnknownClient.dump()}});
    EXPECT_STATUS(400, rUnknownClient);
    STRCMP_EQUAL("MALFORMED_UPLOAD", rUnknownClient.json.value("error", "").c_str());

    /* 7. Oversized client string (> 128 bytes) */
    Json jOversizedClient = Json::parse(makeWonTranscript());
    jOversizedClient["client"]["platform"] = std::string(150, 'a');
    Reply rOversizedClient = http().post("/api/v1/archive/upload", "", Json{{"transcript", jOversizedClient.dump()}});
    EXPECT_STATUS(400, rOversizedClient);
    STRCMP_EQUAL("MALFORMED_UPLOAD", rOversizedClient.json.value("error", "").c_str());

    /* 8. Oversized transcript string (> 48 KiB) */
    std::string hugeTranscript(49153, 'x');
    Reply rHuge = http().post("/api/v1/archive/upload", "", Json{{"transcript", hugeTranscript}});
    EXPECT_STATUS(400, rHuge);
    STRCMP_EQUAL("MALFORMED_UPLOAD", rHuge.json.value("error", "").c_str());
}

TEST(OctiHttpServerIntegrationTest, ArchiveUploadDeduplicationAndInstallIdDeletion)
{
    std::string wonTr = makeWonTranscript("inst-delete-test", 3);
    Reply r1 = http().post("/api/v1/archive/upload", "", Json{{"transcript", wonTr}});
    EXPECT_STATUS(200, r1);
    std::string gameId = r1.json.value("game_id", "");
    CHECK_FALSE(gameId.empty());

    /* Submitting the exact same game twice returns 409 DUPLICATE_ID */
    Reply rDup = http().post("/api/v1/archive/upload", "", Json{{"transcript", wonTr}});
    EXPECT_STATUS(409, rDup);
    STRCMP_EQUAL("DUPLICATE_ID", rDup.json.value("error", "").c_str());

    /* Verify game exists before deletion */
    EXPECT_STATUS(200, http().get("/api/v1/archive/games/" + gameId));

    /* DELETE /api/v1/archive/install/:installId removes all games uploaded under this install ID */
    Reply rDel = http().del("/api/v1/archive/install/inst-delete-test", "");
    EXPECT_STATUS(200, rDel);
    CHECK_TRUE(rDel.json.value("ok", false));
    CHECK_EQUAL(1, rDel.json.value("deleted", 0));

    /* Deleted game is now gone */
    Reply rGone = http().get("/api/v1/archive/games/" + gameId);
    EXPECT_STATUS(404, rGone);

    /* Second deletion returns 0 deleted */
    Reply rDel2 = http().del("/api/v1/archive/install/inst-delete-test", "");
    EXPECT_STATUS(200, rDel2);
    CHECK_EQUAL(0, rDel2.json.value("deleted", -1));

    /* Malformed installId returns 404 NOT_FOUND per pathId validator */
    Reply rBadDel = http().del("/api/v1/archive/install/bad_id_that_exceeds_the_sixty_four_character_limit_01234567890123456", "");
    EXPECT_STATUS(404, rBadDel);
}

TEST(OctiHttpServerIntegrationTest, ArchiveUploadRateLimiting)
{
    HttpServerConfig cfg = testConfig();
    cfg.uploadRateLimitMax = 3;
    cfg.uploadRateLimitWindowMs = 60000;
    startEnv(cfg, true, nullptr);

    /* First 3 uploads from the same IP succeed */
    std::string t1 = makeWonTranscript("inst-rl-1", 1);
    std::string t2 = makeResignedTranscript({"33+a", "37+e"}, "inst-rl-2", 2);
    std::string t3 = makeResignedTranscript({"33+b", "37+f"}, "inst-rl-3", 3);
    EXPECT_STATUS(200, http().post("/api/v1/archive/upload", "", Json{{"transcript", t1}}));
    EXPECT_STATUS(200, http().post("/api/v1/archive/upload", "", Json{{"transcript", t2}}));
    EXPECT_STATUS(200, http().post("/api/v1/archive/upload", "", Json{{"transcript", t3}}));

    /* 4th upload from the same IP is rate-limited */
    std::string t4 = makeResignedTranscript({"33+c", "37+g"}, "inst-rl-4", 4);
    Reply r4 = http().post("/api/v1/archive/upload", "", Json{{"transcript", t4}});
    EXPECT_STATUS(429, r4);
    STRCMP_EQUAL("RATE_LIMITED", r4.json.value("error", "").c_str());
    CHECK_FALSE(r4.header("Retry-After").empty());
}

TEST(OctiHttpServerIntegrationTest, ArchiveFilteringSortingStatsAndNdjsonExport)
{
    /* Upload two distinct games with different properties */
    std::string wonTr = makeWonTranscript("inst-filt-1", 3, "linux-x86_64");
    std::string resTr = makeResignedTranscript({"33+a", "37+e"}, "inst-filt-2", 1, "android-arm64");
    Reply r1 = http().post("/api/v1/archive/upload", "", Json{{"transcript", wonTr}});
    Reply r2 = http().post("/api/v1/archive/upload", "", Json{{"transcript", resTr}});
    EXPECT_STATUS(200, r1);
    EXPECT_STATUS(200, r2);

    /* Test filtering by source */
    Reply rFiltSrc = http().get("/api/v1/archive/games?source=client_upload");
    EXPECT_STATUS(200, rFiltSrc);
    CHECK_EQUAL(2, rFiltSrc.json.value("total", 0));

    Reply rFiltOnline = http().get("/api/v1/archive/games?source=online");
    EXPECT_STATUS(200, rFiltOnline);
    CHECK_EQUAL(0, rFiltOnline.json.value("total", 0));

    /* Test filtering by winner */
    Reply rWinA = http().get("/api/v1/archive/games?winner=A");
    EXPECT_STATUS(200, rWinA);
    CHECK_EQUAL(1, rWinA.json.value("total", 0));

    Reply rWinB = http().get("/api/v1/archive/games?winner=B");
    EXPECT_STATUS(200, rWinB);
    CHECK_EQUAL(1, rWinB.json.value("total", 0));

    /* Test filtering by status */
    Reply rStGov = http().get("/api/v1/archive/games?status=GAME_OVER");
    EXPECT_STATUS(200, rStGov);
    CHECK_EQUAL(1, rStGov.json.value("total", 0));

    Reply rStRes = http().get("/api/v1/archive/games?status=RESIGNED");
    EXPECT_STATUS(200, rStRes);
    CHECK_EQUAL(1, rStRes.json.value("total", 0));

    /* Test filtering by AI difficulty */
    Reply rAi3 = http().get("/api/v1/archive/games?aiDifficulty=3");
    EXPECT_STATUS(200, rAi3);
    CHECK_EQUAL(1, rAi3.json.value("total", 0));

    Reply rAi1 = http().get("/api/v1/archive/games?aiDifficulty=1");
    EXPECT_STATUS(200, rAi1);
    CHECK_EQUAL(1, rAi1.json.value("total", 0));

    Reply rAi5 = http().get("/api/v1/archive/games?aiDifficulty=5");
    EXPECT_STATUS(200, rAi5);
    CHECK_EQUAL(0, rAi5.json.value("total", 0));

    /* Test sorting */
    Reply rSortDesc = http().get("/api/v1/archive/games?sort=ply_count:desc");
    EXPECT_STATUS(200, rSortDesc);
    CHECK_EQUAL(2, static_cast<int>(rSortDesc.json["games"].size()));
    CHECK_EQUAL(7, rSortDesc.json["games"][0].value("ply_count", 0));
    CHECK_EQUAL(2, rSortDesc.json["games"][1].value("ply_count", 0));

    Reply rSortAsc = http().get("/api/v1/archive/games?sort=ply_count:asc");
    EXPECT_STATUS(200, rSortAsc);
    CHECK_EQUAL(2, rSortAsc.json["games"][0].value("ply_count", 0));
    CHECK_EQUAL(7, rSortAsc.json["games"][1].value("ply_count", 0));

    /* Invalid filter argument produces 400 INVALID_ARGUMENT */
    Reply rBadArg = http().get("/api/v1/archive/games?source=invalid_src");
    EXPECT_STATUS(400, rBadArg);
    STRCMP_EQUAL("INVALID_ARGUMENT", rBadArg.json.value("error", "").c_str());

    /* Test /api/v1/archive/stats */
    Reply rStats = http().get("/api/v1/archive/stats");
    EXPECT_STATUS(200, rStats);
    CHECK_EQUAL(2, rStats.json.value("total_games", 0));
    CHECK_EQUAL(2, rStats.json["by_source"].value("client_upload", 0));
    CHECK_EQUAL(2, rStats.json["by_mode"].value("2P", 0));
    CHECK_EQUAL(1, rStats.json["by_winner"].value("A", 0));
    CHECK_EQUAL(1, rStats.json["by_winner"].value("B", 0));
    CHECK_TRUE(rStats.json.contains("ai_win_rates") && rStats.json["ai_win_rates"].is_array());

    /* Test /api/v1/archive/export (NDJSON streaming) */
    Reply rExp = http().get("/api/v1/archive/export");
    EXPECT_STATUS(200, rExp);
    STRCMP_EQUAL("application/x-ndjson; charset=utf-8", rExp.contentType.c_str());
    CHECK_TRUE(rExp.header("Content-Disposition").find("octi-archive-export.ndjson") != std::string::npos);

    /* Split NDJSON lines */
    std::stringstream ss(rExp.body);
    std::string line;
    int lineCount = 0;
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        ++lineCount;
        auto jRow = Json::parse(line);
        STRCMP_EQUAL("octi.transcript.v3", jRow.value("format", "").c_str());
        STRCMP_EQUAL("CC-BY-4.0", jRow.value("license", "").c_str());
        STRCMP_EQUAL("Octi game data, Charles Chiou (selfso.com)",
                     jRow.value("attribution", "").c_str());
        CHECK_TRUE(line.find("inst-filt") == std::string::npos);
        CHECK_TRUE(line.find("\"extended\"") == std::string::npos);
    }
    CHECK_EQUAL(2, lineCount);
}

/* ================================================================== */
/* ServerApp: options and a real process-style run                     */
/* ================================================================== */

TEST(ServerFoundationHttpTest, RunServesAndStopsCleanlyOnRequest)
{
    char tmpl[] = "/tmp/octi_app_XXXXXX";
    CHECK_TRUE(mkdtemp(tmpl) != nullptr);
    std::string dir = tmpl;
    std::string db = dir + "/app.db";
    const char *argv[] = {"octi_server", "--port", "0", "--bind", "127.0.0.1",
                          "--db", db.c_str()};
    ServerApp app;
    std::string err;
    CHECK_TRUE(app.parseArgs(7, argv, err));

    /* run() blocks, so it runs on its own thread and writes to its own stream. */
    std::ostringstream out;
    std::ostringstream errOut;
    std::atomic<int> rc(-99);
    std::thread th([&]() { rc = app.run(out, errOut); });
    int port = 0;
    for (int i = 0; i < 400 && port == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::string text = out.str();
        size_t at = text.find("listening on 127.0.0.1:");
        if (at != std::string::npos) {
            port = std::atoi(text.c_str() + at + 23);
        }
    }
    CHECK_TRUE(port > 0);
    Http http(port);
    Reply h = http.get("/api/v1/health");
    EXPECT_STATUS(200, h);
    STRCMP_EQUAL("ok", h.json.value("status", "").c_str());
    Reply ui = http.get("/");
    EXPECT_STATUS(200, ui);
    CHECK_TRUE(ui.body.find("<title>OCTI") != std::string::npos);
    /* No provider is configured, so the provider list is empty. */
    CHECK_EQUAL(0, static_cast<int>(http.get("/api/v1/auth/providers")
                                        .json["providers"].size()));
    /* The database really is private. */
    struct stat st;
    CHECK_EQUAL(0, stat(db.c_str(), &st));
    CHECK_EQUAL(0600, static_cast<int>(st.st_mode & 0777));

    app.requestStop();
    th.join();
    LONGS_EQUAL(0, rc.load());
    CHECK_TRUE(out.str().find("octi_server stopped") != std::string::npos);
    CHECK_TRUE(errOut.str().find("no SSO provider") != std::string::npos);
    Reply gone = http.get("/api/v1/health");
    CHECK_EQUAL(-1, gone.status);
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(ServerFoundationHttpTest, RunReportsStartupFailures)
{
    ServerApp app;
    std::string err;
    std::ostringstream out;
    std::ostringstream errOut;

    /* No --db. */
    const char *noDb[] = {"octi_server", "--port", "0"};
    CHECK_TRUE(app.parseArgs(3, noDb, err));
    LONGS_EQUAL(2, app.run(out, errOut));
    CHECK_TRUE(errOut.str().find("--db") != std::string::npos);

    /* Unopenable database path. */
    const char *badDb[] = {"octi_server", "--db", "/proc/octi_cannot_exist/x.db"};
    CHECK_TRUE(app.parseArgs(3, badDb, err));
    std::ostringstream err2;
    LONGS_EQUAL(2, app.run(out, err2));
    CHECK_TRUE(err2.str().find("database") != std::string::npos);

    /* Port already taken. */
    char tmpl[] = "/tmp/octi_app_XXXXXX";
    CHECK_TRUE(mkdtemp(tmpl) != nullptr);
    std::string dir = tmpl;
    PortHolder holder;
    int busy = holder.port();
    CHECK_TRUE(busy > 0);
    std::string db = dir + "/x.db";
    std::string portText = std::to_string(busy);
    const char *busyArgs[] = {"octi_server", "--port", portText.c_str(), "--db", db.c_str()};
    CHECK_TRUE(app.parseArgs(5, busyArgs, err));
    std::ostringstream err3;
    LONGS_EQUAL(2, app.run(out, err3));
    CHECK_TRUE(err3.str().find("cannot bind") != std::string::npos);

    /* Bad OIDC config. */
    std::string cfgPath = dir + "/oidc.json";
    {
        std::ofstream f(cfgPath);
        f << "{ not json";
    }
    const char *badCfg[] = {"octi_server", "--db", db.c_str(), "--oidc-config", cfgPath.c_str()};
    CHECK_TRUE(app.parseArgs(5, badCfg, err));
    std::ostringstream err4;
    LONGS_EQUAL(2, app.run(out, err4));
    CHECK_TRUE(err4.str().find("OIDC") != std::string::npos);
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(ServerFoundationHttpTest, OidcConfigIsParsedAndSecretFilesMustBePrivate)
{
    char tmpl[] = "/tmp/octi_cfg_XXXXXX";
    CHECK_TRUE(mkdtemp(tmpl) != nullptr);
    std::string dir = tmpl;
    CHECK_EQUAL(0, chmod(dir.c_str(), 0700));
    std::string path = dir + "/oidc.json";
    auto write = [&](const std::string &text, mode_t mode) {
        std::ofstream f(path.c_str(), std::ios::trunc);
        f << text;
        f.close();
        CHECK_EQUAL(0, chmod(path.c_str(), mode));
    };
    OidcConfig cfg;
    std::string err;

    /* Client ids alone are not secrets: a world-readable file is fine. */
    write("{\"providers\":{\"google\":{\"client_ids\":[\"g1\",\"g2\"]},"
          "\"line\":{\"client_id\":\"l1\"}}}", 0644);
    CHECK_TRUE(ServerApp::loadOidcConfig(path, cfg, err));
    CHECK_EQUAL(2, static_cast<int>(cfg.size()));
    CHECK_EQUAL(2, static_cast<int>(cfg["google"].clientIds.size()));
    STRCMP_EQUAL("l1", cfg["line"].clientIds[0].c_str());

    /* An app secret demands 0600. */
    const std::string withSecret =
        "{\"providers\":{\"facebook\":{\"client_id\":\"f1\",\"app_secret\":\"s3cret\"}}}";
    write(withSecret, 0644);
    CHECK_FALSE(ServerApp::loadOidcConfig(path, cfg, err));
    CHECK_TRUE(err.find("chmod 600") != std::string::npos);
    CHECK_TRUE(err.find("s3cret") == std::string::npos);
    CHECK_TRUE(cfg.empty());
    write(withSecret, 0640);
    CHECK_FALSE(ServerApp::loadOidcConfig(path, cfg, err));
    write(withSecret, 0600);
    CHECK_TRUE(ServerApp::loadOidcConfig(path, cfg, err));
    STRCMP_EQUAL("s3cret", cfg["facebook"].appSecret.c_str());
    STRCMP_EQUAL("f1", cfg["facebook"].clientIds[0].c_str());
    /* A group/other-writable parent could swap the file: refused too. */
    CHECK_EQUAL(0, chmod(dir.c_str(), 0777));
    CHECK_FALSE(ServerApp::loadOidcConfig(path, cfg, err));
    CHECK_TRUE(err.find("writable") != std::string::npos);
    CHECK_EQUAL(0, chmod(dir.c_str(), 0700));

    /* Malformed documents. */
    const char *bad[] = {
        "not json", "[]", "{}", "{\"providers\":[]}",
        "{\"providers\":{\"twitter\":{\"client_id\":\"x\"}}}",
        "{\"providers\":{\"google\":{}}}",
        "{\"providers\":{\"google\":{\"client_ids\":[]}}}",
        "{\"providers\":{\"google\":{\"client_ids\":[5]}}}",
        "{\"providers\":{\"google\":{\"client_id\":\"\"}}}",
        "{\"providers\":{\"google\":{\"client_id\":\"g\",\"app_secret\":\"x\"}}}",
        "{\"providers\":{\"google\":\"g\"}}"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
        write(bad[i], 0600);
        CHECK_FALSE_TEXT(ServerApp::loadOidcConfig(path, cfg, err), bad[i]);
        CHECK_FALSE_TEXT(err.empty(), bad[i]);
    }
    CHECK_FALSE(ServerApp::loadOidcConfig(dir + "/missing.json", cfg, err));

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(ServerFoundationHttpTest, FacebookAppSecretComesFromTheEnvironmentToo)
{
    OidcConfig cfg;
    std::string err;
    CHECK_EQUAL(0, setenv("OCTI_FACEBOOK_APP_SECRET", "env-secret", 1));
    CHECK_TRUE(ServerApp::loadOidcConfig("", cfg, err));
    STRCMP_EQUAL("env-secret", cfg["facebook"].appSecret.c_str());
    CHECK_TRUE(cfg["facebook"].clientIds.empty());
    CHECK_EQUAL(0, unsetenv("OCTI_FACEBOOK_APP_SECRET"));
    CHECK_TRUE(ServerApp::loadOidcConfig("", cfg, err));
    CHECK_TRUE(cfg.empty());
}

/* ================================================================== */
/* The real octi_server binary                                         */
/* ================================================================== */

namespace {

/* Launches the built octi_server, reads its "listening on" banner. */
class RealServer {
public:
    RealServer() : _pid(-1), _fd(-1), _port(0) {}
    ~RealServer()
    {
        if (_pid > 0) {
            ::kill(_pid, SIGKILL);
            int st = 0;
            ::waitpid(_pid, &st, 0);
        }
        if (_fd >= 0) {
            ::close(_fd);
        }
    }

    bool start(const std::string &db, int explicitPort = 0)
    {
        int fds[2];
        if (::pipe(fds) != 0) {
            return false;
        }
        _pid = ::fork();
        if (_pid < 0) {
            ::close(fds[0]);
            ::close(fds[1]);
            return false;
        }
        if (_pid == 0) {
            ::dup2(fds[1], STDOUT_FILENO);
            ::dup2(fds[1], STDERR_FILENO);
            ::close(fds[0]);
            ::close(fds[1]);
            std::string portStr = std::to_string(explicitPort);
            const char *argv[] = {OCTI_SERVER_BINARY, "--port", portStr.c_str(), "--bind",
                                  "127.0.0.1", "--db", db.c_str(), nullptr};
            ::execv(OCTI_SERVER_BINARY, const_cast<char *const *>(argv));
            ::_exit(127);
        }
        ::close(fds[1]);
        _fd = fds[0];
        _port = 0;
        _output.clear();
        const std::string needle = "listening on 127.0.0.1:";
        for (int i = 0; i < 1000 && _port == 0; ++i) {
            readAvailable(10);
            size_t at = _output.find(needle);
            if (at != std::string::npos && _output.find(',', at) != std::string::npos) {
                _port = std::atoi(_output.c_str() + at + needle.size());
            }
        }
        return _port > 0;
    }

    /* Signals the server and waits for it; returns the raw wait status. */
    int stopWith(int sig, int timeoutMs)
    {
        if (_pid < 0) {
            return -1;
        }
        ::kill(_pid, sig);
        int status = -1;
        for (int i = 0; i < timeoutMs / 10; ++i) {
            pid_t r = ::waitpid(_pid, &status, WNOHANG);
            if (r == _pid) {
                _pid = -1;
                readAvailable(10);
                if (_fd >= 0) {
                    ::close(_fd);
                    _fd = -1;
                }
                return status;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return -1;
    }

    int port() const { return _port; }
    const std::string &output() const { return _output; }

private:
    void readAvailable(int waitMs)
    {
        struct pollfd p;
        p.fd = _fd;
        p.events = POLLIN;
        while (::poll(&p, 1, waitMs) > 0 && (p.revents & POLLIN)) {
            char buf[512];
            ssize_t n = ::read(_fd, buf, sizeof(buf));
            if (n <= 0) {
                break;
            }
            _output.append(buf, static_cast<size_t>(n));
            waitMs = 0;
        }
    }

    pid_t _pid;
    int _fd;
    int _port;
    std::string _output;
};

} // namespace

TEST(ServerFoundationHttpTest, RealBinaryServesAndExitsZeroOnSigint)
{
    char tmpl[] = "/tmp/octi_bin_XXXXXX";
    CHECK_TRUE(mkdtemp(tmpl) != nullptr);
    std::string dir = tmpl;
    std::string db = dir + "/real.db";
    {
        RealServer srv;
        CHECK_TRUE_TEXT(srv.start(db), srv.output().c_str());
        Http http(srv.port());
        Reply h = http.get("/api/v1/health");
        EXPECT_STATUS(200, h);
        STRCMP_EQUAL("ok", h.json.value("status", "").c_str());
        Reply ui = http.get("/");
        EXPECT_STATUS(200, ui);
        CHECK_TRUE(ui.body.find("<title>OCTI") != std::string::npos);
        struct stat st;
        CHECK_EQUAL(0, stat(db.c_str(), &st));
        CHECK_EQUAL(0600, static_cast<int>(st.st_mode & 0777));

        int status = srv.stopWith(SIGINT, 5000);
        CHECK_TRUE_TEXT(WIFEXITED(status), srv.output().c_str());
        CHECK_EQUAL(0, WEXITSTATUS(status));
        CHECK_TRUE(srv.output().find("octi_server stopped") != std::string::npos);
        CHECK_EQUAL(-1, http.get("/api/v1/health").status);
    }
    /* A second launch on the same database works and SIGTERM is clean too. */
    {
        RealServer srv;
        CHECK_TRUE_TEXT(srv.start(db), srv.output().c_str());
        EXPECT_STATUS(200, Http(srv.port()).get("/api/v1/health"));
        int status = srv.stopWith(SIGTERM, 5000);
        CHECK_TRUE(WIFEXITED(status));
        CHECK_EQUAL(0, WEXITSTATUS(status));
    }
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(ServerFoundationHttpTest, SubprocessRestartPreservesOngoingGameAndSseStream)
{
    char tmpl[] = "/tmp/octi_subproc_XXXXXX";
    CHECK_TRUE(mkdtemp(tmpl) != nullptr);
    std::string dir = tmpl;
    std::string db = dir + "/subproc.db";

    std::string aliceToken;
    std::string bobToken;
    {
        AccountStore store;
        std::string err;
        CHECK_TRUE(store.openOrCreate(db, err));
        VerifiedClaims c1;
        c1.provider = "google";
        c1.sub = "sub-alice";
        c1.name = "Alice Anders";
        LoginResult r1 = store.loginWithSso(c1, "test");
        CHECK_TRUE(r1.ok);
        aliceToken = r1.sessionToken;

        VerifiedClaims c2;
        c2.provider = "apple";
        c2.sub = "sub-bob";
        c2.name = "Bob Brown";
        LoginResult r2 = store.loginWithSso(c2, "test");
        CHECK_TRUE(r2.ok);
        bobToken = r2.sessionToken;
        store.close();
    }

    int port = 0;
    std::string roomId;

    /* Phase 1: Launch subprocess, start game, make first move, open SSE stream */
    {
        RealServer srv;
        CHECK_TRUE_TEXT(srv.start(db, 0), srv.output().c_str());
        port = srv.port();
        CHECK_TRUE(port > 0);

        Http http(port);
        Reply rTbl = http.post("/api/v1/tables", aliceToken,
                               Json{{"name", "SubprocTest"}});
        EXPECT_STATUS(201, rTbl);
        roomId = rTbl.json.value("room_id", "");
        CHECK_FALSE(roomId.empty());

        Reply rJoin = http.post("/api/v1/tables/" + roomId + "/join", bobToken,
                                Json::object());
        EXPECT_STATUS(200, rJoin);
        CHECK_TRUE(rJoin.json.value("game_started", false));

        SseClient sse;
        sse.start(port, "/api/v1/games/" + roomId + "/events", aliceToken);
        CHECK_TRUE(sse.waitFor("retry: 3000", 3000));

        /* Alice makes the first move: add prong 'b' to pod 33 */
        Reply rMv = http.post("/api/v1/games/" + roomId + "/move", aliceToken,
                              Json{{"move", "33+b"}});
        EXPECT_STATUS(200, rMv);
        STRCMP_EQUAL("33+b", rMv.json.value("move", "").c_str());
        CHECK_TRUE(sse.waitFor("33+b", 3000));

        /* Shutdown server with SIGTERM; stream must receive event: reconnect */
        int status = srv.stopWith(SIGTERM, 5000);
        CHECK_TRUE_TEXT(WIFEXITED(status), srv.output().c_str());
        CHECK_EQUAL(0, WEXITSTATUS(status));
        CHECK_TRUE(srv.output().find("octi_server stopped") != std::string::npos);

        CHECK_TRUE(sse.waitFinished(5000));
        CHECK_TRUE(sse.data().find("event: reconnect") != std::string::npos);
        sse.stop();
    }

    /* Phase 2: Relaunch subprocess on same port and DB; state & stream resume */
    {
        RealServer srv;
        CHECK_TRUE_TEXT(srv.start(db, port), srv.output().c_str());
        CHECK_EQUAL(port, srv.port());

        Http http(port);
        Reply rGame = http.get("/api/v1/games/" + roomId, aliceToken);
        EXPECT_STATUS(200, rGame);
        STRCMP_EQUAL("IN_PROGRESS", rGame.json.value("phase", "").c_str());
        CHECK_EQUAL(2, rGame.json.value("turn_seat", 0));
        CHECK_EQUAL(1, rGame.json.value("ply_count", 0));

        /* Reconnect SSE stream specifying Last-Event-ID */
        httplib::Headers sseHeaders;
        sseHeaders.insert(std::make_pair("Last-Event-ID", "2"));
        SseClient sse;
        sse.start(port, "/api/v1/games/" + roomId + "/events", bobToken, sseHeaders);
        CHECK_TRUE(sse.waitFor("retry: 3000", 3000));

        /* Bob makes the second move: add prong 'e' to pod 37 */
        Reply rMv2 = http.post("/api/v1/games/" + roomId + "/move", bobToken,
                               Json{{"move", "37+e"}});
        EXPECT_STATUS(200, rMv2);
        STRCMP_EQUAL("37+e", rMv2.json.value("move", "").c_str());
        CHECK_TRUE(sse.waitFor("37+e", 3000));

        /* Alice resigns to end the game */
        Reply rResign = http.post("/api/v1/games/" + roomId + "/resign", aliceToken,
                                  Json::object());
        EXPECT_STATUS(200, rResign);
        CHECK_TRUE(sse.waitFor("P2_WIN", 3000));

        Reply rFinal = http.get("/api/v1/games/" + roomId, aliceToken);
        EXPECT_STATUS(200, rFinal);
        STRCMP_EQUAL("FINISHED", rFinal.json.value("phase", "").c_str());

        int status = srv.stopWith(SIGTERM, 5000);
        CHECK_TRUE_TEXT(WIFEXITED(status), srv.output().c_str());
        CHECK_EQUAL(0, WEXITSTATUS(status));
        sse.stop();
    }

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

int main(int argc, char **argv)
{
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    keys();     /* generate the RSA / EC test keys before the first test */
    return CommandLineTestRunner::RunAllTests(argc, argv);
}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
