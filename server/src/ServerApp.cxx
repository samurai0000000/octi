/*
 * ServerApp.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "ServerApp.hxx"
#include "OctiCore.hxx"
#include "Version.hxx"
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <sstream>

namespace octi {

ServerApp::ServerApp()
    : _config()
{
}

ServerApp::~ServerApp()
{
}

bool ServerApp::parseArgs(int argc, const char * const argv[], std::string &errMsg)
{
    errMsg.clear();
    _config = ServerConfig();

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (arg == nullptr) {
            errMsg = "Null argument encountered";
            return false;
        }
        if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
            _config.showHelp = true;
        } else if (std::strcmp(arg, "--version") == 0 || std::strcmp(arg, "-v") == 0) {
            _config.showVersion = true;
        } else if (std::strcmp(arg, "--port") == 0 || std::strcmp(arg, "-p") == 0) {
            if (i + 1 >= argc || argv[i + 1] == nullptr) {
                errMsg = "Missing value for --port";
                return false;
            }
            const char *val = argv[++i];
            if (val[0] == '\0' || val[0] == '-') {
                errMsg = std::string("Invalid port value: ") + val;
                return false;
            }
            char *endPtr = nullptr;
            unsigned long parsed = std::strtoul(val, &endPtr, 10);
            if (endPtr == val || *endPtr != '\0' || parsed < 1UL || parsed > 65535UL) {
                errMsg = std::string("Invalid port value: ") + val;
                return false;
            }
            _config.port = static_cast<uint16_t>(parsed);
        } else {
            errMsg = std::string("Unknown option: ") + arg;
            return false;
        }
    }

    return true;
}

int ServerApp::run(std::ostream &out, std::ostream &err)
{
    (void)err;
    if (_config.showVersion) {
        out << versionBanner();
        return 0;
    }
    if (_config.showHelp) {
        out << usageBanner("octi_server");
        return 0;
    }

    out << "octi_server v" << OctiCore::versionString()
        << " ready (port " << _config.port << ")\n";
    return 0;
}

const ServerConfig &ServerApp::config() const
{
    return _config;
}

std::string ServerApp::versionBanner()
{
    return OctiCore::buildBanner("octi_server");
}

std::string ServerApp::usageBanner(const char *progName)
{
    const char *name = (progName != nullptr && progName[0] != '\0')
        ? progName : "octi_server";
    std::ostringstream oss;
    oss << "Usage: " << name << " [options]\n"
        << "Options:\n"
        << "  -p, --port <port>  Listen port (1..65535, default: 8888)\n"
        << "  -v, --version      Print version information and exit\n"
        << "  -h, --help         Print this help message and exit\n";
    return oss.str();
}

} // namespace octi

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
