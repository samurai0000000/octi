/*
 * OctiNetworkClient.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_OCTI_NETWORK_CLIENT_HXX
#define OCTI_DESKTOP_OCTI_NETWORK_CLIENT_HXX

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "OctiDefs.hxx"

namespace octi {

template <typename T>
struct ApiResult {
    bool ok = false;
    int httpStatus = 0;
    std::string errorCode;
    std::string errorMessage;
    int retryAfterSec = 0;
    T data{};
};

struct HealthInfo {
    std::string status;
    std::string version;
    std::int64_t uptimeSec = 0;
    std::int64_t activeTables = 0;
    std::int64_t onlinePlayers = 0;
    int protocolVersion = OCTI_PROTOCOL_VERSION;
    int minProtocolVersion = OCTI_MIN_PROTOCOL_VERSION;
};

struct AuthProviderInfo {
    std::string name;
    std::string displayName;
    std::vector<std::string> clientIds;
};

struct LinkedIdentityInfo {
    std::string provider;
    std::string providerEmail;
    std::string displayHint;
    std::int64_t linkedAt = 0;
    std::int64_t lastLoginAt = 0;
};

struct ArchivedGamePlayer {
    int seat = 0;
    std::string handle;
    std::string playerUuid;
};

struct ArchivedGameSummary {
    std::string gameId;
    std::string mode;
    int bases = 3;
    bool superProngs = false;
    bool edgeless = false;
    std::string timeControl;
    std::vector<ArchivedGamePlayer> players;
    std::string result;
    int winnerSeat = 0;
    std::string termination;
    int plyCount = 0;
    std::int64_t startedAt = 0;
    std::int64_t endedAt = 0;
    std::string finalFen;
    std::string octiTranscript;
};

struct ArchivedGamesPage {
    std::int64_t total = 0;
    int limit = 20;
    int offset = 0;
    std::vector<ArchivedGameSummary> games;
};

struct PlayerProfileInfo {
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
    std::int64_t createdAt = 0;
    std::int64_t lastSeenAt = 0;
    std::vector<LinkedIdentityInfo> identities;
    std::vector<ArchivedGameSummary> recentGames;
};

struct SsoLoginData {
    std::string sessionToken;
    std::int64_t expiresAt = 0;
    bool isNewAccount = false;
    PlayerProfileInfo profile;
    std::vector<LinkedIdentityInfo> identities;
};

struct LinkIdentityData {
    std::string status;
    std::vector<LinkedIdentityInfo> identities;
    bool conflictAlreadyLinked = false;
    PlayerProfileInfo existingProfile;
};

struct UnlinkIdentityData {
    std::string status;
    std::vector<LinkedIdentityInfo> identities;
};

struct LeaderboardRow {
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

struct LobbyPlayerInfo {
    std::string playerUuid;
    std::string handle;
    std::string status;
    std::string roomId;
};

struct LobbyChatEntry {
    std::int64_t seq = 0;
    std::int64_t atMs = 0;
    std::string playerUuid;
    std::string handle;
    std::string text;
};

struct TableSummaryInfo {
    std::string roomId;
    std::string mode = "2P";
    int bases = 3;
    bool superProngs = false;
    bool edgeless = false;
    bool timeControlEnabled = false;
    std::int64_t initialMs = 0;
    std::int64_t incrementMs = 0;
    std::int64_t disconnectGraceMs = 90000;
    bool rated = true;
    bool allowTakeback = false;
    std::string observerPolicy = "OPEN";
    std::string voicePolicy = "PLAYERS_ONLY";
    std::string phase = "WAITING_FOR_PLAYERS";
    bool hasPasscode = false;
    std::string hostHandle;
    int seatsTotal = 2;
    int seatsFilled = 0;
    std::vector<std::string> players;
    int observerCount = 0;
};

struct LobbyStateData {
    std::vector<LobbyPlayerInfo> players;
    std::vector<TableSummaryInfo> tables;
    std::vector<LobbyChatEntry> recentChat;
    std::int64_t latestSeq = 0;
};

struct CreateTableRequest {
    std::string mode = "2P";
    int bases = 3;
    bool superProngs = false;
    bool edgeless = false;
    bool timeControlEnabled = false;
    std::int64_t initialMs = 600000;
    std::int64_t incrementMs = 5000;
    std::int64_t disconnectGraceMs = 90000;
    bool rated = true;
    bool allowTakeback = false;
    std::string observerPolicy = "OPEN";
    std::string voicePolicy = "PLAYERS_ONLY";
    std::string passcode;
};

struct CreateTableResponse {
    std::string roomId;
    int seat = 1;
};

struct JoinTableResponse {
    std::string roomId;
    int seat = 0;
    bool gameStarted = false;
};

struct MatchmakingRequest {
    std::string mode = "2P";
    int bases = 3;
    bool superProngs = false;
    bool edgeless = false;
    bool timeControlEnabled = false;
    std::int64_t initialMs = 0;
    std::int64_t incrementMs = 0;
};

struct MatchmakingStatusData {
    bool queued = false;
    std::int64_t waitedMs = 0;
    int window = 0;
    int rating = 1200;
    std::string matchedRoomId;
};

struct GameSeatInfo {
    int seat = 0;
    std::string playerUuid;
    std::string handle;
    int rating = 1200;
    bool connected = true;
    std::int64_t disconnectedForMs = 0;
    bool forfeitClaimable = false;
    std::int64_t clockMs = -1;
    bool eliminated = false;
};

struct GameProposalInfo {
    std::string type;
    std::string proposerUuid;
    std::vector<std::string> acceptedBy;
    std::vector<std::string> awaiting;
    std::int64_t expiresAtMs = 0;
};

struct VoicePeerInfo {
    std::string playerUuid;
    bool micMuted = false;
};

struct GameViewData {
    bool live = true;
    std::string roomId;
    std::string mode = "2P";
    int bases = 3;
    bool superProngs = false;
    bool edgeless = false;
    bool timeControlEnabled = false;
    std::int64_t initialMs = 0;
    std::int64_t incrementMs = 0;
    std::int64_t disconnectGraceMs = 90000;
    bool rated = true;
    bool allowTakeback = false;
    std::string observerPolicy = "OPEN";
    std::string voicePolicy = "PLAYERS_ONLY";
    std::string phase = "WAITING_FOR_PLAYERS";
    bool paused = false;
    bool hasPasscode = false;
    std::string hostUuid;
    int viewerSeat = 0;
    std::vector<GameSeatInfo> seats;
    std::vector<std::string> observers;
    int observerCount = 0;
    std::string fen;
    int turnSeat = 0;
    int plyCount = 0;
    std::vector<std::string> moves;
    std::string result;
    std::string termination;
    int winnerSeat = 0;
    std::vector<GameProposalInfo> proposals;
    std::vector<VoicePeerInfo> voicePeers;
    std::int64_t seq = 0;
    bool archived = false;
    std::string rematchRoomId;
    std::int64_t createdAtMs = 0;
};

struct MoveSubmitData {
    std::string move;
    std::int64_t clockMs = -1;
    std::int64_t elapsedMs = 0;
    bool gameOver = false;
    std::int64_t seq = 0;
};

struct GameActionData {
    std::string action;
    std::string roomId;
    bool gameOver = false;
    std::int64_t seq = 0;
};

struct RoomSettingsData {
    std::string observerPolicy;
    std::string voicePolicy;
    std::int64_t seq = 0;
};

struct SseEvent {
    std::uint64_t id = 0;
    std::string event;
    std::string dataJson;
};

/*
 * Ephemeral local HTTP callback listener bound strictly to 127.0.0.1:0 for
 * system-browser OAuth SSO sign-in redirects.
 */
class OAuthLoopbackListener {
public:
    OAuthLoopbackListener();
    ~OAuthLoopbackListener();

    OAuthLoopbackListener(const OAuthLoopbackListener &) = delete;
    OAuthLoopbackListener &operator=(const OAuthLoopbackListener &) = delete;

    bool start(
        std::function<void(const std::string &provider,
                           const std::string &token)> onCallback,
        std::string &errorOut);
    void stop();

    bool isRunning() const;
    int boundPort() const;
    std::string callbackUrl() const;

    bool waitForCallback(int timeoutMs,
                         std::string &outProvider,
                         std::string &outToken);

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

/*
 * Hard-coded REST + Server-Sent Events (SSE) client for trumpet.selfso.com:6874.
 *
 * Production instances created via OctiNetworkClient() always connect to
 * OCTI_SERVER_HOST ("trumpet.selfso.com") and OCTI_SERVER_PORT (6874), with no
 * public mutator to change the server host or port.
 */
class OctiNetworkClient {
public:
    static constexpr const char *canonicalHost() { return OCTI_SERVER_HOST; }
    static constexpr int canonicalPort() { return OCTI_SERVER_PORT; }

    OctiNetworkClient();
    ~OctiNetworkClient();

    OctiNetworkClient(const OctiNetworkClient &) = delete;
    OctiNetworkClient &operator=(const OctiNetworkClient &) = delete;

    static std::unique_ptr<OctiNetworkClient> createForLoopbackTest(
        int loopbackPort);
    static std::unique_ptr<OctiNetworkClient> forHermeticTest(
        int loopbackPort)
    {
        return createForLoopbackTest(loopbackPort);
    }

    const std::string &host() const;
    int port() const;

    void setSessionToken(const std::string &token);
    std::string sessionToken() const;
    void clearSessionToken();

    /* ---- 1..10: Health, Auth, Profile & Leaderboard ---- */
    ApiResult<HealthInfo> fetchHealth();
    ApiResult<std::vector<AuthProviderInfo>> fetchAuthProviders();
    ApiResult<std::vector<AuthProviderInfo>> fetchProviders()
    {
        return fetchAuthProviders();
    }
    ApiResult<SsoLoginData> loginWithSso(
        const std::string &provider,
        const std::string &token,
        const std::string &desiredHandle = "",
        bool isAccessToken = false);
    ApiResult<LinkIdentityData> linkIdentity(
        const std::string &provider,
        const std::string &token,
        bool isAccessToken = false);
    ApiResult<UnlinkIdentityData> unlinkIdentity(const std::string &provider);
    ApiResult<bool> logout();
    ApiResult<PlayerProfileInfo> fetchMyProfile();
    ApiResult<PlayerProfileInfo> updateMyProfile(
        const std::string &handle,
        const std::string &avatarUrl = "",
        const std::string &displayName = "");
    ApiResult<PlayerProfileInfo> fetchPlayerProfile(
        const std::string &playerUuid);
    ApiResult<std::vector<LeaderboardRow>> fetchLeaderboard(
        const std::string &mode = "2P",
        int limit = 50);

    /* ---- 11..22: Lobby, Tables & Matchmaking ---- */
    ApiResult<LobbyStateData> fetchLobbyState();
    ApiResult<std::int64_t> postLobbyChat(const std::string &text);
    ApiResult<std::vector<TableSummaryInfo>> listTables(
        bool includeFinished = false);
    ApiResult<CreateTableResponse> createTable(
        const CreateTableRequest &req = CreateTableRequest());
    ApiResult<JoinTableResponse> joinTable(
        const std::string &roomId,
        int seat = 0,
        const std::string &passcode = "");
    ApiResult<bool> leaveTable(const std::string &roomId);
    ApiResult<bool> kickSeat(
        const std::string &roomId,
        const std::string &targetPlayerUuid);
    ApiResult<bool> kickSeat(const std::string &roomId, int seat);
    ApiResult<bool> observeTable(
        const std::string &roomId,
        const std::string &passcode = "");
    ApiResult<bool> unobserveTable(const std::string &roomId);
    ApiResult<bool> enqueueMatchmaking(
        const MatchmakingRequest &req = MatchmakingRequest());
    ApiResult<bool> dequeueMatchmaking();
    ApiResult<MatchmakingStatusData> fetchMatchmakingStatus();

    /* ---- 23..32: Live Game, Chat, Voice Signaling & Moderation ---- */
    ApiResult<GameViewData> fetchGameView(const std::string &roomId);
    ApiResult<MoveSubmitData> submitMove(
        const std::string &roomId,
        const std::string &moveStr);
    ApiResult<bool> resignGame(const std::string &roomId);
    ApiResult<GameActionData> submitGameAction(
        const std::string &roomId,
        const std::string &action);
    ApiResult<GameActionData> submitAction(
        const std::string &roomId,
        const std::string &action)
    {
        return submitGameAction(roomId, action);
    }
    ApiResult<std::int64_t> postGameChat(
        const std::string &roomId,
        const std::string &scope,
        const std::string &text);
    ApiResult<std::int64_t> submitVoiceSignal(
        const std::string &roomId,
        const std::string &toPlayerUuid,
        const std::string &signalType,
        const std::string &payloadJson);
    ApiResult<std::int64_t> submitVoiceMeta(
        const std::string &roomId,
        const std::string &eventType,
        const std::string &targetPlayerUuid = "");
    ApiResult<bool> banObserver(
        const std::string &roomId,
        const std::string &targetPlayerUuid,
        const std::string &reason = "");
    ApiResult<bool> muteParticipant(
        const std::string &roomId,
        const std::string &targetPlayerUuid,
        bool muteChat,
        bool muteVoice);
    ApiResult<RoomSettingsData> updateRoomSettings(
        const std::string &roomId,
        const std::string &observerPolicy,
        const std::string &voicePolicy);

    /* ---- 33..35: Game Archive & .octi Transcript ---- */
    ApiResult<ArchivedGamesPage> listArchivedGames(
        const std::string &mode = "",
        const std::string &playerUuid = "",
        int limit = 20,
        int offset = 0);
    ApiResult<ArchivedGameSummary> fetchArchivedGame(
        const std::string &gameId);
    ApiResult<std::string> downloadOctiTranscript(
        const std::string &gameId);

    /* ---- Poll & Server-Sent Events (SSE) Live Streams ---- */
    ApiResult<std::vector<SseEvent>> pollLobbyEvents(
        std::uint64_t sinceSeq = 0,
        int limit = 100);
    ApiResult<std::vector<SseEvent>> pollGameEvents(
        const std::string &roomId,
        std::uint64_t sinceSeq = 0,
        int limit = 100);

    void startLobbyEventStream(
        std::uint64_t sinceSeq,
        std::function<void(const SseEvent &)> onEvent);
    void stopLobbyEventStream();
    bool isLobbyEventStreamActive() const;
    std::uint64_t lastLobbyEventId() const;

    void startGameEventStream(
        const std::string &roomId,
        std::uint64_t sinceSeq,
        std::function<void(const SseEvent &)> onEvent);
    void stopGameEventStream();
    bool isGameEventStreamActive() const;
    std::uint64_t lastGameEventId() const;

    void stopEventStreams();

    static std::vector<SseEvent> parseSseBuffer(std::string &buffer);

private:
    OctiNetworkClient(const std::string &host, int port);

    struct Impl;
    std::unique_ptr<Impl> _impl;
    std::string _host;
    int _port;
};

} // namespace octi

#endif // OCTI_DESKTOP_OCTI_NETWORK_CLIENT_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
