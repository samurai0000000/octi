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
#include "OctiNetworkClient.hxx"
#include "OctiRulebook.hxx"
#include "OctiSoundCue.hxx"
#include "OidcVerifier.hxx"
#include "ServerApp.hxx"
#include "WebUi.hxx"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <zlib.h>

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
#include <condition_variable>
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
    claims["email_verified"] = true;
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

    Reply getRaw(const std::string &path, const std::string &token = "",
                 const httplib::Headers &extra = httplib::Headers()) const
    {
        httplib::Client cli = client();
        cli.set_decompress(false);
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
    c.browseAnonIpBurst = 200.0;
    c.lobbySnapAnonIpBurst = 100.0;
    c.livePollAnonIpBurst = 100.0;
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
    CHECK_EQUAL(8, playPlies(room, players, GAME_MODE_2, 8));
    CHECK_EQUAL(8, view(room).value("ply_count", 0));
    CHECK_EQUAL(8, static_cast<int>(view(room)["moves"].size()));

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
    CHECK_EQUAL(8, list.json["games"][0].value("ply_count", 0));
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
    /* Self-contained: no third-party external resource (allow w3.org svg namespace,
     * first-party selfso.com software download links, and local favicon <link> tags). */
    CHECK_TRUE(ui.body.find("https://www.selfso.com/software/windows/octi/") != std::string::npos);
    CHECK_TRUE(ui.body.find("https://www.selfso.com/software/android/octi/") != std::string::npos);
    CHECK_TRUE(ui.body.find("home-native-apps-promo") != std::string::npos);
    Reply favReply = http().get("/favicon.svg");
    EXPECT_STATUS(200, favReply);
    CHECK_TRUE(favReply.body.find("<svg") != std::string::npos);
    std::string strippedUi = ui.body;
    const char *kAllowedTokens[] = {
        "http://www.w3.org/2000/svg",
        "https://www.selfso.com/software/windows/octi/",
        "https://www.selfso.com/software/android/octi/",
        "placeholder=\"Robot URL (optional http:// or https://, no '?')\"",
        "<link rel=\"icon\"",
        "<link rel=\"apple-touch-icon\""
    };
    for (size_t ti = 0; ti < sizeof(kAllowedTokens) / sizeof(kAllowedTokens[0]); ++ti) {
        const std::string tok(kAllowedTokens[ti]);
        size_t pos = 0;
        while ((pos = strippedUi.find(tok)) != std::string::npos) {
            strippedUi.replace(pos, tok.size(), "");
        }
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
    CHECK_EQUAL(8, playPlies(kids, two, GAME_MODE_OFK, 8, Json{{"bases", 1}}));
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
    STRCMP_EQUAL("octi.selfso.com",
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
    std::string resTr = makeResignedTranscript({"33+a", "37+e"}, "inst-res-02", 4, "android-arm64");
    Reply rRes = http().post("/api/v1/archive/upload", "", Json{{"transcript", resTr}});
    EXPECT_STATUS(200, rRes);
    std::string resId = rRes.json.value("game_id", "");
    CHECK_FALSE(resId.empty());
    CHECK_EQUAL(2, rRes.json.value("ply_count", 0));

    Reply rGetRes = http().get("/api/v1/archive/games/" + resId);
    EXPECT_STATUS(200, rGetRes);
    STRCMP_EQUAL("RESIGNED", rGetRes.json.value("status", "").c_str());
    STRCMP_EQUAL("B", rGetRes.json.value("winner", "").c_str());
    CHECK_EQUAL(4, rGetRes.json.value("ai_difficulty", 0));
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
    const std::string installTok = "0123456789abcdef0123456789abcdef";
    std::string wonTr = makeWonTranscript("inst-delete-test", 3);
    Reply r1 = http().post("/api/v1/archive/upload", "",
                           Json{{"transcript", wonTr}},
                           {{"X-Octi-Install-Token", installTok}});
    EXPECT_STATUS(200, r1);
    std::string gameId = r1.json.value("game_id", "");
    CHECK_FALSE(gameId.empty());

    /* Submitting the exact same game twice returns 409 DUPLICATE_ID */
    Reply rDup = http().post("/api/v1/archive/upload", "",
                             Json{{"transcript", wonTr}},
                             {{"X-Octi-Install-Token", installTok}});
    EXPECT_STATUS(409, rDup);
    STRCMP_EQUAL("DUPLICATE_ID", rDup.json.value("error", "").c_str());

    /* Verify game exists before deletion */
    EXPECT_STATUS(200, http().get("/api/v1/archive/games/" + gameId));

    /* Unauthenticated or wrong-token DELETE is rejected */
    EXPECT_STATUS(401, http().del("/api/v1/archive/install/inst-delete-test", ""));
    EXPECT_STATUS(403, http().del("/api/v1/archive/install/inst-delete-test", "",
                                  {{"X-Octi-Install-Token", "fedcba9876543210fedcba9876543210"}}));

    /* DELETE /api/v1/archive/install/:installId with matching X-Octi-Install-Token succeeds */
    Reply rDel = http().del("/api/v1/archive/install/inst-delete-test", "",
                            {{"X-Octi-Install-Token", installTok}});
    EXPECT_STATUS(200, rDel);
    CHECK_TRUE(rDel.json.value("ok", false));
    CHECK_EQUAL(1, rDel.json.value("deleted", 0));

    /* Deleted game is now gone */
    Reply rGone = http().get("/api/v1/archive/games/" + gameId);
    EXPECT_STATUS(404, rGone);

    /* Second deletion with valid token returns 0 deleted */
    Reply rDel2 = http().del("/api/v1/archive/install/inst-delete-test", "",
                             {{"X-Octi-Install-Token", installTok}});
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
    std::string resTr = makeResignedTranscript({"33+a", "37+e"}, "inst-filt-2", 4, "android-arm64");
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

    Reply rAi4 = http().get("/api/v1/archive/games?aiDifficulty=4");
    EXPECT_STATUS(200, rAi4);
    CHECK_EQUAL(1, rAi4.json.value("total", 0));

    Reply rAi1 = http().get("/api/v1/archive/games?aiDifficulty=1");
    EXPECT_STATUS(200, rAi1);
    CHECK_EQUAL(0, rAi1.json.value("total", 0));

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
    STRCMP_EQUAL("octi.selfso.com", rStats.json.value("attribution", "").c_str());

    /* Test /api/v1/archive/export (NDJSON streaming requires Bearer auth) */
    EXPECT_ERROR(401, "UNAUTHORIZED", http().get("/api/v1/archive/export"));
    User exporter = signIn("google", "sub-arch-exporter", "Arch Exporter");
    Reply rExp = http().get("/api/v1/archive/export", exporter.token);
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
        STRCMP_EQUAL("octi.selfso.com",
                     jRow.value("attribution", "").c_str());
        CHECK_TRUE(line.find("inst-filt") == std::string::npos);
        CHECK_TRUE(line.find("\"extended\"") == std::string::npos);
    }
    CHECK_EQUAL(2, lineCount);
}

TEST(OctiHttpServerIntegrationTest, CrashTelemetryNominalUploadAndRetrieval)
{
    env->store.setAdminGoogleIdentities({"sub-telemetry-admin"});
    User admin = signIn("google", "sub-telemetry-admin", "Telemetry Admin");
    User nonAdmin = signIn("google", "sub-telemetry-user", "Regular User");

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

    /* 1b. Unauthenticated & non-admin callers are denied (401 / 403) */
    EXPECT_STATUS(401, http().get("/api/v1/telemetry/crashes"));
    EXPECT_STATUS(403, http().get("/api/v1/telemetry/crashes", nonAdmin.token));
    EXPECT_STATUS(401, http().get("/api/v1/telemetry/crashes/" + crashId));
    EXPECT_STATUS(403, http().get("/api/v1/telemetry/crashes/" + crashId, nonAdmin.token));

    /* 2. List crash reports as admin and check filtering */
    Reply rList = http().get("/api/v1/telemetry/crashes", admin.token);
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
    Reply rWin = http().get("/api/v1/telemetry/crashes?platform=windows-x86_64", admin.token);
    EXPECT_STATUS(200, rWin);
    CHECK_EQUAL(1, rWin.json.value("total", 0));

    Reply rLin = http().get("/api/v1/telemetry/crashes?platform=linux-x86_64", admin.token);
    EXPECT_STATUS(200, rLin);
    CHECK_EQUAL(0, rLin.json.value("total", 0));

    /* Filter by app_version */
    Reply rVer = http().get("/api/v1/telemetry/crashes?app_version=2.1.2", admin.token);
    EXPECT_STATUS(200, rVer);
    CHECK_EQUAL(1, rVer.json.value("total", 0));

    Reply rOldVer = http().get("/api/v1/telemetry/crashes?app_version=1.0.0", admin.token);
    EXPECT_STATUS(200, rOldVer);
    CHECK_EQUAL(0, rOldVer.json.value("total", 0));

    /* Filter by exception_type */
    Reply rExc = http().get("/api/v1/telemetry/crashes?exception_type=std::invalid_argument", admin.token);
    EXPECT_STATUS(200, rExc);
    CHECK_EQUAL(1, rExc.json.value("total", 0));

    Reply rOtherExc = http().get("/api/v1/telemetry/crashes?exception_type=std::out_of_range", admin.token);
    EXPECT_STATUS(200, rOtherExc);
    CHECK_EQUAL(0, rOtherExc.json.value("total", 0));

    /* Pagination */
    Reply rPage0 = http().get("/api/v1/telemetry/crashes?limit=1&offset=0", admin.token);
    EXPECT_STATUS(200, rPage0);
    CHECK_EQUAL(1, static_cast<int>(rPage0.json["crashes"].size()));

    Reply rPage1 = http().get("/api/v1/telemetry/crashes?limit=1&offset=1", admin.token);
    EXPECT_STATUS(200, rPage1);
    CHECK_EQUAL(0, static_cast<int>(rPage1.json["crashes"].size()));

    /* 3. Get single crash report detail */
    Reply rDetail = http().get("/api/v1/telemetry/crashes/" + crashId, admin.token);
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
    Reply rNotFound = http().get("/api/v1/telemetry/crashes/non-existent-crash-id-12345", admin.token);
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
    env->store.setAdminGoogleIdentities({"sub-purge-admin"});
    User admin = signIn("google", "sub-purge-admin", "Purge Admin");
    std::string installToPurge = "purge-install-id-99";
    std::string purgeToken = "abcdef0123456789abcdef0123456789";
    std::string crashId = "crash-to-be-purged-001";

    Json crashPayload = {
        {"crash_id", crashId},
        {"platform", "linux-x86_64"},
        {"app_version", "2.1.2"},
        {"exception_type", "std::logic_error"},
        {"exception_message", "privacy purge test"},
        {"install_id", installToPurge}
    };
    Reply rUpload = http().post("/api/v1/telemetry/crash", "", crashPayload,
                                {{"X-Octi-Install-Token", purgeToken}});
    EXPECT_STATUS(201, rUpload);

    /* Also upload a game for the same install_id */
    std::string gameTr = makeWonTranscript(installToPurge, 3);
    Reply rGame = http().post("/api/v1/archive/upload", "",
                              Json{{"transcript", gameTr}},
                              {{"X-Octi-Install-Token", purgeToken}});
    EXPECT_STATUS(200, rGame);
    std::string gameId = rGame.json.value("game_id", "");
    CHECK_FALSE(gameId.empty());

    /* Both game and crash report exist */
    EXPECT_STATUS(200, http().get("/api/v1/archive/games/" + gameId));
    EXPECT_STATUS(200, http().get("/api/v1/telemetry/crashes/" + crashId, admin.token));

    /* Execute privacy deletion for installId with valid X-Octi-Install-Token */
    Reply rDel = http().del("/api/v1/archive/install/" + installToPurge, "",
                            {{"X-Octi-Install-Token", purgeToken}});
    EXPECT_STATUS(200, rDel);
    CHECK_TRUE(rDel.json.value("ok", false));
    /* Expect 2 records deleted: 1 game + 1 crash report */
    CHECK_EQUAL(2, rDel.json.value("deleted", 0));

    /* Both game and crash report are now purged (404) */
    EXPECT_STATUS(404, http().get("/api/v1/archive/games/" + gameId));
    EXPECT_STATUS(404, http().get("/api/v1/telemetry/crashes/" + crashId, admin.token));

    /* Crash report list no longer includes crashId */
    Reply rListAfter = http().get("/api/v1/telemetry/crashes", admin.token);
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
    CHECK_TRUE(rLobbyReal.body.find("203.0.113.42") == std::string::npos);
    Json players = rLobbyReal.json["players"];
    CHECK_TRUE(players.is_array());
    bool foundAnon = false;
    for (size_t i = 0; i < players.size(); ++i) {
        std::string h = players[i].value("handle", "");
        if (h.rfind("anonymous-", 0) == 0 && h != "anonymous-127.0.0.1") {
            CHECK_EQUAL(18, static_cast<int>(h.size()));
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
    std::string tr1 = makeResignedTranscript({"33+b", "37+f"}, "dev-bound-01", 3);
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

    // 2. SEC-01: Unauthenticated upload reusing a bound install_id must NOT attribute to Alice
    std::string tr2 = makeResignedTranscript({"33+c", "37+g"}, "dev-bound-01", 4);
    Reply rUp2 = http().post("/api/v1/archive/upload", "",
                             Json{{"transcript", tr2}});
    EXPECT_STATUS(200, rUp2);
    std::string gid2 = rUp2.json.value("game_id", "");
    CHECK_FALSE(gid2.empty());

    Reply rGet2 = http().get("/api/v1/archive/games/" + gid2);
    EXPECT_STATUS(200, rGet2);
    CHECK_TRUE(rGet2.json["players"].is_array() && !rGet2.json["players"].empty());
    STRCMP_EQUAL("Player", rGet2.json["players"][0].value("handle", "").c_str());
    CHECK_TRUE(rGet2.json["players"][0].value("player_uuid", "").empty());

    // 3. Upload from unknown device without auth -> remains anonymous Player
    std::string tr3 = makeResignedTranscript({"33+d", "37+h"}, "dev-unknown-99", 5);
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

    // 6. Strict self-contained CSP: zero third-party external URLs
    std::string strippedUi = rUi.body;
    const char *kAllowedTokens[] = {
        "http://www.w3.org/2000/svg",
        "https://www.selfso.com/software/windows/octi/",
        "https://www.selfso.com/software/android/octi/",
        "placeholder=\"Robot URL (optional http:// or https://, no '?')\"",
        "<link rel=\"icon\"",
        "<link rel=\"apple-touch-icon\""
    };
    for (size_t ti = 0; ti < sizeof(kAllowedTokens) / sizeof(kAllowedTokens[0]); ++ti) {
        const std::string tok(kAllowedTokens[ti]);
        size_t pos = 0;
        while ((pos = strippedUi.find(tok)) != std::string::npos) {
            strippedUi.replace(pos, tok.size(), "");
        }
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
        "33+a",   "37+f",    "77+e",    "73+b",    "33a-34",  "37f+h",
        "77e-76", "73b-84",  "34a-35",  "37fh-26", "76e-75",  "84b+g",
        "35a-36", "26fh+a",  "75e-74",  "84bg+h",  "36a-37",  "74e-73"
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
    CHECK_TRUE(rUi.body.find("AI_L1") != std::string::npos);
    CHECK_TRUE(rUi.body.find("AI_L2") != std::string::npos);
    CHECK_TRUE(rUi.body.find("AI_L3") != std::string::npos);
    CHECK_TRUE(rUi.body.find("AI_L4") != std::string::npos);
    CHECK_TRUE(rUi.body.find("AI_L5") != std::string::npos);
    CHECK_TRUE(rUi.body.find("AI_Casual") == std::string::npos);
    CHECK_TRUE(rUi.body.find("AI_Tactical") == std::string::npos);
    CHECK_TRUE(rUi.body.find("AI_Master") == std::string::npos);
    CHECK_TRUE(rUi.body.find("AI_Expert") == std::string::npos);
    CHECK_TRUE(rUi.body.find("AI_Grandmaster") == std::string::npos);

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

// Test 1: WebMoveComposer human moves advance turn, reject prong reposition (R1), support jump capture toggle, and botSearch reply
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

// R1: Prong repositioning removed from WebMoveComposer and rejected by applyMoveToState
assert.strictEqual(typeof moveComposer.stageRepositionProng, "undefined",
                   "stageRepositionProng must be removed under R1");
assert.throws(function() {
  applyMoveToState(cloneState(activeState), "33a+b-a", "A");
}, /reposition/i, "applyMoveToState must reject +add-rm reposition syntax");
assert.throws(function() {
  applyMoveToState(cloneState(activeState), "33a-a+b", "A");
}, /reposition/i, "applyMoveToState must reject -rm+add reposition syntax");

// Stage & commit second normal prong addition on A's pod at (3,3): 33a+b
var podIdxA = 0;
assert.strictEqual(moveComposer.stageAddProng(3, 3, podIdxA, 1, false), true);
assert.strictEqual(moveComposer.stagedNotation, "33a+b");
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
    { seat: 2, player: "B", handle: "AI_Tactical", type: "AI", aiDifficulty: 3, aiTimeBudgetMs: 3000 }
  ],
  moves: moves2p,
  plies: plies2p,
  timestamp: Math.floor(Date.now() / 1000)
};
fs.writeFileSync(process.argv[2], JSON.stringify(tr2p));

// Build a full 4P victory transcript using JS engine and write to disk for C++ server validation
var s4p = parseOctiFen("4:3:0:0:P:A:-:0:3,0,12,0:3,0,12,0:3,0,12,0:3,0,12,0:A33,D73,B37,C77");
var moves4p = [
  "33+a", "37+f", "77+e", "73+b", "33a-34", "37f+h",
  "77e-76", "73b-84", "34a-35", "37fh-26", "76e-75", "84b+g",
  "35a-36", "26fh+a", "75e-74", "84bg+h", "36a-37", "74e-73"
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
    { seat: 2, player: "B", handle: "AI_Tactical", type: "AI", aiDifficulty: 3, aiTimeBudgetMs: 3000 },
    { seat: 3, player: "C", handle: "AI_Master", type: "AI", aiDifficulty: 3, aiTimeBudgetMs: 3000 },
    { seat: 4, player: "D", handle: "AI_Tactical", type: "AI", aiDifficulty: 3, aiTimeBudgetMs: 3000 }
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

    // 1. Validate all 8 post-R1 reposition-free real database games through C++
    //    OctiBoard::deserializeGame and POST /api/v1/archive/upload
    const size_t numValid = sizeof(octi::test::kValidArchiveCorpusGames) /
                            sizeof(octi::test::kValidArchiveCorpusGames[0]);
    CHECK_EQUAL(8, static_cast<int>(numValid));

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

        bool isIgnored = rUp.json.value("ignored", false);
        Reply rGet = http().get(std::string("/api/v1/archive/games/") + entry.gameId);
        if (isIgnored) {
            EXPECT_STATUS(404, rGet);
        } else {
            EXPECT_STATUS(200, rGet);
            STRCMP_EQUAL("octi.selfso.com", rGet.json.value("attribution", "").c_str());
            STRCMP_EQUAL(entry.expectedWinner, rGet.json.value("winner", "").c_str());
            STRCMP_EQUAL(entry.finalFen, rGet.json.value("final_fen", "").c_str());
            if (std::string(entry.expectedRuleTermination) == "TOTAL_CAPTURE") {
                STRCMP_EQUAL("TOTAL_CAPTURE", rGet.json.value("termination", "").c_str());
            }
        }
    }

    // 1b. Verify all 11 pre-R1 historical games containing prong reposition plies
    //     are deterministically rejected under R1 with IllegalMoveSyntaxException / 400 INVALID_GAME
    const size_t numLegacyRepo = sizeof(octi::test::kLegacyRepositionRejectedCorpusGames) /
                                 sizeof(octi::test::kLegacyRepositionRejectedCorpusGames[0]);
    CHECK_EQUAL(11, static_cast<int>(numLegacyRepo));

    for (size_t i = 0; i < numLegacyRepo; ++i) {
        const auto &entry = octi::test::kLegacyRepositionRejectedCorpusGames[i];
        octi::OctiBoard replayBoard;
        CHECK_THROWS(octi::IllegalMoveSyntaxException,
                     replayBoard.deserializeGame(entry.transcriptJson));

        Reply rRepo = http().post("/api/v1/archive/upload", uCorpus.token,
                                  Json{{"transcript", std::string(entry.transcriptJson)}});
        EXPECT_STATUS(400, rRepo);
        STRCMP_EQUAL("INVALID_GAME", rRepo.json.value("error", "").c_str());
    }

    // 2. Verify all 8 invalid database games are rejected by the server
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

    // 3. Replay all 8 valid games, 11 legacy-reposition games, and 8 invalid games
    //    through the Web JS engine (WebUi.cxx) in Node.js to verify 100% C++/JS parity
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
    corpusPayload["legacyRepositionRejected"] = Json::array();
    for (size_t i = 0; i < numLegacyRepo; ++i) {
        const auto &entry = octi::test::kLegacyRepositionRejectedCorpusGames[i];
        corpusPayload["legacyRepositionRejected"].push_back(Json{
            {"gameId", entry.gameId},
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
assert.strictEqual(payload.valid.length, 8);
assert.strictEqual(payload.legacyRepositionRejected.length, 11);
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

for (var lr = 0; lr < payload.legacyRepositionRejected.length; lr++) {
  var legItem = payload.legacyRepositionRejected[lr];
  var lt = legItem.transcript;
  var lst = parseOctiFen(makeInitialFen(lt.mode, lt.bases, !!lt.superProngs, !!lt.edgeless, lt.startPlayer || "A"));
  var threwRepo = false;
  try {
    for (var lpIdx = 0; lpIdx < lt.plies.length; lpIdx++) {
      var lply = lt.plies[lpIdx];
      lst = applyMoveToState(lst, lply.move, lst.turn);
    }
  } catch (err) {
    threwRepo = true;
    assert.ok(/reposition/i.test(String(err && err.message ? err.message : err)),
              "Expected reposition rejection error on " + legItem.gameId + ", got: " + err);
  }
  assert.strictEqual(threwRepo, true, "Legacy reposition game " + legItem.gameId + " must throw in JS applyMoveToState");
}

for (var j = 0; j < payload.invalid.length; j++) {
  var bad = payload.invalid[j];
  var bt = bad.transcript;
  var bst = parseOctiFen(makeInitialFen(bt.mode, bt.bases, !!bt.superProngs, !!bt.edgeless, bt.startPlayer || "A"));
  var threwIllegal = false;
  try {
    for (var bpIdx = 0; bpIdx < bt.plies.length; bpIdx++) {
      var bply = bt.plies[bpIdx];
      bst = applyMoveToState(bst, bply.move, bst.turn);
    }
  } catch (err) {
    threwIllegal = true;
  }
  if (bad.expectedErrorCode === "GAME_NOT_COMPLETED") {
    assert.strictEqual(threwIllegal, false, "Prematurely terminated game " + bad.gameId + " must not throw");
    assert.strictEqual(bst.status, "P", "Prematurely terminated game " + bad.gameId + " must still be in progress (freePods > 0)");
    var remLegal = generateMoves(bst, bst.turn, false);
    assert.ok(remLegal.length > 0, "Side with reserve pods in " + bad.gameId + " must have legal reserve placement moves");
  } else {
    assert.strictEqual(bad.expectedErrorCode, "INVALID_GAME");
    assert.ok(threwIllegal || serializeOctiFen(bst) !== bad.finalFen,
              "Invalid game " + bad.gameId + " must throw or mismatch claimed finalFen");
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
    auto fakeNow = std::make_shared<std::atomic<int64_t>>(100000);
    env->rooms->setClock([fakeNow]() { return fakeNow->load(); });

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
    fakeNow->store(115000);
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
    fakeNow->store(122000);
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
    CHECK_TRUE(rIndex.body.find("Browser-Play") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("Online-Play") != std::string::npos);
    CHECK_TRUE(rIndex.body.find(">Archive</button>") != std::string::npos);
    CHECK_TRUE(rIndex.body.find(">Journal</button>") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"nav-journal-btn\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"tab-journal\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"journal-issue-2-card\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("href=\"/journal/2026-11-20/the-architecture-of-octi.html\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("src=\"/journal/2026-11-20/img/cover_architecture_of_octi.jpg\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"journal-issue-3-card\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("href=\"/journal/2026-11-20/stacks-bases-and-the-octi-system.html\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("src=\"/journal/2026-11-20/img/cover_octi_system.jpg\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"journal-issue-1-card\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("href=\"/journal/2026-11-18/g-20261118-2165.html\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("src=\"/journal/2026-11-18/img/cover_g20261118_2165.jpg\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("Play in Browser</button>") == std::string::npos);
    CHECK_TRUE(rIndex.body.find("Play Online</button>") == std::string::npos);
    CHECK_TRUE(rIndex.body.find(">Game Archive</button>") == std::string::npos);
    CHECK_TRUE(rIndex.body.find("OCTI v" + std::string(OCTI_APP_VERSION)) != std::string::npos);
    CHECK_TRUE(rIndex.body.find("Official Platform") == std::string::npos);
    CHECK_TRUE(rIndex.body.find("OFFICIAL PLATFORM") == std::string::npos);
    CHECK_TRUE(rIndex.body.find("Lv1: AI_L1 (5s)") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("Lv2: AI_L2 (10s)") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("Lv3: AI_L3 (20s)") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("Lv4: AI_L4 (40s)") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("Lv5: AI_L5 (60s)") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("Lv4: Expert") == std::string::npos);
    CHECK_TRUE(rIndex.body.find("Lv5: Grandmaster") == std::string::npos);
    CHECK_TRUE(rIndex.body.find("explore grandmaster replays") == std::string::npos);
    CHECK_TRUE(rIndex.body.find("explore archived replays") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"web-roster-entity-modal\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"web-host-hover-card\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("openOnlineEntityDialog") != std::string::npos);
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

    /* Move History moved to left column (#web-play-reserves-col); Lobby Chat in right column (#web-side-top-half) */
    size_t posReservesCol = rIndex.body.find("id=\"web-play-reserves-col\"");
    size_t posMoveHistory = rIndex.body.find("id=\"web-move-history-panel\"");
    size_t posResCardA = rIndex.body.find("id=\"web-res-card-a\"");
    size_t posSideTopHalf = rIndex.body.find("id=\"web-side-top-half\"");
    size_t posBoardLobbyChat = rIndex.body.find("id=\"web-board-lobby-chat-panel\"");
    size_t posBoardChatPanel = rIndex.body.find("id=\"web-board-chat-panel\"");
    CHECK_TRUE(posReservesCol != std::string::npos);
    CHECK_TRUE(posMoveHistory != std::string::npos);
    CHECK_TRUE(posResCardA != std::string::npos);
    CHECK_TRUE(posSideTopHalf != std::string::npos);
    CHECK_TRUE(posBoardLobbyChat != std::string::npos);
    CHECK_TRUE(posBoardChatPanel != std::string::npos);
    CHECK_TRUE(posReservesCol < posMoveHistory);
    CHECK_TRUE(posMoveHistory < posResCardA);
    CHECK_TRUE(posSideTopHalf < posBoardLobbyChat);
    CHECK_TRUE(posBoardLobbyChat < posBoardChatPanel);

    /* Account tab admin purge toggle (default off), context isolation, @bot removal, and realtime lobby sync */
    CHECK_TRUE(rIndex.body.find("id=\"account-admin-controls\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"web-admin-purge-toggle\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("var webAdminPurgeEnabled = false;") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("function isWebAdminPurgeActive()") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("function saveLocalPlayContext()") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("function restoreLocalPlayContext()") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("function saveOnlinePlayContext()") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("function restoreOnlinePlayContext()") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("function startLobbyRealtimeSync()") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("function stopLobbyRealtimeSync()") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("function appendLobbyChatEvent(ev)") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("/api/v1/lobby/events?stream=1&since_seq=") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("@bot help") == std::string::npos);
    CHECK_TRUE(rIndex.body.find("@bot eval") == std::string::npos);
    CHECK_TRUE(rIndex.body.find("@bot stats") == std::string::npos);
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

TEST(OctiHttpServerIntegrationTest, WebUiTenThemesTabletopLobbyRefinementsAndLevelOneTwoAiIgnore)
{
    Reply rIndex = http().get("/");
    EXPECT_STATUS(200, rIndex);

    // 1. All 10 themes in PALETTES and selector options
    const std::vector<std::string> kThemes = {
        "obsidian", "rosewood", "neon", "jade", "aurora",
        "forge", "candy", "pearl", "steam", "nebula"
    };
    for (const auto &th : kThemes) {
        CHECK_TRUE(rIndex.body.find(th + ": {") != std::string::npos);
        CHECK_TRUE(rIndex.body.find("<option value=\"" + th + "\"") != std::string::npos);
    }
    CHECK_TRUE(rIndex.body.find("id=\"btn-theme\"") == std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"web-play-theme-select\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"replay-theme-select\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("applyTheme(t") != std::string::npos ||
               rIndex.body.find("function applyTheme(") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("getBoardTexturePattern") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("drawPodPatternAndFinish") != std::string::npos);

    // 2. isGameActiveForPinkLine pre-game running light gate
    CHECK_TRUE(rIndex.body.find("isGameActiveForPinkLine") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("activeOnlineRoomView.phase === \"IN_PROGRESS\"") != std::string::npos);

    // 3. Match actions card in #web-side-top-half without Sit/Sit buttons & Leave Table in Table Chat
    CHECK_TRUE(rIndex.body.find("id=\"web-online-seat-actions\"") == std::string::npos);
    size_t posSideTopHalf = rIndex.body.find("id=\"web-side-top-half\"");
    size_t posBoardLobbyChat = rIndex.body.find("id=\"web-board-lobby-chat-panel\"");
    size_t posMatchActionsCard = rIndex.body.find("id=\"web-match-actions-card\"");
    size_t posBoardChatPanel = rIndex.body.find("id=\"web-board-chat-panel\"");
    size_t posLeaveTableBtn = rIndex.body.find("id=\"btn-web-leave-table\"");

    CHECK_TRUE(posSideTopHalf != std::string::npos);
    CHECK_TRUE(posBoardLobbyChat != std::string::npos);
    CHECK_TRUE(posMatchActionsCard != std::string::npos);
    CHECK_TRUE(posBoardChatPanel != std::string::npos);
    CHECK_TRUE(posLeaveTableBtn != std::string::npos);

    CHECK_TRUE(posSideTopHalf < posBoardLobbyChat);
    CHECK_TRUE(posBoardLobbyChat < posBoardChatPanel);
    CHECK_TRUE(posBoardChatPanel < posLeaveTableBtn);
    CHECK_TRUE(posLeaveTableBtn < posMatchActionsCard);

    // Zoom modal suppression guards
    CHECK_TRUE(rIndex.body.find("var tableActions = document.getElementById(\"web-online-table-actions\");") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("if (tableActions && tableActions.contains(ev.target)) return;") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("closeBoardChatZoomModal();") != std::string::npos);

    // Review-Board-only game over dialog (#btn-gameover-review only, no #btn-gameover-new)
    CHECK_TRUE(rIndex.body.find("id=\"btn-gameover-review\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"btn-gameover-new\"") == std::string::npos);

    // Footnote in start match overlay
    CHECK_TRUE(rIndex.body.find("id=\"web-start-ai-footnote\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("Browser AI runs on your PC/laptop in JavaScript.") != std::string::npos);

    // Local timezone formatting (no hardcoded UTC/toISOString)
    CHECK_TRUE(rIndex.body.find("<th>Date (UTC)</th>") == std::string::npos);
    CHECK_TRUE(rIndex.body.find("<th>Date</th>") != std::string::npos);

    // 4. Connected Users (humans) followed by Connected Robots
    size_t posUsersSec = rIndex.body.find("id=\"web-online-users-section\"");
    size_t posRobotsSec = rIndex.body.find("id=\"web-online-robots-section\"");
    CHECK_TRUE(posUsersSec != std::string::npos);
    CHECK_TRUE(posRobotsSec != std::string::npos);
    CHECK_TRUE(posUsersSec < posRobotsSec);
    CHECK_TRUE(rIndex.body.find("id=\"web-online-robots-count\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("id=\"web-online-robots-list\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("Connected Robots") != std::string::npos);

    // 5. Roster filtering logic
    CHECK_TRUE(rIndex.body.find("!p.is_robot && !p.robot_uuid") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("Boolean(p.is_robot || p.robot_uuid)") != std::string::npos);

    // 6. Client auto-upload skip for level 1 and 2
    CHECK_TRUE(rIndex.body.find("localMatchConfig.botDifficulty === 1 || localMatchConfig.botDifficulty === 2") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("res.ignored") != std::string::npos);

    // 7. Server level 1 & 2 AI upload ignore behavior (HTTP 200 with ignored: true; total count unchanged)
    Reply rList0 = http().get("/api/v1/archive/games");
    EXPECT_STATUS(200, rList0);
    int total0 = rList0.json.value("total", 0);

    // Upload Level 1 AI transcript -> HTTP 200 with ignored: true, total count unchanged
    std::string trAi1 = makeResignedTranscript({"33+a", "37+e"}, "inst-ign-01", 1);
    Reply rUpAi1 = http().post("/api/v1/archive/upload", "", Json{{"transcript", trAi1}});
    EXPECT_STATUS(200, rUpAi1);
    CHECK_TRUE(rUpAi1.json.value("ok", false));
    CHECK_TRUE(rUpAi1.json.value("ignored", false));
    std::string ignId1 = rUpAi1.json.value("game_id", "");
    CHECK_FALSE(ignId1.empty());

    Reply rList1 = http().get("/api/v1/archive/games");
    EXPECT_STATUS(200, rList1);
    CHECK_EQUAL(total0, rList1.json.value("total", 0));

    // Upload Level 2 AI transcript -> HTTP 200 with ignored: true, total count unchanged
    std::string trAi2 = makeResignedTranscript({"33+a", "37+e"}, "inst-ign-02", 2);
    Reply rUpAi2 = http().post("/api/v1/archive/upload", "", Json{{"transcript", trAi2}});
    EXPECT_STATUS(200, rUpAi2);
    CHECK_TRUE(rUpAi2.json.value("ok", false));
    CHECK_TRUE(rUpAi2.json.value("ignored", false));

    Reply rList2 = http().get("/api/v1/archive/games");
    EXPECT_STATUS(200, rList2);
    CHECK_EQUAL(total0, rList2.json.value("total", 0));

    // Upload Level 3 AI transcript -> HTTP 200 without ignored flag, stored in games table
    std::string trAi3 = makeResignedTranscript({"33+a", "37+e"}, "inst-stored-03", 3);
    Reply rUpAi3 = http().post("/api/v1/archive/upload", "", Json{{"transcript", trAi3}});
    EXPECT_STATUS(200, rUpAi3);
    CHECK_TRUE(rUpAi3.json.value("ok", false));
    CHECK_FALSE(rUpAi3.json.value("ignored", false));
    std::string storedId3 = rUpAi3.json.value("game_id", "");
    CHECK_FALSE(storedId3.empty());

    Reply rList3 = http().get("/api/v1/archive/games");
    EXPECT_STATUS(200, rList3);
    CHECK_EQUAL(total0 + 1, rList3.json.value("total", 0));

    Reply rGet3 = http().get("/api/v1/archive/games/" + storedId3);
    EXPECT_STATUS(200, rGet3);
    CHECK_EQUAL(3, rGet3.json.value("ai_difficulty", 0));
}

TEST(OctiHttpServerIntegrationTest, ArchiveListFilterByRobotUuid)
{
    User humanA = signIn("google", "sub-arch-robot-a", "ArchHumanA");
    User humanB = signIn("google", "sub-arch-robot-b", "ArchHumanB");

    Reply rBot1 = http().post(
        "/api/v1/robots", humanA.token,
        Json{{"robot_name", "arch-filter-bot-1"}, {"description", "Archive filter test bot 1"}});
    EXPECT_STATUS(201, rBot1);
    std::string bot1Uuid = rBot1.json.value("robot_uuid", "");
    std::string bot1Token = rBot1.json.value("robot_token", "");
    CHECK_FALSE(bot1Uuid.empty());
    CHECK_FALSE(bot1Token.empty());

    Reply rBot2 = http().post(
        "/api/v1/robots", humanB.token,
        Json{{"robot_name", "arch-filter-bot-2"}, {"description", "Archive filter test bot 2"}});
    EXPECT_STATUS(201, rBot2);
    std::string bot2Uuid = rBot2.json.value("robot_uuid", "");
    std::string bot2Token = rBot2.json.value("robot_token", "");
    CHECK_FALSE(bot2Uuid.empty());
    CHECK_FALSE(bot2Token.empty());

    /* Game 1: humanA (seat 1) vs bot1 (seat 2) */
    Reply rTab1 = http().post(
        "/api/v1/tables", humanA.token,
        Json{{"mode", "2P"}, {"bases", 3}, {"allow_robots", true}});
    EXPECT_STATUS(201, rTab1);
    std::string roomBot1 = rTab1.json.value("room_id", "");
    CHECK_FALSE(roomBot1.empty());

    Reply rJoin1 = http().post(
        "/api/v1/tables/" + roomBot1 + "/join", bot1Token, Json{{"seat", 2}});
    EXPECT_STATUS(200, rJoin1);
    CHECK_TRUE(rJoin1.json.value("game_started", false));

    Reply rResign1 = http().post(
        "/api/v1/games/" + roomBot1 + "/resign", humanA.token, Json::object());
    EXPECT_STATUS(200, rResign1);
    CHECK_TRUE(rResign1.json.value("game_over", false));

    /* Game 2: humanB (seat 1) vs bot2 (seat 2) */
    Reply rTab2 = http().post(
        "/api/v1/tables", humanB.token,
        Json{{"mode", "2P"}, {"bases", 3}, {"allow_robots", true}});
    EXPECT_STATUS(201, rTab2);
    std::string roomBot2 = rTab2.json.value("room_id", "");
    CHECK_FALSE(roomBot2.empty());

    Reply rJoin2 = http().post(
        "/api/v1/tables/" + roomBot2 + "/join", bot2Token, Json{{"seat", 2}});
    EXPECT_STATUS(200, rJoin2);
    CHECK_TRUE(rJoin2.json.value("game_started", false));

    Reply rResign2 = http().post(
        "/api/v1/games/" + roomBot2 + "/resign", bot2Token, Json::object());
    EXPECT_STATUS(200, rResign2);
    CHECK_TRUE(rResign2.json.value("game_over", false));

    /* Game 3: humanA vs humanB (no robot) */
    std::string roomHumans = playDrawGame(humanA, humanB);
    CHECK_FALSE(roomHumans.empty());

    /* 1. Filter by bot1 UUID -> only roomBot1 */
    Reply rListBot1 = http().get("/api/v1/archive/games?robot=" + bot1Uuid);
    EXPECT_STATUS(200, rListBot1);
    CHECK_EQUAL(1, rListBot1.json.value("total", -1));
    CHECK_TRUE(rListBot1.json["games"].is_array());
    CHECK_EQUAL(1, static_cast<int>(rListBot1.json["games"].size()));
    STRCMP_EQUAL(roomBot1.c_str(), rListBot1.json["games"][0].value("game_id", "").c_str());

    const Json &pl = rListBot1.json["games"][0]["players"];
    CHECK_TRUE(pl.is_array());
    CHECK_EQUAL(2, static_cast<int>(pl.size()));
    CHECK_FALSE(pl[0].value("is_robot", false));
    STRCMP_EQUAL(humanA.uuid.c_str(), pl[0].value("player_uuid", "").c_str());
    CHECK_TRUE(pl[1].value("is_robot", false));
    STRCMP_EQUAL(bot1Uuid.c_str(), pl[1].value("robot_uuid", "").c_str());

    /* 2. Filter by unknown robot UUID -> empty page */
    Reply rListUnknown = http().get(
        "/api/v1/archive/games?robot=00000000-0000-0000-0000-000000000000");
    EXPECT_STATUS(200, rListUnknown);
    CHECK_EQUAL(0, rListUnknown.json.value("total", -1));
    CHECK_TRUE(rListUnknown.json["games"].is_array());
    CHECK_EQUAL(0, static_cast<int>(rListUnknown.json["games"].size()));

    /* 3. Filter by player + robot seated together -> roomBot1; non-matching pair -> empty page */
    Reply rPlayerAndBot1 = http().get(
        "/api/v1/archive/games?player=" + humanA.uuid + "&robot=" + bot1Uuid);
    EXPECT_STATUS(200, rPlayerAndBot1);
    CHECK_EQUAL(1, rPlayerAndBot1.json.value("total", -1));
    CHECK_EQUAL(1, static_cast<int>(rPlayerAndBot1.json["games"].size()));
    STRCMP_EQUAL(roomBot1.c_str(), rPlayerAndBot1.json["games"][0].value("game_id", "").c_str());

    Reply rOtherPlayerAndBot1 = http().get(
        "/api/v1/archive/games?player=" + humanB.uuid + "&robot=" + bot1Uuid);
    EXPECT_STATUS(200, rOtherPlayerAndBot1);
    CHECK_EQUAL(0, rOtherPlayerAndBot1.json.value("total", -1));
    CHECK_EQUAL(0, static_cast<int>(rOtherPlayerAndBot1.json["games"].size()));

    /* 4. Invalid robot id -> 400 INVALID_ARGUMENT */
    Reply rBadRobot = http().get("/api/v1/archive/games?robot=bad!id");
    EXPECT_ERROR(400, "INVALID_ARGUMENT", rBadRobot);
}

TEST(OctiHttpServerIntegrationTest, RobotRenameRateLimitArchiveAndUrlValidation)
{
    User u1 = signIn("google", "sub-url-test-1", "UrlTestUser1");
    User u2 = signIn("google", "sub-url-test-2", "UrlTestUser2");

    // 1. Invalid robot URLs rejected upon creation
    Reply rQuery = http().post(
        "/api/v1/robots", u1.token,
        Json{{"robot_name", "bot-url-q"},
             {"robot_url", "https://octi.selfso.com/bot?track=1"}});
    EXPECT_ERROR(400, "INVALID_ROBOT_URL", rQuery);

    Reply rUserinfo = http().post(
        "/api/v1/robots", u1.token,
        Json{{"robot_name", "bot-url-u"},
             {"robot_url", "https://user@octi.selfso.com/"}});
    EXPECT_ERROR(400, "INVALID_ROBOT_URL", rUserinfo);

    Reply rSsrfMeta = http().post(
        "/api/v1/robots", u1.token,
        Json{{"robot_name", "bot-url-meta"},
             {"robot_url", "http://169.254.169.254/latest/meta-data"}});
    EXPECT_ERROR(400, "INVALID_ROBOT_URL", rSsrfMeta);

    Reply rSsrfPriv = http().post(
        "/api/v1/robots", u1.token,
        Json{{"robot_name", "bot-url-priv"},
             {"robot_url", "http://10.0.0.1/secret"}});
    EXPECT_ERROR(400, "INVALID_ROBOT_URL", rSsrfPriv);

    // Loopback strictly rejected when OCTI_ALLOW_LOOPBACK_ROBOT_URL_FOR_TEST is unset
    unsetenv("OCTI_ALLOW_LOOPBACK_ROBOT_URL_FOR_TEST");
    Reply rLoopbackDefault = http().post(
        "/api/v1/robots", u1.token,
        Json{{"robot_name", "bot-url-loop"},
             {"robot_url", "http://127.0.0.1:" + std::to_string(http().port()) + "/"}});
    EXPECT_ERROR(400, "INVALID_ROBOT_URL", rLoopbackDefault);

    // With loopback test override: 404 is rejected, 200 is accepted
    setenv("OCTI_ALLOW_LOOPBACK_ROBOT_URL_FOR_TEST", "1", 1);
    Reply rNotFound = http().post(
        "/api/v1/robots", u1.token,
        Json{{"robot_name", "bot-url-404"},
             {"robot_url", "http://127.0.0.1:" + std::to_string(http().port()) + "/api/v1/nonexistent-route-for-test"}});
    EXPECT_ERROR(400, "INVALID_ROBOT_URL", rNotFound);

    std::string validUrl = "http://127.0.0.1:" + std::to_string(http().port()) + "/api/v1/archive/games";
    Reply rCreateOk = http().post(
        "/api/v1/robots", u1.token,
        Json{{"robot_name", "bot-url-ok"},
             {"description", "Bot with valid URL"},
             {"robot_url", validUrl}});
    EXPECT_STATUS(201, rCreateOk);
    std::string bot1Uuid = rCreateOk.json.value("robot_uuid", "");
    std::string bot1Token = rCreateOk.json.value("robot_token", "");
    CHECK_FALSE(bot1Uuid.empty());
    CHECK_FALSE(bot1Token.empty());
    STRCMP_EQUAL(validUrl.c_str(), rCreateOk.json.value("robot_url", "").c_str());

    Reply rGetRobots = http().get("/api/v1/robots", u1.token);
    EXPECT_STATUS(200, rGetRobots);
    CHECK_TRUE(rGetRobots.json.is_object());
    CHECK_TRUE(rGetRobots.json["robots"].is_array());
    CHECK_TRUE(rGetRobots.json["robots"].size() >= 1);
    STRCMP_EQUAL(validUrl.c_str(), rGetRobots.json["robots"][0].value("robot_url", "").c_str());

    // 2. Editable robot name with 24-hour cooldown
    Reply rRenameOk = http().patch(
        "/api/v1/robots/" + bot1Uuid, u1.token,
        Json{{"robot_name", "bot-renamed-1"}});
    EXPECT_STATUS(200, rRenameOk);
    STRCMP_EQUAL("bot-renamed-1", rRenameOk.json.value("robot_name", "").c_str());
    CHECK_TRUE(rRenameOk.json.value("name_changed_at", 0) > 0);

    // Immediate second rename returns 429 ROBOT_NAME_RATE_LIMITED
    Reply rRenameRateLimited = http().patch(
        "/api/v1/robots/" + bot1Uuid, u1.token,
        Json{{"robot_name", "bot-renamed-2"}});
    EXPECT_ERROR(429, "ROBOT_NAME_RATE_LIMITED", rRenameRateLimited);

    // u2 creates another robot
    Reply rBot2 = http().post(
        "/api/v1/robots", u2.token,
        Json{{"robot_name", "bot-u2-orig"}, {"description", "u2 bot"}});
    EXPECT_STATUS(201, rBot2);
    std::string bot2Uuid = rBot2.json.value("robot_uuid", "");

    // Renaming to already taken name returns 409 ROBOT_NAME_TAKEN
    Reply rRenameTaken = http().patch(
        "/api/v1/robots/" + bot2Uuid, u2.token,
        Json{{"robot_name", "bot-renamed-1"}});
    EXPECT_ERROR(409, "ROBOT_NAME_TAKEN", rRenameTaken);

    // Clearing robot_url by passing empty string is accepted immediately
    Reply rClearUrl = http().patch(
        "/api/v1/robots/" + bot1Uuid, u1.token,
        Json{{"robot_url", ""}});
    EXPECT_STATUS(200, rClearUrl);
    STRCMP_EQUAL("", rClearUrl.json.value("robot_url", "NON_EMPTY").c_str());

    unsetenv("OCTI_ALLOW_LOOPBACK_ROBOT_URL_FOR_TEST");

    // 3. Play a match with bot-renamed-1 and verify archive query reflection by name
    Reply rTab = http().post(
        "/api/v1/tables", u1.token,
        Json{{"mode", "2P"}, {"bases", 3}, {"allow_robots", true}});
    EXPECT_STATUS(201, rTab);
    std::string roomId = rTab.json.value("room_id", "");
    CHECK_FALSE(roomId.empty());

    Reply rJoin = http().post(
        "/api/v1/tables/" + roomId + "/join", bot1Token, Json{{"seat", 2}});
    EXPECT_STATUS(200, rJoin);

    Reply rResign = http().post(
        "/api/v1/games/" + roomId + "/resign", u1.token, Json::object());
    EXPECT_STATUS(200, rResign);

    Reply rArchByName = http().get("/api/v1/archive/games?robot=bot-renamed-1");
    EXPECT_STATUS(200, rArchByName);
    CHECK_EQUAL(1, rArchByName.json.value("total", 0));
    CHECK_TRUE(rArchByName.json["games"].is_array());
    CHECK_EQUAL(1, static_cast<int>(rArchByName.json["games"].size()));
    STRCMP_EQUAL(roomId.c_str(), rArchByName.json["games"][0].value("game_id", "").c_str());

    const Json &players = rArchByName.json["games"][0]["players"];
    CHECK_EQUAL(2, static_cast<int>(players.size()));
    STRCMP_EQUAL("bot-renamed-1", players[1].value("handle", "").c_str());
    STRCMP_EQUAL(bot1Uuid.c_str(), players[1].value("robot_uuid", "").c_str());
}

TEST(OctiHttpServerIntegrationTest, WebUiJumpConfirmAndCaptureOverlaysDodgeTargetSquaresAndAreDraggable)
{
    Reply rPlay = http().get("/");
    EXPECT_STATUS(200, rPlay);
    CHECK_TRUE(rPlay.body.find("autoPositionConfirmBar") != std::string::npos);
    CHECK_TRUE(rPlay.body.find("autoPositionCaptureCard") != std::string::npos);
    CHECK_TRUE(rPlay.body.find("makeWebOverlayDraggable") != std::string::npos);
    CHECK_TRUE(rPlay.body.find("web-drag-grip") != std::string::npos);
    CHECK_TRUE(rPlay.body.find("_confirmBarUserDragged") != std::string::npos);
    CHECK_TRUE(rPlay.body.find("_captureCardUserDragged") != std::string::npos);
}

TEST(OctiHttpServerIntegrationTest, WebUiStrengthenedBotSearchAndSuperProngBonusServed)
{
    Reply rPlay = http().get("/");
    EXPECT_STATUS(200, rPlay);
    CHECK_TRUE(rPlay.body.find("dfsSuperBonusJumps") != std::string::npos);
    CHECK_TRUE(rPlay.body.find("enumerateSuperBonusPreludes") != std::string::npos);
    CHECK_TRUE(rPlay.body.find("generateBaseMoves") != std::string::npos);
    CHECK_TRUE(rPlay.body.find("isTacticalMove") != std::string::npos);
    CHECK_TRUE(rPlay.body.find("quiescenceSearch") != std::string::npos);
    CHECK_TRUE(rPlay.body.find("computeBestMove") != std::string::npos);
    CHECK_TRUE(rPlay.body.find("ctx.qttMap") != std::string::npos);
    CHECK_TRUE(rPlay.body.find("ctx.killers") != std::string::npos);
    CHECK_TRUE(rPlay.body.find("ctx.history") != std::string::npos);
}

TEST(OctiHttpServerIntegrationTest, SharedSoundAssetsEndpointAndWebPlayerMoveCues)
{
    /* 1. Nominal HTTP GET /sounds/:file byte-for-byte identity against OctiSoundCue */
    Json expectedWavHex = Json::object();
    for (std::size_t i = 0; i < OCTI_SOUND_CUE_COUNT; ++i) {
        SoundCue cue = static_cast<SoundCue>(i);
        const char *cueName = soundCueName(cue);
        const char *fileName = soundCueAssetFileName(cue);
        std::size_t wavSize = 0;
        const std::uint8_t *wavBytes = soundCueWavBytes(cue, wavSize);
        CHECK_TRUE(wavBytes != nullptr);
        CHECK_TRUE(wavSize >= 44u);

        Reply rWav = http().get(std::string("/sounds/") + fileName);
        EXPECT_STATUS(200, rWav);
        CHECK_TRUE(rWav.contentType.find("audio/wav") != std::string::npos);
        STRCMP_EQUAL("public, max-age=86400", rWav.header("Cache-Control").c_str());
        LONGS_EQUAL(static_cast<long>(wavSize), static_cast<long>(rWav.body.size()));
        CHECK_EQUAL(0, std::memcmp(rWav.body.data(), wavBytes, wavSize));

        static const char kHexDigits[] = "0123456789abcdef";
        std::string hex;
        hex.reserve(wavSize * 2u);
        for (std::size_t b = 0; b < wavSize; ++b) {
            hex.push_back(kHexDigits[(wavBytes[b] >> 4) & 0x0Fu]);
            hex.push_back(kHexDigits[wavBytes[b] & 0x0Fu]);
        }
        expectedWavHex[cueName] = hex;
    }

    /* 2. Fault & path-traversal rejection on GET /sounds/:file */
    EXPECT_ERROR(404, "NOT_FOUND", http().get("/sounds/nonexistent.wav"));
    EXPECT_ERROR(404, "NOT_FOUND", http().get("/sounds/..%2fetc%2fpasswd"));
    EXPECT_ERROR(404, "NOT_FOUND", http().get("/sounds/prong_snap.mp3"));
    EXPECT_STATUS(404, http().get("/sounds/.."));
    EXPECT_STATUS(404, http().get("/sounds/"));

    /* 3. Strict CSP self-containment & media-src policy on GET / */
    Reply rUi = http().get("/");
    EXPECT_STATUS(200, rUi);
    CHECK_TRUE(rUi.header("Content-Security-Policy").find("media-src 'self' data:") !=
               std::string::npos);
    {
        std::string strippedUi = rUi.body;
        const char *kAllowedTokens[] = {
            "http://www.w3.org/2000/svg",
            "https://www.selfso.com/software/windows/octi/",
            "https://www.selfso.com/software/android/octi/",
            "placeholder=\"Robot URL (optional http:// or https://, no '?')\"",
            "<link rel=\"icon\"",
            "<link rel=\"apple-touch-icon\""
        };
        for (size_t ti = 0; ti < sizeof(kAllowedTokens) / sizeof(kAllowedTokens[0]); ++ti) {
            const std::string tok(kAllowedTokens[ti]);
            size_t pos = 0;
            while ((pos = strippedUi.find(tok)) != std::string::npos) {
                strippedUi.replace(pos, tok.size(), "");
            }
        }
        CHECK_TRUE(strippedUi.find("http://") == std::string::npos);
        CHECK_TRUE(strippedUi.find("https://") == std::string::npos);
        CHECK_TRUE(strippedUi.find("src=\"//") == std::string::npos);
        CHECK_TRUE(strippedUi.find("<link") == std::string::npos);
        CHECK_TRUE(strippedUi.find("<iframe") == std::string::npos);
    }

    /* 4. Prepare two real online rooms on octi_server for Node.js syncOnlineGameRoom verification:
     *    - prePopulatedRoom: already has 6 plies committed (initial sync must fire 0 cues).
     *    - liveRoom: 2P 1-base match at ply 0 for live human echo suppression + opponent move + victory cues.
     */
    User uA = signIn("google", "sub-sfx-alice", "SfxAlice");
    User uB = signIn("apple", "sub-sfx-bob", "SfxBob");
    std::vector<User> pairUsers = {uA, uB};
    std::string prePopulatedRoom = startGame(pairUsers, Json{{"mode", "2P"}, {"bases", 3}});
    CHECK_EQUAL(6, playPlies(prePopulatedRoom, pairUsers, GAME_MODE_2, 6,
                             Json{{"mode", "2P"}, {"bases", 3}}));

    User uC = signIn("google", "sub-sfx-carol", "SfxCarol");
    User uD = signIn("apple", "sub-sfx-dave", "SfxDave");
    std::vector<User> liveUsers = {uC, uD};
    std::string liveRoom = startGame(liveUsers, Json{{"mode", "2P"}, {"bases", 1}});

    size_t scStart = rUi.body.find("<script>");
    CHECK_TRUE(scStart != std::string::npos);
    size_t scEnd = rUi.body.find("</script>", scStart);
    CHECK_TRUE(scEnd != std::string::npos);
    std::string jsCode = rUi.body.substr(scStart + 8, scEnd - (scStart + 8));

    std::string pidTag = std::to_string(getpid());
    std::string webJsPath = "/tmp/octi_sfx_webui_" + pidTag + ".js";
    std::string cfgJsonPath = "/tmp/octi_sfx_cfg_" + pidTag + ".json";
    std::string runnerJsPath = "/tmp/octi_sfx_runner_" + pidTag + ".js";

    {
        std::ofstream ofs(webJsPath);
        ofs << jsCode;
    }
    {
        Json cfg;
        cfg["port"] = http().port();
        cfg["expectedWavHex"] = expectedWavHex;
        cfg["prePopulatedRoom"] = prePopulatedRoom;
        cfg["liveRoom"] = liveRoom;
        cfg["seat1Token"] = uC.token;
        cfg["seat1Uuid"] = uC.uuid;
        cfg["seat1Handle"] = uC.handle;
        cfg["seat2Token"] = uD.token;
        std::ofstream cfs(cfgJsonPath);
        cfs << cfg.dump();
    }
    {
        std::ofstream rfs(runnerJsPath);
        rfs << R"NODEJS(
const assert = require("assert");
const crypto = require("crypto");
const fs = require("fs");
const vm = require("vm");

const webJsRaw = fs.readFileSync(process.argv[2], "utf8");
const cfg = JSON.parse(fs.readFileSync(process.argv[3], "utf8"));

const unhandledRejections = [];
process.on("unhandledRejection", (reason) => {
  unhandledRejections.push(String(reason));
});

const iifeClose = webJsRaw.lastIndexOf("})();");
assert.ok(iifeClose > 0, "Expected closing })(); in WebUi script");
const instrumentedJs =
  webJsRaw.slice(0, iifeClose) +
  "\nwindow.__octiTest = {\n" +
  "  parseOctiFen: parseOctiFen,\n" +
  "  applyMoveToState: applyMoveToState,\n" +
  "  cloneState: cloneState,\n" +
  "  clockTimers: clockTimers\n" +
  "};\n})();\n";

const canvas2dProxy = new Proxy({
  createLinearGradient: () => ({ addColorStop() {} }),
  createRadialGradient: () => ({ addColorStop() {} }),
  createPattern: () => ({}),
  measureText: () => ({ width: 20 })
}, {
  get(target, prop) {
    if (prop in target) return target[prop];
    return () => {};
  },
  set(target, prop, val) {
    target[prop] = val;
    return true;
  }
});

class MockNode {}
const elements = {};
function makeElement(id) {
  const elObj = Object.assign(new MockNode(), {
    id: id || "",
    tagName: "DIV",
    textContent: "",
    innerHTML: "",
    value: "",
    checked: false,
    disabled: false,
    className: "",
    width: 600,
    height: 600,
    clientWidth: 600,
    clientHeight: 600,
    offsetWidth: 600,
    offsetHeight: 600,
    scrollHeight: 0,
    scrollTop: 0,
    firstChild: null,
    parentNode: null,
    style: {
      setProperty(k, v) { this[k] = v; },
      removeProperty(k) { delete this[k]; },
      getPropertyValue(k) { return this[k] || ""; }
    },
    dataset: {},
    listeners: {},
    addEventListener(ev, fn, opts) {
      if (!this.listeners[ev]) this.listeners[ev] = [];
      this.listeners[ev].push({ fn, opts });
    },
    removeEventListener() {},
    click() {
      const arr = this.listeners["click"] || [];
      for (const item of arr) item.fn.call(this, { preventDefault() {}, stopPropagation() {}, target: this });
    },
    classList: {
      add() {},
      remove() {},
      toggle() {},
      contains() { return false; }
    },
    appendChild(ch) { return ch; },
    insertBefore(ch) { return ch; },
    removeChild(ch) { return ch; },
    replaceChild(ch) { return ch; },
    cloneNode() { return makeElement(""); },
    focus() {},
    blur() {},
    select() {},
    scrollIntoView() {},
    contains() { return false; },
    getAttribute() { return null; },
    setAttribute() {},
    removeAttribute() {},
    querySelector() { return makeElement(""); },
    querySelectorAll() { return []; },
    getBoundingClientRect() {
      return { left: 0, top: 0, right: 600, bottom: 600, width: 600, height: 600 };
    },
      getContext() { return canvas2dProxy; }
  });
  return elObj;
}

const storageMap = new Map();
const localStorageShim = {
  getItem(k) { return storageMap.has(k) ? storageMap.get(k) : null; },
  setItem(k, v) { storageMap.set(String(k), String(v)); },
  removeItem(k) { storageMap.delete(k); },
  clear() { storageMap.clear(); }
};

let nextIntervalId = 0;
const activeIntervals = new Map();
const windowListeners = {};
const documentListeners = {};

let decodedInputs = [];
let startedBufferSources = [];
class MockRunningAudioContext {
  constructor() {
    this.state = "running";
    this.destination = { kind: "destination" };
  }
  resume() {
    this.state = "running";
    return Promise.resolve();
  }
  decodeAudioData(ab, okCb) {
    assert.ok(ab instanceof ArrayBuffer || (ab && typeof ab.byteLength === "number"),
              "decodeAudioData must receive an ArrayBuffer");
    const u8 = new Uint8Array(ab);
    assert.strictEqual(String.fromCharCode(u8[0], u8[1], u8[2], u8[3]), "RIFF");
    assert.strictEqual(String.fromCharCode(u8[8], u8[9], u8[10], u8[11]), "WAVE");
    const sampleRate = u8[24] | (u8[25] << 8) | (u8[26] << 16) | (u8[27] << 24);
    assert.strictEqual(sampleRate, 16000, "Canonical shared WAV sampleRate must be 16000 Hz");
    const dataBytes = u8[40] | (u8[41] << 8) | (u8[42] << 16) | (u8[43] << 24);
    decodedInputs.push(u8.length);
    /* Simulate real WebAudio neutering/transferring input ArrayBuffer via structuredClone transfer */
    try { structuredClone(ab, { transfer: [ab] }); } catch (_) {}
    const fakeBuf = { sampleRate, length: dataBytes / 2, numberOfChannels: 1 };
    if (typeof okCb === "function") okCb(fakeBuf);
    return Promise.resolve(fakeBuf);
  }
  createBufferSource() {
    const node = {
      buffer: null,
      connectedTo: null,
      started: false,
      connect(dest) { this.connectedTo = dest; },
      start() {
        this.started = true;
        startedBufferSources.push(this);
      }
    };
    return node;
  }
}

const htmlAudioCreated = [];
const htmlAudioPlayed = [];
class MockHtmlAudio {
  constructor(src) {
    this.src = src || "";
    this.preload = "none";
    this.paused = true;
    this.ended = false;
    this.currentTime = 0;
    htmlAudioCreated.push(this);
  }
  play() {
    this.paused = false;
    htmlAudioPlayed.push(this.src);
    this.paused = true;
    this.ended = true;
    return Promise.resolve();
  }
}

const sandbox = {
  console,
  Date,
  Math,
  JSON,
  Array,
  Object,
  String,
  Number,
  Boolean,
  RegExp,
  Error,
  Map,
  Set,
  BigInt,
  Promise,
  Uint8Array,
  Int16Array,
  Float32Array,
  ArrayBuffer,
  DataView,
  URL,
  URLSearchParams,
  Blob,
  FormData,
  Headers,
  AbortController,
  crypto,
  Node: MockNode,
  parseInt,
  parseFloat,
  isNaN,
  isFinite,
  encodeURIComponent,
  decodeURIComponent,
  atob: (b64) => Buffer.from(b64, "base64").toString("binary"),
  btoa: (bin) => Buffer.from(bin, "binary").toString("base64"),
  localStorage: localStorageShim,
  location: {
    hash: "",
    pathname: "/",
    search: "",
    origin: "http://127.0.0.1:" + cfg.port,
    href: "http://127.0.0.1:" + cfg.port + "/"
  },
  history: { replaceState() {}, pushState() {} },
  navigator: { clipboard: { writeText: () => Promise.resolve() }, userAgent: "OctiNodeTest/1.0" },
  devicePixelRatio: 1,
  innerWidth: 1280,
  innerHeight: 800,
  AudioContext: MockRunningAudioContext,
  Audio: MockHtmlAudio,
  EventSource: function () { this.close = function () {}; },
  Image: function () {},
  alert() {},
  confirm() { return true; },
  prompt() { return ""; },
  open() { return null; },
  requestAnimationFrame() { return 0; },
  setTimeout() { return 0; },
  clearTimeout() {},
  setInterval(fn, ms) {
    const id = ++nextIntervalId;
    activeIntervals.set(id, { fn, ms });
    return id;
  },
  clearInterval(id) {
    activeIntervals.delete(id);
  },
  addEventListener(ev, fn, opts) {
    if (!windowListeners[ev]) windowListeners[ev] = [];
    windowListeners[ev].push({ fn, opts });
  },
  removeEventListener() {},
  fetch(url, opts) {
    const target = (typeof url === "string" && url.startsWith("/"))
      ? ("http://127.0.0.1:" + cfg.port + url)
      : url;
    return fetch(target, opts);
  },
  document: {
    body: makeElement("body"),
    documentElement: makeElement("html"),
    activeElement: null,
    hidden: false,
    visibilityState: "visible",
    getElementById(id) {
      if (!elements[id]) elements[id] = makeElement(id);
      return elements[id];
    },
    createElement(tag) {
      const e = makeElement("");
      e.tagName = String(tag || "DIV").toUpperCase();
      return e;
    },
    createElementNS(ns, tag) {
      const e = makeElement("");
      e.namespaceURI = String(ns || "");
      e.tagName = String(tag || "SVG").toUpperCase();
      return e;
    },
    createTextNode(txt) {
      return Object.assign(new MockNode(), {
        nodeType: 3,
        textContent: txt == null ? "" : String(txt),
        parentNode: null
      });
    },
    querySelector(sel) { return makeElement(sel); },
    querySelectorAll() { return []; },
    addEventListener(ev, fn, opts) {
      if (!documentListeners[ev]) documentListeners[ev] = [];
      documentListeners[ev].push({ fn, opts });
    },
    removeEventListener() {}
  }
};
sandbox.window = sandbox;
sandbox.self = sandbox;

vm.runInNewContext(instrumentedJs, sandbox, { filename: "WebUi.js", timeout: 4000 });

function sleepMs(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

let currentTestStep = "boot";
function markStep(name) {
  currentTestStep = name;
  if (process.env.OCTI_SFX_DEBUG) {
    console.error("[SFX-STEP] " + name);
  }
}

(async () => {
  markStep("initial-sleep");
  await sleepMs(40);
  markStep("sec-A");

  const synth = sandbox.OctiWebSoundSynth;
  assert.ok(synth, "OctiWebSoundSynth must be defined");
  assert.strictEqual(sandbox.window.OctiWebSoundSynth, synth, "OctiWebSoundSynth must be exported on window");
  assert.strictEqual(typeof synth.getWavBytes, "function", "getWavBytes must exist");
  assert.strictEqual(typeof synth.unlockAudio, "function", "unlockAudio must exist");
  assert.strictEqual(typeof synth.classifyMoveCue, "function", "classifyMoveCue must exist");
  assert.strictEqual(typeof synth.playForMove, "function", "playForMove must exist");
  assert.strictEqual(typeof synth.triggeredCueCount, "function", "triggeredCueCount must exist");
  assert.strictEqual(typeof synth.lastTriggeredCue, "function", "lastTriggeredCue must exist");

  /* A. Byte-for-byte identity of OctiWebSoundSynth.getWavBytes(cue) with C++ soundCueWavBytes */
  const cueNames = Object.keys(cfg.expectedWavHex);
  assert.strictEqual(cueNames.length, 7);
  for (const cue of cueNames) {
    const wavU8 = synth.getWavBytes(cue);
    assert.ok(wavU8 instanceof sandbox.Uint8Array || Uint8Array.prototype.isPrototypeOf(wavU8) || wavU8.byteLength > 0);
    const actualHex = Buffer.from(wavU8).toString("hex");
    assert.strictEqual(actualHex, cfg.expectedWavHex[cue], "WAV byte mismatch for cue " + cue);
  }
  /* POD_DROP alias normalizes to POD_PLACE */
  assert.strictEqual(
    Buffer.from(synth.getWavBytes("POD_DROP")).toString("hex"),
    cfg.expectedWavHex["POD_PLACE"]
  );
  assert.strictEqual(synth.getWavBytes("NONEXISTENT_CUE").length, 0);

  /* B. Passive gesture unlock listeners & non-detaching pre-decode of all 7 WAV buffers */
  for (const evName of ["pointerdown", "keydown", "touchstart"]) {
    const reg = (windowListeners[evName] || []).concat(documentListeners[evName] || []);
    assert.ok(
      reg.some((item) => item.opts && item.opts.passive === true && item.opts.capture === true),
      "Expected passive capture unlock listener on " + evName
    );
  }
  synth.unlockAudio();
  await sleepMs(20);
  assert.strictEqual(decodedInputs.length, 7, "unlockAudio must pre-decode all 7 shared WAV assets");
  /* Verify decodeAudioData neutering did NOT detach cached getWavBytes(cue) */
  assert.strictEqual(
    Buffer.from(synth.getWavBytes("POD_PLACE")).toString("hex"),
    cfg.expectedWavHex["POD_PLACE"],
    "getWavBytes must remain intact after decodeAudioData transfers ArrayBuffer"
  );

  /* C. POD_DROP normalization + WebAudio playback */
  const c0 = synth.triggeredCueCount();
  const src0 = startedBufferSources.length;
  synth.play("POD_DROP");
  await sleepMs(10);
  assert.strictEqual(synth.triggeredCueCount(), c0 + 1);
  assert.strictEqual(synth.lastTriggeredCue(), "POD_PLACE");
  assert.strictEqual(startedBufferSources.length, src0 + 1);

  /* D. SFX: Off suppression & localStorage ("octi.sfxEnabled") persistence */
  synth.setEnabled(false);
  assert.strictEqual(synth.isEnabled(), false);
  assert.strictEqual(localStorageShim.getItem("octi.sfxEnabled"), "0");
  assert.strictEqual(elements["btn-sfx"].textContent, "SFX: Off");
  const mutedCountBefore = synth.triggeredCueCount();
  synth.play("PRONG_SNAP");
  synth.playForMove("57+e");
  assert.strictEqual(synth.triggeredCueCount(), mutedCountBefore, "Muted synth must suppress all cues");

  synth.setEnabled(true);
  assert.strictEqual(synth.isEnabled(), true);
  assert.strictEqual(localStorageShim.getItem("octi.sfxEnabled"), "1");
  assert.strictEqual(elements["btn-sfx"].textContent, "SFX: On");

  /* E. classifyMoveCue priority rules (regular, jump, capture, superprong prelude, edgeless wrap) */
  assert.strictEqual(synth.classifyMoveCue("57"), "POD_PLACE");
  assert.strictEqual(synth.classifyMoveCue("57*"), "POD_PLACE");
  assert.strictEqual(synth.classifyMoveCue("57+e"), "PRONG_SNAP");
  assert.strictEqual(synth.classifyMoveCue("57e+f-e"), "POD_PLACE");
  assert.strictEqual(synth.classifyMoveCue("57e-e+f"), "POD_PLACE");
  assert.strictEqual(synth.classifyMoveCue("57e-56"), "POD_STEP");
  assert.strictEqual(synth.classifyMoveCue("57e-55"), "POD_JUMP");
  assert.strictEqual(synth.classifyMoveCue("57e-55x"), "POD_CAPTURE");
  assert.strictEqual(synth.classifyMoveCue("33A-34;34+b"), "PRONG_SNAP");
  assert.strictEqual(synth.classifyMoveCue("33A-35x;35+b"), "POD_CAPTURE");
  assert.strictEqual(synth.classifyMoveCue("33a!c!e!-34;34a!c!e!-44;44a!c!e!-43;43a!c!e!+b"), "PRONG_SNAP");
  assert.strictEqual(synth.classifyMoveCue("33a!c!e!g!-34;34a!c!e!g!-44;44a!c!e!g!-43;43a!c!e!g!-33;33a!c!e!g!+b"), "PRONG_SNAP");
  assert.strictEqual(synth.classifyMoveCue("33a!-34;34a!-35;35a!-36;36a!-37;37a!-38;38a!+b"), "POD_PLACE");
  assert.strictEqual(synth.classifyMoveCue("15g-85", { edgeless: true, gw: 9, gh: 9 }), "POD_JUMP");
  assert.strictEqual(synth.classifyMoveCue(""), "POD_PLACE");

  const t = sandbox.window.__octiTest;
  assert.ok(t, "__octiTest helpers must be exported");

  /* F. Local vs. AI bot move cues in onLocalMoveCommitted (and no double-play on human move) */
  markStep("sec-F");
  sandbox.window.stopOnlineGameSync(false);
  sandbox.window.localMatchConfig.playMode = "bot";
  sandbox.window.localMatchConfig.humanSeat = "A";
  sandbox.window.matchStarted = true;

  /* Human move commit without aiMeta must NOT fire duplicate cue inside onLocalMoveCommitted */
  sandbox.window.localMatchConfig.playMode = "pass";
  sandbox.window.activePlayState = t.parseOctiFen("2:1:0:0:P:A:-:0:4,0,25,0:4,0,25,0:A33,A53,A73,B37,B57,B77");
  const beforeHumanLocal = synth.triggeredCueCount();
  const afterHumanSt = t.applyMoveToState(t.cloneState(sandbox.window.activePlayState), "53+a", "A");
  sandbox.window.onLocalMoveCommitted("53+a", afterHumanSt);
  assert.strictEqual(synth.triggeredCueCount(), beforeHumanLocal, "Human local move must not double-play in onLocalMoveCommitted");

  /* AI bot moves (with aiMeta) trigger exact move cues */
  sandbox.window.localMatchConfig.playMode = "bot";
  sandbox.window.localMatchConfig.humanSeat = "A";
  const fakeAiMeta = { score: 12, depth: 4, nodes: 500, elapsedMs: 25 };

  const aiCases = [
    {
      fen: "2:3:0:0:P:B:-:0:4,0,24,0:4,0,25,0:A33,A53a,A73,B37,B57,B77",
      move: "57+e",
      expectedCue: "PRONG_SNAP"
    },
    {
      fen: "2:3:0:0:P:B:-:0:4,0,24,0:4,0,24,0:A33,A53a,A73,B37,B57e,B77",
      move: "57e-56",
      expectedCue: "POD_STEP"
    },
    {
      fen: "2:3:0:0:P:B:-:0:4,0,24,0:4,0,24,0:A33,A56,A73,B37,B57e,B77",
      move: "57e-55",
      expectedCue: "POD_JUMP"
    },
    {
      fen: "2:3:0:0:P:B:-:0:4,0,24,0:4,0,24,0:A33,A56,A73,B37,B57e,B77",
      move: "57e-55x",
      expectedCue: "POD_CAPTURE"
    },
    {
      fen: "2:3:0:0:P:B:-:0:4,0,24,0:4,0,24,0:A33,A53a,A73,B37,B77",
      move: "57",
      expectedCue: "POD_PLACE"
    },
    {
      fen: "2:1:0:0:P:B:-:0:4,0,24,0:4,0,24,0:A33,A73,B37,B54e,B77",
      move: "54e-53",
      expectedCue: "GAME_VICTORY"
    }
  ];

  for (const tc of aiCases) {
    sandbox.window.activePlayState = t.parseOctiFen(tc.fen);
    const nextSt = t.applyMoveToState(t.cloneState(sandbox.window.activePlayState), tc.move, "B");
    const cntBefore = synth.triggeredCueCount();
    sandbox.window.onLocalMoveCommitted(tc.move, nextSt, fakeAiMeta);
    assert.strictEqual(synth.triggeredCueCount(), cntBefore + 1,
                       "AI move " + tc.move + " must trigger exactly 1 cue");
    assert.strictEqual(synth.lastTriggeredCue(), tc.expectedCue,
                       "AI move " + tc.move + " expected " + tc.expectedCue);
  }

  /* G. Online Room Sync against REAL octi_server */
  markStep("sec-G1");
  /* G1: Initial sync on prePopulatedRoom (6 existing moves) must trigger 0 cues */
  localStorageShim.setItem("octi_jwt_token", cfg.seat1Token);
  const beforePrePopOpen = synth.triggeredCueCount();
  sandbox.window.openOnlineTableBoard(cfg.prePopulatedRoom);
  await sleepMs(100);
  assert.strictEqual(sandbox.window.playMatchPlies.length, 7, "Expected initial setup + 6 existing plies");
  assert.strictEqual(synth.triggeredCueCount(), beforePrePopOpen,
                     "Initial syncOnlineGameRoom on room with 6 moves must fire 0 cues");

  /* G2: Switch to liveRoom (2P 1-base) as Seat 1 (uC) */
  markStep("sec-G2");
  sandbox.window.stopOnlineGameSync(false);
  const beforeLiveOpen = synth.triggeredCueCount();
  sandbox.window.openOnlineTableBoard(cfg.liveRoom);
  await sleepMs(100);
  assert.strictEqual(synth.triggeredCueCount(), beforeLiveOpen,
                     "Initial sync on fresh online room must fire 0 cues");

  /* G3: Seat 1 (local human) commits "53+a" via onLocalMoveCommitted -> own echo must fire 0 cues */
  markStep("sec-G3");
  const beforeOwnOnline = synth.triggeredCueCount();
  const sAfterOwn1 = t.applyMoveToState(t.cloneState(sandbox.window.activePlayState), "53+a", "A");
  sandbox.window.onLocalMoveCommitted("53+a", sAfterOwn1);
  await sleepMs(120);
  assert.strictEqual(sandbox.window.playMatchPlies.length, 2);
  assert.strictEqual(synth.triggeredCueCount(), beforeOwnOnline,
                     "Local human online move echo must fire 0 duplicate cues");

  /* G4: Opponent Seat 2 (uD) submits "57+e" to server -> syncOnlineGameRoom must trigger PRONG_SNAP */
  markStep("sec-G4");
  const oppMove1Res = await fetch("http://127.0.0.1:" + cfg.port + "/api/v1/games/" + cfg.liveRoom + "/move", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      "Authorization": "Bearer " + cfg.seat2Token
    },
    body: JSON.stringify({ move: "57+e" })
  });
  assert.strictEqual(oppMove1Res.status, 200);
  const beforeOpp1Sync = synth.triggeredCueCount();
  sandbox.window.syncOnlineGameRoom(cfg.liveRoom, false);
  await sleepMs(100);
  assert.strictEqual(synth.triggeredCueCount(), beforeOpp1Sync + 1,
                     "Opponent online move must trigger 1 cue");
  assert.strictEqual(synth.lastTriggeredCue(), "PRONG_SNAP");

  /* G5: Advance to jump & terminal base capture in liveRoom */
  markStep("sec-G5");
  /* Seat 1 local move "53a-54" -> 0 cues */
  const beforeOwn2 = synth.triggeredCueCount();
  sandbox.window.onLocalMoveCommitted(
    "53a-54",
    t.applyMoveToState(t.cloneState(sandbox.window.activePlayState), "53a-54", "A")
  );
  await sleepMs(120);
  assert.strictEqual(synth.triggeredCueCount(), beforeOwn2);

  /* Opponent Seat 2 plays "57e-56" -> triggers POD_STEP */
  assert.strictEqual((await fetch("http://127.0.0.1:" + cfg.port + "/api/v1/games/" + cfg.liveRoom + "/move", {
    method: "POST",
    headers: { "Content-Type": "application/json", "Authorization": "Bearer " + cfg.seat2Token },
    body: JSON.stringify({ move: "57e-56" })
  })).status, 200);
  const beforeOppStep = synth.triggeredCueCount();
  sandbox.window.syncOnlineGameRoom(cfg.liveRoom, false);
  await sleepMs(100);
  assert.strictEqual(synth.triggeredCueCount(), beforeOppStep + 1);
  assert.strictEqual(synth.lastTriggeredCue(), "POD_STEP");

  /* Seat 1 local move "54a-55" -> 0 cues */
  const beforeOwn3 = synth.triggeredCueCount();
  sandbox.window.onLocalMoveCommitted(
    "54a-55",
    t.applyMoveToState(t.cloneState(sandbox.window.activePlayState), "54a-55", "A")
  );
  await sleepMs(120);
  assert.strictEqual(synth.triggeredCueCount(), beforeOwn3);

  /* Opponent Seat 2 jumps over A55: "56e-54x" -> triggers POD_CAPTURE */
  assert.strictEqual((await fetch("http://127.0.0.1:" + cfg.port + "/api/v1/games/" + cfg.liveRoom + "/move", {
    method: "POST",
    headers: { "Content-Type": "application/json", "Authorization": "Bearer " + cfg.seat2Token },
    body: JSON.stringify({ move: "56e-54x" })
  })).status, 200);
  const beforeOppCap = synth.triggeredCueCount();
  sandbox.window.syncOnlineGameRoom(cfg.liveRoom, false);
  await sleepMs(100);
  assert.strictEqual(synth.triggeredCueCount(), beforeOppCap + 1);
  assert.strictEqual(synth.lastTriggeredCue(), "POD_CAPTURE");

  /* Seat 1 local move "33+a" -> 0 cues */
  sandbox.window.onLocalMoveCommitted(
    "33+a",
    t.applyMoveToState(t.cloneState(sandbox.window.activePlayState), "33+a", "A")
  );
  await sleepMs(120);

  /* Opponent Seat 2 steps into A's base (5,3) in 1-base match: "54e-53" -> triggers GAME_VICTORY once */
  assert.strictEqual((await fetch("http://127.0.0.1:" + cfg.port + "/api/v1/games/" + cfg.liveRoom + "/move", {
    method: "POST",
    headers: { "Content-Type": "application/json", "Authorization": "Bearer " + cfg.seat2Token },
    body: JSON.stringify({ move: "54e-53" })
  })).status, 200);
  const beforeOppWin = synth.triggeredCueCount();
  sandbox.window.syncOnlineGameRoom(cfg.liveRoom, false);
  await sleepMs(100);
  assert.strictEqual(synth.triggeredCueCount(), beforeOppWin + 1);
  assert.strictEqual(synth.lastTriggeredCue(), "GAME_VICTORY");

  /* Polling finished room again must trigger 0 extra victory cues */
  sandbox.window.syncOnlineGameRoom(cfg.liveRoom, true);
  await sleepMs(100);
  assert.strictEqual(synth.triggeredCueCount(), beforeOppWin + 1,
                     "Repeated sync on already-finished room must not re-fire GAME_VICTORY");
  sandbox.window.stopOnlineGameSync(false);

  /* H. Local clockInterval boundary checks: 11s -> silent, 10s..1s -> CLOCK_LOW_TICK, 0s -> GAME_VICTORY */
  markStep("sec-H");
  sandbox.window.localMatchConfig.playMode = "pass";
  sandbox.window.localMatchConfig.timeControl = "300+3";
  sandbox.window.prepareLocalMatchInitialState();
  const intervalsBeforeStart = new Set(activeIntervals.keys());
  elements["btn-web-start-match"].click();
  let clockTickFn = null;
  for (const [id, entry] of activeIntervals.entries()) {
    if (!intervalsBeforeStart.has(id) && entry.ms === 1000) {
      clockTickFn = entry.fn;
    }
  }
  assert.strictEqual(typeof clockTickFn, "function", "Expected 1000ms clockInterval on timed match start");

  t.clockTimers.A = 12;
  const beforeTick11 = synth.triggeredCueCount();
  clockTickFn(); /* 12 -> 11s */
  assert.strictEqual(t.clockTimers.A, 11);
  assert.strictEqual(synth.triggeredCueCount(), beforeTick11, "11s remaining must not trigger CLOCK_LOW_TICK");

  clockTickFn(); /* 11 -> 10s */
  assert.strictEqual(t.clockTimers.A, 10);
  assert.strictEqual(synth.triggeredCueCount(), beforeTick11 + 1, "10s remaining must trigger CLOCK_LOW_TICK");
  assert.strictEqual(synth.lastTriggeredCue(), "CLOCK_LOW_TICK");

  t.clockTimers.A = 2;
  const beforeTick1 = synth.triggeredCueCount();
  clockTickFn(); /* 2 -> 1s */
  assert.strictEqual(t.clockTimers.A, 1);
  assert.strictEqual(synth.triggeredCueCount(), beforeTick1 + 1, "1s remaining must trigger CLOCK_LOW_TICK");
  assert.strictEqual(synth.lastTriggeredCue(), "CLOCK_LOW_TICK");

  clockTickFn(); /* 1 -> 0s -> timeout victory */
  assert.strictEqual(t.clockTimers.A, 0);
  assert.strictEqual(synth.triggeredCueCount(), beforeTick1 + 2, "0s timeout must trigger GAME_VICTORY");
  assert.strictEqual(synth.lastTriggeredCue(), "GAME_VICTORY");

  /* I. Fault injection: suspended AudioContext whose resume() rejects falls back cleanly to HTMLAudioElement */
  markStep("sec-I");
  class RejectingSuspendedAudioContext {
    constructor() {
      this.state = "suspended";
      this.destination = {};
    }
    resume() {
      return Promise.reject(new Error("NotAllowedError: AudioContext resume blocked in background tab"));
    }
    decodeAudioData() {
      return Promise.reject(new Error("decode blocked"));
    }
    createBufferSource() {
      throw new Error("createBufferSource blocked");
    }
  }
  class RejectingPlayHtmlAudio {
    constructor(src) {
      this.src = src || "";
      this.preload = "none";
      this.paused = true;
      this.ended = false;
      this.currentTime = 0;
    }
    play() {
      htmlAudioPlayed.push(this.src);
      return Promise.reject(new Error("HTMLAudioElement autoplay blocked"));
    }
  }
  sandbox.AudioContext = RejectingSuspendedAudioContext;
  sandbox.window.AudioContext = RejectingSuspendedAudioContext;
  sandbox.Audio = RejectingPlayHtmlAudio;
  sandbox.window.Audio = RejectingPlayHtmlAudio;

  const fallbackPlayedBefore = htmlAudioPlayed.length;
  const countBeforeFallback = synth.triggeredCueCount();
  synth.play("PRONG_SNAP");
  await sleepMs(30);
  assert.strictEqual(synth.triggeredCueCount(), countBeforeFallback + 1);
  assert.strictEqual(synth.lastTriggeredCue(), "PRONG_SNAP");
  assert.ok(htmlAudioPlayed.length > fallbackPlayedBefore,
            "Suspended rejecting AudioContext must fall back to HTMLAudioElement");
  assert.ok(
    htmlAudioPlayed[htmlAudioPlayed.length - 1].startsWith("data:audio/wav;base64,"),
    "HTMLAudioElement fallback must use data:audio/wav;base64,..."
  );
  assert.strictEqual(unhandledRejections.length, 0,
                     "Must have zero unhandled Promise rejections, got: " + unhandledRejections.join("; "));

  process.exit(0);
})().catch((err) => {
  console.error(err);
  process.exit(1);
});
)NODEJS";
    }

    int rcNode = std::system(("timeout -k 2 25 node " + runnerJsPath + " " + webJsPath + " " + cfgJsonPath).c_str());
    std::remove(webJsPath.c_str());
    std::remove(cfgJsonPath.c_str());
    std::remove(runnerJsPath.c_str());
    CHECK_EQUAL(0, rcNode);
}

TEST(OctiHttpServerIntegrationTest, SuperProngJsEngineComposerStagingAndStackInspector)
{
    /* 1. Strict CSP self-containment check on GET / */
    Reply rUi = http().get("/");
    EXPECT_STATUS(200, rUi);
    {
        std::string strippedUi = rUi.body;
        const char *kAllowedTokens[] = {
            "http://www.w3.org/2000/svg",
            "https://www.selfso.com/software/windows/octi/",
            "https://www.selfso.com/software/android/octi/",
            "placeholder=\"Robot URL (optional http:// or https://, no '?')\"",
            "<link rel=\"icon\"",
            "<link rel=\"apple-touch-icon\""
        };
        for (size_t ti = 0; ti < sizeof(kAllowedTokens) / sizeof(kAllowedTokens[0]); ++ti) {
            const std::string tok(kAllowedTokens[ti]);
            size_t pos = 0;
            while ((pos = strippedUi.find(tok)) != std::string::npos) {
                strippedUi.replace(pos, tok.size(), "");
            }
        }
        CHECK_TRUE(strippedUi.find("http://") == std::string::npos);
        CHECK_TRUE(strippedUi.find("https://") == std::string::npos);
        CHECK_TRUE(strippedUi.find("src=\"//") == std::string::npos);
        CHECK_TRUE(strippedUi.find("<link") == std::string::npos);
        CHECK_TRUE(strippedUi.find("<iframe") == std::string::npos);
    }

    /* 2. Compute authoritative C++ OctiGame::generateLegalMoves sets across R1, R4, R5, and R6 positions */
    struct ParityFixture {
        const char *label;
        const char *fen;
    };
    const ParityFixture kParityFixtures[] = {
        {"single_superprong_A33a_bang", "2:3:1:0:P:A:-:0:3,0,10,1:3,0,12,2:A33a!,B57"},
        {"single_superprong_jump_A33a_bang", "2:3:1:0:P:A:-:0:3,0,10,1:2,0,12,2:A33a!,B34e,B57"},
        {"dual_superprong_single_pod_A33a_bang_c_bang", "2:3:1:0:P:A:-:0:3,0,10,0:3,0,12,2:A33a!c!,B57"},
        {"dual_pod_A33a_bang_A53a_bang", "2:3:1:0:P:A:-:0:2,0,10,0:3,0,12,2:A33a!,A53a!,B57"},
        {"stacked_A33a_bang_A33a_bang", "2:3:1:0:P:A:-:0:2,0,10,0:3,0,12,2:A33a!,A33a!,B57"},
        {"stacked_jump_over_B34_A33a_bang_A33a_bang", "2:3:1:0:P:A:-:0:2,0,0,0:2,0,12,2:A33a!,A33a!,B34e,B57"},
        {"pre_existing_dest_A33a_bang_A34a_bang", "2:3:1:0:P:A:-:0:2,0,10,0:3,0,12,2:A33a!,A34a!,B57"},
        {"edgeless_wrap_A15g_bang", "2:3:1:1:P:A:-:0:3,0,10,1:3,0,12,2:A15g!,B57"},
        {"edgeless_wrap_jump_A15g_bang", "2:3:1:1:P:A:-:0:3,0,10,1:2,0,12,2:A15g!,B95c,B57"},
        /* R1: zero reposition moves & boxed-in 0-reserve stalemate */
        {"r1_2p_no_repositions", "2:3:0:0:P:A:-:0:2,0,4,0:2,0,4,0:A33ab,A53,B37ef,B57"},
        {"r1_boxed_in_zero_legal_moves", "2:3:0:0:P:A:-:0:0,0,0,0:0,0,0,0:A11a,B12,B13,B57"},
        /* R4: stationary stack-mate blocks loop-back landing & _ED hop-over S */
        {"r4_stationary_stack_mate_blocks_loopback",
         "2:3:0:0:P:A:-:0:2,0,20,0:3,0,23,0:A53,A73,A44a,A44beg,A54c,B55e,B65e,B37,B77"},
        {"r4_single_pod_allows_loopback",
         "2:3:0:0:P:A:-:0:3,0,21,0:3,0,23,0:A53,A73,A44beg,A54c,B55e,B65e,B37,B77"},
        {"r4_edgeless_forbids_hop_over_stationary_S",
         "2:3:0:1:P:A:-:0:1,0,10,0:1,0,10,0:A44a,A44bcd,B55,B76,B96,B25,B57"},
        {"r4_sp_edgeless_forbids_hop_over_stationary_S",
         "2:3:1:1:P:A:-:0:1,0,10,0:0,0,10,0:A44a,A44b!,B55,B77,B99,B22,B57"},
        /* R5: 3 & 4 SP preludes in 4P_SP + immediate capture clearing + friendly SP self-capture */
        {"r5_4p_sp_three_preludes_single_pod",
         "4:3:1:0:P:A:-:0:0,0,2,0:0,0,2,0:0,0,2,0:0,0,2,0:A33a!c!e!,B37,C77,D73"},
        {"r5_4p_sp_four_preludes_single_pod",
         "4:3:1:0:P:A:-:0:0,0,1,0:0,0,1,0:0,0,1,0:0,0,1,0:A44a!c!e!g!,B37,C77,D73"},
        {"r5_sp_immediate_capture_clears_square_for_sp2",
         "2:3:1:0:P:A:-:0:1,0,2,0:1,0,2,0:A31a!,A12c!,B22,B32e,B57"},
        {"r5_sp_friendly_self_capture_removes_sp2_socket",
         "2:3:1:0:P:A:-:0:1,0,1,0:2,0,2,0:A31a!,A32c!,B57"},
        /* R6: 4P eliminated partner revival forbidden vs surviving own-pod liberation on partnerHome */
        {"r6_4p_no_partner_revival_when_own_captured_zero",
         "4:3:0:0:P:A:-:4:0,0,2,0:1,0,2,0:0,2,0,0:1,0,2,0:A53,A77a,B37,D73"},
        {"r6_4p_surviving_own_liberation_on_partner_home",
         "4:3:0:0:P:A:-:4:0,1,2,0:1,0,2,0:0,2,0,0:1,0,2,0:A53,A77a,B37,D73"}
    };

    Json parityCases = Json::array();
    for (size_t i = 0; i < sizeof(kParityFixtures) / sizeof(kParityFixtures[0]); ++i) {
        octi::OctiBoard board;
        board.fromPositionString(kParityFixtures[i].fen);
        CHECK_TRUE(board.getGame() != nullptr);
        std::vector<octi::OctiMove> legal = board.getGame()->generateLegalMoves();
        Json moveStrings = Json::array();
        for (const auto &mv : legal) {
            moveStrings.push_back(mv.asString());
        }
        parityCases.push_back(Json{
            {"label", kParityFixtures[i].label},
            {"fen", kParityFixtures[i].fen},
            {"moves", moveStrings}
        });
    }

    /* Authoritative post-move FENs under R4, R5 (immediate capture removal), and R6 */
    const std::string stackedImmediateStartFen =
        "2:3:1:0:P:A:-:0:2,0,0,0:1,0,12,2:A31a!,A31a!,B32e,B34,B57";
    {
        /* SP1 capturing B32e with 'x' removes B32e immediately, so SP2 jumping over B32 throws! */
        octi::OctiBoard badBoard;
        badBoard.fromPositionString(stackedImmediateStartFen);
        CHECK_THROWS(octi::IllegalMoveException,
                     badBoard.getGame()->move("31a!-33x-35;31a!-33x;33a!+b"));
    }
    const std::string stackedVaultThenCaptureMove = "31a!-33-35;31a!-33x;33a!+b";
    std::string stackedVaultThenCaptureAfterFen;
    {
        octi::OctiBoard board;
        board.fromPositionString(stackedImmediateStartFen);
        board.getGame()->move(stackedVaultThenCaptureMove);
        stackedVaultThenCaptureAfterFen = board.toPositionString();
    }

    const std::string spImmediateClearStartFen =
        "2:3:1:0:P:A:-:0:1,0,2,0:1,0,2,0:A31a!,A12c!,B22,B32e,B57";
    const std::string spImmediateClearMove = "31a!-33x;12c!-32;33a!+b";
    std::string spImmediateClearAfterFen;
    {
        octi::OctiBoard board;
        board.fromPositionString(spImmediateClearStartFen);
        board.getGame()->move(spImmediateClearMove);
        spImmediateClearAfterFen = board.toPositionString();
    }

    const std::string r4CompositeLoopStartFen =
        "2:3:0:0:P:A:-:0:2,0,20,0:3,0,23,0:A53,A73,A44a,A44beg,A54c,B55e,B65e,B37,B77";
    const std::string r4CompositeLoopMove = "44a-45,44beg-66x-64x-44";
    std::string r4CompositeLoopAfterFen;
    {
        octi::OctiBoard board;
        board.fromPositionString(r4CompositeLoopStartFen);
        board.getGame()->move(r4CompositeLoopMove);
        r4CompositeLoopAfterFen = board.toPositionString();
    }

    const std::string r6OwnLibPartnerHomeStartFen =
        "4:3:0:0:P:A:-:4:0,1,2,0:1,0,2,0:0,2,0,0:1,0,2,0:A53,A77a,B37,D73";
    const std::string r6OwnLibPartnerHomeMove = "77*";
    std::string r6OwnLibPartnerHomeAfterFen;
    {
        octi::OctiBoard board;
        board.fromPositionString(r6OwnLibPartnerHomeStartFen);
        board.getGame()->move(r6OwnLibPartnerHomeMove);
        r6OwnLibPartnerHomeAfterFen = board.toPositionString();
    }

    const std::string r1StalemateWinStartFen =
        "2:3:0:0:P:A:-:0:2,0,4,0:0,0,0,0:A53a,B11e,A12,A21,A22,A13,A31,A33";
    const std::string r1StalemateWinMove = "53a-54";
    std::string r1StalemateWinAfterFen;
    {
        octi::OctiBoard board;
        board.fromPositionString(r1StalemateWinStartFen);
        board.getGame()->move(r1StalemateWinMove);
        CHECK_TRUE(board.getGame()->isGameOver());
        CHECK_EQUAL(octi::PLAYER_A, board.getGame()->getWinner());
        r1StalemateWinAfterFen = board.toPositionString();
    }

    size_t scStart = rUi.body.find("<script>");
    CHECK_TRUE(scStart != std::string::npos);
    size_t scEnd = rUi.body.find("</script>", scStart);
    CHECK_TRUE(scEnd != std::string::npos);
    std::string jsCode = rUi.body.substr(scStart + 8, scEnd - (scStart + 8));

    std::string pidTag = std::to_string(getpid());
    std::string webJsPath = "/tmp/octi_sp_webui_" + pidTag + ".js";
    std::string cfgJsonPath = "/tmp/octi_sp_cfg_" + pidTag + ".json";
    std::string runnerJsPath = "/tmp/octi_sp_runner_" + pidTag + ".js";

    {
        std::ofstream ofs(webJsPath);
        ofs << jsCode;
    }
    {
        Json cfg;
        cfg["port"] = http().port();
        cfg["parityCases"] = parityCases;
        cfg["stackedImmediateStartFen"] = stackedImmediateStartFen;
        cfg["stackedVaultThenCaptureMove"] = stackedVaultThenCaptureMove;
        cfg["stackedVaultThenCaptureAfterFen"] = stackedVaultThenCaptureAfterFen;
        cfg["spImmediateClearStartFen"] = spImmediateClearStartFen;
        cfg["spImmediateClearMove"] = spImmediateClearMove;
        cfg["spImmediateClearAfterFen"] = spImmediateClearAfterFen;
        cfg["r4CompositeLoopStartFen"] = r4CompositeLoopStartFen;
        cfg["r4CompositeLoopMove"] = r4CompositeLoopMove;
        cfg["r4CompositeLoopAfterFen"] = r4CompositeLoopAfterFen;
        cfg["r6OwnLibPartnerHomeStartFen"] = r6OwnLibPartnerHomeStartFen;
        cfg["r6OwnLibPartnerHomeMove"] = r6OwnLibPartnerHomeMove;
        cfg["r6OwnLibPartnerHomeAfterFen"] = r6OwnLibPartnerHomeAfterFen;
        cfg["r1StalemateWinStartFen"] = r1StalemateWinStartFen;
        cfg["r1StalemateWinMove"] = r1StalemateWinMove;
        cfg["r1StalemateWinAfterFen"] = r1StalemateWinAfterFen;
        std::ofstream cfs(cfgJsonPath);
        cfs << cfg.dump();
    }
    {
        std::ofstream rfs(runnerJsPath);
        rfs << R"NODEJS(
const assert = require("assert");
const crypto = require("crypto");
const fs = require("fs");
const vm = require("vm");

const webJsRaw = fs.readFileSync(process.argv[2], "utf8");
const cfg = JSON.parse(fs.readFileSync(process.argv[3], "utf8"));

const iifeClose = webJsRaw.lastIndexOf("})();");
assert.ok(iifeClose > 0, "Expected closing })(); in WebUi script");
const instrumentedJs =
  webJsRaw.slice(0, iifeClose) +
  "\nwindow.__octiSpTest = {\n" +
  "  parseOctiFen: parseOctiFen,\n" +
  "  serializeOctiFen: serializeOctiFen,\n" +
  "  cloneState: cloneState,\n" +
  "  generateMoves: generateMoves,\n" +
  "  generateMovesForState: (typeof generateMovesForState === 'function') ? generateMovesForState : null,\n" +
  "  applyMoveToState: applyMoveToState,\n" +
  "  WebMoveComposer: WebMoveComposer,\n" +
  "  getMoveComposer: function() { return moveComposer; },\n" +
  "  setMoveComposer: function(mc) { moveComposer = mc; },\n" +
  "  getPlayBoardGeom: function() { return playBoardGeom; },\n" +
  "  renderPlayBoardCanvas: renderPlayBoardCanvas,\n" +
  "  handlePlaySquareClick: handlePlaySquareClick,\n" +
  "  openZoomedSquare: openZoomedSquare,\n" +
  "  closeZoomedSquare: closeZoomedSquare,\n" +
  "  setActivePlayState: function(s) { activePlayState = s; window.activePlayState = s; },\n" +
  "  getActivePlayState: function() { return activePlayState; },\n" +
  "  setMatchStarted: function(v) { matchStarted = v; window.matchStarted = v; },\n" +
  "  setOnlineSpectator: function(roomId, seat, roomView) {\n" +
  "    activeOnlineGameId = roomId;\n" +
  "    activeOnlineViewerSeat = seat;\n" +
  "    activeOnlineRoomView = roomView;\n" +
  "  },\n" +
  "  setVcrState: function(plies, idx) {\n" +
  "    playMatchPlies = plies;\n" +
  "    playVcrIndex = idx;\n" +
  "  }\n" +
  "};\n})();\n";

function createCanvas2dContext(ownerEl) {
  return new Proxy({
    createLinearGradient: () => ({ addColorStop() {} }),
    createRadialGradient: () => ({ addColorStop() {} }),
    createPattern: () => ({}),
    measureText: () => ({ width: 20 })
  }, {
    get(target, prop) {
      if (prop in target) return target[prop];
      return function () {
        if (ownerEl && (prop === "fill" || prop === "stroke" || prop === "arc" || prop === "lineTo")) {
          ownerEl._drawCalls = (ownerEl._drawCalls || 0) + 1;
        }
      };
    },
    set(target, prop, val) {
      target[prop] = val;
      return true;
    }
  });
}

class MockNode {}
const elements = {};
function makeElement(id) {
  let _innerHTML = "";
  const elObj = Object.assign(new MockNode(), {
    id: id || "",
    tagName: "DIV",
    textContent: "",
    value: "",
    checked: false,
    disabled: false,
    className: "",
    width: 760,
    height: 760,
    clientWidth: 760,
    clientHeight: 760,
    offsetWidth: 760,
    offsetHeight: 760,
    scrollHeight: 0,
    scrollTop: 0,
    children: [],
    firstChild: null,
    parentNode: null,
    _drawCalls: 0,
    style: {
      setProperty(k, v) { this[k] = v; },
      removeProperty(k) { delete this[k]; },
      getPropertyValue(k) { return this[k] || ""; }
    },
    dataset: {},
    listeners: {},
    addEventListener(ev, fn, opts) {
      if (!this.listeners[ev]) this.listeners[ev] = [];
      this.listeners[ev].push({ fn, opts });
    },
    removeEventListener() {},
    dispatch(ev, payload) {
      const arr = this.listeners[ev] || [];
      const evt = Object.assign({
        button: 0,
        preventDefault() {},
        stopPropagation() {},
        target: this
      }, payload || {});
      for (const item of arr) item.fn.call(this, evt);
    },
    click() {
      if (typeof this.onclick === "function") {
        this.onclick({ preventDefault() {}, stopPropagation() {}, target: this });
      }
      this.dispatch("click");
    },
    classList: {
      add(...cls) {
        const cur = new Set(String(elObj.className || "").split(/\s+/).filter(Boolean));
        for (const c of cls) cur.add(c);
        elObj.className = Array.from(cur).join(" ");
      },
      remove(...cls) {
        const cur = new Set(String(elObj.className || "").split(/\s+/).filter(Boolean));
        for (const c of cls) cur.delete(c);
        elObj.className = Array.from(cur).join(" ");
      },
      toggle(c, force) {
        const has = this.contains(c);
        const next = (force !== undefined) ? Boolean(force) : !has;
        if (next) this.add(c); else this.remove(c);
        return next;
      },
      contains(c) {
        return String(elObj.className || "").split(/\s+/).filter(Boolean).includes(c);
      }
    },
    appendChild(ch) {
      if (ch) {
        ch.parentNode = this;
        this.children.push(ch);
        this.firstChild = this.children[0] || null;
      }
      return ch;
    },
    insertBefore(ch) { return this.appendChild(ch); },
    removeChild(ch) {
      const idx = this.children.indexOf(ch);
      if (idx >= 0) {
        this.children.splice(idx, 1);
        ch.parentNode = null;
      }
      this.firstChild = this.children[0] || null;
      return ch;
    },
    replaceChild(ch, oldCh) {
      const idx = this.children.indexOf(oldCh);
      if (idx >= 0) this.children[idx] = ch;
      else this.children.push(ch);
      this.firstChild = this.children[0] || null;
      return oldCh;
    },
    cloneNode() { return makeElement(""); },
    focus() {},
    blur() {},
    select() {},
    scrollIntoView() {},
    contains(other) {
      if (other === this) return true;
      return this.children.some((c) => c === other || (c.contains && c.contains(other)));
    },
    getAttribute() { return null; },
    setAttribute() {},
    removeAttribute() {},
    querySelector() { return null; },
    querySelectorAll() { return []; },
    getBoundingClientRect() {
      const w = Number(this.width) || 760;
      const h = Number(this.height) || 760;
      return { left: 0, top: 0, right: w, bottom: h, width: w, height: h };
    },
    getContext() {
      if (!this._ctx2d) this._ctx2d = createCanvas2dContext(this);
      return this._ctx2d;
    }
  });
  Object.defineProperty(elObj, "innerHTML", {
    get() { return _innerHTML; },
    set(v) {
      _innerHTML = String(v || "");
      elObj.children = [];
      elObj.firstChild = null;
    }
  });
  return elObj;
}

const storageMap = new Map();
const localStorageShim = {
  getItem(k) { return storageMap.has(k) ? storageMap.get(k) : null; },
  setItem(k, v) { storageMap.set(String(k), String(v)); },
  removeItem(k) { storageMap.delete(k); },
  clear() { storageMap.clear(); }
};

const windowListeners = {};
const documentListeners = {};

class StubAudioCtx {
  constructor() { this.state = "running"; this.destination = {}; }
  resume() { return Promise.resolve(); }
  decodeAudioData() { return Promise.resolve({ sampleRate: 16000, length: 160, numberOfChannels: 1 }); }
  createBufferSource() { return { connect() {}, start() {} }; }
}
class StubHtmlAudio {
  play() { return Promise.resolve(); }
}

const sandbox = {
  console,
  Date,
  Math,
  JSON,
  Array,
  Object,
  String,
  Number,
  Boolean,
  RegExp,
  Error,
  Map,
  Set,
  BigInt,
  Promise,
  Uint8Array,
  Int16Array,
  Float32Array,
  ArrayBuffer,
  DataView,
  URL,
  URLSearchParams,
  Blob,
  FormData,
  Headers,
  AbortController,
  crypto,
  Node: MockNode,
  parseInt,
  parseFloat,
  isNaN,
  isFinite,
  encodeURIComponent,
  decodeURIComponent,
  atob: (b64) => Buffer.from(b64, "base64").toString("binary"),
  btoa: (bin) => Buffer.from(bin, "binary").toString("base64"),
  localStorage: localStorageShim,
  location: {
    hash: "",
    pathname: "/",
    search: "",
    origin: "http://127.0.0.1:" + cfg.port,
    href: "http://127.0.0.1:" + cfg.port + "/"
  },
  history: { replaceState() {}, pushState() {} },
  navigator: { clipboard: { writeText: () => Promise.resolve() }, userAgent: "OctiNodeTest/1.0" },
  devicePixelRatio: 1,
  innerWidth: 1280,
  innerHeight: 800,
  AudioContext: StubAudioCtx,
  Audio: StubHtmlAudio,
  EventSource: function () { this.close = function () {}; },
  Image: function () {},
  alert() {},
  confirm() { return true; },
  prompt() { return ""; },
  open() { return null; },
  requestAnimationFrame() { return 0; },
  setTimeout() { return 0; },
  clearTimeout() {},
  setInterval() { return 1; },
  clearInterval() {},
  addEventListener(ev, fn, opts) {
    if (!windowListeners[ev]) windowListeners[ev] = [];
    windowListeners[ev].push({ fn, opts });
  },
  removeEventListener() {},
  fetch() { return Promise.resolve({ ok: true, status: 200, json: () => Promise.resolve({}) }); },
  document: {
    body: makeElement("body"),
    documentElement: makeElement("html"),
    activeElement: null,
    hidden: false,
    visibilityState: "visible",
    getElementById(id) {
      if (!elements[id]) {
        const el = makeElement(id);
        if (id === "web-zoom-canvas") { el.width = 320; el.height = 320; }
        elements[id] = el;
      }
      return elements[id];
    },
    createElement(tag) {
      const e = makeElement("");
      e.tagName = String(tag || "DIV").toUpperCase();
      return e;
    },
    createElementNS(ns, tag) {
      const e = makeElement("");
      e.namespaceURI = String(ns || "");
      e.tagName = String(tag || "SVG").toUpperCase();
      return e;
    },
    createTextNode(txt) {
      return Object.assign(new MockNode(), {
        nodeType: 3,
        textContent: txt == null ? "" : String(txt),
        parentNode: null
      });
    },
    querySelector() { return null; },
    querySelectorAll() { return []; },
    addEventListener(ev, fn, opts) {
      if (!documentListeners[ev]) documentListeners[ev] = [];
      documentListeners[ev].push({ fn, opts });
    },
    removeEventListener() {}
  }
};
sandbox.window = sandbox;
sandbox.self = sandbox;

vm.runInNewContext(instrumentedJs, sandbox, { filename: "WebUi.js", timeout: 5000 });

const t = sandbox.window.__octiSpTest;
assert.ok(t, "__octiSpTest must be available");

/* ============================================================================
 * PART 1: Engine Parity (generateMovesForState vs C++ OctiGame::generateLegalMoves
 *         + applyMoveToState validation & preludeDeferredCaps)
 * ============================================================================ */
assert.strictEqual(typeof t.generateMovesForState, "function",
                   "generateMovesForState must be defined on WebUi JS engine");

for (const pc of cfg.parityCases) {
  const st = t.parseOctiFen(pc.fen);
  const jsMoves = t.generateMovesForState(st);
  const cppSet = new Set(pc.moves);
  const jsSet = new Set(jsMoves);
  const missingInJs = pc.moves.filter((m) => !jsSet.has(m));
  const extraInJs = jsMoves.filter((m) => !cppSet.has(m));
  assert.strictEqual(
    jsMoves.length,
    jsSet.size,
    "JS generateMovesForState emitted duplicate moves on " + pc.label
  );
  assert.deepStrictEqual(
    { missingInJs, extraInJs },
    { missingInJs: [], extraInJs: [] },
    "Move-set mismatch on " + pc.label + " (" + pc.fen + ")"
  );
}

/* applyMoveToState rejections & R1/R4/R5/R6 C++/JS post-move FEN parity */
assert.throws(() => {
  const s = t.parseOctiFen("2:3:1:0:P:A:-:0:3,0,10,1:3,0,12,2:A33a!,B57");
  t.applyMoveToState(s, "33a!-34;34a!-35;35a!+b", "A");
}, "Single-socket double-prelude must be rejected");

assert.throws(() => {
  const s = t.parseOctiFen("2:3:1:0:P:A:-:0:3,0,10,0:1,0,12,2:A31a!c!,B32,B43,B57");
  t.applyMoveToState(s, "31a!c!-33-53;53a!c!+b", "A");
}, "Mid-prelude bend must be rejected");

{
  /* 3 and 4 controlled SPs on board succeed; >4 preludes or reusing SP socket throws */
  const s3 = t.parseOctiFen("4:3:1:0:P:A:-:0:0,0,2,0:0,0,2,0:0,0,2,0:0,0,2,0:A33a!c!e!,B37,C77,D73");
  const after3 = t.applyMoveToState(s3, "33a!c!e!-34;34a!c!e!-44;44a!c!e!-43;43a!c!e!+b", "A");
  assert.strictEqual(after3.turn, "B", "3 distinct SP preludes must advance turn cleanly");

  const s4 = t.parseOctiFen("4:3:1:0:P:A:-:0:0,0,1,0:0,0,1,0:0,0,1,0:0,0,1,0:A44a!c!e!g!,B37,C77,D73");
  const after4 = t.applyMoveToState(s4, "44a!c!e!g!-45;45a!c!e!g!-55;55a!c!e!g!-54;54a!c!e!g!-44;44a!c!e!g!+b", "A");
  assert.strictEqual(after4.turn, "B", "4 distinct SP preludes must advance turn cleanly");

  assert.throws(() => {
    const sReuse = t.parseOctiFen("4:3:1:0:P:A:-:0:0,0,2,0:0,0,2,0:0,0,2,0:0,0,2,0:A33a!c!e!,B37,C77,D73");
    t.applyMoveToState(sReuse, "33a!c!e!-34;34a!c!e!-44;44a!c!e!-45;45a!c!e!+b", "A");
  }, "Reusing socket a! in prelude 3 after prelude 1 must be rejected");

  assert.throws(() => {
    const s5 = t.parseOctiFen("4:3:1:0:P:A:-:0:0,0,1,0:0,0,1,0:0,0,1,0:0,0,1,0:A44a!b!c!e!g!,B37,C77,D73");
    t.applyMoveToState(s5, "44a!b!c!e!g!-45;45a!b!c!e!g!-56;56a!b!c!e!g!-66;66a!b!c!e!g!-65;65a!b!c!e!g!-55;55a!b!c!e!g!+d", "A");
  }, ">4 SP preludes must be rejected");
}

assert.throws(() => {
  const s = t.parseOctiFen("2:3:0:0:P:A:-:0:3,0,10,0:3,0,12,0:A33a,B57");
  t.applyMoveToState(s, "33a-34;34a+b", "A");
}, "Semicolon superprong prelude must be rejected when !s.superProngs");

/* R5: Immediate per-prelude capture removal */
assert.throws(() => {
  const sImmBad = t.parseOctiFen(cfg.stackedImmediateStartFen);
  t.applyMoveToState(sImmBad, "31a!-33x-35;31a!-33x;33a!+b", "A");
}, "SP1 capture removes B32e immediately so SP2 cannot jump over vacated (3,2)");

{
  const sVault = t.parseOctiFen(cfg.stackedImmediateStartFen);
  const afterVault = t.applyMoveToState(sVault, cfg.stackedVaultThenCaptureMove, "A");
  assert.strictEqual(
    t.serializeOctiFen(afterVault),
    cfg.stackedVaultThenCaptureAfterFen,
    "SP1 vault + SP2 capture must match C++ OctiBoard::toPositionString()"
  );

  const sClear = t.parseOctiFen(cfg.spImmediateClearStartFen);
  const afterClear = t.applyMoveToState(sClear, cfg.spImmediateClearMove, "A");
  assert.strictEqual(
    t.serializeOctiFen(afterClear),
    cfg.spImmediateClearAfterFen,
    "Immediate SP1 capture clearing landing square for SP2 must match C++ FEN"
  );

  /* SP prelude landing on final enemy base then stepping off in regular move does NOT win */
  const sEndWin = t.parseOctiFen("2:1:1:0:P:A:-:0:3,0,10,0:3,0,10,0:A36a!e,B77");
  const afterStepOff = t.applyMoveToState(sEndWin, "36a!e-37;37a!e-36", "A");
  assert.strictEqual(afterStepOff.status, "P", "Victory must only be evaluated at end of full turn");
  const sEndWin2 = t.parseOctiFen("2:1:1:0:P:A:-:0:3,0,10,0:3,0,10,0:A36a!e,B77");
  const afterStay = t.applyMoveToState(sEndWin2, "36a!e-37;37a!e+b", "A");
  assert.strictEqual(afterStay.status, "O");
  assert.strictEqual(afterStay.winner, "A");
}

/* R4: Stationary stack-mate blocks loop-back & _ED hop-over S; full clear-out succeeds */
assert.throws(() => {
  const sLoopBad = t.parseOctiFen(cfg.r4CompositeLoopStartFen);
  t.applyMoveToState(sLoopBad, "44beg-66x-64x-44", "A");
}, "Loop-back onto stationary A44a on (4,4) must throw");

assert.throws(() => {
  const sEdHopBad = t.parseOctiFen("2:3:0:1:P:A:-:0:1,0,10,0:1,0,10,0:A44a,A44bcd,B55,B76,B96,B25,B57");
  t.applyMoveToState(sEdHopBad, "44bcd-66-86-16-34-54", "A");
}, "_ED hop-over stationary A44a on starting square (4,4) must throw");

{
  const sCompOk = t.parseOctiFen(cfg.r4CompositeLoopStartFen);
  const afterCompOk = t.applyMoveToState(sCompOk, cfg.r4CompositeLoopMove, "A");
  assert.strictEqual(
    t.serializeOctiFen(afterCompOk),
    cfg.r4CompositeLoopAfterFen,
    "Composite clear-out loop-back 44a-45,44beg-66x-64x-44 must match C++ FEN"
  );
}

/* R6 & R1: No 4P partner revival; surviving own-pod liberation on partnerHome; boxed-in stalemate */
assert.throws(() => {
  const sRevBad = t.parseOctiFen("4:3:0:0:P:A:-:4:0,0,2,0:1,0,2,0:0,2,0,0:1,0,2,0:A53,A77a,B37,D73");
  t.applyMoveToState(sRevBad, "77*", "A");
}, "4P teammate revival 77* must be rejected");

assert.throws(() => {
  const sEmptyRevBad = t.parseOctiFen("4:3:0:0:P:A:-:4:0,1,2,0:1,0,2,0:0,2,0,0:1,0,2,0:A53,B37,D73");
  t.applyMoveToState(sEmptyRevBad, "77*", "A");
}, "Liberating onto unoccupied partner home 77* must be rejected");

{
  const sOwnLib = t.parseOctiFen(cfg.r6OwnLibPartnerHomeStartFen);
  const afterOwnLib = t.applyMoveToState(sOwnLib, cfg.r6OwnLibPartnerHomeMove, "A");
  assert.strictEqual(
    t.serializeOctiFen(afterOwnLib),
    cfg.r6OwnLibPartnerHomeAfterFen,
    "Surviving player liberating own pod on partnerHome must keep partner eliminated and match C++ FEN"
  );
  assert.strictEqual((afterOwnLib.elimMask & 4) !== 0, true, "Eliminated partner C must stay eliminated");

  const sStale = t.parseOctiFen(cfg.r1StalemateWinStartFen);
  const afterStale = t.applyMoveToState(sStale, cfg.r1StalemateWinMove, "A");
  assert.strictEqual(
    t.serializeOctiFen(afterStale),
    cfg.r1StalemateWinAfterFen,
    "Boxed-in 0-reserve armed pod stalemate must match C++ FEN"
  );
  assert.strictEqual(afterStale.status, "O");
  assert.strictEqual(afterStale.winner, "A");
}

/* ============================================================================
 * PART 2: WebMoveComposer ';'-Prelude Staging
 * ============================================================================ */
function makeComposerForFen(fen, committedOut) {
  let curSt = t.parseOctiFen(fen);
  const comp = new t.WebMoveComposer(
    function () { return curSt; },
    function (notStr, nextSt) {
      if (committedOut) committedOut.push(notStr);
      curSt = nextSt;
    }
  );
  return { comp, getState: () => curSt };
}

/* 2.1: A33a! steps 33 -> 34: committable immediately AND isAwaitingSuperProngBonusMove() === true */
{
  const committed = [];
  const { comp, getState } = makeComposerForFen("2:3:1:0:P:A:-:0:3,0,4,1:3,0,4,1:A33a!,B57", committed);
  assert.strictEqual(typeof comp.isAwaitingSuperProngBonusMove, "function",
                     "WebMoveComposer.prototype.isAwaitingSuperProngBonusMove must exist");
  assert.strictEqual(comp.beginGuidedPodMove(3, 3, 0), true);
  assert.strictEqual(comp.selectGuidedDestination(3, 4), true);
  assert.strictEqual(comp.stagedNotation, "33a!-34");
  assert.strictEqual(comp.isAwaitingConfirmation, true);
  assert.strictEqual(comp.isAwaitingSuperProngBonusMove(), true);
  assert.strictEqual(comp.commitStagedMove(), true);
  assert.deepStrictEqual(committed, ["33a!-34"]);
  assert.strictEqual(getState().turn, "B");
}

/* 2.2: A33a! steps 33 -> 34 then adds prong +b at (3,4) -> "33a!-34;34a!+b" */
{
  const committed = [];
  const { comp, getState } = makeComposerForFen("2:3:1:0:P:A:-:0:3,0,4,1:3,0,4,1:A33a!,B57", committed);
  assert.strictEqual(comp.beginGuidedPodMove(3, 3, 0), true);
  assert.strictEqual(comp.selectGuidedDestination(3, 4), true);
  assert.strictEqual(comp.isAwaitingSuperProngBonusMove(), true);
  assert.strictEqual(comp.stageAddProng(3, 4, 0, 1, false), true);
  assert.strictEqual(comp.stagedNotation, "33a!-34;34a!+b");
  assert.strictEqual(comp.isAwaitingSuperProngBonusMove(), false);
  assert.strictEqual(comp.commitStagedMove(), true);
  assert.deepStrictEqual(committed, ["33a!-34;34a!+b"]);
  assert.strictEqual(getState().turn, "B");
}

/* 2.3: A33a! steps 33 -> 34 then moves 34 -> 35 -> "33a!-34;34a!-35", 3rd move blocked */
{
  const committed = [];
  const { comp } = makeComposerForFen("2:3:1:0:P:A:-:0:3,0,4,1:3,0,4,1:A33a!,B57", committed);
  assert.strictEqual(comp.beginGuidedPodMove(3, 3, 0), true);
  assert.strictEqual(comp.selectGuidedDestination(3, 4), true);
  assert.strictEqual(comp.beginGuidedPodMove(3, 4, 0), true);
  assert.strictEqual(comp.selectGuidedDestination(3, 5), true);
  assert.strictEqual(comp.stagedNotation, "33a!-34;34a!-35");
  assert.strictEqual(comp.isAwaitingSuperProngBonusMove(), false);
  assert.strictEqual(comp.beginGuidedPodMove(3, 5, 0), false,
                     "3rd move must be blocked on a single superprong socket");
  assert.strictEqual(comp.stageMoveOrJump(3, 5, 3, 6), false,
                     "3rd drag/stage move must be blocked on a single superprong socket");
  assert.strictEqual(comp.stagedNotation, "33a!-34;34a!-35");
  assert.strictEqual(comp.commitStagedMove(), true);
  assert.deepStrictEqual(committed, ["33a!-34;34a!-35"]);
}

/* 2.4a: 2 superprongs across 2 pods (A33a!,A53a!): early commit at "33a!-34;53a!-54" AND full "33a!-34;53a!-54;34a!+b" */
{
  const earlyCommitted = [];
  const early = makeComposerForFen("2:3:1:0:P:A:-:0:2,0,4,0:3,0,4,0:A33a!,A53a!,B57", earlyCommitted);
  assert.strictEqual(early.comp.beginGuidedPodMove(3, 3, 0), true);
  assert.strictEqual(early.comp.selectGuidedDestination(3, 4), true);
  assert.strictEqual(early.comp.isAwaitingSuperProngBonusMove(), true);
  assert.strictEqual(early.comp.beginGuidedPodMove(5, 3, 0), true);
  assert.strictEqual(early.comp.selectGuidedDestination(5, 4), true);
  assert.strictEqual(early.comp.stagedNotation, "33a!-34;53a!-54");
  assert.strictEqual(early.comp.isAwaitingSuperProngBonusMove(), true);
  assert.strictEqual(early.comp.commitStagedMove(), true);
  assert.deepStrictEqual(earlyCommitted, ["33a!-34;53a!-54"]);

  const fullCommitted = [];
  const full = makeComposerForFen("2:3:1:0:P:A:-:0:2,0,4,0:3,0,4,0:A33a!,A53a!,B57", fullCommitted);
  assert.strictEqual(full.comp.beginGuidedPodMove(3, 3, 0), true);
  assert.strictEqual(full.comp.selectGuidedDestination(3, 4), true);
  assert.strictEqual(full.comp.beginGuidedPodMove(5, 3, 0), true);
  assert.strictEqual(full.comp.selectGuidedDestination(5, 4), true);
  assert.strictEqual(full.comp.stageAddProng(3, 4, 0, 1, false), true);
  assert.strictEqual(full.comp.stagedNotation, "33a!-34;53a!-54;34a!+b");
  assert.strictEqual(full.comp.isAwaitingSuperProngBonusMove(), false);
  assert.strictEqual(full.comp.commitStagedMove(), true);
  assert.deepStrictEqual(fullCommitted, ["33a!-34;53a!-54;34a!+b"]);
}

/* 2.4b: 2 superprongs on 1 pod (A33a!c!): "33a!c!-34;34a!c!-44;44a!c!+b" */
{
  const committed = [];
  const { comp } = makeComposerForFen("2:3:1:0:P:A:-:0:3,0,4,0:3,0,4,0:A33a!c!,B57", committed);
  assert.strictEqual(comp.beginGuidedPodMove(3, 3, 0), true);
  assert.strictEqual(comp.selectGuidedDestination(3, 4), true);
  assert.strictEqual(comp.stagedNotation, "33a!c!-34");
  assert.strictEqual(comp.isAwaitingSuperProngBonusMove(), true);
  assert.strictEqual(comp.beginGuidedPodMove(3, 4, 0), true);
  assert.strictEqual(comp.selectGuidedDestination(4, 4), true);
  assert.strictEqual(comp.stagedNotation, "33a!c!-34;34a!c!-44");
  assert.strictEqual(comp.isAwaitingSuperProngBonusMove(), true);
  assert.strictEqual(comp.stageAddProng(4, 4, 0, 1, false), true);
  assert.strictEqual(comp.stagedNotation, "33a!c!-34;34a!c!-44;44a!c!+b");
  assert.strictEqual(comp.isAwaitingSuperProngBonusMove(), false);
  assert.strictEqual(comp.commitStagedMove(), true);
  assert.deepStrictEqual(committed, ["33a!c!-34;34a!c!-44;44a!c!+b"]);
}

/* 2.5: 0 reserve prongs + prelude capture credits freeProngs === 1 with turn === "A" -> "33a!-35x;35a!+b" */
{
  const committed = [];
  const { comp, getState } = makeComposerForFen("2:3:1:0:P:A:-:0:0,0,0,0:0,0,0,0:A33a!,B34e", committed);
  assert.strictEqual(comp.beginGuidedPodMove(3, 3, 0), true);
  assert.strictEqual(comp.selectGuidedDestination(3, 5), true);
  assert.strictEqual(comp.stagedNotation, "33a!-35x");
  assert.strictEqual(comp.previewState.turn, "A", "Post-prelude previewState must keep turn === 'A'");
  assert.strictEqual(comp.previewState.reserves[0].freeProngs, 1,
                     "Prelude capture must credit B34e's prong to A's previewState freeProngs");
  /* Toggle capture off & back on before staging +b */
  assert.strictEqual(comp.toggleCaptureAtSquare(3, 4), true);
  assert.strictEqual(comp.stagedNotation, "33a!-35");
  assert.strictEqual(comp.previewState.reserves[0].freeProngs, 0);
  assert.strictEqual(comp.toggleCaptureAtSquare(3, 4), true);
  assert.strictEqual(comp.stagedNotation, "33a!-35x");
  assert.strictEqual(comp.previewState.reserves[0].freeProngs, 1);

  assert.strictEqual(comp.stageAddProng(3, 5, 0, 1, false), true);
  assert.strictEqual(comp.stagedNotation, "33a!-35x;35a!+b");
  assert.strictEqual(comp.previewState.reserves[0].freeProngs, 0);
  assert.strictEqual(comp.commitStagedMove(), true);
  assert.deepStrictEqual(committed, ["33a!-35x;35a!+b"]);
  assert.strictEqual(getState().status, "O");
  assert.strictEqual(getState().winner, "A");
}

/* 2.6: WebMoveComposer 3 & 4 SP preludes (4P_SP) + immediate capture clearing between SP1 & SP2 + R4 composite clear-out */
{
  /* 4 SP preludes on A44a!c!e!g! + regular +b */
  const committed4 = [];
  const c4 = makeComposerForFen("4:3:1:0:P:A:-:0:0,0,1,0:0,0,1,0:0,0,1,0:0,0,1,0:A44a!c!e!g!,B37,C77,D73", committed4);
  assert.strictEqual(c4.comp.stageMoveOrJump(4, 4, 4, 5), true);
  assert.strictEqual(c4.comp.isAwaitingSuperProngBonusMove(), true);
  assert.strictEqual(c4.comp.stageMoveOrJump(4, 5, 5, 5), true);
  assert.strictEqual(c4.comp.isAwaitingSuperProngBonusMove(), true);
  assert.strictEqual(c4.comp.stageMoveOrJump(5, 5, 5, 4), true);
  assert.strictEqual(c4.comp.isAwaitingSuperProngBonusMove(), true);
  assert.strictEqual(c4.comp.stageMoveOrJump(5, 4, 4, 4), true);
  assert.strictEqual(c4.comp.isAwaitingSuperProngBonusMove(), true);
  assert.strictEqual(c4.comp.stageAddProng(4, 4, 0, 1, false), true);
  assert.strictEqual(c4.comp.stagedNotation, "44a!c!e!g!-45;45a!c!e!g!-55;55a!c!e!g!-54;54a!c!e!g!-44;44a!c!e!g!+b");
  assert.strictEqual(c4.comp.isAwaitingSuperProngBonusMove(), false);
  assert.strictEqual(c4.comp.commitStagedMove(), true);
  assert.deepStrictEqual(committed4, ["44a!c!e!g!-45;45a!c!e!g!-55;55a!c!e!g!-54;54a!c!e!g!-44;44a!c!e!g!+b"]);

  /* Immediate capture removal in composer: SP1 31a!-33x removes B32e so SP2 12c!-32x / 12c!-32 can land on (3,2) */
  const committedImm = [];
  const cImm = makeComposerForFen(cfg.spImmediateClearStartFen, committedImm);
  assert.strictEqual(cImm.comp.stageMoveOrJump(3, 1, 3, 3), true);
  assert.strictEqual(cImm.comp.stagedNotation, "31a!-33x");
  assert.strictEqual(cImm.comp.stageMoveOrJump(1, 2, 3, 2), true);
  assert.strictEqual(cImm.comp.stagedNotation, "31a!-33x;12c!-32x");
  assert.strictEqual(cImm.comp.toggleCaptureAtSquare(2, 2), true);
  assert.strictEqual(cImm.comp.stagedNotation, "31a!-33x;12c!-32");
  assert.strictEqual(cImm.comp.stageAddProng(3, 3, 0, 1, false), true);
  assert.strictEqual(cImm.comp.stagedNotation, cfg.spImmediateClearMove);
  assert.strictEqual(cImm.comp.commitStagedMove(), true);
  assert.strictEqual(t.serializeOctiFen(cImm.getState()), cfg.spImmediateClearAfterFen);

  /* R4 composer: stationary A44a blocks 44beg loop-back onto 44, whereas staging 44a-45 first enables 44beg-66x-64x-44 */
  const cLoopBlocked = makeComposerForFen(cfg.r4CompositeLoopStartFen);
  assert.strictEqual(cLoopBlocked.comp.beginGuidedPodMove(4, 4, 1), true);
  assert.strictEqual(cLoopBlocked.comp.selectGuidedDestination(6, 6), true);
  assert.strictEqual(cLoopBlocked.comp.selectGuidedDestination(6, 4), true);
  assert.strictEqual(cLoopBlocked.comp.selectGuidedDestination(4, 4), false,
                     "Stationary A44a on (4,4) must block loop-back destination in WebMoveComposer");

  const committedLoop = [];
  const cLoopOk = makeComposerForFen(cfg.r4CompositeLoopStartFen, committedLoop);
  assert.strictEqual(cLoopOk.comp.beginGuidedPodMove(4, 4, 0), true);
  assert.strictEqual(cLoopOk.comp.selectGuidedDestination(4, 5), true);
  assert.strictEqual(cLoopOk.comp.beginGuidedPodMove(4, 4, 1), true);
  assert.strictEqual(cLoopOk.comp.selectGuidedDestination(6, 6), true);
  assert.strictEqual(cLoopOk.comp.selectGuidedDestination(6, 4), true);
  assert.strictEqual(cLoopOk.comp.selectGuidedDestination(4, 4), true,
                     "After 44a-45 vacates (4,4), 44beg must be allowed to loop back onto (4,4)");
  assert.strictEqual(cLoopOk.comp.stagedNotation, "44a-45,44beg-66x-64x-44");
  assert.strictEqual(cLoopOk.comp.commitStagedMove(), true);
  assert.strictEqual(t.serializeOctiFen(cLoopOk.getState()), cfg.r4CompositeLoopAfterFen);

  /* R4 _ED composer: stationary A44a on (4,4) blocks jumping over (4,4) on wraparound */
  const cEdBlocked = makeComposerForFen("2:3:0:1:P:A:-:0:1,0,10,0:1,0,10,0:A44a,A44bcd,B55,B76,B96,B25,B57");
  assert.strictEqual(cEdBlocked.comp.beginGuidedPodMove(4, 4, 1), true);
  assert.strictEqual(cEdBlocked.comp.selectGuidedDestination(6, 6), true);
  assert.strictEqual(cEdBlocked.comp.selectGuidedDestination(8, 6), true);
  assert.strictEqual(cEdBlocked.comp.selectGuidedDestination(1, 6), true);
  assert.strictEqual(cEdBlocked.comp.selectGuidedDestination(3, 4), true);
  assert.strictEqual(cEdBlocked.comp.selectGuidedDestination(5, 4), false,
                     "Jumping over starting square (4,4) must be blocked in WebMoveComposer");
}

/* ============================================================================
 * PART 3: Pod-Stack Click Inspector & 34x34 Mini Pod Diagrams
 * ============================================================================ */
const playCanvas = elements["web-play-canvas"];
const zoomOverlay = elements["web-zoom-overlay"];
const zoomTabs = elements["web-zoom-pod-tabs"];
assert.ok(playCanvas, "web-play-canvas must be registered");

function collectDescendantTexts(node) {
  let out = String(node.textContent || "");
  if (Array.isArray(node.children)) {
    for (const c of node.children) out += collectDescendantTexts(c);
  }
  return out;
}

function findDescendantCanvases(node) {
  const res = [];
  if (node && node.tagName === "CANVAS") res.push(node);
  if (node && Array.isArray(node.children)) {
    for (const c of node.children) res.push(...findDescendantCanvases(c));
  }
  return res;
}

function squarePt(x, y) {
  t.renderPlayBoardCanvas();
  const geom = t.getPlayBoardGeom();
  assert.ok(geom, "playBoardGeom must be initialized");
  const r = geom.squareRect(x, y);
  return { x: r.x + r.w * 0.5, y: r.y + r.h * 0.5 };
}

function clickSquareOnPlayCanvas(x, y) {
  const pt = squarePt(x, y);
  playCanvas.dispatch("mousedown", { clientX: pt.x, clientY: pt.y, button: 0 });
  playCanvas.dispatch("mouseup", { clientX: pt.x, clientY: pt.y, button: 0 });
}

function dragSquaresOnPlayCanvas(x0, y0, x1, y1) {
  const p0 = squarePt(x0, y0);
  const p1 = squarePt(x1, y1);
  playCanvas.dispatch("mousedown", { clientX: p0.x, clientY: p0.y, button: 0 });
  playCanvas.dispatch("mousemove", { clientX: p1.x, clientY: p1.y, button: 0 });
  playCanvas.dispatch("mouseup", { clientX: p1.x, clientY: p1.y, button: 0 });
}

function setupBoardSession(fen, opts) {
  const o = opts || {};
  t.closeZoomedSquare();
  zoomOverlay.style.display = "none";
  sandbox.window.stopOnlineGameSync(false);
  sandbox.window.localMatchConfig.playMode = o.playMode || "pass";
  sandbox.window.localMatchConfig.humanSeat = o.humanSeat || "A";
  t.setMatchStarted(o.matchStarted !== undefined ? o.matchStarted : true);
  if (o.onlineRoomId) {
    t.setOnlineSpectator(o.onlineRoomId, o.viewerSeat || 0, o.roomView || { phase: "IN_PROGRESS", seats: [] });
  }
  const st = t.parseOctiFen(fen);
  t.setActivePlayState(st);
  t.setVcrState(o.plies || [], o.vcrIndex !== undefined ? o.vcrIndex : 0);
  const mc = new t.WebMoveComposer(
    function () { return t.getActivePlayState(); },
    function (notStr, nextSt) { t.setActivePlayState(nextSt); }
  );
  t.setMoveComposer(mc);
  t.renderPlayBoardCanvas();
  return mc;
}

function assertReadOnlyZoomControls() {
  assert.strictEqual(zoomOverlay.style.display, "flex", "Zoom inspector must open");
  assert.notStrictEqual(elements["btn-zoom-move-pod"].style.display, "inline-block",
                        "Move Pod button must be hidden in read-only inspect");
  assert.notStrictEqual(elements["btn-zoom-place-pod"].style.display, "inline-block",
                        "Place Pod button must be hidden in read-only inspect");
  assert.notStrictEqual(elements["btn-zoom-liberate-pod"].style.display, "inline-block",
                        "Liberate Pod button must be hidden in read-only inspect");
  const mc = t.getMoveComposer();
  const prevNotation = mc ? mc.stagedNotation : "";
  elements["web-zoom-canvas"].dispatch("click", { clientX: 160, clientY: 40 });
  assert.strictEqual(mc ? mc.stagedNotation : "", prevNotation,
                     "Clicking zoom canvas in read-only inspect must not stage prong mutations");
}

/* 3.1: 2-pod friendly stack 1st click opens #web-zoom-overlay with 34x34 <canvas> mini pod diagrams & #1/#2 labels */
{
  const mc = setupBoardSession("2:3:1:0:P:A:-:0:1,0,4,1:3,0,4,0:A33a!c,A33e,A53a,B57ef");
  clickSquareOnPlayCanvas(3, 3);
  assert.strictEqual(zoomOverlay.style.display, "flex",
                     "1st click on 2-pod friendly stack must open #web-zoom-overlay immediately");
  assert.strictEqual(mc.isGuidedMoveActive, false,
                     "1st click on 2-pod friendly stack must not silently start beginGuidedPodMove");
  assert.strictEqual(zoomTabs.children.length, 2,
                     "Expected 2 pod tab buttons in #web-zoom-pod-tabs");

  const tabTexts = zoomTabs.children.map((btn) => collectDescendantTexts(btn));
  assert.ok(tabTexts[0].includes("#1 a!c"), "Expected '#1 a!c' in 1st tab, got: " + tabTexts[0]);
  assert.ok(tabTexts[1].includes("#2 e"), "Expected '#2 e' in 2nd tab, got: " + tabTexts[1]);

  for (let i = 0; i < zoomTabs.children.length; i++) {
    const canvases = findDescendantCanvases(zoomTabs.children[i]);
    assert.strictEqual(canvases.length, 1, "Each pod tab must contain a mini <canvas>");
    assert.strictEqual(canvases[0].width, 34, "Mini pod <canvas> width must be 34");
    assert.strictEqual(canvases[0].height, 34, "Mini pod <canvas> height must be 34");
    assert.ok(canvases[0]._drawCalls > 0, "Mini pod <canvas> must draw pod & prong geometry");
  }

  /* Close zoom overlay and verify drag-to-move on the 2-pod stack still drags directly without opening zoom */
  t.closeZoomedSquare();
  assert.strictEqual(zoomOverlay.style.display, "none");
  dragSquaresOnPlayCanvas(3, 3, 3, 4);
  assert.strictEqual(zoomOverlay.style.display, "none",
                     "Drag-to-move on 2-pod stack must not open #web-zoom-overlay");
  assert.strictEqual(mc.stagedNotation, "33a!c-34",
                     "Drag-to-move on 2-pod stack must move the top movable pod directly");
}

/* 3.2: 1-pod armed friendly square (5,3) still starts beginGuidedPodMove on 1st click, opens zoom on 2nd click */
{
  const mc = setupBoardSession("2:3:1:0:P:A:-:0:1,0,4,1:3,0,4,0:A33a!c,A33e,A53a,B57ef");
  clickSquareOnPlayCanvas(5, 3);
  assert.strictEqual(zoomOverlay.style.display, "none",
                     "1st click on 1-pod armed friendly square must NOT open zoom overlay");
  assert.strictEqual(mc.isGuidedMoveActive, true,
                     "1st click on 1-pod armed friendly square must start beginGuidedPodMove");
  clickSquareOnPlayCanvas(5, 3);
  assert.strictEqual(zoomOverlay.style.display, "flex",
                     "2nd click on 1-pod armed origin square must open zoom overlay");
}

/* 3.3: Enemy square (5,7) during A's turn opens read-only zoom inspector with 34x34 mini diagram */
{
  setupBoardSession("2:3:1:0:P:A:-:0:1,0,4,1:3,0,4,0:A33a!c,A33e,A53a,B57ef");
  clickSquareOnPlayCanvas(5, 7);
  assertReadOnlyZoomControls();
  assert.strictEqual(zoomTabs.children.length, 1);
  assert.ok(collectDescendantTexts(zoomTabs.children[0]).includes("#1 ef"));
  const canvases = findDescendantCanvases(zoomTabs.children[0]);
  assert.strictEqual(canvases.length, 1);
  assert.strictEqual(canvases[0].width, 34);
  assert.strictEqual(canvases[0].height, 34);
}

/* 3.4: Read-only states: AI/opponent turn, online spectator, VCR replay, status === 'O', status === 'R' */
{
  /* AI / opponent turn (humanSeat = A, turn = B) */
  setupBoardSession("2:3:1:0:P:B:-:0:1,0,4,1:3,0,4,0:A33a!c,A33e,B57ef",
                    { playMode: "bot", humanSeat: "A" });
  clickSquareOnPlayCanvas(3, 3);
  assertReadOnlyZoomControls();

  /* Online spectator (viewerSeat = 0) */
  setupBoardSession("2:3:1:0:P:A:-:0:1,0,4,1:3,0,4,0:A33a!c,A33e,B57ef",
                    { onlineRoomId: "room-spec-1", viewerSeat: 0 });
  clickSquareOnPlayCanvas(3, 3);
  assertReadOnlyZoomControls();

  /* VCR history replay (playVcrIndex = 0 < playMatchPlies.length - 1) */
  setupBoardSession("2:3:1:0:P:A:-:0:1,0,4,1:3,0,4,0:A33a!c,A33e,B57ef",
                    { plies: [{ ply: 1, move: "33+a" }, { ply: 2, move: "57+e" }], vcrIndex: 0 });
  clickSquareOnPlayCanvas(3, 3);
  assertReadOnlyZoomControls();

  /* Game Over (status === "O") */
  setupBoardSession("2:3:1:0:O:A:A:10:1,0,4,1:3,0,4,0:A33a!c,A33e,B57ef");
  clickSquareOnPlayCanvas(3, 3);
  assertReadOnlyZoomControls();

  /* Resigned (status === "R") */
  setupBoardSession("2:3:1:0:R:A:B:10:1,0,4,1:3,0,4,0:A33a!c,A33e,B57ef");
  clickSquareOnPlayCanvas(3, 3);
  assertReadOnlyZoomControls();
}

process.exit(0);
)NODEJS";
    }

    int rcNode = std::system(("timeout -k 2 20 node " + runnerJsPath + " " + webJsPath + " " + cfgJsonPath).c_str());
    std::remove(webJsPath.c_str());
    std::remove(cfgJsonPath.c_str());
    std::remove(runnerJsPath.c_str());
    CHECK_EQUAL(0, rcNode);
}

TEST(OctiHttpServerIntegrationTest, WebPlayerBotWorkerExecutionAndProgress)
{
    Reply rUi = http().get("/");
    EXPECT_STATUS(200, rUi);

    struct WorkerCase {
        const char *variant;
        const char *mode;
        const char *side;
        const char *fen;
        int searchId;
    };
    const WorkerCase kCases[] = {
        {"2P_3B", "2P", "A",
         "2:3:0:0:P:A:-:0:4,0,19,0:4,0,19,0:A34ab,A53ac,A73a,B37e,B56eg,B77ef",
         101},
        {"2P_3B_SP", "2P", "A",
         "2:3:1:0:P:A:-:0:4,0,19,0:4,0,19,0:A34a!b,A53ac,A73a,B37e!,B56eg,B77ef",
         102},
        {"4P_SP", "4P", "A",
         "4:3:1:0:P:A:-:0:3,0,9,0:3,0,9,0:3,0,9,0:3,0,9,0:A34a!b,D73g,B36e!f,C77c",
         103},
        {"OFK", "OFK", "A",
         "OFK:1:0:0:P:A:-:0:0,0,6,0:0,0,6,0:A23ab,A32a,A42c,A52a,B26e,B35ef,B46g,B56e",
         104}
    };

    Json casesJson = Json::array();
    for (size_t i = 0; i < sizeof(kCases) / sizeof(kCases[0]); ++i) {
        octi::OctiBoard board;
        board.fromPositionString(kCases[i].fen);
        CHECK_TRUE(board.getGame() != nullptr);
        CHECK_FALSE(board.getGame()->isGameOver());
        std::vector<octi::OctiMove> legal = board.getGame()->generateLegalMoves();
        CHECK_TRUE(!legal.empty());
        Json legalJson = Json::array();
        for (size_t m = 0; m < legal.size(); ++m) {
            legalJson.push_back(legal[m].asString());
        }
        casesJson.push_back(Json{
            {"variant", kCases[i].variant},
            {"mode", kCases[i].mode},
            {"side", kCases[i].side},
            {"fen", kCases[i].fen},
            {"searchId", kCases[i].searchId},
            {"legalMoves", legalJson}
        });
    }

    size_t scStart = rUi.body.find("<script>");
    CHECK_TRUE(scStart != std::string::npos);
    size_t scEnd = rUi.body.find("</script>", scStart);
    CHECK_TRUE(scEnd != std::string::npos);
    std::string jsCode = rUi.body.substr(scStart + 8, scEnd - (scStart + 8));

    std::string pidTag = std::to_string(getpid());
    std::string webJsPath = "/tmp/octi_worker_webui_" + pidTag + ".js";
    std::string cfgJsonPath = "/tmp/octi_worker_cfg_" + pidTag + ".json";
    std::string runnerJsPath = "/tmp/octi_worker_runner_" + pidTag + ".js";

    {
        std::ofstream ofs(webJsPath);
        ofs << jsCode;
    }
    {
        Json cfg;
        cfg["port"] = http().port();
        cfg["cases"] = casesJson;
        std::ofstream cfs(cfgJsonPath);
        cfs << cfg.dump();
    }
    {
        std::ofstream rfs(runnerJsPath);
        rfs << R"NODEJS(
const assert = require("assert");
const crypto = require("crypto");
const fs = require("fs");
const vm = require("vm");

const webJsRaw = fs.readFileSync(process.argv[2], "utf8");
const cfg = JSON.parse(fs.readFileSync(process.argv[3], "utf8"));

const iifeClose = webJsRaw.lastIndexOf("})();");
assert.ok(iifeClose > 0, "Expected closing })(); in WebUi script");
const instrumentedJs =
  webJsRaw.slice(0, iifeClose) +
  "\nwindow.__octiWorkerTest = {\n" +
  "  getWorkerCode: function() { return (typeof workerCode === 'string') ? workerCode : ''; },\n" +
  "  initBotWorker: (typeof initBotWorker === 'function') ? initBotWorker : null,\n" +
  "  cancelPendingAiSearch: (typeof cancelPendingAiSearch === 'function') ? cancelPendingAiSearch : null,\n" +
  "  updateThinkingProgressUi: (typeof updateThinkingProgressUi === 'function') ? updateThinkingProgressUi : null,\n" +
  "  getBotWorker: function() { return botWorker; },\n" +
  "  getActiveAiSearchId: function() { return (typeof activeAiSearchId === 'number') ? activeAiSearchId : -1; },\n" +
  "  isAiSearchInFlight: function() { return (typeof aiSearchInFlight === 'boolean') ? aiSearchInFlight : false; },\n" +
  "  parseOctiFen: parseOctiFen,\n" +
  "  setLocalContextForRestore: function(fen) {\n" +
  "    localMatchConfig.playMode = 'bot';\n" +
  "    localMatchConfig.humanSeat = 'A';\n" +
  "    localMatchConfig.botDifficulty = 2;\n" +
  "    localPlayContext = {\n" +
  "      state: parseOctiFen(fen),\n" +
  "      plies: [\n" +
  "        { plyIndex: 0, player: '-', move: '(Initial Setup)', fenAfter: fen },\n" +
  "        { plyIndex: 1, player: 'A', move: '33+a', fenAfter: fen }\n" +
  "      ],\n" +
  "      vcrIndex: 1,\n" +
  "      matchStarted: true,\n" +
  "      lastMoveHighlightSquares: [],\n" +
  "      lastMoveSetAtMs: 0,\n" +
  "      clockTimers: { A: 300, B: 300, C: 300, D: 300 },\n" +
  "      boardRotationDeg: 0,\n" +
  "      startOverlayVisible: false,\n" +
  "      gameoverVisible: false,\n" +
  "      gameoverTitle: '',\n" +
  "      gameoverSubtitle: '',\n" +
  "      archiveStatusText: ''\n" +
  "    };\n" +
  "  },\n" +
  "  restoreLocalPlayContext: restoreLocalPlayContext\n" +
  "};\n})();\n";

let capturedBlobText = "";
class CapturingBlob {
  constructor(parts, opts) {
    this.parts = Array.isArray(parts) ? parts : [];
    this.type = (opts && opts.type) || "";
    this.text = this.parts.map((p) => String(p)).join("");
    if (this.text.includes("self.onmessage")) {
      capturedBlobText = this.text;
    }
  }
}

const workerInstances = [];
class MockWorker {
  constructor(url) {
    this.url = url;
    this.terminated = false;
    this.posted = [];
    this.onmessage = null;
    this.onerror = null;
    workerInstances.push(this);
  }
  postMessage(msg) {
    this.posted.push(msg);
  }
  terminate() {
    this.terminated = true;
  }
}

class MockNode {}
const elements = {};
function makeElement(id) {
  let _innerHTML = "";
  const elObj = Object.assign(new MockNode(), {
    id: id || "",
    tagName: "DIV",
    textContent: "",
    value: "",
    checked: false,
    disabled: false,
    className: "",
    width: 760,
    height: 760,
    clientWidth: 760,
    clientHeight: 760,
    offsetWidth: 760,
    offsetHeight: 760,
    scrollHeight: 0,
    scrollTop: 0,
    children: [],
    firstChild: null,
    parentNode: null,
    style: {
      setProperty(k, v) { this[k] = v; },
      removeProperty(k) { delete this[k]; },
      getPropertyValue(k) { return this[k] || ""; }
    },
    dataset: {},
    listeners: {},
    addEventListener(ev, fn, opts) {
      if (!this.listeners[ev]) this.listeners[ev] = [];
      this.listeners[ev].push({ fn, opts });
    },
    removeEventListener() {},
    classList: {
      add() {},
      remove() {},
      toggle() {},
      contains() { return false; }
    },
    appendChild(ch) {
      if (ch) this.children.push(ch);
      return ch;
    },
    insertBefore(ch) { return this.appendChild(ch); },
    removeChild(ch) { return ch; },
    replaceChild(ch) { return ch; },
    cloneNode() { return makeElement(""); },
    focus() {},
    blur() {},
    select() {},
    scrollIntoView() {},
    contains() { return false; },
    getAttribute() { return null; },
    setAttribute() {},
    removeAttribute() {},
    querySelector() { return null; },
    querySelectorAll() { return []; },
    getBoundingClientRect() {
      return { left: 0, top: 0, right: 760, bottom: 760, width: 760, height: 760 };
    },
    getContext() {
      return new Proxy({
        createLinearGradient: () => ({ addColorStop() {} }),
        createRadialGradient: () => ({ addColorStop() {} }),
        createPattern: () => ({}),
        measureText: () => ({ width: 20 })
      }, {
        get(target, prop) { return prop in target ? target[prop] : function () {}; },
        set(target, prop, val) { target[prop] = val; return true; }
      });
    }
  });
  Object.defineProperty(elObj, "innerHTML", {
    get() { return _innerHTML; },
    set(v) { _innerHTML = String(v || ""); elObj.children = []; }
  });
  return elObj;
}

const storageMap = new Map();
const sandbox = {
  console,
  Date,
  Math,
  JSON,
  Array,
  Object,
  String,
  Number,
  Boolean,
  RegExp,
  Error,
  Map,
  Set,
  BigInt,
  Promise,
  Uint8Array,
  Int16Array,
  Float32Array,
  ArrayBuffer,
  DataView,
  URL: Object.assign(function (u, b) { return new URL(u, b); }, {
    createObjectURL() { return "blob:octi-bot-worker"; },
    revokeObjectURL() {}
  }),
  URLSearchParams,
  Blob: CapturingBlob,
  Worker: MockWorker,
  FormData,
  Headers,
  AbortController,
  crypto,
  Node: MockNode,
  parseInt,
  parseFloat,
  isNaN,
  isFinite,
  encodeURIComponent,
  decodeURIComponent,
  atob: (b64) => Buffer.from(b64, "base64").toString("binary"),
  btoa: (bin) => Buffer.from(bin, "binary").toString("base64"),
  localStorage: {
    getItem(k) { return storageMap.has(k) ? storageMap.get(k) : null; },
    setItem(k, v) { storageMap.set(String(k), String(v)); },
    removeItem(k) { storageMap.delete(k); },
    clear() { storageMap.clear(); }
  },
  location: {
    hash: "",
    pathname: "/",
    search: "",
    origin: "http://127.0.0.1:" + cfg.port,
    href: "http://127.0.0.1:" + cfg.port + "/"
  },
  history: { replaceState() {}, pushState() {} },
  navigator: { clipboard: { writeText: () => Promise.resolve() }, userAgent: "OctiWorkerTest/1.0" },
  devicePixelRatio: 1,
  innerWidth: 1280,
  innerHeight: 800,
  AudioContext: function () { this.state = "running"; this.destination = {}; },
  Audio: function () { this.play = () => Promise.resolve(); },
  EventSource: function () { this.close = function () {}; },
  Image: function () {},
  alert() {},
  confirm() { return true; },
  prompt() { return ""; },
  open() { return null; },
  requestAnimationFrame() { return 0; },
  setTimeout() { return 1; },
  clearTimeout() {},
  setInterval() { return 1; },
  clearInterval() {},
  addEventListener() {},
  removeEventListener() {},
  fetch() { return Promise.resolve({ ok: true, status: 200, json: () => Promise.resolve({}) }); },
  document: {
    body: makeElement("body"),
    documentElement: makeElement("html"),
    activeElement: null,
    hidden: false,
    visibilityState: "visible",
    getElementById(id) {
      if (!elements[id]) elements[id] = makeElement(id);
      return elements[id];
    },
    createElement(tag) {
      const e = makeElement("");
      e.tagName = String(tag || "DIV").toUpperCase();
      return e;
    },
    createElementNS(ns, tag) {
      const e = makeElement("");
      e.tagName = String(tag || "SVG").toUpperCase();
      return e;
    },
    createTextNode(txt) {
      return Object.assign(new MockNode(), { nodeType: 3, textContent: String(txt || "") });
    },
    querySelector() { return null; },
    querySelectorAll() { return []; },
    addEventListener() {},
    removeEventListener() {}
  }
};
sandbox.window = sandbox;
sandbox.self = sandbox;

vm.runInNewContext(instrumentedJs, sandbox, { filename: "WebUi.js", timeout: 5000 });

const wt = sandbox.window.__octiWorkerTest;
assert.ok(wt, "__octiWorkerTest must be exposed");
const workerCode = capturedBlobText || wt.getWorkerCode();
assert.ok(workerCode && workerCode.length > 1000, "Failed to extract inline workerCode from WebUi");

/* Execute workerCode in a strictly isolated Worker VM context (zero DOM / main-thread globals) */
const workerMessages = [];
const workerSandbox = {
  console,
  Date,
  Math,
  JSON,
  Array,
  Object,
  String,
  Number,
  Boolean,
  RegExp,
  Error,
  Map,
  Set,
  BigInt,
  parseInt,
  parseFloat,
  isNaN,
  isFinite,
  postMessage(msg) {
    workerMessages.push(msg);
  }
};
workerSandbox.self = workerSandbox;

vm.runInNewContext(workerCode, workerSandbox, { filename: "botWorker.js", timeout: 5000 });
assert.strictEqual(typeof workerSandbox.self.onmessage, "function", "worker self.onmessage must be a function");

const tStartAll = Date.now();
for (const c of cfg.cases) {
  workerMessages.length = 0;
  workerSandbox.self.onmessage({
    data: {
      type: "search",
      searchId: c.searchId,
      fen: c.fen,
      depth: 3,
      mode: c.mode,
      side: c.side,
      budgetMs: 600
    }
  });

  const progMsgs = workerMessages.filter((m) => m && m.type === "progress");
  const doneMsgs = workerMessages.filter((m) => m && m.type === "done");

  assert.ok(
    progMsgs.length >= 1,
    "Expected >= 1 'progress' message for " + c.variant + ", got " + progMsgs.length
  );
  for (const pm of progMsgs) {
    assert.strictEqual(
      pm.searchId,
      c.searchId,
      "Progress message searchId mismatch for " + c.variant
    );
    assert.ok(pm.depth >= 1, "Progress depth must be >= 1 for " + c.variant);
    assert.ok(pm.nodes > 0, "Progress nodes must be > 0 for " + c.variant);
    assert.strictEqual(pm.budgetMs, 600, "Progress budgetMs must be 600 for " + c.variant);
  }

  assert.strictEqual(doneMsgs.length, 1, "Expected 1 'done' message for " + c.variant);
  const dm = doneMsgs[0];
  assert.strictEqual(dm.searchId, c.searchId, "Done message searchId mismatch for " + c.variant);
  assert.ok(dm.bestMove && typeof dm.bestMove === "string", "Done bestMove must be non-empty string for " + c.variant);
  assert.ok(
    c.legalMoves.includes(dm.bestMove),
    "Worker returned illegal move '" + dm.bestMove + "' for " + c.variant
  );
  assert.ok(dm.nodes > 1, "Expected non-book tree search (nodes > 1) for " + c.variant + ", got " + dm.nodes);
}
const totalElapsedMs = Date.now() - tStartAll;
assert.ok(totalElapsedMs < 3000, "Expected all 4 worker searches to finish in < 3000ms, took " + totalElapsedMs + "ms");

/* Verify main-thread worker lifecycle, cancellation, and restoreLocalPlayContext unfreeze */
assert.strictEqual(typeof wt.initBotWorker, "function", "initBotWorker must be defined");
assert.strictEqual(typeof wt.cancelPendingAiSearch, "function", "cancelPendingAiSearch must be defined");
assert.strictEqual(typeof wt.updateThinkingProgressUi, "function", "updateThinkingProgressUi must be defined");

wt.setLocalContextForRestore("2:3:0:0:P:B:-:0:4,0,24,0:4,0,25,0:A33a,A53,A73,B37,B57,B77");
wt.restoreLocalPlayContext();
assert.strictEqual(wt.isAiSearchInFlight(), true, "restoreLocalPlayContext must re-trigger AI search when returning to #play on AI turn");
const activeWorker = wt.getBotWorker();
assert.ok(activeWorker && activeWorker.posted.length > 0, "restoreLocalPlayContext must post search to botWorker");
const lastPosted = activeWorker.posted[activeWorker.posted.length - 1];
assert.strictEqual(lastPosted.searchId, wt.getActiveAiSearchId(), "Posted searchId must match activeAiSearchId");

wt.cancelPendingAiSearch();
assert.strictEqual(wt.isAiSearchInFlight(), false, "cancelPendingAiSearch must clear aiSearchInFlight");
assert.strictEqual(activeWorker.terminated, true, "cancelPendingAiSearch must terminate in-flight worker");
assert.strictEqual(elements["web-thinking-overlay"].style.display, "none", "cancelPendingAiSearch must hide thinking overlay");

process.exit(0);
)NODEJS";
    }

    int rcNode = std::system(("timeout -k 2 20 node " + runnerJsPath + " " + webJsPath + " " + cfgJsonPath).c_str());
    std::remove(webJsPath.c_str());
    std::remove(cfgJsonPath.c_str());
    std::remove(runnerJsPath.c_str());
    CHECK_EQUAL(0, rcNode);
}

TEST(OctiHttpServerIntegrationTest, ArchiveExact13VariantFilteringAndByVariantStats)
{
    User alice = signIn("google", "sub-var-alice", "Alice Variant");
    User bob = signIn("apple", "sub-var-bob", "Bob Variant");

    Reply rBot = http().post("/api/v1/robots", alice.token,
                             Json{{"robot_name", "alice_variant_bot"},
                                  {"description", "Variant test bot"}});
    EXPECT_STATUS(201, rBot);
    std::string botUuid = rBot.json.value("robot_uuid", "");
    CHECK_FALSE(botUuid.empty());

    PlayerProfile aliceProf;
    PlayerProfile bobProf;
    RobotRecord botRec;
    CHECK_TRUE(env->store.getProfileByUuid(alice.uuid, aliceProf));
    CHECK_TRUE(env->store.getProfileByUuid(bob.uuid, bobProf));
    CHECK_TRUE(env->store.getRobotByUuid(botUuid, botRec));

    auto seedGame = [&](const std::string &id, const std::string &mode,
                        int bases, bool sp, bool ed,
                        int64_t p1Pid, int64_t p2Pid, int64_t p2Rid,
                        const std::string &p1Handle,
                        const std::string &p2Handle,
                        int64_t endedAt) {
        GameRecord g;
        g.gameId = id;
        g.gameMode = mode;
        g.gameBases = bases;
        g.superProngs = sp;
        g.edgeless = ed;
        g.timeControl = "untimed";
        g.p1PlayerId = p1Pid;
        g.p2PlayerId = p2Pid;
        g.p2RobotId = p2Rid;
        g.p1Handle = p1Handle;
        g.p2Handle = p2Handle;
        g.result = (mode == "4P") ? "TEAM_13_WIN" : "P1_WIN";
        g.winnerSeat = 1;
        g.termination = "BASE_CAPTURE";
        g.plyCount = 12;
        g.finalFen = "2:3:0:0:P:A:-:12:4,0,24,0:4,0,24,0:A33";
        g.startedAt = endedAt - 60;
        g.endedAt = endedAt;
        g.uploadedAt = endedAt;
        g.source = "online";
        g.winner = "A";
        g.status = "GAME_OVER";
        Json tr = {
            {"format", "octi.transcript.v3"},
            {"game_id", id},
            {"mode", mode},
            {"bases", bases},
            {"super_prongs", sp},
            {"edgeless", ed}
        };
        g.octiTranscript = tr.dump();
        std::string err;
        ArchiveStatus st = env->store.archiveGame(g, err);
        CHECK_EQUAL_TEXT(static_cast<int>(ArchiveStatus::OK),
                         static_cast<int>(st), err.c_str());
    };

    /*
     * Seed 10 games spanning 9 distinct canonical variants, including BOTH
     * a normal 4P row (game_bases = 3) and a legacy 4P row (game_bases = 1),
     * plus a legacy 4P_SP row (game_bases = 1).
     */
    seedGame("var_2p_3b",      "2P",  3, false, false, aliceProf.playerId, bobProf.playerId, 0, alice.handle, bob.handle, 1762000010);
    seedGame("var_2p_3b_sp",   "2P",  3, true,  false, aliceProf.playerId, 0, botRec.robotId, alice.handle, botRec.robotName, 1762000020);
    seedGame("var_2p_1b",      "2P",  1, false, false, aliceProf.playerId, bobProf.playerId, 0, alice.handle, bob.handle, 1762000030);
    seedGame("var_2p_1b_ed",   "2P",  1, false, true,  bobProf.playerId,   0, botRec.robotId, bob.handle, botRec.robotName, 1762000040);
    seedGame("var_2p_1b_sped", "2P",  1, true,  true,  0, 0, 0, "guest1", "guest2", 1762000050);
    seedGame("var_4p_std3",    "4P",  3, false, false, 0, 0, 0, "g1", "g2", 1762000060);
    seedGame("var_4p_leg1",    "4P",  1, false, false, aliceProf.playerId, 0, botRec.robotId, alice.handle, botRec.robotName, 1762000070);
    seedGame("var_4p_sp_leg1", "4P",  1, true,  false, 0, 0, 0, "g3", "g4", 1762000080);
    seedGame("var_4p_sped",    "4P",  3, true,  true,  0, 0, 0, "g5", "g6", 1762000090);
    seedGame("var_ofk",        "OFK", 1, false, false, aliceProf.playerId, bobProf.playerId, 0, alice.handle, bob.handle, 1762000100);
    CHECK_TRUE(env->store.reconcileRobotStats());

    /* 1. Direct AccountStore ArchiveFilter::mode (broad) vs ArchiveFilter::variant (exact) */
    {
        std::vector<GameRecord> rows;
        int64_t total = 0;

        ArchiveFilter fBroad4p;
        fBroad4p.mode = "4P";
        CHECK_TRUE(env->store.listArchivedGames(fBroad4p, rows, &total));
        LONGS_EQUAL(4, static_cast<long>(total));
        LONGS_EQUAL(4, static_cast<long>(rows.size()));

        ArchiveFilter fExact4p;
        fExact4p.variant = "4P";
        CHECK_TRUE(env->store.listArchivedGames(fExact4p, rows, &total));
        LONGS_EQUAL(2, static_cast<long>(total));
        LONGS_EQUAL(2, static_cast<long>(rows.size()));
        STRCMP_EQUAL("var_4p_leg1", rows[0].gameId.c_str());
        STRCMP_EQUAL("var_4p_std3", rows[1].gameId.c_str());

        ArchiveFilter fExact4pSp;
        fExact4pSp.variant = "4p_sp";
        CHECK_TRUE(env->store.listArchivedGames(fExact4pSp, rows, &total));
        LONGS_EQUAL(1, static_cast<long>(total));
        STRCMP_EQUAL("var_4p_sp_leg1", rows[0].gameId.c_str());

        ArchiveFilter fBad;
        fBad.variant = "BOGUS_VARIANT";
        CHECK_FALSE(env->store.listArchivedGames(fBad, rows, &total));
    }

    /* 2. HTTP /api/v1/archive/games broad mode= vs exact variant= and exact mode= */
    Reply rBroad2p = http().get("/api/v1/archive/games?mode=2P");
    EXPECT_STATUS(200, rBroad2p);
    CHECK_EQUAL(5, rBroad2p.json.value("total", 0));

    Reply rBroad4p = http().get("/api/v1/archive/games?mode=4P");
    EXPECT_STATUS(200, rBroad4p);
    CHECK_EQUAL(4, rBroad4p.json.value("total", 0));

    Reply rBroadOfk = http().get("/api/v1/archive/games?mode=OFK");
    EXPECT_STATUS(200, rBroadOfk);
    CHECK_EQUAL(1, rBroadOfk.json.value("total", 0));
    STRCMP_EQUAL("OFK", rBroadOfk.json["games"][0].value("variant", "").c_str());

    Reply rExact4p = http().get("/api/v1/archive/games?variant=4P");
    EXPECT_STATUS(200, rExact4p);
    CHECK_EQUAL(2, rExact4p.json.value("total", 0));
    STRCMP_EQUAL("var_4p_leg1", rExact4p.json["games"][0].value("game_id", "").c_str());
    STRCMP_EQUAL("4P", rExact4p.json["games"][0].value("variant", "").c_str());
    STRCMP_EQUAL("var_4p_std3", rExact4p.json["games"][1].value("game_id", "").c_str());
    STRCMP_EQUAL("4P", rExact4p.json["games"][1].value("variant", "").c_str());

    Reply rExactViaMode = http().get("/api/v1/archive/games?mode=2p_1b_ed");
    EXPECT_STATUS(200, rExactViaMode);
    CHECK_EQUAL(1, rExactViaMode.json.value("total", 0));
    STRCMP_EQUAL("var_2p_1b_ed", rExactViaMode.json["games"][0].value("game_id", "").c_str());
    STRCMP_EQUAL("2P_1B_ED", rExactViaMode.json["games"][0].value("variant", "").c_str());

    Reply rExact2p3b = http().get("/api/v1/archive/games?variant=2P_3B");
    EXPECT_STATUS(200, rExact2p3b);
    CHECK_EQUAL(1, rExact2p3b.json.value("total", 0));
    STRCMP_EQUAL("2P_3B", rExact2p3b.json["games"][0].value("variant", "").c_str());

    Reply rEmptyVar = http().get("/api/v1/archive/games?variant=2P_3B_ED");
    EXPECT_STATUS(200, rEmptyVar);
    CHECK_EQUAL(0, rEmptyVar.json.value("total", 0));

    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().get("/api/v1/archive/games?variant=INVALID"));
    EXPECT_ERROR(400, "INVALID_ARGUMENT", http().get("/api/v1/archive/games?mode=INVALID"));

    /* 3. HTTP /api/v1/archive/export with mode= and variant= */
    auto countNdjsonLines = [](const std::string &body, std::vector<std::string> &idsOut) {
        idsOut.clear();
        std::stringstream ss(body);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.empty()) {
                continue;
            }
            Json j = Json::parse(line);
            idsOut.push_back(j.value("game_id", ""));
        }
        return static_cast<int>(idsOut.size());
    };

    std::vector<std::string> expIds;
    EXPECT_ERROR(401, "UNAUTHORIZED",
                 http().get("/api/v1/archive/export?variant=4P"));
    Reply rExpExact4p = http().get("/api/v1/archive/export?variant=4P", alice.token);
    EXPECT_STATUS(200, rExpExact4p);
    CHECK_EQUAL(2, countNdjsonLines(rExpExact4p.body, expIds));
    STRCMP_EQUAL("var_4p_leg1", expIds[0].c_str());
    STRCMP_EQUAL("var_4p_std3", expIds[1].c_str());

    Reply rExpBroad4p = http().get("/api/v1/archive/export?mode=4P", alice.token);
    EXPECT_STATUS(200, rExpBroad4p);
    CHECK_EQUAL(4, countNdjsonLines(rExpBroad4p.body, expIds));

    Reply rExpModeExact = http().get("/api/v1/archive/export?mode=2p_1b_sped", alice.token);
    EXPECT_STATUS(200, rExpModeExact);
    CHECK_EQUAL(1, countNdjsonLines(rExpModeExact.body, expIds));
    STRCMP_EQUAL("var_2p_1b_sped", expIds[0].c_str());

    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/archive/export?variant=INVALID", alice.token));

    /* 4. "variant" on archive detail, user recent_games, robot recent_games, table & room views */
    Reply rDetail = http().get("/api/v1/archive/games/var_4p_leg1");
    EXPECT_STATUS(200, rDetail);
    STRCMP_EQUAL("4P", rDetail.json.value("variant", "").c_str());

    Reply rUser = http().get("/api/v1/users/" + alice.uuid);
    EXPECT_STATUS(200, rUser);
    CHECK_TRUE(rUser.json.contains("recent_games") && rUser.json["recent_games"].is_array());
    bool sawUserLeg4p = false;
    for (size_t i = 0; i < rUser.json["recent_games"].size(); ++i) {
        const Json &rg = rUser.json["recent_games"][i];
        CHECK_FALSE(rg.value("variant", "").empty());
        if (rg.value("game_id", "") == "var_4p_leg1") {
            STRCMP_EQUAL("4P", rg.value("variant", "").c_str());
            sawUserLeg4p = true;
        }
    }
    CHECK_TRUE(sawUserLeg4p);

    Reply rRobotProf = http().get("/api/v1/robots/profile/" + botUuid);
    EXPECT_STATUS(200, rRobotProf);
    CHECK_TRUE(rRobotProf.json.contains("recent_games") && rRobotProf.json["recent_games"].is_array());
    bool sawBotLeg4p = false;
    bool sawBot2p3bSp = false;
    for (size_t i = 0; i < rRobotProf.json["recent_games"].size(); ++i) {
        const Json &rg = rRobotProf.json["recent_games"][i];
        if (rg.value("game_id", "") == "var_4p_leg1") {
            STRCMP_EQUAL("4P", rg.value("variant", "").c_str());
            sawBotLeg4p = true;
        } else if (rg.value("game_id", "") == "var_2p_3b_sp") {
            STRCMP_EQUAL("2P_3B_SP", rg.value("variant", "").c_str());
            sawBot2p3bSp = true;
        }
    }
    CHECK_TRUE(sawBotLeg4p);
    CHECK_TRUE(sawBot2p3bSp);

    std::string roomId = createTable(alice, Json{
        {"mode", "2P"},
        {"bases", 1},
        {"super_prongs", true},
        {"edgeless", false}
    });
    Reply rTables = http().get("/api/v1/tables");
    EXPECT_STATUS(200, rTables);
    CHECK_EQUAL(1, static_cast<int>(rTables.json["tables"].size()));
    STRCMP_EQUAL("2P_1B_SP", rTables.json["tables"][0].value("variant", "").c_str());

    Reply rLobby = http().get("/api/v1/lobby/state");
    EXPECT_STATUS(200, rLobby);
    CHECK_EQUAL(1, static_cast<int>(rLobby.json["tables"].size()));
    STRCMP_EQUAL("2P_1B_SP", rLobby.json["tables"][0].value("variant", "").c_str());

    Json rView = view(roomId, alice.token);
    STRCMP_EQUAL("2P_1B_SP", rView.value("variant", "").c_str());

    /* 5. /api/v1/archive/stats "by_variant" counts (legacy 4P bases=1 aggregated into "4P") */
    Reply rStats = http().get("/api/v1/archive/stats");
    EXPECT_STATUS(200, rStats);
    CHECK_EQUAL(10, rStats.json.value("total_games", 0));
    CHECK_TRUE(rStats.json.contains("by_variant") && rStats.json["by_variant"].is_object());
    const Json &bv = rStats.json["by_variant"];
    CHECK_EQUAL(13, static_cast<int>(bv.size()));
    CHECK_EQUAL(1, bv.value("2P_3B", -1));
    CHECK_EQUAL(1, bv.value("2P_3B_SP", -1));
    CHECK_EQUAL(0, bv.value("2P_3B_ED", -1));
    CHECK_EQUAL(0, bv.value("2P_3B_SPED", -1));
    CHECK_EQUAL(1, bv.value("2P_1B", -1));
    CHECK_EQUAL(0, bv.value("2P_1B_SP", -1));
    CHECK_EQUAL(1, bv.value("2P_1B_ED", -1));
    CHECK_EQUAL(1, bv.value("2P_1B_SPED", -1));
    CHECK_EQUAL(2, bv.value("4P", -1));
    CHECK_EQUAL(1, bv.value("4P_SP", -1));
    CHECK_EQUAL(0, bv.value("4P_ED", -1));
    CHECK_EQUAL(1, bv.value("4P_SPED", -1));
    CHECK_EQUAL(1, bv.value("OFK", -1));

    /* 6. OctiNetworkClient round-trip for 13-variant archive queries & model fields */
    auto cli = OctiNetworkClient::createForLoopbackTest(env->server->port());
    auto resExact4p = cli->listArchivedGames("4P");
    CHECK_TRUE(resExact4p.ok);
    LONGS_EQUAL(2, static_cast<long>(resExact4p.data.games.size()));
    STRCMP_EQUAL("4P", resExact4p.data.games[0].variant.c_str());
    STRCMP_EQUAL("4P", resExact4p.data.games[0].variantCode().c_str());

    auto resBroad4p = cli->listArchivedGames("4P_ALL");
    CHECK_TRUE(resBroad4p.ok);
    LONGS_EQUAL(4, static_cast<long>(resBroad4p.data.games.size()));

    auto resBroad2p = cli->listArchivedGames("2P");
    CHECK_TRUE(resBroad2p.ok);
    LONGS_EQUAL(5, static_cast<long>(resBroad2p.data.games.size()));

    auto resExact2p1bEd = cli->listArchivedGames("2P_1B_ED");
    CHECK_TRUE(resExact2p1bEd.ok);
    LONGS_EQUAL(1, static_cast<long>(resExact2p1bEd.data.games.size()));
    STRCMP_EQUAL("2P_1B_ED", resExact2p1bEd.data.games[0].variant.c_str());

    auto resTables = cli->listTables();
    CHECK_TRUE(resTables.ok);
    LONGS_EQUAL(1, static_cast<long>(resTables.data.size()));
    STRCMP_EQUAL("2P_1B_SP", resTables.data[0].variant.c_str());
    STRCMP_EQUAL("2P_1B_SP", resTables.data[0].variantCode().c_str());

    auto resRoom = cli->fetchGameView(roomId);
    CHECK_TRUE(resRoom.ok);
    STRCMP_EQUAL("2P_1B_SP", resRoom.data.variant.c_str());
    STRCMP_EQUAL("2P_1B_SP", resRoom.data.variantCode().c_str());

    /* 7. Web UI HTML #ar-mode <optgroup> + 13 exact options + formatVariantCode */
    Reply rRoot = http().get("/");
    EXPECT_STATUS(200, rRoot);
    CHECK_TRUE(rRoot.body.find("<optgroup") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("2P (All)") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("4P (All)") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("formatVariantCode") != std::string::npos);
    for (const auto &code : octi::allOctiVariantCodes()) {
        std::string optNeedle = "value=\"" + code + "\"";
        CHECK_TRUE_TEXT(rRoot.body.find(optNeedle) != std::string::npos,
                        optNeedle.c_str());
    }
}

TEST(OctiHttpServerIntegrationTest, DualPoolLeaderboardUsersArenaDirectoryAndWebUi)
{
    User alice = signIn("google", "sub-dp-alice", "Alice Dual");
    User bob = signIn("apple", "sub-dp-bob", "Bob Dual");
    User carol = signIn("microsoft", "sub-dp-carol", "Carol Dual");

    /* 1. /api/v1/leaderboard rejects 4P (400) and serves 2P_3B, 2P_1B, OFK, and legacy 2P alias */
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/leaderboard?mode=4P"));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/leaderboard?ladder=4P"));
    EXPECT_STATUS(200, http().get("/api/v1/leaderboard?mode=2P_3B"));
    EXPECT_STATUS(200, http().get("/api/v1/leaderboard?mode=2P_1B"));
    EXPECT_STATUS(200, http().get("/api/v1/leaderboard?mode=OFK"));
    EXPECT_STATUS(200, http().get("/api/v1/leaderboard?mode=2P"));
    EXPECT_STATUS(200, http().get("/api/v1/leaderboard?ladder=2P_1B"));

    /* 2. Casual variants clamp rated=false on POST /api/v1/tables */
    Reply t4p = http().post("/api/v1/tables", alice.token,
                            Json{{"mode", "4P"}, {"rated", true}});
    EXPECT_STATUS(201, t4p);
    std::string r4pId = t4p.json.value("room_id", "");
    CHECK_FALSE(view(r4pId).value("rated", true));
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + r4pId + "/leave",
                                   alice.token, Json::object()));

    Reply tSp = http().post("/api/v1/tables", alice.token,
                            Json{{"mode", "2P"}, {"bases", 3},
                                 {"super_prongs", true}, {"rated", true}});
    EXPECT_STATUS(201, tSp);
    std::string rSpId = tSp.json.value("room_id", "");
    CHECK_FALSE(view(rSpId).value("rated", true));
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + rSpId + "/leave",
                                   alice.token, Json::object()));

    /* 3. Pool 1 Human-vs-Human 2P_1B rated game (8 plies + resign) */
    std::vector<User> ab = {alice, bob};
    std::string room1b = startGame(ab, Json{{"mode", "2P"}, {"bases", 1}, {"rated", true}});
    CHECK_EQUAL(8, playPlies(room1b, ab, GAME_MODE_2, 8, Json{{"bases", 1}}));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room1b + "/resign",
                                   bob.token, Json::object()));

    Reply lb1b = http().get("/api/v1/leaderboard?mode=2P_1B");
    EXPECT_STATUS(200, lb1b);
    STRCMP_EQUAL(alice.uuid.c_str(),
                 lb1b.json["entries"][0].value("uuid", "").c_str());
    CHECK_EQUAL(1216, lb1b.json["entries"][0].value("elo", 0));

    /* 2P_3B ladder remains 1200 for Alice */
    Reply lb3b = http().get("/api/v1/leaderboard?mode=2P_3B");
    EXPECT_STATUS(200, lb3b);
    for (size_t i = 0; i < lb3b.json["entries"].size(); ++i) {
        if (lb3b.json["entries"][i].value("uuid", "") == alice.uuid) {
            CHECK_EQUAL(1200, lb3b.json["entries"][i].value("elo", 0));
        }
    }

    /* 4. Pool 2 Cross-Owner Human-vs-Robot 2P_3B rated game (Alice vs Carol's bot) */
    Reply createBot = http().post("/api/v1/robots", carol.token,
                                  Json{{"robot_name", "CarolArenaBot"},
                                       {"description", "Carol Arena Bot"}});
    EXPECT_STATUS(201, createBot);
    std::string botToken = createBot.json.value("robot_token", "");
    std::string botUuid = createBot.json.value("robot_uuid", "");
    CHECK_FALSE(botToken.empty());

    User botUser;
    botUser.uuid = botUuid;
    botUser.handle = "CarolArenaBot";
    botUser.token = botToken;

    Reply hostArena = http().post("/api/v1/tables", alice.token,
                                  Json{{"mode", "2P"}, {"bases", 3},
                                       {"rated", true}, {"allow_robots", true}});
    EXPECT_STATUS(201, hostArena);
    std::string arenaRoom = hostArena.json.value("room_id", "");
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + arenaRoom + "/join",
                                   botToken, Json{{"seat", 2}}));

    std::vector<User> aliceAndBot = {alice, botUser};
    CHECK_EQUAL(8, playPlies(arenaRoom, aliceAndBot, GAME_MODE_2, 8, Json{{"bases", 3}}));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + arenaRoom + "/resign",
                                   botToken, Json::object()));

    /* Archive summary exposes rated and elo_pool */
    Reply archList = http().get("/api/v1/archive/games?limit=10");
    EXPECT_STATUS(200, archList);
    bool sawPool1 = false;
    bool sawPool2 = false;
    for (size_t i = 0; i < archList.json["games"].size(); ++i) {
        const Json &gm = archList.json["games"][i];
        if (gm.value("game_id", "") == room1b) {
            CHECK_TRUE(gm.value("rated", false));
            CHECK_EQUAL(1, gm.value("elo_pool", -1));
            sawPool1 = true;
        } else if (gm.value("game_id", "") == arenaRoom) {
            CHECK_TRUE(gm.value("rated", false));
            CHECK_EQUAL(2, gm.value("elo_pool", -1));
            sawPool2 = true;
        }
    }
    CHECK_TRUE(sawPool1);
    CHECK_TRUE(sawPool2);

    /* 5. /api/v1/profile/me, /api/v1/users, /api/v1/users/:id, /api/v1/robots/profile/:id, /api/v1/robots */
    Reply aliceMe = http().get("/api/v1/profile/me", alice.token);
    EXPECT_STATUS(200, aliceMe);
    CHECK_EQUAL(0, aliceMe.json.value("games_2p_3b", -1));
    CHECK_EQUAL(1, aliceMe.json.value("games_2p_1b", -1));
    CHECK_EQUAL(0, aliceMe.json.value("games_ofk", -1));
    CHECK_EQUAL(1, aliceMe.json.value("arena_games_2p_3b", -1));
    CHECK_EQUAL(0, aliceMe.json.value("arena_games_2p_1b", -1));
    CHECK_EQUAL(0, aliceMe.json.value("arena_games_ofk", -1));

    Reply uSort1b = http().get("/api/v1/users?sort=elo_2p_1b&order=desc");
    EXPECT_STATUS(200, uSort1b);
    STRCMP_EQUAL(alice.uuid.c_str(),
                 uSort1b.json["users"][0].value("uuid", "").c_str());
    STRCMP_EQUAL(bob.uuid.c_str(),
                 uSort1b.json["users"][1].value("uuid", "").c_str());
    STRCMP_EQUAL(carol.uuid.c_str(),
                 uSort1b.json["users"][2].value("uuid", "").c_str());
    CHECK_EQUAL(1200, uSort1b.json["users"][0].value("elo_2p_3b", 0));
    CHECK_EQUAL(1216, uSort1b.json["users"][0].value("elo_2p_1b", 0));
    CHECK_EQUAL(1200, uSort1b.json["users"][0].value("elo_ofk", 0));
    CHECK_EQUAL(0, uSort1b.json["users"][0].value("games_2p_3b", -1));
    CHECK_EQUAL(1, uSort1b.json["users"][0].value("games_2p_1b", -1));
    CHECK_EQUAL(0, uSort1b.json["users"][0].value("games_ofk", -1));
    CHECK_EQUAL(1216, uSort1b.json["users"][0].value("arena_elo_2p_3b", 0));
    CHECK_EQUAL(1, uSort1b.json["users"][0].value("arena_games_2p_3b", -1));
    CHECK_EQUAL(0, uSort1b.json["users"][0].value("arena_games_2p_1b", -1));
    CHECK_EQUAL(0, uSort1b.json["users"][0].value("arena_games_ofk", -1));
    CHECK_EQUAL(1, uSort1b.json["users"][0].value("arena_games_played", 0));

    Reply aliceDetail = http().get("/api/v1/users/" + alice.uuid);
    EXPECT_STATUS(200, aliceDetail);
    CHECK_EQUAL(1200, aliceDetail.json.value("elo_2p_3b", 0));
    CHECK_EQUAL(1216, aliceDetail.json.value("elo_2p_1b", 0));
    CHECK_EQUAL(0, aliceDetail.json.value("games_2p_3b", -1));
    CHECK_EQUAL(1, aliceDetail.json.value("games_2p_1b", -1));
    CHECK_EQUAL(0, aliceDetail.json.value("games_ofk", -1));
    CHECK_EQUAL(1216, aliceDetail.json.value("arena_elo_2p_3b", 0));
    CHECK_EQUAL(1, aliceDetail.json.value("arena_games_2p_3b", -1));
    CHECK_EQUAL(0, aliceDetail.json.value("arena_games_2p_1b", -1));
    CHECK_EQUAL(0, aliceDetail.json.value("arena_games_ofk", -1));
    CHECK_EQUAL(1, aliceDetail.json.value("arena_games_played", 0));

    Reply carolDetail = http().get("/api/v1/users/" + carol.uuid);
    EXPECT_STATUS(200, carolDetail);
    CHECK_EQUAL(1, static_cast<int>(carolDetail.json["robots"].size()));
    CHECK_EQUAL(1, carolDetail.json["robots"][0].value("games_2p_3b", -1));
    CHECK_EQUAL(0, carolDetail.json["robots"][0].value("games_2p_1b", -1));
    CHECK_EQUAL(0, carolDetail.json["robots"][0].value("games_ofk", -1));

    Reply botProf = http().get("/api/v1/robots/profile/" + botUuid);
    EXPECT_STATUS(200, botProf);
    CHECK_EQUAL(1184, botProf.json.value("elo_2p_3b", 0));
    CHECK_EQUAL(1200, botProf.json.value("elo_2p_1b", 0));
    CHECK_EQUAL(1200, botProf.json.value("elo_ofk", 0));
    CHECK_EQUAL(1, botProf.json.value("games_2p_3b", -1));
    CHECK_EQUAL(0, botProf.json.value("games_2p_1b", -1));
    CHECK_EQUAL(0, botProf.json.value("games_ofk", -1));
    CHECK_EQUAL(1, botProf.json.value("games_played", 0));

    Reply carolBots = http().get("/api/v1/robots", carol.token);
    EXPECT_STATUS(200, carolBots);
    CHECK_EQUAL(1184, carolBots.json["robots"][0].value("elo_2p_3b", 0));
    CHECK_EQUAL(1, carolBots.json["robots"][0].value("games_2p_3b", -1));
    CHECK_EQUAL(0, carolBots.json["robots"][0].value("games_2p_1b", -1));
    CHECK_EQUAL(0, carolBots.json["robots"][0].value("games_ofk", -1));

    /* 6. /api/v1/robots/directory merged Arena standings: all | robots | humans */
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/robots/directory?participant_type=invalid"));

    Reply dirAll = http().get("/api/v1/robots/directory?participant_type=all&sort=elo_2p_3b&order=desc");
    EXPECT_STATUS(200, dirAll);
    CHECK_EQUAL(2, dirAll.json.value("total", 0));
    CHECK_EQUAL(2, static_cast<int>(dirAll.json["entries"].size()));
    CHECK_EQUAL(2, static_cast<int>(dirAll.json["robots"].size()));
    STRCMP_EQUAL("human", dirAll.json["entries"][0].value("entry_type", "").c_str());
    STRCMP_EQUAL(alice.uuid.c_str(), dirAll.json["entries"][0].value("uuid", "").c_str());
    CHECK_EQUAL(1216, dirAll.json["entries"][0].value("elo_2p_3b", 0));
    CHECK_EQUAL(1, dirAll.json["entries"][0].value("games_2p_3b", -1));
    CHECK_EQUAL(0, dirAll.json["entries"][0].value("games_2p_1b", -1));
    CHECK_EQUAL(0, dirAll.json["entries"][0].value("games_ofk", -1));
    STRCMP_EQUAL("robot", dirAll.json["entries"][1].value("entry_type", "").c_str());
    STRCMP_EQUAL(botUuid.c_str(), dirAll.json["entries"][1].value("robot_uuid", "").c_str());
    CHECK_EQUAL(1184, dirAll.json["entries"][1].value("elo_2p_3b", 0));
    CHECK_EQUAL(1, dirAll.json["entries"][1].value("games_2p_3b", -1));
    CHECK_EQUAL(0, dirAll.json["entries"][1].value("games_2p_1b", -1));
    CHECK_EQUAL(0, dirAll.json["entries"][1].value("games_ofk", -1));

    Reply dirRobots = http().get("/api/v1/robots/directory?participant_type=robots");
    EXPECT_STATUS(200, dirRobots);
    CHECK_EQUAL(1, dirRobots.json.value("total", 0));
    STRCMP_EQUAL("robot", dirRobots.json["entries"][0].value("entry_type", "").c_str());

    Reply dirHumans = http().get("/api/v1/robots/directory?participant_type=humans");
    EXPECT_STATUS(200, dirHumans);
    CHECK_EQUAL(1, dirHumans.json.value("total", 0));
    STRCMP_EQUAL("human", dirHumans.json["entries"][0].value("entry_type", "").c_str());

    /* 6b. GET /api/v1/leaderboard Pool 1 & Pool 2 (pool=arena) + fault cases + POST /api/v1/admin/recompute-ratings */
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/leaderboard?mode=2P_3B&pool=invalid"));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/leaderboard?mode=2P_3B&pool=arena&participant_type=invalid"));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/leaderboard?mode=2P_3B&pool=human&participant_type=robots"));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/leaderboard?mode=4P&pool=arena"));

    Reply lbHuman1b = http().get("/api/v1/leaderboard?mode=2P_1B&pool=human");
    EXPECT_STATUS(200, lbHuman1b);
    STRCMP_EQUAL("human", lbHuman1b.json.value("pool", "").c_str());
    CHECK_EQUAL(3, static_cast<int>(lbHuman1b.json["entries"].size()));
    STRCMP_EQUAL(alice.uuid.c_str(), lbHuman1b.json["entries"][0].value("uuid", "").c_str());
    CHECK_EQUAL(1, lbHuman1b.json["entries"][0].value("rank", -1));
    CHECK_EQUAL(1, lbHuman1b.json["entries"][0].value("ladder_games", -1));
    CHECK_TRUE(lbHuman1b.json["entries"][0].value("ranked", false));
    STRCMP_EQUAL(bob.uuid.c_str(), lbHuman1b.json["entries"][1].value("uuid", "").c_str());
    CHECK_EQUAL(2, lbHuman1b.json["entries"][1].value("rank", -1));
    CHECK_EQUAL(1, lbHuman1b.json["entries"][1].value("ladder_games", -1));
    CHECK_TRUE(lbHuman1b.json["entries"][1].value("ranked", false));
    STRCMP_EQUAL(carol.uuid.c_str(), lbHuman1b.json["entries"][2].value("uuid", "").c_str());
    CHECK_EQUAL(0, lbHuman1b.json["entries"][2].value("rank", -1));
    CHECK_EQUAL(0, lbHuman1b.json["entries"][2].value("ladder_games", -1));
    CHECK_FALSE(lbHuman1b.json["entries"][2].value("ranked", true));

    Reply lbArena3b = http().get("/api/v1/leaderboard?mode=2P_3B&pool=arena&participant_type=all");
    EXPECT_STATUS(200, lbArena3b);
    STRCMP_EQUAL("arena", lbArena3b.json.value("pool", "").c_str());
    STRCMP_EQUAL("all", lbArena3b.json.value("participant_type", "").c_str());
    CHECK_EQUAL(2, lbArena3b.json.value("total", 0));
    CHECK_EQUAL(2, static_cast<int>(lbArena3b.json["entries"].size()));
    STRCMP_EQUAL("human", lbArena3b.json["entries"][0].value("entry_type", "").c_str());
    CHECK_FALSE(lbArena3b.json["entries"][0].value("is_robot", true));
    STRCMP_EQUAL(alice.uuid.c_str(), lbArena3b.json["entries"][0].value("user_uuid", "").c_str());
    CHECK_EQUAL(1, lbArena3b.json["entries"][0].value("rank", -1));
    CHECK_EQUAL(1, lbArena3b.json["entries"][0].value("ladder_games", -1));
    CHECK_TRUE(lbArena3b.json["entries"][0].value("ranked", false));
    CHECK_EQUAL(1216, lbArena3b.json["entries"][0].value("elo", 0));
    STRCMP_EQUAL("robot", lbArena3b.json["entries"][1].value("entry_type", "").c_str());
    CHECK_TRUE(lbArena3b.json["entries"][1].value("is_robot", false));
    STRCMP_EQUAL(botUuid.c_str(), lbArena3b.json["entries"][1].value("robot_uuid", "").c_str());
    CHECK_EQUAL(2, lbArena3b.json["entries"][1].value("rank", -1));
    CHECK_EQUAL(1, lbArena3b.json["entries"][1].value("ladder_games", -1));
    CHECK_TRUE(lbArena3b.json["entries"][1].value("ranked", false));
    CHECK_EQUAL(1184, lbArena3b.json["entries"][1].value("elo", 0));

    Reply lbArenaRobots = http().get("/api/v1/leaderboard?mode=2P_3B&pool=arena&participant_type=robots");
    EXPECT_STATUS(200, lbArenaRobots);
    CHECK_EQUAL(1, static_cast<int>(lbArenaRobots.json["entries"].size()));
    STRCMP_EQUAL("robot", lbArenaRobots.json["entries"][0].value("entry_type", "").c_str());

    Reply lbArenaHumans = http().get("/api/v1/leaderboard?mode=2P_3B&pool=arena&participant_type=humans");
    EXPECT_STATUS(200, lbArenaHumans);
    CHECK_EQUAL(1, static_cast<int>(lbArenaHumans.json["entries"].size()));
    STRCMP_EQUAL("human", lbArenaHumans.json["entries"][0].value("entry_type", "").c_str());

    Reply lbArenaOfk = http().get("/api/v1/leaderboard?mode=OFK&pool=arena");
    EXPECT_STATUS(200, lbArenaOfk);
    CHECK_EQUAL(2, static_cast<int>(lbArenaOfk.json["entries"].size()));
    for (size_t i = 0; i < lbArenaOfk.json["entries"].size(); ++i) {
        CHECK_EQUAL(0, lbArenaOfk.json["entries"][i].value("rank", -1));
        CHECK_EQUAL(0, lbArenaOfk.json["entries"][i].value("ladder_games", -1));
        CHECK_FALSE(lbArenaOfk.json["entries"][i].value("ranked", true));
        CHECK_EQUAL(1200, lbArenaOfk.json["entries"][i].value("elo", 0));
    }

    /* Admin recompute-ratings endpoint auth & execution */
    EXPECT_STATUS(401, http().post("/api/v1/admin/recompute-ratings", "", Json::object()));
    EXPECT_STATUS(403, http().post("/api/v1/admin/recompute-ratings", bob.token, Json::object()));
    EXPECT_STATUS(403, http().post("/api/v1/admin/recompute-ratings", botToken, Json::object()));
    env->store.setAdminGoogleIdentities({"sub-dp-alice"});
    Reply recomputeOk = http().post("/api/v1/admin/recompute-ratings", alice.token, Json::object());
    EXPECT_STATUS(200, recomputeOk);
    CHECK_TRUE(recomputeOk.json.value("ok", false));
    CHECK_EQUAL(2, recomputeOk.json.value("games_scanned", -1));
    CHECK_EQUAL(1, recomputeOk.json.value("pool1_games_rated", -1));
    CHECK_EQUAL(1, recomputeOk.json.value("pool2_games_rated", -1));
    CHECK_EQUAL(2, recomputeOk.json.value("players_updated", -1));
    CHECK_EQUAL(1, recomputeOk.json.value("robots_updated", -1));

    /* 7. OctiNetworkClient Dual-Pool parsing & directory filtering */
    auto cli = OctiNetworkClient::createForLoopbackTest(env->server->port());
    auto cli4p = cli->fetchLeaderboard("4P");
    CHECK_FALSE(cli4p.ok);
    CHECK_EQUAL(400, cli4p.httpStatus);

    auto cli1b = cli->fetchLeaderboard("2P_1B");
    CHECK_TRUE(cli1b.ok);
    CHECK_EQUAL(1216, cli1b.data[0].elo);

    auto cliAlice = cli->fetchPlayerProfile(alice.uuid);
    CHECK_TRUE(cliAlice.ok);
    CHECK_EQUAL(1200, cliAlice.data.elo2p3b);
    CHECK_EQUAL(1216, cliAlice.data.elo2p1b);
    CHECK_EQUAL(1216, cliAlice.data.arenaElo2p3b);
    CHECK_EQUAL(1, cliAlice.data.arenaGamesPlayed);

    auto cliDirAll = cli->listRobotsDirectory("", "all", "elo_2p_3b", false);
    CHECK_TRUE(cliDirAll.ok);
    CHECK_EQUAL(2, static_cast<int>(cliDirAll.data.size()));
    STRCMP_EQUAL("human", cliDirAll.data[0].entryType.c_str());
    CHECK_EQUAL(1216, cliDirAll.data[0].elo2p3b);
    STRCMP_EQUAL("robot", cliDirAll.data[1].entryType.c_str());
    CHECK_EQUAL(1184, cliDirAll.data[1].elo2p3b);

    auto cliDirHumans = cli->listRobotsDirectory("", "humans", "elo_2p_3b", false);
    CHECK_TRUE(cliDirHumans.ok);
    CHECK_EQUAL(1, static_cast<int>(cliDirHumans.data.size()));
    STRCMP_EQUAL("human", cliDirHumans.data[0].entryType.c_str());

    /* 8. Web UI HTML Dual-Pool markers + zero duplicate DOM IDs + www/octi parity */
    Reply ui = http().get("/");
    EXPECT_STATUS(200, ui);
    CHECK_TRUE(ui.body == std::string(octi::webUiHtml()));
    CHECK_TRUE(ui.body.find("OCTIUI") == std::string::npos);
    CHECK_TRUE(ui.body.find("OCTI v2.6.4") != std::string::npos);
    CHECK_TRUE(ui.body.find("id=\"robots-participant-type\"") != std::string::npos);
    CHECK_TRUE(ui.body.find("value=\"elo_2p_3b\"") != std::string::npos);
    CHECK_TRUE(ui.body.find("value=\"elo_2p_1b\"") != std::string::npos);
    CHECK_TRUE(ui.body.find("id=\"lb-human-panel\"") != std::string::npos);
    CHECK_TRUE(ui.body.find("id=\"lb-arena-panel\"") != std::string::npos);
    CHECK_TRUE(ui.body.find("id=\"lb-arena-type\"") != std::string::npos);
    CHECK_TRUE(ui.body.find("id=\"lb-arena-msg\"") != std::string::npos);
    CHECK_TRUE(ui.body.find("id=\"lb-arena-table\"") != std::string::npos);
    CHECK_TRUE(ui.body.find("value=\"overall\" selected>Overall (All Ladders)</option>") != std::string::npos);
    CHECK_TRUE(ui.body.find("id=\"lb-sort-wrap\"") != std::string::npos);
    CHECK_TRUE(ui.body.find("id=\"lb-sort\"") != std::string::npos);
    CHECK_TRUE(ui.body.find("id=\"lb-order\"") != std::string::npos);
    CHECK_TRUE(ui.body.find("played_only=1") != std::string::npos);
    CHECK_TRUE(ui.body.find("Lv1: AI_L1 (5s)") != std::string::npos);
    CHECK_TRUE(ui.body.find("Lv5: AI_L5 (60s)") != std::string::npos);
    CHECK_TRUE(ui.body.find("AI_Grandmaster") == std::string::npos);
    CHECK_TRUE(ui.body.find("AI_Expert") == std::string::npos);
    CHECK_TRUE(ui.body.find("AI_Master") == std::string::npos);
    CHECK_TRUE(ui.body.find("AI_Tactical") == std::string::npos);
    CHECK_TRUE(ui.body.find("AI_Casual") == std::string::npos);
    CHECK_TRUE(ui.body.find("Lv4: Expert") == std::string::npos);
    CHECK_TRUE(ui.body.find("Lv5: Grandmaster") == std::string::npos);
    CHECK_TRUE(ui.body.find("explore grandmaster replays") == std::string::npos);
    CHECK_TRUE(ui.body.find("id=\"web-roster-entity-modal\"") != std::string::npos);
    CHECK_TRUE(ui.body.find("id=\"web-host-hover-card\"") != std::string::npos);
    CHECK_TRUE(ui.body.find("openOnlineEntityDialog") != std::string::npos);
    CHECK_TRUE(ui.body.find("Click or tap for details") != std::string::npos);
    CHECK_TRUE(ui.body.find("id=\"btn-admin-recompute-ratings\"") != std::string::npos);
    CHECK_TRUE(ui.body.find("id=\"admin-recompute-msg\"") != std::string::npos);
    CHECK_TRUE(ui.body.find("function fmtElo(elo, gamesCount)") != std::string::npos);
    CHECK_TRUE(ui.body.find("/api/v1/admin/recompute-ratings") != std::string::npos);
    CHECK_TRUE(ui.body.find("pool=arena") != std::string::npos);
    CHECK_TRUE(ui.body.find("Only 2P_3B, 2P_1B, and OFK are rated; 4P / SP / ED are casual.") !=
               std::string::npos);
    CHECK_TRUE(ui.body.find("Rating Pools") != std::string::npos);

    {
        std::set<std::string> seenDomIds;
        std::size_t pos = 0u;
        while ((pos = ui.body.find("id=\"", pos)) != std::string::npos) {
            if (pos > 0u) {
                const char prev = ui.body[pos - 1u];
                if (prev != ' ' && prev != '\t' && prev != '\n' && prev != '\r') {
                    pos += 4u;
                    continue;
                }
            }
            const std::size_t valStart = pos + 4u;
            const std::size_t valEnd = ui.body.find('"', valStart);
            CHECK_TRUE(valEnd != std::string::npos);
            const std::string domId = ui.body.substr(valStart, valEnd - valStart);
            CHECK_FALSE(domId.empty());
            const bool inserted = seenDomIds.insert(domId).second;
            if (!inserted) {
                FAIL(("Duplicate DOM id in WebUi.cxx: " + domId).c_str());
            }
            pos = valEnd + 1u;
        }
        CHECK_TRUE(seenDomIds.size() >= 340u);
    }

    {
        const std::filesystem::path testFilePath(__FILE__);
        const std::filesystem::path repoRoot =
            testFilePath.parent_path().parent_path().parent_path();
        const std::filesystem::path wwwIndexHtml =
            repoRoot.parent_path() / "www" / "octi" / "html" / "index.html";
        if (std::filesystem::exists(wwwIndexHtml)) {
            std::ifstream wfs(wwwIndexHtml, std::ios::binary);
            std::string wwwBytes((std::istreambuf_iterator<char>(wfs)),
                                 std::istreambuf_iterator<char>());
            CHECK_TRUE(wwwBytes.find("OCTIUI") == std::string::npos);
            LONGS_EQUAL(static_cast<long>(ui.body.size()),
                        static_cast<long>(wwwBytes.size()));
            CHECK_EQUAL(0, std::memcmp(ui.body.data(), wwwBytes.data(), ui.body.size()));
        }
    }
}

TEST(OctiHttpServerIntegrationTest, IllustratedRulebookAssetsEndpointAndTabParity)
{
    /* 1. Embedded rulebook PNG descriptors & HTTP /assets/rulebook/:file parity */
    const std::vector<std::string> files = octi::rulebookImageFileNames();
    LONGS_EQUAL(static_cast<long>(octi::OCTI_RULEBOOK_IMAGE_COUNT),
                static_cast<long>(files.size()));
    LONGS_EQUAL(12, static_cast<long>(files.size()));

    const std::filesystem::path testFilePath(__FILE__);
    const std::filesystem::path repoRoot = testFilePath.parent_path().parent_path().parent_path();
    const std::filesystem::path repoRulebookDir = repoRoot / "assets" / "rulebook";
    const std::filesystem::path wwwRulebookDir =
        repoRoot.parent_path() / "www" / "octi" / "html" / "assets" / "rulebook";

    static const std::uint8_t kPngSig[8] = {
        0x89u, 0x50u, 0x4Eu, 0x47u, 0x0Du, 0x0Au, 0x1Au, 0x0Au
    };

    std::size_t totalBytes = 0u;
    for (const std::string &fileName : files) {
        std::size_t pngSize = 0u;
        const std::uint8_t *pngBytes = octi::rulebookImageBytes(fileName, pngSize);
        CHECK_TRUE(pngBytes != nullptr);
        CHECK_TRUE(pngSize >= 1024u);
        CHECK_TRUE(pngSize <= 80u * 1024u);
        CHECK_EQUAL(0, std::memcmp(pngBytes, kPngSig, sizeof(kPngSig)));
        totalBytes += pngSize;

        Reply rImg = http().get("/assets/rulebook/" + fileName);
        EXPECT_STATUS(200, rImg);
        CHECK_TRUE(rImg.contentType.find("image/png") != std::string::npos);
        STRCMP_EQUAL("public, max-age=86400", rImg.header("Cache-Control").c_str());
        LONGS_EQUAL(static_cast<long>(pngSize), static_cast<long>(rImg.body.size()));
        CHECK_EQUAL(0, std::memcmp(rImg.body.data(), pngBytes, pngSize));

        /* Disk parity with canonical repo assets/rulebook/<fileName> */
        const std::filesystem::path diskPath = repoRulebookDir / fileName;
        CHECK_TRUE(std::filesystem::exists(diskPath));
        std::ifstream ifs(diskPath, std::ios::binary);
        std::string diskBytes((std::istreambuf_iterator<char>(ifs)),
                              std::istreambuf_iterator<char>());
        LONGS_EQUAL(static_cast<long>(pngSize), static_cast<long>(diskBytes.size()));
        CHECK_EQUAL(0, std::memcmp(diskBytes.data(), pngBytes, pngSize));

        if (std::filesystem::exists(wwwRulebookDir)) {
            const std::filesystem::path wwwPath = wwwRulebookDir / fileName;
            CHECK_TRUE(std::filesystem::exists(wwwPath));
            std::ifstream wfs(wwwPath, std::ios::binary);
            std::string wwwBytes((std::istreambuf_iterator<char>(wfs)),
                                 std::istreambuf_iterator<char>());
            LONGS_EQUAL(static_cast<long>(pngSize), static_cast<long>(wwwBytes.size()));
            CHECK_EQUAL(0, std::memcmp(wwwBytes.data(), pngBytes, pngSize));
        }
    }
    CHECK_TRUE(totalBytes < 1024u * 1024u);

    /* 2. Strict allowlist & path-traversal rejection */
    std::size_t badSize = 99u;
    CHECK_TRUE(octi::rulebookImageBytes("nonexistent.png", badSize) == nullptr);
    LONGS_EQUAL(0, static_cast<long>(badSize));
    CHECK_TRUE(octi::rulebookImageBytes("../sounds/pod_place.wav", badSize) == nullptr);

    EXPECT_ERROR(404, "NOT_FOUND", http().get("/assets/rulebook/nonexistent.png"));
    EXPECT_ERROR(404, "NOT_FOUND", http().get("/assets/rulebook/..%2fetc%2fpasswd"));
    EXPECT_ERROR(404, "NOT_FOUND", http().get("/assets/rulebook/rule_01_2p_3b_setup.svg"));
    EXPECT_STATUS(404, http().get("/assets/rulebook/.."));
    EXPECT_STATUS(404, http().get("/assets/rulebook/"));

    /* 3. Web UI #tab-rules, nav button, 7 chapters, 13 variants, and #tab-home upgrade */
    Reply rUi = http().get("/");
    EXPECT_STATUS(200, rUi);
    CHECK_TRUE(rUi.body.find("id=\"nav-rules-btn\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("data-tab=\"rules\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"tab-rules\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"home-cta-rules\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("Chapter 1") != std::string::npos);
    CHECK_TRUE(rUi.body.find("Chapter 2") != std::string::npos);
    CHECK_TRUE(rUi.body.find("Chapter 3") != std::string::npos);
    CHECK_TRUE(rUi.body.find("Chapter 4") != std::string::npos);
    CHECK_TRUE(rUi.body.find("Chapter 5") != std::string::npos);
    CHECK_TRUE(rUi.body.find("Chapter 6") != std::string::npos);
    CHECK_TRUE(rUi.body.find("Chapter 7") != std::string::npos);
    CHECK_TRUE(rUi.body.find("4. Victory &amp; 4P Elimination") != std::string::npos);
    CHECK_TRUE(rUi.body.find("44a-45,44beg-66x-64x-44") != std::string::npos);
    CHECK_TRUE(rUi.body.find("44bcd-66-86-16-34-54") != std::string::npos);
    CHECK_TRUE(rUi.body.find("whole-stack capture") != std::string::npos);
    CHECK_TRUE(rUi.body.find("2P_3B*") != std::string::npos);
    CHECK_TRUE(rUi.body.find("1 bonus prelude move per SuperProng") != std::string::npos);
    CHECK_TRUE(rUi.body.find("permanently eliminated") != std::string::npos);
    CHECK_TRUE(rUi.body.find("capturing enemy player") != std::string::npos);
    CHECK_TRUE(rUi.body.find("4. Victory &amp; 4P Revival") == std::string::npos);
    CHECK_TRUE(rUi.body.find("Teammate Revival") == std::string::npos);
    CHECK_TRUE(rUi.body.find("Reposition a Normal Prong") == std::string::npos);
    CHECK_TRUE(rUi.body.find("revive eliminated 4P partner") == std::string::npos);
    CHECK_TRUE(rUi.body.find("33b+a-b") == std::string::npos);

    /* Cross-surface R1-R6 parity with Desktop OctiRulebook.cxx, Android rulebook_mobile.html, and doc/OCTI*.md */
    {
        const std::string desktopHtml = octi::buildDesktopRulebookHtml(false);
        const std::filesystem::path mobileHtmlPath = repoRulebookDir / "rulebook_mobile.html";
        CHECK_TRUE(std::filesystem::exists(mobileHtmlPath));
        std::ifstream mfs(mobileHtmlPath, std::ios::binary);
        const std::string mobileHtml((std::istreambuf_iterator<char>(mfs)),
                                     std::istreambuf_iterator<char>());

        for (const std::string *surfPtr : { &desktopHtml, &mobileHtml }) {
            const std::string &surf = *surfPtr;
            CHECK_TRUE(surf.find("4. Victory &amp; 4P Elimination") != std::string::npos);
            CHECK_TRUE(surf.find("44a-45,44beg-66x-64x-44") != std::string::npos);
            CHECK_TRUE(surf.find("44bcd-66-86-16-34-54") != std::string::npos);
            CHECK_TRUE(surf.find("whole-stack capture") != std::string::npos);
            CHECK_TRUE(surf.find("2P_3B*") != std::string::npos);
            CHECK_TRUE(surf.find("1 bonus prelude move per SuperProng") != std::string::npos);
            CHECK_TRUE(surf.find("permanently eliminated") != std::string::npos);
            CHECK_TRUE(surf.find("capturing enemy player") != std::string::npos);
            CHECK_TRUE(surf.find("4. Victory &amp; 4P Revival") == std::string::npos);
            CHECK_TRUE(surf.find("Teammate Revival") == std::string::npos);
            CHECK_TRUE(surf.find("Reposition a Normal Prong") == std::string::npos);
            CHECK_TRUE(surf.find("revive eliminated 4P partner") == std::string::npos);
            CHECK_TRUE(surf.find("33b+a-b") == std::string::npos);
        }

        const char *const kDocFiles[] = {
            "octi_game_instructions.md",
            "octi_notation.md",
            "octi_gui_design_book.md",
            "octi_for_kids_instructions.md"
        };
        for (const char *docName : kDocFiles) {
            const std::filesystem::path docPath = repoRoot / "doc" / docName;
            CHECK_TRUE(std::filesystem::exists(docPath));
            std::ifstream dfs(docPath, std::ios::binary);
            const std::string docText((std::istreambuf_iterator<char>(dfs)),
                                      std::istreambuf_iterator<char>());
            CHECK_TRUE(docText.find("33b+a-b") == std::string::npos);
            CHECK_TRUE(docText.find("33a+b-a") == std::string::npos);
            CHECK_TRUE(docText.find("33a-a+b") == std::string::npos);
            CHECK_TRUE(docText.find("Teammate Revival") == std::string::npos);
            CHECK_TRUE(docText.find("liberated player may then play normally") == std::string::npos);
            CHECK_TRUE(docText.find("whole starting stack counts as vacated") == std::string::npos);
        }
    }

    for (const std::string &fileName : files) {
        CHECK_TRUE(rUi.body.find("/assets/rulebook/" + fileName) != std::string::npos);
    }
    for (const std::string &vCode : octi::allOctiVariantCodes()) {
        CHECK_TRUE(rUi.body.find(vCode) != std::string::npos);
    }

    /* 4. Engine legality verification of featured Rulebook FENs & moves */
    {
        OctiBoard b5;
        b5.fromPositionString(
            "2:3:0:0:P:A:-:0:3,0,21,0:3,0,23,0:A33abd,A53,A73,A44h,B56e,B66c,B37,B77");
        b5.getGame()->move("33abd-55-57x-75x");

        OctiBoard b6a;
        b6a.fromPositionString(
            "2:3:0:0:P:A:-:0:2,0,20,0:3,0,23,0:A53,A73,A44a,A44beg,A54c,B55e,B65e,B37,B77");
        b6a.getGame()->move("44a-45,44beg-34");

        OctiBoard b6b;
        b6b.fromPositionString(
            "2:3:0:0:P:A:-:0:2,0,20,0:3,0,23,0:A53,A73,A44a,A44beg,A54c,B55e,B65e,B37,B77");
        CHECK_THROWS(IllegalMoveException, b6b.getGame()->move("44beg-66x-64x-44"));
        b6b.getGame()->move("44a-45,44beg-66x-64x-44");

        OctiBoard b7;
        b7.fromPositionString(
            "2:3:0:0:P:A:-:0:3,1,22,0:3,0,23,0:A53a,A73,B36e,B76e,B37,A57ab,B77");
        b7.getGame()->move("53*");

        OctiBoard b8;
        b8.fromPositionString(
            "2:3:0:1:P:A:-:0:3,0,23,0:3,0,24,0:A53,A73,A15g,B37,B57,B77,A97b,B18e");
        b8.getGame()->move("97b-29x");

        OctiBoard b9;
        b9.fromPositionString(
            "2:3:1:0:P:A:-:0:4,0,23,0:4,0,24,0:A53,A73,A44ab!,B55e!,B37,B77");
        b9.getGame()->move("44ab!-66x;66ab!-67");

        OctiBoard b11;
        b11.fromPositionString(
            "OFK:1:0:0:P:A:-:0:0,0,10,0:0,0,10,0:A22,A42b,A52,A34a,B35e,B45e,B26,B56");
        b11.getGame()->move("34a-36x");
        CHECK_TRUE(b11.getGame()->isGameOver());
        CHECK_EQUAL(PLAYER_A, b11.getGame()->getWinner());
    }
}

TEST(OctiHttpServerIntegrationTest, LiveRoomNoProgressDrawRatedEloRestartAndArchiveUpload)
{
    User alice = signIn("google", "sub-r7-alice", "Alice R7");
    User bob = signIn("apple", "sub-r7-bob", "Bob R7");
    User carol = signIn("microsoft", "sub-r7-carol", "Carol R7");
    std::vector<User> ab = {alice, bob};

    /* 1. Seed unequal Pool 1 ratings (Alice 1216, Bob 1184) so a 0.5 Elo draw visibly moves both ratings. */
    std::string seedRoom = startGame(ab, Json{{"mode", "2P"}, {"bases", 3}, {"rated", true}});
    CHECK_EQUAL(8, playPlies(seedRoom, ab, GAME_MODE_2, 8, Json{{"bases", 3}}));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + seedRoom + "/resign",
                                   bob.token, Json::object()));
    CHECK_EQUAL(1216, http().get("/api/v1/profile/me", alice.token).json.value("elo_2p_3b", 0));
    CHECK_EQUAL(1184, http().get("/api/v1/profile/me", bob.token).json.value("elo_2p_3b", 0));

    /* 2. Play a live rated 2P_3B game through 4 prong-add plies + 30 pure step plies (with mid-game SQLite restart). */
    std::string room = startGame(ab, Json{{"mode", "2P"}, {"bases", 3}, {"rated", true}});
    CHECK_EQUAL(0, view(room).value("no_progress_plies", -1));

    const char *setupMoves[4] = {"33+a", "37+e", "33a+e", "37e+a"};
    for (int i = 0; i < 4; ++i) {
        Reply mr = http().post("/api/v1/games/" + room + "/move",
                               ab[static_cast<size_t>(i % 2)].token,
                               Json{{"move", setupMoves[i]}});
        EXPECT_STATUS(200, mr);
        CHECK_FALSE(mr.json.value("game_over", true));
        CHECK_EQUAL(0, view(room).value("no_progress_plies", -1));
    }

    const char *stepCycle[4] = {"33ae-34", "37ae-36", "34ae-33", "36ae-37"};
    for (int i = 0; i < 30; ++i) {
        if (i == 14) {
            CHECK_TRUE(env->rooms->checkpointToStore());
            CHECK_TRUE(env->rooms->restoreFromStore());
            Json restoredView = view(room);
            STRCMP_EQUAL("IN_PROGRESS", restoredView.value("phase", "").c_str());
            CHECK_EQUAL(14, restoredView.value("no_progress_plies", -1));
        }
        Reply mr = http().post("/api/v1/games/" + room + "/move",
                               ab[static_cast<size_t>(i % 2)].token,
                               Json{{"move", stepCycle[i % 4]}});
        EXPECT_STATUS(200, mr);
        if (i < 29) {
            CHECK_FALSE(mr.json.value("game_over", true));
            Json vMid = view(room);
            STRCMP_EQUAL("IN_PROGRESS", vMid.value("phase", "").c_str());
            CHECK_EQUAL(i + 1, vMid.value("no_progress_plies", -1));
        } else {
            CHECK_TRUE(mr.json.value("game_over", false));
        }
    }

    Json vEnd = view(room);
    STRCMP_EQUAL("FINISHED", vEnd.value("phase", "").c_str());
    STRCMP_EQUAL("DRAW", vEnd.value("result", "").c_str());
    STRCMP_EQUAL("", vEnd.value("winner", "").c_str());
    CHECK_EQUAL(0, vEnd.value("winner_seat", -1));
    STRCMP_EQUAL("NO_PROGRESS_DRAW", vEnd.value("termination", "").c_str());
    CHECK_EQUAL(30, vEnd.value("no_progress_plies", -1));
    CHECK_TRUE(vEnd.value("archived", false));

    std::vector<Json> goEvs = ofType(events(room, ""), "GAME_OVER");
    CHECK_EQUAL(1, static_cast<int>(goEvs.size()));
    STRCMP_EQUAL("NO_PROGRESS_DRAW", goEvs[0].value("termination", "").c_str());
    STRCMP_EQUAL("DRAW", goEvs[0].value("result", "").c_str());
    CHECK_EQUAL(0, goEvs[0].value("winner_seat", -1));

    /* Verify Pool 1 0.5 Elo draw: Alice (1216) drops to 1215, Bob (1184) rises to 1185, draws == 1. */
    Json aliceProf = http().get("/api/v1/profile/me", alice.token).json;
    Json bobProf = http().get("/api/v1/profile/me", bob.token).json;
    CHECK_EQUAL(1215, aliceProf.value("elo_2p_3b", 0));
    CHECK_EQUAL(1185, bobProf.value("elo_2p_3b", 0));
    CHECK_EQUAL(1, aliceProf.value("draws", 0));
    CHECK_EQUAL(1, bobProf.value("draws", 0));

    /* Verify SQLite archive record and stored JSON v3 transcript. */
    Reply rArch = http().get("/api/v1/archive/games/" + room);
    EXPECT_STATUS(200, rArch);
    STRCMP_EQUAL("DRAW", rArch.json.value("result", "").c_str());
    STRCMP_EQUAL("NO_PROGRESS_DRAW", rArch.json.value("termination", "").c_str());
    CHECK_EQUAL(0, rArch.json.value("winner_seat", -1));
    STRCMP_EQUAL("NONE", rArch.json.value("winner", "").c_str());
    STRCMP_EQUAL("GAME_OVER", rArch.json.value("status", "").c_str());
    CHECK_TRUE(rArch.json.value("rated", false));
    CHECK_EQUAL(1, rArch.json.value("elo_pool", -1));

    Reply rTr = http().get("/api/v1/archive/games/" + room + "/transcript");
    EXPECT_STATUS(200, rTr);
    Json storedTr = Json::parse(rTr.body);
    STRCMP_EQUAL("NO_PROGRESS_DRAW", storedTr.value("termination", "").c_str());
    STRCMP_EQUAL("NONE", storedTr.value("winner", "").c_str());
    CHECK_EQUAL(30, storedTr.value("noProgressPlies", -1));

    /* 3. Verify Pool 2 (Human vs Robot) rated 2P_3B 30-ply NO_PROGRESS_DRAW + recompute-ratings parity. */
    Reply rBot = http().post("/api/v1/robots", carol.token,
                             Json{{"robot_name", "CarolR7Bot"},
                                  {"description", "R7 draw test bot"}});
    EXPECT_STATUS(201, rBot);
    std::string botToken = rBot.json.value("robot_token", "");
    std::string botUuid = rBot.json.value("robot_uuid", "");
    User botUser{botToken, botUuid, "CarolR7Bot"};

    Reply hostArena = http().post("/api/v1/tables", alice.token,
                                  Json{{"mode", "2P"}, {"bases", 3},
                                       {"rated", true}, {"allow_robots", true}});
    EXPECT_STATUS(201, hostArena);
    std::string arenaRoom = hostArena.json.value("room_id", "");
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + arenaRoom + "/join",
                                   botToken, Json{{"seat", 2}}));
    std::vector<User> aliceAndBot = {alice, botUser};
    for (int i = 0; i < 4; ++i) {
        EXPECT_STATUS(200, http().post("/api/v1/games/" + arenaRoom + "/move",
                                       aliceAndBot[static_cast<size_t>(i % 2)].token,
                                       Json{{"move", setupMoves[i]}}));
    }
    for (int i = 0; i < 30; ++i) {
        EXPECT_STATUS(200, http().post("/api/v1/games/" + arenaRoom + "/move",
                                       aliceAndBot[static_cast<size_t>(i % 2)].token,
                                       Json{{"move", stepCycle[i % 4]}}));
    }
    Json vArena = view(arenaRoom);
    STRCMP_EQUAL("FINISHED", vArena.value("phase", "").c_str());
    STRCMP_EQUAL("NO_PROGRESS_DRAW", vArena.value("termination", "").c_str());
    CHECK_EQUAL(30, vArena.value("no_progress_plies", -1));

    Json aliceProf2 = http().get("/api/v1/profile/me", alice.token).json;
    CHECK_EQUAL(1, aliceProf2.value("arena_draws", 0));
    CHECK_EQUAL(1200, aliceProf2.value("arena_elo_2p_3b", 0));
    Json botProf = http().get("/api/v1/robots/profile/" + botUuid).json;
    CHECK_EQUAL(1, botProf.value("draws", 0));
    CHECK_EQUAL(1200, botProf.value("elo_2p_3b", 0));

    env->store.setAdminGoogleIdentities({"sub-r7-alice"});
    Reply recompute = http().post("/api/v1/admin/recompute-ratings", alice.token, Json::object());
    EXPECT_STATUS(200, recompute);
    CHECK_EQUAL(1215, http().get("/api/v1/profile/me", alice.token).json.value("elo_2p_3b", 0));
    CHECK_EQUAL(1185, http().get("/api/v1/profile/me", bob.token).json.value("elo_2p_3b", 0));

    /* 4. Offline archive upload of an OFK 30-ply NO_PROGRESS_DRAW transcript succeeds. */
    OctiBoard ofkBoard(GAME_MODE_OFK, false, false, GAME_BASES_1);
    ofkBoard.getGame()->start(PLAYER_A);
    const char *ofkSetup[4] = {"22+a", "26+e", "22a+e", "26e+a"};
    const char *ofkSteps[4] = {"22ae-23", "26ae-25", "23ae-22", "25ae-26"};
    for (int i = 0; i < 4; ++i) {
        ofkBoard.getGame()->move(ofkSetup[i]);
    }
    for (int i = 0; i < 30; ++i) {
        ofkBoard.getGame()->move(ofkSteps[i % 4]);
    }
    CHECK_TRUE(ofkBoard.getGame()->isNoProgressDraw());
    OctiGameMetadata upMeta;
    upMeta.players = {
        {1, PLAYER_A, "HUMAN", "Alice", 0},
        {2, PLAYER_B, "AI", "AI_L4", 4}
    };
    std::string ofkTr = ofkBoard.serializeJson(upMeta);
    Reply rUpOfk = http().post("/api/v1/archive/upload", alice.token,
                               Json{{"transcript", ofkTr}});
    EXPECT_STATUS(200, rUpOfk);
    std::string upGid = rUpOfk.json.value("game_id", "");
    CHECK_FALSE(upGid.empty());
    Reply rGetOfk = http().get("/api/v1/archive/games/" + upGid);
    EXPECT_STATUS(200, rGetOfk);
    STRCMP_EQUAL("DRAW", rGetOfk.json.value("result", "").c_str());
    STRCMP_EQUAL("NO_PROGRESS_DRAW", rGetOfk.json.value("termination", "").c_str());
    STRCMP_EQUAL("NONE", rGetOfk.json.value("winner", "").c_str());
}

TEST(OctiHttpServerIntegrationTest, WebJsNoProgressDrawEngineWorkerAndUiParity)
{
    Reply rUi = http().get("/");
    EXPECT_STATUS(200, rUi);

    const std::string openTag = "<script>";
    const std::string closeTag = "</script>";
    const std::size_t sIdx = rUi.body.find(openTag);
    const std::size_t eIdx = rUi.body.rfind(closeTag);
    CHECK_TRUE(sIdx != std::string::npos);
    CHECK_TRUE(eIdx != std::string::npos && eIdx > sIdx);
    const std::string webJsRaw =
        rUi.body.substr(sIdx + openTag.size(), eIdx - (sIdx + openTag.size()));

    const std::string tmpBase = env->dir + "/r7_web_js_parity";
    const std::string webJsPath = tmpBase + "_webui.js";
    const std::string runnerJsPath = tmpBase + "_runner.js";

    {
        std::ofstream ofs(webJsPath, std::ios::binary | std::ios::trunc);
        ofs << webJsRaw;
    }

    {
        std::ofstream ofs(runnerJsPath, std::ios::binary | std::ios::trunc);
        ofs << R"NODEJS(
const fs = require("fs");
const vm = require("vm");
const assert = require("assert");
const crypto = require("crypto");

const webJsRaw = fs.readFileSync(process.argv[2], "utf8");
const iifeClose = webJsRaw.lastIndexOf("})();");
assert.ok(iifeClose > 0, "Expected closing })(); in WebUi script");

const instrumentedJs =
  webJsRaw.slice(0, iifeClose) +
  "\nwindow.__octiR7Test = {\n" +
  "  parseOctiFen: parseOctiFen,\n" +
  "  serializeOctiFen: serializeOctiFen,\n" +
  "  cloneState: cloneState,\n" +
  "  check4PlayerElimination: check4PlayerElimination,\n" +
  "  check4PElimination: (typeof check4PElimination === 'function') ? check4PElimination : check4PlayerElimination,\n" +
  "  applyMoveToState: applyMoveToState,\n" +
  "  generateMoves: generateMoves,\n" +
  "  botSearch: botSearch,\n" +
  "  getWorkerCode: function() { return (typeof workerCode === 'string') ? workerCode : ''; },\n" +
  "  getBotWorker: function() { return botWorker; },\n" +
  "  setActivePlayState: function(st) { activePlayState = st; matchStarted = true; activeOnlineGameId = ''; },\n" +
  "  getActivePlayState: function() { return activePlayState; },\n" +
  "  onLocalMoveCommitted: onLocalMoveCommitted,\n" +
  "  moveComposer: moveComposer,\n" +
  "  triggerAiBotTurn: triggerAiBotTurn,\n" +
  "  openOnlineRoomForTest: function(rid) { activeOnlineGameId = rid; activeOnlineTableRoomId = rid; activeOnlineLastSyncedFen = ''; activeOnlineLastSyncedPlyCount = -1; activeOnlinePrevFinished = false; syncOnlineGameRoom(rid, true); },\n" +
  "  setLocalBotMode: function() { localMatchConfig.playMode = 'bot'; localMatchConfig.humanSeat = 'B'; localMatchConfig.botDifficulty = 2; }\n" +
  "};\n})();\n";

let capturedBlobText = "";
class CapturingBlob {
  constructor(parts, opts) {
    this.parts = Array.isArray(parts) ? parts : [];
    this.type = (opts && opts.type) || "";
    this.text = this.parts.map((p) => String(p)).join("");
    if (this.text.includes("self.onmessage")) {
      capturedBlobText = this.text;
    }
  }
}

class MockWorker {
  constructor(url) {
    this.url = url;
    this.terminated = false;
    this.posted = [];
  }
  postMessage(msg) { this.posted.push(msg); }
  terminate() { this.terminated = true; }
}

class MockNode {}
const elements = {};
function makeElement(id) {
  let _innerHTML = "";
  const elObj = Object.assign(new MockNode(), {
    id: id || "",
    tagName: "DIV",
    textContent: "",
    value: "",
    checked: false,
    disabled: false,
    className: "",
    width: 760,
    height: 760,
    clientWidth: 760,
    clientHeight: 760,
    offsetWidth: 760,
    offsetHeight: 760,
    scrollHeight: 0,
    scrollTop: 0,
    children: [],
    style: {
      setProperty(k, v) { this[k] = v; },
      removeProperty(k) { delete this[k]; },
      getPropertyValue(k) { return this[k] || ""; }
    },
    dataset: {},
    listeners: {},
    addEventListener(ev, fn, opts) {
      if (!this.listeners[ev]) this.listeners[ev] = [];
      this.listeners[ev].push({ fn, opts });
    },
    removeEventListener() {},
    classList: { add() {}, remove() {}, toggle() {}, contains() { return false; } },
    appendChild(ch) { if (ch) this.children.push(ch); return ch; },
    insertBefore(ch) { return this.appendChild(ch); },
    removeChild(ch) { return ch; },
    replaceChild(ch) { return ch; },
    cloneNode() { return makeElement(""); },
    focus() {},
    blur() {},
    select() {},
    scrollIntoView() {},
    contains() { return false; },
    getAttribute() { return null; },
    setAttribute() {},
    removeAttribute() {},
    querySelector() { return null; },
    querySelectorAll() { return []; },
    getBoundingClientRect() { return { left: 0, top: 0, right: 760, bottom: 760, width: 760, height: 760 }; },
    getContext() {
      return new Proxy({
        createLinearGradient: () => ({ addColorStop() {} }),
        createRadialGradient: () => ({ addColorStop() {} }),
        createPattern: () => ({}),
        measureText: () => ({ width: 20 })
      }, {
        get(target, prop) { return prop in target ? target[prop] : function () {}; },
        set(target, prop, val) { target[prop] = val; return true; }
      });
    }
  });
  Object.defineProperty(elObj, "innerHTML", {
    get() { return _innerHTML; },
    set(v) { _innerHTML = String(v || ""); elObj.children = []; }
  });
  return elObj;
}

const storageMap = new Map();
let mockRoomResponse = null;
const sandbox = {
  console, Date, Math, JSON, Array, Object, String, Number, Boolean, RegExp, Error,
  Map, Set, BigInt, Promise, Uint8Array, Int16Array, Float32Array, ArrayBuffer, DataView,
  URL: Object.assign(function (u, b) { return new URL(u, b); }, {
    createObjectURL() { return "blob:octi-r7-worker"; },
    revokeObjectURL() {}
  }),
  URLSearchParams, Blob: CapturingBlob, Worker: MockWorker, FormData, Headers, AbortController,
  crypto, Node: MockNode, parseInt, parseFloat, isNaN, isFinite, encodeURIComponent, decodeURIComponent,
  atob: (b64) => Buffer.from(b64, "base64").toString("binary"),
  btoa: (bin) => Buffer.from(bin, "binary").toString("base64"),
  localStorage: {
    getItem(k) { return storageMap.has(k) ? storageMap.get(k) : null; },
    setItem(k, v) { storageMap.set(String(k), String(v)); },
    removeItem(k) { storageMap.delete(k); },
    clear() { storageMap.clear(); }
  },
  location: { hash: "", pathname: "/", search: "", origin: "http://127.0.0.1:6874", href: "http://127.0.0.1:6874/" },
  history: { replaceState() {}, pushState() {} },
  navigator: { clipboard: { writeText: () => Promise.resolve() }, userAgent: "OctiR7Test/1.0" },
  devicePixelRatio: 1, innerWidth: 1280, innerHeight: 800,
  AudioContext: function () { this.state = "running"; this.destination = {}; },
  Audio: function () { this.play = () => Promise.resolve(); },
  EventSource: function () { this.close = function () {}; },
  Image: function () {},
  alert() {}, confirm() { return true; }, prompt() { return ""; }, open() { return null; },
  requestAnimationFrame() { return 0; }, setTimeout() { return 1; }, clearTimeout() {},
  setInterval() { return 1; }, clearInterval() {}, addEventListener() {}, removeEventListener() {},
  fetch() {
    if (mockRoomResponse) {
      const payload = mockRoomResponse;
      return Promise.resolve({ ok: true, status: 200, text: () => Promise.resolve(JSON.stringify(payload)), json: () => Promise.resolve(payload) });
    }
    return Promise.resolve({ ok: true, status: 200, text: () => Promise.resolve("{}"), json: () => Promise.resolve({}) });
  },
  document: {
    body: makeElement("body"),
    documentElement: makeElement("html"),
    activeElement: null,
    hidden: false,
    visibilityState: "visible",
    getElementById(id) {
      if (!elements[id]) elements[id] = makeElement(id);
      return elements[id];
    },
    createElement(tag) { const e = makeElement(""); e.tagName = String(tag || "DIV").toUpperCase(); return e; },
    createElementNS(ns, tag) { const e = makeElement(""); e.tagName = String(tag || "SVG").toUpperCase(); return e; },
    createTextNode(txt) { return Object.assign(new MockNode(), { nodeType: 3, textContent: String(txt || "") }); },
    querySelector() { return null; },
    querySelectorAll() { return []; },
    addEventListener() {},
    removeEventListener() {}
  }
};
sandbox.window = sandbox;
sandbox.self = sandbox;

vm.runInNewContext(instrumentedJs, sandbox, { filename: "WebUi.js", timeout: 5000 });
const t = sandbox.window.__octiR7Test;
assert.ok(t, "__octiR7Test must exist");

/* 1. FEN 11/12 & 13/14 token parsing + canonical 11/13 serializeOctiFen preservation */
const base2pFen = "2:3:0:0:P:A:-:0:4,0,23,0:4,0,23,0:A33ae,A53,A73,B37ae,B57,B77";
const s11 = t.parseOctiFen(base2pFen);
assert.strictEqual(s11.noProgressPlies, 0);
const s12 = t.parseOctiFen(base2pFen + ":27");
assert.strictEqual(s12.noProgressPlies, 27);
assert.strictEqual(t.cloneState(s12).noProgressPlies, 27);
assert.strictEqual(t.serializeOctiFen(s12), base2pFen, "serializeOctiFen must remain 11 tokens");

const base4pFen = "4:3:0:0:P:A:-:0:3,0,10,0:3,0,10,0:3,0,10,0:3,0,10,0:A33ae,D73ae,B37ae,C77ae";
const s14 = t.parseOctiFen(base4pFen + ":58");
assert.strictEqual(s14.noProgressPlies, 58);
assert.strictEqual(t.serializeOctiFen(s14), base4pFen, "serializeOctiFen must remain 13 tokens");

const sTermDraw = t.parseOctiFen("2:3:0:0:O:-:-:0:4,0,23,0:4,0,23,0:A34ae,A53,A73,B36ae,B57,B77");
assert.strictEqual(sTermDraw.noProgressPlies, 30);
assert.strictEqual(sTermDraw.termination, "NO_PROGRESS_DRAW");

/* 2. 2P_3B 30-ply non-progress draw & progress resets (+d, +d!, pod deploy, liberate, capture, _SP bonus capture) */
let s2p = t.parseOctiFen(base2pFen);
const stepCycle2p = ["33ae-34", "37ae-36", "34ae-33", "36ae-37"];
for (let i = 0; i < 30; i++) {
  s2p = t.applyMoveToState(t.cloneState(s2p), stepCycle2p[i % 4], s2p.turn);
  assert.strictEqual(s2p.noProgressPlies, i + 1);
  if (i < 29) {
    assert.strictEqual(s2p.status, "P");
  }
}
assert.strictEqual(s2p.status, "O");
assert.strictEqual(s2p.turn, "-");
assert.strictEqual(s2p.winner, "-");
assert.strictEqual(s2p.termination, "NO_PROGRESS_DRAW");

/* Stepping onto/off an unoccupied enemy base in 2P_3B without capture does NOT reset counter */
let sBaseStep = t.parseOctiFen("2:3:0:0:P:A:-:0:4,0,23,0:4,0,24,0:A36ae,A53,A73,B57,B77:12");
sBaseStep = t.applyMoveToState(t.cloneState(sBaseStep), "36ae-37", "A");
assert.strictEqual(sBaseStep.noProgressPlies, 13, "Stepping onto enemy base without capture must not reset counter");

/* Prong add, pod deploy, pod liberate, and _SP bonus prelude capture all reset counter to 0 */
let sProng = t.parseOctiFen(base2pFen + ":19");
sProng = t.applyMoveToState(t.cloneState(sProng), "53+a", "A");
assert.strictEqual(sProng.noProgressPlies, 0);

let sDeploy = t.parseOctiFen(base2pFen + ":19");
sDeploy = t.applyMoveToState(t.cloneState(sDeploy), "53", "A");
assert.strictEqual(sDeploy.noProgressPlies, 0);

let sLib = t.parseOctiFen("2:3:0:0:P:A:-:0:0,1,20,0:4,0,24,0:A33,A37a,B77:19");
sLib = t.applyMoveToState(t.cloneState(sLib), "33*", "A");
assert.strictEqual(sLib.noProgressPlies, 0);

let sSp = t.parseOctiFen("2:3:1:0:P:A:-:0:4,0,22,0:4,0,24,0:A44ab!,A53,A73,B55,B37,B77:21");
sSp = t.applyMoveToState(t.cloneState(sSp), "44ab!-66x;66ab!-67", "A");
assert.strictEqual(sSp.noProgressPlies, 0, "_SP bonus prelude capture must reset noProgressPlies to 0");

/* 3. Non-capturing circular jump (S -> ... -> S) remains legal and increments noProgressPlies by +1 */
let sCirc = t.parseOctiFen("2:3:0:0:P:A:-:0:4,0,20,0:4,0,24,0:A44bdfh,B35,B55,B37,B57:10");
const circMoves = t.generateMoves(sCirc, "A", false);
assert.ok(circMoves.includes("44bdfh-66-48-26-44"), "Circular non-capturing jump must be generated");
sCirc = t.applyMoveToState(t.cloneState(sCirc), "44bdfh-66-48-26-44", "A");
assert.strictEqual(sCirc.noProgressPlies, 11);
assert.strictEqual(sCirc.status, "P");

/* 4. 4P: 60-ply threshold (does NOT draw at 30, draws at 60) + 4P base elimination resets to 0 */
let s4pElim = t.parseOctiFen("4:3:0:0:P:A:-:0:3,0,11,0:3,0,12,0:3,0,12,0:3,0,12,0:A36a,B47,C77,D73:25");
s4pElim = t.applyMoveToState(t.cloneState(s4pElim), "36a-37", "A");
assert.strictEqual(s4pElim.elimMask & 2, 2);
assert.strictEqual(s4pElim.noProgressPlies, 0, "New 4P permanent elimination must reset noProgressPlies to 0");

let s4p = t.parseOctiFen(base4pFen);
const cycle4p = [
  "33ae-34", "37ae-36", "77ae-76", "73ae-74",
  "34ae-33", "36ae-37", "76ae-77", "74ae-73"
];
for (let i = 0; i < 60; i++) {
  s4p = t.applyMoveToState(t.cloneState(s4p), cycle4p[i % 8], s4p.turn);
  assert.strictEqual(s4p.noProgressPlies, i + 1);
  if (i === 29) {
    assert.strictEqual(s4p.status, "P", "4P must NOT draw at 30 plies");
  }
}
assert.strictEqual(s4p.status, "O");
assert.strictEqual(s4p.winner, "-");
assert.strictEqual(s4p.termination, "NO_PROGRESS_DRAW");

/* 5. Ply-30 base victory precedence over R7 draw */
let sWin30 = t.parseOctiFen("2:1:0:0:P:A:-:0:4,0,23,0:4,0,24,0:A36a,B77:29");
sWin30 = t.applyMoveToState(t.cloneState(sWin30), "36a-37", "A");
assert.strictEqual(sWin30.noProgressPlies, 30);
assert.strictEqual(sWin30.status, "O");
assert.strictEqual(sWin30.winner, "A");
assert.strictEqual(sWin30.termination, "BASE_CAPTURE");

/* 6. Web Worker & botSearch draw seeking (when losing) and draw avoidance (when winning) at ply 29 */
const workerCode = capturedBlobText || t.getWorkerCode();
assert.ok(workerCode.length > 1000);
const workerMessages = [];
const workerSandbox = {
  console, Date, Math, JSON, Array, Object, String, Number, Boolean, RegExp, Error,
  Map, Set, BigInt, parseInt, parseFloat, isNaN, isFinite,
  postMessage(msg) { workerMessages.push(msg); }
};
workerSandbox.self = workerSandbox;
vm.runInNewContext(workerCode, workerSandbox, { filename: "botWorker.js", timeout: 5000 });

/* Losing position: A has 11c-21 (R7 draw = 0) vs 11c+<dir> (resets counter -> B wins 34e-33 next ply) */
workerMessages.length = 0;
workerSandbox.self.onmessage({
  data: {
    type: "search",
    searchId: 901,
    fen: "2:1:0:0:P:A:-:0:0,0,1,0:0,0,0,0:A11c,B34e",
    noProgressPlies: 29,
    difficulty: 2,
    mode: "2P",
    botSide: "A",
    budgetMs: 500
  }
});
const doneLose = workerMessages.find((m) => m && m.type === "done");
assert.ok(doneLose, "Worker must emit done for losing R7 position");
assert.strictEqual(doneLose.bestMove, "11c-21", "Losing bot at ply 29 must force R7 draw via 11c-21");
assert.strictEqual(doneLose.score, 0, "Forced R7 draw must evaluate to 0");

/* Winning position: A has 36a-37 (ply-30 base win) vs 11c-21 (ply-30 R7 draw) */
workerMessages.length = 0;
workerSandbox.self.onmessage({
  data: {
    type: "search",
    searchId: 902,
    fen: "2:1:0:0:P:A:-:0:0,0,0,0:0,0,0,0:A36a,A11c,B99g",
    noProgressPlies: 29,
    difficulty: 2,
    mode: "2P",
    botSide: "A",
    budgetMs: 500
  }
});
const doneWin = workerMessages.find((m) => m && m.type === "done");
assert.ok(doneWin, "Worker must emit done for winning R7 position");
assert.strictEqual(doneWin.bestMove, "36a-37", "Winning bot at ply 29 must take base victory over R7 draw");
assert.ok(doneWin.score >= 999000);

/* Verify triggerAiBotTurn posts noProgressPlies to Worker */
const stBotTurn = t.parseOctiFen("2:1:0:0:P:A:-:0:0,0,1,0:0,0,0,0:A11c,B34e:29");
t.setActivePlayState(stBotTurn);
t.setLocalBotMode();
t.triggerAiBotTurn();
const bw = t.getBotWorker();
assert.ok(bw && bw.posted.length > 0);
assert.strictEqual(bw.posted[bw.posted.length - 1].noProgressPlies, 29, "triggerAiBotTurn must send noProgressPlies to Worker");

/* 7. WebMoveComposer + onLocalMoveCommitted UI game-over banners for R7 draw */
const stUi29 = t.parseOctiFen("2:3:0:0:P:A:-:0:4,0,23,0:4,0,23,0:A33ae,A53,A73,B37ae,B57,B77:29");
t.setActivePlayState(stUi29);
assert.strictEqual(t.moveComposer.stageMoveOrJump(3, 3, 3, 4, 0, false), true);
assert.strictEqual(t.moveComposer.commitStagedMove(), true);
const postUi = t.getActivePlayState();
assert.strictEqual(postUi.status, "O");
assert.strictEqual(postUi.winner, "-");
assert.strictEqual(postUi.termination, "NO_PROGRESS_DRAW");
assert.strictEqual(postUi.noProgressPlies, 30);
assert.ok(elements["web-gameover-title"].textContent.includes("DRAW"), "Expected DRAW in gameover title, got: " + elements["web-gameover-title"].textContent);
assert.ok(!elements["web-gameover-title"].textContent.includes("PLAYER -"), "Must not display PLAYER - WINS");
assert.ok(elements["web-play-turn-badge"].textContent.includes("Draw"), "Expected Draw in turn badge, got: " + elements["web-play-turn-badge"].textContent);

/* 8. syncOnlineGameRoom syncs room.no_progress_plies and renders NO_PROGRESS_DRAW banner */
(async () => {
  mockRoomResponse = {
    ok: true,
    room_id: "r7-live-sync",
    phase: "IN_PROGRESS",
    mode: "2P",
    bases: 3,
    super_prongs: false,
    edgeless: false,
    fen: "2:3:0:0:P:A:-:0:4,0,23,0:4,0,23,0:A33ae,A53,A73,B37ae,B57,B77",
    no_progress_plies: 17,
    ply_count: 21,
    moves: [],
    seats: [{ seat: 1, handle: "alice" }, { seat: 2, handle: "bob" }]
  };
  t.openOnlineRoomForTest("r7-live-sync");
  await new Promise((r) => setImmediate(r));
  await new Promise((r) => setImmediate(r));
  assert.strictEqual(t.getActivePlayState().noProgressPlies, 17, "Online sync must populate noProgressPlies from room.no_progress_plies");

  mockRoomResponse = {
    ok: true,
    room_id: "r7-fin-sync",
    phase: "FINISHED",
    mode: "2P",
    bases: 3,
    super_prongs: false,
    edgeless: false,
    fen: "2:3:0:0:O:-:-:0:4,0,23,0:4,0,23,0:A34ae,A53,A73,B36ae,B57,B77",
    no_progress_plies: 30,
    ply_count: 34,
    result: "DRAW",
    termination: "NO_PROGRESS_DRAW",
    winner_seat: 0,
    moves: [],
    seats: [{ seat: 1, handle: "alice" }, { seat: 2, handle: "bob" }]
  };
  t.openOnlineRoomForTest("r7-fin-sync");
  await new Promise((r) => setImmediate(r));
  await new Promise((r) => setImmediate(r));
  assert.strictEqual(t.getActivePlayState().noProgressPlies, 30);
  assert.strictEqual(elements["web-gameover-title"].textContent, "DRAW \u2014 NO PROGRESS");
  assert.ok(elements["web-gameover-subtitle"].textContent.includes("No-Progress Rule"));
  process.exit(0);
})().catch((err) => {
  console.error(err);
  process.exit(1);
});
)NODEJS";
    }

    int rcNode = std::system(("timeout -k 2 20 node " + runnerJsPath + " " + webJsPath).c_str());
    std::remove(webJsPath.c_str());
    std::remove(runnerJsPath.c_str());
    CHECK_EQUAL(0, rcNode);
}

TEST(OctiHttpServerIntegrationTest, LeaderboardOverallVariantIsolationPlayedOnlyAndLobbyEntityDetails)
{
    User alice = signIn("google", "sub-lbov-alice", "Alice Strategist");
    User bob = signIn("google", "sub-lbov-bob", "Bob Tactician");
    User carol = signIn("google", "sub-lbov-carol", "Carol Builder");
    User daveUnplayed = signIn("google", "sub-lbov-dave", "Dave Idle");
    (void)daveUnplayed;

    /* Enrich Alice's human profile with display_name, bio, and location */
    EXPECT_STATUS(200, http().patch("/api/v1/profile/me", alice.token,
                                    Json{{"display_name", "Alice Strategist"},
                                         {"bio", "Endgame specialist"},
                                         {"location", "Taipei"}}));

    /* Create two robots owned by Carol: one active Arena bot and one 0-game lobby/host bot */
    setenv("OCTI_ALLOW_LOOPBACK_ROBOT_URL_FOR_TEST", "1", 1);
    std::string validBotUrl =
        "http://127.0.0.1:" + std::to_string(http().port()) + "/api/v1/health";
    Reply rBotAlpha = http().post(
        "/api/v1/robots", carol.token,
        Json{{"robot_name", "alpha-arena-bot"},
             {"description", "Active multi-variant Arena bot"},
             {"robot_url", validBotUrl}});
    Reply rBotZero = http().post(
        "/api/v1/robots", carol.token,
        Json{{"robot_name", "zero-game-host-bot"},
             {"description", "Fresh zero-game connected robot"},
             {"robot_url", validBotUrl}});
    unsetenv("OCTI_ALLOW_LOOPBACK_ROBOT_URL_FOR_TEST");

    EXPECT_STATUS(201, rBotAlpha);
    EXPECT_STATUS(201, rBotZero);
    std::string botAlphaUuid = rBotAlpha.json.value("robot_uuid", "");
    std::string botAlphaToken = rBotAlpha.json.value("robot_token", "");
    std::string botZeroUuid = rBotZero.json.value("robot_uuid", "");
    std::string botZeroToken = rBotZero.json.value("robot_token", "");

    User botAlphaUser;
    botAlphaUser.uuid = botAlphaUuid;
    botAlphaUser.handle = "alpha-arena-bot";
    botAlphaUser.token = botAlphaToken;

    /* 1. Pool 1 2P_1B rated win: Alice beats Bob (8 plies + resign) */
    std::vector<User> aliceBob = {alice, bob};
    std::string room1b =
        startGame(aliceBob, Json{{"mode", "2P"}, {"bases", 1}, {"rated", true}});
    CHECK_EQUAL(8, playPlies(room1b, aliceBob, GAME_MODE_2, 8, Json{{"bases", 1}}));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room1b + "/resign",
                                   bob.token, Json::object()));

    /* 2. Pool 1 OFK rated draw: Alice and Carol agree to draw */
    std::vector<User> aliceCarol = {alice, carol};
    std::string roomOfk =
        startGame(aliceCarol, Json{{"mode", "OFK"}, {"bases", 1}, {"rated", true}});
    EXPECT_STATUS(200, act(roomOfk, alice, "offer_draw"));
    EXPECT_STATUS(200, act(roomOfk, carol, "accept_draw"));

    /* 3. Pool 2 2P_3B rated win: Alice beats botAlpha (8 plies + resign) */
    Reply hostArena3b = http().post(
        "/api/v1/tables", alice.token,
        Json{{"mode", "2P"}, {"bases", 3}, {"rated", true}, {"allow_robots", true}});
    EXPECT_STATUS(201, hostArena3b);
    std::string arena3bRoom = hostArena3b.json.value("room_id", "");
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + arena3bRoom + "/join",
                                   botAlphaToken, Json{{"seat", 2}}));
    std::vector<User> aliceBot = {alice, botAlphaUser};
    CHECK_EQUAL(8, playPlies(arena3bRoom, aliceBot, GAME_MODE_2, 8, Json{{"bases", 3}}));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + arena3bRoom + "/resign",
                                   botAlphaToken, Json::object()));

    /* 4. Pool 2 OFK rated R7 NO_PROGRESS_DRAW archived between Bob and botAlpha */
    {
        octi::PlayerProfile bobProf;
        CHECK_TRUE(env->store.getProfileByUuid(bob.uuid, bobProf));
        octi::RobotRecord botRec;
        CHECK_TRUE(env->store.getRobotByUuid(botAlphaUuid, botRec));

        octi::GameRecord gR7;
        gR7.gameId = "g-arena-ofk-r7draw";
        gR7.gameMode = "OFK";
        gR7.gameBases = 1;
        gR7.timeControl = "untimed";
        gR7.p1PlayerId = bobProf.playerId;
        gR7.p1Handle = bobProf.handle;
        gR7.p2PlayerId = 0;
        gR7.p2RobotId = botRec.robotId;
        gR7.p2Handle = botRec.robotName;
        gR7.result = "DRAW";
        gR7.winnerSeat = 0;
        gR7.winner = "NONE";
        gR7.termination = "NO_PROGRESS_DRAW";
        gR7.status = "GAME_OVER";
        gR7.plyCount = 30;
        gR7.octiTranscript = "{\"format\":\"octi-transcript-v1\",\"plies\":[]}";
        gR7.finalFen = "K:1:0:0:O:-:-:0:4,0,23,0:4,0,23,0:A22,B26";
        gR7.startedAt = 1700000100;
        gR7.endedAt = 1700000200;
        gR7.source = "online";
        gR7.rated = 1;
        gR7.eloPool = 2;
        std::string archErr;
        CHECK_EQUAL(static_cast<int>(octi::ArchiveStatus::OK),
                    static_cast<int>(env->store.archiveGameWithRatings(
                        gR7,
                        {{bobProf.playerId, 0, 1200, octi::GameOutcome::DRAW},
                         {0, botRec.robotId, 1200, octi::GameOutcome::DRAW}},
                        {{botRec.robotId, octi::GameOutcome::DRAW}}, archErr)));
    }

    /* 5. Fault cases -> 400 INVALID_ARGUMENT */
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/leaderboard?mode=overall&sort=invalid"));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/leaderboard?mode=overall&order=sideways"));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/leaderboard?mode=2P_3B&played_only=maybe"));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/leaderboard?mode=overall&pool=human&participant_type=robots"));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/leaderboard?mode=bogus"));

    /* 6. Variant-scoped Pool 1 + played_only=1 (SQL-level filtering before LIMIT) */
    Reply lb1bPlayed =
        http().get("/api/v1/leaderboard?mode=2P_1B&pool=human&played_only=1");
    EXPECT_STATUS(200, lb1bPlayed);
    CHECK_EQUAL(2, lb1bPlayed.json.value("total", -1));
    CHECK_EQUAL(2, static_cast<int>(lb1bPlayed.json["entries"].size()));
    STRCMP_EQUAL(alice.uuid.c_str(),
                 lb1bPlayed.json["entries"][0].value("uuid", "").c_str());
    CHECK_EQUAL(1, lb1bPlayed.json["entries"][0].value("games_played", -1));
    CHECK_EQUAL(1, lb1bPlayed.json["entries"][0].value("wins", -1));
    CHECK_EQUAL(0, lb1bPlayed.json["entries"][0].value("losses", -1));
    CHECK_EQUAL(0, lb1bPlayed.json["entries"][0].value("draws", -1));
    DOUBLES_EQUAL(100.0, lb1bPlayed.json["entries"][0].value("win_rate", -1.0), 0.01);

    STRCMP_EQUAL(bob.uuid.c_str(),
                 lb1bPlayed.json["entries"][1].value("uuid", "").c_str());
    CHECK_EQUAL(1, lb1bPlayed.json["entries"][1].value("games_played", -1));
    CHECK_EQUAL(0, lb1bPlayed.json["entries"][1].value("wins", -1));
    CHECK_EQUAL(1, lb1bPlayed.json["entries"][1].value("losses", -1));
    CHECK_EQUAL(0, lb1bPlayed.json["entries"][1].value("draws", -1));
    DOUBLES_EQUAL(0.0, lb1bPlayed.json["entries"][1].value("win_rate", -1.0), 0.01);

    Reply lb3bPlayed =
        http().get("/api/v1/leaderboard?mode=2P_3B&pool=human&played_only=1");
    EXPECT_STATUS(200, lb3bPlayed);
    CHECK_EQUAL(0, lb3bPlayed.json.value("total", -1));
    CHECK_EQUAL(0, static_cast<int>(lb3bPlayed.json["entries"].size()));

    /* Default played_only=0 preserves backward compatibility (returns all 4 unplayed 2P_3B accounts) */
    Reply lb3bCompat = http().get("/api/v1/leaderboard?mode=2P_3B&pool=human");
    EXPECT_STATUS(200, lb3bCompat);
    CHECK_EQUAL(4, static_cast<int>(lb3bCompat.json["entries"].size()));

    /* 7. Variant-scoped Pool 2 + played_only=1 + limit=1 check */
    Reply lbArena3bPlayed =
        http().get("/api/v1/leaderboard?mode=2P_3B&pool=arena&played_only=1");
    EXPECT_STATUS(200, lbArena3bPlayed);
    CHECK_EQUAL(2, lbArena3bPlayed.json.value("total", -1));
    CHECK_EQUAL(2, static_cast<int>(lbArena3bPlayed.json["entries"].size()));
    /* botAlpha played 1 2P_3B loss and 1 OFK NO_PROGRESS_DRAW -> in 2P_3B must show 1 game (0-1-0), NOT 2! */
    STRCMP_EQUAL(botAlphaUuid.c_str(),
                 lbArena3bPlayed.json["entries"][1].value("robot_uuid", "").c_str());
    CHECK_EQUAL(1, lbArena3bPlayed.json["entries"][1].value("games_played", -1));
    CHECK_EQUAL(0, lbArena3bPlayed.json["entries"][1].value("wins", -1));
    CHECK_EQUAL(1, lbArena3bPlayed.json["entries"][1].value("losses", -1));
    CHECK_EQUAL(0, lbArena3bPlayed.json["entries"][1].value("draws", -1));

    Reply lbArenaOfkLim1 =
        http().get("/api/v1/leaderboard?mode=OFK&pool=arena&played_only=1&limit=1");
    EXPECT_STATUS(200, lbArenaOfkLim1);
    CHECK_EQUAL(2, lbArenaOfkLim1.json.value("total", -1));
    CHECK_EQUAL(1, static_cast<int>(lbArenaOfkLim1.json["entries"].size()));
    CHECK_EQUAL(1, lbArenaOfkLim1.json["entries"][0].value("games_played", -1));
    CHECK_EQUAL(0, lbArenaOfkLim1.json["entries"][0].value("wins", -1));
    CHECK_EQUAL(0, lbArenaOfkLim1.json["entries"][0].value("losses", -1));
    CHECK_EQUAL(1, lbArenaOfkLim1.json["entries"][0].value("draws", -1));

    /* 8. mode=overall (Pool 1 & Pool 2) excludes 0-game accounts & orders active ladder players ahead of 1200 defaults */
    Reply lbOverallHuman =
        http().get("/api/v1/leaderboard?mode=overall&pool=human&sort=elo_2p_1b&order=desc");
    EXPECT_STATUS(200, lbOverallHuman);
    STRCMP_EQUAL("OVERALL", lbOverallHuman.json.value("ladder", "").c_str());
    CHECK_EQUAL(3, lbOverallHuman.json.value("total", -1));
    CHECK_EQUAL(3, static_cast<int>(lbOverallHuman.json["entries"].size()));
    /* Alice (1216, 1 game in 2P_1B) and Bob (1184, 1 game in 2P_1B) sort ahead of Carol (1200, 0 games in 2P_1B); Dave (0 games total) excluded */
    STRCMP_EQUAL(alice.uuid.c_str(),
                 lbOverallHuman.json["entries"][0].value("uuid", "").c_str());
    CHECK_EQUAL(1216, lbOverallHuman.json["entries"][0].value("elo_2p_1b", 0));
    CHECK_EQUAL(1, lbOverallHuman.json["entries"][0].value("games_2p_1b", -1));
    CHECK_EQUAL(1, lbOverallHuman.json["entries"][0].value("games_ofk", -1));
    CHECK_EQUAL(2, lbOverallHuman.json["entries"][0].value("games_played", -1));
    CHECK_EQUAL(1, lbOverallHuman.json["entries"][0].value("wins", -1));
    CHECK_EQUAL(1, lbOverallHuman.json["entries"][0].value("draws", -1));
    STRCMP_EQUAL(bob.uuid.c_str(),
                 lbOverallHuman.json["entries"][1].value("uuid", "").c_str());
    STRCMP_EQUAL(carol.uuid.c_str(),
                 lbOverallHuman.json["entries"][2].value("uuid", "").c_str());

    Reply lbOverallHumanAsc =
        http().get("/api/v1/leaderboard?mode=overall&pool=human&sort=elo_2p_1b&order=asc");
    EXPECT_STATUS(200, lbOverallHumanAsc);
    STRCMP_EQUAL(bob.uuid.c_str(),
                 lbOverallHumanAsc.json["entries"][0].value("uuid", "").c_str());
    STRCMP_EQUAL(alice.uuid.c_str(),
                 lbOverallHumanAsc.json["entries"][1].value("uuid", "").c_str());
    STRCMP_EQUAL(carol.uuid.c_str(),
                 lbOverallHumanAsc.json["entries"][2].value("uuid", "").c_str());

    Reply lbOverallArena =
        http().get("/api/v1/leaderboard?mode=overall&pool=arena&participant_type=all&sort=games_played&order=desc");
    EXPECT_STATUS(200, lbOverallArena);
    STRCMP_EQUAL("OVERALL", lbOverallArena.json.value("ladder", "").c_str());
    /* Active Pool 2 participants: botAlpha (2 games), Alice (1 game), Bob (1 game); botZero (0 games) excluded */
    CHECK_EQUAL(3, lbOverallArena.json.value("total", -1));
    STRCMP_EQUAL(botAlphaUuid.c_str(),
                 lbOverallArena.json["entries"][0].value("robot_uuid", "").c_str());
    CHECK_EQUAL(2, lbOverallArena.json["entries"][0].value("games_played", -1));
    CHECK_EQUAL(1, lbOverallArena.json["entries"][0].value("games_2p_3b", -1));
    CHECK_EQUAL(1, lbOverallArena.json["entries"][0].value("games_ofk", -1));
    CHECK_EQUAL(1, lbOverallArena.json["entries"][0].value("losses_2p_3b", -1));
    CHECK_EQUAL(1, lbOverallArena.json["entries"][0].value("draws_ofk", -1));

    /* 9. /api/v1/lobby/state (players[] + tables[].host_detail) and /api/v1/tables (host_detail), including 0-game bot */
    EXPECT_STATUS(404, http().get("/api/v1/robots/profile/" + botZeroUuid));

    Reply aliceHostTab = http().post(
        "/api/v1/tables", alice.token, Json{{"mode", "2P"}, {"bases", 3}});
    EXPECT_STATUS(201, aliceHostTab);
    std::string aliceTabId = aliceHostTab.json.value("room_id", "");

    Reply botZeroHostTab = http().post(
        "/api/v1/tables", botZeroToken,
        Json{{"mode", "OFK"}, {"bases", 1}, {"allow_robots", true}});
    EXPECT_STATUS(201, botZeroHostTab);
    std::string botZeroTabId = botZeroHostTab.json.value("room_id", "");

    EXPECT_STATUS(200, http().get("/api/v1/lobby/state", alice.token));
    Reply lobby = http().get("/api/v1/lobby/state", botZeroToken);
    EXPECT_STATUS(200, lobby);

    bool foundAliceInLobby = false;
    bool foundBotZeroInLobby = false;
    for (size_t i = 0; i < lobby.json["players"].size(); ++i) {
        const Json &p = lobby.json["players"][i];
        if (!p.value("is_robot", false) && p.value("player_uuid", "") == alice.uuid) {
            foundAliceInLobby = true;
            STRCMP_EQUAL("Alice Strategist", p.value("display_name", "").c_str());
            STRCMP_EQUAL("Endgame specialist", p.value("bio", "").c_str());
            STRCMP_EQUAL("Taipei", p.value("location", "").c_str());
            CHECK_EQUAL(1216, p.value("elo_2p_1b", 0));
            CHECK_EQUAL(1, p.value("games_2p_1b", -1));
            CHECK_EQUAL(1216, p.value("arena_elo_2p_3b", 0));
            CHECK_EQUAL(1, p.value("arena_games_2p_3b", -1));
        } else if (p.value("is_robot", false) &&
                   p.value("robot_uuid", "") == botZeroUuid) {
            foundBotZeroInLobby = true;
            STRCMP_EQUAL(validBotUrl.c_str(), p.value("robot_url", "").c_str());
            STRCMP_EQUAL("Fresh zero-game connected robot",
                         p.value("robot_description", "").c_str());
            STRCMP_EQUAL(carol.handle.c_str(), p.value("owner_handle", "").c_str());
            CHECK_EQUAL(1200, p.value("elo_2p_3b", 0));
            CHECK_EQUAL(1200, p.value("elo_2p_1b", 0));
            CHECK_EQUAL(1200, p.value("elo_ofk", 0));
            CHECK_EQUAL(0, p.value("games_played", -1));
        }
    }
    CHECK_TRUE(foundAliceInLobby);
    CHECK_TRUE(foundBotZeroInLobby);

    /* Check host_detail in BOTH /api/v1/lobby/state tables[] and /api/v1/tables tables[] */
    Reply tablesResp = http().get("/api/v1/tables");
    EXPECT_STATUS(200, tablesResp);
    const Json *tableLists[2] = {&lobby.json["tables"], &tablesResp.json["tables"]};
    for (int li = 0; li < 2; ++li) {
        bool sawAliceHost = false;
        bool sawBotZeroHost = false;
        for (size_t ti = 0; ti < tableLists[li]->size(); ++ti) {
            const Json &t = (*tableLists[li])[ti];
            CHECK_TRUE(t.contains("host_detail") && t["host_detail"].is_object());
            const Json &hd = t["host_detail"];
            if (t.value("room_id", "") == aliceTabId) {
                sawAliceHost = true;
                CHECK_FALSE(hd.value("is_robot", true));
                STRCMP_EQUAL("Alice Strategist", hd.value("display_name", "").c_str());
                STRCMP_EQUAL("Endgame specialist", hd.value("bio", "").c_str());
                STRCMP_EQUAL("Taipei", hd.value("location", "").c_str());
                CHECK_EQUAL(1216, hd.value("elo_2p_1b", 0));
                CHECK_EQUAL(1216, hd.value("arena_elo_2p_3b", 0));
            } else if (t.value("room_id", "") == botZeroTabId) {
                sawBotZeroHost = true;
                CHECK_TRUE(hd.value("is_robot", false));
                STRCMP_EQUAL(botZeroUuid.c_str(), hd.value("robot_uuid", "").c_str());
                STRCMP_EQUAL(validBotUrl.c_str(), hd.value("robot_url", "").c_str());
                STRCMP_EQUAL("Fresh zero-game connected robot",
                             hd.value("robot_description", "").c_str());
                STRCMP_EQUAL(carol.handle.c_str(), hd.value("owner_handle", "").c_str());
                CHECK_EQUAL(1200, hd.value("elo_2p_3b", 0));
                CHECK_EQUAL(0, hd.value("games_played", -1));
            }
        }
        CHECK_TRUE(sawAliceHost);
        CHECK_TRUE(sawBotZeroHost);
    }

    /* And /api/v1/robots/profile/:id MUST still return 404 for 0-game botZero! */
    EXPECT_STATUS(404, http().get("/api/v1/robots/profile/" + botZeroUuid));
}

TEST(OctiHttpServerIntegrationTest, Envelope1TrustedProxyRateLimitCrawlerSseAndBounds)
{
    /* 0. Production HttpServerConfig defaults match Section 2.3.1 specifications */
    {
        HttpServerConfig prodDefaults;
        CHECK_EQUAL(96, prodDefaults.maxSseStreams);
        CHECK_EQUAL(64, prodDefaults.maxSsePerIp);
        CHECK_EQUAL(6, prodDefaults.maxSseAnonPerIp);
        CHECK_EQUAL(8, prodDefaults.maxSsePerPlayer);
        LONGS_EQUAL(120000, prodDefaults.sseAnonMaxDurationMs);
        CHECK_EQUAL(30, prodDefaults.authFailureRateLimitMax);
        CHECK_EQUAL(10, prodDefaults.passcodeFailureRateLimitMax);
        CHECK_EQUAL(12, prodDefaults.exportRateLimitMax);
        CHECK_EQUAL(120, prodDefaults.mutationRateLimitMax);
        DOUBLES_EQUAL(25.0, prodDefaults.livePollAnonIpBurst, 1e-6);
        DOUBLES_EQUAL(15.0, prodDefaults.lobbySnapAnonIpBurst, 1e-6);
        DOUBLES_EQUAL(15.0, prodDefaults.browseAnonIpBurst, 1e-6);
    }

    /* 1. GET /robots.txt allows public pages while protecting /api/, and index CSP object-src */
    Reply robots = http().get("/robots.txt");
    EXPECT_STATUS(200, robots);
    CHECK_TRUE(robots.contentType.find("text/plain") == 0);
    STRCMP_EQUAL("User-agent: *\nAllow: /\nDisallow: /api/\n", robots.body.c_str());
    CHECK_TRUE(robots.header("X-Robots-Tag").empty());

    Reply idx = http().get("/");
    EXPECT_STATUS(200, idx);
    CHECK_TRUE(idx.header("X-Robots-Tag").empty());
    CHECK_TRUE(idx.header("Content-Security-Policy").find("object-src 'none'") !=
               std::string::npos);
    CHECK_TRUE(idx.header("Content-Security-Policy").find("frame-ancestors 'none'") !=
               std::string::npos);

    {
        HttpServerConfig disallowRobotsCfg = testConfig();
        disallowRobotsCfg.disallowRobotsTxt = true;
        startEnv(disallowRobotsCfg, true, nullptr);
        Reply disallowReply = http().get("/robots.txt");
        EXPECT_STATUS(200, disallowReply);
        STRCMP_EQUAL("User-agent: *\nDisallow: /\n", disallowReply.body.c_str());
    }

    /* 2. Forwarded client IP isolation, multi-hop XFF, and ::ffff: normalization */
    {
        HttpServerConfig cfg = testConfig();
        cfg.ssoRateLimitMax = 2;
        cfg.ssoRateLimitWindowMs = 60000;
        startEnv(cfg, true, nullptr);

        httplib::Headers ipA = {{"X-Forwarded-For", "198.51.100.10"}};
        httplib::Headers ipAMapped = {{"X-Forwarded-For", "1.2.3.4, ::ffff:198.51.100.10, 127.0.0.1"}};
        httplib::Headers ipB = {{"X-Forwarded-For", "198.51.100.11"}};

        EXPECT_STATUS(400, http().post("/api/v1/auth/sso", "",
                                       Json{{"provider", "google"}, {"id_token", "bad"}}, ipA));
        /* ::ffff:198.51.100.10 and spoofed leftmost 1.2.3.4 map to the SAME 198.51.100.10 bucket */
        EXPECT_STATUS(400, http().post("/api/v1/auth/sso", "",
                                       Json{{"provider", "google"}, {"id_token", "bad"}}, ipAMapped));
        Reply limA = http().post("/api/v1/auth/sso", "",
                                 Json{{"provider", "google"}, {"id_token", "bad"}}, ipA);
        EXPECT_ERROR(429, "RATE_LIMITED", limA);
        CHECK_TRUE(std::atoi(limA.header("Retry-After").c_str()) >= 1);

        /* Multi-header X-Forwarded-For smuggling cannot escape 198.51.100.10 */
        httplib::Headers multiHdr;
        multiHdr.insert(std::make_pair("X-Forwarded-For", "203.0.113.99"));
        multiHdr.insert(std::make_pair("X-Forwarded-For", "198.51.100.10"));
        EXPECT_ERROR(429, "RATE_LIMITED",
                     http().post("/api/v1/auth/sso", "",
                                 Json{{"provider", "google"}, {"id_token", "bad"}}, multiHdr));

        /* Independent client IP behind the same reverse proxy is NOT locked out */
        EXPECT_STATUS(400, http().post("/api/v1/auth/sso", "",
                                       Json{{"provider", "google"}, {"id_token", "bad"}}, ipB));
    }

    /* 3. Untrusted peer spoofing X-Forwarded-For cannot escape peer bucket */
    {
        HttpServerConfig cfg = testConfig();
        cfg.trustedProxies.clear();
        cfg.ssoRateLimitMax = 2;
        cfg.ssoRateLimitWindowMs = 60000;
        startEnv(cfg, true, nullptr);

        EXPECT_STATUS(400, http().post("/api/v1/auth/sso", "",
                                       Json{{"provider", "google"}, {"id_token", "bad"}},
                                       {{"X-Forwarded-For", "198.51.100.1"}}));
        EXPECT_STATUS(400, http().post("/api/v1/auth/sso", "",
                                       Json{{"provider", "google"}, {"id_token", "bad"}},
                                       {{"X-Forwarded-For", "198.51.100.2"}}));
        EXPECT_ERROR(429, "RATE_LIMITED",
                     http().post("/api/v1/auth/sso", "",
                                 Json{{"provider", "google"}, {"id_token", "bad"}},
                                 {{"X-Forwarded-For", "198.51.100.3"}}));
    }

    /* 4. TokenBucketLimiter burst, Retry-After, refill recovery, and shared-NAT actor headroom */
    {
        HttpServerConfig cfg = testConfig();
        cfg.browseAnonIpBurst = 3.0;
        cfg.browseAnonIpRefillPerSec = 4.0;
        cfg.browseActorBurst = 4.0;
        cfg.browseActorRefillPerSec = 4.0;
        cfg.browseAuthedIpBurst = 10.0;
        cfg.browseAuthedIpRefillPerSec = 10.0;

        cfg.lobbySnapAnonIpBurst = 2.0;
        cfg.lobbySnapAnonIpRefillPerSec = 4.0;
        cfg.livePollAnonIpBurst = 2.0;
        cfg.livePollAnonIpRefillPerSec = 4.0;
        startEnv(cfg, true, nullptr);

        User alice = signIn("google", "sub-tb-alice", "Alice Bucket");
        User bob = signIn("apple", "sub-tb-bob", "Bob Bucket");

        httplib::Headers anonIp1 = {{"X-Forwarded-For", "203.0.113.11"}};
        httplib::Headers anonIp2 = {{"X-Forwarded-For", "203.0.113.12"}};
        for (int i = 0; i < 3; ++i) {
            EXPECT_STATUS(200, http().get("/api/v1/archive/games", "", anonIp1));
        }
        Reply burstOver = http().get("/api/v1/archive/games", "", anonIp1);
        EXPECT_ERROR(429, "RATE_LIMITED", burstOver);
        CHECK_TRUE(std::atoi(burstOver.header("Retry-After").c_str()) >= 1);

        /* Other anonymous IP unaffected */
        EXPECT_STATUS(200, http().get("/api/v1/archive/games", "", anonIp2));

        /* Token refill recovery after ~300ms (4.0 tokens/sec -> 1.2 tokens refilled) */
        std::this_thread::sleep_for(std::chrono::milliseconds(320));
        EXPECT_STATUS(200, http().get("/api/v1/archive/games", "", anonIp1));

        /* LobbySnap (/tables) and LivePoll (/lobby/events) exact burst boundaries */
        httplib::Headers snapIp = {{"X-Forwarded-For", "203.0.113.13"}};
        EXPECT_STATUS(200, http().get("/api/v1/tables", "", snapIp));
        EXPECT_STATUS(200, http().get("/api/v1/tables", "", snapIp));
        EXPECT_ERROR(429, "RATE_LIMITED", http().get("/api/v1/tables", "", snapIp));

        EXPECT_STATUS(200, http().get("/api/v1/lobby/events", "", snapIp));
        EXPECT_STATUS(200, http().get("/api/v1/lobby/events", "", snapIp));
        EXPECT_ERROR(429, "RATE_LIMITED", http().get("/api/v1/lobby/events", "", snapIp));

        /* Shared-NAT authenticated headroom: Alice and Bob behind same NAT IP each get full ActorBurst (4) */
        httplib::Headers sharedNat = {{"X-Forwarded-For", "198.51.100.55"}};
        for (int i = 0; i < 4; ++i) {
            EXPECT_STATUS(200, http().get("/api/v1/archive/games", alice.token, sharedNat));
        }
        EXPECT_ERROR(429, "RATE_LIMITED",
                     http().get("/api/v1/archive/games", alice.token, sharedNat));
        /* Bob on the exact same NAT IP still gets all 4 of his actor tokens */
        for (int i = 0; i < 4; ++i) {
            EXPECT_STATUS(200, http().get("/api/v1/archive/games", bob.token, sharedNat));
        }
        EXPECT_ERROR(429, "RATE_LIMITED",
                     http().get("/api/v1/archive/games", bob.token, sharedNat));
    }

    /* 5. Disallowed anonymous crawler UAs rejected on API routes while authenticated bots succeed */
    {
        startEnv(testConfig(), true, nullptr);
        User owner = signIn("google", "sub-crawler-owner", "Bot Trainer");
        Reply rBot = http().post("/api/v1/robots", owner.token,
                                 Json{{"robot_name", "trainer_bot_01"}});
        EXPECT_STATUS(201, rBot);
        std::string botToken = rBot.json.value("robot_token", "");

        const char *crawlers[] = {
            "Mozilla/5.0 (compatible; GPTBot/1.0)",
            "ClaudeBot/1.0 (+https://anthropic.com)",
            "Bytespider; spider-feedback@bytedance.com",
            "Scrapy/2.11.0 (+https://scrapy.org)"
        };
        for (size_t i = 0; i < sizeof(crawlers) / sizeof(crawlers[0]); ++i) {
            httplib::Headers ua = {{"User-Agent", crawlers[i]}};
            EXPECT_ERROR(403, "FORBIDDEN_CRAWLER",
                         http().get("/api/v1/archive/games", "", ua));
            EXPECT_ERROR(403, "FORBIDDEN_CRAWLER",
                         http().get("/api/v1/lobby/state", "", ua));
            /* Health and robots.txt remain accessible */
            EXPECT_STATUS(200, http().get("/robots.txt", "", ua));
            EXPECT_STATUS(200, http().get("/api/v1/health", "", ua));
        }

        /* Authenticated bot and human callers succeed regardless of User-Agent */
        httplib::Headers botCustomUa = {{"User-Agent", "OctiTrainerScript/1.0 (Scrapy/2.11)"}};
        EXPECT_STATUS(200, http().get("/api/v1/archive/games", botToken, botCustomUa));
        EXPECT_STATUS(200, http().get("/api/v1/archive/games", owner.token,
                                      {{"User-Agent", "GPTBot/1.0"}}));
    }

    /* 6. Bearer 401 brute-force, passcode 403 brute-force, plies, export-conf, and mutation throttles */
    {
        HttpServerConfig cfg = testConfig();
        cfg.authFailureRateLimitMax = 3;
        cfg.authFailureRateLimitWindowMs = 60000;
        cfg.passcodeFailureRateLimitMax = 3;
        cfg.passcodeFailureRateLimitWindowMs = 60000;
        cfg.pliesRateLimitMax = 2;
        cfg.pliesRateLimitWindowMs = 60000;
        cfg.robotExportConfRateLimitMax = 2;
        cfg.robotExportConfRateLimitWindowMs = 60000;
        cfg.mutationRateLimitMax = 3;
        cfg.mutationRateLimitWindowMs = 60000;
        startEnv(cfg, true, nullptr);

        User host = signIn("google", "sub-bf-host", "Host Brute");
        User guest = signIn("apple", "sub-bf-guest", "Guest Brute");

        httplib::Headers bruteIp = {{"X-Forwarded-For", "198.51.100.70"}};
        for (int i = 0; i < 3; ++i) {
            EXPECT_ERROR(401, "UNAUTHORIZED",
                         http().get("/api/v1/profile/me", "bogus_token_" + std::to_string(i), bruteIp));
        }
        Reply authLim = http().get("/api/v1/profile/me", "bogus_token_4", bruteIp);
        EXPECT_ERROR(429, "RATE_LIMITED", authLim);
        CHECK_FALSE(authLim.header("Retry-After").empty());
        /* Valid session on the same NAT IP still works */
        EXPECT_STATUS(200, http().get("/api/v1/profile/me", host.token, bruteIp));

        /* Passcode brute-force lockout */
        std::string room = createTable(host, Json{{"passcode", "vault99"}});
        httplib::Headers guestIp = {{"X-Forwarded-For", "198.51.100.71"}};
        cfg.mutationRateLimitMax = 50;
        /* Note: guest gets 3 mutation slots; use 3 wrong passcodes then verify 429 */
        EXPECT_ERROR(403, "FORBIDDEN_PASSCODE",
                     http().post("/api/v1/tables/" + room + "/join", guest.token,
                                 Json{{"passcode", "w1"}}, guestIp));
        EXPECT_ERROR(403, "FORBIDDEN_PASSCODE",
                     http().post("/api/v1/tables/" + room + "/join", guest.token,
                                 Json{{"passcode", "w2"}}, guestIp));
        EXPECT_ERROR(403, "FORBIDDEN_PASSCODE",
                     http().post("/api/v1/tables/" + room + "/join", guest.token,
                                 Json{{"passcode", "w3"}}, guestIp));

        /* Mutation rate limit on /api/v1/auth/data-deletion (max=3) */
        httplib::Headers mutIp = {{"X-Forwarded-For", "198.51.100.72"}};
        for (int i = 0; i < 3; ++i) {
            EXPECT_STATUS(200, http().post("/api/v1/auth/data-deletion", "", Json::object(), mutIp));
        }
        EXPECT_ERROR(429, "RATE_LIMITED",
                     http().post("/api/v1/auth/data-deletion", "", Json::object(), mutIp));

        /* Export-conf rate limit (max=2) */
        httplib::Headers confIp = {{"X-Forwarded-For", "198.51.100.73"}};
        EXPECT_STATUS(200, http().get("/api/v1/robots/export-conf", host.token, confIp));
        EXPECT_STATUS(200, http().get("/api/v1/robots/export-conf", host.token, confIp));
        EXPECT_ERROR(429, "RATE_LIMITED",
                     http().get("/api/v1/robots/export-conf", host.token, confIp));

        /* Plies rate limit (max=2) */
        httplib::Headers pliesIp = {{"X-Forwarded-For", "198.51.100.74"}};
        EXPECT_STATUS(404, http().get("/api/v1/archive/games/nonexistent/plies", "", pliesIp));
        EXPECT_STATUS(404, http().get("/api/v1/archive/games/nonexistent/plies", "", pliesIp));
        EXPECT_ERROR(429, "RATE_LIMITED",
                     http().get("/api/v1/archive/games/nonexistent/plies", "", pliesIp));
    }

    /* Dedicated passcode oracle block check with generous mutation budget */
    {
        HttpServerConfig cfg = testConfig();
        cfg.passcodeFailureRateLimitMax = 2;
        cfg.passcodeFailureRateLimitWindowMs = 60000;
        startEnv(cfg, true, nullptr);

        User host = signIn("google", "sub-pass-host", "Pass Host");
        User attacker = signIn("apple", "sub-pass-atk", "Pass Attacker");
        std::string room = createTable(host, Json{{"passcode", "correct99"}});
        httplib::Headers atkIp = {{"X-Forwarded-For", "198.51.100.75"}};

        EXPECT_ERROR(403, "FORBIDDEN_PASSCODE",
                     http().post("/api/v1/tables/" + room + "/join", attacker.token,
                                 Json{{"passcode", "guess1"}}, atkIp));
        EXPECT_ERROR(403, "FORBIDDEN_PASSCODE",
                     http().post("/api/v1/tables/" + room + "/join", attacker.token,
                                 Json{{"passcode", "guess2"}}, atkIp));
        /* Even sending the real passcode on the 3rd attempt is blocked with 429 */
        EXPECT_ERROR(429, "RATE_LIMITED",
                     http().post("/api/v1/tables/" + room + "/join", attacker.token,
                                 Json{{"passcode", "correct99"}}, atkIp));
    }

    /* 7. SSE per-IP, per-player, and anonymous duration bounds */
    {
        HttpServerConfig cfg = testConfig();
        cfg.maxSseStreams = 16;
        cfg.maxSsePerIp = 2;
        cfg.maxSsePerPlayer = 2;
        cfg.sseAnonMaxDurationMs = 250;
        cfg.sseMaxDurationMs = 5000;
        startEnv(cfg, true, nullptr);

        User alice = signIn("google", "sub-sse-alice", "Alice Sse");
        User bob = signIn("apple", "sub-sse-bob", "Bob Sse");

        httplib::Headers sseIp1 = {{"X-Forwarded-For", "198.51.100.81"}};
        httplib::Headers sseIp2 = {{"X-Forwarded-For", "198.51.100.82"}};
        httplib::Headers sseIp3 = {{"X-Forwarded-For", "198.51.100.83"}};

        /* Per-IP boundary: 2 allowed on 198.51.100.81, 3rd returns 429 TOO_MANY_STREAMS */
        SseClient s1;
        SseClient s2;
        s1.start(env->server->port(), "/api/v1/lobby/events?stream=1", alice.token, sseIp1);
        s2.start(env->server->port(), "/api/v1/lobby/events?stream=1", bob.token, sseIp1);
        CHECK_TRUE(s1.waitFor("retry: 3000", 3000));
        CHECK_TRUE(s2.waitFor("retry: 3000", 3000));

        Reply ipOver = http().get("/api/v1/lobby/events?stream=1", bob.token, sseIp1);
        EXPECT_ERROR(429, "TOO_MANY_STREAMS", ipOver);
        CHECK_FALSE(ipOver.header("Retry-After").empty());

        /* Per-player boundary across distinct IPs: Alice opens 2nd stream on sseIp2, 3rd on sseIp3 fails 429 */
        SseClient sAlice2;
        sAlice2.start(env->server->port(), "/api/v1/lobby/events?stream=1", alice.token, sseIp2);
        CHECK_TRUE(sAlice2.waitFor("retry: 3000", 3000));

        Reply playerOver = http().get("/api/v1/lobby/events?stream=1", alice.token, sseIp3);
        EXPECT_ERROR(429, "TOO_MANY_STREAMS", playerOver);
        /* Bob still has 1 remaining player slot and sseIp3 is free -> succeeds */
        SseClient sBob2;
        sBob2.start(env->server->port(), "/api/v1/lobby/events?stream=1", bob.token, sseIp3);
        CHECK_TRUE(sBob2.waitFor("retry: 3000", 3000));

        s1.stop();
        s2.stop();
        sAlice2.stop();
        sBob2.stop();
        bool allFreed = false;
        for (int i = 0; i < 200 && !allFreed; ++i) {
            allFreed = (env->server->activeStreams() == 0);
            if (!allFreed) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
        CHECK_TRUE(allFreed);

        /* Anonymous SSE stream reconnects at sseAnonMaxDurationMs (250ms) */
        SseClient anonSse;
        anonSse.start(env->server->port(), "/api/v1/lobby/events?stream=1", "", sseIp1);
        CHECK_TRUE(anonSse.waitFor("event: reconnect", 2000));
        CHECK_TRUE(anonSse.waitFinished(2000));
        anonSse.stop();

        /* Shared-NAT regression: 8 concurrent authed streams + wait_ms polling
         * succeed on one IP while anonymous SSE remains tightly capped per IP. */
        HttpServerConfig natCfg = testConfig();
        natCfg.maxSseStreams = 32;
        natCfg.maxSsePerIp = 16;
        natCfg.maxSseAnonPerIp = 2;
        natCfg.maxSsePerPlayer = 4;
        natCfg.sseAnonMaxDurationMs = 5000;
        natCfg.sseMaxDurationMs = 5000;
        startEnv(natCfg, true, nullptr);

        User u1 = signIn("google", "sub-nat-1", "NatUser1");
        User u2 = signIn("apple", "sub-nat-2", "NatUser2");
        User u3 = signIn("microsoft", "sub-nat-3", "NatUser3");
        User u4 = signIn("google", "sub-nat-4", "NatUser4");
        const httplib::Headers natHdr = {{"X-Forwarded-For", "203.0.113.200"}};
        std::vector<std::unique_ptr<SseClient> > natStreams;
        const std::string tokens[4] = {u1.token, u2.token, u3.token, u4.token};
        for (int idx = 0; idx < 8; ++idx) {
            auto sc = std::make_unique<SseClient>();
            sc->start(env->server->port(), "/api/v1/lobby/events?stream=1",
                      tokens[idx % 4], natHdr);
            CHECK_TRUE(sc->waitFor("retry: 3000", 3000));
            natStreams.push_back(std::move(sc));
        }

        /* 2 anonymous SSE streams still allowed on same NAT IP, 3rd returns 429,
         * and wait_ms polling fallback still returns 200. */
        SseClient a1, a2;
        a1.start(env->server->port(), "/api/v1/lobby/events?stream=1", "", natHdr);
        a2.start(env->server->port(), "/api/v1/lobby/events?stream=1", "", natHdr);
        CHECK_TRUE(a1.waitFor("retry: 3000", 3000));
        CHECK_TRUE(a2.waitFor("retry: 3000", 3000));
        Reply a3 = http().get("/api/v1/lobby/events?stream=1", "", natHdr);
        EXPECT_ERROR(429, "TOO_MANY_STREAMS", a3);
        EXPECT_STATUS(200, http().get("/api/v1/lobby/events?since_seq=999999&wait_ms=10",
                                      "", natHdr));

        a1.stop();
        a2.stop();
        natStreams.clear();
        for (int i = 0; i < 200 && env->server->activeStreams() != 0; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        CHECK_EQUAL(0, env->server->activeStreams());
    }

    /* 8. Per-route payload caps (16 KiB / 64 KiB / 128 KiB / 512 KiB) & form Content-Type checks */
    {
        startEnv(testConfig(), true, nullptr);
        User alice = signIn("google", "sub-bounds-alice", "Alice Bounds");

        /* 20 KiB on 16 KiB routes (/lobby/chat, /auth/data-deletion, /privacy) rejected with 413 */
        std::string body20k = "{\"text\":\"" + repeat('x', 20480) + "\"}";
        EXPECT_ERROR(413, "PAYLOAD_TOO_LARGE",
                     http().raw("POST", "/api/v1/lobby/chat", alice.token,
                                body20k, "application/json"));
        EXPECT_ERROR(413, "PAYLOAD_TOO_LARGE",
                     http().raw("POST", "/api/v1/auth/data-deletion", "",
                                body20k, "application/json"));
        EXPECT_ERROR(413, "PAYLOAD_TOO_LARGE",
                     http().raw("POST", "/privacy", "",
                                "signed_request=" + repeat('x', 20480),
                                "application/x-www-form-urlencoded"));

        /* Valid form-urlencoded on /api/v1/auth/data-deletion and /privacy succeeds, octet-stream rejected 415 */
        EXPECT_STATUS(200, http().raw("POST", "/api/v1/auth/data-deletion", "",
                                      "signed_request=abc.def",
                                      "application/x-www-form-urlencoded"));
        EXPECT_STATUS(200, http().raw("POST", "/privacy", "",
                                      "signed_request=abc.def",
                                      "application/x-www-form-urlencoded"));
        EXPECT_ERROR(415, "UNSUPPORTED_MEDIA_TYPE",
                     http().raw("POST", "/api/v1/auth/data-deletion", "",
                                "binary-bytes", "application/octet-stream"));

        /* 70 KiB passes 128 KiB transport cap on /archive/upload (fails schema 400) while 140 KiB fails 413 */
        std::string body70k = "{\"transcript\":\"" + repeat('a', 70000) + "\"}";
        EXPECT_ERROR(400, "MALFORMED_UPLOAD",
                     http().raw("POST", "/api/v1/archive/upload", "",
                                body70k, "application/json"));
        std::string body140k = "{\"transcript\":\"" + repeat('a', 140000) + "\"}";
        EXPECT_ERROR(413, "PAYLOAD_TOO_LARGE",
                     http().raw("POST", "/api/v1/archive/upload", "",
                                body140k, "application/json"));
    }
}

TEST(OctiHttpServerIntegrationTest, Envelope2OauthAdminInstallAndUploadSecurityGate)
{
    auto urlDecodeParam = [](const std::string &in) -> std::string {
        std::string out;
        for (size_t i = 0; i < in.size(); ++i) {
            if (in[i] == '%' && i + 2 < in.size()) {
                char hex[3] = {in[i + 1], in[i + 2], '\0'};
                char *endPtr = nullptr;
                long v = std::strtol(hex, &endPtr, 16);
                if (endPtr == hex + 2) {
                    out.push_back(static_cast<char>(v));
                    i += 2;
                    continue;
                }
            } else if (in[i] == '+') {
                out.push_back(' ');
                continue;
            }
            out.push_back(in[i]);
        }
        return out;
    };
    auto extractQueryParam = [&](const std::string &url,
                                 const std::string &name) -> std::string {
        size_t q = url.find('?');
        if (q == std::string::npos) {
            return std::string();
        }
        std::string query = url.substr(q + 1);
        size_t hash = query.find('#');
        if (hash != std::string::npos) {
            query = query.substr(0, hash);
        }
        std::string prefix = name + "=";
        size_t pos = 0;
        while (pos < query.size()) {
            size_t amp = query.find('&', pos);
            std::string part = (amp == std::string::npos)
                ? query.substr(pos)
                : query.substr(pos, amp - pos);
            if (part.rfind(prefix, 0) == 0) {
                return urlDecodeParam(part.substr(prefix.size()));
            }
            if (amp == std::string::npos) {
                break;
            }
            pos = amp + 1;
        }
        return std::string();
    };

    /* 1. WebUI <meta name="robots"> present, ?auth_token= removed, export-conf uses octi_jwt_token */
    Reply rRoot = http().get("/");
    EXPECT_STATUS(200, rRoot);
    CHECK_TRUE(rRoot.body.find("<meta name=\"robots\" content=\"index, follow\">") != std::string::npos);
    CHECK_TRUE(rRoot.body.find("searchParams.get(\"auth_token\")") == std::string::npos);
    CHECK_TRUE(rRoot.body.find("var tok = sessionToken") == std::string::npos);
    CHECK_TRUE(rRoot.body.find("localStorage.getItem(\"octi_jwt_token\")") != std::string::npos);

    /* 2. Verified-email Google admin check + crash telemetry gate + /api/v1/robots* Cache-Control */
    env->store.setAdminGoogleIdentities({"admin@selfso.com", "sub-explicit-admin"});

    /* Unauthenticated crash reads -> 401 */
    EXPECT_STATUS(401, http().get("/api/v1/telemetry/crashes"));
    EXPECT_STATUS(401, http().get("/api/v1/telemetry/crashes/some-id"));

    /* Google JWT with email=admin@selfso.com and email_verified=false -> NOT admin (403) */
    std::string tokUnver = makeToken(
        "google", "sub-evil-unver", "Evil Unverified",
        [](Json &, Json &c) {
            c["email"] = "admin@selfso.com";
            c["email_verified"] = false;
        });
    Reply rSsoUnver = http().post("/api/v1/auth/sso", "",
                                  Json{{"provider", "google"},
                                       {"id_token", tokUnver},
                                       {"client_agent", "test/1.0"}});
    CHECK_TRUE(rSsoUnver.status == 200 || rSsoUnver.status == 201);
    CHECK_FALSE(rSsoUnver.json["profile"].value("is_admin", true));
    std::string sessUnver = rSsoUnver.json.value("session_token", "");
    EXPECT_ERROR(403, "FORBIDDEN", http().get("/api/v1/telemetry/crashes", sessUnver));

    /* Google JWT omitting email_verified -> NOT admin (403) */
    std::string tokMissingEv = makeToken(
        "google", "sub-evil-no-ev", "Evil Missing EV",
        [](Json &, Json &c) {
            c["email"] = "admin@selfso.com";
            c.erase("email_verified");
        });
    Reply rSsoMissingEv = http().post("/api/v1/auth/sso", "",
                                      Json{{"provider", "google"},
                                           {"id_token", tokMissingEv},
                                           {"client_agent", "test/1.0"}});
    CHECK_TRUE(rSsoMissingEv.status == 200 || rSsoMissingEv.status == 201);
    CHECK_FALSE(rSsoMissingEv.json["profile"].value("is_admin", true));
    EXPECT_ERROR(403, "FORBIDDEN",
                 http().get("/api/v1/telemetry/crashes",
                            rSsoMissingEv.json.value("session_token", "")));

    /* Google JWT with email=admin@selfso.com and email_verified=true -> IS admin (200) */
    std::string tokVerAdmin = makeToken(
        "google", "sub-good-ver-admin", "Verified Admin",
        [](Json &, Json &c) {
            c["email"] = "admin@selfso.com";
            c["email_verified"] = true;
        });
    Reply rSsoAdmin = http().post("/api/v1/auth/sso", "",
                                  Json{{"provider", "google"},
                                       {"id_token", tokVerAdmin},
                                       {"client_agent", "test/1.0"}});
    CHECK_TRUE(rSsoAdmin.status == 200 || rSsoAdmin.status == 201);
    CHECK_TRUE(rSsoAdmin.json["profile"].value("is_admin", false));
    std::string adminSess = rSsoAdmin.json.value("session_token", "");
    EXPECT_STATUS(200, http().get("/api/v1/telemetry/crashes", adminSess));

    /* Robot token denied on crash telemetry & /api/v1/robots* emits Cache-Control: no-store */
    Reply rBotCreate = http().post("/api/v1/robots", sessUnver,
                                   Json{{"robot_name", "SecGateBot"}});
    EXPECT_STATUS(201, rBotCreate);
    STRCMP_EQUAL("no-store", rBotCreate.header("Cache-Control").c_str());
    std::string botToken = rBotCreate.json.value("robot_token", "");
    EXPECT_ERROR(403, "FORBIDDEN", http().get("/api/v1/telemetry/crashes", botToken));

    Reply rRobotsList = http().get("/api/v1/robots", sessUnver);
    EXPECT_STATUS(200, rRobotsList);
    STRCMP_EQUAL("no-store", rRobotsList.header("Cache-Control").c_str());
    Reply rRobotsExport = http().get("/api/v1/robots/export-conf", sessUnver);
    EXPECT_STATUS(200, rRobotsExport);
    STRCMP_EQUAL("no-store", rRobotsExport.header("Cache-Control").c_str());

    /* 3. Install-token minting, boundary checks & authorized DELETE /api/v1/archive/install/:id */
    const std::string instId = "inst-sec-e2-token";
    const std::string instTok32 = "a1b2c3d4e5f60718a1b2c3d4e5f60718";
    EXPECT_STATUS(401, http().del("/api/v1/archive/install/" + instId, ""));
    EXPECT_STATUS(401, http().del("/api/v1/archive/install/" + instId, "",
                                  {{"X-Octi-Install-Token", ""}}));

    /* Empty X-Octi-Install-Token on upload rejected 400 before replay */
    std::string trInst = makeWonTranscript(instId, 3);
    EXPECT_ERROR(400, "MALFORMED_UPLOAD",
                 http().post("/api/v1/archive/upload", "",
                             Json{{"transcript", trInst}},
                             {{"X-Octi-Install-Token", ""}}));

    /* Valid 32-char X-Octi-Install-Token mints install_tokens row */
    EXPECT_STATUS(200, http().post("/api/v1/archive/upload", "",
                                   Json{{"transcript", trInst}},
                                   {{"X-Octi-Install-Token", instTok32}}));

    /* Uploading under registered installId without token or with wrong token rejected 403 */
    std::string trInst2 = makeResignedTranscript({"33+a", "37+e"}, instId, 4);
    EXPECT_ERROR(403, "FORBIDDEN",
                 http().post("/api/v1/archive/upload", "",
                             Json{{"transcript", trInst2}}));
    EXPECT_ERROR(403, "FORBIDDEN",
                 http().post("/api/v1/archive/upload", "",
                             Json{{"transcript", trInst2}},
                             {{"X-Octi-Install-Token", "00000000000000000000000000000000"}}));

    /* Delete with wrong token, non-owner user, or robot rejected 403 */
    EXPECT_ERROR(403, "FORBIDDEN",
                 http().del("/api/v1/archive/install/" + instId, "",
                            {{"X-Octi-Install-Token", "00000000000000000000000000000000"}}));
    EXPECT_ERROR(403, "FORBIDDEN",
                 http().del("/api/v1/archive/install/" + instId, sessUnver));
    EXPECT_ERROR(403, "FORBIDDEN",
                 http().del("/api/v1/archive/install/" + instId, botToken));

    /* Delete with valid 32-char token succeeds (deleted=1), re-delete returns 0, admin returns 0 */
    Reply rInstDel1 = http().del("/api/v1/archive/install/" + instId, "",
                                 {{"X-Octi-Install-Token", instTok32}});
    EXPECT_STATUS(200, rInstDel1);
    CHECK_EQUAL(1, rInstDel1.json.value("deleted", -1));
    Reply rInstDel2 = http().del("/api/v1/archive/install/" + instId, "",
                                 {{"X-Octi-Install-Token", instTok32}});
    EXPECT_STATUS(200, rInstDel2);
    CHECK_EQUAL(0, rInstDel2.json.value("deleted", -1));
    Reply rInstDelAdmin = http().del("/api/v1/archive/install/" + instId, adminSess);
    EXPECT_STATUS(200, rInstDelAdmin);
    CHECK_EQUAL(0, rInstDelAdmin.json.value("deleted", -1));

    /* 4. Anonymous client_upload spoofing playerId or robot_name cannot pollute profiles/stats */
    User victim = signIn("google", "sub-spoof-victim", "Spoof Victim");
    EXPECT_STATUS(200, http().patch("/api/v1/profile/me", victim.token,
                                    Json{{"handle", "victim_hero"}}));
    octi::PlayerProfile victimProf;
    CHECK_TRUE(env->store.getProfileByUuid(victim.uuid, victimProf));

    Json spoofTr = Json::parse(makeResignedTranscript({"33+b", "37+f"}, "inst-anon-spoof", 3));
    spoofTr["players"] = Json::array({
        Json{{"seat", 1}, {"player", "A"}, {"type", "HUMAN"},
             {"handle", "victim_hero"}, {"playerId", victimProf.playerId},
             {"player_uuid", victim.uuid}},
        Json{{"seat", 2}, {"player", "B"}, {"type", "AI"},
             {"handle", "SecGateBot"}, {"aiDifficulty", 3}}
    });
    Reply rSpoofUp = http().post("/api/v1/archive/upload", "",
                                 Json{{"transcript", spoofTr.dump()}});
    EXPECT_STATUS(200, rSpoofUp);
    CHECK_TRUE(env->store.reconcileRobotStats());
    EXPECT_STATUS(200, http().post("/api/v1/admin/recompute-ratings", adminSess, Json::object()));

    Reply rVictimGet = http().get("/api/v1/users/victim_hero");
    EXPECT_STATUS(200, rVictimGet);
    CHECK_EQUAL(0, static_cast<int>(rVictimGet.json["recent_games"].size()));

    Reply rBotListAfter = http().get("/api/v1/robots", sessUnver);
    EXPECT_STATUS(200, rBotListAfter);
    CHECK_EQUAL(1, static_cast<int>(rBotListAfter.json["robots"].size()));
    CHECK_EQUAL(0, rBotListAfter.json["robots"][0].value("games_played", -1));
    CHECK_EQUAL(0, rBotListAfter.json["robots"][0].value("wins", -1));
    CHECK_EQUAL(0, rBotListAfter.json["robots"][0].value("losses", -1));

    /* 5. OAuth state CSPRNG/HMAC, single-use replay block, nonce, Host check, eviction & native redirect */
    {
        HttpServerConfig oCfg = testConfig();
        oCfg.allowSyntheticOauthCallbackForTesting = true;
        oCfg.publicBaseUrl = "https://octi.selfso.com";
        oCfg.oauthStateSecret = "unit-test-oauth-secret-key-material";
        oCfg.maxPendingOauthStates = 3;
        startEnv(oCfg, true, nullptr);

        /* Spoofed Host rejected 400 on both authorize and callback */
        EXPECT_ERROR(400, "INVALID_HOST",
                     http().get("/api/v1/auth/authorize?provider=google&redirect_uri=/#account",
                                "", {{"Host", "evil.example"}}));
        EXPECT_ERROR(400, "INVALID_HOST",
                     http().get("/api/v1/auth/callback?state=a.b",
                                "", {{"Host", "evil.example"}}));

        /* Authorize -> verify publicBaseUrl callback_url, state, and OIDC nonce */
        Reply rAuthWeb = http().get("/api/v1/auth/authorize?provider=google&redirect_uri=/#account");
        EXPECT_STATUS(302, rAuthWeb);
        std::string locWeb = rAuthWeb.header("Location");
        STRCMP_EQUAL("https://octi.selfso.com/api/v1/auth/callback",
                     extractQueryParam(locWeb, "redirect_uri").c_str());
        std::string stateWeb = extractQueryParam(locWeb, "state");
        std::string nonceWeb = extractQueryParam(locWeb, "nonce");
        CHECK_FALSE(stateWeb.empty());
        CHECK_FALSE(nonceWeb.empty());

        /* Forged HMAC signature rejected 400 */
        size_t dotPos = stateWeb.find('.');
        CHECK_TRUE(dotPos != std::string::npos);
        std::string tamperedState = stateWeb.substr(0, dotPos) + ".AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
        EXPECT_ERROR(400, "INVALID_OAUTH_STATE",
                     http().get("/api/v1/auth/callback?state=" + tamperedState));

        /* Wrong OIDC nonce rejected 401 TOKEN_REJECTED and consumes state single-use */
        std::string wrongNonceJwt = makeToken(
            "google", "sub-oauth-web-1", "OAuth Web User",
            [](Json &, Json &c) { c["nonce"] = "wrong-nonce-value"; });
        EXPECT_ERROR(401, "TOKEN_REJECTED",
                     http().get("/api/v1/auth/callback?state=" + stateWeb +
                                "&id_token=" + wrongNonceJwt));
        /* Replaying consumed state rejected 400 INVALID_OAUTH_STATE */
        EXPECT_ERROR(400, "INVALID_OAUTH_STATE",
                     http().get("/api/v1/auth/callback?state=" + stateWeb +
                                "&id_token=" + wrongNonceJwt));

        /* Nominal single-use web redirect: fragment-only /#auth_token=... */
        Reply rAuthWeb2 = http().get("/api/v1/auth/authorize?provider=google&redirect_uri=/#account");
        EXPECT_STATUS(302, rAuthWeb2);
        std::string stateWeb2 = extractQueryParam(rAuthWeb2.header("Location"), "state");
        std::string nonceWeb2 = extractQueryParam(rAuthWeb2.header("Location"), "nonce");
        std::string goodWebJwt = makeToken(
            "google", "sub-oauth-web-1", "OAuth Web User",
            [&](Json &, Json &c) { c["nonce"] = nonceWeb2; });
        Reply rCbWeb = http().get("/api/v1/auth/callback?state=" + stateWeb2 +
                                  "&id_token=" + goodWebJwt);
        EXPECT_STATUS(302, rCbWeb);
        std::string webTarget = rCbWeb.header("Location");
        CHECK_TRUE(webTarget.rfind("/#auth_token=", 0) == 0);
        CHECK_TRUE(webTarget.find("?auth_token=") == std::string::npos);
        CHECK_TRUE(webTarget.find("id_token=") == std::string::npos);
        /* Replaying redeemed stateWeb2 fails 400 INVALID_OAUTH_STATE */
        EXPECT_ERROR(400, "INVALID_OAUTH_STATE",
                     http().get("/api/v1/auth/callback?state=" + stateWeb2 +
                                "&id_token=" + goodWebJwt));

        /* Section 0 item 2: Native loopback & octi:// redirects preserve id_token */
        Reply rAuthLoop = http().get(
            "/api/v1/auth/authorize?provider=google&redirect_uri=http%3A%2F%2F127.0.0.1%3A43210%2Foauth%2Fcallback");
        EXPECT_STATUS(302, rAuthLoop);
        std::string stateLoop = extractQueryParam(rAuthLoop.header("Location"), "state");
        std::string nonceLoop = extractQueryParam(rAuthLoop.header("Location"), "nonce");
        std::string loopJwt = makeToken(
            "google", "sub-oauth-loop-1", "Loopback User",
            [&](Json &, Json &c) { c["nonce"] = nonceLoop; });
        Reply rCbLoop = http().get("/api/v1/auth/callback?state=" + stateLoop +
                                   "&id_token=" + loopJwt);
        EXPECT_STATUS(302, rCbLoop);
        std::string loopTarget = rCbLoop.header("Location");
        CHECK_TRUE(loopTarget.rfind("http://127.0.0.1:43210/oauth/callback?", 0) == 0);
        STRCMP_EQUAL(loopJwt.c_str(), extractQueryParam(loopTarget, "id_token").c_str());
        CHECK_FALSE(extractQueryParam(loopTarget, "auth_token").empty());

        Reply rAuthScheme = http().get(
            "/api/v1/auth/authorize?provider=google&redirect_uri=octi%3A%2F%2Fauth%2Fcallback");
        EXPECT_STATUS(302, rAuthScheme);
        std::string stateScheme = extractQueryParam(rAuthScheme.header("Location"), "state");
        std::string nonceScheme = extractQueryParam(rAuthScheme.header("Location"), "nonce");
        std::string schemeJwt = makeToken(
            "google", "sub-oauth-scheme-1", "Android Scheme User",
            [&](Json &, Json &c) { c["nonce"] = nonceScheme; });
        Reply rCbScheme = http().get("/api/v1/auth/callback?state=" + stateScheme +
                                     "&id_token=" + schemeJwt);
        EXPECT_STATUS(302, rCbScheme);
        std::string schemeTarget = rCbScheme.header("Location");
        CHECK_TRUE(schemeTarget.rfind("octi://auth/callback?", 0) == 0);
        STRCMP_EQUAL(schemeJwt.c_str(), extractQueryParam(schemeTarget, "id_token").c_str());

        /* Boundary: pendingOauthStates capacity eviction (maxPendingOauthStates == 3) */
        Reply rCap1 = http().get("/api/v1/auth/authorize?provider=google&redirect_uri=/#account");
        Reply rCap2 = http().get("/api/v1/auth/authorize?provider=google&redirect_uri=/#account");
        Reply rCap3 = http().get("/api/v1/auth/authorize?provider=google&redirect_uri=/#account");
        Reply rCap4 = http().get("/api/v1/auth/authorize?provider=google&redirect_uri=/#account");
        std::string sCap1 = extractQueryParam(rCap1.header("Location"), "state");
        std::string nCap1 = extractQueryParam(rCap1.header("Location"), "nonce");
        std::string sCap4 = extractQueryParam(rCap4.header("Location"), "state");
        std::string nCap4 = extractQueryParam(rCap4.header("Location"), "nonce");
        std::string jwtCap1 = makeToken("google", "sub-cap-1", "Cap1",
                                        [&](Json &, Json &c) { c["nonce"] = nCap1; });
        std::string jwtCap4 = makeToken("google", "sub-cap-4", "Cap4",
                                        [&](Json &, Json &c) { c["nonce"] = nCap4; });
        /* Oldest state sCap1 was evicted when sCap4 arrived */
        EXPECT_ERROR(400, "INVALID_OAUTH_STATE",
                     http().get("/api/v1/auth/callback?state=" + sCap1 +
                                "&id_token=" + jwtCap1));
        /* Newest state sCap4 succeeds */
        EXPECT_STATUS(302,
                      http().get("/api/v1/auth/callback?state=" + sCap4 +
                                 "&id_token=" + jwtCap4));

        /* Boundary: expired state (exp < now) rejected 400 INVALID_OAUTH_STATE */
        unsigned char derivedSecret[32];
        unsigned int mdLen = 0;
        EVP_Digest(oCfg.oauthStateSecret.data(), oCfg.oauthStateSecret.size(),
                   derivedSecret, &mdLen, EVP_sha256(), nullptr);
        Json expStateJson = {
            {"provider", "google"},
            {"redirect_uri", "/#account"},
            {"callback_url", "https://octi.selfso.com/api/v1/auth/callback"},
            {"jti", "expired-jti"},
            {"oidc_nonce", "expired-nonce"},
            {"exp", realNow() - 10}
        };
        std::string expB64 = base64UrlEncode(expStateJson.dump());
        unsigned char hmacBuf[EVP_MAX_MD_SIZE];
        unsigned int hmacLen = 0;
        HMAC(EVP_sha256(), derivedSecret, 32,
             reinterpret_cast<const unsigned char *>(expB64.data()),
             expB64.size(), hmacBuf, &hmacLen);
        std::string expSigB64 = base64UrlEncode(hmacBuf, hmacLen);
        EXPECT_ERROR(400, "INVALID_OAUTH_STATE",
                     http().get("/api/v1/auth/callback?state=" + expB64 + "." + expSigB64));
    }

    /* 6. SEC-14: When providerClientSecrets configured, ?id_token= and JWT-shaped ?code= rejected 400 */
    {
        HttpServerConfig sCfg = testConfig();
        sCfg.allowSyntheticOauthCallbackForTesting = true;
        sCfg.providerClientSecrets["google"] = "configured-google-client-secret";
        startEnv(sCfg, true, nullptr);

        Reply rAuth1 = http().get("/api/v1/auth/authorize?provider=google&redirect_uri=/#account");
        EXPECT_STATUS(302, rAuth1);
        std::string st1 = extractQueryParam(rAuth1.header("Location"), "state");
        std::string n1 = extractQueryParam(rAuth1.header("Location"), "nonce");
        std::string forgedJwt = makeToken("google", "sub-forged-cb", "Forged",
                                          [&](Json &, Json &c) { c["nonce"] = n1; });
        EXPECT_ERROR(400, "INVALID_OAUTH_CALLBACK",
                     http().get("/api/v1/auth/callback?state=" + st1 +
                                "&id_token=" + forgedJwt));

        Reply rAuth2 = http().get("/api/v1/auth/authorize?provider=google&redirect_uri=/#account");
        EXPECT_STATUS(302, rAuth2);
        std::string st2 = extractQueryParam(rAuth2.header("Location"), "state");
        EXPECT_ERROR(400, "INVALID_OAUTH_CALLBACK",
                     http().get("/api/v1/auth/callback?state=" + st2 +
                                "&code=" + forgedJwt));
    }
}

namespace {

std::string e3StdBase64Encode(const std::string &raw)
{
    if (raw.empty()) {
        return std::string();
    }
    std::string out(4 * ((raw.size() + 2) / 3), '\0');
    int n = EVP_EncodeBlock(
        reinterpret_cast<unsigned char *>(&out[0]),
        reinterpret_cast<const unsigned char *>(raw.data()),
        static_cast<int>(raw.size()));
    if (n <= 0) {
        return std::string();
    }
    out.resize(static_cast<size_t>(n));
    return out;
}

struct ScopedLoopbackProbeEnv {
    explicit ScopedLoopbackProbeEnv(bool allow)
    {
        if (allow) {
            setenv("OCTI_ALLOW_LOOPBACK_ROBOT_URL_FOR_TEST", "1", 1);
        } else {
            unsetenv("OCTI_ALLOW_LOOPBACK_ROBOT_URL_FOR_TEST");
        }
    }
    ~ScopedLoopbackProbeEnv()
    {
        unsetenv("OCTI_ALLOW_LOOPBACK_ROBOT_URL_FOR_TEST");
    }
};

class LocalSelfSignedTlsFixture {
public:
    explicit LocalSelfSignedTlsFixture(const char *sanSpec)
        : _listenFd(-1), _port(0), _stop(false), _ctx(nullptr), _cert(nullptr)
    {
        _cert = X509_new();
        ASN1_INTEGER_set(X509_get_serialNumber(_cert), 1);
        X509_gmtime_adj(X509_getm_notBefore(_cert), -60);
        X509_gmtime_adj(X509_getm_notAfter(_cert), 3600);
        X509_set_pubkey(_cert, keys().rsa.get());
        X509_NAME *subj = X509_get_subject_name(_cert);
        X509_NAME_add_entry_by_txt(
            subj, "CN", MBSTRING_ASC,
            reinterpret_cast<const unsigned char *>("localhost"), -1, -1, 0);
        X509_set_issuer_name(_cert, subj);
        if (sanSpec != nullptr && sanSpec[0] != '\0') {
            X509V3_CTX v3ctx;
            X509V3_set_ctx_nodb(&v3ctx);
            X509V3_set_ctx(&v3ctx, _cert, _cert, nullptr, nullptr, 0);
            X509_EXTENSION *ext = X509V3_EXT_conf_nid(
                nullptr, &v3ctx, NID_subject_alt_name, sanSpec);
            if (ext != nullptr) {
                X509_add_ext(_cert, ext, -1);
                X509_EXTENSION_free(ext);
            }
        }
        X509_sign(_cert, keys().rsa.get(), EVP_sha256());

        _ctx = SSL_CTX_new(TLS_server_method());
        SSL_CTX_use_certificate(_ctx, _cert);
        SSL_CTX_use_PrivateKey(_ctx, keys().rsa.get());

        _listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        ::setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        struct sockaddr_in sin;
        std::memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        sin.sin_port = 0;
        if (::bind(_listenFd, reinterpret_cast<struct sockaddr *>(&sin), sizeof(sin)) == 0 &&
            ::listen(_listenFd, 8) == 0) {
            socklen_t len = sizeof(sin);
            if (::getsockname(_listenFd, reinterpret_cast<struct sockaddr *>(&sin), &len) == 0) {
                _port = ntohs(sin.sin_port);
            }
        }
        _thread = std::thread([this]() { serveLoop(); });
    }

    ~LocalSelfSignedTlsFixture()
    {
        _stop.store(true);
        if (_listenFd >= 0) {
            ::shutdown(_listenFd, SHUT_RDWR);
            ::close(_listenFd);
            _listenFd = -1;
        }
        if (_thread.joinable()) {
            _thread.join();
        }
        if (_ctx != nullptr) {
            SSL_CTX_free(_ctx);
        }
        if (_cert != nullptr) {
            X509_free(_cert);
        }
    }

    int port() const { return _port; }

private:
    void serveLoop()
    {
        while (!_stop.load()) {
            struct pollfd pfd;
            pfd.fd = _listenFd;
            pfd.events = POLLIN;
            int prc = ::poll(&pfd, 1, 50);
            if (prc <= 0 || _stop.load()) {
                continue;
            }
            int cfd = ::accept(_listenFd, nullptr, nullptr);
            if (cfd < 0) {
                continue;
            }
            struct timeval tv;
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            ::setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            ::setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
            SSL *ssl = SSL_new(_ctx);
            SSL_set_fd(ssl, cfd);
            if (SSL_accept(ssl) == 1) {
                char buf[1024];
                (void)SSL_read(ssl, buf, sizeof(buf));
                const char resp[] =
                    "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK";
                (void)SSL_write(ssl, resp, static_cast<int>(sizeof(resp) - 1));
                SSL_shutdown(ssl);
            }
            SSL_free(ssl);
            ::close(cfd);
        }
    }

    int _listenFd;
    int _port;
    std::atomic<bool> _stop;
    SSL_CTX *_ctx;
    X509 *_cert;
    std::thread _thread;
};

class LocalBarrierHttpProbeFixture {
public:
    LocalBarrierHttpProbeFixture()
        : _listenFd(-1), _port(0), _stop(false), _release(false), _inFlight(0)
    {
        _listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        ::setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        struct sockaddr_in sin;
        std::memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        sin.sin_port = 0;
        if (::bind(_listenFd, reinterpret_cast<struct sockaddr *>(&sin), sizeof(sin)) == 0 &&
            ::listen(_listenFd, 16) == 0) {
            socklen_t len = sizeof(sin);
            if (::getsockname(_listenFd, reinterpret_cast<struct sockaddr *>(&sin), &len) == 0) {
                _port = ntohs(sin.sin_port);
            }
        }
        _acceptThread = std::thread([this]() { acceptLoop(); });
    }

    ~LocalBarrierHttpProbeFixture()
    {
        releaseAll();
        _stop.store(true);
        if (_listenFd >= 0) {
            ::shutdown(_listenFd, SHUT_RDWR);
            ::close(_listenFd);
            _listenFd = -1;
        }
        if (_acceptThread.joinable()) {
            _acceptThread.join();
        }
        for (size_t i = 0; i < _workers.size(); ++i) {
            if (_workers[i].joinable()) {
                _workers[i].join();
            }
        }
    }

    int port() const { return _port; }

    bool waitForInFlight(int target, int timeoutMs)
    {
        std::unique_lock<std::mutex> lock(_mu);
        return _cv.wait_for(
            lock, std::chrono::milliseconds(timeoutMs),
            [&]() { return _inFlight >= target || _stop.load(); });
    }

    void releaseAll()
    {
        {
            std::lock_guard<std::mutex> lock(_mu);
            _release = true;
        }
        _cv.notify_all();
    }

private:
    void acceptLoop()
    {
        while (!_stop.load()) {
            struct pollfd pfd;
            pfd.fd = _listenFd;
            pfd.events = POLLIN;
            int prc = ::poll(&pfd, 1, 50);
            if (prc <= 0 || _stop.load()) {
                continue;
            }
            int cfd = ::accept(_listenFd, nullptr, nullptr);
            if (cfd < 0) {
                continue;
            }
            _workers.emplace_back([this, cfd]() {
                char buf[1024];
                (void)::recv(cfd, buf, sizeof(buf), 0);
                {
                    std::unique_lock<std::mutex> lock(_mu);
                    ++_inFlight;
                    _cv.notify_all();
                    _cv.wait_for(lock, std::chrono::milliseconds(2500),
                                 [&]() { return _release || _stop.load(); });
                }
                const char resp[] =
                    "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK";
                (void)::send(cfd, resp, sizeof(resp) - 1, MSG_NOSIGNAL);
                ::close(cfd);
            });
        }
    }

    int _listenFd;
    int _port;
    std::atomic<bool> _stop;
    std::mutex _mu;
    std::condition_variable _cv;
    bool _release;
    int _inFlight;
    std::thread _acceptThread;
    std::vector<std::thread> _workers;
};

} // namespace

TEST(OctiHttpServerIntegrationTest, Envelope3AvatarSandboxAndSsrfTlsProbeGate)
{
    HttpServerConfig pCfg = testConfig();
    pCfg.maxConcurrentUrlProbes = 4;
    pCfg.probeRateLimitMax = 50;
    startEnv(pCfg, true, nullptr);

    std::string avDir = env->dir + "/avatars_e3";
    env->store.setAvatarDir(avDir);

    User alice = signIn("google", "sub-e3-alice", "Envelope3 Alice");

    /* 1. SEC-05: SVG data URI upload rejected with 400 INVALID_AVATAR_URL */
    std::string svgUri =
        "data:image/svg+xml;base64," +
        e3StdBase64Encode("<svg xmlns='http://www.w3.org/2000/svg' onload='alert(1)'/>");
    EXPECT_ERROR(400, "INVALID_AVATAR_URL",
                 http().patch("/api/v1/profile/me", alice.token,
                              Json{{"avatar_url", svgUri}}));

    /* Fake PNG wrapping <script> rejected 400 INVALID_AVATAR_URL */
    std::string scriptAsPng =
        "data:image/png;base64," +
        e3StdBase64Encode("<script>fetch('/api/v1/profile/me')</script>");
    EXPECT_ERROR(400, "INVALID_AVATAR_URL",
                 http().patch("/api/v1/profile/me", alice.token,
                              Json{{"avatar_url", scriptAsPng}}));

    /* Oversized 1025x1024 PNG & 16383x16383 WebP bomb rejected 400 INVALID_AVATAR_URL */
    {
        std::string png1025("\x89PNG\r\n\x1a\n\x00\x00\x00\x0dIHDR"
                            "\x00\x00\x04\x01\x00\x00\x04\x00"
                            "\x08\x06\x00\x00\x00\x00\x00\x00\x00"
                            "\x00\x00\x00\x00IEND\xae\x42\x60\x82", 45);
        EXPECT_ERROR(400, "INVALID_AVATAR_URL",
                     http().patch("/api/v1/profile/me", alice.token,
                                  Json{{"avatar_url", "data:image/png;base64," + e3StdBase64Encode(png1025)}}));

        const uint32_t packed16383 = 16382u | (16382u << 14);
        unsigned char webpBombRaw[28] = {
            'R', 'I', 'F', 'F', 20, 0, 0, 0, 'W', 'E', 'B', 'P',
            'V', 'P', '8', 'L', 8, 0, 0, 0, 0x2F,
            static_cast<unsigned char>(packed16383 & 0xFFu),
            static_cast<unsigned char>((packed16383 >> 8) & 0xFFu),
            static_cast<unsigned char>((packed16383 >> 16) & 0xFFu),
            static_cast<unsigned char>((packed16383 >> 24) & 0xFFu),
            0, 0, 0
        };
        std::string webpBombStr(reinterpret_cast<const char *>(webpBombRaw), sizeof(webpBombRaw));
        EXPECT_ERROR(400, "INVALID_AVATAR_URL",
                     http().patch("/api/v1/profile/me", alice.token,
                                  Json{{"avatar_url", "data:image/webp;base64," + e3StdBase64Encode(webpBombStr)}}));
    }

    /* Nominal 1x1 GIF & 1x1 JPEG uploads accepted and served with CSP sandbox */
    {
        std::string gif1x1 =
            "data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7";
        Reply rOkGif = http().patch("/api/v1/profile/me", alice.token,
                                    Json{{"avatar_url", gif1x1}});
        EXPECT_STATUS(200, rOkGif);
        std::string gifUrl = rOkGif.json.value("avatar_url", "");
        std::string gifPath = gifUrl.substr(0, gifUrl.find('?'));
        Reply rGetGif = http().get(gifPath);
        EXPECT_STATUS(200, rGetGif);
        STRCMP_EQUAL("image/gif", rGetGif.contentType.c_str());
        STRCMP_EQUAL("sandbox; default-src 'none'",
                     rGetGif.header("Content-Security-Policy").c_str());

        std::string jpg1x1Raw(
            "\xff\xd8\xff\xe0\x00\x10JFIF\x00\x01\x01\x00\x00\x01\x00\x01\x00\x00"
            "\xff\xc0\x00\x0b\x08\x00\x01\x00\x01\x01\x01\x11\x00\xff\xd9", 35);
        Reply rOkJpg = http().patch(
            "/api/v1/profile/me", alice.token,
            Json{{"avatar_url", "data:image/jpeg;base64," + e3StdBase64Encode(jpg1x1Raw)}});
        EXPECT_STATUS(200, rOkJpg);
        std::string jpgUrl = rOkJpg.json.value("avatar_url", "");
        std::string jpgPath = jpgUrl.substr(0, jpgUrl.find('?'));
        Reply rGetJpg = http().get(jpgPath);
        EXPECT_STATUS(200, rGetJpg);
        STRCMP_EQUAL("image/jpeg", rGetJpg.contentType.c_str());
        STRCMP_EQUAL("sandbox; default-src 'none'",
                     rGetJpg.header("Content-Security-Policy").c_str());
    }

    /* Valid 1x1 PNG accepted and served with CSP sandbox headers on both routes */
    std::string png1x1 =
        "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";
    Reply rOkPng = http().patch("/api/v1/profile/me", alice.token,
                                Json{{"avatar_url", png1x1}});
    EXPECT_STATUS(200, rOkPng);
    std::string avUrl = rOkPng.json.value("avatar_url", "");
    size_t qPos = avUrl.find('?');
    std::string avPath = (qPos != std::string::npos) ? avUrl.substr(0, qPos) : avUrl;
    std::string avFile = avPath.substr(std::string("/api/v1/avatars/").size());

    Reply rAv1 = http().get(avPath);
    EXPECT_STATUS(200, rAv1);
    STRCMP_EQUAL("sandbox; default-src 'none'",
                 rAv1.header("Content-Security-Policy").c_str());
    STRCMP_EQUAL("nosniff",
                 rAv1.header("X-Content-Type-Options").c_str());
    STRCMP_EQUAL("", rAv1.header("Content-Disposition").c_str());

    Reply rAvAlias = http().get("/avatar/" + avFile);
    EXPECT_STATUS(200, rAvAlias);
    STRCMP_EQUAL("sandbox; default-src 'none'",
                 rAvAlias.header("Content-Security-Policy").c_str());

    /* Legacy .svg file on disk served with Content-Security-Policy sandbox + Content-Disposition: attachment */
    {
        std::ofstream legacySvg((avDir + "/88888.svg").c_str(),
                                std::ios::binary | std::ios::trunc);
        legacySvg << "<svg xmlns=\"http://www.w3.org/2000/svg\"></svg>";
    }
    Reply rLegacySvg1 = http().get("/api/v1/avatars/88888.svg");
    EXPECT_STATUS(200, rLegacySvg1);
    STRCMP_EQUAL("sandbox; default-src 'none'",
                 rLegacySvg1.header("Content-Security-Policy").c_str());
    STRCMP_EQUAL("attachment",
                 rLegacySvg1.header("Content-Disposition").c_str());

    Reply rLegacySvg2 = http().get("/avatar/88888.svg");
    EXPECT_STATUS(200, rLegacySvg2);
    STRCMP_EQUAL("sandbox; default-src 'none'",
                 rLegacySvg2.header("Content-Security-Policy").c_str());
    STRCMP_EQUAL("attachment",
                 rLegacySvg2.header("Content-Disposition").c_str());

    /* 2. SEC-08: CGNAT 100.64.0.1 and multicast 224.0.0.1 rejected immediately as disallowed IPs */
    auto t0 = std::chrono::steady_clock::now();
    Reply rCgnat = http().post(
        "/api/v1/robots", alice.token,
        Json{{"robot_name", "e3-cgnat-bot"},
             {"robot_url", "http://100.64.0.1/bot"}});
    auto cgnatMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - t0).count();
    EXPECT_ERROR(400, "INVALID_ROBOT_URL", rCgnat);
    CHECK_TRUE(cgnatMs < 1500);

    Reply rMcast = http().post(
        "/api/v1/robots", alice.token,
        Json{{"robot_name", "e3-mcast-bot"},
             {"robot_url", "http://224.0.0.1/bot"}});
    EXPECT_ERROR(400, "INVALID_ROBOT_URL", rMcast);

    const char *ssrfUrls[] = {
        "http://[::ffff:127.0.0.1]/bot",
        "http://[::ffff:169.254.169.254]/latest/meta-data",
        "http://[64:ff9b::7f00:1]/bot",
        "http://[2002:7f00:0001::1]/bot"
    };
    for (size_t i = 0; i < sizeof(ssrfUrls) / sizeof(ssrfUrls[0]); ++i) {
        EXPECT_ERROR(400, "INVALID_ROBOT_URL",
                     http().post("/api/v1/robots", alice.token,
                                 Json{{"robot_name", "e3-ssrf-" + std::to_string(i)},
                                      {"robot_url", ssrfUrls[i]}}));
    }

    /* 3. SEC-08: Untrusted self-signed TLS certificate rejected by SSL_VERIFY_PEER */
    {
        ScopedLoopbackProbeEnv allowLoop(true);
        LocalSelfSignedTlsFixture tlsSrv("DNS:localhost,IP:127.0.0.1");
        CHECK_TRUE(tlsSrv.port() > 0);
        Reply rSelfSigned = http().post(
            "/api/v1/robots", alice.token,
            Json{{"robot_name", "e3-tls-selfsigned"},
                 {"robot_url", "https://localhost:" + std::to_string(tlsSrv.port()) + "/"}});
        EXPECT_ERROR(400, "INVALID_ROBOT_URL", rSelfSigned);
    }

    /* 3b. SEC-08: HEAD->405->GET fallback, >8 KiB header cap, and 302 redirect-to-bogon block */
    {
        ScopedLoopbackProbeEnv allowLoop(true);
        int rawSrvFd = ::socket(AF_INET, SOCK_STREAM, 0);
        CHECK_TRUE(rawSrvFd >= 0);
        int opt = 1;
        ::setsockopt(rawSrvFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        struct sockaddr_in sin;
        std::memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        sin.sin_port = 0;
        CHECK_EQUAL(0, ::bind(rawSrvFd, reinterpret_cast<struct sockaddr *>(&sin), sizeof(sin)));
        CHECK_EQUAL(0, ::listen(rawSrvFd, 8));
        socklen_t slen = sizeof(sin);
        CHECK_EQUAL(0, ::getsockname(rawSrvFd, reinterpret_cast<struct sockaddr *>(&sin), &slen));
        const int rawPort = ntohs(sin.sin_port);

        std::atomic<bool> rawStop(false);
        std::thread rawSrvThread([&]() {
            while (!rawStop.load()) {
                struct pollfd pfd;
                pfd.fd = rawSrvFd;
                pfd.events = POLLIN;
                if (::poll(&pfd, 1, 50) <= 0 || rawStop.load()) {
                    continue;
                }
                int cfd = ::accept(rawSrvFd, nullptr, nullptr);
                if (cfd < 0) {
                    continue;
                }
                char reqBuf[1024];
                ssize_t nr = ::recv(cfd, reqBuf, sizeof(reqBuf) - 1, 0);
                std::string rq = (nr > 0) ? std::string(reqBuf, static_cast<size_t>(nr)) : std::string();
                std::string resp;
                if (rq.find("/head405 ") != std::string::npos) {
                    if (rq.rfind("HEAD ", 0) == 0) {
                        resp = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
                    } else {
                        resp = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\nOK";
                    }
                } else if (rq.find("/redir-bogon ") != std::string::npos) {
                    resp = "HTTP/1.1 302 Found\r\nLocation: http://100.64.0.1/internal\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
                } else if (rq.find("/huge-hdr ") != std::string::npos) {
                    resp = "HTTP/1.1 200 OK\r\nX-Big: " + std::string(9000, 'H') + "\r\n\r\n";
                } else {
                    resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
                }
                (void)::send(cfd, resp.data(), resp.size(), MSG_NOSIGNAL);
                ::close(cfd);
            }
        });

        Reply rHeadFallback = http().post(
            "/api/v1/robots", alice.token,
            Json{{"robot_name", "e3-head405-ok"},
                 {"robot_url", "http://127.0.0.1:" + std::to_string(rawPort) + "/head405"}});
        EXPECT_STATUS(201, rHeadFallback);

        Reply rRedirBogon = http().post(
            "/api/v1/robots", alice.token,
            Json{{"robot_name", "e3-redir-bogon"},
                 {"robot_url", "http://127.0.0.1:" + std::to_string(rawPort) + "/redir-bogon"}});
        EXPECT_ERROR(400, "INVALID_ROBOT_URL", rRedirBogon);

        Reply rHugeHdr = http().post(
            "/api/v1/robots", alice.token,
            Json{{"robot_name", "e3-huge-hdr"},
                 {"robot_url", "http://127.0.0.1:" + std::to_string(rawPort) + "/huge-hdr"}});
        EXPECT_ERROR(400, "INVALID_ROBOT_URL", rHugeHdr);

        /* Also verify PATCH /api/v1/robots/:id rejects redirect-to-bogon */
        std::string createdUuid = rHeadFallback.json.value("robot_uuid", "");
        EXPECT_ERROR(400, "INVALID_ROBOT_URL",
                     http().patch("/api/v1/robots/" + createdUuid, alice.token,
                                  Json{{"robot_url", "http://127.0.0.1:" + std::to_string(rawPort) + "/redir-bogon"}}));

        rawStop.store(true);
        ::shutdown(rawSrvFd, SHUT_RDWR);
        ::close(rawSrvFd);
        rawSrvThread.join();
    }

    /* 4. SEC-08: maxConcurrentUrlProbes = 4 exact boundary (4 allowed, 5th returns 503 SERVICE_BUSY) */
    {
        ScopedLoopbackProbeEnv allowLoop(true);
        LocalBarrierHttpProbeFixture slowHttp;
        CHECK_TRUE(slowHttp.port() > 0);
        const std::string slowUrl =
            "http://127.0.0.1:" + std::to_string(slowHttp.port()) + "/bot";

        std::vector<User> probeUsers;
        for (int i = 0; i < 5; ++i) {
            probeUsers.push_back(signIn("google",
                                        "sub-e3-probe-" + std::to_string(i),
                                        "ProbeUser" + std::to_string(i)));
        }

        std::vector<Reply> fourReplies(4);
        std::vector<std::thread> fourThreads;
        for (int i = 0; i < 4; ++i) {
            fourThreads.emplace_back([&, i]() {
                auto c = http();
                fourReplies[i] = c.post(
                    "/api/v1/robots", probeUsers[i].token,
                    Json{{"robot_name", "e3-conc-bot-" + std::to_string(i)},
                         {"robot_url", slowUrl}});
            });
        }

        CHECK_TRUE(slowHttp.waitForInFlight(4, 2000));

        Reply fifthReply = http().post(
            "/api/v1/robots", probeUsers[4].token,
            Json{{"robot_name", "e3-conc-bot-4"},
                 {"robot_url", slowUrl}});

        slowHttp.releaseAll();
        for (size_t i = 0; i < fourThreads.size(); ++i) {
            fourThreads[i].join();
        }

        EXPECT_ERROR(503, "SERVICE_BUSY", fifthReply);
        for (int i = 0; i < 4; ++i) {
            EXPECT_STATUS(201, fourReplies[i]);
        }
    }
}

namespace {

bool gunzipToString(const std::string &compressed, std::string &out)
{
    out.clear();
    if (compressed.size() < 10 ||
        static_cast<unsigned char>(compressed[0]) != 0x1f ||
        static_cast<unsigned char>(compressed[1]) != 0x8b) {
        return false;
    }
    z_stream zs;
    std::memset(&zs, 0, sizeof(zs));
    if (inflateInit2(&zs, 15 + 16) != Z_OK) {
        return false;
    }
    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(compressed.data()));
    zs.avail_in = static_cast<uInt>(compressed.size());
    char buf[16384];
    int ret = Z_OK;
    while (ret != Z_STREAM_END) {
        zs.next_out = reinterpret_cast<Bytef *>(buf);
        zs.avail_out = sizeof(buf);
        ret = inflate(&zs, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&zs);
            return false;
        }
        size_t have = sizeof(buf) - zs.avail_out;
        out.append(buf, have);
    }
    inflateEnd(&zs);
    return true;
}

} /* namespace */

TEST(OctiHttpServerIntegrationTest, Envelope4StartupGzipSnapshotEtagAndExportGate)
{
    /* 1. Anonymous archive export must fail with 401 UNAUTHORIZED */
    EXPECT_ERROR(401, "UNAUTHORIZED", http().get("/api/v1/archive/export"));

    /* 2. Startup-precomputed GET / gzip + ETag + 304 Not Modified */
    httplib::Headers gzHeaders = {{"Accept-Encoding", "gzip, deflate"}};
    Reply rIdxGz = http().getRaw("/", "", gzHeaders);
    EXPECT_STATUS(200, rIdxGz);
    STRCMP_EQUAL("gzip", rIdxGz.header("Content-Encoding").c_str());
    CHECK_TRUE(rIdxGz.header("Vary").find("Accept-Encoding") != std::string::npos);
    std::string idxEtag = rIdxGz.header("ETag");
    CHECK_FALSE(idxEtag.empty());
    const std::string expectedHtml(webUiHtml());
    CHECK_TRUE(rIdxGz.body.size() < expectedHtml.size() * 35 / 100);
    std::string idxUnpacked;
    CHECK_TRUE(gunzipToString(rIdxGz.body, idxUnpacked));
    CHECK_TRUE(idxUnpacked == expectedHtml);

    Reply rIdx304 = http().getRaw("/", "",
                                  {{"Accept-Encoding", "gzip"},
                                   {"If-None-Match", idxEtag}});
    EXPECT_STATUS(304, rIdx304);
    CHECK_TRUE(rIdx304.body.empty());

    /* 3. < 1024 B dynamic JSON stays uncompressed; >= 1024 B uses gzip + 304 */
    Reply rSmall = http().getRaw("/api/v1/tables", "", gzHeaders);
    EXPECT_STATUS(200, rSmall);
    CHECK_TRUE(rSmall.body.size() < 1024);
    STRCMP_EQUAL("", rSmall.header("Content-Encoding").c_str());
    std::string smallEtag = rSmall.header("ETag");
    CHECK_FALSE(smallEtag.empty());
    EXPECT_STATUS(304, http().getRaw("/api/v1/tables", "",
                                     {{"If-None-Match", smallEtag}}));

    /* Weak ETag matches 304; malformed If-None-Match falls back to 200; q=0 skips gzip */
    EXPECT_STATUS(304, http().getRaw("/", "",
                                     {{"If-None-Match", "W/" + idxEtag}}));
    Reply rBadInm = http().getRaw("/", "",
                                  {{"If-None-Match", "unquoted-malformed-etag"}});
    EXPECT_STATUS(200, rBadInm);
    Reply rNoGz = http().getRaw("/", "", {{"Accept-Encoding", "gzip;q=0"}});
    EXPECT_STATUS(200, rNoGz);
    STRCMP_EQUAL("", rNoGz.header("Content-Encoding").c_str());
    CHECK_TRUE(rNoGz.body == expectedHtml);
    CHECK_TRUE(rNoGz.body.find("<button type=\"button\" class=\"btn\" id=\"ar-export-btn\">") !=
               std::string::npos);
    CHECK_TRUE(rNoGz.header("Content-Security-Policy").find("object-src 'none'") !=
               std::string::npos);
    CHECK_TRUE(rNoGz.header("Content-Security-Policy").find("frame-ancestors 'none'") !=
               std::string::npos);

    /* Plain HTTP omits HSTS; trusted X-Forwarded-Proto: https emits HSTS */
    STRCMP_EQUAL("", rNoGz.header("Strict-Transport-Security").c_str());
    Reply rHttpsProxy = http().getRaw("/", "", {{"X-Forwarded-Proto", "https"}});
    EXPECT_STATUS(200, rHttpsProxy);
    CHECK_TRUE(rHttpsProxy.header("Strict-Transport-Security")
                   .find("max-age=31536000") != std::string::npos);
}

TEST(OctiHttpServerIntegrationTest,
     Envelope4SnapshotInvalidationLongPollKeysetExportAndRetention)
{
    User alice = signIn("google", "sub-e4-alice", "Alice E4");
    User bob = signIn("apple", "sub-e4-bob", "Bob E4");

    /* 1. Snapshot ETag -> 304, >= 1024 B Z_BEST_SPEED gzip, and immediate invalidation */
    httplib::Headers gzHdr = {{"Accept-Encoding", "gzip"}};
    Reply rTbl0 = http().getRaw("/api/v1/tables", "", gzHdr);
    EXPECT_STATUS(200, rTbl0);
    std::string etagTbl0 = rTbl0.header("ETag");
    CHECK_FALSE(etagTbl0.empty());

    std::string t1 = createTable(alice, Json{{"mode", "2P"}});
    std::string t2 = createTable(bob, Json{{"mode", "2P"}, {"bases", 1}});
    CHECK_FALSE(t1.empty());
    CHECK_FALSE(t2.empty());

    /* Old ETag must NOT return 304 after table creation; >= 1024 B payload is gzipped */
    Reply rTbl1 = http().getRaw(
        "/api/v1/tables", "",
        {{"Accept-Encoding", "gzip"}, {"If-None-Match", etagTbl0}});
    EXPECT_STATUS(200, rTbl1);
    STRCMP_EQUAL("gzip", rTbl1.header("Content-Encoding").c_str());
    std::string tbl1JsonStr;
    CHECK_TRUE(gunzipToString(rTbl1.body, tbl1JsonStr));
    CHECK_TRUE(tbl1JsonStr.size() >= 1024);
    Json tbl1Json = Json::parse(tbl1JsonStr);
    CHECK_EQUAL(2, static_cast<int>(tbl1Json["tables"].size()));
    std::string etagTbl1 = rTbl1.header("ETag");
    CHECK_TRUE(etagTbl1 != etagTbl0);
    EXPECT_STATUS(304, http().getRaw("/api/v1/tables", "",
                                     {{"If-None-Match", etagTbl1}}));

    /* Passive non-SSE polls must not allocate anonymous observer entries */
    size_t anonBefore = env->rooms->anonymousObserverCount();
    EXPECT_STATUS(200, http().get("/api/v1/lobby/events?since_seq=0", "",
                                  {{"X-Forwarded-For", "198.51.100.201"}}));
    EXPECT_STATUS(200, http().get("/api/v1/games/" + t1, "",
                                  {{"X-Forwarded-For", "198.51.100.202"}}));
    EXPECT_STATUS(200, http().get("/api/v1/games/" + t1 + "/events?since_seq=0", "",
                                  {{"X-Forwarded-For", "198.51.100.203"}}));
    CHECK_EQUAL(anonBefore, env->rooms->anonymousObserverCount());

    EXPECT_STATUS(200, http().post("/api/v1/tables/" + t1 + "/leave",
                                   alice.token, Json::object()));
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + t2 + "/leave",
                                   bob.token, Json::object()));

    /* /lobby/state, /robots/directory, /leaderboard ETag -> 304 + rating mutation invalidation */
    Reply rLs = http().getRaw("/api/v1/lobby/state", alice.token, gzHdr);
    EXPECT_STATUS(200, rLs);
    EXPECT_STATUS(304, http().getRaw("/api/v1/lobby/state", alice.token,
                                     {{"If-None-Match", rLs.header("ETag")}}));

    Reply rLb0 = http().getRaw("/api/v1/leaderboard?mode=2P_3B", "");
    EXPECT_STATUS(200, rLb0);
    std::string etagLb0 = rLb0.header("ETag");
    EXPECT_STATUS(304, http().getRaw("/api/v1/leaderboard?mode=2P_3B", "",
                                     {{"If-None-Match", etagLb0}}));

    Reply rDir0 = http().getRaw("/api/v1/robots/directory", "");
    EXPECT_STATUS(200, rDir0);
    STRCMP_EQUAL("no-store", rDir0.header("Cache-Control").c_str());
    std::string etagDir0 = rDir0.header("ETag");
    EXPECT_STATUS(304, http().getRaw("/api/v1/robots/directory", "",
                                     {{"If-None-Match", etagDir0}}));

    /* 2. wait_ms long-polling wakes < 250 ms on lobby event and room move */
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/lobby/events?since_seq=0&wait_ms=bad"));

    Reply rCurEv = http().get("/api/v1/lobby/events?since_seq=999999");
    EXPECT_STATUS(200, rCurEv);
    int64_t curLobbySeq = rCurEv.json.value("latest_seq", 0LL);
    std::string evEtag = rCurEv.header("ETag");
    CHECK_FALSE(evEtag.empty());
    EXPECT_STATUS(304, http().getRaw(
                           "/api/v1/lobby/events?since_seq=" +
                               std::to_string(curLobbySeq),
                           "", {{"If-None-Match", evEtag}}));

    Reply pollReply;
    int64_t pollElapsedMs = 0;
    std::thread waiter([&]() {
        auto t0 = std::chrono::steady_clock::now();
        pollReply = http().get("/api/v1/lobby/events?since_seq=" +
                                   std::to_string(curLobbySeq) +
                                   "&wait_ms=4000",
                               bob.token);
        auto t1 = std::chrono::steady_clock::now();
        pollElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                            t1 - t0)
                            .count();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    auto postT0 = std::chrono::steady_clock::now();
    EXPECT_STATUS(200, http().post("/api/v1/lobby/chat", alice.token,
                                   Json{{"text", "wake long poll now"}}));
    waiter.join();
    auto postElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - postT0)
                             .count();
    EXPECT_STATUS(200, pollReply);
    CHECK_TRUE(pollReply.json["events"].size() >= 1u);
    CHECK_TRUE(postElapsedMs < 250);
    CHECK_TRUE(pollElapsedMs < 1000);

    /* 3. Play rated online game (>= 8 plies + resign) + upload client_upload games */
    std::string onlineRoom = startGame({alice, bob}, Json{{"mode", "2P"}, {"rated", true}});
    playPlies(onlineRoom, {alice, bob}, GAME_MODE_2, 8);
    EXPECT_STATUS(200, http().post("/api/v1/games/" + onlineRoom + "/resign",
                                   bob.token, Json::object()));

    /* Leaderboard ETag must invalidate immediately after rated online game */
    Reply rLb1 = http().getRaw("/api/v1/leaderboard?mode=2P_3B", "",
                               {{"If-None-Match", etagLb0}});
    EXPECT_STATUS(200, rLb1);
    CHECK_TRUE(rLb1.header("ETag") != etagLb0);

    /* /archive/stats and /archive/games/:id/plies ETag -> 304 + upload invalidation */
    Reply rStats0 = http().getRaw("/api/v1/archive/stats", "");
    EXPECT_STATUS(200, rStats0);
    std::string etagStats0 = rStats0.header("ETag");
    EXPECT_STATUS(304, http().getRaw("/api/v1/archive/stats", "",
                                     {{"If-None-Match", etagStats0}}));

    Reply rPlies0 = http().getRaw("/api/v1/archive/games/" + onlineRoom + "/plies", "");
    EXPECT_STATUS(200, rPlies0);
    EXPECT_STATUS(304, http().getRaw(
                           "/api/v1/archive/games/" + onlineRoom + "/plies", "",
                           {{"If-None-Match", rPlies0.header("ETag")}}));

    const std::vector<std::vector<std::string>> distinctMoveSeqs = {
        {"33+a", "37+e"},
        {"53+a", "57+e"},
        {"73+a", "77+e"}
    };
    for (int i = 0; i < 3; ++i) {
        std::string upTr = makeResignedTranscript(
            distinctMoveSeqs[static_cast<size_t>(i)],
            "inst-e4-exp-" + std::to_string(i), 3 + (i % 3), "linux-x86_64");
        Json jTr = Json::parse(upTr);
        jTr["client"]["extended"]["startedAt"] = 1750000000 + i * 10;
        jTr["client"]["extended"]["endedAt"] = 1750000100 + i * 10;
        EXPECT_STATUS(200, http().post("/api/v1/archive/upload", "",
                                       Json{{"transcript", jTr.dump()}}));
    }

    /* /archive/stats ETag invalidated after archive uploads */
    Reply rStats1 = http().getRaw("/api/v1/archive/stats", "",
                                  {{"If-None-Match", etagStats0}});
    EXPECT_STATUS(200, rStats1);

    /* 4. Authenticated keyset /api/v1/archive/export (human + robot token + gzip + cursor) */
    Reply rBot = http().post("/api/v1/robots", alice.token,
                             Json{{"robot_name", "e4_export_bot"}});
    EXPECT_STATUS(201, rBot);
    std::string botTok = rBot.json.value("robot_token", "");
    CHECK_FALSE(botTok.empty());

    /* Robot Bearer token succeeds */
    Reply rBotExp = http().get("/api/v1/archive/export?limit=2", botTok);
    EXPECT_STATUS(200, rBotExp);
    CHECK_FALSE(rBotExp.header("X-Octi-Next-Cursor").empty());

    /* Boundary & fault checks on offset / cursor */
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/archive/export?offset=5001", alice.token));
    EXPECT_STATUS(200,
                  http().get("/api/v1/archive/export?offset=5000&limit=5", alice.token));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/archive/export?cursor=malformed", alice.token));
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().get("/api/v1/archive/export?after_game_id=only_id", alice.token));

    /* Walk entire 4-game corpus (1 online + 3 client_upload) with limit=2 via X-Octi-Next-Cursor */
    std::vector<std::string> exportedIds;
    std::set<std::string> seenSources;
    std::string cursorParam;
    for (int pageIdx = 0; pageIdx < 10; ++pageIdx) {
        std::string url = "/api/v1/archive/export?limit=2" + cursorParam;
        Reply rPage = http().get(url, alice.token);
        EXPECT_STATUS(200, rPage);
        int rowsInPage = 0;
        std::stringstream ss(rPage.body);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.empty()) {
                continue;
            }
            ++rowsInPage;
            CHECK_TRUE(line.find('\r') == std::string::npos);
            CHECK_TRUE(line.find("inst-e4-exp-") == std::string::npos);
            CHECK_TRUE(line.find("\"extended\"") == std::string::npos);
            Json row = Json::parse(line);
            STRCMP_EQUAL("octi.transcript.v3", row.value("format", "").c_str());
            STRCMP_EQUAL("CC-BY-4.0", row.value("license", "").c_str());
            exportedIds.push_back(
                row.value("gameId", row.value("game_id", "")));
            seenSources.insert(row.value("source", ""));
        }
        std::string nextCur = rPage.header("X-Octi-Next-Cursor");
        if (rowsInPage < 2 || nextCur.empty()) {
            break;
        }
        cursorParam = "&cursor=" + nextCur;
    }
    CHECK_EQUAL(4, static_cast<int>(exportedIds.size()));
    CHECK_EQUAL(4, static_cast<int>(std::set<std::string>(
                                        exportedIds.begin(), exportedIds.end())
                                        .size()));
    CHECK_TRUE(seenSources.count("ONLINE") == 1);

    /* Gzip streaming on /api/v1/archive/export (>= 1024 B) matches uncompressed byte-for-byte */
    Reply rExpPlain = http().get("/api/v1/archive/export?limit=10", alice.token);
    EXPECT_STATUS(200, rExpPlain);
    CHECK_TRUE(rExpPlain.body.size() >= 1024);
    Reply rExpGzip = http().getRaw("/api/v1/archive/export?limit=10", alice.token, gzHdr);
    EXPECT_STATUS(200, rExpGzip);
    STRCMP_EQUAL("gzip", rExpGzip.header("Content-Encoding").c_str());
    std::string expGunzipped;
    CHECK_TRUE(gunzipToString(rExpGzip.body, expGunzipped));
    CHECK_TRUE(expGunzipped == rExpPlain.body);

    /* 5. Concurrent export cap (maxConcurrentExports = 2 -> 3rd gets 503 + Retry-After) & tick retention */
    {
        HttpServerConfig cfg = testConfig();
        cfg.maxConcurrentExports = 2;
        cfg.exportBatchDelayMsForTesting = 250;
        cfg.tickIntervalMs = 20;
        cfg.retentionPurgeIntervalSec = 1;
        cfg.retentionMaxRows = 2;
        startEnv(cfg, true, nullptr);

        User u1 = signIn("google", "sub-e4-c1", "Conc1");
        User u2 = signIn("apple", "sub-e4-c2", "Conc2");
        User u3 = signIn("microsoft", "sub-e4-c3", "Conc3");
        std::string tr = makeResignedTranscript(
            {"33+a", "37+e"}, "inst-e4-conc", 3, "linux-x86_64");
        EXPECT_STATUS(200, http().post("/api/v1/archive/upload", "",
                                       Json{{"transcript", tr}}));

        auto fakeSec = std::make_shared<std::atomic<int64_t>>(realNow());
        env->store.setClock([fakeSec]() { return fakeSec->load(); });

        /* Seed 5 crash reports and 5 chat logs so tickLoop() prunes to retentionMaxRows = 2 */
        const int64_t seedSec = fakeSec->load();
        for (int i = 0; i < 5; ++i) {
            CrashReportRecord cr;
            cr.crashId = "tick-cr-" + std::to_string(i);
            cr.createdAt = seedSec - 10 + i;
            cr.platform = "linux";
            cr.appVersion = "2.6.3";
            cr.exceptionType = "SIGABRT";
            std::string err;
            env->store.recordCrashReport(cr, err);
            env->store.appendLobbyChatLog(
                static_cast<uint64_t>(i + 1), u1.uuid, u1.handle,
                "tick chat " + std::to_string(i), (seedSec + i) * 1000LL);
        }

        Reply expA;
        Reply expB;
        std::thread tA([&]() {
            expA = http().get("/api/v1/archive/export?limit=10", u1.token);
        });
        std::thread tB([&]() {
            expB = http().get("/api/v1/archive/export?limit=10", u2.token);
        });

        bool twoInFlight = false;
        for (int i = 0; i < 100 && !twoInFlight; ++i) {
            twoInFlight = (env->server->activeExports() >= 2);
            if (!twoInFlight) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
        CHECK_TRUE(twoInFlight);

        Reply expThird = http().get("/api/v1/archive/export?limit=10", u3.token);
        EXPECT_ERROR(503, "SERVICE_BUSY", expThird);
        STRCMP_EQUAL("5", expThird.header("Retry-After").c_str());

        tA.join();
        tB.join();
        EXPECT_STATUS(200, expA);
        EXPECT_STATUS(200, expB);

        /* Advance simulated epoch clock past retentionPurgeIntervalSec = 1 and wait 2 ticks */
        fakeSec->fetch_add(2);
        const uint64_t baseTicks = env->server->tickCount();
        for (int i = 0; i < 100 && env->server->tickCount() <= baseTicks + 1; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
        CrashReportFilter cf;
        cf.limit = 20;
        std::vector<CrashReportRecord> crashes;
        int64_t crashTotal = 0;
        CHECK_TRUE(env->store.listCrashReports(cf, crashes, &crashTotal));
        CHECK_EQUAL(2, static_cast<int>(crashTotal));

        std::vector<LobbyChatLogRow> chats;
        CHECK_TRUE(env->store.listLobbyChatLogs(chats, 20));
        CHECK_EQUAL(2, static_cast<int>(chats.size()));
    }
}

TEST(OctiHttpServerIntegrationTest, Envelope5AdminBansCrudCidrMoveSseAndXssGate)
{
    HttpServerConfig cfg = testConfig();
    cfg.trustedProxies = {"127.0.0.1", "::1", "::ffff:127.0.0.1", "192.0.2.254"};
    cfg.sseSessionRecheckMs = 50;
    cfg.ssePollMs = 20;
    startEnv(cfg, true, nullptr);

    const std::string banConfigPath = env->dir + "/bans.json";
    cfg.banConfigPath = banConfigPath;
    startEnv(cfg, true, nullptr);

    env->store.setAdminGoogleIdentities({"sub-e5-admin", "sub-e5-admin-two"});

    /* 1. WebUI HTML Ban Editor controls present inside #account-admin-controls */
    Reply rUi = http().getRaw("/");
    EXPECT_STATUS(200, rUi);
    CHECK_TRUE(rUi.body.find("id=\"account-admin-controls\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"admin-temp-bans-table\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"admin-perm-bans-table\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"admin-ban-type\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"admin-ban-target\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"admin-ban-duration\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"admin-ban-reason\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"btn-admin-ban-add\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"btn-admin-promote-perm\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"btn-admin-demote-24h\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"btn-admin-unban-temp\"") != std::string::npos);
    CHECK_TRUE(rUi.body.find("id=\"btn-admin-unban-perm\"") != std::string::npos);

    User admin = signIn("google", "sub-e5-admin", "AdminOne");
    User adminTwo = signIn("google", "sub-e5-admin-two", "AdminTwo");
    User mallory = signIn("apple", "sub-e5-mallory", "MalloryUser");
    User bystander = signIn("microsoft", "sub-e5-bystander", "BystanderUser");

    Reply rAdminBot = http().post("/api/v1/robots", admin.token,
                                  Json{{"robot_name", "e5_admin_owned_bot"}});
    EXPECT_STATUS(201, rAdminBot);
    Reply rMalloryBot = http().post("/api/v1/robots", mallory.token,
                                    Json{{"robot_name", "e5_mallory_bot"}});
    EXPECT_STATUS(201, rMalloryBot);
    const std::string malloryBotTok = rMalloryBot.json.value("robot_token", "");
    const std::string malloryBotUuid = rMalloryBot.json.value("robot_uuid", "");
    Reply rBystanderBot = http().post("/api/v1/robots", bystander.token,
                                      Json{{"robot_name", "e5_rogue_bot"}});
    EXPECT_STATUS(201, rBystanderBot);
    const std::string rogueBotTok = rBystanderBot.json.value("robot_token", "");
    const std::string rogueBotUuid = rBystanderBot.json.value("robot_uuid", "");

    /* 2. Authorization gate on /api/v1/admin/bans* */
    EXPECT_ERROR(401, "UNAUTHORIZED", http().get("/api/v1/admin/bans"));
    EXPECT_ERROR(403, "FORBIDDEN", http().get("/api/v1/admin/bans", bystander.token));
    EXPECT_ERROR(403, "FORBIDDEN", http().post("/api/v1/admin/bans", bystander.token,
                                               Json{{"target_type", "ip"},
                                                    {"target", "198.51.100.10"},
                                                    {"duration", "24h"}}));
    EXPECT_ERROR(403, "FORBIDDEN", http().get("/api/v1/admin/bans", rogueBotTok));

    Reply rList0 = http().get("/api/v1/admin/bans", admin.token);
    EXPECT_STATUS(200, rList0);
    STRCMP_EQUAL("no-store", rList0.header("Cache-Control").c_str());
    CHECK_TRUE(rList0.json.value("ok", false));
    STRCMP_EQUAL(banConfigPath.c_str(),
                 rList0.json.value("ban_config_path", "").c_str());
    CHECK_EQUAL(0, static_cast<int>(rList0.json["temp_bans"].size()));
    CHECK_EQUAL(0, static_cast<int>(rList0.json["permanent_bans"].size()));

    /* 3. Anti-lockout & CIDR boundary rejections */
    EXPECT_ERROR(400, "PROTECTED_TARGET",
                 http().post("/api/v1/admin/bans", admin.token,
                             Json{{"target_type", "ip"},
                                  {"target", "127.0.0.1"},
                                  {"duration", "24h"}}));
    EXPECT_ERROR(400, "PROTECTED_TARGET",
                 http().post("/api/v1/admin/bans", admin.token,
                             Json{{"target_type", "ip"},
                                  {"target", "127.0.0.0/8"},
                                  {"duration", "permanent"}}));
    EXPECT_ERROR(400, "PROTECTED_TARGET",
                 http().post("/api/v1/admin/bans", admin.token,
                             Json{{"target_type", "ip"},
                                  {"target", "::1"},
                                  {"duration", "24h"}}));
    EXPECT_ERROR(400, "PROTECTED_TARGET",
                 http().post("/api/v1/admin/bans", admin.token,
                             Json{{"target_type", "ip"},
                                  {"target", "192.0.2.254"},
                                  {"duration", "24h"}}));
    EXPECT_ERROR(400, "PROTECTED_TARGET",
                 http().post("/api/v1/admin/bans", admin.token,
                             Json{{"target_type", "ip"},
                                  {"target", "192.0.2.0/24"},
                                  {"duration", "24h"}}));
    /* Caller's current forwarded IP rejected with SELF_LOCKOUT_PREVENTED */
    EXPECT_ERROR(400, "SELF_LOCKOUT_PREVENTED",
                 http().post("/api/v1/admin/bans", admin.token,
                             Json{{"target_type", "ip"},
                                  {"target", "203.0.113.250/24"},
                                  {"duration", "24h"}},
                             {{"X-Forwarded-For", "203.0.113.250"}}));
    /* Self, other admin, and admin-owned robot rejected with SELF_LOCKOUT_PREVENTED */
    EXPECT_ERROR(400, "SELF_LOCKOUT_PREVENTED",
                 http().post("/api/v1/admin/bans", admin.token,
                             Json{{"target_type", "account"},
                                  {"target", "@" + admin.handle},
                                  {"duration", "24h"}}));
    EXPECT_ERROR(400, "SELF_LOCKOUT_PREVENTED",
                 http().post("/api/v1/admin/bans", admin.token,
                             Json{{"target_type", "account"},
                                  {"target", adminTwo.uuid},
                                  {"duration", "permanent"}}));
    EXPECT_ERROR(400, "SELF_LOCKOUT_PREVENTED",
                 http().post("/api/v1/admin/bans", admin.token,
                             Json{{"target_type", "robot"},
                                  {"target", "e5_admin_owned_bot"},
                                  {"duration", "24h"}}));

    /* CIDR prefix boundaries: /7 IPv4 & /15 IPv6 rejected; /8 IPv4 & /16 IPv6 accepted */
    EXPECT_ERROR(400, "INVALID_CIDR_PREFIX",
                 http().post("/api/v1/admin/bans", admin.token,
                             Json{{"target_type", "ip"},
                                  {"target", "0.0.0.0/0"},
                                  {"duration", "24h"}}));
    EXPECT_ERROR(400, "INVALID_CIDR_PREFIX",
                 http().post("/api/v1/admin/bans", admin.token,
                             Json{{"target_type", "ip"},
                                  {"target", "198.0.0.0/7"},
                                  {"duration", "24h"}}));
    EXPECT_ERROR(400, "INVALID_CIDR_PREFIX",
                 http().post("/api/v1/admin/bans", admin.token,
                             Json{{"target_type", "ip"},
                                  {"target", "::/0"},
                                  {"duration", "24h"}}));
    EXPECT_ERROR(400, "INVALID_CIDR_PREFIX",
                 http().post("/api/v1/admin/bans", admin.token,
                             Json{{"target_type", "ip"},
                                  {"target", "2000::/15"},
                                  {"duration", "24h"}}));
    EXPECT_ERROR(400, "INVALID_BAN_TARGET",
                 http().post("/api/v1/admin/bans", admin.token,
                             Json{{"target_type", "ip"},
                                  {"target", "not-a-valid-ip/24"},
                                  {"duration", "24h"}}));

    Reply rEdgeV4 = http().post("/api/v1/admin/bans", admin.token,
                                Json{{"target_type", "ip"},
                                     {"target", "203.0.0.1/8"},
                                     {"duration", "24h"},
                                     {"reason", "edge /8"}});
    EXPECT_STATUS(200, rEdgeV4);
    const std::string edgeV4Id = rEdgeV4.json["temp_bans"][0].value("ban_id", "");
    STRCMP_EQUAL("203.0.0.0/8",
                 rEdgeV4.json["temp_bans"][0].value("target_value", "").c_str());

    Reply rEdgeV6 = http().post("/api/v1/admin/bans", admin.token,
                                Json{{"target_type", "ip"},
                                     {"target", "2001:db8::1/16"},
                                     {"duration", "24h"},
                                     {"reason", "edge /16"}});
    EXPECT_STATUS(200, rEdgeV6);
    std::string edgeV6Id;
    for (const auto &tb : rEdgeV6.json["temp_bans"]) {
        if (tb.value("target_value", "") == "2001::/16") {
            edgeV6Id = tb.value("ban_id", "");
        }
    }
    CHECK_FALSE(edgeV6Id.empty());
    EXPECT_STATUS(200, http().post("/api/v1/admin/bans/remove", admin.token,
                                   Json{{"ban_ids", Json::array({edgeV4Id, edgeV6Id})}}));

    /* 4. Open SSE streams for mallory and IP 198.51.100.42 before banning */
    SseClient mallorySse;
    mallorySse.start(env->server->port(), "/api/v1/lobby/events?stream=1",
                     mallory.token, {{"X-Forwarded-For", "192.0.2.55"}});
    CHECK_TRUE(mallorySse.waitFor("retry:", 2000));

    SseClient ipSse;
    ipSse.start(env->server->port(), "/api/v1/lobby/events?stream=1",
                "", {{"X-Forwarded-For", "198.51.100.42"}});
    CHECK_TRUE(ipSse.waitFor("retry:", 2000));

    /* Add IPv4 CIDR (198.51.100.45/24 -> 198.51.100.0/24) as 24h with XSS payload reason */
    const std::string xssReason = "<img src=x onerror=alert(1)>";
    Reply rAddV4 = http().post("/api/v1/admin/bans", admin.token,
                               Json{{"target_type", "ip"},
                                    {"target", "198.51.100.45/24"},
                                    {"duration", "24h"},
                                    {"reason", xssReason}});
    EXPECT_STATUS(200, rAddV4);
    CHECK_EQUAL(1, static_cast<int>(rAddV4.json["temp_bans"].size()));
    STRCMP_EQUAL("198.51.100.0/24",
                 rAddV4.json["temp_bans"][0].value("target_value", "").c_str());
    STRCMP_EQUAL(xssReason.c_str(),
                 rAddV4.json["temp_bans"][0].value("reason", "").c_str());
    const std::string v4BanId = rAddV4.json["temp_bans"][0].value("ban_id", "");

    /* IP SSE stream receives event: closed and terminates promptly */
    CHECK_TRUE(ipSse.waitFinished(1500));
    CHECK_TRUE(ipSse.data().find("event: closed") != std::string::npos);
    CHECK_TRUE(ipSse.data().find("IP_BANNED") != std::string::npos);

    /* CIDR matching: 198.51.100.255 and ::ffff:198.51.100.42 blocked; 198.51.101.1 unblocked */
    Reply rBlockedV4 = http().get("/api/v1/tables", "",
                                  {{"X-Forwarded-For", "198.51.100.255"}});
    EXPECT_ERROR(403, "IP_BANNED", rBlockedV4);
    CHECK_FALSE(rBlockedV4.header("Retry-After").empty());

    EXPECT_ERROR(403, "IP_BANNED",
                 http().get("/api/v1/tables", "",
                            {{"X-Forwarded-For", "::ffff:198.51.100.42"}}));
    EXPECT_STATUS(200, http().get("/api/v1/tables", "",
                                  {{"X-Forwarded-For", "198.51.101.1"}}));
    /* /health, /api/v1/health, and /robots.txt remain reachable even from banned IP */
    EXPECT_STATUS(200, http().get("/health", "",
                                  {{"X-Forwarded-For", "198.51.100.42"}}));
    EXPECT_STATUS(200, http().get("/api/v1/health", "",
                                  {{"X-Forwarded-For", "198.51.100.42"}}));
    EXPECT_STATUS(200, http().get("/robots.txt", "",
                                  {{"X-Forwarded-For", "198.51.100.42"}}));

    /* Add IPv6 CIDR (2001:db8:abcd:12::1/48 -> 2001:db8:abcd::/48) to permanent list */
    Reply rAddV6 = http().post("/api/v1/admin/bans", admin.token,
                               Json{{"target_type", "ip"},
                                    {"target", "2001:db8:abcd:12::1/48"},
                                    {"duration", "permanent"},
                                    {"reason", "IPv6 subnet block"}});
    EXPECT_STATUS(200, rAddV6);
    CHECK_EQUAL(1, static_cast<int>(rAddV6.json["permanent_bans"].size()));
    STRCMP_EQUAL("2001:db8:abcd::/48",
                 rAddV6.json["permanent_bans"][0].value("target_value", "").c_str());
    EXPECT_ERROR(403, "IP_BANNED",
                 http().get("/api/v1/tables", "",
                            {{"X-Forwarded-For", "2001:db8:abcd:ffff::99"}}));
    EXPECT_STATUS(200, http().get("/api/v1/tables", "",
                                  {{"X-Forwarded-For", "2001:db8:abce::1"}}));

    /* Ban @handle account (mallory) -> active SSE closes, session & SSO & owned bot rejected 403 */
    Reply rBanAcct = http().post("/api/v1/admin/bans", admin.token,
                                 Json{{"target_type", "account"},
                                      {"target", "@" + mallory.handle},
                                      {"duration", "24h"},
                                      {"reason", "Account abuse"}});
    EXPECT_STATUS(200, rBanAcct);
    CHECK_TRUE(mallorySse.waitFinished(1500));
    CHECK_TRUE(mallorySse.data().find("event: closed") != std::string::npos);
    CHECK_TRUE(mallorySse.data().find("ACCOUNT_BANNED") != std::string::npos);

    EXPECT_ERROR(403, "ACCOUNT_BANNED",
                 http().get("/api/v1/profile/me", mallory.token));
    EXPECT_ERROR(403, "ACCOUNT_BANNED",
                 http().post("/api/v1/auth/sso", "",
                             Json{{"provider", "apple"},
                                  {"id_token", makeToken("apple", "sub-e5-mallory", "MalloryUser")}}));
    EXPECT_ERROR(403, "ACCOUNT_BANNED",
                 http().post("/api/v1/robots/connect", malloryBotTok, Json::object()));

    /* Ban standalone robot by name (e5_rogue_bot) while owner (bystander) stays unbanned */
    Reply rBanBot = http().post("/api/v1/admin/bans", admin.token,
                                Json{{"target_type", "robot"},
                                     {"target", "e5_rogue_bot"},
                                     {"duration", "permanent"},
                                     {"reason", "Rogue bot"}});
    EXPECT_STATUS(200, rBanBot);
    EXPECT_ERROR(403, "ACCOUNT_BANNED",
                 http().post("/api/v1/robots/connect", rogueBotTok, Json::object()));
    EXPECT_ERROR(403, "ACCOUNT_BANNED",
                 http().get("/api/v1/profile/me", rogueBotTok));
    EXPECT_STATUS(200, http().get("/api/v1/profile/me", bystander.token));

    /* 5. Move 24h -> permanent (verify 0640 bans.json & reload across restart) and permanent -> 24h */
    Reply rPromote = http().post("/api/v1/admin/bans/move", admin.token,
                                 Json{{"ban_ids", Json::array({v4BanId})},
                                      {"target_duration", "permanent"}});
    EXPECT_STATUS(200, rPromote);

    struct stat banSt;
    CHECK_EQUAL(0, ::stat(banConfigPath.c_str(), &banSt));
    LONGS_EQUAL(0640, static_cast<long>(banSt.st_mode & 0777));
    {
        std::ifstream bfs(banConfigPath, std::ios::binary);
        Json diskDoc = Json::parse(bfs, nullptr, false);
        CHECK_TRUE(diskDoc.is_object());
        LONGS_EQUAL(1, diskDoc.value("version", 0));
        CHECK_EQUAL(3, static_cast<int>(diskDoc["entries"].size()));
    }

    /* Verify XSS payload never appears unescaped in GET / */
    Reply rUiAfter = http().getRaw("/");
    EXPECT_STATUS(200, rUiAfter);
    CHECK_TRUE(rUiAfter.body.find(xssReason) == std::string::npos);

    /* Restart OctiHttpServer on the same DB + bans.json and verify both perm & 24h bans reload */
    env->server.reset();
    std::string startErr;
    env->server.reset(new OctiHttpServer(env->store, env->verifier, *env->rooms, cfg));
    CHECK_TRUE_TEXT(env->server->start(startErr), startErr.c_str());

    EXPECT_ERROR(403, "IP_BANNED",
                 http().get("/api/v1/tables", "",
                            {{"X-Forwarded-For", "198.51.100.42"}}));
    EXPECT_ERROR(403, "ACCOUNT_BANNED",
                 http().get("/api/v1/profile/me", mallory.token));
    EXPECT_ERROR(403, "ACCOUNT_BANNED",
                 http().post("/api/v1/robots/connect", rogueBotTok, Json::object()));

    /* Find promoted IPv4 ban id, demote permanent -> 24h via PATCH, then unban via DELETE */
    Reply rReloaded = http().get("/api/v1/admin/bans", admin.token);
    EXPECT_STATUS(200, rReloaded);
    std::string permV4Id;
    std::string tempMalloryId;
    for (const auto &pb : rReloaded.json["permanent_bans"]) {
        if (pb.value("target_value", "") == "198.51.100.0/24") {
            permV4Id = pb.value("ban_id", "");
        }
    }
    for (const auto &tb : rReloaded.json["temp_bans"]) {
        if (tb.value("target_value", "") == mallory.uuid) {
            tempMalloryId = tb.value("ban_id", "");
        }
    }
    CHECK_FALSE(permV4Id.empty());
    CHECK_FALSE(tempMalloryId.empty());

    Reply rDemote = http().patch("/api/v1/admin/bans/" + permV4Id, admin.token,
                                 Json{{"target_duration", "24h"}});
    EXPECT_STATUS(200, rDemote);
    {
        std::ifstream bfs(banConfigPath, std::ios::binary);
        std::string raw((std::istreambuf_iterator<char>(bfs)),
                        std::istreambuf_iterator<char>());
        CHECK_TRUE(raw.find("198.51.100.0/24") == std::string::npos);
    }
    std::string demotedV4Id;
    for (const auto &tb : rDemote.json["temp_bans"]) {
        if (tb.value("target_value", "") == "198.51.100.0/24") {
            demotedV4Id = tb.value("ban_id", "");
            CHECK_TRUE(tb.value("remaining_sec", 0LL) > 86000);
        }
    }
    CHECK_FALSE(demotedV4Id.empty());

    /* Unban IPv4 CIDR and mallory -> immediate restoration of access */
    EXPECT_STATUS(200, http().del("/api/v1/admin/bans/" + demotedV4Id, admin.token));
    EXPECT_STATUS(200, http().del("/api/v1/admin/bans/" + tempMalloryId, admin.token));

    EXPECT_STATUS(200, http().get("/api/v1/tables", "",
                                  {{"X-Forwarded-For", "198.51.100.42"}}));
    User malloryAgain = signIn("apple", "sub-e5-mallory", "MalloryUser");
    EXPECT_STATUS(200, http().get("/api/v1/profile/me", malloryAgain.token));
}

TEST(OctiHttpServerIntegrationTest,
     Envelope5AutoBan24hEscalationRestartPersistenceAndAntiLockout)
{
    HttpServerConfig cfg = testConfig();
    cfg.trustedProxies = {"127.0.0.1", "::1", "::ffff:127.0.0.1"};
    cfg.lobbySnapAnonIpBurst = 2.0;
    cfg.lobbySnapAnonIpRefillPerSec = 0.01;
    cfg.tickIntervalMs = 20;
    startEnv(cfg, true, nullptr);

    auto fakeSec = std::make_shared<std::atomic<int64_t>>(1763440000LL);
    env->store.setClock([fakeSec]() { return fakeSec->load(); });
    env->store.setAdminGoogleIdentities({"sub-e5-auto-admin"});
    User admin = signIn("google", "sub-e5-auto-admin", "AutoAdmin");

    /* 1. Sustained 429 flood from 203.0.113.77 escalates to mandatory 24h auto-ban (>= 40 429s in 60s) */
    int seen429 = 0;
    int seen403 = 0;
    for (int i = 0; i < 50; ++i) {
        Reply r = http().get("/api/v1/tables", "",
                             {{"X-Forwarded-For", "203.0.113.77"}});
        if (r.status == 429) {
            ++seen429;
        } else if (r.status == 403 && r.json.value("error", "") == "IP_BANNED") {
            ++seen403;
            break;
        }
    }
    CHECK_TRUE(seen429 >= 40);
    CHECK_TRUE(seen403 >= 1);

    Reply rBanned = http().get("/api/v1/lobby/state", "",
                               {{"X-Forwarded-For", "203.0.113.77"}});
    EXPECT_ERROR(403, "IP_BANNED", rBanned);
    STRCMP_EQUAL("24h", rBanned.json.value("ban_type", "").c_str());
    LONGS_EQUAL(1763440000LL + 86400LL, rBanned.json.value("expires_at", 0LL));
    CHECK_FALSE(rBanned.header("Retry-After").empty());

    /* Neighboring IP 203.0.113.78 and loopback 127.0.0.1 remain completely unaffected */
    EXPECT_STATUS(200, http().get("/api/v1/tables", "",
                                  {{"X-Forwarded-For", "203.0.113.78"}}));
    EXPECT_STATUS(200, http().get("/api/v1/tables", admin.token));

    /* Abuse flood from loopback (127.0.0.1) or admin never auto-bans loopback or admin */
    for (int i = 0; i < 55; ++i) {
        (void)http().get("/api/v1/tables");
    }
    Reply rLoopbackAfter = http().get("/api/v1/profile/me", admin.token);
    EXPECT_STATUS(200, rLoopbackAfter);

    /* 2. Restart OctiHttpServer on the same SQLite DB -> 24h auto-ban persists */
    env->server.reset();
    std::string startErr;
    env->server.reset(new OctiHttpServer(env->store, env->verifier, *env->rooms, cfg));
    CHECK_TRUE_TEXT(env->server->start(startErr), startErr.c_str());

    /* Exact expires_at boundary: t == expires_at - 1 is still blocked */
    fakeSec->store(1763440000LL + 86399LL);
    EXPECT_ERROR(403, "IP_BANNED",
                 http().get("/api/v1/tables", "",
                            {{"X-Forwarded-For", "203.0.113.77"}}));

    /* t == expires_at is immediately unbanned on lookup, and t + 86401s + tickLoop() purges DB */
    fakeSec->store(1763440000LL + 86400LL);
    EXPECT_STATUS(200, http().get("/api/v1/tables", "",
                                  {{"X-Forwarded-For", "203.0.113.77"}}));

    fakeSec->store(1763440000LL + 86401LL);
    const uint64_t baseTicks = env->server->tickCount();
    for (int i = 0; i < 100 && env->server->tickCount() <= baseTicks + 1; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    Reply rAdminAfter = http().get("/api/v1/admin/bans", admin.token);
    EXPECT_STATUS(200, rAdminAfter);
    CHECK_EQUAL(0, static_cast<int>(rAdminAfter.json["temp_bans"].size()));

    /* 3. Disallowed crawler UA (>= 5 hits) escalates to 24h IP_BANNED even if UA changes */
    fakeSec->store(1763530000LL);
    for (int i = 0; i < 5; ++i) {
        (void)http().get("/api/v1/tables", "",
                         {{"X-Forwarded-For", "198.51.100.88"},
                          {"User-Agent", "GPTBot/1.0"}});
    }
    Reply rCrawlerBanned = http().get("/api/v1/tables", "",
                                      {{"X-Forwarded-For", "198.51.100.88"},
                                       {"User-Agent", "Mozilla/5.0"}});
    EXPECT_ERROR(403, "IP_BANNED", rCrawlerBanned);

    /* 4. Exploit probes (malformed X-Octi-Protocol-Version >= 20 hits) escalate to 24h IP_BANNED */
    for (int i = 0; i < 20; ++i) {
        (void)http().get("/api/v1/tables", "",
                         {{"X-Forwarded-For", "198.51.100.89"},
                          {"X-Octi-Protocol-Version", "not-a-number"}});
    }
    EXPECT_ERROR(403, "IP_BANNED",
                 http().get("/api/v1/tables", "",
                            {{"X-Forwarded-For", "198.51.100.89"}}));

    /* 5. Queued player banned by admin is immediately ejected from matchmaking & lobby */
    User queuedUser = signIn("apple", "sub-e5-queued", "QueuedPlayer");
    EXPECT_STATUS(200, http().post("/api/v1/matchmaking/enqueue", queuedUser.token,
                                   Json{{"mode", "2P"}, {"bases", 3}}));
    Reply rMatchBefore = http().get("/api/v1/matchmaking/status", queuedUser.token);
    EXPECT_STATUS(200, rMatchBefore);
    CHECK_TRUE(rMatchBefore.json.value("queued", false));

    Reply rBanQueued = http().post("/api/v1/admin/bans", admin.token,
                                   Json{{"target_type", "account"},
                                        {"target", "@" + queuedUser.handle},
                                        {"duration", "24h"},
                                        {"reason", "Queue abuse"}});
    EXPECT_STATUS(200, rBanQueued);
    const std::string qBanId = rBanQueued.json.value("ban_id", "");
    EXPECT_ERROR(403, "ACCOUNT_BANNED",
                 http().get("/api/v1/matchmaking/status", queuedUser.token));
    EXPECT_STATUS(200, http().del("/api/v1/admin/bans/" + qBanId, admin.token));
    User queuedRestored = signIn("apple", "sub-e5-queued", "QueuedPlayer");
    Reply rMatchAfter = http().get("/api/v1/matchmaking/status", queuedRestored.token);
    EXPECT_STATUS(200, rMatchAfter);
    CHECK_FALSE(rMatchAfter.json.value("queued", true));
}

TEST(OctiHttpServerIntegrationTest, Ply0AbortAndStandUpEndpointsAcrossVariantsRejectionsAndLegacyCompat)
{
    User alice = signIn("google", "sub-ply0-alice", "Ply0 Alice");
    User bob = signIn("apple", "sub-ply0-bob", "Ply0 Bob");
    User carol = signIn("microsoft", "sub-ply0-carol", "Ply0 Carol");
    User dave = signIn("facebook", "sub-ply0-dave", "Ply0 Dave");
    User erin = signIn("line", "sub-ply0-erin", "Ply0 Erin");

    Reply rBotCreate = http().post(
        "/api/v1/robots", alice.token,
        Json{{"robot_name", "ply0_http_bot"}, {"description", "Ply0 HTTP bot"}});
    EXPECT_STATUS(201, rBotCreate);
    const std::string botToken = rBotCreate.json.value("robot_token", "");
    const std::string botUuid = rBotCreate.json.value("robot_uuid", "");
    CHECK_FALSE(botToken.empty());

    /* 1. Rated 2P Human vs Robot: POST /api/v1/games/:id/action {"action":"abort"} */
    std::string room2p = createTable(
        alice, Json{{"mode", "2P"}, {"bases", 3}, {"rated", true}, {"allow_robots", true}});
    Reply rBotJoin = http().post("/api/v1/tables/" + room2p + "/join", botToken, Json::object());
    EXPECT_STATUS(200, rBotJoin);
    CHECK_TRUE(rBotJoin.json.value("game_started", false));
    STRCMP_EQUAL("IN_PROGRESS", view(room2p, alice.token).value("phase", "").c_str());

    Reply rAbort = http().post("/api/v1/games/" + room2p + "/action", botToken,
                               Json{{"action", "abort"}});
    EXPECT_STATUS(200, rAbort);
    CHECK_TRUE(rAbort.json.value("ok", false));
    STRCMP_EQUAL("abort", rAbort.json.value("action", "").c_str());
    STRCMP_EQUAL(room2p.c_str(), rAbort.json.value("room_id", "").c_str());
    CHECK_FALSE(rAbort.json.value("game_over", true));
    CHECK_TRUE(rAbort.json.value("stood_up_ply0", false));
    STRCMP_EQUAL("ply0_standup", rAbort.json.value("detail", "").c_str());
    CHECK_TRUE(rAbort.json.value("seq", 0LL) > 0);

    Json v2p = view(room2p, alice.token);
    STRCMP_EQUAL("WAITING_FOR_PLAYERS", v2p.value("phase", "").c_str());
    STRCMP_EQUAL(alice.uuid.c_str(), v2p.value("host_uuid", "").c_str());
    CHECK_EQUAL(0, v2p.value("ply_count", -1));
    CHECK_TRUE(v2p["seats"][1]["player_uuid"].is_null());

    /* Events poll delivers SEAT_LEFT with detail="ply0_standup" */
    Reply rEvs = http().get("/api/v1/games/" + room2p + "/events?since_seq=0", alice.token);
    EXPECT_STATUS(200, rEvs);
    bool foundPly0Ev = false;
    for (size_t i = 0; i < rEvs.json["events"].size(); ++i) {
        const Json &ev = rEvs.json["events"][i];
        if (ev.value("type", "") == "SEAT_LEFT" &&
            ev.value("detail", "") == "ply0_standup" &&
            ev.value("seat", 0) == 2) {
            foundPly0Ev = true;
        }
    }
    CHECK_TRUE(foundPly0Ev);

    /* Zero archived game and zero robot losses */
    Reply rBotList = http().get("/api/v1/robots", alice.token);
    EXPECT_STATUS(200, rBotList);
    for (size_t i = 0; i < rBotList.json["robots"].size(); ++i) {
        if (rBotList.json["robots"][i].value("robot_uuid", "") == botUuid) {
            CHECK_EQUAL(0, rBotList.json["robots"][i].value("games_played", -1));
            CHECK_EQUAL(0, rBotList.json["robots"][i].value("losses", -1));
        }
    }

    /* Bob immediately fills Seat 2 of the SAME room2p; host Alice stands up via {"abort_if_ply0": true} */
    Reply rBobJoin = http().post("/api/v1/tables/" + room2p + "/join", bob.token, Json::object());
    EXPECT_STATUS(200, rBobJoin);
    CHECK_TRUE(rBobJoin.json.value("game_started", false));

    /* Invalid abort_if_ply0 type -> 400 INVALID_ARGUMENT */
    EXPECT_ERROR(400, "INVALID_ARGUMENT",
                 http().post("/api/v1/tables/" + room2p + "/leave", alice.token,
                             Json{{"abort_if_ply0", "yes"}}));

    Reply rAliceLeavePly0 = http().post("/api/v1/tables/" + room2p + "/leave", alice.token,
                                        Json{{"abort_if_ply0", true}});
    EXPECT_STATUS(200, rAliceLeavePly0);
    CHECK_TRUE(rAliceLeavePly0.json.value("ok", false));
    CHECK_TRUE(rAliceLeavePly0.json.value("stood_up_ply0", false));
    STRCMP_EQUAL("ply0_standup", rAliceLeavePly0.json.value("detail", "").c_str());

    Json vAfterAlice = view(room2p, bob.token);
    STRCMP_EQUAL("WAITING_FOR_PLAYERS", vAfterAlice.value("phase", "").c_str());
    STRCMP_EQUAL(bob.uuid.c_str(), vAfterAlice.value("host_uuid", "").c_str());

    /* 2. OFK uppercase "STAND_UP" & 4P uppercase "ABORT" normalization */
    std::string roomOfk = startGame({alice, carol}, Json{{"mode", "OFK"}});
    Reply rOfkStandUp = http().post("/api/v1/games/" + roomOfk + "/action", carol.token,
                                    Json{{"action", "STAND_UP"}});
    EXPECT_STATUS(200, rOfkStandUp);
    STRCMP_EQUAL("stand_up", rOfkStandUp.json.value("action", "").c_str());
    CHECK_TRUE(rOfkStandUp.json.value("stood_up_ply0", false));
    STRCMP_EQUAL("WAITING_FOR_PLAYERS", view(roomOfk, alice.token).value("phase", "").c_str());
    EXPECT_STATUS(200, http().post("/api/v1/tables/" + roomOfk + "/leave", alice.token, Json::object()));

    std::string room4p = startGame({alice, carol, dave, erin}, Json{{"mode", "4P"}});
    Reply r4pAbort = http().post("/api/v1/games/" + room4p + "/action", dave.token,
                                 Json{{"action", "ABORT"}});
    EXPECT_STATUS(200, r4pAbort);
    STRCMP_EQUAL("abort", r4pAbort.json.value("action", "").c_str());
    CHECK_TRUE(r4pAbort.json.value("stood_up_ply0", false));
    Json v4p = view(room4p, alice.token);
    STRCMP_EQUAL("WAITING_FOR_PLAYERS", v4p.value("phase", "").c_str());
    CHECK_TRUE(v4p["seats"][2]["player_uuid"].is_null());

    /* 3. Strict Ply > 0 rejection & empty-body /leave backwards compatibility */
    Reply rDaveRejoin = http().post("/api/v1/tables/" + room4p + "/join", dave.token, Json::object());
    EXPECT_STATUS(200, rDaveRejoin);
    CHECK_TRUE(rDaveRejoin.json.value("game_started", false));
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room4p + "/move", alice.token,
                                   Json{{"move", "33+a"}}));

    EXPECT_ERROR(409, "INVALID_STATE",
                 http().post("/api/v1/games/" + room4p + "/action", alice.token,
                             Json{{"action", "abort"}}));
    EXPECT_ERROR(409, "INVALID_STATE",
                 http().post("/api/v1/games/" + room4p + "/action", carol.token,
                             Json{{"action", "stand_up"}}));
    EXPECT_ERROR(409, "INVALID_STATE",
                 http().post("/api/v1/tables/" + room4p + "/leave", erin.token,
                             Json{{"abort_if_ply0", true}}));

    Json v4pAfterReject = view(room4p, alice.token);
    STRCMP_EQUAL("IN_PROGRESS", v4pAfterReject.value("phase", "").c_str());
    CHECK_EQUAL(1, v4pAfterReject.value("ply_count", 0));
    CHECK_TRUE(v4pAfterReject["seats"][3].value("connected", false));

    /* Cleanly finish room4p via resignation, then verify finished-room abort rejection */
    EXPECT_STATUS(200, http().post("/api/v1/games/" + room4p + "/resign", carol.token,
                                   Json::object()));
    EXPECT_ERROR(409, "INVALID_STATE",
                 http().post("/api/v1/games/" + room4p + "/action", alice.token,
                             Json{{"action", "abort"}}));

    /* Empty-body POST /leave (no Content-Type) preserves legacy detach */
    Reply rEmptyLeave = http().raw("POST", "/api/v1/tables/" + room2p + "/leave", bob.token, "", "");
    EXPECT_STATUS(200, rEmptyLeave);
    CHECK_FALSE(rEmptyLeave.json.value("stood_up_ply0", true));
    STRCMP_EQUAL("waiting_left", rEmptyLeave.json.value("detail", "").c_str());

    std::string roomLegacy = startGame({alice, bob}, Json{{"mode", "2P"}});
    Reply rLegacyInProg = http().raw("POST", "/api/v1/tables/" + roomLegacy + "/leave", bob.token, "", "");
    EXPECT_STATUS(200, rLegacyInProg);
    CHECK_FALSE(rLegacyInProg.json.value("stood_up_ply0", true));
    STRCMP_EQUAL("in_progress_disconnected", rLegacyInProg.json.value("detail", "").c_str());
    Json vLegacy = view(roomLegacy, alice.token);
    STRCMP_EQUAL("IN_PROGRESS", vLegacy.value("phase", "").c_str());
    CHECK_FALSE(vLegacy["seats"][1].value("connected", true));
}

TEST(OctiHttpServerIntegrationTest, Ply0AbortWebUiControlsPhaseSyncAndEventFormatting)
{
    Reply rIndex = http().get("/");
    EXPECT_STATUS(200, rIndex);

    /* Preserve exact DOM order invariants alongside new Ply-0 JS logic */
    size_t posSideTopHalf = rIndex.body.find("id=\"web-side-top-half\"");
    size_t posBoardLobbyChat = rIndex.body.find("id=\"web-board-lobby-chat-panel\"");
    size_t posBoardChatPanel = rIndex.body.find("id=\"web-board-chat-panel\"");
    size_t posLeaveTableBtn = rIndex.body.find("id=\"btn-web-leave-table\"");
    size_t posMatchActionsCard = rIndex.body.find("id=\"web-match-actions-card\"");
    size_t posResignBtn = rIndex.body.find("id=\"btn-web-resign\"");

    CHECK_TRUE(posSideTopHalf != std::string::npos);
    CHECK_TRUE(posBoardLobbyChat != std::string::npos);
    CHECK_TRUE(posBoardChatPanel != std::string::npos);
    CHECK_TRUE(posLeaveTableBtn != std::string::npos);
    CHECK_TRUE(posMatchActionsCard != std::string::npos);
    CHECK_TRUE(posResignBtn != std::string::npos);
    CHECK_TRUE(posSideTopHalf < posBoardLobbyChat);
    CHECK_TRUE(posBoardLobbyChat < posBoardChatPanel);
    CHECK_TRUE(posBoardChatPanel < posLeaveTableBtn);
    CHECK_TRUE(posLeaveTableBtn < posMatchActionsCard);
    CHECK_TRUE(posMatchActionsCard < posResignBtn);

    CHECK_TRUE(rIndex.body.find("resignBtn.textContent = \"Abort\";") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("leaveTableBtn.textContent = \"Stand Up\";") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("body: { abort_if_ply0: true }") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("body: { action: \"abort\" }") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("activeOnlinePrevPhase") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("ev.detail === \"ply0_standup\"") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("stood up before move 1 (") != std::string::npos);
    CHECK_TRUE(rIndex.body.find("table reopened for players)") != std::string::npos);
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
