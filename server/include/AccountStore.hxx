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
#include <mutex>
#include <string>
#include <vector>

struct sqlite3;

namespace octi {

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
    int elo2p = 1200;
    int elo4p = 1200;
    int eloOfk = 1200;
    int gamesPlayed = 0;
    int wins = 0;
    int losses = 0;
    int draws = 0;
    int64_t createdAt = 0;
    int64_t lastSeenAt = 0;
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
    LINKED,
    ALREADY_LINKED_SELF,
    CONFLICT_ALREADY_LINKED,
    PLAYER_NOT_FOUND,
    INVALID_CLAIMS,
    ERROR
};

struct LinkResult {
    LinkStatus status = LinkStatus::ERROR;
    PublicProfile conflictProfile;      // set for CONFLICT_ALREADY_LINKED
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
};

enum class UpdateStatus {
    OK,
    NOT_FOUND,
    INVALID_HANDLE,
    HANDLE_TAKEN,
    INVALID_DISPLAY_NAME,
    INVALID_AVATAR_URL,
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
};

enum class ArchiveStatus {
    OK,
    DUPLICATE_ID,
    INVALID,
    ERROR
};

struct ArchiveFilter {
    std::string mode;                   // empty = any
    std::string playerUuid;             // empty = any
    int limit = 20;                     // clamped to 1..100
    int offset = 0;                     // clamped to >= 0
};

struct LiveRoomRow {
    std::string roomId;
    std::string stateJson;
    int64_t updatedAt = 0;
};

class AccountStore {
public:
    static constexpr int kMinHandleLength = 3;
    static constexpr int kMaxHandleLength = 24;
    static constexpr int kMaxLeaderboardLimit = 100;
    static constexpr int kMaxArchiveLimit = 100;
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

    static bool isValidHandle(const std::string &handle);
    static std::string sha256Hex(const std::string &data);

    LoginResult loginWithSso(const VerifiedClaims &claims,
                             const std::string &clientAgent);
    LinkResult linkIdentity(int64_t playerId, const VerifiedClaims &claims);
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

    bool getProfileById(int64_t playerId, PlayerProfile &out);
    bool getProfileByUuid(const std::string &uuid, PlayerProfile &out);
    UpdateStatus updateProfile(int64_t playerId, const ProfileUpdate &update);

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
