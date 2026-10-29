/*
 * OctiBotEngine.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiBotEngine.hxx"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <set>
#include <thread>
#include <tuple>
#include <vector>

namespace octi {

namespace {

constexpr int WIN_SCORE = 1000000;
constexpr int INF_SCORE = 2000000;
constexpr std::size_t TT_SIZE = 262144; // 256K entries (1 << 18)
constexpr std::size_t TT_MUTEX_COUNT = 1024;

long steadyClockMs()
{
    using namespace std::chrono;
    return static_cast<long>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

// ---------------------------------------------------------------------------
// 64-bit Zobrist Binary Hashing Tables
// ---------------------------------------------------------------------------

struct ZobristTables {
    std::uint64_t piece[81][7][4];
    std::uint64_t prong[81][7][8][3];
    std::uint64_t baseCaptured[81][5];
    std::uint64_t freePods[5][8];
    std::uint64_t capturedPods[5][8];
    std::uint64_t freeProngs[5][33];
    std::uint64_t freeSuperProngs[5][9];
    std::uint64_t turn[5];
    std::uint64_t mode[4];

    ZobristTables()
    {
        std::uint64_t state = 0x9E3779B97F4A7C15ULL;
        auto splitmix64 = [&state]() -> std::uint64_t {
            std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
            return z ^ (z >> 31);
        };

        for (int sq = 0; sq < 81; ++sq) {
            for (int p = 0; p < 7; ++p) {
                for (int t = 0; t < 4; ++t) {
                    piece[sq][p][t] = splitmix64();
                }
                for (int d = 0; d < 8; ++d) {
                    for (int pt = 0; pt < 3; ++pt) {
                        prong[sq][p][d][pt] = splitmix64();
                    }
                }
            }
            for (int c = 0; c < 5; ++c) {
                baseCaptured[sq][c] = splitmix64();
            }
        }
        for (int pl = 0; pl < 5; ++pl) {
            for (int i = 0; i < 8; ++i) {
                freePods[pl][i] = splitmix64();
                capturedPods[pl][i] = splitmix64();
            }
            for (int i = 0; i < 33; ++i) {
                freeProngs[pl][i] = splitmix64();
            }
            for (int i = 0; i < 9; ++i) {
                freeSuperProngs[pl][i] = splitmix64();
            }
            turn[pl] = splitmix64();
        }
        for (int m = 0; m < 4; ++m) {
            mode[m] = splitmix64();
        }
    }
};

static const ZobristTables g_zobrist;

static std::array<std::mutex, TT_MUTEX_COUNT> g_ttMutexes;

// ---------------------------------------------------------------------------
// Board Square & Target Queries
// ---------------------------------------------------------------------------

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
    std::vector<std::pair<int, int>> uncaptured;
    std::vector<std::pair<int, int>> allEnemy;
    OctiGameMode mode = board.getGameMode();
    int w = board.getBoardWidth();
    int h = board.getBoardHeight();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const OctiSquare &sq = board.squareAt(x, y);
            if (isEnemyHomeSquareFor(sq.getType(), mode, p)) {
                allEnemy.push_back({x, y});
                if (!sq.isCapturedByTeamOf(p)) {
                    uncaptured.push_back({x, y});
                }
            }
        }
    }
    return !uncaptured.empty() ? uncaptured : allEnemy;
}

bool isOrthogonalDir(OctiDir dir)
{
    return dir == DIR_N || dir == DIR_E || dir == DIR_S || dir == DIR_W;
}

bool isDiagonalDir(OctiDir dir)
{
    return dir == DIR_NE || dir == DIR_SE || dir == DIR_SW || dir == DIR_NW;
}

bool hasUnarmedPodOnHomeSquare(const OctiBoard &board, OctiPlayer turn)
{
    OctiGameMode mode = board.getGameMode();
    int w = board.getBoardWidth();
    int h = board.getBoardHeight();
    OctiPodType podType = playerToPodType(turn);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const OctiSquare &sq = board.squareAt(x, y);
            if (!isAlliedHomeSquareFor(sq.getType(), mode, turn)) {
                continue;
            }
            for (int i = 0; i < sq.getNumPods(); ++i) {
                const OctiPod &pod = sq.getPodAt(i);
                if (pod.getType() == podType && pod.getNumProngs() < 2) {
                    return true;
                }
            }
        }
    }
    return false;
}

int torusDist(int a, int b, int dim, bool edgeless)
{
    int d = std::abs(a - b);
    if (edgeless && dim > 0) {
        d = std::min(d, dim - d);
    }
    return d;
}

int chebyshevDist(int x1, int y1, int x2, int y2, int w, int h, bool edgeless)
{
    int dx = torusDist(x1, x2, w, edgeless);
    int dy = torusDist(y1, y2, h, edgeless);
    return std::max(dx, dy);
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
        int dist = chebyshevDist(x, y, t.first, t.second, w, h, edgeless);
        if (dist < best) {
            best = dist;
        }
    }
    return best;
}

// ---------------------------------------------------------------------------
// 2001 Jump-Capture Reachability & Threat Evaluation
// ---------------------------------------------------------------------------

struct JumpMobilityThreat {
    int reachableSquares = 0;
    int capturablePods = 0;
    int capturableProngs = 0;
    std::uint64_t capturedSqMask[2] = {0ULL, 0ULL};
};

void evaluateJumpCaptureRecursive(const OctiBoard &board,
                                  bool edgeless,
                                  int w,
                                  int h,
                                  int x,
                                  int y,
                                  int depth,
                                  OctiPlayer podOwner,
                                  const OctiPod &pod,
                                  std::uint64_t visitedDestMask[2],
                                  JumpMobilityThreat &threat)
{
    if (depth > 4) {
        return;
    }

    for (int d = 0; d < DIR_MAX; ++d) {
        OctiDir dir = static_cast<OctiDir>(d);
        if (pod.prongAt(dir) == NO_PRONG) {
            continue;
        }

        int dx = dirDeltaX(dir);
        int dy = dirDeltaY(dir);

        int nx = edgeless ? (((x + dx) % w + w) % w) : (x + dx);
        int ny = edgeless ? (((y + dy) % h + h) % h) : (y + dy);
        int jx = edgeless ? (((x + 2 * dx) % w + w) % w) : (x + 2 * dx);
        int jy = edgeless ? (((y + 2 * dy) % h + h) % h) : (y + 2 * dy);

        if (!board.isValidCoord(nx, ny) || !board.isValidCoord(jx, jy)) {
            continue;
        }

        const OctiSquare &midSq = board.squareAt(nx, ny);
        const OctiSquare &destSq = board.squareAt(jx, jy);

        if (midSq.isEmpty() || destSq.hasEnemyOf(podOwner)) {
            continue;
        }

        int destIdx = jy * 9 + jx;
        int destWord = (destIdx >> 6) & 1;
        std::uint64_t destBit = 1ULL << (destIdx & 63);
        if ((visitedDestMask[destWord] & destBit) != 0ULL) {
            continue;
        }
        visitedDestMask[destWord] |= destBit;
        threat.reachableSquares++;

        if (midSq.hasEnemyOf(podOwner)) {
            int midIdx = ny * 9 + nx;
            int midWord = (midIdx >> 6) & 1;
            std::uint64_t midBit = 1ULL << (midIdx & 63);
            if ((threat.capturedSqMask[midWord] & midBit) == 0ULL) {
                threat.capturedSqMask[midWord] |= midBit;
                threat.capturablePods += midSq.getNumPods();
                for (int pi = 0; pi < midSq.getNumPods(); ++pi) {
                    const OctiPod &p = midSq.getPodAt(pi);
                    threat.capturableProngs += p.getNumProngs();
                }
            }
        }

        evaluateJumpCaptureRecursive(board, edgeless, w, h, jx, jy, depth + 1,
                                     podOwner, pod, visitedDestMask, threat);
    }
}

JumpMobilityThreat evaluatePodJumpThreat(const OctiBoard &board,
                                        int x,
                                        int y,
                                        const OctiPod &pod)
{
    JumpMobilityThreat threat;
    const OctiGame *g = board.getGame();
    if (!g) {
        return threat;
    }
    bool edgeless = g->useEdgelessBoard();
    int w = board.getBoardWidth();
    int h = board.getBoardHeight();
    OctiPlayer owner = podTypeToPlayer(pod.getType());

    std::uint64_t visitedDestMask[2] = {0ULL, 0ULL};

    evaluateJumpCaptureRecursive(board, edgeless, w, h, x, y, 0,
                                 owner, pod, visitedDestMask, threat);
    return threat;
}

// ---------------------------------------------------------------------------
// Dominated Move Filtering
// ---------------------------------------------------------------------------

std::vector<OctiMove> filterDominatedMoves(const std::vector<OctiMove> &moves)
{
    if (moves.size() <= 1) {
        return moves;
    }

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

// ---------------------------------------------------------------------------
// Composite Multi-Pod Move Generation
// ---------------------------------------------------------------------------

void appendCompositeMoves(OctiBoard &scratchBoard,
                          std::vector<OctiMove> &moves,
                          OctiPlayer turn)
{
    OctiGame *game = scratchBoard.getGame();
    if (!game || moves.empty()) {
        return;
    }

    int w = scratchBoard.getBoardWidth();
    int h = scratchBoard.getBoardHeight();
    OctiPodType friendlyPodType = playerToPodType(turn);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const OctiSquare &sq = scratchBoard.squareAt(x, y);
            if (sq.getNumPods() < 2) {
                continue;
            }

            std::vector<OctiPod> friendlyPods;
            for (int i = 0; i < sq.getNumPods(); ++i) {
                const OctiPod &p = sq.getPodAt(i);
                if (p.getType() == friendlyPodType && p.getNumProngs() > 0) {
                    friendlyPods.push_back(p);
                }
            }

            if (friendlyPods.size() < 2) {
                continue;
            }

            // Group single-pod movement operations from (x, y) by matching pod config
            std::vector<std::vector<OctiMoveOp>> podMoves(friendlyPods.size());
            for (std::size_t k = 0; k < friendlyPods.size(); ++k) {
                std::uint32_t targetCfg = friendlyPods[k].rawConfig() & 0xFFFFu;
                for (const auto &m : moves) {
                    const auto &ops = m.getOps();
                    if (ops.size() == 1 && ops[0].x0 == x && ops[0].y0 == y &&
                        !ops[0].hops.empty()) {
                        if ((ops[0].pod.rawConfig() & 0xFFFFu) == targetCfg) {
                            podMoves[k].push_back(ops[0]);
                        }
                    }
                }
            }

            int validPodsCount = 0;
            for (const auto &pm : podMoves) {
                if (!pm.empty()) {
                    ++validPodsCount;
                }
            }
            if (validPodsCount < 2) {
                continue;
            }

            std::size_t compositeAddedForSquare = 0;
            constexpr std::size_t MAX_COMPOSITE_PER_SQUARE = 24;

            // Generate 2-pod composite moves
            for (std::size_t i = 0; i < friendlyPods.size() && compositeAddedForSquare < MAX_COMPOSITE_PER_SQUARE; ++i) {
                if (podMoves[i].empty()) continue;
                for (std::size_t j = i + 1; j < friendlyPods.size() && compositeAddedForSquare < MAX_COMPOSITE_PER_SQUARE; ++j) {
                    if (podMoves[j].empty()) continue;

                    for (const auto &op1 : podMoves[i]) {
                        if (compositeAddedForSquare >= MAX_COMPOSITE_PER_SQUARE) break;
                        for (const auto &op2 : podMoves[j]) {
                            if (compositeAddedForSquare >= MAX_COMPOSITE_PER_SQUARE) break;

                            // Duplicate capture check across pods
                            bool dupCapture = false;
                            for (const auto &h1 : op1.hops) {
                                if (h1.capture) {
                                    for (const auto &h2 : op2.hops) {
                                        if (h2.capture && h1.x == h2.x && h1.y == h2.y) {
                                            dupCapture = true;
                                            break;
                                        }
                                    }
                                }
                                if (dupCapture) break;
                            }
                            if (dupCapture) {
                                continue;
                            }

                            OctiGameSnapshot snap = game->captureSnapshot();
                            try {
                                OctiMove cand(std::vector<OctiMoveOp>{op1, op2});
                                game->move(cand);
                                moves.push_back(cand);
                                ++compositeAddedForSquare;
                            } catch (...) {
                            }
                            game->restoreSnapshot(snap);
                        }
                    }
                }
            }

            // If 3+ pods with moves, generate 3-pod composite moves
            if (friendlyPods.size() >= 3 && validPodsCount >= 3 &&
                compositeAddedForSquare < MAX_COMPOSITE_PER_SQUARE) {
                for (std::size_t i = 0; i < friendlyPods.size() && compositeAddedForSquare < MAX_COMPOSITE_PER_SQUARE; ++i) {
                    if (podMoves[i].empty()) continue;
                    for (std::size_t j = i + 1; j < friendlyPods.size() && compositeAddedForSquare < MAX_COMPOSITE_PER_SQUARE; ++j) {
                        if (podMoves[j].empty()) continue;
                        for (std::size_t k = j + 1; k < friendlyPods.size() && compositeAddedForSquare < MAX_COMPOSITE_PER_SQUARE; ++k) {
                            if (podMoves[k].empty()) continue;

                            for (const auto &op1 : podMoves[i]) {
                                if (compositeAddedForSquare >= MAX_COMPOSITE_PER_SQUARE) break;
                                for (const auto &op2 : podMoves[j]) {
                                    if (compositeAddedForSquare >= MAX_COMPOSITE_PER_SQUARE) break;
                                    for (const auto &op3 : podMoves[k]) {
                                        if (compositeAddedForSquare >= MAX_COMPOSITE_PER_SQUARE) break;

                                        // Capture conflict check
                                        bool dupCap = false;
                                        std::vector<std::pair<int, int>> capCoords;
                                        for (const auto *op : {&op1, &op2, &op3}) {
                                            for (const auto &hp : op->hops) {
                                                if (hp.capture) {
                                                    std::pair<int, int> c{hp.x, hp.y};
                                                    if (std::find(capCoords.begin(), capCoords.end(), c) != capCoords.end()) {
                                                        dupCap = true;
                                                        break;
                                                    }
                                                    capCoords.push_back(c);
                                                }
                                            }
                                            if (dupCap) break;
                                        }
                                        if (dupCap) continue;

                                        OctiGameSnapshot snap = game->captureSnapshot();
                                        try {
                                            OctiMove cand(std::vector<OctiMoveOp>{op1, op2, op3});
                                            game->move(cand);
                                            moves.push_back(cand);
                                            ++compositeAddedForSquare;
                                        } catch (...) {
                                        }
                                        game->restoreSnapshot(snap);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Move Ordering & Heuristic Scoring
// ---------------------------------------------------------------------------

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
    bool earlyDevelopment = game && game->getFreeProngs(turn) >= 8;

    int score = 0;

    // Prioritize composite moves
    if (move.getOps().size() > 1) {
        score += 8000 + static_cast<int>(move.getOps().size() - 1) * 4000;
    }

    for (const auto &op : move.getOps()) {
        if (!op.hops.empty()) {
            int startDist = minChebyshevToTargets(op.x0, op.y0, enemyTargets, w, h, edgeless);
            const OctiHop &lastHop = op.hops.back();
            int endDist = minChebyshevToTargets(lastHop.x, lastHop.y, enemyTargets, w, h, edgeless);

            score += (startDist - endDist) * 550;

            const OctiSquare &destSq = board.squareAt(lastHop.x, lastHop.y);
            if (isEnemyHomeSquareFor(destSq.getType(), mode, turn) &&
                !destSq.isCapturedByTeamOf(turn)) {
                score += 55000;
            }
            if (destSq.getType() == NEUTRAL_OCTI_SQUARE && destSq.isEmpty()) {
                score += 3000;
            }

            for (const auto &hop : op.hops) {
                if (hop.capture) {
                    score += 14000;
                } else {
                    score += 250;
                }
            }

            const OctiSquare &startSq = board.squareAt(op.x0, op.y0);
            if (isEnemyHomeSquareFor(startSq.getType(), mode, turn)) {
                if (startSq.getNumPods() <= 1 &&
                    !isEnemyHomeSquareFor(destSq.getType(), mode, turn)) {
                    score -= 22000;
                }
            }

            // Avoid stacking multiple friendly pods on non-enemy-base squares
            if (!isEnemyHomeSquareFor(destSq.getType(), mode, turn) &&
                destSq.getNumPods() >= 1) {
                score -= 1200 * destSq.getNumPods();
            }

            // Penalize premature 1-prong lone-pod rushes during development
            if (earlyDevelopment && op.pod.getNumProngs() <= 1 && endDist <= 3) {
                score -= 3200;
            }
        } else if (op.addProngDir != DIR_UNDEF && op.rmProngDir == DIR_UNDEF) {
            score += 650;
            if (op.superProng) {
                score += 200;
            }
            int nx = op.x0 + dirDeltaX(op.addProngDir);
            int ny = op.y0 + dirDeltaY(op.addProngDir);
            int curDist = minChebyshevToTargets(op.x0, op.y0, enemyTargets, w, h, edgeless);
            int nextDist = minChebyshevToTargets(nx, ny, enemyTargets, w, h, edgeless);
            if (nextDist < curDist) {
                score += 600;
            }
            if (op.pod.getNumProngs() == 0) {
                score += 900; // Strongly prioritize arming 0-prong pods first!
            } else if (op.pod.getNumProngs() < 3) {
                score += 450;
            }

            bool hasOrth = isOrthogonalDir(op.addProngDir);
            bool hasDiag = isDiagonalDir(op.addProngDir);
            for (int d = 0; d < DIR_MAX; ++d) {
                OctiDir dir = static_cast<OctiDir>(d);
                if (op.pod.prongAt(dir) != NO_PRONG) {
                    if (isOrthogonalDir(dir)) hasOrth = true;
                    if (isDiagonalDir(dir)) hasDiag = true;
                }
            }
            if (hasOrth && hasDiag) {
                score += 650;
            }

            if (turn == PLAYER_A || turn == PLAYER_C) {
                if (op.addProngDir == DIR_N) {
                    score += 1100;
                } else if (op.addProngDir == DIR_NE || op.addProngDir == DIR_NW) {
                    score += 750;
                } else if (op.addProngDir == DIR_E || op.addProngDir == DIR_W) {
                    score += 400;
                }
            } else if (turn == PLAYER_B || turn == PLAYER_D) {
                if (op.addProngDir == DIR_S) {
                    score += 1100;
                } else if (op.addProngDir == DIR_SE || op.addProngDir == DIR_SW) {
                    score += 750;
                } else if (op.addProngDir == DIR_E || op.addProngDir == DIR_W) {
                    score += 400;
                }
            }
        } else if (op.addPod) {
            if (op.liberate) {
                score += 6500;
            } else {
                const OctiSquare &sq = board.squareAt(op.x0, op.y0);
                if (sq.getNumPods() > 0 || hasUnarmedPodOnHomeSquare(board, turn)) {
                    score -= 6000;
                } else {
                    score += 150;
                }
            }
        } else if (op.rmProngDir != DIR_UNDEF) {
            score += 60;
        }
    }
    return score;
}

std::vector<OctiMove> orderAndCapMoves(OctiBoard &board,
                                       std::vector<OctiMove> &rawMoves,
                                       OctiPlayer turn,
                                       const std::string &ttBestMove,
                                       std::size_t maxBranch,
                                       bool includeComposites = true)
{
    if (includeComposites) {
        appendCompositeMoves(board, rawMoves, turn);
    }
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

// ---------------------------------------------------------------------------
// Construction & Difficulty Levels
// ---------------------------------------------------------------------------

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

int OctiBotEngine::defaultBudgetForDifficulty(BotDifficulty d)
{
    switch (d) {
    case BotDifficulty::CASUAL:   return 3000;
    case BotDifficulty::TACTICAL: return 8000;
    case BotDifficulty::MASTER:   return 15000;
    }
    return 8000;
}

int OctiBotEngine::maxDepthForDifficulty(BotDifficulty d)
{
    switch (d) {
    case BotDifficulty::CASUAL:   return 5;
    case BotDifficulty::TACTICAL: return 7;
    case BotDifficulty::MASTER:   return 9;
    }
    return 7;
}

int OctiBotEngine::rootCapForDifficulty(BotDifficulty d)
{
    switch (d) {
    case BotDifficulty::CASUAL:   return 28;
    case BotDifficulty::TACTICAL: return 40;
    case BotDifficulty::MASTER:   return 56;
    }
    return 40;
}

int OctiBotEngine::branchCapForDifficulty(BotDifficulty d)
{
    switch (d) {
    case BotDifficulty::CASUAL:   return 18;
    case BotDifficulty::TACTICAL: return 24;
    case BotDifficulty::MASTER:   return 32;
    }
    return 24;
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

// ---------------------------------------------------------------------------
// Fast Deterministic 64-bit Zobrist Hashing (Zero Heap Allocations)
// ---------------------------------------------------------------------------

std::uint64_t OctiBotEngine::computePositionHash(const OctiBoard &board) const
{
    std::uint64_t h = 0;
    const OctiGame *game = board.getGame();
    if (!game) {
        return h;
    }

    int w = board.getBoardWidth();
    int hgt = board.getBoardHeight();
    OctiGameMode mode = board.getGameMode();
    h ^= g_zobrist.mode[static_cast<int>(mode) % 4];
    h ^= g_zobrist.turn[static_cast<int>(game->getTurn()) % 5];

    for (int y = 0; y < hgt; ++y) {
        for (int x = 0; x < w; ++x) {
            int sqIdx = y * 9 + x;
            if (sqIdx >= 81) break;
            const OctiSquare &sq = board.squareAt(x, y);
            if (sq.isCaptured()) {
                for (int pl = 1; pl <= 4; ++pl) {
                    if (sq.isCapturedBy(static_cast<OctiPlayer>(pl))) {
                        h ^= g_zobrist.baseCaptured[sqIdx][pl];
                        break;
                    }
                }
            }
            int numPods = std::min(7, sq.getNumPods());
            for (int p = 0; p < numPods; ++p) {
                const OctiPod &pod = sq.getPodAt(p);
                int pType = static_cast<int>(pod.getType()) - 1;
                if (pType >= 0 && pType < 4) {
                    h ^= g_zobrist.piece[sqIdx][p][pType];
                }
                for (int d = 0; d < 8; ++d) {
                    OctiProngType pt = pod.prongAt(static_cast<OctiDir>(d));
                    int ptIdx = static_cast<int>(pt);
                    if (ptIdx >= 0 && ptIdx < 3) {
                        h ^= g_zobrist.prong[sqIdx][p][d][ptIdx];
                    }
                }
            }
        }
    }

    int numPlayers = (mode == GAME_MODE_4) ? 4 : 2;
    for (int pl = 1; pl <= numPlayers; ++pl) {
        OctiPlayer p = static_cast<OctiPlayer>(pl);
        int fp = std::min(7, std::max(0, game->getFreePods(p)));
        int cp = std::min(7, std::max(0, game->getCapturedPods(p)));
        int fpr = std::min(32, std::max(0, game->getFreeProngs(p)));
        int fspr = std::min(8, std::max(0, game->getFreeSuperProngs(p)));
        h ^= g_zobrist.freePods[pl][fp];
        h ^= g_zobrist.capturedPods[pl][cp];
        h ^= g_zobrist.freeProngs[pl][fpr];
        h ^= g_zobrist.freeSuperProngs[pl][fspr];
    }

    return h;
}

// ---------------------------------------------------------------------------
// Hybrid 2001 + 2026 Evaluation Function
// ---------------------------------------------------------------------------

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
    int totalAllyPodsOnBoard = 0;
    int totalEnemyPodsOnBoard = 0;
    bool allyHasUnarmedHomePod = false;
    bool enemyHasUnarmedHomePod = false;

    std::vector<std::pair<int, int>> allyPodCoords;
    std::vector<std::pair<int, int>> enemyPodCoords;
    std::uint64_t allyJumpThreatMask[2] = {0ULL, 0ULL};
    std::uint64_t enemyJumpThreatMask[2] = {0ULL, 0ULL};

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const OctiSquare &sq = board.squareAt(x, y);
            OctiSquareType sqType = sq.getType();

            if (isEnemyHomeSquareFor(sqType, mode, perspective)) {
                if (sq.isCapturedByTeamOf(perspective)) {
                    totalScore += 28000;
                }
            } else if (isAlliedHomeSquareFor(sqType, mode, perspective)) {
                if (sq.isCaptured() && !sq.isCapturedByTeamOf(perspective)) {
                    totalScore -= 32000;
                }
            } else if (sqType == NEUTRAL_OCTI_SQUARE) {
                if (sq.isCapturedByTeamOf(perspective)) {
                    totalScore += 4000;
                } else if (sq.isCaptured()) {
                    totalScore -= 4000;
                }
            }

            int numPods = sq.getNumPods();
            if (numPods > 0) {
                OctiPlayer sqOwner = podTypeToPlayer(sq.getPodAt(0).getType());
                bool sqAlly = areAlliedPlayers(mode, sqOwner, perspective);
                if (sqAlly) {
                    totalAllyPodsOnBoard += numPods;
                } else {
                    totalEnemyPodsOnBoard += numPods;
                }

                bool onCapturedEnemyBase = isEnemyHomeSquareFor(sqType, mode, sqOwner) &&
                                           sq.isCapturedByTeamOf(sqOwner);
                bool onAlliedHome = isAlliedHomeSquareFor(sqType, mode, sqOwner);
                int unarmedCount = 0;
                for (int i = 0; i < numPods; ++i) {
                    const OctiPod &p = sq.getPodAt(i);
                    if (p.getNumProngs() == 0) {
                        ++unarmedCount;
                    }
                    if (onAlliedHome && p.getNumProngs() < 2) {
                        if (sqAlly) {
                            allyHasUnarmedHomePod = true;
                        } else {
                            enemyHasUnarmedHomePod = true;
                        }
                    }
                }

                if (numPods > 1 && !onCapturedEnemyBase) {
                    int stackPen = (numPods - 1) * 450 + unarmedCount * 950;
                    totalScore += sqAlly ? -stackPen : stackPen;
                }
            }

            for (int i = 0; i < numPods; ++i) {
                const OctiPod &pod = sq.getPodAt(i);
                OctiPlayer owner = podTypeToPlayer(pod.getType());
                bool isAlly = areAlliedPlayers(mode, owner, perspective);
                const auto &targets = isAlly ? allyTargets : enemyTargets;
                int numProngs = pod.getNumProngs();

                if (numProngs > 0) {
                    if (isAlly) {
                        allyPodCoords.push_back({x, y});
                    } else {
                        enemyPodCoords.push_back({x, y});
                    }
                }

                int dist = minChebyshevToTargets(x, y, targets, w, h, edgeless);
                bool onCapturedEnemyBase = isEnemyHomeSquareFor(sqType, mode, owner) &&
                                           sq.isCapturedByTeamOf(owner);

                int podVal = 0;
                if (numProngs == 0 && !onCapturedEnemyBase && sqType != NEUTRAL_OCTI_SQUARE) {
                    podVal = 320;
                } else if (numProngs == 1) {
                    podVal = 950 + std::max(0, (10 - dist)) * 110;
                    if (!isAlliedHomeSquareFor(sqType, mode, owner) && dist <= 4 && dist >= 2) {
                        podVal -= 550;
                    }
                } else if (numProngs == 2) {
                    podVal = 1450 + std::max(0, (10 - dist)) * 180;
                } else {
                    podVal = 1750 + std::max(0, (10 - dist)) * 210;
                }

                bool hasOrth = false;
                bool hasDiag = false;

                for (int d = 0; d < DIR_MAX; ++d) {
                    OctiDir dir = static_cast<OctiDir>(d);
                    OctiProngType pt = pod.prongAt(dir);
                    if (pt == NO_PRONG) {
                        continue;
                    }
                    podVal += (pt == SUPER_PRONG) ? 350 : 220;
                    if (isOrthogonalDir(dir)) hasOrth = true;
                    if (isDiagonalDir(dir)) hasDiag = true;

                    if (owner == PLAYER_A || owner == PLAYER_C) {
                        if (dir == DIR_N) {
                            podVal += 380;
                        } else if (dir == DIR_NE || dir == DIR_NW) {
                            podVal += 260;
                        } else if (dir == DIR_E || dir == DIR_W) {
                            podVal += 120;
                        }
                    } else if (owner == PLAYER_B || owner == PLAYER_D) {
                        if (dir == DIR_S) {
                            podVal += 380;
                        } else if (dir == DIR_SE || dir == DIR_SW) {
                            podVal += 260;
                        } else if (dir == DIR_E || dir == DIR_W) {
                            podVal += 120;
                        }
                    }

                    int nx = edgeless ? (((x + dirDeltaX(dir)) % w + w) % w) : (x + dirDeltaX(dir));
                    int ny = edgeless ? (((y + dirDeltaY(dir)) % h + h) % h) : (y + dirDeltaY(dir));
                    if (board.isValidCoord(nx, ny)) {
                        int ndist = minChebyshevToTargets(nx, ny, targets, w, h, edgeless);
                        if (ndist < dist) {
                            podVal += 140;
                        }
                        if (ndist == 0 && !board.squareAt(nx, ny).hasEnemyOf(owner)) {
                            podVal += 8500;
                        }
                    }

                    int jx = edgeless ? (((x + 2 * dirDeltaX(dir)) % w + w) % w) : (x + 2 * dirDeltaX(dir));
                    int jy = edgeless ? (((y + 2 * dirDeltaY(dir)) % h + h) % h) : (y + 2 * dirDeltaY(dir));
                    if (board.isValidCoord(nx, ny) && board.isValidCoord(jx, jy)) {
                        if (!board.squareAt(nx, ny).isEmpty() &&
                            !board.squareAt(jx, jy).hasEnemyOf(owner)) {
                            int jdist = minChebyshevToTargets(jx, jy, targets, w, h, edgeless);
                            if (jdist == 0) {
                                podVal += 12000;
                            }
                        }
                    }
                }

                if (hasOrth && hasDiag) {
                    podVal += 550;
                } else if (numProngs >= 2 && hasDiag && !hasOrth) {
                    podVal -= 350;
                }

                if (numProngs > 0) {
                    JumpMobilityThreat threat = evaluatePodJumpThreat(board, x, y, pod);
                    podVal += threat.reachableSquares * 55;
                    podVal += threat.capturablePods * 1100;
                    podVal += threat.capturableProngs * 180;
                    if (isAlly) {
                        allyJumpThreatMask[0] |= threat.capturedSqMask[0];
                        allyJumpThreatMask[1] |= threat.capturedSqMask[1];
                    } else {
                        enemyJumpThreatMask[0] |= threat.capturedSqMask[0];
                        enemyJumpThreatMask[1] |= threat.capturedSqMask[1];
                    }
                }

                totalScore += isAlly ? podVal : -podVal;
            }
        }
    }

    // Hanging-Pod Vulnerability Penalty (squares that the opponent can jump over and capture)
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const OctiSquare &sq = board.squareAt(x, y);
            int numPods = sq.getNumPods();
            if (numPods == 0) {
                continue;
            }
            int sqBit = y * 9 + x;
            int wordIdx = (sqBit >> 6) & 1;
            std::uint64_t bitMask = 1ULL << (sqBit & 63);
            OctiPlayer sqOwner = podTypeToPlayer(sq.getPodAt(0).getType());
            bool sqAlly = areAlliedPlayers(mode, sqOwner, perspective);

            int prongsOnSq = 0;
            for (int i = 0; i < numPods; ++i) {
                prongsOnSq += sq.getPodAt(i).getNumProngs();
            }

            if (sqAlly && (enemyJumpThreatMask[wordIdx] & bitMask) != 0ULL) {
                int vulnPen = numPods * 1800 + prongsOnSq * 250;
                if (numPods >= totalAllyPodsOnBoard) {
                    vulnPen += 15000;
                }
                totalScore -= vulnPen;
            } else if (!sqAlly && (allyJumpThreatMask[wordIdx] & bitMask) != 0ULL) {
                int vulnPen = numPods * 1800 + prongsOnSq * 250;
                if (numPods >= totalEnemyPodsOnBoard) {
                    vulnPen += 15000;
                }
                totalScore += vulnPen;
            }
        }
    }

    // 2001 Multi-Base Minimum Chebyshev Distance Sum across uncaptured enemy bases
    if (!allyTargets.empty()) {
        int allyDistSum = 0;
        for (const auto &tb : allyTargets) {
            const OctiSquare &tsq = board.squareAt(tb.first, tb.second);
            if (!tsq.isCapturedByTeamOf(perspective)) {
                int bestDist = 999;
                for (const auto &ap : allyPodCoords) {
                    int d = chebyshevDist(ap.first, ap.second, tb.first, tb.second, w, h, edgeless);
                    if (d < bestDist) {
                        bestDist = d;
                    }
                }
                allyDistSum += (bestDist < 999) ? bestDist : 8;
            }
        }
        totalScore += std::max(0, 30 - allyDistSum) * 320;
    }

    if (!enemyTargets.empty()) {
        int enemyDistSum = 0;
        for (const auto &tb : enemyTargets) {
            const OctiSquare &tsq = board.squareAt(tb.first, tb.second);
            if (!tsq.isCapturedByTeamOf(sampleEnemy)) {
                int bestDist = 999;
                for (const auto &ep : enemyPodCoords) {
                    int d = chebyshevDist(ep.first, ep.second, tb.first, tb.second, w, h, edgeless);
                    if (d < bestDist) {
                        bestDist = d;
                    }
                }
                enemyDistSum += (bestDist < 999) ? bestDist : 8;
            }
        }
        totalScore -= std::max(0, 30 - enemyDistSum) * 320;
    }

    // Reserves and material bonuses
    int numPlayers = (mode == GAME_MODE_4) ? 4 : 2;
    for (int pIdx = 1; pIdx <= numPlayers; ++pIdx) {
        OctiPlayer p = static_cast<OctiPlayer>(pIdx);
        bool isAlly = areAlliedPlayers(mode, p, perspective);
        int freePods = game->getFreePods(p);
        int resVal = freePods * 450
                   - game->getCapturedPods(p) * 380
                   + game->getFreeProngs(p) * 50
                   + game->getFreeSuperProngs(p) * 150;
        if (freePods < 4 && (isAlly ? allyHasUnarmedHomePod : enemyHasUnarmedHomePod) &&
            game->getFreeProngs(p) >= 6) {
            resVal -= (4 - freePods) * 1500;
        }
        if (mode == GAME_MODE_4 && game->isPlayerEliminated(p)) {
            resVal -= 20000;
        }
        if (board.canLiberate(p) && game->getCapturedPods(p) > 0) {
            resVal += 2500;
        }
        totalScore += isAlly ? resVal : -resVal;
    }

    return totalScore;
}

// ---------------------------------------------------------------------------
// Negamax Search with Transposition Table
// ---------------------------------------------------------------------------

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
    std::size_t ttIdx = hashKey % TT_SIZE;
    std::string ttMove;
    {
        std::lock_guard<std::mutex> lock(g_ttMutexes[ttIdx % TT_MUTEX_COUNT]);
        const TranspositionEntry &ttEntry = _tt[ttIdx];
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
    }

    std::vector<OctiMove> rawMoves = game->generateLegalMoves();
    if (rawMoves.empty()) {
        return evaluatePosition(scratchBoard, currentPerspective);
    }

    OctiPlayer turn = game->getTurn();
    std::size_t maxBranch = branchCapForDifficulty(_difficulty);
    std::vector<OctiMove> ordered = orderAndCapMoves(
        scratchBoard, rawMoves, turn, ttMove, maxBranch, (ply <= 1 && depth >= 2));

    int origAlpha = alpha;
    int bestVal = -INF_SCORE;
    std::string bestMoveStr = ordered[0].asString();
    OctiGameSnapshot snap = game->captureSnapshot();

    for (const auto &mv : ordered) {
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
        std::lock_guard<std::mutex> lock(g_ttMutexes[ttIdx % TT_MUTEX_COUNT]);
        TranspositionEntry &ttEntry = _tt[ttIdx];
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

// ---------------------------------------------------------------------------
// chooseMove with Multi-Core Lazy SMP
// ---------------------------------------------------------------------------

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

    const std::string rootFen = board.toPositionString();
    OctiBoard scratchBoard;
    scratchBoard.suppressBoardUpdateNotification(true);
    scratchBoard.fromPositionString(rootFen);
    OctiGame *game = scratchBoard.getGame();
    if (!game) {
        return "";
    }
    game->setSearchMode(true);

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

    std::size_t rootCap = rootCapForDifficulty(_difficulty);
    std::vector<OctiMove> rootCandidates = orderAndCapMoves(
        scratchBoard, legalMoves, rootTurn, "", rootCap);

    std::string bestMoveStr = rootCandidates[0].asString();
    int bestScore = -INF_SCORE;
    int maxTargetDepth = maxDepthForDifficulty(_difficulty);
    int completedDepth = 0;

    // Lazy SMP: spawn helper threads if budget >= 500ms and difficulty >= TACTICAL
    std::atomic<bool> stopFlag{false};
    std::vector<std::thread> helpers;

    if (_timeBudgetMs >= 500 && _difficulty >= BotDifficulty::TACTICAL) {
        unsigned int hw = std::thread::hardware_concurrency();
        int numThreads = std::min(4, static_cast<int>(hw > 0 ? hw : 2));
        for (int t = 1; t < numThreads; ++t) {
            int depthOffset = (t % 2 == 1) ? 1 : 2;
            helpers.emplace_back([this, rootFen, &stopFlag, deadlineMs, depthOffset, maxTargetDepth, rootTurn]() {
                OctiBoard threadBoard;
                threadBoard.suppressBoardUpdateNotification(true);
                threadBoard.fromPositionString(rootFen);
                OctiGame *tGame = threadBoard.getGame();
                if (!tGame) return;
                tGame->setSearchMode(true);
                std::uint64_t tNodes = 0;

                for (int d = 1 + depthOffset; d <= maxTargetDepth && !stopFlag.load(); ++d) {
                    if (steadyClockMs() >= deadlineMs || stopFlag.load()) break;
                    bool tTimedOut = false;
                    negamax(threadBoard, d, 0, -INF_SCORE, INF_SCORE,
                            rootTurn, tNodes, deadlineMs, tTimedOut);
                    if (tTimedOut || steadyClockMs() >= deadlineMs) break;
                }
            });
        }
    }

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

            if (timedOut && d > 1) {
                break;
            }

            if (_difficulty == BotDifficulty::CASUAL &&
                std::abs(score) < WIN_SCORE / 2) {
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
        }

        if (!timedOut || d == 1) {
            bestMoveStr = iterBestMove;
            bestScore = iterBestScore;
            completedDepth = d;
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

    stopFlag.store(true);
    for (auto &hTh : helpers) {
        if (hTh.joinable()) {
            hTh.join();
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
