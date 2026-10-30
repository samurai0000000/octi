/*
 * ServerApp.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_SERVER_SERVER_APP_HXX
#define OCTI_SERVER_SERVER_APP_HXX

#include "OctiDefs.hxx"

#include <atomic>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

namespace octi {

/*
 * Per-provider OIDC settings read from the --oidc-config JSON file:
 *
 *   {
 *     "providers": {
 *       "google":   { "client_ids": ["<client id>", ...] },
 *       "facebook": { "client_id": "<app id>", "app_secret": "<secret>" },
 *       "line":     { "client_ids": ["<channel id>"] }
 *     }
 *   }
 *
 * "client_id" (one string) and "client_ids" (array) are both accepted.
 * "app_secret" is only meaningful for facebook (Graph access tokens); the
 * environment variable OCTI_FACEBOOK_APP_SECRET overrides it so the secret
 * need not live in a file at all.
 */
struct OidcProviderConfig {
    std::vector<std::string> clientIds;
    std::string appSecret;
};

typedef std::map<std::string, OidcProviderConfig> OidcConfig;

struct ServerConfig {
    uint16_t port = static_cast<uint16_t>(OCTI_SERVER_DAEMON_PORT); // 0 = ephemeral, reported at start
    std::string bindAddress = "127.0.0.1";
    std::string dbPath;                 // required to serve (--db)
    std::string oidcConfigPath;         // optional (--oidc-config)
    bool showHelp = false;
    bool showVersion = false;
};

class ServerApp {
public:
    ServerApp();
    ~ServerApp();

    bool parseArgs(int argc, const char * const argv[], std::string &errMsg);

    /*
     * Serve until SIGINT / SIGTERM (or requestStop()). Returns 0 after a
     * clean shutdown, 2 when the server could not start. Version and help
     * requests print and return 0 without starting anything.
     */
    int run(std::ostream &out, std::ostream &err);

    /* Thread-safe: ask a running run() to shut down cleanly. */
    void requestStop();

    const ServerConfig &config() const;
    static std::string versionBanner();
    static std::string usageBanner(const char *progName);

    /*
     * Parse the OIDC configuration JSON. When the document holds an
     * app_secret the file at path must be a regular file owned by the
     * current user with no group/other permission bits (0600), inside a
     * directory that is not group/other writable; otherwise it is
     * rejected. path may be empty to apply only the environment override.
     */
    static bool loadOidcConfig(const std::string &path, OidcConfig &out,
                               std::string &error);

private:
    ServerConfig _config;
    std::atomic<bool> _stopRequested;
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
