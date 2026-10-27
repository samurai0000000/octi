/*
 * OctiMoveComposer.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiMoveComposer.hxx"
#include <algorithm>
#include <cmath>
#include <set>

namespace octi {

namespace {

bool isOfkBackwardDirection(OctiPlayer turn, OctiDir dir)
{
    if (turn == PLAYER_A) {
        return dirDeltaY(dir) < 0;
    }
    if (turn == PLAYER_B) {
        return dirDeltaY(dir) > 0;
    }
    return false;
}

bool computeDisplacement(int cx, int cy, int tx, int ty,
                         bool edgeless, int width, int height,
                         int &outDx, int &outDy)
{
    if (!edgeless) {
        outDx = tx - cx;
        outDy = ty - cy;
        return true;
    }

    bool foundX = false;
    int bestDx = 0;
    for (int d = -2; d <= 2; ++d) {
        if (((cx + d) % width + width) % width == tx) {
            bestDx = d;
            foundX = true;
            break;
        }
    }
    if (!foundX) {
        return false;
    }

    bool foundY = false;
    int bestDy = 0;
    for (int d = -2; d <= 2; ++d) {
        if (((cy + d) % height + height) % height == ty) {
            bestDy = d;
            foundY = true;
            break;
        }
    }
    if (!foundY) {
        return false;
    }

    outDx = bestDx;
    outDy = bestDy;
    return true;
}

bool findJumpPathBfs(const OctiBoard *board,
                     bool edgeless,
                     OctiPlayer turn,
                     int startX, int startY,
                     int curX, int curY,
                     int targetX, int targetY,
                     const OctiPod &pod,
                     const std::vector<OctiPod> &ghostPods,
                     const std::vector<std::pair<int, int>> &initialJumped,
                     std::vector<OctiHop> &outHops,
                     std::vector<std::pair<int, int>> &outJumpedCoords)
{
    int w = board->getBoardWidth();
    int h = board->getBoardHeight();

    struct Node {
        int x;
        int y;
        std::vector<OctiHop> hops;
        std::vector<std::pair<int, int>> jumped;
    };

    std::vector<Node> queue;
    queue.push_back({curX, curY, {}, initialJumped});

    for (std::size_t head = 0; head < queue.size() && queue.size() < 256; ++head) {
        Node cur = queue[head];
        for (int d = DIR_N; d <= DIR_NW; ++d) {
            OctiDir dir = static_cast<OctiDir>(d);
            if (pod.prongAt(dir) == NO_PRONG) {
                continue;
            }
            int jdx = dirDeltaX(dir);
            int jdy = dirDeltaY(dir);
            int dx = jdx * 2;
            int dy = jdy * 2;

            int jx = edgeless ? (((cur.x + jdx) % w + w) % w) : (cur.x + jdx);
            int jy = edgeless ? (((cur.y + jdy) % h + h) % h) : (cur.y + jdy);
            int lx = edgeless ? (((cur.x + dx) % w + w) % w) : (cur.x + dx);
            int ly = edgeless ? (((cur.y + dy) % h + h) % h) : (cur.y + dy);

            if (!edgeless) {
                if (jx < 0 || jx >= w || jy < 0 || jy >= h) continue;
                if (lx < 0 || lx >= w || ly < 0 || ly >= h) continue;
            }

            std::pair<int, int> jCoord(jx, jy);
            if (std::find(cur.jumped.begin(), cur.jumped.end(), jCoord) != cur.jumped.end()) {
                continue;
            }

            const OctiSquare &jumpedSq = board->squareAt(jx, jy);
            bool jumpedEmpty = (jx == startX && jy == startY)
                ? jumpedSq.isEmptyWithout(ghostPods)
                : jumpedSq.isEmpty();
            if (jumpedEmpty) {
                continue;
            }

            const OctiSquare &landSq = board->squareAt(lx, ly);
            bool landEmpty = (lx == startX && ly == startY)
                ? landSq.isEmptyWithout(ghostPods)
                : landSq.isEmpty();
            if (!landEmpty) {
                continue;
            }

            OctiHop hop;
            hop.x = lx;
            hop.y = ly;
            hop.capture = jumpedSq.hasEnemyOf(turn);

            std::vector<OctiHop> nextHops = cur.hops;
            nextHops.push_back(hop);
            std::vector<std::pair<int, int>> nextJumped = cur.jumped;
            nextJumped.push_back(jCoord);

            if (lx == targetX && ly == targetY) {
                outHops = nextHops;
                outJumpedCoords = nextJumped;
                return true;
            }

            queue.push_back({lx, ly, std::move(nextHops), std::move(nextJumped)});
        }
    }

    return false;
}

} // namespace

OctiMoveComposer::OctiMoveComposer(OctiBoard *board)
    : _board(board)
    , _confirmAllMoves(true)
    , _moveEntireMatchingStack(false)
    , _awaitingConfirmation(false)
    , _awaitingSuperProngBonusMove(false)
    , _activeJumpHasChain(false)
    , _activeJumpStartBx(-1)
    , _activeJumpStartBy(-1)
    , _activeJumpCurBx(-1)
    , _activeJumpCurBy(-1)
    , _hasEighthProngBonusHop(false)
    , _eighthProngTargetBx(-1)
    , _eighthProngTargetBy(-1)
    , _guidedMoveActive(false)
    , _guidedMidHop(false)
    , _guidedOriginBx(-1)
    , _guidedOriginBy(-1)
    , _guidedCurBx(-1)
    , _guidedCurBy(-1)
    , _guidedPodIndex(-1)
    , _guidedMoveAllStack(false)
{
    refreshLegalMoves();
}

OctiMoveComposer::~OctiMoveComposer() = default;

void OctiMoveComposer::attachBoard(OctiBoard *board)
{
    _board = board;
    _selectedStackPods.clear();
    cancelAndRollback();
}

OctiBoard *OctiMoveComposer::board() const
{
    return _board;
}

void OctiMoveComposer::setConfirmAllMoves(bool confirmAll)
{
    _confirmAllMoves = confirmAll;
}

bool OctiMoveComposer::confirmAllMoves() const
{
    return _confirmAllMoves;
}

void OctiMoveComposer::setMoveCommittedCallback(
    std::function<void(const std::string &moveNotation)> cb)
{
    _onMoveCommitted = std::move(cb);
}

void OctiMoveComposer::refreshLegalMoves()
{
    _cachedLegalMoves.clear();
    if (_board && _board->getGame() &&
        _board->getGame()->isGameStarted() &&
        !_board->getGame()->isGameOver()) {
        _board->getGame()->generateLegalMoves(_cachedLegalMoves);
    }
}

void OctiMoveComposer::selectStackPodIndex(int x, int y, int podIndex)
{
    if (!_board || !_board->isValidCoord(x - 1, y - 1)) {
        return;
    }
    const OctiSquare &sq = _board->squareAt(x - 1, y - 1);
    if (podIndex < 0 || podIndex >= sq.getNumPods()) {
        _selectedStackPods.erase({x, y});
        return;
    }
    _selectedStackPods[{x, y}] = podIndex;
}

int OctiMoveComposer::selectedStackPodIndex(int x, int y) const
{
    if (!_board || !_board->isValidCoord(x - 1, y - 1)) {
        return -1;
    }
    const OctiSquare &sq = _board->squareAt(x - 1, y - 1);
    if (sq.isEmpty()) {
        return -1;
    }
    auto it = _selectedStackPods.find({x, y});
    if (it != _selectedStackPods.end() &&
        it->second >= 0 && it->second < sq.getNumPods()) {
        return it->second;
    }
    return sq.getNumPods() - 1;
}

void OctiMoveComposer::setMoveEntireMatchingStack(bool moveAll)
{
    _moveEntireMatchingStack = moveAll;
}

bool OctiMoveComposer::moveEntireMatchingStack() const
{
    return _moveEntireMatchingStack;
}

bool OctiMoveComposer::isPodClaimed(int bx, int by, int podIdx) const
{
    int key = bx * 10 + by;
    return std::find(_claimedStartPods.begin(), _claimedStartPods.end(),
                     std::make_pair(key, podIdx)) != _claimedStartPods.end();
}

int OctiMoveComposer::resolvePodIndex(const OctiSquare &sq,
                                      int x, int y,
                                      int requestedIndex,
                                      OctiPlayer turn,
                                      bool excludeClaimed) const
{
    int bx = x - 1;
    int by = y - 1;
    OctiPodType desiredType = playerToPodType(turn);

    if (requestedIndex >= 0 && requestedIndex < sq.getNumPods()) {
        if (sq.getPodAt(requestedIndex).getType() == desiredType &&
            (!excludeClaimed || !isPodClaimed(bx, by, requestedIndex))) {
            return requestedIndex;
        }
        return -1;
    }

    auto it = _selectedStackPods.find({x, y});
    if (it != _selectedStackPods.end()) {
        int sel = it->second;
        if (sel >= 0 && sel < sq.getNumPods() &&
            sq.getPodAt(sel).getType() == desiredType &&
            (!excludeClaimed || !isPodClaimed(bx, by, sel))) {
            return sel;
        }
    }

    for (int i = sq.getNumPods() - 1; i >= 0; --i) {
        if (sq.getPodAt(i).getType() == desiredType &&
            (!excludeClaimed || !isPodClaimed(bx, by, i))) {
            return i;
        }
    }

    return -1;
}

bool OctiMoveComposer::validateOpsOnScratch(const std::vector<OctiMoveOp> &ops) const
{
    if (!_board || !_board->getGame() || ops.empty()) {
        return false;
    }
    try {
        OctiMove mv(ops);
        OctiBoard scratch;
        scratch.suppressBoardUpdateNotification(true);
        scratch.fromPositionString(_board->toPositionString());
        scratch.getGame()->move(mv);
        return true;
    } catch (...) {
        return false;
    }
}

bool OctiMoveComposer::hasContinuationJumpFrom(
    int bx, int by,
    const OctiPod &pod,
    int startBx, int startBy,
    const std::vector<OctiPod> &ghostPods,
    const std::vector<std::pair<int, int>> &alreadyJumped) const
{
    if (!_board || !_board->getGame()) {
        return false;
    }
    int w = _board->getBoardWidth();
    int h = _board->getBoardHeight();
    bool edgeless = _board->getGame()->useEdgelessBoard();

    for (int d = DIR_N; d <= DIR_NW; ++d) {
        OctiDir dir = static_cast<OctiDir>(d);
        if (pod.prongAt(dir) == NO_PRONG) {
            continue;
        }
        int jdx = dirDeltaX(dir);
        int jdy = dirDeltaY(dir);
        int dx = jdx * 2;
        int dy = jdy * 2;

        int jx = edgeless ? (((bx + jdx) % w + w) % w) : (bx + jdx);
        int jy = edgeless ? (((by + jdy) % h + h) % h) : (by + jdy);
        int lx = edgeless ? (((bx + dx) % w + w) % w) : (bx + dx);
        int ly = edgeless ? (((by + dy) % h + h) % h) : (by + dy);

        if (!edgeless) {
            if (jx < 0 || jx >= w || jy < 0 || jy >= h) continue;
            if (lx < 0 || lx >= w || ly < 0 || ly >= h) continue;
        }

        if (std::find(alreadyJumped.begin(), alreadyJumped.end(),
                      std::make_pair(jx, jy)) != alreadyJumped.end()) {
            continue;
        }

        const OctiSquare &jumpedSq = _board->squareAt(jx, jy);
        bool jumpedEmpty = (jx == startBx && jy == startBy)
            ? jumpedSq.isEmptyWithout(ghostPods)
            : jumpedSq.isEmpty();
        if (jumpedEmpty) {
            continue;
        }

        const OctiSquare &landSq = _board->squareAt(lx, ly);
        bool landEmpty = (lx == startBx && ly == startBy)
            ? landSq.isEmptyWithout(ghostPods)
            : landSq.isEmpty();
        if (!landEmpty) {
            continue;
        }

        return true;
    }

    return false;
}

bool OctiMoveComposer::hasOtherMovableFriendlyPods(int bx, int by, OctiPlayer turn) const
{
    if (!_board || !_board->isValidCoord(bx, by)) {
        return false;
    }
    const OctiSquare &sq = _board->squareAt(bx, by);
    OctiPodType desiredType = playerToPodType(turn);

    for (int i = 0; i < sq.getNumPods(); ++i) {
        if (isPodClaimed(bx, by, i)) {
            continue;
        }
        const OctiPod &pod = sq.getPodAt(i);
        if (pod.getType() != desiredType) {
            continue;
        }
        if (pod.getNumProngs() > 0) {
            return true;
        }
    }
    return false;
}

void OctiMoveComposer::syncPreparedStateToGame()
{
    if (!_board || !_board->getGame()) {
        return;
    }
    OctiGame *game = _board->getGame();
    game->rollbackPreparedMove();

    for (const auto &op : _stagedOps) {
        if (op.addPod) {
            game->prepareAddPod(op.x0, op.y0);
        } else if (op.addProngDir != DIR_UNDEF && op.rmProngDir == DIR_UNDEF) {
            game->prepareAddProng(op.x0, op.y0, op.pod, op.addProngDir, op.superProng);
        } else if (op.addProngDir != DIR_UNDEF && op.rmProngDir != DIR_UNDEF) {
            game->prepareReposition(op.x0, op.y0, op.pod, op.rmProngDir, op.addProngDir);
        } else if (!op.hops.empty()) {
            game->prepareHops(op.x0, op.y0, op.pod, op.hops);
        }
    }
}

bool OctiMoveComposer::stageOrCommitAddPod(int targetX, int targetY, bool liberate)
{
    if (!_board || !_board->getGame()) {
        return false;
    }
    OctiGame *game = _board->getGame();
    if (!game->isGameStarted() || game->isGameOver()) {
        return false;
    }

    int bx = targetX - 1;
    int by = targetY - 1;
    if (!_board->isValidCoord(bx, by)) {
        return false;
    }

    // Cannot combine pod placement with other staged actions
    if (!_stagedOps.empty()) {
        return false;
    }

    OctiPlayer turn = game->getTurn();
    if (liberate) {
        if (game->getCapturedPods(turn) <= 0 || !_board->canLiberate(turn)) {
            return false;
        }
    } else {
        if (game->getFreePods(turn) <= 0) {
            return false;
        }
    }

    OctiMoveOp op;
    op.x0 = bx;
    op.y0 = by;
    op.addPod = true;
    op.liberate = liberate;

    if (!validateOpsOnScratch({op})) {
        return false;
    }

    _stagedOps = {op};
    _awaitingConfirmation = true;
    syncPreparedStateToGame();

    if (!_confirmAllMoves) {
        return commitStagedMove();
    }
    return true;
}

bool OctiMoveComposer::stageOrCommitAddProng(int x, int y,
                                             int podIndex,
                                             OctiDir dir,
                                             bool superProng)
{
    if (!_board || !_board->getGame()) {
        return false;
    }
    OctiGame *game = _board->getGame();
    if (!game->isGameStarted() || game->isGameOver()) {
        return false;
    }
    if (dir < DIR_N || dir > DIR_NW) {
        return false;
    }

    int bx = x - 1;
    int by = y - 1;
    if (!_board->isValidCoord(bx, by)) {
        return false;
    }

    OctiPlayer turn = game->getTurn();
    if (_board->getGameMode() == GAME_MODE_OFK) {
        if (superProng || isOfkBackwardDirection(turn, dir)) {
            return false;
        }
    }

    if (!_stagedOps.empty()) {
        return false;
    }

    const OctiSquare &sq = _board->squareAt(bx, by);
    int resolvedIdx = resolvePodIndex(sq, x, y, podIndex, turn, false);
    if (resolvedIdx < 0) {
        return false;
    }

    const OctiPod &pod = sq.getPodAt(resolvedIdx);
    if (pod.prongAt(dir) != NO_PRONG) {
        return false;
    }

    OctiMoveOp op;
    op.x0 = bx;
    op.y0 = by;
    op.pod = pod;
    op.addProngDir = dir;
    op.superProng = superProng;

    if (!validateOpsOnScratch({op})) {
        return false;
    }

    _stagedOps = {op};
    _awaitingSuperProngBonusMove = (superProng && pod.getNumProngs() == 7);
    _awaitingConfirmation = true;
    syncPreparedStateToGame();

    if (!_confirmAllMoves && !_awaitingSuperProngBonusMove) {
        return commitStagedMove();
    }
    return true;
}

bool OctiMoveComposer::stageOrCommitRepositionProng(int x, int y,
                                                    int podIndex,
                                                    OctiDir srcDir,
                                                    OctiDir tgtDir)
{
    if (!_board || !_board->getGame()) {
        return false;
    }
    OctiGame *game = _board->getGame();
    if (!game->isGameStarted() || game->isGameOver()) {
        return false;
    }
    if (_board->getGameMode() == GAME_MODE_OFK) {
        return false;
    }
    if (srcDir < DIR_N || srcDir > DIR_NW ||
        tgtDir < DIR_N || tgtDir > DIR_NW ||
        srcDir == tgtDir) {
        return false;
    }
    if (!_stagedOps.empty()) {
        return false;
    }

    int bx = x - 1;
    int by = y - 1;
    if (!_board->isValidCoord(bx, by)) {
        return false;
    }

    OctiPlayer turn = game->getTurn();
    const OctiSquare &sq = _board->squareAt(bx, by);
    int resolvedIdx = resolvePodIndex(sq, x, y, podIndex, turn, false);
    if (resolvedIdx < 0) {
        return false;
    }

    const OctiPod &pod = sq.getPodAt(resolvedIdx);
    if (pod.prongAt(srcDir) != NORMAL_PRONG || pod.prongAt(tgtDir) != NO_PRONG) {
        return false;
    }

    OctiMoveOp op;
    op.x0 = bx;
    op.y0 = by;
    op.pod = pod;
    op.rmProngDir = srcDir;
    op.addProngDir = tgtDir;
    op.superProng = false;

    if (!validateOpsOnScratch({op})) {
        return false;
    }

    _stagedOps = {op};
    _awaitingConfirmation = true;
    syncPreparedStateToGame();

    if (!_confirmAllMoves) {
        return commitStagedMove();
    }
    return true;
}

bool OctiMoveComposer::stageOrCommitPodMoveOrJump(int fromX, int fromY,
                                                  int podIndex,
                                                  int toX, int toY,
                                                  bool moveAllMatchingStack)
{
    if (!_board || !_board->getGame()) {
        return false;
    }
    OctiGame *game = _board->getGame();
    if (!game->isGameStarted() || game->isGameOver()) {
        return false;
    }

    int fbx = fromX - 1;
    int fby = fromY - 1;
    int tbx = toX - 1;
    int tby = toY - 1;
    if (!_board->isValidCoord(fbx, fby) || !_board->isValidCoord(tbx, tby)) {
        return false;
    }
    if (fbx == tbx && fby == tby && !_activeJumpHasChain) {
        return false;
    }

    OctiPlayer turn = game->getTurn();
    int w = _board->getBoardWidth();
    int h = _board->getBoardHeight();
    bool edgeless = game->useEdgelessBoard();

    // Case A: 8th-prong superprong bonus move follow-up
    if (_awaitingSuperProngBonusMove && _stagedOps.size() == 1 &&
        _stagedOps[0].addProngDir != DIR_UNDEF &&
        fbx == _stagedOps[0].x0 && fby == _stagedOps[0].y0) {
        OctiPod fullPod = _stagedOps[0].pod;
        fullPod.addProng(_stagedOps[0].addProngDir, true);

        int dx = 0, dy = 0;
        if (computeDisplacement(fbx, fby, tbx, tby, edgeless, w, h, dx, dy)) {
            bool isDist1 = (std::max(std::abs(dx), std::abs(dy)) == 1 &&
                            (std::abs(dx) == 1 || std::abs(dy) == 1));
            bool isDist2 = ((std::abs(dx) == 0 || std::abs(dx) == 2) &&
                            (std::abs(dy) == 0 || std::abs(dy) == 2) &&
                            (std::abs(dx) == 2 || std::abs(dy) == 2) &&
                            (std::abs(dx) == 0 || std::abs(dy) == 0 || std::abs(dx) == std::abs(dy)));
            OctiDir moveDir = isDist1 ? deltaToDir(dx, dy) :
                              isDist2 ? deltaToDir(dx / 2, dy / 2) : DIR_UNDEF;
            if (moveDir != DIR_UNDEF && fullPod.prongAt(moveDir) != NO_PRONG &&
                !_board->squareAt(tbx, tby).hasEnemyOf(turn)) {
                _hasEighthProngBonusHop = true;
                _eighthProngTargetBx = tbx;
                _eighthProngTargetBy = tby;
                _eighthProngBonusNotation =
                    OctiMove::formatOps(_stagedOps) + "," +
                    std::string(1, static_cast<char>('1' + fbx)) +
                    std::string(1, static_cast<char>('1' + fby)) + "-" +
                    std::string(1, static_cast<char>('1' + tbx)) +
                    std::string(1, static_cast<char>('1' + tby));
                _awaitingConfirmation = true;
                return true;
            }
        }
        return false;
    }

    // Case B: Appending a continuation hop to an active staged jump chain
    if (_activeJumpHasChain && !_stagedOps.empty() &&
        fbx == _activeJumpCurBx && fby == _activeJumpCurBy) {
        OctiMoveOp &activeOp = _stagedOps.back();
        const OctiSquare &startSq = _board->squareAt(_activeJumpStartBx, _activeJumpStartBy);
        std::vector<OctiPod> ghostPods = startSq.pods();

        std::vector<OctiHop> extraHops;
        std::vector<std::pair<int, int>> nextJumped;
        if (!findJumpPathBfs(_board, edgeless, turn,
                             _activeJumpStartBx, _activeJumpStartBy,
                             _activeJumpCurBx, _activeJumpCurBy,
                             tbx, tby, activeOp.pod, ghostPods,
                             _activeJumpedCoords, extraHops, nextJumped)) {
            return false;
        }

        std::vector<OctiMoveOp> candidateOps = _stagedOps;
        for (const auto &eh : extraHops) {
            candidateOps.back().hops.push_back(eh);
        }
        if (!validateOpsOnScratch(candidateOps)) {
            return false;
        }

        std::size_t opIdx = _stagedOps.size() - 1;
        std::size_t prevJumpedCount = _activeJumpedCoords.size();
        for (std::size_t i = 0; i < extraHops.size(); ++i) {
            std::size_t hopIdx = activeOp.hops.size();
            activeOp.hops.push_back(extraHops[i]);
            std::pair<int, int> jc = nextJumped[prevJumpedCount + i];
            const OctiSquare &jsq = _board->squareAt(jc.first, jc.second);
            if (jsq.hasEnemyOf(turn)) {
                int jx1 = jc.first + 1;
                int jy1 = jc.second + 1;
                _hopCaptureLinks.push_back({opIdx, hopIdx, jx1, jy1});
                bool alreadyListed = false;
                for (const auto &cc : _captureChoices) {
                    if (cc.jumpedX == jx1 && cc.jumpedY == jy1) {
                        alreadyListed = true;
                        break;
                    }
                }
                if (!alreadyListed) {
                    _captureChoices.push_back({
                        jx1, jy1, true, _board->getGameMode() == GAME_MODE_OFK
                    });
                }
            }
        }

        _activeJumpedCoords = nextJumped;
        _activeJumpCurBx = tbx;
        _activeJumpCurBy = tby;
        _awaitingConfirmation = true;
        syncPreparedStateToGame();
        return true;
    }

    // Case C: Starting a new move/jump op (or adding a composite pod from the same start square)
    if (!_stagedOps.empty()) {
        for (const auto &existingOp : _stagedOps) {
            if (existingOp.hops.empty() ||
                existingOp.x0 != fbx || existingOp.y0 != fby) {
                return false;
            }
        }
    }

    const OctiSquare &startSq = _board->squareAt(fbx, fby);
    int resolvedIdx = resolvePodIndex(startSq, fromX, fromY, podIndex, turn, true);
    if (resolvedIdx < 0) {
        return false;
    }

    const OctiPod &movingPod = startSq.getPodAt(resolvedIdx);
    std::vector<OctiPod> ghostPods = startSq.pods();

    int dx = 0, dy = 0;
    bool hasDirectDelta = computeDisplacement(fbx, fby, tbx, tby, edgeless, w, h, dx, dy);
    bool isDist1 = hasDirectDelta &&
                   (std::max(std::abs(dx), std::abs(dy)) == 1 &&
                    (std::abs(dx) == 1 || std::abs(dy) == 1));

    std::vector<OctiHop> chosenHops;
    std::vector<std::pair<int, int>> chosenJumped;

    if (isDist1) {
        OctiDir dir = deltaToDir(dx, dy);
        if (dir == DIR_UNDEF || movingPod.prongAt(dir) == NO_PRONG) {
            return false;
        }
        OctiHop hop;
        hop.x = tbx;
        hop.y = tby;
        hop.capture = false;
        chosenHops.push_back(hop);
    } else {
        if (!findJumpPathBfs(_board, edgeless, turn,
                             fbx, fby, fbx, fby, tbx, tby,
                             movingPod, ghostPods, {},
                             chosenHops, chosenJumped)) {
            return false;
        }
    }

    bool moveStack = moveAllMatchingStack || _moveEntireMatchingStack;
    std::vector<int> podIndicesToMove;
    podIndicesToMove.push_back(resolvedIdx);

    if (moveStack && _board->getGameMode() != GAME_MODE_OFK) {
        for (int i = 0; i < startSq.getNumPods(); ++i) {
            if (i == resolvedIdx || isPodClaimed(fbx, fby, i)) {
                continue;
            }
            const OctiPod &otherPod = startSq.getPodAt(i);
            if (otherPod.getType() != playerToPodType(turn)) {
                continue;
            }
            // Check if otherPod has identical config or can perform all hops in chosenHops
            bool canFollowSamePath = true;
            int cx = fbx;
            int cy = fby;
            for (const auto &hop : chosenHops) {
                int hdx = 0, hdy = 0;
                if (!computeDisplacement(cx, cy, hop.x, hop.y, edgeless, w, h, hdx, hdy)) {
                    canFollowSamePath = false;
                    break;
                }
                OctiDir reqDir = (std::max(std::abs(hdx), std::abs(hdy)) == 1)
                    ? deltaToDir(hdx, hdy)
                    : deltaToDir(hdx / 2, hdy / 2);
                if (reqDir == DIR_UNDEF || otherPod.prongAt(reqDir) == NO_PRONG) {
                    canFollowSamePath = false;
                    break;
                }
                cx = hop.x;
                cy = hop.y;
            }
            if (canFollowSamePath) {
                podIndicesToMove.push_back(i);
            }
        }
    }

    std::vector<OctiMoveOp> candidateOps = _stagedOps;
    for (int pIdx : podIndicesToMove) {
        OctiMoveOp op;
        op.x0 = fbx;
        op.y0 = fby;
        op.pod = startSq.getPodAt(pIdx);
        op.hops = chosenHops;
        candidateOps.push_back(op);
    }

    if (!validateOpsOnScratch(candidateOps)) {
        return false;
    }

    int startKey = fbx * 10 + fby;
    for (int pIdx : podIndicesToMove) {
        std::size_t opIdx = _stagedOps.size();
        OctiMoveOp op;
        op.x0 = fbx;
        op.y0 = fby;
        op.pod = startSq.getPodAt(pIdx);
        op.hops = chosenHops;
        _stagedOps.push_back(op);
        _claimedStartPods.push_back({startKey, pIdx});

        if (!isDist1) {
            for (std::size_t hi = 0; hi < chosenJumped.size(); ++hi) {
                std::pair<int, int> jc = chosenJumped[hi];
                const OctiSquare &jsq = _board->squareAt(jc.first, jc.second);
                if (jsq.hasEnemyOf(turn)) {
                    int jx1 = jc.first + 1;
                    int jy1 = jc.second + 1;
                    _hopCaptureLinks.push_back({opIdx, hi, jx1, jy1});
                    bool alreadyListed = false;
                    for (const auto &cc : _captureChoices) {
                        if (cc.jumpedX == jx1 && cc.jumpedY == jy1) {
                            alreadyListed = true;
                            break;
                        }
                    }
                    if (!alreadyListed) {
                        _captureChoices.push_back({
                            jx1, jy1, true, _board->getGameMode() == GAME_MODE_OFK
                        });
                    }
                }
            }
        }
    }

    _activeJumpHasChain = !isDist1;
    _activeJumpStartBx = fbx;
    _activeJumpStartBy = fby;
    _activeJumpCurBx = tbx;
    _activeJumpCurBy = tby;
    _activeJumpedCoords = chosenJumped;

    bool hasMorePodsOnStart = hasOtherMovableFriendlyPods(fbx, fby, turn);
    bool hasContinuation = !isDist1 &&
        hasContinuationJumpFrom(tbx, tby, movingPod, fbx, fby, ghostPods, chosenJumped);
    bool hasOptionalCapture = false;
    for (const auto &cc : _captureChoices) {
        if (!cc.mandatory) {
            hasOptionalCapture = true;
            break;
        }
    }

    bool shouldPause = _confirmAllMoves ||
                       !isDist1 ||
                       hasMorePodsOnStart ||
                       hasContinuation ||
                       hasOptionalCapture ||
                       _stagedOps.size() > 1;

    _awaitingConfirmation = true;
    syncPreparedStateToGame();

    if (!shouldPause) {
        return commitStagedMove();
    }
    return true;
}

bool OctiMoveComposer::beginGuidedPodMove(int x, int y, int podIndex, bool moveAllMatchingStack)
{
    if (!_board || !_board->getGame()) {
        return false;
    }
    OctiGame *game = _board->getGame();
    if (!game->isGameStarted() || game->isGameOver()) {
        return false;
    }

    int bx = x - 1;
    int by = y - 1;
    if (!_board->isValidCoord(bx, by)) {
        return false;
    }

    // If there are existing staged ops, ensure they all originate from this same square
    if (!_stagedOps.empty()) {
        for (const auto &op : _stagedOps) {
            if (op.hops.empty() || op.x0 != bx || op.y0 != by) {
                return false;
            }
        }
    }

    OctiPlayer turn = game->getTurn();
    const OctiSquare &sq = _board->squareAt(bx, by);
    int resolvedIdx = resolvePodIndex(sq, x, y, podIndex, turn, true);
    if (resolvedIdx < 0 || resolvedIdx >= sq.getNumPods()) {
        return false;
    }

    _guidedOriginBx = bx;
    _guidedOriginBy = by;
    _guidedCurBx = bx;
    _guidedCurBy = by;
    _guidedPodIndex = resolvedIdx;
    _guidedMoveAllStack = moveAllMatchingStack;
    _guidedMidHop = false;
    _guidedMoveActive = true;

    // Verify this pod has at least one legal next square
    if (guidedNextSquares().empty()) {
        _guidedMoveActive = false;
        return false;
    }

    return true;
}

bool OctiMoveComposer::isGuidedMoveActive() const
{
    return _guidedMoveActive;
}

std::pair<int, int> OctiMoveComposer::guidedCurrentSquare() const
{
    if (!_guidedMoveActive) {
        return {0, 0};
    }
    return {_guidedCurBx + 1, _guidedCurBy + 1};
}

std::pair<int, int> OctiMoveComposer::guidedOriginSquare() const
{
    if (!_guidedMoveActive) {
        return {0, 0};
    }
    return {_guidedOriginBx + 1, _guidedOriginBy + 1};
}

std::vector<std::pair<int, int>> OctiMoveComposer::guidedNextSquares() const
{
    std::vector<std::pair<int, int>> result;
    if (!_guidedMoveActive || !_board || !_board->getGame()) {
        return result;
    }
    OctiGame *game = _board->getGame();
    if (!game->isGameStarted() || game->isGameOver()) {
        return result;
    }

    OctiPlayer turn = game->getTurn();
    int w = _board->getBoardWidth();
    int h = _board->getBoardHeight();
    bool edgeless = game->useEdgelessBoard();
    std::set<std::pair<int, int>> seen;

    if (!_guidedMidHop) {
        if (!_board->isValidCoord(_guidedOriginBx, _guidedOriginBy)) {
            return result;
        }
        const OctiSquare &startSq = _board->squareAt(_guidedOriginBx, _guidedOriginBy);
        if (_guidedPodIndex < 0 || _guidedPodIndex >= startSq.getNumPods()) {
            return result;
        }
        const OctiPod &pod = startSq.getPodAt(_guidedPodIndex);
        if (pod.getType() != playerToPodType(turn)) {
            return result;
        }
        std::vector<OctiPod> ghostPods = startSq.pods();

        // 1. Distance-1 steps
        for (int d = DIR_N; d <= DIR_NW; ++d) {
            OctiDir dir = static_cast<OctiDir>(d);
            if (pod.prongAt(dir) == NO_PRONG) {
                continue;
            }
            if (_board->getGameMode() == GAME_MODE_OFK && isOfkBackwardDirection(turn, dir)) {
                continue;
            }
            int jdx = dirDeltaX(dir);
            int jdy = dirDeltaY(dir);
            int sx = edgeless ? (((_guidedOriginBx + jdx) % w + w) % w) : (_guidedOriginBx + jdx);
            int sy = edgeless ? (((_guidedOriginBy + jdy) % h + h) % h) : (_guidedOriginBy + jdy);
            if (!_board->isValidCoord(sx, sy)) {
                continue;
            }
            const OctiSquare &destSq = _board->squareAt(sx, sy);
            if (_board->getGameMode() == GAME_MODE_OFK) {
                if (!destSq.isEmpty()) continue;
            } else {
                if (destSq.hasEnemyOf(turn)) continue;
                if (destSq.getNumPods() >= 7) continue;
            }
            OctiHop hop; hop.x = sx; hop.y = sy; hop.capture = false;
            OctiMoveOp op; op.x0 = _guidedOriginBx; op.y0 = _guidedOriginBy;
            op.pod = pod; op.hops.push_back(hop);
            std::vector<OctiMoveOp> cand = _stagedOps;
            cand.push_back(op);
            if (validateOpsOnScratch(cand)) {
                if (seen.insert({sx + 1, sy + 1}).second) {
                    result.push_back({sx + 1, sy + 1});
                }
            }
        }

        // 2. Distance-2 jumps
        for (int d = DIR_N; d <= DIR_NW; ++d) {
            OctiDir dir = static_cast<OctiDir>(d);
            if (pod.prongAt(dir) == NO_PRONG) {
                continue;
            }
            if (_board->getGameMode() == GAME_MODE_OFK && isOfkBackwardDirection(turn, dir)) {
                continue;
            }
            int jdx = dirDeltaX(dir);
            int jdy = dirDeltaY(dir);
            int dx = jdx * 2;
            int dy = jdy * 2;
            int jx = edgeless ? (((_guidedOriginBx + jdx) % w + w) % w) : (_guidedOriginBx + jdx);
            int jy = edgeless ? (((_guidedOriginBy + jdy) % h + h) % h) : (_guidedOriginBy + jdy);
            int lx = edgeless ? (((_guidedOriginBx + dx) % w + w) % w) : (_guidedOriginBx + dx);
            int ly = edgeless ? (((_guidedOriginBy + dy) % h + h) % h) : (_guidedOriginBy + dy);
            if (!_board->isValidCoord(jx, jy) || !_board->isValidCoord(lx, ly)) {
                continue;
            }
            const OctiSquare &jumpedSq = _board->squareAt(jx, jy);
            bool jumpedEmpty = (jx == _guidedOriginBx && jy == _guidedOriginBy)
                ? jumpedSq.isEmptyWithout(ghostPods) : jumpedSq.isEmpty();
            if (jumpedEmpty) continue;

            const OctiSquare &landSq = _board->squareAt(lx, ly);
            bool landEmpty = (lx == _guidedOriginBx && ly == _guidedOriginBy)
                ? landSq.isEmptyWithout(ghostPods) : landSq.isEmpty();
            if (!landEmpty) continue;

            OctiHop hop; hop.x = lx; hop.y = ly;
            hop.capture = (_board->getGameMode() == GAME_MODE_OFK) || jumpedSq.hasEnemyOf(turn);
            OctiMoveOp op; op.x0 = _guidedOriginBx; op.y0 = _guidedOriginBy;
            op.pod = pod; op.hops.push_back(hop);
            std::vector<OctiMoveOp> cand = _stagedOps;
            cand.push_back(op);
            if (validateOpsOnScratch(cand)) {
                if (seen.insert({lx + 1, ly + 1}).second) {
                    result.push_back({lx + 1, ly + 1});
                }
            }
        }
    } else {
        // Mid-jump continuation from (_guidedCurBx, _guidedCurBy)
        if (_stagedOps.empty()) {
            return result;
        }
        const OctiMoveOp &curOp = _stagedOps.back();
        const OctiPod &pod = curOp.pod;
        const OctiSquare &startSq = _board->squareAt(_guidedOriginBx, _guidedOriginBy);
        std::vector<OctiPod> ghostPods = startSq.pods();

        for (int d = DIR_N; d <= DIR_NW; ++d) {
            OctiDir dir = static_cast<OctiDir>(d);
            if (pod.prongAt(dir) == NO_PRONG) {
                continue;
            }
            if (_board->getGameMode() == GAME_MODE_OFK && isOfkBackwardDirection(turn, dir)) {
                continue;
            }
            int jdx = dirDeltaX(dir);
            int jdy = dirDeltaY(dir);
            int dx = jdx * 2;
            int dy = jdy * 2;
            int jx = edgeless ? (((_guidedCurBx + jdx) % w + w) % w) : (_guidedCurBx + jdx);
            int jy = edgeless ? (((_guidedCurBy + jdy) % h + h) % h) : (_guidedCurBy + jdy);
            int lx = edgeless ? (((_guidedCurBx + dx) % w + w) % w) : (_guidedCurBx + dx);
            int ly = edgeless ? (((_guidedCurBy + dy) % h + h) % h) : (_guidedCurBy + dy);
            if (!_board->isValidCoord(jx, jy) || !_board->isValidCoord(lx, ly)) {
                continue;
            }
            if (std::find(_activeJumpedCoords.begin(), _activeJumpedCoords.end(),
                          std::make_pair(jx, jy)) != _activeJumpedCoords.end()) {
                continue;
            }
            const OctiSquare &jumpedSq = _board->squareAt(jx, jy);
            bool jumpedEmpty = (jx == _guidedOriginBx && jy == _guidedOriginBy)
                ? jumpedSq.isEmptyWithout(ghostPods) : jumpedSq.isEmpty();
            if (jumpedEmpty) continue;

            const OctiSquare &landSq = _board->squareAt(lx, ly);
            bool landEmpty = (lx == _guidedOriginBx && ly == _guidedOriginBy)
                ? landSq.isEmptyWithout(ghostPods) : landSq.isEmpty();
            if (!landEmpty) continue;

            std::vector<OctiMoveOp> cand = _stagedOps;
            OctiHop hop; hop.x = lx; hop.y = ly;
            hop.capture = (_board->getGameMode() == GAME_MODE_OFK) || jumpedSq.hasEnemyOf(turn);
            cand.back().hops.push_back(hop);
            if (validateOpsOnScratch(cand)) {
                if (seen.insert({lx + 1, ly + 1}).second) {
                    result.push_back({lx + 1, ly + 1});
                }
            }
        }
    }

    return result;
}

bool OctiMoveComposer::selectGuidedDestination(int toX, int toY)
{
    if (!_guidedMoveActive || !_board || !_board->getGame()) {
        return false;
    }
    auto legal = guidedNextSquares();
    bool found = false;
    for (const auto &sq : legal) {
        if (sq.first == toX && sq.second == toY) {
            found = true;
            break;
        }
    }
    if (!found) {
        return false;
    }

    int tbx = toX - 1;
    int tby = toY - 1;
    int fromBx = _guidedMidHop ? _guidedCurBx : _guidedOriginBx;
    int fromBy = _guidedMidHop ? _guidedCurBy : _guidedOriginBy;

    int w = _board->getBoardWidth();
    int h = _board->getBoardHeight();
    bool edgeless = _board->getGame()->useEdgelessBoard();
    int dx = 0, dy = 0;
    computeDisplacement(fromBx, fromBy, tbx, tby, edgeless, w, h, dx, dy);
    bool isStep = (std::max(std::abs(dx), std::abs(dy)) == 1);

    if (!_guidedMidHop) {
        if (!stageOrCommitPodMoveOrJump(fromBx + 1, fromBy + 1, _guidedPodIndex,
                                        toX, toY, _guidedMoveAllStack)) {
            return false;
        }
        _guidedCurBx = tbx;
        _guidedCurBy = tby;
        if (isStep) {
            _guidedMoveActive = false;
        } else {
            _guidedMidHop = true;
            if (guidedNextSquares().empty()) {
                _guidedMoveActive = false;
            }
        }
        return true;
    } else {
        if (!stageOrCommitPodMoveOrJump(fromBx + 1, fromBy + 1, _guidedPodIndex,
                                        toX, toY, _guidedMoveAllStack)) {
            return false;
        }
        _guidedCurBx = tbx;
        _guidedCurBy = tby;
        if (guidedNextSquares().empty()) {
            _guidedMoveActive = false;
        }
        return true;
    }
}

bool OctiMoveComposer::finishGuidedMoveHere()
{
    if (!_guidedMoveActive) {
        return false;
    }
    if (_guidedMidHop) {
        _guidedMoveActive = false;
        _activeJumpHasChain = false;
        return true;
    }
    _guidedMoveActive = false;
    return false;
}

bool OctiMoveComposer::hasOtherMovablePodAtOrigin() const
{
    if (_stagedOps.empty() || !_board || !_board->getGame()) {
        return false;
    }
    int origBx = _stagedOps[0].x0;
    int origBy = _stagedOps[0].y0;
    OctiPlayer turn = _board->getGame()->getTurn();
    const OctiSquare &sq = _board->squareAt(origBx, origBy);
    OctiPodType desiredType = playerToPodType(turn);

    for (int i = 0; i < sq.getNumPods(); ++i) {
        if (isPodClaimed(origBx, origBy, i)) {
            continue;
        }
        const OctiPod &pod = sq.getPodAt(i);
        if (pod.getType() != desiredType) {
            continue;
        }
        if (pod.getNumProngs() > 0) {
            auto dests = legalDestinationsFrom(origBx + 1, origBy + 1, i);
            if (!dests.empty()) {
                return true;
            }
        }
    }
    return false;
}

bool OctiMoveComposer::setCaptureAtSquare(int jumpedX, int jumpedY, bool capture)
{
    if (_stagedOps.empty()) {
        return false;
    }

    int targetIdx = -1;
    for (std::size_t i = 0; i < _captureChoices.size(); ++i) {
        if (_captureChoices[i].jumpedX == jumpedX &&
            _captureChoices[i].jumpedY == jumpedY) {
            targetIdx = static_cast<int>(i);
            break;
        }
    }
    if (targetIdx < 0) {
        return false;
    }
    if (_captureChoices[static_cast<std::size_t>(targetIdx)].mandatory) {
        return false;
    }

    _captureChoices[static_cast<std::size_t>(targetIdx)].capture = capture;

    for (const auto &link : _hopCaptureLinks) {
        if (link.jumpedX == jumpedX && link.jumpedY == jumpedY &&
            link.opIndex < _stagedOps.size() &&
            link.hopIndex < _stagedOps[link.opIndex].hops.size()) {
            _stagedOps[link.opIndex].hops[link.hopIndex].capture = capture;
        }
    }

    syncPreparedStateToGame();
    return true;
}

bool OctiMoveComposer::toggleCaptureAtSquare(int jumpedX, int jumpedY)
{
    if (_stagedOps.empty()) {
        return false;
    }

    int targetIdx = -1;
    for (std::size_t i = 0; i < _captureChoices.size(); ++i) {
        if (_captureChoices[i].jumpedX == jumpedX &&
            _captureChoices[i].jumpedY == jumpedY) {
            targetIdx = static_cast<int>(i);
            break;
        }
    }
    if (targetIdx < 0) {
        return false;
    }
    if (_captureChoices[static_cast<std::size_t>(targetIdx)].mandatory) {
        return false;
    }

    bool newCap = !_captureChoices[static_cast<std::size_t>(targetIdx)].capture;
    _captureChoices[static_cast<std::size_t>(targetIdx)].capture = newCap;

    for (const auto &link : _hopCaptureLinks) {
        if (link.jumpedX == jumpedX && link.jumpedY == jumpedY &&
            link.opIndex < _stagedOps.size() &&
            link.hopIndex < _stagedOps[link.opIndex].hops.size()) {
            _stagedOps[link.opIndex].hops[link.hopIndex].capture = newCap;
        }
    }

    syncPreparedStateToGame();
    return true;
}

bool OctiMoveComposer::toggleHopCapture(std::size_t hopIndex)
{
    if (hopIndex >= _captureChoices.size()) {
        return false;
    }
    return toggleCaptureAtSquare(_captureChoices[hopIndex].jumpedX,
                                 _captureChoices[hopIndex].jumpedY);
}

bool OctiMoveComposer::canCommitStagedMove() const
{
    return !_stagedOps.empty() && validateOpsOnScratch(_stagedOps);
}

bool OctiMoveComposer::isAwaitingConfirmation() const
{
    return _awaitingConfirmation && !_stagedOps.empty();
}

bool OctiMoveComposer::isAwaitingSuperProngBonusMove() const
{
    return _awaitingSuperProngBonusMove;
}

std::string OctiMoveComposer::stagedMoveNotation() const
{
    if (_stagedOps.empty()) {
        return "";
    }
    if (_hasEighthProngBonusHop && !_eighthProngBonusNotation.empty()) {
        return _eighthProngBonusNotation;
    }
    return OctiMove::formatOps(_stagedOps);
}

const std::vector<CaptureChoice> &OctiMoveComposer::captureChoices() const
{
    return _captureChoices;
}

bool OctiMoveComposer::populatePreviewBoard(OctiBoard &outBoard) const
{
    if (!_board || !_board->getGame()) {
        return false;
    }
    outBoard.suppressBoardUpdateNotification(true);
    outBoard.fromPositionString(_board->toPositionString());
    if (_stagedOps.empty()) {
        return true;
    }
    try {
        OctiMove mv(_stagedOps);
        outBoard.getGame()->move(mv);
        if (_hasEighthProngBonusHop &&
            outBoard.isValidCoord(_eighthProngTargetBx, _eighthProngTargetBy) &&
            _stagedOps.size() == 1) {
            int sx = _stagedOps[0].x0;
            int sy = _stagedOps[0].y0;
            OctiSquare &srcSq = outBoard.squareAt(sx, sy);
            OctiSquare &dstSq = outBoard.squareAt(_eighthProngTargetBx, _eighthProngTargetBy);
            if (!srcSq.isEmpty()) {
                OctiPod movedPod = srcSq.getPodAt(srcSq.getNumPods() - 1);
                srcSq.removePod(movedPod);
                dstSq.addPod(movedPod);
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<std::pair<int, int>> OctiMoveComposer::legalDestinationsFrom(
    int x, int y, int podIndex) const
{
    std::vector<std::pair<int, int>> result;
    if (!_board || !_board->getGame()) {
        return result;
    }
    OctiGame *game = _board->getGame();
    if (!game->isGameStarted() || game->isGameOver()) {
        return result;
    }

    int bx = x - 1;
    int by = y - 1;
    if (!_board->isValidCoord(bx, by)) {
        return result;
    }

    OctiPlayer turn = game->getTurn();
    int w = _board->getBoardWidth();
    int h = _board->getBoardHeight();
    bool edgeless = game->useEdgelessBoard();

    // If querying from the current end of an active jump chain
    if (_activeJumpHasChain && !_stagedOps.empty() &&
        bx == _activeJumpCurBx && by == _activeJumpCurBy) {
        const OctiPod &pod = _stagedOps.back().pod;
        const OctiSquare &startSq = _board->squareAt(_activeJumpStartBx, _activeJumpStartBy);
        std::vector<OctiPod> ghostPods = startSq.pods();

        for (int ty = 0; ty < h; ++ty) {
            for (int tx = 0; tx < w; ++tx) {
                std::vector<OctiHop> extraHops;
                std::vector<std::pair<int, int>> nextJumped;
                if (findJumpPathBfs(_board, edgeless, turn,
                                    _activeJumpStartBx, _activeJumpStartBy,
                                    _activeJumpCurBx, _activeJumpCurBy,
                                    tx, ty, pod, ghostPods,
                                    _activeJumpedCoords, extraHops, nextJumped)) {
                    result.push_back({tx + 1, ty + 1});
                }
            }
        }
        return result;
    }

    // If awaiting 8th-prong superprong bonus move on (x, y)
    if (_awaitingSuperProngBonusMove && _stagedOps.size() == 1 &&
        _stagedOps[0].x0 == bx && _stagedOps[0].y0 == by) {
        OctiPod fullPod = _stagedOps[0].pod;
        fullPod.addProng(_stagedOps[0].addProngDir, true);
        for (int d = DIR_N; d <= DIR_NW; ++d) {
            OctiDir dir = static_cast<OctiDir>(d);
            if (fullPod.prongAt(dir) == NO_PRONG) continue;
            int sx = edgeless ? (((bx + dirDeltaX(dir)) % w + w) % w) : (bx + dirDeltaX(dir));
            int sy = edgeless ? (((by + dirDeltaY(dir)) % h + h) % h) : (by + dirDeltaY(dir));
            if (_board->isValidCoord(sx, sy) && !_board->squareAt(sx, sy).hasEnemyOf(turn)) {
                result.push_back({sx + 1, sy + 1});
            }
            int jx = edgeless ? (((bx + 2 * dirDeltaX(dir)) % w + w) % w) : (bx + 2 * dirDeltaX(dir));
            int jy = edgeless ? (((by + 2 * dirDeltaY(dir)) % h + h) % h) : (by + 2 * dirDeltaY(dir));
            if (_board->isValidCoord(sx, sy) && _board->isValidCoord(jx, jy) &&
                !_board->squareAt(sx, sy).isEmpty() && _board->squareAt(jx, jy).isEmpty()) {
                result.push_back({jx + 1, jy + 1});
            }
        }
        return result;
    }

    const OctiSquare &sq = _board->squareAt(bx, by);
    int resolvedIdx = resolvePodIndex(sq, x, y, podIndex, turn, true);
    bool filterByPod = (podIndex >= 0 && resolvedIdx >= 0);
    std::uint32_t targetPodConf = filterByPod
        ? (sq.getPodAt(resolvedIdx).rawConfig() & 0xFFFFu) : 0u;

    std::set<std::pair<int, int>> seen;
    std::vector<OctiMove> legal = game->generateLegalMoves();
    for (const auto &mv : legal) {
        const auto &ops = mv.getOps();
        if (ops.size() != 1 || ops[0].hops.empty()) {
            continue;
        }
        const OctiMoveOp &op = ops[0];
        if (op.x0 != bx || op.y0 != by) {
            continue;
        }
        if (filterByPod && (op.pod.rawConfig() & 0xFFFFu) != targetPodConf) {
            continue;
        }
        const OctiHop &lastHop = op.hops.back();
        std::pair<int, int> dest(lastHop.x + 1, lastHop.y + 1);
        if (seen.insert(dest).second) {
            result.push_back(dest);
        }
    }

    return result;
}

std::vector<OctiDir> OctiMoveComposer::legalProngAdditionsAt(
    int x, int y, int podIndex, bool superProng) const
{
    std::vector<OctiDir> result;
    if (!_board || !_board->getGame()) {
        return result;
    }
    OctiGame *game = _board->getGame();
    if (!game->isGameStarted() || game->isGameOver()) {
        return result;
    }

    int bx = x - 1;
    int by = y - 1;
    if (!_board->isValidCoord(bx, by)) {
        return result;
    }

    OctiPlayer turn = game->getTurn();
    if (superProng) {
        if (!game->useSuperProngs() || game->getFreeSuperProngs(turn) <= 0) {
            return result;
        }
    } else {
        if (game->getFreeProngs(turn) <= 0) {
            return result;
        }
    }

    const OctiSquare &sq = _board->squareAt(bx, by);
    int resolvedIdx = resolvePodIndex(sq, x, y, podIndex, turn, false);
    if (resolvedIdx < 0) {
        return result;
    }

    const OctiPod &pod = sq.getPodAt(resolvedIdx);
    for (int d = DIR_N; d <= DIR_NW; ++d) {
        OctiDir dir = static_cast<OctiDir>(d);
        if (pod.prongAt(dir) != NO_PRONG) {
            continue;
        }
        if (_board->getGameMode() == GAME_MODE_OFK &&
            isOfkBackwardDirection(turn, dir)) {
            continue;
        }
        result.push_back(dir);
    }

    return result;
}

std::vector<std::pair<int, int>> OctiMoveComposer::legalPodDropSquares(bool liberate) const
{
    std::vector<std::pair<int, int>> result;
    if (!_board || !_board->getGame()) {
        return result;
    }
    OctiGame *game = _board->getGame();
    if (!game->isGameStarted() || game->isGameOver()) {
        return result;
    }

    OctiPlayer turn = game->getTurn();
    if (liberate) {
        if (game->getCapturedPods(turn) <= 0 || !_board->canLiberate(turn)) {
            return result;
        }
    } else {
        if (game->getFreePods(turn) <= 0) {
            return result;
        }
    }

    int w = _board->getBoardWidth();
    int h = _board->getBoardHeight();
    for (int by = 0; by < h; ++by) {
        for (int bx = 0; bx < w; ++bx) {
            OctiMoveOp op;
            op.x0 = bx;
            op.y0 = by;
            op.addPod = true;
            op.liberate = liberate;
            if (validateOpsOnScratch({op})) {
                result.push_back({bx + 1, by + 1});
            }
        }
    }
    return result;
}

bool OctiMoveComposer::commitStagedMove()
{
    if (!_board || !_board->getGame() || _stagedOps.empty()) {
        return false;
    }
    OctiGame *game = _board->getGame();
    std::vector<OctiMoveOp> opsToCommit = _stagedOps;
    bool hadBonusHop = _hasEighthProngBonusHop;
    int bonusTx = _eighthProngTargetBx;
    int bonusTy = _eighthProngTargetBy;
    std::string committedNotation = stagedMoveNotation();

    game->rollbackPreparedMove();

    try {
        OctiMove mv(opsToCommit);
        game->move(mv);
        if (hadBonusHop && _board->isValidCoord(bonusTx, bonusTy) &&
            opsToCommit.size() == 1) {
            int sx = opsToCommit[0].x0;
            int sy = opsToCommit[0].y0;
            OctiSquare &srcSq = _board->squareAt(sx, sy);
            OctiSquare &dstSq = _board->squareAt(bonusTx, bonusTy);
            if (!srcSq.isEmpty()) {
                OctiPod movedPod = srcSq.getPodAt(srcSq.getNumPods() - 1);
                srcSq.removePod(movedPod);
                dstSq.addPod(movedPod);
            }
        }
        if (!hadBonusHop) {
            committedNotation = mv.asString();
        }
    } catch (...) {
        cancelAndRollback();
        return false;
    }

    _stagedOps.clear();
    _claimedStartPods.clear();
    _captureChoices.clear();
    _hopCaptureLinks.clear();
    _awaitingConfirmation = false;
    _awaitingSuperProngBonusMove = false;
    _activeJumpHasChain = false;
    _hasEighthProngBonusHop = false;
    _eighthProngBonusNotation.clear();

    _guidedMoveActive = false;
    _guidedMidHop = false;
    _guidedOriginBx = -1;
    _guidedOriginBy = -1;
    _guidedCurBx = -1;
    _guidedCurBy = -1;
    _guidedPodIndex = -1;
    _guidedMoveAllStack = false;

    refreshLegalMoves();

    if (_onMoveCommitted) {
        _onMoveCommitted(committedNotation);
    }
    return true;
}

void OctiMoveComposer::cancelAndRollback()
{
    if (_board && _board->getGame()) {
        _board->getGame()->rollbackPreparedMove();
    }
    _stagedOps.clear();
    _claimedStartPods.clear();
    _captureChoices.clear();
    _hopCaptureLinks.clear();
    _awaitingConfirmation = false;
    _awaitingSuperProngBonusMove = false;
    _activeJumpHasChain = false;
    _activeJumpStartBx = -1;
    _activeJumpStartBy = -1;
    _activeJumpCurBx = -1;
    _activeJumpCurBy = -1;
    _activeJumpedCoords.clear();
    _hasEighthProngBonusHop = false;
    _eighthProngTargetBx = -1;
    _eighthProngTargetBy = -1;
    _eighthProngBonusNotation.clear();

    _guidedMoveActive = false;
    _guidedMidHop = false;
    _guidedOriginBx = -1;
    _guidedOriginBy = -1;
    _guidedCurBx = -1;
    _guidedCurBy = -1;
    _guidedPodIndex = -1;
    _guidedMoveAllStack = false;

    refreshLegalMoves();
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
