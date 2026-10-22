/*
 * TestServerFoundation.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "ServerApp.hxx"
#include "OctiCore.hxx"
#include "Version.hxx"
#include <sstream>
#include <string>
#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness.h>

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
                           "/tmp/octi_foundation_oidc.json" };
    CHECK_TRUE(app.parseArgs(9, argv, errMsg));
    LONGS_EQUAL(9100, app.config().port);
    STRCMP_EQUAL("0.0.0.0", app.config().bindAddress.c_str());
    STRCMP_EQUAL("/tmp/octi_foundation.db", app.config().dbPath.c_str());
    STRCMP_EQUAL("/tmp/octi_foundation_oidc.json",
                 app.config().oidcConfigPath.c_str());

    /* Parsing again starts from the defaults. */
    const char *again[] = { "octi_server", "--db", "x.db" };
    CHECK_TRUE(app.parseArgs(3, again, errMsg));
    LONGS_EQUAL(6874, app.config().port);
    STRCMP_EQUAL("127.0.0.1", app.config().bindAddress.c_str());
    STRCMP_EQUAL("", app.config().oidcConfigPath.c_str());
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
