/*
 * GameRoomManager.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_SERVER_GAME_ROOM_MANAGER_HXX
#define OCTI_SERVER_GAME_ROOM_MANAGER_HXX

#include "AccountStore.hxx"
#include "MatchmakingQueue.hxx"
#include "OctiDefs.hxx"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace octi {

/* ------------------------------------------------------------------ */
/* Policies and enums                                                  */
/* ------------------------------------------------------------------ */

/*
 * OPEN: observers watch and chat.
 * SILENT_OBSERVERS: observers watch but can neither chat nor use voice.
 * NO_OBSERVERS: only seated players may view an in-progress room, and
 *               nobody can register as an observer until it is finished.
 */
enum class ObserverPolicy { OPEN, SILENT_OBSERVERS, NO_OBSERVERS };

/*
 * DISABLED: no voice signaling or meta at all.
 * PLAYERS_ONLY: only seated players (sender and target) take part.
 * ALL_ALLOWED: seated players and registered, unmuted observers take part
 *              (observers only while observerPolicy is not SILENT).
 */
enum class VoicePolicy { DISABLED, PLAYERS_ONLY, ALL_ALLOWED };

enum class PresenceStatus { OFFLINE, IN_LOBBY, AT_TABLE, PLAYING, OBSERVING };

enum class RoomPhase { WAITING_FOR_PLAYERS, IN_PROGRESS, FINISHED };

enum class ChatScope { ALL, TEAM };

enum class VoiceSignalType { SDP_OFFER, SDP_ANSWER, ICE_CANDIDATE };

enum class VoiceMetaAction {
    JOINED_VOICE,
    LEFT_VOICE,
    MIC_MUTED,
    MIC_UNMUTED
};

enum class ProposalType {
    DRAW_OFFER,
    PAUSE_REQUEST,
    UNDO_REQUEST,
    REMATCH_REQUEST
};

/* Outcome code of every manager operation (the HTTP layer maps these). */
enum class RoomCode {
    OK,
    UNKNOWN_PLAYER,             // no such account
    NOT_FOUND,                  // no such room
    INVALID_ARGUMENT,
    FORBIDDEN_BANNED,
    FORBIDDEN_NOT_SEATED,
    FORBIDDEN_NOT_PARTICIPANT,  // neither seated nor a registered observer
    FORBIDDEN_MUTED,
    FORBIDDEN_POLICY,           // observer/voice policy refuses the action
    FORBIDDEN_SEATED_TARGET,    // moderation target holds a seat
    FORBIDDEN_PASSCODE,
    FORBIDDEN_NOT_HOST,
    NOT_YOUR_TURN,
    ILLEGAL_SYNTAX,
    ILLEGAL_MOVE,
    INVALID_STATE,
    PAYLOAD_TOO_LARGE,
    RATE_LIMITED,
    SEAT_TAKEN,
    ALREADY_SEATED,
    CONFLICT,                   // e.g. proposal already pending, already queued
    LIMIT_REACHED,
    INTERNAL_ERROR
};

const char *toString(ObserverPolicy v);
const char *toString(VoicePolicy v);
const char *toString(PresenceStatus v);
const char *toString(RoomPhase v);
const char *toString(ChatScope v);
const char *toString(VoiceSignalType v);
const char *toString(VoiceMetaAction v);
const char *toString(ProposalType v);
const char *toString(RoomCode v);

bool parseObserverPolicy(const std::string &s, ObserverPolicy &out);
bool parseVoicePolicy(const std::string &s, VoicePolicy &out);
bool parseChatScope(const std::string &s, ChatScope &out);
bool parseVoiceSignalType(const std::string &s, VoiceSignalType &out);
bool parseVoiceMetaAction(const std::string &s, VoiceMetaAction &out);
bool parseProposalType(const std::string &s, ProposalType &out);

/* ------------------------------------------------------------------ */
/* Table configuration                                                 */
/* ------------------------------------------------------------------ */

struct TimeControlConfig {
    bool enabled = false;
    int64_t initialMs = 0;
    int64_t incrementMs = 0;
    int64_t disconnectGraceMs = 90000;
};

struct GameTableConfig {
    OctiGameMode mode = GAME_MODE_2;
    OctiGameBases bases = GAME_BASES_3;
    bool useSuperProngs = false;
    bool useEdgelessBoard = false;
    TimeControlConfig timeControl;
    bool rated = true;
    bool allowTakeback = false;     // forced false on rated tables
    ObserverPolicy observerPolicy = ObserverPolicy::OPEN;
    VoicePolicy voicePolicy = VoicePolicy::PLAYERS_ONLY;
    std::string passcode;           // empty = public table
};

/* Tunable resource bounds; defaults are production values. */
struct RoomLimits {
    size_t maxRooms = 5000;
    size_t maxRoomEvents = 4096;
    size_t maxLobbyEvents = 1000;
    size_t maxLobbyChat = 200;
    size_t maxPendingSignals = 256;
    int64_t finishedRetentionMs = 30LL * 60 * 1000;
    int64_t waitingTtlMs = 60LL * 60 * 1000;
    int64_t proposalTtlMs = 60LL * 1000;
    int64_t signalTtlMs = 60LL * 1000;
    int lobbyChatBurst = 5;
    int64_t lobbyChatWindowMs = 10000;
    int boardChatBurst = 10;
    int64_t boardChatWindowMs = 10000;
};

/* ------------------------------------------------------------------ */
/* Results                                                             */
/* ------------------------------------------------------------------ */

/*
 * Every operation returns this. Optional fields are only meaningful for
 * the operations that document them.
 */
struct RoomResult {
    RoomCode code = RoomCode::OK;
    std::string message;
    std::string roomId;         // created / related room
    int seat = 0;               // seat taken or acted from
    int64_t seq = 0;            // sequence of the event the call produced
    bool gameStarted = false;   // join: the table filled and play began
    bool gameOver = false;      // move/resign/...: the game is now finished
    std::string move;           // submitMove: canonical move text
    int64_t clockMs = -1;       // submitMove: mover's clock after the move
    int64_t elapsedMs = 0;      // submitMove: time the mover spent

    bool ok() const { return code == RoomCode::OK; }
};

/* ------------------------------------------------------------------ */
/* Events                                                              */
/* ------------------------------------------------------------------ */

enum class RoomEventType {
    ROOM_CREATED,
    SEAT_JOINED,
    SEAT_LEFT,
    SEAT_KICKED,
    HOST_CHANGED,
    OBSERVER_JOINED,
    OBSERVER_LEFT,
    GAME_STARTED,
    MOVE,
    CHAT,
    VOICE_SIGNAL,
    VOICE_META,
    OBSERVER_BANNED,
    USER_MUTED,
    POLICY_UPDATED,
    PROPOSAL_OFFERED,
    PROPOSAL_ACCEPTED,
    PROPOSAL_DECLINED,
    PROPOSAL_EXPIRED,
    GAME_PAUSED,
    GAME_RESUMED,
    UNDO_APPLIED,
    REMATCH_CREATED,
    GAME_OVER,
    PLAYER_DISCONNECTED,
    PLAYER_RECONNECTED
};

const char *toString(RoomEventType v);

/*
 * One entry of a room's ordered event stream. Field use per type:
 *   MOVE            playerId, seat, ply, text = canonical move,
 *                   clockMs = mover clock after the move, elapsedMs
 *   CHAT            playerId, seat (0 = observer), scope, text
 *   VOICE_SIGNAL    playerId = sender, targetPlayerId, detail = signal
 *                   type, text = opaque payload (relayed, never archived)
 *   VOICE_META      playerId, detail = action
 *   OBSERVER_BANNED playerId = actor, targetPlayerId, text = reason
 *   USER_MUTED      playerId = actor, targetPlayerId,
 *                   detail = "mute_chat=<0|1>;mute_voice=<0|1>"
 *   POLICY_UPDATED  playerId, detail = "observer_policy=..;voice_policy=.."
 *   PROPOSAL_*      playerId, detail = proposal type
 *   UNDO_APPLIED    playerId = requester, text = undone move, ply
 *   REMATCH_CREATED text = new room id
 *   GAME_OVER       detail = termination, text = result, seat = winner seat
 */
struct RoomEvent {
    int64_t seq = 0;
    RoomEventType type = RoomEventType::ROOM_CREATED;
    int64_t atMs = 0;
    int64_t playerId = 0;
    std::string handle;
    int64_t targetPlayerId = 0;
    int seat = 0;
    ChatScope scope = ChatScope::ALL;
    int ply = 0;
    int64_t clockMs = 0;
    int64_t elapsedMs = 0;
    std::string text;
    std::string detail;
};

struct RoomPoll {
    std::vector<RoomEvent> events;  // ascending seq, > sinceSeq, visible
    int64_t latestSeq = 0;          // newest sequence number issued
    bool gap = false;               // events older than the buffer were lost
    bool hasMore = false;           // limit reached; poll again
};

/* ------------------------------------------------------------------ */
/* Views                                                               */
/* ------------------------------------------------------------------ */

struct SeatView {
    int seat = 0;                   // 1..4
    int64_t playerId = 0;           // 0 = empty
    std::string handle;
    int rating = 0;
    bool connected = true;
    int64_t disconnectedForMs = 0;
    bool forfeitClaimable = false;  // disconnected past the grace window
    int64_t clockMs = -1;           // live remaining time, -1 if untimed
    bool eliminated = false;
};

struct ProposalView {
    ProposalType type = ProposalType::DRAW_OFFER;
    int64_t proposerPlayerId = 0;
    std::vector<int64_t> acceptedBy;
    std::vector<int64_t> awaiting;
    int64_t expiresAtMs = 0;
};

struct VoicePeerView {
    int64_t playerId = 0;
    bool micMuted = false;
};

struct RoomView {
    std::string roomId;
    RoomPhase phase = RoomPhase::WAITING_FOR_PLAYERS;
    bool paused = false;
    GameTableConfig config;         // passcode is always cleared
    bool hasPasscode = false;
    int64_t hostPlayerId = 0;
    int viewerSeat = 0;             // 0 when the viewer is not seated
    std::vector<SeatView> seats;
    std::vector<int64_t> observerIds;
    std::string fen;
    int turnSeat = 0;               // 0 when no one is to move
    int plyCount = 0;
    std::vector<std::string> moves;
    std::string result;             // archive vocabulary, empty while live
    std::string termination;
    int winnerSeat = 0;             // 0 = draw / none; 4P: 1 = seats 1+3
    std::vector<ProposalView> proposals;
    std::vector<VoicePeerView> voicePeers;
    int64_t latestSeq = 0;
    bool archived = false;
    std::string archiveError;
    std::string rematchRoomId;
    int64_t createdAtMs = 0;
};

struct TableSummary {
    std::string roomId;
    GameTableConfig config;         // passcode cleared
    bool hasPasscode = false;
    RoomPhase phase = RoomPhase::WAITING_FOR_PLAYERS;
    std::string hostHandle;
    int seatsTotal = 2;
    int seatsFilled = 0;
    std::vector<std::string> playerHandles;
    int observerCount = 0;
};

/* ------------------------------------------------------------------ */
/* Lobby                                                               */
/* ------------------------------------------------------------------ */

enum class LobbyEventType {
    LOBBY_CHAT,
    PRESENCE,
    TABLE_UPDATED,
    TABLE_CLOSED,
    MATCH_FOUND
};

const char *toString(LobbyEventType v);

struct LobbyEvent {
    int64_t seq = 0;
    LobbyEventType type = LobbyEventType::LOBBY_CHAT;
    int64_t atMs = 0;
    int64_t playerId = 0;
    std::string handle;
    int64_t targetPlayerId = 0;     // non-zero: private to that player
    std::string text;               // chat text
    PresenceStatus status = PresenceStatus::IN_LOBBY;
    std::string roomId;
};

struct LobbyPoll {
    std::vector<LobbyEvent> events;
    int64_t latestSeq = 0;
    bool gap = false;
    bool hasMore = false;
};

struct PresenceEntry {
    int64_t playerId = 0;
    std::string handle;
    PresenceStatus status = PresenceStatus::IN_LOBBY;
    std::string roomId;             // table the player is at / watching
};

struct LobbySnapshot {
    std::vector<PresenceEntry> players;
    std::vector<TableSummary> tables;
    std::vector<LobbyEvent> recentChat;
    int64_t latestSeq = 0;
};

struct MatchmakingStatus {
    bool queued = false;
    int64_t waitedMs = 0;
    int window = 0;
    int rating = 0;
    MatchRuleset ruleset;
    std::string matchedRoomId;      // set once a room was spawned for the player
};

struct TickReport {
    int matchesSpawned = 0;
    int gamesTimedOut = 0;
    int roomsReaped = 0;
    int archiveRetries = 0;
};

/* ------------------------------------------------------------------ */
/* GameRoomManager                                                     */
/* ------------------------------------------------------------------ */

/*
 * Thread-safe lobby, table, game-room and matchmaking manager. One internal
 * mutex serialises every operation; AccountStore (which has its own lock)
 * is only ever called while holding it, never the other way round.
 *
 * Time is injectable and never read from a background thread: deadlines
 * (clock flags, proposal expiry, table TTLs) are evaluated lazily at the
 * start of every room operation and in tick(), which the HTTP layer should
 * call periodically (about once a second) so idle rooms still flag.
 *
 * Players are AccountStore player ids. An id the store does not know yields
 * UNKNOWN_PLAYER. viewerPlayerId 0 stands for an anonymous spectator in the
 * read-only calls.
 *
 * Voice: the server only relays opaque signaling blobs to the named peer
 * and records short meta events. It never receives or stores audio, and a
 * relayed signal is dropped from the buffer as soon as the target's poll
 * cursor passes it (or after RoomLimits::signalTtlMs). It is never part of
 * the transcript or the archive.
 *
 * configure setClock/setWallClock/setLimits/setMatchmakingConfig before the
 * manager is shared between threads.
 */
class GameRoomManager {
public:
    static constexpr size_t kMaxChatBytes = 500;
    static constexpr size_t kMaxVoicePayloadBytes = 4096;
    static constexpr size_t kMaxMoveBytes = 512;
    static constexpr size_t kMaxPasscodeBytes = 32;
    static constexpr size_t kMaxReasonBytes = 200;
    static constexpr int64_t kMaxClockSettingMs = 4LL * 3600 * 1000;
    static constexpr int64_t kMaxGraceMs = 10LL * 60 * 1000;
    static constexpr int64_t kMaxLiveRoomOfflineSec = 24LL * 3600;

    explicit GameRoomManager(AccountStore &store);
    ~GameRoomManager();

    GameRoomManager(const GameRoomManager &) = delete;
    GameRoomManager &operator=(const GameRoomManager &) = delete;

    /* Flush all live rooms, lobby state, and matchmaking queue to AccountStore. */
    bool checkpointToStore();

    /* Reload live rooms, lobby state, and matchmaking queue from AccountStore. */
    bool restoreFromStore();

    /* Monotonic milliseconds (clocks, deadlines); default steady_clock. */
    void setClock(std::function<int64_t()> nowMs);
    /* Unix epoch seconds (ids, archive timestamps); default system_clock. */
    void setWallClock(std::function<int64_t()> epochSec);
    void setLimits(const RoomLimits &limits);
    void setMatchmakingConfig(const MatchmakingConfig &config);

    /* ---- lobby presence & chat ---- */
    RoomResult enterLobby(int64_t playerId);
    RoomResult leaveLobby(int64_t playerId);
    PresenceStatus presenceOf(int64_t playerId) const;
    LobbySnapshot lobbyState() const;
    RoomResult postLobbyChat(int64_t playerId, const std::string &text);
    LobbyPoll pollLobbyEvents(int64_t viewerPlayerId, int64_t sinceSeq,
                              size_t limit) const;
    std::vector<TableSummary> listTables(bool includeFinished = false) const;

    /* ---- tables and seats ---- */
    RoomResult createTable(int64_t hostPlayerId, const GameTableConfig &config);
    RoomResult joinTable(const std::string &roomId, int64_t playerId, int seat,
                         const std::string &passcode);
    RoomResult leaveTable(const std::string &roomId, int64_t playerId);
    RoomResult kickSeat(const std::string &roomId, int64_t hostPlayerId,
                        int64_t targetPlayerId);
    RoomResult observeRoom(const std::string &roomId, int64_t playerId);
    RoomResult stopObserving(const std::string &roomId, int64_t playerId);

    /* ---- matchmaking ---- */
    RoomResult enqueueMatchmaking(int64_t playerId, const MatchRuleset &ruleset);
    RoomResult dequeueMatchmaking(int64_t playerId);
    MatchmakingStatus matchmakingStatus(int64_t playerId) const;

    /* Evaluate deadlines, spawn matched rooms, retry archives, reap rooms. */
    TickReport tick();

    /* ---- play ---- */
    RoomResult submitMove(const std::string &roomId, int64_t playerId,
                          const std::string &moveText);
    RoomResult resign(const std::string &roomId, int64_t playerId);
    RoomResult submitProposal(const std::string &roomId, int64_t playerId,
                              ProposalType type);
    RoomResult respondProposal(const std::string &roomId, int64_t playerId,
                               ProposalType type, bool accept);
    /* Any seated player may resume a paused game. */
    RoomResult resumeGame(const std::string &roomId, int64_t playerId);
    RoomResult markDisconnected(const std::string &roomId, int64_t playerId);
    RoomResult markReconnected(const std::string &roomId, int64_t playerId);
    /* An opponent forfeits a seat that stayed disconnected past the grace. */
    RoomResult claimDisconnectForfeit(const std::string &roomId,
                                      int64_t claimantPlayerId);

    /* ---- board chat, voice, moderation ---- */
    RoomResult postBoardChat(const std::string &roomId, int64_t senderPlayerId,
                             ChatScope scope, const std::string &text);
    RoomResult submitVoiceSignal(const std::string &roomId,
                                 int64_t senderPlayerId, int64_t targetPlayerId,
                                 VoiceSignalType type,
                                 const std::string &payloadJson);
    RoomResult submitVoiceMeta(const std::string &roomId,
                               int64_t senderPlayerId, VoiceMetaAction action);
    RoomResult banUser(const std::string &roomId, int64_t actorPlayerId,
                       int64_t targetPlayerId, const std::string &reason);
    RoomResult muteUser(const std::string &roomId, int64_t actorPlayerId,
                        int64_t targetPlayerId, bool muteChat, bool muteVoice);
    RoomResult updateRoomPolicies(const std::string &roomId,
                                  int64_t actorPlayerId,
                                  ObserverPolicy observerPolicy,
                                  VoicePolicy voicePolicy);
    /*
     * Partial update: a null field keeps the room's current value. The
     * read and the write happen under one lock, so concurrent partial
     * updates of different fields never overwrite each other.
     */
    RoomResult updateRoomPolicies(const std::string &roomId,
                                  int64_t actorPlayerId,
                                  const std::optional<ObserverPolicy> &observerPolicy,
                                  const std::optional<VoicePolicy> &voicePolicy);

    /* ---- reads ---- */
    RoomResult getRoomView(const std::string &roomId, int64_t viewerPlayerId,
                           RoomView &out);
    RoomResult pollRoomEvents(const std::string &roomId, int64_t viewerPlayerId,
                              int64_t sinceSeq, size_t limit, RoomPoll &out);
    /* Current .octi v2.0 transcript (live or finished). */
    RoomResult getTranscript(const std::string &roomId, int64_t viewerPlayerId,
                             std::string &out);
    bool isBanned(const std::string &roomId, int64_t playerId) const;
    std::vector<int64_t> bannedPlayers(const std::string &roomId) const;
    size_t roomCount() const;
    /* Number of players currently online (cheap; no snapshot is built). */
    size_t onlinePlayerCount() const;

private:
    struct Room;
    struct PlayerInfo {
        std::string handle;
        bool online = false;
        PresenceStatus lastPresence = PresenceStatus::OFFLINE;
    };

    int64_t nowMs() const;
    int64_t wallSec() const;

    /* player / presence */
    bool ensurePlayerLocked(int64_t playerId, bool refresh = false);
    void touchOnlineLocked(int64_t playerId);
    PresenceStatus presenceLocked(int64_t playerId, std::string *roomId) const;
    void refreshPresenceLocked(int64_t playerId);
    const std::string &handleOf(int64_t playerId) const;

    /* lobby */
    void pushLobbyEventLocked(LobbyEvent e);
    TableSummary summarize(const Room &r) const;
    void announceTableLocked(const Room &r, bool closed);

    /* room lookup and lifecycle */
    Room *findRoomLocked(const std::string &roomId);
    const Room *findRoomLocked(const std::string &roomId) const;
    std::string newRoomIdLocked();
    Room *createRoomLocked(const GameTableConfig &cfg, int64_t hostId,
                           int64_t now);
    void deleteRoomLocked(const std::string &roomId);
    void advanceRoomLocked(Room &r, int64_t now);
    void startGameLocked(Room &r, int64_t now);
    void seatPlayerLocked(Room &r, int seat, int64_t playerId, int64_t now);
    void unseatLocked(Room &r, int seat);
    /* Voluntary leave of a waiting table; true when the room was closed. */
    bool leaveWaitingSeatLocked(Room &r, int seat, int64_t now);
    void spawnMatchRoomLocked(const MatchGroup &g, int64_t now);
    RoomResult spawnRematchLocked(Room &old, int64_t now);

    /* events */
    int64_t emitLocked(Room &r, RoomEvent e, int64_t now);
    void purgeUserEventsLocked(Room &r, int64_t playerId);
    bool eventVisibleLocked(const Room &r, const RoomEvent &e,
                            int64_t viewerId) const;

    /* observers / voice state */
    void addObserverLocked(Room &r, int64_t playerId);
    void removeObserverLocked(Room &r, int64_t playerId);
    void enforcePoliciesLocked(Room &r, int64_t now);

    /* access control */
    RoomResult checkViewLocked(const Room &r, int64_t viewerId) const;
    int seatOfLocked(const Room &r, int64_t playerId) const;
    bool isParticipantLocked(const Room &r, int64_t playerId) const;
    bool voiceAllowedLocked(const Room &r, int64_t playerId) const;

    /* game flow */
    void finishGameLocked(Room &r, int64_t now, const std::string &termination,
                          int winnerSeat);
    void forfeitSeatLocked(Room &r, int64_t now, int loserSeat,
                           const std::string &termination);
    bool archiveLocked(Room &r);
    std::string buildTranscriptLocked(const Room &r) const;
    int64_t liveClockLocked(const Room &r, int seat, int64_t now) const;
    std::vector<int> approversLocked(const Room &r, int proposerSeat) const;
    RoomResult completeProposalLocked(Room &r, ProposalType type,
                                      int64_t proposerId, int64_t now);
    void fillView(const Room &r, int64_t viewerId, int64_t now,
                  RoomView &out) const;

    /* Live state persistence */
    bool checkpointToStoreLocked();
    bool restoreFromStoreLocked();
    void saveRoomLocked(const Room &r);
    void saveLobbyMetaLocked();

    mutable std::mutex _mutex;
    AccountStore &_store;
    std::function<int64_t()> _clock;
    std::function<int64_t()> _wallClock;
    bool _customWallClock;
    bool _restoring;
    RoomLimits _limits;
    MatchmakingQueue _queue;

    std::map<int64_t, PlayerInfo> _players;
    std::map<std::string, std::unique_ptr<Room>> _rooms;
    std::map<int64_t, std::string> _seatRoom;           // active seat
    std::map<int64_t, std::set<std::string>> _observing;
    std::map<int64_t, std::string> _matchAssignments;
    std::map<int64_t, std::deque<int64_t>> _lobbyChatTimes;

    std::deque<LobbyEvent> _lobbyEvents;
    int64_t _lobbySeq;
    int64_t _lobbyEvictedUpToSeq;
    size_t _lobbyChatCount;
    uint64_t _roomCounter;
};

} // namespace octi

#endif // OCTI_SERVER_GAME_ROOM_MANAGER_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
