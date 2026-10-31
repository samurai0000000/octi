/*
 * TestDesktopNetworkAndVoice.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "ClientConfigStore.hxx"
#include "OctiCrashReporter.hxx"
#include "OctiNetworkClient.hxx"
#include "P2pVoiceEngine.hxx"

#include "AccountStore.hxx"
#include "GameRoomManager.hxx"
#include "OctiDefs.hxx"
#include "OctiHttpServer.hxx"
#include "OidcVerifier.hxx"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <rapidcheck.h>
#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness.h>

using namespace octi;
using Json = nlohmann::json;

namespace {

struct PkeyDeleter {
    void operator()(EVP_PKEY *p) const { EVP_PKEY_free(p); }
};
using Pkey = std::unique_ptr<EVP_PKEY, PkeyDeleter>;

struct BnDeleter {
    void operator()(BIGNUM *p) const { BN_free(p); }
};
using Bn = std::unique_ptr<BIGNUM, BnDeleter>;

std::string bnParam(EVP_PKEY *key, const char *name, int padTo)
{
    BIGNUM *raw = nullptr;
    if (EVP_PKEY_get_bn_param(key, name, &raw) != 1) {
        return std::string();
    }
    Bn bn(raw);
    int len = padTo > 0 ? padTo : BN_num_bytes(bn.get());
    std::string out(static_cast<std::size_t>(len), '\0');
    if (padTo > 0) {
        BN_bn2binpad(bn.get(),
                     reinterpret_cast<unsigned char *>(&out[0]), len);
    } else {
        BN_bn2bin(bn.get(), reinterpret_cast<unsigned char *>(&out[0]));
    }
    return out;
}

std::string digestSign(EVP_PKEY *key, const std::string &input)
{
    std::unique_ptr<EVP_MD_CTX, void (*)(EVP_MD_CTX *)> md(
        EVP_MD_CTX_new(), EVP_MD_CTX_free);
    std::size_t len = 0;
    if (!md ||
        EVP_DigestSignInit(md.get(), nullptr, EVP_sha256(), nullptr, key) != 1 ||
        EVP_DigestSign(md.get(), nullptr, &len,
                       reinterpret_cast<const unsigned char *>(input.data()),
                       input.size()) != 1) {
        return std::string();
    }
    std::string sig(len, '\0');
    if (EVP_DigestSign(md.get(),
                       reinterpret_cast<unsigned char *>(&sig[0]),
                       &len,
                       reinterpret_cast<const unsigned char *>(input.data()),
                       input.size()) != 1) {
        return std::string();
    }
    sig.resize(len);
    return sig;
}

std::string signEs256(EVP_PKEY *key, const std::string &input)
{
    std::string der = digestSign(key, input);
    const unsigned char *p =
        reinterpret_cast<const unsigned char *>(der.data());
    ECDSA_SIG *sig =
        d2i_ECDSA_SIG(nullptr, &p, static_cast<long>(der.size()));
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
        : rsa(EVP_PKEY_Q_keygen(nullptr, nullptr, "RSA", std::size_t(2048))),
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
    rsa["n"] = base64UrlEncode(
        bnParam(keys().rsa.get(), OSSL_PKEY_PARAM_RSA_N, 0));
    rsa["e"] = base64UrlEncode(
        bnParam(keys().rsa.get(), OSSL_PKEY_PARAM_RSA_E, 0));

    Json ec;
    ec["kty"] = "EC";
    ec["use"] = "sig";
    ec["kid"] = "ec-1";
    ec["crv"] = "P-256";
    ec["x"] = base64UrlEncode(
        bnParam(keys().ec.get(), OSSL_PKEY_PARAM_EC_PUB_X, 32));
    ec["y"] = base64UrlEncode(
        bnParam(keys().ec.get(), OSSL_PKEY_PARAM_EC_PUB_Y, 32));

    Json doc;
    doc["keys"] = Json::array({rsa, ec});
    return doc.dump();
}

std::int64_t realNow()
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

std::string makeToken(const std::string &provider,
                      const std::string &sub,
                      const std::string &name)
{
    Json header;
    header["alg"] = (provider == "line") ? "ES256" : "RS256";
    header["typ"] = "JWT";
    header["kid"] = (provider == "line") ? "ec-1" : "rsa-1";

    Json claims;
    claims["iss"] = issuerFor(provider);
    claims["aud"] = "client-" + provider;
    claims["sub"] = sub;
    claims["iat"] = realNow() - 10;
    claims["exp"] = realNow() + 3600;
    claims["email"] = sub + "@example.com";
    claims["name"] = name;
    claims["picture"] = "https://example.com/avatar.png";

    std::string input =
        base64UrlEncode(header.dump()) + "." + base64UrlEncode(claims.dump());
    std::string alg = header.value("alg", "RS256");
    std::string sig = (alg == "ES256")
                          ? signEs256(keys().ec.get(), input)
                          : digestSign(keys().rsa.get(), input);
    return input + "." + base64UrlEncode(sig);
}

HttpServerConfig defaultTestServerConfig()
{
    HttpServerConfig c;
    c.bindAddress = "127.0.0.1";
    c.port = 0;
    c.tickIntervalMs = 40;
    c.ssePollMs = 20;
    c.sseHeartbeatMs = 200;
    c.sseSessionRecheckMs = 100;
    static const char *names[] = {
        "google", "apple", "microsoft", "facebook", "line"
    };
    for (const char *name : names) {
        c.providerClientIds[name].push_back(std::string("client-") + name);
    }
    return c;
}

struct LiveServerEnv {
    std::string dir;
    std::string dbPath;
    AccountStore store;
    OidcVerifier verifier;
    std::unique_ptr<GameRoomManager> rooms;
    std::unique_ptr<OctiHttpServer> server;
    int savedPort = 0;

    explicit LiveServerEnv(
        const HttpServerConfig &cfg = defaultTestServerConfig())
    {
        char tmpl[] = "/tmp/octi_desktop_net_XXXXXX";
        if (mkdtemp(tmpl) == nullptr) {
            FAIL("mkdtemp failed");
        }
        dir = tmpl;
        dbPath = dir + "/octi.db";
        std::string err;
        if (!store.openOrCreate(dbPath, err)) {
            FAIL(err.c_str());
        }
        static const char *names[] = {
            "google", "apple", "microsoft", "facebook", "line"
        };
        for (const char *n : names) {
            verifier.setClientId(n, std::string("client-") + n);
            if (!verifier.setKeySet(n, jwks())) {
                FAIL("setKeySet failed");
            }
        }
        rooms = std::make_unique<GameRoomManager>(store);
        server = std::make_unique<OctiHttpServer>(
            store, verifier, *rooms, cfg);
        if (!server->start(err)) {
            FAIL(err.c_str());
        }
        savedPort = server->port();
    }

    ~LiveServerEnv()
    {
        server.reset();
        rooms.reset();
        store.close();
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    int port() const { return server ? server->port() : savedPort; }

    void stop()
    {
        if (rooms) {
            rooms->checkpointToStore();
        }
        if (server) {
            server->stop();
        }
        server.reset();
        rooms.reset();
    }

    void startAgain(const HttpServerConfig *newCfg = nullptr)
    {
        HttpServerConfig c =
            (newCfg != nullptr) ? *newCfg : defaultTestServerConfig();
        c.port = savedPort;
        rooms = std::make_unique<GameRoomManager>(store);
        server = std::make_unique<OctiHttpServer>(store, verifier, *rooms, c);
        std::string err;
        if (!server->start(err)) {
            FAIL(err.c_str());
        }
        savedPort = server->port();
    }

    void restart(const HttpServerConfig *newCfg = nullptr)
    {
        stop();
        startAgain(newCfg);
    }
};

} // namespace

/* ==================================================================== */
/* Group 1: ClientConfigStoreTest                                       */
/* ==================================================================== */

TEST_GROUP(ClientConfigStoreTest)
{
    std::string tempDir;

    void setup() override
    {
        char tmpl[] = "/tmp/octi_cfg_test_XXXXXX";
        CHECK(mkdtemp(tmpl) != nullptr);
        tempDir = tmpl;
    }

    void teardown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(tempDir, ec);
    }
};

TEST(ClientConfigStoreTest, SaveLoadAndPosix0600Permissions)
{
    std::string cfgPath = tempDir + "/nested/config.json";
    ClientConfigStore store(cfgPath);

    STRCMP_EQUAL("octi.selfso.com", ClientConfigStore::serverHost());
    LONGS_EQUAL(443, ClientConfigStore::serverPort());

    ClientPreferences &prefs = store.preferences();
    prefs.sessionToken = "secret_session_tok_12345";
    prefs.playerUuid = "uuid-player-001";
    prefs.handle = "GrandmasterA";
    prefs.theme = ThemeId::ROSEWOOD_IVORY;
    prefs.confirmAllMoves = true;
    prefs.sfxMuted = true;
    prefs.pttKey = 86;

    std::string err;
    CHECK_TRUE_TEXT(store.save(err), err.c_str());

    struct stat dirSt;
    CHECK_EQUAL(0, ::stat((tempDir + "/nested").c_str(), &dirSt));
    LONGS_EQUAL(0700, dirSt.st_mode & 0777);

    struct stat fileSt;
    CHECK_EQUAL(0, ::stat(cfgPath.c_str(), &fileSt));
    LONGS_EQUAL(0600, fileSt.st_mode & 0777);

    ClientConfigStore reloadStore(cfgPath);
    CHECK_TRUE_TEXT(reloadStore.load(err), err.c_str());
    const ClientPreferences &loaded = reloadStore.preferences();
    STRCMP_EQUAL("secret_session_tok_12345", loaded.sessionToken.c_str());
    STRCMP_EQUAL("uuid-player-001", loaded.playerUuid.c_str());
    STRCMP_EQUAL("GrandmasterA", loaded.handle.c_str());
    LONGS_EQUAL(static_cast<long>(ThemeId::ROSEWOOD_IVORY),
                static_cast<long>(loaded.theme));
    CHECK_TRUE(loaded.confirmAllMoves);
    CHECK_TRUE(loaded.sfxMuted);
    LONGS_EQUAL(86, loaded.pttKey);

    reloadStore.clearSession();
    CHECK_TRUE_TEXT(reloadStore.save(err), err.c_str());
    ClientConfigStore afterClearStore(cfgPath);
    CHECK_TRUE_TEXT(afterClearStore.load(err), err.c_str());
    STRCMP_EQUAL("", afterClearStore.preferences().sessionToken.c_str());
    STRCMP_EQUAL("", afterClearStore.preferences().playerUuid.c_str());
    STRCMP_EQUAL("GrandmasterA", afterClearStore.preferences().handle.c_str());
}

TEST(ClientConfigStoreTest, IgnoresAndStripsInjectedServerHostAndPortKeys)
{
    std::string cfgPath = tempDir + "/injected.json";
    std::string rawJson = R"({
        "session_token": "tok_abc",
        "player_uuid": "uuid_xyz",
        "handle": "Samurai",
        "theme": "ROSEWOOD_IVORY",
        "confirm_all_moves": false,
        "sfx_muted": false,
        "ptt_key": 86,
        "server_url": "https://evil.example.com:9999",
        "server_host": "evil.example.com",
        "server_port": 9999
    })";

    {
        std::ofstream ofs(cfgPath, std::ios::binary | std::ios::trunc);
        ofs << rawJson;
    }

    ClientConfigStore store(cfgPath);
    std::string err;
    CHECK_TRUE_TEXT(store.load(err), err.c_str());
    STRCMP_EQUAL("tok_abc", store.preferences().sessionToken.c_str());
    STRCMP_EQUAL("Samurai", store.preferences().handle.c_str());
    STRCMP_EQUAL("octi.selfso.com", store.serverHost());
    LONGS_EQUAL(443, store.serverPort());

    /* Re-saving must strip any server_url / server_host / server_port keys */
    CHECK_TRUE_TEXT(store.save(err), err.c_str());
    std::ifstream ifs(cfgPath, std::ios::binary);
    std::string rewritten((std::istreambuf_iterator<char>(ifs)),
                          std::istreambuf_iterator<char>());
    CHECK_TRUE(rewritten.find("server_url") == std::string::npos);
    CHECK_TRUE(rewritten.find("server_host") == std::string::npos);
    CHECK_TRUE(rewritten.find("server_port") == std::string::npos);
    CHECK_TRUE(rewritten.find("evil.example.com") == std::string::npos);
}

TEST(ClientConfigStoreTest, RejectsCorruptAndOversizedConfigFiles)
{
    std::string cfgPath = tempDir + "/bad.json";
    {
        std::ofstream ofs(cfgPath, std::ios::binary | std::ios::trunc);
        ofs << "{ not valid json ";
    }
    ClientConfigStore store(cfgPath);
    std::string err;
    CHECK_FALSE(store.load(err));

    /* Oversized (> 64 KiB) file must be rejected */
    {
        std::ofstream ofs(cfgPath, std::ios::binary | std::ios::trunc);
        std::string huge(70000, 'a');
        ofs << "{\"handle\":\"" << huge << "\"}";
    }
    CHECK_FALSE(store.load(err));
}

/* ==================================================================== */
/* Group 2: OctiNetworkClientTest                                       */
/* ==================================================================== */

TEST_GROUP(OctiNetworkClientTest)
{
};

TEST(OctiNetworkClientTest, DefaultConstructorHardCodesOctiSelfsoComPort443)
{
    STRCMP_EQUAL("octi.selfso.com", OctiNetworkClient::canonicalHost());
    LONGS_EQUAL(443, OctiNetworkClient::canonicalPort());

    OctiNetworkClient prodClient;
    STRCMP_EQUAL("octi.selfso.com", prodClient.host().c_str());
    LONGS_EQUAL(443, prodClient.port());
    STRCMP_EQUAL("https://octi.selfso.com/api/v1/health",
                 prodClient.buildUrl("/api/v1/health").c_str());
    STRCMP_EQUAL("https://octi.selfso.com",
                 prodClient.buildUrl().c_str());

    /* Test fallback / legacy endpoint buildUrl */
    prodClient.setEndpoint(OCTI_SERVER_FALLBACK_HOST, OCTI_SERVER_FALLBACK_PORT);
    STRCMP_EQUAL("trumpet.selfso.com", prodClient.host().c_str());
    LONGS_EQUAL(6874, prodClient.port());
    STRCMP_EQUAL("http://trumpet.selfso.com:6874/api/v1/health",
                 prodClient.buildUrl("/api/v1/health").c_str());
}

TEST(OctiNetworkClientTest,
     HealthProvidersAndFiveProviderSsoOnboardingLinkUnlinkAndConflict409)
{
    LiveServerEnv env;
    auto clientA = OctiNetworkClient::forHermeticTest(env.port());
    auto clientB = OctiNetworkClient::forHermeticTest(env.port());

    auto health = clientA->fetchHealth();
    CHECK_TRUE_TEXT(health.ok, health.errorMessage.c_str());
    LONGS_EQUAL(200, health.httpStatus);
    STRCMP_EQUAL("ok", health.data.status.c_str());

    auto provs = clientA->fetchProviders();
    CHECK_TRUE_TEXT(provs.ok, provs.errorMessage.c_str());
    LONGS_EQUAL(5, static_cast<long>(provs.data.size()));

    /* 1. First-time Google SSO sign-in -> 201 Created */
    auto loginA1 = clientA->loginWithSso(
        "google", makeToken("google", "sub-google-a", "Alice"), "Alice_GM");
    CHECK_TRUE_TEXT(loginA1.ok, loginA1.errorMessage.c_str());
    LONGS_EQUAL(201, loginA1.httpStatus);
    CHECK_TRUE(loginA1.data.isNewAccount);
    CHECK_FALSE(loginA1.data.sessionToken.empty());
    std::string uuidA = loginA1.data.profile.uuid;

    /* 2. Repeat Google SSO sign-in -> 200 OK */
    auto loginA2 = clientA->loginWithSso(
        "google", makeToken("google", "sub-google-a", "Alice"));
    CHECK_TRUE_TEXT(loginA2.ok, loginA2.errorMessage.c_str());
    LONGS_EQUAL(200, loginA2.httpStatus);
    CHECK_FALSE(loginA2.data.isNewAccount);
    STRCMP_EQUAL(uuidA.c_str(), loginA2.data.profile.uuid.c_str());

    /* 3. Update handle via PATCH /me */
    auto patchMe = clientA->updateMyProfile("Alice_Crown");
    CHECK_TRUE_TEXT(patchMe.ok, patchMe.errorMessage.c_str());
    STRCMP_EQUAL("Alice_Crown", patchMe.data.handle.c_str());

    /* 4. Link Apple, Microsoft, Facebook, and LINE (ES256) to Player A */
    static const char *otherProviders[] = {
        "apple", "microsoft", "facebook", "line"
    };
    for (const char *prov : otherProviders) {
        auto linkRes = clientA->linkIdentity(
            prov, makeToken(prov, std::string("sub-") + prov + "-a", "Alice"));
        CHECK_TRUE_TEXT(linkRes.ok, linkRes.errorMessage.c_str());
        LONGS_EQUAL(200, linkRes.httpStatus);
    }

    auto meA = clientA->fetchMyProfile();
    CHECK_TRUE_TEXT(meA.ok, meA.errorMessage.c_str());
    LONGS_EQUAL(5, static_cast<long>(meA.data.identities.size()));

    /* 5. Unlink 4 providers leaving Google; unlinking last identity -> 400 */
    for (const char *prov : otherProviders) {
        auto unl = clientA->unlinkIdentity(prov);
        CHECK_TRUE_TEXT(unl.ok, unl.errorMessage.c_str());
    }
    auto unlinkLast = clientA->unlinkIdentity("google");
    CHECK_FALSE(unlinkLast.ok);
    LONGS_EQUAL(400, unlinkLast.httpStatus);

    /* 6. Player B signs in with Apple; Player A tries to link Player B's Apple -> 409 Conflict */
    std::string bobAppleToken = makeToken("apple", "sub-apple-bob", "Bob");
    auto loginB = clientB->loginWithSso("apple", bobAppleToken, "Bob_Player");
    CHECK_TRUE_TEXT(loginB.ok, loginB.errorMessage.c_str());
    LONGS_EQUAL(201, loginB.httpStatus);
    std::string uuidB = loginB.data.profile.uuid;

    auto conflictLink = clientA->linkIdentity("apple", bobAppleToken);
    CHECK_FALSE(conflictLink.ok);
    LONGS_EQUAL(409, conflictLink.httpStatus);
    CHECK_TRUE(conflictLink.data.conflictAlreadyLinked);
    STRCMP_EQUAL(uuidB.c_str(),
                 conflictLink.data.existingProfile.uuid.c_str());
    STRCMP_EQUAL("Bob_Player",
                 conflictLink.data.existingProfile.handle.c_str());

    /* 7. Public profile & leaderboard & logout */
    auto pubB = clientA->fetchPlayerProfile(uuidB);
    CHECK_TRUE_TEXT(pubB.ok, pubB.errorMessage.c_str());
    STRCMP_EQUAL("Bob_Player", pubB.data.handle.c_str());

    auto lb = clientA->fetchLeaderboard("2P", 10);
    CHECK_TRUE_TEXT(lb.ok, lb.errorMessage.c_str());
    CHECK_TRUE(lb.data.size() >= 2);

    auto outB = clientB->logout();
    CHECK_TRUE_TEXT(outB.ok, outB.errorMessage.c_str());
    CHECK_TRUE(clientB->sessionToken().empty());
}

TEST(OctiNetworkClientTest, OAuthLoopbackListenerCapturesBrowserRedirect)
{
    OAuthLoopbackListener listener;
    std::string err;
    std::string cbProv;
    std::string cbTok;
    CHECK_TRUE_TEXT(
        listener.start(
            [&](const std::string &prov, const std::string &tok) {
                cbProv = prov;
                cbTok = tok;
            },
            err),
        err.c_str());
    CHECK_TRUE(listener.isRunning());
    CHECK_TRUE(listener.boundPort() > 0);

    httplib::Client browser("127.0.0.1", listener.boundPort());
    auto res = browser.Get(
        "/oauth/callback?provider=google&id_token=loopback_jwt_token_999");
    CHECK_TRUE(res != nullptr);
    LONGS_EQUAL(200, res->status);
    CHECK_TRUE(res->body.find("Sign-in complete") != std::string::npos);

    std::string waitedProv;
    std::string waitedTok;
    CHECK_TRUE(listener.waitForCallback(1000, waitedProv, waitedTok));
    STRCMP_EQUAL("google", waitedProv.c_str());
    STRCMP_EQUAL("loopback_jwt_token_999", waitedTok.c_str());
    STRCMP_EQUAL("google", cbProv.c_str());
    STRCMP_EQUAL("loopback_jwt_token_999", cbTok.c_str());

    listener.stop();
    CHECK_FALSE(listener.isRunning());
}

TEST(OctiNetworkClientTest, LobbyChatTablesPasscodeKickObserveAndMatchmaking)
{
    LiveServerEnv env;
    auto clientA = OctiNetworkClient::forHermeticTest(env.port());
    auto clientB = OctiNetworkClient::forHermeticTest(env.port());
    auto clientC = OctiNetworkClient::forHermeticTest(env.port());

    auto loginA = clientA->loginWithSso(
        "google", makeToken("google", "sub-lobby-a", "LobbyA"), "LobbyA");
    auto loginB = clientB->loginWithSso(
        "google", makeToken("google", "sub-lobby-b", "LobbyB"), "LobbyB");
    auto loginC = clientC->loginWithSso(
        "google", makeToken("google", "sub-lobby-c", "LobbyC"), "LobbyC");
    CHECK_TRUE(loginA.ok && loginB.ok && loginC.ok);

    /* Start lobby SSE stream on clientA */
    std::mutex sseMutex;
    std::vector<SseEvent> lobbyEvents;
    clientA->startLobbyEventStream(0, [&](const SseEvent &ev) {
        std::lock_guard<std::mutex> lock(sseMutex);
        lobbyEvents.push_back(ev);
    });

    auto chatRes = clientB->postLobbyChat("Hello OCTI Lobby!");
    CHECK_TRUE_TEXT(chatRes.ok, chatRes.errorMessage.c_str());

    bool gotLobbyChatSse = false;
    for (int i = 0; i < 40 && !gotLobbyChatSse; ++i) {
        {
            std::lock_guard<std::mutex> lock(sseMutex);
            for (const auto &ev : lobbyEvents) {
                if (ev.event == "LOBBY_CHAT" &&
                    ev.dataJson.find("Hello OCTI Lobby!") !=
                        std::string::npos) {
                    gotLobbyChatSse = true;
                    break;
                }
            }
        }
        if (!gotLobbyChatSse) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    CHECK_TRUE(gotLobbyChatSse);
    clientA->stopLobbyEventStream();

    auto lobbyState = clientA->fetchLobbyState();
    CHECK_TRUE_TEXT(lobbyState.ok, lobbyState.errorMessage.c_str());
    CHECK_TRUE(lobbyState.data.players.size() >= 3);
    CHECK_FALSE(lobbyState.data.recentChat.empty());

    /* Create 4P table with passcode so joining 1 player stays pre-game for kickSeat */
    CreateTableRequest createReq;
    createReq.mode = "4P";
    createReq.bases = 3;
    createReq.passcode = "4321";
    auto created = clientA->createTable(createReq);
    CHECK_TRUE_TEXT(created.ok, created.errorMessage.c_str());
    std::string roomId = created.data.roomId;

    auto tables = clientB->listTables();
    CHECK_TRUE_TEXT(tables.ok, tables.errorMessage.c_str());
    CHECK_FALSE(tables.data.empty());

    /* Wrong passcode rejected */
    auto badJoin = clientB->joinTable(roomId, 2, "0000");
    CHECK_FALSE(badJoin.ok);
    LONGS_EQUAL(403, badJoin.httpStatus);

    /* Right passcode succeeds */
    auto goodJoin = clientB->joinTable(roomId, 2, "4321");
    CHECK_TRUE_TEXT(goodJoin.ok, goodJoin.errorMessage.c_str());
    LONGS_EQUAL(2, goodJoin.data.seat);

    /* Host kicks Seat 2 before game starts */
    auto kickRes = clientA->kickSeat(roomId, 2);
    CHECK_TRUE_TEXT(kickRes.ok, kickRes.errorMessage.c_str());

    /* Observer C observes and unobserves */
    auto obsRes = clientC->observeTable(roomId, "4321");
    CHECK_TRUE_TEXT(obsRes.ok, obsRes.errorMessage.c_str());
    auto unobsRes = clientC->unobserveTable(roomId);
    CHECK_TRUE_TEXT(unobsRes.ok, unobsRes.errorMessage.c_str());

    auto leaveRes = clientA->leaveTable(roomId);
    CHECK_TRUE_TEXT(leaveRes.ok, leaveRes.errorMessage.c_str());

    /* Matchmaking enqueue / status / dequeue */
    MatchmakingRequest mmReq;
    mmReq.mode = "2P";
    mmReq.bases = 3;
    auto enq = clientA->enqueueMatchmaking(mmReq);
    CHECK_TRUE_TEXT(enq.ok, enq.errorMessage.c_str());

    auto mmStatus = clientA->fetchMatchmakingStatus();
    CHECK_TRUE_TEXT(mmStatus.ok, mmStatus.errorMessage.c_str());
    CHECK_TRUE(mmStatus.data.queued);

    auto deq = clientA->dequeueMatchmaking();
    CHECK_TRUE_TEXT(deq.ok, deq.errorMessage.c_str());
}

TEST(OctiNetworkClientTest,
     LiveGameMovesChatProposalsModerationArchiveAndTranscript)
{
    LiveServerEnv env;
    auto clientA = OctiNetworkClient::forHermeticTest(env.port());
    auto clientB = OctiNetworkClient::forHermeticTest(env.port());
    auto clientC = OctiNetworkClient::forHermeticTest(env.port());

    auto loginA = clientA->loginWithSso(
        "google", makeToken("google", "sub-game-a", "GameA"), "GameA");
    auto loginB = clientB->loginWithSso(
        "google", makeToken("google", "sub-game-b", "GameB"), "GameB");
    auto loginC = clientC->loginWithSso(
        "google", makeToken("google", "sub-game-c", "GameC"), "GameC");
    CHECK_TRUE(loginA.ok && loginB.ok && loginC.ok);
    std::string uuidC = loginC.data.profile.uuid;

    CreateTableRequest req;
    req.mode = "2P";
    req.bases = 3;
    req.allowTakeback = true;
    auto table = clientA->createTable(req);
    CHECK_TRUE_TEXT(table.ok, table.errorMessage.c_str());
    std::string roomId = table.data.roomId;

    auto joinB = clientB->joinTable(roomId, 2);
    CHECK_TRUE_TEXT(joinB.ok, joinB.errorMessage.c_str());
    CHECK_TRUE(joinB.data.gameStarted);

    auto obsC = clientC->observeTable(roomId);
    CHECK_TRUE_TEXT(obsC.ok, obsC.errorMessage.c_str());

    std::mutex sseMutex;
    std::vector<SseEvent> gameEvents;
    clientB->startGameEventStream(roomId, 0, [&](const SseEvent &ev) {
        std::lock_guard<std::mutex> lock(sseMutex);
        gameEvents.push_back(ev);
    });

    auto view0 = clientA->fetchGameView(roomId);
    CHECK_TRUE_TEXT(view0.ok, view0.errorMessage.c_str());
    STRCMP_EQUAL("IN_PROGRESS", view0.data.phase.c_str());
    LONGS_EQUAL(1, view0.data.turnSeat);

    /* Opening moves: Player A adds prong at 33 (33+a), Player B at 37 (37+e) */
    auto m1 = clientA->submitMove(roomId, "33+a");
    CHECK_TRUE_TEXT(m1.ok, m1.errorMessage.c_str());
    auto m2 = clientB->submitMove(roomId, "37+e");
    CHECK_TRUE_TEXT(m2.ok, m2.errorMessage.c_str());

    /* Game chat */
    auto chatAll = clientA->postGameChat(roomId, "ALL", "Good luck!");
    CHECK_TRUE_TEXT(chatAll.ok, chatAll.errorMessage.c_str());

    /* Pause / Resume & Draw offer / decline */
    auto reqPause = clientA->submitAction(roomId, "request_pause");
    CHECK_TRUE_TEXT(reqPause.ok, reqPause.errorMessage.c_str());
    auto accPause = clientB->submitAction(roomId, "accept_pause");
    CHECK_TRUE_TEXT(accPause.ok, accPause.errorMessage.c_str());
    auto resumeRes = clientA->submitAction(roomId, "resume");
    CHECK_TRUE_TEXT(resumeRes.ok, resumeRes.errorMessage.c_str());

    auto offerDraw = clientA->submitAction(roomId, "offer_draw");
    CHECK_TRUE_TEXT(offerDraw.ok, offerDraw.errorMessage.c_str());
    auto decDraw = clientB->submitAction(roomId, "decline_draw");
    CHECK_TRUE_TEXT(decDraw.ok, decDraw.errorMessage.c_str());

    /* Host moderation: update settings, mute observer C, ban observer C */
    auto settingsRes = clientA->updateRoomSettings(
        roomId, "OPEN", "ALL_ALLOWED");
    CHECK_TRUE_TEXT(settingsRes.ok, settingsRes.errorMessage.c_str());

    auto muteRes = clientA->muteParticipant(roomId, uuidC, true, true);
    CHECK_TRUE_TEXT(muteRes.ok, muteRes.errorMessage.c_str());

    auto banRes = clientA->banObserver(roomId, uuidC, "Disruptive");
    CHECK_TRUE_TEXT(banRes.ok, banRes.errorMessage.c_str());

    /* Resign game -> game archived */
    auto resignRes = clientB->resignGame(roomId);
    CHECK_TRUE_TEXT(resignRes.ok, resignRes.errorMessage.c_str());

    /* Rematch request & accept */
    auto reqRematch = clientA->submitAction(roomId, "request_rematch");
    CHECK_TRUE_TEXT(reqRematch.ok, reqRematch.errorMessage.c_str());
    auto accRematch = clientB->submitAction(roomId, "accept_rematch");
    CHECK_TRUE_TEXT(accRematch.ok, accRematch.errorMessage.c_str());

    /* Archive list, get, and .octi transcript download */
    auto archiveList = clientA->listArchivedGames();
    CHECK_TRUE_TEXT(archiveList.ok, archiveList.errorMessage.c_str());
    CHECK_TRUE(archiveList.data.total >= 1);

    auto archivedGame = clientA->fetchArchivedGame(roomId);
    CHECK_TRUE_TEXT(archivedGame.ok, archivedGame.errorMessage.c_str());
    STRCMP_EQUAL(roomId.c_str(), archivedGame.data.gameId.c_str());

    auto octiFile = clientA->downloadOctiTranscript(roomId);
    CHECK_TRUE_TEXT(octiFile.ok, octiFile.errorMessage.c_str());
    CHECK_TRUE(octiFile.data.find("octi.transcript.v3") != std::string::npos);

    clientB->stopEventStreams();
    CHECK_FALSE(clientB->isGameEventStreamActive());
}

TEST(OctiNetworkClientTest, RateLimit429RetryAfterPropagation)
{
    HttpServerConfig cfg = defaultTestServerConfig();
    cfg.ssoRateLimitMax = 2;
    cfg.ssoRateLimitWindowMs = 60 * 1000;
    LiveServerEnv env(cfg);

    auto client = OctiNetworkClient::forHermeticTest(env.port());
    std::string tok = makeToken("google", "sub-ratelimit", "RateUser");

    auto r1 = client->loginWithSso("google", tok, "RateUser");
    CHECK_TRUE(r1.ok);
    auto r2 = client->loginWithSso("google", tok);
    CHECK_TRUE(r2.ok);
    auto r3 = client->loginWithSso("google", tok);
    CHECK_FALSE(r3.ok);
    LONGS_EQUAL(429, r3.httpStatus);
    CHECK_TRUE(r3.retryAfterSec >= 1);
}

TEST(OctiNetworkClientTest, HealthReportsProtocolVersionAndDriftTolerance)
{
    LiveServerEnv env;
    auto client = OctiNetworkClient::forHermeticTest(env.port());

    auto health = client->fetchHealth();
    CHECK_TRUE_TEXT(health.ok, health.errorMessage.c_str());
    LONGS_EQUAL(200, health.httpStatus);
    STRCMP_EQUAL("ok", health.data.status.c_str());
    LONGS_EQUAL(1, health.data.protocolVersion);
    LONGS_EQUAL(1, health.data.minProtocolVersion);

    auto providers = client->fetchProviders();
    CHECK_TRUE(providers.ok);
}

TEST(OctiNetworkClientTest, UpgradeRequired426WhenServerMinProtocolVersionHigher)
{
    HttpServerConfig cfg = defaultTestServerConfig();
    cfg.minProtocolVersion = 2;
    LiveServerEnv env(cfg);

    auto client = OctiNetworkClient::forHermeticTest(env.port());

    auto health = client->fetchHealth();
    CHECK_TRUE_TEXT(health.ok, health.errorMessage.c_str());
    LONGS_EQUAL(200, health.httpStatus);
    LONGS_EQUAL(2, health.data.minProtocolVersion);

    CreateTableRequest req;
    req.mode = "2P";
    auto createRes = client->createTable(req);
    CHECK_FALSE(createRes.ok);
    LONGS_EQUAL(426, createRes.httpStatus);
    STRCMP_EQUAL("UPGRADE_REQUIRED", createRes.errorCode.c_str());

    std::atomic<int> eventsSeen{0};
    client->startGameEventStream("non-existent-room", 0, [&](const SseEvent &) {
        eventsSeen++;
    });
    for (int i = 0; i < 50; ++i) {
        if (!client->isGameEventStreamActive()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    CHECK_FALSE(client->isGameEventStreamActive());
    LONGS_EQUAL(0, eventsSeen.load());
}

TEST(OctiNetworkClientTest, LiveServerRestartPreservesGameSseAndAllowsMoveContinuation)
{
    LiveServerEnv env;
    auto clientA = OctiNetworkClient::forHermeticTest(env.port());
    auto clientB = OctiNetworkClient::forHermeticTest(env.port());

    auto loginA = clientA->loginWithSso(
        "google", makeToken("google", "sub-live-a", "AliceLive"), "AliceLive");
    auto loginB = clientB->loginWithSso(
        "google", makeToken("google", "sub-live-b", "BobLive"), "BobLive");
    CHECK_TRUE(loginA.ok && loginB.ok);

    CreateTableRequest req;
    req.mode = "2P";
    req.timeControlEnabled = true;
    req.initialMs = 600000;
    req.incrementMs = 0;
    auto createRes = clientA->createTable(req);
    CHECK_TRUE_TEXT(createRes.ok, createRes.errorMessage.c_str());
    std::string roomId = createRes.data.roomId;

    auto joinRes = clientB->joinTable(roomId, 2);
    CHECK_TRUE_TEXT(joinRes.ok, joinRes.errorMessage.c_str());

    std::mutex sseMutex;
    std::vector<SseEvent> eventsA;
    std::vector<SseEvent> eventsB;

    clientA->startGameEventStream(roomId, 0, [&](const SseEvent &ev) {
        std::lock_guard<std::mutex> lk(sseMutex);
        eventsA.push_back(ev);
    });
    clientB->startGameEventStream(roomId, 0, [&](const SseEvent &ev) {
        std::lock_guard<std::mutex> lk(sseMutex);
        eventsB.push_back(ev);
    });

    for (int i = 0; i < 50; ++i) {
        if (clientA->isGameEventStreamActive() && clientB->isGameEventStreamActive()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    CHECK_TRUE(clientA->isGameEventStreamActive());
    CHECK_TRUE(clientB->isGameEventStreamActive());

    // Move 1: Alice moves "73+a"
    auto mv1 = clientA->submitMove(roomId, "73+a");
    CHECK_TRUE_TEXT(mv1.ok, mv1.errorMessage.c_str());

    bool seenMv1 = false;
    for (int i = 0; i < 50; ++i) {
        {
            std::lock_guard<std::mutex> lk(sseMutex);
            for (const auto &ev : eventsB) {
                if (ev.dataJson.find("73+a") != std::string::npos) {
                    seenMv1 = true;
                    break;
                }
            }
        }
        if (seenMv1) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    CHECK_TRUE(seenMv1);

    // Move 2: Bob moves "77+e"
    auto mv2 = clientB->submitMove(roomId, "77+e");
    CHECK_TRUE_TEXT(mv2.ok, mv2.errorMessage.c_str());

    bool seenMv2 = false;
    for (int i = 0; i < 50; ++i) {
        {
            std::lock_guard<std::mutex> lk(sseMutex);
            for (const auto &ev : eventsA) {
                if (ev.dataJson.find("77+e") != std::string::npos) {
                    seenMv2 = true;
                    break;
                }
            }
        }
        if (seenMv2) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    CHECK_TRUE(seenMv2);

    auto viewBefore = clientA->fetchGameView(roomId);
    CHECK_TRUE(viewBefore.ok);
    LONGS_EQUAL(1, viewBefore.data.turnSeat);
    LONGS_EQUAL(2, viewBefore.data.plyCount);

    // Live restart the server on the exact same port and SQLite DB!
    env.restart();

    // Give SSE clients time to reconnect automatically
    for (int i = 0; i < 50; ++i) {
        if (clientA->isGameEventStreamActive() && clientB->isGameEventStreamActive()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    CHECK_TRUE(clientA->isGameEventStreamActive());
    CHECK_TRUE(clientB->isGameEventStreamActive());

    // Move 3: Alice submits next move "73a-74" after restart
    auto mv3 = clientA->submitMove(roomId, "73a-74");
    CHECK_TRUE_TEXT(mv3.ok, mv3.errorMessage.c_str());

    bool seenMv3 = false;
    for (int i = 0; i < 50; ++i) {
        {
            std::lock_guard<std::mutex> lk(sseMutex);
            for (const auto &ev : eventsB) {
                if (ev.dataJson.find("73a-74") != std::string::npos) {
                    seenMv3 = true;
                    break;
                }
            }
        }
        if (seenMv3) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    CHECK_TRUE(seenMv3);

    auto viewAfter = clientA->fetchGameView(roomId);
    CHECK_TRUE(viewAfter.ok);
    LONGS_EQUAL(3, viewAfter.data.plyCount);
    LONGS_EQUAL(2, viewAfter.data.turnSeat);
    CHECK_TRUE(viewAfter.data.seats[0].clockMs > 0);

    clientA->stopEventStreams();
    clientB->stopEventStreams();
}

TEST(OctiNetworkClientTest, TransientTransportRetryRecoversAcrossServerDowntime)
{
    LiveServerEnv env;
    auto client = OctiNetworkClient::forHermeticTest(env.port());

    auto health1 = client->fetchHealth();
    CHECK_TRUE(health1.ok);

    env.stop();

    std::thread restarter([&env]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        env.startAgain();
    });

    auto health2 = client->fetchHealth();
    restarter.join();

    CHECK_TRUE_TEXT(health2.ok, health2.errorMessage.c_str());
    LONGS_EQUAL(200, health2.httpStatus);
}

TEST(OctiNetworkClientTest, Port443ClientConstructionDoesNotThrowAndFallsBackSafely)
{
    /* Construct standard client targeting port 443 */
    OctiNetworkClient prodClient;
    LONGS_EQUAL(443, prodClient.port());
    STRCMP_EQUAL("octi.selfso.com", prodClient.host().c_str());

    /* Calling fetchHealth() must NEVER throw std::invalid_argument ("'https' scheme is not supported.") */
    bool threwException = false;
    try {
        auto res = prodClient.fetchHealth();
        (void)res;
    } catch (...) {
        threwException = true;
    }
    CHECK_FALSE_TEXT(threwException, "OctiNetworkClient targeting port 443 must not throw exceptions");
}

TEST(OctiNetworkClientTest, UploadCrashReportAndOctiCrashReporterDurableSpoolAndFlush)
{
    LiveServerEnv env;
    auto client = OctiNetworkClient::forHermeticTest(env.port());

    /* 1. Direct upload of CrashReportRequest to server */
    CrashReportRequest req;
    req.crashId = "crash_test_hermetic_01";
    req.createdAt = 1761837000;
    req.platform = "linux-x86_64";
    req.appVersion = "2.1.2";
    req.osVersion = "Linux";
    req.exceptionType = "std::runtime_error";
    req.exceptionMessage = "Hermetic test crash";
    req.threadName = "WorkerThread";
    req.stackTrace = "#0 test+0x1000\n#1 test+0x2000";
    req.contextJson = "{\"match_mode\":\"LOCAL_BOT\",\"ply_count\":10}";
    req.installId = "install_uuid_hermetic_999";

    auto resp1 = client->uploadCrashReport(req);
    CHECK_TRUE_TEXT(resp1.ok, resp1.errorMessage.c_str());
    LONGS_EQUAL(201, resp1.httpStatus);
    CHECK_FALSE(resp1.deduplicated);
    STRCMP_EQUAL("crash_test_hermetic_01", resp1.crashId.c_str());

    /* Deduplication on repeat upload */
    auto resp2 = client->uploadCrashReport(req);
    CHECK_TRUE_TEXT(resp2.ok, resp2.errorMessage.c_str());
    LONGS_EQUAL(200, resp2.httpStatus);
    CHECK_TRUE(resp2.deduplicated);

    /* 2. OctiCrashReporter spooling and flushing */
    std::string testDir = env.dir + "/crash_reporter_test";
    std::filesystem::remove_all(testDir);
    octi::desktop::OctiCrashReporter::setStorageRootDir(testDir);

    octi::desktop::OctiCrashReporter::updateGameContext("LOCAL_BOT", 15, "test_fen");
    std::string recordedFile = octi::desktop::OctiCrashReporter::recordHandledException(
        "SimulatedThread", "std::out_of_range", "Index out of range",
        "{\"ply\":15}", "install_uuid_hermetic_999");
    CHECK_FALSE(recordedFile.empty());
    CHECK_TRUE(std::filesystem::exists(recordedFile));
    LONGS_EQUAL(1, static_cast<long>(octi::desktop::OctiCrashReporter::listPendingCrashFiles().size()));

    std::string transcriptJson = "{\"game_id\":\"game_spool_01\",\"fen\":\"test_fen\",\"num_moves\":4}";
    std::string spooledFile = octi::desktop::OctiCrashReporter::spoolTrainingTranscript(
        "game_spool_01", transcriptJson, "install_uuid_hermetic_999");
    CHECK_FALSE(spooledFile.empty());
    CHECK_TRUE(std::filesystem::exists(spooledFile));
    LONGS_EQUAL(1, static_cast<long>(octi::desktop::OctiCrashReporter::listPendingTranscriptFiles().size()));

    /* Offline retention: when server is down, spooled files remain intact */
    env.stop();
    int flushedOfflineCrash = octi::desktop::OctiCrashReporter::flushPendingCrashReports(*client, "install_uuid_hermetic_999");
    LONGS_EQUAL(0, flushedOfflineCrash);
    LONGS_EQUAL(1, static_cast<long>(octi::desktop::OctiCrashReporter::listPendingCrashFiles().size()));

    int flushedOfflineTx = octi::desktop::OctiCrashReporter::flushPendingTranscripts(*client, "install_uuid_hermetic_999");
    LONGS_EQUAL(0, flushedOfflineTx);
    LONGS_EQUAL(1, static_cast<long>(octi::desktop::OctiCrashReporter::listPendingTranscriptFiles().size()));

    /* Online flush: when server is back up, flush uploads and deletes spooled files */
    env.startAgain();
    int flushedOnlineCrash = octi::desktop::OctiCrashReporter::flushPendingCrashReports(*client, "install_uuid_hermetic_999");
    LONGS_EQUAL(1, flushedOnlineCrash);
    LONGS_EQUAL(0, static_cast<long>(octi::desktop::OctiCrashReporter::listPendingCrashFiles().size()));

    int flushedOnlineTx = octi::desktop::OctiCrashReporter::flushPendingTranscripts(*client, "install_uuid_hermetic_999");
    LONGS_EQUAL(1, flushedOnlineTx);
    LONGS_EQUAL(0, static_cast<long>(octi::desktop::OctiCrashReporter::listPendingTranscriptFiles().size()));

    std::filesystem::remove_all(testDir);
}

/* ==================================================================== */
/* Group 3: P2pVoiceEngineTest                                          */
/* ==================================================================== */

TEST_GROUP(P2pVoiceEngineTest)
{
};

TEST(P2pVoiceEngineTest, TwoPeersSignalingAndPushToTalkSineWaveAudioTransfer)
{
    LiveServerEnv env;
    auto clientA = OctiNetworkClient::forHermeticTest(env.port());
    auto clientB = OctiNetworkClient::forHermeticTest(env.port());

    auto loginA = clientA->loginWithSso(
        "google", makeToken("google", "sub-voice-a", "VoiceA"), "VoiceA");
    auto loginB = clientB->loginWithSso(
        "google", makeToken("google", "sub-voice-b", "VoiceB"), "VoiceB");
    CHECK_TRUE(loginA.ok && loginB.ok);

    std::string uuidA = loginA.data.profile.uuid;
    std::string uuidB = loginB.data.profile.uuid;

    CreateTableRequest req;
    req.mode = "2P";
    req.voicePolicy = "PLAYERS_ONLY";
    auto table = clientA->createTable(req);
    CHECK_TRUE(table.ok);
    std::string roomId = table.data.roomId;
    CHECK_TRUE(clientB->joinTable(roomId, 2).ok);

    P2pVoiceEngine voiceA(clientA.get());
    P2pVoiceEngine voiceB(clientB.get());
    voiceA.setHardwareAudioEnabled(false);
    voiceB.setHardwareAudioEnabled(false);

    std::string err;
    CHECK_TRUE_TEXT(voiceA.joinVoiceRoom(roomId, uuidA, err), err.c_str());
    CHECK_TRUE_TEXT(voiceB.joinVoiceRoom(roomId, uuidB, err), err.c_str());
    CHECK_TRUE(voiceA.localUdpPort() > 0);
    CHECK_TRUE(voiceB.localUdpPort() > 0);
    CHECK_FALSE(voiceA.isPushToTalkActive());
    CHECK_FALSE(voiceB.isPushToTalkActive());

    /* Exchange SDP_OFFER -> SDP_ANSWER + ICE_CANDIDATE via server events */
    std::uint64_t seqA = 0;
    std::uint64_t seqB = 0;
    CHECK_TRUE(voiceA.initiatePeerOffer(uuidB));

    auto pumpSignaling = [&]() {
        auto evsB = clientB->pollGameEvents(roomId, seqB);
        if (evsB.ok) {
            for (const auto &ev : evsB.data) {
                seqB = std::max(seqB, ev.id);
                voiceB.handleGameSseEvent(ev);
            }
        }
        auto evsA = clientA->pollGameEvents(roomId, seqA);
        if (evsA.ok) {
            for (const auto &ev : evsA.data) {
                seqA = std::max(seqA, ev.id);
                voiceA.handleGameSseEvent(ev);
            }
        }
    };

    pumpSignaling();
    pumpSignaling();

    CHECK_TRUE(voiceA.isPeerConnected(uuidB));
    CHECK_TRUE(voiceB.isPeerConnected(uuidA));

    /* Synthesize a 440 Hz 16 kHz mono 20 ms sine wave frame (320 samples) */
    std::vector<std::int16_t> sineFrame(VoiceUdpPacket::kPayloadSamples);
    for (std::size_t i = 0; i < sineFrame.size(); ++i) {
        double phase = 2.0 * M_PI * 440.0 * static_cast<double>(i) / 16000.0;
        sineFrame[i] = static_cast<std::int16_t>(std::sin(phase) * 14000.0);
    }

    /* 1. While Push-to-Talk is INACTIVE, zero UDP packets may be transmitted */
    for (int i = 0; i < 5; ++i) {
        LONGS_EQUAL(0, static_cast<long>(
                           voiceA.transmitPcmFrame20Ms(sineFrame.data())));
    }
    LONGS_EQUAL(0, static_cast<long>(voiceA.totalPacketsSent()));
    LONGS_EQUAL(0, static_cast<long>(voiceB.totalPacketsReceived()));

    /* 2. Press Push-to-Talk -> MIC_UNMUTED posted, UDP audio flows peer-to-peer */
    CHECK_TRUE(voiceA.setPushToTalkActive(true));
    CHECK_TRUE(voiceA.isPushToTalkActive());

    for (int i = 0; i < 8; ++i) {
        LONGS_EQUAL(1, static_cast<long>(
                           voiceA.transmitPcmFrame20Ms(sineFrame.data())));
    }
    LONGS_EQUAL(8, static_cast<long>(voiceA.totalPacketsSent()));

    for (int wait = 0; wait < 40 && voiceB.packetsReceivedFrom(uuidA) < 4;
         ++wait) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK_TRUE(voiceB.packetsReceivedFrom(uuidA) >= 4);
    CHECK_TRUE(voiceB.lastReceivedRmsFrom(uuidA) > 6000.0);

    std::vector<std::int16_t> decoded = voiceB.popDecodedFrameFrom(uuidA);
    LONGS_EQUAL(320, static_cast<long>(decoded.size()));

    /* 3. Release Push-to-Talk -> MIC_MUTED posted, UDP transmission stops */
    CHECK_TRUE(voiceA.setPushToTalkActive(false));
    CHECK_FALSE(voiceA.isPushToTalkActive());
    std::uint64_t sentBeforeReleaseCheck = voiceA.totalPacketsSent();
    LONGS_EQUAL(0, static_cast<long>(
                       voiceA.transmitPcmFrame20Ms(sineFrame.data())));
    LONGS_EQUAL(static_cast<long>(sentBeforeReleaseCheck),
                static_cast<long>(voiceA.totalPacketsSent()));

    voiceA.leaveVoiceRoom();
    voiceB.leaveVoiceRoom();
    CHECK_FALSE(voiceA.isInVoiceRoom());
    CHECK_FALSE(voiceB.isInVoiceRoom());
}

TEST(P2pVoiceEngineTest, ModerationMuteBanAndPolicyDisabledCutoff)
{
    LiveServerEnv env;
    auto clientA = OctiNetworkClient::forHermeticTest(env.port());
    auto clientB = OctiNetworkClient::forHermeticTest(env.port());

    auto loginA = clientA->loginWithSso(
        "google", makeToken("google", "sub-mod-a", "ModA"), "ModA");
    auto loginB = clientB->loginWithSso(
        "google", makeToken("google", "sub-mod-b", "ModB"), "ModB");
    CHECK_TRUE(loginA.ok && loginB.ok);
    std::string uuidA = loginA.data.profile.uuid;
    std::string uuidB = loginB.data.profile.uuid;

    CreateTableRequest req;
    req.mode = "2P";
    auto table = clientA->createTable(req);
    std::string roomId = table.data.roomId;
    CHECK_TRUE(clientB->joinTable(roomId, 2).ok);

    P2pVoiceEngine voiceA(clientA.get());
    P2pVoiceEngine voiceB(clientB.get());
    std::string err;
    CHECK_TRUE(voiceA.joinVoiceRoom(roomId, uuidA, err));
    CHECK_TRUE(voiceB.joinVoiceRoom(roomId, uuidB, err));

    std::uint64_t seqA = 0;
    std::uint64_t seqB = 0;
    CHECK_TRUE(voiceA.initiatePeerOffer(uuidB));
    for (int i = 0; i < 2; ++i) {
        auto evsB = clientB->pollGameEvents(roomId, seqB);
        for (const auto &ev : evsB.data) {
            seqB = std::max(seqB, ev.id);
            voiceB.handleGameSseEvent(ev);
        }
        auto evsA = clientA->pollGameEvents(roomId, seqA);
        for (const auto &ev : evsA.data) {
            seqA = std::max(seqA, ev.id);
            voiceA.handleGameSseEvent(ev);
        }
    }
    CHECK_TRUE(voiceB.isPeerConnected(uuidA));

    /* 1. Forged UDP packet with wrong session key is dropped */
    VoiceUdpPacket forged;
    std::memcpy(forged.magic, "OCTV", 4);
    forged.seq = 99;
    forged.timestampMs = 12345;
    std::memset(forged.payload, 0x7F, sizeof(forged.payload));
    forged.authTag = P2pVoiceEngine::computeAuthTag(
        forged.seq, forged.timestampMs, forged.payload, "wrong_session_key");
    std::vector<std::uint8_t> wire = P2pVoiceEngine::serializePacket(forged);

    int rawSock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    CHECK_TRUE(rawSock >= 0);
    sockaddr_in dst;
    std::memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    dst.sin_port = htons(static_cast<std::uint16_t>(voiceB.localUdpPort()));
    ::sendto(rawSock, wire.data(), wire.size(), 0,
             reinterpret_cast<const sockaddr *>(&dst), sizeof(dst));
    ::close(rawSock);

    for (int i = 0; i < 30 && voiceB.totalInvalidPacketsDropped() == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    CHECK_TRUE(voiceB.totalInvalidPacketsDropped() >= 1);
    LONGS_EQUAL(0, static_cast<long>(voiceB.totalPacketsReceived()));

    /* 2. Server USER_MUTED (mute_voice=true) forces PTT inactive */
    CHECK_TRUE(voiceA.setPushToTalkActive(true));
    CHECK_TRUE(voiceA.isPushToTalkActive());

    SseEvent muteEv;
    muteEv.event = "USER_MUTED";
    muteEv.dataJson = Json({
        {"target", uuidA},
        {"mute_chat", false},
        {"mute_voice", true}
    }).dump();
    voiceA.handleGameSseEvent(muteEv);

    CHECK_TRUE(voiceA.isVoiceMutedByServer());
    CHECK_FALSE(voiceA.isPushToTalkActive());
    CHECK_FALSE(voiceA.setPushToTalkActive(true));

    /* 3. POLICY_UPDATED (voice_policy = DISABLED) disables voice and clears peers */
    SseEvent policyEv;
    policyEv.event = "POLICY_UPDATED";
    policyEv.dataJson = Json({
        {"observer_policy", "OPEN"},
        {"voice_policy", "DISABLED"}
    }).dump();
    voiceB.handleGameSseEvent(policyEv);
    CHECK_TRUE(voiceB.isVoicePolicyDisabled());
    LONGS_EQUAL(0, static_cast<long>(voiceB.activePeerCount()));

    /* 4. OBSERVER_BANNED tears down banned participant's voice session */
    SseEvent banEv;
    banEv.event = "OBSERVER_BANNED";
    banEv.dataJson = Json({{"target", uuidA}}).dump();
    voiceA.handleGameSseEvent(banEv);
    CHECK_FALSE(voiceA.isInVoiceRoom());
}

TEST(P2pVoiceEngineTest, JitterBufferBoundingAndWrap)
{
    LiveServerEnv env;
    auto clientA = OctiNetworkClient::forHermeticTest(env.port());
    auto clientB = OctiNetworkClient::forHermeticTest(env.port());

    auto loginA = clientA->loginWithSso(
        "google", makeToken("google", "sub-jb-a", "JitterA"), "JitterA");
    auto loginB = clientB->loginWithSso(
        "google", makeToken("google", "sub-jb-b", "JitterB"), "JitterB");
    CHECK_TRUE(loginA.ok && loginB.ok);
    std::string uuidA = loginA.data.profile.uuid;
    std::string uuidB = loginB.data.profile.uuid;

    auto table = clientA->createTable();
    std::string roomId = table.data.roomId;
    clientB->joinTable(roomId, 2);

    P2pVoiceEngine voiceA(clientA.get());
    P2pVoiceEngine voiceB(clientB.get());
    std::string err;
    voiceA.joinVoiceRoom(roomId, uuidA, err);
    voiceB.joinVoiceRoom(roomId, uuidB, err);

    std::uint64_t seqA = 0;
    std::uint64_t seqB = 0;
    voiceA.initiatePeerOffer(uuidB);
    for (int i = 0; i < 2; ++i) {
        auto evsB = clientB->pollGameEvents(roomId, seqB);
        for (const auto &ev : evsB.data) {
            seqB = std::max(seqB, ev.id);
            voiceB.handleGameSseEvent(ev);
        }
        auto evsA = clientA->pollGameEvents(roomId, seqA);
        for (const auto &ev : evsA.data) {
            seqA = std::max(seqA, ev.id);
            voiceA.handleGameSseEvent(ev);
        }
    }

    voiceA.setPushToTalkActive(true);
    std::vector<std::int16_t> frame(320, 4096);
    for (int i = 0; i < 40; ++i) {
        voiceA.injectTestPcmFrame(frame.data(), frame.size());
    }
    for (int w = 0; w < 50 && voiceB.packetsReceivedFrom(uuidA) < 40; ++w) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    int poppedCount = 0;
    while (!voiceB.popDecodedFrameFrom(uuidA).empty()) {
        ++poppedCount;
    }
    CHECK_TRUE(poppedCount > 0 && poppedCount <= 32);
}

TEST(P2pVoiceEngineTest, RapidCheckUlawMonotonicityAndPacketFramingFuzz)
{
    bool prop1 = rc::check(
        "G.711 u-law codec is strictly monotonic and has bounded quantization error",
        [](std::int16_t a, std::int16_t b) {
            if (a > b) {
                std::swap(a, b);
            }
            std::uint8_t ua = P2pVoiceEngine::linear16ToUlaw(a);
            std::uint8_t ub = P2pVoiceEngine::linear16ToUlaw(b);
            std::int16_t da = P2pVoiceEngine::ulawToLinear16(ua);
            std::int16_t db = P2pVoiceEngine::ulawToLinear16(ub);
            RC_ASSERT(da <= db);
            RC_ASSERT(std::abs(static_cast<int>(da) - static_cast<int>(a)) <= 1250);
            RC_ASSERT(std::abs(static_cast<int>(db) - static_cast<int>(b)) <= 1250);
        });
    CHECK_TRUE(prop1);

    bool prop2 = rc::check(
        "VoiceUdpPacket serializer/parser authenticates valid packets and rejects mutations",
        [](std::uint32_t seq,
           std::uint32_t ts,
           std::uint8_t fillByte,
           std::size_t mutateOffset) {
            std::string key = "0123456789abcdef0123456789abcdef";
            VoiceUdpPacket pkt;
            std::memcpy(pkt.magic, "OCTV", 4);
            pkt.seq = seq;
            pkt.timestampMs = ts;
            std::memset(pkt.payload, fillByte, VoiceUdpPacket::kPayloadSamples);
            pkt.authTag = P2pVoiceEngine::computeAuthTag(
                pkt.seq, pkt.timestampMs, pkt.payload, key);

            std::vector<std::uint8_t> wire =
                P2pVoiceEngine::serializePacket(pkt);
            RC_ASSERT(wire.size() == VoiceUdpPacket::kPacketBytes);

            VoiceUdpPacket parsed;
            RC_ASSERT(P2pVoiceEngine::parseAndVerifyPacket(
                wire.data(), wire.size(), key, parsed));
            RC_ASSERT(parsed.seq == seq);
            RC_ASSERT(parsed.timestampMs == ts);
            RC_ASSERT(parsed.payload[0] == fillByte);

            /* Any single-byte mutation must fail authentication or header check */
            std::size_t idx = mutateOffset % wire.size();
            wire[idx] ^= 0x5AU;
            VoiceUdpPacket rejected;
            RC_ASSERT(!P2pVoiceEngine::parseAndVerifyPacket(
                wire.data(), wire.size(), key, rejected));
        });
    CHECK_TRUE(prop2);

    bool prop3 = rc::check(
        "Arbitrary random byte buffers never parse as valid authenticated packets",
        [](const std::vector<std::uint8_t> &randomBytes) {
            VoiceUdpPacket out;
            bool ok = P2pVoiceEngine::parseAndVerifyPacket(
                randomBytes.data(), randomBytes.size(),
                "session_key_secret_abc", out);
            RC_ASSERT(!ok);
        });
    CHECK_TRUE(prop3);
}

int main(int argc, char **argv)
{
    ::signal(SIGPIPE, SIG_IGN);
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
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
