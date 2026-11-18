/*
 * TestServerFoundation.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "ServerApp.hxx"
#include "BanManager.hxx"
#include "OctiHttpServer.hxx"
#include "OctiCore.hxx"
#include "Version.hxx"
#include <rapidcheck.h>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness.h>

#ifndef OCTI_SERVER_DEB_PATH
#define OCTI_SERVER_DEB_PATH ""
#endif

static std::string runCmd(const std::string &cmd)
{
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return "";
    }
    char buf[512];
    std::string result;
    while (fgets(buf, sizeof(buf), pipe) != nullptr) {
        result += buf;
    }
    pclose(pipe);
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    return result;
}

static std::string readFile(const std::string &path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());
}

TEST_GROUP(ServerFoundationTest)
{
    octi::ServerApp app;
    std::string errMsg;
};

TEST(ServerFoundationTest, NominalDefaultConfigAndRun)
{
    const char *argv[] = { "octi_server" };
    CHECK_TRUE(app.parseArgs(1, argv, errMsg));
    STRCMP_EQUAL("", errMsg.c_str());
    LONGS_EQUAL(6874, app.config().port);
    STRCMP_EQUAL("127.0.0.1", app.config().bindAddress.c_str());
    STRCMP_EQUAL("", app.config().dbPath.c_str());
    STRCMP_EQUAL("", app.config().oidcConfigPath.c_str());
    CHECK_FALSE(app.config().showHelp);
    CHECK_FALSE(app.config().showVersion);

    /* A server without a database path must refuse to start (and not block). */
    std::ostringstream out;
    std::ostringstream err;
    LONGS_EQUAL(2, app.run(out, err));
    CHECK_TRUE(err.str().find("--db") != std::string::npos);
    STRCMP_EQUAL("", out.str().c_str());
}

TEST(ServerFoundationTest, NominalNewOptionsAreParsed)
{
    const char *argv[] = { "octi_server", "--port", "9100", "--bind", "0.0.0.0",
                           "--db", "/tmp/octi_foundation.db", "--oidc-config",
                           "/tmp/octi_foundation_oidc.json",
                           "--avatar-dir", "/tmp/avatars" };
    CHECK_TRUE(app.parseArgs(11, argv, errMsg));
    LONGS_EQUAL(9100, app.config().port);
    STRCMP_EQUAL("0.0.0.0", app.config().bindAddress.c_str());
    STRCMP_EQUAL("/tmp/octi_foundation.db", app.config().dbPath.c_str());
    STRCMP_EQUAL("/tmp/octi_foundation_oidc.json",
                 app.config().oidcConfigPath.c_str());
    STRCMP_EQUAL("/tmp/avatars", app.config().avatarDir.c_str());

    /* Parsing again starts from the defaults. */
    const char *again[] = { "octi_server", "--db", "x.db" };
    CHECK_TRUE(app.parseArgs(3, again, errMsg));
    LONGS_EQUAL(6874, app.config().port);
    STRCMP_EQUAL("127.0.0.1", app.config().bindAddress.c_str());
    STRCMP_EQUAL("", app.config().oidcConfigPath.c_str());
    STRCMP_EQUAL("", app.config().avatarDir.c_str());
}

TEST(ServerFoundationTest, NominalVersionAndHelpFlags)
{
    const char *argvVer[] = { "octi_server", "--version" };
    CHECK_TRUE(app.parseArgs(2, argvVer, errMsg));
    CHECK_TRUE(app.config().showVersion);

    std::ostringstream outVer;
    std::ostringstream errVer;
    LONGS_EQUAL(0, app.run(outVer, errVer));
    CHECK_TRUE(outVer.str().find(octi::OctiCore::versionString()) != std::string::npos);

    const char *argvHelp[] = { "octi_server", "--help" };
    CHECK_TRUE(app.parseArgs(2, argvHelp, errMsg));
    CHECK_TRUE(app.config().showHelp);

    std::ostringstream outHelp;
    std::ostringstream errHelp;
    LONGS_EQUAL(0, app.run(outHelp, errHelp));
    CHECK_TRUE(outHelp.str().find("Usage: octi_server") != std::string::npos);
    CHECK_TRUE(outHelp.str().find("--bind") != std::string::npos);
    CHECK_TRUE(outHelp.str().find("--db") != std::string::npos);
    CHECK_TRUE(outHelp.str().find("--oidc-config") != std::string::npos);
    CHECK_TRUE(outHelp.str().find("--avatar-dir") != std::string::npos);
}

TEST(ServerFoundationTest, BoundaryPortValues)
{
    /* Port 0 asks the OS for an ephemeral port. */
    const char *argvZero[] = { "octi_server", "--port", "0" };
    CHECK_TRUE(app.parseArgs(3, argvZero, errMsg));
    LONGS_EQUAL(0, app.config().port);

    const char *argvMin[] = { "octi_server", "--port", "1" };
    CHECK_TRUE(app.parseArgs(3, argvMin, errMsg));
    LONGS_EQUAL(1, app.config().port);

    const char *argvMax[] = { "octi_server", "-p", "65535" };
    CHECK_TRUE(app.parseArgs(3, argvMax, errMsg));
    LONGS_EQUAL(65535, app.config().port);
}

TEST(ServerFoundationTest, FaultInvalidArgsAndPorts)
{
    const char *argvUnknown[] = { "octi_server", "--unknown" };
    CHECK_FALSE(app.parseArgs(2, argvUnknown, errMsg));
    CHECK_TRUE(errMsg.find("Unknown option") != std::string::npos);

    const char *argvMissing[] = { "octi_server", "--port" };
    CHECK_FALSE(app.parseArgs(2, argvMissing, errMsg));
    CHECK_TRUE(errMsg.find("Missing value") != std::string::npos);

    const char *argvOverflow[] = { "octi_server", "--port", "65536" };
    CHECK_FALSE(app.parseArgs(3, argvOverflow, errMsg));

    const char *argvNeg[] = { "octi_server", "--port", "-1" };
    CHECK_FALSE(app.parseArgs(3, argvNeg, errMsg));

    const char *argvAlpha[] = { "octi_server", "--port", "8080abc" };
    CHECK_FALSE(app.parseArgs(3, argvAlpha, errMsg));

    /* New options need a value, and a value may not be another option. */
    const char *argvNoDb[] = { "octi_server", "--db" };
    CHECK_FALSE(app.parseArgs(2, argvNoDb, errMsg));
    CHECK_TRUE(errMsg.find("Missing value for --db") != std::string::npos);

    const char *argvDbOpt[] = { "octi_server", "--db", "--port", "80" };
    CHECK_FALSE(app.parseArgs(4, argvDbOpt, errMsg));
    CHECK_TRUE(errMsg.find("Missing value for --db") != std::string::npos);

    const char *argvNoBind[] = { "octi_server", "--bind" };
    CHECK_FALSE(app.parseArgs(2, argvNoBind, errMsg));
    CHECK_TRUE(errMsg.find("Missing value for --bind") != std::string::npos);

    const char *argvEmptyBind[] = { "octi_server", "--bind", "" };
    CHECK_FALSE(app.parseArgs(3, argvEmptyBind, errMsg));

    const char *argvSpaceBind[] = { "octi_server", "--bind", "127.0.0.1 ; x" };
    CHECK_FALSE(app.parseArgs(3, argvSpaceBind, errMsg));
    CHECK_TRUE(errMsg.find("Invalid bind address") != std::string::npos);

    const char *argvNoOidc[] = { "octi_server", "--oidc-config" };
    CHECK_FALSE(app.parseArgs(2, argvNoOidc, errMsg));
    CHECK_TRUE(errMsg.find("Missing value for --oidc-config") != std::string::npos);
}

TEST(ServerFoundationTest, DebPackageMatchesReadmeVersionAndSystemdSpec)
{
    std::string debPath = OCTI_SERVER_DEB_PATH;
    CHECK_FALSE_TEXT(debPath.empty(), "OCTI_SERVER_DEB_PATH must not be empty");

    struct stat st;
    CHECK_EQUAL_TEXT(0, stat(debPath.c_str(), &st), "Debian package must exist");
    CHECK_TRUE(st.st_size > 0);

    const std::string expectedFileName =
        std::string("octi-server_") + octi::OctiCore::versionString() + "_amd64.deb";
    CHECK_TRUE_TEXT(debPath.find(expectedFileName) != std::string::npos, debPath.c_str());

    std::string pkg = runCmd("dpkg-deb -f '" + debPath + "' Package");
    STRCMP_EQUAL("octi-server", pkg.c_str());

    std::string ver = runCmd("dpkg-deb -f '" + debPath + "' Version");
    STRCMP_EQUAL(octi::OctiCore::versionString(), ver.c_str());

    std::string arch = runCmd("dpkg-deb -f '" + debPath + "' Architecture");
    STRCMP_EQUAL("amd64", arch.c_str());

    std::string deps = runCmd("dpkg-deb -f '" + debPath + "' Depends");
    CHECK_TRUE(deps.find("libssl3t64") != std::string::npos);
    CHECK_TRUE(deps.find("libsqlite3-0") != std::string::npos);
    CHECK_TRUE(deps.find("libc6") != std::string::npos);
    CHECK_TRUE(deps.find("adduser") != std::string::npos);

    char tmpDirTemplate[] = "/tmp/octi_deb_test_XXXXXX";
    char *tmpDir = mkdtemp(tmpDirTemplate);
    CHECK_TRUE(tmpDir != nullptr);
    std::string tmpDirStr(tmpDir);

    runCmd("mkdir -p '" + tmpDirStr + "/DEBIAN' && dpkg-deb -e '" + debPath + "' '" + tmpDirStr + "/DEBIAN'");
    runCmd("mkdir -p '" + tmpDirStr + "/root' && dpkg-deb -x '" + debPath + "' '" + tmpDirStr + "/root'");

    std::string conffiles = readFile(tmpDirStr + "/DEBIAN/conffiles");
    CHECK_TRUE(conffiles.find("/etc/default/octi-server") != std::string::npos);
    CHECK_TRUE(conffiles.find("/etc/octi/oidc.json") != std::string::npos);
    CHECK_TRUE(conffiles.find("/etc/octi/bans.json") != std::string::npos);

    struct stat scriptSt;
    CHECK_EQUAL(0, stat((tmpDirStr + "/DEBIAN/postinst").c_str(), &scriptSt));
    CHECK_TRUE((scriptSt.st_mode & 0111) != 0);
    CHECK_EQUAL(0, stat((tmpDirStr + "/DEBIAN/prerm").c_str(), &scriptSt));
    CHECK_TRUE((scriptSt.st_mode & 0111) != 0);
    CHECK_EQUAL(0, stat((tmpDirStr + "/DEBIAN/postrm").c_str(), &scriptSt));
    CHECK_TRUE((scriptSt.st_mode & 0111) != 0);

    std::string binPath = tmpDirStr + "/root/usr/sbin/octi-server";
    struct stat binSt;
    CHECK_EQUAL(0, stat(binPath.c_str(), &binSt));
    CHECK_TRUE((binSt.st_mode & 0111) != 0);

    std::string verOutput = runCmd("'" + binPath + "' --version");
    CHECK_TRUE(verOutput.find(octi::OctiCore::versionString()) != std::string::npos);

    std::string svcContent = readFile(tmpDirStr + "/root/lib/systemd/system/octi-server.service");
    CHECK_TRUE(svcContent.find("ExecStart=/usr/sbin/octi-server") != std::string::npos);
    CHECK_TRUE(svcContent.find("--ban-config ${OCTI_BAN_CONFIG}") != std::string::npos);
    CHECK_TRUE(svcContent.find("--avatar-dir ${OCTI_AVATAR_DIR}") != std::string::npos);
    CHECK_TRUE(svcContent.find("$OCTI_EXTRA_ARGS") != std::string::npos);
    CHECK_TRUE(svcContent.find("LimitNOFILE=65536") != std::string::npos);
    CHECK_TRUE(svcContent.find("UMask=0077") != std::string::npos);
    CHECK_TRUE(svcContent.find("ReadWritePaths=/var/lib/octi /var/octi/avatar /etc/octi") != std::string::npos);
    CHECK_TRUE(svcContent.find("ReadOnlyPaths=-/etc/octi/oidc.json") != std::string::npos);
    CHECK_TRUE(svcContent.find("User=octi") != std::string::npos);
    CHECK_TRUE(svcContent.find("Group=octi") != std::string::npos);
    CHECK_TRUE(svcContent.find("KillSignal=SIGTERM") != std::string::npos);
    CHECK_TRUE(svcContent.find("EnvironmentFile=-/etc/default/octi-server") != std::string::npos);

    std::string defaultContent = readFile(tmpDirStr + "/root/etc/default/octi-server");
    CHECK_TRUE(defaultContent.find("OCTI_BIND_ADDRESS=127.0.0.1") != std::string::npos);
    CHECK_TRUE(defaultContent.find("OCTI_PORT=6874") != std::string::npos);
    CHECK_TRUE(defaultContent.find("OCTI_DB_PATH=/var/lib/octi/octi.db") != std::string::npos);
    CHECK_TRUE(defaultContent.find("OCTI_OIDC_CONFIG=/etc/octi/oidc.json") != std::string::npos);
    CHECK_TRUE(defaultContent.find("OCTI_BAN_CONFIG=/etc/octi/bans.json") != std::string::npos);
    CHECK_TRUE(defaultContent.find("OCTI_AVATAR_DIR=/var/octi/avatar") != std::string::npos);
    CHECK_TRUE(defaultContent.find("OCTI_EXTRA_ARGS") != std::string::npos);

    std::string postinstContent = readFile(tmpDirStr + "/DEBIAN/postinst");
    CHECK_TRUE(postinstContent.find("/var/octi/avatar") != std::string::npos);
    CHECK_TRUE(postinstContent.find("install -d -m 0750 -o octi -g octi /etc/octi") != std::string::npos);
    CHECK_TRUE(postinstContent.find("chmod 0600 /etc/octi/oidc.json") != std::string::npos);
    CHECK_TRUE(postinstContent.find("chmod 0640 /etc/octi/bans.json") != std::string::npos);

    std::string bansPath = tmpDirStr + "/root/etc/octi/bans.json";
    struct stat bansSt;
    CHECK_EQUAL(0, stat(bansPath.c_str(), &bansSt));
    LONGS_EQUAL(0640, bansSt.st_mode & 0777);
    std::string bansContent = readFile(bansPath);
    CHECK_TRUE(bansContent.find("\"version\":1") != std::string::npos ||
               bansContent.find("\"version\": 1") != std::string::npos);
    CHECK_TRUE(bansContent.find("\"entries\":[]") != std::string::npos ||
               bansContent.find("\"entries\": []") != std::string::npos);

    std::string apacheConfPath = tmpDirStr + "/root/usr/share/doc/octi-server/apache2-octi.conf.example";
    struct stat apacheSt;
    CHECK_EQUAL(0, stat(apacheConfPath.c_str(), &apacheSt));
    std::string apacheContent = readFile(apacheConfPath);
    CHECK_TRUE(apacheContent.find("127.0.0.1:6874") != std::string::npos);
    CHECK_TRUE(apacheContent.find("Strict-Transport-Security") != std::string::npos);

    struct stat etcOctiSt;
    CHECK_EQUAL(0, stat((tmpDirStr + "/root/etc/octi").c_str(), &etcOctiSt));
    LONGS_EQUAL(0750, etcOctiSt.st_mode & 0777);

    std::string oidcPath = tmpDirStr + "/root/etc/octi/oidc.json";
    struct stat oidcSt;
    CHECK_EQUAL(0, stat(oidcPath.c_str(), &oidcSt));
    LONGS_EQUAL(0600, oidcSt.st_mode & 0777);

    octi::OidcConfig parsedConfig;
    std::string oidcErr;
    CHECK_TRUE(octi::ServerApp::loadOidcConfig(oidcPath, parsedConfig, oidcErr));
    STRCMP_EQUAL("", oidcErr.c_str());

    /* Boundary check: /etc/octi with 0750 permissions also satisfies checkSecretFile
     * when oidc.json contains a client_secret. */
    {
        std::ofstream ofs(oidcPath, std::ios::out | std::ios::trunc);
        ofs << "{\n"
               "  \"providers\": {\n"
               "    \"google\": {\n"
               "      \"client_ids\": [\"test-client.apps.googleusercontent.com\"],\n"
               "      \"client_secret\": \"GOCSPX-test-secret-0123456789\"\n"
               "    }\n"
               "  }\n"
               "}\n";
        ofs.close();
        chmod(oidcPath.c_str(), 0600);
        octi::OidcConfig secretConfig;
        std::string secretErr;
        CHECK_TRUE_TEXT(
            octi::ServerApp::loadOidcConfig(oidcPath, secretConfig, secretErr),
            secretErr.c_str());
        STRCMP_EQUAL("", secretErr.c_str());
    }

    runCmd("rm -rf '" + tmpDirStr + "'");
}

TEST(ServerFoundationTest, ProtocolVersionMatchesReadme)
{
    LONGS_EQUAL(1, octi::OCTI_PROTOCOL_VERSION);
    LONGS_EQUAL(1, octi::OCTI_MIN_PROTOCOL_VERSION);

    /* Read README.md to verify Protocol Version annotation */
    std::string readme = readFile("README.md");
    if (readme.empty()) {
        readme = readFile("../README.md");
    }
    if (readme.empty()) {
        readme = readFile("../../README.md");
    }
    const char *home = std::getenv("HOME");
    if (readme.empty() && home != nullptr) {
        readme = readFile(std::string(home) + "/work/octi/README.md");
    }
    CHECK_FALSE_TEXT(readme.empty(), "README.md must be readable");
    size_t pos = readme.find("**Protocol Version: ");
    CHECK_TRUE_TEXT(pos != std::string::npos, "README.md must contain **Protocol Version: <N>**");
    size_t endPos = readme.find("**", pos + 20);
    CHECK_TRUE_TEXT(endPos != std::string::npos, "Malformed **Protocol Version: <N>** tag");
    std::string protoStr = readme.substr(pos + 20, endPos - (pos + 20));
    int protoVal = std::atoi(protoStr.c_str());
    LONGS_EQUAL(octi::OCTI_PROTOCOL_VERSION, protoVal);
}

TEST(ServerFoundationTest, Canonical13VariantCodeBijectiveRoundTrip)
{
    const std::vector<std::string> expected = {
        "2P_3B",    "2P_3B_SP", "2P_3B_ED", "2P_3B_SPED",
        "2P_1B",    "2P_1B_SP", "2P_1B_ED", "2P_1B_SPED",
        "4P",       "4P_SP",    "4P_ED",    "4P_SPED",
        "OFK"
    };

    const auto &codes = octi::allOctiVariantCodes();
    LONGS_EQUAL(13, static_cast<long>(codes.size()));
    for (size_t i = 0; i < expected.size(); ++i) {
        STRCMP_EQUAL(expected[i].c_str(), codes[i].c_str());
    }

    std::vector<std::string> seenTuples;
    for (size_t i = 0; i < codes.size(); ++i) {
        const std::string &code = codes[i];
        CHECK_TRUE(octi::isValidOctiVariantCode(code));

        std::string lowerCode = code;
        for (size_t c = 0; c < lowerCode.size(); ++c) {
            if (lowerCode[c] >= 'A' && lowerCode[c] <= 'Z') {
                lowerCode[c] = static_cast<char>(lowerCode[c] - 'A' + 'a');
            }
        }
        CHECK_TRUE(octi::isValidOctiVariantCode(lowerCode));

        octi::OctiGameMode mode = octi::GAME_MODE_2;
        int bases = 0;
        bool sp = false;
        bool ed = false;
        CHECK_TRUE(octi::parseOctiVariantCode(code, mode, bases, sp, ed));

        octi::OctiGameMode modeLower = octi::GAME_MODE_2;
        int basesLower = 0;
        bool spLower = false;
        bool edLower = false;
        CHECK_TRUE(octi::parseOctiVariantCode(lowerCode, modeLower, basesLower,
                                              spLower, edLower));
        LONGS_EQUAL(static_cast<long>(mode), static_cast<long>(modeLower));
        LONGS_EQUAL(bases, basesLower);
        CHECK_EQUAL(sp, spLower);
        CHECK_EQUAL(ed, edLower);

        octi::OctiGameMode modeEnum = octi::GAME_MODE_2;
        octi::OctiGameBases basesEnum = octi::GAME_BASES_3;
        bool spEnum = false;
        bool edEnum = false;
        CHECK_TRUE(octi::parseOctiVariantCode(code, modeEnum, basesEnum,
                                              spEnum, edEnum));
        LONGS_EQUAL(bases, static_cast<int>(basesEnum));

        STRCMP_EQUAL(code.c_str(),
                     octi::octiVariantCode(mode, bases, sp, ed).c_str());
        STRCMP_EQUAL(code.c_str(),
                     octi::octiVariantCode(modeEnum, basesEnum, spEnum, edEnum)
                         .c_str());

        const char *modeStr = (mode == octi::GAME_MODE_OFK) ? "OFK"
                            : (mode == octi::GAME_MODE_4)   ? "4P"
                                                            : "2P";
        STRCMP_EQUAL(code.c_str(),
                     octi::octiVariantCodeFromModeStr(modeStr, bases, sp, ed)
                         .c_str());
        STRCMP_EQUAL(code.c_str(),
                     octi::octiVariantCodeFromModeStr(code, bases, sp, ed)
                         .c_str());
        STRCMP_EQUAL(code.c_str(),
                     octi::octiVariantCodeFromModeStr(lowerCode, bases, sp, ed)
                         .c_str());

        std::string tupleKey = std::to_string(static_cast<int>(mode)) + ":" +
                               std::to_string(bases) + ":" +
                               (sp ? "1" : "0") + ":" + (ed ? "1" : "0");
        for (size_t k = 0; k < seenTuples.size(); ++k) {
            CHECK_TRUE(seenTuples[k] != tupleKey);
        }
        seenTuples.push_back(tupleKey);
    }
    LONGS_EQUAL(13, static_cast<long>(seenTuples.size()));

    /* OFK normalizes to "OFK" regardless of bases or superProngs/edgeless */
    STRCMP_EQUAL("OFK",
                 octi::octiVariantCode(octi::GAME_MODE_OFK, 1, false, false).c_str());
    STRCMP_EQUAL("OFK",
                 octi::octiVariantCode(octi::GAME_MODE_OFK, 3, true, true).c_str());
    STRCMP_EQUAL("OFK",
                 octi::octiVariantCodeFromModeStr("OFK", 3, true, true).c_str());
    STRCMP_EQUAL("OFK",
                 octi::octiVariantCodeFromModeStr("ofk", 1, true, false).c_str());

    /* 4P* normalizes regardless of legacy bases (1 or 3) */
    STRCMP_EQUAL("4P",
                 octi::octiVariantCode(octi::GAME_MODE_4, 1, false, false).c_str());
    STRCMP_EQUAL("4P_SP",
                 octi::octiVariantCode(octi::GAME_MODE_4, 1, true, false).c_str());
    STRCMP_EQUAL("4P_ED",
                 octi::octiVariantCode(octi::GAME_MODE_4, 1, false, true).c_str());
    STRCMP_EQUAL("4P_SPED",
                 octi::octiVariantCode(octi::GAME_MODE_4, 1, true, true).c_str());
    STRCMP_EQUAL("4P",
                 octi::octiVariantCodeFromModeStr("4P", 1, false, false).c_str());
    STRCMP_EQUAL("4P_SP",
                 octi::octiVariantCodeFromModeStr("4P", 1, true, false).c_str());
    STRCMP_EQUAL("4P_ED",
                 octi::octiVariantCodeFromModeStr("4", 1, false, true).c_str());
    STRCMP_EQUAL("4P_SPED",
                 octi::octiVariantCodeFromModeStr("4", 1, true, true).c_str());

    /* Reject non-canonical / broad / malformed variant codes */
    const std::vector<std::string> invalid = {
        "", "2P", "2P_ALL", "4P_ALL", "3P", "2P_2B",
        "4P_1B", "4P_3B", "OFK_SP", "OFK_ED", "OFK_SPED", "BOGUS"
    };
    for (size_t i = 0; i < invalid.size(); ++i) {
        octi::OctiGameMode m = octi::GAME_MODE_2;
        int b = 3;
        bool sp = false;
        bool ed = false;
        CHECK_FALSE(octi::isValidOctiVariantCode(invalid[i]));
        CHECK_FALSE(octi::parseOctiVariantCode(invalid[i], m, b, sp, ed));
    }

    /* Official rated variants: strictly 2P_3B, 2P_1B, and OFK */
    for (size_t i = 0; i < codes.size(); ++i) {
        const std::string &code = codes[i];
        bool wantRated = (code == "2P_3B" || code == "2P_1B" || code == "OFK");
        CHECK_EQUAL(wantRated, octi::isOfficialRatedVariantCode(code));

        octi::OctiGameMode m = octi::GAME_MODE_2;
        int b = 3;
        bool sp = false;
        bool ed = false;
        CHECK_TRUE(octi::parseOctiVariantCode(code, m, b, sp, ed));
        CHECK_EQUAL(wantRated, octi::isOfficialRatedVariant(m, b, sp, ed));
    }
    CHECK_TRUE(octi::isOfficialRatedVariantCode("2p_3b"));
    CHECK_TRUE(octi::isOfficialRatedVariantCode("2p_1b"));
    CHECK_TRUE(octi::isOfficialRatedVariantCode("ofk"));
    CHECK_TRUE(octi::isOfficialRatedVariant(octi::GAME_MODE_OFK, 3, false, false));
    CHECK_FALSE(octi::isOfficialRatedVariant(octi::GAME_MODE_4, 1, false, false));
    CHECK_FALSE(octi::isOfficialRatedVariantCode("2P"));
    CHECK_FALSE(octi::isOfficialRatedVariantCode(""));
}

TEST(ServerFoundationTest, TrustedProxyPublicBaseUrlAndBehindTlsCliOptions)
{
    const char *argvOk[] = {
        "octi_server",
        "--db", "/tmp/octi_cli_sec.db",
        "--trusted-proxies", "127.0.0.1, ::1, ::ffff:198.51.100.5",
        "--public-base-url", "HTTPS://OCTI.SELFSO.COM/",
        "--behind-tls"
    };
    CHECK_TRUE(app.parseArgs(
        static_cast<int>(sizeof(argvOk) / sizeof(argvOk[0])), argvOk, errMsg));
    STRCMP_EQUAL("", errMsg.c_str());
    CHECK_TRUE(app.config().behindTls);
    STRCMP_EQUAL("https://octi.selfso.com", app.config().publicBaseUrl.c_str());
    LONGS_EQUAL(3, static_cast<long>(app.config().trustedProxies.size()));
    STRCMP_EQUAL("127.0.0.1", app.config().trustedProxies[0].c_str());
    STRCMP_EQUAL("::1", app.config().trustedProxies[1].c_str());
    STRCMP_EQUAL("198.51.100.5", app.config().trustedProxies[2].c_str());

    const char *argvIpv6Url[] = {
        "octi_server",
        "--trusted-proxies", "none",
        "--public-base-url", "http://[2001:DB8::1]:8443"
    };
    CHECK_TRUE(app.parseArgs(5, argvIpv6Url, errMsg));
    CHECK_TRUE(app.config().trustedProxies.empty());
    CHECK_FALSE(app.config().behindTls);
    STRCMP_EQUAL("http://[2001:db8::1]:8443", app.config().publicBaseUrl.c_str());

    std::string helpText = octi::ServerApp::usageBanner("octi_server");
    CHECK_TRUE(helpText.find("--trusted-proxies") != std::string::npos);
    CHECK_TRUE(helpText.find("--public-base-url") != std::string::npos);
    CHECK_TRUE(helpText.find("--behind-tls") != std::string::npos);

    /* Fault injection: missing, empty, or malformed CLI option values */
    const char *badTpMissing[] = { "octi_server", "--trusted-proxies" };
    CHECK_FALSE(app.parseArgs(2, badTpMissing, errMsg));

    const char *badTpEmpty[] = { "octi_server", "--trusted-proxies", "   " };
    CHECK_FALSE(app.parseArgs(3, badTpEmpty, errMsg));

    const char *badTpTrailing[] = { "octi_server", "--trusted-proxies", "127.0.0.1," };
    CHECK_FALSE(app.parseArgs(3, badTpTrailing, errMsg));

    const char *badTpGarbage[] = { "octi_server", "--trusted-proxies", "127.0.0.1,not-an-ip" };
    CHECK_FALSE(app.parseArgs(3, badTpGarbage, errMsg));

    const char *badTpCidr[] = { "octi_server", "--trusted-proxies", "127.0.0.1/24" };
    CHECK_FALSE(app.parseArgs(3, badTpCidr, errMsg));

    const char *badUrlMissing[] = { "octi_server", "--public-base-url" };
    CHECK_FALSE(app.parseArgs(2, badUrlMissing, errMsg));

    const char *badUrlScheme[] = { "octi_server", "--public-base-url", "ftp://octi.selfso.com" };
    CHECK_FALSE(app.parseArgs(3, badUrlScheme, errMsg));

    const char *badUrlPath[] = { "octi_server", "--public-base-url", "https://octi.selfso.com/api" };
    CHECK_FALSE(app.parseArgs(3, badUrlPath, errMsg));

    const char *badUrlQuery[] = { "octi_server", "--public-base-url", "https://octi.selfso.com?a=1" };
    CHECK_FALSE(app.parseArgs(3, badUrlQuery, errMsg));

    const char *badUrlPort[] = { "octi_server", "--public-base-url", "https://octi.selfso.com:70000" };
    CHECK_FALSE(app.parseArgs(3, badUrlPort, errMsg));
}

TEST(ServerFoundationTest, NormalizeIpLiteralAndTrustedProxyRightToLeftExtraction)
{
    STRCMP_EQUAL("203.0.113.9",
                 octi::OctiHttpServer::normalizeIpLiteral("  203.0.113.9 \r\n").c_str());
    STRCMP_EQUAL("203.0.113.9",
                 octi::OctiHttpServer::normalizeIpLiteral("::ffff:203.0.113.9").c_str());
    STRCMP_EQUAL("203.0.113.9",
                 octi::OctiHttpServer::normalizeIpLiteral("::FFFF:cb00:7109").c_str());
    STRCMP_EQUAL("2001:db8::1",
                 octi::OctiHttpServer::normalizeIpLiteral("2001:0DB8:0000:0000:0000:0000:0000:0001").c_str());
    STRCMP_EQUAL("", octi::OctiHttpServer::normalizeIpLiteral("").c_str());
    STRCMP_EQUAL("", octi::OctiHttpServer::normalizeIpLiteral("999.1.2.3").c_str());
    STRCMP_EQUAL("", octi::OctiHttpServer::normalizeIpLiteral("203.0.113.9:8080").c_str());
    STRCMP_EQUAL("", octi::OctiHttpServer::normalizeIpLiteral("203.0.113.0/24").c_str());
    STRCMP_EQUAL("", octi::OctiHttpServer::normalizeIpLiteral(std::string(200, '1')).c_str());

    const std::vector<std::string> trusted = {
        "127.0.0.1", "::1", "::ffff:127.0.0.1", "198.51.100.254"
    };

    /* Multi-hop behind trusted proxy selects rightmost untrusted hop */
    STRCMP_EQUAL("203.0.113.9",
                 octi::OctiHttpServer::resolveClientIp(
                     "127.0.0.1",
                     "1.2.3.4, 203.0.113.9, 198.51.100.254, 127.0.0.1",
                     "", trusted).c_str());

    /* IPv4-mapped IPv6 peer and hop normalize cleanly */
    STRCMP_EQUAL("203.0.113.9",
                 octi::OctiHttpServer::resolveClientIp(
                     "::ffff:127.0.0.1", "::ffff:203.0.113.9", "", trusted).c_str());

    /* Untrusted peer cannot spoof X-Forwarded-For or X-Real-IP */
    STRCMP_EQUAL("198.51.100.44",
                 octi::OctiHttpServer::resolveClientIp(
                     "::ffff:198.51.100.44", "203.0.113.9", "203.0.113.10", trusted).c_str());

    /* Malformed rightmost hop fails closed to peer */
    STRCMP_EQUAL("127.0.0.1",
                 octi::OctiHttpServer::resolveClientIp(
                     "127.0.0.1", "203.0.113.9, malformed_hop", "198.51.100.88", trusted).c_str());

    /* Spoofed garbage on the left of a valid untrusted hop appended by proxy */
    STRCMP_EQUAL("203.0.113.9",
                 octi::OctiHttpServer::resolveClientIp(
                     "127.0.0.1", "not_an_ip, 203.0.113.9", "", trusted).c_str());

    /* X-Real-IP fallback when X-Forwarded-For is absent or all-trusted */
    STRCMP_EQUAL("198.51.100.88",
                 octi::OctiHttpServer::resolveClientIp(
                     "::1", "", "198.51.100.88", trusted).c_str());
    STRCMP_EQUAL("::1",
                 octi::OctiHttpServer::resolveClientIp(
                     "::1", "", "bogus_ip", trusted).c_str());
}

TEST(ServerFoundationTest, RapidCheckNormalizeIpLiteralInvariants)
{
    bool idempotentOk = rc::check(
        "normalizeIpLiteral is idempotent and bounded on arbitrary inputs",
        []() {
            std::string s = *rc::gen::arbitrary<std::string>();
            std::string n = octi::OctiHttpServer::normalizeIpLiteral(s);
            if (!n.empty()) {
                RC_ASSERT(n.size() <= 45u);
                RC_ASSERT(octi::OctiHttpServer::normalizeIpLiteral(n) == n);
                for (size_t i = 0; i < n.size(); ++i) {
                    unsigned char c = static_cast<unsigned char>(n[i]);
                    RC_ASSERT(c > 0x20 && c < 0x7F);
                    RC_ASSERT(!std::isupper(c));
                }
            }
            std::string oversized(129 + *rc::gen::inRange<size_t>(0, 256), '1');
            RC_ASSERT(octi::OctiHttpServer::normalizeIpLiteral(oversized).empty());
        });
    CHECK_TRUE(idempotentOk);

    bool v4MappedOk = rc::check(
        "IPv4 and IPv4-mapped IPv6 representations normalize to identical dotted-quad",
        []() {
            int a = *rc::gen::inRange(0, 256);
            int b = *rc::gen::inRange(0, 256);
            int c = *rc::gen::inRange(0, 256);
            int d = *rc::gen::inRange(0, 256);
            std::string dotted = std::to_string(a) + "." + std::to_string(b) +
                                 "." + std::to_string(c) + "." + std::to_string(d);
            std::ostringstream hexMapped;
            hexMapped << "::FFFF:" << std::hex << std::uppercase
                      << ((a << 8) | b) << ":" << ((c << 8) | d);

            RC_ASSERT(octi::OctiHttpServer::normalizeIpLiteral(dotted) == dotted);
            RC_ASSERT(octi::OctiHttpServer::normalizeIpLiteral("  " + dotted + "\t") == dotted);
            RC_ASSERT(octi::OctiHttpServer::normalizeIpLiteral("::ffff:" + dotted) == dotted);
            RC_ASSERT(octi::OctiHttpServer::normalizeIpLiteral(hexMapped.str()) == dotted);
        });
    CHECK_TRUE(v4MappedOk);

    bool v6CanonicalOk = rc::check(
        "expanded uppercase IPv6 normalizes to canonical lowercase IPv6",
        []() {
            std::array<int, 8> g;
            for (size_t i = 0; i < 8; ++i) {
                g[i] = *rc::gen::inRange(0, 65536);
            }
            /* Exclude ::ffff:0:0/96 which intentionally maps to IPv4 */
            RC_PRE(!(g[0] == 0 && g[1] == 0 && g[2] == 0 &&
                     g[3] == 0 && g[4] == 0 && g[5] == 0xFFFF));

            std::ostringstream expanded;
            expanded << std::hex << std::uppercase << std::setfill('0');
            for (size_t i = 0; i < 8; ++i) {
                if (i > 0) {
                    expanded << ":";
                }
                expanded << std::setw(4) << g[i];
            }
            std::string norm = octi::OctiHttpServer::normalizeIpLiteral(expanded.str());
            RC_ASSERT(!norm.empty());
            RC_ASSERT(octi::OctiHttpServer::normalizeIpLiteral(norm) == norm);
        });
    CHECK_TRUE(v6CanonicalOk);
}

TEST(ServerFoundationTest, RapidCheckRightToLeftXForwardedForInvariants)
{
    const std::vector<std::string> trusted = {
        "127.0.0.1", "::1", "198.51.100.254"
    };

    bool untrustedPeerOk = rc::check(
        "untrusted peer always ignores spoofed X-Forwarded-For and X-Real-IP",
        [&]() {
            int lastOctet = *rc::gen::inRange(1, 250);
            std::string peer = "198.51.100." + std::to_string(lastOctet);
            std::string xff = *rc::gen::arbitrary<std::string>();
            std::string xri = *rc::gen::arbitrary<std::string>();
            RC_ASSERT(octi::OctiHttpServer::resolveClientIp(peer, xff, xri, trusted) == peer);
            RC_ASSERT(octi::OctiHttpServer::resolveClientIp(
                          "::ffff:" + peer, "203.0.113.9, 127.0.0.1", "203.0.113.10", trusted) == peer);
        });
    CHECK_TRUE(untrustedPeerOk);

    bool rightToLeftOk = rc::check(
        "trusted proxy right-to-left scan extracts exact client IP despite spoofed leftmost hops",
        [&]() {
            int cOctet = *rc::gen::inRange(1, 255);
            std::string realClient = "203.0.113." + std::to_string(cOctet);
            bool mapClientV6 = *rc::gen::arbitrary<bool>();
            std::string wireClient = mapClientV6 ? ("::ffff:" + realClient) : realClient;

            int leftCount = *rc::gen::inRange(0, 5);
            std::vector<std::string> chain;
            for (int i = 0; i < leftCount; ++i) {
                if (*rc::gen::arbitrary<bool>()) {
                    chain.push_back("192.0.2." + std::to_string(*rc::gen::inRange(1, 255)));
                } else {
                    chain.push_back("spoofed-garbage-" + std::to_string(i));
                }
            }
            chain.push_back(wireClient);

            int rightTrustedCount = *rc::gen::inRange(0, 4);
            for (int i = 0; i < rightTrustedCount; ++i) {
                int pick = *rc::gen::inRange(0, 3);
                chain.push_back(pick == 0 ? "127.0.0.1"
                              : pick == 1 ? "::1"
                                          : "::ffff:198.51.100.254");
            }

            std::string xff;
            for (size_t i = 0; i < chain.size(); ++i) {
                if (i > 0) {
                    xff += ", ";
                }
                xff += chain[i];
            }
            std::string peer = *rc::gen::element<std::string>(
                "127.0.0.1", "::1", "::ffff:127.0.0.1");
            RC_ASSERT(octi::OctiHttpServer::resolveClientIp(
                          peer, xff, "192.0.2.99", trusted) == realClient);
        });
    CHECK_TRUE(rightToLeftOk);

    bool malformedFailClosedOk = rc::check(
        "malformed rightmost X-Forwarded-For hop fails closed to trusted peer",
        [&]() {
            int badKind = *rc::gen::inRange(0, 4);
            std::string badHop = (badKind == 0) ? ""
                               : (badKind == 1) ? "not_an_ip"
                               : (badKind == 2) ? "203.0.113."
                                                : std::string(140, '9');
            std::string xff = "203.0.113.9, " + badHop;
            RC_ASSERT(octi::OctiHttpServer::resolveClientIp(
                          "127.0.0.1", xff, "203.0.113.77", trusted) == "127.0.0.1");
        });
    CHECK_TRUE(malformedFailClosedOk);
}

TEST(ServerFoundationTest, Envelope5BanConfigCliParsingAndCorruptStartupExit2)
{
    char tmpDirTemplate[] = "/tmp/octi_ban_cli_XXXXXX";
    char *tmpDir = mkdtemp(tmpDirTemplate);
    CHECK_TRUE(tmpDir != nullptr);
    const std::string dirStr(tmpDir);
    const std::string dbPath = dirStr + "/octi.db";
    const std::string corruptPath = dirStr + "/corrupt_bans.json";
    const std::string slashZeroPath = dirStr + "/slash_zero_bans.json";
    const std::string loopbackBanPath = dirStr + "/loopback_bans.json";

    const char *argvOk[] = {
        "octi_server",
        "--db", dbPath.c_str(),
        "--ban-config", corruptPath.c_str(),
        "--port", "0"
    };
    CHECK_TRUE_TEXT(app.parseArgs(7, argvOk, errMsg), errMsg.c_str());
    STRCMP_EQUAL("", errMsg.c_str());
    STRCMP_EQUAL(corruptPath.c_str(), app.config().banConfigPath.c_str());

    std::string helpText = octi::ServerApp::usageBanner("octi_server");
    CHECK_TRUE(helpText.find("--ban-config") != std::string::npos);

    const char *argvMissing[] = { "octi_server", "--ban-config" };
    CHECK_FALSE(app.parseArgs(2, argvMissing, errMsg));
    CHECK_TRUE(errMsg.find("Missing value for --ban-config") != std::string::npos);

    /* Corrupt JSON in explicit --ban-config must fail ServerApp::run with exit code 2 */
    {
        std::ofstream ofs(corruptPath, std::ios::binary | std::ios::trunc);
        ofs << "{corrupt_json_not_closed";
    }
    CHECK_TRUE(app.parseArgs(7, argvOk, errMsg));
    {
        std::ostringstream out;
        std::ostringstream err;
        LONGS_EQUAL(2, app.run(out, err));
        CHECK_FALSE(err.str().empty());
    }

    /* Protected /0 footgun in --ban-config must fail ServerApp::run with exit code 2 */
    {
        std::ofstream ofs(slashZeroPath, std::ios::binary | std::ios::trunc);
        ofs << "{\"version\":1,\"updated_at\":1,\"entries\":["
            << "{\"ban_id\":\"b0\",\"target_type\":\"ip\",\"target_value\":\"0.0.0.0/0\"}"
            << "]}\n";
    }
    const char *argvSlashZero[] = {
        "octi_server",
        "--db", dbPath.c_str(),
        "--ban-config", slashZeroPath.c_str(),
        "--port", "0"
    };
    CHECK_TRUE(app.parseArgs(7, argvSlashZero, errMsg));
    {
        std::ostringstream out;
        std::ostringstream err;
        LONGS_EQUAL(2, app.run(out, err));
        CHECK_FALSE(err.str().empty());
    }

    /* Protected loopback 127.0.0.1 in --ban-config must fail ServerApp::run with exit code 2 */
    {
        std::ofstream ofs(loopbackBanPath, std::ios::binary | std::ios::trunc);
        ofs << "{\"version\":1,\"updated_at\":1,\"entries\":["
            << "{\"ban_id\":\"b1\",\"target_type\":\"ip\",\"target_value\":\"127.0.0.1/32\"}"
            << "]}\n";
    }
    const char *argvLoopback[] = {
        "octi_server",
        "--db", dbPath.c_str(),
        "--ban-config", loopbackBanPath.c_str(),
        "--port", "0"
    };
    CHECK_TRUE(app.parseArgs(7, argvLoopback, errMsg));
    {
        std::ostringstream out;
        std::ostringstream err;
        LONGS_EQUAL(2, app.run(out, err));
        CHECK_FALSE(err.str().empty());
    }

    runCmd("rm -rf '" + dirStr + "'");
}

TEST(ServerFoundationTest, RapidCheckBanManagerCidrAndAntiLockoutInvariants)
{
    octi::BanManager bm;
    bm.configure("", "127.0.0.1", {"127.0.0.1", "::1", "198.51.100.254"});

    bool fuzzSafeAndIdempotent = rc::check(
        "arbitrary strings never crash parseIpOrCidr and canonical output is idempotent",
        [&]() {
            const std::string raw = *rc::gen::arbitrary<std::string>();
            std::string canon;
            int fam = 0;
            std::array<uint8_t, 16> net{};
            int pref = 0;
            std::string err;
            if (octi::BanManager::parseIpOrCidr(raw, canon, fam, net, pref, err)) {
                std::string canon2;
                int fam2 = 0;
                std::array<uint8_t, 16> net2{};
                int pref2 = 0;
                std::string err2;
                RC_ASSERT(octi::BanManager::parseIpOrCidr(
                    canon, canon2, fam2, net2, pref2, err2));
                RC_ASSERT(canon2 == canon);
                RC_ASSERT(fam2 == fam);
                RC_ASSERT(pref2 == pref);
                RC_ASSERT(net2 == net);
            } else {
                RC_ASSERT(err == "INVALID_BAN_TARGET" ||
                          err == "INVALID_CIDR_PREFIX");
            }
            (void)bm.isProtectedIpOrCidr(raw);
        });
    CHECK_TRUE(fuzzSafeAndIdempotent);

    bool ipv4BoundsAndMappedOk = rc::check(
        "IPv4 /0..33 prefix bounds, host masking, and ::ffff: mapped matching hold",
        [&]() {
            const int a = *rc::gen::inRange(192, 204);
            const int b = *rc::gen::inRange(0, 256);
            const int c = *rc::gen::inRange(0, 256);
            const int d = *rc::gen::inRange(0, 256);
            const int pref = *rc::gen::inRange(0, 34);
            const std::string ipStr =
                std::to_string(a) + "." + std::to_string(b) + "." +
                std::to_string(c) + "." + std::to_string(d);
            const std::string spec = ipStr + "/" + std::to_string(pref);

            std::string canon;
            int fam = 0;
            std::array<uint8_t, 16> net{};
            int outPref = 0;
            std::string err;
            const bool ok = octi::BanManager::parseIpOrCidr(
                spec, canon, fam, net, outPref, err);
            if (pref < 8 || pref > 32) {
                RC_ASSERT(!ok);
                RC_ASSERT(err == "INVALID_CIDR_PREFIX");
            } else {
                RC_ASSERT(ok);
                RC_ASSERT(outPref == pref);
                RC_ASSERT(octi::BanManager::cidrContainsIp(fam, net, outPref, ipStr));
                RC_ASSERT(octi::BanManager::cidrContainsIp(
                    fam, net, outPref, "::ffff:" + ipStr));

                /* Flip the highest network bit (in byte 0..3 within prefix) -> must NOT match */
                const int flipBit = pref - 1;
                std::array<uint8_t, 4> outside = {
                    static_cast<uint8_t>(a), static_cast<uint8_t>(b),
                    static_cast<uint8_t>(c), static_cast<uint8_t>(d)};
                outside[static_cast<size_t>(flipBit / 8)] ^=
                    static_cast<uint8_t>(1u << (7 - (flipBit % 8)));
                const std::string outIp =
                    std::to_string(outside[0]) + "." +
                    std::to_string(outside[1]) + "." +
                    std::to_string(outside[2]) + "." +
                    std::to_string(outside[3]);
                RC_ASSERT(!octi::BanManager::cidrContainsIp(fam, net, outPref, outIp));
            }
        });
    CHECK_TRUE(ipv4BoundsAndMappedOk);

    bool ipv6BoundsOk = rc::check(
        "IPv6 /0..129 prefix bounds and bit-level containment hold",
        [&]() {
            const uint16_t h1 = static_cast<uint16_t>(*rc::gen::inRange(0, 65536));
            const uint16_t h2 = static_cast<uint16_t>(*rc::gen::inRange(0, 65536));
            const uint16_t h3 = static_cast<uint16_t>(*rc::gen::inRange(0, 65536));
            const int pref = *rc::gen::inRange(0, 130);
            std::ostringstream oss;
            oss << "2001:db8:" << std::hex << h1 << ":" << h2 << "::" << h3;
            const std::string ip6 = oss.str();
            const std::string spec = ip6 + "/" + std::to_string(pref);

            std::string canon;
            int fam = 0;
            std::array<uint8_t, 16> net{};
            int outPref = 0;
            std::string err;
            const bool ok = octi::BanManager::parseIpOrCidr(
                spec, canon, fam, net, outPref, err);
            if (pref < 16 || pref > 128) {
                RC_ASSERT(!ok);
                RC_ASSERT(err == "INVALID_CIDR_PREFIX");
            } else {
                RC_ASSERT(ok);
                RC_ASSERT(outPref == pref);
                RC_ASSERT(octi::BanManager::cidrContainsIp(fam, net, outPref, ip6));
            }
        });
    CHECK_TRUE(ipv6BoundsOk);

    bool antiLockoutOverlapOk = rc::check(
        "every loopback subnet and any CIDR covering a trusted proxy is protected",
        [&]() {
            const int b = *rc::gen::inRange(0, 256);
            const int c = *rc::gen::inRange(0, 256);
            const int d = *rc::gen::inRange(1, 255);
            const int pref = *rc::gen::inRange(8, 33);
            const std::string loSpec =
                "127." + std::to_string(b) + "." + std::to_string(c) + "." +
                std::to_string(d) + "/" + std::to_string(pref);
            RC_ASSERT(bm.isProtectedIpOrCidr(loSpec));

            /* Any /8../32 subnet covering trusted proxy 198.51.100.254 must also be protected */
            const std::string proxyCover =
                "198.51.100.254/" + std::to_string(pref);
            RC_ASSERT(bm.isProtectedIpOrCidr(proxyCover));
        });
    CHECK_TRUE(antiLockoutOverlapOk);
}

int main(int argc, char **argv)
{
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
