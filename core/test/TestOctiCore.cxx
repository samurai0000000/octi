/*
 * TestOctiCore.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiCore.hxx"
#include "Version.hxx"
#include <string>
#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness.h>

TEST_GROUP(OctiCoreTest)
{
};

TEST(OctiCoreTest, VersionAndBuildBanner)
{
    octi::OctiCore core;
     (void)core;
    STRCMP_EQUAL(OCTI_VERSION_STRING, octi::OctiCore::versionString());

    std::string banner = octi::OctiCore::buildBanner("octi_core");
    CHECK_TRUE(banner.find("octi_core version ") != std::string::npos);
    CHECK_TRUE(banner.find(OCTI_VERSION_STRING) != std::string::npos);

    std::string fallbackBanner = octi::OctiCore::buildBanner(nullptr);
    CHECK_TRUE(fallbackBanner.find("octi version ") != std::string::npos);
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
