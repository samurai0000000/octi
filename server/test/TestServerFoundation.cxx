/*
 * TestServerFoundation.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "ServerApp.hxx"
#include "OctiCore.hxx"
#include "Version.hxx"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
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
    CHECK_TRUE(svcContent.find("--avatar-dir ${OCTI_AVATAR_DIR}") != std::string::npos);
    CHECK_TRUE(svcContent.find("ReadWritePaths=/var/lib/octi /var/octi/avatar") != std::string::npos);
    CHECK_TRUE(svcContent.find("User=octi") != std::string::npos);
    CHECK_TRUE(svcContent.find("Group=octi") != std::string::npos);
    CHECK_TRUE(svcContent.find("KillSignal=SIGTERM") != std::string::npos);
    CHECK_TRUE(svcContent.find("EnvironmentFile=-/etc/default/octi-server") != std::string::npos);

    std::string defaultContent = readFile(tmpDirStr + "/root/etc/default/octi-server");
    CHECK_TRUE(defaultContent.find("OCTI_PORT=6874") != std::string::npos);
    CHECK_TRUE(defaultContent.find("OCTI_DB_PATH=/var/lib/octi/octi.db") != std::string::npos);
    CHECK_TRUE(defaultContent.find("OCTI_AVATAR_DIR=/var/octi/avatar") != std::string::npos);

    std::string postinstContent = readFile(tmpDirStr + "/DEBIAN/postinst");
    CHECK_TRUE(postinstContent.find("/var/octi/avatar") != std::string::npos);

    std::string oidcPath = tmpDirStr + "/root/etc/octi/oidc.json";
    octi::OidcConfig parsedConfig;
    std::string oidcErr;
    CHECK_TRUE(octi::ServerApp::loadOidcConfig(oidcPath, parsedConfig, oidcErr));
    STRCMP_EQUAL("", oidcErr.c_str());

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

int main(int argc, char **argv)
{
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
