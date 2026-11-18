/*
 * BanManager.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_SERVER_BAN_MANAGER_HXX
#define OCTI_SERVER_BAN_MANAGER_HXX

#include "AccountStore.hxx"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace octi {

enum class BanTargetType {
    IP,
    ACCOUNT,
    ROBOT
};

enum class BanDuration {
    TEMP_24H,
    PERMANENT
};

enum class AbusePattern {
    RATE_LIMIT_429,
    AUTH_OR_PASSCODE_BRUTE_FORCE,
    CRAWLER_SCRAPE,
    EXPLOIT_PROBE
};

const char *toString(BanTargetType t);
bool parseBanTargetType(const std::string &s, BanTargetType &out);

const char *toString(BanDuration d);
bool parseBanDuration(const std::string &s, BanDuration &out);

const char *toString(AbusePattern p);

struct BanThresholds {
    int rateLimit429In60s = 40;
    int rateLimit429In300s = 100;
    int authFailuresIn300s = 60;
    int passcodeFailuresIn300s = 25;
    int crawlerUaIn300s = 5;
    int anonEnumPathsIn300s = 120;
    int exploitProbesIn300s = 20;
};

struct BanEntry {
    std::string banId;
    BanTargetType targetType = BanTargetType::IP;
    BanDuration duration = BanDuration::TEMP_24H;
    std::string targetValue;        // canonical CIDR, player_uuid, or robot_uuid
    std::string displayLabel;       // CIDR, @handle, or robot_name
    std::string reason;
    std::string source = "admin";   // "auto" | "admin"
    std::string createdBy = "system";
    int64_t createdAt = 0;
    int64_t expiresAt = 0;          // 0 for permanent
    int family = 0;                 // AF_INET or AF_INET6 when targetType == IP
    std::array<uint8_t, 16> networkBytes{};
    int prefixLen = 0;
};

struct BanMatch {
    bool matched = false;
    std::string banId;
    BanTargetType targetType = BanTargetType::IP;
    BanDuration duration = BanDuration::TEMP_24H;
    std::string targetValue;
    std::string reason;
    int64_t expiresAt = 0;
};

struct AbuseActorContext {
    std::string remoteIp;
    int64_t playerId = 0;           // human player_id (> 0) when authenticated
    std::string playerUuid;
    std::string playerHandle;
    int64_t robotId = 0;            // robot_id (> 0) when authenticated as bot
    std::string robotUuid;
    std::string robotName;
    int64_t ownerPlayerId = 0;
    bool isAdmin = false;           // human admin or robot owned by admin
};

struct BanSideEffects {
    bool changed = false;
    std::vector<int64_t> bannedPlayerIds;
    std::vector<int64_t> bannedRobotIds;
    std::vector<int64_t> unbannedPlayerIds;
};

enum class BanOpStatus {
    OK = 0,
    INVALID_BAN_TARGET,
    INVALID_CIDR_PREFIX,
    INVALID_ARGUMENT,
    PROTECTED_TARGET,
    SELF_LOCKOUT_PREVENTED,
    NOT_FOUND,
    IO_ERROR
};

struct BanOpResult {
    BanOpStatus status = BanOpStatus::IO_ERROR;
    std::string errorCode;
    std::string errorMessage;
    BanEntry entry;
    BanSideEffects sideEffects;
};

/*
 * Unified BanManager enforcing:
 *   1. SQLite-persisted 24-hour bans (temp_bans) + AbusePatternDetector
 *      mandatory 86400s auto-escalation.
 *   2. Separate crash-safe 0640 permanent ban config file (--ban-config,
 *      /etc/octi/bans.json) for IPv4/IPv6 CIDRs, Accounts, and Robots.
 *   3. Hard anti-lockout guards protecting 127.0.0.0/8, ::1/128,
 *      bindAddress, trustedProxies, and administrator accounts/robots/IPs.
 *
 * Lock hierarchy invariant: _writeMutex -> _mutex are strict leaf locks.
 * Neither lock is ever held while calling AccountStore or GameRoomManager.
 */
class BanManager {
public:
    static constexpr int64_t kAutoBanDurationSec = 86400;
    static constexpr int64_t kAbuseHorizonSec = 300;
    static constexpr int64_t kShortRateWindowSec = 60;
    static constexpr size_t kMaxTrackedKeys = 16384;
    static constexpr size_t kMaxBanConfigBytes = 4 * 1024 * 1024;
    static constexpr size_t kMaxReasonBytes = 500;
    static constexpr int64_t kRecentAdminIpTtlSec = 3600;

    BanManager();
    ~BanManager();

    BanManager(const BanManager &) = delete;
    BanManager &operator=(const BanManager &) = delete;

    void configure(const std::string &banConfigPath,
                   const std::string &bindAddress,
                   const std::vector<std::string> &trustedProxies,
                   const BanThresholds &thresholds = BanThresholds());

    const std::string &banConfigPath() const;

    /*
     * Parse and normalize an IPv4/IPv6 literal or CIDR string.
     *   - Default prefix if omitted: /32 (IPv4) or /128 (IPv6).
     *   - ::ffff:a.b.c.d is normalized to AF_INET (prefix 96..128 -> -96).
     *   - Rejects syntax errors with outError = "INVALID_BAN_TARGET".
     *   - Rejects IPv4 prefix < 8 or > 32 and IPv6 prefix < 16 or > 128
     *     with outError = "INVALID_CIDR_PREFIX".
     *   - Zeroes host bits beyond outPrefixLen in outNetworkBytes and
     *     formats outCanonicalCidr (e.g. "198.51.100.0/24").
     */
    static bool parseIpOrCidr(const std::string &input,
                              std::string &outCanonicalCidr,
                              int &outFamily,
                              std::array<uint8_t, 16> &outNetworkBytes,
                              int &outPrefixLen,
                              std::string &outError);

    static bool cidrContainsIp(int cidrFamily,
                               const std::array<uint8_t, 16> &cidrNetworkBytes,
                               int cidrPrefixLen,
                               const std::string &rawIp);

    static bool cidrsOverlap(int famA,
                             const std::array<uint8_t, 16> &netA,
                             int prefA,
                             int famB,
                             const std::array<uint8_t, 16> &netB,
                             int prefB);

    bool isProtectedIpOrCidr(int family,
                             const std::array<uint8_t, 16> &networkBytes,
                             int prefixLen) const;
    bool isProtectedIpOrCidr(const std::string &ipOrCidr) const;

    /* Load permanent bans from --ban-config (if present) and active temp_bans from SQLite. */
    bool loadFromStorage(AccountStore &store, int64_t nowSec,
                         std::string &error);

    /* Fast in-memory O(1) / O(prefix_count) enforcement lookups. */
    bool matchIp(const std::string &rawIp, int64_t nowSec,
                 BanMatch &outMatch) const;
    bool matchAccount(const std::string &playerUuid, int64_t nowSec,
                      BanMatch &outMatch) const;
    bool matchRobot(const std::string &robotUuid, int64_t nowSec,
                    BanMatch &outMatch) const;

    /* Record verified admin traffic IP to prevent shared-IP auto-ban lockout. */
    void noteVerifiedAdminIp(const std::string &rawIp, int64_t nowSec);

    /* AbusePatternDetector sliding-window event feeds (escalate to 86400s auto-ban). */
    BanSideEffects recordRateLimit429(AccountStore &store,
                                      const AbuseActorContext &ctx,
                                      int64_t nowSec);
    BanSideEffects recordAuthFailure(AccountStore &store,
                                     const AbuseActorContext &ctx,
                                     int64_t nowSec);
    BanSideEffects recordPasscodeFailure(AccountStore &store,
                                         const AbuseActorContext &ctx,
                                         int64_t nowSec);
    BanSideEffects recordCrawlerUaHit(AccountStore &store,
                                      const std::string &remoteIp,
                                      int64_t nowSec);
    BanSideEffects recordAnonymousEnumeration(AccountStore &store,
                                              const std::string &remoteIp,
                                              const std::string &path,
                                              int httpStatus,
                                              int64_t nowSec);
    BanSideEffects recordExploitProbe(AccountStore &store,
                                      const AbuseActorContext &ctx,
                                      int64_t nowSec);

    /* Purge expired 24-hour bans in memory and SQLite. */
    int purgeExpired(AccountStore &store, int64_t nowSec);

    /* Admin CRUD & move operations. */
    void listBans(AccountStore &store, int64_t nowSec,
                  std::vector<BanEntry> &outTempBans,
                  std::vector<BanEntry> &outPermBans);

    BanOpResult addBan(AccountStore &store,
                       BanTargetType targetType,
                       const std::string &rawTarget,
                       BanDuration duration,
                       const std::string &rawReason,
                       int64_t actorPlayerId,
                       const std::string &actorLabel,
                       const std::string &callerRemoteIp,
                       int64_t nowSec);

    BanOpResult moveBans(AccountStore &store,
                         const std::vector<std::string> &banIds,
                         BanDuration targetDuration,
                         const std::string &actorLabel,
                         int64_t nowSec);

    BanOpResult removeBans(AccountStore &store,
                           const std::vector<std::string> &banIds,
                           const std::string &actorLabel,
                           int64_t nowSec);

private:
    struct ProtectedCidr {
        int family = 0;
        std::array<uint8_t, 16> networkBytes{};
        int prefixLen = 0;
    };

    struct SlidingBucket {
        std::deque<int64_t> eventsSec;
        int64_t lastTouchedSec = 0;
    };

    struct AnonEnumBucket {
        std::deque<std::pair<int64_t, std::string> > hits;
        int64_t lastTouchedSec = 0;
    };

    bool isProtectedLocked(int family,
                           const std::array<uint8_t, 16> &networkBytes,
                           int prefixLen) const;
    bool hasRecentAdminIpLocked(const std::string &normIp, int64_t nowSec);
    SlidingBucket &touchBucketLocked(const std::string &key, int64_t nowSec);
    int recordWindowEventLocked(const std::string &key, int64_t windowSec,
                                int64_t nowSec);

    BanSideEffects triggerAutoBan(AccountStore &store,
                                  const AbuseActorContext &ctx,
                                  AbusePattern pattern,
                                  int count,
                                  int64_t windowSec,
                                  int64_t nowSec);

    static bool writePermanentConfigAtomic(const std::string &path,
                                           int64_t updatedAt,
                                           const std::vector<BanEntry> &entries,
                                           std::string &error);

    static std::string makeBanId(BanDuration d);
    static bool cleanReason(const std::string &raw, std::string &out);

    mutable std::mutex _writeMutex;
    mutable std::mutex _mutex;

    std::string _banConfigPath;
    BanThresholds _thresholds;
    std::vector<ProtectedCidr> _protectedCidrs;

    std::vector<BanEntry> _tempBans;
    std::vector<BanEntry> _permBans;

    std::unordered_map<std::string, SlidingBucket> _eventWindows;
    std::unordered_map<std::string, AnonEnumBucket> _anonEnumWindows;
    std::unordered_map<std::string, int64_t> _recentAdminIps;

    static std::atomic<uint64_t> _tmpFileSeq;
};

} // namespace octi

#endif // OCTI_SERVER_BAN_MANAGER_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
