/*
 * ServerApp.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_SERVER_SERVER_APP_HXX
#define OCTI_SERVER_SERVER_APP_HXX

#include <cstdint>
#include <iosfwd>
#include <string>

namespace octi {

struct ServerConfig {
    uint16_t port = 8888;
    bool showHelp = false;
    bool showVersion = false;
};

class ServerApp {
public:
    ServerApp();
    ~ServerApp();

    bool parseArgs(int argc, const char * const argv[], std::string &errMsg);
    int run(std::ostream &out, std::ostream &err);

    const ServerConfig &config() const;
    static std::string versionBanner();
    static std::string usageBanner(const char *progName);

private:
    ServerConfig _config;
};

} // namespace octi

#endif // OCTI_SERVER_SERVER_APP_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
