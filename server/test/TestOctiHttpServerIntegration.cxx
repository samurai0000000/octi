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
#include "DatabaseArchiveCorpus.hxx"
#include "EloCalculator.hxx"
#include "GameRoomManager.hxx"
#include "OctiBoard.hxx"
#include "OctiCore.hxx"
#include "OctiDefs.hxx"
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
                      const std::string &name = "",
                      const std::function<void(Json &, Json &)> &mutate =
                          std::function<void(Json &, Json &)>())
{
    std::string actualName = name;
    if (actualName.empty()) {
        if (sub == "sub-presence-alice") {
            actualName = "PresenceAlice";
        }
    }
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
    claims["name"] = actualName;
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

    Env() : Env(testConfig(), true, nullptr) {}

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
    CHECK_TRUE(ui.body.find("Live Arena") == std::string::npos);
    CHECK_TRUE(ui.body.find("tab-live") == std::string::npos);
    CHECK_TRUE(ui.body.find("Game Archive") != std::string::npos);
    CHECK_TRUE(ui.body.find("Leaderboard") != std::string::npos);
    CHECK_TRUE(ui.body.find("<canvas id=\"board-canvas\">") != std::string::npos);
    CHECK_TRUE(ui.body.find("THEME_PALETTES") != std::string::npos);
    CHECK_TRUE(ui.body.find("renderBoardCanvas") != std::string::npos);
    CHECK_TRUE(ui.body.find("drawOctagonPod") != std::string::npos);
    CHECK_TRUE(ui.body.find("drawProngs") != std::string::npos);
    CHECK_TRUE(ui.body.find("LAST_MOVE_DISPLAY_DURATION_MS = 5000") != std::string::npos);
    CHECK_TRUE(ui.body.find("theme-select") != std::string::npos);
    CHECK_TRUE(ui.body.find("btn-web-rotate") != std::string::npos);
    CHECK_TRUE(ui.body.find("btn-replay-rotate") != std::string::npos);
    CHECK_TRUE(ui.body.find("res-card-a") != std::string::npos);
    CHECK_TRUE(ui.body.find("res-card-b") != std::string::npos);
    CHECK_TRUE(ui.body.find("stack-inspector-body") != std::string::npos);
    CHECK_TRUE(ui.body.find("Archive Replay") != std::string::npos);
    CHECK_TRUE(ui.body.find("value=\"2200\">0.5x") != std::string::npos);
    CHECK_TRUE(ui.body.find("value=\"1100\" selected>1.0x") != std::string::npos);
    CHECK_TRUE(ui.body.find("value=\"550\">2.0x") != std::string::npos);
    CHECK_TRUE(ui.body.find("/api/v1/leaderboard") != std::string::npos);
    CHECK_TRUE(ui.body.find("/api/v1/archive/games") != std::string::npos);
    CHECK_FALSE(ui.header("Content-Security-Policy").empty());
    /* Self-contained: no external resource of any kind (allow w3.org svg namespace). */
    std::string strippedUi = ui.body;
    size_t svgNs = 0;
    while ((svgNs = strippedUi.find("http://www.w3.org/2000/svg")) != std::string::npos) {
        strippedUi.replace(svgNs, 26, "");
    }
    CHECK_TRUE(strippedUi.find("http://") == std::string::npos);
    CHECK_TRUE(strippedUi.find("https://") == std::string::npos);
    CHECK_TRUE(strippedUi.find("src=\"//") == std::string::npos);
    CHECK_TRUE(strippedUi.find("<link") == std::string::npos);
    CHECK_TRUE(strippedUi.find("<iframe") == std::string::npos);
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

TEST(OctiHttpServerIntegrationTest, ActionAliasesAndClaimForfeitSupport)
{
    User a = signIn("google", "sub-action-a", "Alice Alias");
    User b = signIn("apple", "sub-action-b", "Bob Alias");
    std::vector<User> users = {a, b};

    std::string casual = startGame(
        users, Json{{"rated", false}, {"allow_takeback", true}});
    CHECK_EQUAL(2, playPlies(casual, users, GAME_MODE_2, 2));

    /* Uppercase DRAW_OFFER -> offer_draw, DECLINE_DRAW -> decline_draw */
    EXPECT_STATUS(200, act(casual, a, "DRAW_OFFER"));
    EXPECT_STATUS(200, act(casual, b, "DECLINE_DRAW"));

    /* Uppercase PAUSE_REQUEST -> request_pause, ACCEPT_PAUSE -> accept_pause, RESUME -> resume */
    EXPECT_STATUS(200, act(casual, a, "PAUSE_REQUEST"));
    EXPECT_STATUS(200, act(casual, b, "ACCEPT_PAUSE"));
    CHECK_TRUE(view(casual).value("paused", false));
    EXPECT_STATUS(200, act(casual, a, "RESUME"));
    CHECK_FALSE(view(casual).value("paused", true));

    /* Uppercase UNDO_REQUEST -> request_undo, DECLINE_UNDO -> decline_undo */
    EXPECT_STATUS(200, act(casual, a, "UNDO_REQUEST"));
    EXPECT_STATUS(200, act(casual, b, "DECLINE_UNDO"));

    /* claim_forfeit routing: when both players are connected and grace has not elapsed,
     * rooms.claimDisconnectForfeit returns INVALID_STATE (409) rather than 400 unknown action. */
    Reply cf = act(casual, a, "claim_forfeit");
    LONGS_EQUAL(409, cf.status);
    STRCMP_EQUAL("INVALID_STATE", cf.json.value("error", "").c_str());

    /* Complete casual game with agreed draw via uppercase aliases */
    EXPECT_STATUS(200, act(casual, a, "DRAW_OFFER"));
    Reply accDraw = act(casual, b, "ACCEPT_DRAW");
    EXPECT_STATUS(200, accDraw);
    CHECK_TRUE(accDraw.json.value("game_over", false));

    /* Uppercase REMATCH_REQUEST -> request_rematch, ACCEPT_REMATCH -> accept_rematch */
    EXPECT_STATUS(200, act(casual, a, "REMATCH_REQUEST"));
    Reply accRematch = act(casual, b, "ACCEPT_REMATCH");
    EXPECT_STATUS(200, accRematch);
    std::string rematchRoom = view(casual).value("rematch_room_id", "");
    CHECK_FALSE(rematchRoom.empty());
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
    const char *protectedGets[] = {"/api/v1/profile/me",
                                   "/api/v1/matchmaking/status"};
    for (size_t i = 0; i < sizeof(protectedGets) / sizeof(protectedGets[0]); ++i) {
        Reply none = http().get(protectedGets[i]);
        EXPECT_ERROR(401, "UNAUTHORIZED", none);
        STRCMP_EQUAL("Bearer", none.header("WWW-Authenticate").c_str());
        EXPECT_STATUS(401, http().get(protectedGets[i], "not-a-real-token"));
    }
    EXPECT_STATUS(401, http().get("/api/v1/lobby/state", "not-a-real-token"));
    EXPECT_STATUS(401, http().get("/api/v1/lobby/events", "not-a-real-token"));
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
    std::string room = startGame(users);
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
    EXPECT_STATUS(200, http().post(base + "/mute", b.token,
                                   Json{{"target", c.uuid}, {"mute_chat", false}}));
    EXPECT_STATUS(200, http().post(base + "/chat", c.token, Json{{"text", "unmuted"}}));
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

    /* SILENT_OBSERVERS: the observer may watch but no longer chat. */
    Reply s = http().post(base + "/settings", a.token,
                          Json{{"observer_policy", "SILENT_OBSERVERS"}});
    EXPECT_STATUS(200, s);
    STRCMP_EQUAL("SILENT_OBSERVERS", s.json.value("observer_policy", "").c_str());
    EXPECT_ERROR(403, "FORBIDDEN_POLICY",
                 http().post(base + "/chat", c.token, Json{{"text", "psst"}}));
    EXPECT_STATUS(200, http().get(base, c.token));
    EXPECT_STATUS(200, http().post(base + "/chat", b.token, Json{{"text", "players still chat"}}));

    std::vector<Json> pol = ofType(events(room, ""), "POLICY_UPDATED");
    CHECK_EQUAL(1, static_cast<int>(pol.size()));
    STRCMP_EQUAL("SILENT_OBSERVERS", pol[0].value("observer_policy", "").c_str());

    /* NO_OBSERVERS evicts the observer and hides the in-progress game. */
    EXPECT_STATUS(200, http().post(base + "/settings", a.token,
                                   Json{{"observer_policy", "NO_OBSERVERS"}}));
    EXPECT_ERROR(403, "FORBIDDEN_POLICY", http().get(base, c.token));
    EXPECT_ERROR(403, "FORBIDDEN_POLICY", http().get(base));
    EXPECT_ERROR(403, "FORBIDDEN_POLICY", http().get(base + "/events"));
    EXPECT_STATUS(200, http().get(base, a.token));
    EXPECT_STATUS(200, http().get(base + "/events", b.token));
}

TEST(OctiHttpServerIntegrationTest, SettingsResponseReportsTheMergedPolicies)
{
    User a = signIn("google", "sub-a", "Alice Anders");
    User b = signIn("apple", "sub-b", "Bob Brown");
    std::vector<User> users = {a, b};
    std::string room = startGame(users);
    const std::string path = "/api/v1/games/" + room + "/settings";
    Reply r = http().post(path, a.token, Json{{"observer_policy", "NO_OBSERVERS"}});
    EXPECT_STATUS(200, r);
    STRCMP_EQUAL("NO_OBSERVERS", r.json.value("observer_policy", "").c_str());
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
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/chat", a.token,
                                   Json{{"text", "marker after move"}}));
    CHECK_TRUE(sse.waitFor("marker after move", 3000));
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

TEST(OctiHttpServerIntegrationTest, SseResumesFromLastEventId)
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
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room + "/chat", a.token, Json{{"text", "fresh news"}}));
    CHECK_TRUE(sse.waitFor("fresh news", 3000));
    CHECK_TRUE(sse.data().find("old news") == std::string::npos);
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

TEST(OctiHttpServerIntegrationTest, CrashTelemetryNominalUploadAndRetrieval)
{
    /* 1. Upload nominal crash report without crash_id (server computes deterministic SHA-256) */
    Json payload = {
        {"platform", "windows-x86_64"},
        {"app_version", "2.1.2"},
        {"os_version", "Windows 11 Version 23H2"},
        {"exception_type", "std::invalid_argument"},
        {"exception_message", "'https' scheme is not supported."},
        {"thread_name", "OctiUploadThread"},
        {"stack_trace", "#0 octi.exe+0x0004a2b0\n#1 octi.exe+0x000511c4"},
        {"context", {{"match_mode", "LOCAL_BOT"}, {"ply_count", 42}}},
        {"install_id", "inst-crash-test-01"},
        {"created_at", 1761837000}
    };

    Reply rUpload = http().post("/api/v1/telemetry/crash", "", payload);
    EXPECT_STATUS(201, rUpload);
    CHECK_FALSE(rUpload.json.value("deduplicated", true));
    std::string crashId = rUpload.json.value("crash_id", "");
    CHECK_EQUAL(64, static_cast<int>(crashId.size()));

    /* 2. List crash reports and check filtering */
    Reply rList = http().get("/api/v1/telemetry/crashes");
    EXPECT_STATUS(200, rList);
    CHECK_EQUAL(1, rList.json.value("total", 0));
    CHECK_TRUE(rList.json.contains("crashes") && rList.json["crashes"].is_array());
    CHECK_EQUAL(1, static_cast<int>(rList.json["crashes"].size()));

    const auto &item = rList.json["crashes"][0];
    STRCMP_EQUAL(crashId.c_str(), item.value("crash_id", "").c_str());
    STRCMP_EQUAL("windows-x86_64", item.value("platform", "").c_str());
    STRCMP_EQUAL("2.1.2", item.value("app_version", "").c_str());
    STRCMP_EQUAL("Windows 11 Version 23H2", item.value("os_version", "").c_str());
    STRCMP_EQUAL("std::invalid_argument", item.value("exception_type", "").c_str());
    STRCMP_EQUAL("'https' scheme is not supported.", item.value("exception_message", "").c_str());
    STRCMP_EQUAL("OctiUploadThread", item.value("thread_name", "").c_str());
    /* Privacy check: install_id must NEVER be exposed in list */
    CHECK_FALSE(item.contains("install_id"));
    CHECK_TRUE(rList.body.find("inst-crash-test-01") == std::string::npos);

    /* Filter by platform */
    Reply rWin = http().get("/api/v1/telemetry/crashes?platform=windows-x86_64");
    EXPECT_STATUS(200, rWin);
    CHECK_EQUAL(1, rWin.json.value("total", 0));

    Reply rLin = http().get("/api/v1/telemetry/crashes?platform=linux-x86_64");
    EXPECT_STATUS(200, rLin);
    CHECK_EQUAL(0, rLin.json.value("total", 0));

    /* Filter by app_version */
    Reply rVer = http().get("/api/v1/telemetry/crashes?app_version=2.1.2");
    EXPECT_STATUS(200, rVer);
    CHECK_EQUAL(1, rVer.json.value("total", 0));

    Reply rOldVer = http().get("/api/v1/telemetry/crashes?app_version=1.0.0");
    EXPECT_STATUS(200, rOldVer);
    CHECK_EQUAL(0, rOldVer.json.value("total", 0));

    /* Filter by exception_type */
    Reply rExc = http().get("/api/v1/telemetry/crashes?exception_type=std::invalid_argument");
    EXPECT_STATUS(200, rExc);
    CHECK_EQUAL(1, rExc.json.value("total", 0));

    Reply rOtherExc = http().get("/api/v1/telemetry/crashes?exception_type=std::out_of_range");
    EXPECT_STATUS(200, rOtherExc);
    CHECK_EQUAL(0, rOtherExc.json.value("total", 0));

    /* Pagination */
    Reply rPage0 = http().get("/api/v1/telemetry/crashes?limit=1&offset=0");
    EXPECT_STATUS(200, rPage0);
    CHECK_EQUAL(1, static_cast<int>(rPage0.json["crashes"].size()));

    Reply rPage1 = http().get("/api/v1/telemetry/crashes?limit=1&offset=1");
    EXPECT_STATUS(200, rPage1);
    CHECK_EQUAL(0, static_cast<int>(rPage1.json["crashes"].size()));

    /* 3. Get single crash report detail */
    Reply rDetail = http().get("/api/v1/telemetry/crashes/" + crashId);
    EXPECT_STATUS(200, rDetail);
    STRCMP_EQUAL(crashId.c_str(), rDetail.json.value("crash_id", "").c_str());
    STRCMP_EQUAL("#0 octi.exe+0x0004a2b0\n#1 octi.exe+0x000511c4",
                 rDetail.json.value("stack_trace", "").c_str());
    CHECK_TRUE(rDetail.json.contains("context") && rDetail.json["context"].is_object());
    STRCMP_EQUAL("LOCAL_BOT", rDetail.json["context"].value("match_mode", "").c_str());
    CHECK_EQUAL(42, rDetail.json["context"].value("ply_count", 0));

    /* Privacy check: install_id must NEVER be exposed in detail */
    CHECK_FALSE(rDetail.json.contains("install_id"));
    CHECK_TRUE(rDetail.body.find("inst-crash-test-01") == std::string::npos);

    /* 4. Non-existent crash returns 404 */
    Reply rNotFound = http().get("/api/v1/telemetry/crashes/non-existent-crash-id-12345");
    EXPECT_STATUS(404, rNotFound);
}

TEST(OctiHttpServerIntegrationTest, CrashTelemetryDeduplicationAndPayloadBounds)
{
    /* 1. First upload succeeds with 201 Created */
    Json payload = {
        {"crash_id", "explicit-crash-id-001"},
        {"platform", "android-arm64"},
        {"app_version", "2.1.2"},
        {"exception_type", "java.lang.NullPointerException"},
        {"exception_message", "Attempt to invoke virtual method on a null object reference"},
        {"thread_name", "OctiBotThinkingThread"},
        {"stack_trace", "com.selfso.octi.ai.OctiBotEngine.think(OctiBotEngine.java:123)"},
        {"context", {{"turn", 5}}},
        {"install_id", "inst-dup-test"}
    };

    Reply r1 = http().post("/api/v1/telemetry/crash", "", payload);
    EXPECT_STATUS(201, r1);
    STRCMP_EQUAL("explicit-crash-id-001", r1.json.value("crash_id", "").c_str());
    CHECK_FALSE(r1.json.value("deduplicated", true));

    /* 2. Submitting duplicate crash_id returns 200 OK with deduplicated: true */
    Reply rDup = http().post("/api/v1/telemetry/crash", "", payload);
    EXPECT_STATUS(200, rDup);
    STRCMP_EQUAL("explicit-crash-id-001", rDup.json.value("crash_id", "").c_str());
    CHECK_TRUE(rDup.json.value("deduplicated", false));

    /* 3. Oversized payload (>64 KiB) rejected with 413 */
    std::string huge(70000, 'x');
    Reply rHuge = http().raw("POST", "/api/v1/telemetry/crash", "", huge, "application/json");
    EXPECT_STATUS(413, rHuge);
}

TEST(OctiHttpServerIntegrationTest, CrashTelemetryFaultInjectionAndInstallIdPurge)
{
    /* 1. Missing platform returns 400 */
    Json noPlatform = {
        {"exception_type", "std::runtime_error"},
        {"exception_message", "test error"}
    };
    Reply rNoPlat = http().post("/api/v1/telemetry/crash", "", noPlatform);
    EXPECT_STATUS(400, rNoPlat);

    /* 2. Missing exception_type returns 400 */
    Json noExc = {
        {"platform", "linux-x86_64"},
        {"exception_message", "test error"}
    };
    Reply rNoExc = http().post("/api/v1/telemetry/crash", "", noExc);
    EXPECT_STATUS(400, rNoExc);

    /* 3. Empty platform returns 400 */
    Json emptyPlat = {
        {"platform", ""},
        {"exception_type", "std::runtime_error"}
    };
    EXPECT_STATUS(400, http().post("/api/v1/telemetry/crash", "", emptyPlat));

    /* 4. Empty exception_type returns 400 */
    Json emptyExc = {
        {"platform", "linux-x86_64"},
        {"exception_type", ""}
    };
    EXPECT_STATUS(400, http().post("/api/v1/telemetry/crash", "", emptyExc));

    /* 5. Non-JSON body returns 400 */
    Reply rGarbage = http().raw("POST", "/api/v1/telemetry/crash", "", "this is not json", "application/json");
    EXPECT_STATUS(400, rGarbage);

    /* 6. Verify DELETE /api/v1/archive/install/:installId purges both games and crash reports */
    std::string installToPurge = "purge-install-id-99";
    std::string crashId = "crash-to-be-purged-001";

    Json crashPayload = {
        {"crash_id", crashId},
        {"platform", "linux-x86_64"},
        {"app_version", "2.1.2"},
        {"exception_type", "std::logic_error"},
        {"exception_message", "privacy purge test"},
        {"install_id", installToPurge}
    };
    Reply rUpload = http().post("/api/v1/telemetry/crash", "", crashPayload);
    EXPECT_STATUS(201, rUpload);

    /* Also upload a game for the same install_id */
    std::string gameTr = makeWonTranscript(installToPurge, 3);
    Reply rGame = http().post("/api/v1/archive/upload", "", Json{{"transcript", gameTr}});
    EXPECT_STATUS(200, rGame);
    std::string gameId = rGame.json.value("game_id", "");
    CHECK_FALSE(gameId.empty());

    /* Both game and crash report exist */
    EXPECT_STATUS(200, http().get("/api/v1/archive/games/" + gameId));
    EXPECT_STATUS(200, http().get("/api/v1/telemetry/crashes/" + crashId));

    /* Execute privacy deletion for installId */
    Reply rDel = http().del("/api/v1/archive/install/" + installToPurge, "");
    EXPECT_STATUS(200, rDel);
    CHECK_TRUE(rDel.json.value("ok", false));
    /* Expect 2 records deleted: 1 game + 1 crash report */
    CHECK_EQUAL(2, rDel.json.value("deleted", 0));

    /* Both game and crash report are now purged (404) */
    EXPECT_STATUS(404, http().get("/api/v1/archive/games/" + gameId));
    EXPECT_STATUS(404, http().get("/api/v1/telemetry/crashes/" + crashId));

    /* Crash report list no longer includes crashId */
    Reply rListAfter = http().get("/api/v1/telemetry/crashes");
    EXPECT_STATUS(200, rListAfter);
    CHECK_TRUE(rListAfter.body.find(crashId) == std::string::npos);
}

TEST(OctiHttpServerIntegrationTest, PublicUserDirectoryAndProfileInspector)
{
    User a = signIn("google", "sub-alice", "Alice Directory");
    User b = signIn("apple", "sub-bob", "Bob Directory");

    /* 1. Profile patch updates bio, location, display_name, and data:image/png avatar */
    std::string smallPng = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";
    Json patchOk = {
        {"display_name", "Alice Wonder"},
        {"location", "San Francisco, CA"},
        {"bio", "Competitive OCTI tactician and robot creator."},
        {"avatar_url", smallPng}
    };
    Reply rPatch = http().patch("/api/v1/profile/me", a.token, patchOk);
    EXPECT_STATUS(200, rPatch);
    STRCMP_EQUAL("Alice Wonder", rPatch.json.value("display_name", "").c_str());
    STRCMP_EQUAL("San Francisco, CA", rPatch.json.value("location", "").c_str());
    STRCMP_EQUAL("Competitive OCTI tactician and robot creator.", rPatch.json.value("bio", "").c_str());
    STRCMP_EQUAL(smallPng.c_str(), rPatch.json.value("avatar_url", "").c_str());

    /* PATCH /api/v1/profile alias */
    Reply rPatchAlt = http().patch("/api/v1/profile", a.token,
                                   Json{{"location", "Oakland, CA"}});
    EXPECT_STATUS(200, rPatchAlt);
    STRCMP_EQUAL("Oakland, CA", rPatchAlt.json.value("location", "").c_str());

    Reply rGetProfile = http().get("/api/v1/profile", a.token);
    EXPECT_STATUS(200, rGetProfile);
    STRCMP_EQUAL("Oakland, CA", rGetProfile.json.value("location", "").c_str());

    /* 2. Oversized avatar URI (>262144 bytes) is rejected with 400 */
    std::string hugeAvatar = "data:image/png;base64," + repeat('A', 270000);
    Reply rHugeAvatar = http().patch("/api/v1/profile/me", a.token,
                                    Json{{"avatar_url", hugeAvatar}});
    EXPECT_ERROR(400, "INVALID_AVATAR_URL", rHugeAvatar);

    /* 3. Oversized bio (>500 bytes) or location (>64 bytes) is rejected */
    Reply rHugeBio = http().patch("/api/v1/profile/me", a.token,
                                 Json{{"bio", repeat('B', 501)}});
    EXPECT_ERROR(400, "INVALID_BIO", rHugeBio);

    Reply rHugeLoc = http().patch("/api/v1/profile/me", a.token,
                                 Json{{"location", repeat('L', 65)}});
    EXPECT_ERROR(400, "INVALID_LOCATION", rHugeLoc);

    /* 4. Play a draw game so Alice and Bob have an archived match */
    playDrawGame(a, b);

    /* 5. Register a robot for Alice so active_robots_count increments */
    Reply rBot = http().post("/api/v1/robots", a.token,
                             Json{{"robot_name", "alice_bot_one"},
                                  {"description", "Alice's opening engine"}});
    EXPECT_STATUS(201, rBot);

    /* 6. GET /api/v1/users directory search and pagination */
    Reply rUsers = http().get("/api/v1/users?q=Alice&limit=10&offset=0");
    EXPECT_STATUS(200, rUsers);
    CHECK_TRUE(rUsers.json.value("total", 0) >= 1);
    CHECK_EQUAL(10, rUsers.json.value("limit", 0));
    CHECK_EQUAL(0, rUsers.json.value("offset", 0));
    Json uList = rUsers.json["users"];
    CHECK_TRUE(uList.is_array());
    CHECK_TRUE(uList.size() >= 1);
    bool foundAlice = false;
    for (size_t i = 0; i < uList.size(); ++i) {
        if (uList[i].value("uuid", "") == a.uuid) {
            foundAlice = true;
            STRCMP_EQUAL("Oakland, CA", uList[i].value("location", "").c_str());
            STRCMP_EQUAL("Competitive OCTI tactician and robot creator.",
                         uList[i].value("bio", "").c_str());
            CHECK_EQUAL(1, uList[i].value("active_robots_count", 0));
            CHECK_EQUAL(1, uList[i].value("games_played", 0));
            CHECK_EQUAL(1, uList[i].value("draws", 0));
            break;
        }
    }
    CHECK_TRUE(foundAlice);

    /* 7. Lookup user profile by UUID, handle, and @handle */
    Reply rByUuid = http().get("/api/v1/users/" + a.uuid);
    EXPECT_STATUS(200, rByUuid);
    STRCMP_EQUAL(a.handle.c_str(), rByUuid.json.value("handle", "").c_str());
    STRCMP_EQUAL("Oakland, CA", rByUuid.json.value("location", "").c_str());

    Reply rByHandle = http().get("/api/v1/users/" + a.handle);
    EXPECT_STATUS(200, rByHandle);
    STRCMP_EQUAL(a.uuid.c_str(), rByHandle.json.value("uuid", "").c_str());

    Reply rByAtHandle = http().get("/api/v1/users/@" + a.handle);
    EXPECT_STATUS(200, rByAtHandle);
    STRCMP_EQUAL(a.uuid.c_str(), rByAtHandle.json.value("uuid", "").c_str());

    /* 8. Backward compatibility: GET /api/v1/profile/:id and /api/v1/players/:uuid */
    Reply rProfileCompat = http().get("/api/v1/profile/" + a.uuid);
    EXPECT_STATUS(200, rProfileCompat);
    STRCMP_EQUAL(a.handle.c_str(), rProfileCompat.json.value("handle", "").c_str());

    Reply rPlayerCompat = http().get("/api/v1/players/" + a.uuid);
    EXPECT_STATUS(200, rPlayerCompat);
    STRCMP_EQUAL(a.handle.c_str(), rPlayerCompat.json.value("handle", "").c_str());

    /* 9. Critical security check: public profile / robots list MUST NOT leak robot_token */
    CHECK_TRUE(rByUuid.body.find("robot_token") == std::string::npos);
    CHECK_TRUE(rByUuid.body.find("token_prefix") == std::string::npos);
    Json pubBots = rByUuid.json["robots"];
    CHECK_TRUE(pubBots.is_array());
    CHECK_EQUAL(1, static_cast<int>(pubBots.size()));
    STRCMP_EQUAL("alice_bot_one", pubBots[0].value("robot_name", "").c_str());
    STRCMP_EQUAL("Alice's opening engine", pubBots[0].value("description", "").c_str());

    /* 10. Recent games history includes DRAW outcome */
    Json recGames = rByUuid.json["recent_games"];
    CHECK_TRUE(recGames.is_array());
    CHECK_EQUAL(1, static_cast<int>(recGames.size()));
    STRCMP_EQUAL("DRAW", recGames[0].value("player_outcome", "").c_str());

    /* 11. Unknown user returns 404 */
    EXPECT_ERROR(404, "NOT_FOUND", http().get("/api/v1/users/non-existent-user-uuid"));
}

TEST(OctiHttpServerIntegrationTest, RobotRegistration)
{
    User a = signIn("google", "sub-bot-owner", "Bot Owner");
    User b = signIn("apple", "sub-other-user", "Other User");

    /* 1. Authentication required */
    EXPECT_STATUS(401, http().get("/api/v1/robots"));
    EXPECT_STATUS(401, http().post("/api/v1/robots", "", Json{{"robot_name", "unauth_bot"}}));

    /* 2. Create 32 robots up to the maximum limit */
    Reply r1 = http().post("/api/v1/robots", a.token,
                          Json{{"robot_name", "octi_engine_01"},
                               {"description", "First engine"}});
    EXPECT_STATUS(201, r1);
    std::string bot1Uuid = r1.json.value("robot_uuid", "");
    CHECK_FALSE(bot1Uuid.empty());
    std::string token1 = r1.json.value("robot_token", "");
    CHECK_TRUE(token1.rfind("octi_bot_", 0) == 0);
    STRCMP_EQUAL("octi_engine_01", r1.json.value("robot_name", "").c_str());
    CHECK_FALSE(r1.json.value("revoked", true));

    for (int i = 2; i <= 32; ++i) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "octi_engine_%02d", i);
        Reply r = http().post("/api/v1/robots", a.token, Json{{"robot_name", buf}});
        EXPECT_STATUS(201, r);
    }

    /* 3. 33rd robot attempt is rejected with 409 ROBOT_LIMIT_REACHED */
    Reply rOver = http().post("/api/v1/robots", a.token,
                             Json{{"robot_name", "octi_engine_33"}});
    EXPECT_ERROR(409, "ROBOT_LIMIT_REACHED", rOver);

    /* 4. Duplicate name rejected with 409 ROBOT_NAME_TAKEN */
    Reply rDup = http().post("/api/v1/robots", b.token,
                            Json{{"robot_name", "octi_engine_01"}});
    EXPECT_ERROR(409, "ROBOT_NAME_TAKEN", rDup);

    /* 5. Invalid robot name rejected with 400 INVALID_ROBOT_NAME */
    Reply rShort = http().post("/api/v1/robots", b.token,
                              Json{{"robot_name", "ab"}});
    EXPECT_ERROR(400, "INVALID_ROBOT_NAME", rShort);

    /* 6. List robots for owner includes full robot_token */
    Reply rList = http().get("/api/v1/robots", a.token);
    EXPECT_STATUS(200, rList);
    CHECK_EQUAL(32, rList.json.value("max_robots", 0));
    CHECK_EQUAL(32, rList.json.value("active_count", 0));
    Json bots = rList.json["robots"];
    CHECK_TRUE(bots.is_array());
    CHECK_EQUAL(32, static_cast<int>(bots.size()));
    CHECK_FALSE(bots[0].value("robot_token", "").empty());

    /* 7. Patch robot description */
    Reply rPatch = http().patch("/api/v1/robots/" + bot1Uuid, a.token,
                                Json{{"description", "Updated description for engine a"}});
    EXPECT_STATUS(200, rPatch);
    CHECK_TRUE(rPatch.json.value("updated", false));

    /* Cannot patch another user's robot */
    Reply rCrossPatch = http().patch("/api/v1/robots/" + bot1Uuid, b.token,
                                     Json{{"description", "Hacked"}});
    EXPECT_ERROR(404, "NOT_FOUND", rCrossPatch);

    /* 8. Revoke robot via DELETE */
    Reply rRevoke = http().del("/api/v1/robots/" + bot1Uuid, a.token);
    EXPECT_STATUS(200, rRevoke);
    CHECK_TRUE(rRevoke.json.value("revoked", false));

    /* Revoking again fails */
    Reply rRevokeAgain = http().del("/api/v1/robots/" + bot1Uuid, a.token);
    CHECK_TRUE(rRevokeAgain.status == 404 || rRevokeAgain.status == 409);

    /* Active count decreased to 31 */
    Reply rListAfter = http().get("/api/v1/robots", a.token);
    EXPECT_STATUS(200, rListAfter);
    CHECK_EQUAL(31, rListAfter.json.value("active_count", 0));

    /* 9. Re-registering released robot name succeeds now that old one is revoked */
    Reply rReuseName = http().post("/api/v1/robots", b.token,
                                  Json{{"robot_name", "octi_engine_01"}});
    EXPECT_STATUS(201, rReuseName);
    STRCMP_EQUAL("octi_engine_01", rReuseName.json.value("robot_name", "").c_str());

    /* 10. Permanent robot deletion via DELETE /api/v1/robots/:id?permanent=1 */
    /* Delete already revoked robot */
    Reply rDelRevoked = http().del("/api/v1/robots/" + bot1Uuid + "?permanent=1", a.token);
    EXPECT_STATUS(200, rDelRevoked);
    CHECK_TRUE(rDelRevoked.json.value("deleted", false));

    /* Deleting again returns 404 NOT_FOUND */
    Reply rDelAgain = http().del("/api/v1/robots/" + bot1Uuid + "?permanent=1", a.token);
    EXPECT_ERROR(404, "NOT_FOUND", rDelAgain);

    /* Permanent delete directly on an active robot (octi_engine_02) */
    Reply rDelActive = http().del("/api/v1/robots/octi_engine_02?permanent=1", a.token);
    EXPECT_STATUS(200, rDelActive);
    CHECK_TRUE(rDelActive.json.value("deleted", false));

    /* Active count decreased from 31 to 30 */
    Reply rListAfterDel = http().get("/api/v1/robots", a.token);
    EXPECT_STATUS(200, rListAfterDel);
    CHECK_EQUAL(30, rListAfterDel.json.value("active_count", 0));
}

TEST(OctiHttpServerIntegrationTest, VisibleAnonymousObservation)
{
    User a = signIn("google", "sub-host", "Host Alice");
    User b = signIn("apple", "sub-guest", "Guest Bob");

    /* 1. Loopback 127.0.0.1 is filtered out of lobby presence, while real anonymous IPs show */
    Reply rLobbyLoopback = http().get("/api/v1/lobby/state");
    EXPECT_STATUS(200, rLobbyLoopback);
    Json lbPlayers = rLobbyLoopback.json["players"];
    CHECK_TRUE(lbPlayers.is_array());
    for (size_t i = 0; i < lbPlayers.size(); ++i) {
        CHECK_TRUE(lbPlayers[i].value("handle", "") != "anonymous-127.0.0.1");
    }

    httplib::Headers xffHeaders = {
        {"X-Forwarded-For", "203.0.113.42"},
        {"X-Octi-Client-Type", "web"}
    };
    Reply rLobbyReal = http().get("/api/v1/lobby/state", "", xffHeaders);
    EXPECT_STATUS(200, rLobbyReal);
    Json players = rLobbyReal.json["players"];
    CHECK_TRUE(players.is_array());
    bool foundAnon = false;
    for (size_t i = 0; i < players.size(); ++i) {
        if (players[i].value("handle", "") == "anonymous-203.0.113.42") {
            foundAnon = true;
            CHECK_TRUE(players[i].contains("clients"));
            CHECK_EQUAL(1, static_cast<int>(players[i]["clients"].size()));
            STRCMP_EQUAL("web", players[i]["clients"][0].get<std::string>().c_str());
            break;
        }
    }
    CHECK_TRUE(foundAnon);

    /* 2. Create public table and observe without authentication */
    std::string pubRoom = createTable(a, Json{{"mode", "2P"}});
    CHECK_FALSE(pubRoom.empty());

    Reply rObs = http().post("/api/v1/tables/" + pubRoom + "/observe", "", Json::object());
    EXPECT_STATUS(200, rObs);
    CHECK_TRUE(rObs.json.value("ok", false));
    Json obsList = rObs.json["observer_handles"];
    CHECK_TRUE(obsList.is_array());
    bool foundAnonObs = false;
    for (size_t i = 0; i < obsList.size(); ++i) {
        if (obsList[i].get<std::string>() == "anonymous-127.0.0.1") {
            foundAnonObs = true;
            break;
        }
    }
    CHECK_TRUE(foundAnonObs);

    /* Table state / game view includes anonymous-127.0.0.1 in observer_handles */
    Reply rGame = http().get("/api/v1/games/" + pubRoom);
    EXPECT_STATUS(200, rGame);
    Json gameObs = rGame.json["observer_handles"];
    CHECK_TRUE(gameObs.is_array());
    bool inGameObs = false;
    for (size_t i = 0; i < gameObs.size(); ++i) {
        if (gameObs[i].get<std::string>() == "anonymous-127.0.0.1") {
            inGameObs = true;
            break;
        }
    }
    CHECK_TRUE(inGameObs);

    /* 3. Passcode-protected table: anonymous without passcode or wrong passcode returns 403 */
    std::string privRoom = createTable(b, Json{{"mode", "2P"}, {"passcode", "secretPass"}});
    CHECK_FALSE(privRoom.empty());

    /* Passcode-protected table: anonymous without passcode or with passcode is rejected with 403 */
    Reply rPrivFail = http().post("/api/v1/tables/" + privRoom + "/observe", "", Json::object());
    EXPECT_ERROR(403, "FORBIDDEN_PASSCODE", rPrivFail);

    Reply rPrivAnon = http().post("/api/v1/tables/" + privRoom + "/observe", "",
                                  Json{{"passcode", "secretPass"}});
    EXPECT_ERROR(403, "FORBIDDEN_PASSCODE", rPrivAnon);

    /* Authenticated user can observe the passcode-protected room */
    Reply rPrivOk = http().post("/api/v1/tables/" + privRoom + "/observe", a.token,
                                Json::object());
    EXPECT_STATUS(200, rPrivOk);

    /* 4. Unauthenticated mutating actions are strictly 401 UNAUTHORIZED */
    EXPECT_STATUS(401, http().post("/api/v1/lobby/chat", "", Json{{"text", "hello"}}));
    EXPECT_STATUS(401, http().post("/api/v1/games/" + pubRoom + "/chat", "", Json{{"text", "hello"}}));
    EXPECT_STATUS(401, http().post("/api/v1/tables/" + pubRoom + "/join", "", Json::object()));
    EXPECT_STATUS(401, http().post("/api/v1/matchmaking/enqueue", "", Json{{"mode", "2P"}}));
    EXPECT_STATUS(401, http().post("/api/v1/games/" + pubRoom + "/move", "", Json{{"move", "33+a"}}));
}

TEST(OctiHttpServerIntegrationTest, OAuthBrokerAndAccountLinking)
{
    /* 1. GET /privacy returns 200 and privacy policy HTML */
    Reply rPriv = http().get("/privacy");
    EXPECT_STATUS(200, rPriv);
    CHECK_TRUE(rPriv.body.find("Privacy") != std::string::npos);
    CHECK_TRUE(rPriv.body.find("selfso.com") != std::string::npos);

    /* 2. GET / serves Web UI with 5 main tab anchors */
    Reply rRoot = http().get("/");
    EXPECT_STATUS(200, rRoot);
    CHECK_TRUE(rRoot.body.find("tab-home") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("tab-play") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("tab-users") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("tab-replay") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("tab-account") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("btn-toggle-robots-section") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("account-robots-expand-body") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("nav-robots-btn") != std::string::npos);

    /* 3. Authorize endpoint: provider validation */
    Reply rBadProv = http().get("/api/v1/auth/authorize?provider=twitter");
    EXPECT_ERROR(400, "UNSUPPORTED_PROVIDER", rBadProv);

    /* 4. Open-redirect prevention: reject illegal redirect_uri */
    EXPECT_ERROR(400, "INVALID_REDIRECT_URI",
                 http().get("/api/v1/auth/authorize?provider=google&redirect_uri=//evil.com"));
    EXPECT_ERROR(400, "INVALID_REDIRECT_URI",
                 http().get("/api/v1/auth/authorize?provider=google&redirect_uri=https://evil.com"));
    EXPECT_ERROR(400, "INVALID_REDIRECT_URI",
                 http().get("/api/v1/auth/authorize?provider=google&redirect_uri=javascript:alert(1)"));
    EXPECT_ERROR(400, "INVALID_REDIRECT_URI",
                 http().get("/api/v1/auth/authorize?provider=google&redirect_uri=http://evil.com:8080/callback"));

    /* 5. Valid authorize returns 302 with Location pointing to Google / Facebook */
    Reply rAuthGoogle = http().get("/api/v1/auth/authorize?provider=google&redirect_uri=/#account");
    EXPECT_STATUS(302, rAuthGoogle);
    std::string locGoogle = rAuthGoogle.header("Location");
    CHECK_TRUE(locGoogle.find("accounts.google.com") != std::string::npos);
    CHECK_TRUE(locGoogle.find("state=") != std::string::npos);
    CHECK_TRUE(locGoogle.find("client_id=") != std::string::npos);

    Reply rAuthFb = http().get("/api/v1/auth/authorize?provider=facebook&redirect_uri=http://127.0.0.1:8080/callback");
    EXPECT_STATUS(302, rAuthFb);
    std::string locFb = rAuthFb.header("Location");
    CHECK_TRUE(locFb.find("facebook.com") != std::string::npos);
    CHECK_TRUE(locFb.find("state=") != std::string::npos);

    /* 6. Callback validation: missing or invalid state is 400 INVALID_OAUTH_STATE */
    EXPECT_ERROR(400, "INVALID_OAUTH_STATE", http().get("/api/v1/auth/callback"));
    EXPECT_ERROR(400, "INVALID_OAUTH_STATE", http().get("/api/v1/auth/callback?state=bad.state"));

    /* 7. Account linking with force_relink: true merges empty conflicting account */
    User u1 = signIn("google", "sub-u1-google", "Primary User");
    User u2 = signIn("facebook", "sub-u2-fb", "Conflicting FB User");

    /* Attempt link without force_relink -> fails with conflict */
    std::string fbToken = makeToken("facebook", "sub-u2-fb", "Conflicting FB User");
    Reply rLinkConflict = http().post("/api/v1/auth/link", u1.token,
                                     Json{{"provider", "facebook"},
                                          {"id_token", fbToken}});
    CHECK_TRUE(rLinkConflict.status == 409 || rLinkConflict.status == 400);

    /* Attempt link with force_relink: true -> succeeds and merges empty account */
    Reply rForceLink = http().post("/api/v1/auth/link", u1.token,
                                   Json{{"provider", "facebook"},
                                        {"id_token", fbToken},
                                        {"force_relink", true}});
    EXPECT_STATUS(200, rForceLink);
    CHECK_TRUE(rForceLink.json.value("linked", false));
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

TEST(OctiHttpServerIntegrationTest, WebAuthAndIdentitiesManagement)
{
    /* 1. GET / serves Web UI with async function loadHealth() and handleAuthHash() */
    Reply rRoot = http().get("/");
    EXPECT_STATUS(200, rRoot);
    CHECK_TRUE(rRoot.body.find("async function loadHealth()") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("handleAuthHash()") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("/api/v1/health") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("btn-web-save-profile") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("octi.transcript.v3") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("image/png") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("<th>Bio</th>") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("btn-web-save-handle") == std::string::npos);
    CHECK_TRUE(rRoot.body.find("Save Handle") == std::string::npos);
    CHECK_TRUE(rRoot.body.find("web-purge-confirm-modal") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("syncWebPlayBoardSquareSize") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("fmtTtlCountdown") != std::string::npos);

    /* 2. Unauthenticated GET /api/v1/auth/identities returns 401 */
    Reply rUnauth = http().get("/api/v1/auth/identities");
    EXPECT_ERROR(401, "UNAUTHORIZED", rUnauth);

    /* 3. Authenticated GET /api/v1/auth/identities returns 200 with identities array */
    User a = signIn("google", "sub-webauth-a", "Alice WebAuth");
    Reply rAuth = http().get("/api/v1/auth/identities", a.token);
    EXPECT_STATUS(200, rAuth);
    CHECK_TRUE(rAuth.json.value("ok", false));
    CHECK_TRUE(rAuth.json.contains("identities"));
    CHECK_TRUE(rAuth.json["identities"].is_array());
    CHECK_EQUAL(1, static_cast<int>(rAuth.json["identities"].size()));
    STRCMP_EQUAL("google", rAuth.json["identities"][0].value("provider", "").c_str());
    CHECK_TRUE(rAuth.json["identities"][0].contains("provider_email"));
    CHECK_TRUE(rAuth.json["identities"][0].contains("email"));

    /* 4. DELETE /api/v1/auth/identities/google blocks unlinking sole remaining identity (409) */
    Reply rUnlinkLast = http().del("/api/v1/auth/identities/google", a.token);
    EXPECT_STATUS(409, rUnlinkLast);
    STRCMP_EQUAL("cannot_unlink_last_identity",
                 rUnlinkLast.json.value("error", "").c_str());

    /* 5. Link a second identity */
    Reply rLink = http().post("/api/v1/auth/link", a.token,
                             Json{{"provider", "line"},
                                  {"id_token", makeToken("line", "sub-line-webauth",
                                                         "Alice Line")}});
    EXPECT_STATUS(200, rLink);

    Reply rAuth2 = http().get("/api/v1/auth/identities", a.token);
    EXPECT_STATUS(200, rAuth2);
    CHECK_EQUAL(2, static_cast<int>(rAuth2.json["identities"].size()));

    /* 6. Unlinking succeeds when second identity exists (200) */
    Reply rUnlink = http().del("/api/v1/auth/identities/google", a.token);
    EXPECT_STATUS(200, rUnlink);
    CHECK_TRUE(rUnlink.json.value("ok", false));
    STRCMP_EQUAL("unlinked", rUnlink.json.value("status", "").c_str());
    CHECK_EQUAL(1, static_cast<int>(rUnlink.json["linked_identities"].size()));
    STRCMP_EQUAL("line",
                 rUnlink.json["linked_identities"][0].value("provider", "").c_str());

    /* 7. Unlinking remaining identity fails again with 409 */
    Reply rUnlinkLast2 = http().del("/api/v1/auth/identities/line", a.token);
    EXPECT_STATUS(409, rUnlinkLast2);
    STRCMP_EQUAL("cannot_unlink_last_identity",
                 rUnlinkLast2.json.value("error", "").c_str());
}

TEST(OctiHttpServerIntegrationTest, HealthRouteCompatibilityAlias)
{
    Reply rApi = http().get("/api/v1/health");
    EXPECT_STATUS(200, rApi);
    Reply rAlias = http().get("/health");
    EXPECT_STATUS(200, rAlias);
    STRCMP_EQUAL("ok", rAlias.json.value("status", "").c_str());
    STRCMP_EQUAL(rApi.json.value("version", "").c_str(),
                 rAlias.json.value("version", "").c_str());
    CHECK_EQUAL(rApi.json.value("protocol_version", -1),
                rAlias.json.value("protocol_version", -2));
}

TEST(OctiHttpServerIntegrationTest, HandleCooldownAndActionableErrorMessages)
{
    User a = signIn("google", "sub-cd-http", "Cooldown Alice");

    // 1. Initial handle customization succeeds
    Reply r1 = http().patch("/api/v1/profile/me", a.token,
                            Json{{"handle", "cool_alice"}});
    EXPECT_STATUS(200, r1);
    STRCMP_EQUAL("cool_alice", r1.json.value("handle", "").c_str());

    // 2. Immediate second handle change returns 429
    Reply r2 = http().patch("/api/v1/profile/me", a.token,
                            Json{{"handle", "cool_alice2"}});
    EXPECT_STATUS(429, r2);
    STRCMP_EQUAL("HANDLE_CHANGE_RATE_LIMITED", r2.json.value("error", "").c_str());
    CHECK_TRUE(r2.json.value("message", "").find("24 hours") != std::string::npos);

    // 3. Saving profile with the SAME handle is a no-op on handle and not rate limited
    Reply rSame = http().patch("/api/v1/profile/me", a.token,
                               Json{{"handle", "cool_alice"},
                                    {"display_name", "Cool Alice Updated"}});
    EXPECT_STATUS(200, rSame);
    STRCMP_EQUAL("Cool Alice Updated", rSame.json.value("display_name", "").c_str());

    // 4. Actionable error messages
    Reply rBadH = http().patch("/api/v1/profile/me", a.token,
                               Json{{"handle", "ab"}});
    EXPECT_STATUS(400, rBadH);
    STRCMP_EQUAL("INVALID_HANDLE", rBadH.json.value("error", "").c_str());
    CHECK_TRUE(rBadH.json.value("message", "").find("3 to 24 characters") != std::string::npos);

    Reply rBadBio = http().patch("/api/v1/profile/me", a.token,
                                 Json{{"bio", repeat('B', 501)}});
    EXPECT_STATUS(400, rBadBio);
    STRCMP_EQUAL("INVALID_BIO", rBadBio.json.value("error", "").c_str());
    CHECK_TRUE(rBadBio.json.value("message", "").find("500 characters") != std::string::npos);

    Reply rBadLoc = http().patch("/api/v1/profile/me", a.token,
                                 Json{{"location", repeat('L', 65)}});
    EXPECT_STATUS(400, rBadLoc);
    STRCMP_EQUAL("INVALID_LOCATION", rBadLoc.json.value("error", "").c_str());
    CHECK_TRUE(rBadLoc.json.value("message", "").find("64 printable characters") != std::string::npos);
}

TEST(OctiHttpServerIntegrationTest, ServerAvatarStorageAndEndpointServing)
{
    std::string avDir = env->dir + "/avatars";
    env->store.setAvatarDir(avDir);

    User a = signIn("google", "sub-av-http", "Avatar Alice");
    std::string pngUri = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";

    Reply rPatch = http().patch("/api/v1/profile/me", a.token,
                                Json{{"avatar_url", pngUri}});
    EXPECT_STATUS(200, rPatch);
    std::string avUrl = rPatch.json.value("avatar_url", "");
    CHECK_TRUE(avUrl.rfind("/api/v1/avatars/", 0) == 0);
    size_t q = avUrl.find('?');
    std::string avPath = (q != std::string::npos) ? avUrl.substr(0, q) : avUrl;
    std::string avFile = avPath.substr(std::string("/api/v1/avatars/").size());

    // Fetch avatar via GET /api/v1/avatars/:file
    Reply rGetAv = http().get(avPath);
    EXPECT_STATUS(200, rGetAv);
    STRCMP_EQUAL("image/png", rGetAv.contentType.c_str());
    CHECK_TRUE(rGetAv.body.size() > 0);

    // Fetch avatar via GET /avatar/:file alias
    Reply rGetAlias = http().get("/avatar/" + avFile);
    EXPECT_STATUS(200, rGetAlias);
    STRCMP_EQUAL("image/png", rGetAlias.contentType.c_str());

    // Non-existent avatar returns 404
    Reply rMissing = http().get("/api/v1/avatars/999999.png");
    EXPECT_STATUS(404, rMissing);
}

TEST(OctiHttpServerIntegrationTest, WebpAvatarTranscodingAndProfilePatchParity)
{
    std::string avDir = env->dir + "/avatars_webp";
    env->store.setAvatarDir(avDir);

    User a = signIn("google", "sub-webp-user", "WebP Alice");

    // 1. Direct transcoding verification
    std::string rawWebp(
        "\x52\x49\x46\x46\x3c\x00\x00\x00\x57\x45\x42\x50\x56\x50\x38\x20"
        "\x30\x00\x00\x00\xd0\x01\x00\x9d\x01\x2a\x02\x00\x02\x00\x02\x00"
        "\x34\x25\xa0\x02\x74\xba\x01\xf8\x00\x03\xb0\x00\xfe\xf0\xc4\x0b"
        "\xff\x20\xb9\x61\x75\xc8\xd7\xff\x20\x3f\xe4\x07\xfc\x80\xff\xf8"
        "\xf2\x00\x00\x00",
        68);
    std::string directPng;
    CHECK_TRUE(transcodeWebpToPng(rawWebp, directPng));
    CHECK_TRUE(directPng.size() >= 8);
    CHECK_EQUAL(0, std::memcmp(directPng.data(), "\x89PNG\r\n\x1a\n", 8));

    // 2. PATCH /api/v1/profile/me with WebP avatar, bio, and location
    std::string webpUri = "data:image/webp;base64,UklGRjwAAABXRUJQVlA4IDAAAADQAQCdASoCAAIAAgA0JaACdLoB+AADsAD+8MQL/yC5YXXI1/8gP+QH/ID/+PIAAAA=";
    Reply rPatch = http().patch("/api/v1/profile/me", a.token, Json{
        {"bio", "Master OCTI strategist"},
        {"location", "Tokyo, Japan"},
        {"avatar_url", webpUri}
    });
    EXPECT_STATUS(200, rPatch);

    // Verify linked_identities is present in PATCH response
    CHECK_TRUE(rPatch.json.contains("linked_identities"));
    CHECK_TRUE(rPatch.json["linked_identities"].is_array());
    CHECK_EQUAL(1, static_cast<int>(rPatch.json["linked_identities"].size()));
    STRCMP_EQUAL("google", rPatch.json["linked_identities"][0].value("provider", "").c_str());

    // Verify bio and location are persisted and last_seen_at updated
    STRCMP_EQUAL("Master OCTI strategist", rPatch.json.value("bio", "").c_str());
    STRCMP_EQUAL("Tokyo, Japan", rPatch.json.value("location", "").c_str());
    CHECK_TRUE(rPatch.json.value("last_seen_at", 0) > 0);

    // Verify avatar was transcoded to PNG and stored with .png extension
    std::string avUrl = rPatch.json.value("avatar_url", "");
    CHECK_TRUE(avUrl.rfind("/api/v1/avatars/", 0) == 0);
    size_t q = avUrl.find('?');
    std::string avPath = (q != std::string::npos) ? avUrl.substr(0, q) : avUrl;
    CHECK_TRUE(avPath.size() >= 4 && avPath.substr(avPath.size() - 4) == ".png");

    // Fetch avatar via GET /api/v1/avatars/... and verify PNG magic bytes and Content-Type
    Reply rGetAv = http().get(avPath);
    EXPECT_STATUS(200, rGetAv);
    STRCMP_EQUAL("image/png", rGetAv.contentType.c_str());
    CHECK_TRUE(rGetAv.body.size() >= 8);
    CHECK_EQUAL(0, std::memcmp(rGetAv.body.data(), "\x89PNG\r\n\x1a\n", 8));

    // GET /api/v1/profile/me matches PATCH response
    Reply rGetMe = http().get("/api/v1/profile/me", a.token);
    EXPECT_STATUS(200, rGetMe);
    STRCMP_EQUAL("Master OCTI strategist", rGetMe.json.value("bio", "").c_str());
    STRCMP_EQUAL("Tokyo, Japan", rGetMe.json.value("location", "").c_str());
    STRCMP_EQUAL(avUrl.c_str(), rGetMe.json.value("avatar_url", "").c_str());
    CHECK_TRUE(rGetMe.json.contains("linked_identities"));
    CHECK_EQUAL(1, static_cast<int>(rGetMe.json["linked_identities"].size()));

    // 3. Existing .webp file in avatarDir is served as image/png
    std::string diskWebpFile = avDir + "/7777.webp";
    {
        std::ofstream out(diskWebpFile.c_str(), std::ios::binary | std::ios::trunc);
        out.write(rawWebp.data(), rawWebp.size());
    }
    // GET /api/v1/avatars/7777.webp returns image/png
    Reply rGetWebp = http().get("/api/v1/avatars/7777.webp");
    EXPECT_STATUS(200, rGetWebp);
    STRCMP_EQUAL("image/png", rGetWebp.contentType.c_str());
    CHECK_TRUE(rGetWebp.body.size() >= 8);
    CHECK_EQUAL(0, std::memcmp(rGetWebp.body.data(), "\x89PNG\r\n\x1a\n", 8));

    // GET /api/v1/avatars/7777.png (where only 7777.webp exists on disk) also returns image/png
    Reply rGetFallback = http().get("/api/v1/avatars/7777.png");
    EXPECT_STATUS(200, rGetFallback);
    STRCMP_EQUAL("image/png", rGetFallback.contentType.c_str());
    CHECK_TRUE(rGetFallback.body.size() >= 8);
    CHECK_EQUAL(0, std::memcmp(rGetFallback.body.data(), "\x89PNG\r\n\x1a\n", 8));
}

TEST(OctiHttpServerIntegrationTest, AuthenticatedGameArchiveAndInstallAttribution)
{
    User a = signIn("google", "sub-attr-user", "Archiver Alice");
    Reply rHandle = http().patch("/api/v1/profile/me", a.token,
                                 Json{{"handle", "archiver_alice"}});
    EXPECT_STATUS(200, rHandle);

    // 1. Upload game with Authorization header and installId
    std::string tr1 = makeResignedTranscript({"33+b", "37+f"}, "dev-bound-01", 1);
    Reply rUp1 = http().post("/api/v1/archive/upload", a.token,
                             Json{{"transcript", tr1}});
    EXPECT_STATUS(200, rUp1);
    std::string gid1 = rUp1.json.value("game_id", "");
    CHECK_FALSE(gid1.empty());

    Reply rGet1 = http().get("/api/v1/archive/games/" + gid1);
    EXPECT_STATUS(200, rGet1);
    CHECK_TRUE(rGet1.json["players"].is_array() && !rGet1.json["players"].empty());
    STRCMP_EQUAL("archiver_alice", rGet1.json["players"][0].value("handle", "").c_str());
    STRCMP_EQUAL(a.uuid.c_str(), rGet1.json["players"][0].value("player_uuid", "").c_str());

    // 2. Upload game from same device WITHOUT Authorization header -> attributed via bound install_id!
    std::string tr2 = makeResignedTranscript({"33+c", "37+g"}, "dev-bound-01", 2);
    Reply rUp2 = http().post("/api/v1/archive/upload", "",
                             Json{{"transcript", tr2}});
    EXPECT_STATUS(200, rUp2);
    std::string gid2 = rUp2.json.value("game_id", "");
    CHECK_FALSE(gid2.empty());

    Reply rGet2 = http().get("/api/v1/archive/games/" + gid2);
    EXPECT_STATUS(200, rGet2);
    CHECK_TRUE(rGet2.json["players"].is_array() && !rGet2.json["players"].empty());
    STRCMP_EQUAL("archiver_alice", rGet2.json["players"][0].value("handle", "").c_str());
    STRCMP_EQUAL(a.uuid.c_str(), rGet2.json["players"][0].value("player_uuid", "").c_str());

    // 3. Upload from unknown device without auth -> remains anonymous Player
    std::string tr3 = makeResignedTranscript({"33+d", "37+h"}, "dev-unknown-99", 1);
    Reply rUp3 = http().post("/api/v1/archive/upload", "",
                             Json{{"transcript", tr3}});
    EXPECT_STATUS(200, rUp3);
    std::string gid3 = rUp3.json.value("game_id", "");
    CHECK_FALSE(gid3.empty());

    Reply rGet3 = http().get("/api/v1/archive/games/" + gid3);
    EXPECT_STATUS(200, rGet3);
    CHECK_TRUE(rGet3.json["players"].is_array() && !rGet3.json["players"].empty());
    STRCMP_EQUAL("Player", rGet3.json["players"][0].value("handle", "").c_str());
    CHECK_TRUE(rGet3.json["players"][0].value("player_uuid", "").empty());
}

TEST(OctiHttpServerIntegrationTest, CanonicalArchivePlayerSeatNormalizationAndDatabaseRepair)
{
    User u = signIn("google", "sub-canon-samurai", "Samurai");
    Reply rPatch = http().patch("/api/v1/profile/me", u.token,
                                Json{{"handle", "samurai"}});
    EXPECT_STATUS(200, rPatch);

    // 1. Construct a transcript where Human played side "B" (and won as "B")
    //    but the client uploaded "players" with inverted seat numbers:
    //    [{handle: "samurai", player: "B", seat: 1, type: "HUMAN"},
    //     {handle: "AI_L3", player: "A", seat: 2, type: "AI", aiDifficulty: 3}]
    Json jTr = Json::parse(makeResignedTranscript({"33+a", "37+e"}, "dev-canon-01", 3));
    jTr["winner"] = "B";
    OctiBoard bReplay(GAME_MODE_2, false, false, GAME_BASES_3);
    bReplay.getGame()->start(PLAYER_A);
    bReplay.getGame()->move(PLAYER_A, "33+a");
    bReplay.getGame()->move(PLAYER_B, "37+e");
    bReplay.getGame()->setResignedWinner(PLAYER_B);
    jTr["finalFen"] = bReplay.toPositionString();
    jTr["players"] = Json::array({
        Json{{"handle", "Player"}, {"player", "B"}, {"seat", 1}, {"type", "HUMAN"}},
        Json{{"handle", "AI_L3"}, {"player", "A"}, {"seat", 2}, {"type", "AI"}, {"aiDifficulty", 3}}
    });

    Reply rUp = http().post("/api/v1/archive/upload", u.token,
                            Json{{"transcript", jTr.dump()}});
    EXPECT_STATUS(200, rUp);
    std::string gid = rUp.json.value("game_id", "");
    CHECK_FALSE(gid.empty());

    // Verify GET /api/v1/archive/games/:id maps Seat 1 (A) -> AI_L3 and Seat 2 (B) -> samurai
    Reply rGet = http().get("/api/v1/archive/games/" + gid);
    EXPECT_STATUS(200, rGet);
    STRCMP_EQUAL("B", rGet.json.value("winner", "").c_str());
    CHECK_EQUAL(2, rGet.json.value("winner_seat", 0));
    STRCMP_EQUAL("P2_WIN", rGet.json.value("result", "").c_str());
    CHECK_TRUE(rGet.json["players"].is_array());
    CHECK_EQUAL(2, static_cast<int>(rGet.json["players"].size()));

    // Seat 1 is Player A = AI_L3
    CHECK_EQUAL(1, rGet.json["players"][0].value("seat", 0));
    STRCMP_EQUAL("AI_L3", rGet.json["players"][0].value("handle", "").c_str());
    CHECK_TRUE(rGet.json["players"][0].value("player_uuid", "").empty());

    // Seat 2 is Player B = samurai (authenticated user)
    CHECK_EQUAL(2, rGet.json["players"][1].value("seat", 0));
    STRCMP_EQUAL("samurai", rGet.json["players"][1].value("handle", "").c_str());
    STRCMP_EQUAL(u.uuid.c_str(), rGet.json["players"][1].value("player_uuid", "").c_str());

    // Also verify the stored transcript JSON itself has normalized seats sorted [1, 2]
    Reply rTr = http().get("/api/v1/archive/games/" + gid + "/transcript");
    EXPECT_STATUS(200, rTr);
    Json storedTr = Json::parse(rTr.body);
    CHECK_TRUE(storedTr["players"].is_array());
    CHECK_EQUAL(2, static_cast<int>(storedTr["players"].size()));
    CHECK_EQUAL(1, storedTr["players"][0].value("seat", 0));
    STRCMP_EQUAL("A", storedTr["players"][0].value("player", "").c_str());
    STRCMP_EQUAL("AI_L3", storedTr["players"][0].value("handle", "").c_str());
    CHECK_EQUAL(2, storedTr["players"][1].value("seat", 0));
    STRCMP_EQUAL("B", storedTr["players"][1].value("player", "").c_str());
    STRCMP_EQUAL("samurai", storedTr["players"][1].value("handle", "").c_str());

    // 2. Test automatic database repair on openOrCreate() for a legacy inverted record
    std::string repairDbPath = env->dir + "/repair_test.db";
    {
        AccountStore s1;
        std::string err;
        CHECK_TRUE(s1.openOrCreate(repairDbPath, err));
        VerifiedClaims vc;
        vc.provider = "google";
        vc.sub = "sub-repair-1";
        vc.name = "Samurai";
        vc.email = "samurai@example.com";
        vc.issuer = "https://accounts.google.com";
        vc.exp = 4000000000LL;
        LoginResult lr = s1.loginWithSso(vc, "test");
        CHECK_TRUE(lr.ok);
        ProfileUpdate pu;
        pu.setHandle = true;
        pu.handle = "samurai";
        PlayerProfile prof;
        CHECK_TRUE(s1.updateProfile(lr.profile.playerId, pu, &prof) == UpdateStatus::OK);

        // Insert a legacy inverted record directly via archiveGame
        GameRecord badRec;
        badRec.gameId = "g-legacy-inverted-record";
        badRec.gameMode = "2P";
        badRec.gameBases = 3;
        badRec.timeControl = "untimed";
        badRec.p1PlayerId = prof.playerId;
        badRec.p2PlayerId = 0;
        badRec.p1Handle = "samurai";
        badRec.p2Handle = "AI_L3";
        badRec.result = "P2_WIN";
        badRec.winnerSeat = 2;
        badRec.winner = "B";
        badRec.status = "GAME_OVER";
        badRec.termination = "BASE_OCCUPATION";
        badRec.plyCount = 42;
        badRec.source = "client_upload";
        badRec.startedAt = 1700000000;
        badRec.endedAt = 1700000100;
        badRec.uploadedAt = 1700000100;
        badRec.finalFen = bReplay.toPositionString();
        Json badJson = jTr;
        badJson["players"] = Json::array({
            Json{{"handle", "samurai"}, {"player", "B"}, {"seat", 1}, {"type", "HUMAN"}},
            Json{{"handle", "AI_L3"}, {"player", "A"}, {"seat", 2}, {"type", "AI"}, {"aiDifficulty", 3}}
        });
        badRec.octiTranscript = badJson.dump();
        CHECK_TRUE(s1.archiveGame(badRec, err) == ArchiveStatus::OK);
        s1.close();
    }
    {
        // Re-opening runs repairInvertedArchiveRecordsLocked()
        AccountStore s2;
        std::string err;
        CHECK_TRUE(s2.openOrCreate(repairDbPath, err));
        GameRecord fixed;
        CHECK_TRUE(s2.getArchivedGame("g-legacy-inverted-record", fixed));
        CHECK_EQUAL(0, fixed.p1PlayerId);
        CHECK_TRUE(fixed.p2PlayerId > 0);
        STRCMP_EQUAL("AI_L3", fixed.p1Handle.c_str());
        STRCMP_EQUAL("samurai", fixed.p2Handle.c_str());
        Json fixedTr = Json::parse(fixed.octiTranscript);
        CHECK_EQUAL(1, fixedTr["players"][0].value("seat", 0));
        STRCMP_EQUAL("A", fixedTr["players"][0].value("player", "").c_str());
        STRCMP_EQUAL("AI_L3", fixedTr["players"][0].value("handle", "").c_str());
        CHECK_EQUAL(2, fixedTr["players"][1].value("seat", 0));
        STRCMP_EQUAL("B", fixedTr["players"][1].value("player", "").c_str());
        STRCMP_EQUAL("samurai", fixedTr["players"][1].value("handle", "").c_str());
        s2.close();
    }
}

TEST(OctiHttpServerIntegrationTest, LobbyTablesSeatRosterChatTimestampAndWebUiRedesign)
{
    User host = signIn("google", "sub-host-table", "Table Host");
    EXPECT_STATUS(200, http().patch("/api/v1/profile/me", host.token,
                                    Json{{"handle", "table_host"}}));

    Reply rCreate = http().post("/api/v1/tables", host.token,
                                Json{{"mode", "2P"}, {"bases", 3}, {"rated", true}});
    EXPECT_STATUS(201, rCreate);
    std::string roomId = rCreate.json.value("room_id", "");
    CHECK_FALSE(roomId.empty());

    // Post lobby chat and verify created_at_ms & at_ms are epoch milliseconds
    Reply rChat = http().post("/api/v1/lobby/chat", host.token,
                              Json{{"text", "Join my 2P table!"}});
    EXPECT_STATUS(200, rChat);

    Reply rLobby = http().get("/api/v1/lobby/state", host.token);
    EXPECT_STATUS(200, rLobby);
    CHECK_TRUE(rLobby.json["tables"].is_array());
    CHECK_EQUAL(1, static_cast<int>(rLobby.json["tables"].size()));
    const Json &tJson = rLobby.json["tables"][0];
    STRCMP_EQUAL(roomId.c_str(), tJson.value("room_id", "").c_str());
    STRCMP_EQUAL(host.uuid.c_str(), tJson.value("host_uuid", "").c_str());
    STRCMP_EQUAL("table_host", tJson.value("host_handle", "").c_str());
    CHECK_TRUE(tJson.contains("created_at_ms"));
    CHECK_TRUE(tJson.contains("seats") && tJson["seats"].is_array());
    CHECK_EQUAL(2, static_cast<int>(tJson["seats"].size()));
    CHECK_EQUAL(1, tJson["seats"][0].value("seat", 0));
    STRCMP_EQUAL(host.uuid.c_str(), tJson["seats"][0].value("player_uuid", "").c_str());
    STRCMP_EQUAL("table_host", tJson["seats"][0].value("handle", "").c_str());
    CHECK_TRUE(tJson["seats"][0].value("connected", false));
    CHECK_TRUE(tJson["seats"][0].value("occupied", false));
    CHECK_EQUAL(2, tJson["seats"][1].value("seat", 0));
    STRCMP_EQUAL("", tJson["seats"][1].value("player_uuid", "").c_str());
    STRCMP_EQUAL("", tJson["seats"][1].value("handle", "").c_str());
    CHECK_FALSE(tJson["seats"][1].value("connected", true));
    CHECK_FALSE(tJson["seats"][1].value("occupied", true));

    // Verify GET /api/v1/tables/{id} returns occupied seat booleans
    Reply rTable = http().get("/api/v1/tables/" + roomId, host.token);
    EXPECT_STATUS(200, rTable);
    CHECK_TRUE(rTable.json.contains("seats") && rTable.json["seats"].is_array());
    CHECK_EQUAL(2, static_cast<int>(rTable.json["seats"].size()));
    CHECK_TRUE(rTable.json["seats"][0].value("occupied", false));
    CHECK_FALSE(rTable.json["seats"][1].value("occupied", true));

    // Verify GET /api/v1/leaderboard?mode=2P entries include avatar_url
    Reply rLb = http().get("/api/v1/leaderboard?mode=2P");
    EXPECT_STATUS(200, rLb);
    CHECK_TRUE(rLb.json.contains("entries") && rLb.json["entries"].is_array());
    CHECK_TRUE(rLb.json["entries"].size() >= 1);
    CHECK_TRUE(rLb.json["entries"][0].contains("avatar_url"));

    // Verify recent_chat has epoch millisecond created_at_ms
    CHECK_TRUE(rLobby.json["recent_chat"].is_array());
    CHECK_EQUAL(1, static_cast<int>(rLobby.json["recent_chat"].size()));
    int64_t chatTs = rLobby.json["recent_chat"][0].value("created_at_ms", 0LL);
    CHECK_TRUE(chatTs > 1000000000000LL);
    CHECK_EQUAL(chatTs, rLobby.json["recent_chat"][0].value("at_ms", 0LL));

    // Verify WebUI HTML includes confirmation modal, table-centric lobby elements, and omits quick match
    Reply rUi = http().get("/");
    EXPECT_STATUS(200, rUi);
    CHECK_TRUE(rUi.body.find("web-confirm-modal") != std::string::npos);
    CHECK_TRUE(rUi.body.find("showConfirmModal") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-my-table-banner") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-create-table-panel") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-lobby-tables-body") != std::string::npos);
    CHECK_TRUE(rUi.body.find("winnerHandleOf") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-web-quick-match") == std::string::npos);
}

TEST(OctiHttpServerIntegrationTest, WebPlayerDesktopAndroidParityElements)
{
    Reply rUi = http().get("/");
    EXPECT_STATUS(200, rUi);

    // 1. Tabletop 3-column stage & Reserve HUD containers
    CHECK_TRUE(rUi.body.find("web-play-reserves-col") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-play-board-wrap") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-play-canvas") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-play-side-col") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-res-card-a") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-res-card-b") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-web-rotate") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-sfx") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-play-theme-select") != std::string::npos);

    // 2. Overlays & Dialogs
    CHECK_TRUE(rUi.body.find("web-start-match-overlay") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-web-change-type") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-web-start-match") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-match-setup-modal") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-zoom-overlay") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-zoom-canvas") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-zoom-move-pod") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-zoom-place-pod") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-zoom-liberate-pod") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-ply-confirm-bar") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-web-commit-ply") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-web-cancel-ply") != std::string::npos);
    CHECK_TRUE(rUi.body.find("commitPulse1Hz") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-capture-overlay") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-thinking-overlay") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-thinking-progress-fill") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"brand-version-title\">OCTI v" + std::string(OCTI_APP_VERSION) + "</span>") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"web-thinking-title\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("pointer-events: none;") != std::string::npos);
    CHECK_TRUE(rUi.body.find("max-height: 48px;") != std::string::npos);
    CHECK_TRUE(rUi.body.find("var canDragMove =") != std::string::npos);
    CHECK_TRUE(rUi.body.find("inset: 24px;") == std::string::npos);
    CHECK_TRUE(rUi.body.find("web-gameover-overlay") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-export-modal") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-web-export-moves") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"web-online-users-section\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"web-online-users-list\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"web-online-tables\"") < rUi.body.find("id=\"web-online-users-section\""));
    CHECK_TRUE(rUi.body.find("X-Octi-Client-Type") != std::string::npos);
    CHECK_TRUE(rUi.body.find("renderClientTypeIconSvg") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-user-chip") != std::string::npos);

    // 3. Core parity JS functions & variables
    CHECK_TRUE(rUi.body.find("WebMoveComposer") != std::string::npos);
    CHECK_TRUE(rUi.body.find("OctiWebSoundSynth") != std::string::npos);
    CHECK_TRUE(rUi.body.find("openZoomedSquare") != std::string::npos);
    CHECK_TRUE(rUi.body.find("sortedPodIndicesMostProngsFirst") != std::string::npos);
    CHECK_TRUE(rUi.body.find("canLiberate") != std::string::npos);
    CHECK_TRUE(rUi.body.find("boardZoomScale") != std::string::npos);
    CHECK_TRUE(rUi.body.find("liveRotationOffsetDeg") != std::string::npos);

    // 4. Opponent pods captured display in reserve & replay HUD cards
    CHECK_TRUE(rUi.body.find("enemyPodsCapturedByPlayer = oppRes ? oppRes.capturedPods : 0") != std::string::npos);
    CHECK_TRUE(rUi.body.find("enemyPodsCaptured = (parsed.mode === \"4P\") ? r.capturedPods : (oppRes ? oppRes.capturedPods : 0)") != std::string::npos);

    // 5. Desktop 50/50 sidebar, chat zoom modal, online room chat & mobile responsive rules
    CHECK_TRUE(rUi.body.find("web-side-top-half") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-board-chat-panel") != std::string::npos);
    CHECK_TRUE(rUi.body.find("flex: 1 1 50%") != std::string::npos || rUi.body.find("flex:1 1 50%") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-board-chat-zoom-overlay") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-board-chat-zoom-card") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-board-chat-zoom-log") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-board-chat-zoom-scope") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-board-chat-zoom-input") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-web-board-chat-zoom-send") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-web-board-chat-zoom-close") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-web-board-chat-zoom") != std::string::npos);
    CHECK_TRUE(rUi.body.find("openBoardChatZoomModal") != std::string::npos);
    CHECK_TRUE(rUi.body.find("isInteractiveBoardTurn") != std::string::npos);
    CHECK_TRUE(rUi.body.find("syncOnlineGameRoom") != std::string::npos);
    CHECK_TRUE(rUi.body.find("pollOnlineGameEvents") != std::string::npos);
    CHECK_TRUE(rUi.body.find("sendOnlineRoomChatMessage") != std::string::npos);
    CHECK_TRUE(rUi.body.find("#web-play-board-wrap") != std::string::npos);
    CHECK_TRUE(rUi.body.find("order: 1") != std::string::npos || rUi.body.find("order:1") != std::string::npos);
    CHECK_TRUE(rUi.body.find("repeat(2") != std::string::npos);

    // 6. Strict absence of banned strings
    CHECK_TRUE(rUi.body.find("Close Inspector") == std::string::npos);
    CHECK_TRUE(rUi.body.find("Live Arena") == std::string::npos);
    CHECK_TRUE(rUi.body.find("tab-live") == std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-web-save-handle") == std::string::npos);
    CHECK_TRUE(rUi.body.find("Save Handle") == std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-web-quick-match") == std::string::npos);

    // 6. Strict self-contained CSP: zero external URLs
    std::string strippedUi = rUi.body;
    size_t svgNs = 0;
    while ((svgNs = strippedUi.find("http://www.w3.org/2000/svg")) != std::string::npos) {
        strippedUi.replace(svgNs, 26, "");
    }
    CHECK_TRUE(strippedUi.find("http://") == std::string::npos);
    CHECK_TRUE(strippedUi.find("https://") == std::string::npos);
    CHECK_TRUE(strippedUi.find("src=\"//") == std::string::npos);
    CHECK_TRUE(strippedUi.find("<link") == std::string::npos);
    CHECK_TRUE(strippedUi.find("<iframe") == std::string::npos);
}

TEST(OctiHttpServerIntegrationTest, ArchiveUploadFourPlayerAttributionAndValidation)
{
    User u1 = signIn("google", "sub-4p-u1", "Alice");
    User u2 = signIn("apple", "sub-4p-u2", "Bob");
    User u3 = signIn("facebook", "sub-4p-u3", "Carol");
    User u4 = signIn("microsoft", "sub-4p-u4", "Dave");

    EXPECT_STATUS(200, http().patch("/api/v1/profile/me", u1.token, Json{{"handle", "alice_4p"}}));
    EXPECT_STATUS(200, http().patch("/api/v1/profile/me", u2.token, Json{{"handle", "bob_4p"}}));
    EXPECT_STATUS(200, http().patch("/api/v1/profile/me", u3.token, Json{{"handle", "carol_4p"}}));
    EXPECT_STATUS(200, http().patch("/api/v1/profile/me", u4.token, Json{{"handle", "dave_4p"}}));

    const std::vector<std::string> kMoves = {
        "33+a",   "37+h",    "77+e",    "73+b",    "33a-34",  "37h+f-h",
        "77e-76", "73b-84",  "34a-35",  "37f-26",  "76e-75",  "84b+g",
        "35a-36", "26f+a",   "75e-74",  "84bg+h",  "36a-37",  "74e-73"
    };

    octi::OctiBoard b(octi::GAME_MODE_4, false, false, octi::GAME_BASES_3);
    octi::OctiGame *g = b.getGame();
    g->start(octi::PLAYER_A);
    for (const auto &mv : kMoves) {
        g->move(mv);
    }
    CHECK_TRUE(g->isGameOver());
    CHECK_EQUAL(static_cast<int>(octi::PLAYER_A), static_cast<int>(g->getWinner()));

    octi::OctiGameMetadata meta;
    meta.status = "GAME_OVER";
    meta.termination = "BASE_CAPTURE";
    meta.appVersion = "2.2.5";
    meta.platform = "linux-x86_64";
    meta.buildType = "release";
    meta.extended.present = true;
    meta.extended.installId = "dev-canon-4p";
    meta.players.push_back({1, octi::PLAYER_A, "HUMAN", "alice_4p", 0, 0});
    meta.players.push_back({2, octi::PLAYER_B, "HUMAN", "bob_4p", 0, 0});
    meta.players.push_back({3, octi::PLAYER_C, "HUMAN", "carol_4p", 0, 0});
    meta.players.push_back({4, octi::PLAYER_D, "HUMAN", "dave_4p", 0, 0});

    std::string trStr = b.serializeJson(meta);
    Json jTr = Json::parse(trStr);

    // 1. Upload valid 4P transcript
    Reply rUp = http().post("/api/v1/archive/upload", u1.token,
                            Json{{"transcript", jTr.dump()}});
    EXPECT_STATUS(200, rUp);
    std::string gid = rUp.json.value("game_id", "");
    CHECK_FALSE(gid.empty());

    // 2. Verify archive record retrieval
    Reply rGet = http().get("/api/v1/archive/games/" + gid);
    EXPECT_STATUS(200, rGet);
    STRCMP_EQUAL("4P", rGet.json.value("mode", "").c_str());
    CHECK_EQUAL(3, rGet.json.value("bases", 0));
    STRCMP_EQUAL("TEAM_13_WIN", rGet.json.value("result", "").c_str());
    CHECK_EQUAL(1, rGet.json.value("winner_seat", 0));
    STRCMP_EQUAL("A", rGet.json.value("winner", "").c_str());
    STRCMP_EQUAL("BASE_CAPTURE", rGet.json.value("termination", "").c_str());

    CHECK_TRUE(rGet.json["players"].is_array());
    CHECK_EQUAL(4, static_cast<int>(rGet.json["players"].size()));
    CHECK_EQUAL(1, rGet.json["players"][0].value("seat", 0));
    STRCMP_EQUAL("alice_4p", rGet.json["players"][0].value("handle", "").c_str());
    STRCMP_EQUAL(u1.uuid.c_str(), rGet.json["players"][0].value("player_uuid", "").c_str());

    CHECK_EQUAL(2, rGet.json["players"][1].value("seat", 0));
    STRCMP_EQUAL("bob_4p", rGet.json["players"][1].value("handle", "").c_str());

    CHECK_EQUAL(3, rGet.json["players"][2].value("seat", 0));
    STRCMP_EQUAL("carol_4p", rGet.json["players"][2].value("handle", "").c_str());

    CHECK_EQUAL(4, rGet.json["players"][3].value("seat", 0));
    STRCMP_EQUAL("dave_4p", rGet.json["players"][3].value("handle", "").c_str());

    // 3. Corrupted 4P transcripts rejected
    Json badWinner = jTr;
    badWinner["winner"] = "INVALID";
    EXPECT_STATUS(400, http().post("/api/v1/archive/upload", u1.token,
                                   Json{{"transcript", badWinner.dump()}}));

    Json badTurn = jTr;
    badTurn["plies"][0]["player"] = "B";
    EXPECT_STATUS(400, http().post("/api/v1/archive/upload", u1.token,
                                   Json{{"transcript", badTurn.dump()}}));
}

TEST(OctiHttpServerIntegrationTest, WebPlayerBotProfilesQuiescenceAndRulesValidation)
{
    Reply rUi = http().get("/");
    EXPECT_STATUS(200, rUi);

    // 1. Bot tuning & profiles
    CHECK_TRUE(rUi.body.find("DEFAULT_BOT_TUNING") != std::string::npos);
    CHECK_TRUE(rUi.body.find("BOT_PROFILES") != std::string::npos);
    CHECK_TRUE(rUi.body.find("AI_Casual") != std::string::npos);

    // 2. AI search engine functions
    CHECK_TRUE(rUi.body.find("dfsJumpChains") != std::string::npos);
    CHECK_TRUE(rUi.body.find("quiesce") != std::string::npos);
    CHECK_TRUE(rUi.body.find("alphaBeta") != std::string::npos);
    CHECK_TRUE(rUi.body.find("orderMoves") != std::string::npos);
    CHECK_TRUE(rUi.body.find("evaluateState") != std::string::npos);
    CHECK_TRUE(rUi.body.find("botSearch") != std::string::npos);

    // 3. Canonical 2P / 4P / OFK rulebook functions & helpers
    CHECK_TRUE(rUi.body.find("canLiberate") != std::string::npos);
    CHECK_TRUE(rUi.body.find("canPlaceReservePodOnSquare") != std::string::npos);
    CHECK_TRUE(rUi.body.find("canLiberateOnSquare") != std::string::npos);
    CHECK_TRUE(rUi.body.find("isSquareOccupiedBy") != std::string::npos);
    CHECK_TRUE(rUi.body.find("squareHasEnemyOf") != std::string::npos);
    CHECK_TRUE(rUi.body.find("elimMask") != std::string::npos);
    CHECK_TRUE(rUi.body.find("NEUTRAL_OCTI") != std::string::npos);

    // 4. Capture dialog & thinking overlays
    CHECK_TRUE(rUi.body.find("web-capture-overlay") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-capture-prompt-text") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-capture-yes") != std::string::npos);
    CHECK_TRUE(rUi.body.find("btn-capture-no") != std::string::npos);
    CHECK_TRUE(rUi.body.find("selectGuidedDestination") != std::string::npos);
    CHECK_TRUE(rUi.body.find("toggleCaptureAtSquare") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-thinking-overlay") != std::string::npos);
    CHECK_TRUE(rUi.body.find("web-thinking-progress-fill") != std::string::npos);

    // 5. Validate embedded JavaScript syntax via node --check
    size_t scStart = rUi.body.find("<script>");
    CHECK_TRUE(scStart != std::string::npos);
    size_t scEnd = rUi.body.find("</script>", scStart);
    CHECK_TRUE(scEnd != std::string::npos);
    std::string jsCode = rUi.body.substr(scStart + 8, scEnd - (scStart + 8));
    std::string jsTmpPath = "/tmp/octi_test_webui_" + std::to_string(getpid()) + ".js";
    {
        std::ofstream ofs(jsTmpPath);
        ofs << jsCode;
    }
    int rc = std::system(("node --check " + jsTmpPath + " >/dev/null 2>&1").c_str());
    std::remove(jsTmpPath.c_str());
    CHECK_EQUAL(0, rc);

    // 6. Execute WebPlayer JS Engine + WebMoveComposer + botSearch + generateOctiTranscriptV3
    //    via Node.js and verify the generated 2P & 4P transcripts pass server archive upload!
    size_t engStart = jsCode.find("var DIR_DELTAS = [");
    CHECK_TRUE(engStart != std::string::npos);
    size_t engEnd = jsCode.find("/* Inline Web Worker Initialization */", engStart);
    CHECK_TRUE(engEnd != std::string::npos);
    size_t compStart = jsCode.find("function sortedPodIndicesMostProngsFirst(stack)");
    CHECK_TRUE(compStart != std::string::npos);
    size_t compEnd = jsCode.find("/* --- Vector Reserve HUD Rendering --- */", compStart);
    CHECK_TRUE(compEnd != std::string::npos);

    std::string nodeHarnessPath = "/tmp/octi_test_harness_" + std::to_string(getpid()) + ".js";
    std::string out2pPath = "/tmp/octi_test_2p_" + std::to_string(getpid()) + ".json";
    std::string out4pPath = "/tmp/octi_test_4p_" + std::to_string(getpid()) + ".json";
    {
        std::ofstream hfs(nodeHarnessPath);
        hfs << "var document = { getElementById: function() { return { style: {}, textContent: '', addEventListener: function(){}, classList: { add: function(){}, remove: function(){}, contains: function(){ return false; } } }; }, querySelectorAll: function() { return []; } };\n";
        hfs << "var window = { addEventListener: function(){} };\n";
        hfs << "function $(id) { return null; }\n";
        hfs << "function requestAnimationFrame() {}\n";
        hfs << "var OctiWebSoundSynth = { play: function(){}, setEnabled: function(){}, isEnabled: function(){ return true; } };\n";
        hfs << "function renderPlayBoardCanvas() {}\n";
        hfs << "function renderAllWebReserves() {}\n";
        hfs << "var moveComposer = null;\n";
        hfs << jsCode.substr(engStart, engEnd - engStart) << "\n";
        hfs << jsCode.substr(compStart, compEnd - compStart) << "\n";
        hfs << R"NODEJS(
var assert = require("assert");
var fs = require("fs");

// Test 1: WebMoveComposer human moves advance turn, support reposition, jump capture toggle, and botSearch reply
var activeState = parseOctiFen("2:1:0:0:P:A:-:0:4,0,25,0:4,0,25,0:A33,A53,A73,B37,B57,B77");
var committedMoves = [];
moveComposer = new WebMoveComposer(function() { return activeState; }, function(not, newSt) {
  committedMoves.push(not);
  activeState = newSt;
});

// Stage & commit prong addition 33+a
assert.strictEqual(moveComposer.stageAddProng(3, 3, 0, 0, false), true);
assert.strictEqual(moveComposer.stagedNotation, "33+a");
assert.strictEqual(moveComposer.commitStagedMove(), true);
assert.strictEqual(activeState.turn, "B", "Turn must advance from A to B after human move commit");

// Bot search on B's turn & Level 2 tactical quality checks
assert.strictEqual(BOT_PROFILES[1].timeBudgetMs, 5000, "Level 1 time budget must be 5000ms");
assert.strictEqual(BOT_PROFILES[2].timeBudgetMs, 10000, "Level 2 time budget must be 10000ms");
assert.strictEqual(BOT_PROFILES[3].timeBudgetMs, 20000, "Level 3 time budget must be 20000ms");
assert.strictEqual(BOT_PROFILES[1].useOpeningBook, false, "Level 1 opening book must be disabled");
assert.strictEqual(BOT_PROFILES[2].useOpeningBook, true, "Level 2 opening book must be enabled");
assert.strictEqual(BOT_PROFILES[3].useOpeningBook, true, "Level 3 opening book must be enabled");
assert.ok(BOT_PROFILES[2].maxSearchDepth >= 6, "Level 2 maxSearchDepth must be >= 6");
var botRes = botSearch(serializeOctiFen(activeState), 1, "2P", "B");
assert.ok(botRes && botRes.bestMove, "botSearch must return a valid move for B");
assert.ok(/^[357]7\+[edf]$/.test(botRes.bestMove), "Bot B opening move must add a forward prong, got " + botRes.bestMove);
activeState = applyMoveToState(cloneState(activeState), botRes.bestMove, "B");
assert.strictEqual(activeState.turn, "A", "Turn must advance back to A after bot move");

// Tactical capture & hanging-pod avoidance at Level 2
var capRes = botSearch("2:3:0:0:P:A:-:0:4,0,22,0:4,0,22,0:A33a,A54a,A73a,B37e,B55ef,B77e", 2, "2P", "A", null, 800);
assert.strictEqual(capRes.bestMove, "54a-56x", "Level 2 bot must jump-capture hanging enemy pod at (5,5)");
var hangRes = botSearch("2:3:0:0:P:A:-:0:4,0,23,0:3,0,23,0:A33,A54a,A73,B37,B56e,B57,B77", 2, "2P", "A", null, 800);
assert.notStrictEqual(hangRes.bestMove, "54a-55", "Level 2 bot must not blunder sole armed pod into (5,5)");

// Test prong repositioning on A's pod at (3,3): 33a+b-a
var podIdxA = 0;
assert.strictEqual(moveComposer.stageRepositionProng(3, 3, podIdxA, 0, 1), true);
assert.strictEqual(moveComposer.stagedNotation, "33a+b-a");
assert.strictEqual(moveComposer.commitStagedMove(), true);
assert.strictEqual(activeState.turn, "B");

// Test guidedHops initialized and 2nd click does not throw
moveComposer.beginGuidedPodMove(3, 3, 0);
assert.strictEqual(Array.isArray(moveComposer.guidedHops), true);
assert.strictEqual(moveComposer.guidedHops.length, 0);
moveComposer.cancel();

// Test jump capture prompt & toggleCaptureAtSquare
var jumpSt = parseOctiFen("2:1:0:0:P:A:-:0:4,0,24,0:4,0,25,0:A55a,B56");
var jumpComp = new WebMoveComposer(function() { return jumpSt; }, function(not, ns) { jumpSt = ns; });
assert.strictEqual(jumpComp.beginGuidedPodMove(5, 5, 0), true);
assert.strictEqual(jumpComp.selectGuidedDestination(5, 7), true);
assert.strictEqual(jumpComp.stagedNotation, "55a-57x");
assert.strictEqual(jumpComp.toggleCaptureAtSquare(5, 6), true);
assert.strictEqual(jumpComp.stagedNotation, "55a-57");
assert.strictEqual(jumpComp.toggleCaptureAtSquare(5, 6), true);
assert.strictEqual(jumpComp.stagedNotation, "55a-57x");
assert.strictEqual(jumpComp.commitStagedMove(), true);
assert.strictEqual(jumpSt.status, "O");
assert.strictEqual(jumpSt.winner, "A");

// Build a full 2P 1-base victory transcript using JS engine and write to disk for C++ server validation
var s2p = parseOctiFen("2:1:0:0:P:A:-:0:4,0,25,0:4,0,25,0:A33,A53,A73,B37,B57,B77");
var moves2p = ["53+a", "57+g", "53a-54", "57g-47", "54a-55", "47g-37", "55a-56", "37+e", "56a-57"];
var plies2p = [];
for (var i = 0; i < moves2p.length; i++) {
  var p = s2p.turn;
  var leg2 = generateMoves(s2p, p, false);
  assert.ok(leg2.indexOf(moves2p[i]) >= 0, "Expected 2P move " + moves2p[i] + " to be in generateMoves");
  s2p = applyMoveToState(cloneState(s2p), moves2p[i], p);
  plies2p.push({ ply: i + 1, player: p, move: moves2p[i], elapsedMs: 120 });
}
assert.strictEqual(s2p.status, "O");
assert.strictEqual(s2p.winner, "A");
var tr2p = {
  format: "octi.transcript.v3",
  schema: "octi.transcript.v3",
  version: 3,
  game_id: "web-js-e2e-2p",
  mode: "2",
  bases: 1,
  superProngs: false,
  edgeless: false,
  startPlayer: "A",
  status: "GAME_OVER",
  termination: "BASE_CAPTURE",
  winner: s2p.winner,
  finalFen: serializeOctiFen(s2p),
  final_fen: serializeOctiFen(s2p),
  players: [
    { seat: 1, player: "A", handle: "Player_A", type: "HUMAN" },
    { seat: 2, player: "B", handle: "AI_Tactical", type: "AI", aiDifficulty: 2, aiTimeBudgetMs: 3000 }
  ],
  moves: moves2p,
  plies: plies2p,
  timestamp: Math.floor(Date.now() / 1000)
};
fs.writeFileSync(process.argv[2], JSON.stringify(tr2p));

// Build a full 4P victory transcript using JS engine and write to disk for C++ server validation
var s4p = parseOctiFen("4:3:0:0:P:A:-:0:3,0,12,0:3,0,12,0:3,0,12,0:3,0,12,0:A33,D73,B37,C77");
var moves4p = [
  "33+a", "37+h", "77+e", "73+b", "33a-34", "37h+f-h",
  "77e-76", "73b-84", "34a-35", "37f-26", "76e-75", "84b+g",
  "35a-36", "26f+a", "75e-74", "84bg+h", "36a-37", "74e-73"
];
var plies4p = [];
for (var j = 0; j < moves4p.length; j++) {
  var p4 = s4p.turn;
  var leg4 = generateMoves(s4p, p4, false);
  assert.ok(leg4.indexOf(moves4p[j]) >= 0, "Expected 4P move " + moves4p[j] + " to be in generateMoves");
  s4p = applyMoveToState(cloneState(s4p), moves4p[j], p4);
  plies4p.push({ ply: j + 1, player: p4, move: moves4p[j], elapsedMs: 150 });
}
assert.strictEqual(s4p.status, "O");
assert.strictEqual(s4p.winner, "A");
var tr4p = {
  format: "octi.transcript.v3",
  schema: "octi.transcript.v3",
  version: 3,
  game_id: "web-js-e2e-4p",
  mode: "4",
  bases: 3,
  superProngs: false,
  edgeless: false,
  startPlayer: "A",
  status: "GAME_OVER",
  termination: "BASE_CAPTURE",
  winner: s4p.winner,
  finalFen: serializeOctiFen(s4p),
  final_fen: serializeOctiFen(s4p),
  players: [
    { seat: 1, player: "A", handle: "Player_A", type: "HUMAN" },
    { seat: 2, player: "B", handle: "AI_Tactical", type: "AI", aiDifficulty: 2, aiTimeBudgetMs: 3000 },
    { seat: 3, player: "C", handle: "AI_Master", type: "AI", aiDifficulty: 2, aiTimeBudgetMs: 3000 },
    { seat: 4, player: "D", handle: "AI_Tactical", type: "AI", aiDifficulty: 2, aiTimeBudgetMs: 3000 }
  ],
  moves: moves4p,
  plies: plies4p,
  timestamp: Math.floor(Date.now() / 1000)
};
fs.writeFileSync(process.argv[3], JSON.stringify(tr4p));
if (typeof process !== "undefined" && typeof process.exit === "function") process.exit(0);
)NODEJS";
    }
    int rcRun = std::system(("node " + nodeHarnessPath + " " + out2pPath + " " + out4pPath).c_str());
    std::remove(nodeHarnessPath.c_str());
    CHECK_EQUAL(0, rcRun);

    // Upload both JS-generated transcripts to the C++ HTTP server to verify 100% FEN & move replay parity
    User uWeb = signIn("google", "sub-web-js-e2e", "WebPlayerTester");
    {
        std::ifstream ifs2(out2pPath);
        std::string tr2Str((std::istreambuf_iterator<char>(ifs2)), std::istreambuf_iterator<char>());
        std::remove(out2pPath.c_str());
        Reply rUp2 = http().post("/api/v1/archive/upload", uWeb.token, Json{{"transcript", tr2Str}});
        EXPECT_STATUS(200, rUp2);
    }
    {
        std::ifstream ifs4(out4pPath);
        std::string tr4Str((std::istreambuf_iterator<char>(ifs4)), std::istreambuf_iterator<char>());
        std::remove(out4pPath.c_str());
        Reply rUp4 = http().post("/api/v1/archive/upload", uWeb.token, Json{{"transcript", tr4Str}});
        EXPECT_STATUS(200, rUp4);
    }
}

TEST(OctiHttpServerIntegrationTest, DatabaseArchiveCorpusReplayAndInvalidGameRejection)
{
    User uCorpus = signIn("google", "sub-corpus-replay", "CorpusValidator");

    // 1. Validate all 19 real valid database games through C++ OctiBoard::deserializeGame
    //    and POST /api/v1/archive/upload
    const size_t numValid = sizeof(octi::test::kValidArchiveCorpusGames) /
                            sizeof(octi::test::kValidArchiveCorpusGames[0]);
    CHECK_EQUAL(19, static_cast<int>(numValid));

    for (size_t i = 0; i < numValid; ++i) {
        const auto &entry = octi::test::kValidArchiveCorpusGames[i];

        octi::OctiBoard replayBoard;
        replayBoard.deserializeGame(entry.transcriptJson);
        CHECK_TRUE(replayBoard.getGame() != nullptr);
        CHECK_TRUE(replayBoard.getGame()->isGameOver());
        STRCMP_EQUAL(entry.finalFen, replayBoard.toPositionString().c_str());
        STRCMP_EQUAL(entry.gameId, replayBoard.computeCanonicalGameId().c_str());

        Reply rUp = http().post("/api/v1/archive/upload", uCorpus.token,
                                Json{{"transcript", std::string(entry.transcriptJson)}});
        EXPECT_STATUS(200, rUp);
        STRCMP_EQUAL(entry.gameId, rUp.json.value("game_id", "").c_str());

        Reply rGet = http().get(std::string("/api/v1/archive/games/") + entry.gameId);
        EXPECT_STATUS(200, rGet);
        STRCMP_EQUAL(entry.expectedWinner, rGet.json.value("winner", "").c_str());
        STRCMP_EQUAL(entry.finalFen, rGet.json.value("final_fen", "").c_str());
        if (std::string(entry.expectedRuleTermination) == "TOTAL_CAPTURE") {
            STRCMP_EQUAL("TOTAL_CAPTURE", rGet.json.value("termination", "").c_str());
        }
    }

    // 2. Verify all 8 invalid database games (7 premature TOTAL_CAPTURE while freePods > 0
    //    and 1 corrupted reserve/captured pod liberation game) are rejected by the server
    const size_t numInvalid = sizeof(octi::test::kInvalidArchiveCorpusGames) /
                              sizeof(octi::test::kInvalidArchiveCorpusGames[0]);
    CHECK_EQUAL(8, static_cast<int>(numInvalid));

    for (size_t i = 0; i < numInvalid; ++i) {
        const auto &entry = octi::test::kInvalidArchiveCorpusGames[i];
        Reply rBad = http().post("/api/v1/archive/upload", uCorpus.token,
                                 Json{{"transcript", std::string(entry.transcriptJson)}});
        EXPECT_STATUS(400, rBad);
        STRCMP_EQUAL(entry.expectedErrorCode, rBad.json.value("error", "").c_str());
    }

    // 3. Replay all 19 valid games and all 8 invalid games through the Web JS engine
    //    (WebUi.cxx) in Node.js to verify 100% host-side C++/JS parity
    Reply rUi = http().get("/");
    EXPECT_STATUS(200, rUi);
    size_t scStart = rUi.body.find("<script>");
    CHECK_TRUE(scStart != std::string::npos);
    size_t scEnd = rUi.body.find("</script>", scStart);
    CHECK_TRUE(scEnd != std::string::npos);
    std::string jsCode = rUi.body.substr(scStart + 8, scEnd - (scStart + 8));
    size_t engStart = jsCode.find("var DIR_DELTAS = [");
    CHECK_TRUE(engStart != std::string::npos);
    size_t engEnd = jsCode.find("/* Inline Web Worker Initialization */", engStart);
    CHECK_TRUE(engEnd != std::string::npos);

    Json corpusPayload;
    corpusPayload["valid"] = Json::array();
    for (size_t i = 0; i < numValid; ++i) {
        const auto &entry = octi::test::kValidArchiveCorpusGames[i];
        corpusPayload["valid"].push_back(Json{
            {"gameId", entry.gameId},
            {"expectedWinner", entry.expectedWinner},
            {"expectedRuleTermination", entry.expectedRuleTermination},
            {"finalFen", entry.finalFen},
            {"transcript", Json::parse(entry.transcriptJson)}
        });
    }
    corpusPayload["invalid"] = Json::array();
    for (size_t i = 0; i < numInvalid; ++i) {
        const auto &entry = octi::test::kInvalidArchiveCorpusGames[i];
        corpusPayload["invalid"].push_back(Json{
            {"gameId", entry.gameId},
            {"expectedErrorCode", entry.expectedErrorCode},
            {"finalFen", entry.finalFen},
            {"transcript", Json::parse(entry.transcriptJson)}
        });
    }

    std::string corpusJsonPath = "/tmp/octi_corpus_payload_" + std::to_string(getpid()) + ".json";
    std::string corpusHarnessPath = "/tmp/octi_corpus_harness_" + std::to_string(getpid()) + ".js";
    {
        std::ofstream cfs(corpusJsonPath);
        cfs << corpusPayload.dump();
    }
    {
        std::ofstream hfs(corpusHarnessPath);
        hfs << "var document = { getElementById: function() { return { style: {}, textContent: '', addEventListener: function(){}, classList: { add: function(){}, remove: function(){}, contains: function(){ return false; } } }; }, querySelectorAll: function() { return []; } };\n";
        hfs << "var window = { addEventListener: function(){} };\n";
        hfs << "function $(id) { return null; }\n";
        hfs << "function requestAnimationFrame() {}\n";
        hfs << jsCode.substr(engStart, engEnd - engStart) << "\n";
        hfs << R"NODEJS(
var assert = require("assert");
var fs = require("fs");

function makeInitialFen(mode, bases, sp, ed, startP) {
  var spBit = sp ? "1" : "0";
  var edBit = ed ? "1" : "0";
  var p = startP || "A";
  if (mode === "OFK") {
    return "OFK:1:0:0:P:" + p + ":-:0:0,0,12,0:0,0,12,0:A22,A32,A42,A52,B26,B36,B46,B56";
  }
  if (mode === "4" || mode === "4P") {
    var r4p = "3,0," + (sp ? "11,1" : "12,0");
    return "4:3:" + spBit + ":" + edBit + ":P:" + p + ":-:0:" + r4p + ":" + r4p + ":" + r4p + ":" + r4p + ":A33,D73,B37,C77";
  }
  var r2p = "4,0," + (sp ? "24,1" : "25,0");
  return "2:" + bases + ":" + spBit + ":" + edBit + ":P:" + p + ":-:0:" + r2p + ":" + r2p + ":A33,A53,A73,B37,B57,B77";
}

var payload = JSON.parse(fs.readFileSync(process.argv[2], "utf8"));
assert.strictEqual(payload.valid.length, 19);
assert.strictEqual(payload.invalid.length, 8);

for (var i = 0; i < payload.valid.length; i++) {
  var item = payload.valid[i];
  var t = item.transcript;
  var st = parseOctiFen(makeInitialFen(t.mode, t.bases, !!t.superProngs, !!t.edgeless, t.startPlayer || "A"));
  for (var pIdx = 0; pIdx < t.plies.length; pIdx++) {
    assert.strictEqual(st.status, "P", "Game " + item.gameId + " prematurely ended before ply " + (pIdx + 1));
    var ply = t.plies[pIdx];
    assert.strictEqual(st.turn, ply.player, "Turn mismatch in " + item.gameId + " at ply " + (pIdx + 1));
    if (ply.move.indexOf(",") === -1) {
      var legal = generateMoves(st, st.turn, false);
      assert.ok(legal.indexOf(ply.move) >= 0, "Move " + ply.move + " not in generateMoves for " + item.gameId + " at ply " + (pIdx + 1));
    }
    st = applyMoveToState(st, ply.move, st.turn);
  }
  assert.strictEqual(st.status, "O", "Valid game " + item.gameId + " did not reach game over");
  assert.strictEqual(st.winner, item.expectedWinner, "Winner mismatch for " + item.gameId);
  assert.strictEqual(st.termination, item.expectedRuleTermination, "Termination mismatch for " + item.gameId);
  assert.strictEqual(serializeOctiFen(st), item.finalFen, "Final FEN mismatch for " + item.gameId);
}

for (var j = 0; j < payload.invalid.length; j++) {
  var bad = payload.invalid[j];
  var bt = bad.transcript;
  var bst = parseOctiFen(makeInitialFen(bt.mode, bt.bases, !!bt.superProngs, !!bt.edgeless, bt.startPlayer || "A"));
  for (var bpIdx = 0; bpIdx < bt.plies.length; bpIdx++) {
    var bply = bt.plies[bpIdx];
    bst = applyMoveToState(bst, bply.move, bst.turn);
  }
  if (bad.expectedErrorCode === "GAME_NOT_COMPLETED") {
    assert.strictEqual(bst.status, "P", "Prematurely terminated game " + bad.gameId + " must still be in progress (freePods > 0)");
    var remLegal = generateMoves(bst, bst.turn, false);
    assert.ok(remLegal.length > 0, "Side with reserve pods in " + bad.gameId + " must have legal reserve placement moves");
  } else {
    assert.notStrictEqual(serializeOctiFen(bst), bad.finalFen, "Corrupted FEN game " + bad.gameId + " must not match claimed finalFen");
  }
}

// Specific regression check on game 130ab4abef6c9a83... ply 34 & 35
var g27 = payload.invalid.find(function(x) { return x.gameId.indexOf("130ab4ab") === 0; });
assert.ok(g27, "Must find game 130ab4ab in invalid corpus");
var st33 = parseOctiFen(makeInitialFen("2P", 3, false, false, "A"));
for (var k = 0; k < 33; k++) {
  st33 = applyMoveToState(st33, g27.transcript.plies[k].move, st33.turn);
}
// After ply 34 blunder (55adefg-54), 53a-55x must rank #1 in orderMoves and not end the game
var st34 = applyMoveToState(cloneState(st33), "55adefg-54", "B");
var prof2 = BOT_PROFILES[2];
var ordA = orderMoves(generateMoves(st34, "A", false), st34, "A", prof2, null);
assert.strictEqual(ordA[0], "53a-55x", "Capturing jump 53a-55x must be ranked #1 in orderMoves");
var st35 = applyMoveToState(cloneState(st34), "53a-55x", "A");
assert.strictEqual(st35.status, "P", "After 53a-55x, game must continue since B has 4 freePods");
assert.strictEqual(st35.turn, "B");
assert.strictEqual(st35.reserves[1].freePods, 4);
)NODEJS";
    }
    int rcCorpus = std::system(("node " + corpusHarnessPath + " " + corpusJsonPath).c_str());
    std::remove(corpusHarnessPath.c_str());
    std::remove(corpusJsonPath.c_str());
    CHECK_EQUAL(0, rcCorpus);
}

TEST(OctiHttpServerIntegrationTest, WebBotEngineThreeVariantsAndRealGameTactics)
{
    Reply rUi = http().get("/");
    EXPECT_STATUS(200, rUi);
    size_t scStart = rUi.body.find("<script>");
    CHECK_TRUE(scStart != std::string::npos);
    size_t scEnd = rUi.body.find("</script>", scStart);
    CHECK_TRUE(scEnd != std::string::npos);
    std::string jsCode = rUi.body.substr(scStart + 8, scEnd - (scStart + 8));
    size_t engStart = jsCode.find("var DIR_DELTAS = [");
    CHECK_TRUE(engStart != std::string::npos);
    size_t engEnd = jsCode.find("/* Inline Web Worker Initialization */", engStart);
    CHECK_TRUE(engEnd != std::string::npos);

    std::string testHarnessPath = "/tmp/octi_bot_harness_" + std::to_string(getpid()) + ".js";
    {
        std::ofstream hfs(testHarnessPath);
        hfs << "var document = { getElementById: function() { return { style: {}, textContent: '', addEventListener: function(){}, classList: { add: function(){}, remove: function(){}, contains: function(){ return false; } } }; }, querySelectorAll: function() { return []; } };\n";
        hfs << "var window = { addEventListener: function(){} };\n";
        hfs << "function $(id) { return null; }\n";
        hfs << "function requestAnimationFrame() {}\n";
        hfs << "var URL = { createObjectURL: function() { return ''; } };\n";
        hfs << "function Worker() {}\n";
        hfs << jsCode.substr(engStart, engEnd - engStart) << "\n";
        hfs << R"NODEJS(
var assert = require("assert");

// 1. Ply 74 of game 3c8ded8792...:
// Jump capture 77defh-55-33x (capturing A44b and landing on A's base 33) exceeds passive 66abcdefg-65
var fenPly74 = "2:3:0:0:P:B:-:0:2,3,23,0:0,5,19,0:A44b,A64bh,A35abefgh,B66abcdefg,B77defh";
var stCap = applyMoveToState(parseOctiFen(fenPly74), "77defh-55-33x", "B");
var stPass = applyMoveToState(parseOctiFen(fenPly74), "66abcdefg-65", "B");
var evalCap = evaluateState(stCap, "B");
var evalPass = evaluateState(stPass, "B");
assert.ok(evalCap > evalPass, "Ply 74 evalCap must exceed evalPass");

// 2. Ply 92: B must capture via jump-capture rather than stepping away
var fenPly92 = "2:3:0:0:P:B:-:0:1,5,21,0:0,5,19,0:A35abefgh,A76h,B66abcdefg,B77cdef";
var res92 = botSearch(fenPly92, 2, "2P", "B", null, 1500);
assert.ok(res92.bestMove && res92.bestMove.indexOf("x") !== -1, "Ply 92 must choose capturing move");
assert.notStrictEqual(res92.bestMove, "66abcdefg-65", "Ply 92 must not step away to 66abcdefg-65");

// 3. Ply 98: B has 1 pod on board, 0 freePods, 3-base game (base win mathematically impossible).
// B must capture A55h via 65acdefg-45x rather than chasing empty base distance via 65acdefg-54
var fenPly98 = "2:3:0:0:P:B:-:0:0,5,21,0:0,6,24,0:B65acdefg,A55h,A35abefgh";
var res98 = botSearch(fenPly98, 2, "2P", "B", null, 1500);
assert.ok(res98.bestMove && res98.bestMove.startsWith("65acdefg-45x"), "Ply 98 must start with 65acdefg-45x");
assert.notStrictEqual(res98.bestMove, "65acdefg-54", "Ply 98 must not chase base distance");

// 4. Per-pod base-threat deduplication & 3-base vs 1-base scaling
var fen1 = "2:1:0:0:P:A:-:0:3,0,20,0:3,0,20,0:A66bh,B37";
var fen3 = "2:3:0:0:P:A:-:0:3,0,20,0:3,0,20,0:A66bh,B37";
var e1 = evaluateState(parseOctiFen(fen1), "A");
var e3 = evaluateState(parseOctiFen(fen3), "A");
assert.ok(e1 > e3 + 4000, "1-base eval must exceed 3-base eval by > 4000");

// 5. 4P (2v2 coalition) single-seat elimination defense penalty
var fenThreatened = "4:2:0:0:P:B:-:0:3,0,10,0:3,0,10,0:0,3,10,0:3,0,10,0:A22,C44,B43a,D68";
var fenSafe = "4:2:0:0:P:B:-:0:3,0,10,0:3,0,10,0:1,2,10,0:3,0,10,0:A22,C44,B43a,D68";
var scoreThreatened = evaluateState(parseOctiFen(fenThreatened), "A");
var scoreWithReserve = evaluateState(parseOctiFen(fenSafe), "A");
assert.ok(scoreWithReserve > scoreThreatened + 5000, "Safe score must exceed threatened score by > 5000");

// 6. OFK (7x7 OCTI for Kids) capture prioritization with zero reserves
var fenOfk = "OFK:1:0:0:P:A:-:0:0,0,8,0:0,0,8,0:A33a,A22,B34e,B56";
var resOfk = botSearch(fenOfk, 2, "OFK", "A", null, 1000);
assert.strictEqual(resOfk.bestMove, "33a-35x", "OFK must choose winning capture 33a-35x");
)NODEJS";
    }
    int rcBot = std::system(("node " + testHarnessPath).c_str());
    std::remove(testHarnessPath.c_str());
    CHECK_EQUAL(0, rcBot);
}

TEST(OctiHttpServerIntegrationTest, LobbyConnectedUsersMultiClientCollapseAndIdleTracking)
{
    int64_t fakeNow = 100000;
    env->rooms->setClock([&fakeNow]() { return fakeNow; });

    User alice = signIn("google", "sub-presence-alice", "PresenceAlice");
    std::string tokenAlice = alice.token;
    CHECK_FALSE(tokenAlice.empty());

    // 1. With header {"X-Octi-Client-Type", "web"}
    httplib::Headers hWeb = {{"X-Octi-Client-Type", "web"}};
    Reply r1 = http().get("/api/v1/lobby/state", tokenAlice, hWeb);
    EXPECT_STATUS(200, r1);

    // 2. With header {"X-Octi-Client-Type", "android"}
    httplib::Headers hAndroid = {{"X-Octi-Client-Type", "android"}};
    Reply r2 = http().get("/api/v1/lobby/state", tokenAlice, hAndroid);
    EXPECT_STATUS(200, r2);

    // 3. With header {"User-Agent", "octi-desktop/2.2.7"} (testing UA fallback when X-Octi-Client-Type is omitted)
    httplib::Headers hDesktop = {{"User-Agent", "octi-desktop/2.2.7"}};
    Reply r3 = http().get("/api/v1/lobby/state", tokenAlice, hDesktop);
    EXPECT_STATUS(200, r3);

    // Inspect the third GET /api/v1/lobby/state response JSON j["players"]:
    const Json &players = r3.json["players"];
    CHECK_TRUE(players.is_array());
    int aliceCount = 0;
    Json aliceObj;
    for (size_t i = 0; i < players.size(); ++i) {
        if (players[i].value("handle", "") == "PresenceAlice") {
            aliceCount++;
            aliceObj = players[i];
        }
    }
    // Verify "PresenceAlice" appears only once in j["players"] (collapsed by user).
    CHECK_EQUAL(1, aliceCount);

    // Verify player object fields:
    STRCMP_EQUAL("PresenceAlice", aliceObj.value("handle", "").c_str());
    CHECK_TRUE(aliceObj.contains("avatar_url") && aliceObj["avatar_url"].is_string());
    CHECK_TRUE(aliceObj.contains("clients") && aliceObj["clients"].is_array());
    CHECK_EQUAL(3, static_cast<int>(aliceObj["clients"].size()));
    STRCMP_EQUAL("android", aliceObj["clients"][0].get<std::string>().c_str());
    STRCMP_EQUAL("desktop", aliceObj["clients"][1].get<std::string>().c_str());
    STRCMP_EQUAL("web", aliceObj["clients"][2].get<std::string>().c_str());
    CHECK_TRUE(aliceObj.contains("online_sec") && aliceObj["online_sec"].is_number_integer());
    CHECK_TRUE(aliceObj["online_sec"].get<int64_t>() >= 0);
    CHECK_TRUE(aliceObj.contains("idle_sec") && aliceObj["idle_sec"].is_number_integer());
    CHECK_TRUE(aliceObj["idle_sec"].get<int64_t>() >= 0);

    // Advance simulated time to 115000 (+15 seconds) and do passive polls:
    fakeNow = 115000;
    http().get("/api/v1/lobby/state", tokenAlice, hWeb);
    http().get("/api/v1/lobby/state", tokenAlice, hAndroid);
    Reply rPoll = http().get("/api/v1/lobby/state", tokenAlice, hDesktop);
    EXPECT_STATUS(200, rPoll);
    Json alicePoll;
    for (size_t i = 0; i < rPoll.json["players"].size(); ++i) {
        if (rPoll.json["players"][i].value("handle", "") == "PresenceAlice") {
            alicePoll = rPoll.json["players"][i];
            break;
        }
    }
    CHECK_EQUAL(15, alicePoll.value("online_sec", -1));
    CHECK_EQUAL(15, alicePoll.value("idle_sec", -1));

    // Advance simulated time to 122000 (+22 seconds) and post an interactive lobby chat:
    fakeNow = 122000;
    httplib::Headers hChatDesktop = {{"X-Octi-Client-Type", "desktop"}};
    Reply rChat = http().post("/api/v1/lobby/chat", tokenAlice,
                              Json{{"text", "Hello from desktop"}}, hChatDesktop);
    EXPECT_STATUS(200, rChat);

    Reply rLobbyAfterChat = http().get("/api/v1/lobby/state", tokenAlice, hDesktop);
    EXPECT_STATUS(200, rLobbyAfterChat);
    Json aliceAfterChat;
    for (size_t i = 0; i < rLobbyAfterChat.json["players"].size(); ++i) {
        if (rLobbyAfterChat.json["players"][i].value("handle", "") == "PresenceAlice") {
            aliceAfterChat = rLobbyAfterChat.json["players"][i];
            break;
        }
    }
    CHECK_EQUAL(22, aliceAfterChat.value("online_sec", -1));
    CHECK_EQUAL(0, aliceAfterChat.value("idle_sec", -1));

    // Test single-client logout:
    Reply rSsoWeb = http().post(
        "/api/v1/auth/sso", "",
        Json{{"provider", "google"},
             {"id_token", makeToken("google", "sub-presence-alice", "PresenceAlice")},
             {"client_agent", "octi-test/1.0"}},
        hWeb);
    CHECK_TEXT(rSsoWeb.status == 200 || rSsoWeb.status == 201,
               (rSsoWeb.error + rSsoWeb.body).c_str());
    std::string tokWeb = rSsoWeb.json.value("session_token", "");
    CHECK_FALSE(tokWeb.empty());

    Reply rLogout = http().post("/api/v1/auth/logout", tokWeb, Json::object(), hWeb);
    EXPECT_STATUS(200, rLogout);

    Reply rLobbyAfterLogout = http().get("/api/v1/lobby/state", tokenAlice, hDesktop);
    EXPECT_STATUS(200, rLobbyAfterLogout);
    bool foundAlice = false;
    Json aliceAfterLogout;
    for (size_t i = 0; i < rLobbyAfterLogout.json["players"].size(); ++i) {
        if (rLobbyAfterLogout.json["players"][i].value("handle", "") == "PresenceAlice") {
            foundAlice = true;
            aliceAfterLogout = rLobbyAfterLogout.json["players"][i];
            break;
        }
    }
    CHECK_TRUE(foundAlice);
    CHECK_TRUE(aliceAfterLogout.contains("clients") && aliceAfterLogout["clients"].is_array());
    CHECK_EQUAL(2, static_cast<int>(aliceAfterLogout["clients"].size()));
    STRCMP_EQUAL("android", aliceAfterLogout["clients"][0].get<std::string>().c_str());
    STRCMP_EQUAL("desktop", aliceAfterLogout["clients"][1].get<std::string>().c_str());
}

TEST(OctiHttpServerIntegrationTest, RobotConnectDisconnectAndLeaseSuperseding)
{
    User u = signIn("google", "sub-robot-proto-user", "RobotMaster");

    // Create a robot
    Reply rCreate = http().post(
        "/api/v1/robots", u.token,
        Json{{"robot_name", "deep-octi"}, {"description", "Neural OCTI bot"}});
    EXPECT_STATUS(201, rCreate);
    std::string botToken = rCreate.json.value("robot_token", "");
    std::string botUuid = rCreate.json.value("robot_uuid", "");
    CHECK_FALSE(botToken.empty());
    CHECK_FALSE(botUuid.empty());

    // Connect without token -> 401
    Reply rNoToken = http().post("/api/v1/robots/connect", "", Json::object());
    CHECK_EQUAL(401, rNoToken.status);

    // Connect with bad token -> 401
    Reply rBadToken = http().post("/api/v1/robots/connect", "octi_bot_bogus", Json::object());
    CHECK_EQUAL(401, rBadToken.status);

    // Initial connect with Bearer auth
    Reply rConn1 = http().post("/api/v1/robots/connect", botToken, Json::object());
    EXPECT_STATUS(200, rConn1);
    CHECK_TRUE(rConn1.json.value("ok", false));
    std::string lease1 = rConn1.json.value("lease_id", "");
    CHECK_EQUAL(32, static_cast<int>(lease1.size()));
    STRCMP_EQUAL("deep-octi", rConn1.json.value("robot_name", "").c_str());
    CHECK_FALSE(rConn1.json.value("in_game", true));

    // Connect with if_unused: true while active -> 409 ROBOT_TOKEN_IN_USE
    Reply rInUse = http().post("/api/v1/robots/connect", botToken, Json{{"if_unused", true}});
    CHECK_EQUAL(409, rInUse.status);
    STRCMP_EQUAL("ROBOT_TOKEN_IN_USE", rInUse.json.value("error", "").c_str());

    // Connect again without if_unused mints a superseding lease
    Reply rConn2 = http().post("/api/v1/robots/connect", botToken, Json{{"if_unused", false}});
    EXPECT_STATUS(200, rConn2);
    std::string lease2 = rConn2.json.value("lease_id", "");
    CHECK_EQUAL(32, static_cast<int>(lease2.size()));
    CHECK_TRUE(lease1 != lease2);

    // Request with old lease header is rejected with 409 ROBOT_TOKEN_SUPERSEDED
    httplib::Headers hOld = {{"X-Octi-Robot-Lease", lease1}};
    Reply rOld = http().get("/api/v1/lobby/state", botToken, hOld);
    CHECK_EQUAL(409, rOld.status);
    STRCMP_EQUAL("ROBOT_TOKEN_SUPERSEDED", rOld.json.value("error", "").c_str());

    // Request with new active lease header succeeds
    httplib::Headers hNew = {{"X-Octi-Robot-Lease", lease2}};
    Reply rNew = http().get("/api/v1/lobby/state", botToken, hNew);
    EXPECT_STATUS(200, rNew);

    // Disconnect robot
    Reply rDisc = http().post("/api/v1/robots/disconnect", botToken, Json::object(), hNew);
    EXPECT_STATUS(200, rDisc);
    CHECK_TRUE(rDisc.json.value("ok", false));
}

TEST(OctiHttpServerIntegrationTest, RobotExportConfAndForbiddenEndpoints)
{
    User u = signIn("google", "sub-export-proto-user", "ExportMaster");

    Reply r1 = http().post(
        "/api/v1/robots", u.token,
        Json{{"robot_name", "export-bot-1"}, {"description", "First bot"}});
    EXPECT_STATUS(201, r1);
    std::string tok1 = r1.json.value("robot_token", "");

    Reply r2 = http().post(
        "/api/v1/robots", u.token,
        Json{{"robot_name", "export-bot-2"}, {"description", "Second bot"}});
    EXPECT_STATUS(201, r2);
    std::string tok2 = r2.json.value("robot_token", "");

    // Human downloads export-conf
    Reply rExport = http().get("/api/v1/robots/export-conf", u.token);
    EXPECT_STATUS(200, rExport);
    CHECK_TRUE(rExport.body.find("export-bot-1=" + tok1) != std::string::npos);
    CHECK_TRUE(rExport.body.find("export-bot-2=" + tok2) != std::string::npos);

    // Robot cannot call export-conf -> 403 FORBIDDEN_ROBOT
    Reply rBotExport = http().get("/api/v1/robots/export-conf", tok1);
    CHECK_EQUAL(403, rBotExport.status);
    STRCMP_EQUAL("FORBIDDEN_ROBOT", rBotExport.json.value("error", "").c_str());

    // Robot calling forbidden endpoints:
    Reply rAuth = http().get("/api/v1/auth/identities", tok1);
    CHECK_EQUAL(403, rAuth.status);
    STRCMP_EQUAL("FORBIDDEN_ROBOT", rAuth.json.value("error", "").c_str());

    Reply rRobotMgmt = http().post(
        "/api/v1/robots", tok1,
        Json{{"robot_name", "sub-bot"}, {"description", "Illegal child bot"}});
    CHECK_EQUAL(403, rRobotMgmt.status);
    STRCMP_EQUAL("FORBIDDEN_ROBOT", rRobotMgmt.json.value("error", "").c_str());

    Reply rProfilePatch = http().patch(
        "/api/v1/profile", tok1, Json{{"display_name", "Robot Impostor"}});
    CHECK_EQUAL(403, rProfilePatch.status);
    STRCMP_EQUAL("FORBIDDEN_ROBOT", rProfilePatch.json.value("error", "").c_str());
}

TEST(OctiHttpServerIntegrationTest, RobotGamePlayAndMoveAiTelemetry)
{
    User u = signIn("google", "sub-telemetry-user", "TelemetryHuman");

    Reply rBot = http().post(
        "/api/v1/robots", u.token,
        Json{{"robot_name", "ai-search-bot"}, {"description", "Bot with search telemetry"}});
    EXPECT_STATUS(201, rBot);
    std::string botToken = rBot.json.value("robot_token", "");

    // Human creates table with allow_robots: true
    Reply rTable = http().post(
        "/api/v1/tables", u.token,
        Json{{"mode", "2P"}, {"bases", 3}, {"allow_robots", true}});
    EXPECT_STATUS(201, rTable);
    std::string roomId = rTable.json.value("room_id", "");
    CHECK_FALSE(roomId.empty());

    // Robot joins table seat 2
    Reply rJoin = http().post(
        "/api/v1/tables/" + roomId + "/join", botToken, Json{{"seat", 2}});
    EXPECT_STATUS(200, rJoin);
    CHECK_TRUE(rJoin.json.value("game_started", false));

    // Robot connect now reports active_room_id
    Reply rConn = http().post("/api/v1/robots/connect", botToken, Json::object());
    EXPECT_STATUS(200, rConn);
    STRCMP_EQUAL(roomId.c_str(), rConn.json.value("active_room_id", "").c_str());
    CHECK_TRUE(rConn.json.value("in_game", false));

    // Human submits move 1
    Reply rM1 = http().post(
        "/api/v1/games/" + roomId + "/move", u.token, Json{{"move", "73+a"}});
    EXPECT_STATUS(200, rM1);

    // Robot submits move 2 with AI telemetry
    Json aiData = {
        {"depth", 6},
        {"nodes", 48210},
        {"score", 35},
        {"elapsed_ms", 125}
    };
    Reply rM2 = http().post(
        "/api/v1/games/" + roomId + "/move", botToken,
        Json{{"move", "77+e"}, {"ai", aiData}});
    EXPECT_STATUS(200, rM2);
    CHECK_TRUE(rM2.json.contains("move"));
    STRCMP_EQUAL("77+e", rM2.json.value("move", "").c_str());

    // Verify room view reflects moves
    Reply rView = http().get("/api/v1/games/" + roomId, u.token);
    EXPECT_STATUS(200, rView);
    CHECK_EQUAL(2, rView.json.value("ply_count", 0));
    CHECK_TRUE(rView.json["seats"][1].value("is_robot", false));
    STRCMP_EQUAL("ai-search-bot", rView.json["seats"][1].value("handle", "").c_str());
}

TEST(OctiHttpServerIntegrationTest, AdminIdentityAndPurgeEndpoints)
{
    // Configure admin Google ID
    env->store.setAdminGoogleIdentities({"sub-admin-google", "admin@selfso.com"});

    User normalUser = signIn("google", "sub-normal-user", "Normal User");
    User adminUser = signIn("google", "sub-admin-google", "Admin User");
    User otherUser = signIn("apple", "sub-other-normal-user", "Other Normal User");

    // GET /api/v1/profile/me for normal user has is_admin == false
    Reply rNormalMe = http().get("/api/v1/profile/me", normalUser.token);
    EXPECT_STATUS(200, rNormalMe);
    CHECK_FALSE(rNormalMe.json.value("is_admin", false));

    // GET /api/v1/profile/me for admin user has is_admin == true
    Reply rAdminMe = http().get("/api/v1/profile/me", adminUser.token);
    EXPECT_STATUS(200, rAdminMe);
    CHECK_TRUE(rAdminMe.json.value("is_admin", false));

    // Create a table and play game to completion so it's archived
    std::vector<User> pair = {normalUser, adminUser};
    std::string room = startGame(pair);

    // Move-inactivity countdown in GET /api/v1/tables and GET /api/v1/games/:id
    Reply rTables = http().get("/api/v1/tables", normalUser.token);
    EXPECT_STATUS(200, rTables);
    CHECK_TRUE(rTables.json.contains("tables"));
    CHECK_TRUE(rTables.json["tables"].size() >= 1);
    CHECK_TRUE(rTables.json["tables"][0].contains("last_move_at_ms"));
    CHECK_TRUE(rTables.json["tables"][0].contains("idle_ms"));
    CHECK_TRUE(rTables.json["tables"][0].contains("ttl_remaining_ms"));

    Reply rGame = http().get("/api/v1/games/" + room, normalUser.token);
    EXPECT_STATUS(200, rGame);
    CHECK_TRUE(rGame.json.contains("last_move_at_ms"));
    CHECK_TRUE(rGame.json.contains("idle_ms"));
    CHECK_TRUE(rGame.json.contains("ttl_remaining_ms"));

    // Non-admin, non-host trying to purge active table gets 403 Forbidden
    Reply rFailTablePurge = http().del("/api/v1/tables/" + room, otherUser.token);
    EXPECT_STATUS(403, rFailTablePurge);

    // Admin purging active table gets 200 OK
    Reply rOkTablePurge = http().del("/api/v1/tables/" + room, adminUser.token);
    EXPECT_STATUS(200, rOkTablePurge);
    STRCMP_EQUAL("purged", rOkTablePurge.json.value("status", "").c_str());

    // Purging again gets 404 NOT_FOUND
    Reply rNotFoundTable = http().del("/api/v1/tables/" + room, adminUser.token);
    EXPECT_STATUS(404, rNotFoundTable);

    // Now test archive purge:
    // Play a game to completion so an archived game exists
    std::string archivedGameId = playDrawGame(normalUser, adminUser);
    Reply rArchivedBefore = http().get("/api/v1/archive/games/" + archivedGameId);
    EXPECT_STATUS(200, rArchivedBefore);

    // Non-admin trying to purge archived game gets 403 Forbidden
    Reply rFailArchivePurge =
        http().del("/api/v1/archive/games/" + archivedGameId, normalUser.token);
    EXPECT_STATUS(403, rFailArchivePurge);

    // Admin purging archived game gets 200 OK
    Reply rOkArchivePurge =
        http().del("/api/v1/archive/games/" + archivedGameId, adminUser.token);
    EXPECT_STATUS(200, rOkArchivePurge);
    STRCMP_EQUAL("purged", rOkArchivePurge.json.value("status", "").c_str());

    // Purged game is gone (404)
    Reply rArchivedAfter = http().get("/api/v1/archive/games/" + archivedGameId);
    EXPECT_STATUS(404, rArchivedAfter);

    // Purging again gets 404
    Reply rArchivePurgeAgain =
        http().del("/api/v1/archive/games/" + archivedGameId, adminUser.token);
    EXPECT_STATUS(404, rArchivePurgeAgain);
}

TEST(OctiHttpServerIntegrationTest, RobotsDirectoryAndProfileEndpoints)
{
    User u = signIn("google", "sub-robot-dir-owner", "RobotDirOwner");

    Reply rBot = http().post(
        "/api/v1/robots", u.token,
        Json{{"robot_name", "dir-alpha-bot"}, {"description", "Alpha bot for directory testing"}});
    EXPECT_STATUS(201, rBot);
    std::string botUuid = rBot.json.value("robot_uuid", "");
    std::string botToken = rBot.json.value("robot_token", "");
    CHECK_FALSE(botUuid.empty());
    CHECK_FALSE(botToken.empty());

    /* Robot with 0 games played is NOT in the public directory */
    Reply rDirEmpty = http().get("/api/v1/robots/directory");
    EXPECT_STATUS(200, rDirEmpty);
    CHECK_EQUAL(0, rDirEmpty.json.value("total", -1));

    /* Robot profile returns 404 when games_played == 0 */
    Reply rProf404 = http().get("/api/v1/robots/profile/" + botUuid);
    EXPECT_STATUS(404, rProf404);

    /* Human creates table with allow_robots: true and robot plays */
    Reply rTable = http().post(
        "/api/v1/tables", u.token,
        Json{{"mode", "2P"}, {"bases", 3}, {"allow_robots", true}});
    EXPECT_STATUS(201, rTable);
    std::string roomId = rTable.json.value("room_id", "");
    CHECK_FALSE(roomId.empty());

    Reply rJoin = http().post(
        "/api/v1/tables/" + roomId + "/join", botToken, Json{{"seat", 2}});
    EXPECT_STATUS(200, rJoin);
    CHECK_TRUE(rJoin.json.value("game_started", false));

    /* Human resigns, giving robot a recorded win */
    Reply rResign = http().post(
        "/api/v1/games/" + roomId + "/resign", u.token, Json::object());
    EXPECT_STATUS(200, rResign);
    CHECK_TRUE(rResign.json.value("game_over", false));

    /* Now robot has games_played > 0 and appears in public directory */
    Reply rDir = http().get("/api/v1/robots/directory");
    EXPECT_STATUS(200, rDir);
    CHECK_TRUE(rDir.json.value("total", 0) >= 1);
    Json bots = rDir.json["robots"];
    CHECK_TRUE(bots.is_array());
    bool found = false;
    for (size_t i = 0; i < bots.size(); ++i) {
        if (bots[i].value("robot_uuid", "") == botUuid) {
            found = true;
            STRCMP_EQUAL("dir-alpha-bot", bots[i].value("name", "").c_str());
            STRCMP_EQUAL("active", bots[i].value("status", "").c_str());
            CHECK_EQUAL(1, bots[i].value("games_played", 0));
            CHECK_EQUAL(1, bots[i].value("wins", 0));
            CHECK_EQUAL(0, bots[i].value("losses", 0));
            CHECK_FALSE(bots[i].contains("token"));
            CHECK_FALSE(bots[i].contains("token_prefix"));
            STRCMP_EQUAL("RobotDirOwner", bots[i].value("owner_handle", "").c_str());
        }
    }
    CHECK_TRUE(found);

    /* Robot profile by UUID */
    Reply rProfUuid = http().get("/api/v1/robots/profile/" + botUuid);
    EXPECT_STATUS(200, rProfUuid);
    CHECK_TRUE(rProfUuid.json.value("is_robot", false));
    STRCMP_EQUAL("dir-alpha-bot", rProfUuid.json.value("name", "").c_str());
    STRCMP_EQUAL("active", rProfUuid.json.value("status", "").c_str());
    CHECK_EQUAL(1, rProfUuid.json.value("games_played", 0));
    CHECK_EQUAL(1, rProfUuid.json.value("wins", 0));
    CHECK_TRUE(rProfUuid.json["recent_games"].is_array());
    CHECK_EQUAL(1, static_cast<int>(rProfUuid.json["recent_games"].size()));
    STRCMP_EQUAL("WIN", rProfUuid.json["recent_games"][0].value("player_outcome", "").c_str());

    /* Robot profile by name */
    Reply rProfName = http().get("/api/v1/robots/profile/dir-alpha-bot");
    EXPECT_STATUS(200, rProfName);
    STRCMP_EQUAL(botUuid.c_str(), rProfName.json.value("robot_uuid", "").c_str());

    /* User profile endpoint fallback to robot profile */
    Reply rUserBot = http().get("/api/v1/users/dir-alpha-bot");
    EXPECT_STATUS(200, rUserBot);
    CHECK_TRUE(rUserBot.json.value("is_robot", false));
    STRCMP_EQUAL("dir-alpha-bot", rUserBot.json.value("name", "").c_str());

    /* Permanent deletion on robot with games played: in-place retirement */
    Reply rDel = http().del("/api/v1/robots/" + botUuid + "?permanent=1", u.token);
    EXPECT_STATUS(200, rDel);
    CHECK_TRUE(rDel.json.value("deleted", false));
    Reply rProfRetired = http().get("/api/v1/robots/profile/" + botUuid);
    EXPECT_STATUS(200, rProfRetired);
    STRCMP_EQUAL("retired", rProfRetired.json.value("status", "").c_str());

    /* Permanent deletion on robot with 0 games played: physically deleted */
    Reply rBot0 = http().post(
        "/api/v1/robots", u.token,
        Json{{"robot_name", "zero-game-bot"}});
    EXPECT_STATUS(201, rBot0);
    std::string bot0Uuid = rBot0.json.value("robot_uuid", "");
    Reply rDel0 = http().del("/api/v1/robots/" + bot0Uuid + "?permanent=1", u.token);
    EXPECT_STATUS(200, rDel0);
    CHECK_TRUE(rDel0.json.value("deleted", false));
    Reply rProf0 = http().get("/api/v1/robots/profile/" + bot0Uuid);
    EXPECT_STATUS(404, rProf0);

    /* Purging the archived robot game recalculates robot W/L/D statistics */
    env->store.setAdminGoogleIdentities({"sub-robot-dir-owner"});
    Reply rPurgeGame = http().del("/api/v1/archive/games/" + roomId, u.token);
    EXPECT_STATUS(200, rPurgeGame);
    STRCMP_EQUAL("purged", rPurgeGame.json.value("status", "").c_str());

    octi::RobotRecord afterPurgeRec;
    CHECK_TRUE(env->store.getRobotByUuid(botUuid, afterPurgeRec));
    CHECK_EQUAL(0, afterPurgeRec.gamesPlayed);
    CHECK_EQUAL(0, afterPurgeRec.wins);
    CHECK_EQUAL(0, afterPurgeRec.losses);
    CHECK_EQUAL(0, afterPurgeRec.draws);
}

TEST(OctiHttpServerIntegrationTest, TableHostDeleteEndpoint)
{
    User userA = signIn("google", "sub-table-host-a", "HostAlice");
    User userB = signIn("apple", "sub-table-user-b", "GuestBob");

    Reply rTab = http().post(
        "/api/v1/tables", userA.token,
        Json{{"mode", "2P"}, {"bases", 3}});
    EXPECT_STATUS(201, rTab);
    std::string roomId = rTab.json.value("room_id", "");
    CHECK_FALSE(roomId.empty());

    /* Guest userB attempts to delete table -> 403 ERR_FORBIDDEN */
    Reply rDelB = http().del("/api/v1/tables/" + roomId, userB.token);
    EXPECT_STATUS(403, rDelB);
    STRCMP_EQUAL("ERR_FORBIDDEN", rDelB.json.value("error", "").c_str());

    /* Host userA deletes table -> 200 OK */
    Reply rDelA = http().del("/api/v1/tables/" + roomId, userA.token);
    EXPECT_STATUS(200, rDelA);
    STRCMP_EQUAL("purged", rDelA.json.value("status", "").c_str());

    /* Admin deletes table created by host */
    env->store.setAdminGoogleIdentities({"sub-admin-host-test"});
    User admin = signIn("google", "sub-admin-host-test", "AdminCharlie");

    Reply rTab2 = http().post(
        "/api/v1/tables", userA.token,
        Json{{"mode", "2P"}, {"bases", 3}});
    EXPECT_STATUS(201, rTab2);
    std::string roomId2 = rTab2.json.value("room_id", "");

    Reply rDelAdmin = http().del("/api/v1/tables/" + roomId2, admin.token);
    EXPECT_STATUS(200, rDelAdmin);
    STRCMP_EQUAL("purged", rDelAdmin.json.value("status", "").c_str());
}

TEST(OctiHttpServerIntegrationTest, WebUiNewNavAndRobotsTabServed)
{
    Reply rIndex = http().get("/");
    EXPECT_STATUS(200, rIndex);
    CHECK_TRUE(rIndex.body.find("id=\"tab-robots\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"nav-online-btn\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("Play in Browser") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("Play Online") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("OCTI v2.3.7") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("Official Platform") == std::string::npos);
    CHECK_TRUE(rIndex.body.find("OFFICIAL PLATFORM") == std::string::npos);
    CHECK_TRUE(rIndex.body.find("Lv4: Expert (40s)") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("Lv5: Grandmaster (60s)") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("timeBudgetMs: 40000") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("timeBudgetMs: 60000") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("hasOtherMovablePodAtOrigin") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("unclaimedMovablePodIndicesAtOrigin") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("_claimedStartPods") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"robot-profile-detail\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"robots-tbody\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"account-robots-tbody\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("var tbody = $(\"account-robots-tbody\");") != std::string::npos);

    /* Ensure zero duplicate HTML id="..." attributes across the entire WebUi DOM */
    {
        const size_t scriptPos = rIndex.body.find("<script>");
        const std::string htmlPart = (scriptPos != std::string::npos)
            ? rIndex.body.substr(0, scriptPos)
            : rIndex.body;
        std::set<std::string> seenIds;
        size_t pos = 0;
        while ((pos = htmlPart.find("id=\"", pos)) != std::string::npos) {
            pos += 4;
            const size_t endQuote = htmlPart.find('"', pos);
            if (endQuote == std::string::npos) {
                break;
            }
            const std::string idVal = htmlPart.substr(pos, endQuote - pos);
            CHECK_FALSE_TEXT(seenIds.count(idVal) > 0,
                             ("Duplicate HTML id attribute: " + idVal).c_str());
            seenIds.insert(idVal);
            pos = endQuote + 1;
        }
    }
    CHECK_TRUE(rIndex.body.find("Quick Guide") == std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"web-room-chat-log\"") == std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"web-table-info-card\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("formatRoomEventForChat") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("has joined the table") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("has left the table") != std::string::npos);
    size_t posChatBox = rIndex.body.find("id=\"web-lobby-chat-box\"");
    size_t posUsersSec = rIndex.body.find("id=\"web-online-users-section\"");
    CHECK_TRUE(posChatBox != std::string::npos);
    CHECK_TRUE(posUsersSec != std::string::npos);
    CHECK_TRUE(posChatBox < posUsersSec);
}

TEST(OctiHttpServerIntegrationTest, WebUiGuiDesignBookComplianceInvariants)
{
    Reply rIndex = http().get("/");
    EXPECT_STATUS(200, rIndex);

    /* CHK-01 (GDB-10.1a): Undo is strictly hidden and disabled in online games */
    CHECK_TRUE(rIndex.body.find("if (undoBtn) undoBtn.style.display = \"none\";") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("var undoAction = hasPendingUndo ? \"accept_undo\" : \"request_undo\";") == std::string::npos);

    /* CHK-02 (GDB-05.1): 30%-edge Pink (#FF69B4) Running Light in renderPlayBoardCanvas */
    CHECK_TRUE(rIndex.body.find("function drawPlayPinkSub(a, b)") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("var segFrac = 0.30;") != std::string::npos);

    /* CHK-03 (GDB-06.1): Auto-rotate board immediately upon sitting at an online seat */
    CHECK_TRUE(rIndex.body.find("boardRotationDeg = rotationForSeat(modeForRot, seatLetterOf(joinedSeat));") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("boardRotationDeg = rotationForSeat(modeForRot, seatLetterOf(vSeat));") != std::string::npos);

    /* CHK-04 (GDB-07.2): Single-click empty friendly home square stages reserve pod placement */
    CHECK_TRUE(rIndex.body.find("moveComposer.stageAddPod(x, y, false);") != std::string::npos);

    /* CHK-06 (GDB-07.1): Zoom-only prong addition (no direct outer-ring prong click on main board) */
    CHECK_TRUE(rIndex.body.find("moveComposer.stageAddProng(x, y, bestFriendlyIdx, sector, false)") == std::string::npos);

    /* CHK-07 (GDB-04.2a): Main board pod stacks sorted descending (most-prongs-at-bottom) */
    CHECK_TRUE(rIndex.body.find("var sortedDesc = podStack.slice().sort(function (a, b) {") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("return (b.cfg ? b.cfg.count : 0) - (a.cfg ? a.cfg.count : 0);") != std::string::npos);

    /* Synced 2,038-entry opening book in WebUi.cxx */
    CHECK_TRUE(rIndex.body.find("BEGIN_OPENING_BOOK_DATA") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("END_OPENING_BOOK_DATA") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("\"0x00172E8C050D20ED\"") != std::string::npos);
}

TEST(OctiHttpServerIntegrationTest, CreateTableWithSeatSelectionAndUnseatedHost)
{
    User hostA = signIn("google", "sub-polite-host-a", "PoliteHostA");
    User humanB = signIn("google", "sub-polite-human-b", "HumanPlayerB");

    /* Fault: out-of-range seat (-1, 3 in 2P, 5) rejected with 400 INVALID_ARGUMENT */
    Reply rBadNeg = http().post(
        "/api/v1/tables", hostA.token,
        Json{{"mode", "2P"}, {"bases", 3}, {"seat", -1}});
    EXPECT_ERROR(400, "INVALID_ARGUMENT", rBadNeg);

    Reply rBad2P = http().post(
        "/api/v1/tables", hostA.token,
        Json{{"mode", "2P"}, {"bases", 3}, {"seat", 3}});
    EXPECT_ERROR(400, "INVALID_ARGUMENT", rBad2P);

    Reply rBadHigh = http().post(
        "/api/v1/tables", hostA.token,
        Json{{"mode", "4P"}, {"bases", 3}, {"seat", 5}});
    EXPECT_ERROR(400, "INVALID_ARGUMENT", rBadHigh);

    /* Nominal 1: Create 2P table explicitly in Seat 2 (B) */
    Reply rSeat2 = http().post(
        "/api/v1/tables", hostA.token,
        Json{{"mode", "2P"}, {"bases", 3}, {"seat", 2}});
    EXPECT_STATUS(201, rSeat2);
    CHECK_EQUAL(2, rSeat2.json.value("seat", 0));
    std::string roomSeat2 = rSeat2.json.value("room_id", "");
    CHECK_FALSE(roomSeat2.empty());

    Reply rViewSeat2 = http().get("/api/v1/tables/" + roomSeat2, hostA.token);
    EXPECT_STATUS(200, rViewSeat2);
    CHECK_EQUAL(2, rViewSeat2.json.value("viewer_seat", 0));
    CHECK_TRUE(rViewSeat2.json["seats"][0].value("handle", "").empty());
    STRCMP_EQUAL(hostA.handle.c_str(), rViewSeat2.json["seats"][1].value("handle", "").c_str());

    /* Clean up roomSeat2 by leaving */
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + roomSeat2 + "/leave", hostA.token, Json::object()));
    EXPECT_STATUS(404, http().get("/api/v1/tables/" + roomSeat2, hostA.token));

    /* Boundary: Create unseated table (seat: 0), verify duplicate create blocked, then leave cleans up room */
    Reply rUnseatedLeave = http().post(
        "/api/v1/tables", hostA.token,
        Json{{"mode", "2P"}, {"bases", 3}, {"seat", 0}});
    EXPECT_STATUS(201, rUnseatedLeave);
    CHECK_EQUAL(0, rUnseatedLeave.json.value("seat", -1));
    std::string roomUnseatedLeave = rUnseatedLeave.json.value("room_id", "");
    CHECK_FALSE(roomUnseatedLeave.empty());

    /* Fault: cannot host a second table while already hosting an unseated waiting table */
    Reply rDupHost = http().post(
        "/api/v1/tables", hostA.token,
        Json{{"mode", "2P"}, {"bases", 3}, {"seat", 1}});
    EXPECT_ERROR(409, "ALREADY_SEATED", rDupHost);

    /* Leaving empty unseated table deletes it */
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + roomUnseatedLeave + "/leave", hostA.token, Json::object()));
    EXPECT_STATUS(404, http().get("/api/v1/tables/" + roomUnseatedLeave, hostA.token));

    /* Nominal 2: Unseated host (seat: 0) -> human picks Seat 2 (B) -> host joins seat: 0 and gets Seat 1 (A) */
    Reply rWait1 = http().post(
        "/api/v1/tables", hostA.token,
        Json{{"mode", "2P"}, {"bases", 3}, {"seat", 0}});
    EXPECT_STATUS(201, rWait1);
    CHECK_EQUAL(0, rWait1.json.value("seat", -1));
    std::string roomWait1 = rWait1.json.value("room_id", "");

    Reply rHumanPick2 = http().post(
        "/api/v1/tables/" + roomWait1 + "/join", humanB.token,
        Json{{"seat", 2}});
    EXPECT_STATUS(200, rHumanPick2);
    CHECK_EQUAL(2, rHumanPick2.json.value("seat", 0));
    CHECK_FALSE(rHumanPick2.json.value("game_started", true));

    Reply rHostFill1 = http().post(
        "/api/v1/tables/" + roomWait1 + "/join", hostA.token,
        Json{{"seat", 0}});
    EXPECT_STATUS(200, rHostFill1);
    CHECK_EQUAL(1, rHostFill1.json.value("seat", 0));
    CHECK_TRUE(rHostFill1.json.value("game_started", false));
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
