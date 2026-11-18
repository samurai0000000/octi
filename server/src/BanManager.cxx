/*
 * BanManager.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "BanManager.hxx"

#include <nlohmann/json.hpp>
#include <openssl/rand.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <unordered_set>

namespace octi {

namespace {

typedef nlohmann::json Json;

std::string trimAscii(const std::string &s)
{
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return std::string();
    }
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

std::string lowerAscii(const std::string &s)
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(out[i])));
    }
    return out;
}

void applyPrefixMask(std::array<uint8_t, 16> &bytes, size_t totalBytes,
                     int prefixLen)
{
    const size_t fullBytes = static_cast<size_t>(prefixLen / 8);
    const int remBits = prefixLen % 8;
    if (remBits > 0 && fullBytes < totalBytes) {
        const uint8_t mask = static_cast<uint8_t>(0xFFu << (8 - remBits));
        bytes[fullBytes] &= mask;
        for (size_t i = fullBytes + 1; i < totalBytes; ++i) {
            bytes[i] = 0;
        }
    } else {
        for (size_t i = fullBytes; i < totalBytes; ++i) {
            bytes[i] = 0;
        }
    }
    for (size_t i = totalBytes; i < bytes.size(); ++i) {
        bytes[i] = 0;
    }
}

bool parseSingleIpBytes(const std::string &rawIp, int &outFamily,
                        std::array<uint8_t, 16> &outBytes,
                        std::string *outNormalized = nullptr)
{
    outBytes.fill(0);
    std::string s = trimAscii(rawIp);
    if (s.empty() || s.size() > 128 || s.find('/') != std::string::npos ||
        s.find('%') != std::string::npos) {
        return false;
    }
    if (s.front() == '[' && s.back() == ']' && s.size() > 2) {
        s = s.substr(1, s.size() - 2);
    }
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 0x20 || c >= 0x7F) {
            return false;
        }
    }

    struct in_addr a4;
    if (::inet_pton(AF_INET, s.c_str(), &a4) == 1) {
        outFamily = AF_INET;
        std::memcpy(outBytes.data(), &a4.s_addr, 4);
        if (outNormalized != nullptr) {
            char buf[INET_ADDRSTRLEN];
            if (::inet_ntop(AF_INET, outBytes.data(), buf, sizeof(buf)) != nullptr) {
                *outNormalized = buf;
            } else {
                *outNormalized = s;
            }
        }
        return true;
    }

    struct in6_addr a6;
    if (::inet_pton(AF_INET6, s.c_str(), &a6) == 1) {
        bool v4Mapped = true;
        for (int i = 0; i < 10; ++i) {
            if (a6.s6_addr[i] != 0) {
                v4Mapped = false;
                break;
            }
        }
        if (v4Mapped && a6.s6_addr[10] == 0xFF && a6.s6_addr[11] == 0xFF) {
            outFamily = AF_INET;
            std::memcpy(outBytes.data(), a6.s6_addr + 12, 4);
            if (outNormalized != nullptr) {
                char buf4[INET_ADDRSTRLEN];
                if (::inet_ntop(AF_INET, outBytes.data(), buf4, sizeof(buf4)) != nullptr) {
                    *outNormalized = buf4;
                } else {
                    *outNormalized = s;
                }
            }
            return true;
        }
        outFamily = AF_INET6;
        std::memcpy(outBytes.data(), a6.s6_addr, 16);
        if (outNormalized != nullptr) {
            char buf6[INET6_ADDRSTRLEN];
            if (::inet_ntop(AF_INET6, outBytes.data(), buf6, sizeof(buf6)) != nullptr) {
                *outNormalized = lowerAscii(buf6);
            } else {
                *outNormalized = lowerAscii(s);
            }
        }
        return true;
    }
    return false;
}

bool bitsMatchPrefix(const std::array<uint8_t, 16> &a,
                     const std::array<uint8_t, 16> &b,
                     int prefixBits)
{
    if (prefixBits <= 0) {
        return true;
    }
    const size_t fullBytes = static_cast<size_t>(prefixBits / 8);
    const int remBits = prefixBits % 8;
    if (fullBytes > 0 &&
        std::memcmp(a.data(), b.data(), fullBytes) != 0) {
        return false;
    }
    if (remBits > 0) {
        const uint8_t mask = static_cast<uint8_t>(0xFFu << (8 - remBits));
        if ((a[fullBytes] & mask) != (b[fullBytes] & mask)) {
            return false;
        }
    }
    return true;
}

bool looksLikeUuid(const std::string &s)
{
    if (s.size() < 8 || s.size() > 64) {
        return false;
    }
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (!(std::isalnum(c) || c == '-' || c == '_')) {
            return false;
        }
    }
    return true;
}

} // namespace

std::atomic<uint64_t> BanManager::_tmpFileSeq{0};

const char *toString(BanTargetType t)
{
    switch (t) {
    case BanTargetType::IP: return "ip";
    case BanTargetType::ACCOUNT: return "account";
    case BanTargetType::ROBOT: return "robot";
    }
    return "ip";
}

bool parseBanTargetType(const std::string &s, BanTargetType &out)
{
    const std::string l = lowerAscii(trimAscii(s));
    if (l == "ip" || l == "cidr") {
        out = BanTargetType::IP;
        return true;
    }
    if (l == "account" || l == "user" || l == "player") {
        out = BanTargetType::ACCOUNT;
        return true;
    }
    if (l == "robot" || l == "bot") {
        out = BanTargetType::ROBOT;
        return true;
    }
    return false;
}

const char *toString(BanDuration d)
{
    switch (d) {
    case BanDuration::TEMP_24H: return "24h";
    case BanDuration::PERMANENT: return "permanent";
    }
    return "24h";
}

bool parseBanDuration(const std::string &s, BanDuration &out)
{
    const std::string l = lowerAscii(trimAscii(s));
    if (l == "24h" || l == "temp" || l == "temp_24h" || l == "temporary") {
        out = BanDuration::TEMP_24H;
        return true;
    }
    if (l == "permanent" || l == "perm") {
        out = BanDuration::PERMANENT;
        return true;
    }
    return false;
}

const char *toString(AbusePattern p)
{
    switch (p) {
    case AbusePattern::RATE_LIMIT_429:
        return "RATE_LIMIT_429";
    case AbusePattern::AUTH_OR_PASSCODE_BRUTE_FORCE:
        return "AUTH_OR_PASSCODE_BRUTE_FORCE";
    case AbusePattern::CRAWLER_SCRAPE:
        return "CRAWLER_SCRAPE";
    case AbusePattern::EXPLOIT_PROBE:
        return "EXPLOIT_PROBE";
    }
    return "RATE_LIMIT_429";
}

BanManager::BanManager()
{
    configure(std::string(), "127.0.0.1", {"127.0.0.1", "::1"});
}

BanManager::~BanManager()
{
}

void BanManager::configure(const std::string &banConfigPath,
                           const std::string &bindAddress,
                           const std::vector<std::string> &trustedProxies,
                           const BanThresholds &thresholds)
{
    std::vector<ProtectedCidr> built;

    auto addProtected = [&](const std::string &spec) {
        std::string canon;
        int fam = 0;
        std::array<uint8_t, 16> net{};
        int pref = 0;
        std::string err;
        if (parseIpOrCidr(spec, canon, fam, net, pref, err)) {
            ProtectedCidr pc;
            pc.family = fam;
            pc.networkBytes = net;
            pc.prefixLen = pref;
            built.push_back(pc);
        }
    };

    /* Mandatory loopback immunity: 127.0.0.0/8 and ::1/128 */
    addProtected("127.0.0.0/8");
    addProtected("::1/128");

    /* Concrete bindAddress immunity (skipping wildcard 0.0.0.0 / ::) */
    const std::string trimmedBind = trimAscii(bindAddress);
    if (!trimmedBind.empty() && trimmedBind != "0.0.0.0" &&
        trimmedBind != "::" && lowerAscii(trimmedBind) != "localhost") {
        int fam = 0;
        std::array<uint8_t, 16> net{};
        if (parseSingleIpBytes(trimmedBind, fam, net, nullptr)) {
            ProtectedCidr pc;
            pc.family = fam;
            pc.networkBytes = net;
            pc.prefixLen = (fam == AF_INET) ? 32 : 128;
            built.push_back(pc);
        }
    }

    /* Configured trusted reverse-proxy IP immunity */
    for (size_t i = 0; i < trustedProxies.size(); ++i) {
        int fam = 0;
        std::array<uint8_t, 16> net{};
        if (parseSingleIpBytes(trustedProxies[i], fam, net, nullptr)) {
            ProtectedCidr pc;
            pc.family = fam;
            pc.networkBytes = net;
            pc.prefixLen = (fam == AF_INET) ? 32 : 128;
            built.push_back(pc);
        }
    }

    std::lock_guard<std::mutex> wlock(_writeMutex);
    std::lock_guard<std::mutex> lock(_mutex);
    _banConfigPath = banConfigPath;
    _thresholds = thresholds;
    _protectedCidrs = std::move(built);
}

const std::string &BanManager::banConfigPath() const
{
    return _banConfigPath;
}

bool BanManager::parseIpOrCidr(const std::string &input,
                               std::string &outCanonicalCidr,
                               int &outFamily,
                               std::array<uint8_t, 16> &outNetworkBytes,
                               int &outPrefixLen,
                               std::string &outError)
{
    outCanonicalCidr.clear();
    outFamily = 0;
    outNetworkBytes.fill(0);
    outPrefixLen = 0;
    outError.clear();

    const std::string trimmed = trimAscii(input);
    if (trimmed.empty() || trimmed.size() > 128) {
        outError = "INVALID_BAN_TARGET";
        return false;
    }
    for (size_t i = 0; i < trimmed.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(trimmed[i]);
        if (c <= 0x20 || c >= 0x7F || c == '%') {
            outError = "INVALID_BAN_TARGET";
            return false;
        }
    }

    std::string addrPart = trimmed;
    bool hasSlash = false;
    int rawPrefix = -1;

    const size_t slashPos = trimmed.find('/');
    if (slashPos != std::string::npos) {
        hasSlash = true;
        if (slashPos == 0 || slashPos + 1 >= trimmed.size() ||
            trimmed.find('/', slashPos + 1) != std::string::npos) {
            outError = "INVALID_BAN_TARGET";
            return false;
        }
        addrPart = trimmed.substr(0, slashPos);
        const std::string prefPart = trimmed.substr(slashPos + 1);
        if (prefPart.size() > 3) {
            outError = "INVALID_CIDR_PREFIX";
            return false;
        }
        for (size_t i = 0; i < prefPart.size(); ++i) {
            if (prefPart[i] < '0' || prefPart[i] > '9') {
                outError = "INVALID_BAN_TARGET";
                return false;
            }
        }
        if (prefPart.size() > 1 && prefPart[0] == '0') {
            outError = "INVALID_BAN_TARGET";
            return false;
        }
        rawPrefix = std::stoi(prefPart);
    }

    if (addrPart.empty()) {
        outError = "INVALID_BAN_TARGET";
        return false;
    }
    if (addrPart.front() == '[' || addrPart.back() == ']') {
        if (addrPart.front() != '[' || addrPart.back() != ']' ||
            addrPart.size() <= 2) {
            outError = "INVALID_BAN_TARGET";
            return false;
        }
        addrPart = addrPart.substr(1, addrPart.size() - 2);
    }

    struct in_addr a4;
    if (::inet_pton(AF_INET, addrPart.c_str(), &a4) == 1) {
        const int pref = hasSlash ? rawPrefix : 32;
        if (pref < 8 || pref > 32) {
            outError = "INVALID_CIDR_PREFIX";
            return false;
        }
        std::memcpy(outNetworkBytes.data(), &a4.s_addr, 4);
        applyPrefixMask(outNetworkBytes, 4, pref);
        char buf4[INET_ADDRSTRLEN];
        if (::inet_ntop(AF_INET, outNetworkBytes.data(), buf4, sizeof(buf4)) == nullptr) {
            outError = "INVALID_BAN_TARGET";
            return false;
        }
        outFamily = AF_INET;
        outPrefixLen = pref;
        outCanonicalCidr = std::string(buf4) + "/" + std::to_string(pref);
        return true;
    }

    struct in6_addr a6;
    if (::inet_pton(AF_INET6, addrPart.c_str(), &a6) == 1) {
        bool v4Mapped = true;
        for (int i = 0; i < 10; ++i) {
            if (a6.s6_addr[i] != 0) {
                v4Mapped = false;
                break;
            }
        }
        if (v4Mapped && a6.s6_addr[10] == 0xFF && a6.s6_addr[11] == 0xFF) {
            int pref4 = 32;
            if (hasSlash) {
                if (rawPrefix < 96 + 8 || rawPrefix > 128) {
                    outError = "INVALID_CIDR_PREFIX";
                    return false;
                }
                pref4 = rawPrefix - 96;
            }
            std::memcpy(outNetworkBytes.data(), a6.s6_addr + 12, 4);
            applyPrefixMask(outNetworkBytes, 4, pref4);
            char buf4[INET_ADDRSTRLEN];
            if (::inet_ntop(AF_INET, outNetworkBytes.data(), buf4, sizeof(buf4)) == nullptr) {
                outError = "INVALID_BAN_TARGET";
                return false;
            }
            outFamily = AF_INET;
            outPrefixLen = pref4;
            outCanonicalCidr = std::string(buf4) + "/" + std::to_string(pref4);
            return true;
        }

        const int pref6 = hasSlash ? rawPrefix : 128;
        if (pref6 < 16 || pref6 > 128) {
            outError = "INVALID_CIDR_PREFIX";
            return false;
        }
        std::memcpy(outNetworkBytes.data(), a6.s6_addr, 16);
        applyPrefixMask(outNetworkBytes, 16, pref6);
        char buf6[INET6_ADDRSTRLEN];
        if (::inet_ntop(AF_INET6, outNetworkBytes.data(), buf6, sizeof(buf6)) == nullptr) {
            outError = "INVALID_BAN_TARGET";
            return false;
        }
        outFamily = AF_INET6;
        outPrefixLen = pref6;
        outCanonicalCidr = lowerAscii(buf6) + "/" + std::to_string(pref6);
        return true;
    }

    outError = "INVALID_BAN_TARGET";
    return false;
}

bool BanManager::cidrContainsIp(int cidrFamily,
                                const std::array<uint8_t, 16> &cidrNetworkBytes,
                                int cidrPrefixLen,
                                const std::string &rawIp)
{
    int ipFamily = 0;
    std::array<uint8_t, 16> ipBytes{};
    if (!parseSingleIpBytes(rawIp, ipFamily, ipBytes, nullptr)) {
        return false;
    }
    if (ipFamily != cidrFamily) {
        return false;
    }
    const int maxBits = (cidrFamily == AF_INET) ? 32 : 128;
    if (cidrPrefixLen < 0 || cidrPrefixLen > maxBits) {
        return false;
    }
    return bitsMatchPrefix(ipBytes, cidrNetworkBytes, cidrPrefixLen);
}

bool BanManager::cidrsOverlap(int famA,
                              const std::array<uint8_t, 16> &netA,
                              int prefA,
                              int famB,
                              const std::array<uint8_t, 16> &netB,
                              int prefB)
{
    if (famA != famB) {
        return false;
    }
    const int maxBits = (famA == AF_INET) ? 32 : 128;
    if (prefA < 0 || prefA > maxBits || prefB < 0 || prefB > maxBits) {
        return false;
    }
    return bitsMatchPrefix(netA, netB, std::min(prefA, prefB));
}

bool BanManager::isProtectedLocked(int family,
                                   const std::array<uint8_t, 16> &networkBytes,
                                   int prefixLen) const
{
    for (size_t i = 0; i < _protectedCidrs.size(); ++i) {
        const ProtectedCidr &pc = _protectedCidrs[i];
        if (cidrsOverlap(family, networkBytes, prefixLen,
                         pc.family, pc.networkBytes, pc.prefixLen)) {
            return true;
        }
    }
    return false;
}

bool BanManager::isProtectedIpOrCidr(int family,
                                     const std::array<uint8_t, 16> &networkBytes,
                                     int prefixLen) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return isProtectedLocked(family, networkBytes, prefixLen);
}

bool BanManager::isProtectedIpOrCidr(const std::string &ipOrCidr) const
{
    std::string canon;
    int fam = 0;
    std::array<uint8_t, 16> net{};
    int pref = 0;
    std::string err;
    if (!parseIpOrCidr(ipOrCidr, canon, fam, net, pref, err)) {
        return false;
    }
    return isProtectedIpOrCidr(fam, net, pref);
}

std::string BanManager::makeBanId(BanDuration d)
{
    unsigned char raw[8];
    if (RAND_bytes(raw, sizeof(raw)) != 1) {
        uint64_t fallback = ++_tmpFileSeq;
        std::memcpy(raw, &fallback, sizeof(raw));
    }
    static const char hex[] = "0123456789abcdef";
    std::string out = (d == BanDuration::PERMANENT) ? "ban_perm_" : "ban_temp_";
    out.reserve(out.size() + 16);
    for (size_t i = 0; i < sizeof(raw); ++i) {
        out.push_back(hex[(raw[i] >> 4) & 0x0F]);
        out.push_back(hex[raw[i] & 0x0F]);
    }
    return out;
}

bool BanManager::cleanReason(const std::string &raw, std::string &out)
{
    out = trimAscii(raw);
    if (out.size() > kMaxReasonBytes) {
        return false;
    }
    for (size_t i = 0; i < out.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(out[i]);
        if (c < 0x20 || c == 0x7F) {
            return false;
        }
    }
    return true;
}

bool BanManager::writePermanentConfigAtomic(const std::string &path,
                                            int64_t updatedAt,
                                            const std::vector<BanEntry> &entries,
                                            std::string &error)
{
    if (path.empty()) {
        return true;
    }

    std::filesystem::path targetPath(path);
    std::filesystem::path parentDir = targetPath.parent_path();
    if (parentDir.empty()) {
        parentDir = ".";
    }
    std::error_code ec;
    if (!std::filesystem::exists(parentDir, ec)) {
        std::filesystem::create_directories(parentDir, ec);
        if (ec) {
            error = "cannot create ban config directory " + parentDir.string() +
                    ": " + ec.message();
            return false;
        }
    }

    Json doc;
    doc["version"] = 1;
    doc["updated_at"] = updatedAt;
    Json arr = Json::array();
    for (size_t i = 0; i < entries.size(); ++i) {
        const BanEntry &e = entries[i];
        Json item;
        item["ban_id"] = e.banId;
        item["target_type"] = toString(e.targetType);
        item["target_value"] = e.targetValue;
        item["display_label"] = e.displayLabel;
        item["reason"] = e.reason;
        item["created_at"] = e.createdAt;
        item["created_by"] = e.createdBy;
        arr.push_back(std::move(item));
    }
    doc["entries"] = std::move(arr);
    const std::string serialized =
        doc.dump(2, ' ', false, Json::error_handler_t::replace) + "\n";

    const uint64_t seq = ++_tmpFileSeq;
    const std::string tmpPath =
        path + ".tmp." + std::to_string(static_cast<long long>(::getpid())) +
        "." + std::to_string(seq);

    int fd = ::open(tmpPath.c_str(),
                    O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
                    0640);
    if (fd < 0) {
        error = "cannot open temporary ban config " + tmpPath + ": " +
                std::strerror(errno);
        return false;
    }

    size_t written = 0;
    while (written < serialized.size()) {
        ssize_t nw = ::write(fd, serialized.data() + written,
                             serialized.size() - written);
        if (nw < 0) {
            if (errno == EINTR) {
                continue;
            }
            error = "write failed for " + tmpPath + ": " + std::strerror(errno);
            ::close(fd);
            (void)::unlink(tmpPath.c_str());
            return false;
        }
        written += static_cast<size_t>(nw);
    }

    if (::fchmod(fd, 0640) != 0) {
        error = "fchmod(0640) failed for " + tmpPath + ": " +
                std::strerror(errno);
        ::close(fd);
        (void)::unlink(tmpPath.c_str());
        return false;
    }
    if (::fsync(fd) != 0) {
        error = "fsync failed for " + tmpPath + ": " + std::strerror(errno);
        ::close(fd);
        (void)::unlink(tmpPath.c_str());
        return false;
    }
    if (::close(fd) != 0) {
        error = "close failed for " + tmpPath + ": " + std::strerror(errno);
        (void)::unlink(tmpPath.c_str());
        return false;
    }
    if (::rename(tmpPath.c_str(), path.c_str()) != 0) {
        error = "rename failed for " + path + ": " + std::strerror(errno);
        (void)::unlink(tmpPath.c_str());
        return false;
    }

    int dirFd = ::open(parentDir.string().c_str(),
                       O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (dirFd >= 0) {
        (void)::fsync(dirFd);
        ::close(dirFd);
    }
    return true;
}

bool BanManager::loadFromStorage(AccountStore &store, int64_t nowSec,
                                 std::string &error)
{
    error.clear();
    std::string cfgPath;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        cfgPath = _banConfigPath;
    }

    std::vector<BanEntry> loadedPerm;
    std::vector<std::string> permAccountUuids;
    std::vector<int64_t> syncBannedPlayerIds;

    if (!cfgPath.empty()) {
        struct stat st;
        if (::stat(cfgPath.c_str(), &st) != 0) {
            if (errno != ENOENT) {
                error = "cannot stat ban config " + cfgPath + ": " +
                        std::strerror(errno);
                return false;
            }
        } else {
            if (!S_ISREG(st.st_mode) ||
                static_cast<uint64_t>(st.st_size) > kMaxBanConfigBytes) {
                error = "ban config " + cfgPath +
                        " must be a regular file of at most 4 MiB";
                return false;
            }
            std::ifstream in(cfgPath.c_str(), std::ios::binary);
            if (!in) {
                error = "cannot open ban config " + cfgPath;
                return false;
            }
            std::ostringstream ss;
            ss << in.rdbuf();
            Json doc = Json::parse(ss.str(), nullptr, false);
            if (doc.is_discarded() || !doc.is_object()) {
                error = "ban config " + cfgPath + " is not a valid JSON object";
                return false;
            }
            if (!doc.contains("version") || !doc["version"].is_number_integer() ||
                doc["version"].get<int>() != 1) {
                error = "ban config " + cfgPath + " must have \"version\": 1";
                return false;
            }
            if (!doc.contains("entries") || !doc["entries"].is_array()) {
                error = "ban config " + cfgPath + " must have an \"entries\" array";
                return false;
            }

            std::set<std::pair<int, std::string> > seenPerm;
            const Json &entries = doc["entries"];
            for (size_t i = 0; i < entries.size(); ++i) {
                const Json &item = entries[i];
                if (!item.is_object()) {
                    error = "ban config entry #" + std::to_string(i) +
                            " is not an object";
                    return false;
                }
                std::string rawType = item.value("target_type", "");
                std::string rawTarget = item.value("target_value", "");
                BanTargetType tType = BanTargetType::IP;
                if (!parseBanTargetType(rawType, tType) || rawTarget.empty()) {
                    error = "ban config entry #" + std::to_string(i) +
                            " has invalid target_type or target_value";
                    return false;
                }

                BanEntry be;
                be.banId = item.value("ban_id", "");
                if (be.banId.empty()) {
                    be.banId = makeBanId(BanDuration::PERMANENT);
                }
                be.targetType = tType;
                be.duration = BanDuration::PERMANENT;
                be.displayLabel = item.value("display_label", "");
                be.reason = item.value("reason", "");
                be.source = "admin";
                be.createdBy = item.value("created_by", "config");
                be.createdAt = item.value("created_at", nowSec);
                be.expiresAt = 0;

                if (tType == BanTargetType::IP) {
                    std::string parseErr;
                    if (!parseIpOrCidr(rawTarget, be.targetValue, be.family,
                                       be.networkBytes, be.prefixLen, parseErr)) {
                        error = "ban config entry #" + std::to_string(i) +
                                " invalid CIDR '" + rawTarget + "' (" + parseErr + ")";
                        return false;
                    }
                    if (isProtectedIpOrCidr(be.family, be.networkBytes, be.prefixLen)) {
                        error = "ban config entry #" + std::to_string(i) +
                                " targets protected IP/CIDR '" + be.targetValue + "'";
                        return false;
                    }
                    if (be.displayLabel.empty()) {
                        be.displayLabel = be.targetValue;
                    }
                } else if (tType == BanTargetType::ACCOUNT) {
                    std::string trimmed = trimAscii(rawTarget);
                    PlayerProfile prof;
                    bool found = store.getProfileByUuid(trimmed, prof);
                    if (!found) {
                        std::string handle = trimmed;
                        if (!handle.empty() && handle[0] == '@') {
                            handle = handle.substr(1);
                        }
                        found = store.getProfileByHandle(handle, prof);
                    }
                    if (found) {
                        if (store.isPlayerAdmin(prof.playerId)) {
                            error = "ban config entry #" + std::to_string(i) +
                                    " cannot ban administrator account @" + prof.handle;
                            return false;
                        }
                        be.targetValue = prof.uuid;
                        if (be.displayLabel.empty()) {
                            be.displayLabel = "@" + prof.handle;
                        }
                        syncBannedPlayerIds.push_back(prof.playerId);
                    } else if (trimmed[0] != '@' && looksLikeUuid(trimmed)) {
                        be.targetValue = trimmed;
                        if (be.displayLabel.empty()) {
                            be.displayLabel = trimmed;
                        }
                    } else {
                        error = "ban config entry #" + std::to_string(i) +
                                " references unknown account '" + rawTarget + "'";
                        return false;
                    }
                    permAccountUuids.push_back(be.targetValue);
                } else {
                    std::string trimmed = trimAscii(rawTarget);
                    RobotRecord rob;
                    bool found = store.getRobotByUuid(trimmed, rob) ||
                                 store.getRobotByName(trimmed, rob);
                    if (found) {
                        if (store.isPlayerAdmin(rob.ownerPlayerId)) {
                            error = "ban config entry #" + std::to_string(i) +
                                    " cannot ban admin-owned robot " + rob.robotName;
                            return false;
                        }
                        be.targetValue = rob.robotUuid;
                        if (be.displayLabel.empty()) {
                            be.displayLabel = rob.robotName;
                        }
                    } else if (looksLikeUuid(trimmed)) {
                        be.targetValue = trimmed;
                        if (be.displayLabel.empty()) {
                            be.displayLabel = trimmed;
                        }
                    } else {
                        error = "ban config entry #" + std::to_string(i) +
                                " references unknown robot '" + rawTarget + "'";
                        return false;
                    }
                }

                auto key = std::make_pair(static_cast<int>(be.targetType),
                                          be.targetValue);
                if (seenPerm.insert(key).second) {
                    loadedPerm.push_back(std::move(be));
                }
            }
        }
    }

    for (size_t i = 0; i < syncBannedPlayerIds.size(); ++i) {
        store.setBanned(syncBannedPlayerIds[i], true);
        store.revokeAllSessions(syncBannedPlayerIds[i]);
    }

    store.purgeExpiredTempBans(nowSec, permAccountUuids);

    std::vector<TempBanRow> tempRows;
    if (!store.listTempBans(tempRows)) {
        error = "failed to load temp_bans from SQLite";
        return false;
    }

    std::vector<BanEntry> loadedTemp;
    for (size_t i = 0; i < tempRows.size(); ++i) {
        const TempBanRow &r = tempRows[i];
        if (r.expiresAt <= nowSec) {
            continue;
        }
        BanTargetType tType = BanTargetType::IP;
        if (!parseBanTargetType(r.targetType, tType)) {
            continue;
        }
        BanEntry be;
        be.banId = r.banId;
        be.targetType = tType;
        be.duration = BanDuration::TEMP_24H;
        be.targetValue = r.targetValue;
        be.displayLabel = r.displayLabel.empty() ? r.targetValue : r.displayLabel;
        be.reason = r.reason;
        be.source = r.source;
        be.createdBy = r.createdBy;
        be.createdAt = r.createdAt;
        be.expiresAt = r.expiresAt;

        if (tType == BanTargetType::IP) {
            std::string parseErr;
            if (!parseIpOrCidr(r.targetValue, be.targetValue, be.family,
                               be.networkBytes, be.prefixLen, parseErr)) {
                continue;
            }
            if (isProtectedIpOrCidr(be.family, be.networkBytes, be.prefixLen)) {
                continue;
            }
        } else if (tType == BanTargetType::ACCOUNT) {
            PlayerProfile prof;
            if (store.getProfileByUuid(r.targetValue, prof)) {
                if (store.isPlayerAdmin(prof.playerId)) {
                    continue;
                }
                store.setBanned(prof.playerId, true);
            }
        }
        loadedTemp.push_back(std::move(be));
    }

    std::lock_guard<std::mutex> wlock(_writeMutex);
    std::lock_guard<std::mutex> lock(_mutex);
    _permBans = std::move(loadedPerm);
    _tempBans = std::move(loadedTemp);
    return true;
}

bool BanManager::matchIp(const std::string &rawIp, int64_t nowSec,
                         BanMatch &outMatch) const
{
    outMatch = BanMatch{};
    int ipFamily = 0;
    std::array<uint8_t, 16> ipBytes{};
    if (!parseSingleIpBytes(rawIp, ipFamily, ipBytes, nullptr)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    if (isProtectedLocked(ipFamily, ipBytes, ipFamily == AF_INET ? 32 : 128)) {
        return false;
    }

    for (size_t i = 0; i < _permBans.size(); ++i) {
        const BanEntry &e = _permBans[i];
        if (e.targetType == BanTargetType::IP && e.family == ipFamily &&
            bitsMatchPrefix(ipBytes, e.networkBytes, e.prefixLen)) {
            outMatch.matched = true;
            outMatch.banId = e.banId;
            outMatch.targetType = BanTargetType::IP;
            outMatch.duration = BanDuration::PERMANENT;
            outMatch.targetValue = e.targetValue;
            outMatch.reason = e.reason;
            outMatch.expiresAt = 0;
            return true;
        }
    }

    for (size_t i = 0; i < _tempBans.size(); ++i) {
        const BanEntry &e = _tempBans[i];
        if (e.targetType == BanTargetType::IP && nowSec < e.expiresAt &&
            e.family == ipFamily &&
            bitsMatchPrefix(ipBytes, e.networkBytes, e.prefixLen)) {
            outMatch.matched = true;
            outMatch.banId = e.banId;
            outMatch.targetType = BanTargetType::IP;
            outMatch.duration = BanDuration::TEMP_24H;
            outMatch.targetValue = e.targetValue;
            outMatch.reason = e.reason;
            outMatch.expiresAt = e.expiresAt;
            return true;
        }
    }
    return false;
}

bool BanManager::matchAccount(const std::string &playerUuid, int64_t nowSec,
                              BanMatch &outMatch) const
{
    outMatch = BanMatch{};
    if (playerUuid.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    for (size_t i = 0; i < _permBans.size(); ++i) {
        const BanEntry &e = _permBans[i];
        if (e.targetType == BanTargetType::ACCOUNT &&
            e.targetValue == playerUuid) {
            outMatch.matched = true;
            outMatch.banId = e.banId;
            outMatch.targetType = BanTargetType::ACCOUNT;
            outMatch.duration = BanDuration::PERMANENT;
            outMatch.targetValue = e.targetValue;
            outMatch.reason = e.reason;
            outMatch.expiresAt = 0;
            return true;
        }
    }
    for (size_t i = 0; i < _tempBans.size(); ++i) {
        const BanEntry &e = _tempBans[i];
        if (e.targetType == BanTargetType::ACCOUNT &&
            nowSec < e.expiresAt && e.targetValue == playerUuid) {
            outMatch.matched = true;
            outMatch.banId = e.banId;
            outMatch.targetType = BanTargetType::ACCOUNT;
            outMatch.duration = BanDuration::TEMP_24H;
            outMatch.targetValue = e.targetValue;
            outMatch.reason = e.reason;
            outMatch.expiresAt = e.expiresAt;
            return true;
        }
    }
    return false;
}

bool BanManager::matchRobot(const std::string &robotUuid, int64_t nowSec,
                            BanMatch &outMatch) const
{
    outMatch = BanMatch{};
    if (robotUuid.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    for (size_t i = 0; i < _permBans.size(); ++i) {
        const BanEntry &e = _permBans[i];
        if (e.targetType == BanTargetType::ROBOT &&
            e.targetValue == robotUuid) {
            outMatch.matched = true;
            outMatch.banId = e.banId;
            outMatch.targetType = BanTargetType::ROBOT;
            outMatch.duration = BanDuration::PERMANENT;
            outMatch.targetValue = e.targetValue;
            outMatch.reason = e.reason;
            outMatch.expiresAt = 0;
            return true;
        }
    }
    for (size_t i = 0; i < _tempBans.size(); ++i) {
        const BanEntry &e = _tempBans[i];
        if (e.targetType == BanTargetType::ROBOT &&
            nowSec < e.expiresAt && e.targetValue == robotUuid) {
            outMatch.matched = true;
            outMatch.banId = e.banId;
            outMatch.targetType = BanTargetType::ROBOT;
            outMatch.duration = BanDuration::TEMP_24H;
            outMatch.targetValue = e.targetValue;
            outMatch.reason = e.reason;
            outMatch.expiresAt = e.expiresAt;
            return true;
        }
    }
    return false;
}

void BanManager::noteVerifiedAdminIp(const std::string &rawIp, int64_t nowSec)
{
    int fam = 0;
    std::array<uint8_t, 16> bytes{};
    std::string norm;
    if (!parseSingleIpBytes(rawIp, fam, bytes, &norm) || norm.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    if (_recentAdminIps.size() >= 1024) {
        for (auto it = _recentAdminIps.begin(); it != _recentAdminIps.end();) {
            if (nowSec - it->second >= kRecentAdminIpTtlSec) {
                it = _recentAdminIps.erase(it);
            } else {
                ++it;
            }
        }
        if (_recentAdminIps.size() >= 1024) {
            _recentAdminIps.erase(_recentAdminIps.begin());
        }
    }
    _recentAdminIps[norm] = nowSec;
}

bool BanManager::hasRecentAdminIpLocked(const std::string &normIp,
                                        int64_t nowSec)
{
    auto it = _recentAdminIps.find(normIp);
    if (it == _recentAdminIps.end()) {
        return false;
    }
    if (nowSec - it->second >= kRecentAdminIpTtlSec) {
        _recentAdminIps.erase(it);
        return false;
    }
    return true;
}

BanManager::SlidingBucket &BanManager::touchBucketLocked(const std::string &key,
                                                         int64_t nowSec)
{
    auto it = _eventWindows.find(key);
    if (it == _eventWindows.end()) {
        if (_eventWindows.size() >= kMaxTrackedKeys) {
            for (auto scan = _eventWindows.begin();
                 scan != _eventWindows.end();) {
                if (nowSec - scan->second.lastTouchedSec >= kAbuseHorizonSec) {
                    scan = _eventWindows.erase(scan);
                } else {
                    ++scan;
                }
            }
            if (_eventWindows.size() >= kMaxTrackedKeys) {
                auto oldest = _eventWindows.begin();
                for (auto scan = _eventWindows.begin();
                     scan != _eventWindows.end(); ++scan) {
                    if (scan->second.lastTouchedSec <
                        oldest->second.lastTouchedSec) {
                        oldest = scan;
                    }
                }
                _eventWindows.erase(oldest);
            }
        }
        SlidingBucket fresh;
        fresh.lastTouchedSec = nowSec;
        it = _eventWindows.emplace(key, std::move(fresh)).first;
    }
    it->second.lastTouchedSec = nowSec;
    return it->second;
}

int BanManager::recordWindowEventLocked(const std::string &key,
                                        int64_t windowSec,
                                        int64_t nowSec)
{
    SlidingBucket &b = touchBucketLocked(key, nowSec);
    while (!b.eventsSec.empty() &&
           (nowSec - b.eventsSec.front() >= kAbuseHorizonSec ||
            b.eventsSec.size() >= 256)) {
        b.eventsSec.pop_front();
    }
    b.eventsSec.push_back(nowSec);

    int count = 0;
    for (size_t i = 0; i < b.eventsSec.size(); ++i) {
        if (nowSec - b.eventsSec[i] < windowSec) {
            ++count;
        }
    }
    return count;
}

BanSideEffects BanManager::triggerAutoBan(AccountStore &store,
                                          const AbuseActorContext &ctx,
                                          AbusePattern pattern,
                                          int count,
                                          int64_t windowSec,
                                          int64_t nowSec)
{
    BanSideEffects fx;
    if (ctx.isAdmin) {
        return fx;
    }

    const std::string reason =
        std::string("auto:") + toString(pattern) + " (" +
        std::to_string(count) + " in " + std::to_string(windowSec) + "s)";
    const int64_t expiresAt = nowSec + kAutoBanDurationSec;

    std::vector<TempBanRow> dbUpserts;
    std::vector<BanAuditLogRow> audits;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        /* 1. IP 24h auto-ban (unless protected or recently used by a verified admin) */
        int ipFam = 0;
        std::array<uint8_t, 16> ipBytes{};
        std::string normIp;
        if (!ctx.remoteIp.empty() &&
            parseSingleIpBytes(ctx.remoteIp, ipFam, ipBytes, &normIp)) {
            const int pref = (ipFam == AF_INET) ? 32 : 128;
            if (!isProtectedLocked(ipFam, ipBytes, pref) &&
                !hasRecentAdminIpLocked(normIp, nowSec)) {
                const std::string cidr = normIp + "/" + std::to_string(pref);
                bool alreadyPerm = false;
                for (size_t i = 0; i < _permBans.size(); ++i) {
                    if (_permBans[i].targetType == BanTargetType::IP &&
                        _permBans[i].family == ipFam &&
                        bitsMatchPrefix(ipBytes, _permBans[i].networkBytes,
                                        _permBans[i].prefixLen)) {
                        alreadyPerm = true;
                        break;
                    }
                }
                if (!alreadyPerm) {
                    BanEntry *existing = nullptr;
                    for (size_t i = 0; i < _tempBans.size(); ++i) {
                        if (_tempBans[i].targetType == BanTargetType::IP &&
                            _tempBans[i].targetValue == cidr) {
                            existing = &_tempBans[i];
                            break;
                        }
                    }
                    if (existing == nullptr || existing->expiresAt <= nowSec) {
                        BanEntry be;
                        be.banId = makeBanId(BanDuration::TEMP_24H);
                        be.targetType = BanTargetType::IP;
                        be.duration = BanDuration::TEMP_24H;
                        be.targetValue = cidr;
                        be.displayLabel = cidr;
                        be.reason = reason;
                        be.source = "auto";
                        be.createdBy = "system";
                        be.createdAt = nowSec;
                        be.expiresAt = expiresAt;
                        be.family = ipFam;
                        be.networkBytes = ipBytes;
                        be.prefixLen = pref;

                        if (existing != nullptr) {
                            *existing = be;
                        } else {
                            _tempBans.push_back(be);
                        }

                        TempBanRow row;
                        row.banId = be.banId;
                        row.targetType = "ip";
                        row.targetValue = be.targetValue;
                        row.displayLabel = be.displayLabel;
                        row.reason = be.reason;
                        row.source = "auto";
                        row.createdBy = "system";
                        row.createdAt = be.createdAt;
                        row.expiresAt = be.expiresAt;
                        dbUpserts.push_back(std::move(row));

                        BanAuditLogRow audit;
                        audit.createdAt = nowSec;
                        audit.actorLabel = "system";
                        audit.action = "auto_ban_24h";
                        audit.targetType = "ip";
                        audit.targetValue = cidr;
                        audit.reason = reason;
                        audits.push_back(std::move(audit));
                        fx.changed = true;
                    }
                }
            }
        }

        /* 2. Authenticated non-admin robot or account 24h auto-ban */
        if (ctx.robotId > 0 && !ctx.robotUuid.empty()) {
            bool alreadyPerm = false;
            for (size_t i = 0; i < _permBans.size(); ++i) {
                if (_permBans[i].targetType == BanTargetType::ROBOT &&
                    _permBans[i].targetValue == ctx.robotUuid) {
                    alreadyPerm = true;
                    break;
                }
            }
            if (!alreadyPerm) {
                BanEntry *existing = nullptr;
                for (size_t i = 0; i < _tempBans.size(); ++i) {
                    if (_tempBans[i].targetType == BanTargetType::ROBOT &&
                        _tempBans[i].targetValue == ctx.robotUuid) {
                        existing = &_tempBans[i];
                        break;
                    }
                }
                if (existing == nullptr || existing->expiresAt <= nowSec) {
                    BanEntry be;
                    be.banId = makeBanId(BanDuration::TEMP_24H);
                    be.targetType = BanTargetType::ROBOT;
                    be.duration = BanDuration::TEMP_24H;
                    be.targetValue = ctx.robotUuid;
                    be.displayLabel = ctx.robotName.empty()
                                          ? ctx.robotUuid
                                          : ctx.robotName;
                    be.reason = reason;
                    be.source = "auto";
                    be.createdBy = "system";
                    be.createdAt = nowSec;
                    be.expiresAt = expiresAt;
                    if (existing != nullptr) {
                        *existing = be;
                    } else {
                        _tempBans.push_back(be);
                    }

                    TempBanRow row;
                    row.banId = be.banId;
                    row.targetType = "robot";
                    row.targetValue = be.targetValue;
                    row.displayLabel = be.displayLabel;
                    row.reason = be.reason;
                    row.source = "auto";
                    row.createdBy = "system";
                    row.createdAt = be.createdAt;
                    row.expiresAt = be.expiresAt;
                    dbUpserts.push_back(std::move(row));

                    BanAuditLogRow audit;
                    audit.createdAt = nowSec;
                    audit.actorLabel = "system";
                    audit.action = "auto_ban_24h";
                    audit.targetType = "robot";
                    audit.targetValue = ctx.robotUuid;
                    audit.reason = reason;
                    audits.push_back(std::move(audit));

                    fx.bannedRobotIds.push_back(ctx.robotId);
                    fx.changed = true;
                }
            }
        } else if (ctx.playerId > 0 && !ctx.playerUuid.empty()) {
            bool alreadyPerm = false;
            for (size_t i = 0; i < _permBans.size(); ++i) {
                if (_permBans[i].targetType == BanTargetType::ACCOUNT &&
                    _permBans[i].targetValue == ctx.playerUuid) {
                    alreadyPerm = true;
                    break;
                }
            }
            if (!alreadyPerm) {
                BanEntry *existing = nullptr;
                for (size_t i = 0; i < _tempBans.size(); ++i) {
                    if (_tempBans[i].targetType == BanTargetType::ACCOUNT &&
                        _tempBans[i].targetValue == ctx.playerUuid) {
                        existing = &_tempBans[i];
                        break;
                    }
                }
                if (existing == nullptr || existing->expiresAt <= nowSec) {
                    BanEntry be;
                    be.banId = makeBanId(BanDuration::TEMP_24H);
                    be.targetType = BanTargetType::ACCOUNT;
                    be.duration = BanDuration::TEMP_24H;
                    be.targetValue = ctx.playerUuid;
                    be.displayLabel = ctx.playerHandle.empty()
                                          ? ctx.playerUuid
                                          : ("@" + ctx.playerHandle);
                    be.reason = reason;
                    be.source = "auto";
                    be.createdBy = "system";
                    be.createdAt = nowSec;
                    be.expiresAt = expiresAt;
                    if (existing != nullptr) {
                        *existing = be;
                    } else {
                        _tempBans.push_back(be);
                    }

                    TempBanRow row;
                    row.banId = be.banId;
                    row.targetType = "account";
                    row.targetValue = be.targetValue;
                    row.displayLabel = be.displayLabel;
                    row.reason = be.reason;
                    row.source = "auto";
                    row.createdBy = "system";
                    row.createdAt = be.createdAt;
                    row.expiresAt = be.expiresAt;
                    dbUpserts.push_back(std::move(row));

                    BanAuditLogRow audit;
                    audit.createdAt = nowSec;
                    audit.actorLabel = "system";
                    audit.action = "auto_ban_24h";
                    audit.targetType = "account";
                    audit.targetValue = ctx.playerUuid;
                    audit.reason = reason;
                    audits.push_back(std::move(audit));

                    fx.bannedPlayerIds.push_back(ctx.playerId);
                    fx.changed = true;
                }
            }
        }
    }

    /* Persist outside _mutex to strictly preserve leaf-lock hierarchy */
    for (size_t i = 0; i < dbUpserts.size(); ++i) {
        store.upsertTempBan(dbUpserts[i]);
    }
    for (size_t i = 0; i < fx.bannedPlayerIds.size(); ++i) {
        store.setBanned(fx.bannedPlayerIds[i], true);
        store.revokeAllSessions(fx.bannedPlayerIds[i]);
    }
    for (size_t i = 0; i < audits.size(); ++i) {
        store.appendBanAuditLog(audits[i].createdAt, audits[i].actorLabel,
                                audits[i].action, audits[i].targetType,
                                audits[i].targetValue, audits[i].reason);
    }
    return fx;
}

BanSideEffects BanManager::recordRateLimit429(AccountStore &store,
                                              const AbuseActorContext &ctx,
                                              int64_t nowSec)
{
    if (ctx.isAdmin) {
        return BanSideEffects{};
    }
    bool tripped = false;
    int tripCount = 0;
    int64_t tripWindow = kShortRateWindowSec;

    {
        std::lock_guard<std::mutex> lock(_mutex);
        int ipFam = 0;
        std::array<uint8_t, 16> ipBytes{};
        std::string normIp;
        if (!ctx.remoteIp.empty() &&
            parseSingleIpBytes(ctx.remoteIp, ipFam, ipBytes, &normIp) &&
            !isProtectedLocked(ipFam, ipBytes, ipFam == AF_INET ? 32 : 128) &&
            !hasRecentAdminIpLocked(normIp, nowSec)) {
            const std::string k = "429:ip:" + normIp;
            SlidingBucket &b = touchBucketLocked(k, nowSec);
            while (!b.eventsSec.empty() &&
                   (nowSec - b.eventsSec.front() >= kAbuseHorizonSec ||
                    b.eventsSec.size() >= 256)) {
                b.eventsSec.pop_front();
            }
            b.eventsSec.push_back(nowSec);
            int c60 = 0;
            int c300 = static_cast<int>(b.eventsSec.size());
            for (size_t i = 0; i < b.eventsSec.size(); ++i) {
                if (nowSec - b.eventsSec[i] < kShortRateWindowSec) {
                    ++c60;
                }
            }
            if (c60 >= _thresholds.rateLimit429In60s) {
                tripped = true;
                tripCount = c60;
                tripWindow = kShortRateWindowSec;
                b.eventsSec.clear();
            } else if (c300 >= _thresholds.rateLimit429In300s) {
                tripped = true;
                tripCount = c300;
                tripWindow = kAbuseHorizonSec;
                b.eventsSec.clear();
            }
        }

        if (!tripped && (ctx.robotId > 0 || ctx.playerId > 0)) {
            const std::string actorKey =
                ctx.robotId > 0
                    ? ("429:bot:" + std::to_string(ctx.robotId))
                    : ("429:usr:" + std::to_string(ctx.playerId));
            SlidingBucket &b = touchBucketLocked(actorKey, nowSec);
            while (!b.eventsSec.empty() &&
                   (nowSec - b.eventsSec.front() >= kAbuseHorizonSec ||
                    b.eventsSec.size() >= 256)) {
                b.eventsSec.pop_front();
            }
            b.eventsSec.push_back(nowSec);
            int c60 = 0;
            int c300 = static_cast<int>(b.eventsSec.size());
            for (size_t i = 0; i < b.eventsSec.size(); ++i) {
                if (nowSec - b.eventsSec[i] < kShortRateWindowSec) {
                    ++c60;
                }
            }
            if (c60 >= _thresholds.rateLimit429In60s) {
                tripped = true;
                tripCount = c60;
                tripWindow = kShortRateWindowSec;
                b.eventsSec.clear();
            } else if (c300 >= _thresholds.rateLimit429In300s) {
                tripped = true;
                tripCount = c300;
                tripWindow = kAbuseHorizonSec;
                b.eventsSec.clear();
            }
        }
    }

    if (!tripped) {
        return BanSideEffects{};
    }
    return triggerAutoBan(store, ctx, AbusePattern::RATE_LIMIT_429,
                          tripCount, tripWindow, nowSec);
}

BanSideEffects BanManager::recordAuthFailure(AccountStore &store,
                                             const AbuseActorContext &ctx,
                                             int64_t nowSec)
{
    if (ctx.isAdmin) {
        return BanSideEffects{};
    }
    bool tripped = false;
    int tripCount = 0;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        int ipFam = 0;
        std::array<uint8_t, 16> ipBytes{};
        std::string normIp;
        if (!ctx.remoteIp.empty() &&
            parseSingleIpBytes(ctx.remoteIp, ipFam, ipBytes, &normIp) &&
            !isProtectedLocked(ipFam, ipBytes, ipFam == AF_INET ? 32 : 128) &&
            !hasRecentAdminIpLocked(normIp, nowSec)) {
            const std::string k = "auth:ip:" + normIp;
            const int c = recordWindowEventLocked(k, kAbuseHorizonSec, nowSec);
            if (c >= _thresholds.authFailuresIn300s) {
                tripped = true;
                tripCount = c;
                _eventWindows[k].eventsSec.clear();
            }
        }
    }
    if (!tripped) {
        return BanSideEffects{};
    }
    return triggerAutoBan(store, ctx,
                          AbusePattern::AUTH_OR_PASSCODE_BRUTE_FORCE,
                          tripCount, kAbuseHorizonSec, nowSec);
}

BanSideEffects BanManager::recordPasscodeFailure(AccountStore &store,
                                                 const AbuseActorContext &ctx,
                                                 int64_t nowSec)
{
    if (ctx.isAdmin) {
        return BanSideEffects{};
    }
    bool tripped = false;
    int tripCount = 0;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        int ipFam = 0;
        std::array<uint8_t, 16> ipBytes{};
        std::string normIp;
        if (!ctx.remoteIp.empty() &&
            parseSingleIpBytes(ctx.remoteIp, ipFam, ipBytes, &normIp) &&
            !isProtectedLocked(ipFam, ipBytes, ipFam == AF_INET ? 32 : 128) &&
            !hasRecentAdminIpLocked(normIp, nowSec)) {
            const std::string k = "pass:ip:" + normIp;
            const int c = recordWindowEventLocked(k, kAbuseHorizonSec, nowSec);
            if (c >= _thresholds.passcodeFailuresIn300s) {
                tripped = true;
                tripCount = c;
                _eventWindows[k].eventsSec.clear();
            }
        }
        if (!tripped && (ctx.robotId > 0 || ctx.playerId > 0)) {
            const std::string ak =
                ctx.robotId > 0
                    ? ("pass:bot:" + std::to_string(ctx.robotId))
                    : ("pass:usr:" + std::to_string(ctx.playerId));
            const int c = recordWindowEventLocked(ak, kAbuseHorizonSec, nowSec);
            if (c >= _thresholds.passcodeFailuresIn300s) {
                tripped = true;
                tripCount = c;
                _eventWindows[ak].eventsSec.clear();
            }
        }
    }
    if (!tripped) {
        return BanSideEffects{};
    }
    return triggerAutoBan(store, ctx,
                          AbusePattern::AUTH_OR_PASSCODE_BRUTE_FORCE,
                          tripCount, kAbuseHorizonSec, nowSec);
}

BanSideEffects BanManager::recordCrawlerUaHit(AccountStore &store,
                                              const std::string &remoteIp,
                                              int64_t nowSec)
{
    bool tripped = false;
    int tripCount = 0;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        int ipFam = 0;
        std::array<uint8_t, 16> ipBytes{};
        std::string normIp;
        if (!remoteIp.empty() &&
            parseSingleIpBytes(remoteIp, ipFam, ipBytes, &normIp) &&
            !isProtectedLocked(ipFam, ipBytes, ipFam == AF_INET ? 32 : 128) &&
            !hasRecentAdminIpLocked(normIp, nowSec)) {
            const std::string k = "crawler:ip:" + normIp;
            const int c = recordWindowEventLocked(k, kAbuseHorizonSec, nowSec);
            if (c >= _thresholds.crawlerUaIn300s) {
                tripped = true;
                tripCount = c;
                _eventWindows[k].eventsSec.clear();
            }
        }
    }
    if (!tripped) {
        return BanSideEffects{};
    }
    AbuseActorContext ctx;
    ctx.remoteIp = remoteIp;
    return triggerAutoBan(store, ctx, AbusePattern::CRAWLER_SCRAPE,
                          tripCount, kAbuseHorizonSec, nowSec);
}

BanSideEffects BanManager::recordAnonymousEnumeration(
    AccountStore &store,
    const std::string &remoteIp,
    const std::string &path,
    int httpStatus,
    int64_t nowSec)
{
    (void)httpStatus;
    if (remoteIp.empty() || path.empty()) {
        return BanSideEffects{};
    }
    bool tripped = false;
    int distinctCount = 0;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        int ipFam = 0;
        std::array<uint8_t, 16> ipBytes{};
        std::string normIp;
        if (!parseSingleIpBytes(remoteIp, ipFam, ipBytes, &normIp) ||
            isProtectedLocked(ipFam, ipBytes, ipFam == AF_INET ? 32 : 128) ||
            hasRecentAdminIpLocked(normIp, nowSec)) {
            return BanSideEffects{};
        }

        auto it = _anonEnumWindows.find(normIp);
        if (it == _anonEnumWindows.end()) {
            if (_anonEnumWindows.size() >= 4096) {
                for (auto scan = _anonEnumWindows.begin();
                     scan != _anonEnumWindows.end();) {
                    if (nowSec - scan->second.lastTouchedSec >= kAbuseHorizonSec) {
                        scan = _anonEnumWindows.erase(scan);
                    } else {
                        ++scan;
                    }
                }
                if (_anonEnumWindows.size() >= 4096) {
                    auto oldest = _anonEnumWindows.begin();
                    for (auto scan = _anonEnumWindows.begin();
                         scan != _anonEnumWindows.end(); ++scan) {
                        if (scan->second.lastTouchedSec <
                            oldest->second.lastTouchedSec) {
                            oldest = scan;
                        }
                    }
                    _anonEnumWindows.erase(oldest);
                }
            }
            AnonEnumBucket fresh;
            fresh.lastTouchedSec = nowSec;
            it = _anonEnumWindows.emplace(normIp, std::move(fresh)).first;
        }
        AnonEnumBucket &b = it->second;
        b.lastTouchedSec = nowSec;
        while (!b.hits.empty() &&
               (nowSec - b.hits.front().first >= kAbuseHorizonSec ||
                b.hits.size() >= 256)) {
            b.hits.pop_front();
        }
        b.hits.push_back(std::make_pair(nowSec, path.substr(0, 128)));

        std::unordered_set<std::string> distinct;
        for (size_t i = 0; i < b.hits.size(); ++i) {
            distinct.insert(b.hits[i].second);
        }
        distinctCount = static_cast<int>(distinct.size());
        if (distinctCount >= _thresholds.anonEnumPathsIn300s) {
            tripped = true;
            b.hits.clear();
        }
    }
    if (!tripped) {
        return BanSideEffects{};
    }
    AbuseActorContext ctx;
    ctx.remoteIp = remoteIp;
    return triggerAutoBan(store, ctx, AbusePattern::CRAWLER_SCRAPE,
                          distinctCount, kAbuseHorizonSec, nowSec);
}

BanSideEffects BanManager::recordExploitProbe(AccountStore &store,
                                              const AbuseActorContext &ctx,
                                              int64_t nowSec)
{
    if (ctx.isAdmin) {
        return BanSideEffects{};
    }
    bool tripped = false;
    int tripCount = 0;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        int ipFam = 0;
        std::array<uint8_t, 16> ipBytes{};
        std::string normIp;
        if (!ctx.remoteIp.empty() &&
            parseSingleIpBytes(ctx.remoteIp, ipFam, ipBytes, &normIp) &&
            !isProtectedLocked(ipFam, ipBytes, ipFam == AF_INET ? 32 : 128) &&
            !hasRecentAdminIpLocked(normIp, nowSec)) {
            const std::string k = "exploit:ip:" + normIp;
            const int c = recordWindowEventLocked(k, kAbuseHorizonSec, nowSec);
            if (c >= _thresholds.exploitProbesIn300s) {
                tripped = true;
                tripCount = c;
                _eventWindows[k].eventsSec.clear();
            }
        }
        if (!tripped && (ctx.robotId > 0 || ctx.playerId > 0)) {
            const std::string ak =
                ctx.robotId > 0
                    ? ("exploit:bot:" + std::to_string(ctx.robotId))
                    : ("exploit:usr:" + std::to_string(ctx.playerId));
            const int c = recordWindowEventLocked(ak, kAbuseHorizonSec, nowSec);
            if (c >= _thresholds.exploitProbesIn300s) {
                tripped = true;
                tripCount = c;
                _eventWindows[ak].eventsSec.clear();
            }
        }
    }
    if (!tripped) {
        return BanSideEffects{};
    }
    return triggerAutoBan(store, ctx, AbusePattern::EXPLOIT_PROBE,
                          tripCount, kAbuseHorizonSec, nowSec);
}

int BanManager::purgeExpired(AccountStore &store, int64_t nowSec)
{
    std::vector<std::string> permAccounts;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _tempBans.erase(
            std::remove_if(_tempBans.begin(), _tempBans.end(),
                           [nowSec](const BanEntry &e) {
                               return e.expiresAt <= nowSec;
                           }),
            _tempBans.end());
        for (size_t i = 0; i < _permBans.size(); ++i) {
            if (_permBans[i].targetType == BanTargetType::ACCOUNT) {
                permAccounts.push_back(_permBans[i].targetValue);
            }
        }
    }
    return store.purgeExpiredTempBans(nowSec, permAccounts);
}

void BanManager::listBans(AccountStore &store, int64_t nowSec,
                          std::vector<BanEntry> &outTempBans,
                          std::vector<BanEntry> &outPermBans)
{
    purgeExpired(store, nowSec);
    std::lock_guard<std::mutex> lock(_mutex);
    outTempBans = _tempBans;
    outPermBans = _permBans;
    std::sort(outTempBans.begin(), outTempBans.end(),
              [](const BanEntry &a, const BanEntry &b) {
                  if (a.createdAt != b.createdAt) {
                      return a.createdAt > b.createdAt;
                  }
                  return a.banId < b.banId;
              });
    std::sort(outPermBans.begin(), outPermBans.end(),
              [](const BanEntry &a, const BanEntry &b) {
                  if (a.createdAt != b.createdAt) {
                      return a.createdAt > b.createdAt;
                  }
                  return a.banId < b.banId;
              });
}

BanOpResult BanManager::addBan(AccountStore &store,
                               BanTargetType targetType,
                               const std::string &rawTarget,
                               BanDuration duration,
                               const std::string &rawReason,
                               int64_t actorPlayerId,
                               const std::string &actorLabel,
                               const std::string &callerRemoteIp,
                               int64_t nowSec)
{
    BanOpResult res;
    std::string reason;
    if (!cleanReason(rawReason, reason)) {
        res.status = BanOpStatus::INVALID_ARGUMENT;
        res.errorCode = "INVALID_ARGUMENT";
        res.errorMessage =
            "reason must be at most 500 printable characters without control characters";
        return res;
    }

    BanEntry candidate;
    candidate.banId = makeBanId(duration);
    candidate.targetType = targetType;
    candidate.duration = duration;
    candidate.reason = reason;
    candidate.source = "admin";
    candidate.createdBy = actorLabel.empty() ? "admin" : actorLabel;
    candidate.createdAt = nowSec;
    candidate.expiresAt =
        (duration == BanDuration::TEMP_24H) ? (nowSec + kAutoBanDurationSec) : 0;

    int64_t resolvedPlayerId = 0;
    int64_t resolvedRobotId = 0;

    if (targetType == BanTargetType::IP) {
        std::string parseErr;
        if (!parseIpOrCidr(rawTarget, candidate.targetValue, candidate.family,
                           candidate.networkBytes, candidate.prefixLen,
                           parseErr)) {
            res.status = (parseErr == "INVALID_CIDR_PREFIX")
                             ? BanOpStatus::INVALID_CIDR_PREFIX
                             : BanOpStatus::INVALID_BAN_TARGET;
            res.errorCode = parseErr;
            res.errorMessage = (parseErr == "INVALID_CIDR_PREFIX")
                                   ? "CIDR prefix out of allowed bounds (/8..32 IPv4, /16..128 IPv6)"
                                   : "invalid IP address or CIDR syntax";
            return res;
        }
        if (isProtectedIpOrCidr(candidate.family, candidate.networkBytes,
                                candidate.prefixLen)) {
            res.status = BanOpStatus::PROTECTED_TARGET;
            res.errorCode = "PROTECTED_TARGET";
            res.errorMessage =
                "cannot ban loopback, bind address, or configured trusted proxy";
            return res;
        }
        if (!callerRemoteIp.empty() &&
            cidrContainsIp(candidate.family, candidate.networkBytes,
                           candidate.prefixLen, callerRemoteIp)) {
            res.status = BanOpStatus::SELF_LOCKOUT_PREVENTED;
            res.errorCode = "SELF_LOCKOUT_PREVENTED";
            res.errorMessage =
                "cannot ban an IP or subnet matching your current connection IP";
            return res;
        }
        candidate.displayLabel = candidate.targetValue;
    } else if (targetType == BanTargetType::ACCOUNT) {
        const std::string trimmed = trimAscii(rawTarget);
        if (trimmed.empty()) {
            res.status = BanOpStatus::INVALID_BAN_TARGET;
            res.errorCode = "INVALID_BAN_TARGET";
            res.errorMessage = "missing account handle or UUID";
            return res;
        }
        PlayerProfile prof;
        bool found = store.getProfileByUuid(trimmed, prof);
        if (!found) {
            std::string h = trimmed;
            if (!h.empty() && h[0] == '@') {
                h = h.substr(1);
            }
            found = store.getProfileByHandle(h, prof);
        }
        if (!found) {
            res.status = BanOpStatus::NOT_FOUND;
            res.errorCode = "NOT_FOUND";
            res.errorMessage = "target account not found";
            return res;
        }
        if (prof.playerId == actorPlayerId || store.isPlayerAdmin(prof.playerId)) {
            res.status = BanOpStatus::SELF_LOCKOUT_PREVENTED;
            res.errorCode = "SELF_LOCKOUT_PREVENTED";
            res.errorMessage = "cannot ban yourself or an administrator account";
            return res;
        }
        resolvedPlayerId = prof.playerId;
        candidate.targetValue = prof.uuid;
        candidate.displayLabel = "@" + prof.handle;
    } else {
        const std::string trimmed = trimAscii(rawTarget);
        if (trimmed.empty()) {
            res.status = BanOpStatus::INVALID_BAN_TARGET;
            res.errorCode = "INVALID_BAN_TARGET";
            res.errorMessage = "missing robot name or UUID";
            return res;
        }
        RobotRecord rob;
        bool found = store.getRobotByUuid(trimmed, rob) ||
                     store.getRobotByName(trimmed, rob);
        if (!found) {
            res.status = BanOpStatus::NOT_FOUND;
            res.errorCode = "NOT_FOUND";
            res.errorMessage = "target robot not found";
            return res;
        }
        if (store.isPlayerAdmin(rob.ownerPlayerId)) {
            res.status = BanOpStatus::SELF_LOCKOUT_PREVENTED;
            res.errorCode = "SELF_LOCKOUT_PREVENTED";
            res.errorMessage = "cannot ban a robot owned by an administrator";
            return res;
        }
        resolvedRobotId = rob.robotId;
        candidate.targetValue = rob.robotUuid;
        candidate.displayLabel = rob.robotName;
    }

    bool removeFromTempDb = false;
    {
        std::lock_guard<std::mutex> wlock(_writeMutex);
        std::vector<BanEntry> nextPerm;
        bool permDirty = false;
        std::string cfgPath;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            cfgPath = _banConfigPath;
            nextPerm = _permBans;
            auto prevSize = nextPerm.size();
            nextPerm.erase(
                std::remove_if(nextPerm.begin(), nextPerm.end(),
                               [&](const BanEntry &e) {
                                   return e.targetType == candidate.targetType &&
                                          e.targetValue == candidate.targetValue;
                               }),
                nextPerm.end());
            if (nextPerm.size() != prevSize) {
                permDirty = true;
            }
            if (duration == BanDuration::PERMANENT) {
                nextPerm.push_back(candidate);
                permDirty = true;
            }
        }

        if (permDirty) {
            std::string writeErr;
            if (!writePermanentConfigAtomic(cfgPath, nowSec, nextPerm, writeErr)) {
                res.status = BanOpStatus::IO_ERROR;
                res.errorCode = "INTERNAL_ERROR";
                res.errorMessage = writeErr;
                return res;
            }
        }

        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (permDirty) {
                _permBans = std::move(nextPerm);
            }
            auto prevTempSize = _tempBans.size();
            _tempBans.erase(
                std::remove_if(_tempBans.begin(), _tempBans.end(),
                               [&](const BanEntry &e) {
                                   return e.targetType == candidate.targetType &&
                                          e.targetValue == candidate.targetValue;
                               }),
                _tempBans.end());
            if (_tempBans.size() != prevTempSize) {
                removeFromTempDb = true;
            }
            if (duration == BanDuration::TEMP_24H) {
                _tempBans.push_back(candidate);
            }
        }
    }

    if (duration == BanDuration::TEMP_24H) {
        TempBanRow row;
        row.banId = candidate.banId;
        row.targetType = toString(candidate.targetType);
        row.targetValue = candidate.targetValue;
        row.displayLabel = candidate.displayLabel;
        row.reason = candidate.reason;
        row.source = "admin";
        row.createdBy = candidate.createdBy;
        row.createdAt = candidate.createdAt;
        row.expiresAt = candidate.expiresAt;
        store.upsertTempBan(row);
    } else if (removeFromTempDb) {
        store.deleteTempBanByTarget(toString(candidate.targetType),
                                    candidate.targetValue);
    }

    if (resolvedPlayerId > 0) {
        store.setBanned(resolvedPlayerId, true);
        store.revokeAllSessions(resolvedPlayerId);
        res.sideEffects.bannedPlayerIds.push_back(resolvedPlayerId);
        std::vector<RobotRecord> ownedRobots;
        if (store.listRobots(resolvedPlayerId, ownedRobots)) {
            for (size_t i = 0; i < ownedRobots.size(); ++i) {
                res.sideEffects.bannedRobotIds.push_back(ownedRobots[i].robotId);
            }
        }
    }
    if (resolvedRobotId > 0) {
        res.sideEffects.bannedRobotIds.push_back(resolvedRobotId);
    }
    res.sideEffects.changed = true;

    store.appendBanAuditLog(
        nowSec,
        candidate.createdBy,
        duration == BanDuration::PERMANENT ? "add_perm" : "add_24h",
        toString(candidate.targetType),
        candidate.targetValue,
        candidate.reason);

    res.status = BanOpStatus::OK;
    res.entry = std::move(candidate);
    return res;
}

BanOpResult BanManager::moveBans(AccountStore &store,
                                 const std::vector<std::string> &banIds,
                                 BanDuration targetDuration,
                                 const std::string &actorLabel,
                                 int64_t nowSec)
{
    BanOpResult res;
    if (banIds.empty()) {
        res.status = BanOpStatus::INVALID_ARGUMENT;
        res.errorCode = "INVALID_ARGUMENT";
        res.errorMessage = "ban_ids must not be empty";
        return res;
    }

    const std::unordered_set<std::string> idSet(banIds.begin(), banIds.end());
    std::vector<TempBanRow> tempUpserts;
    std::vector<std::pair<std::string, std::string> > tempDeletes;
    std::vector<BanAuditLogRow> audits;
    std::vector<std::string> ensureBannedUuids;

    {
        std::lock_guard<std::mutex> wlock(_writeMutex);
        std::vector<BanEntry> nextTemp;
        std::vector<BanEntry> nextPerm;
        bool permDirty = false;
        bool matchedAny = false;
        std::string cfgPath;

        {
            std::lock_guard<std::mutex> lock(_mutex);
            cfgPath = _banConfigPath;
            nextTemp = _tempBans;
            nextPerm = _permBans;

            if (targetDuration == BanDuration::PERMANENT) {
                std::vector<BanEntry> keptTemp;
                for (size_t i = 0; i < nextTemp.size(); ++i) {
                    const BanEntry &e = nextTemp[i];
                    if (idSet.count(e.banId) == 0) {
                        keptTemp.push_back(e);
                        continue;
                    }
                    matchedAny = true;
                    nextPerm.erase(
                        std::remove_if(nextPerm.begin(), nextPerm.end(),
                                       [&](const BanEntry &p) {
                                           return p.targetType == e.targetType &&
                                                  p.targetValue == e.targetValue;
                                       }),
                        nextPerm.end());
                    BanEntry promoted = e;
                    promoted.banId = makeBanId(BanDuration::PERMANENT);
                    promoted.duration = BanDuration::PERMANENT;
                    promoted.source = "admin";
                    promoted.createdBy = actorLabel.empty() ? "admin" : actorLabel;
                    promoted.createdAt = nowSec;
                    promoted.expiresAt = 0;
                    nextPerm.push_back(promoted);
                    permDirty = true;

                    tempDeletes.push_back(
                        std::make_pair(std::string(toString(e.targetType)),
                                       e.targetValue));
                    if (e.targetType == BanTargetType::ACCOUNT) {
                        ensureBannedUuids.push_back(e.targetValue);
                    }
                    BanAuditLogRow audit;
                    audit.createdAt = nowSec;
                    audit.actorLabel = promoted.createdBy;
                    audit.action = "move_to_perm";
                    audit.targetType = toString(e.targetType);
                    audit.targetValue = e.targetValue;
                    audit.reason = e.reason;
                    audits.push_back(std::move(audit));
                }
                for (size_t i = 0; i < nextPerm.size() && !matchedAny; ++i) {
                    if (idSet.count(nextPerm[i].banId) != 0) {
                        matchedAny = true;
                    }
                }
                nextTemp = std::move(keptTemp);
            } else {
                std::vector<BanEntry> keptPerm;
                for (size_t i = 0; i < nextPerm.size(); ++i) {
                    const BanEntry &e = nextPerm[i];
                    if (idSet.count(e.banId) == 0) {
                        keptPerm.push_back(e);
                        continue;
                    }
                    matchedAny = true;
                    permDirty = true;
                    nextTemp.erase(
                        std::remove_if(nextTemp.begin(), nextTemp.end(),
                                       [&](const BanEntry &t) {
                                           return t.targetType == e.targetType &&
                                                  t.targetValue == e.targetValue;
                                       }),
                        nextTemp.end());
                    BanEntry demoted = e;
                    demoted.banId = makeBanId(BanDuration::TEMP_24H);
                    demoted.duration = BanDuration::TEMP_24H;
                    demoted.source = "admin";
                    demoted.createdBy = actorLabel.empty() ? "admin" : actorLabel;
                    demoted.createdAt = nowSec;
                    demoted.expiresAt = nowSec + kAutoBanDurationSec;
                    nextTemp.push_back(demoted);

                    TempBanRow row;
                    row.banId = demoted.banId;
                    row.targetType = toString(demoted.targetType);
                    row.targetValue = demoted.targetValue;
                    row.displayLabel = demoted.displayLabel;
                    row.reason = demoted.reason;
                    row.source = "admin";
                    row.createdBy = demoted.createdBy;
                    row.createdAt = demoted.createdAt;
                    row.expiresAt = demoted.expiresAt;
                    tempUpserts.push_back(std::move(row));

                    BanAuditLogRow audit;
                    audit.createdAt = nowSec;
                    audit.actorLabel = demoted.createdBy;
                    audit.action = "move_to_24h";
                    audit.targetType = toString(e.targetType);
                    audit.targetValue = e.targetValue;
                    audit.reason = e.reason;
                    audits.push_back(std::move(audit));
                }
                for (size_t i = 0; i < nextTemp.size() && !matchedAny; ++i) {
                    if (idSet.count(nextTemp[i].banId) != 0) {
                        matchedAny = true;
                        nextTemp[i].expiresAt = nowSec + kAutoBanDurationSec;
                        TempBanRow row;
                        row.banId = nextTemp[i].banId;
                        row.targetType = toString(nextTemp[i].targetType);
                        row.targetValue = nextTemp[i].targetValue;
                        row.displayLabel = nextTemp[i].displayLabel;
                        row.reason = nextTemp[i].reason;
                        row.source = nextTemp[i].source;
                        row.createdBy = nextTemp[i].createdBy;
                        row.createdAt = nextTemp[i].createdAt;
                        row.expiresAt = nextTemp[i].expiresAt;
                        tempUpserts.push_back(std::move(row));
                    }
                }
                nextPerm = std::move(keptPerm);
            }
        }

        if (!matchedAny) {
            res.status = BanOpStatus::NOT_FOUND;
            res.errorCode = "NOT_FOUND";
            res.errorMessage = "no matching ban entry found";
            return res;
        }

        if (permDirty) {
            std::string writeErr;
            if (!writePermanentConfigAtomic(cfgPath, nowSec, nextPerm, writeErr)) {
                res.status = BanOpStatus::IO_ERROR;
                res.errorCode = "INTERNAL_ERROR";
                res.errorMessage = writeErr;
                return res;
            }
        }

        {
            std::lock_guard<std::mutex> lock(_mutex);
            _tempBans = std::move(nextTemp);
            if (permDirty) {
                _permBans = std::move(nextPerm);
            }
        }
    }

    for (size_t i = 0; i < tempDeletes.size(); ++i) {
        store.deleteTempBanByTarget(tempDeletes[i].first, tempDeletes[i].second);
    }
    for (size_t i = 0; i < tempUpserts.size(); ++i) {
        store.upsertTempBan(tempUpserts[i]);
    }
    for (size_t i = 0; i < ensureBannedUuids.size(); ++i) {
        PlayerProfile prof;
        if (store.getProfileByUuid(ensureBannedUuids[i], prof)) {
            store.setBanned(prof.playerId, true);
        }
    }
    for (size_t i = 0; i < audits.size(); ++i) {
        store.appendBanAuditLog(audits[i].createdAt, audits[i].actorLabel,
                                audits[i].action, audits[i].targetType,
                                audits[i].targetValue, audits[i].reason);
    }

    res.sideEffects.changed = true;
    res.status = BanOpStatus::OK;
    return res;
}

BanOpResult BanManager::removeBans(AccountStore &store,
                                   const std::vector<std::string> &banIds,
                                   const std::string &actorLabel,
                                   int64_t nowSec)
{
    BanOpResult res;
    if (banIds.empty()) {
        res.status = BanOpStatus::INVALID_ARGUMENT;
        res.errorCode = "INVALID_ARGUMENT";
        res.errorMessage = "ban_ids must not be empty";
        return res;
    }

    const std::unordered_set<std::string> idSet(banIds.begin(), banIds.end());
    std::vector<std::string> deletedTempIds;
    std::vector<std::string> maybeUnbannedAccountUuids;
    std::vector<BanAuditLogRow> audits;

    {
        std::lock_guard<std::mutex> wlock(_writeMutex);
        std::vector<BanEntry> nextTemp;
        std::vector<BanEntry> nextPerm;
        bool permDirty = false;
        bool matchedAny = false;
        std::string cfgPath;

        {
            std::lock_guard<std::mutex> lock(_mutex);
            cfgPath = _banConfigPath;

            for (size_t i = 0; i < _tempBans.size(); ++i) {
                const BanEntry &e = _tempBans[i];
                if (idSet.count(e.banId) == 0) {
                    nextTemp.push_back(e);
                    continue;
                }
                matchedAny = true;
                deletedTempIds.push_back(e.banId);
                if (e.targetType == BanTargetType::ACCOUNT) {
                    maybeUnbannedAccountUuids.push_back(e.targetValue);
                }
                BanAuditLogRow audit;
                audit.createdAt = nowSec;
                audit.actorLabel = actorLabel.empty() ? "admin" : actorLabel;
                audit.action = "remove";
                audit.targetType = toString(e.targetType);
                audit.targetValue = e.targetValue;
                audit.reason = e.reason;
                audits.push_back(std::move(audit));
            }

            for (size_t i = 0; i < _permBans.size(); ++i) {
                const BanEntry &e = _permBans[i];
                if (idSet.count(e.banId) == 0) {
                    nextPerm.push_back(e);
                    continue;
                }
                matchedAny = true;
                permDirty = true;
                if (e.targetType == BanTargetType::ACCOUNT) {
                    maybeUnbannedAccountUuids.push_back(e.targetValue);
                }
                BanAuditLogRow audit;
                audit.createdAt = nowSec;
                audit.actorLabel = actorLabel.empty() ? "admin" : actorLabel;
                audit.action = "remove";
                audit.targetType = toString(e.targetType);
                audit.targetValue = e.targetValue;
                audit.reason = e.reason;
                audits.push_back(std::move(audit));
            }
        }

        if (!matchedAny) {
            res.status = BanOpStatus::NOT_FOUND;
            res.errorCode = "NOT_FOUND";
            res.errorMessage = "no matching ban entry found";
            return res;
        }

        if (permDirty) {
            std::string writeErr;
            if (!writePermanentConfigAtomic(cfgPath, nowSec, nextPerm, writeErr)) {
                res.status = BanOpStatus::IO_ERROR;
                res.errorCode = "INTERNAL_ERROR";
                res.errorMessage = writeErr;
                return res;
            }
        }

        std::vector<std::string> trulyUnbannedUuids;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _tempBans = std::move(nextTemp);
            if (permDirty) {
                _permBans = std::move(nextPerm);
            }
            for (size_t i = 0; i < maybeUnbannedAccountUuids.size(); ++i) {
                const std::string &u = maybeUnbannedAccountUuids[i];
                bool stillBanned = false;
                for (size_t k = 0; k < _permBans.size(); ++k) {
                    if (_permBans[k].targetType == BanTargetType::ACCOUNT &&
                        _permBans[k].targetValue == u) {
                        stillBanned = true;
                        break;
                    }
                }
                for (size_t k = 0; !stillBanned && k < _tempBans.size(); ++k) {
                    if (_tempBans[k].targetType == BanTargetType::ACCOUNT &&
                        nowSec < _tempBans[k].expiresAt &&
                        _tempBans[k].targetValue == u) {
                        stillBanned = true;
                        break;
                    }
                }
                if (!stillBanned) {
                    trulyUnbannedUuids.push_back(u);
                }
            }
        }
        maybeUnbannedAccountUuids = std::move(trulyUnbannedUuids);
    }

    for (size_t i = 0; i < deletedTempIds.size(); ++i) {
        store.deleteTempBanById(deletedTempIds[i]);
    }
    for (size_t i = 0; i < maybeUnbannedAccountUuids.size(); ++i) {
        PlayerProfile prof;
        if (store.getProfileByUuid(maybeUnbannedAccountUuids[i], prof)) {
            store.setBanned(prof.playerId, false);
            res.sideEffects.unbannedPlayerIds.push_back(prof.playerId);
        }
    }
    for (size_t i = 0; i < audits.size(); ++i) {
        store.appendBanAuditLog(audits[i].createdAt, audits[i].actorLabel,
                                audits[i].action, audits[i].targetType,
                                audits[i].targetValue, audits[i].reason);
    }

    res.sideEffects.changed = true;
    res.status = BanOpStatus::OK;
    return res;
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
