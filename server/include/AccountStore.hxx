/*
 * AccountStore.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_SERVER_ACCOUNT_STORE_HXX
#define OCTI_SERVER_ACCOUNT_STORE_HXX

#include "OidcVerifier.hxx"

#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

struct sqlite3;

namespace octi {

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
    int elo2p = 1200;
    int elo4p = 1200;
    int eloOfk = 1200;
    int gamesPlayed = 0;
    int wins = 0;
    int losses = 0;
    int draws = 0;
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

/* New absolute rating and outcome for one player of a finished game. */
struct RatingUpdate {
    int64_t playerId = 0;
    int newElo = 1200;
    GameOutcome outcome = GameOutcome::DRAW;
};

struct LeaderboardEntry {
    int rank = 0;
    std::string uuid;
    std::string handle;
    std::string displayName;
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
    std::string mode;                   // empty = any
    std::string playerUuid;             // empty = any
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

static constexpr int kMaxRobotsPerPlayer = 3;

struct RobotRecord {
    int64_t robotId = 0;
    std::string robotUuid;
    int64_t ownerPlayerId = 0;
    std::string ownerUuid;
    std::string ownerHandle;
    std::string robotName;
    std::string description;
    std::string token;       // Full "octi_bot_<base64url>" for owner; cleared in public queries
    std::string tokenPrefix; // "octi_bot_xxxx..."
    int64_t createdAt = 0;
    bool revoked = false;
    int64_t revokedAt = 0;
};

enum class RobotStatus {
    OK = 0,
    INVALID_ROBOT_NAME,
    INVALID_ROBOT_DESCRIPTION,
    ROBOT_NAME_TAKEN,
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

    CreateRobotResult createRobot(int64_t ownerPlayerId,
                                  const std::string &robotName,
                                  const std::string &description = "");
    RobotStatus updateRobotDescription(int64_t ownerPlayerId,
                                       const std::string &robotUuidOrName,
                                       const std::string &description);
    bool listRobots(int64_t ownerPlayerId, std::vector<RobotRecord> &out);
    bool listPublicRobotsByOwner(int64_t ownerPlayerId,
                                 std::vector<RobotRecord> &out);
    RobotStatus revokeRobot(int64_t ownerPlayerId,
                            const std::string &robotUuidOrName);
    RobotTokenValidation validateRobotToken(const std::string &rawToken);

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

private:
    int64_t nowSec() const;

    mutable std::mutex _mutex;
    sqlite3 *_db;
    std::function<int64_t()> _clock;
    int64_t _sessionTtlSec;
    std::string _avatarDir;
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
