/*
 * ClientConfigStore.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_CLIENT_CONFIG_STORE_HXX
#define OCTI_DESKTOP_CLIENT_CONFIG_STORE_HXX

#include <cstddef>
#include <string>
#include "OctiDefs.hxx"
#include "OctiTheme.hxx"

namespace octi {

struct ClientPreferences {
    std::string sessionToken;
    std::string playerUuid;
    std::string handle;
    std::string displayName;
    std::string avatarUrl;
    std::string preferredProvider;
    ThemeId theme = ThemeId::OBSIDIAN_BRASS;
    bool confirmAllMoves = true;
    bool sfxMuted = false;
    int pttKey = 'V';
};

/*
 * Persists local session credentials and tabletop UI preferences with strict
 * 0700 directory / 0600 file permissions on POSIX.
 *
 * Hard-coded endpoint invariant: the server address is permanently compiled as
 * OCTI_SERVER_HOST ("trumpet.selfso.com") and OCTI_SERVER_PORT (6874). Any
 * server_url, server_host, or server_port fields injected into the JSON file
 * are ignored on load and never written on save.
 */
class ClientConfigStore {
public:
    static constexpr std::size_t MAX_CONFIG_FILE_BYTES = 65536;

    explicit ClientConfigStore(
        const std::string &configFilePath = defaultConfigFilePath());
    ~ClientConfigStore();

    static std::string defaultConfigFilePath();

    const std::string &configFilePath() const;

    bool load(std::string &errorOut);
    bool save(std::string &errorOut) const;

    void clearSession();

    const ClientPreferences &preferences() const;
    ClientPreferences &preferences();

    static constexpr const char *serverHost() { return OCTI_SERVER_HOST; }
    static constexpr int serverPort() { return OCTI_SERVER_PORT; }

private:
    std::string _configFilePath;
    ClientPreferences _prefs;
};

} // namespace octi

#endif // OCTI_DESKTOP_CLIENT_CONFIG_STORE_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
