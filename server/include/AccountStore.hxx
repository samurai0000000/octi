/*
 * AccountStore.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_SERVER_ACCOUNT_STORE_HXX
#define OCTI_SERVER_ACCOUNT_STORE_HXX

#include "OctiDefs.hxx"
#include "OidcVerifier.hxx"

#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

struct sqlite3;

namespace octi {

#ifndef OCTI_VARIANT_CODE_HELPERS_DEFINED
#define OCTI_VARIANT_CODE_HELPERS_DEFINED

inline std::vector<std::string> allOctiVariantCodes()
{
    return {
        "2P_3B",    "2P_3B_SP", "2P_3B_ED", "2P_3B_SPED",
        "2P_1B",    "2P_1B_SP", "2P_1B_ED", "2P_1B_SPED",
        "4P",       "4P_SP",    "4P_ED",    "4P_SPED",
        "OFK"
    };
}

inline std::string octiVariantCode(OctiGameMode mode, int bases,
                                   bool superProngs, bool edgeless)
{
    if (mode == GAME_MODE_OFK) {
        return "OFK";
    }
    std::string code;
    if (mode == GAME_MODE_4) {
        code = "4P";
    } else {
        code = (bases == 1) ? "2P_1B" : "2P_3B";
    }
    if (superProngs && edgeless) {
        code += "_SPED";
    } else if (superProngs) {
        code += "_SP";
    } else if (edgeless) {
        code += "_ED";
    }
    return code;
}

inline std::string octiVariantCode(OctiGameMode mode, OctiGameBases bases,
                                   bool superProngs, bool edgeless)
{
    return octiVariantCode(mode, static_cast<int>(bases), superProngs, edgeless);
}

inline bool parseOctiVariantCode(const std::string &code,
                                 OctiGameMode &modeOut,
                                 int &basesOut,
                                 bool &superProngsOut,
                                 bool &edgelessOut)
{
    std::string u = code;
    for (size_t i = 0; i < u.size(); ++i) {
        if (u[i] >= 'a' && u[i] <= 'z') {
            u[i] = static_cast<char>(u[i] - 'a' + 'A');
        }
    }
    if (u == "OFK") {
        modeOut = GAME_MODE_OFK;
        basesOut = 1;
        superProngsOut = false;
        edgelessOut = false;
        return true;
    }
    if (u == "4P" || u == "4P_SP" || u == "4P_ED" || u == "4P_SPED") {
        modeOut = GAME_MODE_4;
        basesOut = 3;
        superProngsOut = (u == "4P_SP" || u == "4P_SPED");
        edgelessOut = (u == "4P_ED" || u == "4P_SPED");
        return true;
    }
    if (u == "2P_3B" || u == "2P_3B_SP" || u == "2P_3B_ED" || u == "2P_3B_SPED") {
        modeOut = GAME_MODE_2;
        basesOut = 3;
        superProngsOut = (u == "2P_3B_SP" || u == "2P_3B_SPED");
        edgelessOut = (u == "2P_3B_ED" || u == "2P_3B_SPED");
        return true;
    }
    if (u == "2P_1B" || u == "2P_1B_SP" || u == "2P_1B_ED" || u == "2P_1B_SPED") {
        modeOut = GAME_MODE_2;
        basesOut = 1;
        superProngsOut = (u == "2P_1B_SP" || u == "2P_1B_SPED");
        edgelessOut = (u == "2P_1B_ED" || u == "2P_1B_SPED");
        return true;
    }
    return false;
}

inline bool parseOctiVariantCode(const std::string &code,
                                 OctiGameMode &modeOut,
                                 OctiGameBases &basesOut,
                                 bool &superProngsOut,
                                 bool &edgelessOut)
{
    int b = 3;
    if (!parseOctiVariantCode(code, modeOut, b, superProngsOut, edgelessOut)) {
        return false;
    }
    basesOut = (b == 1) ? GAME_BASES_1 : GAME_BASES_3;
    return true;
}

inline bool isValidOctiVariantCode(const std::string &code)
{
    OctiGameMode m = GAME_MODE_2;
    int b = 3;
    bool sp = false;
    bool ed = false;
    return parseOctiVariantCode(code, m, b, sp, ed);
}

inline std::string octiVariantCodeFromModeStr(const std::string &modeStr,
                                              int bases,
                                              bool superProngs,
                                              bool edgeless)
{
    std::string u = modeStr;
    for (size_t i = 0; i < u.size(); ++i) {
        if (u[i] >= 'a' && u[i] <= 'z') {
            u[i] = static_cast<char>(u[i] - 'a' + 'A');
        }
    }
    if (u == "OFK") {
        return "OFK";
    }
    if (u == "4P" || u == "4") {
        return octiVariantCode(GAME_MODE_4, bases, superProngs, edgeless);
    }
    if (u == "2P" || u == "2" || u.empty()) {
        return octiVariantCode(GAME_MODE_2, bases, superProngs, edgeless);
    }
    OctiGameMode pm = GAME_MODE_2;
    int pb = 3;
    bool psp = false;
    bool ped = false;
    if (parseOctiVariantCode(u, pm, pb, psp, ped)) {
        return octiVariantCode(pm, pb, psp, ped);
    }
    return octiVariantCode(GAME_MODE_2, bases, superProngs, edgeless);
}

inline bool isOfficialRatedVariant(OctiGameMode mode, int bases,
                                   bool superProngs, bool edgeless)
{
    std::string v = octiVariantCode(mode, bases, superProngs, edgeless);
    return v == "2P_3B" || v == "2P_1B" || v == "OFK";
}

inline bool isOfficialRatedVariant(OctiGameMode mode, OctiGameBases bases,
                                   bool superProngs, bool edgeless)
{
    return isOfficialRatedVariant(mode, static_cast<int>(bases),
                                  superProngs, edgeless);
}

inline bool isOfficialRatedVariantCode(const std::string &code)
{
    OctiGameMode m = GAME_MODE_2;
    int b = 3;
    bool sp = false;
    bool ed = false;
    if (!parseOctiVariantCode(code, m, b, sp, ed)) {
        return false;
    }
    return isOfficialRatedVariant(m, b, sp, ed);
}

#endif // OCTI_VARIANT_CODE_HELPERS_DEFINED

bool transcodeWebpToPng(const std::string &webpBytes, std::string &outPngBytes);

/* Public subset of a profile, safe to show to another player. */
struct PublicProfile {
    std::string uuid;
    std::string handle;
    int elo2p = 1200;
    int gamesPlayed = 0;
};

struct PlayerProfile {
    int64_t playerId = 0;
    std::string uuid;
    std::string handle;
    std::string displayName;
    std::string avatarUrl;
    std::string bio;
    std::string location;
    bool banned = false;
    bool isAdmin = false;
    int elo2p = 1200;
    int elo2p3b = 1200;
    int elo2p1b = 1200;
    int elo4p = 1200;
    int eloOfk = 1200;
    int games2p3b = 0;
    int games2p1b = 0;
    int gamesOfk = 0;
    int gamesPlayed = 0;
    int wins = 0;
    int losses = 0;
    int draws = 0;
    int arenaElo2p3b = 1200;
    int arenaElo2p1b = 1200;
    int arenaEloOfk = 1200;
    int arenaGames2p3b = 0;
    int arenaGames2p1b = 0;
    int arenaGamesOfk = 0;
    int arenaGamesPlayed = 0;
    int arenaWins = 0;
    int arenaLosses = 0;
    int arenaDraws = 0;
    int64_t createdAt = 0;
    int64_t lastSeenAt = 0;
    int64_t handleChangedAt = 0;
};

struct LinkedIdentity {
    std::string provider;
    std::string providerSub;
    std::string providerEmail;
    std::string displayHint;
    int64_t linkedAt = 0;
    int64_t lastLoginAt = 0;
};

struct LoginResult {
    bool ok = false;
    std::string error;
    bool created = false;               // true when a new player was provisioned
    PlayerProfile profile;
    std::vector<LinkedIdentity> identities;
    std::string sessionToken;           // raw bearer token, returned exactly once
    int64_t sessionExpiresAt = 0;
};

enum class LinkStatus {
    OK = 0,
    LINKED = OK,
    ALREADY_LINKED_SELF,
    CONFLICT_ALREADY_LINKED,
    CONFLICT_OTHER_ACCOUNT = CONFLICT_ALREADY_LINKED,
    PLAYER_NOT_FOUND,
    INVALID_CLAIMS,
    ERROR
};

struct LinkResult {
    LinkStatus status = LinkStatus::ERROR;
    bool ok = false;
    PublicProfile conflictProfile;      // set for CONFLICT_ALREADY_LINKED
    int64_t conflictingPlayerId = 0;
    std::string conflictingPlayerUuid;
    std::string conflictingHandle;
    std::string error;
};

enum class UnlinkStatus {
    UNLINKED,
    LAST_IDENTITY_FORBIDDEN,
    NOT_LINKED,
    ERROR
};

struct SessionValidation {
    bool valid = false;
    int64_t playerId = 0;
    int64_t expiresAt = 0;
    std::string reason;                 // 'unknown' | 'revoked' | 'expired' | 'malformed'
};

struct ProfileUpdate {
    bool setHandle = false;
    std::string handle;
    bool setDisplayName = false;
    std::string displayName;
    bool setAvatarUrl = false;
    std::string avatarUrl;
    bool setBio = false;
    std::string bio;
    bool setLocation = false;
    std::string location;
};

enum class UpdateStatus {
    OK,
    NOT_FOUND,
    INVALID_HANDLE,
    HANDLE_TAKEN,
    HANDLE_RATE_LIMITED,
    INVALID_DISPLAY_NAME,
    INVALID_AVATAR_URL,
    INVALID_BIO,
    INVALID_LOCATION,
    ERROR
};

enum class GameOutcome { WIN, LOSS, DRAW };

/* New absolute rating and outcome for one participant of a finished game. */
struct RatingUpdate {
    int64_t playerId = 0;
    int64_t robotId = 0;
    int newElo = 1200;
    GameOutcome outcome = GameOutcome::DRAW;
};

struct RobotOutcomeUpdate {
    int64_t robotId = 0;
    GameOutcome outcome = GameOutcome::DRAW;
};

struct LeaderboardEntry {
    int rank = 0;
    int ladderGames = 0;
    std::string uuid;
    std::string handle;
    std::string displayName;
    std::string avatarUrl;
    int elo = 1200;
    int gamesPlayed = 0;
    int wins = 0;
    int losses = 0;
    int draws = 0;
};

/* Archived game row. listArchivedGames() leaves octiTranscript empty. */
struct GameRecord {
    std::string gameId;
    std::string gameMode;               // '2P' | '4P' | 'OFK'
    int gameBases = 1;                  // 1 or 3
    bool superProngs = false;
    bool edgeless = false;
    std::string timeControl = "untimed";
    int64_t p1PlayerId = 0;             // 0 = no registered player (NULL)
    int64_t p2PlayerId = 0;
    int64_t p3PlayerId = 0;
    int64_t p4PlayerId = 0;
    int64_t p1RobotId = 0;
    int64_t p2RobotId = 0;
    int64_t p3RobotId = 0;
    int64_t p4RobotId = 0;
    std::string p1Handle;
    std::string p2Handle;
    std::string p3Handle;
    std::string p4Handle;
    std::string result;                 // 'P1_WIN' | 'P2_WIN' | 'TEAM_13_WIN' | 'TEAM_24_WIN' | 'DRAW'
    int winnerSeat = 0;                 // 0 = draw/none, 1..4
    std::string termination;
    int plyCount = 0;
    std::string octiTranscript;
    std::string finalFen;
    int64_t startedAt = 0;
    int64_t endedAt = 0;
    std::string source = "online";      // 'online' | 'client_upload'
    int64_t uploadedAt = 0;             // server clock set on receipt
    std::string winner = "NONE";        // 'A' | 'B' | 'C' | 'D' | 'NONE'
    std::string status = "GAME_OVER";   // 'GAME_OVER' | 'RESIGNED'
    int rated = 0;                      // 0 or 1
    int eloPool = 0;                    // 0 = unrated, 1 = Human-Only, 2 = Arena
    int aiDifficulty = 0;               // 0 or 1..5
    std::string appVersion;
    std::string platform;
    std::string installId;              // Internal only, never exposed in public responses
};

enum class ArchiveStatus {
    OK,
    DUPLICATE_ID,
    INVALID,
    ERROR,
    STORED = OK,
    DUPLICATE = DUPLICATE_ID
};

struct ArchiveFilter {
    std::string mode;                   // empty = any ("2P", "4P", "OFK" or exact code)
    std::string variant;                // empty = any, or canonical 13-variant code
    std::string playerUuid;             // empty = any
    std::string robotUuid;
    int64_t robotId = 0;
    int limit = 20;                     // clamped to 1..maxLimit
    int offset = 0;                     // clamped to >= 0
    std::string source;                 // "", "online", "client_upload"
    std::string status;                 // "", "GAME_OVER", "RESIGNED"
    std::string winner;                 // "", "A", "B", "C", "D", "NONE"
    int aiDifficulty = 0;               // 0 = any, or 1..5
    std::string appVersion;             // "" = any
    std::string platform;               // "" = any
    int64_t fromSec = 0;                // 0 = any
    int64_t toSec = 0;                  // 0 = any
    std::string sortField;              // "" / "ended_at" / "uploaded_at" / "ply_count"
    bool sortAsc = false;               // default false = descending
    int maxLimit = 100;                 // default kMaxArchiveLimit; up to kMaxExportLimit
    bool includeTranscript = false;     // true for bulk export NDJSON
};

struct AiDifficultyStats {
    int difficulty = 0;
    int64_t totalGames = 0;
    int64_t aiWins = 0;
    double winRate = 0.0;
};

struct ArchiveStats {
    int64_t totalGames = 0;
    std::map<std::string, int64_t> bySource;        // "online", "client_upload"
    std::map<std::string, int64_t> byMode;          // "2P", "4P", "OFK"
    std::map<std::string, int64_t> byVariant;       // 13 canonical codes ("2P_3B".."OFK")
    std::map<int, int64_t> byAiDifficulty;          // 0, 1..5
    std::map<std::string, int64_t> byWinner;        // "A", "B", "C", "D", "NONE"
    std::vector<AiDifficultyStats> aiDifficultyStats; // for difficulties 1..5
    double averagePlies = 0.0;
    std::map<std::string, int64_t> uploadsByDay;    // "YYYY-MM-DD" -> count
};

struct LiveRoomRow {
    std::string roomId;
    std::string stateJson;
    int64_t updatedAt = 0;
};

struct CrashReportRecord {
    std::string crashId;
    int64_t createdAt = 0;
    int64_t uploadedAt = 0;
    std::string platform;
    std::string appVersion;
    std::string osVersion;
    std::string exceptionType;
    std::string exceptionMessage;
    std::string threadName;
    std::string stackTrace;
    std::string contextJson = "{}";
    std::string installId;
};

struct CrashReportFilter {
    std::string platform;
    std::string appVersion;
    std::string exceptionType;
    int limit = 20;
    int offset = 0;
};

static constexpr int kDefaultUsersPageLimit = 50;
static constexpr int kMaxUsersPageLimit = 100;

struct UserDirectoryFilter {
    std::string query;      // case-insensitive substring on handle, display_name, location, bio
    std::string sortField;  // "handle" | "display_name" | "elo_2p" | "elo_4p" | "elo_ofk" | "games_played" | "wins" | "created_at" | "last_seen_at"
    bool sortAsc = false;
    int limit = kDefaultUsersPageLimit; // clamped to 1..100
    int offset = 0;         // clamped >= 0
};

struct UserDirectoryEntry {
    PlayerProfile profile;
    int activeRobotsCount = 0;
};

static constexpr int kMaxRobotsPerPlayer = 32;

struct RobotRecord {
    int64_t robotId = 0;
    std::string robotUuid;
    int64_t ownerPlayerId = 0;
    std::string ownerUuid;
    std::string ownerHandle;
    std::string ownerAvatarUrl;
    std::string robotName;
    std::string description;
    std::string token;       // Full "octi_bot_<base64url>" for owner; cleared in public queries
    std::string tokenPrefix; // "octi_bot_xxxx..."
    int gamesPlayed = 0;
    int wins = 0;
    int losses = 0;
    int draws = 0;
    int elo2p3b = 1200;
    int elo2p1b = 1200;
    int eloOfk = 1200;
    int games2p3b = 0;
    int games2p1b = 0;
    int gamesOfk = 0;
    int64_t createdAt = 0;
    bool revoked = false;
    int64_t revokedAt = 0;
    bool tokenDeleted = false;
    int64_t nameChangedAt = 0;
    std::string robotUrl;
};

struct RobotDirectoryFilter {
    std::string query;      // case-insensitive substring on robot_name, description, owner handle
    std::string sortField;  // "wins" | "games_played" | "win_rate" | "robot_name" | "created_at"
    bool sortAsc = false;
    int limit = kDefaultUsersPageLimit; // 1..100
    int offset = 0;
};

struct ArenaStandingsFilter {
    std::string participantType = "all"; // "all" | "robots" | "humans"
    std::string query;
    std::string sortField;  // "elo_2p_3b" | "elo_2p_1b" | "elo_ofk" | "wins" | "games_played" | "win_rate" | "name" | "robot_name" | "created_at"
    bool sortAsc = false;
    int limit = kDefaultUsersPageLimit; // 1..100
    int offset = 0;
};

struct ArenaStandingEntry {
    std::string entryType = "robot";    // "robot" | "human"
    int64_t robotId = 0;
    int64_t playerId = 0;
    std::string uuid;
    std::string robotUuid;
    std::string name;
    std::string displayName;
    std::string avatarUrl;
    std::string description;
    std::string robotUrl;
    int64_t nameChangedAt = 0;
    std::string ownerUuid;
    std::string ownerHandle;
    std::string ownerAvatarUrl;
    int elo2p3b = 1200;
    int elo2p1b = 1200;
    int eloOfk = 1200;
    int games2p3b = 0;
    int games2p1b = 0;
    int gamesOfk = 0;
    int gamesPlayed = 0;
    int wins = 0;
    int losses = 0;
    int draws = 0;
    double winRate = 0.0;
    int64_t createdAt = 0;
    int64_t lastSeenAt = 0;
    bool revoked = false;
    bool tokenDeleted = false;
};

enum class RobotStatus {
    OK = 0,
    INVALID_ROBOT_NAME,
    INVALID_ROBOT_DESCRIPTION,
    ROBOT_NAME_TAKEN,
    ROBOT_NAME_RATE_LIMITED,
    INVALID_ROBOT_URL,
    ROBOT_LIMIT_REACHED,
    NOT_FOUND,
    ALREADY_REVOKED,
    DB_ERROR,
};

struct CreateRobotResult {
    RobotStatus status = RobotStatus::DB_ERROR;
    RobotRecord robot;
    std::string rawToken; // "octi_bot_<43-char-base64url>"
    std::string errorMessage;
};

struct RobotTokenValidation {
    bool valid = false;
    RobotRecord robot;
    PlayerProfile owner;
};

struct LobbyChatLogRow {
    uint64_t seq = 0;
    std::string senderUuid;
    std::string senderHandle;
    std::string text;
    int64_t createdAtMs = 0;
};

struct RecomputeRatingsResult {
    int gamesScanned = 0;
    int pool1GamesRated = 0;
    int pool2GamesRated = 0;
    int playersUpdated = 0;
    int robotsUpdated = 0;
};

class AccountStore {
public:
    static constexpr int kMinHandleLength = 3;
    static constexpr int kMaxHandleLength = 24;
    static constexpr int64_t kHandleChangeCooldownSec = 24LL * 3600;
    static constexpr size_t kMaxAvatarDataUriBytes = 262144;
    static constexpr int kMaxLeaderboardLimit = 100;
    static constexpr int kMaxArchiveLimit = 100;
    static constexpr int kMaxExportLimit = 5000;
    static constexpr int64_t kDefaultSessionTtlSec = 30LL * 24 * 3600;

    AccountStore();
    ~AccountStore();

    AccountStore(const AccountStore &) = delete;
    AccountStore &operator=(const AccountStore &) = delete;

    /*
     * Open (creating if needed) the SQLite database in WAL mode with
     * foreign keys on. The file is forced to 0600; a missing parent
     * directory is created 0700 and an existing, owned, non-sticky parent
     * directory loses its group/other permission bits.
     */
    bool openOrCreate(const std::string &path, std::string &error);
    void close();
    bool isOpen() const;

    void setClock(std::function<int64_t()> clock);
    void setSessionTtlSec(int64_t seconds);
    void setAvatarDir(const std::string &dir);
    std::string avatarDir() const;
    void setAdminGoogleIdentities(const std::vector<std::string> &googleIdsOrEmails);
    bool isPlayerAdmin(int64_t playerId) const;
    bool readAvatarFile(const std::string &fileName, std::string &outBytes,
                        std::string &outMimeType) const;

    static bool transcodeWebpToPng(const std::string &webpBytes,
                                   std::string &outPngBytes);

    static bool isValidHandle(const std::string &handle);
    static std::string sha256Hex(const std::string &data);
    static bool validBio(const std::string &bio);
    static bool validLocation(const std::string &location);
    static bool validAvatarUrl(const std::string &url);
    static bool validRobotName(const std::string &name);
    static bool validRobotDescription(const std::string &desc);

    LoginResult loginWithSso(const VerifiedClaims &claims,
                             const std::string &clientAgent);
    LinkResult linkIdentity(int64_t playerId, const VerifiedClaims &claims,
                            bool forceRelinkEmpty = false);
    UnlinkStatus unlinkIdentity(int64_t playerId, const std::string &provider);
    bool listLinkedIdentities(int64_t playerId,
                              std::vector<LinkedIdentity> &out);

    bool mintSession(int64_t playerId, const std::string &clientAgent,
                     std::string &token, int64_t &expiresAt,
                     std::string &error);
    SessionValidation validateSession(const std::string &token);
    SessionValidation validateSession(const std::string &token, int64_t now);
    /* True when a session with that token exists (idempotent). */
    bool revokeSession(const std::string &token);
    /* Number of sessions newly revoked for the player; -1 on error. */
    int revokeAllSessions(int64_t playerId);
    /*
     * Sessions of the player that are neither revoked nor expired at the
     * store's current time; -1 on error.
     */
    int countActiveSessions(int64_t playerId);
    /* Deletes expired or revoked sessions; returns rows removed, -1 on error. */
    int purgeExpiredSessions(int64_t now);

    bool getProfile(int64_t playerId, PlayerProfile &out);
    bool getProfileById(int64_t playerId, PlayerProfile &out);
    bool getProfileByUuid(const std::string &uuid, PlayerProfile &out);
    bool getProfileByHandle(const std::string &handle, PlayerProfile &out);
    bool bindInstallToPlayer(const std::string &installId, int64_t playerId,
                             int *reconciledCount = nullptr);
    bool getPlayerByInstallId(const std::string &installId, PlayerProfile &out);
    int reconcileAnonymousGames();
    UpdateStatus updateProfile(int64_t playerId, const ProfileUpdate &update,
                               PlayerProfile *updated = nullptr);
    bool setBanned(int64_t playerId, bool banned);

    bool listUsers(const UserDirectoryFilter &filter,
                   std::vector<UserDirectoryEntry> &out,
                   int64_t *total = nullptr);

    static bool validRobotUrlSyntax(const std::string &url);

    CreateRobotResult createRobot(int64_t ownerPlayerId,
                                  const std::string &robotName,
                                  const std::string &description = "",
                                  const std::string &robotUrl = "");
    RobotStatus updateRobotDescription(int64_t ownerPlayerId,
                                       const std::string &robotUuidOrName,
                                       const std::string &description);
    RobotStatus updateRobot(int64_t ownerPlayerId,
                            const std::string &robotUuidOrName,
                            bool setRobotName,
                            const std::string &robotName,
                            bool setDescription,
                            const std::string &description,
                            bool setRobotUrl = false,
                            const std::string &robotUrl = "",
                            RobotRecord *updated = nullptr);
    bool listRobots(int64_t ownerPlayerId, std::vector<RobotRecord> &out);
    bool listPublicRobotsByOwner(int64_t ownerPlayerId,
                                 std::vector<RobotRecord> &out);
    bool listPublicRobotsDirectory(const RobotDirectoryFilter &filter,
                                   std::vector<RobotRecord> &out,
                                   int64_t *total = nullptr);
    bool listArenaStandings(const ArenaStandingsFilter &filter,
                            std::vector<ArenaStandingEntry> &out,
                            int64_t *total = nullptr);
    int countRecentArenaRatedPairGames(int64_t p1PlayerId,
                                       int64_t p1RobotId,
                                       int64_t p2PlayerId,
                                       int64_t p2RobotId,
                                       int64_t windowSec = 24LL * 3600);
    RobotStatus revokeRobot(int64_t ownerPlayerId,
                            const std::string &robotUuidOrName);
    RobotStatus deleteRobot(int64_t ownerPlayerId,
                            const std::string &robotUuidOrName);
    RobotTokenValidation validateRobotToken(const std::string &rawToken);
    bool getRobotById(int64_t robotId, RobotRecord &out);
    bool getRobotByUuid(const std::string &robotUuid, RobotRecord &out);
    bool getRobotByName(const std::string &robotName, RobotRecord &out);
    bool recordRobotGameOutcome(int64_t robotId, GameOutcome outcome);
    bool reconcileRobotStats();
    bool recomputeAllRatings(RecomputeRatingsResult &outResult,
                             std::string &errorOut);

    /* mode: '2P' | '4P' | 'OFK'; limit clamped to 1..kMaxLeaderboardLimit. */
    bool leaderboard(const std::string &mode, int limit,
                     std::vector<LeaderboardEntry> &out);

    /* Atomic: either every update is applied or none is. */
    bool applyRatingUpdates(const std::string &mode,
                            const std::vector<RatingUpdate> &updates,
                            std::string &error);

    ArchiveStatus archiveGame(const GameRecord &record, std::string &error);
    /* Archive and rating updates committed in one transaction. */
    ArchiveStatus archiveGameWithRatings(const GameRecord &record,
                                         const std::vector<RatingUpdate> &updates,
                                         std::string &error);
    ArchiveStatus archiveGameWithRatings(
        const GameRecord &record,
        const std::vector<RatingUpdate> &updates,
        const std::vector<RobotOutcomeUpdate> &robotOutcomes,
        std::string &error);
    bool purgeArchivedGame(const std::string &gameId, std::string &errorOut);
    bool getArchivedGame(const std::string &gameId, GameRecord &out);
    bool getArchivedTranscript(const std::string &gameId,
                               std::string &transcript);
    bool listArchivedGames(const ArchiveFilter &filter,
                           std::vector<GameRecord> &out,
                           int64_t *total = nullptr);
    int deleteGamesByInstallId(const std::string &installId);
    bool getArchiveStats(ArchiveStats &out);

    ArchiveStatus recordCrashReport(const CrashReportRecord &report,
                                    std::string &error);
    bool getCrashReport(const std::string &crashId, CrashReportRecord &out);
    bool listCrashReports(const CrashReportFilter &filter,
                          std::vector<CrashReportRecord> &out,
                          int64_t *total = nullptr);

    /* Live room and lobby meta persistence. */
    bool upsertLiveRoom(const std::string &roomId, const std::string &stateJson,
                        std::string &error);
    bool deleteLiveRoom(const std::string &roomId, std::string &error);
    bool listLiveRooms(std::vector<LiveRoomRow> &out, std::string &error);
    bool setLiveMeta(const std::string &metaKey, const std::string &metaJson,
                     std::string &error);
    bool getLiveMeta(const std::string &metaKey, std::string &metaJson,
                     std::string &error);

    /* Persistent server-side lobby chat audit log. */
    bool appendLobbyChatLog(uint64_t seq, const std::string &senderUuid,
                            const std::string &senderHandle,
                            const std::string &text, int64_t createdAtMs);
    bool purgeLobbyChatLogsBefore(int64_t cutoffEpochMs);
    bool listLobbyChatLogs(std::vector<LobbyChatLogRow> &out,
                           int limit = 100);

private:
    int64_t nowSec() const;
    bool isPlayerAdminLocked(int64_t playerId) const;

    mutable std::mutex _mutex;
    sqlite3 *_db;
    std::function<int64_t()> _clock;
    int64_t _sessionTtlSec;
    std::string _avatarDir;
    std::vector<std::string> _adminGoogleIds;
};

} // namespace octi

#endif // OCTI_SERVER_ACCOUNT_STORE_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
