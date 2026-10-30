/*
 * AccountStore.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "AccountStore.hxx"

#include <nlohmann/json.hpp>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sqlite3.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <set>
#include <system_error>

namespace octi {

namespace {

const size_t kMaxSubBytes = 255;
const size_t kMaxEmailBytes = 254;
const size_t kMaxHintBytes = 128;
const size_t kMaxDisplayNameBytes = 64;
const size_t kMaxAvatarUrlBytes = 512;
const size_t kMaxAgentBytes = 128;
const size_t kMaxTokenBytes = 256;
const size_t kMaxTranscriptBytes = 1048576;
const size_t kMaxFenBytes = 1024;
const size_t kMaxGameIdBytes = 64;
const size_t kMaxGameHandleBytes = 64;
const int kMaxElo = 10000;
const int kMaxOffset = 10000000;

const char kSchemaSql[] =
    "CREATE TABLE IF NOT EXISTS players ("
    " player_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " uuid TEXT NOT NULL UNIQUE,"
    " handle TEXT NOT NULL UNIQUE,"
    " display_name TEXT NOT NULL,"
    " avatar_url TEXT NOT NULL DEFAULT '',"
    " elo_2p INTEGER NOT NULL DEFAULT 1200,"
    " elo_4p INTEGER NOT NULL DEFAULT 1200,"
    " elo_ofk INTEGER NOT NULL DEFAULT 1200,"
    " games_played INTEGER NOT NULL DEFAULT 0,"
    " wins INTEGER NOT NULL DEFAULT 0,"
    " losses INTEGER NOT NULL DEFAULT 0,"
    " draws INTEGER NOT NULL DEFAULT 0,"
    " created_at INTEGER NOT NULL,"
    " last_seen_at INTEGER NOT NULL"
    ");"
    "CREATE TABLE IF NOT EXISTS linked_identities ("
    " identity_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " player_id INTEGER NOT NULL REFERENCES players(player_id) ON DELETE CASCADE,"
    " provider TEXT NOT NULL,"
    " provider_sub TEXT NOT NULL,"
    " provider_email TEXT NOT NULL DEFAULT '',"
    " display_hint TEXT NOT NULL DEFAULT '',"
    " linked_at INTEGER NOT NULL,"
    " last_login_at INTEGER NOT NULL,"
    " UNIQUE(provider, provider_sub)"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_linked_identities_player"
    " ON linked_identities(player_id);"
    "CREATE TABLE IF NOT EXISTS sessions ("
    " session_id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " token_hash TEXT NOT NULL UNIQUE,"
    " player_id INTEGER NOT NULL REFERENCES players(player_id) ON DELETE CASCADE,"
    " client_agent TEXT NOT NULL DEFAULT '',"
    " created_at INTEGER NOT NULL,"
    " expires_at INTEGER NOT NULL,"
    " revoked INTEGER NOT NULL DEFAULT 0"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_sessions_player ON sessions(player_id);"
    "CREATE TABLE IF NOT EXISTS games ("
    " game_id TEXT PRIMARY KEY,"
    " game_mode TEXT NOT NULL,"
    " game_bases INTEGER NOT NULL,"
    " super_prongs INTEGER NOT NULL,"
    " edgeless INTEGER NOT NULL,"
    " time_control TEXT NOT NULL DEFAULT 'untimed',"
    " p1_player_id INTEGER REFERENCES players(player_id),"
    " p2_player_id INTEGER REFERENCES players(player_id),"
    " p3_player_id INTEGER REFERENCES players(player_id),"
    " p4_player_id INTEGER REFERENCES players(player_id),"
    " p1_handle TEXT NOT NULL,"
    " p2_handle TEXT NOT NULL,"
    " p3_handle TEXT NOT NULL DEFAULT '',"
    " p4_handle TEXT NOT NULL DEFAULT '',"
    " result TEXT NOT NULL,"
    " winner_seat INTEGER NOT NULL DEFAULT 0,"
    " termination TEXT NOT NULL,"
    " ply_count INTEGER NOT NULL,"
    " octi_transcript TEXT NOT NULL DEFAULT '',"
    " transcript_json TEXT NOT NULL DEFAULT '',"
    " final_fen TEXT NOT NULL,"
    " started_at INTEGER NOT NULL,"
    " ended_at INTEGER NOT NULL,"
    " source TEXT NOT NULL DEFAULT 'online',"
    " uploaded_at INTEGER NOT NULL DEFAULT 0,"
    " winner TEXT NOT NULL DEFAULT 'NONE',"
    " status TEXT NOT NULL DEFAULT 'GAME_OVER',"
    " ai_difficulty INTEGER NOT NULL DEFAULT 0,"
    " app_version TEXT NOT NULL DEFAULT '',"
    " platform TEXT NOT NULL DEFAULT '',"
    " install_id TEXT NOT NULL DEFAULT ''"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_games_ended_at ON games(ended_at DESC);"
    "CREATE INDEX IF NOT EXISTS idx_games_p1 ON games(p1_player_id);"
    "CREATE INDEX IF NOT EXISTS idx_games_p2 ON games(p2_player_id);"
    "CREATE TABLE IF NOT EXISTS live_rooms ("
    " room_id TEXT PRIMARY KEY,"
    " state_json TEXT NOT NULL,"
    " updated_at INTEGER NOT NULL"
    ");"
    "CREATE TABLE IF NOT EXISTS live_meta ("
    " meta_key TEXT PRIMARY KEY,"
    " meta_json TEXT NOT NULL,"
    " updated_at INTEGER NOT NULL"
    ");";

const char kProfileCols[] =
    "player_id, uuid, handle, display_name, avatar_url, elo_2p, elo_4p,"
    " elo_ofk, games_played, wins, losses, draws, created_at, last_seen_at";

#define OCTI_GAME_COLS_HEAD                                                    \
    "game_id, game_mode, game_bases, super_prongs, edgeless, time_control,"    \
    " COALESCE(p1_player_id, 0), COALESCE(p2_player_id, 0),"                   \
    " COALESCE(p3_player_id, 0), COALESCE(p4_player_id, 0),"                   \
    " p1_handle, p2_handle, p3_handle, p4_handle, result, winner_seat,"        \
    " termination, ply_count, "
#define OCTI_GAME_COLS_TAIL                                                    \
    ", final_fen, started_at, ended_at, source, uploaded_at, winner, status," \
    " ai_difficulty, app_version, platform, install_id"

const char kGameColsFull[] =
    OCTI_GAME_COLS_HEAD "COALESCE(NULLIF(transcript_json, ''), octi_transcript) AS octi_transcript" OCTI_GAME_COLS_TAIL;
const char kGameColsNoTranscript[] =
    OCTI_GAME_COLS_HEAD "'' AS octi_transcript" OCTI_GAME_COLS_TAIL;

/* RAII prepared statement; every value is bound, never concatenated. */
class Stmt {
public:
    Stmt(sqlite3 *db, const char *sql)
        : _stmt(nullptr), _rc(SQLITE_OK), _bindRc(SQLITE_OK)
    {
        _rc = sqlite3_prepare_v2(db, sql, -1, &_stmt, nullptr);
    }
    ~Stmt() { sqlite3_finalize(_stmt); }

    Stmt(const Stmt &) = delete;
    Stmt &operator=(const Stmt &) = delete;

    bool ok() const { return _rc == SQLITE_OK && _bindRc == SQLITE_OK; }
    void bindText(int idx, const std::string &s)
    {
        note(sqlite3_bind_text(_stmt, idx, s.data(),
                               static_cast<int>(s.size()), SQLITE_TRANSIENT));
    }
    void bindInt(int idx, int64_t v) { note(sqlite3_bind_int64(_stmt, idx, v)); }
    void bindNull(int idx) { note(sqlite3_bind_null(_stmt, idx)); }
    int step()
    {
        if (!ok()) {
            return SQLITE_MISUSE;
        }
        return sqlite3_step(_stmt);
    }
    int64_t colInt(int c) const { return sqlite3_column_int64(_stmt, c); }
    double colDouble(int c) const { return sqlite3_column_double(_stmt, c); }
    std::string colText(int c) const
    {
        const unsigned char *t = sqlite3_column_text(_stmt, c);
        int n = sqlite3_column_bytes(_stmt, c);
        if (!t || n <= 0) {
            return std::string();
        }
        return std::string(reinterpret_cast<const char *>(t),
                           static_cast<size_t>(n));
    }

private:
    void note(int rc)
    {
        if (rc != SQLITE_OK && _bindRc == SQLITE_OK) {
            _bindRc = rc;
        }
    }

    sqlite3_stmt *_stmt;
    int _rc;
    int _bindRc;
};

bool execSql(sqlite3 *db, const char *sql)
{
    char *err = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (err) {
        sqlite3_free(err);
    }
    return rc == SQLITE_OK;
}

/* BEGIN IMMEDIATE ... COMMIT guard; rolls back unless commit() succeeded. */
class Tx {
public:
    explicit Tx(sqlite3 *db) : _db(db), _active(false)
    {
        _active = execSql(db, "BEGIN IMMEDIATE");
    }
    ~Tx()
    {
        if (_active) {
            execSql(_db, "ROLLBACK");
        }
    }
    Tx(const Tx &) = delete;
    Tx &operator=(const Tx &) = delete;

    bool begun() const { return _active; }
    bool commit()
    {
        if (!_active) {
            return false;
        }
        if (execSql(_db, "COMMIT")) {
            _active = false;
            return true;
        }
        return false;
    }

private:
    sqlite3 *_db;
    bool _active;
};

bool randomBytes(unsigned char *buf, size_t len)
{
    return RAND_bytes(buf, static_cast<int>(len)) == 1;
}

bool randomHex(size_t hexChars, std::string &out)
{
    static const char hex[] = "0123456789abcdef";
    std::string bytes((hexChars + 1) / 2, '\0');
    if (!randomBytes(reinterpret_cast<unsigned char *>(&bytes[0]),
                     bytes.size())) {
        return false;
    }
    out.clear();
    for (size_t i = 0; i < hexChars; ++i) {
        unsigned char b = static_cast<unsigned char>(bytes[i / 2]);
        out.push_back(hex[(i % 2 == 0) ? (b >> 4) : (b & 0x0F)]);
    }
    return true;
}

bool uuidV4(std::string &out)
{
    static const char hex[] = "0123456789abcdef";
    unsigned char b[16];
    if (!randomBytes(b, sizeof(b))) {
        return false;
    }
    b[6] = static_cast<unsigned char>((b[6] & 0x0F) | 0x40);
    b[8] = static_cast<unsigned char>((b[8] & 0x3F) | 0x80);
    out.clear();
    for (int i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            out.push_back('-');
        }
        out.push_back(hex[b[i] >> 4]);
        out.push_back(hex[b[i] & 0x0F]);
    }
    return true;
}

bool isValidUtf8(const std::string &s)
{
    size_t i = 0;
    const size_t n = s.size();
    while (i < n) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        size_t extra;
        uint32_t cp;
        if (c < 0x80) {
            ++i;
            continue;
        } else if (c >= 0xC2 && c <= 0xDF) {
            extra = 1;
            cp = c & 0x1F;
        } else if (c >= 0xE0 && c <= 0xEF) {
            extra = 2;
            cp = c & 0x0F;
        } else if (c >= 0xF0 && c <= 0xF4) {
            extra = 3;
            cp = c & 0x07;
        } else {
            return false;
        }
        if (i + extra >= n) {
            return false;
        }
        for (size_t k = 1; k <= extra; ++k) {
            unsigned char cc = static_cast<unsigned char>(s[i + k]);
            if ((cc & 0xC0) != 0x80) {
                return false;
            }
            cp = (cp << 6) | (cc & 0x3F);
        }
        if ((extra == 2 && cp < 0x800) || (extra == 3 && cp < 0x10000) ||
            cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
            return false;
        }
        i += extra + 1;
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

void truncateUtf8(std::string &s, size_t maxBytes)
{
    if (s.size() <= maxBytes) {
        return;
    }
    size_t pos = maxBytes;
    while (pos > 0 && (static_cast<unsigned char>(s[pos]) & 0xC0) == 0x80) {
        --pos;
    }
    s.resize(pos);
}

/* Control characters become spaces/are dropped, ends trimmed, bytes bounded. */
std::string cleanText(const std::string &in, size_t maxBytes)
{
    if (!isValidUtf8(in)) {
        return std::string();
    }
    std::string out;
    for (size_t i = 0; i < in.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(in[i]);
        if (c == '\t' || c == '\n' || c == '\r') {
            out.push_back(' ');
        } else if (c >= 0x20 && c != 0x7F) {
            out.push_back(static_cast<char>(c));
        }
    }
    truncateUtf8(out, maxBytes);
    size_t b = out.find_first_not_of(' ');
    if (b == std::string::npos) {
        return std::string();
    }
    size_t e = out.find_last_not_of(' ');
    return out.substr(b, e - b + 1);
}

std::string handleBaseFromName(const std::string &name)
{
    std::string out;
    for (size_t i = 0; i < name.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(name[i]);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-') {
            out.push_back(static_cast<char>(c));
        } else if (c == ' ' || c == '.') {
            out.push_back('_');
        }
    }
    if (out.size() > static_cast<size_t>(AccountStore::kMaxHandleLength)) {
        out.resize(static_cast<size_t>(AccountStore::kMaxHandleLength));
    }
    return out;
}

bool handleTaken(sqlite3 *db, const std::string &handle, int64_t exceptPlayer)
{
    Stmt s(db, "SELECT 1 FROM players WHERE handle = ?1 COLLATE NOCASE"
               " AND player_id != ?2 LIMIT 1");
    s.bindText(1, handle);
    s.bindInt(2, exceptPlayer);
    return s.step() != SQLITE_DONE;
}

bool pickUniqueHandle(sqlite3 *db, const std::string &claimName,
                      std::string &handle)
{
    std::string base = handleBaseFromName(claimName);
    std::string suffix;
    for (int attempt = 0; attempt < 32; ++attempt) {
        std::string candidate;
        if (static_cast<int>(base.size()) < AccountStore::kMinHandleLength) {
            if (!randomHex(8, suffix)) {
                return false;
            }
            candidate = "Player-" + suffix;
        } else if (attempt == 0) {
            candidate = base;
        } else if (attempt < 16) {
            if (!randomHex(4, suffix)) {
                return false;
            }
            std::string trimmed = base.substr(
                0, static_cast<size_t>(AccountStore::kMaxHandleLength) - 5);
            candidate = trimmed + "-" + suffix;
        } else {
            if (!randomHex(8, suffix)) {
                return false;
            }
            candidate = "Player-" + suffix;
        }
        if (!handleTaken(db, candidate, 0)) {
            handle = candidate;
            return true;
        }
    }
    return false;
}

void readProfile(const Stmt &s, PlayerProfile &p)
{
    p.playerId = s.colInt(0);
    p.uuid = s.colText(1);
    p.handle = s.colText(2);
    p.displayName = s.colText(3);
    p.avatarUrl = s.colText(4);
    p.elo2p = static_cast<int>(s.colInt(5));
    p.elo4p = static_cast<int>(s.colInt(6));
    p.eloOfk = static_cast<int>(s.colInt(7));
    p.gamesPlayed = static_cast<int>(s.colInt(8));
    p.wins = static_cast<int>(s.colInt(9));
    p.losses = static_cast<int>(s.colInt(10));
    p.draws = static_cast<int>(s.colInt(11));
    p.createdAt = s.colInt(12);
    p.lastSeenAt = s.colInt(13);
}

bool loadProfileById(sqlite3 *db, int64_t playerId, PlayerProfile &out)
{
    std::string sql = std::string("SELECT ") + kProfileCols +
                      " FROM players WHERE player_id = ?1";
    Stmt s(db, sql.c_str());
    s.bindInt(1, playerId);
    if (s.step() != SQLITE_ROW) {
        return false;
    }
    readProfile(s, out);
    return true;
}

bool loadIdentities(sqlite3 *db, int64_t playerId,
                    std::vector<LinkedIdentity> &out)
{
    Stmt s(db, "SELECT provider, provider_sub, provider_email, display_hint,"
               " linked_at, last_login_at FROM linked_identities"
               " WHERE player_id = ?1 ORDER BY identity_id ASC");
    s.bindInt(1, playerId);
    out.clear();
    int rc;
    while ((rc = s.step()) == SQLITE_ROW) {
        LinkedIdentity li;
        li.provider = s.colText(0);
        li.providerSub = s.colText(1);
        li.providerEmail = s.colText(2);
        li.displayHint = s.colText(3);
        li.linkedAt = s.colInt(4);
        li.lastLoginAt = s.colInt(5);
        out.push_back(li);
    }
    return rc == SQLITE_DONE;
}

bool isValidMode(const std::string &mode)
{
    return mode == "2P" || mode == "4P" || mode == "OFK";
}

bool validClaims(const VerifiedClaims &c)
{
    return OidcVerifier::isSupportedProvider(c.provider) && !c.sub.empty() &&
           c.sub.size() <= kMaxSubBytes && isValidUtf8(c.sub);
}

bool validAvatarUrl(const std::string &url)
{
    if (url.empty()) {
        return true;
    }
    if (url.size() > kMaxAvatarUrlBytes ||
        url.compare(0, 8, "https://") != 0 || url.size() == 8) {
        return false;
    }
    for (size_t i = 0; i < url.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(url[i]);
        if (c <= 0x20 || c >= 0x7F) {
            return false;
        }
    }
    return true;
}

bool mintSessionLocked(sqlite3 *db, int64_t playerId,
                       const std::string &clientAgent, int64_t now,
                       int64_t ttl, std::string &token, int64_t &expiresAt,
                       std::string &error)
{
    unsigned char raw[32];
    if (!randomBytes(raw, sizeof(raw))) {
        error = "random generator failure";
        return false;
    }
    std::string t = base64UrlEncode(raw, sizeof(raw));
    OPENSSL_cleanse(raw, sizeof(raw));
    Stmt s(db, "INSERT INTO sessions(token_hash, player_id, client_agent,"
               " created_at, expires_at) VALUES(?1, ?2, ?3, ?4, ?5)");
    s.bindText(1, AccountStore::sha256Hex(t));
    s.bindInt(2, playerId);
    s.bindText(3, cleanText(clientAgent, kMaxAgentBytes));
    s.bindInt(4, now);
    s.bindInt(5, now + ttl);
    if (s.step() != SQLITE_DONE) {
        error = sqlite3_errmsg(db);
        return false;
    }
    token = t;
    expiresAt = now + ttl;
    return true;
}

bool applyRatingsLocked(sqlite3 *db, const std::string &mode,
                        const std::vector<RatingUpdate> &updates,
                        std::string &error)
{
    const char *sql = nullptr;
    if (mode == "2P") {
        sql = "UPDATE players SET elo_2p = ?1, games_played = games_played + 1,"
              " wins = wins + ?2, losses = losses + ?3, draws = draws + ?4"
              " WHERE player_id = ?5";
    } else if (mode == "4P") {
        sql = "UPDATE players SET elo_4p = ?1, games_played = games_played + 1,"
              " wins = wins + ?2, losses = losses + ?3, draws = draws + ?4"
              " WHERE player_id = ?5";
    } else if (mode == "OFK") {
        sql = "UPDATE players SET elo_ofk = ?1, games_played = games_played + 1,"
              " wins = wins + ?2, losses = losses + ?3, draws = draws + ?4"
              " WHERE player_id = ?5";
    } else {
        error = "invalid mode";
        return false;
    }
    for (size_t i = 0; i < updates.size(); ++i) {
        for (size_t j = i + 1; j < updates.size(); ++j) {
            if (updates[i].playerId == updates[j].playerId) {
                error = "duplicate player in rating updates";
                return false;
            }
        }
        if (updates[i].newElo < 0 || updates[i].newElo > kMaxElo) {
            error = "rating out of range";
            return false;
        }
    }
    for (size_t i = 0; i < updates.size(); ++i) {
        const RatingUpdate &u = updates[i];
        Stmt s(db, sql);
        s.bindInt(1, u.newElo);
        s.bindInt(2, u.outcome == GameOutcome::WIN ? 1 : 0);
        s.bindInt(3, u.outcome == GameOutcome::LOSS ? 1 : 0);
        s.bindInt(4, u.outcome == GameOutcome::DRAW ? 1 : 0);
        s.bindInt(5, u.playerId);
        if (s.step() != SQLITE_DONE) {
            error = sqlite3_errmsg(db);
            return false;
        }
        if (sqlite3_changes(db) != 1) {
            error = "unknown player in rating updates";
            return false;
        }
    }
    return true;
}

bool validTermination(const std::string &t)
{
    return t == "BASE_CAPTURE" || t == "BASE_OCCUPATION" || t == "TOTAL_CAPTURE" ||
           t == "ELIMINATION" || t == "STALEMATE" || t == "RESIGNATION" ||
           t == "TIMEOUT" || t == "DISCONNECT_FORFEIT" || t == "AGREED_DRAW";
}

bool validResult(const std::string &r)
{
    return r == "P1_WIN" || r == "P2_WIN" || r == "TEAM_13_WIN" ||
           r == "TEAM_24_WIN" || r == "DRAW";
}

bool validGameId(const std::string &id)
{
    if (id.empty() || id.size() > kMaxGameIdBytes) {
        return false;
    }
    for (size_t i = 0; i < id.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(id[i]);
        bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!ok) {
            return false;
        }
    }
    return true;
}

bool validateRecord(const GameRecord &g, std::string &error)
{
    if (!validGameId(g.gameId)) {
        error = "invalid game id";
    } else if (!isValidMode(g.gameMode)) {
        error = "invalid game mode";
    } else if (g.gameBases != 1 && g.gameBases != 3) {
        error = "invalid game bases";
    } else if (g.timeControl.empty() || g.timeControl.size() > 32 ||
               hasControlChars(g.timeControl) || !isValidUtf8(g.timeControl)) {
        error = "invalid time control";
    } else if (!validResult(g.result)) {
        error = "invalid result";
    } else if (g.winnerSeat < 0 || g.winnerSeat > 4) {
        error = "invalid winner seat";
    } else if (!validTermination(g.termination)) {
        error = "invalid termination";
    } else if (g.plyCount < 0) {
        error = "invalid ply count";
    } else if (g.octiTranscript.empty() ||
               g.octiTranscript.size() > kMaxTranscriptBytes ||
               !isValidUtf8(g.octiTranscript)) {
        error = "invalid transcript";
    } else if (g.finalFen.empty() || g.finalFen.size() > kMaxFenBytes ||
               hasControlChars(g.finalFen) || !isValidUtf8(g.finalFen)) {
        error = "invalid final fen";
    } else if (g.p1Handle.empty() || g.p2Handle.empty() ||
               g.p1Handle.size() > kMaxGameHandleBytes ||
               g.p2Handle.size() > kMaxGameHandleBytes ||
               g.p3Handle.size() > kMaxGameHandleBytes ||
               g.p4Handle.size() > kMaxGameHandleBytes ||
               !isValidUtf8(g.p1Handle) || !isValidUtf8(g.p2Handle) ||
               !isValidUtf8(g.p3Handle) || !isValidUtf8(g.p4Handle)) {
        error = "invalid player handle";
    } else if (g.p1PlayerId < 0 || g.p2PlayerId < 0 || g.p3PlayerId < 0 ||
               g.p4PlayerId < 0) {
        error = "invalid player id";
    } else if (g.startedAt < 0 || g.endedAt < g.startedAt) {
        error = "invalid timestamps";
    } else if (!g.source.empty() && g.source != "online" && g.source != "client_upload") {
        error = "invalid source";
    } else if (!g.winner.empty() && g.winner != "A" && g.winner != "B" && g.winner != "C" && g.winner != "D" && g.winner != "NONE") {
        error = "invalid winner";
    } else if (!g.status.empty() && g.status != "GAME_OVER" && g.status != "RESIGNED") {
        error = "invalid status";
    } else if (g.aiDifficulty < 0 || g.aiDifficulty > 5) {
        error = "invalid ai difficulty";
    } else if (g.appVersion.size() > 128 || hasControlChars(g.appVersion)) {
        error = "invalid app version";
    } else if (g.platform.size() > 128 || hasControlChars(g.platform)) {
        error = "invalid platform";
    } else if (g.installId.size() > 128 || hasControlChars(g.installId)) {
        error = "invalid install id";
    } else {
        return true;
    }
    return false;
}

void bindOptionalPlayer(Stmt &s, int idx, int64_t id)
{
    if (id > 0) {
        s.bindInt(idx, id);
    } else {
        s.bindNull(idx);
    }
}

ArchiveStatus archiveLocked(sqlite3 *db, const GameRecord &g,
                            std::string &error)
{
    {
        Stmt dup(db, "SELECT 1 FROM games WHERE game_id = ?1");
        dup.bindText(1, g.gameId);
        int rc = dup.step();
        if (rc == SQLITE_ROW) {
            error = "duplicate game id";
            return ArchiveStatus::DUPLICATE_ID;
        }
        if (rc != SQLITE_DONE) {
            error = sqlite3_errmsg(db);
            return ArchiveStatus::ERROR;
        }
    }
    Stmt s(db, "INSERT INTO games(game_id, game_mode, game_bases, super_prongs,"
               " edgeless, time_control, p1_player_id, p2_player_id,"
               " p3_player_id, p4_player_id, p1_handle, p2_handle, p3_handle,"
               " p4_handle, result, winner_seat, termination, ply_count,"
               " octi_transcript, transcript_json, final_fen, started_at, ended_at,"
               " source, uploaded_at, winner, status, ai_difficulty, app_version, platform, install_id)"
               " VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12,"
               " ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21, ?22, ?23, ?24, ?25, ?26, ?27, ?28, ?29, ?30, ?31)");
    s.bindText(1, g.gameId);
    s.bindText(2, g.gameMode);
    s.bindInt(3, g.gameBases);
    s.bindInt(4, g.superProngs ? 1 : 0);
    s.bindInt(5, g.edgeless ? 1 : 0);
    s.bindText(6, g.timeControl);
    bindOptionalPlayer(s, 7, g.p1PlayerId);
    bindOptionalPlayer(s, 8, g.p2PlayerId);
    bindOptionalPlayer(s, 9, g.p3PlayerId);
    bindOptionalPlayer(s, 10, g.p4PlayerId);
    s.bindText(11, g.p1Handle);
    s.bindText(12, g.p2Handle);
    s.bindText(13, g.p3Handle);
    s.bindText(14, g.p4Handle);
    s.bindText(15, g.result);
    s.bindInt(16, g.winnerSeat);
    s.bindText(17, g.termination);
    s.bindInt(18, g.plyCount);
    s.bindText(19, g.octiTranscript);
    s.bindText(20, g.octiTranscript);
    s.bindText(21, g.finalFen);
    s.bindInt(22, g.startedAt);
    s.bindInt(23, g.endedAt);
    s.bindText(24, g.source.empty() ? "online" : g.source);
    s.bindInt(25, g.uploadedAt);
    s.bindText(26, g.winner.empty() ? "NONE" : g.winner);
    s.bindText(27, g.status.empty() ? "GAME_OVER" : g.status);
    s.bindInt(28, g.aiDifficulty);
    s.bindText(29, g.appVersion);
    s.bindText(30, g.platform);
    s.bindText(31, g.installId);
    int rc = s.step();
    if (rc == SQLITE_DONE) {
        return ArchiveStatus::OK;
    }
    error = sqlite3_errmsg(db);
    if ((rc & 0xFF) == SQLITE_CONSTRAINT) {
        return ArchiveStatus::INVALID;
    }
    return ArchiveStatus::ERROR;
}

void readGame(const Stmt &s, GameRecord &g)
{
    g.gameId = s.colText(0);
    g.gameMode = s.colText(1);
    g.gameBases = static_cast<int>(s.colInt(2));
    g.superProngs = s.colInt(3) != 0;
    g.edgeless = s.colInt(4) != 0;
    g.timeControl = s.colText(5);
    g.p1PlayerId = s.colInt(6);
    g.p2PlayerId = s.colInt(7);
    g.p3PlayerId = s.colInt(8);
    g.p4PlayerId = s.colInt(9);
    g.p1Handle = s.colText(10);
    g.p2Handle = s.colText(11);
    g.p3Handle = s.colText(12);
    g.p4Handle = s.colText(13);
    g.result = s.colText(14);
    g.winnerSeat = static_cast<int>(s.colInt(15));
    g.termination = s.colText(16);
    g.plyCount = static_cast<int>(s.colInt(17));
    g.octiTranscript = s.colText(18);
    g.finalFen = s.colText(19);
    g.startedAt = s.colInt(20);
    g.endedAt = s.colInt(21);
    g.source = s.colText(22);
    g.uploadedAt = s.colInt(23);
    g.winner = s.colText(24);
    g.status = s.colText(25);
    g.aiDifficulty = static_cast<int>(s.colInt(26));
    g.appVersion = s.colText(27);
    g.platform = s.colText(28);
    g.installId = s.colText(29);
}

bool tightenPermissions(const std::string &path, std::string &error)
{
    namespace fs = std::filesystem;
    size_t slash = path.find_last_of('/');
    if (slash != std::string::npos && slash > 0) {
        std::string dir = path.substr(0, slash);
        struct stat st;
        if (::stat(dir.c_str(), &st) != 0) {
            std::error_code ec;
            fs::create_directories(dir, ec);
            if (ec || ::chmod(dir.c_str(), 0700) != 0) {
                error = "cannot create database directory";
                return false;
            }
        } else if (!S_ISDIR(st.st_mode)) {
            error = "database parent is not a directory";
            return false;
        } else if (st.st_uid == ::geteuid() && !(st.st_mode & S_ISVTX)) {
            if ((st.st_mode & 0077) != 0 &&
                ::chmod(dir.c_str(), st.st_mode & 0700) != 0) {
                error = "cannot restrict database directory";
                return false;
            }
        }
    }
    int fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (fd < 0) {
        error = "cannot create database file";
        return false;
    }
    bool ok = ::fchmod(fd, 0600) == 0;
    ::close(fd);
    if (!ok) {
        error = "cannot restrict database file";
        return false;
    }
    return true;
}

} // namespace

/* ------------------------------------------------------------------ */

AccountStore::AccountStore()
    : _db(nullptr), _sessionTtlSec(kDefaultSessionTtlSec)
{
    _clock = []() -> int64_t {
        return std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    };
}

AccountStore::~AccountStore()
{
    close();
}

bool AccountStore::isValidHandle(const std::string &handle)
{
    if (handle.size() < static_cast<size_t>(kMinHandleLength) ||
        handle.size() > static_cast<size_t>(kMaxHandleLength)) {
        return false;
    }
    for (size_t i = 0; i < handle.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(handle[i]);
        bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == '-';
        if (!ok) {
            return false;
        }
    }
    return true;
}

std::string AccountStore::sha256Hex(const std::string &data)
{
    static const char hex[] = "0123456789abcdef";
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int mdLen = 0;
    if (EVP_Digest(data.data(), data.size(), md, &mdLen, EVP_sha256(),
                   nullptr) != 1) {
        return std::string();
    }
    std::string out;
    for (unsigned int i = 0; i < mdLen; ++i) {
        out.push_back(hex[md[i] >> 4]);
        out.push_back(hex[md[i] & 0x0F]);
    }
    return out;
}

int64_t AccountStore::nowSec() const
{
    return _clock();
}

void AccountStore::setClock(std::function<int64_t()> clock)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (clock) {
        _clock = clock;
    } else {
        _clock = []() -> int64_t {
            return std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                .count();
        };
    }
}

void AccountStore::setSessionTtlSec(int64_t seconds)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _sessionTtlSec = seconds < 1 ? 1 : seconds;
}

bool AccountStore::isOpen() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _db != nullptr;
}

bool AccountStore::openOrCreate(const std::string &path, std::string &error)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_db) {
        error = "database already open";
        return false;
    }
    if (path.empty()) {
        error = "empty database path";
        return false;
    }
    if (!tightenPermissions(path, error)) {
        return false;
    }
    sqlite3 *db = nullptr;
    int rc = sqlite3_open_v2(path.c_str(), &db,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                                 SQLITE_OPEN_FULLMUTEX,
                             nullptr);
    if (rc != SQLITE_OK) {
        error = db ? sqlite3_errmsg(db) : "sqlite3_open_v2 failed";
        sqlite3_close(db);
        return false;
    }
    sqlite3_extended_result_codes(db, 1);
    sqlite3_busy_timeout(db, 5000);

    bool ok = true;
    {
        Stmt wal(db, "PRAGMA journal_mode=WAL");
        ok = wal.step() == SQLITE_ROW && wal.colText(0) == "wal";
        if (!ok) {
            error = "cannot enable WAL journal mode";
        }
    }
    if (ok) {
        ok = execSql(db, "PRAGMA foreign_keys=ON") &&
             execSql(db, "PRAGMA synchronous=NORMAL");
        if (ok) {
            Stmt fk(db, "PRAGMA foreign_keys");
            ok = fk.step() == SQLITE_ROW && fk.colInt(0) == 1;
        }
        if (!ok) {
            error = "cannot enable foreign keys";
        }
    }
    if (ok) {
        ok = execSql(db, kSchemaSql);
        if (!ok) {
            error = sqlite3_errmsg(db);
        } else {
            std::set<std::string> cols;
            Stmt info(db, "PRAGMA table_info(games)");
            while (info.step() == SQLITE_ROW) {
                cols.insert(info.colText(1));
            }
            if (!cols.empty()) {
                if (cols.find("transcript_json") == cols.end()) {
                    execSql(db, "ALTER TABLE games ADD COLUMN transcript_json TEXT NOT NULL DEFAULT '';");
                    execSql(db, "UPDATE games SET transcript_json = octi_transcript WHERE transcript_json = '';");
                }
                if (cols.find("source") == cols.end()) {
                    execSql(db, "ALTER TABLE games ADD COLUMN source TEXT NOT NULL DEFAULT 'online';");
                }
                if (cols.find("uploaded_at") == cols.end()) {
                    execSql(db, "ALTER TABLE games ADD COLUMN uploaded_at INTEGER NOT NULL DEFAULT 0;");
                }
                if (cols.find("winner") == cols.end()) {
                    execSql(db, "ALTER TABLE games ADD COLUMN winner TEXT NOT NULL DEFAULT 'NONE';");
                }
                if (cols.find("status") == cols.end()) {
                    execSql(db, "ALTER TABLE games ADD COLUMN status TEXT NOT NULL DEFAULT 'GAME_OVER';");
                }
                if (cols.find("ai_difficulty") == cols.end()) {
                    execSql(db, "ALTER TABLE games ADD COLUMN ai_difficulty INTEGER NOT NULL DEFAULT 0;");
                }
                if (cols.find("app_version") == cols.end()) {
                    execSql(db, "ALTER TABLE games ADD COLUMN app_version TEXT NOT NULL DEFAULT '';");
                }
                if (cols.find("platform") == cols.end()) {
                    execSql(db, "ALTER TABLE games ADD COLUMN platform TEXT NOT NULL DEFAULT '';");
                }
                if (cols.find("install_id") == cols.end()) {
                    execSql(db, "ALTER TABLE games ADD COLUMN install_id TEXT NOT NULL DEFAULT '';");
                }
                execSql(db, "CREATE INDEX IF NOT EXISTS idx_games_uploaded_at ON games(uploaded_at DESC);");
                execSql(db, "CREATE INDEX IF NOT EXISTS idx_games_source ON games(source);");
                execSql(db, "CREATE INDEX IF NOT EXISTS idx_games_install_id ON games(install_id);");
            }
        }
    }
    if (!ok) {
        sqlite3_close(db);
        return false;
    }
    /* Sidecar files inherit the main file mode; enforce it explicitly. */
    ::chmod((path + "-wal").c_str(), 0600);
    ::chmod((path + "-shm").c_str(), 0600);
    _db = db;
    return true;
}

void AccountStore::close()
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_db) {
        sqlite3_close(_db);
        _db = nullptr;
    }
}

LoginResult AccountStore::loginWithSso(const VerifiedClaims &claims,
                                       const std::string &clientAgent)
{
    LoginResult res;
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        res.error = "database not open";
        return res;
    }
    if (!validClaims(claims)) {
        res.error = "invalid claims";
        return res;
    }
    const int64_t now = nowSec();
    const std::string email = cleanText(claims.email, kMaxEmailBytes);
    const std::string hint = cleanText(claims.name, kMaxHintBytes);

    Tx tx(_db);
    if (!tx.begun()) {
        res.error = sqlite3_errmsg(_db);
        return res;
    }

    int64_t playerId = 0;
    {
        Stmt find(_db, "SELECT identity_id, player_id FROM linked_identities"
                       " WHERE provider = ?1 AND provider_sub = ?2");
        find.bindText(1, claims.provider);
        find.bindText(2, claims.sub);
        int rc = find.step();
        if (rc == SQLITE_ROW) {
            int64_t identityId = find.colInt(0);
            playerId = find.colInt(1);
            Stmt upd(_db, "UPDATE linked_identities SET last_login_at = ?1,"
                          " provider_email = COALESCE(NULLIF(?2, ''),"
                          " provider_email) WHERE identity_id = ?3");
            upd.bindInt(1, now);
            upd.bindText(2, email);
            upd.bindInt(3, identityId);
            if (upd.step() != SQLITE_DONE) {
                res.error = sqlite3_errmsg(_db);
                return res;
            }
            Stmt seen(_db, "UPDATE players SET last_seen_at = ?1"
                           " WHERE player_id = ?2");
            seen.bindInt(1, now);
            seen.bindInt(2, playerId);
            if (seen.step() != SQLITE_DONE) {
                res.error = sqlite3_errmsg(_db);
                return res;
            }
        } else if (rc == SQLITE_DONE) {
            std::string uuid;
            std::string handle;
            if (!uuidV4(uuid) || !pickUniqueHandle(_db, claims.name, handle)) {
                res.error = "cannot allocate player identity";
                return res;
            }
            std::string displayName = cleanText(claims.name, kMaxDisplayNameBytes);
            if (displayName.empty()) {
                displayName = handle;
            }
            std::string avatar = claims.picture;
            if (!validAvatarUrl(avatar)) {
                avatar.clear();
            }
            Stmt ins(_db, "INSERT INTO players(uuid, handle, display_name,"
                          " avatar_url, created_at, last_seen_at)"
                          " VALUES(?1, ?2, ?3, ?4, ?5, ?5)");
            ins.bindText(1, uuid);
            ins.bindText(2, handle);
            ins.bindText(3, displayName);
            ins.bindText(4, avatar);
            ins.bindInt(5, now);
            if (ins.step() != SQLITE_DONE) {
                res.error = sqlite3_errmsg(_db);
                return res;
            }
            playerId = sqlite3_last_insert_rowid(_db);
            Stmt link(_db, "INSERT INTO linked_identities(player_id, provider,"
                           " provider_sub, provider_email, display_hint,"
                           " linked_at, last_login_at)"
                           " VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?6)");
            link.bindInt(1, playerId);
            link.bindText(2, claims.provider);
            link.bindText(3, claims.sub);
            link.bindText(4, email);
            link.bindText(5, hint);
            link.bindInt(6, now);
            if (link.step() != SQLITE_DONE) {
                res.error = sqlite3_errmsg(_db);
                return res;
            }
            res.created = true;
        } else {
            res.error = sqlite3_errmsg(_db);
            return res;
        }
    }

    if (!mintSessionLocked(_db, playerId, clientAgent, now, _sessionTtlSec,
                           res.sessionToken, res.sessionExpiresAt, res.error)) {
        res.sessionToken.clear();
        return res;
    }
    if (!loadProfileById(_db, playerId, res.profile) ||
        !loadIdentities(_db, playerId, res.identities)) {
        res.sessionToken.clear();
        res.error = "cannot load profile";
        return res;
    }
    if (!tx.commit()) {
        res.sessionToken.clear();
        res.error = sqlite3_errmsg(_db);
        return res;
    }
    res.ok = true;
    return res;
}

LinkResult AccountStore::linkIdentity(int64_t playerId,
                                      const VerifiedClaims &claims)
{
    LinkResult res;
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        res.error = "database not open";
        return res;
    }
    if (!validClaims(claims)) {
        res.status = LinkStatus::INVALID_CLAIMS;
        res.error = "invalid claims";
        return res;
    }
    const int64_t now = nowSec();
    const std::string email = cleanText(claims.email, kMaxEmailBytes);
    const std::string hint = cleanText(claims.name, kMaxHintBytes);

    Tx tx(_db);
    if (!tx.begun()) {
        res.error = sqlite3_errmsg(_db);
        return res;
    }
    PlayerProfile self;
    if (!loadProfileById(_db, playerId, self)) {
        res.status = LinkStatus::PLAYER_NOT_FOUND;
        res.error = "player not found";
        return res;
    }

    int64_t ownerId = 0;
    int64_t identityId = 0;
    {
        Stmt find(_db, "SELECT identity_id, player_id FROM linked_identities"
                       " WHERE provider = ?1 AND provider_sub = ?2");
        find.bindText(1, claims.provider);
        find.bindText(2, claims.sub);
        int rc = find.step();
        if (rc == SQLITE_ROW) {
            identityId = find.colInt(0);
            ownerId = find.colInt(1);
        } else if (rc != SQLITE_DONE) {
            res.error = sqlite3_errmsg(_db);
            return res;
        }
    }

    if (ownerId != 0 && ownerId != playerId) {
        PlayerProfile other;
        if (!loadProfileById(_db, ownerId, other)) {
            res.error = "cannot load conflicting profile";
            return res;
        }
        res.status = LinkStatus::CONFLICT_ALREADY_LINKED;
        res.conflictProfile.uuid = other.uuid;
        res.conflictProfile.handle = other.handle;
        res.conflictProfile.elo2p = other.elo2p;
        res.conflictProfile.gamesPlayed = other.gamesPlayed;
        return res;
    }

    if (ownerId == playerId) {
        Stmt upd(_db, "UPDATE linked_identities SET last_login_at = ?1,"
                      " provider_email = COALESCE(NULLIF(?2, ''),"
                      " provider_email), display_hint = COALESCE(NULLIF(?3, ''),"
                      " display_hint) WHERE identity_id = ?4");
        upd.bindInt(1, now);
        upd.bindText(2, email);
        upd.bindText(3, hint);
        upd.bindInt(4, identityId);
        if (upd.step() != SQLITE_DONE || !tx.commit()) {
            res.error = sqlite3_errmsg(_db);
            return res;
        }
        res.status = LinkStatus::ALREADY_LINKED_SELF;
        return res;
    }

    Stmt ins(_db, "INSERT INTO linked_identities(player_id, provider,"
                  " provider_sub, provider_email, display_hint, linked_at,"
                  " last_login_at) VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?6)");
    ins.bindInt(1, playerId);
    ins.bindText(2, claims.provider);
    ins.bindText(3, claims.sub);
    ins.bindText(4, email);
    ins.bindText(5, hint);
    ins.bindInt(6, now);
    if (ins.step() != SQLITE_DONE || !tx.commit()) {
        res.error = sqlite3_errmsg(_db);
        return res;
    }
    res.status = LinkStatus::LINKED;
    return res;
}

UnlinkStatus AccountStore::unlinkIdentity(int64_t playerId,
                                          const std::string &provider)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        return UnlinkStatus::ERROR;
    }
    Tx tx(_db);
    if (!tx.begun()) {
        return UnlinkStatus::ERROR;
    }
    int64_t matching = 0;
    int64_t total = 0;
    {
        Stmt m(_db, "SELECT COUNT(*) FROM linked_identities"
                    " WHERE player_id = ?1 AND provider = ?2");
        m.bindInt(1, playerId);
        m.bindText(2, provider);
        if (m.step() != SQLITE_ROW) {
            return UnlinkStatus::ERROR;
        }
        matching = m.colInt(0);
    }
    {
        Stmt t(_db, "SELECT COUNT(*) FROM linked_identities"
                    " WHERE player_id = ?1");
        t.bindInt(1, playerId);
        if (t.step() != SQLITE_ROW) {
            return UnlinkStatus::ERROR;
        }
        total = t.colInt(0);
    }
    if (matching == 0) {
        return UnlinkStatus::NOT_LINKED;
    }
    if (total - matching <= 0) {
        return UnlinkStatus::LAST_IDENTITY_FORBIDDEN;
    }
    Stmt del(_db, "DELETE FROM linked_identities"
                  " WHERE player_id = ?1 AND provider = ?2");
    del.bindInt(1, playerId);
    del.bindText(2, provider);
    if (del.step() != SQLITE_DONE || !tx.commit()) {
        return UnlinkStatus::ERROR;
    }
    return UnlinkStatus::UNLINKED;
}

bool AccountStore::listLinkedIdentities(int64_t playerId,
                                        std::vector<LinkedIdentity> &out)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        return false;
    }
    return loadIdentities(_db, playerId, out);
}

bool AccountStore::mintSession(int64_t playerId, const std::string &clientAgent,
                               std::string &token, int64_t &expiresAt,
                               std::string &error)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        error = "database not open";
        return false;
    }
    PlayerProfile p;
    if (!loadProfileById(_db, playerId, p)) {
        error = "player not found";
        return false;
    }
    return mintSessionLocked(_db, playerId, clientAgent, nowSec(),
                             _sessionTtlSec, token, expiresAt, error);
}

SessionValidation AccountStore::validateSession(const std::string &token)
{
    int64_t now;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        now = nowSec();
    }
    return validateSession(token, now);
}

SessionValidation AccountStore::validateSession(const std::string &token,
                                                int64_t now)
{
    SessionValidation v;
    bool wellFormed = !token.empty() && token.size() <= kMaxTokenBytes;
    for (size_t i = 0; wellFormed && i < token.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(token[i]);
        wellFormed = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') || c == '-' || c == '_';
    }
    if (!wellFormed) {
        v.reason = "malformed";
        return v;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        v.reason = "unknown";
        return v;
    }
    Stmt s(_db, "SELECT player_id, expires_at, revoked FROM sessions"
                " WHERE token_hash = ?1");
    s.bindText(1, sha256Hex(token));
    if (s.step() != SQLITE_ROW) {
        v.reason = "unknown";
        return v;
    }
    int64_t expiresAt = s.colInt(1);
    if (s.colInt(2) != 0) {
        v.reason = "revoked";
    } else if (now >= expiresAt) {
        v.reason = "expired";
    } else {
        v.valid = true;
        v.playerId = s.colInt(0);
        v.expiresAt = expiresAt;
    }
    return v;
}

bool AccountStore::revokeSession(const std::string &token)
{
    if (token.empty() || token.size() > kMaxTokenBytes) {
        return false;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        return false;
    }
    Stmt s(_db, "UPDATE sessions SET revoked = 1 WHERE token_hash = ?1");
    s.bindText(1, sha256Hex(token));
    if (s.step() != SQLITE_DONE) {
        return false;
    }
    return sqlite3_changes(_db) == 1;
}

int AccountStore::revokeAllSessions(int64_t playerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        return -1;
    }
    Stmt s(_db, "UPDATE sessions SET revoked = 1"
                " WHERE player_id = ?1 AND revoked = 0");
    s.bindInt(1, playerId);
    if (s.step() != SQLITE_DONE) {
        return -1;
    }
    return sqlite3_changes(_db);
}

int AccountStore::countActiveSessions(int64_t playerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        return -1;
    }
    Stmt s(_db, "SELECT COUNT(*) FROM sessions"
                " WHERE player_id = ?1 AND revoked = 0 AND expires_at > ?2");
    s.bindInt(1, playerId);
    s.bindInt(2, nowSec());
    if (s.step() != SQLITE_ROW) {
        return -1;
    }
    return static_cast<int>(s.colInt(0));
}

int AccountStore::purgeExpiredSessions(int64_t now)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        return -1;
    }
    Stmt s(_db, "DELETE FROM sessions WHERE revoked = 1 OR expires_at <= ?1");
    s.bindInt(1, now);
    if (s.step() != SQLITE_DONE) {
        return -1;
    }
    return sqlite3_changes(_db);
}

bool AccountStore::getProfileById(int64_t playerId, PlayerProfile &out)
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _db && loadProfileById(_db, playerId, out);
}

bool AccountStore::getProfileByUuid(const std::string &uuid, PlayerProfile &out)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        return false;
    }
    std::string sql = std::string("SELECT ") + kProfileCols +
                      " FROM players WHERE uuid = ?1";
    Stmt s(_db, sql.c_str());
    s.bindText(1, uuid);
    if (s.step() != SQLITE_ROW) {
        return false;
    }
    readProfile(s, out);
    return true;
}

UpdateStatus AccountStore::updateProfile(int64_t playerId,
                                         const ProfileUpdate &update)
{
    if (update.setHandle && !isValidHandle(update.handle)) {
        return UpdateStatus::INVALID_HANDLE;
    }
    std::string displayName;
    if (update.setDisplayName) {
        displayName = update.displayName;
        if (displayName.empty() || displayName.size() > kMaxDisplayNameBytes ||
            hasControlChars(displayName) || !isValidUtf8(displayName) ||
            displayName.find_first_not_of(' ') == std::string::npos) {
            return UpdateStatus::INVALID_DISPLAY_NAME;
        }
    }
    if (update.setAvatarUrl && !validAvatarUrl(update.avatarUrl)) {
        return UpdateStatus::INVALID_AVATAR_URL;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        return UpdateStatus::ERROR;
    }
    Tx tx(_db);
    if (!tx.begun()) {
        return UpdateStatus::ERROR;
    }
    PlayerProfile current;
    if (!loadProfileById(_db, playerId, current)) {
        return UpdateStatus::NOT_FOUND;
    }
    if (update.setHandle && handleTaken(_db, update.handle, playerId)) {
        return UpdateStatus::HANDLE_TAKEN;
    }
    Stmt s(_db, "UPDATE players SET handle = ?1, display_name = ?2,"
                " avatar_url = ?3 WHERE player_id = ?4");
    s.bindText(1, update.setHandle ? update.handle : current.handle);
    s.bindText(2, update.setDisplayName ? displayName : current.displayName);
    s.bindText(3, update.setAvatarUrl ? update.avatarUrl : current.avatarUrl);
    s.bindInt(4, playerId);
    int rc = s.step();
    if (rc != SQLITE_DONE) {
        return (rc & 0xFF) == SQLITE_CONSTRAINT ? UpdateStatus::HANDLE_TAKEN
                                                : UpdateStatus::ERROR;
    }
    return tx.commit() ? UpdateStatus::OK : UpdateStatus::ERROR;
}

bool AccountStore::leaderboard(const std::string &mode, int limit,
                               std::vector<LeaderboardEntry> &out)
{
    const char *sql = nullptr;
    if (mode == "2P") {
        sql = "SELECT uuid, handle, display_name, elo_2p, games_played, wins,"
              " losses, draws FROM players ORDER BY elo_2p DESC,"
              " games_played DESC, player_id ASC LIMIT ?1";
    } else if (mode == "4P") {
        sql = "SELECT uuid, handle, display_name, elo_4p, games_played, wins,"
              " losses, draws FROM players ORDER BY elo_4p DESC,"
              " games_played DESC, player_id ASC LIMIT ?1";
    } else if (mode == "OFK") {
        sql = "SELECT uuid, handle, display_name, elo_ofk, games_played, wins,"
              " losses, draws FROM players ORDER BY elo_ofk DESC,"
              " games_played DESC, player_id ASC LIMIT ?1";
    } else {
        return false;
    }
    if (limit < 1) {
        limit = 1;
    }
    if (limit > kMaxLeaderboardLimit) {
        limit = kMaxLeaderboardLimit;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        return false;
    }
    Stmt s(_db, sql);
    s.bindInt(1, limit);
    out.clear();
    int rc;
    int rank = 0;
    while ((rc = s.step()) == SQLITE_ROW) {
        LeaderboardEntry e;
        e.rank = ++rank;
        e.uuid = s.colText(0);
        e.handle = s.colText(1);
        e.displayName = s.colText(2);
        e.elo = static_cast<int>(s.colInt(3));
        e.gamesPlayed = static_cast<int>(s.colInt(4));
        e.wins = static_cast<int>(s.colInt(5));
        e.losses = static_cast<int>(s.colInt(6));
        e.draws = static_cast<int>(s.colInt(7));
        out.push_back(e);
    }
    return rc == SQLITE_DONE;
}

bool AccountStore::applyRatingUpdates(const std::string &mode,
                                      const std::vector<RatingUpdate> &updates,
                                      std::string &error)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        error = "database not open";
        return false;
    }
    Tx tx(_db);
    if (!tx.begun()) {
        error = sqlite3_errmsg(_db);
        return false;
    }
    if (!applyRatingsLocked(_db, mode, updates, error)) {
        return false;
    }
    if (!tx.commit()) {
        error = sqlite3_errmsg(_db);
        return false;
    }
    return true;
}

ArchiveStatus AccountStore::archiveGame(const GameRecord &record,
                                        std::string &error)
{
    if (!validateRecord(record, error)) {
        return ArchiveStatus::INVALID;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        error = "database not open";
        return ArchiveStatus::ERROR;
    }
    Tx tx(_db);
    if (!tx.begun()) {
        error = sqlite3_errmsg(_db);
        return ArchiveStatus::ERROR;
    }
    ArchiveStatus st = archiveLocked(_db, record, error);
    if (st != ArchiveStatus::OK) {
        return st;
    }
    if (!tx.commit()) {
        error = sqlite3_errmsg(_db);
        return ArchiveStatus::ERROR;
    }
    return ArchiveStatus::OK;
}

ArchiveStatus
AccountStore::archiveGameWithRatings(const GameRecord &record,
                                     const std::vector<RatingUpdate> &updates,
                                     std::string &error)
{
    if (!validateRecord(record, error)) {
        return ArchiveStatus::INVALID;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        error = "database not open";
        return ArchiveStatus::ERROR;
    }
    Tx tx(_db);
    if (!tx.begun()) {
        error = sqlite3_errmsg(_db);
        return ArchiveStatus::ERROR;
    }
    ArchiveStatus st = archiveLocked(_db, record, error);
    if (st != ArchiveStatus::OK) {
        return st;
    }
    if (!applyRatingsLocked(_db, record.gameMode, updates, error)) {
        return ArchiveStatus::INVALID;
    }
    if (!tx.commit()) {
        error = sqlite3_errmsg(_db);
        return ArchiveStatus::ERROR;
    }
    return ArchiveStatus::OK;
}

bool AccountStore::getArchivedGame(const std::string &gameId, GameRecord &out)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        return false;
    }
    std::string sql = std::string("SELECT ") + kGameColsFull +
                      " FROM games WHERE game_id = ?1";
    Stmt s(_db, sql.c_str());
    s.bindText(1, gameId);
    if (s.step() != SQLITE_ROW) {
        return false;
    }
    readGame(s, out);
    try {
        auto j = nlohmann::json::parse(out.octiTranscript);
        if (!j.is_object() || j.value("format", "") != "octi.transcript.v3") {
            return false;
        }
    } catch (...) {
        return false;
    }
    return true;
}

bool AccountStore::getArchivedTranscript(const std::string &gameId,
                                         std::string &transcript)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        return false;
    }
    Stmt s(_db, "SELECT COALESCE(NULLIF(transcript_json, ''), octi_transcript) FROM games WHERE game_id = ?1");
    s.bindText(1, gameId);
    if (s.step() != SQLITE_ROW) {
        return false;
    }
    transcript = s.colText(0);
    try {
        auto j = nlohmann::json::parse(transcript);
        if (!j.is_object() || j.value("format", "") != "octi.transcript.v3") {
            return false;
        }
    } catch (...) {
        return false;
    }
    return true;
}

bool AccountStore::listArchivedGames(const ArchiveFilter &filter,
                                     std::vector<GameRecord> &out,
                                     int64_t *total)
{
    out.clear();
    if (total) {
        *total = 0;
    }
    if (!filter.mode.empty() && !isValidMode(filter.mode)) {
        return false;
    }
    if (!filter.source.empty() && filter.source != "online" && filter.source != "client_upload") {
        return false;
    }
    if (!filter.status.empty() && filter.status != "GAME_OVER" && filter.status != "RESIGNED") {
        return false;
    }
    if (!filter.winner.empty() && filter.winner != "A" && filter.winner != "B" && filter.winner != "C" && filter.winner != "D" && filter.winner != "NONE") {
        return false;
    }
    if (filter.aiDifficulty < 0 || filter.aiDifficulty > 5) {
        return false;
    }
    if (!filter.sortField.empty() && filter.sortField != "ended_at" && filter.sortField != "uploaded_at" && filter.sortField != "ply_count") {
        return false;
    }

    int maxLimit = filter.maxLimit > 0 ? std::min(filter.maxLimit, kMaxExportLimit) : kMaxArchiveLimit;
    int limit = filter.limit;
    if (limit < 1) {
        limit = 1;
    }
    if (limit > maxLimit) {
        limit = maxLimit;
    }
    int offset = filter.offset;
    if (offset < 0) {
        offset = 0;
    }
    if (offset > kMaxOffset) {
        offset = kMaxOffset;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        return false;
    }
    int64_t playerId = 0;
    if (!filter.playerUuid.empty()) {
        Stmt p(_db, "SELECT player_id FROM players WHERE uuid = ?1");
        p.bindText(1, filter.playerUuid);
        int rc = p.step();
        if (rc == SQLITE_DONE) {
            return true;    // unknown player: empty page
        }
        if (rc != SQLITE_ROW) {
            return false;
        }
        playerId = p.colInt(0);
    }

    // Build parameterized WHERE clause
    std::string where = " WHERE 1=1";
    std::vector<std::pair<int, std::string>> textParams;
    std::vector<std::pair<int, int64_t>> intParams;
    int paramIdx = 1;

    if (!filter.mode.empty()) {
        where += " AND game_mode = ?" + std::to_string(paramIdx);
        textParams.push_back({paramIdx++, filter.mode});
    }
    if (playerId != 0) {
        where += " AND (p1_player_id = ?" + std::to_string(paramIdx) +
                 " OR p2_player_id = ?" + std::to_string(paramIdx) +
                 " OR p3_player_id = ?" + std::to_string(paramIdx) +
                 " OR p4_player_id = ?" + std::to_string(paramIdx) + ")";
        intParams.push_back({paramIdx++, playerId});
    }
    if (!filter.source.empty()) {
        where += " AND source = ?" + std::to_string(paramIdx);
        textParams.push_back({paramIdx++, filter.source});
    }
    if (!filter.status.empty()) {
        where += " AND status = ?" + std::to_string(paramIdx);
        textParams.push_back({paramIdx++, filter.status});
    }
    if (!filter.winner.empty()) {
        where += " AND winner = ?" + std::to_string(paramIdx);
        textParams.push_back({paramIdx++, filter.winner});
    }
    if (filter.aiDifficulty > 0) {
        where += " AND ai_difficulty = ?" + std::to_string(paramIdx);
        intParams.push_back({paramIdx++, filter.aiDifficulty});
    }
    if (!filter.appVersion.empty()) {
        where += " AND app_version = ?" + std::to_string(paramIdx);
        textParams.push_back({paramIdx++, filter.appVersion});
    }
    if (!filter.platform.empty()) {
        where += " AND platform = ?" + std::to_string(paramIdx);
        textParams.push_back({paramIdx++, filter.platform});
    }
    if (filter.fromSec > 0) {
        where += " AND COALESCE(NULLIF(uploaded_at, 0), ended_at) >= ?" + std::to_string(paramIdx);
        intParams.push_back({paramIdx++, filter.fromSec});
    }
    if (filter.toSec > 0) {
        where += " AND COALESCE(NULLIF(uploaded_at, 0), ended_at) <= ?" + std::to_string(paramIdx);
        intParams.push_back({paramIdx++, filter.toSec});
    }

    if (total) {
        std::string csql = "SELECT COUNT(*) FROM games" + where;
        Stmt c(_db, csql.c_str());
        for (const auto &tp : textParams) {
            c.bindText(tp.first, tp.second);
        }
        for (const auto &ip : intParams) {
            c.bindInt(ip.first, ip.second);
        }
        if (c.step() != SQLITE_ROW) {
            return false;
        }
        *total = c.colInt(0);
    }

    std::string sortCol = "ended_at";
    if (filter.sortField == "uploaded_at") {
        sortCol = "COALESCE(NULLIF(uploaded_at, 0), ended_at)";
    } else if (filter.sortField == "ply_count") {
        sortCol = "ply_count";
    }
    std::string sortDir = filter.sortAsc ? "ASC" : "DESC";
    std::string orderBy = " ORDER BY " + sortCol + " " + sortDir + ", game_id " + sortDir;

    std::string sql = std::string("SELECT ") +
                      (filter.includeTranscript ? kGameColsFull : kGameColsNoTranscript) +
                      " FROM games" + where + orderBy +
                      " LIMIT ?" + std::to_string(paramIdx) + " OFFSET ?" + std::to_string(paramIdx + 1);
    Stmt s(_db, sql.c_str());
    for (const auto &tp : textParams) {
        s.bindText(tp.first, tp.second);
    }
    for (const auto &ip : intParams) {
        s.bindInt(ip.first, ip.second);
    }
    s.bindInt(paramIdx, limit);
    s.bindInt(paramIdx + 1, offset);

    int rc;
    while ((rc = s.step()) == SQLITE_ROW) {
        GameRecord g;
        readGame(s, g);
        if (filter.includeTranscript) {
            try {
                auto j = nlohmann::json::parse(g.octiTranscript);
                if (!j.is_object() || j.value("format", "") != "octi.transcript.v3") {
                    continue;
                }
            } catch (...) {
                continue;
            }
        }
        out.push_back(g);
    }
    return rc == SQLITE_DONE;
}

int AccountStore::deleteGamesByInstallId(const std::string &installId)
{
    if (installId.empty() || installId.size() > 64) {
        return -1;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        return -1;
    }
    Stmt s(_db, "DELETE FROM games WHERE install_id = ?1");
    s.bindText(1, installId);
    if (s.step() != SQLITE_DONE) {
        return -1;
    }
    return static_cast<int>(sqlite3_changes(_db));
}

bool AccountStore::getArchiveStats(ArchiveStats &out)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        return false;
    }
    out = ArchiveStats{};

    // 1. Total games and average plies
    {
        Stmt s(_db, "SELECT COUNT(*), COALESCE(AVG(ply_count), 0.0) FROM games");
        if (s.step() == SQLITE_ROW) {
            out.totalGames = s.colInt(0);
            out.averagePlies = s.colDouble(1);
        }
    }

    // 2. Counts by source
    {
        Stmt s(_db, "SELECT source, COUNT(*) FROM games GROUP BY source");
        while (s.step() == SQLITE_ROW) {
            out.bySource[s.colText(0)] = s.colInt(1);
        }
    }

    // 3. Counts by mode
    {
        Stmt s(_db, "SELECT game_mode, COUNT(*) FROM games GROUP BY game_mode");
        while (s.step() == SQLITE_ROW) {
            out.byMode[s.colText(0)] = s.colInt(1);
        }
    }

    // 4. Counts by aiDifficulty
    {
        Stmt s(_db, "SELECT ai_difficulty, COUNT(*) FROM games GROUP BY ai_difficulty");
        while (s.step() == SQLITE_ROW) {
            out.byAiDifficulty[static_cast<int>(s.colInt(0))] = s.colInt(1);
        }
    }

    // 5. Counts by winner
    {
        Stmt s(_db, "SELECT winner, COUNT(*) FROM games GROUP BY winner");
        while (s.step() == SQLITE_ROW) {
            out.byWinner[s.colText(0)] = s.colInt(1);
        }
    }

    // 6. AI win rates per difficulty (1..5)
    {
        Stmt s(_db, "SELECT ai_difficulty, COUNT(*),"
                    " SUM(CASE WHEN (p2_handle LIKE 'AI%' AND winner = 'B') OR"
                    "               (p1_handle LIKE 'AI%' AND winner = 'A')"
                    "          THEN 1 ELSE 0 END)"
                    " FROM games WHERE ai_difficulty > 0 GROUP BY ai_difficulty"
                    " ORDER BY ai_difficulty ASC");
        while (s.step() == SQLITE_ROW) {
            AiDifficultyStats ds;
            ds.difficulty = static_cast<int>(s.colInt(0));
            ds.totalGames = s.colInt(1);
            ds.aiWins = s.colInt(2);
            ds.winRate = ds.totalGames > 0 ? static_cast<double>(ds.aiWins) / static_cast<double>(ds.totalGames) : 0.0;
            out.aiDifficultyStats.push_back(ds);
        }
    }

    // 7. Uploads by day
    {
        Stmt s(_db, "SELECT strftime('%Y-%m-%d', datetime(COALESCE(NULLIF(uploaded_at, 0), ended_at), 'unixepoch')) AS day,"
                    " COUNT(*) FROM games WHERE COALESCE(NULLIF(uploaded_at, 0), ended_at) > 0"
                    " GROUP BY day ORDER BY day ASC");
        while (s.step() == SQLITE_ROW) {
            std::string day = s.colText(0);
            if (!day.empty()) {
                out.uploadsByDay[day] = s.colInt(1);
            }
        }
    }

    return true;
}

bool AccountStore::upsertLiveRoom(const std::string &roomId,
                                  const std::string &stateJson,
                                  std::string &error)
{
    if (roomId.empty() || roomId.size() > 64) {
        error = "invalid room id";
        return false;
    }
    if (stateJson.empty() || stateJson.size() > kMaxTranscriptBytes) {
        error = "invalid state json";
        return false;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        error = "database is closed";
        return false;
    }
    Stmt s(_db, "INSERT OR REPLACE INTO live_rooms(room_id, state_json, updated_at)"
                " VALUES(?1, ?2, ?3)");
    s.bindText(1, roomId);
    s.bindText(2, stateJson);
    s.bindInt(3, nowSec());
    if (s.step() != SQLITE_DONE) {
        error = sqlite3_errmsg(_db);
        return false;
    }
    return true;
}

bool AccountStore::deleteLiveRoom(const std::string &roomId,
                                  std::string &error)
{
    if (roomId.empty() || roomId.size() > 64) {
        error = "invalid room id";
        return false;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        error = "database is closed";
        return false;
    }
    Stmt s(_db, "DELETE FROM live_rooms WHERE room_id = ?1");
    s.bindText(1, roomId);
    if (s.step() != SQLITE_DONE) {
        error = sqlite3_errmsg(_db);
        return false;
    }
    return true;
}

bool AccountStore::listLiveRooms(std::vector<LiveRoomRow> &out,
                                 std::string &error)
{
    out.clear();
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        error = "database is closed";
        return false;
    }
    Stmt s(_db, "SELECT room_id, state_json, updated_at FROM live_rooms"
                " ORDER BY room_id ASC");
    int rc;
    while ((rc = s.step()) == SQLITE_ROW) {
        LiveRoomRow r;
        r.roomId = s.colText(0);
        r.stateJson = s.colText(1);
        r.updatedAt = s.colInt(2);
        out.push_back(std::move(r));
    }
    if (rc != SQLITE_DONE) {
        error = sqlite3_errmsg(_db);
        return false;
    }
    return true;
}

bool AccountStore::setLiveMeta(const std::string &metaKey,
                               const std::string &metaJson,
                               std::string &error)
{
    if (metaKey.empty() || metaKey.size() > 64) {
        error = "invalid meta key";
        return false;
    }
    if (metaJson.empty() || metaJson.size() > kMaxTranscriptBytes) {
        error = "invalid meta json";
        return false;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        error = "database is closed";
        return false;
    }
    Stmt s(_db, "INSERT OR REPLACE INTO live_meta(meta_key, meta_json, updated_at)"
                " VALUES(?1, ?2, ?3)");
    s.bindText(1, metaKey);
    s.bindText(2, metaJson);
    s.bindInt(3, nowSec());
    if (s.step() != SQLITE_DONE) {
        error = sqlite3_errmsg(_db);
        return false;
    }
    return true;
}

bool AccountStore::getLiveMeta(const std::string &metaKey,
                               std::string &metaJson,
                               std::string &error)
{
    if (metaKey.empty() || metaKey.size() > 64) {
        error = "invalid meta key";
        return false;
    }
    metaJson.clear();
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_db) {
        error = "database is closed";
        return false;
    }
    Stmt s(_db, "SELECT meta_json FROM live_meta WHERE meta_key = ?1");
    s.bindText(1, metaKey);
    int rc = s.step();
    if (rc == SQLITE_ROW) {
        metaJson = s.colText(0);
        return true;
    }
    if (rc == SQLITE_DONE) {
        return true;
    }
    error = sqlite3_errmsg(_db);
    return false;
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
