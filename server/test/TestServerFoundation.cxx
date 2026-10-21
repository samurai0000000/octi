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
    LONGS_EQUAL(8888, app.config().port);
    CHECK_FALSE(app.config().showHelp);
    CHECK_FALSE(app.config().showVersion);

    std::ostringstream out;
    std::ostringstream err;
    LONGS_EQUAL(0, app.run(out, err));
    CHECK_TRUE(out.str().find("octi_server v") != std::string::npos);
    CHECK_TRUE(out.str().find("8888") != std::string::npos);
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
}

TEST(ServerFoundationTest, BoundaryPortValues)
{
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

    const char *argvZero[] = { "octi_server", "--port", "0" };
    CHECK_FALSE(app.parseArgs(3, argvZero, errMsg));

    const char *argvOverflow[] = { "octi_server", "--port", "65536" };
    CHECK_FALSE(app.parseArgs(3, argvOverflow, errMsg));

    const char *argvNeg[] = { "octi_server", "--port", "-1" };
    CHECK_FALSE(app.parseArgs(3, argvNeg, errMsg));

    const char *argvAlpha[] = { "octi_server", "--port", "8080abc" };
    CHECK_FALSE(app.parseArgs(3, argvAlpha, errMsg));
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
