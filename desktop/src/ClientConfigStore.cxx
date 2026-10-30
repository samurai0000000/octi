/*
 * ClientConfigStore.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "ClientConfigStore.hxx"

#include <nlohmann/json.hpp>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace octi {

namespace {

typedef nlohmann::json Json;

const char *themeToString(ThemeId theme)
{
    switch (theme) {
    case ThemeId::ROSEWOOD_IVORY:
        return "rosewood_ivory";
    case ThemeId::OBSIDIAN_BRASS:
    default:
        return "obsidian_brass";
    }
}

ThemeId parseTheme(const Json &val)
{
    if (val.is_string()) {
        std::string s = val.get<std::string>();
        if (s == "rosewood_ivory" || s == "ROSEWOOD_IVORY" || s == "1") {
            return ThemeId::ROSEWOOD_IVORY;
        }
        return ThemeId::OBSIDIAN_BRASS;
    }
    if (val.is_number_integer()) {
        int v = val.get<int>();
        if (v == 1) {
            return ThemeId::ROSEWOOD_IVORY;
        }
    }
    return ThemeId::OBSIDIAN_BRASS;
}

#ifndef _WIN32
bool ensureDirectory0700(const std::filesystem::path &dir, std::string &errorOut)
{
    if (dir.empty()) {
        return true;
    }
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        errorOut = "failed to create config directory: " + ec.message();
        return false;
    }
    struct stat st;
    if (::stat(dir.c_str(), &st) != 0) {
        errorOut = std::string("stat failed on config directory: ") +
                   std::strerror(errno);
        return false;
    }
    if (!S_ISDIR(st.st_mode)) {
        errorOut = "config parent path is not a directory";
        return false;
    }
    if (st.st_uid == ::geteuid() && (st.st_mode & S_ISVTX) == 0) {
        if ((st.st_mode & 0777) != 0700) {
            if (::chmod(dir.c_str(), 0700) != 0) {
                errorOut = std::string("chmod 0700 failed on config directory: ") +
                           std::strerror(errno);
                return false;
            }
        }
    }
    return true;
}
#endif

} // namespace

ClientConfigStore::ClientConfigStore(const std::string &configFilePath)
    : _configFilePath(configFilePath.empty() ? defaultConfigFilePath()
                                             : configFilePath),
      _prefs()
{
}

ClientConfigStore::~ClientConfigStore() = default;

std::string ClientConfigStore::defaultConfigFilePath()
{
#ifdef _WIN32
    const char *appData = std::getenv("APPDATA");
    if (appData != nullptr && appData[0] != '\0') {
        std::filesystem::path p(appData);
        p /= "OCTI";
        p /= "client_config.json";
        return p.string();
    }
    const char *userProfile = std::getenv("USERPROFILE");
    if (userProfile != nullptr && userProfile[0] != '\0') {
        std::filesystem::path p(userProfile);
        p /= ".config";
        p /= "octi";
        p /= "client_config.json";
        return p.string();
    }
    return "octi_client_config.json";
#else
    const char *xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg != nullptr && xdg[0] != '\0') {
        std::filesystem::path p(xdg);
        p /= "octi";
        p /= "client_config.json";
        return p.string();
    }
    const char *home = std::getenv("HOME");
    if (home != nullptr && home[0] != '\0') {
        std::filesystem::path p(home);
        p /= ".config";
        p /= "octi";
        p /= "client_config.json";
        return p.string();
    }
    return "octi_client_config.json";
#endif
}

const std::string &ClientConfigStore::configFilePath() const
{
    return _configFilePath;
}

bool ClientConfigStore::load(std::string &errorOut)
{
    errorOut.clear();
    if (_configFilePath.empty()) {
        errorOut = "empty config file path";
        return false;
    }

#ifndef _WIN32
    struct stat st;
    if (::stat(_configFilePath.c_str(), &st) != 0) {
        errorOut = std::string("cannot stat config file: ") +
                   std::strerror(errno);
        return false;
    }
    if (!S_ISREG(st.st_mode)) {
        errorOut = "config path is not a regular file";
        return false;
    }
    if (static_cast<std::size_t>(st.st_size) > MAX_CONFIG_FILE_BYTES) {
        errorOut = "config file exceeds maximum allowed size (64 KiB)";
        return false;
    }
    if ((st.st_mode & 0177) != 0 && st.st_uid == ::geteuid()) {
        if (::chmod(_configFilePath.c_str(), 0600) != 0) {
            errorOut = std::string("failed to repair 0600 permissions: ") +
                       std::strerror(errno);
            return false;
        }
    }
#else
    std::error_code ec;
    if (!std::filesystem::is_regular_file(_configFilePath, ec)) {
        errorOut = "config file does not exist";
        return false;
    }
    auto fileSize = std::filesystem::file_size(_configFilePath, ec);
    if (ec || fileSize > MAX_CONFIG_FILE_BYTES) {
        errorOut = "config file exceeds maximum allowed size (64 KiB)";
        return false;
    }
#endif

    std::ifstream ifs(_configFilePath, std::ios::in | std::ios::binary);
    if (!ifs.is_open()) {
        errorOut = "unable to open config file for reading";
        return false;
    }

    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string raw = oss.str();
    if (raw.size() > MAX_CONFIG_FILE_BYTES) {
        errorOut = "config file exceeds maximum allowed size (64 KiB)";
        return false;
    }
    if (raw.empty()) {
        errorOut = "config file is empty";
        return false;
    }

    Json doc = Json::parse(raw, nullptr, false);
    if (doc.is_discarded() || !doc.is_object()) {
        errorOut = "config file does not contain a valid JSON object";
        return false;
    }

    /*
     * Explicitly ignore any server_url, server_host, server_port, host, or
     * port keys that may have been manually injected into the JSON file.
     */
    ClientPreferences parsed;
    if (doc.contains("session_token") && doc["session_token"].is_string()) {
        parsed.sessionToken = doc["session_token"].get<std::string>();
    }
    if (doc.contains("player_uuid") && doc["player_uuid"].is_string()) {
        parsed.playerUuid = doc["player_uuid"].get<std::string>();
    }
    if (doc.contains("handle") && doc["handle"].is_string()) {
        parsed.handle = doc["handle"].get<std::string>();
    }
    if (doc.contains("display_name") && doc["display_name"].is_string()) {
        parsed.displayName = doc["display_name"].get<std::string>();
    }
    if (doc.contains("avatar_url") && doc["avatar_url"].is_string()) {
        parsed.avatarUrl = doc["avatar_url"].get<std::string>();
    }
    if (doc.contains("preferred_provider") &&
        doc["preferred_provider"].is_string()) {
        parsed.preferredProvider = doc["preferred_provider"].get<std::string>();
    }
    if (doc.contains("theme")) {
        parsed.theme = parseTheme(doc["theme"]);
    }
    if (doc.contains("confirm_all_moves") &&
        doc["confirm_all_moves"].is_boolean()) {
        parsed.confirmAllMoves = doc["confirm_all_moves"].get<bool>();
    }
    if (doc.contains("sfx_muted") && doc["sfx_muted"].is_boolean()) {
        parsed.sfxMuted = doc["sfx_muted"].get<bool>();
    }
    if (doc.contains("ptt_keycode") && doc["ptt_keycode"].is_number_integer()) {
        int key = doc["ptt_keycode"].get<int>();
        if (key >= 32 && key <= 126) {
            parsed.pttKey = key;
        }
    } else if (doc.contains("ptt_key") && doc["ptt_key"].is_number_integer()) {
        int key = doc["ptt_key"].get<int>();
        if (key >= 32 && key <= 126) {
            parsed.pttKey = key;
        }
    }
    if (doc.contains("telemetry_consent") && doc["telemetry_consent"].is_string()) {
        parsed.telemetryConsent = doc["telemetry_consent"].get<std::string>();
    }
    if (doc.contains("telemetry_consent_version") &&
        doc["telemetry_consent_version"].is_number_integer()) {
        parsed.telemetryConsentVersion = doc["telemetry_consent_version"].get<int>();
    }
    if (doc.contains("install_id") && doc["install_id"].is_string()) {
        parsed.installId = doc["install_id"].get<std::string>();
    }

    _prefs = std::move(parsed);
    return true;
}

bool ClientConfigStore::save(std::string &errorOut) const
{
    errorOut.clear();
    if (_configFilePath.empty()) {
        errorOut = "empty config file path";
        return false;
    }

    std::filesystem::path targetPath(_configFilePath);
    std::filesystem::path parentDir = targetPath.parent_path();

#ifndef _WIN32
    if (!ensureDirectory0700(parentDir, errorOut)) {
        return false;
    }
#else
    if (!parentDir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parentDir, ec);
        if (ec) {
            errorOut = "failed to create config directory: " + ec.message();
            return false;
        }
    }
#endif

    /*
     * Never write server_url, server_host, or server_port to disk.
     */
    Json doc;
    doc["session_token"] = _prefs.sessionToken;
    doc["player_uuid"] = _prefs.playerUuid;
    doc["handle"] = _prefs.handle;
    doc["display_name"] = _prefs.displayName;
    doc["avatar_url"] = _prefs.avatarUrl;
    doc["preferred_provider"] = _prefs.preferredProvider;
    doc["theme"] = themeToString(_prefs.theme);
    doc["confirm_all_moves"] = _prefs.confirmAllMoves;
    doc["sfx_muted"] = _prefs.sfxMuted;
    doc["ptt_keycode"] = _prefs.pttKey;
    doc["telemetry_consent"] = _prefs.telemetryConsent;
    doc["telemetry_consent_version"] = _prefs.telemetryConsentVersion;
    doc["install_id"] = _prefs.installId;

    std::string serialized = doc.dump(2) + "\n";

#ifndef _WIN32
    std::string tmpPath = _configFilePath + ".tmp";
    int fd = ::open(tmpPath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0600);
    if (fd < 0) {
        errorOut = std::string("open failed for config temp file: ") +
                   std::strerror(errno);
        return false;
    }
    if (::fchmod(fd, 0600) != 0) {
        int savedErr = errno;
        ::close(fd);
        ::unlink(tmpPath.c_str());
        errorOut = std::string("fchmod 0600 failed: ") +
                   std::strerror(savedErr);
        return false;
    }

    const char *ptr = serialized.data();
    std::size_t remaining = serialized.size();
    while (remaining > 0) {
        ssize_t n = ::write(fd, ptr, remaining);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            int savedErr = errno;
            ::close(fd);
            ::unlink(tmpPath.c_str());
            errorOut = std::string("write failed: ") + std::strerror(savedErr);
            return false;
        }
        ptr += n;
        remaining -= static_cast<std::size_t>(n);
    }

    (void)::fsync(fd);
    if (::close(fd) != 0) {
        int savedErr = errno;
        ::unlink(tmpPath.c_str());
        errorOut = std::string("close failed: ") + std::strerror(savedErr);
        return false;
    }

    if (::rename(tmpPath.c_str(), _configFilePath.c_str()) != 0) {
        int savedErr = errno;
        ::unlink(tmpPath.c_str());
        errorOut = std::string("rename failed: ") + std::strerror(savedErr);
        return false;
    }

    if (::chmod(_configFilePath.c_str(), 0600) != 0) {
        errorOut = std::string("chmod 0600 failed: ") + std::strerror(errno);
        return false;
    }
    return true;
#else
    std::ofstream ofs(_configFilePath,
                      std::ios::out | std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        errorOut = "unable to open config file for writing";
        return false;
    }
    ofs.write(serialized.data(),
              static_cast<std::streamsize>(serialized.size()));
    if (!ofs.good()) {
        errorOut = "failed to write config file";
        return false;
    }
    ofs.close();
    return true;
#endif
}

void ClientConfigStore::clearSession()
{
    _prefs.sessionToken.clear();
    _prefs.playerUuid.clear();
}

const ClientPreferences &ClientConfigStore::preferences() const
{
    return _prefs;
}

ClientPreferences &ClientConfigStore::preferences()
{
    return _prefs;
}

std::string ClientConfigStore::generateUuidV4()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<std::uint64_t> dist;
    std::uint64_t d1 = dist(gen);
    std::uint64_t d2 = dist(gen);

    // Set version 4 (bits 12-15 of time_hi_and_version to 0100)
    d1 = (d1 & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    // Set variant (bits 6-7 of clock_seq_hi_and_reserved to 10)
    d2 = (d2 & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    char buf[37];
    std::snprintf(buf, sizeof(buf),
                  "%08x-%04x-%04x-%02x%02x-%012llx",
                  static_cast<unsigned int>(d1 >> 32),
                  static_cast<unsigned int>((d1 >> 16) & 0xFFFF),
                  static_cast<unsigned int>(d1 & 0xFFFF),
                  static_cast<unsigned int>((d2 >> 56) & 0xFF),
                  static_cast<unsigned int>((d2 >> 48) & 0xFF),
                  static_cast<unsigned long long>(d2 & 0xFFFFFFFFFFFFULL));
    return std::string(buf);
}

void ClientConfigStore::resetInstallId()
{
    _prefs.installId = generateUuidV4();
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
