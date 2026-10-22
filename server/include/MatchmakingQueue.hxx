/*
 * MatchmakingQueue.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_SERVER_MATCHMAKING_QUEUE_HXX
#define OCTI_SERVER_MATCHMAKING_QUEUE_HXX

#include "OctiDefs.hxx"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <vector>

namespace octi {

/*
 * The ruleset options that bucket the queue: players are only ever matched
 * with players asking for exactly the same ruleset.
 */
struct MatchRuleset {
    OctiGameMode mode = GAME_MODE_2;
    OctiGameBases bases = GAME_BASES_3;
    bool useSuperProngs = false;
    bool useEdgelessBoard = false;
    int64_t initialMs = 0;          // 0 = untimed
    int64_t incrementMs = 0;

    bool operator==(const MatchRuleset &o) const;
    bool operator!=(const MatchRuleset &o) const { return !(*this == o); }
    bool operator<(const MatchRuleset &o) const;

    bool isTimed() const { return initialMs > 0; }
    int playersRequired() const { return mode == GAME_MODE_4 ? 4 : 2; }

    /*
     * Fold options the engine ignores onto one canonical value so that
     * equivalent requests share a bucket: OCTI for Kids always uses one
     * base and has no superprong; 4-player always uses the 3-base layout;
     * an untimed request drops any stray increment.
     */
    MatchRuleset normalized() const;
    bool isValid() const;
};

/*
 * Rating window = min(maxWindow, baseWindow + windowStep * floor(wait /
 * stepIntervalMs)). Two players are only paired when their rating distance
 * is within BOTH of their current windows (the narrower one governs).
 */
struct MatchmakingConfig {
    int baseWindow = 100;
    int windowStep = 50;
    int64_t stepIntervalMs = 10000;
    int maxWindow = 1000;
};

struct MatchedPlayer {
    int64_t playerId = 0;
    int rating = 0;
    int64_t waitedMs = 0;
};

/*
 * One matched game. players[i] takes seat i + 1. 2-player and OFK groups
 * hold two players (the longer waiter takes seat 1, which moves first).
 * 4-player groups hold four: seats 1 and 3 are the strongest and weakest
 * player, seats 2 and 4 the two in between, so both teams are balanced.
 */
struct MatchGroup {
    MatchRuleset ruleset;
    std::vector<MatchedPlayer> players;
};

enum class EnqueueStatus {
    OK,
    ALREADY_QUEUED,
    INVALID,
    QUEUE_FULL
};

struct QueueInfo {
    int rating = 0;
    MatchRuleset ruleset;
    int64_t waitedMs = 0;
    int window = 0;
};

class MatchmakingQueue {
public:
    static constexpr size_t kMaxQueueSize = 5000;

    explicit MatchmakingQueue(const MatchmakingConfig &config = MatchmakingConfig());

    MatchmakingQueue(const MatchmakingQueue &) = delete;
    MatchmakingQueue &operator=(const MatchmakingQueue &) = delete;

    /* Millisecond clock; defaults to a monotonic clock. */
    void setClock(std::function<int64_t()> nowMs);
    void setConfig(const MatchmakingConfig &config);
    MatchmakingConfig config() const;

    EnqueueStatus enqueue(int64_t playerId, int rating,
                          const MatchRuleset &ruleset);
    bool dequeue(int64_t playerId);
    bool isQueued(int64_t playerId) const;
    bool queueInfo(int64_t playerId, QueueInfo &out) const;
    size_t size() const;

    /* Rating window after waiting waitedMs (negative waits count as 0). */
    int windowForWait(int64_t waitedMs) const;

    /*
     * Remove and return every group that can be formed right now. Oldest
     * ticket first, closest rating first, ties broken by arrival order, so
     * the result is deterministic for a given clock and arrival sequence.
     */
    std::vector<MatchGroup> findMatches();

private:
    struct Entry {
        int64_t playerId = 0;
        int rating = 0;
        MatchRuleset ruleset;
        int64_t enqueuedAtMs = 0;
        uint64_t order = 0;
    };

    int64_t nowLocked() const;
    int windowLocked(int64_t waitedMs) const;
    bool compatibleLocked(const Entry &a, const Entry &b, int64_t now) const;
    MatchedPlayer describeLocked(const Entry &e, int64_t now) const;

    mutable std::mutex _mutex;
    MatchmakingConfig _config;
    std::function<int64_t()> _clock;
    std::map<int64_t, Entry> _entries;
    uint64_t _nextOrder;
};

} // namespace octi

#endif // OCTI_SERVER_MATCHMAKING_QUEUE_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
