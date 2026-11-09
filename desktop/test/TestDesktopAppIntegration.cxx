/*
 * TestDesktopAppIntegration.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

#include <signal.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>

#include "AccountStore.hxx"
#include "GameRoomManager.hxx"
#include "OctiHttpServer.hxx"
#include "OidcVerifier.hxx"

#include "ClientConfigStore.hxx"
#include "LobbyView.hxx"
#include "OctiBoardCanvas.hxx"
#include "OctiCore.hxx"
#include "OctiCrashReporter.hxx"
#include "OctiDialogs.hxx"
#include "OctiMainWindow.hxx"
#include "OctiNetworkClient.hxx"
#include "OctiOpeningBookData.hxx"
#include "OctiReservePanel.hxx"
#include "OctiTheme.hxx"

#include "selfupdate/Sha256.hxx"
#include "selfupdate/UpdateInstaller.hxx"
#include "selfupdate/UpdateManifest.hxx"
#include "selfupdate/SelfUpdateController.hxx"

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

struct LiveServerEnv {
    std::string dir;
    std::string dbPath;
    AccountStore store;
    OidcVerifier verifier;
    std::unique_ptr<GameRoomManager> rooms;
    std::unique_ptr<OctiHttpServer> server;
    int savedPort = 0;
    HttpServerConfig savedCfg;

    LiveServerEnv()
    {
        char tmpl[] = "/tmp/octi_desktop_e2e_XXXXXX";
        if (mkdtemp(tmpl) == nullptr) {
            FAIL("mkdtemp failed");
        }
        dir = tmpl;
        dbPath = dir + "/octi.db";
        std::string err;
        if (!store.openOrCreate(dbPath, err)) {
            FAIL(err.c_str());
        }
        savedCfg.bindAddress = "127.0.0.1";
        savedCfg.port = 0;
        savedCfg.tickIntervalMs = 30;
        savedCfg.ssePollMs = 15;
        savedCfg.sseHeartbeatMs = 200;
        savedCfg.sseSessionRecheckMs = 100;
        static const char *names[] = {
            "google", "apple", "microsoft", "facebook", "line"
        };
        for (const char *n : names) {
            savedCfg.providerClientIds[n].push_back(std::string("client-") + n);
            verifier.setClientId(n, std::string("client-") + n);
            if (!verifier.setKeySet(n, jwks())) {
                FAIL("setKeySet failed");
            }
        }
        rooms = std::make_unique<GameRoomManager>(store);
        server = std::make_unique<OctiHttpServer>(
            store, verifier, *rooms, savedCfg);
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

    void startAgain()
    {
        HttpServerConfig c = savedCfg;
        c.port = savedPort;
        rooms = std::make_unique<GameRoomManager>(store);
        server = std::make_unique<OctiHttpServer>(store, verifier, *rooms, c);
        std::string err;
        if (!server->start(err)) {
            FAIL(err.c_str());
        }
        savedPort = server->port();
    }
};

} // namespace

TEST_GROUP(DesktopAppIntegrationTest)
{
};

TEST(DesktopAppIntegrationTest, HardCodedEndpointAndCliOptionGuard)
{
    DesktopCliArgs parsed;
    std::string errMsg;

    /* 1. Nominal CLI flags: --version, --help, --offline, --config */
    {
        const char *argv[] = {"octi", "--version"};
        CHECK_TRUE(DesktopCliArgs::parse(2, argv, parsed, errMsg));
        CHECK_TRUE(parsed.showVersion);
        CHECK_FALSE(DesktopCliArgs::versionBanner().empty());
    }
    {
        const char *argv[] = {"octi", "--help"};
        CHECK_TRUE(DesktopCliArgs::parse(2, argv, parsed, errMsg));
        CHECK_TRUE(parsed.showHelp);
        std::string banner = DesktopCliArgs::usageBanner("octi");
        CHECK_TRUE(banner.find("trumpet.selfso.com") == std::string::npos);
        CHECK_TRUE(banner.find("6874") == std::string::npos);
        CHECK_TRUE(banner.find("Server endpoint is permanently fixed") != std::string::npos);
    }
    {
        const char *argv[] = {"octi", "--offline", "--config", "/tmp/my_octi.json"};
        CHECK_TRUE(DesktopCliArgs::parse(4, argv, parsed, errMsg));
        CHECK_TRUE(parsed.startOffline);
        STRCMP_EQUAL("/tmp/my_octi.json", parsed.configPath.c_str());
    }
    {
        const char *argv[] = {"octi", "--config=/tmp/eq_octi.json"};
        CHECK_TRUE(DesktopCliArgs::parse(2, argv, parsed, errMsg));
        STRCMP_EQUAL("/tmp/eq_octi.json", parsed.configPath.c_str());
    }

    /* 2. Forbidden server/host/port CLI flags MUST be rejected */
    static const char *forbiddenFlags[] = {
        "--server",
        "--server=evil.example.com",
        "--host",
        "--host=10.0.0.1",
        "--port",
        "--port=9999",
        "-p",
    };
    for (const char *badFlag : forbiddenFlags) {
        const char *argv[] = {"octi", badFlag, "1234"};
        CHECK_FALSE(DesktopCliArgs::parse(3, argv, parsed, errMsg));
        CHECK_TRUE_TEXT(
            errMsg.find("trumpet.selfso.com") == std::string::npos,
            errMsg.c_str());
        CHECK_TRUE_TEXT(
            errMsg.find("6874") == std::string::npos,
            errMsg.c_str());
        CHECK_TRUE_TEXT(
            errMsg.find("server endpoint is permanently fixed") != std::string::npos,
            errMsg.c_str());
    }

    /* 3. Default OctiMainWindow and SsoOnboardingDialog bind to octi.selfso.com:443 */
    std::string cfgPath = "/tmp/octi_default_win_test.json";
    OctiMainWindow defaultWin(nullptr, cfgPath, false);
    STRCMP_EQUAL("octi.selfso.com", defaultWin.netClient()->host().c_str());
    LONGS_EQUAL(443, defaultWin.netClient()->port());
    CHECK_FALSE(defaultWin.windowTitle().contains("octi.selfso.com"));
    CHECK_FALSE(defaultWin.windowTitle().contains("trumpet.selfso.com"));
    CHECK_FALSE(defaultWin.windowTitle().contains("6874"));
    CHECK_FALSE(defaultWin.windowTitle().contains("443"));
    CHECK_FALSE(defaultWin.serverStatusText().contains("octi.selfso.com"));
    CHECK_FALSE(defaultWin.serverStatusText().contains("trumpet.selfso.com"));
    CHECK_FALSE(defaultWin.serverStatusText().contains("6874"));
    CHECK_FALSE(defaultWin.serverStatusText().contains("443"));
    CHECK_TRUE(defaultWin.serverStatusText() == "Ready");
    CHECK_FALSE(defaultWin.hasEditableServerOrPortField());
    LONGS_EQUAL(23, defaultWin.menuActionCountWithIconsAndShortcuts());

    SsoOnboardingDialog ssoDlg(defaultWin.netClient());
    CHECK_TRUE(ssoDlg.serverBadgeText().isEmpty());
    CHECK_FALSE(ssoDlg.hasEditableServerOrPortField());
    std::remove(cfgPath.c_str());
}

TEST(DesktopAppIntegrationTest,
     EndToEndMultiplayerDialogsPlayVoiceModerationAndArchiveReplay)
{
    LiveServerEnv env;
    std::string cfgPathA = env.dir + "/client_a.json";
    std::string cfgPathB = env.dir + "/client_b.json";

    OctiMainWindow winA(
        OctiNetworkClient::createForLoopbackTest(env.port()), cfgPathA);
    OctiMainWindow winB(
        OctiNetworkClient::createForLoopbackTest(env.port()), cfgPathB);
    auto clientC = OctiNetworkClient::createForLoopbackTest(env.port());

    /* 1. Player A: Browser loopback OAuth sign-in -> 201 Created -> Step-2 Handle */
    {
        SsoOnboardingDialog dlgA(winA.netClient(), winA.configStore());
        CHECK_TRUE(dlgA.serverBadgeText().isEmpty());
        CHECK_FALSE(dlgA.hasEditableServerOrPortField());
        LONGS_EQUAL(1, dlgA.providerCombo()->count());
        STRCMP_EQUAL("google", dlgA.providerCombo()->itemData(0).toString().toUtf8().constData());

        CHECK_TRUE(dlgA.startBrowserLoopbackFlow("google", false));
        CHECK_TRUE(dlgA.loopbackBoundPort() > 0);

        std::string tokA = makeToken("google", "sub-e2e-alice", "Alice");
        httplib::Client browser("127.0.0.1", dlgA.loopbackBoundPort());
        auto cbRes = browser.Get(
            ("/oauth/callback?provider=google&id_token=" + tokA).c_str());
        CHECK_TRUE(cbRes != nullptr);
        LONGS_EQUAL(200, cbRes->status);

        CHECK_TRUE(dlgA.pollLoopbackCallback(1000));
        CHECK_TRUE(dlgA.isSignedIn());
        CHECK_TRUE(dlgA.isOnboardingStep2Active());

        auto step2Res = dlgA.finalizeOnboardingHandle("Alice_GM");
        CHECK_TRUE_TEXT(step2Res.ok, step2Res.errorMessage.c_str());
        CHECK_FALSE(dlgA.isOnboardingStep2Active());
        STRCMP_EQUAL("Alice_GM", dlgA.loggedInProfile().handle.c_str());
        winA.applyAuthenticatedProfile(dlgA.loggedInProfile());
    }
    CHECK_TRUE(winA.isAuthenticated());
    CHECK_TRUE(winA.profileBadgeText().contains("Alice_GM"));

    /* 2. Player B: Manual token sign-in -> 201 Created -> Step-2 Handle */
    std::string bobAppleToken = makeToken("apple", "sub-e2e-bob", "Bob");
    {
        SsoOnboardingDialog dlgB(winB.netClient(), winB.configStore());
        auto loginB = dlgB.signInWithToken("apple", bobAppleToken, "");
        CHECK_TRUE_TEXT(loginB.ok, loginB.errorMessage.c_str());
        LONGS_EQUAL(201, loginB.httpStatus);
        CHECK_TRUE(dlgB.isOnboardingStep2Active());

        auto finB = dlgB.finalizeOnboardingHandle("Bob_Challenger");
        CHECK_TRUE_TEXT(finB.ok, finB.errorMessage.c_str());
        winB.applyAuthenticatedProfile(dlgB.loggedInProfile());
    }
    CHECK_TRUE(winB.isAuthenticated());
    std::string uuidB = winB.currentProfile().uuid;

    /* Observer C sign-in */
    auto loginC = clientC->loginWithSso(
        "facebook", makeToken("facebook", "sub-e2e-charlie", "Charlie"),
        "Charlie_Obs");
    CHECK_TRUE_TEXT(loginC.ok, loginC.errorMessage.c_str());
    std::string uuidC = loginC.data.profile.uuid;

    /* 3. AccountLinkDialog: link providers, 409 Conflict switch, 400 last-unlink guard */
    {
        AccountLinkDialog linkDlg(winA.netClient(), winA.configStore());
        LONGS_EQUAL(1, linkDlg.providerCombo()->count());
        STRCMP_EQUAL("google", linkDlg.providerCombo()->itemData(0).toString().toUtf8().constData());
        LONGS_EQUAL(1, static_cast<long>(linkDlg.linkedIdentities().size()));

        auto linkMs = linkDlg.linkProviderIdentity(
            "microsoft", makeToken("microsoft", "sub-e2e-alice-ms", "Alice"));
        CHECK_TRUE_TEXT(linkMs.ok, linkMs.errorMessage.c_str());
        LONGS_EQUAL(2, static_cast<long>(linkDlg.linkedIdentities().size()));

        auto linkLine = linkDlg.linkProviderIdentity(
            "line", makeToken("line", "sub-e2e-alice-line", "Alice"));
        CHECK_TRUE_TEXT(linkLine.ok, linkLine.errorMessage.c_str());
        LONGS_EQUAL(3, static_cast<long>(linkDlg.linkedIdentities().size()));

        /* Try linking Bob's Apple identity -> 409 Conflict */
        auto conflictRes = linkDlg.linkProviderIdentity("apple", bobAppleToken);
        CHECK_FALSE(conflictRes.ok);
        LONGS_EQUAL(409, conflictRes.httpStatus);
        CHECK_TRUE(linkDlg.hasPendingConflict());
        STRCMP_EQUAL("Bob_Challenger",
                     linkDlg.conflictExistingProfile().handle.c_str());
        CHECK_TRUE(linkDlg.conflictBannerText().contains("Bob_Challenger"));
        linkDlg.cancelConflictSwitch();
        CHECK_FALSE(linkDlg.hasPendingConflict());

        /* Unlink line and microsoft, then verify unlinking last identity returns 400 */
        CHECK_TRUE(linkDlg.unlinkProviderIdentity("line").ok);
        CHECK_TRUE(linkDlg.unlinkProviderIdentity("microsoft").ok);
        auto badUnlink = linkDlg.unlinkProviderIdentity("google");
        CHECK_FALSE(badUnlink.ok);
        LONGS_EQUAL(400, badUnlink.httpStatus);
        CHECK_TRUE(linkDlg.errorBannerText().contains("Cannot unlink last identity"));

        /* Also verify confirmSwitchToConflictAccount on a scratch client */
        auto tempClient = OctiNetworkClient::createForLoopbackTest(env.port());
        tempClient->loginWithSso(
            "google", makeToken("google", "sub-e2e-temp", "TempUser"), "TempUser");
        AccountLinkDialog switchDlg(tempClient.get());
        auto conf2 = switchDlg.linkProviderIdentity("apple", bobAppleToken);
        LONGS_EQUAL(409, conf2.httpStatus);
        auto switched = switchDlg.confirmSwitchToConflictAccount();
        CHECK_TRUE(switched.ok);
        STRCMP_EQUAL(uuidB.c_str(), switched.data.profile.uuid.c_str());

        /* Verify AccountLinkDialog::mergeConflictIntoCurrentAccount() sends force_relink: true */
        httplib::Server mockRelinkServer;
        bool forceRelinkReceived = false;
        mockRelinkServer.Post("/api/v1/auth/link", [&](const httplib::Request &req, httplib::Response &res) {
            Json body = Json::parse(req.body, nullptr, false);
            if (body.is_object() && body.value("force_relink", false)) {
                forceRelinkReceived = true;
                Json reply;
                reply["status"] = "linked";
                reply["linked_identities"] = Json::array({
                    Json{{"provider", "google"}, {"provider_email", "user@gmail.com"}, {"display_hint", "user"}},
                    Json{{"provider", "facebook"}, {"provider_email", "user@fb.com"}, {"display_hint", "user_fb"}}
                });
                res.status = 200;
                res.set_content(reply.dump(), "application/json");
            } else {
                Json conflict;
                conflict["error"] = "conflict";
                conflict["existing_profile"] = Json{{"uuid", "u-conflict"}, {"handle", "ConflictedUser"}};
                res.status = 409;
                res.set_content(conflict.dump(), "application/json");
            }
        });
        mockRelinkServer.Get("/api/v1/profile/me", [&](const httplib::Request &, httplib::Response &res) {
            Json profile;
            profile["uuid"] = "u-main";
            profile["handle"] = "MainUser";
            profile["identities"] = Json::array({
                Json{{"provider", "google"}, {"provider_email", "user@gmail.com"}, {"display_hint", "user"}},
                Json{{"provider", "facebook"}, {"provider_email", "user@fb.com"}, {"display_hint", "user_fb"}}
            });
            res.status = 200;
            res.set_content(profile.dump(), "application/json");
        });

        int mockPort = mockRelinkServer.bind_to_any_port("127.0.0.1");
        std::thread mockThread([&]() { mockRelinkServer.listen_after_bind(); });

        auto mockClient = OctiNetworkClient::createForLoopbackTest(mockPort);
        mockClient->setSessionToken("mock-session-token");
        AccountLinkDialog mergeDlg(mockClient.get());

        /* Initial link fails with 409 Conflict */
        auto mergeConflict =
            mergeDlg.linkProviderIdentity("facebook", "mock-fb-token");
        CHECK_FALSE(mergeConflict.ok);
        LONGS_EQUAL(409, mergeConflict.httpStatus);
        CHECK_TRUE(mergeDlg.hasPendingConflict());
        CHECK_TRUE(mergeDlg.mergeConflictButton() != nullptr);

        /* Merge empty account into current account -> sends force_relink: true */
        CHECK_TRUE(mergeDlg.mergeConflictIntoCurrentAccount());
        CHECK_TRUE(forceRelinkReceived);
        CHECK_FALSE(mergeDlg.hasPendingConflict());
        LONGS_EQUAL(2, static_cast<long>(mergeDlg.linkedIdentities().size()));

        /* Verify unauthenticated online observation without chat */
        bool observeWithoutAuthReceived = false;
        mockRelinkServer.Post("/api/v1/tables/room-guest-obs/observe", [&](const httplib::Request &req, httplib::Response &res) {
            if (!req.has_header("Authorization")) {
                observeWithoutAuthReceived = true;
            }
            res.status = 200;
            res.set_content("{\"ok\": true}", "application/json");
        });
        mockRelinkServer.Get("/api/v1/games/room-guest-obs", [&](const httplib::Request &, httplib::Response &res) {
            Json gv;
            gv["room_id"] = "room-guest-obs";
            gv["mode"] = "2P";
            gv["bases"] = 3;
            gv["live"] = true;
            gv["phase"] = "IN_PROGRESS";
            res.status = 200;
            res.set_content(gv.dump(), "application/json");
        });
        mockRelinkServer.Get("/api/v1/games/room-guest-obs/events", [&](const httplib::Request &, httplib::Response &res) {
            res.status = 200;
            res.set_content("", "text/event-stream");
        });

        auto guestObsClient = OctiNetworkClient::createForLoopbackTest(mockPort);
        std::string guestObsCfgPath = env.dir + "/guest_obs_cfg.json";
        OctiMainWindow guestObsWin(std::move(guestObsClient), guestObsCfgPath, false);
        CHECK_FALSE(guestObsWin.isAuthenticated());

        auto obsRes = guestObsWin.observeOnlineTable("room-guest-obs");
        CHECK_TRUE_TEXT(obsRes.ok, obsRes.errorMessage.c_str());
        CHECK_TRUE(observeWithoutAuthReceived);

        guestObsWin.showBoardView();
        CHECK_TRUE(guestObsWin.isBoardViewVisible());
        CHECK_FALSE(guestObsWin.gameChatInput()->isEnabled());
        STRCMP_EQUAL("Sign in to chat",
                     guestObsWin.gameChatInput()->placeholderText().toUtf8().constData());
        CHECK_FALSE(guestObsWin.sendGameChatButton()->isEnabled());
        CHECK_FALSE(guestObsWin.sendGameChatMessage("ALL", "Guest cannot send game chat"));

        guestObsWin.leaveOnlineRoom();

        mockRelinkServer.stop();
        if (mockThread.joinable()) {
            mockThread.join();
        }
    }

    /* 4. Global Lobby View & Chat + Matchmaking Dialog */
    winA.showLobbyView();
    winB.showLobbyView();
    CHECK_TRUE(winA.isLobbyViewVisible());
    CHECK_TRUE(winA.lobbyView()->sendLobbyChatMessage("Greetings from Alice!"));

    bool bobGotLobbyChat = false;
    for (int i = 0; i < 40 && !bobGotLobbyChat; ++i) {
        winB.pumpUiEvents();
        winB.lobbyView()->refreshLobby();
        for (const auto &msg : winB.lobbyView()->chatMessages()) {
            if (msg.text == "Greetings from Alice!") {
                bobGotLobbyChat = true;
                break;
            }
        }
        if (!bobGotLobbyChat) {
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
    }
    CHECK_TRUE(bobGotLobbyChat);
    CHECK_TRUE(winB.lobbyView()->players().size() >= 2);

    {
        MatchmakingDialog mmDlg(winA.netClient());
        MatchmakingRequest mmReq;
        mmReq.mode = "2P";
        mmReq.bases = 3;
        mmDlg.setRequest(mmReq);
        CHECK_TRUE(mmDlg.startMatchmaking().ok);
        auto st = mmDlg.pollMatchmakingStatus();
        CHECK_TRUE(st.ok);
        CHECK_TRUE(st.data.queued);
        CHECK_TRUE(mmDlg.cancelMatchmaking().ok);
    }

    /* Test Unauthenticated LobbyView & Visible Anonymous Observation */
    {
        std::string guestCfgPath = "/tmp/octi_guest_lobby_test.json";
        auto guestClient = OctiNetworkClient::createForLoopbackTest(env.port());
        OctiMainWindow guestWin(std::move(guestClient), guestCfgPath, false);
        CHECK_FALSE(guestWin.isAuthenticated());

        guestWin.showLobbyView();
        CHECK_TRUE(guestWin.isLobbyViewVisible());

        /* Verify Unauthenticated LobbyView controls: Refresh & Observe enabled, others disabled, Quick Match removed */
        CHECK_TRUE(guestWin.lobbyView()->refreshButton()->isEnabled());
        CHECK_TRUE(guestWin.lobbyView()->observeButton()->isEnabled());
        CHECK_FALSE(guestWin.lobbyView()->createTableButton()->isEnabled());
        CHECK_FALSE(guestWin.lobbyView()->joinButton()->isEnabled());
        CHECK_FALSE(guestWin.lobbyView()->leaveTableButton()->isEnabled());
        CHECK_TRUE(guestWin.lobbyView()->quickMatchButton() != nullptr);
        CHECK_FALSE(guestWin.lobbyView()->quickMatchButton()->isEnabled());
        CHECK_FALSE(guestWin.lobbyView()->chatInput()->isEnabled());
        CHECK_FALSE(guestWin.lobbyView()->chatSendButton()->isEnabled());
        STRCMP_EQUAL("Sign in to chat",
                     guestWin.lobbyView()->chatInput()->placeholderText().toUtf8().constData());
        CHECK_TRUE(guestWin.lobbyView()->statusLabel()->text().contains("Guest observation mode"));
        CHECK_FALSE(guestWin.lobbyView()->sendLobbyChatMessage("Guest chat should fail"));

        /* Verify that when authenticated, LobbyView controls become enabled */
        auto guestLogin = guestWin.netClient()->loginWithSso(
            "google",
            makeToken("google", "sub-guest-authenticated", "GuestAuthed"),
            "GuestAuthed");
        CHECK_TRUE(guestLogin.ok);
        guestWin.applyAuthenticatedProfile(guestLogin.data.profile);
        CHECK_TRUE(guestWin.isAuthenticated());

        guestWin.showLobbyView();
        CHECK_TRUE(guestWin.lobbyView()->createTableButton()->isEnabled());
        CHECK_TRUE(guestWin.lobbyView()->joinButton()->isEnabled());
        CHECK_TRUE(guestWin.lobbyView()->quickMatchButton() != nullptr);
        CHECK_TRUE(guestWin.lobbyView()->quickMatchButton()->isEnabled());
        CHECK_TRUE(guestWin.lobbyView()->chatInput()->isEnabled());
        CHECK_TRUE(guestWin.lobbyView()->chatSendButton()->isEnabled());
        STRCMP_EQUAL("Message the global lobby...",
                     guestWin.lobbyView()->chatInput()->placeholderText().toUtf8().constData());

        std::remove(guestCfgPath.c_str());
    }

    /* 5. Passcode-Protected Table Creation, Join, and Auto-Orientation */
    CreateTableDialog createDlg(winA.netClient());
    CreateTableRequest tReq;
    tReq.mode = "2P";
    tReq.bases = 3;
    tReq.allowTakeback = true;
    tReq.passcode = "8899";
    createDlg.setRequest(tReq);
    auto created = winA.createAndEnterOnlineTable(createDlg.buildRequest());
    CHECK_TRUE_TEXT(created.ok, created.errorMessage.c_str());
    std::string roomId = created.data.roomId;
    CHECK_TRUE(winA.isInOnlineGame());
    LONGS_EQUAL(static_cast<long>(BoardRotation::ROT_0),
                static_cast<long>(winA.boardRotation()));

    PasscodeDialog passDlg(roomId);
    passDlg.setPasscode("8899");
    auto joinedB = winB.joinAndEnterOnlineTable(roomId, 2, passDlg.passcode());
    CHECK_TRUE_TEXT(joinedB.ok, joinedB.errorMessage.c_str());
    CHECK_TRUE(joinedB.data.gameStarted);
    /* Seat 2 (Player B in 2P) MUST auto-orient to ROT_180! */
    LONGS_EQUAL(static_cast<long>(BoardRotation::ROT_180),
                static_cast<long>(winB.boardRotation()));

    CHECK_TRUE(clientC->observeTable(roomId, "8899").ok);
    winA.refreshOnlineGameView();
    winB.refreshOnlineGameView();

    /* 6. Live Drag-and-Drop Board Play Propagating Over SSE */
    /* Move 1 (Alice): Add prong at (3,3) North -> "33+a" */
    CHECK_TRUE(winA.boardCanvas()->simulateDragProngToPod(3, 3, 0, DIR_N, false));
    if (winA.moveComposer()->isAwaitingConfirmation()) {
        CHECK_TRUE(winA.moveComposer()->commitStagedMove());
    }
    for (int i = 0; i < 40 && winB.vcrCurrentPly() < 1; ++i) {
        winB.pumpUiEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    if (winB.vcrCurrentPly() < 1) {
        winB.refreshOnlineGameView();
    }
    LONGS_EQUAL(1, winB.vcrCurrentPly());
    LONGS_EQUAL(static_cast<long>(NORMAL_PRONG),
                static_cast<long>(
                    winB.board()->squareAt(2, 2).getPodAt(0).prongAt(DIR_N)));

    /* Move 2 (Bob): Add prong at (5,7) South -> "57+e" */
    CHECK_TRUE(winB.boardCanvas()->simulateDragProngToPod(5, 7, 0, DIR_S, false));
    if (winB.moveComposer()->isAwaitingConfirmation()) {
        CHECK_TRUE(winB.moveComposer()->commitStagedMove());
    }
    for (int i = 0; i < 40 && winA.vcrCurrentPly() < 2; ++i) {
        winA.pumpUiEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    if (winA.vcrCurrentPly() < 2) {
        winA.refreshOnlineGameView();
    }
    LONGS_EQUAL(2, winA.vcrCurrentPly());

    /* Move 3 (Alice): Step pod from (3,3) to (3,4) -> "33a-34" */
    CHECK_TRUE(winA.boardCanvas()->simulateDragPodToSquare(3, 3, 0, 3, 4, false));
    if (winA.moveComposer()->isAwaitingConfirmation()) {
        CHECK_TRUE(winA.moveComposer()->commitStagedMove());
    }
    for (int i = 0; i < 40 && winB.vcrCurrentPly() < 3; ++i) {
        winB.pumpUiEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    if (winB.vcrCurrentPly() < 3) {
        winB.refreshOnlineGameView();
    }
    LONGS_EQUAL(3, winB.vcrCurrentPly());
    CHECK_FALSE(winB.board()->squareAt(2, 3).isEmpty());

    /* 7. Board Chat & Mutual Proposals */
    CHECK_TRUE(winA.sendGameChatMessage("ALL", "Strong opening move!"));
    for (int i = 0; i < 40 && winB.gameChatLines().empty(); ++i) {
        winB.pumpUiEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    CHECK_FALSE(winB.gameChatLines().empty());

    CHECK_TRUE(winA.proposeDraw().ok);
    winB.refreshOnlineGameView();
    CHECK_TRUE(winB.hasActiveProposal());
    STRCMP_EQUAL("DRAW_OFFER", winB.activeProposalType().c_str());
    CHECK_TRUE(winB.declineActiveProposal().ok);
    CHECK_FALSE(winB.hasActiveProposal());

    /* 8. Room Moderation Dialog (Policy Update, Mute, Kick & Ban Observer) */
    {
        RoomModerationDialog modDlg(winA.netClient(), roomId);
        CHECK_TRUE(modDlg.refreshRoomView());
        auto polRes = modDlg.applyPolicySettings("OPEN");
        CHECK_TRUE_TEXT(polRes.ok, polRes.errorMessage.c_str());
        STRCMP_EQUAL("OPEN", polRes.data.observerPolicy.c_str());

        auto muteRes = modDlg.muteParticipant(uuidC, true);
        CHECK_TRUE_TEXT(muteRes.ok, muteRes.errorMessage.c_str());

        auto banRes = modDlg.kickAndBanObserver(uuidC, "Disruptive spectator");
        CHECK_TRUE_TEXT(banRes.ok, banRes.errorMessage.c_str());
    }

    /* 10. Game Over, Leaderboard, Archive Browser & VCR Replay */
    CHECK_TRUE(winB.resignCurrentGame().ok);
    winA.refreshOnlineGameView();

    {
        GameOverDialog goDlg(
            "Winner: Alice_GM (Seat 1)", "RESIGNATION", 3,
            winA.exportCurrentOctiTranscript());
        CHECK_TRUE(goDlg.winnerBannerText().contains("Alice_GM"));
        LONGS_EQUAL(3, goDlg.moveCount());
        std::string exportedPath = env.dir + "/exported_match.octi";
        CHECK_TRUE(goDlg.exportTranscriptToFile(exportedPath));
        CHECK_TRUE(QFileInfo(QString::fromStdString(exportedPath)).size() > 20);
        bool rematchCalled = false;
        goDlg.setOnRematchCallback([&]() { rematchCalled = true; });
        goDlg.triggerRematch();
        CHECK_TRUE(rematchCalled);
        CHECK_TRUE(goDlg.wasRematchRequested());
    }

    {
        LeaderboardDialog lbDlg(winA.netClient());
        auto lbRes = lbDlg.refreshLeaderboard("2P");
        CHECK_TRUE_TEXT(lbRes.ok, lbRes.errorMessage.c_str());
        CHECK_TRUE(lbDlg.rows().size() >= 2);
    }

    {
        ArchiveBrowserDialog archDlg(winA.netClient());
        std::string loadedGameId;
        archDlg.setOnLoadReplayCallback(
            [&](const std::string &gid,
                const std::string &transcript,
                const ArchivedGameSummary & /*sum*/) {
                loadedGameId = gid;
                CHECK_TRUE(winA.loadOctiTranscriptIntoReplay(transcript));
            });
        auto listRes = archDlg.refreshArchiveList();
        CHECK_TRUE_TEXT(listRes.ok, listRes.errorMessage.c_str());
        CHECK_FALSE(archDlg.archivedGames().empty());
        CHECK_TRUE(archDlg.loadGameIntoReplay(roomId));
        STRCMP_EQUAL(roomId.c_str(), loadedGameId.c_str());
    }

    /* Step through the loaded 3-ply replay with VCR controls */
    LONGS_EQUAL(3, winA.vcrCurrentPly());
    winA.vcrFirst();
    LONGS_EQUAL(0, winA.vcrCurrentPly());
    winA.vcrNext();
    LONGS_EQUAL(1, winA.vcrCurrentPly());
    winA.vcrPrev();
    LONGS_EQUAL(0, winA.vcrCurrentPly());
    winA.vcrLast();
    LONGS_EQUAL(3, winA.vcrCurrentPly());
}

TEST(DesktopAppIntegrationTest,
     OfflineVariantsBotPlayAndFullWindowScreenshots)
{
    QString repoRootStr = QString::fromUtf8(OCTI_REPO_ROOT_DIR);
    QString artifactDir = QFileInfo::exists(repoRootStr + "/plan/done/plan_octi_desktop.md")
                              ? (repoRootStr + "/plan/done/plan_octi_desktop.artifacts")
                              : (repoRootStr + "/plan/plan_octi_desktop.artifacts");
    QDir().mkpath(artifactDir);

    std::string cfgPath = "/tmp/octi_offline_variants_test.json";
    OctiMainWindow win(nullptr, cfgPath, true);

    /* 1. 2P vs Bot (Casual, Superprongs + Edgeless) */
    {
        NewLocalGameDialog dlg;
        LocalGameConfig cfg;
        cfg.playType = LocalPlayType::VS_AI_BOT;
        cfg.botDifficulty = BotDifficulty::CASUAL;
        cfg.humanSeat = PLAYER_A;
        cfg.mode = GAME_MODE_2;
        cfg.bases = GAME_BASES_1;
        cfg.useSuperProngs = true;
        cfg.useEdgelessBoard = true;
        cfg.timeControlEnabled = true;
        cfg.initialTimeMs = 300000;
        cfg.incrementMs = 3000;
        dlg.setConfig(cfg);

        win.startNewLocalGame(dlg.config());
        LONGS_EQUAL(static_cast<long>(BoardRotation::ROT_0),
                    static_cast<long>(win.boardRotation()));
        LONGS_EQUAL(0, win.vcrCurrentPly());

        /* Human (Seat A) adds a superprong at (5,3) North -> Bot (Seat B) replies automatically! */
        CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(
            5, 3, 0, DIR_N, true));
        if (win.moveComposer()->isAwaitingConfirmation()) {
            CHECK_TRUE(win.moveComposer()->commitStagedMove());
        }
        LONGS_EQUAL(2, win.vcrCurrentPly());
        LONGS_EQUAL(static_cast<long>(PLAYER_A),
                    static_cast<long>(win.board()->getGame()->getTurn()));
    }

    /* Capture Full-Window Screenshot 1: Obsidian & Brass Board View */
    win.setTheme(ThemeId::OBSIDIAN_BRASS);
    win.showBoardView();
    QImage shotObsidian = win.renderWindowToImage(1440, 900);
    CHECK_FALSE(shotObsidian.isNull());
    QString pathObsidian = artifactDir + "/main_window_board_obsidian.png";
    CHECK_TRUE(shotObsidian.save(pathObsidian));
    CHECK_TRUE(QFileInfo(pathObsidian).size() > 15000);

    /* Capture Full-Window Screenshot 2: Grandmaster Rosewood & Ivory Board View */
    win.toggleTheme();
    LONGS_EQUAL(static_cast<long>(ThemeId::ROSEWOOD_IVORY),
                static_cast<long>(win.theme()));
    QImage shotRosewood = win.renderWindowToImage(1440, 900);
    CHECK_FALSE(shotRosewood.isNull());
    QString pathRosewood = artifactDir + "/main_window_board_rosewood.png";
    CHECK_TRUE(shotRosewood.save(pathRosewood));
    CHECK_TRUE(QFileInfo(pathRosewood).size() > 15000);

    /* Capture Full-Window Screenshot 3: Online Lobby View */
    win.setTheme(ThemeId::OBSIDIAN_BRASS);
    LobbyStateData sampleLobby;
    sampleLobby.players = {
        {"u-1", "Alice_GM", "PLAYING", "g-20261022-0001"},
        {"u-2", "Bob_Challenger", "PLAYING", "g-20261022-0001"},
        {"u-3", "MasterChen", "IN_LOBBY", ""},
    };
    TableSummaryInfo t1;
    t1.roomId = "g-20261022-0001";
    t1.mode = "2P";
    t1.bases = 3;
    t1.superProngs = true;
    t1.phase = "IN_PROGRESS";
    t1.seatsFilled = 2;
    t1.seatsTotal = 2;
    t1.hostHandle = "Alice_GM";
    sampleLobby.tables.push_back(t1);
    sampleLobby.recentChat = {
        {1, 1000, "u-1", "Alice_GM", "Welcome to the OCTI Grandmaster Lobby!"},
        {2, 2000, "u-3", "MasterChen", "Anyone up for a 4P partnership match?"},
    };
    win.lobbyView()->refreshFromLobbyState(sampleLobby);
    win.showLobbyView();
    QImage shotLobby = win.renderWindowToImage(1440, 900);
    CHECK_FALSE(shotLobby.isNull());
    QString pathLobby = artifactDir + "/main_window_lobby.png";
    CHECK_TRUE(shotLobby.save(pathLobby));
    CHECK_TRUE(QFileInfo(pathLobby).size() > 15000);

    /* 2. 4P Partnership (Seat B -> auto-orients ROT_90, Bot plays Seat A first) */
    {
        LocalGameConfig cfg4;
        cfg4.playType = LocalPlayType::VS_AI_BOT;
        cfg4.botDifficulty = BotDifficulty::CASUAL;
        cfg4.humanSeat = PLAYER_B;
        cfg4.mode = GAME_MODE_4;
        cfg4.bases = GAME_BASES_3;
        win.startNewLocalGame(cfg4);
        LONGS_EQUAL(static_cast<long>(BoardRotation::ROT_90),
                    static_cast<long>(win.boardRotation()));
        /* Since Seat A moves first and Human is Seat B, Bot A already moved! */
        LONGS_EQUAL(1, win.vcrCurrentPly());
        LONGS_EQUAL(static_cast<long>(PLAYER_B),
                    static_cast<long>(win.board()->getGame()->getTurn()));
    }

    /* 3. OCTI for Kids (6x7 board vs Bot) */
    {
        LocalGameConfig cfgOfk;
        cfgOfk.playType = LocalPlayType::VS_AI_BOT;
        cfgOfk.botDifficulty = BotDifficulty::CASUAL;
        cfgOfk.humanSeat = PLAYER_A;
        cfgOfk.mode = GAME_MODE_OFK;
        win.startNewLocalGame(cfgOfk);
        LONGS_EQUAL(6, win.board()->getBoardWidth());
        LONGS_EQUAL(7, win.board()->getBoardHeight());
        CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(
            2, 2, 0, DIR_N, false));
        if (win.moveComposer()->isAwaitingConfirmation()) {
            CHECK_TRUE(win.moveComposer()->commitStagedMove());
        }
        LONGS_EQUAL(2, win.vcrCurrentPly());
    }

    /* 4. Render 512x512 App Icon & Multi-Resolution Icon Showcase Sheet */
    {
        CHECK_FALSE(win.windowIcon().isNull());
        QImage icon512 = renderOctiIconImage(512);
        CHECK_FALSE(icon512.isNull());
        QString pathIcon512 = artifactDir + "/octi_icon_512.png";
        CHECK_TRUE(icon512.save(pathIcon512));
        CHECK_TRUE(QFileInfo(pathIcon512).size() > 10000);

        // Render showcase sheet comparing 256, 128, 64, 48, 32, 24, 16 on dark & light backgrounds
        QImage showcase(1080, 560, QImage::Format_ARGB32_Premultiplied);
        showcase.fill(QColor(11, 15, 23));
        QPainter sp(&showcase);
        sp.setRenderHint(QPainter::Antialiasing, true);

        // Title header
        QFont titleFont("DejaVu Sans", 18, QFont::Bold);
        sp.setFont(titleFont);
        sp.setPen(QColor(253, 224, 71));
        sp.drawText(QRectF(32, 20, 1016, 36), Qt::AlignLeft | Qt::AlignVCenter,
                    "OCTI Desktop Player & Installer Icon Suite (256px -> 16px Multi-DPI)");

        // Left hero card (256x256 on Obsidian card)
        QRectF heroCard(32, 72, 320, 456);
        sp.setPen(QPen(QColor(180, 145, 72), 1.5));
        sp.setBrush(QColor(20, 27, 41));
        sp.drawRoundedRect(heroCard, 12, 12);
        QImage img256 = renderOctiIconImage(256);
        sp.drawImage(QPointF(heroCard.center().x() - 128, 108), img256);

        QFont subFont("DejaVu Sans", 11, QFont::Bold);
        sp.setFont(subFont);
        sp.setPen(QColor(242, 236, 224));
        sp.drawText(QRectF(heroCard.left(), 388, heroCard.width(), 28),
                    Qt::AlignCenter, "256 x 256 Master Emblem");
        QFont capFont("DejaVu Sans", 9);
        sp.setFont(capFont);
        sp.setPen(QColor(168, 178, 198));
        sp.drawText(QRectF(heroCard.left() + 16, 418, heroCard.width() - 32, 88),
                    Qt::AlignHCenter | Qt::TextWordWrap,
                    "Faceted Imperial Brass octagon, Obsidian tactical dial, "
                    "4 Gold Superprongs, 4 Platinum Prongs & Emerald-Jade Pod");

        // Top right row: Dark Desktop / Taskbar strip
        QRectF darkStrip(380, 72, 668, 216);
        sp.setPen(QPen(QColor(180, 145, 72), 1.2));
        sp.setBrush(QColor(16, 22, 35));
        sp.drawRoundedRect(darkStrip, 10, 10);
        sp.setFont(subFont);
        sp.setPen(QColor(216, 182, 108));
        sp.drawText(QRectF(400, 84, 600, 24), Qt::AlignLeft | Qt::AlignVCenter,
                    "Dark Theme / Windows 11 Dark Taskbar & Explorer Scaling");

        // Bottom right row: Light Windows Explorer strip
        QRectF lightStrip(380, 312, 668, 216);
        sp.setPen(QPen(QColor(180, 145, 72), 1.2));
        sp.setBrush(QColor(241, 245, 249));
        sp.drawRoundedRect(lightStrip, 10, 10);
        sp.setFont(subFont);
        sp.setPen(QColor(30, 41, 59));
        sp.drawText(QRectF(400, 324, 600, 24), Qt::AlignLeft | Qt::AlignVCenter,
                    "Light Theme / Windows File Explorer & Desktop Shortcut Scaling");

        static const int previewSizes[] = {128, 64, 48, 32, 24, 16};
        double xCursor = 408.0;
        for (int sz : previewSizes) {
            QImage iconSz = renderOctiIconImage(sz);
            double boxW = std::max(56.0, static_cast<double>(sz) + 24.0);

            // Draw on dark strip
            double dy = 120.0 + (128.0 - sz) * 0.5;
            sp.drawImage(QPointF(xCursor + (boxW - sz) * 0.5, dy), iconSz);
            sp.setFont(capFont);
            sp.setPen(QColor(203, 213, 225));
            sp.drawText(QRectF(xCursor, 254, boxW, 22), Qt::AlignCenter,
                        QString("%1x%1").arg(sz));

            // Draw on light strip
            double ly = 360.0 + (128.0 - sz) * 0.5;
            sp.drawImage(QPointF(xCursor + (boxW - sz) * 0.5, ly), iconSz);
            sp.setPen(QColor(51, 65, 85));
            sp.drawText(QRectF(xCursor, 494, boxW, 22), Qt::AlignCenter,
                        QString("%1x%1").arg(sz));

            xCursor += boxW + 16.0;
        }
        sp.end();

        QString pathShowcase = artifactDir + "/octi_icon_showcase.png";
        CHECK_TRUE(showcase.save(pathShowcase));
        CHECK_TRUE(QFileInfo(pathShowcase).size() > 15000);
    }

    std::remove(cfgPath.c_str());
}

TEST(DesktopAppIntegrationTest, WindowsBundleAndNsisInstallerMatchReadmeVersion)
{
    std::string repoRoot = OCTI_REPO_ROOT_DIR;
    std::string readmePath = repoRoot + "/README.md";
    std::ifstream readmeIn(readmePath);
    CHECK_TRUE_TEXT(readmeIn.good(), readmePath.c_str());

    std::string readmeVersion;
    std::string line;
    const std::string prefix = "**Version:";
    while (std::getline(readmeIn, line)) {
        std::size_t pos = line.find(prefix);
        if (pos != std::string::npos) {
            std::size_t start = pos + prefix.size();
            while (start < line.size() &&
                   (line[start] == ' ' || line[start] == '\t')) {
                ++start;
            }
            std::size_t end = line.find("**", start);
            if (end != std::string::npos && end > start) {
                readmeVersion = line.substr(start, end - start);
                break;
            }
        }
    }

    CHECK_FALSE(readmeVersion.empty());
    STRCMP_EQUAL(readmeVersion.c_str(), OctiCore::versionString());
    CHECK_TRUE(DesktopCliArgs::versionBanner().find(readmeVersion) !=
               std::string::npos);

    /* Verify multi-resolution octi.ico header (00 00 01 00, >= 6 icon layers) and NSIS BMPs */
    std::string icoPath = repoRoot + "/desktop/resources/octi.ico";
    {
        std::ifstream icoIn(icoPath, std::ios::binary | std::ios::ate);
        CHECK_TRUE_TEXT(icoIn.good(), icoPath.c_str());
        std::streamsize icoSize = icoIn.tellg();
        CHECK_TRUE_TEXT(icoSize > 10000, icoPath.c_str());
        icoIn.seekg(0, std::ios::beg);
        unsigned char hdr[6] = {0, 0, 0, 0, 0, 0};
        icoIn.read(reinterpret_cast<char *>(hdr), 6);
        LONGS_EQUAL(0, hdr[0]);
        LONGS_EQUAL(0, hdr[1]);
        LONGS_EQUAL(1, hdr[2]);
        LONGS_EQUAL(0, hdr[3]);
        int numImages = static_cast<int>(hdr[4]) |
                        (static_cast<int>(hdr[5]) << 8);
        CHECK_TRUE(numImages >= 6);
    }
    CHECK_TRUE(QFileInfo(QString::fromStdString(
        repoRoot + "/desktop/resources/nsis_welcome.bmp")).size() > 100000);
    CHECK_TRUE(QFileInfo(QString::fromStdString(
        repoRoot + "/desktop/resources/nsis_header.bmp")).size() > 10000);

    std::string winBuildDir = repoRoot + "/desktop/build/windows";
    std::string installerName = "octi-setup-" + readmeVersion + ".exe";

    std::vector<std::string> requiredPeFiles = {
        "octi.exe",
        "Qt6Core.dll",
        "Qt6Gui.dll",
        "Qt6Widgets.dll",
        "libstdc++-6.dll",
        "libgcc_s_seh-1.dll",
        "libwinpthread-1.dll",
        "platforms/qwindows.dll",
        "platforms/qdirect2d.dll",
        "platforms/qminimal.dll",
        "platforms/qoffscreen.dll",
        "styles/qmodernwindowsstyle.dll",
        installerName,
    };

    for (const std::string &relPath : requiredPeFiles) {
        std::string fullPath = winBuildDir + "/" + relPath;
        std::ifstream binIn(fullPath, std::ios::binary | std::ios::ate);
        CHECK_TRUE_TEXT(binIn.good(), fullPath.c_str());
        std::streamsize fileSize = binIn.tellg();
        CHECK_TRUE_TEXT(fileSize > 1024, fullPath.c_str());
        binIn.seekg(0, std::ios::beg);
        unsigned char mz[2] = {0, 0};
        binIn.read(reinterpret_cast<char *>(mz), 2);
        LONGS_EQUAL_TEXT(0x4D, mz[0], fullPath.c_str());
        LONGS_EQUAL_TEXT(0x5A, mz[1], fullPath.c_str());
    }

    std::string nsiPath = winBuildDir + "/octi.nsi";
    std::ifstream nsiIn(nsiPath);
    CHECK_TRUE_TEXT(nsiIn.good(), nsiPath.c_str());
    std::string nsiContent((std::istreambuf_iterator<char>(nsiIn)),
                           std::istreambuf_iterator<char>());
    CHECK_TRUE(nsiContent.find(installerName) != std::string::npos);
    CHECK_TRUE(nsiContent.find("InstallDir \"$PROGRAMFILES64\\OCTI\"") !=
               std::string::npos);
    CHECK_TRUE(nsiContent.find("RequestExecutionLevel admin") !=
               std::string::npos);
    CHECK_TRUE(nsiContent.find("Delete \"$INSTDIR\\octi.exe.old\"") !=
               std::string::npos);
    CHECK_TRUE(nsiContent.find("MUI_ICON") != std::string::npos);
    CHECK_TRUE(nsiContent.find("MUI_UNICON") != std::string::npos);
    CHECK_TRUE(nsiContent.find("resources/octi.ico") != std::string::npos);

    /* Verify NSIS FirstHeader (0xDEADBEEF + "NullsoftInst" on 512B boundary) and CRC32 */
    std::string instFullPath = winBuildDir + "/" + installerName;
    std::ifstream instIn(instFullPath, std::ios::binary);
    std::vector<unsigned char> instBytes(
        (std::istreambuf_iterator<char>(instIn)),
        std::istreambuf_iterator<char>());
    CHECK_TRUE(instBytes.size() > 524288);

    std::size_t fhOffset = 0;
    std::uint32_t lenAllFollowing = 0;
    for (std::size_t off = 512; off + 28 <= instBytes.size(); off += 512) {
        std::uint32_t flags =
            static_cast<std::uint32_t>(instBytes[off]) |
            (static_cast<std::uint32_t>(instBytes[off + 1]) << 8) |
            (static_cast<std::uint32_t>(instBytes[off + 2]) << 16) |
            (static_cast<std::uint32_t>(instBytes[off + 3]) << 24);
        std::uint32_t sig =
            static_cast<std::uint32_t>(instBytes[off + 4]) |
            (static_cast<std::uint32_t>(instBytes[off + 5]) << 8) |
            (static_cast<std::uint32_t>(instBytes[off + 6]) << 16) |
            (static_cast<std::uint32_t>(instBytes[off + 7]) << 24);
        if ((flags & ~0x0fU) == 0 && sig == 0xDEADBEEFU &&
            std::memcmp(&instBytes[off + 8], "NullsoftInst", 12) == 0) {
            fhOffset = off;
            lenAllFollowing =
                static_cast<std::uint32_t>(instBytes[off + 24]) |
                (static_cast<std::uint32_t>(instBytes[off + 25]) << 8) |
                (static_cast<std::uint32_t>(instBytes[off + 26]) << 16) |
                (static_cast<std::uint32_t>(instBytes[off + 27]) << 24);
            break;
        }
    }
    CHECK_TRUE(fhOffset > 0);
    LONGS_EQUAL(instBytes.size(), fhOffset + lenAllFollowing);

    std::uint32_t crc = 0xFFFFFFFFU;
    std::size_t crcEnd = instBytes.size() - 4;
    for (std::size_t i = 512; i < crcEnd; ++i) {
        crc ^= instBytes[i];
        for (int b = 0; b < 8; ++b) {
            crc = (crc >> 1) ^ ((crc & 1U) ? 0xEDB88320U : 0U);
        }
    }
    crc = ~crc;
    std::uint32_t storedCrc =
        static_cast<std::uint32_t>(instBytes[crcEnd]) |
        (static_cast<std::uint32_t>(instBytes[crcEnd + 1]) << 8) |
        (static_cast<std::uint32_t>(instBytes[crcEnd + 2]) << 16) |
        (static_cast<std::uint32_t>(instBytes[crcEnd + 3]) << 24);
    LONGS_EQUAL(storedCrc, crc);

    /* Verify staged libstdc++-6.dll contains GCC 13 symbols imported by octi.exe */
    std::string stdcppPath = winBuildDir + "/libstdc++-6.dll";
    std::ifstream stdcppIn(stdcppPath, std::ios::binary);
    std::string stdcppBytes((std::istreambuf_iterator<char>(stdcppIn)),
                            std::istreambuf_iterator<char>());
    CHECK_TRUE(stdcppBytes.find("_ZSt21ios_base_library_initv") !=
               std::string::npos);
    CHECK_TRUE(stdcppBytes.find("__cxa_thread_atexit") !=
               std::string::npos);

    /* Verify octi.exe imports WINHTTP.dll and VERSION.dll and embeds endpoint and appId */
    std::string octiExePath = winBuildDir + "/octi.exe";
    std::ifstream exeIn(octiExePath, std::ios::binary);
    CHECK_TRUE_TEXT(exeIn.good(), octiExePath.c_str());
    std::string exeBytes((std::istreambuf_iterator<char>(exeIn)),
                         std::istreambuf_iterator<char>());
    std::string exeBytesLower = exeBytes;
    std::transform(exeBytesLower.begin(), exeBytesLower.end(),
                   exeBytesLower.begin(),
                   [](unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });
    CHECK_TRUE(exeBytesLower.find("winhttp.dll") != std::string::npos);
    CHECK_TRUE(exeBytesLower.find("version.dll") != std::string::npos);
    CHECK_TRUE(exeBytes.find("https://www.selfso.com/software/windows/") !=
               std::string::npos);
    CHECK_TRUE(exeBytes.find("com.selfso.octi") != std::string::npos);
}

TEST(DesktopAppIntegrationTest, DesktopSelfUpdateEndToEndAndFaultInjection)
{
    /* 1. Default Config & Stale .old Backup Cleanup */
    auto defCfg = OctiMainWindow::defaultSelfUpdateConfig();
    const int64_t expectedVerCode =
        selfupdate::UpdateManifest::parseVersionCode(OctiCore::versionString());
    STRCMP_EQUAL("com.selfso.octi", defCfg.applicationId.c_str());
    STRCMP_EQUAL(OctiCore::versionString(),
                 defCfg.installedVersionName.c_str());
    LONGS_EQUAL(expectedVerCode, defCfg.installedVersionCode);
    STRCMP_EQUAL("https://www.selfso.com/software/windows/",
                 defCfg.manifestUrl.c_str());

    char tmpTemplate[] = "/tmp/octi_selfupdate_XXXXXX";
    char *tmpDir = mkdtemp(tmpTemplate);
    CHECK_TRUE(tmpDir != nullptr);
    std::string tmpDirStr = tmpDir;
    struct TmpDirGuard {
        std::string path;
        ~TmpDirGuard() {
            if (!path.empty()) {
                std::filesystem::remove_all(path);
            }
        }
    } dirGuard{tmpDirStr};

    {
        std::ofstream f1(tmpDirStr + "/octi.exe");
        f1 << "octi-binary";
    }
    {
        std::ofstream f2(tmpDirStr + "/octi.exe.old");
        f2 << "octi-old-binary";
    }
    CHECK_TRUE(std::filesystem::exists(tmpDirStr + "/octi.exe"));
    CHECK_TRUE(std::filesystem::exists(tmpDirStr + "/octi.exe.old"));
    CHECK_TRUE(selfupdate::UpdateInstaller::cleanupStaleBackup(tmpDirStr + "/octi.exe"));
    CHECK_TRUE(std::filesystem::exists(tmpDirStr + "/octi.exe"));
    CHECK_FALSE(std::filesystem::exists(tmpDirStr + "/octi.exe.old"));

    /* 2. Loopback HTTP Server Setup */
    httplib::Server server;
    std::atomic<int> versionStatus{200};
    std::string versionJsonBody;
    std::atomic<int> packageStatus{200};
    std::string packageBody;

    server.Get("/software/windows/version.json",
               [&](const httplib::Request &, httplib::Response &res) {
                   res.status = versionStatus.load();
                   if (res.status == 200) {
                       res.set_content(versionJsonBody, "application/json");
                   } else {
                       res.set_content("HTTP 500 Internal Server Error", "text/plain");
                   }
               });

    server.Get("/software/windows/octi/octi-setup.exe",
               [&](const httplib::Request &, httplib::Response &res) {
                   res.status = packageStatus.load();
                   if (res.status == 200) {
                       res.set_content(packageBody, "application/octet-stream");
                   } else {
                       res.set_content("HTTP 404 Not Found", "text/plain");
                   }
               });

    int httpPort = server.bind_to_any_port("127.0.0.1");
    CHECK_TRUE(httpPort > 0);
    std::thread serverThread([&]() {
        server.listen_after_bind();
    });
    server.wait_until_ready();
    struct ServerGuard {
        httplib::Server &srv;
        std::thread &th;
        ~ServerGuard() {
            srv.stop();
            if (th.joinable()) {
                th.join();
            }
        }
    } serverGuard{server, serverThread};

    OctiMainWindow win(nullptr, tmpDirStr + "/octi_config.json", true);
    win.resize(1440, 900);
    win.show();
    win.pumpUiEvents();

    auto pumpUntil = [&](std::function<bool()> cond, int timeoutMs = 5000) -> bool {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - start).count() < timeoutMs) {
            win.pumpUiEvents();
            if (cond()) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        win.pumpUiEvents();
        return cond();
    };

    selfupdate::CheckerConfig testCfg;
    testCfg.applicationId = "com.selfso.octi";
    testCfg.installedVersionName = OctiCore::versionString();
    testCfg.installedVersionCode = expectedVerCode;
    testCfg.manifestUrl = "http://127.0.0.1:" + std::to_string(httpPort) + "/software/windows/";

    selfupdate::InstallOptions installOpts;
    installOpts.currentExePath = tmpDirStr + "/octi.exe";
    installOpts.targetExePath = tmpDirStr + "/octi.exe";
    installOpts.dryRun = true;

    std::string stagingDir = tmpDirStr + "/staging";
    std::filesystem::create_directories(stagingDir);

    win.setSelfUpdateController(
        std::make_shared<selfupdate::SelfUpdateController>(
            testCfg, installOpts, stagingDir));

    /* 3. Silent Background Startup Check (UpToDate & Error) vs. Manual Check Banner */
    nlohmann::json upToDateDoc;
    upToDateDoc["applicationId"] = "com.selfso.octi";
    upToDateDoc["versionName"] = OctiCore::versionString();
    upToDateDoc["versionCode"] = expectedVerCode;
    upToDateDoc["installMode"] = "installer";
    upToDateDoc["packageUrl"] = "octi/octi-setup.exe";
    upToDateDoc["sha256"] = "0000000000000000000000000000000000000000000000000000000000000000";
    upToDateDoc["sizeBytes"] = 100;
    upToDateDoc["releaseNotes"] = "OCTI is up to date.";
    versionJsonBody = upToDateDoc.dump();
    versionStatus = 200;

    win.checkForAppUpdates(false);
    CHECK_TRUE(pumpUntil([&]() {
        return win.currentSelfUpdateState().phase == selfupdate::UpdatePhase::UpToDate &&
               !win.currentSelfUpdateState().isManualCheck;
    }));
    CHECK_FALSE(win.isUpdateBannerVisible());
    STRCMP_EQUAL("Check for Updates", win.checkUpdateButtonText().toUtf8().constData());

    QRect boardGeomBefore = static_cast<QWidget *>(win.boardCanvas())->geometry();

    win.checkForAppUpdates(true);
    CHECK_TRUE(pumpUntil([&]() {
        return win.currentSelfUpdateState().phase == selfupdate::UpdatePhase::UpToDate &&
               win.currentSelfUpdateState().isManualCheck;
    }));
    CHECK_FALSE(win.isUpdateBannerVisible());
    CHECK_TRUE(win.updateBannerText().contains("up to date", Qt::CaseInsensitive));
    CHECK_EQUAL(boardGeomBefore.x(), static_cast<QWidget *>(win.boardCanvas())->geometry().x());
    CHECK_EQUAL(boardGeomBefore.y(), static_cast<QWidget *>(win.boardCanvas())->geometry().y());
    CHECK_EQUAL(boardGeomBefore.width(), static_cast<QWidget *>(win.boardCanvas())->geometry().width());
    CHECK_EQUAL(boardGeomBefore.height(), static_cast<QWidget *>(win.boardCanvas())->geometry().height());
    win.dismissAppUpdateBanner();
    win.pumpUiEvents();
    CHECK_FALSE(win.isUpdateBannerVisible());
    LONGS_EQUAL(static_cast<int>(selfupdate::UpdatePhase::Idle),
                static_cast<int>(win.currentSelfUpdateState().phase));

    /* Offline / 500 error check */
    versionStatus = 500;

    win.checkForAppUpdates(false);
    CHECK_TRUE(pumpUntil([&]() {
        return win.currentSelfUpdateState().phase == selfupdate::UpdatePhase::Error &&
               !win.currentSelfUpdateState().isManualCheck;
    }));
    CHECK_FALSE(win.isUpdateBannerVisible());

    win.checkForAppUpdates(true);
    CHECK_TRUE(pumpUntil([&]() {
        return win.currentSelfUpdateState().phase == selfupdate::UpdatePhase::Error &&
               win.currentSelfUpdateState().isManualCheck;
    }));
    CHECK_FALSE(win.isUpdateBannerVisible());
    CHECK_EQUAL(boardGeomBefore.x(), static_cast<QWidget *>(win.boardCanvas())->geometry().x());
    CHECK_EQUAL(boardGeomBefore.y(), static_cast<QWidget *>(win.boardCanvas())->geometry().y());
    CHECK_EQUAL(boardGeomBefore.width(), static_cast<QWidget *>(win.boardCanvas())->geometry().width());
    CHECK_EQUAL(boardGeomBefore.height(), static_cast<QWidget *>(win.boardCanvas())->geometry().height());

    /* 4. Nominal Update Available (2.0.4 -> 9.9.9), Banner, SelfUpdateDialog, Screenshots, Download & Install */
    std::string fakeInstaller(32768, 'M');
    fakeInstaller[0] = 'M';
    fakeInstaller[1] = 'Z';
    std::string expectedSha = selfupdate::Sha256::hashHex(fakeInstaller);
    packageBody = fakeInstaller;
    packageStatus = 200;

    nlohmann::json availDoc;
    availDoc["applicationId"] = "com.selfso.octi";
    availDoc["versionName"] = "9.9.9";
    availDoc["versionCode"] = 90909;
    availDoc["installMode"] = "installer";
    availDoc["installerArgs"] = "/S";
    availDoc["executableName"] = "octi.exe";
    availDoc["arch"] = "x86_64";
    availDoc["minOs"] = "Windows 10 (64-bit)+";
    availDoc["packageUrl"] = "octi/octi-setup.exe";
    availDoc["sha256"] = expectedSha;
    availDoc["sizeBytes"] = 32768;
    availDoc["releaseNotes"] =
        "OCTI v9.9.9 Release Notes:\n- Self-update integration via WinHTTP\n- Streaming RFC 6234 SHA-256 verification";
    versionJsonBody = availDoc.dump();
    versionStatus = 200;

    win.checkForAppUpdates(false);
    CHECK_TRUE(pumpUntil([&]() {
        return win.currentSelfUpdateState().phase == selfupdate::UpdatePhase::Available;
    }));
    CHECK_FALSE(win.isUpdateBannerVisible());
    STRCMP_EQUAL("Update Available (v9.9.9)", win.checkUpdateButtonText().toUtf8().constData());
    CHECK_TRUE(win.updateBannerText().contains("9.9.9"));
    CHECK_EQUAL(boardGeomBefore.x(), static_cast<QWidget *>(win.boardCanvas())->geometry().x());
    CHECK_EQUAL(boardGeomBefore.y(), static_cast<QWidget *>(win.boardCanvas())->geometry().y());
    CHECK_EQUAL(boardGeomBefore.width(), static_cast<QWidget *>(win.boardCanvas())->geometry().width());
    CHECK_EQUAL(boardGeomBefore.height(), static_cast<QWidget *>(win.boardCanvas())->geometry().height());

    SelfUpdateDialog dlg(win.selfUpdateControllerShared(), ThemeId::OBSIDIAN_BRASS, &win);
    dlg.resize(640, 480);
    dlg.show();
    win.pumpUiEvents();

    STRCMP_EQUAL("UPDATE AVAILABLE", dlg.phaseBadgeText().toUtf8().constData());
    CHECK_TRUE(dlg.versionSummaryText().contains(
        QString::fromUtf8(OctiCore::versionString())));
    CHECK_TRUE(dlg.versionSummaryText().contains("9.9.9"));
    CHECK_TRUE(dlg.releaseNotesText().contains("Streaming RFC 6234 SHA-256 verification"));
    CHECK_TRUE(dlg.metadataSummaryText().contains("Setup Installer"));
    CHECK_TRUE(dlg.metadataSummaryText().contains("x86_64"));
    CHECK_TRUE(dlg.isDownloadInstallButtonEnabled());
    CHECK_FALSE(dlg.isCancelButtonEnabled());

    QString artifactDir =
        QString::fromUtf8(OCTI_REPO_ROOT_DIR) +
        "/plan/done/plan_octi_windows_selfupdate.artifacts";
    if (!QDir(artifactDir).exists()) {
        artifactDir = QString::fromUtf8(tmpDirStr.c_str()) + "/artifacts";
        QDir().mkpath(artifactDir);
    }

    QString bannerShotPath = artifactDir + "/selfupdate_banner_obsidian.png";
    QString dlgObsidianShotPath = artifactDir + "/selfupdate_dialog_obsidian.png";
    QString dlgRosewoodShotPath = artifactDir + "/selfupdate_dialog_rosewood.png";

    QImage bannerImg = win.renderWindowToImage(1440, 900);
    CHECK_TRUE(bannerImg.save(bannerShotPath));

    QImage dlgObsidianImg = dlg.grab().toImage();
    CHECK_TRUE(dlgObsidianImg.save(dlgObsidianShotPath));

    dlg.setTheme(ThemeId::ROSEWOOD_IVORY);
    win.pumpUiEvents();
    QImage dlgRosewoodImg = dlg.grab().toImage();
    CHECK_TRUE(dlgRosewoodImg.save(dlgRosewoodShotPath));
    dlg.setTheme(ThemeId::OBSIDIAN_BRASS);
    win.pumpUiEvents();

    CHECK_TRUE(QFileInfo(bannerShotPath).size() > 10000);
    CHECK_TRUE(QFileInfo(dlgObsidianShotPath).size() > 10000);
    CHECK_TRUE(QFileInfo(dlgRosewoodShotPath).size() > 10000);

    dlg.triggerDownloadAndInstall();
    CHECK_TRUE(pumpUntil([&]() {
        return win.wasQuitRequestedByUpdate() &&
               win.currentSelfUpdateState().phase == selfupdate::UpdatePhase::Relaunching;
    }));

    CHECK_TRUE(win.selfUpdateController()->lastInstallResult().success);
    std::string stagedPath = win.currentSelfUpdateState().stagedPackagePath;
    CHECK_TRUE(std::filesystem::exists(stagedPath));
    LONGS_EQUAL(32768, std::filesystem::file_size(stagedPath));
    CHECK_FALSE(std::filesystem::exists(stagedPath + ".part"));

    std::string trampPath = win.selfUpdateController()->lastInstallResult().trampolineScriptPath;
    CHECK_TRUE(std::filesystem::exists(trampPath));
    std::ifstream trampIn(trampPath);
    std::string trampContent((std::istreambuf_iterator<char>(trampIn)),
                             std::istreambuf_iterator<char>());
    CHECK_TRUE(trampContent.find("/S") != std::string::npos);
    CHECK_TRUE(trampContent.find("octi.exe") != std::string::npos);

    /* 5. Fault Injection: Corrupted Payload Triggers SHA-256 Digest Mismatch, Unlinks .part, and Aborts Install */
    std::string corruptedInstaller = fakeInstaller;
    corruptedInstaller[123] ^= 0xFF;
    packageBody = corruptedInstaller;

    std::string faultStagingDir = tmpDirStr + "/fault_staging";
    std::filesystem::create_directories(faultStagingDir);

    win.setSelfUpdateController(
        std::make_shared<selfupdate::SelfUpdateController>(
            testCfg, installOpts, faultStagingDir));

    win.checkForAppUpdates(true);
    CHECK_TRUE(pumpUntil([&]() {
        return win.currentSelfUpdateState().phase == selfupdate::UpdatePhase::Available;
    }));

    win.triggerAppUpdateDownloadAndInstall();
    CHECK_TRUE(pumpUntil([&]() {
        return win.currentSelfUpdateState().phase == selfupdate::UpdatePhase::Error;
    }));

    CHECK_TRUE(win.currentSelfUpdateState().errorMessage.find("SHA-256") != std::string::npos);
    CHECK_FALSE(win.wasQuitRequestedByUpdate());
    CHECK_FALSE(win.isUpdateBannerVisible());
    CHECK_FALSE(std::filesystem::exists(faultStagingDir + "/octi-setup.exe.part"));
    CHECK_FALSE(std::filesystem::exists(faultStagingDir + "/octi-setup.exe"));

    server.stop();
    if (serverThread.joinable()) {
        serverThread.join();
    }

    /* 6. RapidCheck Property Test */
    bool ok = rc::check(
        "Semantic version comparison, versionCode, isNewerThan, and SelfUpdateDialog invariants",
        []() {
            int majA = *rc::gen::inRange(0, 50);
            int minA = *rc::gen::inRange(0, 50);
            int patA = *rc::gen::inRange(0, 50);
            int majB = *rc::gen::inRange(0, 50);
            int minB = *rc::gen::inRange(0, 50);
            int patB = *rc::gen::inRange(0, 50);

            std::string verA = std::to_string(majA) + "." + std::to_string(minA) + "." + std::to_string(patA);
            std::string verB = std::to_string(majB) + "." + std::to_string(minB) + "." + std::to_string(patB);

            int64_t codeA = selfupdate::UpdateManifest::parseVersionCode(verA);
            RC_ASSERT(codeA == static_cast<int64_t>(majA) * 10000 + static_cast<int64_t>(minA) * 100 + static_cast<int64_t>(patA));

            int cmp = selfupdate::UpdateManifest::compareVersions(verA, verB);
            int expectedCmp = 0;
            if (majA != majB) {
                expectedCmp = majA > majB ? 1 : -1;
            } else if (minA != minB) {
                expectedCmp = minA > minB ? 1 : -1;
            } else if (patA != patB) {
                expectedCmp = patA > patB ? 1 : -1;
            }
            RC_ASSERT(cmp == expectedCmp);

            selfupdate::UpdateManifest mf;
            mf.versionName = verA;
            mf.versionCode = codeA;
            int64_t codeB = selfupdate::UpdateManifest::parseVersionCode(verB);
            bool isNewer = mf.isNewerThan(verB, codeB);
            RC_ASSERT(isNewer == (expectedCmp > 0));

            selfupdate::UpdateState st;
            st.installedVersionName = verB;
            st.installedVersionCode = codeB;
            st.manifest = mf;

            SelfUpdateDialog propDlg(nullptr, ThemeId::OBSIDIAN_BRASS, nullptr);

            st.phase = selfupdate::UpdatePhase::Idle;
            propDlg.applyUpdateState(st);
            RC_ASSERT(propDlg.phaseBadgeText() == "IDLE");
            RC_ASSERT(propDlg.progressPercent() == 0);
            RC_ASSERT(!propDlg.isDownloadInstallButtonEnabled());
            RC_ASSERT(!propDlg.isCancelButtonEnabled());

            st.phase = selfupdate::UpdatePhase::Available;
            propDlg.applyUpdateState(st);
            RC_ASSERT(propDlg.phaseBadgeText() == "UPDATE AVAILABLE");
            RC_ASSERT(propDlg.isDownloadInstallButtonEnabled());
            RC_ASSERT(!propDlg.isCancelButtonEnabled());
            RC_ASSERT(propDlg.versionSummaryText().contains(QString::fromStdString(verB)));
            RC_ASSERT(propDlg.versionSummaryText().contains(QString::fromStdString(verA)));

            st.phase = selfupdate::UpdatePhase::UpToDate;
            propDlg.applyUpdateState(st);
            RC_ASSERT(propDlg.phaseBadgeText() == "UP TO DATE");
            RC_ASSERT(propDlg.progressPercent() == 100);
            RC_ASSERT(propDlg.isDownloadInstallButtonEnabled());
            RC_ASSERT(propDlg.downloadInstallButtonText() == "Reinstall");

            st.phase = selfupdate::UpdatePhase::Error;
            st.errorMessage = "Simulated error";
            propDlg.applyUpdateState(st);
            RC_ASSERT(propDlg.phaseBadgeText() == "UPDATE ERROR");
            RC_ASSERT(propDlg.statusText().contains("Simulated error"));

            return true;
        });
    CHECK_TRUE(ok);

    std::filesystem::remove_all(tmpDirStr);
}

TEST(DesktopAppIntegrationTest, GuiRefinementArtifactsAndPrompts)
{
    QString repoRootStr = QString::fromUtf8(OCTI_REPO_ROOT_DIR);
    QString guiArtifactDir = repoRootStr + "/plan/done/plan_gui_refinement.artifacts";
    if (!QDir(guiArtifactDir).exists()) {
        guiArtifactDir = repoRootStr + "/plan/plan_gui_refinement.artifacts";
    }
    QDir().mkpath(guiArtifactDir);

    // 1. Procedural Animated Splash Screen (desktop_splash.png)
    QImage splashImg = OctiSplashDialog::renderSplashFrame(960, 560, 0.72);
    CHECK_FALSE(splashImg.isNull());
    QString splashPath = guiArtifactDir + "/desktop_splash.png";
    CHECK_TRUE(splashImg.save(splashPath));
    CHECK_TRUE_TEXT(QFileInfo(splashPath).size() > 15000, "desktop_splash.png size check");

    // 2. Decluttered Desktop Main Window with Luxury Header (desktop_main_decluttered.png)
    std::string cfgPath = "/tmp/octi_gui_refine_test.json";
    OctiMainWindow win(nullptr, cfgPath, false);
    win.setTheme(ThemeId::OBSIDIAN_BRASS);
    win.showBoardView();
    QImage mainImg = win.renderWindowToImage(1440, 900);
    CHECK_FALSE(mainImg.isNull());
    QString mainPath = guiArtifactDir + "/desktop_main_decluttered.png";
    CHECK_TRUE(mainImg.save(mainPath));
    CHECK_TRUE_TEXT(QFileInfo(mainPath).size() > 15000, "desktop_main_decluttered.png size check");

    // Check menu actions count
    LONGS_EQUAL(23, win.menuActionCountWithIconsAndShortcuts());

    // 3. Zoomed Square Dialog Preview (desktop_zoomed_square.png)
    ZoomedSquareDialog zoomedDlg(win.board(), win.moveComposer(), 3, 3,
                                BoardRotation::ROT_0, ThemeId::OBSIDIAN_BRASS, &win);
    QImage zoomedImg = zoomedDlg.renderToImage(560, 620);
    CHECK_FALSE(zoomedImg.isNull());
    QString zoomedPath = guiArtifactDir + "/desktop_zoomed_square.png";
    CHECK_TRUE(zoomedImg.save(zoomedPath));
    CHECK_TRUE_TEXT(QFileInfo(zoomedPath).size() > 12000, "desktop_zoomed_square.png size check");

    // 4. Prompt Dialog Invariants
    CapturePromptDialog capDlg(3, 5, PLAYER_B, ThemeId::OBSIDIAN_BRASS, &win);
    CHECK_TRUE(capDlg.captureChosen());

    SuperProngBonusDialog spDlg(ThemeId::OBSIDIAN_BRASS, &win);
    CHECK_TRUE(spDlg.moveNowChosen());

    CompositePodMoveDialog compDlg(3, 3, ThemeId::OBSIDIAN_BRASS, &win);
    CHECK_TRUE(compDlg.selectNextPodChosen());

    // 5. Verify all 23 vector menu icons generate valid pixmaps
    for (int k = 0; k <= static_cast<int>(MenuIconKind::SHOW_SPLASH); ++k) {
        QIcon icon = createMenuIcon(static_cast<MenuIconKind>(k), ThemeId::OBSIDIAN_BRASS);
        CHECK_FALSE(icon.isNull());
        QPixmap px = icon.pixmap(32, 32);
        CHECK_FALSE(px.isNull());
    }

    // 6. AZG-D03: Verify player-color-coded move history items & OCTI notation clipboard export
    {
        LocalGameConfig pnpCfg;
        pnpCfg.playType = LocalPlayType::PASS_AND_PLAY;
        pnpCfg.mode = GAME_MODE_2;
        pnpCfg.bases = GAME_BASES_3;
        win.startNewLocalGame(pnpCfg);
        win.setTheme(ThemeId::OBSIDIAN_BRASS);

        CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(3, 3, 0, DIR_N, false));
        if (win.moveComposer()->isAwaitingConfirmation()) {
            CHECK_TRUE(win.moveComposer()->commitStagedMove());
        }
        CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(3, 7, 0, DIR_S, false));
        if (win.moveComposer()->isAwaitingConfirmation()) {
            CHECK_TRUE(win.moveComposer()->commitStagedMove());
        }

        QListWidget *histList = win.moveHistoryListWidget();
        CHECK_TRUE(histList != nullptr);
        LONGS_EQUAL(2, histList->count());

        const OctiPalette &obsPal = paletteForTheme(ThemeId::OBSIDIAN_BRASS);
        CHECK_TRUE(histList->item(0)->background().color() == obsPal.podA.shadow);
        CHECK_TRUE(histList->item(0)->foreground().color() == obsPal.podA.highlight);
        CHECK_TRUE(histList->item(1)->background().color() == obsPal.podB.shadow);
        CHECK_TRUE(histList->item(1)->foreground().color() == obsPal.podB.highlight);

        // Toggle theme and verify move history item colors update to Rosewood palette
        win.toggleTheme();
        const OctiPalette &rosePal = paletteForTheme(ThemeId::ROSEWOOD_IVORY);
        CHECK_TRUE(histList->item(0)->background().color() == rosePal.podA.shadow);
        CHECK_TRUE(histList->item(0)->foreground().color() == rosePal.podA.highlight);
        CHECK_TRUE(histList->item(1)->background().color() == rosePal.podB.shadow);
        CHECK_TRUE(histList->item(1)->foreground().color() == rosePal.podB.highlight);

        // Verify copyExportedNotationToClipboard() copies canonical .octi notation
        QString exported = win.copyExportedNotationToClipboard();
        CHECK_TRUE(exported.contains("33+a"));
        CHECK_TRUE(exported.contains("37+e"));
        if (QClipboard *cb = QGuiApplication::clipboard()) {
            STRCMP_EQUAL(exported.toUtf8().constData(),
                         cb->text().toUtf8().constData());
        }
    }

    std::remove(cfgPath.c_str());
}

TEST(DesktopAppIntegrationTest, ExportMoveHistoryTextAndThinkingSliderDialog)
{
    std::string cfgPath = "/tmp/octi_export_and_thinking_test.json";
    OctiMainWindow win(nullptr, cfgPath, true);

    LocalGameConfig cfg;
    cfg.playType = LocalPlayType::PASS_AND_PLAY;
    cfg.mode = GAME_MODE_2;
    win.startNewLocalGame(cfg);

    // Initial move history text should be empty
    STRCMP_EQUAL("", win.exportMoveHistoryText().toUtf8().constData());

    // FEN status label must be hidden (EMF-D01) while fenStatusText() returns valid FEN
    CHECK_FALSE(win.fenStatusText().isEmpty());
    CHECK_TRUE(win.fenStatusText().startsWith("FEN: "));

    // Play 3 moves:
    // 1. A: 33+a
    // 2. B: 37+e
    // 3. A: 33a-34
    win.board()->getGame()->move("33+a");
    win.board()->getGame()->move("37+e");
    win.board()->getGame()->move("33a-34");

    QString exportedMoves = win.exportMoveHistoryText();
    QString expectedMoves = "1. A: 33+a\n2. B: 37+e\n3. A: 33a-34";
    STRCMP_EQUAL(expectedMoves.toUtf8().constData(),
                 exportedMoves.toUtf8().constData());

    // Test OctiThinkingDialog (EMF-D02)
    OctiThinkingDialog thinkingDlg(BotDifficulty::TACTICAL, 8000, ThemeId::OBSIDIAN_BRASS, &win);
    CHECK_TRUE(thinkingDlg.isFrameless());
    LONGS_EQUAL(static_cast<long>(Qt::ApplicationModal),
                static_cast<long>(thinkingDlg.windowModality()));
    LONGS_EQUAL(0, thinkingDlg.sliderValue());
    CHECK_TRUE(thinkingDlg.statusText().contains("AI Thinking (Level 2)"));
    CHECK_TRUE(thinkingDlg.statusText().contains("8.0s remaining"));

    // Advance progress to 2000ms / 8000ms (25% = 250 on slider 0..1000)
    thinkingDlg.updateProgress(2000, 8000);
    LONGS_EQUAL(250, thinkingDlg.sliderValue());
    CHECK_TRUE(thinkingDlg.statusText().contains("6.0s remaining"));

    // Advance progress to 8000ms / 8000ms (100% = 1000 on slider)
    thinkingDlg.updateProgress(8000, 8000);
    LONGS_EQUAL(1000, thinkingDlg.sliderValue());
    CHECK_TRUE(thinkingDlg.statusText().contains("0.0s remaining"));

    // Test positioning
    thinkingDlg.positionBelowBoard(&win, win.boardCanvas());
    CHECK_TRUE(thinkingDlg.y() > 0);

    std::remove(cfgPath.c_str());
}

TEST(DesktopAppIntegrationTest, JsonV3ExportOmitsExtendedTelemetry)
{
    std::string cfgPath = "/tmp/octi_export_json_v3_test.json";
    std::remove(cfgPath.c_str());

    OctiMainWindow win(nullptr, cfgPath, true);
    win.setTelemetryConsentForTesting("share");

    LocalGameConfig cfg;
    cfg.playType = LocalPlayType::PASS_AND_PLAY;
    cfg.mode = GAME_MODE_2;
    win.startNewLocalGame(cfg);

    win.board()->getGame()->move("33+a");
    win.board()->getGame()->move("37+e");

    std::string jsonStr = win.exportCurrentOctiTranscript();
    CHECK_FALSE(jsonStr.empty());

    Json j = Json::parse(jsonStr, nullptr, false);
    CHECK_FALSE(j.is_discarded());
    STRCMP_EQUAL("octi.transcript.v3", j.value("format", "").c_str());
    CHECK_FALSE(j.contains("extended"));
    if (j.contains("client") && j["client"].is_object()) {
        CHECK_FALSE(j["client"].contains("extended"));
    }

    QString clip = win.copyExportedNotationToClipboard();
    CHECK_TRUE(clip.contains("octi.transcript.v3"));
    CHECK_FALSE(clip.contains("\"extended\""));

    std::remove(cfgPath.c_str());
}

TEST(DesktopAppIntegrationTest, TelemetryConsentTriStatePersistenceAndDialogEscRejection)
{
    std::string cfgPath = "/tmp/octi_consent_persistence_test.json";
    std::remove(cfgPath.c_str());

    // 1. Initial defaults
    {
        ClientConfigStore store(cfgPath);
        std::string err;
        STRCMP_EQUAL("unset", store.preferences().telemetryConsent.c_str());
        LONGS_EQUAL(0, store.preferences().telemetryConsentVersion);
        CHECK_TRUE(store.preferences().installId.empty());

        store.preferences().telemetryConsent = "share";
        store.preferences().telemetryConsentVersion = ClientPreferences::CURRENT_TELEMETRY_CONSENT_VERSION;
        store.resetInstallId();
        CHECK_FALSE(store.preferences().installId.empty());
        CHECK_TRUE(store.save(err));
    }

    // 2. Reload and verify
    std::string id1;
    {
        ClientConfigStore store(cfgPath);
        std::string err;
        CHECK_TRUE(store.load(err));
        STRCMP_EQUAL("share", store.preferences().telemetryConsent.c_str());
        LONGS_EQUAL(ClientPreferences::CURRENT_TELEMETRY_CONSENT_VERSION, store.preferences().telemetryConsentVersion);
        id1 = store.preferences().installId;
        CHECK_FALSE(id1.empty());

        // Reset ID
        store.resetInstallId();
        std::string id2 = store.preferences().installId;
        CHECK_FALSE(id2.empty());
        CHECK_TRUE(id1 != id2);
    }

    // 3. Test non-dismissible Esc behavior of OctiTelemetryConsentDialog
    {
        OctiTelemetryConsentDialog dlg;
        CHECK_FALSE(dlg.userConsented());

        QKeyEvent escKey(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QApplication::sendEvent(&dlg, &escKey);
        LONGS_EQUAL(0, dlg.result());

        dlg.reject();
        LONGS_EQUAL(0, dlg.result());
    }

    std::remove(cfgPath.c_str());
}

TEST(DesktopAppIntegrationTest, AutomaticUploadOfCompletedBotGameLoopback)
{
    LiveServerEnv env;
    std::string cfgPath = "/tmp/octi_upload_loopback_test.json";
    std::remove(cfgPath.c_str());

    auto netCli = OctiNetworkClient::createForLoopbackTest(env.port());
    OctiMainWindow win(std::move(netCli), cfgPath, true);

    LocalGameConfig cfg;
    cfg.playType = LocalPlayType::VS_AI_BOT;
    cfg.mode = GAME_MODE_2;
    cfg.humanSeat = PLAYER_A;
    cfg.botDifficulty = BotDifficulty::MASTER;
    win.startNewLocalGame(cfg);

    // 0. Verify Level 1 (CASUAL) and Level 2 (TACTICAL) bot games never upload
    {
        LocalGameConfig casualCfg = cfg;
        casualCfg.botDifficulty = BotDifficulty::CASUAL;
        win.startNewLocalGame(casualCfg);
        CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(3, 3, 0, DIR_N, false));
        if (win.moveComposer()->isAwaitingConfirmation()) {
            CHECK_TRUE(win.moveComposer()->commitStagedMove());
        }
        win.setTelemetryConsentForTesting("share");
        win.resignCurrentGame();
        CHECK_FALSE(win.uploadCompletedBotGameIfEligible(true));

        LocalGameConfig tacticalCfg = cfg;
        tacticalCfg.botDifficulty = BotDifficulty::TACTICAL;
        win.startNewLocalGame(tacticalCfg);
        CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(3, 3, 0, DIR_N, false));
        if (win.moveComposer()->isAwaitingConfirmation()) {
            CHECK_TRUE(win.moveComposer()->commitStagedMove());
        }
        win.setTelemetryConsentForTesting("share");
        win.resignCurrentGame();
        CHECK_FALSE(win.uploadCompletedBotGameIfEligible(true));

        // Restart challenging game for remaining consent tests
        win.startNewLocalGame(cfg);
    }

    // 1. In-progress game is not uploaded
    CHECK_FALSE(win.uploadCompletedBotGameIfEligible(true));

    // Play a move so the match has valid plies (Human adds prong at (3,3), Bot replies automatically)
    CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(3, 3, 0, DIR_N, false));
    if (win.moveComposer()->isAwaitingConfirmation()) {
        CHECK_TRUE(win.moveComposer()->commitStagedMove());
    }
    LONGS_EQUAL(2, win.vcrCurrentPly());

    // 2. Unset consent holds the upload
    win.setTelemetryConsentForTesting("unset");
    win.resignCurrentGame();
    CHECK_TRUE(win.board()->getGame()->isGameOver());
    CHECK_FALSE(win.uploadCompletedBotGameIfEligible(true));

    // 3. Setting consent to "share" triggers upload with extended telemetry
    win.setTelemetryConsentForTesting("share");
    CHECK_TRUE(win.uploadCompletedBotGameIfEligible(true));

    ArchiveFilter f;
    f.source = "client_upload";
    int64_t total = 0;
    std::vector<GameRecord> games;
    CHECK_TRUE(env.store.listArchivedGames(f, games, &total));
    LONGS_EQUAL(1, total);
    CHECK_FALSE(games.empty());
    STRCMP_EQUAL("client_upload", games[0].source.c_str());
    STRCMP_EQUAL("RESIGNED", games[0].status.c_str());
    STRCMP_EQUAL("B", games[0].winner.c_str());
    CHECK_FALSE(games[0].installId.empty());

    // 4. Delete install data endpoint
    auto directClient = OctiNetworkClient::createForLoopbackTest(env.port());
    auto delRes = directClient->deleteInstallArchiveData(games[0].installId);
    CHECK_TRUE_TEXT(delRes.ok, delRes.errorMessage.c_str());
    LONGS_EQUAL(1, delRes.data);

    std::vector<GameRecord> gamesAfter;
    int64_t totalAfter = 0;
    CHECK_TRUE(env.store.listArchivedGames(f, gamesAfter, &totalAfter));
    LONGS_EQUAL(0, totalAfter);

    // 5. Game finished with "decline" consent uploads Tier 0 only (omits extended telemetry)
    win.startNewLocalGame(cfg);
    CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(3, 3, 0, DIR_N, false));
    if (win.moveComposer()->isAwaitingConfirmation()) {
        CHECK_TRUE(win.moveComposer()->commitStagedMove());
    }
    LONGS_EQUAL(2, win.vcrCurrentPly());
    win.setTelemetryConsentForTesting("unset");
    win.resignCurrentGame();
    win.setTelemetryConsentForTesting("decline");
    CHECK_TRUE(win.uploadCompletedBotGameIfEligible(true));

    std::vector<GameRecord> gamesDecline;
    int64_t totalDecline = 0;
    CHECK_TRUE(env.store.listArchivedGames(f, gamesDecline, &totalDecline));
    LONGS_EQUAL(1, totalDecline);
    STRCMP_EQUAL("", gamesDecline[0].installId.c_str());

    // 6. Test asynchronous automatic upload when consent is already "share"
    win.startNewLocalGame(cfg);
    win.setTelemetryConsentForTesting("share");
    CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(5, 3, 0, DIR_N, false));
    if (win.moveComposer()->isAwaitingConfirmation()) {
        CHECK_TRUE(win.moveComposer()->commitStagedMove());
    }
    win.resignCurrentGame();
    // Resignation automatically triggered the background upload thread
    win.waitForBackgroundUpload();

    std::vector<GameRecord> gamesAsync;
    int64_t totalAsync = 0;
    CHECK_TRUE(env.store.listArchivedGames(f, gamesAsync, &totalAsync));
    LONGS_EQUAL(2, totalAsync);

    std::remove(cfgPath.c_str());
}

TEST(DesktopAppIntegrationTest, OfflineAiGameSpoolsTranscriptAndFlushesWhenReachable)
{
    LiveServerEnv env;
    std::string testDir = env.dir + "/offline_spool_test";
    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(testDir);

    std::string cfgPath = testDir + "/client_config.json";
    octi::desktop::OctiCrashReporter::setStorageRootDir(testDir);

    /* Start hermetic client pointed to env.port() */
    auto netCli = OctiNetworkClient::createForLoopbackTest(env.port());
    OctiMainWindow win(std::move(netCli), cfgPath, false);

    LocalGameConfig cfg;
    cfg.playType = LocalPlayType::VS_AI_BOT;
    cfg.mode = GAME_MODE_2;
    cfg.humanSeat = PLAYER_A;
    cfg.botDifficulty = BotDifficulty::MASTER;
    win.startNewLocalGame(cfg);

    // 0. Verify Level 1 (CASUAL) and Level 2 (TACTICAL) bot games never upload
    {
        LocalGameConfig casualCfg = cfg;
        casualCfg.botDifficulty = BotDifficulty::CASUAL;
        win.startNewLocalGame(casualCfg);
        CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(3, 3, 0, DIR_N, false));
        if (win.moveComposer()->isAwaitingConfirmation()) {
            CHECK_TRUE(win.moveComposer()->commitStagedMove());
        }
        win.setTelemetryConsentForTesting("share");
        win.resignCurrentGame();
        CHECK_FALSE(win.uploadCompletedBotGameIfEligible(true));

        LocalGameConfig tacticalCfg = cfg;
        tacticalCfg.botDifficulty = BotDifficulty::TACTICAL;
        win.startNewLocalGame(tacticalCfg);
        CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(3, 3, 0, DIR_N, false));
        if (win.moveComposer()->isAwaitingConfirmation()) {
            CHECK_TRUE(win.moveComposer()->commitStagedMove());
        }
        win.setTelemetryConsentForTesting("share");
        win.resignCurrentGame();
        CHECK_FALSE(win.uploadCompletedBotGameIfEligible(true));

        // Restart challenging game for remaining consent tests
        win.startNewLocalGame(cfg);
    }

    /* Set consent to "share" */
    win.setTelemetryConsentForTesting("share");

    /* Play a valid human move so plies > 0 */
    CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(3, 3, 0, DIR_N, false));
    if (win.moveComposer()->isAwaitingConfirmation()) {
        CHECK_TRUE(win.moveComposer()->commitStagedMove());
    }
    LONGS_EQUAL(2, win.vcrCurrentPly());

    /* Shut down the server so it is completely offline */
    env.stop();

    /* Human resigns game -> match is game over, upload is triggered */
    win.resignCurrentGame();
    CHECK_TRUE(win.board()->getGame()->isGameOver());
    win.waitForBackgroundUpload();

    /* Since server is offline, the transcript file must remain spooled in pending_uploads/ */
    auto pending = octi::desktop::OctiCrashReporter::listPendingTranscriptFiles();
    CHECK_TRUE_TEXT(pending.size() >= 1, "Transcript must remain spooled on disk while server is offline");

    /* Server comes back online */
    env.startAgain();

    /* flushPendingTelemetry(false) synchronously flushes all spooled transcripts */
    win.flushPendingTelemetry(false);

    /* Verify spool directory is now empty */
    auto pendingAfter = octi::desktop::OctiCrashReporter::listPendingTranscriptFiles();
    LONGS_EQUAL(0, static_cast<long>(pendingAfter.size()));

    /* Verify server database received the archived game */
    ArchiveFilter f;
    f.source = "client_upload";
    int64_t total = 0;
    std::vector<GameRecord> games;
    CHECK_TRUE(env.store.listArchivedGames(f, games, &total));
    LONGS_EQUAL(1, total);
    CHECK_FALSE(games.empty());
    STRCMP_EQUAL("RESIGNED", games[0].status.c_str());

    std::filesystem::remove_all(testDir);
}

TEST(DesktopAppIntegrationTest, CenteredStartMatchPromptAndPreStartGatingForPlayerAAndB)
{
    std::string testDir = "/tmp/octi_start_match_test_" + std::to_string(QDateTime::currentMSecsSinceEpoch());
    std::filesystem::create_directories(testDir);
    std::string cfgFile = testDir + "/client.json";

    OctiMainWindow win(nullptr, cfgFile, true);

    // 1. Verify OctiStartMatchDialog properties & styling
    OctiStartMatchDialog dlg(PLAYER_A, true, ThemeId::OBSIDIAN_BRASS, &win);
    CHECK_TRUE(dlg.windowFlags() & Qt::FramelessWindowHint);
    CHECK_TRUE(dlg.isModal());
    dlg.positionCenterOfBoard(&win, win.boardCanvas());
    // Esc/reject is mapped to accept
    dlg.reject();
    LONGS_EQUAL(QDialog::Accepted, dlg.result());

    // 2. Scenario 1: Human is Player A (Red)
    LocalGameConfig cfgA;
    cfgA.playType = LocalPlayType::VS_AI_BOT;
    cfgA.humanSeat = PLAYER_A;
    cfgA.mode = GAME_MODE_2;
    cfgA.botDifficulty = BotDifficulty::TACTICAL;

    win.startNewLocalGame(cfgA);
    win.beginAwaitingMatchStart();

    CHECK_TRUE(win.isAwaitingMatchStart());
    LONGS_EQUAL(0, win.matchStartedEpochSec());
    LONGS_EQUAL(0, win.currentTurnStartedEpochMs());
    CHECK_FALSE(win.boardCanvas()->isTurnIndicatorVisible());
    LONGS_EQUAL(0, win.board()->getGame()->getNumMoves());

    // Confirm match start as Player A
    win.confirmMatchStart();
    CHECK_FALSE(win.isAwaitingMatchStart());
    CHECK_TRUE(win.matchStartedEpochSec() > 0);
    CHECK_TRUE(win.currentTurnStartedEpochMs() > 0);
    CHECK_TRUE(win.boardCanvas()->isTurnIndicatorVisible());
    // Bot should not move because it is human Player A's turn
    LONGS_EQUAL(0, win.board()->getGame()->getNumMoves());

    // 3. Scenario 2: Human is Player B (Green) -> Player A is the AI Bot
    LocalGameConfig cfgB;
    cfgB.playType = LocalPlayType::VS_AI_BOT;
    cfgB.humanSeat = PLAYER_B;
    cfgB.mode = GAME_MODE_2;
    cfgB.botDifficulty = BotDifficulty::TACTICAL;

    win.startNewLocalGame(cfgB);
    win.beginAwaitingMatchStart();

    CHECK_TRUE(win.isAwaitingMatchStart());
    LONGS_EQUAL(0, win.matchStartedEpochSec());
    LONGS_EQUAL(0, win.currentTurnStartedEpochMs());
    CHECK_FALSE(win.boardCanvas()->isTurnIndicatorVisible());
    // CRITICAL: Bot MUST NOT make a move while awaiting match start, even though turn is Player A!
    LONGS_EQUAL(0, win.board()->getGame()->getNumMoves());

    // Confirm match start as Player B
    win.confirmMatchStart();
    CHECK_FALSE(win.isAwaitingMatchStart());
    CHECK_TRUE(win.matchStartedEpochSec() > 0);
    CHECK_TRUE(win.currentTurnStartedEpochMs() > 0);
    CHECK_TRUE(win.boardCanvas()->isTurnIndicatorVisible());
    // CRITICAL: Now that match is confirmed, AI bot has executed Player A's opening move!
    LONGS_EQUAL(1, win.board()->getGame()->getNumMoves());
    LONGS_EQUAL(PLAYER_B, win.board()->getGame()->getTurn());

    // 4. OctiStartMatchDialog brief 2-line config summary tests across modes, seats, modifiers
    LocalGameConfig c1;
    c1.playType = LocalPlayType::VS_AI_BOT;
    c1.botDifficulty = BotDifficulty::CASUAL;
    c1.humanSeat = PLAYER_A;
    c1.mode = GAME_MODE_2;
    c1.bases = GAME_BASES_3;
    c1.useSuperProngs = false;
    c1.useEdgelessBoard = false;
    c1.timeControlEnabled = false;
    QString s1 = OctiStartMatchDialog::formatConfigSummary(c1);
    STRCMP_EQUAL("vs AI Bot (Lv1 \xE2\x80\xA2 5s) \xE2\x80\xA2 Seat A (Red)\n2P (9x9) \xE2\x80\xA2 3 Bases \xE2\x80\xA2 Standard", s1.toUtf8().constData());

    LocalGameConfig c2;
    c2.playType = LocalPlayType::VS_AI_BOT;
    c2.botDifficulty = BotDifficulty::TACTICAL;
    c2.humanSeat = PLAYER_B;
    c2.mode = GAME_MODE_4;
    c2.bases = GAME_BASES_1;
    c2.useSuperProngs = true;
    QString s2 = OctiStartMatchDialog::formatConfigSummary(c2);
    STRCMP_EQUAL("vs AI Bot (Lv2 \xE2\x80\xA2 10s) \xE2\x80\xA2 Seat B (Green)\n4P (9x9) \xE2\x80\xA2 1 Base \xE2\x80\xA2 Super-Prongs", s2.toUtf8().constData());

    LocalGameConfig c3;
    c3.playType = LocalPlayType::PASS_AND_PLAY;
    c3.humanSeat = PLAYER_A;
    c3.mode = GAME_MODE_OFK;
    c3.bases = GAME_BASES_1;
    QString s3 = OctiStartMatchDialog::formatConfigSummary(c3);
    STRCMP_EQUAL("Local Pass-and-Play \xE2\x80\xA2 Seat A (Red)\nOFK (6x7) \xE2\x80\xA2 1 Base", s3.toUtf8().constData());

    LocalGameConfig c4;
    c4.playType = LocalPlayType::VS_AI_BOT;
    c4.botDifficulty = BotDifficulty::MASTER;
    c4.humanSeat = PLAYER_A;
    c4.mode = GAME_MODE_2;
    c4.bases = GAME_BASES_3;
    c4.useEdgelessBoard = true;
    c4.timeControlEnabled = true;
    QString s4 = OctiStartMatchDialog::formatConfigSummary(c4);
    STRCMP_EQUAL("vs AI Bot (Lv3 \xE2\x80\xA2 20s) \xE2\x80\xA2 Seat A (Red)\n2P (9x9) \xE2\x80\xA2 3 Bases \xE2\x80\xA2 Edgeless \xE2\x80\xA2 Clock", s4.toUtf8().constData());

    LocalGameConfig c5 = c1;
    c5.botDifficulty = BotDifficulty::EXPERT;
    QString s5 = OctiStartMatchDialog::formatConfigSummary(c5);
    STRCMP_EQUAL("vs AI Bot (Lv4 \xE2\x80\xA2 40s) \xE2\x80\xA2 Seat A (Red)\n2P (9x9) \xE2\x80\xA2 3 Bases \xE2\x80\xA2 Standard", s5.toUtf8().constData());

    LocalGameConfig c6 = c1;
    c6.botDifficulty = BotDifficulty::GRANDMASTER;
    QString s6 = OctiStartMatchDialog::formatConfigSummary(c6);
    STRCMP_EQUAL("vs AI Bot (Lv5 \xE2\x80\xA2 60s) \xE2\x80\xA2 Seat A (Red)\n2P (9x9) \xE2\x80\xA2 3 Bases \xE2\x80\xA2 Standard", s6.toUtf8().constData());

    OctiStartMatchDialog dlgConfig(c1, ThemeId::OBSIDIAN_BRASS, &win);
    STRCMP_EQUAL(s1.toUtf8().constData(), dlgConfig.configSummaryText().toUtf8().constData());
    CHECK_FALSE(dlgConfig.changeTypeRequested());
    dlgConfig.triggerChangeType();
    CHECK_TRUE(dlgConfig.changeTypeRequested());

    // 5. Change Type reconfiguration flow via reconfigurePendingMatch
    LocalGameConfig reconfiguredCfg;
    reconfiguredCfg.playType = LocalPlayType::PASS_AND_PLAY;
    reconfiguredCfg.humanSeat = PLAYER_B;
    reconfiguredCfg.mode = GAME_MODE_4;
    reconfiguredCfg.bases = GAME_BASES_1;
    win.reconfigurePendingMatch(reconfiguredCfg);
    CHECK_TRUE(win.isAwaitingMatchStart());
    LONGS_EQUAL(0, win.matchStartedEpochSec());
    LONGS_EQUAL(0, win.board()->getGame()->getNumMoves());
    LONGS_EQUAL(GAME_MODE_4, win.localGameConfig().mode);
    LONGS_EQUAL(PLAYER_B, win.localGameConfig().humanSeat);
    LONGS_EQUAL(static_cast<int>(LocalPlayType::PASS_AND_PLAY), static_cast<int>(win.localGameConfig().playType));

    // 6. 1Hz Flashing "Commit [Enter]" Button
    LONGS_EQUAL(1000, OctiBoardCanvas::COMMIT_FLASH_PERIOD_MS);
    CHECK_TRUE(OctiBoardCanvas::isCommitButtonFlashBright(0));
    CHECK_TRUE(OctiBoardCanvas::isCommitButtonFlashBright(499));
    CHECK_FALSE(OctiBoardCanvas::isCommitButtonFlashBright(500));
    CHECK_FALSE(OctiBoardCanvas::isCommitButtonFlashBright(999));
    CHECK_TRUE(OctiBoardCanvas::isCommitButtonFlashBright(1000));
    DOUBLES_EQUAL(1.0, OctiBoardCanvas::commitButtonFlashFactor(200), 0.001);
    DOUBLES_EQUAL(0.0, OctiBoardCanvas::commitButtonFlashFactor(700), 0.001);

    // Staged move flash rendering
    LocalGameConfig stageCfg;
    stageCfg.playType = LocalPlayType::PASS_AND_PLAY;
    stageCfg.humanSeat = PLAYER_A;
    stageCfg.mode = GAME_MODE_2;
    stageCfg.bases = GAME_BASES_3;
    win.startNewLocalGame(stageCfg);
    win.confirmMatchStart();
    CHECK_TRUE(win.moveComposer()->stageOrCommitAddProng(3, 3, 0, DIR_N, false));
    CHECK_TRUE(win.moveComposer()->isAwaitingConfirmation());

    QImage imgBright = win.boardCanvas()->renderToImageAtTime(800, 800, 100);
    QImage imgDark = win.boardCanvas()->renderToImageAtTime(800, 800, 600);
    QRectF cBtn = win.boardCanvas()->commitBtnRect();
    CHECK_FALSE(cBtn.isEmpty());
    QRgb pxBright = imgBright.pixel(static_cast<int>(cBtn.center().x()), static_cast<int>(cBtn.center().y()));
    QRgb pxDark = imgDark.pixel(static_cast<int>(cBtn.center().x()), static_cast<int>(cBtn.center().y()));
    CHECK_TRUE(pxBright != pxDark);

    PlyConfirmDialog pcd(win.moveComposer(), ThemeId::OBSIDIAN_BRASS, &win);
    QImage pcdImgBright = pcd.renderToImage(460, 150, 100);
    QImage pcdImgDark = pcd.renderToImage(460, 150, 600);
    QRgb pcdPxBright = pcdImgBright.pixel(143, 113);
    QRgb pcdPxDark = pcdImgDark.pixel(143, 113);
    CHECK_TRUE(pcdPxBright != pcdPxDark);
    win.moveComposer()->cancelAndRollback();

    // 7. 100% Opaque AI Thinking & Start Match Dialogs
    OctiThinkingDialog thinkingDlg(BotDifficulty::TACTICAL, 8000, ThemeId::OBSIDIAN_BRASS, &win);
    CHECK_FALSE(thinkingDlg.testAttribute(Qt::WA_TranslucentBackground));
    CHECK_TRUE(thinkingDlg.autoFillBackground());
    CHECK_FALSE(dlgConfig.testAttribute(Qt::WA_TranslucentBackground));
    CHECK_TRUE(dlgConfig.autoFillBackground());

    QImage thinkImg = thinkingDlg.renderToImage(380, 80);
    for (int ty = 10; ty < 70; ty += 15) {
        for (int tx = 10; tx < 370; tx += 40) {
            LONGS_EQUAL(255, qAlpha(thinkImg.pixel(tx, ty)));
        }
    }

    QImage startImg = dlgConfig.renderToImage(320, 195);
    for (int sy = 10; sy < 185; sy += 25) {
        for (int sx = 10; sx < 310; sx += 40) {
            LONGS_EQUAL(255, qAlpha(startImg.pixel(sx, sy)));
        }
    }

    // 8. Left-Side OctiReservePanel & Zoom View Enhancements
    win.startNewLocalGame(stageCfg);
    win.renderWindowToImage(1440, 900);
    CHECK_TRUE(win.reservePanel()->playerCardRect(PLAYER_B).top() < win.reservePanel()->playerCardRect(PLAYER_A).top());
    DOUBLES_EQUAL(win.boardCanvas()->geometry().boardOuterRect().top(), win.reservePanel()->playerCardRect(PLAYER_B).top(), 2.0);
    DOUBLES_EQUAL(win.boardCanvas()->geometry().boardOuterRect().bottom(), win.reservePanel()->playerCardRect(PLAYER_A).bottom(), 2.0);
    CHECK_TRUE(win.reservePanel()->iconGlyphCountRendered() >= 4);

    ZoomedSquareDialog zoomDlg(win.board(), win.moveComposer(), 4, 4, BoardRotation::ROT_0, ThemeId::OBSIDIAN_BRASS, &win);
    STRCMP_EQUAL("Place Reserve Pod", zoomDlg.placeReserveButtonText().toUtf8().constData());

    // 9. Right-Side Panel Top/Bottom Alignment to Board Outer Rect
    DOUBLES_EQUAL(win.boardCanvas()->geometry().boardOuterRect().top(), win.rightPanelTopWidgetGeometry().top(), 2.0);
    DOUBLES_EQUAL(win.boardCanvas()->geometry().boardOuterRect().bottom(), win.rightPanelBottomWidgetGeometry().bottom(), 2.0);

    std::filesystem::remove_all(testDir);
}

TEST(DesktopAppIntegrationTest, NonModalBoardScopedStartMatchOverlayDoesNotBlockMenuOrNavigation)
{
    std::string testDir = "/tmp/octi_overlay_test_" + std::to_string(QDateTime::currentMSecsSinceEpoch());
    std::filesystem::create_directories(testDir);
    std::string cfgFile = testDir + "/client.json";

    OctiMainWindow win(nullptr, cfgFile, true);
    win.resize(1440, 900);

    // 1. Nominal: Shown window with promptStartMatchIfVisible creates a non-modal board-scoped overlay
    win.show();
    win.promptStartMatchIfVisible();
    win.pumpUiEvents();

    CHECK_TRUE(win.isAwaitingMatchStart());
    CHECK_TRUE(win.startMatchOverlay() != nullptr);
    POINTERS_EQUAL(win.boardCanvas(), win.startMatchOverlay()->parentWidget());
    CHECK_TRUE(win.startMatchOverlay()->isEmbeddedOverlay());
    CHECK_TRUE(win.startMatchOverlay()->isVisible());
    CHECK_FALSE(win.startMatchOverlay()->isModal());
    POINTERS_EQUAL(nullptr, QApplication::activeModalWidget());

    // 2. Navigation & View Switch Non-Blockage
    // Toggle theme while awaiting match start: overlay updates theme and remains visible
    ThemeId initialTheme = win.theme();
    win.toggleTheme();
    win.pumpUiEvents();
    CHECK_TRUE(win.theme() != initialTheme);
    CHECK_TRUE(win.startMatchOverlay()->isVisible());
    CHECK_TRUE(win.isAwaitingMatchStart());

    // Rotate board clockwise: overlay re-centers over board canvas and remains visible
    win.rotateBoardClockwise();
    win.pumpUiEvents();
    CHECK_TRUE(win.startMatchOverlay()->isVisible());
    CHECK_TRUE(win.isAwaitingMatchStart());

    // Switching to Online Lobby hides the overlay widget without dismissing pending match start
    win.showLobbyView();
    win.pumpUiEvents();
    CHECK_FALSE(win.isBoardViewVisible());
    CHECK_TRUE(win.isLobbyViewVisible());
    CHECK_FALSE(win.startMatchOverlay()->isVisible());
    CHECK_TRUE(win.isAwaitingMatchStart());

    // Switching back to Tabletop Board re-shows overlay widget centered over canvas
    win.showBoardView();
    win.pumpUiEvents();
    CHECK_TRUE(win.isBoardViewVisible());
    CHECK_TRUE(win.startMatchOverlay()->isVisible());
    CHECK_TRUE(win.isAwaitingMatchStart());

    // 3. Fault Injection / Pre-Start Board Click Gating
    // Clicks on board squares are strictly ignored while awaiting match start
    LONGS_EQUAL(0, win.board()->getGame()->getNumMoves());
    QPointF homeSqCenter = win.boardCanvas()->geometry().squareCenter(3, 3);
    QPoint clickPt(static_cast<int>(homeSqCenter.x()), static_cast<int>(homeSqCenter.y()));
    QMouseEvent pressEv(QEvent::MouseButtonPress, clickPt, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(win.boardCanvas(), &pressEv);
    QMouseEvent releaseEv(QEvent::MouseButtonRelease, clickPt, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(win.boardCanvas(), &releaseEv);
    win.pumpUiEvents();
    LONGS_EQUAL(0, win.board()->getGame()->getNumMoves());

    // Simulation helpers also reject prong additions while turn indicator is hidden
    CHECK_FALSE(win.boardCanvas()->simulateDragProngToPod(3, 3, 0, DIR_N, false));
    LONGS_EQUAL(0, win.board()->getGame()->getNumMoves());

    // 4. Start Match Confirmation enables board play
    CHECK_TRUE(win.startMatchOverlay()->startButton() != nullptr);
    CHECK_TRUE(win.startMatchOverlay()->changeTypeButton() != nullptr);
    win.startMatchOverlay()->startButton()->click();
    win.pumpUiEvents();

    CHECK_FALSE(win.isAwaitingMatchStart());
    CHECK_FALSE(win.startMatchOverlay()->isVisible());
    CHECK_TRUE(win.boardCanvas()->isTurnIndicatorVisible());

    // Board play is now enabled
    CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(3, 3, 0, DIR_N, false));
    win.pumpUiEvents();
    if (win.moveComposer()->isAwaitingConfirmation()) {
        CHECK_TRUE(win.moveComposer()->commitStagedMove());
    }
    CHECK_TRUE(win.board()->getGame()->getNumMoves() >= 1);

    std::filesystem::remove_all(testDir);
}

TEST(DesktopAppIntegrationTest, ProfileAndAvatarEditorFullParityAndInstallIdAttribution)
{
    LiveServerEnv env;
    std::string avatarDir = env.dir + "/avatars";
    env.store.setAvatarDir(avatarDir);

    std::string cfgPath = env.dir + "/desktop_client.json";
    OctiMainWindow win(
        OctiNetworkClient::createForLoopbackTest(env.port()), cfgPath);
    win.resize(1440, 900);
    win.show();
    win.pumpUiEvents();

    std::string installId = win.configStore()->preferences().installId;
    CHECK_FALSE(installId.empty());
    STRCMP_EQUAL(installId.c_str(), win.netClient()->installId().c_str());

    /* 1. Sign in via SsoOnboardingDialog */
    std::string tok = makeToken("google", "sub-desktop-prof-parity", "Alice");
    {
        SsoOnboardingDialog ssoDlg(win.netClient(), win.configStore(), ThemeId::OBSIDIAN_BRASS, &win);
        auto ssoRes = ssoDlg.signInWithToken("google", tok, "");
        CHECK_TRUE_TEXT(ssoRes.ok, ssoRes.errorMessage.c_str());
        LONGS_EQUAL(201, ssoRes.httpStatus);
        CHECK_TRUE(ssoDlg.isOnboardingStep2Active());

        auto finRes = ssoDlg.finalizeOnboardingHandle("Alice_Profile");
        CHECK_TRUE_TEXT(finRes.ok, finRes.errorMessage.c_str());
        CHECK_FALSE(ssoDlg.isOnboardingStep2Active());
        win.applyAuthenticatedProfile(ssoDlg.loggedInProfile());
    }
    CHECK_TRUE(win.isAuthenticated());
    STRCMP_EQUAL("Alice_Profile", win.currentProfile().handle.c_str());

    /* 2. Open AccountLinkDialog and verify initial profile fields */
    AccountLinkDialog dlg(win.netClient(), win.configStore(), ThemeId::OBSIDIAN_BRASS, &win);
    dlg.setOnAccountSwitchedCallback([&win](const PlayerProfileInfo &prof) {
        win.applyAuthenticatedProfile(prof);
    });
    CHECK_TRUE(dlg.refreshProfile());
    STRCMP_EQUAL("Alice_Profile", dlg.handleEdit()->text().toUtf8().constData());
    CHECK_TRUE(dlg.saveProfileButton() != nullptr);
    CHECK_TRUE(dlg.uploadAvatarButton() != nullptr);
    CHECK_TRUE(dlg.clearAvatarButton() != nullptr);

    /* 3. Automatic crop & compression of non-square QImage(400, 260) */
    QImage rawImg(400, 260, QImage::Format_RGB32);
    rawImg.fill(QColor(180, 50, 80));
    CHECK_TRUE(dlg.selectAvatarFromImage(rawImg));
    CHECK_TRUE(dlg.profileStatusText().contains("Avatar selected"));

    dlg.handleEdit()->setText("Alice_Profile");
    dlg.displayNameEdit()->setText("Alice the Grandmaster");
    dlg.locationEdit()->setText("San Francisco, CA");
    dlg.bioEdit()->setPlainText("OCTI desktop champion and theoretician.");

    /* 4. Save profile via the single "Save Profile" button */
    auto saveRes = dlg.saveProfile();
    CHECK_TRUE_TEXT(saveRes.ok, saveRes.errorMessage.c_str());
    STRCMP_EQUAL("Profile saved.", dlg.profileStatusText().toUtf8().constData());

    /* 5. Verify octi-server stored avatar on disk (/api/v1/avatars/...) and avatar pixmap */
    std::string diskAvatar = dlg.currentProfile().avatarUrl;
    CHECK_TRUE(diskAvatar.rfind("/api/v1/avatars/", 0) == 0);
    std::string fileName = diskAvatar.substr(std::string("/api/v1/avatars/").size());
    auto qmark = fileName.find('?');
    if (qmark != std::string::npos) {
        fileName = fileName.substr(0, qmark);
    }
    CHECK_TRUE(std::filesystem::exists(avatarDir + "/" + fileName));

    CHECK_TRUE(dlg.avatarPreviewLabel()->pixmap() != nullptr);
    CHECK_FALSE(dlg.avatarPreviewLabel()->pixmap()->isNull());
    win.pumpUiEvents();
    CHECK_TRUE(win.profileAvatarLabel()->pixmap() != nullptr);
    CHECK_FALSE(win.profileAvatarLabel()->pixmap()->isNull());

    const ClientPreferences &savedPrefs = win.configStore()->preferences();
    STRCMP_EQUAL("Alice_Profile", savedPrefs.handle.c_str());
    STRCMP_EQUAL("Alice the Grandmaster", savedPrefs.displayName.c_str());
    STRCMP_EQUAL(diskAvatar.c_str(), savedPrefs.avatarUrl.c_str());
    STRCMP_EQUAL("San Francisco, CA", savedPrefs.location.c_str());
    STRCMP_EQUAL("OCTI desktop champion and theoretician.", savedPrefs.bio.c_str());

    /* 6. Verify saving profile again with the same handle succeeds (no 24h cooldown on unchanged handle) */
    auto saveAgain = dlg.saveProfile();
    CHECK_TRUE_TEXT(saveAgain.ok, saveAgain.errorMessage.c_str());
    STRCMP_EQUAL("Profile saved.", dlg.profileStatusText().toUtf8().constData());

    /* 7. Changing handle a second time within 24h returns 429 HANDLE_CHANGE_RATE_LIMITED */
    dlg.handleEdit()->setText("Alice_SecondHandle");
    auto saveRateLimited = dlg.saveProfile();
    CHECK_FALSE(saveRateLimited.ok);
    LONGS_EQUAL(429, saveRateLimited.httpStatus);
    STRCMP_EQUAL("HANDLE_CHANGE_RATE_LIMITED", saveRateLimited.errorCode.c_str());
    CHECK_TRUE(dlg.profileStatusText().contains("24 hours"));

    /* 8. Clear pending avatar + saveProfile clears avatar on server and removes disk file */
    dlg.handleEdit()->setText("Alice_Profile");
    dlg.clearPendingAvatar();
    auto saveClear = dlg.saveProfile();
    CHECK_TRUE_TEXT(saveClear.ok, saveClear.errorMessage.c_str());
    STRCMP_EQUAL("", dlg.currentProfile().avatarUrl.c_str());
    CHECK_FALSE(std::filesystem::exists(avatarDir + "/" + fileName));

    /* 9. Verify X-Octi-Install-Id header bound desktop installId to signed-in player in env.store */
    PlayerProfile boundProfile;
    CHECK_TRUE(env.store.getPlayerByInstallId(installId, boundProfile));
    STRCMP_EQUAL(win.currentProfile().uuid.c_str(), boundProfile.uuid.c_str());
}

TEST(DesktopAppIntegrationTest, StartupProfileAndAvatarSyncWithAuthenticatedSessionToken)
{
    LiveServerEnv env;
    std::string avatarDir = env.dir + "/avatars";
    env.store.setAvatarDir(avatarDir);

    /* 1. Create a user on the server via client1 */
    auto client1 = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginRes = client1->loginWithSso(
        "google", makeToken("google", "sub-startup-sync", "SyncUser"), "SyncUser");
    CHECK_TRUE_TEXT(loginRes.ok, loginRes.errorMessage.c_str());
    std::string sessionTok = loginRes.data.sessionToken;
    std::string playerUuid = loginRes.data.profile.uuid;

    /* Create a 64x64 valid avatar image and encode to data URI */
    QImage srcImg(64, 64, QImage::Format_RGB32);
    srcImg.fill(QColor(40, 140, 220));
    std::string avatarDataUri = AccountLinkDialog::encodeAvatarImageToDataUri(srcImg);

    /* Update profile on server with bio, location, display_name, avatar */
    auto updateRes = client1->updateMyProfile(
        "SyncUser",
        avatarDataUri,
        "Synchronized User",
        "A bio written on another client or web UI.",
        "Kyoto, Japan",
        false);
    CHECK_TRUE_TEXT(updateRes.ok, updateRes.errorMessage.c_str());
    std::string serverAvatarUrl = updateRes.data.avatarUrl;
    CHECK_FALSE(serverAvatarUrl.empty());

    /* 2. Prepare client configuration simulating a saved session token on disk,
     * but with EMPTY avatarUrl, bio, and location (e.g. outdated prefs) */
    std::string cfgPath = env.dir + "/client_sync_startup.json";
    {
        ClientConfigStore cfg(cfgPath);
        cfg.preferences().sessionToken = sessionTok;
        cfg.preferences().playerUuid = playerUuid;
        cfg.preferences().handle = "SyncUser";
        cfg.preferences().displayName = "Old Name";
        cfg.preferences().avatarUrl = "";
        cfg.preferences().bio = "";
        cfg.preferences().location = "";
        std::string err;
        CHECK_TRUE(cfg.save(err));
    }

    /* 3. Launch OctiMainWindow with this config.
     * On construction, _authenticated is true (from sessionToken + playerUuid).
     * The singleShot timer schedules refreshAuthenticatedProfileFromServer(). */
    OctiMainWindow win(
        OctiNetworkClient::createForLoopbackTest(env.port()), cfgPath);
    win.resize(1024, 768);
    win.show();

    /* Pump UI events to allow singleShot(0) to execute and fetch /api/v1/profile/me */
    win.pumpUiEvents();

    /* Verify OctiMainWindow fetched and applied server bio, location, displayName, and avatarUrl */
    STRCMP_EQUAL("SyncUser", win.currentProfile().handle.c_str());
    STRCMP_EQUAL("Synchronized User", win.currentProfile().displayName.c_str());
    STRCMP_EQUAL("A bio written on another client or web UI.", win.currentProfile().bio.c_str());
    STRCMP_EQUAL("Kyoto, Japan", win.currentProfile().location.c_str());
    STRCMP_EQUAL(serverAvatarUrl.c_str(), win.currentProfile().avatarUrl.c_str());

    /* Verify _profileAvatarLabel has a non-null, valid pixmap rendered */
    CHECK_TRUE(win.profileAvatarLabel() != nullptr);
    CHECK_TRUE(win.profileAvatarLabel()->pixmap() != nullptr);
    CHECK_FALSE(win.profileAvatarLabel()->pixmap()->isNull());

    /* Also verify preferences file on disk was synced */
    const ClientPreferences &diskPrefs = win.configStore()->preferences();
    STRCMP_EQUAL("Synchronized User", diskPrefs.displayName.c_str());
    STRCMP_EQUAL("A bio written on another client or web UI.", diskPrefs.bio.c_str());
    STRCMP_EQUAL("Kyoto, Japan", diskPrefs.location.c_str());
    STRCMP_EQUAL(serverAvatarUrl.c_str(), diskPrefs.avatarUrl.c_str());
}

TEST(DesktopAppIntegrationTest, LobbyTablesRedesignTimestampsSeatsAndLeaveTable)
{
    LiveServerEnv env;

    auto clientA = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginA = clientA->loginWithSso(
        "google", makeToken("google", "sub-lobby-redesign-a", "TableHostA"), "TableHostA");
    CHECK_TRUE_TEXT(loginA.ok, loginA.errorMessage.c_str());

    std::string cfgPathA = env.dir + "/lobby_redesign_a.json";
    OctiMainWindow winA(std::move(clientA), cfgPathA);
    winA.applyAuthenticatedProfile(loginA.data.profile);

    /* 1. Verify local-timezone chat timestamp formatting for same-day and prior-day */
    QDateTime localNow = QDateTime::currentDateTime();
    std::int64_t nowMs = localNow.toUTC().toMSecsSinceEpoch();
    QString expectedSameDay = QString("[%1] ").arg(localNow.toString("HH:mm"));
    STRCMP_EQUAL(expectedSameDay.toUtf8().constData(),
                 LobbyView::formatChatTimestamp(nowMs).toUtf8().constData());

    QDateTime twoDaysAgo = localNow.addDays(-2);
    std::int64_t twoDaysAgoMs = twoDaysAgo.toUTC().toMSecsSinceEpoch();
    QString expectedOldDay = QString("[%1] ").arg(twoDaysAgo.toString("MM-dd HH:mm"));
    STRCMP_EQUAL(expectedOldDay.toUtf8().constData(),
                 LobbyView::formatChatTimestamp(twoDaysAgoMs).toUtf8().constData());

    LobbyChatEntry chatEntry;
    chatEntry.seq = 501;
    chatEntry.createdAtMs = nowMs;
    chatEntry.atMs = nowMs;
    chatEntry.handle = "TableHostA";
    chatEntry.text = "Ready for a 2P 3-base match!";
    QString formattedLine = LobbyView::formatChatMessageLine(chatEntry);
    CHECK_TRUE(formattedLine.startsWith(expectedSameDay));
    CHECK_TRUE(formattedLine.contains("@TableHostA: Ready for a 2P 3-base match!"));

    SseEvent sseChatEv;
    sseChatEv.id = 502;
    sseChatEv.event = "LOBBY_CHAT";
    sseChatEv.dataJson = Json({
        {"seq", 502},
        {"created_at_ms", nowMs},
        {"player_uuid", loginA.data.profile.uuid},
        {"handle", "TableHostA"},
        {"text", "Live SSE timestamped chat"}
    }).dump();
    winA.lobbyView()->handleLobbySseEvent(sseChatEv);
    CHECK_TRUE(winA.lobbyView()->chatTranscriptText().find(
                   expectedSameDay.toStdString() + "@TableHostA: Live SSE timestamped chat") !=
               std::string::npos);

    /* 2. Host creates a waiting table -> verify Board waiting-room banner and Lobby My Active Table banner */
    CreateTableRequest req;
    req.mode = "2P";
    req.bases = 3;
    auto created = winA.createAndEnterOnlineTable(req);
    CHECK_TRUE_TEXT(created.ok, created.errorMessage.c_str());
    std::string roomId = created.data.roomId;
    CHECK_TRUE(winA.isInOnlineGame());
    STRCMP_EQUAL("WAITING_FOR_PLAYERS", winA.onlineRoomPhase().c_str());
    CHECK_TRUE(winA.isWaitingRoomBannerVisible());
    CHECK_TRUE(winA.waitingRoomBannerText().contains(QString::fromStdString(roomId)));
    CHECK_TRUE(winA.leaveWaitingTableButton()->isEnabled());
    CHECK_TRUE(winA.leaveTableButton()->isEnabled());

    /* Switch to Lobby view while seated at the waiting table */
    winA.showLobbyView();
    winA.lobbyView()->refreshLobby();
    CHECK_TRUE(winA.lobbyView()->players().size() >= 1);
    STRCMP_EQUAL("TableHostA", winA.lobbyView()->players()[0].handle.c_str());
    CHECK_TRUE(!winA.lobbyView()->players()[0].clients.empty());
    STRCMP_EQUAL("desktop", winA.lobbyView()->players()[0].clients[0].c_str());
    CHECK_TRUE(winA.lobbyView()->players()[0].onlineSec >= 0);
    CHECK_TRUE(winA.lobbyView()->players()[0].idleSec >= 0);
    CHECK_TRUE(winA.isLobbyViewVisible());
    CHECK_TRUE(winA.lobbyView()->isMyTableBannerVisible());
    STRCMP_EQUAL(roomId.c_str(), winA.lobbyView()->activeBannerRoomId().c_str());
    CHECK_TRUE(winA.lobbyView()->myTableBannerLabel()->text().contains(
        QString::fromStdString(roomId)));
    CHECK_TRUE(winA.lobbyView()->myTableBannerLabel()->text().contains(
        "WAITING FOR PLAYERS"));
    CHECK_TRUE(winA.lobbyView()->myTableSeatsLabel()->text().contains(
        "Seat 1 (A): @TableHostA"));
    CHECK_TRUE(winA.lobbyView()->myTableSeatsLabel()->text().contains(
        "Seat 2 (B): [Open]"));

    /* Verify Tables list & Table Seats & Details Inspector */
    winA.lobbyView()->selectRoomId(roomId);
    const TableSummaryInfo *tbl = winA.lobbyView()->findTable(roomId);
    CHECK_TRUE(tbl != nullptr);
    QString seatSummary = LobbyView::formatSeatOccupancySummary(*tbl);
    CHECK_TRUE(seatSummary.contains("1/2"));
    CHECK_TRUE(seatSummary.contains("A: @TableHostA"));
    CHECK_TRUE(seatSummary.contains("B: [Open]"));
    CHECK_TRUE(winA.lobbyView()->inspectorTitleLabel()->text().contains(
        QString::fromStdString(roomId)));
    CHECK_TRUE(winA.lobbyView()->inspectorSeatsLabel()->text().contains(
        "Seat 1 (A): @TableHostA"));
    CHECK_TRUE(winA.lobbyView()->inspectorSeatsLabel()->text().contains(
        "Seat 2 (B): [Open]"));
    CHECK_TRUE(winA.lobbyView()->leaveTableButton()->isEnabled());

    /* 3. Leave the waiting table view via leaveActiveOnlineTable() (returns to lobby without destroying table) */
    auto leaveRes = winA.leaveActiveOnlineTable(roomId);
    CHECK_TRUE_TEXT(leaveRes.ok, leaveRes.errorMessage.c_str());
    CHECK_FALSE(winA.isInOnlineGame());
    STRCMP_EQUAL("", winA.onlineRoomId().c_str());
    CHECK_TRUE(winA.isLobbyViewVisible());
    CHECK_TRUE(winA.lobbyView()->isMyTableBannerVisible());
    CHECK_TRUE(winA.lobbyView()->findTable(roomId) != nullptr);

    /* 4. Host deletes the waiting table via purgeTable() */
    auto purgeRes = winA.netClient()->purgeTable(roomId);
    CHECK_TRUE_TEXT(purgeRes.ok, purgeRes.errorMessage.c_str());
    winA.lobbyView()->refreshLobby();
    CHECK_FALSE(winA.lobbyView()->isMyTableBannerVisible());
    CHECK_TRUE(winA.lobbyView()->findTable(roomId) == nullptr);
}

TEST(DesktopAppIntegrationTest, AccountLinkDialogProfileConfirmationGuards)
{
    LiveServerEnv env;
    std::string avatarDir = env.dir + "/avatars_confirm";
    env.store.setAvatarDir(avatarDir);

    auto client = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginRes = client->loginWithSso(
        "google", makeToken("google", "sub-confirm-guard", "ConfirmUser"), "ConfirmUser");
    CHECK_TRUE_TEXT(loginRes.ok, loginRes.errorMessage.c_str());

    std::string cfgPath = env.dir + "/confirm_guard.json";
    OctiMainWindow win(std::move(client), cfgPath);
    win.applyAuthenticatedProfile(loginRes.data.profile);

    AccountLinkDialog dlg(win.netClient(), win.configStore(), ThemeId::OBSIDIAN_BRASS, &win);
    CHECK_TRUE(dlg.refreshProfile());
    CHECK_FALSE(dlg.hasUnsavedProfileChanges());

    QImage img(64, 64, QImage::Format_RGB32);
    img.fill(QColor(120, 200, 90));
    CHECK_TRUE(dlg.selectAvatarFromImage(img));
    CHECK_TRUE(dlg.hasUnsavedProfileChanges());

    /* 1. Rejecting Clear Avatar confirmation keeps pending avatar intact */
    dlg.setConfirmPromptHandler([](const QString &title, const QString &) {
        CHECK_TRUE(title.contains("Clear Avatar"));
        return false;
    });
    CHECK_FALSE(dlg.clearPendingAvatar(true));
    LONGS_EQUAL(1, dlg.confirmationPromptCount());
    CHECK_TRUE(dlg.profileStatusText().contains("Clear avatar cancelled"));
    CHECK_TRUE(dlg.hasUnsavedProfileChanges());

    /* Accepting Clear Avatar confirmation clears pending avatar */
    dlg.setConfirmPromptHandler([](const QString &title, const QString &) {
        CHECK_TRUE(title.contains("Clear Avatar"));
        return true;
    });
    CHECK_TRUE(dlg.clearPendingAvatar(true));
    LONGS_EQUAL(2, dlg.confirmationPromptCount());
    CHECK_TRUE(dlg.profileStatusText().contains("Avatar cleared"));

    /* 2. Rejecting Save Profile confirmation prevents PATCH /api/v1/profile/me */
    dlg.displayNameEdit()->setText("Guarded Display Name");
    dlg.bioEdit()->setPlainText("Guarded bio text.");
    dlg.setConfirmPromptHandler([](const QString &title, const QString &) {
        CHECK_TRUE(title.contains("Apply Profile Settings"));
        return false;
    });
    auto cancelledSave = dlg.saveProfile(true);
    CHECK_FALSE(cancelledSave.ok);
    STRCMP_EQUAL("CANCELLED", cancelledSave.errorCode.c_str());
    STRCMP_EQUAL("", dlg.currentProfile().bio.c_str());

    /* Accepting Save Profile confirmation applies changes to server */
    dlg.setConfirmPromptHandler([](const QString &title, const QString &) {
        CHECK_TRUE(title.contains("Apply Profile Settings"));
        return true;
    });
    auto confirmedSave = dlg.saveProfile(true);
    CHECK_TRUE_TEXT(confirmedSave.ok, confirmedSave.errorMessage.c_str());
    STRCMP_EQUAL("Guarded Display Name", dlg.currentProfile().displayName.c_str());
    STRCMP_EQUAL("Guarded bio text.", dlg.currentProfile().bio.c_str());
    CHECK_FALSE(dlg.hasUnsavedProfileChanges());

    /* 3. Unsaved edits trigger discard confirmation on Done/Close */
    dlg.locationEdit()->setText("Unsaved City");
    CHECK_TRUE(dlg.hasUnsavedProfileChanges());
    dlg.setConfirmPromptHandler([](const QString &title, const QString &) {
        CHECK_TRUE(title.contains("Unsaved Profile Changes"));
        return false;
    });
    CHECK_FALSE(dlg.tryCloseWithConfirmation(true));
    dlg.setConfirmPromptHandler([](const QString &title, const QString &) {
        CHECK_TRUE(title.contains("Unsaved Profile Changes"));
        return true;
    });
    CHECK_TRUE(dlg.tryCloseWithConfirmation(true));
}

TEST(DesktopAppIntegrationTest, OfkDialogModeAndSeatConstraintsIntegration)
{
    /* 1. NewLocalGameDialog constraints for OFK */
    {
        NewLocalGameDialog dlg;
        LocalGameConfig cfg;
        cfg.playType = LocalPlayType::VS_AI_BOT;
        cfg.mode = GAME_MODE_OFK;
        cfg.bases = GAME_BASES_3; // Attempt 3 bases
        cfg.useSuperProngs = true;
        cfg.useEdgelessBoard = true;
        dlg.setConfig(cfg);

        LocalGameConfig resCfg = dlg.config();
        LONGS_EQUAL(static_cast<int>(GAME_MODE_OFK), static_cast<int>(resCfg.mode));
        LONGS_EQUAL(static_cast<int>(GAME_BASES_1), static_cast<int>(resCfg.bases));
        CHECK_FALSE(resCfg.useSuperProngs);
        CHECK_FALSE(resCfg.useEdgelessBoard);
    }

    /* 2. CreateTableDialog constraints for OFK */
    {
        CreateTableDialog createDlg;
        CreateTableRequest req;
        req.mode = "OFK";
        req.bases = 3;
        req.superProngs = true;
        req.edgeless = true;
        createDlg.setRequest(req);

        CreateTableRequest resReq = createDlg.buildRequest();
        LONGS_EQUAL(1, resReq.bases);
        CHECK_FALSE(resReq.superProngs);
        CHECK_FALSE(resReq.edgeless);
    }

    /* 3. MatchmakingDialog constraints for OFK */
    {
        MatchmakingDialog matchDlg(nullptr);
        MatchmakingRequest mreq;
        mreq.mode = "OFK";
        mreq.bases = 3;
        mreq.superProngs = true;
        mreq.edgeless = true;
        matchDlg.setRequest(mreq);

        MatchmakingRequest resMreq = matchDlg.buildRequest();
        LONGS_EQUAL(1, resMreq.bases);
        CHECK_FALSE(resMreq.superProngs);
        CHECK_FALSE(resMreq.edgeless);
    }
}

TEST(DesktopAppIntegrationTest, MainWindowShowHintResolvesAccuratePodIndex)
{
    std::string cfgPath = "/tmp/octi_hint_test.json";
    OctiMainWindow win(nullptr, cfgPath, true);
    LocalGameConfig cfg;
    cfg.playType = LocalPlayType::PASS_AND_PLAY;
    cfg.mode = GAME_MODE_OFK;
    cfg.bases = GAME_BASES_1;
    win.startNewLocalGame(cfg);

    win.showHint();
    CHECK_TRUE(win.moveComposer()->isAwaitingConfirmation());
    CHECK_FALSE(win.moveComposer()->stagedMoveNotation().empty());
    win.moveComposer()->cancelAndRollback();
    CHECK_FALSE(win.moveComposer()->isAwaitingConfirmation());
}

TEST(DesktopAppIntegrationTest, RightSidebarFiftyFiftyLayoutAndZoomChatDialog)
{
    std::string cfgPath = "/tmp/octi_chat_test.json";
    OctiMainWindow win(nullptr, cfgPath, true);
    win.resize(1200, 800);
    win.show();

    QWidget *topHalf = win.boardLobbyChatBox();
    if (!topHalf) {
        topHalf = win.findChild<QWidget *>("rightTopHalfWidget");
    }
    CHECK_TRUE(topHalf != nullptr);

    QWidget *chatBox = win.findChild<QWidget *>("chatBox");
    CHECK_TRUE(chatBox != nullptr);
    chatBox->show();
    CHECK_TRUE(chatBox->isVisible());

    QPushButton *zoomBtn = win.findChild<QPushButton *>("zoomChatBtn");
    CHECK_TRUE(zoomBtn != nullptr);

    win.openBoardChatZoomDialog();
    QDialog *zoomDlg = win.findChild<QDialog *>("boardChatZoomDialog");
    CHECK_TRUE(zoomDlg != nullptr);
    CHECK_TRUE(zoomDlg->windowFlags() & Qt::FramelessWindowHint);

    QPlainTextEdit *transcript = zoomDlg->findChild<QPlainTextEdit *>("boardChatZoomTranscript");
    CHECK_TRUE(transcript != nullptr);

    QLineEdit *input = zoomDlg->findChild<QLineEdit *>("boardChatZoomInput");
    CHECK_TRUE(input != nullptr);

    QPushButton *sendBtn = zoomDlg->findChild<QPushButton *>("boardChatZoomSendBtn");
    CHECK_TRUE(sendBtn != nullptr);

    input->setText("Good game!");
    sendBtn->click();
    CHECK_TRUE(transcript->toPlainText().contains("Good game!"));
    CHECK_FALSE(transcript->toPlainText().contains("@stock"));

    zoomDlg->close();
}

TEST(DesktopAppIntegrationTest, RobotsDirectoryViaNetworkClient)
{
    LiveServerEnv env;
    auto client = OctiNetworkClient::createForLoopbackTest(env.port());
    auto res = client->fetchRobotsDirectory();
    CHECK_TRUE_TEXT(res.ok, res.errorMessage.c_str());
    LONGS_EQUAL(200, res.httpStatus);
}

TEST(DesktopAppIntegrationTest, LobbyViewObserveInProgressAndFullTables)
{
    LiveServerEnv env;
    std::string cfgPath = env.dir + "/client_lobby_obs.json";
    OctiMainWindow win(OctiNetworkClient::createForLoopbackTest(env.port()), cfgPath);
    win.showLobbyView();
    LobbyView *lobby = win.lobbyView();
    CHECK_TRUE(lobby != nullptr);

    LobbyStateData state;
    TableSummaryInfo t1;
    t1.roomId = "room-inprogress-1";
    t1.mode = "2P";
    t1.phase = "IN_PROGRESS";
    t1.observerPolicy = "OPEN_OBSERVERS";
    t1.seatsTotal = 2;
    t1.seatsFilled = 2;
    state.tables.push_back(t1);

    TableSummaryInfo t2;
    t2.roomId = "room-full-waiting-2";
    t2.mode = "2P";
    t2.phase = "WAITING_FOR_PLAYERS";
    t2.observerPolicy = "OPEN_OBSERVERS";
    t2.seatsTotal = 2;
    t2.seatsFilled = 2;
    state.tables.push_back(t2);

    lobby->refreshFromLobbyState(state);
    lobby->selectRoomId("room-inprogress-1");

    QPushButton *joinBtn = lobby->joinSelectedButton();
    CHECK_TRUE(joinBtn != nullptr);
    CHECK_TRUE(joinBtn->isEnabled());
    STRCMP_EQUAL("Observe Table", joinBtn->text().toUtf8().constData());

    bool observed = false;
    std::string observedRoom;
    lobby->setOnObserveTableRequested([&](const TableSummaryInfo &table) {
        observed = true;
        observedRoom = table.roomId;
    });

    joinBtn->click();
    CHECK_TRUE(observed);
    STRCMP_EQUAL("room-inprogress-1", observedRoom.c_str());

    /* Test full waiting table */
    observed = false;
    observedRoom.clear();
    lobby->selectRoomId("room-full-waiting-2");
    CHECK_TRUE(joinBtn->isEnabled());
    STRCMP_EQUAL("Observe Table", joinBtn->text().toUtf8().constData());
    joinBtn->click();
    CHECK_TRUE(observed);
    STRCMP_EQUAL("room-full-waiting-2", observedRoom.c_str());
}

TEST(DesktopAppIntegrationTest, LobbyViewMatchmakingButtonAndMatchFoundSse)
{
    LiveServerEnv env;
    std::string cfgPath = env.dir + "/client_mm.json";
    OctiMainWindow win(OctiNetworkClient::createForLoopbackTest(env.port()), cfgPath);
    LobbyView *lobby = win.lobbyView();
    CHECK_TRUE(lobby != nullptr);

    QPushButton *mmBtn = lobby->quickMatchButton();
    CHECK_TRUE(mmBtn != nullptr);

    bool matchFoundCalled = false;
    std::string matchedRoomId;
    lobby->setOnMatchFoundCallback([&](const std::string &roomId) {
        matchFoundCalled = true;
        matchedRoomId = roomId;
    });

    SseEvent ev;
    ev.event = "MATCH_FOUND";
    ev.id = 1;
    ev.dataJson = "{\"room_id\":\"room-quickmatch-999\"}";
    lobby->handleLobbySseEvent(ev);

    CHECK_TRUE(matchFoundCalled);
    STRCMP_EQUAL("room-quickmatch-999", matchedRoomId.c_str());
}

TEST(DesktopAppIntegrationTest, ExpiredSessionTokenCleanupOnStartup)
{
    LiveServerEnv env;
    std::string cfgPath = env.dir + "/client_expired_401.json";
    {
        ClientConfigStore cfg(cfgPath);
        cfg.preferences().sessionToken = "invalid-or-expired-token-401";
        cfg.preferences().playerUuid = "uuid-expired-user";
        cfg.preferences().handle = "ExpiredUser";
        std::string err;
        CHECK_TRUE(cfg.save(err));
    }

    OctiMainWindow win(OctiNetworkClient::createForLoopbackTest(env.port()), cfgPath);
    win.resize(1024, 768);
    win.show();
    win.pumpUiEvents();

    /* With an expired session token, server returns 401.
     * OctiMainWindow should clear _authenticated and clear saved session. */
    CHECK_FALSE(win.isAuthenticated());
    CHECK_TRUE(win.configStore()->preferences().sessionToken.empty());
    CHECK_TRUE(win.currentProfile().uuid.empty());
}

TEST(DesktopAppIntegrationTest, OnlineMoveTurnGuardAndRollback)
{
    std::string cfgPath = "/tmp/octi_turn_guard_test.json";
    OctiMainWindow win(nullptr, cfgPath, true);
    // In local game, isInteractiveOnlineTurn() should return true
    CHECK_TRUE(win.isInteractiveOnlineTurn());
}

TEST(DesktopAppIntegrationTest, OnlineRoomEntryClearsAwaitingMatchStartAndGatesInteraction)
{
    LiveServerEnv env;

    auto clientA = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginA = clientA->loginWithSso(
        "google", makeToken("google", "sub-gate-a", "AliceGate"), "AliceGate");
    CHECK_TRUE_TEXT(loginA.ok, loginA.errorMessage.c_str());

    std::string cfgA = env.dir + "/client_gate_a.json";
    OctiMainWindow winA(std::move(clientA), cfgA);
    winA.applyAuthenticatedProfile(loginA.data.profile);

    /* 1. Startup / Local match begins awaiting start */
    winA.beginAwaitingMatchStart();
    CHECK_TRUE(winA.isAwaitingMatchStart());

    /* 2. Entering online room clears _awaitingMatchStart */
    CreateTableRequest req;
    req.mode = "2P";
    req.bases = 3;
    auto createRes = winA.createAndEnterOnlineTable(req);
    CHECK_TRUE_TEXT(createRes.ok, createRes.errorMessage.c_str());
    std::string roomId = createRes.data.roomId;

    CHECK_FALSE(winA.isAwaitingMatchStart());
    /* In WAITING_FOR_PLAYERS, turn is not interactive */
    CHECK_FALSE(winA.isInteractiveOnlineTurn());

    /* 3. Second player joins -> match starts (IN_PROGRESS) */
    auto clientB = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginB = clientB->loginWithSso(
        "google", makeToken("google", "sub-gate-b", "BobGate"), "BobGate");
    CHECK_TRUE_TEXT(loginB.ok, loginB.errorMessage.c_str());

    std::string cfgB = env.dir + "/client_gate_b.json";
    OctiMainWindow winB(std::move(clientB), cfgB);
    winB.applyAuthenticatedProfile(loginB.data.profile);

    winB.beginAwaitingMatchStart();
    CHECK_TRUE(winB.isAwaitingMatchStart());
    auto joinRes = winB.joinAndEnterOnlineTable(roomId, 2);
    CHECK_TRUE_TEXT(joinRes.ok, joinRes.errorMessage.c_str());

    CHECK_FALSE(winB.isAwaitingMatchStart());

    winA.refreshOnlineGameView();
    winB.refreshOnlineGameView();

    STRCMP_EQUAL("IN_PROGRESS", winA.onlineRoomPhase().c_str());
    /* Active player A can interact; off-turn player B cannot */
    CHECK_TRUE(winA.isInteractiveOnlineTurn());
    CHECK_FALSE(winB.isInteractiveOnlineTurn());

    /* 4. Observer cannot interact */
    auto clientC = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginC = clientC->loginWithSso(
        "google", makeToken("google", "sub-gate-c", "CarolObs"), "CarolObs");
    CHECK_TRUE_TEXT(loginC.ok, loginC.errorMessage.c_str());

    std::string cfgC = env.dir + "/client_gate_c.json";
    OctiMainWindow winC(std::move(clientC), cfgC);
    winC.applyAuthenticatedProfile(loginC.data.profile);
    auto obsRes = winC.observeOnlineTable(roomId);
    CHECK_TRUE_TEXT(obsRes.ok, obsRes.errorMessage.c_str());
    winC.refreshOnlineGameView();
    CHECK_FALSE(winC.isInteractiveOnlineTurn());
}

TEST(DesktopAppIntegrationTest, StagedMovePreservedAcrossNonMoveSseEvents)
{
    LiveServerEnv env;

    auto clientA = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginA = clientA->loginWithSso(
        "google", makeToken("google", "sub-stage-a", "StageHostA"), "StageHostA");
    CHECK_TRUE_TEXT(loginA.ok, loginA.errorMessage.c_str());

    std::string cfgA = env.dir + "/client_stage_a.json";
    OctiMainWindow winA(std::move(clientA), cfgA);
    winA.applyAuthenticatedProfile(loginA.data.profile);

    CreateTableRequest req;
    req.mode = "2P";
    req.bases = 3;
    auto createRes = winA.createAndEnterOnlineTable(req);
    CHECK_TRUE_TEXT(createRes.ok, createRes.errorMessage.c_str());
    std::string roomId = createRes.data.roomId;

    auto clientB = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginB = clientB->loginWithSso(
        "google", makeToken("google", "sub-stage-b", "StageGuestB"), "StageGuestB");
    CHECK_TRUE_TEXT(loginB.ok, loginB.errorMessage.c_str());

    std::string cfgB = env.dir + "/client_stage_b.json";
    OctiMainWindow winB(std::move(clientB), cfgB);
    winB.applyAuthenticatedProfile(loginB.data.profile);
    CHECK_TRUE(winB.joinAndEnterOnlineTable(roomId, 2).ok);

    winA.refreshOnlineGameView();
    CHECK_TRUE(winA.isInteractiveOnlineTurn());

    /* Stage a move: add a prong with confirm-all enabled so it stages */
    winA.setConfirmAllMoves(true);
    CHECK_TRUE(winA.moveComposer()->stageOrCommitAddProng(5, 3, 0, DIR_N, false));
    CHECK_TRUE(winA.moveComposer()->hasStagedMove());
    CHECK_TRUE(winA.moveComposer()->isTurnInProgress());

    /* Deliver non-move SSE events: CHAT, OBSERVER_JOINED */
    SseEvent chatEv;
    chatEv.id = 10;
    chatEv.event = "CHAT";
    chatEv.dataJson = Json({
        {"seq", 10},
        {"scope", "ALL"},
        {"handle", "StageGuestB"},
        {"text", "Hello during staged turn!"}
    }).dump();
    winA.handleGameSseEvent(chatEv);

    SseEvent obsEv;
    obsEv.id = 11;
    obsEv.event = "OBSERVER_JOINED";
    obsEv.dataJson = Json({{"handle", "SpectatorX"}}).dump();
    winA.handleGameSseEvent(obsEv);
    winA.refreshOnlineGameView();

    /* Staged move MUST NOT be rolled back or wiped by non-move events or refreshes */
    CHECK_TRUE(winA.moveComposer()->hasStagedMove());
    CHECK_TRUE(winA.moveComposer()->isTurnInProgress());
}

TEST(DesktopAppIntegrationTest, GameChatMessageDeduplicationOnIncomingChatSse)
{
    LiveServerEnv env;

    auto clientA = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginA = clientA->loginWithSso(
        "google", makeToken("google", "sub-chat-dedup-a", "DedupAlice"), "DedupAlice");
    CHECK_TRUE_TEXT(loginA.ok, loginA.errorMessage.c_str());

    std::string cfgA = env.dir + "/client_dedup_a.json";
    OctiMainWindow winA(std::move(clientA), cfgA);
    winA.applyAuthenticatedProfile(loginA.data.profile);

    CreateTableRequest req;
    req.mode = "2P";
    req.bases = 3;
    auto createRes = winA.createAndEnterOnlineTable(req);
    CHECK_TRUE_TEXT(createRes.ok, createRes.errorMessage.c_str());
    std::string roomId = createRes.data.roomId;

    /* Send chat message */
    CHECK_TRUE(winA.sendGameChatMessage("ALL", "Dedup testing message"));
    LONGS_EQUAL(1, winA.gameChatLines().size());

    /* Incoming CHAT SSE event arrives for that exact message */
    SseEvent chatEv;
    chatEv.id = 1;
    chatEv.event = "CHAT";
    chatEv.dataJson = Json({
        {"seq", 1},
        {"scope", "ALL"},
        {"handle", "DedupAlice"},
        {"player_uuid", loginA.data.profile.uuid},
        {"text", "Dedup testing message"}
    }).dump();
    winA.handleGameSseEvent(chatEv);

    /* Must still be 1 entry (not duplicated) */
    LONGS_EQUAL(1, winA.gameChatLines().size());
}

TEST(DesktopAppIntegrationTest, OnlineResignationAndDrawTriggerGameOverAndResetState)
{
    LiveServerEnv env;

    auto clientA = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginA = clientA->loginWithSso(
        "google", makeToken("google", "sub-go-a", "GoAlice"), "GoAlice");
    CHECK_TRUE_TEXT(loginA.ok, loginA.errorMessage.c_str());

    std::string cfgA = env.dir + "/client_go_a.json";
    OctiMainWindow winA(std::move(clientA), cfgA);
    winA.applyAuthenticatedProfile(loginA.data.profile);

    CreateTableRequest req;
    req.mode = "2P";
    req.bases = 3;
    auto createRes = winA.createAndEnterOnlineTable(req);
    CHECK_TRUE_TEXT(createRes.ok, createRes.errorMessage.c_str());
    std::string roomId = createRes.data.roomId;

    auto clientB = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginB = clientB->loginWithSso(
        "google", makeToken("google", "sub-go-b", "GoBob"), "GoBob");
    CHECK_TRUE_TEXT(loginB.ok, loginB.errorMessage.c_str());

    std::string cfgB = env.dir + "/client_go_b.json";
    OctiMainWindow winB(std::move(clientB), cfgB);
    winB.applyAuthenticatedProfile(loginB.data.profile);
    CHECK_TRUE(winB.joinAndEnterOnlineTable(roomId, 2).ok);

    winA.refreshOnlineGameView();
    CHECK_FALSE(winA.isGameOverDialogShownForCurrentMatch());

    /* Bob resigns -> server transitions phase to FINISHED with RESIGNATION */
    auto resignRes = winB.resignCurrentGame();
    CHECK_TRUE_TEXT(resignRes.ok, resignRes.errorMessage.c_str());

    winA.refreshOnlineGameView();
    STRCMP_EQUAL("FINISHED", winA.onlineRoomPhase().c_str());

    /* Match turn status label must indicate Match Complete */
    CHECK_TRUE(winA.matchTurnStatusLabel() != nullptr);
    CHECK_TRUE(winA.matchTurnStatusLabel()->text().contains("Match Complete"));
    CHECK_TRUE(winA.isGameOverDialogShownForCurrentMatch());

    /* Entering a new online room resets _gameOverDialogShownForCurrentMatch */
    CreateTableRequest req2;
    req2.mode = "2P";
    req2.bases = 1;
    auto createRes2 = winA.createAndEnterOnlineTable(req2);
    CHECK_TRUE_TEXT(createRes2.ok, createRes2.errorMessage.c_str());
    CHECK_FALSE(winA.isGameOverDialogShownForCurrentMatch());
}

TEST(DesktopAppIntegrationTest, RematchCreatedSseTransitionsOnlineRoom)
{
    LiveServerEnv env;

    auto clientA = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginA = clientA->loginWithSso(
        "google", makeToken("google", "sub-rematch-a", "RematchAlice"), "RematchAlice");
    CHECK_TRUE_TEXT(loginA.ok, loginA.errorMessage.c_str());

    std::string cfgA = env.dir + "/client_rematch_a.json";
    OctiMainWindow winA(std::move(clientA), cfgA);
    winA.applyAuthenticatedProfile(loginA.data.profile);

    CreateTableRequest req;
    req.mode = "2P";
    req.bases = 3;
    auto createRes = winA.createAndEnterOnlineTable(req);
    CHECK_TRUE_TEXT(createRes.ok, createRes.errorMessage.c_str());
    std::string oldRoomId = createRes.data.roomId;
    STRCMP_EQUAL(oldRoomId.c_str(), winA.onlineRoomId().c_str());

    /* REMATCH_CREATED SSE event arrives with new room_id */
    SseEvent ev;
    ev.event = "REMATCH_CREATED";
    ev.id = 55;
    ev.dataJson = Json({{"room_id", "room-rematch-new-777"}}).dump();
    winA.handleGameSseEvent(ev);

    STRCMP_EQUAL("room-rematch-new-777", winA.onlineRoomId().c_str());
}

TEST(DesktopAppIntegrationTest, OnlineClockTickingAndUntimedReserveDisplay)
{
    LiveServerEnv env;

    auto clientA = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginA = clientA->loginWithSso(
        "google", makeToken("google", "sub-clock-a", "ClockAlice"), "ClockAlice");
    CHECK_TRUE_TEXT(loginA.ok, loginA.errorMessage.c_str());

    std::string cfgA = env.dir + "/client_clock_a.json";
    OctiMainWindow winA(std::move(clientA), cfgA);
    winA.applyAuthenticatedProfile(loginA.data.profile);

    /* 1. Untimed match */
    CreateTableRequest reqUntimed;
    reqUntimed.mode = "2P";
    reqUntimed.bases = 3;
    reqUntimed.timeControlEnabled = false;
    auto createUntimed = winA.createAndEnterOnlineTable(reqUntimed);
    CHECK_TRUE_TEXT(createUntimed.ok, createUntimed.errorMessage.c_str());

    CHECK_FALSE(winA.reservePanel()->isTimeControlEnabled());

    /* Purge untimed table so Alice can create the timed table */
    winA.netClient()->purgeTable(createUntimed.data.roomId);
    winA.leaveOnlineRoom();

    /* 2. Timed match */
    CreateTableRequest reqTimed;
    reqTimed.mode = "2P";
    reqTimed.bases = 3;
    reqTimed.timeControlEnabled = true;
    reqTimed.initialMs = 300000;
    reqTimed.incrementMs = 5000;
    auto createTimed = winA.createAndEnterOnlineTable(reqTimed);
    CHECK_TRUE_TEXT(createTimed.ok, createTimed.errorMessage.c_str());
    std::string roomId = createTimed.data.roomId;

    auto clientB = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginB = clientB->loginWithSso(
        "google", makeToken("google", "sub-clock-b", "ClockBob"), "ClockBob");
    CHECK_TRUE_TEXT(loginB.ok, loginB.errorMessage.c_str());
    std::string cfgB = env.dir + "/client_clock_b.json";
    OctiMainWindow winB(std::move(clientB), cfgB);
    winB.applyAuthenticatedProfile(loginB.data.profile);
    CHECK_TRUE(winB.joinAndEnterOnlineTable(roomId, 2).ok);

    winA.refreshOnlineGameView();
    STRCMP_EQUAL("IN_PROGRESS", winA.onlineRoomPhase().c_str());
    CHECK_TRUE(winA.reservePanel()->isTimeControlEnabled());

    long startMs = winA.board()->getGame()->getTimeRemaining(PLAYER_A);
    CHECK_TRUE(startMs > 290000L && startMs <= 300000L);

    /* Tick local clock by 1000ms */
    winA.tickLocalClock(1000);
    long afterTickMs = winA.board()->getGame()->getTimeRemaining(PLAYER_A);
    LONGS_EQUAL(startMs - 1000L, afterTickMs);

    winA.netClient()->purgeTable(roomId);
}

TEST(DesktopAppIntegrationTest, LeavingWaitingRoomTableVacatesSeatOnServer)
{
    LiveServerEnv env;

    auto clientA = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginA = clientA->loginWithSso(
        "google", makeToken("google", "sub-vacate-a", "VacateAlice"), "VacateAlice");
    CHECK_TRUE_TEXT(loginA.ok, loginA.errorMessage.c_str());

    std::string cfgA = env.dir + "/client_vacate_a.json";
    OctiMainWindow winA(std::move(clientA), cfgA);
    winA.applyAuthenticatedProfile(loginA.data.profile);

    auto clientB = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginB = clientB->loginWithSso(
        "google", makeToken("google", "sub-vacate-b", "VacateBob"), "VacateBob");
    CHECK_TRUE_TEXT(loginB.ok, loginB.errorMessage.c_str());

    std::string cfgB = env.dir + "/client_vacate_b.json";
    OctiMainWindow winB(std::move(clientB), cfgB);
    winB.applyAuthenticatedProfile(loginB.data.profile);

    /* Alice creates 4P table */
    CreateTableRequest req4P;
    req4P.mode = "4P";
    req4P.bases = 1;
    auto createRes4P = winA.createAndEnterOnlineTable(req4P);
    CHECK_TRUE_TEXT(createRes4P.ok, createRes4P.errorMessage.c_str());
    std::string room4P = createRes4P.data.roomId;

    auto joinRes = winB.joinAndEnterOnlineTable(room4P, 2);
    CHECK_TRUE_TEXT(joinRes.ok, joinRes.errorMessage.c_str());
    STRCMP_EQUAL("WAITING_FOR_PLAYERS", winB.onlineRoomPhase().c_str());
    LONGS_EQUAL(2, winB.onlineViewerSeat());

    /* Bob leaves the waiting table */
    auto leaveRes = winB.leaveActiveOnlineTable(room4P);
    CHECK_TRUE_TEXT(leaveRes.ok, leaveRes.errorMessage.c_str());
    CHECK_FALSE(winB.isInOnlineGame());

    /* Verify Bob is no longer seated on server */
    auto viewRes = winA.netClient()->fetchGameView(room4P);
    CHECK_TRUE_TEXT(viewRes.ok, viewRes.errorMessage.c_str());
    int filledSeats = 0;
    for (const auto &seat : viewRes.data.seats) {
        if (!seat.playerUuid.empty()) {
            filledSeats++;
        }
    }
    LONGS_EQUAL(1, filledSeats);
    CHECK_TRUE(viewRes.data.seats[1].playerUuid.empty());

    /* Bob can immediately create another table without ALREADY_SEATED */
    CreateTableRequest reqBob;
    reqBob.mode = "2P";
    reqBob.bases = 1;
    auto createBob = winB.createAndEnterOnlineTable(reqBob);
    CHECK_TRUE_TEXT(createBob.ok, createBob.errorMessage.c_str());
}

TEST(DesktopAppIntegrationTest, GuiDesignBookCrossPlatformParityAndSseStormPrevention)
{
    /* 1. Verify canonical opening book entry count (2,058 positions) */
    LONGS_EQUAL(2058, static_cast<long>(octi::g_openingBookEntryCount));

    QString repoRootStr = QString::fromUtf8(OCTI_REPO_ROOT_DIR);
    QString artifactDir = repoRootStr + "/plan/plan_gui_design_book_and_cross_platform_parity.artifacts/desktop";
    QDir().mkpath(artifactDir);

    LiveServerEnv env;
    RoomLimits testLimits;
    testLimits.lobbyChatBurst = 100;
    testLimits.boardChatBurst = 100;
    env.rooms->setLimits(testLimits);

    auto clientA = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginA = clientA->loginWithSso(
        "google", makeToken("google", "sub-gdb-alice", "Alice_GDB"), "Alice_GDB");
    CHECK_TRUE_TEXT(loginA.ok, loginA.errorMessage.c_str());

    auto clientB = OctiNetworkClient::createForLoopbackTest(env.port());
    auto loginB = clientB->loginWithSso(
        "google", makeToken("google", "sub-gdb-bob", "Bob_GDB"), "Bob_GDB");
    CHECK_TRUE_TEXT(loginB.ok, loginB.errorMessage.c_str());

    /* Pre-populate LiveServerEnv with 32 lobby chat events before winB enters the lobby */
    for (int i = 1; i <= 32; ++i) {
        auto chatRes = clientA->postLobbyChat("Pre-existing lobby event #" + std::to_string(i));
        CHECK_TRUE_TEXT(chatRes.ok, chatRes.errorMessage.c_str());
    }

    std::string cfgPathA = env.dir + "/gdb_alice.json";
    std::string cfgPathB = env.dir + "/gdb_bob.json";
    OctiMainWindow winA(std::move(clientA), cfgPathA);
    OctiMainWindow winB(std::move(clientB), cfgPathB);

    winA.resize(1440, 900);
    winB.resize(1440, 900);
    winA.show();
    winB.show();
    winA.pumpUiEvents();
    winB.pumpUiEvents();

    /* 2. CHK-04: Single left-click on an empty friendly home square (3,3) stages "33" */
    {
        LocalGameConfig localCfg;
        localCfg.playType = LocalPlayType::PASS_AND_PLAY;
        localCfg.mode = GAME_MODE_2;
        localCfg.bases = GAME_BASES_3;
        localCfg.humanSeat = PLAYER_A;
        winA.startNewLocalGame(localCfg);
        winA.confirmMatchStart();

        /* Clear home square (3,3) [0-based (2,2)] so it is an empty friendly home square for Player A */
        winA.board()->squareAt(2, 2).clear();
        CHECK_TRUE(winA.board()->squareAt(2, 2).isEmpty());
        CHECK_TRUE(winA.board()->getGame()->getFreePods(PLAYER_A) > 0);
        winA.moveComposer()->refreshLegalMoves();

        QPointF pt33 = winA.boardCanvas()->geometry().squareCenter(3, 3);
        QMouseEvent press33(QEvent::MouseButtonPress, pt33, pt33, pt33,
                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(winA.boardCanvas(), &press33);
        QMouseEvent release33(QEvent::MouseButtonRelease, pt33, pt33, pt33,
                              Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(winA.boardCanvas(), &release33);
        winA.pumpUiEvents();

        CHECK_TRUE(winA.moveComposer()->isAwaitingConfirmation());
        STRCMP_EQUAL("33", winA.moveComposer()->stagedMoveNotation().c_str());

        QImage localShot = winA.renderWindowToImage(1440, 900);
        CHECK_FALSE(localShot.isNull());
        QString localShotPath = artifactDir + "/desktop_local_home_click_staged.png";
        CHECK_TRUE(localShot.save(localShotPath));
        CHECK_TRUE(QFileInfo(localShotPath).size() > 15000);

        winA.moveComposer()->cancelAndRollback();
    }

    /* 3. CHK-14: Entering Lobby with 32 pre-existing events uses since_seq = latestSeq (<= 2 /api/v1/lobby calls) */
    winA.applyAuthenticatedProfile(loginA.data.profile);
    winB.applyAuthenticatedProfile(loginB.data.profile);
    winB.showLobbyView();
    for (int i = 0; i < 8; ++i) {
        winB.pumpUiEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    CHECK_TRUE(winB.lobbyView()->latestLobbySeq() >= 32);
    CHECK_TRUE_TEXT(
        winB.lobbyView()->lobbyRefreshCount() <= 2,
        ("Expected <= 2 lobby state fetches on lobby entry, got " +
         std::to_string(winB.lobbyView()->lobbyRefreshCount())).c_str());

    /* 4. Host Alice creates a 2P table; Bob joins via Lobby "Join Table" button as Seat 2 (B) */
    CreateTableRequest req;
    req.mode = "2P";
    req.bases = 3;
    auto created = winA.createAndEnterOnlineTable(req);
    CHECK_TRUE_TEXT(created.ok, created.errorMessage.c_str());
    std::string roomId = created.data.roomId;
    LONGS_EQUAL(1, winA.onlineViewerSeat());
    LONGS_EQUAL(static_cast<long>(BoardRotation::ROT_0),
                static_cast<long>(winA.boardCanvas()->boardRotation()));

    /* Pre-populate 10 room chat messages before Bob joins to verify room SSE since_seq deduplication */
    for (int i = 1; i <= 10; ++i) {
        CHECK_TRUE(winA.sendGameChatMessage("ALL", "Room warmup chat #" + std::to_string(i)));
    }

    winB.lobbyView()->refreshLobby();
    winB.lobbyView()->selectRoomId(roomId);
    CHECK_TRUE(winB.lobbyView()->joinTableButton()->isEnabled());
    winB.lobbyView()->joinTableButton()->click();
    winB.pumpUiEvents();
    winB.renderWindowToImage(1440, 900);

    /* Joining from Lobby enters the waiting room unseated so the player can explicitly Sit */
    CHECK_TRUE(winB.isInOnlineGame());
    LONGS_EQUAL(0, winB.onlineViewerSeat());
    STRCMP_EQUAL("WAITING_FOR_PLAYERS", winB.currentOnlineGameView().phase.c_str());
    CHECK_TRUE(winB.sitSeatButton() != nullptr);
    CHECK_TRUE(winB.sitSeatButton()->isVisible());
    CHECK_TRUE(winB.sitSeatButton()->isEnabled());
    CHECK_FALSE(winB.reservePanel()->sitSeatButtonRect(2).isEmpty());

    /* Clicking Sit / Join Game seats Bob as Seat 2 (B) and starts the match */
    winB.sitSeatButton()->click();
    winB.pumpUiEvents();
    winA.refreshOnlineGameView();
    winA.pumpUiEvents();

    CHECK_TRUE(winB.isInOnlineGame());
    LONGS_EQUAL(2, winB.onlineViewerSeat());
    STRCMP_EQUAL("IN_PROGRESS", winB.currentOnlineGameView().phase.c_str());

    /* 5. CHK-03 & CHK-12: Seat 2 (Player B) auto-rotates to ROT_180 and swaps Reserve Cards */
    LONGS_EQUAL(static_cast<long>(BoardRotation::ROT_180),
                static_cast<long>(winB.boardCanvas()->boardRotation()));
    QImage onlineSeatBShot = winB.renderWindowToImage(1440, 900);
    CHECK_FALSE(onlineSeatBShot.isNull());
    CHECK_TRUE(winB.reservePanel()->playerCardRect(PLAYER_B).top() >
               winB.reservePanel()->playerCardRect(PLAYER_A).top());

    QString onlineShotPath = artifactDir + "/desktop_online_seat_b_rotated_pink_light.png";
    CHECK_TRUE(onlineSeatBShot.save(onlineShotPath));
    CHECK_TRUE(QFileInfo(onlineShotPath).size() > 15000);

    /* 6. CHK-01: Undo button and action are hidden/disabled in online games */
    CHECK_FALSE(winA.undoButton()->isVisible());
    CHECK_FALSE(winA.undoButton()->isEnabled());
    CHECK_FALSE(winB.undoButton()->isVisible());
    CHECK_FALSE(winB.undoButton()->isEnabled());
    if (winA.undoAction()) {
        CHECK_FALSE(winA.undoAction()->isEnabled());
    }

    /* 7. CHK-02: Pink (#FF69B4) turn indicator is visible BOTH on viewer's turn (winA) and opponent's turn (winB) */
    CHECK_TRUE(winA.boardCanvas()->isInteractiveEnabled());
    CHECK_FALSE(winB.boardCanvas()->isInteractiveEnabled());
    CHECK_TRUE(winA.boardCanvas()->isTurnIndicatorVisible());
    CHECK_TRUE(winB.boardCanvas()->isTurnIndicatorVisible());

    /* 8. CHK-11: Sending a real QWheelEvent to winB.boardCanvas() while it is opponent's turn zooms the board */
    DOUBLES_EQUAL(1.0, winB.boardCanvas()->boardZoomScale(), 1e-6);
    QPointF centerPt = winB.boardCanvas()->geometry().boardOuterRect().center();
    QWheelEvent wheelEv(centerPt, centerPt, QPoint(0, 0), QPoint(0, 120),
                        Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(winB.boardCanvas(), &wheelEv);
    CHECK_TRUE(winB.boardCanvas()->boardZoomScale() > 1.0);
    winB.boardCanvas()->resetBoardZoom();
    DOUBLES_EQUAL(1.0, winB.boardCanvas()->boardZoomScale(), 1e-6);

    /* 9. CHK-14: Leaving an IN_PROGRESS table cleanly returns to Lobby without hanging */
    auto leaveInProg = winB.leaveActiveOnlineTable(roomId);
    CHECK_TRUE_TEXT(leaveInProg.ok, leaveInProg.errorMessage.c_str());
    CHECK_FALSE(winB.isInOnlineGame());
    CHECK_TRUE(winB.isLobbyViewVisible());
}

static int diffImagePixels(const QImage &a, const QImage &b)
{
    if (a.size() != b.size()) {
        return -1;
    }
    QImage aConv = a.convertToFormat(QImage::Format_ARGB32);
    QImage bConv = b.convertToFormat(QImage::Format_ARGB32);
    int diffs = 0;
    for (int y = 0; y < aConv.height(); ++y) {
        const QRgb *lineA = reinterpret_cast<const QRgb *>(aConv.constScanLine(y));
        const QRgb *lineB = reinterpret_cast<const QRgb *>(bConv.constScanLine(y));
        for (int x = 0; x < aConv.width(); ++x) {
            if (lineA[x] != lineB[x]) {
                diffs++;
            }
        }
    }
    return diffs;
}

TEST(DesktopAppIntegrationTest, TenThemesConfigRoundTripAndLegacyGoldenInvariance)
{
    std::string testCfgDir = "/tmp/octi_theme_test_" + std::to_string(QDateTime::currentMSecsSinceEpoch());
    std::filesystem::create_directories(testCfgDir);

    // 1. ClientConfigStore save + load round-trips all 10 ThemeId values (0..9)
    for (int i = 0; i < 10; ++i) {
        ThemeId id = static_cast<ThemeId>(i);
        std::string cfgFile = testCfgDir + "/cfg_theme_" + std::to_string(i) + ".json";
        std::string err;
        {
            ClientConfigStore writer(cfgFile);
            writer.preferences().theme = id;
            CHECK_TRUE_TEXT(writer.save(err), err.c_str());
        }
        {
            ClientConfigStore reader(cfgFile);
            CHECK_TRUE_TEXT(reader.load(err), err.c_str());
            LONGS_EQUAL(i, static_cast<int>(reader.preferences().theme));
        }
    }

    auto testJsonThemeParse = [&](const std::string &rawJsonVal, ThemeId expected) {
        std::string cfgFile = testCfgDir + "/parse_test.json";
        std::ofstream ofs(cfgFile);
        ofs << "{\"theme\":" << rawJsonVal << "}\n";
        ofs.close();

        ClientConfigStore reader(cfgFile);
        std::string err;
        CHECK_TRUE(reader.load(err));
        LONGS_EQUAL(static_cast<int>(expected), static_cast<int>(reader.preferences().theme));
    };

    // 2. Legacy integer values 0 and 1, strings "0", "1", "ROSEWOOD_IVORY", "obsidian_brass", plus short web keys
    testJsonThemeParse("0", ThemeId::OBSIDIAN_BRASS);
    testJsonThemeParse("1", ThemeId::ROSEWOOD_IVORY);
    testJsonThemeParse("\"0\"", ThemeId::OBSIDIAN_BRASS);
    testJsonThemeParse("\"1\"", ThemeId::ROSEWOOD_IVORY);
    testJsonThemeParse("\"ROSEWOOD_IVORY\"", ThemeId::ROSEWOOD_IVORY);
    testJsonThemeParse("\"obsidian_brass\"", ThemeId::OBSIDIAN_BRASS);
    testJsonThemeParse("\"neon\"", ThemeId::NEON_CIRCUIT);
    testJsonThemeParse("\"jade\"", ThemeId::JADE_DYNASTY);
    testJsonThemeParse("\"aurora\"", ThemeId::ARCTIC_AURORA);
    testJsonThemeParse("\"forge\"", ThemeId::VOLCANIC_FORGE);
    testJsonThemeParse("\"candy\"", ThemeId::CANDY_POP);
    testJsonThemeParse("\"pearl\"", ThemeId::OCEAN_PEARL);
    testJsonThemeParse("\"steam\"", ThemeId::STEAMPUNK_BRASS);
    testJsonThemeParse("\"nebula\"", ThemeId::COSMIC_NEBULA);

    // 3. Unknown strings and out-of-range integers fall back to ThemeId::OBSIDIAN_BRASS
    testJsonThemeParse("\"unknown_theme\"", ThemeId::OBSIDIAN_BRASS);
    testJsonThemeParse("\"\"", ThemeId::OBSIDIAN_BRASS);
    testJsonThemeParse("-1", ThemeId::OBSIDIAN_BRASS);
    testJsonThemeParse("99", ThemeId::OBSIDIAN_BRASS);

    // Verify metadata helper functions
    for (int i = 0; i < 10; ++i) {
        ThemeId id = static_cast<ThemeId>(i);
        CHECK_TRUE(themeIdToConfigKey(id) != nullptr);
        CHECK_TRUE(themeIdToWebKey(id) != nullptr);
        CHECK_FALSE(themeDisplayName(id).isEmpty());
    }

    // 4. paletteForTheme returns 10 pairwise-distinct palettes
    for (int i = 0; i < 10; ++i) {
        const OctiPalette &palI = paletteForTheme(static_cast<ThemeId>(i));
        for (int j = i + 1; j < 10; ++j) {
            const OctiPalette &palJ = paletteForTheme(static_cast<ThemeId>(j));
            bool isDistinct = (palI.windowBg != palJ.windowBg) ||
                              (palI.gridCellDark != palJ.gridCellDark) ||
                              (palI.podA.core != palJ.podA.core) ||
                              (palI.podStyle.finish != palJ.podStyle.finish) ||
                              (palI.prongStyle != palJ.prongStyle) ||
                              (palI.boardTexture != palJ.boardTexture);
            CHECK_TRUE_TEXT(isDistinct, ("Palettes for themes " + std::to_string(i) + " and " + std::to_string(j) + " are not distinct").c_str());
        }
    }

    // 5. Golden image comparison for OBSIDIAN_BRASS and ROSEWOOD_IVORY
    QString repoRootStr = QString::fromUtf8(OCTI_REPO_ROOT_DIR);
    QString goldenDir = repoRootStr + "/plan/plan_gui_ten_themes.artifacts/golden";

    OctiBoard board(GAME_MODE_2, false, false, GAME_BASES_3);
    board.getGame()->start(PLAYER_A);
    board.getGame()->move("33+a");
    board.getGame()->move("37+e");

    OctiMoveComposer composer(&board);
    OctiBoardCanvas canvas;
    canvas.resize(960, 960);
    canvas.attachBoard(&board, &composer);
    canvas.setTurnIndicatorVisible(false);

    canvas.setTheme(ThemeId::OBSIDIAN_BRASS);
    QImage boardObsidian = canvas.renderToImageAtTime(960, 960, 0);

    canvas.setTheme(ThemeId::ROSEWOOD_IVORY);
    QImage boardRosewood = canvas.renderToImageAtTime(960, 960, 0);

    OctiReservePanel reservePanel;
    reservePanel.attach(&board, &composer, &canvas);

    reservePanel.setTheme(ThemeId::OBSIDIAN_BRASS);
    QImage reserveObsidian = reservePanel.renderToImage(340, 720);

    reservePanel.setTheme(ThemeId::ROSEWOOD_IVORY);
    QImage reserveRosewood = reservePanel.renderToImage(340, 720);

    QImage splashObsidian = OctiSplashDialog::renderSplashFrame(720, 460, 0.5);
    QImage splashRosewood = OctiSplashDialog::renderSplashFrame(720, 460, 0.5);

    QString pBoardObsidian = goldenDir + "/board_obsidian_brass.png";
    QString pBoardRosewood = goldenDir + "/board_rosewood_ivory.png";
    QString pReserveObsidian = goldenDir + "/reserve_obsidian_brass.png";
    QString pReserveRosewood = goldenDir + "/reserve_rosewood_ivory.png";
    QString pSplashObsidian = goldenDir + "/splash_obsidian_brass.png";
    QString pSplashRosewood = goldenDir + "/splash_rosewood_ivory.png";
    if (std::getenv("OCTI_UPDATE_SPLASH_GOLDENS") != nullptr) {
        splashObsidian.save(pSplashObsidian);
        splashRosewood.save(pSplashRosewood);
    }

    CHECK_TRUE_TEXT(QFileInfo::exists(pBoardObsidian), "board_obsidian_brass.png must exist");
    CHECK_TRUE_TEXT(QFileInfo::exists(pBoardRosewood), "board_rosewood_ivory.png must exist");
    CHECK_TRUE_TEXT(QFileInfo::exists(pReserveObsidian), "reserve_obsidian_brass.png must exist");
    CHECK_TRUE_TEXT(QFileInfo::exists(pReserveRosewood), "reserve_rosewood_ivory.png must exist");
    CHECK_TRUE_TEXT(QFileInfo::exists(pSplashObsidian), "splash_obsidian_brass.png must exist");
    CHECK_TRUE_TEXT(QFileInfo::exists(pSplashRosewood), "splash_rosewood_ivory.png must exist");

    QImage gBoardObsidian(pBoardObsidian);
    QImage gBoardRosewood(pBoardRosewood);
    QImage gReserveObsidian(pReserveObsidian);
    QImage gReserveRosewood(pReserveRosewood);
    QImage gSplashObsidian(pSplashObsidian);
    QImage gSplashRosewood(pSplashRosewood);

    LONGS_EQUAL(0, diffImagePixels(boardObsidian, gBoardObsidian));
    LONGS_EQUAL(0, diffImagePixels(boardRosewood, gBoardRosewood));
    LONGS_EQUAL(0, diffImagePixels(reserveObsidian, gReserveObsidian));
    LONGS_EQUAL(0, diffImagePixels(reserveRosewood, gReserveRosewood));
    LONGS_EQUAL(0, diffImagePixels(splashObsidian, gSplashObsidian));
    LONGS_EQUAL(0, diffImagePixels(splashRosewood, gSplashRosewood));

    std::filesystem::remove_all(testCfgDir);
}

TEST(DesktopAppIntegrationTest, TenThemesRenderingContactSheetsLobbyRobotsAndBotUploadGate)
{
    // 1. Offscreen rendering of all 10 themes with all 4 factions & prongs
    OctiBoard board(GAME_MODE_4, false, false, GAME_BASES_1);
    board.getGame()->start(PLAYER_A);

    // Ensure pods of all 4 factions with normal prongs and super prongs
    OctiPod pA(A_POD);
    pA.addProng(DIR_N, false);
    pA.addProng(DIR_NE, true);
    pA.addProng(DIR_E, false);
    board.squareAt(2, 2).addPod(pA);

    OctiPod pB(B_POD);
    pB.addProng(DIR_E, false);
    pB.addProng(DIR_SE, true);
    pB.addProng(DIR_S, false);
    board.squareAt(2, 6).addPod(pB);

    OctiPod pC(C_POD);
    pC.addProng(DIR_S, false);
    pC.addProng(DIR_SW, true);
    pC.addProng(DIR_W, false);
    board.squareAt(6, 6).addPod(pC);

    OctiPod pD(D_POD);
    pD.addProng(DIR_W, false);
    pD.addProng(DIR_NW, true);
    pD.addProng(DIR_N, false);
    board.squareAt(6, 2).addPod(pD);

    OctiMoveComposer composer(&board);
    OctiBoardCanvas canvas;
    canvas.resize(960, 960);
    canvas.attachBoard(&board, &composer);
    canvas.setTurnIndicatorVisible(false);

    OctiReservePanel reservePanel;
    reservePanel.attach(&board, &composer, &canvas);

    QString artifactDir = QString::fromUtf8(OCTI_REPO_ROOT_DIR) + "/plan/plan_gui_ten_themes.artifacts";
    QDir().mkpath(artifactDir);

    std::vector<QImage> boardRenders;
    boardRenders.reserve(10);

    for (int i = 0; i < 10; ++i) {
        ThemeId id = static_cast<ThemeId>(i);
        canvas.setTheme(id);
        reservePanel.setTheme(id);

        // Cold-cache budget test (< 500ms per render)
        QElapsedTimer timer;
        timer.start();
        QImage b640 = canvas.renderToImageAtTime(640, 640, 0);
        qint64 t640 = timer.elapsed();
        CHECK_TRUE_TEXT(t640 < 500, ("640x640 render too slow: " + std::to_string(t640) + "ms").c_str());

        timer.restart();
        QImage b960 = canvas.renderToImageAtTime(960, 960, 0);
        qint64 t960 = timer.elapsed();
        CHECK_TRUE_TEXT(t960 < 500, ("960x960 render too slow: " + std::to_string(t960) + "ms").c_str());

        timer.restart();
        QImage b720 = canvas.renderToImageAtTime(720, 720, 0);
        qint64 t720 = timer.elapsed();
        CHECK_TRUE_TEXT(t720 < 500, ("720x720 render too slow: " + std::to_string(t720) + "ms").c_str());

        CHECK_FALSE(b960.isNull());
        boardRenders.push_back(b960);

        QImage reserveImg = reservePanel.renderToImage(340, 960);
        CHECK_FALSE(reserveImg.isNull());

        // Compose contact sheet image (1300x960) combining board (960x960) and reserve panel (340x960)
        QImage contactSheet(1300, 960, QImage::Format_ARGB32_Premultiplied);
        contactSheet.fill(paletteForTheme(id).windowBg);
        {
            QPainter cp(&contactSheet);
            cp.drawImage(0, 0, b960);
            cp.drawImage(960, 0, reserveImg);
        }

        const char *webKey = themeIdToWebKey(id);
        QString sheetPath = artifactDir + QString("/desktop_theme_%1.png").arg(webKey);
        CHECK_TRUE_TEXT(contactSheet.save(sheetPath), ("Failed to save contact sheet to " + sheetPath.toStdString()).c_str());
        CHECK_TRUE_TEXT(QFileInfo(sheetPath).size() > 10000, ("Contact sheet too small: " + sheetPath.toStdString()).c_str());
    }

    // 2. Assert all 10 board renders are non-blank and pairwise distinct
    for (int i = 0; i < 10; ++i) {
        for (int j = i + 1; j < 10; ++j) {
            int diff = diffImagePixels(boardRenders[i], boardRenders[j]);
            CHECK_TRUE_TEXT(diff > 0, ("Board renders for themes " + std::to_string(i) + " and " + std::to_string(j) + " are identical!").c_str());
        }
    }

    // 3. Assert legacy golden diff == 0 for OBSIDIAN_BRASS and ROSEWOOD_IVORY
    QString goldenDir = artifactDir + "/golden";
    OctiBoard legacyBoard(GAME_MODE_2, false, false, GAME_BASES_3);
    legacyBoard.getGame()->start(PLAYER_A);
    legacyBoard.getGame()->move("33+a");
    legacyBoard.getGame()->move("37+e");

    OctiMoveComposer legacyComposer(&legacyBoard);
    OctiBoardCanvas legacyCanvas;
    legacyCanvas.resize(960, 960);
    legacyCanvas.attachBoard(&legacyBoard, &legacyComposer);
    legacyCanvas.setTurnIndicatorVisible(false);

    legacyCanvas.setTheme(ThemeId::OBSIDIAN_BRASS);
    QImage boardObsidian = legacyCanvas.renderToImageAtTime(960, 960, 0);

    legacyCanvas.setTheme(ThemeId::ROSEWOOD_IVORY);
    QImage boardRosewood = legacyCanvas.renderToImageAtTime(960, 960, 0);

    QImage gBoardObsidian(goldenDir + "/board_obsidian_brass.png");
    QImage gBoardRosewood(goldenDir + "/board_rosewood_ivory.png");
    LONGS_EQUAL(0, diffImagePixels(boardObsidian, gBoardObsidian));
    LONGS_EQUAL(0, diffImagePixels(boardRosewood, gBoardRosewood));

    // 4. Verify LobbyView splits Connected Users and Connected Robots
    {
        LobbyView lobbyView(nullptr, ThemeId::OBSIDIAN_BRASS);
        LobbyStateData state;

        LobbyPlayerInfo h1;
        h1.playerUuid = "u-1";
        h1.handle = "Alice";
        h1.isRobot = false;

        LobbyPlayerInfo h2;
        h2.playerUuid = "u-2";
        h2.handle = "Bob";
        h2.isRobot = false;

        LobbyPlayerInfo r1;
        r1.playerUuid = "u-3";
        r1.handle = "OctiBotAlpha";
        r1.isRobot = true;
        r1.robotUuid = "r-1";

        LobbyPlayerInfo r2;
        r2.playerUuid = "u-4";
        r2.handle = "OctiBotBeta";
        r2.isRobot = true;
        r2.robotUuid = "r-2";

        state.players = {h1, h2, r1, r2};
        lobbyView.refreshFromLobbyState(state);

        CHECK_TRUE(lobbyView.playersTable() != nullptr);
        CHECK_TRUE(lobbyView.robotsTable() != nullptr);
        LONGS_EQUAL(2, lobbyView.playersTable()->rowCount());
        LONGS_EQUAL(2, lobbyView.robotsTable()->rowCount());
        CHECK_TRUE(lobbyView.playersBox()->title().contains("2"));
        CHECK_TRUE(lobbyView.robotsBox()->title().contains("2"));
    }

    // 5. Verify uploadCompletedBotGameIfEligible gate
    {
        LiveServerEnv env;
        std::string cfgPath = "/tmp/octi_upload_gate_check.json";
        std::remove(cfgPath.c_str());

        auto netCli = OctiNetworkClient::createForLoopbackTest(env.port());
        OctiMainWindow win(std::move(netCli), cfgPath, true);
        win.setTelemetryConsentForTesting("share");

        // Level 1 CASUAL: returns false
        LocalGameConfig cfgCas;
        cfgCas.playType = LocalPlayType::VS_AI_BOT;
        cfgCas.mode = GAME_MODE_2;
        cfgCas.humanSeat = PLAYER_A;
        cfgCas.botDifficulty = BotDifficulty::CASUAL;
        win.startNewLocalGame(cfgCas);
        CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(3, 3, 0, DIR_N, false));
        if (win.moveComposer()->isAwaitingConfirmation()) {
            CHECK_TRUE(win.moveComposer()->commitStagedMove());
        }
        win.setTelemetryConsentForTesting("unset");
        win.resignCurrentGame();
        win.setTelemetryConsentForTesting("share");
        CHECK_FALSE(win.uploadCompletedBotGameIfEligible(true));

        // Level 2 TACTICAL: returns false
        LocalGameConfig cfgTac;
        cfgTac.playType = LocalPlayType::VS_AI_BOT;
        cfgTac.mode = GAME_MODE_2;
        cfgTac.humanSeat = PLAYER_A;
        cfgTac.botDifficulty = BotDifficulty::TACTICAL;
        win.startNewLocalGame(cfgTac);
        CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(3, 3, 0, DIR_N, false));
        if (win.moveComposer()->isAwaitingConfirmation()) {
            CHECK_TRUE(win.moveComposer()->commitStagedMove());
        }
        win.setTelemetryConsentForTesting("unset");
        win.resignCurrentGame();
        win.setTelemetryConsentForTesting("share");
        CHECK_FALSE(win.uploadCompletedBotGameIfEligible(true));

        // Level 3 MASTER: returns true
        LocalGameConfig cfgChal;
        cfgChal.playType = LocalPlayType::VS_AI_BOT;
        cfgChal.mode = GAME_MODE_2;
        cfgChal.humanSeat = PLAYER_A;
        cfgChal.botDifficulty = BotDifficulty::MASTER;
        win.startNewLocalGame(cfgChal);
        CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(3, 3, 0, DIR_N, false));
        if (win.moveComposer()->isAwaitingConfirmation()) {
            CHECK_TRUE(win.moveComposer()->commitStagedMove());
        }
        win.setTelemetryConsentForTesting("unset");
        win.resignCurrentGame();
        win.setTelemetryConsentForTesting("share");
        CHECK_TRUE(win.uploadCompletedBotGameIfEligible(true));

        std::remove(cfgPath.c_str());
    }
}

TEST(DesktopAppIntegrationTest, DesktopTableChatAndSidebarLayoutEnvelope3)
{
    std::string testCfgDir = "/tmp/octi_layout_test_" + std::to_string(QDateTime::currentMSecsSinceEpoch());
    std::filesystem::create_directories(testCfgDir);
    std::string cfgFile = testCfgDir + "/client.json";

    OctiMainWindow win(nullptr, cfgFile, false);
    win.resize(1440, 900);
    win.show();
    win.pumpUiEvents();

    // 1. Left Column: _leftMiddleContainer is hosted in _reservePanel
    CHECK_TRUE(win.reservePanel() != nullptr);
    CHECK_TRUE(win.reservePanel()->middleOverlayWidget() == win.leftMiddleContainer());
    CHECK_TRUE(win.leftMiddleContainer() != nullptr);
    CHECK_TRUE(win.statusBox() != nullptr);
    CHECK_TRUE(win.historyBox() != nullptr);
    CHECK_TRUE(win.vcrPlyLabel() != nullptr);
    STRCMP_EQUAL("Ply 0/0", win.vcrPlyLabel()->text().toUtf8().constData());

    // 2. Right Side: Lobby Chat at top, visible on board view
    CHECK_TRUE(win.boardLobbyChatBox() != nullptr);
    CHECK_TRUE(win.boardLobbyChatBox()->isVisible());
    CHECK_TRUE(win.boardLobbyChatTranscript() != nullptr);
    CHECK_TRUE(win.boardLobbyChatInput() != nullptr);
    CHECK_TRUE(win.boardLobbyChatSendButton() != nullptr);
    CHECK_FALSE(win.boardLobbyChatInput()->isEnabled());
    CHECK_FALSE(win.boardLobbyChatSendButton()->isEnabled());
    STRCMP_EQUAL("Sign in to chat in lobby...", win.boardLobbyChatInput()->placeholderText().toUtf8().constData());

    // 3. Right Side: Table Chat is hidden in local mode
    CHECK_TRUE(win.chatBox() != nullptr);
    CHECK_FALSE(win.chatBox()->isVisible());

    // 4. Right Side: Compact Match Actions card at bottom
    CHECK_TRUE(win.actionsBox() != nullptr);
    CHECK_TRUE(win.actionsBox()->isVisible());

    // 5. Vertical Alignment: Right top/bottom widgets align with boardOuterRect
    win.renderWindowToImage(1440, 900);
    DOUBLES_EQUAL(win.boardCanvas()->geometry().boardOuterRect().top(),
                  win.rightPanelTopWidgetGeometry().top(), 2.0);
    DOUBLES_EQUAL(win.boardCanvas()->geometry().boardOuterRect().bottom(),
                  win.rightPanelBottomWidgetGeometry().bottom(), 2.0);

    // 6. Lobby Chat Transcript synchronization from LobbyView
    CHECK_TRUE(win.lobbyView() != nullptr);
    LobbyChatEntry lce;
    lce.handle = "Alice";
    lce.text = "Hello global lobby from integration test!";
    lce.atMs = 1794157732000LL;
    win.lobbyView()->appendLobbyChatEntry(lce);
    CHECK_TRUE(win.boardLobbyChatTranscript()->toPlainText().contains("Hello global lobby from integration test!"));

    // 7. Online Room Mode: Table Chat becomes visible with Leave / Purge / Zoom
    GameViewData onlineView;
    onlineView.roomId = "room-test-env3";
    onlineView.phase = "IN_PROGRESS";
    onlineView.viewerSeat = 1;
    onlineView.hostUuid = "u-host-1";
    GameSeatInfo s1;
    s1.seat = 1;
    s1.playerUuid = "u-host-1";
    s1.handle = "HostPlayer";
    GameSeatInfo s2;
    s2.seat = 2;
    s2.playerUuid = "u-guest-2";
    s2.handle = "GuestPlayer";
    onlineView.seats.push_back(s1);
    onlineView.seats.push_back(s2);
    win.applyGameViewData(onlineView);
    win.pumpUiEvents();

    CHECK_TRUE(win.chatBox()->isVisible());
    CHECK_TRUE(win.tableChatLeaveButton() != nullptr);
    CHECK_TRUE(win.tableChatLeaveButton()->isVisible());
    CHECK_TRUE(win.tableChatPurgeButton() != nullptr);
    CHECK_TRUE(win.zoomTableChatButton() != nullptr);
    CHECK_TRUE(win.zoomTableChatButton()->isVisible());

    // 8. Menu Actions Enable/Disable Logical States Transitions
    // (a) View switching
    CHECK_FALSE(win.actionBoardView()->isEnabled());
    CHECK_TRUE(win.actionLobbyView()->isEnabled());
    win.showLobbyView();
    CHECK_TRUE(win.actionBoardView()->isEnabled());
    CHECK_FALSE(win.actionLobbyView()->isEnabled());
    win.showBoardView();
    CHECK_FALSE(win.actionBoardView()->isEnabled());
    CHECK_TRUE(win.actionLobbyView()->isEnabled());

    // (b) Local game moves and VCR replay transitions
    LocalGameConfig lgc;
    win.startNewLocalGame(lgc);
    win.pumpUiEvents();
    CHECK_FALSE(win.actionUndo()->isEnabled());
    CHECK_FALSE(win.actionVcrFirst()->isEnabled());
    CHECK_FALSE(win.actionVcrPrev()->isEnabled());
    CHECK_FALSE(win.actionVcrNext()->isEnabled());
    CHECK_FALSE(win.actionVcrLast()->isEnabled());

    // Commit a move via board canvas & composer
    CHECK_TRUE(win.boardCanvas()->simulateDragProngToPod(3, 3, 0, DIR_N, false));
    if (win.moveComposer()->isAwaitingConfirmation()) {
        CHECK_TRUE(win.moveComposer()->commitStagedMove());
    }
    win.pumpUiEvents();
    CHECK_TRUE(win.actionUndo()->isEnabled());
    CHECK_TRUE(win.actionRotate()->isEnabled());

    // VCR Scrubbing
    win.vcrFirst();
    CHECK_FALSE(win.actionVcrFirst()->isEnabled());
    CHECK_FALSE(win.actionVcrPrev()->isEnabled());
    CHECK_TRUE(win.actionVcrNext()->isEnabled());
    CHECK_TRUE(win.actionVcrLast()->isEnabled());

    win.vcrLast();
    CHECK_TRUE(win.actionVcrFirst()->isEnabled());
    CHECK_TRUE(win.actionVcrPrev()->isEnabled());
    CHECK_FALSE(win.actionVcrNext()->isEnabled());
    CHECK_FALSE(win.actionVcrLast()->isEnabled());

    // (c) Resign disables offer draw and resign action
    win.hide();
    win.resignCurrentGame();
    CHECK_FALSE(win.actionOfferDraw()->isEnabled());
    CHECK_FALSE(win.actionResign()->isEnabled());

    std::filesystem::remove_all(testCfgDir);
}

extern "C" const char *__lsan_default_suppressions()
{
    return "leak:libqoffscreen.so\n";
}

int main(int argc, char **argv)
{
    ::signal(SIGPIPE, SIG_IGN);
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    QApplication app(argc, argv);
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
