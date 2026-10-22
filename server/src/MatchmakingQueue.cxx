/*
 * MatchmakingQueue.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "MatchmakingQueue.hxx"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <set>
#include <tuple>

namespace octi {

namespace {

const int64_t kMaxClockMs = 4LL * 3600 * 1000;

int64_t steadyNowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

} // namespace

bool MatchRuleset::operator==(const MatchRuleset &o) const
{
    return mode == o.mode && bases == o.bases &&
           useSuperProngs == o.useSuperProngs &&
           useEdgelessBoard == o.useEdgelessBoard &&
           initialMs == o.initialMs && incrementMs == o.incrementMs;
}

bool MatchRuleset::operator<(const MatchRuleset &o) const
{
    return std::make_tuple(static_cast<int>(mode), static_cast<int>(bases),
                           useSuperProngs, useEdgelessBoard, initialMs,
                           incrementMs) <
           std::make_tuple(static_cast<int>(o.mode),
                           static_cast<int>(o.bases), o.useSuperProngs,
                           o.useEdgelessBoard, o.initialMs, o.incrementMs);
}

MatchRuleset MatchRuleset::normalized() const
{
    MatchRuleset r = *this;
    if (r.mode == GAME_MODE_OFK) {
        r.bases = GAME_BASES_1;
        r.useSuperProngs = false;
    } else if (r.mode == GAME_MODE_4) {
        r.bases = GAME_BASES_3;
    }
    if (r.initialMs <= 0) {
        r.initialMs = 0;
        r.incrementMs = 0;
    }
    return r;
}

bool MatchRuleset::isValid() const
{
    if (mode != GAME_MODE_2 && mode != GAME_MODE_4 && mode != GAME_MODE_OFK) {
        return false;
    }
    if (bases != GAME_BASES_1 && bases != GAME_BASES_3) {
        return false;
    }
    if (initialMs < 0 || initialMs > kMaxClockMs || incrementMs < 0 ||
        incrementMs > kMaxClockMs) {
        return false;
    }
    return true;
}

MatchmakingQueue::MatchmakingQueue(const MatchmakingConfig &config)
    : _config(config), _clock(steadyNowMs), _nextOrder(0)
{
    setConfig(config);
}

void MatchmakingQueue::setClock(std::function<int64_t()> nowMs)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _clock = nowMs ? std::move(nowMs) : std::function<int64_t()>(steadyNowMs);
}

void MatchmakingQueue::setConfig(const MatchmakingConfig &config)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _config = config;
    _config.baseWindow = std::max(0, _config.baseWindow);
    _config.windowStep = std::max(0, _config.windowStep);
    _config.stepIntervalMs = std::max<int64_t>(1, _config.stepIntervalMs);
    _config.maxWindow = std::max(_config.baseWindow, _config.maxWindow);
}

MatchmakingConfig MatchmakingQueue::config() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _config;
}

int64_t MatchmakingQueue::nowLocked() const
{
    return _clock();
}

int MatchmakingQueue::windowLocked(int64_t waitedMs) const
{
    int64_t steps = waitedMs > 0 ? waitedMs / _config.stepIntervalMs : 0;
    int64_t w = static_cast<int64_t>(_config.baseWindow) +
                static_cast<int64_t>(_config.windowStep) * steps;
    if (steps != 0 && _config.windowStep != 0 &&
        steps > static_cast<int64_t>(_config.maxWindow)) {
        w = _config.maxWindow; /* also avoids any overflow of the product */
    }
    return static_cast<int>(std::min<int64_t>(w, _config.maxWindow));
}

int MatchmakingQueue::windowForWait(int64_t waitedMs) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return windowLocked(waitedMs);
}

EnqueueStatus MatchmakingQueue::enqueue(int64_t playerId, int rating,
                                        const MatchRuleset &ruleset)
{
    MatchRuleset rs = ruleset.normalized();
    if (playerId <= 0 || !rs.isValid() || rating < 0) {
        return EnqueueStatus::INVALID;
    }
    std::lock_guard<std::mutex> lock(_mutex);
    if (_entries.count(playerId) != 0) {
        return EnqueueStatus::ALREADY_QUEUED;
    }
    if (_entries.size() >= kMaxQueueSize) {
        return EnqueueStatus::QUEUE_FULL;
    }
    Entry e;
    e.playerId = playerId;
    e.rating = rating;
    e.ruleset = rs;
    e.enqueuedAtMs = nowLocked();
    e.order = _nextOrder++;
    _entries[playerId] = e;
    return EnqueueStatus::OK;
}

bool MatchmakingQueue::dequeue(int64_t playerId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _entries.erase(playerId) != 0;
}

bool MatchmakingQueue::isQueued(int64_t playerId) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _entries.count(playerId) != 0;
}

bool MatchmakingQueue::queueInfo(int64_t playerId, QueueInfo &out) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::map<int64_t, Entry>::const_iterator it = _entries.find(playerId);
    if (it == _entries.end()) {
        return false;
    }
    int64_t waited = std::max<int64_t>(0, nowLocked() - it->second.enqueuedAtMs);
    out.rating = it->second.rating;
    out.ruleset = it->second.ruleset;
    out.waitedMs = waited;
    out.window = windowLocked(waited);
    return true;
}

size_t MatchmakingQueue::size() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _entries.size();
}

bool MatchmakingQueue::compatibleLocked(const Entry &a, const Entry &b,
                                        int64_t now) const
{
    if (a.ruleset != b.ruleset) {
        return false;
    }
    int wa = windowLocked(std::max<int64_t>(0, now - a.enqueuedAtMs));
    int wb = windowLocked(std::max<int64_t>(0, now - b.enqueuedAtMs));
    int diff = std::abs(a.rating - b.rating);
    return diff <= std::min(wa, wb);
}

MatchedPlayer MatchmakingQueue::describeLocked(const Entry &e,
                                               int64_t now) const
{
    MatchedPlayer m;
    m.playerId = e.playerId;
    m.rating = e.rating;
    m.waitedMs = std::max<int64_t>(0, now - e.enqueuedAtMs);
    return m;
}

std::vector<MatchGroup> MatchmakingQueue::findMatches()
{
    std::lock_guard<std::mutex> lock(_mutex);
    int64_t now = nowLocked();

    /* Bucket by ruleset; within a bucket oldest ticket first. */
    std::map<MatchRuleset, std::vector<Entry>> buckets;
    for (std::map<int64_t, Entry>::const_iterator it = _entries.begin();
         it != _entries.end(); ++it) {
        buckets[it->second.ruleset].push_back(it->second);
    }

    std::vector<MatchGroup> groups;
    std::set<int64_t> matched;

    for (std::map<MatchRuleset, std::vector<Entry>>::iterator b =
             buckets.begin();
         b != buckets.end(); ++b) {
        std::vector<Entry> &list = b->second;
        std::sort(list.begin(), list.end(),
                  [](const Entry &x, const Entry &y) {
                      return std::make_tuple(x.enqueuedAtMs, x.order) <
                             std::make_tuple(y.enqueuedAtMs, y.order);
                  });
        const size_t need = static_cast<size_t>(b->first.playersRequired());

        for (size_t i = 0; i < list.size(); ++i) {
            const Entry &anchor = list[i];
            if (matched.count(anchor.playerId) != 0) {
                continue;
            }

            /* Candidates ordered by closeness to the anchor's rating. */
            std::vector<const Entry *> cands;
            for (size_t j = 0; j < list.size(); ++j) {
                if (j != i && matched.count(list[j].playerId) == 0 &&
                    compatibleLocked(anchor, list[j], now)) {
                    cands.push_back(&list[j]);
                }
            }
            std::sort(cands.begin(), cands.end(),
                      [&anchor](const Entry *x, const Entry *y) {
                          int dx = std::abs(x->rating - anchor.rating);
                          int dy = std::abs(y->rating - anchor.rating);
                          return std::make_tuple(dx, x->enqueuedAtMs,
                                                 x->order) <
                                 std::make_tuple(dy, y->enqueuedAtMs,
                                                 y->order);
                      });

            std::vector<const Entry *> chosen;
            chosen.push_back(&anchor);
            for (size_t c = 0; c < cands.size() && chosen.size() < need; ++c) {
                bool ok = true;
                for (size_t k = 1; k < chosen.size(); ++k) {
                    if (!compatibleLocked(*chosen[k], *cands[c], now)) {
                        ok = false;
                        break;
                    }
                }
                if (ok) {
                    chosen.push_back(cands[c]);
                }
            }
            if (chosen.size() != need) {
                continue;
            }

            MatchGroup g;
            g.ruleset = b->first;
            if (need == 2) {
                g.players.push_back(describeLocked(*chosen[0], now));
                g.players.push_back(describeLocked(*chosen[1], now));
            } else {
                /* Strongest + weakest vs the two in between. */
                std::vector<const Entry *> byRating = chosen;
                std::sort(byRating.begin(), byRating.end(),
                          [](const Entry *x, const Entry *y) {
                              return std::make_tuple(-x->rating,
                                                     x->enqueuedAtMs,
                                                     x->order) <
                                     std::make_tuple(-y->rating,
                                                     y->enqueuedAtMs,
                                                     y->order);
                          });
                g.players.push_back(describeLocked(*byRating[0], now));
                g.players.push_back(describeLocked(*byRating[1], now));
                g.players.push_back(describeLocked(*byRating[3], now));
                g.players.push_back(describeLocked(*byRating[2], now));
            }
            for (size_t k = 0; k < chosen.size(); ++k) {
                matched.insert(chosen[k]->playerId);
            }
            groups.push_back(g);
        }
    }

    for (std::set<int64_t>::const_iterator it = matched.begin();
         it != matched.end(); ++it) {
        _entries.erase(*it);
    }
    return groups;
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
