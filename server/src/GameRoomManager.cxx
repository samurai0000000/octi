/*
 * GameRoomManager.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "GameRoomManager.hxx"

#include "EloCalculator.hxx"
#include "OctiBoard.hxx"
#include "OctiExceptions.hxx"
#include "OctiGame.hxx"
#include "OctiMove.hxx"
#include "OctiSquare.hxx"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <exception>
#include <sstream>

namespace octi {

/* ------------------------------------------------------------------ */
/* Names                                                               */
/* ------------------------------------------------------------------ */

const char *toString(ObserverPolicy v)
{
    switch (v) {
    case ObserverPolicy::OPEN: return "OPEN";
    case ObserverPolicy::SILENT_OBSERVERS: return "SILENT_OBSERVERS";
    case ObserverPolicy::NO_OBSERVERS: return "NO_OBSERVERS";
    }
    return "OPEN";
}

const char *toString(VoicePolicy v)
{
    switch (v) {
    case VoicePolicy::DISABLED: return "DISABLED";
    case VoicePolicy::PLAYERS_ONLY: return "PLAYERS_ONLY";
    case VoicePolicy::ALL_ALLOWED: return "ALL_ALLOWED";
    }
    return "PLAYERS_ONLY";
}

const char *toString(PresenceStatus v)
{
    switch (v) {
    case PresenceStatus::OFFLINE: return "OFFLINE";
    case PresenceStatus::IN_LOBBY: return "IN_LOBBY";
    case PresenceStatus::AT_TABLE: return "AT_TABLE";
    case PresenceStatus::PLAYING: return "PLAYING";
    case PresenceStatus::OBSERVING: return "OBSERVING";
    }
    return "OFFLINE";
}

const char *toString(RoomPhase v)
{
    switch (v) {
    case RoomPhase::WAITING_FOR_PLAYERS: return "WAITING_FOR_PLAYERS";
    case RoomPhase::IN_PROGRESS: return "IN_PROGRESS";
    case RoomPhase::FINISHED: return "FINISHED";
    }
    return "FINISHED";
}

const char *toString(ChatScope v)
{
    return v == ChatScope::TEAM ? "TEAM" : "ALL";
}

const char *toString(VoiceSignalType v)
{
    switch (v) {
    case VoiceSignalType::SDP_OFFER: return "SDP_OFFER";
    case VoiceSignalType::SDP_ANSWER: return "SDP_ANSWER";
    case VoiceSignalType::ICE_CANDIDATE: return "ICE_CANDIDATE";
    }
    return "SDP_OFFER";
}

const char *toString(VoiceMetaAction v)
{
    switch (v) {
    case VoiceMetaAction::JOINED_VOICE: return "JOINED_VOICE";
    case VoiceMetaAction::LEFT_VOICE: return "LEFT_VOICE";
    case VoiceMetaAction::MIC_MUTED: return "MIC_MUTED";
    case VoiceMetaAction::MIC_UNMUTED: return "MIC_UNMUTED";
    }
    return "JOINED_VOICE";
}

const char *toString(ProposalType v)
{
    switch (v) {
    case ProposalType::DRAW_OFFER: return "DRAW_OFFER";
    case ProposalType::PAUSE_REQUEST: return "PAUSE_REQUEST";
    case ProposalType::UNDO_REQUEST: return "UNDO_REQUEST";
    case ProposalType::REMATCH_REQUEST: return "REMATCH_REQUEST";
    }
    return "DRAW_OFFER";
}

const char *toString(RoomCode v)
{
    switch (v) {
    case RoomCode::OK: return "OK";
    case RoomCode::UNKNOWN_PLAYER: return "UNKNOWN_PLAYER";
    case RoomCode::NOT_FOUND: return "NOT_FOUND";
    case RoomCode::INVALID_ARGUMENT: return "INVALID_ARGUMENT";
    case RoomCode::FORBIDDEN_BANNED: return "FORBIDDEN_BANNED";
    case RoomCode::FORBIDDEN_NOT_SEATED: return "FORBIDDEN_NOT_SEATED";
    case RoomCode::FORBIDDEN_NOT_PARTICIPANT:
        return "FORBIDDEN_NOT_PARTICIPANT";
    case RoomCode::FORBIDDEN_MUTED: return "FORBIDDEN_MUTED";
    case RoomCode::FORBIDDEN_POLICY: return "FORBIDDEN_POLICY";
    case RoomCode::FORBIDDEN_SEATED_TARGET: return "FORBIDDEN_SEATED_TARGET";
    case RoomCode::FORBIDDEN_PASSCODE: return "FORBIDDEN_PASSCODE";
    case RoomCode::FORBIDDEN_NOT_HOST: return "FORBIDDEN_NOT_HOST";
    case RoomCode::NOT_YOUR_TURN: return "NOT_YOUR_TURN";
    case RoomCode::ILLEGAL_SYNTAX: return "ILLEGAL_SYNTAX";
    case RoomCode::ILLEGAL_MOVE: return "ILLEGAL_MOVE";
    case RoomCode::INVALID_STATE: return "INVALID_STATE";
    case RoomCode::PAYLOAD_TOO_LARGE: return "PAYLOAD_TOO_LARGE";
    case RoomCode::RATE_LIMITED: return "RATE_LIMITED";
    case RoomCode::SEAT_TAKEN: return "SEAT_TAKEN";
    case RoomCode::ALREADY_SEATED: return "ALREADY_SEATED";
    case RoomCode::CONFLICT: return "CONFLICT";
    case RoomCode::LIMIT_REACHED: return "LIMIT_REACHED";
    case RoomCode::INTERNAL_ERROR: return "INTERNAL_ERROR";
    }
    return "INTERNAL_ERROR";
}

const char *toString(RoomEventType v)
{
    switch (v) {
    case RoomEventType::ROOM_CREATED: return "ROOM_CREATED";
    case RoomEventType::SEAT_JOINED: return "SEAT_JOINED";
    case RoomEventType::SEAT_LEFT: return "SEAT_LEFT";
    case RoomEventType::SEAT_KICKED: return "SEAT_KICKED";
    case RoomEventType::HOST_CHANGED: return "HOST_CHANGED";
    case RoomEventType::OBSERVER_JOINED: return "OBSERVER_JOINED";
    case RoomEventType::OBSERVER_LEFT: return "OBSERVER_LEFT";
    case RoomEventType::GAME_STARTED: return "GAME_STARTED";
    case RoomEventType::MOVE: return "MOVE";
    case RoomEventType::CHAT: return "CHAT";
    case RoomEventType::VOICE_SIGNAL: return "VOICE_SIGNAL";
    case RoomEventType::VOICE_META: return "VOICE_META";
    case RoomEventType::OBSERVER_BANNED: return "OBSERVER_BANNED";
    case RoomEventType::USER_MUTED: return "USER_MUTED";
    case RoomEventType::POLICY_UPDATED: return "POLICY_UPDATED";
    case RoomEventType::PROPOSAL_OFFERED: return "PROPOSAL_OFFERED";
    case RoomEventType::PROPOSAL_ACCEPTED: return "PROPOSAL_ACCEPTED";
    case RoomEventType::PROPOSAL_DECLINED: return "PROPOSAL_DECLINED";
    case RoomEventType::PROPOSAL_EXPIRED: return "PROPOSAL_EXPIRED";
    case RoomEventType::GAME_PAUSED: return "GAME_PAUSED";
    case RoomEventType::GAME_RESUMED: return "GAME_RESUMED";
    case RoomEventType::UNDO_APPLIED: return "UNDO_APPLIED";
    case RoomEventType::REMATCH_CREATED: return "REMATCH_CREATED";
    case RoomEventType::GAME_OVER: return "GAME_OVER";
    case RoomEventType::PLAYER_DISCONNECTED: return "PLAYER_DISCONNECTED";
    case RoomEventType::PLAYER_RECONNECTED: return "PLAYER_RECONNECTED";
    }
    return "ROOM_CREATED";
}

const char *toString(LobbyEventType v)
{
    switch (v) {
    case LobbyEventType::LOBBY_CHAT: return "LOBBY_CHAT";
    case LobbyEventType::PRESENCE: return "PRESENCE";
    case LobbyEventType::TABLE_UPDATED: return "TABLE_UPDATED";
    case LobbyEventType::TABLE_CLOSED: return "TABLE_CLOSED";
    case LobbyEventType::MATCH_FOUND: return "MATCH_FOUND";
    }
    return "LOBBY_CHAT";
}

bool parseObserverPolicy(const std::string &s, ObserverPolicy &out)
{
    if (s == "OPEN") { out = ObserverPolicy::OPEN; return true; }
    if (s == "SILENT_OBSERVERS") {
        out = ObserverPolicy::SILENT_OBSERVERS;
        return true;
    }
    if (s == "NO_OBSERVERS") { out = ObserverPolicy::NO_OBSERVERS; return true; }
    return false;
}

bool parseVoicePolicy(const std::string &s, VoicePolicy &out)
{
    if (s == "DISABLED") { out = VoicePolicy::DISABLED; return true; }
    if (s == "PLAYERS_ONLY") { out = VoicePolicy::PLAYERS_ONLY; return true; }
    if (s == "ALL_ALLOWED") { out = VoicePolicy::ALL_ALLOWED; return true; }
    return false;
}

bool parseChatScope(const std::string &s, ChatScope &out)
{
    if (s == "ALL" || s == "all") { out = ChatScope::ALL; return true; }
    if (s == "TEAM" || s == "team") { out = ChatScope::TEAM; return true; }
    return false;
}

bool parseVoiceSignalType(const std::string &s, VoiceSignalType &out)
{
    if (s == "SDP_OFFER") { out = VoiceSignalType::SDP_OFFER; return true; }
    if (s == "SDP_ANSWER") { out = VoiceSignalType::SDP_ANSWER; return true; }
    if (s == "ICE_CANDIDATE") {
        out = VoiceSignalType::ICE_CANDIDATE;
        return true;
    }
    return false;
}

bool parseVoiceMetaAction(const std::string &s, VoiceMetaAction &out)
{
    if (s == "JOINED_VOICE") { out = VoiceMetaAction::JOINED_VOICE; return true; }
    if (s == "LEFT_VOICE") { out = VoiceMetaAction::LEFT_VOICE; return true; }
    if (s == "MIC_MUTED") { out = VoiceMetaAction::MIC_MUTED; return true; }
    if (s == "MIC_UNMUTED") { out = VoiceMetaAction::MIC_UNMUTED; return true; }
    return false;
}

bool parseProposalType(const std::string &s, ProposalType &out)
{
    if (s == "DRAW_OFFER") { out = ProposalType::DRAW_OFFER; return true; }
    if (s == "PAUSE_REQUEST") { out = ProposalType::PAUSE_REQUEST; return true; }
    if (s == "UNDO_REQUEST") { out = ProposalType::UNDO_REQUEST; return true; }
    if (s == "REMATCH_REQUEST") {
        out = ProposalType::REMATCH_REQUEST;
        return true;
    }
    return false;
}

/* ------------------------------------------------------------------ */
/* Internal helpers                                                    */
/* ------------------------------------------------------------------ */

namespace {

using Json = nlohmann::json;

const std::string kEmptyString;

RoomResult fail(RoomCode code, const std::string &message)
{
    RoomResult r;
    r.code = code;
    r.message = message;
    return r;
}

RoomResult success()
{
    return RoomResult();
}

bool isValidUtf8(const std::string &s)
{
    size_t i = 0;
    const size_t n = s.size();
    while (i < n) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) {
            ++i;
            continue;
        }
        size_t len;
        unsigned int cp;
        if (c >= 0xC2 && c <= 0xDF) {
            len = 2;
            cp = c & 0x1F;
        } else if (c >= 0xE0 && c <= 0xEF) {
            len = 3;
            cp = c & 0x0F;
        } else if (c >= 0xF0 && c <= 0xF4) {
            len = 4;
            cp = c & 0x07;
        } else {
            return false;
        }
        if (i + len > n) {
            return false;
        }
        for (size_t k = 1; k < len; ++k) {
            unsigned char cc = static_cast<unsigned char>(s[i + k]);
            if ((cc & 0xC0) != 0x80) {
                return false;
            }
            cp = (cp << 6) | (cc & 0x3F);
        }
        if ((len == 3 && cp < 0x800) || (len == 4 && cp < 0x10000) ||
            cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
            return false;
        }
        i += len;
    }
    return true;
}

bool hasControlChars(const std::string &s)
{
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x20 || c == 0x7F) {
            return true;
        }
    }
    return false;
}

std::string trimmed(const std::string &s)
{
    size_t b = 0;
    size_t e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t')) {
        ++b;
    }
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t')) {
        --e;
    }
    return s.substr(b, e - b);
}

/* Validates and normalizes chat / reason text. */
bool cleanText(const std::string &in, size_t maxBytes, std::string &out,
               std::string &error)
{
    std::string t = trimmed(in);
    if (t.empty()) {
        error = "text is empty";
        return false;
    }
    if (t.size() > maxBytes) {
        error = "text is too long";
        return false;
    }
    if (hasControlChars(t) || !isValidUtf8(t)) {
        error = "text contains control characters or invalid UTF-8";
        return false;
    }
    out = t;
    return true;
}

bool constantTimeEquals(const std::string &a, const std::string &b)
{
    size_t n = std::max(a.size(), b.size());
    unsigned char diff = static_cast<unsigned char>(a.size() != b.size());
    for (size_t i = 0; i < n; ++i) {
        unsigned char ca = i < a.size() ? static_cast<unsigned char>(a[i]) : 0;
        unsigned char cb = i < b.size() ? static_cast<unsigned char>(b[i]) : 0;
        diff = static_cast<unsigned char>(diff | (ca ^ cb));
    }
    return diff == 0;
}

int64_t steadyMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

int64_t systemSec()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string dateOf(int64_t epochSec)
{
    time_t t = static_cast<time_t>(epochSec);
    struct tm tmv;
    if (gmtime_r(&t, &tmv) == nullptr) {
        return "1970-01-01";
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tmv.tm_year + 1900,
                  tmv.tm_mon + 1, tmv.tm_mday);
    return buf;
}

std::string compactDateOf(int64_t epochSec)
{
    std::string d = dateOf(epochSec);
    std::string c;
    for (size_t i = 0; i < d.size(); ++i) {
        if (d[i] != '-') {
            c += d[i];
        }
    }
    return c;
}

const char *modeName(OctiGameMode m)
{
    switch (m) {
    case GAME_MODE_2: return "2P";
    case GAME_MODE_4: return "4P";
    case GAME_MODE_OFK: return "OFK";
    default: break;
    }
    return "2P";
}

int seatCountOf(OctiGameMode m)
{
    return m == GAME_MODE_4 ? 4 : 2;
}

/* 4P: seats 1,3 are team 1 and seats 2,4 team 2; otherwise one team each. */
int teamOf(OctiGameMode m, int seat)
{
    if (m == GAME_MODE_4) {
        return (seat % 2 == 1) ? 1 : 2;
    }
    return seat;
}

std::string timeControlString(const TimeControlConfig &tc)
{
    if (!tc.enabled) {
        return "untimed";
    }
    char buf[48];
    if (tc.initialMs % 1000 == 0 && tc.incrementMs % 1000 == 0) {
        std::snprintf(buf, sizeof(buf), "%lld+%lld",
                      static_cast<long long>(tc.initialMs / 1000),
                      static_cast<long long>(tc.incrementMs / 1000));
    } else {
        std::snprintf(buf, sizeof(buf), "%lldms+%lldms",
                      static_cast<long long>(tc.initialMs),
                      static_cast<long long>(tc.incrementMs));
    }
    return buf;
}

std::string cleanHeaderValue(const std::string &s)
{
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        out += (c < 0x20 || c == 0x7F) ? '_' : static_cast<char>(c);
    }
    return out;
}

bool validateConfig(const GameTableConfig &in, GameTableConfig &out,
                    std::string &error)
{
    out = in;
    if (in.mode != GAME_MODE_2 && in.mode != GAME_MODE_4 &&
        in.mode != GAME_MODE_OFK) {
        error = "invalid game mode";
        return false;
    }
    if (in.bases != GAME_BASES_1 && in.bases != GAME_BASES_3) {
        error = "invalid bases";
        return false;
    }
    int op = static_cast<int>(in.observerPolicy);
    int vp = static_cast<int>(in.voicePolicy);
    if (op < 0 || op > static_cast<int>(ObserverPolicy::NO_OBSERVERS) ||
        vp < 0 || vp > static_cast<int>(VoicePolicy::ALL_ALLOWED)) {
        error = "invalid policy";
        return false;
    }
    const TimeControlConfig &tc = in.timeControl;
    if (tc.disconnectGraceMs < 0 ||
        tc.disconnectGraceMs > GameRoomManager::kMaxGraceMs) {
        error = "invalid disconnect grace";
        return false;
    }
    if (tc.enabled) {
        if (tc.initialMs < 1 || tc.initialMs > GameRoomManager::kMaxClockSettingMs ||
            tc.incrementMs < 0 ||
            tc.incrementMs > GameRoomManager::kMaxClockSettingMs) {
            error = "invalid time control";
            return false;
        }
    } else {
        out.timeControl.initialMs = 0;
        out.timeControl.incrementMs = 0;
    }
    if (in.passcode.size() > GameRoomManager::kMaxPasscodeBytes ||
        hasControlChars(in.passcode) || !isValidUtf8(in.passcode)) {
        error = "invalid passcode";
        return false;
    }
    if (out.mode == GAME_MODE_OFK) {
        out.bases = GAME_BASES_1;
        out.useSuperProngs = false;
    } else if (out.mode == GAME_MODE_4) {
        out.bases = GAME_BASES_3;
    }
    if (out.rated) {
        out.allowTakeback = false;
    }
    return true;
}

} // namespace

/* ------------------------------------------------------------------ */
/* Room state                                                          */
/* ------------------------------------------------------------------ */

struct GameRoomManager::Room {
    struct Ply {
        int seat = 0;
        std::string move;
        int64_t clockBeforeMs = 0;
        int64_t clockAfterMs = 0;
        int64_t elapsedMs = 0;
    };

    struct Mute {
        bool chat = false;
        bool voice = false;
    };

    struct Proposal {
        ProposalType type = ProposalType::DRAW_OFFER;
        int64_t proposerId = 0;
        std::set<int64_t> accepted;
        int64_t expiresAtMs = 0;
    };

    std::string id;
    GameTableConfig config;
    RoomPhase phase = RoomPhase::WAITING_FOR_PLAYERS;
    int64_t hostId = 0;
    int seatCount = 2;

    int64_t seatPlayer[5] = {0, 0, 0, 0, 0};
    std::string seatHandle[5];
    int seatRating[5] = {0, 0, 0, 0, 0};
    bool seatConnected[5] = {true, true, true, true, true};
    int64_t seatDisconnectedAt[5] = {0, 0, 0, 0, 0};

    std::unique_ptr<OctiBoard> board;
    int64_t clockMs[5] = {0, 0, 0, 0, 0};
    int64_t turnStartedMs = 0;
    int64_t turnCarryMs = 0;    // time already charged before a pause
    bool paused = false;
    std::vector<Ply> plies;

    std::set<int64_t> observers;
    std::set<int64_t> banned;
    std::map<int64_t, Mute> muted;
    std::map<int64_t, bool> voicePeers;     // player -> mic muted

    std::deque<RoomEvent> events;
    int64_t nextSeq = 1;
    int64_t evictedUpToSeq = 0;

    std::vector<Proposal> proposals;

    std::string result;
    std::string termination;
    int winnerSeat = 0;

    bool archived = false;
    std::string archiveError;
    int archiveAttempts = 0;
    std::string rematchRoomId;

    int64_t createdAtMs = 0;
    int64_t lastActivityMs = 0;
    int64_t endedAtMs = 0;
    int64_t startedAtSec = 0;
    int64_t endedAtSec = 0;

    std::map<int64_t, std::deque<int64_t>> chatTimes;
};

namespace {

const int kMaxArchiveAttempts = 8;

} // namespace

/* ------------------------------------------------------------------ */
/* Construction, clocks, players, presence                             */
/* ------------------------------------------------------------------ */

namespace {

bool rateAllow(std::deque<int64_t> &times, int64_t now, int burst,
               int64_t windowMs)
{
    while (!times.empty() && times.front() <= now - windowMs) {
        times.pop_front();
    }
    if (static_cast<int>(times.size()) >= burst) {
        return false;
    }
    times.push_back(now);
    return true;
}

} // namespace

GameRoomManager::GameRoomManager(AccountStore &store)
    : _store(store),
      _clock(steadyMs),
      _wallClock(systemSec),
      _customWallClock(false),
      _restoring(false),
      _lobbySeq(0),
      _lobbyEvictedUpToSeq(0),
      _lobbyChatCount(0),
      _roomCounter(0)
{
    _queue.setClock([this]() -> int64_t { return _clock(); });
    restoreFromStoreLocked();
}

GameRoomManager::~GameRoomManager() = default;

bool GameRoomManager::checkpointToStore()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return checkpointToStoreLocked();
}

bool GameRoomManager::restoreFromStore()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return restoreFromStoreLocked();
}

void GameRoomManager::setClock(std::function<int64_t()> nowMs)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _clock = nowMs ? std::move(nowMs) : std::function<int64_t()>(steadyMs);
    _queue.setClock([this]() -> int64_t { return _clock(); });
    restoreFromStoreLocked();
}

void GameRoomManager::setWallClock(std::function<int64_t()> epochSec)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _customWallClock = (epochSec != nullptr);
    _wallClock = epochSec ? std::move(epochSec)
                          : std::function<int64_t()>(systemSec);
    restoreFromStoreLocked();
}

void GameRoomManager::setLimits(const RoomLimits &limits)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _limits = limits;
    _limits.maxRoomEvents = std::max<size_t>(1, _limits.maxRoomEvents);
    _limits.maxLobbyEvents = std::max<size_t>(1, _limits.maxLobbyEvents);
    _limits.maxLobbyChat = std::max<size_t>(1, _limits.maxLobbyChat);
    _limits.maxPendingSignals = std::max<size_t>(1, _limits.maxPendingSignals);
    _limits.lobbyChatBurst = std::max(1, _limits.lobbyChatBurst);
    _limits.boardChatBurst = std::max(1, _limits.boardChatBurst);
    _limits.lobbyChatWindowMs = std::max<int64_t>(1, _limits.lobbyChatWindowMs);
    _limits.boardChatWindowMs = std::max<int64_t>(1, _limits.boardChatWindowMs);
}

void GameRoomManager::setMatchmakingConfig(const MatchmakingConfig &config)
{
    _queue.setConfig(config);
}

int64_t GameRoomManager::nowMs() const
{
    return _clock();
}

int64_t GameRoomManager::wallSec() const
{
    return _wallClock();
}

bool GameRoomManager::ensurePlayerLocked(int64_t playerId, bool refresh)
{
    if (playerId <= 0) {
        return false;
    }
    std::map<int64_t, PlayerInfo>::iterator it = _players.find(playerId);
    if (it != _players.end() && !refresh) {
        return true;
    }
    PlayerProfile p;
    if (!_store.getProfileById(playerId, p)) {
        return false;
    }
    _players[playerId].handle = p.handle;
    return true;
}

const std::string &GameRoomManager::handleOf(int64_t playerId) const
{
    std::map<int64_t, PlayerInfo>::const_iterator it = _players.find(playerId);
    return it == _players.end() ? kEmptyString : it->second.handle;
}

void GameRoomManager::touchOnlineLocked(int64_t playerId)
{
    PlayerInfo &pi = _players[playerId];
    if (!pi.online) {
        pi.online = true;
        refreshPresenceLocked(playerId);
    }
}

PresenceStatus GameRoomManager::presenceLocked(int64_t playerId,
                                               std::string *roomId) const
{
    if (roomId) {
        roomId->clear();
    }
    std::map<int64_t, PlayerInfo>::const_iterator pit = _players.find(playerId);
    if (pit == _players.end() || !pit->second.online) {
        return PresenceStatus::OFFLINE;
    }
    std::map<int64_t, std::string>::const_iterator sr = _seatRoom.find(playerId);
    if (sr != _seatRoom.end()) {
        const Room *r = findRoomLocked(sr->second);
        if (r != nullptr) {
            if (roomId) {
                *roomId = r->id;
            }
            return r->phase == RoomPhase::IN_PROGRESS ? PresenceStatus::PLAYING
                                                      : PresenceStatus::AT_TABLE;
        }
    }
    std::map<int64_t, std::set<std::string>>::const_iterator ob =
        _observing.find(playerId);
    if (ob != _observing.end()) {
        for (std::set<std::string>::const_iterator it = ob->second.begin();
             it != ob->second.end(); ++it) {
            const Room *r = findRoomLocked(*it);
            if (r != nullptr && r->phase != RoomPhase::FINISHED) {
                if (roomId) {
                    *roomId = r->id;
                }
                return PresenceStatus::OBSERVING;
            }
        }
    }
    return PresenceStatus::IN_LOBBY;
}

void GameRoomManager::refreshPresenceLocked(int64_t playerId)
{
    std::map<int64_t, PlayerInfo>::iterator pit = _players.find(playerId);
    if (pit == _players.end()) {
        return;
    }
    std::string roomId;
    PresenceStatus st = presenceLocked(playerId, &roomId);
    if (st == pit->second.lastPresence) {
        return;
    }
    pit->second.lastPresence = st;
    LobbyEvent e;
    e.type = LobbyEventType::PRESENCE;
    e.playerId = playerId;
    e.handle = pit->second.handle;
    e.status = st;
    e.roomId = roomId;
    pushLobbyEventLocked(e);
}

PresenceStatus GameRoomManager::presenceOf(int64_t playerId) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return presenceLocked(playerId, nullptr);
}

/* ------------------------------------------------------------------ */
/* Lobby                                                               */
/* ------------------------------------------------------------------ */

void GameRoomManager::pushLobbyEventLocked(LobbyEvent e)
{
    e.seq = ++_lobbySeq;
    e.atMs = nowMs();
    bool isChat = e.type == LobbyEventType::LOBBY_CHAT;
    _lobbyEvents.push_back(std::move(e));
    if (isChat) {
        ++_lobbyChatCount;
        while (_lobbyChatCount > _limits.maxLobbyChat) {
            for (std::deque<LobbyEvent>::iterator it = _lobbyEvents.begin();
                 it != _lobbyEvents.end(); ++it) {
                if (it->type == LobbyEventType::LOBBY_CHAT) {
                    _lobbyEvictedUpToSeq =
                        std::max(_lobbyEvictedUpToSeq, it->seq);
                    _lobbyEvents.erase(it);
                    --_lobbyChatCount;
                    break;
                }
            }
        }
    }
    while (_lobbyEvents.size() > _limits.maxLobbyEvents) {
        const LobbyEvent &front = _lobbyEvents.front();
        if (front.type == LobbyEventType::LOBBY_CHAT && _lobbyChatCount > 0) {
            --_lobbyChatCount;
        }
        _lobbyEvictedUpToSeq = std::max(_lobbyEvictedUpToSeq, front.seq);
        _lobbyEvents.pop_front();
    }
    saveLobbyMetaLocked();
}

void GameRoomManager::announceTableLocked(const Room &r, bool closed)
{
    LobbyEvent e;
    e.type = closed ? LobbyEventType::TABLE_CLOSED
                    : LobbyEventType::TABLE_UPDATED;
    e.roomId = r.id;
    e.playerId = r.hostId;
    e.handle = handleOf(r.hostId);
    pushLobbyEventLocked(e);
}

RoomResult GameRoomManager::enterLobby(int64_t playerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!ensurePlayerLocked(playerId, true)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    PlayerInfo &pi = _players[playerId];
    pi.online = true;
    std::map<int64_t, std::string>::iterator sr = _seatRoom.find(playerId);
    if (sr != _seatRoom.end()) {
        Room *r = findRoomLocked(sr->second);
        if (r != nullptr) {
            advanceRoomLocked(*r, now);
            int seat = seatOfLocked(*r, playerId);
            if (seat != 0 && !r->seatConnected[seat]) {
                r->seatConnected[seat] = true;
                r->seatDisconnectedAt[seat] = 0;
                RoomEvent e;
                e.type = RoomEventType::PLAYER_RECONNECTED;
                e.playerId = playerId;
                e.seat = seat;
                emitLocked(*r, e, now);
            }
        }
    }
    refreshPresenceLocked(playerId);
    return success();
}

RoomResult GameRoomManager::leaveLobby(int64_t playerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!ensurePlayerLocked(playerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    _queue.dequeue(playerId);
    _matchAssignments.erase(playerId);

    std::map<int64_t, std::set<std::string>>::iterator ob =
        _observing.find(playerId);
    if (ob != _observing.end()) {
        std::vector<std::string> ids(ob->second.begin(), ob->second.end());
        for (size_t i = 0; i < ids.size(); ++i) {
            Room *r = findRoomLocked(ids[i]);
            if (r != nullptr) {
                removeObserverLocked(*r, playerId);
                RoomEvent e;
                e.type = RoomEventType::OBSERVER_LEFT;
                e.playerId = playerId;
                e.detail = "left";
                emitLocked(*r, e, now);
            }
        }
        _observing.erase(playerId);
    }

    std::map<int64_t, std::string>::iterator sr = _seatRoom.find(playerId);
    if (sr != _seatRoom.end()) {
        std::string roomId = sr->second;
        Room *r = findRoomLocked(roomId);
        if (r != nullptr) {
            advanceRoomLocked(*r, now);
            int seat = seatOfLocked(*r, playerId);
            if (seat != 0 && r->phase == RoomPhase::WAITING_FOR_PLAYERS) {
                if (leaveWaitingSeatLocked(*r, seat, now)) {
                    _players[playerId].online = false;
                    refreshPresenceLocked(playerId);
                    return success();
                }
            } else if (seat != 0 && r->phase == RoomPhase::IN_PROGRESS &&
                       r->seatConnected[seat]) {
                r->seatConnected[seat] = false;
                r->seatDisconnectedAt[seat] = now;
                RoomEvent e;
                e.type = RoomEventType::PLAYER_DISCONNECTED;
                e.playerId = playerId;
                e.seat = seat;
                emitLocked(*r, e, now);
            }
        }
    }

    _players[playerId].online = false;
    refreshPresenceLocked(playerId);
    return success();
}

RoomResult GameRoomManager::postLobbyChat(int64_t playerId,
                                          const std::string &text)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!ensurePlayerLocked(playerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    std::string clean;
    std::string err;
    if (!cleanText(text, kMaxChatBytes, clean, err)) {
        return fail(RoomCode::INVALID_ARGUMENT, err);
    }
    int64_t now = nowMs();
    touchOnlineLocked(playerId);
    if (!rateAllow(_lobbyChatTimes[playerId], now, _limits.lobbyChatBurst,
                   _limits.lobbyChatWindowMs)) {
        return fail(RoomCode::RATE_LIMITED, "lobby chat rate limit exceeded");
    }
    LobbyEvent e;
    e.type = LobbyEventType::LOBBY_CHAT;
    e.playerId = playerId;
    e.handle = handleOf(playerId);
    e.text = clean;
    pushLobbyEventLocked(e);
    RoomResult r = success();
    r.seq = _lobbySeq;
    return r;
}

LobbyPoll GameRoomManager::pollLobbyEvents(int64_t viewerPlayerId,
                                           int64_t sinceSeq,
                                           size_t limit) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    LobbyPoll out;
    limit = std::max<size_t>(1, std::min<size_t>(limit, 500));
    out.latestSeq = _lobbySeq;
    out.gap = sinceSeq < _lobbyEvictedUpToSeq;
    for (std::deque<LobbyEvent>::const_iterator it = _lobbyEvents.begin();
         it != _lobbyEvents.end(); ++it) {
        if (it->seq <= sinceSeq) {
            continue;
        }
        if (it->targetPlayerId != 0 && it->targetPlayerId != viewerPlayerId) {
            continue;
        }
        if (out.events.size() >= limit) {
            out.hasMore = true;
            break;
        }
        out.events.push_back(*it);
    }
    return out;
}

TableSummary GameRoomManager::summarize(const Room &r) const
{
    TableSummary t;
    t.roomId = r.id;
    t.config = r.config;
    t.config.passcode.clear();
    t.hasPasscode = !r.config.passcode.empty();
    t.phase = r.phase;
    t.hostHandle = handleOf(r.hostId);
    t.seatsTotal = r.seatCount;
    for (int s = 1; s <= r.seatCount; ++s) {
        if (r.seatPlayer[s] != 0) {
            ++t.seatsFilled;
            t.playerHandles.push_back(r.seatHandle[s]);
        }
    }
    t.observerCount = static_cast<int>(r.observers.size());
    return t;
}

std::vector<TableSummary> GameRoomManager::listTables(bool includeFinished) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<TableSummary> out;
    for (std::map<std::string, std::unique_ptr<Room>>::const_iterator it =
             _rooms.begin();
         it != _rooms.end(); ++it) {
        if (!includeFinished && it->second->phase == RoomPhase::FINISHED) {
            continue;
        }
        out.push_back(summarize(*it->second));
    }
    return out;
}

LobbySnapshot GameRoomManager::lobbyState() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    LobbySnapshot snap;
    snap.latestSeq = _lobbySeq;
    for (std::map<int64_t, PlayerInfo>::const_iterator it = _players.begin();
         it != _players.end(); ++it) {
        if (!it->second.online) {
            continue;
        }
        PresenceEntry pe;
        pe.playerId = it->first;
        pe.handle = it->second.handle;
        pe.status = presenceLocked(it->first, &pe.roomId);
        snap.players.push_back(pe);
    }
    for (std::map<std::string, std::unique_ptr<Room>>::const_iterator it =
             _rooms.begin();
         it != _rooms.end(); ++it) {
        if (it->second->phase != RoomPhase::FINISHED) {
            snap.tables.push_back(summarize(*it->second));
        }
    }
    for (std::deque<LobbyEvent>::const_iterator it = _lobbyEvents.begin();
         it != _lobbyEvents.end(); ++it) {
        if (it->type == LobbyEventType::LOBBY_CHAT) {
            snap.recentChat.push_back(*it);
        }
    }
    return snap;
}

/* ------------------------------------------------------------------ */
/* Room lookup, lifecycle, events                                      */
/* ------------------------------------------------------------------ */

GameRoomManager::Room *GameRoomManager::findRoomLocked(const std::string &roomId)
{
    std::map<std::string, std::unique_ptr<Room>>::iterator it =
        _rooms.find(roomId);
    return it == _rooms.end() ? nullptr : it->second.get();
}

const GameRoomManager::Room *
GameRoomManager::findRoomLocked(const std::string &roomId) const
{
    std::map<std::string, std::unique_ptr<Room>>::const_iterator it =
        _rooms.find(roomId);
    return it == _rooms.end() ? nullptr : it->second.get();
}

size_t GameRoomManager::roomCount() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _rooms.size();
}

size_t GameRoomManager::onlinePlayerCount() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    size_t n = 0;
    for (std::map<int64_t, PlayerInfo>::const_iterator it = _players.begin();
         it != _players.end(); ++it) {
        if (it->second.online) {
            ++n;
        }
    }
    return n;
}

std::string GameRoomManager::newRoomIdLocked()
{
    const std::string date = compactDateOf(wallSec());
    std::string id;
    for (int attempt = 0; attempt < 10000; ++attempt) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%04llu",
                      static_cast<unsigned long long>(++_roomCounter));
        id = "g-" + date + "-" + buf;
        GameRecord probe;
        if (_rooms.count(id) == 0 && !_store.getArchivedGame(id, probe)) {
            break;
        }
    }
    return id;
}

GameRoomManager::Room *GameRoomManager::createRoomLocked(const GameTableConfig &cfg,
                                                         int64_t hostId,
                                                         int64_t now)
{
    std::unique_ptr<Room> room(new Room());
    room->id = newRoomIdLocked();
    room->config = cfg;
    room->hostId = hostId;
    room->seatCount = seatCountOf(cfg.mode);
    room->board.reset(new OctiBoard(cfg.mode, cfg.useSuperProngs,
                                    cfg.useEdgelessBoard, cfg.bases));
    for (int s = 1; s <= room->seatCount; ++s) {
        room->clockMs[s] = cfg.timeControl.enabled ? cfg.timeControl.initialMs : 0;
    }
    room->createdAtMs = now;
    room->lastActivityMs = now;
    Room *raw = room.get();
    _rooms[raw->id] = std::move(room);

    RoomEvent e;
    e.type = RoomEventType::ROOM_CREATED;
    e.playerId = hostId;
    emitLocked(*raw, e, now);
    return raw;
}

void GameRoomManager::deleteRoomLocked(const std::string &roomIdRef)
{
    /* Callers may pass the room's own id, which dies with the room. */
    const std::string roomId = roomIdRef;
    std::string delErr;
    _store.deleteLiveRoom(roomId, delErr);

    std::map<std::string, std::unique_ptr<Room>>::iterator it =
        _rooms.find(roomId);
    if (it == _rooms.end()) {
        return;
    }
    std::vector<int64_t> affected;
    Room &r = *it->second;
    for (int s = 1; s <= r.seatCount; ++s) {
        if (r.seatPlayer[s] != 0) {
            affected.push_back(r.seatPlayer[s]);
        }
    }
    affected.insert(affected.end(), r.observers.begin(), r.observers.end());
    announceTableLocked(r, true);
    _rooms.erase(it);

    for (std::map<int64_t, std::string>::iterator sr = _seatRoom.begin();
         sr != _seatRoom.end();) {
        sr = (sr->second == roomId) ? _seatRoom.erase(sr) : std::next(sr);
    }
    for (std::map<int64_t, std::set<std::string>>::iterator ob =
             _observing.begin();
         ob != _observing.end();) {
        ob->second.erase(roomId);
        ob = ob->second.empty() ? _observing.erase(ob) : std::next(ob);
    }
    for (std::map<int64_t, std::string>::iterator ma = _matchAssignments.begin();
         ma != _matchAssignments.end();) {
        ma = (ma->second == roomId) ? _matchAssignments.erase(ma)
                                    : std::next(ma);
    }
    for (size_t i = 0; i < affected.size(); ++i) {
        refreshPresenceLocked(affected[i]);
    }
}

int64_t GameRoomManager::emitLocked(Room &r, RoomEvent e, int64_t now)
{
    e.seq = r.nextSeq++;
    e.atMs = now;
    if (e.handle.empty() && e.playerId != 0) {
        e.handle = handleOf(e.playerId);
    }
    const int64_t seq = e.seq;
    if (e.type == RoomEventType::VOICE_SIGNAL) {
        size_t pending = 0;
        for (size_t i = 0; i < r.events.size(); ++i) {
            if (r.events[i].type == RoomEventType::VOICE_SIGNAL) {
                ++pending;
            }
        }
        while (pending >= _limits.maxPendingSignals) {
            for (std::deque<RoomEvent>::iterator it = r.events.begin();
                 it != r.events.end(); ++it) {
                if (it->type == RoomEventType::VOICE_SIGNAL) {
                    r.events.erase(it);
                    --pending;
                    break;
                }
            }
        }
    }
    r.events.push_back(std::move(e));
    while (r.events.size() > _limits.maxRoomEvents) {
        r.evictedUpToSeq = std::max(r.evictedUpToSeq, r.events.front().seq);
        r.events.pop_front();
    }
    return seq;
}

void GameRoomManager::purgeUserEventsLocked(Room &r, int64_t playerId)
{
    std::deque<RoomEvent> kept;
    for (size_t i = 0; i < r.events.size(); ++i) {
        const RoomEvent &e = r.events[i];
        bool drop = false;
        if (e.playerId == playerId) {
            drop = e.type == RoomEventType::CHAT ||
                   e.type == RoomEventType::VOICE_META ||
                   e.type == RoomEventType::VOICE_SIGNAL ||
                   e.type == RoomEventType::OBSERVER_JOINED ||
                   e.type == RoomEventType::OBSERVER_LEFT;
        }
        if (e.type == RoomEventType::VOICE_SIGNAL && e.targetPlayerId == playerId) {
            drop = true;
        }
        if (!drop) {
            kept.push_back(e);
        }
    }
    r.events.swap(kept);
}

int GameRoomManager::seatOfLocked(const Room &r, int64_t playerId) const
{
    if (playerId <= 0) {
        return 0;
    }
    for (int s = 1; s <= r.seatCount; ++s) {
        if (r.seatPlayer[s] == playerId) {
            return s;
        }
    }
    return 0;
}

bool GameRoomManager::isParticipantLocked(const Room &r, int64_t playerId) const
{
    return seatOfLocked(r, playerId) != 0 || r.observers.count(playerId) != 0;
}

bool GameRoomManager::voiceAllowedLocked(const Room &r, int64_t playerId) const
{
    if (r.banned.count(playerId) != 0 ||
        r.config.voicePolicy == VoicePolicy::DISABLED) {
        return false;
    }
    if (seatOfLocked(r, playerId) != 0) {
        return true;
    }
    return r.config.voicePolicy == VoicePolicy::ALL_ALLOWED &&
           r.config.observerPolicy == ObserverPolicy::OPEN &&
           r.observers.count(playerId) != 0;
}

void GameRoomManager::addObserverLocked(Room &r, int64_t playerId)
{
    r.observers.insert(playerId);
    _observing[playerId].insert(r.id);
    refreshPresenceLocked(playerId);
}

void GameRoomManager::removeObserverLocked(Room &r, int64_t playerId)
{
    r.observers.erase(playerId);
    r.voicePeers.erase(playerId);
    std::map<int64_t, std::set<std::string>>::iterator ob =
        _observing.find(playerId);
    if (ob != _observing.end()) {
        ob->second.erase(r.id);
        if (ob->second.empty()) {
            _observing.erase(ob);
        }
    }
    refreshPresenceLocked(playerId);
}

void GameRoomManager::enforcePoliciesLocked(Room &r, int64_t now)
{
    if (r.config.observerPolicy == ObserverPolicy::NO_OBSERVERS &&
        r.phase != RoomPhase::FINISHED) {
        std::vector<int64_t> evict(r.observers.begin(), r.observers.end());
        for (size_t i = 0; i < evict.size(); ++i) {
            removeObserverLocked(r, evict[i]);
            RoomEvent e;
            e.type = RoomEventType::OBSERVER_LEFT;
            e.playerId = evict[i];
            e.detail = "evicted";
            emitLocked(r, e, now);
        }
    }
    for (std::map<int64_t, bool>::iterator it = r.voicePeers.begin();
         it != r.voicePeers.end();) {
        it = voiceAllowedLocked(r, it->first) ? std::next(it)
                                              : r.voicePeers.erase(it);
    }
    /* Relayed-but-undelivered signals involving a now-ineligible peer. */
    std::deque<RoomEvent> kept;
    for (size_t i = 0; i < r.events.size(); ++i) {
        const RoomEvent &e = r.events[i];
        if (e.type == RoomEventType::VOICE_SIGNAL &&
            (!voiceAllowedLocked(r, e.playerId) ||
             !voiceAllowedLocked(r, e.targetPlayerId))) {
            continue;
        }
        kept.push_back(e);
    }
    r.events.swap(kept);
}

bool GameRoomManager::eventVisibleLocked(const Room &r, const RoomEvent &e,
                                         int64_t viewerId) const
{
    switch (e.type) {
    case RoomEventType::VOICE_SIGNAL:
        return viewerId != 0 && e.targetPlayerId == viewerId;
    case RoomEventType::CHAT:
        if (e.scope == ChatScope::TEAM) {
            int vs = seatOfLocked(r, viewerId);
            return vs != 0 &&
                   teamOf(r.config.mode, vs) == teamOf(r.config.mode, e.seat);
        }
        return true;
    default:
        break;
    }
    return true;
}

RoomResult GameRoomManager::checkViewLocked(const Room &r, int64_t viewerId) const
{
    if (viewerId != 0 && r.banned.count(viewerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    if (r.config.observerPolicy == ObserverPolicy::NO_OBSERVERS &&
        r.phase == RoomPhase::IN_PROGRESS && seatOfLocked(r, viewerId) == 0) {
        return fail(RoomCode::FORBIDDEN_POLICY,
                    "room does not allow observers while in progress");
    }
    return success();
}

/* ------------------------------------------------------------------ */
/* Seats and tables                                                    */
/* ------------------------------------------------------------------ */

void GameRoomManager::seatPlayerLocked(Room &r, int seat, int64_t playerId,
                                       int64_t now)
{
    r.seatPlayer[seat] = playerId;
    r.seatHandle[seat] = handleOf(playerId);
    PlayerProfile prof;
    r.seatRating[seat] = 0;
    if (_store.getProfileById(playerId, prof)) {
        r.seatRating[seat] = r.config.mode == GAME_MODE_4     ? prof.elo4p
                             : r.config.mode == GAME_MODE_OFK ? prof.eloOfk
                                                              : prof.elo2p;
    }
    r.seatConnected[seat] = true;
    r.seatDisconnectedAt[seat] = 0;
    r.lastActivityMs = now;
    _seatRoom[playerId] = r.id;
    _queue.dequeue(playerId);
    _matchAssignments.erase(playerId);
    if (r.observers.count(playerId) != 0) {
        removeObserverLocked(r, playerId);
    }
    RoomEvent e;
    e.type = RoomEventType::SEAT_JOINED;
    e.playerId = playerId;
    e.seat = seat;
    emitLocked(r, e, now);
    refreshPresenceLocked(playerId);
}

void GameRoomManager::unseatLocked(Room &r, int seat)
{
    int64_t pid = r.seatPlayer[seat];
    r.seatPlayer[seat] = 0;
    r.seatHandle[seat].clear();
    r.seatConnected[seat] = true;
    r.seatDisconnectedAt[seat] = 0;
    std::map<int64_t, std::string>::iterator sr = _seatRoom.find(pid);
    if (sr != _seatRoom.end() && sr->second == r.id) {
        _seatRoom.erase(sr);
    }
    refreshPresenceLocked(pid);
}

bool GameRoomManager::leaveWaitingSeatLocked(Room &r, int seat, int64_t now)
{
    int64_t pid = r.seatPlayer[seat];
    RoomEvent e;
    e.type = RoomEventType::SEAT_LEFT;
    e.playerId = pid;
    e.seat = seat;
    emitLocked(r, e, now);
    unseatLocked(r, seat);
    r.lastActivityMs = now;
    if (r.hostId == pid) {
        int next = 0;
        for (int s = 1; s <= r.seatCount; ++s) {
            if (r.seatPlayer[s] != 0) {
                next = s;
                break;
            }
        }
        if (next == 0) {
            deleteRoomLocked(r.id);
            return true;
        }
        r.hostId = r.seatPlayer[next];
        RoomEvent h;
        h.type = RoomEventType::HOST_CHANGED;
        h.playerId = r.hostId;
        emitLocked(r, h, now);
    }
    announceTableLocked(r, false);
    return false;
}

void GameRoomManager::startGameLocked(Room &r, int64_t now)
{
    OctiGame *g = r.board->getGame();
    g->start(PLAYER_A);
    r.phase = RoomPhase::IN_PROGRESS;
    r.paused = false;
    r.plies.clear();
    r.proposals.clear();
    for (int s = 1; s <= r.seatCount; ++s) {
        r.clockMs[s] = r.config.timeControl.enabled ? r.config.timeControl.initialMs : 0;
        r.seatConnected[s] = true;
        r.seatDisconnectedAt[s] = 0;
    }
    r.turnStartedMs = now;
    r.turnCarryMs = 0;
    r.startedAtSec = wallSec();
    r.lastActivityMs = now;

    RoomEvent e;
    e.type = RoomEventType::GAME_STARTED;
    e.detail = modeName(r.config.mode);
    emitLocked(r, e, now);
    for (int s = 1; s <= r.seatCount; ++s) {
        refreshPresenceLocked(r.seatPlayer[s]);
    }
    announceTableLocked(r, false);
}

RoomResult GameRoomManager::createTable(int64_t hostPlayerId,
                                        const GameTableConfig &config)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!ensurePlayerLocked(hostPlayerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    GameTableConfig cfg;
    std::string err;
    if (!validateConfig(config, cfg, err)) {
        return fail(RoomCode::INVALID_ARGUMENT, err);
    }
    if (_seatRoom.count(hostPlayerId) != 0) {
        return fail(RoomCode::ALREADY_SEATED, "already seated at a table");
    }
    if (_rooms.size() >= _limits.maxRooms) {
        return fail(RoomCode::LIMIT_REACHED, "too many rooms");
    }
    int64_t now = nowMs();
    touchOnlineLocked(hostPlayerId);
    Room *r = createRoomLocked(cfg, hostPlayerId, now);
    seatPlayerLocked(*r, 1, hostPlayerId, now);
    announceTableLocked(*r, false);
    saveRoomLocked(*r);
    saveLobbyMetaLocked();
    RoomResult res = success();
    res.roomId = r->id;
    res.seat = 1;
    return res;
}

RoomResult GameRoomManager::joinTable(const std::string &roomId,
                                      int64_t playerId, int seat,
                                      const std::string &passcode)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(playerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->banned.count(playerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    if (r->phase != RoomPhase::WAITING_FOR_PLAYERS) {
        return fail(RoomCode::INVALID_STATE, "table is not waiting for players");
    }
    if (_seatRoom.count(playerId) != 0) {
        return fail(RoomCode::ALREADY_SEATED, "already seated at a table");
    }
    if (!r->config.passcode.empty() &&
        !constantTimeEquals(r->config.passcode, passcode)) {
        return fail(RoomCode::FORBIDDEN_PASSCODE, "wrong passcode");
    }
    if (seat < 0 || seat > r->seatCount) {
        return fail(RoomCode::INVALID_ARGUMENT, "seat out of range");
    }
    if (seat == 0) {
        for (int s = 1; s <= r->seatCount; ++s) {
            if (r->seatPlayer[s] == 0) {
                seat = s;
                break;
            }
        }
        if (seat == 0) {
            return fail(RoomCode::SEAT_TAKEN, "table is full");
        }
    } else if (r->seatPlayer[seat] != 0) {
        return fail(RoomCode::SEAT_TAKEN, "seat is taken");
    }
    touchOnlineLocked(playerId);
    seatPlayerLocked(*r, seat, playerId, now);
    bool full = true;
    for (int s = 1; s <= r->seatCount; ++s) {
        if (r->seatPlayer[s] == 0) {
            full = false;
        }
    }
    RoomResult res = success();
    res.roomId = r->id;
    res.seat = seat;
    if (full) {
        startGameLocked(*r, now);
        res.gameStarted = true;
    } else {
        announceTableLocked(*r, false);
    }
    saveRoomLocked(*r);
    return res;
}

RoomResult GameRoomManager::leaveTable(const std::string &roomId,
                                       int64_t playerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(playerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    int seat = seatOfLocked(*r, playerId);
    if (seat == 0) {
        return fail(RoomCode::FORBIDDEN_NOT_SEATED, "not seated at this table");
    }
    if (r->phase != RoomPhase::WAITING_FOR_PLAYERS) {
        return fail(RoomCode::INVALID_STATE,
                    "game already started; resign instead");
    }
    leaveWaitingSeatLocked(*r, seat, now);
    if (findRoomLocked(roomId) != nullptr) {
        saveRoomLocked(*r);
    }
    return success();
}

RoomResult GameRoomManager::kickSeat(const std::string &roomId,
                                     int64_t hostPlayerId,
                                     int64_t targetPlayerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(hostPlayerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->banned.count(hostPlayerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    if (seatOfLocked(*r, hostPlayerId) == 0) {
        return fail(RoomCode::FORBIDDEN_NOT_SEATED, "not seated at this table");
    }
    if (r->hostId != hostPlayerId) {
        return fail(RoomCode::FORBIDDEN_NOT_HOST, "only the host can kick");
    }
    if (r->phase != RoomPhase::WAITING_FOR_PLAYERS) {
        return fail(RoomCode::INVALID_STATE, "game already started");
    }
    int ts = seatOfLocked(*r, targetPlayerId);
    if (ts == 0 || targetPlayerId == hostPlayerId) {
        return fail(RoomCode::INVALID_ARGUMENT, "target is not another seated player");
    }
    RoomEvent e;
    e.type = RoomEventType::SEAT_KICKED;
    e.playerId = hostPlayerId;
    e.targetPlayerId = targetPlayerId;
    e.seat = ts;
    RoomResult res = success();
    res.seq = emitLocked(*r, e, now);
    unseatLocked(*r, ts);
    r->lastActivityMs = now;
    announceTableLocked(*r, false);
    saveRoomLocked(*r);
    return res;
}

RoomResult GameRoomManager::observeRoom(const std::string &roomId,
                                        int64_t playerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(playerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->banned.count(playerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    if (seatOfLocked(*r, playerId) != 0) {
        return fail(RoomCode::INVALID_STATE, "seated players need not observe");
    }
    if (r->config.observerPolicy == ObserverPolicy::NO_OBSERVERS &&
        r->phase != RoomPhase::FINISHED) {
        return fail(RoomCode::FORBIDDEN_POLICY, "room does not allow observers");
    }
    RoomResult res = success();
    res.roomId = r->id;
    if (r->observers.count(playerId) != 0) {
        return res;
    }
    touchOnlineLocked(playerId);
    addObserverLocked(*r, playerId);
    RoomEvent e;
    e.type = RoomEventType::OBSERVER_JOINED;
    e.playerId = playerId;
    res.seq = emitLocked(*r, e, now);
    saveRoomLocked(*r);
    return res;
}

RoomResult GameRoomManager::stopObserving(const std::string &roomId,
                                          int64_t playerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(playerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->observers.count(playerId) == 0) {
        return fail(RoomCode::INVALID_STATE, "not observing this room");
    }
    removeObserverLocked(*r, playerId);
    RoomEvent e;
    e.type = RoomEventType::OBSERVER_LEFT;
    e.playerId = playerId;
    e.detail = "left";
    RoomResult res = success();
    res.seq = emitLocked(*r, e, now);
    saveRoomLocked(*r);
    return res;
}

/* ------------------------------------------------------------------ */
/* Matchmaking and housekeeping                                        */
/* ------------------------------------------------------------------ */

RoomResult GameRoomManager::enqueueMatchmaking(int64_t playerId,
                                               const MatchRuleset &ruleset)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!ensurePlayerLocked(playerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    MatchRuleset rs = ruleset.normalized();
    if (!rs.isValid()) {
        return fail(RoomCode::INVALID_ARGUMENT, "invalid ruleset");
    }
    if (_seatRoom.count(playerId) != 0) {
        return fail(RoomCode::ALREADY_SEATED, "already seated at a table");
    }
    PlayerProfile p;
    if (!_store.getProfileById(playerId, p)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int rating = rs.mode == GAME_MODE_4 ? p.elo4p
                 : rs.mode == GAME_MODE_OFK ? p.eloOfk
                                            : p.elo2p;
    touchOnlineLocked(playerId);
    switch (_queue.enqueue(playerId, rating, rs)) {
    case EnqueueStatus::OK:
        _matchAssignments.erase(playerId);
        saveLobbyMetaLocked();
        return success();
    case EnqueueStatus::ALREADY_QUEUED:
        return fail(RoomCode::CONFLICT, "already in the matchmaking queue");
    case EnqueueStatus::QUEUE_FULL:
        return fail(RoomCode::LIMIT_REACHED, "matchmaking queue is full");
    case EnqueueStatus::INVALID:
        break;
    }
    return fail(RoomCode::INVALID_ARGUMENT, "invalid matchmaking request");
}

RoomResult GameRoomManager::dequeueMatchmaking(int64_t playerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!ensurePlayerLocked(playerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    if (!_queue.dequeue(playerId)) {
        return fail(RoomCode::INVALID_STATE, "not in the matchmaking queue");
    }
    saveLobbyMetaLocked();
    return success();
}

MatchmakingStatus GameRoomManager::matchmakingStatus(int64_t playerId) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    MatchmakingStatus st;
    QueueInfo qi;
    if (_queue.queueInfo(playerId, qi)) {
        st.queued = true;
        st.waitedMs = qi.waitedMs;
        st.window = qi.window;
        st.rating = qi.rating;
        st.ruleset = qi.ruleset;
    }
    std::map<int64_t, std::string>::const_iterator it =
        _matchAssignments.find(playerId);
    if (it != _matchAssignments.end() && findRoomLocked(it->second) != nullptr) {
        st.matchedRoomId = it->second;
    }
    return st;
}

void GameRoomManager::spawnMatchRoomLocked(const MatchGroup &g, int64_t now)
{
    GameTableConfig cfg;
    cfg.mode = g.ruleset.mode;
    cfg.bases = g.ruleset.bases;
    cfg.useSuperProngs = g.ruleset.useSuperProngs;
    cfg.useEdgelessBoard = g.ruleset.useEdgelessBoard;
    cfg.timeControl.enabled = g.ruleset.isTimed();
    cfg.timeControl.initialMs = g.ruleset.initialMs;
    cfg.timeControl.incrementMs = g.ruleset.incrementMs;
    cfg.rated = true;
    cfg.allowTakeback = false;

    int64_t hostId = g.players.empty() ? 0 : g.players[0].playerId;
    Room *r = createRoomLocked(cfg, hostId, now);
    for (size_t i = 0; i < g.players.size(); ++i) {
        int64_t pid = g.players[i].playerId;
        touchOnlineLocked(pid);
        seatPlayerLocked(*r, static_cast<int>(i) + 1, pid, now);
    }
    startGameLocked(*r, now);
    for (size_t i = 0; i < g.players.size(); ++i) {
        int64_t pid = g.players[i].playerId;
        _matchAssignments[pid] = r->id;
        LobbyEvent e;
        e.type = LobbyEventType::MATCH_FOUND;
        e.targetPlayerId = pid;
        e.playerId = pid;
        e.handle = handleOf(pid);
        e.roomId = r->id;
        pushLobbyEventLocked(e);
    }
    saveRoomLocked(*r);
}

TickReport GameRoomManager::tick()
{
    std::lock_guard<std::mutex> lock(_mutex);
    TickReport rep;
    int64_t now = nowMs();

    if (_rooms.size() < _limits.maxRooms) {
        std::vector<MatchGroup> groups = _queue.findMatches();
        for (size_t i = 0; i < groups.size(); ++i) {
            spawnMatchRoomLocked(groups[i], now);
            ++rep.matchesSpawned;
        }
    }

    std::vector<std::string> reap;
    for (std::map<std::string, std::unique_ptr<Room>>::iterator it =
             _rooms.begin();
         it != _rooms.end(); ++it) {
        Room &r = *it->second;
        RoomPhase before = r.phase;
        advanceRoomLocked(r, now);
        if (before == RoomPhase::IN_PROGRESS && r.phase == RoomPhase::FINISHED) {
            ++rep.gamesTimedOut;
        }
        if (r.phase == RoomPhase::FINISHED && !r.archived &&
            r.archiveAttempts < kMaxArchiveAttempts) {
            archiveLocked(r);
            ++rep.archiveRetries;
        }
        if (r.phase == RoomPhase::FINISHED &&
            now - r.endedAtMs >= _limits.finishedRetentionMs &&
            (r.archived || r.archiveAttempts >= kMaxArchiveAttempts)) {
            reap.push_back(r.id);
        } else if (r.phase == RoomPhase::WAITING_FOR_PLAYERS &&
                   now - r.lastActivityMs >= _limits.waitingTtlMs) {
            reap.push_back(r.id);
        }
    }
    for (size_t i = 0; i < reap.size(); ++i) {
        deleteRoomLocked(reap[i]);
        ++rep.roomsReaped;
    }

    for (std::map<int64_t, std::deque<int64_t>>::iterator it =
             _lobbyChatTimes.begin();
         it != _lobbyChatTimes.end();) {
        while (!it->second.empty() &&
               it->second.front() <= now - _limits.lobbyChatWindowMs) {
            it->second.pop_front();
        }
        it = it->second.empty() ? _lobbyChatTimes.erase(it) : std::next(it);
    }
    return rep;
}

/* ------------------------------------------------------------------ */
/* Deadlines                                                           */
/* ------------------------------------------------------------------ */

int64_t GameRoomManager::liveClockLocked(const Room &r, int seat,
                                         int64_t now) const
{
    if (!r.config.timeControl.enabled || seat < 1 || seat > r.seatCount) {
        return -1;
    }
    int64_t base = r.clockMs[seat];
    if (r.phase == RoomPhase::IN_PROGRESS && r.board != nullptr &&
        static_cast<int>(r.board->getGame()->getTurn()) == seat) {
        base -= r.turnCarryMs;
        if (!r.paused) {
            base -= std::max<int64_t>(0, now - r.turnStartedMs);
        }
    }
    return std::max<int64_t>(0, base);
}

void GameRoomManager::advanceRoomLocked(Room &r, int64_t now)
{
    bool proposalsExpired = false;
    for (size_t i = 0; i < r.proposals.size();) {
        if (now >= r.proposals[i].expiresAtMs) {
            proposalsExpired = true;
            RoomEvent e;
            e.type = RoomEventType::PROPOSAL_EXPIRED;
            e.playerId = r.proposals[i].proposerId;
            e.detail = toString(r.proposals[i].type);
            e.text = "timed out";
            r.proposals.erase(r.proposals.begin() + static_cast<long>(i));
            emitLocked(r, e, now);
        } else {
            ++i;
        }
    }
    if (proposalsExpired) {
        saveRoomLocked(r);
    }

    for (std::deque<RoomEvent>::iterator it = r.events.begin();
         it != r.events.end();) {
        if (it->type == RoomEventType::VOICE_SIGNAL &&
            now - it->atMs >= _limits.signalTtlMs) {
            it = r.events.erase(it);
        } else {
            ++it;
        }
    }

    if (r.phase == RoomPhase::IN_PROGRESS && !r.paused &&
        r.config.timeControl.enabled) {
        int seat = static_cast<int>(r.board->getGame()->getTurn());
        if (seat >= 1 && seat <= r.seatCount) {
            int64_t remaining = r.clockMs[seat] - r.turnCarryMs -
                                std::max<int64_t>(0, now - r.turnStartedMs);
            if (remaining <= 0) {
                r.clockMs[seat] = 0;
                forfeitSeatLocked(r, now, seat, "TIMEOUT");
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Moves                                                               */
/* ------------------------------------------------------------------ */

namespace {

int countPods(const OctiBoard &b, OctiPlayer p)
{
    int n = 0;
    for (int x = 0; x < b.getBoardWidth(); ++x) {
        for (int y = 0; y < b.getBoardHeight(); ++y) {
            const OctiSquare &sq = b.squareAt(x, y);
            for (int i = 0; i < sq.getNumPods(); ++i) {
                if (static_cast<int>(sq.getPodAt(i).getType()) ==
                    static_cast<int>(p)) {
                    ++n;
                }
            }
        }
    }
    return n;
}

/* Why did the rules end the game? Mirrors OctiGame::updateGameOverAndWinner. */
std::string classifyRuleTermination(const OctiBoard &b, const OctiGame &g)
{
    OctiPlayer w = g.getWinner();
    if (g.getMode() == GAME_MODE_2) {
        const int xs[3] = {2, 4, 6};
        int y = (w == PLAYER_A) ? 6 : 2;
        int captured = 0;
        for (int i = 0; i < 3; ++i) {
            if (b.squareAt(xs[i], y).isCapturedBy(w)) {
                ++captured;
            }
        }
        int needed = (g.getBases() == GAME_BASES_1) ? 1 : 3;
        return captured >= needed ? "BASE_CAPTURE" : "ELIMINATION";
    }
    if (g.getMode() == GAME_MODE_OFK) {
        int y = (w == PLAYER_A) ? 5 : 1;
        for (int x = 1; x <= 4; ++x) {
            if (b.squareAt(x, y).isCapturedBy(w)) {
                return "BASE_CAPTURE";
            }
        }
        OctiPlayer opp = (w == PLAYER_A) ? PLAYER_B : PLAYER_A;
        return countPods(b, opp) == 0 ? "ELIMINATION" : "STALEMATE";
    }
    bool bases;
    if (w == PLAYER_A) {
        bases = b.squareAt(2, 6).isCapturedByTeamOf(PLAYER_A) &&
                b.squareAt(6, 2).isCapturedByTeamOf(PLAYER_A);
    } else {
        bases = b.squareAt(2, 2).isCapturedByTeamOf(PLAYER_B) &&
                b.squareAt(6, 6).isCapturedByTeamOf(PLAYER_B);
    }
    return bases ? "BASE_CAPTURE" : "ELIMINATION";
}

} // namespace

RoomResult GameRoomManager::submitMove(const std::string &roomId,
                                       int64_t playerId,
                                       const std::string &moveText)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(playerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->banned.count(playerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    int seat = seatOfLocked(*r, playerId);
    if (seat == 0) {
        return fail(RoomCode::FORBIDDEN_NOT_SEATED, "not seated at this table");
    }
    if (r->phase != RoomPhase::IN_PROGRESS) {
        return fail(RoomCode::INVALID_STATE, "game is not in progress");
    }
    if (r->paused) {
        return fail(RoomCode::INVALID_STATE, "game is paused");
    }
    OctiGame *g = r->board->getGame();
    if (static_cast<int>(g->getTurn()) != seat) {
        return fail(RoomCode::NOT_YOUR_TURN, "it is not your turn");
    }
    std::string mv = trimmed(moveText);
    if (mv.size() > kMaxMoveBytes) {
        return fail(RoomCode::INVALID_ARGUMENT, "move text is too long");
    }
    if (mv.empty() || hasControlChars(mv)) {
        return fail(RoomCode::ILLEGAL_SYNTAX, "empty or malformed move");
    }

    touchOnlineLocked(playerId);
    const int64_t elapsed =
        r->turnCarryMs + std::max<int64_t>(0, now - r->turnStartedMs);
    const int64_t before = r->clockMs[seat];
    const int plyBefore = g->getNumMoves();

    try {
        g->move(static_cast<OctiPlayer>(seat), mv);
    } catch (const IllegalMoveSyntaxException &ex) {
        return fail(RoomCode::ILLEGAL_SYNTAX, ex.what());
    } catch (const IllegalMoveException &ex) {
        return fail(RoomCode::ILLEGAL_MOVE, ex.what());
    } catch (const std::exception &ex) {
        if (g->getNumMoves() == plyBefore &&
            static_cast<int>(g->getTurn()) == seat) {
            return fail(RoomCode::ILLEGAL_MOVE, ex.what());
        }
        return fail(RoomCode::INTERNAL_ERROR, "engine failure");
    }

    std::vector<std::pair<OctiPlayer, OctiMove>> hist;
    g->getMoves(hist);
    if (hist.size() != r->plies.size() + 1) {
        return fail(RoomCode::INTERNAL_ERROR, "engine history out of sync");
    }

    Room::Ply ply;
    ply.seat = seat;
    ply.move = hist.back().second.asString();
    ply.clockBeforeMs = before;
    ply.elapsedMs = elapsed;
    if (r->config.timeControl.enabled) {
        ply.clockAfterMs = before - elapsed + r->config.timeControl.incrementMs;
        r->clockMs[seat] = ply.clockAfterMs;
    }
    r->plies.push_back(ply);
    OctiPlyMetadata pmeta;
    pmeta.elapsedMs = elapsed;
    pmeta.clockAfterMs = r->config.timeControl.enabled ? ply.clockAfterMs : -1;
    g->setLastPlyMetadata(pmeta);
    r->lastActivityMs = now;

    for (size_t i = 0; i < r->proposals.size();) {
        ProposalType t = r->proposals[i].type;
        if (t == ProposalType::DRAW_OFFER || t == ProposalType::UNDO_REQUEST) {
            RoomEvent pe;
            pe.type = RoomEventType::PROPOSAL_EXPIRED;
            pe.playerId = r->proposals[i].proposerId;
            pe.detail = toString(t);
            pe.text = "superseded by move";
            r->proposals.erase(r->proposals.begin() + static_cast<long>(i));
            emitLocked(*r, pe, now);
        } else {
            ++i;
        }
    }

    RoomEvent e;
    e.type = RoomEventType::MOVE;
    e.playerId = playerId;
    e.seat = seat;
    e.ply = static_cast<int>(r->plies.size());
    e.text = ply.move;
    e.clockMs = r->config.timeControl.enabled ? ply.clockAfterMs : -1;
    e.elapsedMs = elapsed;
    RoomResult res = success();
    res.seq = emitLocked(*r, e, now);
    res.roomId = r->id;
    res.seat = seat;
    res.move = ply.move;
    res.clockMs = e.clockMs;
    res.elapsedMs = elapsed;

    if (g->isGameOver()) {
        std::string term = classifyRuleTermination(*r->board, *g);
        finishGameLocked(*r, now, term, static_cast<int>(g->getWinner()));
        res.gameOver = true;
    } else {
        r->turnStartedMs = now;
        r->turnCarryMs = 0;
    }
    saveRoomLocked(*r);
    return res;
}

void GameRoomManager::forfeitSeatLocked(Room &r, int64_t now, int loserSeat,
                                        const std::string &termination)
{
    OctiGame *g = r.board->getGame();
    if (!g->isGameOver()) {
        g->resign(static_cast<OctiPlayer>(loserSeat));
    }
    int winner;
    if (r.config.mode == GAME_MODE_4) {
        winner = (loserSeat % 2 == 1) ? 2 : 1;
    } else {
        winner = 3 - loserSeat;
    }
    finishGameLocked(r, now, termination, winner);
}

RoomResult GameRoomManager::resign(const std::string &roomId, int64_t playerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(playerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->banned.count(playerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    int seat = seatOfLocked(*r, playerId);
    if (seat == 0) {
        return fail(RoomCode::FORBIDDEN_NOT_SEATED, "not seated at this table");
    }
    if (r->phase != RoomPhase::IN_PROGRESS) {
        return fail(RoomCode::INVALID_STATE, "game is not in progress");
    }
    if (r->board->getGame()->isPlayerEliminated(static_cast<OctiPlayer>(seat))) {
        return fail(RoomCode::INVALID_STATE, "already eliminated");
    }
    forfeitSeatLocked(*r, now, seat, "RESIGNATION");
    saveRoomLocked(*r);
    RoomResult res = success();
    res.roomId = r->id;
    res.seat = seat;
    res.gameOver = true;
    return res;
}

/* ------------------------------------------------------------------ */
/* Connection health                                                   */
/* ------------------------------------------------------------------ */

RoomResult GameRoomManager::markDisconnected(const std::string &roomId,
                                             int64_t playerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(playerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    int seat = seatOfLocked(*r, playerId);
    if (seat == 0) {
        return fail(RoomCode::FORBIDDEN_NOT_SEATED, "not seated at this table");
    }
    if (r->phase == RoomPhase::FINISHED || !r->seatConnected[seat]) {
        return success();
    }
    r->seatConnected[seat] = false;
    r->seatDisconnectedAt[seat] = now;
    RoomEvent e;
    e.type = RoomEventType::PLAYER_DISCONNECTED;
    e.playerId = playerId;
    e.seat = seat;
    RoomResult res = success();
    res.seq = emitLocked(*r, e, now);
    saveRoomLocked(*r);
    return res;
}

RoomResult GameRoomManager::markReconnected(const std::string &roomId,
                                            int64_t playerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(playerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    int seat = seatOfLocked(*r, playerId);
    if (seat == 0) {
        return fail(RoomCode::FORBIDDEN_NOT_SEATED, "not seated at this table");
    }
    if (r->seatConnected[seat]) {
        return success();
    }
    r->seatConnected[seat] = true;
    r->seatDisconnectedAt[seat] = 0;
    RoomEvent e;
    e.type = RoomEventType::PLAYER_RECONNECTED;
    e.playerId = playerId;
    e.seat = seat;
    RoomResult res = success();
    res.seq = emitLocked(*r, e, now);
    saveRoomLocked(*r);
    return res;
}

RoomResult GameRoomManager::claimDisconnectForfeit(const std::string &roomId,
                                                   int64_t claimantPlayerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(claimantPlayerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->banned.count(claimantPlayerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    int cs = seatOfLocked(*r, claimantPlayerId);
    if (cs == 0) {
        return fail(RoomCode::FORBIDDEN_NOT_SEATED, "not seated at this table");
    }
    if (r->phase != RoomPhase::IN_PROGRESS) {
        return fail(RoomCode::INVALID_STATE, "game is not in progress");
    }
    if (r->paused) {
        return fail(RoomCode::INVALID_STATE, "game is paused");
    }
    OctiGame *g = r->board->getGame();
    if (g->isPlayerEliminated(static_cast<OctiPlayer>(cs))) {
        return fail(RoomCode::INVALID_STATE, "already eliminated");
    }
    int victim = 0;
    int64_t oldest = 0;
    for (int s = 1; s <= r->seatCount; ++s) {
        if (teamOf(r->config.mode, s) == teamOf(r->config.mode, cs) ||
            r->seatConnected[s] ||
            g->isPlayerEliminated(static_cast<OctiPlayer>(s))) {
            continue;
        }
        if (now - r->seatDisconnectedAt[s] >=
            r->config.timeControl.disconnectGraceMs) {
            if (victim == 0 || r->seatDisconnectedAt[s] < oldest) {
                victim = s;
                oldest = r->seatDisconnectedAt[s];
            }
        }
    }
    if (victim == 0) {
        return fail(RoomCode::INVALID_STATE,
                    "no opposing seat is past the reconnect grace window");
    }
    forfeitSeatLocked(*r, now, victim, "DISCONNECT_FORFEIT");
    saveRoomLocked(*r);
    RoomResult res = success();
    res.roomId = r->id;
    res.seat = victim;
    res.gameOver = true;
    return res;
}

/* ------------------------------------------------------------------ */
/* Mutual proposals                                                    */
/* ------------------------------------------------------------------ */

std::vector<int> GameRoomManager::approversLocked(const Room &r,
                                                  int proposerSeat) const
{
    std::vector<int> seats;
    for (int s = 1; s <= r.seatCount; ++s) {
        if (s == proposerSeat || r.seatPlayer[s] == 0) {
            continue;
        }
        if (r.phase == RoomPhase::IN_PROGRESS &&
            r.board->getGame()->isPlayerEliminated(static_cast<OctiPlayer>(s))) {
            continue;
        }
        seats.push_back(s);
    }
    return seats;
}

RoomResult GameRoomManager::submitProposal(const std::string &roomId,
                                           int64_t playerId, ProposalType type)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(playerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->banned.count(playerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    int seat = seatOfLocked(*r, playerId);
    if (seat == 0) {
        return fail(RoomCode::FORBIDDEN_NOT_SEATED, "not seated at this table");
    }
    switch (type) {
    case ProposalType::DRAW_OFFER:
    case ProposalType::PAUSE_REQUEST:
    case ProposalType::UNDO_REQUEST:
        if (r->phase != RoomPhase::IN_PROGRESS) {
            return fail(RoomCode::INVALID_STATE, "game is not in progress");
        }
        if (r->board->getGame()->isPlayerEliminated(static_cast<OctiPlayer>(seat))) {
            return fail(RoomCode::INVALID_STATE, "already eliminated");
        }
        if (type == ProposalType::PAUSE_REQUEST && r->paused) {
            return fail(RoomCode::INVALID_STATE, "game is already paused");
        }
        if (type == ProposalType::UNDO_REQUEST) {
            if (!r->config.allowTakeback || r->config.rated) {
                return fail(RoomCode::FORBIDDEN_POLICY,
                            "takeback is not allowed at this table");
            }
            if (r->plies.empty()) {
                return fail(RoomCode::INVALID_STATE, "nothing to take back");
            }
        }
        break;
    case ProposalType::REMATCH_REQUEST:
        if (r->phase != RoomPhase::FINISHED) {
            return fail(RoomCode::INVALID_STATE, "game is not finished");
        }
        if (!r->rematchRoomId.empty()) {
            return fail(RoomCode::INVALID_STATE, "rematch already created");
        }
        break;
    }
    for (size_t i = 0; i < r->proposals.size(); ++i) {
        if (r->proposals[i].type == type) {
            return fail(RoomCode::CONFLICT, "proposal already pending");
        }
    }
    touchOnlineLocked(playerId);

    Room::Proposal p;
    p.type = type;
    p.proposerId = playerId;
    p.accepted.insert(playerId);
    p.expiresAtMs = now + _limits.proposalTtlMs;
    r->proposals.push_back(p);

    RoomEvent e;
    e.type = RoomEventType::PROPOSAL_OFFERED;
    e.playerId = playerId;
    e.seat = seat;
    e.detail = toString(type);
    int64_t seq = emitLocked(*r, e, now);

    if (approversLocked(*r, seat).empty()) {
        r->proposals.pop_back();
        RoomResult done = completeProposalLocked(*r, type, playerId, now);
        if (done.seq == 0) {
            done.seq = seq;
        }
        saveRoomLocked(*r);
        return done;
    }
    saveRoomLocked(*r);
    RoomResult res = success();
    res.seq = seq;
    res.roomId = r->id;
    return res;
}

RoomResult GameRoomManager::respondProposal(const std::string &roomId,
                                            int64_t playerId,
                                            ProposalType type, bool accept)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(playerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->banned.count(playerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    int seat = seatOfLocked(*r, playerId);
    if (seat == 0) {
        return fail(RoomCode::FORBIDDEN_NOT_SEATED, "not seated at this table");
    }
    size_t idx = r->proposals.size();
    for (size_t i = 0; i < r->proposals.size(); ++i) {
        if (r->proposals[i].type == type) {
            idx = i;
            break;
        }
    }
    if (idx == r->proposals.size()) {
        return fail(RoomCode::INVALID_STATE, "no such pending proposal");
    }
    Room::Proposal prop = r->proposals[idx];
    const bool isProposer = prop.proposerId == playerId;
    if (isProposer && accept) {
        return fail(RoomCode::INVALID_ARGUMENT,
                    "the proposer cannot accept their own proposal");
    }
    int proposerSeat = seatOfLocked(*r, prop.proposerId);
    std::vector<int> approvers = approversLocked(*r, proposerSeat);
    if (!isProposer &&
        std::find(approvers.begin(), approvers.end(), seat) == approvers.end()) {
        return fail(RoomCode::FORBIDDEN_NOT_SEATED,
                    "not an active seat that can answer this proposal");
    }
    touchOnlineLocked(playerId);

    if (!accept) {
        r->proposals.erase(r->proposals.begin() + static_cast<long>(idx));
        RoomEvent e;
        e.type = RoomEventType::PROPOSAL_DECLINED;
        e.playerId = playerId;
        e.seat = seat;
        e.detail = toString(type);
        e.text = isProposer ? "withdrawn" : "declined";
        RoomResult res = success();
        res.seq = emitLocked(*r, e, now);
        saveRoomLocked(*r);
        return res;
    }

    r->proposals[idx].accepted.insert(playerId);
    bool all = true;
    for (size_t i = 0; i < approvers.size(); ++i) {
        if (r->proposals[idx].accepted.count(r->seatPlayer[approvers[i]]) == 0) {
            all = false;
        }
    }
    if (!all) {
        RoomEvent e;
        e.type = RoomEventType::PROPOSAL_ACCEPTED;
        e.playerId = playerId;
        e.seat = seat;
        e.detail = toString(type);
        RoomResult res = success();
        res.seq = emitLocked(*r, e, now);
        saveRoomLocked(*r);
        return res;
    }
    int64_t proposer = prop.proposerId;
    r->proposals.erase(r->proposals.begin() + static_cast<long>(idx));
    RoomResult cRes = completeProposalLocked(*r, type, proposer, now);
    saveRoomLocked(*r);
    return cRes;
}

RoomResult GameRoomManager::completeProposalLocked(Room &r, ProposalType type,
                                                   int64_t proposerId,
                                                   int64_t now)
{
    RoomResult res = success();
    res.roomId = r.id;
    switch (type) {
    case ProposalType::DRAW_OFFER:
        if (r.phase != RoomPhase::IN_PROGRESS) {
            return fail(RoomCode::INVALID_STATE, "game is not in progress");
        }
        finishGameLocked(r, now, "AGREED_DRAW", 0);
        res.gameOver = true;
        return res;
    case ProposalType::PAUSE_REQUEST: {
        if (r.phase != RoomPhase::IN_PROGRESS || r.paused) {
            return fail(RoomCode::INVALID_STATE, "cannot pause now");
        }
        /* Freeze: charge the running turn so far; resume restarts it. */
        r.turnCarryMs += std::max<int64_t>(0, now - r.turnStartedMs);
        r.paused = true;
        RoomEvent e;
        e.type = RoomEventType::GAME_PAUSED;
        e.playerId = proposerId;
        res.seq = emitLocked(r, e, now);
        saveRoomLocked(r);
        return res;
    }
    case ProposalType::UNDO_REQUEST: {
        if (r.phase != RoomPhase::IN_PROGRESS || !r.config.allowTakeback ||
            r.config.rated || r.plies.empty()) {
            return fail(RoomCode::INVALID_STATE, "cannot take back now");
        }
        OctiGame *g = r.board->getGame();
        if (!g->canUndo()) {
            return fail(RoomCode::INVALID_STATE, "engine has nothing to undo");
        }
        g->undo();
        Room::Ply p = r.plies.back();
        r.plies.pop_back();
        if (r.config.timeControl.enabled) {
            r.clockMs[p.seat] = p.clockBeforeMs;
        }
        r.turnStartedMs = now;
        r.turnCarryMs = 0;
        if (g->getNumMoves() != static_cast<int>(r.plies.size())) {
            return fail(RoomCode::INTERNAL_ERROR, "engine history out of sync");
        }
        RoomEvent e;
        e.type = RoomEventType::UNDO_APPLIED;
        e.playerId = proposerId;
        e.ply = static_cast<int>(r.plies.size());
        e.text = p.move;
        res.seq = emitLocked(r, e, now);
        saveRoomLocked(r);
        return res;
    }
    case ProposalType::REMATCH_REQUEST:
        return spawnRematchLocked(r, now);
    }
    return fail(RoomCode::INVALID_ARGUMENT, "unknown proposal type");
}

RoomResult GameRoomManager::resumeGame(const std::string &roomId,
                                       int64_t playerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(playerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->banned.count(playerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    if (seatOfLocked(*r, playerId) == 0) {
        return fail(RoomCode::FORBIDDEN_NOT_SEATED, "not seated at this table");
    }
    if (r->phase != RoomPhase::IN_PROGRESS || !r->paused) {
        return fail(RoomCode::INVALID_STATE, "game is not paused");
    }
    r->paused = false;
    r->turnStartedMs = now;
    RoomEvent e;
    e.type = RoomEventType::GAME_RESUMED;
    e.playerId = playerId;
    RoomResult res = success();
    res.seq = emitLocked(*r, e, now);
    res.roomId = r->id;
    saveRoomLocked(*r);
    return res;
}

RoomResult GameRoomManager::spawnRematchLocked(Room &old, int64_t now)
{
    if (old.phase != RoomPhase::FINISHED || !old.rematchRoomId.empty()) {
        return fail(RoomCode::INVALID_STATE, "rematch is not available");
    }
    for (int s = 1; s <= old.seatCount; ++s) {
        if (old.seatPlayer[s] == 0) {
            return fail(RoomCode::INVALID_STATE, "a seat is empty");
        }
        if (_seatRoom.count(old.seatPlayer[s]) != 0) {
            return fail(RoomCode::INVALID_STATE,
                        "a player is already seated at another table");
        }
    }
    if (_rooms.size() >= _limits.maxRooms) {
        return fail(RoomCode::LIMIT_REACHED, "too many rooms");
    }
    GameTableConfig cfg = old.config;
    int64_t hostId = old.hostId;
    int64_t seatPlayers[5] = {0, 0, 0, 0, 0};
    for (int s = 1; s <= old.seatCount; ++s) {
        seatPlayers[s] = old.seatPlayer[s];
    }
    std::set<int64_t> bans = old.banned;
    std::map<int64_t, Room::Mute> mutes = old.muted;
    const int seatCount = old.seatCount;
    const OctiGameMode mode = old.config.mode;
    const std::string oldId = old.id;

    Room *n = createRoomLocked(cfg, hostId, now);
    n->banned = bans;
    n->muted = mutes;
    for (int s = 1; s <= seatCount; ++s) {
        int target = (mode == GAME_MODE_4) ? ((s % 2 == 1) ? s + 1 : s - 1)
                                           : 3 - s;
        touchOnlineLocked(seatPlayers[s]);
        seatPlayerLocked(*n, target, seatPlayers[s], now);
    }
    startGameLocked(*n, now);

    Room *o = findRoomLocked(oldId);
    RoomResult res = success();
    res.roomId = n->id;
    if (o != nullptr) {
        o->rematchRoomId = n->id;
        RoomEvent e;
        e.type = RoomEventType::REMATCH_CREATED;
        e.playerId = hostId;
        e.text = n->id;
        res.seq = emitLocked(*o, e, now);
        saveRoomLocked(*o);
    }
    saveRoomLocked(*n);
    return res;
}

/* ------------------------------------------------------------------ */
/* Game end, archive, transcript                                       */
/* ------------------------------------------------------------------ */

void GameRoomManager::finishGameLocked(Room &r, int64_t now,
                                       const std::string &termination,
                                       int winnerSeat)
{
    r.phase = RoomPhase::FINISHED;
    r.paused = false;
    r.proposals.clear();
    r.termination = termination;
    r.winnerSeat = winnerSeat;
    if (winnerSeat == 0) {
        r.result = "DRAW";
    } else if (r.config.mode == GAME_MODE_4) {
        r.result = (winnerSeat == 1) ? "TEAM_13_WIN" : "TEAM_24_WIN";
    } else {
        r.result = (winnerSeat == 1) ? "P1_WIN" : "P2_WIN";
    }
    r.endedAtMs = now;
    r.endedAtSec = std::max(wallSec(), r.startedAtSec);
    r.lastActivityMs = now;

    for (int s = 1; s <= r.seatCount; ++s) {
        std::map<int64_t, std::string>::iterator sr =
            _seatRoom.find(r.seatPlayer[s]);
        if (sr != _seatRoom.end() && sr->second == r.id) {
            _seatRoom.erase(sr);
        }
    }

    RoomEvent e;
    e.type = RoomEventType::GAME_OVER;
    e.seat = winnerSeat;
    e.detail = termination;
    e.text = r.result;
    emitLocked(r, e, now);

    archiveLocked(r);

    for (int s = 1; s <= r.seatCount; ++s) {
        refreshPresenceLocked(r.seatPlayer[s]);
    }
    announceTableLocked(r, false);
    saveRoomLocked(r);
}

bool GameRoomManager::archiveLocked(Room &r)
{
    ++r.archiveAttempts;

    GameRecord rec;
    rec.gameId = r.id;
    rec.gameMode = modeName(r.config.mode);
    rec.gameBases = static_cast<int>(r.board->getGame()->getBases());
    rec.superProngs = r.board->getGame()->useSuperProngs();
    rec.edgeless = r.board->getGame()->useEdgelessBoard();
    rec.timeControl = timeControlString(r.config.timeControl);
    rec.p1PlayerId = r.seatPlayer[1];
    rec.p2PlayerId = r.seatPlayer[2];
    rec.p1Handle = r.seatHandle[1];
    rec.p2Handle = r.seatHandle[2];
    if (r.seatCount == 4) {
        rec.p3PlayerId = r.seatPlayer[3];
        rec.p4PlayerId = r.seatPlayer[4];
        rec.p3Handle = r.seatHandle[3];
        rec.p4Handle = r.seatHandle[4];
    }
    rec.result = r.result;
    rec.winnerSeat = r.winnerSeat;
    rec.termination = r.termination;
    rec.plyCount = static_cast<int>(r.plies.size());
    rec.octiTranscript = buildTranscriptLocked(r);
    rec.finalFen = r.board->toPositionString();
    rec.startedAt = r.startedAtSec;
    rec.endedAt = std::max(r.endedAtSec, r.startedAtSec);
    rec.source = "online";
    rec.uploadedAt = 0;
    if (r.winnerSeat == 1) {
        rec.winner = "A";
    } else if (r.winnerSeat == 2) {
        rec.winner = "B";
    } else if (r.winnerSeat == 3) {
        rec.winner = "C";
    } else if (r.winnerSeat == 4) {
        rec.winner = "D";
    } else {
        rec.winner = "NONE";
    }
    rec.status = (r.board->getGame() && r.board->getGame()->isGameResigned()) ? "RESIGNED" : "GAME_OVER";
    rec.aiDifficulty = 0;
    rec.appVersion = "";
    rec.platform = "";
    rec.installId = "";

    std::vector<RatingUpdate> updates;
    if (r.config.rated) {
        PlayerProfile prof[5];
        for (int s = 1; s <= r.seatCount; ++s) {
            if (!_store.getProfileById(r.seatPlayer[s], prof[s])) {
                r.archiveError = "player profile lookup failed";
                saveRoomLocked(r);
                return false;
            }
        }
        auto ratingOf = [&r](const PlayerProfile &p) -> int {
            if (r.config.mode == GAME_MODE_4) {
                return p.elo4p;
            }
            return r.config.mode == GAME_MODE_OFK ? p.eloOfk : p.elo2p;
        };
        if (r.seatCount == 4) {
            std::array<int, 4> rt = {{ratingOf(prof[1]), ratingOf(prof[2]),
                                      ratingOf(prof[3]), ratingOf(prof[4])}};
            EloOutcome oc = r.winnerSeat == 0   ? EloOutcome::DRAW
                            : r.winnerSeat == 1 ? EloOutcome::FIRST_WINS
                                                : EloOutcome::SECOND_WINS;
            EloCalculator::TeamUpdate up = EloCalculator::updateTeams(rt, oc);
            for (int i = 0; i < 4; ++i) {
                int team = (i % 2 == 0) ? 1 : 2;
                RatingUpdate ru;
                ru.playerId = r.seatPlayer[i + 1];
                ru.newElo = up.newRating[static_cast<size_t>(i)];
                ru.outcome = r.winnerSeat == 0      ? GameOutcome::DRAW
                             : r.winnerSeat == team ? GameOutcome::WIN
                                                    : GameOutcome::LOSS;
                updates.push_back(ru);
            }
        } else {
            EloOutcome oc = r.winnerSeat == 0   ? EloOutcome::DRAW
                            : r.winnerSeat == 1 ? EloOutcome::FIRST_WINS
                                                : EloOutcome::SECOND_WINS;
            EloCalculator::DuelUpdate up = EloCalculator::updateDuel(
                ratingOf(prof[1]), ratingOf(prof[2]), oc);
            for (int s = 1; s <= 2; ++s) {
                RatingUpdate ru;
                ru.playerId = r.seatPlayer[s];
                ru.newElo = s == 1 ? up.newRating1 : up.newRating2;
                ru.outcome = r.winnerSeat == 0   ? GameOutcome::DRAW
                             : r.winnerSeat == s ? GameOutcome::WIN
                                                 : GameOutcome::LOSS;
                updates.push_back(ru);
            }
        }
    }

    std::string err;
    ArchiveStatus st = _store.archiveGameWithRatings(rec, updates, err);
    if (st == ArchiveStatus::OK) {
        r.archived = true;
        r.archiveError.clear();
        saveRoomLocked(r);
        return true;
    }
    r.archiveError = err.empty() ? "archive failed" : err;
    saveRoomLocked(r);
    return false;
}

std::string GameRoomManager::buildTranscriptLocked(const Room &r) const
{
    if (!r.board) {
        return "";
    }
    OctiGame *g = r.board->getGame();
    if (!g) {
        return "";
    }

    // Populate per-ply metadata on the game from r.plies
    for (size_t i = 0; i < r.plies.size(); ++i) {
        const Room::Ply &p = r.plies[i];
        OctiPlyMetadata pmd;
        pmd.elapsedMs = p.elapsedMs;
        pmd.clockAfterMs = r.config.timeControl.enabled ? p.clockAfterMs : -1;
        g->setPlyMetadata(i, pmd);
    }

    OctiGameMetadata meta;
    meta.gameId = r.id;
    meta.source = "ONLINE";
    meta.timeControl = r.config.timeControl.enabled ? timeControlString(r.config.timeControl) : "untimed";

    if (!g->isGameStarted()) {
        meta.status = "NOT_STARTED";
    } else if (g->isGameResigned()) {
        meta.status = "RESIGNED";
    } else if (r.phase == RoomPhase::FINISHED) {
        meta.status = "GAME_OVER";
    } else {
        meta.status = "IN_PROGRESS";
    }

    if (r.phase == RoomPhase::FINISHED) {
        std::string reason = r.termination;
        if (reason == "BASE_CAPTURE") {
            reason = "BASE_OCCUPATION";
        } else if (reason == "ELIMINATION") {
            reason = "TOTAL_CAPTURE";
        }
        meta.termination = reason;
    } else if (g->isGameResigned()) {
        meta.termination = "RESIGNATION";
    }
    meta.license = "CC-BY-4.0";
    meta.attribution = "Octi game data, Charles Chiou (selfso.com)";

    for (int s = 1; s <= r.seatCount; ++s) {
        OctiPlayerMetadata pmd;
        pmd.seat = s;
        pmd.player = static_cast<OctiPlayer>(PLAYER_A + s - 1);
        pmd.type = "HUMAN";
        pmd.handle = r.seatPlayer[s] != 0 ? cleanHeaderValue(r.seatHandle[s]) : ("Seat " + std::to_string(s));
        meta.players.push_back(pmd);
    }

    return r.board->serializeJson(meta);
}

/* ------------------------------------------------------------------ */
/* Board chat                                                          */
/* ------------------------------------------------------------------ */

RoomResult GameRoomManager::postBoardChat(const std::string &roomId,
                                          int64_t senderPlayerId,
                                          ChatScope scope,
                                          const std::string &text)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(senderPlayerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->banned.count(senderPlayerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    int seat = seatOfLocked(*r, senderPlayerId);
    if (seat == 0 && r->observers.count(senderPlayerId) == 0) {
        return fail(RoomCode::FORBIDDEN_NOT_PARTICIPANT,
                    "neither seated nor observing this room");
    }
    if (seat == 0 && r->config.observerPolicy != ObserverPolicy::OPEN) {
        return fail(RoomCode::FORBIDDEN_POLICY,
                    "observers cannot chat at this table");
    }
    std::map<int64_t, Room::Mute>::const_iterator mu = r->muted.find(senderPlayerId);
    if (mu != r->muted.end() && mu->second.chat) {
        return fail(RoomCode::FORBIDDEN_MUTED, "muted in this room");
    }
    if (scope == ChatScope::TEAM) {
        if (r->config.mode != GAME_MODE_4) {
            return fail(RoomCode::INVALID_ARGUMENT,
                        "team chat only exists in 4-player games");
        }
        if (seat == 0) {
            return fail(RoomCode::FORBIDDEN_NOT_SEATED,
                        "only seated players can use team chat");
        }
    }
    std::string clean;
    std::string err;
    if (!cleanText(text, kMaxChatBytes, clean, err)) {
        return fail(RoomCode::INVALID_ARGUMENT, err);
    }
    touchOnlineLocked(senderPlayerId);
    if (!rateAllow(r->chatTimes[senderPlayerId], now, _limits.boardChatBurst,
                   _limits.boardChatWindowMs)) {
        return fail(RoomCode::RATE_LIMITED, "board chat rate limit exceeded");
    }
    RoomEvent e;
    e.type = RoomEventType::CHAT;
    e.playerId = senderPlayerId;
    e.seat = seat;
    e.scope = scope;
    e.text = clean;
    RoomResult res = success();
    res.seq = emitLocked(*r, e, now);
    res.roomId = r->id;
    saveRoomLocked(*r);
    return res;
}

/* ------------------------------------------------------------------ */
/* P2P voice signaling and meta                                        */
/* ------------------------------------------------------------------ */

RoomResult GameRoomManager::submitVoiceSignal(const std::string &roomId,
                                              int64_t senderPlayerId,
                                              int64_t targetPlayerId,
                                              VoiceSignalType type,
                                              const std::string &payloadJson)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(senderPlayerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->banned.count(senderPlayerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    if (!isParticipantLocked(*r, senderPlayerId)) {
        return fail(RoomCode::FORBIDDEN_NOT_PARTICIPANT,
                    "neither seated nor observing this room");
    }
    if (r->config.voicePolicy == VoicePolicy::DISABLED) {
        return fail(RoomCode::FORBIDDEN_POLICY, "voice is disabled at this table");
    }
    if (!voiceAllowedLocked(*r, senderPlayerId)) {
        return fail(RoomCode::FORBIDDEN_POLICY,
                    "voice policy does not allow you to use voice");
    }
    std::map<int64_t, Room::Mute>::const_iterator mu = r->muted.find(senderPlayerId);
    if (mu != r->muted.end() && mu->second.voice) {
        return fail(RoomCode::FORBIDDEN_MUTED, "voice muted in this room");
    }
    if (targetPlayerId == senderPlayerId || targetPlayerId <= 0 ||
        !isParticipantLocked(*r, targetPlayerId) ||
        !voiceAllowedLocked(*r, targetPlayerId)) {
        return fail(RoomCode::INVALID_ARGUMENT,
                    "target is not a voice participant of this room");
    }
    if (payloadJson.size() > kMaxVoicePayloadBytes) {
        return fail(RoomCode::PAYLOAD_TOO_LARGE, "signal payload exceeds 4 KiB");
    }
    if (payloadJson.empty() || !nlohmann::json::accept(payloadJson)) {
        return fail(RoomCode::INVALID_ARGUMENT, "signal payload is not valid JSON");
    }
    touchOnlineLocked(senderPlayerId);
    RoomEvent e;
    e.type = RoomEventType::VOICE_SIGNAL;
    e.playerId = senderPlayerId;
    e.targetPlayerId = targetPlayerId;
    e.seat = seatOfLocked(*r, senderPlayerId);
    e.detail = toString(type);
    e.text = payloadJson;
    RoomResult res = success();
    res.seq = emitLocked(*r, e, now);
    res.roomId = r->id;
    return res;
}

RoomResult GameRoomManager::submitVoiceMeta(const std::string &roomId,
                                            int64_t senderPlayerId,
                                            VoiceMetaAction action)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(senderPlayerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->banned.count(senderPlayerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    if (!isParticipantLocked(*r, senderPlayerId)) {
        return fail(RoomCode::FORBIDDEN_NOT_PARTICIPANT,
                    "neither seated nor observing this room");
    }
    if (r->config.voicePolicy == VoicePolicy::DISABLED) {
        return fail(RoomCode::FORBIDDEN_POLICY, "voice is disabled at this table");
    }
    if (!voiceAllowedLocked(*r, senderPlayerId)) {
        return fail(RoomCode::FORBIDDEN_POLICY,
                    "voice policy does not allow you to use voice");
    }
    std::map<int64_t, Room::Mute>::const_iterator mu = r->muted.find(senderPlayerId);
    if (mu != r->muted.end() && mu->second.voice) {
        return fail(RoomCode::FORBIDDEN_MUTED, "voice muted in this room");
    }
    std::map<int64_t, bool>::iterator vp = r->voicePeers.find(senderPlayerId);
    switch (action) {
    case VoiceMetaAction::JOINED_VOICE:
        r->voicePeers[senderPlayerId] = false;
        break;
    case VoiceMetaAction::LEFT_VOICE:
        if (vp == r->voicePeers.end()) {
            return fail(RoomCode::INVALID_STATE, "not in voice");
        }
        r->voicePeers.erase(vp);
        break;
    case VoiceMetaAction::MIC_MUTED:
    case VoiceMetaAction::MIC_UNMUTED:
        if (vp == r->voicePeers.end()) {
            return fail(RoomCode::INVALID_STATE, "not in voice");
        }
        vp->second = (action == VoiceMetaAction::MIC_MUTED);
        break;
    }
    touchOnlineLocked(senderPlayerId);
    RoomEvent e;
    e.type = RoomEventType::VOICE_META;
    e.playerId = senderPlayerId;
    e.seat = seatOfLocked(*r, senderPlayerId);
    e.detail = toString(action);
    RoomResult res = success();
    res.seq = emitLocked(*r, e, now);
    res.roomId = r->id;
    saveRoomLocked(*r);
    return res;
}

/* ------------------------------------------------------------------ */
/* Moderation                                                          */
/* ------------------------------------------------------------------ */

RoomResult GameRoomManager::banUser(const std::string &roomId,
                                    int64_t actorPlayerId,
                                    int64_t targetPlayerId,
                                    const std::string &reason)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(actorPlayerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->banned.count(actorPlayerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    if (seatOfLocked(*r, actorPlayerId) == 0) {
        return fail(RoomCode::FORBIDDEN_NOT_SEATED,
                    "only seated players can ban");
    }
    if (targetPlayerId == actorPlayerId) {
        return fail(RoomCode::INVALID_ARGUMENT, "cannot ban yourself");
    }
    if (!ensurePlayerLocked(targetPlayerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown target player");
    }
    std::string cleanReason;
    if (!trimmed(reason).empty()) {
        std::string err;
        if (!cleanText(reason, kMaxReasonBytes, cleanReason, err)) {
            return fail(RoomCode::INVALID_ARGUMENT, "invalid reason: " + err);
        }
    }
    int ts = seatOfLocked(*r, targetPlayerId);
    if (ts != 0) {
        /* Only the host of a not-yet-started table may evict a seat. */
        if (!(r->phase == RoomPhase::WAITING_FOR_PLAYERS &&
              r->hostId == actorPlayerId)) {
            return fail(RoomCode::FORBIDDEN_SEATED_TARGET,
                        "a seated player cannot be banned once the table "
                        "is full or by a non-host");
        }
    }
    RoomResult res = success();
    res.roomId = r->id;
    if (r->banned.count(targetPlayerId) != 0) {
        res.message = "already banned";
        return res;
    }
    touchOnlineLocked(actorPlayerId);

    bool kickedSeat = ts != 0;
    r->banned.insert(targetPlayerId);
    if (r->observers.count(targetPlayerId) != 0) {
        removeObserverLocked(*r, targetPlayerId);
    }
    r->voicePeers.erase(targetPlayerId);
    purgeUserEventsLocked(*r, targetPlayerId);
    r->chatTimes.erase(targetPlayerId);

    RoomEvent e;
    e.type = RoomEventType::OBSERVER_BANNED;
    e.playerId = actorPlayerId;
    e.targetPlayerId = targetPlayerId;
    e.seat = ts;
    e.text = cleanReason;
    res.seq = emitLocked(*r, e, now);
    if (kickedSeat) {
        unseatLocked(*r, ts);
        r->lastActivityMs = now;
        announceTableLocked(*r, false);
    }
    refreshPresenceLocked(targetPlayerId);
    saveRoomLocked(*r);
    return res;
}

RoomResult GameRoomManager::muteUser(const std::string &roomId,
                                     int64_t actorPlayerId,
                                     int64_t targetPlayerId, bool muteChat,
                                     bool muteVoice)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(actorPlayerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->banned.count(actorPlayerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    if (seatOfLocked(*r, actorPlayerId) == 0) {
        return fail(RoomCode::FORBIDDEN_NOT_SEATED,
                    "only seated players can mute");
    }
    if (targetPlayerId == actorPlayerId) {
        return fail(RoomCode::INVALID_ARGUMENT, "cannot mute yourself");
    }
    if (!ensurePlayerLocked(targetPlayerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown target player");
    }
    /* Mutes are room-wide, so a seated player cannot be silenced. */
    if ((muteChat || muteVoice) && seatOfLocked(*r, targetPlayerId) != 0) {
        return fail(RoomCode::FORBIDDEN_SEATED_TARGET,
                    "a seated player cannot be muted");
    }
    touchOnlineLocked(actorPlayerId);
    Room::Mute &m = r->muted[targetPlayerId];
    bool changed = m.chat != muteChat || m.voice != muteVoice;
    m.chat = muteChat;
    m.voice = muteVoice;
    if (!muteChat && !muteVoice) {
        r->muted.erase(targetPlayerId);
    }
    if (muteVoice) {
        r->voicePeers.erase(targetPlayerId);
    }
    RoomResult res = success();
    res.roomId = r->id;
    if (changed) {
        RoomEvent e;
        e.type = RoomEventType::USER_MUTED;
        e.playerId = actorPlayerId;
        e.targetPlayerId = targetPlayerId;
        e.detail = std::string("mute_chat=") + (muteChat ? "1" : "0") +
                   ";mute_voice=" + (muteVoice ? "1" : "0");
        res.seq = emitLocked(*r, e, now);
    }
    saveRoomLocked(*r);
    return res;
}

RoomResult GameRoomManager::updateRoomPolicies(const std::string &roomId,
                                               int64_t actorPlayerId,
                                               ObserverPolicy observerPolicy,
                                               VoicePolicy voicePolicy)
{
    return updateRoomPolicies(roomId, actorPlayerId,
                              std::optional<ObserverPolicy>(observerPolicy),
                              std::optional<VoicePolicy>(voicePolicy));
}

RoomResult GameRoomManager::updateRoomPolicies(
    const std::string &roomId, int64_t actorPlayerId,
    const std::optional<ObserverPolicy> &observerPolicyIn,
    const std::optional<VoicePolicy> &voicePolicyIn)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    if (!ensurePlayerLocked(actorPlayerId)) {
        return fail(RoomCode::UNKNOWN_PLAYER, "unknown player");
    }
    if (!observerPolicyIn && !voicePolicyIn) {
        return fail(RoomCode::INVALID_ARGUMENT, "no policy given");
    }
    if (observerPolicyIn) {
        int op = static_cast<int>(*observerPolicyIn);
        if (op < 0 || op > static_cast<int>(ObserverPolicy::NO_OBSERVERS)) {
            return fail(RoomCode::INVALID_ARGUMENT, "invalid policy");
        }
    }
    if (voicePolicyIn) {
        int vp = static_cast<int>(*voicePolicyIn);
        if (vp < 0 || vp > static_cast<int>(VoicePolicy::ALL_ALLOWED)) {
            return fail(RoomCode::INVALID_ARGUMENT, "invalid policy");
        }
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    if (r->banned.count(actorPlayerId) != 0) {
        return fail(RoomCode::FORBIDDEN_BANNED, "banned from this room");
    }
    if (seatOfLocked(*r, actorPlayerId) == 0) {
        return fail(RoomCode::FORBIDDEN_NOT_SEATED,
                    "only seated players can change policies");
    }
    touchOnlineLocked(actorPlayerId);
    ObserverPolicy observerPolicy =
        observerPolicyIn ? *observerPolicyIn : r->config.observerPolicy;
    VoicePolicy voicePolicy =
        voicePolicyIn ? *voicePolicyIn : r->config.voicePolicy;
    RoomResult res = success();
    res.roomId = r->id;
    if (r->config.observerPolicy == observerPolicy &&
        r->config.voicePolicy == voicePolicy) {
        return res;
    }
    r->config.observerPolicy = observerPolicy;
    r->config.voicePolicy = voicePolicy;
    RoomEvent e;
    e.type = RoomEventType::POLICY_UPDATED;
    e.playerId = actorPlayerId;
    e.detail = std::string("observer_policy=") + toString(observerPolicy) +
               ";voice_policy=" + toString(voicePolicy);
    res.seq = emitLocked(*r, e, now);
    enforcePoliciesLocked(*r, now);
    announceTableLocked(*r, false);
    saveRoomLocked(*r);
    return res;
}

/* ------------------------------------------------------------------ */
/* Reads                                                               */
/* ------------------------------------------------------------------ */

void GameRoomManager::fillView(const Room &r, int64_t viewerId, int64_t now,
                               RoomView &out) const
{
    out = RoomView();
    out.roomId = r.id;
    out.phase = r.phase;
    out.paused = r.paused;
    out.config = r.config;
    out.config.passcode.clear();
    out.hasPasscode = !r.config.passcode.empty();
    out.hostPlayerId = r.hostId;
    out.viewerSeat = seatOfLocked(r, viewerId);
    const OctiGame *g = r.board->getGame();
    for (int s = 1; s <= r.seatCount; ++s) {
        SeatView sv;
        sv.seat = s;
        sv.playerId = r.seatPlayer[s];
        sv.handle = r.seatHandle[s];
        sv.rating = r.seatRating[s];
        sv.connected = r.seatConnected[s];
        sv.eliminated = r.phase == RoomPhase::IN_PROGRESS &&
                        g->isPlayerEliminated(static_cast<OctiPlayer>(s));
        if (!sv.connected && r.phase == RoomPhase::IN_PROGRESS) {
            sv.disconnectedForMs =
                std::max<int64_t>(0, now - r.seatDisconnectedAt[s]);
            sv.forfeitClaimable =
                !r.paused && !sv.eliminated &&
                sv.disconnectedForMs >= r.config.timeControl.disconnectGraceMs;
        }
        sv.clockMs = liveClockLocked(r, s, now);
        out.seats.push_back(sv);
    }
    out.observerIds.assign(r.observers.begin(), r.observers.end());
    out.fen = r.board->toPositionString();
    out.turnSeat = r.phase == RoomPhase::IN_PROGRESS
                       ? static_cast<int>(g->getTurn())
                       : 0;
    out.plyCount = static_cast<int>(r.plies.size());
    for (size_t i = 0; i < r.plies.size(); ++i) {
        out.moves.push_back(r.plies[i].move);
    }
    out.result = r.result;
    out.termination = r.termination;
    out.winnerSeat = r.winnerSeat;
    for (size_t i = 0; i < r.proposals.size(); ++i) {
        const Room::Proposal &p = r.proposals[i];
        ProposalView pv;
        pv.type = p.type;
        pv.proposerPlayerId = p.proposerId;
        pv.acceptedBy.assign(p.accepted.begin(), p.accepted.end());
        std::vector<int> ap = approversLocked(r, seatOfLocked(r, p.proposerId));
        for (size_t k = 0; k < ap.size(); ++k) {
            if (p.accepted.count(r.seatPlayer[ap[k]]) == 0) {
                pv.awaiting.push_back(r.seatPlayer[ap[k]]);
            }
        }
        pv.expiresAtMs = p.expiresAtMs;
        out.proposals.push_back(pv);
    }
    for (std::map<int64_t, bool>::const_iterator it = r.voicePeers.begin();
         it != r.voicePeers.end(); ++it) {
        VoicePeerView v;
        v.playerId = it->first;
        v.micMuted = it->second;
        out.voicePeers.push_back(v);
    }
    out.latestSeq = r.nextSeq - 1;
    out.archived = r.archived;
    out.archiveError = r.archiveError;
    out.rematchRoomId = r.rematchRoomId;
    out.createdAtMs = r.createdAtMs;
}

RoomResult GameRoomManager::getRoomView(const std::string &roomId,
                                        int64_t viewerPlayerId, RoomView &out)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    RoomResult chk = checkViewLocked(*r, viewerPlayerId);
    if (!chk.ok()) {
        return chk;
    }
    fillView(*r, viewerPlayerId, now, out);
    RoomResult res = success();
    res.roomId = r->id;
    return res;
}

RoomResult GameRoomManager::pollRoomEvents(const std::string &roomId,
                                           int64_t viewerPlayerId,
                                           int64_t sinceSeq, size_t limit,
                                           RoomPoll &out)
{
    std::lock_guard<std::mutex> lock(_mutex);
    out = RoomPoll();
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    RoomResult chk = checkViewLocked(*r, viewerPlayerId);
    if (!chk.ok()) {
        return chk;
    }
    limit = std::max<size_t>(1, std::min<size_t>(limit, 500));

    /*
     * The cursor acknowledges everything up to sinceSeq, so relayed voice
     * signals addressed to this viewer at or below it are now delivered and
     * can be dropped: signals are never retained beyond delivery.
     */
    if (viewerPlayerId != 0) {
        for (std::deque<RoomEvent>::iterator it = r->events.begin();
             it != r->events.end();) {
            if (it->type == RoomEventType::VOICE_SIGNAL &&
                it->targetPlayerId == viewerPlayerId && it->seq <= sinceSeq) {
                it = r->events.erase(it);
            } else {
                ++it;
            }
        }
    }

    out.latestSeq = r->nextSeq - 1;
    out.gap = sinceSeq < r->evictedUpToSeq;
    for (std::deque<RoomEvent>::const_iterator it = r->events.begin();
         it != r->events.end(); ++it) {
        if (it->seq <= sinceSeq || !eventVisibleLocked(*r, *it, viewerPlayerId)) {
            continue;
        }
        if (out.events.size() >= limit) {
            out.hasMore = true;
            break;
        }
        out.events.push_back(*it);
    }
    RoomResult res = success();
    res.roomId = r->id;
    return res;
}

RoomResult GameRoomManager::getTranscript(const std::string &roomId,
                                          int64_t viewerPlayerId,
                                          std::string &out)
{
    std::lock_guard<std::mutex> lock(_mutex);
    Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return fail(RoomCode::NOT_FOUND, "no such room");
    }
    int64_t now = nowMs();
    advanceRoomLocked(*r, now);
    RoomResult chk = checkViewLocked(*r, viewerPlayerId);
    if (!chk.ok()) {
        return chk;
    }
    out = buildTranscriptLocked(*r);
    RoomResult res = success();
    res.roomId = r->id;
    return res;
}

bool GameRoomManager::isBanned(const std::string &roomId,
                               int64_t playerId) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    const Room *r = findRoomLocked(roomId);
    return r != nullptr && r->banned.count(playerId) != 0;
}

std::vector<int64_t> GameRoomManager::bannedPlayers(const std::string &roomId) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    const Room *r = findRoomLocked(roomId);
    if (r == nullptr) {
        return std::vector<int64_t>();
    }
    return std::vector<int64_t>(r->banned.begin(), r->banned.end());
}

/* ------------------------------------------------------------------ */
/* Live state persistence                                              */
/* ------------------------------------------------------------------ */

bool GameRoomManager::checkpointToStoreLocked()
{
    for (std::map<std::string, std::unique_ptr<Room>>::const_iterator it =
             _rooms.begin();
         it != _rooms.end(); ++it) {
        saveRoomLocked(*it->second);
    }
    saveLobbyMetaLocked();
    return true;
}

void GameRoomManager::saveRoomLocked(const Room &r)
{
    if (_restoring) {
        return;
    }
    int64_t now = nowMs();
    Json j;
    j["id"] = r.id;

    // config
    Json cfg;
    cfg["mode"] = static_cast<int>(r.config.mode);
    cfg["bases"] = static_cast<int>(r.config.bases);
    cfg["useSuperProngs"] = r.config.useSuperProngs;
    cfg["useEdgelessBoard"] = r.config.useEdgelessBoard;
    Json tc;
    tc["enabled"] = r.config.timeControl.enabled;
    tc["initialMs"] = r.config.timeControl.initialMs;
    tc["incrementMs"] = r.config.timeControl.incrementMs;
    tc["disconnectGraceMs"] = r.config.timeControl.disconnectGraceMs;
    cfg["timeControl"] = tc;
    cfg["rated"] = r.config.rated;
    cfg["allowTakeback"] = r.config.allowTakeback;
    cfg["observerPolicy"] = toString(r.config.observerPolicy);
    cfg["voicePolicy"] = toString(r.config.voicePolicy);
    cfg["passcode"] = r.config.passcode;
    j["config"] = cfg;

    j["phase"] = toString(r.phase);
    j["hostId"] = r.hostId;
    j["seatCount"] = r.seatCount;

    // seats
    Json seats = Json::array();
    for (int s = 1; s <= 4; ++s) {
        Json sj;
        sj["seat"] = s;
        sj["player"] = r.seatPlayer[s];
        sj["handle"] = r.seatHandle[s];
        sj["rating"] = r.seatRating[s];
        sj["connected"] = r.seatConnected[s];
        int64_t disconnectedForMs = 0;
        if (!r.seatConnected[s] && r.seatDisconnectedAt[s] > 0) {
            disconnectedForMs = std::max<int64_t>(0, now - r.seatDisconnectedAt[s]);
        }
        sj["disconnectedForMs"] = disconnectedForMs;
        sj["clockMs"] = r.clockMs[s];
        seats.push_back(sj);
    }
    j["seats"] = seats;

    // clock / turn freeze
    int64_t effectiveTurnCarryMs = r.turnCarryMs;
    if (r.phase == RoomPhase::IN_PROGRESS && r.config.timeControl.enabled &&
        !r.paused && r.turnStartedMs > 0) {
        effectiveTurnCarryMs += std::max<int64_t>(0, now - r.turnStartedMs);
    }
    j["effectiveTurnCarryMs"] = effectiveTurnCarryMs;
    j["paused"] = r.paused;

    // plies
    Json plies = Json::array();
    for (size_t i = 0; i < r.plies.size(); ++i) {
        Json p;
        p["seat"] = r.plies[i].seat;
        p["move"] = r.plies[i].move;
        p["clockBeforeMs"] = r.plies[i].clockBeforeMs;
        p["clockAfterMs"] = r.plies[i].clockAfterMs;
        p["elapsedMs"] = r.plies[i].elapsedMs;
        plies.push_back(p);
    }
    j["plies"] = plies;

    // board fen position snapshot
    if (r.board) {
        j["fen"] = r.board->toPositionString();
    } else {
        j["fen"] = "";
    }

    // observers
    Json obs = Json::array();
    for (int64_t o : r.observers) {
        obs.push_back(o);
    }
    j["observers"] = obs;

    // banned
    Json banned = Json::array();
    for (int64_t b : r.banned) {
        banned.push_back(b);
    }
    j["banned"] = banned;

    // muted
    Json muted = Json::object();
    for (std::map<int64_t, Room::Mute>::const_iterator it = r.muted.begin();
         it != r.muted.end(); ++it) {
        Json m;
        m["chat"] = it->second.chat;
        m["voice"] = it->second.voice;
        muted[std::to_string(it->first)] = m;
    }
    j["muted"] = muted;

    // voicePeers
    Json vp = Json::object();
    for (std::map<int64_t, bool>::const_iterator it = r.voicePeers.begin();
         it != r.voicePeers.end(); ++it) {
        vp[std::to_string(it->first)] = it->second;
    }
    j["voicePeers"] = vp;

    // proposals
    Json props = Json::array();
    for (size_t i = 0; i < r.proposals.size(); ++i) {
        const Room::Proposal &p = r.proposals[i];
        Json pj;
        pj["type"] = toString(p.type);
        pj["proposerId"] = p.proposerId;
        Json acc = Json::array();
        for (int64_t a : p.accepted) {
            acc.push_back(a);
        }
        pj["accepted"] = acc;
        pj["remainingTtlMs"] = std::max<int64_t>(1, p.expiresAtMs - now);
        props.push_back(pj);
    }
    j["proposals"] = props;

    // events (exclude RoomEventType::VOICE_SIGNAL)
    j["nextSeq"] = r.nextSeq;
    j["evictedUpToSeq"] = r.evictedUpToSeq;
    Json evs = Json::array();
    for (size_t i = 0; i < r.events.size(); ++i) {
        const RoomEvent &ev = r.events[i];
        if (ev.type == RoomEventType::VOICE_SIGNAL) {
            continue;
        }
        Json ej;
        ej["seq"] = ev.seq;
        ej["type"] = toString(ev.type);
        ej["ageMs"] = std::max<int64_t>(0, now - ev.atMs);
        ej["playerId"] = ev.playerId;
        ej["handle"] = ev.handle;
        ej["targetPlayerId"] = ev.targetPlayerId;
        ej["seat"] = ev.seat;
        ej["scope"] = toString(ev.scope);
        ej["ply"] = ev.ply;
        ej["clockMs"] = ev.clockMs;
        ej["elapsedMs"] = ev.elapsedMs;
        ej["text"] = ev.text;
        ej["detail"] = ev.detail;
        evs.push_back(ej);
    }
    j["events"] = evs;

    // result / termination / winnerSeat
    j["result"] = r.result;
    j["termination"] = r.termination;
    j["winnerSeat"] = r.winnerSeat;

    // archive / rematch
    j["archived"] = r.archived;
    j["archiveError"] = r.archiveError;
    j["archiveAttempts"] = r.archiveAttempts;
    j["rematchRoomId"] = r.rematchRoomId;

    // timestamps
    j["idleElapsedMs"] = std::max<int64_t>(0, now - r.lastActivityMs);
    j["createdElapsedMs"] = std::max<int64_t>(0, now - r.createdAtMs);
    j["endedElapsedMs"] = (r.endedAtMs > 0) ? std::max<int64_t>(0, now - r.endedAtMs) : -1;
    j["startedAtSec"] = r.startedAtSec;
    j["endedAtSec"] = r.endedAtSec;

    std::string err;
    _store.upsertLiveRoom(r.id, j.dump(), err);
}

void GameRoomManager::saveLobbyMetaLocked()
{
    if (_restoring) {
        return;
    }
    int64_t now = nowMs();
    Json j;
    j["roomCounter"] = _roomCounter;
    j["lobbySeq"] = _lobbySeq;
    j["lobbyEvictedUpToSeq"] = _lobbyEvictedUpToSeq;

    Json evs = Json::array();
    for (size_t i = 0; i < _lobbyEvents.size(); ++i) {
        const LobbyEvent &ev = _lobbyEvents[i];
        Json ej;
        ej["seq"] = ev.seq;
        ej["type"] = toString(ev.type);
        ej["ageMs"] = std::max<int64_t>(0, now - ev.atMs);
        ej["playerId"] = ev.playerId;
        ej["handle"] = ev.handle;
        ej["targetPlayerId"] = ev.targetPlayerId;
        ej["text"] = ev.text;
        ej["status"] = toString(ev.status);
        ej["roomId"] = ev.roomId;
        evs.push_back(ej);
    }
    j["lobbyEvents"] = evs;

    Json assignments = Json::object();
    for (std::map<int64_t, std::string>::const_iterator it =
             _matchAssignments.begin();
         it != _matchAssignments.end(); ++it) {
        assignments[std::to_string(it->first)] = it->second;
    }
    j["matchAssignments"] = assignments;

    std::vector<QueuedPlayerSnapshot> qSnap = _queue.snapshot();
    Json qArr = Json::array();
    for (size_t i = 0; i < qSnap.size(); ++i) {
        const QueuedPlayerSnapshot &s = qSnap[i];
        Json qj;
        qj["playerId"] = s.playerId;
        qj["rating"] = s.rating;
        Json rs;
        rs["mode"] = static_cast<int>(s.ruleset.mode);
        rs["bases"] = static_cast<int>(s.ruleset.bases);
        rs["useSuperProngs"] = s.ruleset.useSuperProngs;
        rs["useEdgelessBoard"] = s.ruleset.useEdgelessBoard;
        rs["initialMs"] = s.ruleset.initialMs;
        rs["incrementMs"] = s.ruleset.incrementMs;
        qj["ruleset"] = rs;
        qj["waitedMs"] = s.waitedMs;
        qArr.push_back(qj);
    }
    j["queue"] = qArr;

    std::string err;
    _store.setLiveMeta("lobby_state", j.dump(), err);
}

bool GameRoomManager::restoreFromStoreLocked()
{
    _restoring = true;
    struct RestoringGuard {
        bool &flag;
        ~RestoringGuard() { flag = false; }
    } guard{_restoring};

    _rooms.clear();
    _seatRoom.clear();
    _observing.clear();

    int64_t restoreNow = nowMs();
    int64_t curWallSec = wallSec();

    std::vector<LiveRoomRow> rows;
    std::string err;
    if (!_store.listLiveRooms(rows, err)) {
        return false;
    }

    for (size_t idx = 0; idx < rows.size(); ++idx) {
        const LiveRoomRow &row = rows[idx];
        // 24-hour stale expiration
        bool eligibleForStaleCheck = _customWallClock || (row.updatedAt >= 1000000000LL);
        if (eligibleForStaleCheck && row.updatedAt > 0 && curWallSec > row.updatedAt &&
            (curWallSec - row.updatedAt) > kMaxLiveRoomOfflineSec) {
            _store.deleteLiveRoom(row.roomId, err);
            continue;
        }

        Json j;
        try {
            j = Json::parse(row.stateJson, nullptr, false);
            if (j.is_discarded() || !j.is_object()) {
                _store.deleteLiveRoom(row.roomId, err);
                continue;
            }
        } catch (...) {
            _store.deleteLiveRoom(row.roomId, err);
            continue;
        }

        try {
            std::unique_ptr<Room> r(new Room());
            r->id = j.value("id", row.roomId);
            if (r->id.empty()) {
                _store.deleteLiveRoom(row.roomId, err);
                continue;
            }

            // config must exist and be an object
            if (!j.contains("config") || !j["config"].is_object()) {
                _store.deleteLiveRoom(row.roomId, err);
                continue;
            }
            const Json &cfg = j["config"];
                r->config.mode = static_cast<OctiGameMode>(
                    cfg.value("mode", static_cast<int>(GAME_MODE_2)));
                r->config.bases = static_cast<OctiGameBases>(
                    cfg.value("bases", static_cast<int>(GAME_BASES_3)));
                r->config.useSuperProngs = cfg.value("useSuperProngs", false);
                r->config.useEdgelessBoard = cfg.value("useEdgelessBoard", false);
                if (cfg.contains("timeControl") && cfg["timeControl"].is_object()) {
                    const Json &tc = cfg["timeControl"];
                    r->config.timeControl.enabled = tc.value("enabled", false);
                    r->config.timeControl.initialMs = tc.value("initialMs", 0LL);
                    r->config.timeControl.incrementMs = tc.value("incrementMs", 0LL);
                    r->config.timeControl.disconnectGraceMs =
                        tc.value("disconnectGraceMs", 90000LL);
                }
                r->config.rated = cfg.value("rated", true);
                r->config.allowTakeback = cfg.value("allowTakeback", false);
                parseObserverPolicy(cfg.value("observerPolicy", "OPEN"),
                                    r->config.observerPolicy);
                parseVoicePolicy(cfg.value("voicePolicy", "PLAYERS_ONLY"),
                                 r->config.voicePolicy);
                r->config.passcode = cfg.value("passcode", "");

            std::string phaseStr = j.value("phase", "WAITING_FOR_PLAYERS");
            if (phaseStr == "IN_PROGRESS") {
                r->phase = RoomPhase::IN_PROGRESS;
            } else if (phaseStr == "FINISHED") {
                r->phase = RoomPhase::FINISHED;
            } else {
                r->phase = RoomPhase::WAITING_FOR_PLAYERS;
            }

            r->hostId = j.value("hostId", 0LL);
            if (r->hostId <= 0) {
                _store.deleteLiveRoom(row.roomId, err);
                continue;
            }
            r->seatCount = j.value("seatCount", seatCountOf(r->config.mode));

            // seats
            if (j.contains("seats") && j["seats"].is_array()) {
                for (size_t si = 0; si < j["seats"].size(); ++si) {
                    const Json &sj = j["seats"][si];
                    int s = sj.value("seat", 0);
                    if (s >= 1 && s <= 4) {
                        r->seatPlayer[s] = sj.value("player", 0LL);
                        r->seatHandle[s] = sj.value("handle", "");
                        r->seatRating[s] = sj.value("rating", 0);
                        r->seatConnected[s] = sj.value("connected", true);
                        int64_t disconnectedForMs = sj.value("disconnectedForMs", 0LL);
                        if (!r->seatConnected[s]) {
                            r->seatDisconnectedAt[s] =
                                restoreNow - std::max<int64_t>(0, disconnectedForMs);
                        } else {
                            r->seatDisconnectedAt[s] = 0;
                        }
                        r->clockMs[s] = sj.value("clockMs", 0LL);
                    }
                }
            }

            // clocks / turn
            int64_t effTurnCarryMs = j.value("effectiveTurnCarryMs", 0LL);
            r->turnCarryMs = effTurnCarryMs;
            r->paused = j.value("paused", false);
            r->turnStartedMs =
                (r->phase == RoomPhase::IN_PROGRESS && !r->paused) ? restoreNow : 0;

            // plies
            if (j.contains("plies") && j["plies"].is_array()) {
                for (size_t pi = 0; pi < j["plies"].size(); ++pi) {
                    const Json &pj = j["plies"][pi];
                    Room::Ply ply;
                    ply.seat = pj.value("seat", 0);
                    ply.move = pj.value("move", "");
                    ply.clockBeforeMs = pj.value("clockBeforeMs", 0LL);
                    ply.clockAfterMs = pj.value("clockAfterMs", 0LL);
                    ply.elapsedMs = pj.value("elapsedMs", 0LL);
                    r->plies.push_back(ply);
                }
            }

            // board reconstruction
            r->board.reset(new OctiBoard(r->config.mode, r->config.useSuperProngs,
                                         r->config.useEdgelessBoard, r->config.bases));
            if (r->phase != RoomPhase::WAITING_FOR_PLAYERS) {
                OctiGame *g = r->board->getGame();
                g->start(PLAYER_A);
                bool replayFailed = false;
                for (size_t pidx = 0; pidx < r->plies.size(); ++pidx) {
                    try {
                        g->move(static_cast<OctiPlayer>(r->plies[pidx].seat),
                                r->plies[pidx].move);
                    } catch (...) {
                        replayFailed = true;
                        break;
                    }
                }
                std::string fen = j.value("fen", "");
                if (replayFailed && !fen.empty()) {
                    try {
                        r->board->fromPositionString(fen);
                    } catch (...) {}
                }
                if (r->phase == RoomPhase::FINISHED && !g->isGameOver()) {
                    int wSeat = j.value("winnerSeat", 0);
                    if (wSeat > 0) {
                        OctiPlayer loser = (wSeat == 1) ? PLAYER_B : PLAYER_A;
                        try {
                            g->resign(loser);
                        } catch (...) {}
                    }
                }
            }

            // observers
            if (j.contains("observers") && j["observers"].is_array()) {
                for (size_t oi = 0; oi < j["observers"].size(); ++oi) {
                    int64_t pid = j["observers"][oi].get<int64_t>();
                    if (pid > 0) {
                        r->observers.insert(pid);
                    }
                }
            }

            // banned
            if (j.contains("banned") && j["banned"].is_array()) {
                for (size_t bi = 0; bi < j["banned"].size(); ++bi) {
                    int64_t pid = j["banned"][bi].get<int64_t>();
                    if (pid > 0) {
                        r->banned.insert(pid);
                    }
                }
            }

            // muted
            if (j.contains("muted") && j["muted"].is_object()) {
                for (Json::const_iterator it = j["muted"].begin();
                     it != j["muted"].end(); ++it) {
                    int64_t pid = std::stoll(it.key());
                    Room::Mute m;
                    m.chat = it.value().value("chat", false);
                    m.voice = it.value().value("voice", false);
                    r->muted[pid] = m;
                }
            }

            // voicePeers
            if (j.contains("voicePeers") && j["voicePeers"].is_object()) {
                for (Json::const_iterator it = j["voicePeers"].begin();
                     it != j["voicePeers"].end(); ++it) {
                    int64_t pid = std::stoll(it.key());
                    r->voicePeers[pid] = it.value().get<bool>();
                }
            }

            // proposals
            if (j.contains("proposals") && j["proposals"].is_array()) {
                for (size_t pri = 0; pri < j["proposals"].size(); ++pri) {
                    const Json &pj = j["proposals"][pri];
                    Room::Proposal prop;
                    parseProposalType(pj.value("type", "DRAW_OFFER"), prop.type);
                    prop.proposerId = pj.value("proposerId", 0LL);
                    if (pj.contains("accepted") && pj["accepted"].is_array()) {
                        for (size_t ai = 0; ai < pj["accepted"].size(); ++ai) {
                            prop.accepted.insert(pj["accepted"][ai].get<int64_t>());
                        }
                    }
                    int64_t remainingTtlMs = pj.value("remainingTtlMs", 60000LL);
                    prop.expiresAtMs = restoreNow + std::max<int64_t>(1, remainingTtlMs);
                    r->proposals.push_back(prop);
                }
            }

            // events
            r->nextSeq = j.value("nextSeq", 1LL);
            r->evictedUpToSeq = j.value("evictedUpToSeq", 0LL);
            if (j.contains("events") && j["events"].is_array()) {
                for (size_t ei = 0; ei < j["events"].size(); ++ei) {
                    const Json &ej = j["events"][ei];
                    RoomEvent ev;
                    ev.seq = ej.value("seq", 0LL);
                    std::string tStr = ej.value("type", "ROOM_CREATED");
                    for (int ti = 0;
                         ti <= static_cast<int>(RoomEventType::PLAYER_RECONNECTED);
                         ++ti) {
                        if (toString(static_cast<RoomEventType>(ti)) == tStr) {
                            ev.type = static_cast<RoomEventType>(ti);
                            break;
                        }
                    }
                    int64_t ageMs = ej.value("ageMs", 0LL);
                    ev.atMs = restoreNow - std::max<int64_t>(0, ageMs);
                    ev.playerId = ej.value("playerId", 0LL);
                    ev.handle = ej.value("handle", "");
                    ev.targetPlayerId = ej.value("targetPlayerId", 0LL);
                    ev.seat = ej.value("seat", 0);
                    parseChatScope(ej.value("scope", "ALL"), ev.scope);
                    ev.ply = ej.value("ply", 0);
                    ev.clockMs = ej.value("clockMs", 0LL);
                    ev.elapsedMs = ej.value("elapsedMs", 0LL);
                    ev.text = ej.value("text", "");
                    ev.detail = ej.value("detail", "");
                    r->events.push_back(ev);
                }
            }

            // outcome
            r->result = j.value("result", "");
            r->termination = j.value("termination", "");
            r->winnerSeat = j.value("winnerSeat", 0);

            // archive / rematch
            r->archived = j.value("archived", false);
            r->archiveError = j.value("archiveError", "");
            r->archiveAttempts = j.value("archiveAttempts", 0);
            r->rematchRoomId = j.value("rematchRoomId", "");

            // timestamps
            int64_t idleElapsedMs = j.value("idleElapsedMs", 0LL);
            int64_t createdElapsedMs = j.value("createdElapsedMs", 0LL);
            int64_t endedElapsedMs = j.value("endedElapsedMs", -1LL);
            r->lastActivityMs = restoreNow - std::max<int64_t>(0, idleElapsedMs);
            r->createdAtMs = restoreNow - std::max<int64_t>(0, createdElapsedMs);
            r->endedAtMs = (endedElapsedMs >= 0) ? (restoreNow - endedElapsedMs) : 0;
            r->startedAtSec = j.value("startedAtSec", 0LL);
            r->endedAtSec = j.value("endedAtSec", 0LL);

            // Link presence and seat mapping
            if (r->hostId > 0) {
                ensurePlayerLocked(r->hostId);
            }
            for (int s = 1; s <= r->seatCount; ++s) {
                int64_t pid = r->seatPlayer[s];
                if (pid > 0) {
                    ensurePlayerLocked(pid);
                    if (r->phase != RoomPhase::FINISHED) {
                        _seatRoom[pid] = r->id;
                        if (r->seatConnected[s]) {
                            _players[pid].online = true;
                        }
                    }
                }
            }
            for (std::set<int64_t>::const_iterator oit = r->observers.begin();
                 oit != r->observers.end(); ++oit) {
                ensurePlayerLocked(*oit);
                if (r->phase != RoomPhase::FINISHED) {
                    _observing[*oit].insert(r->id);
                    _players[*oit].online = true;
                }
            }

            _rooms[r->id] = std::move(r);
        } catch (...) {
            _store.deleteLiveRoom(row.roomId, err);
            continue;
        }
    }

    // Restore live_meta "lobby_state"
    std::string metaJson;
    if (_store.getLiveMeta("lobby_state", metaJson, err) && !metaJson.empty()) {
        try {
            Json mj = Json::parse(metaJson, nullptr, false);
            if (!mj.is_discarded() && mj.is_object()) {
                _roomCounter = mj.value("roomCounter", 0ULL);
                _lobbySeq = mj.value("lobbySeq", 0LL);
                _lobbyEvictedUpToSeq = mj.value("lobbyEvictedUpToSeq", 0LL);

                _lobbyEvents.clear();
                _lobbyChatCount = 0;
                if (mj.contains("lobbyEvents") && mj["lobbyEvents"].is_array()) {
                    for (size_t lei = 0; lei < mj["lobbyEvents"].size(); ++lei) {
                        const Json &ej = mj["lobbyEvents"][lei];
                        LobbyEvent ev;
                        ev.seq = ej.value("seq", 0LL);
                        std::string tStr = ej.value("type", "LOBBY_CHAT");
                        if (tStr == "PRESENCE") ev.type = LobbyEventType::PRESENCE;
                        else if (tStr == "TABLE_UPDATED") ev.type = LobbyEventType::TABLE_UPDATED;
                        else if (tStr == "TABLE_CLOSED") ev.type = LobbyEventType::TABLE_CLOSED;
                        else if (tStr == "MATCH_FOUND") ev.type = LobbyEventType::MATCH_FOUND;
                        else ev.type = LobbyEventType::LOBBY_CHAT;

                        int64_t ageMs = ej.value("ageMs", 0LL);
                        ev.atMs = restoreNow - std::max<int64_t>(0, ageMs);
                        ev.playerId = ej.value("playerId", 0LL);
                        ev.handle = ej.value("handle", "");
                        ev.targetPlayerId = ej.value("targetPlayerId", 0LL);
                        ev.text = ej.value("text", "");
                        std::string stStr = ej.value("status", "IN_LOBBY");
                        if (stStr == "OFFLINE") ev.status = PresenceStatus::OFFLINE;
                        else if (stStr == "AT_TABLE") ev.status = PresenceStatus::AT_TABLE;
                        else if (stStr == "PLAYING") ev.status = PresenceStatus::PLAYING;
                        else if (stStr == "OBSERVING") ev.status = PresenceStatus::OBSERVING;
                        else ev.status = PresenceStatus::IN_LOBBY;
                        ev.roomId = ej.value("roomId", "");

                        if (ev.type == LobbyEventType::LOBBY_CHAT) {
                            ++_lobbyChatCount;
                        }
                        _lobbyEvents.push_back(ev);
                    }
                }

                _matchAssignments.clear();
                if (mj.contains("matchAssignments") && mj["matchAssignments"].is_object()) {
                    for (Json::const_iterator it = mj["matchAssignments"].begin();
                         it != mj["matchAssignments"].end(); ++it) {
                        int64_t pid = std::stoll(it.key());
                        std::string rid = it.value().get<std::string>();
                        if (_rooms.count(rid) != 0) {
                            _matchAssignments[pid] = rid;
                        }
                    }
                }

                if (mj.contains("queue") && mj["queue"].is_array()) {
                    std::vector<QueuedPlayerSnapshot> qEntries;
                    for (size_t qi = 0; qi < mj["queue"].size(); ++qi) {
                        const Json &qj = mj["queue"][qi];
                        QueuedPlayerSnapshot s;
                        s.playerId = qj.value("playerId", 0LL);
                        s.rating = qj.value("rating", 1200);
                        if (qj.contains("ruleset") && qj["ruleset"].is_object()) {
                            const Json &rs = qj["ruleset"];
                            s.ruleset.mode = static_cast<OctiGameMode>(
                                rs.value("mode", static_cast<int>(GAME_MODE_2)));
                            s.ruleset.bases = static_cast<OctiGameBases>(
                                rs.value("bases", static_cast<int>(GAME_BASES_3)));
                            s.ruleset.useSuperProngs = rs.value("useSuperProngs", false);
                            s.ruleset.useEdgelessBoard = rs.value("useEdgelessBoard", false);
                            s.ruleset.initialMs = rs.value("initialMs", 0LL);
                            s.ruleset.incrementMs = rs.value("incrementMs", 0LL);
                        }
                        s.waitedMs = qj.value("waitedMs", 0LL);
                        qEntries.push_back(s);
                    }
                    _queue.restore(qEntries);
                }
            }
        } catch (...) {}
    }

    return true;
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
