/*
 * OctiBotEngine.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiBotEngine.hxx"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <set>
#include <tuple>

namespace octi {

namespace {

constexpr int WIN_SCORE = 1000000;
constexpr int INF_SCORE = 2000000;
constexpr std::size_t TT_SIZE = 65536;

long steadyClockMs()
{
    using namespace std::chrono;
    return static_cast<long>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

bool isEnemyHomeSquareFor(OctiSquareType sqType, OctiGameMode mode, OctiPlayer p)
{
    if (mode == GAME_MODE_2 || mode == GAME_MODE_OFK) {
        if (p == PLAYER_A) {
            return sqType == B_HOME_SQUARE;
        }
        if (p == PLAYER_B) {
            return sqType == A_HOME_SQUARE;
        }
        return false;
    }
    if (mode == GAME_MODE_4) {
        if (p == PLAYER_A || p == PLAYER_C) {
            return sqType == B_HOME_SQUARE || sqType == D_HOME_SQUARE;
        }
        if (p == PLAYER_B || p == PLAYER_D) {
            return sqType == A_HOME_SQUARE || sqType == C_HOME_SQUARE;
        }
    }
    return false;
}

bool isAlliedHomeSquareFor(OctiSquareType sqType, OctiGameMode mode, OctiPlayer p)
{
    if (mode == GAME_MODE_2 || mode == GAME_MODE_OFK) {
        if (p == PLAYER_A) {
            return sqType == A_HOME_SQUARE;
        }
        if (p == PLAYER_B) {
            return sqType == B_HOME_SQUARE;
        }
        return false;
    }
    if (mode == GAME_MODE_4) {
        if (p == PLAYER_A || p == PLAYER_C) {
            return sqType == A_HOME_SQUARE || sqType == C_HOME_SQUARE;
        }
        if (p == PLAYER_B || p == PLAYER_D) {
            return sqType == B_HOME_SQUARE || sqType == D_HOME_SQUARE;
        }
    }
    return false;
}

std::vector<std::pair<int, int>> targetEnemyBases(const OctiBoard &board,
                                                  OctiPlayer p)
{
    std::vector<std::pair<int, int>> targets;
    OctiGameMode mode = board.getGameMode();
    int w = board.getBoardWidth();
    int h = board.getBoardHeight();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const OctiSquare &sq = board.squareAt(x, y);
            if (isEnemyHomeSquareFor(sq.getType(), mode, p)) {
                targets.push_back({x, y});
            }
        }
    }
    return targets;
}

int torusDist(int a, int b, int dim, bool edgeless)
{
    int d = std::abs(a - b);
    if (edgeless && dim > 0) {
        d = std::min(d, dim - d);
    }
    return d;
}

int minChebyshevToTargets(int x, int y,
                          const std::vector<std::pair<int, int>> &targets,
                          int w, int h, bool edgeless)
{
    if (targets.empty()) {
        return 0;
    }
    int best = 999;
    for (const auto &t : targets) {
        int dx = torusDist(x, t.first, w, edgeless);
        int dy = torusDist(y, t.second, h, edgeless);
        int dist = std::max(dx, dy);
        if (dist < best) {
            best = dist;
        }
    }
    return best;
}

// Prunes non-capturing jumps when an identical jump path with captures exists.
std::vector<OctiMove> filterDominatedMoves(const std::vector<OctiMove> &moves)
{
    if (moves.size() <= 1) {
        return moves;
    }

    // Map (x0, y0, rawPodConfig, hopCoordsVector) -> maximum capture count
    using HopCoordVec = std::vector<std::pair<int, int>>;
    using PathKey = std::tuple<int, int, std::uint32_t, HopCoordVec>;
    std::set<PathKey> hasCapturingVariant;

    for (const auto &m : moves) {
        const auto &ops = m.getOps();
        if (ops.size() != 1 || ops[0].hops.empty()) {
            continue;
        }
        const OctiMoveOp &op = ops[0];
        bool anyCap = false;
        HopCoordVec coords;
        coords.reserve(op.hops.size());
        for (const auto &h : op.hops) {
            coords.push_back({h.x, h.y});
            if (h.capture) {
                anyCap = true;
            }
        }
        if (anyCap) {
            hasCapturingVariant.insert(
                PathKey(op.x0, op.y0, op.pod.rawConfig() & 0xFFFFu, coords));
        }
    }

    if (hasCapturingVariant.empty()) {
        return moves;
    }

    std::vector<OctiMove> filtered;
    filtered.reserve(moves.size());
    for (const auto &m : moves) {
        const auto &ops = m.getOps();
        if (ops.size() == 1 && !ops[0].hops.empty()) {
            const OctiMoveOp &op = ops[0];
            bool anyCap = false;
            HopCoordVec coords;
            coords.reserve(op.hops.size());
            for (const auto &h : op.hops) {
                coords.push_back({h.x, h.y});
                if (h.capture) {
                    anyCap = true;
                }
            }
            if (!anyCap) {
                PathKey key(op.x0, op.y0, op.pod.rawConfig() & 0xFFFFu, coords);
                if (hasCapturingVariant.count(key) > 0) {
                    continue;
                }
            }
        }
        filtered.push_back(m);
    }

    return filtered.empty() ? moves : filtered;
}

int scoreMoveHeuristic(const OctiBoard &board,
                       const OctiMove &move,
                       OctiPlayer turn,
                       const std::vector<std::pair<int, int>> &enemyTargets)
{
    const OctiGame *game = board.getGame();
    OctiGameMode mode = board.getGameMode();
    int w = board.getBoardWidth();
    int h = board.getBoardHeight();
    bool edgeless = game ? game->useEdgelessBoard() : false;

    int score = 0;
    for (const auto &op : move.getOps()) {
        if (!op.hops.empty()) {
            int startDist = minChebyshevToTargets(op.x0, op.y0, enemyTargets, w, h, edgeless);
            const OctiHop &lastHop = op.hops.back();
            int endDist = minChebyshevToTargets(lastHop.x, lastHop.y, enemyTargets, w, h, edgeless);

            score += (startDist - endDist) * 450;

            const OctiSquare &destSq = board.squareAt(lastHop.x, lastHop.y);
            if (isEnemyHomeSquareFor(destSq.getType(), mode, turn) &&
                !destSq.isCapturedByTeamOf(turn)) {
                score += 45000;
            }
            if (destSq.getType() == NEUTRAL_OCTI_SQUARE && destSq.isEmpty()) {
                score += 2000;
            }

            for (const auto &hop : op.hops) {
                if (hop.capture) {
                    score += 9000;
                } else {
                    score += 250;
                }
            }

            // If starting square is an allied home square that has an enemy pod on it,
            // jumping over and capturing that enemy pod is huge; moving away without capture is bad.
            const OctiSquare &startSq = board.squareAt(op.x0, op.y0);
            if (isEnemyHomeSquareFor(startSq.getType(), mode, turn)) {
                // Leaving an already captured enemy home base is discouraged unless winning
                if (startSq.getNumPods() <= 1 &&
                    !isEnemyHomeSquareFor(destSq.getType(), mode, turn)) {
                    score -= 12000;
                }
            }
        } else if (op.addProngDir != DIR_UNDEF && op.rmProngDir == DIR_UNDEF) {
            score += 350;
            if (op.superProng) {
                score += 150;
            }
            int nx = op.x0 + dirDeltaX(op.addProngDir);
            int ny = op.y0 + dirDeltaY(op.addProngDir);
            int curDist = minChebyshevToTargets(op.x0, op.y0, enemyTargets, w, h, edgeless);
            int nextDist = minChebyshevToTargets(nx, ny, enemyTargets, w, h, edgeless);
            if (nextDist < curDist) {
                score += 400;
            }
            if (op.pod.getNumProngs() < 3) {
                score += 200;
            }
        } else if (op.addPod) {
            if (op.liberate) {
                score += 1800;
            } else {
                score += 300;
            }
        } else if (op.rmProngDir != DIR_UNDEF) {
            score += 40;
        }
    }
    return score;
}

std::vector<OctiMove> orderAndCapMoves(const OctiBoard &board,
                                       const std::vector<OctiMove> &rawMoves,
                                       OctiPlayer turn,
                                       const std::string &ttBestMove,
                                       std::size_t maxBranch)
{
    std::vector<OctiMove> moves = filterDominatedMoves(rawMoves);
    std::vector<std::pair<int, int>> targets = targetEnemyBases(board, turn);

    std::vector<std::pair<int, std::size_t>> scored;
    scored.reserve(moves.size());
    for (std::size_t i = 0; i < moves.size(); ++i) {
        int s = scoreMoveHeuristic(board, moves[i], turn, targets);
        if (!ttBestMove.empty() && moves[i].asString() == ttBestMove) {
            s += 200000;
        }
        scored.push_back({s, i});
    }

    std::stable_sort(scored.begin(), scored.end(),
                     [](const auto &a, const auto &b) {
                         return a.first > b.first;
                     });

    std::size_t limit = std::min(moves.size(), maxBranch);
    std::vector<OctiMove> result;
    result.reserve(limit);
    for (std::size_t i = 0; i < limit; ++i) {
        result.push_back(moves[scored[i].second]);
    }
    return result;
}

} // namespace

OctiBotEngine::OctiBotEngine(BotDifficulty difficulty, std::uint64_t rngSeed)
    : _difficulty(difficulty)
    , _timeBudgetMs(defaultBudgetForDifficulty(difficulty))
    , _customTimeBudget(false)
    , _rngState(rngSeed ? rngSeed : 0x5EED2026ULL)
    , _tt(TT_SIZE)
{
}

OctiBotEngine::~OctiBotEngine() = default;

void OctiBotEngine::setDifficulty(BotDifficulty d)
{
    _difficulty = d;
    if (!_customTimeBudget) {
        _timeBudgetMs = defaultBudgetForDifficulty(d);
    }
}

BotDifficulty OctiBotEngine::difficulty() const
{
    return _difficulty;
}

void OctiBotEngine::setTimeBudgetMs(int maxTimeMs)
{
    _timeBudgetMs = std::max(10, maxTimeMs);
    _customTimeBudget = true;
}

int OctiBotEngine::timeBudgetMs() const
{
    return _timeBudgetMs;
}

int OctiBotEngine::defaultBudgetForDifficulty(BotDifficulty d) const
{
    switch (d) {
    case BotDifficulty::CASUAL:   return 60;
    case BotDifficulty::TACTICAL: return 180;
    case BotDifficulty::MASTER:   return 450;
    }
    return 180;
}

int OctiBotEngine::maxDepthForDifficulty(BotDifficulty d) const
{
    switch (d) {
    case BotDifficulty::CASUAL:   return 2;
    case BotDifficulty::TACTICAL: return 3;
    case BotDifficulty::MASTER:   return 4;
    }
    return 3;
}

std::uint64_t OctiBotEngine::nextRandom()
{
    std::uint64_t x = _rngState;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    _rngState = x;
    return x;
}

std::uint64_t OctiBotEngine::computePositionHash(const OctiBoard &board) const
{
    std::string pos = board.toPositionString();
    std::uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : pos) {
        h ^= static_cast<std::uint64_t>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

int OctiBotEngine::evaluatePosition(const OctiBoard &board,
                                    OctiPlayer perspective)
{
    const OctiGame *game = board.getGame();
    if (!game || perspective == PLAYER_UNDEF) {
        return 0;
    }

    OctiGameMode mode = board.getGameMode();
    if (game->isGameOver()) {
        OctiPlayer w = game->getWinner();
        if (w == PLAYER_UNDEF) {
            return 0;
        }
        return areAlliedPlayers(mode, w, perspective) ? WIN_SCORE : -WIN_SCORE;
    }

    int w = board.getBoardWidth();
    int h = board.getBoardHeight();
    bool edgeless = game->useEdgelessBoard();

    std::vector<std::pair<int, int>> allyTargets = targetEnemyBases(board, perspective);
    OctiPlayer sampleEnemy = (perspective == PLAYER_A || perspective == PLAYER_C)
        ? PLAYER_B : PLAYER_A;
    std::vector<std::pair<int, int>> enemyTargets = targetEnemyBases(board, sampleEnemy);

    int totalScore = 0;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const OctiSquare &sq = board.squareAt(x, y);
            OctiSquareType sqType = sq.getType();

            if (isEnemyHomeSquareFor(sqType, mode, perspective)) {
                if (sq.isCapturedByTeamOf(perspective)) {
                    totalScore += 26000;
                }
            } else if (isAlliedHomeSquareFor(sqType, mode, perspective)) {
                if (sq.isCaptured() && !sq.isCapturedByTeamOf(perspective)) {
                    totalScore -= 30000;
                }
            } else if (sqType == NEUTRAL_OCTI_SQUARE) {
                if (sq.isCapturedByTeamOf(perspective)) {
                    totalScore += 3500;
                } else if (sq.isCaptured()) {
                    totalScore -= 3500;
                }
            }

            for (int i = 0; i < sq.getNumPods(); ++i) {
                const OctiPod &pod = sq.getPodAt(i);
                OctiPlayer owner = podTypeToPlayer(pod.getType());
                bool isAlly = areAlliedPlayers(mode, owner, perspective);
                const auto &targets = isAlly ? allyTargets : enemyTargets;

                int podVal = 1400;
                int dist = minChebyshevToTargets(x, y, targets, w, h, edgeless);
                podVal += std::max(0, (10 - dist)) * 160;

                for (int d = 0; d < DIR_MAX; ++d) {
                    OctiDir dir = static_cast<OctiDir>(d);
                    OctiProngType pt = pod.prongAt(dir);
                    if (pt == NO_PRONG) {
                        continue;
                    }
                    podVal += (pt == SUPER_PRONG) ? 320 : 180;

                    int nx = edgeless ? (((x + dirDeltaX(dir)) % w + w) % w) : (x + dirDeltaX(dir));
                    int ny = edgeless ? (((y + dirDeltaY(dir)) % h + h) % h) : (y + dirDeltaY(dir));
                    if (board.isValidCoord(nx, ny)) {
                        int ndist = minChebyshevToTargets(nx, ny, targets, w, h, edgeless);
                        if (ndist < dist) {
                            podVal += 110;
                        }
                        if (ndist == 0) {
                            // 1 step away from an enemy home base with a prong aimed at it!
                            podVal += 6500;
                        }
                    }

                    int jx = edgeless ? (((x + 2 * dirDeltaX(dir)) % w + w) % w) : (x + 2 * dirDeltaX(dir));
                    int jy = edgeless ? (((y + 2 * dirDeltaY(dir)) % h + h) % h) : (y + 2 * dirDeltaY(dir));
                    if (board.isValidCoord(nx, ny) && board.isValidCoord(jx, jy)) {
                        if (!board.squareAt(nx, ny).isEmpty() &&
                            board.squareAt(jx, jy).isEmpty()) {
                            int jdist = minChebyshevToTargets(jx, jy, targets, w, h, edgeless);
                            if (jdist == 0) {
                                // Ready to jump directly onto an enemy home base!
                                podVal += 9500;
                            }
                        }
                    }
                }

                totalScore += isAlly ? podVal : -podVal;
            }
        }
    }

    int numPlayers = (mode == GAME_MODE_4) ? 4 : 2;
    for (int pIdx = 1; pIdx <= numPlayers; ++pIdx) {
        OctiPlayer p = static_cast<OctiPlayer>(pIdx);
        bool isAlly = areAlliedPlayers(mode, p, perspective);
        int resVal = game->getFreePods(p) * 380
                   - game->getCapturedPods(p) * 300
                   + game->getFreeProngs(p) * 45
                   + game->getFreeSuperProngs(p) * 140;
        if (mode == GAME_MODE_4 && game->isPlayerEliminated(p)) {
            resVal -= 18000;
        }
        totalScore += isAlly ? resVal : -resVal;
    }

    return totalScore;
}

int OctiBotEngine::negamax(OctiBoard &scratchBoard,
                           int depth,
                           int ply,
                           int alpha,
                           int beta,
                           OctiPlayer currentPerspective,
                           std::uint64_t &nodesVisited,
                           long deadlineMs,
                           bool &timedOut)
{
    ++nodesVisited;
    if ((nodesVisited & 63ULL) == 0ULL && steadyClockMs() >= deadlineMs) {
        timedOut = true;
        return evaluatePosition(scratchBoard, currentPerspective);
    }

    OctiGame *game = scratchBoard.getGame();
    OctiGameMode mode = scratchBoard.getGameMode();
    if (game->isGameOver()) {
        OctiPlayer w = game->getWinner();
        if (w == PLAYER_UNDEF) {
            return 0;
        }
        return areAlliedPlayers(mode, w, currentPerspective)
            ? (WIN_SCORE - ply)
            : (-WIN_SCORE + ply);
    }

    if (depth <= 0) {
        return evaluatePosition(scratchBoard, currentPerspective);
    }

    std::uint64_t hashKey = computePositionHash(scratchBoard);
    TranspositionEntry &ttEntry = _tt[hashKey % TT_SIZE];
    std::string ttMove;
    if (ttEntry.key == hashKey && ttEntry.depth >= depth) {
        if (ttEntry.flag == 0) {
            return ttEntry.score;
        }
        if (ttEntry.flag == 1 && ttEntry.score > alpha) {
            alpha = ttEntry.score;
        } else if (ttEntry.flag == 2 && ttEntry.score < beta) {
            beta = ttEntry.score;
        }
        if (alpha >= beta) {
            return ttEntry.score;
        }
        ttMove = ttEntry.bestMove;
    }

    std::vector<OctiMove> rawMoves = game->generateLegalMoves();
    if (rawMoves.empty()) {
        return evaluatePosition(scratchBoard, currentPerspective);
    }

    OctiPlayer turn = game->getTurn();
    std::size_t maxBranch = (_difficulty == BotDifficulty::MASTER) ? 20 : 14;
    std::vector<OctiMove> ordered = orderAndCapMoves(
        scratchBoard, rawMoves, turn, ttMove, maxBranch);

    int origAlpha = alpha;
    int bestVal = -INF_SCORE;
    std::string bestMoveStr = ordered[0].asString();

    for (const auto &mv : ordered) {
        OctiGameSnapshot snap = game->captureSnapshot();
        try {
            game->move(mv);
        } catch (...) {
            game->restoreSnapshot(snap);
            continue;
        }

        int val = 0;
        if (game->isGameOver()) {
            OctiPlayer w = game->getWinner();
            val = areAlliedPlayers(mode, w, turn)
                ? (WIN_SCORE - (ply + 1))
                : (-WIN_SCORE + (ply + 1));
        } else {
            OctiPlayer nextTurn = game->getTurn();
            bool sameTeamNext = areAlliedPlayers(mode, turn, nextTurn);
            if (sameTeamNext) {
                val = negamax(scratchBoard, depth - 1, ply + 1,
                              alpha, beta, turn, nodesVisited, deadlineMs, timedOut);
            } else {
                OctiPlayer oppPerspective = (nextTurn != PLAYER_UNDEF) ? nextTurn :
                    ((turn == PLAYER_A || turn == PLAYER_C) ? PLAYER_B : PLAYER_A);
                val = -negamax(scratchBoard, depth - 1, ply + 1,
                               -beta, -alpha, oppPerspective, nodesVisited, deadlineMs, timedOut);
            }
        }

        game->restoreSnapshot(snap);

        if (timedOut) {
            break;
        }

        if (val > bestVal) {
            bestVal = val;
            bestMoveStr = mv.asString();
        }
        if (val > alpha) {
            alpha = val;
        }
        if (alpha >= beta) {
            break;
        }
    }

    if (!timedOut && bestVal > -INF_SCORE) {
        ttEntry.key = hashKey;
        ttEntry.depth = depth;
        ttEntry.score = bestVal;
        ttEntry.bestMove = bestMoveStr;
        if (bestVal <= origAlpha) {
            ttEntry.flag = 2;
        } else if (bestVal >= beta) {
            ttEntry.flag = 1;
        } else {
            ttEntry.flag = 0;
        }
    }

    return bestVal;
}

std::string OctiBotEngine::chooseMove(const OctiBoard &board,
                                      BotSearchStats *outStats)
{
    if (outStats) {
        *outStats = BotSearchStats();
    }

    const OctiGame *srcGame = board.getGame();
    if (!srcGame || !srcGame->isGameStarted() || srcGame->isGameOver() ||
        srcGame->getTurn() == PLAYER_UNDEF) {
        return "";
    }

    long startMs = steadyClockMs();
    long deadlineMs = startMs + _timeBudgetMs;

    OctiBoard scratchBoard;
    scratchBoard.suppressBoardUpdateNotification(true);
    scratchBoard.fromPositionString(board.toPositionString());
    OctiGame *game = scratchBoard.getGame();
    if (!game) {
        return "";
    }

    std::vector<OctiMove> legalMoves = game->generateLegalMoves();
    if (legalMoves.empty()) {
        return "";
    }

    OctiPlayer rootTurn = game->getTurn();
    OctiGameMode mode = scratchBoard.getGameMode();
    std::uint64_t nodesVisited = 0;

    // 1. Check for immediate 1-ply victory across ALL legal moves first
    OctiGameSnapshot rootSnap = game->captureSnapshot();
    for (const auto &mv : legalMoves) {
        ++nodesVisited;
        try {
            game->move(mv);
            if (game->isGameOver() &&
                areAlliedPlayers(mode, game->getWinner(), rootTurn)) {
                game->restoreSnapshot(rootSnap);
                if (outStats) {
                    outStats->depthReached = 1;
                    outStats->nodesVisited = nodesVisited;
                    outStats->bestScore = WIN_SCORE;
                    outStats->elapsedMs = std::max(0L, steadyClockMs() - startMs);
                }
                return mv.asString();
            }
        } catch (...) {
        }
        game->restoreSnapshot(rootSnap);
    }

    std::size_t rootCap = (_difficulty == BotDifficulty::MASTER) ? 32 :
                          (_difficulty == BotDifficulty::TACTICAL) ? 24 : 20;
    std::vector<OctiMove> rootCandidates = orderAndCapMoves(
        scratchBoard, legalMoves, rootTurn, "", rootCap);

    std::string bestMoveStr = rootCandidates[0].asString();
    int bestScore = -INF_SCORE;
    int maxTargetDepth = maxDepthForDifficulty(_difficulty);
    int completedDepth = 0;

    for (int d = 1; d <= maxTargetDepth; ++d) {
        bool timedOut = false;
        int alpha = -INF_SCORE;
        int beta = INF_SCORE;
        std::string iterBestMove = bestMoveStr;
        int iterBestScore = -INF_SCORE;

        for (const auto &mv : rootCandidates) {
            try {
                game->move(mv);
            } catch (...) {
                game->restoreSnapshot(rootSnap);
                continue;
            }

            int score = 0;
            if (game->isGameOver()) {
                OctiPlayer w = game->getWinner();
                score = areAlliedPlayers(mode, w, rootTurn)
                    ? (WIN_SCORE - 1)
                    : (-WIN_SCORE + 1);
            } else {
                OctiPlayer nextTurn = game->getTurn();
                bool sameTeamNext = areAlliedPlayers(mode, rootTurn, nextTurn);
                if (sameTeamNext) {
                    score = negamax(scratchBoard, d - 1, 1,
                                    alpha, beta, rootTurn, nodesVisited,
                                    deadlineMs, timedOut);
                } else {
                    OctiPlayer oppPerspective = (nextTurn != PLAYER_UNDEF) ? nextTurn :
                        ((rootTurn == PLAYER_A || rootTurn == PLAYER_C) ? PLAYER_B : PLAYER_A);
                    score = -negamax(scratchBoard, d - 1, 1,
                                     -beta, -alpha, oppPerspective, nodesVisited,
                                     deadlineMs, timedOut);
                }
            }

            game->restoreSnapshot(rootSnap);

            if ( _difficulty == BotDifficulty::CASUAL &&
                 std::abs(score) < WIN_SCORE / 2 ) {
                int jitter = static_cast<int>(nextRandom() % 31ULL) - 15;
                score += jitter;
            }

            if (score > iterBestScore) {
                iterBestScore = score;
                iterBestMove = mv.asString();
            }
            if (score > alpha) {
                alpha = score;
            }

            if (timedOut && d > 1) {
                break;
            }
        }

        if (!timedOut || d == 1) {
            bestMoveStr = iterBestMove;
            bestScore = iterBestScore;
            completedDepth = d;
            // Promote iterBestMove to front of rootCandidates for next iteration
            auto it = std::find_if(rootCandidates.begin(), rootCandidates.end(),
                                   [&](const OctiMove &m) {
                                       return m.asString() == bestMoveStr;
                                   });
            if (it != rootCandidates.end() && it != rootCandidates.begin()) {
                std::rotate(rootCandidates.begin(), it, it + 1);
            }
        }

        if (timedOut || steadyClockMs() >= deadlineMs ||
            std::abs(bestScore) >= WIN_SCORE - 100) {
            break;
        }
    }

    if (outStats) {
        outStats->depthReached = std::max(1, completedDepth);
        outStats->nodesVisited = nodesVisited;
        outStats->bestScore = bestScore;
        outStats->elapsedMs = std::max(0L, steadyClockMs() - startMs);
    }

    return bestMoveStr;
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
