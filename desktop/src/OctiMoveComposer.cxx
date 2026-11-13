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

std::vector<int> sortedPodIndicesMostProngsFirst(const OctiSquare &sq)
{
    int numPods = sq.getNumPods();
    std::vector<int> indices(numPods);
    for (int i = 0; i < numPods; ++i) {
        indices[i] = i;
    }
    std::stable_sort(indices.begin(), indices.end(), [&](int a, int b) {
        const OctiPod &podA = sq.getPodAt(a);
        const OctiPod &podB = sq.getPodAt(b);
        if (podA.getNumProngs() != podB.getNumProngs()) {
            return podA.getNumProngs() > podB.getNumProngs();
        }
        if (podA.getNumSuperProngs() != podB.getNumSuperProngs()) {
            return podA.getNumSuperProngs() > podB.getNumSuperProngs();
        }
        return a > b;
    });
    return indices;
}

namespace {

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

bool OctiMoveComposer::hasStagedMove() const
{
    return !_stagedOps.empty() || _awaitingConfirmation || _guidedMoveActive;
}

bool OctiMoveComposer::isTurnInProgress() const
{
    return hasStagedMove();
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
    auto sorted = sortedPodIndicesMostProngsFirst(sq);
    return sorted.empty() ? -1 : sorted.front();
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
    return resolvePodIndexOnBoard(sq, x, y, requestedIndex, turn, excludeClaimed, 0);
}

int OctiMoveComposer::resolvePodIndexOnBoard(const OctiSquare &sq,
                                             int x, int y,
                                             int requestedIndex,
                                             OctiPlayer turn,
                                             bool excludeClaimed,
                                             int removedByPreludes) const
{
    int bx = x - 1;
    int by = y - 1;
    OctiPodType desiredType = playerToPodType(turn);

    if (requestedIndex >= 0) {
        int effIdx = requestedIndex;
        if (effIdx >= sq.getNumPods() && removedByPreludes > 0 &&
            requestedIndex >= removedByPreludes &&
            requestedIndex < sq.getNumPods() + removedByPreludes) {
            effIdx = requestedIndex - removedByPreludes;
        }
        if (effIdx >= 0 && effIdx < sq.getNumPods()) {
            if (sq.getPodAt(effIdx).getType() == desiredType &&
                (!excludeClaimed || !isPodClaimed(bx, by, effIdx))) {
                return effIdx;
            }
            if (excludeClaimed && isPodClaimed(bx, by, effIdx)) {
                int unclaimedOrd = 0;
                for (int i = 0; i < sq.getNumPods(); ++i) {
                    if (sq.getPodAt(i).getType() == desiredType &&
                        !isPodClaimed(bx, by, i)) {
                        if (unclaimedOrd == requestedIndex) {
                            return i;
                        }
                        ++unclaimedOrd;
                    }
                }
            }
        }
        return -1;
    }

    auto it = _selectedStackPods.find({x, y});
    if (it != _selectedStackPods.end()) {
        int sel = it->second;
        if (sel >= sq.getNumPods() && removedByPreludes > 0 &&
            sel >= removedByPreludes &&
            sel < sq.getNumPods() + removedByPreludes) {
            sel -= removedByPreludes;
        }
        if (sel >= 0 && sel < sq.getNumPods() &&
            sq.getPodAt(sel).getType() == desiredType &&
            (!excludeClaimed || !isPodClaimed(bx, by, sel))) {
            return sel;
        }
    }

    auto sorted = sortedPodIndicesMostProngsFirst(sq);
    for (int i : sorted) {
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

bool OctiMoveComposer::buildPostPreludeBoard(
    std::vector<OctiMoveOp> &outPromotedOps,
    OctiBoard &outBoard) const
{
    outPromotedOps.clear();
    if (!_board || !_board->getGame()) {
        return false;
    }
    OctiGame *game = _board->getGame();
    if (!game->isGameStarted() || game->isGameOver() || !game->useSuperProngs()) {
        return false;
    }
    if (_stagedOps.empty() || _stagedOps.size() > 2) {
        return false;
    }
    for (const auto &op : _stagedOps) {
        if (op.hops.empty() || op.addPod ||
            op.addProngDir != DIR_UNDEF || op.rmProngDir != DIR_UNDEF) {
            return false;
        }
    }

    std::vector<OctiMoveOp> promoted = _stagedOps;
    for (auto &op : promoted) {
        op.isSuperProngBonusMove = true;
    }

    try {
        outBoard.suppressBoardUpdateNotification(true);
        outBoard.fromPositionString(_board->toPositionString());
        outBoard.getGame()->applyUncommittedTurnForPreview(promoted);
        if (!outBoard.getGame()->hasAnyLegalMove()) {
            return false;
        }
        outPromotedOps = std::move(promoted);
        return true;
    } catch (...) {
        return false;
    }
}

bool OctiMoveComposer::canPromoteStagedToSuperProngPreludes() const
{
    std::vector<OctiMoveOp> promoted;
    OctiBoard postBoard;
    return buildPostPreludeBoard(promoted, postBoard);
}

bool OctiMoveComposer::buildBoardAfterBonusOps(
    const std::vector<OctiMoveOp> &ops,
    OctiBoard &outBoard,
    std::size_t *outBonusCount) const
{
    std::size_t bonusCount = 0;
    while (bonusCount < ops.size() && ops[bonusCount].isSuperProngBonusMove) {
        ++bonusCount;
    }
    if (outBonusCount) {
        *outBonusCount = bonusCount;
    }
    if (!_board || !_board->getGame()) {
        return false;
    }
    outBoard.suppressBoardUpdateNotification(true);
    outBoard.fromPositionString(_board->toPositionString());
    if (bonusCount == 0) {
        return true;
    }
    try {
        std::vector<OctiMoveOp> bonusOps(
            ops.begin(), ops.begin() + static_cast<std::ptrdiff_t>(bonusCount));
        outBoard.getGame()->applyUncommittedTurnForPreview(bonusOps);
        return true;
    } catch (...) {
        return false;
    }
}

bool OctiMoveComposer::hasContinuationJumpFrom(
    const OctiBoard *refBoard,
    int bx, int by,
    const OctiPod &pod,
    int startBx, int startBy,
    const std::vector<OctiPod> &ghostPods,
    const std::vector<std::pair<int, int>> &alreadyJumped) const
{
    const OctiBoard *activeBoard = refBoard ? refBoard : _board;
    if (!activeBoard || !activeBoard->getGame()) {
        return false;
    }
    int w = activeBoard->getBoardWidth();
    int h = activeBoard->getBoardHeight();
    bool edgeless = activeBoard->getGame()->useEdgelessBoard();

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

        const OctiSquare &jumpedSq = activeBoard->squareAt(jx, jy);
        bool jumpedEmpty = (jx == startBx && jy == startBy)
            ? jumpedSq.isEmptyWithout(ghostPods)
            : jumpedSq.isEmpty();
        if (jumpedEmpty) {
            continue;
        }

        const OctiSquare &landSq = activeBoard->squareAt(lx, ly);
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
    OctiBoard refBoard;
    const OctiBoard *activeBoard = _board;
    std::size_t bonusCount = 0;
    if (!_stagedOps.empty() && buildBoardAfterBonusOps(_stagedOps, refBoard, &bonusCount) &&
        bonusCount > 0) {
        activeBoard = &refBoard;
    }

    const OctiSquare &sq = activeBoard->squareAt(bx, by);
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
            game->prepareAddPod(op.x0, op.y0, op.liberate);
        } else if (op.addProngDir != DIR_UNDEF && op.rmProngDir == DIR_UNDEF) {
            game->prepareAddProng(op.x0, op.y0, op.pod, op.addProngDir, op.superProng);
        } else if (op.addProngDir != DIR_UNDEF && op.rmProngDir != DIR_UNDEF) {
            game->prepareReposition(op.x0, op.y0, op.pod, op.rmProngDir, op.addProngDir);
        } else if (!op.hops.empty()) {
            game->prepareHops(op.x0, op.y0, op.pod, op.hops);
        }
    }
}

bool OctiMoveComposer::canPlaceReservePodAt(int x, int y) const
{
    if (!_board || !_board->getGame()) {
        return false;
    }
    OctiGame *game = _board->getGame();
    if (!game->isGameStarted() || game->isGameOver() || game->getMode() == GAME_MODE_OFK) {
        return false;
    }
    int bx = x - 1;
    int by = y - 1;
    if (!_board->isValidCoord(bx, by)) {
        return false;
    }

    std::vector<OctiMoveOp> prefixOps;
    OctiBoard postBoard;
    const OctiBoard *activeBoard = _board;
    if (!_stagedOps.empty()) {
        if (!buildPostPreludeBoard(prefixOps, postBoard)) {
            return false;
        }
        activeBoard = &postBoard;
    }

    OctiPlayer turn = activeBoard->getGame()->getTurn();
    if (activeBoard->getGame()->getFreePods(turn) <= 0) {
        return false;
    }
    const OctiSquare &sq = activeBoard->squareAt(bx, by);
    if (sq.getType() == NEUTRAL_SQUARE || sq.getType() == NEUTRAL_OCTI_SQUARE) {
        return false;
    }
    if (activeBoard->getGameMode() == GAME_MODE_4) {
        OctiPlayer partner = (turn == PLAYER_A ? PLAYER_C :
                              turn == PLAYER_C ? PLAYER_A :
                              turn == PLAYER_B ? PLAYER_D : PLAYER_B);
        OctiSquareType partnerHome = (partner == PLAYER_A ? A_HOME_SQUARE :
                                      partner == PLAYER_B ? B_HOME_SQUARE :
                                      partner == PLAYER_C ? C_HOME_SQUARE : D_HOME_SQUARE);
        if (sq.getType() == partnerHome) {
            return false;
        }
    }
    OctiMoveOp op;
    op.x0 = bx;
    op.y0 = by;
    op.addPod = true;
    op.liberate = false;
    prefixOps.push_back(op);
    return validateOpsOnScratch(prefixOps);
}

bool OctiMoveComposer::canRevivePartnerPodAt(int x, int y) const
{
    if (!_board || !_board->getGame() || _board->getGameMode() != GAME_MODE_4) {
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

    std::vector<OctiMoveOp> prefixOps;
    OctiBoard postBoard;
    const OctiBoard *activeBoard = _board;
    if (!_stagedOps.empty()) {
        if (!buildPostPreludeBoard(prefixOps, postBoard)) {
            return false;
        }
        activeBoard = &postBoard;
    }

    OctiPlayer turn = activeBoard->getGame()->getTurn();
    OctiPlayer partner = (turn == PLAYER_A ? PLAYER_C :
                          turn == PLAYER_C ? PLAYER_A :
                          turn == PLAYER_B ? PLAYER_D : PLAYER_B);
    OctiSquareType partnerHome = (partner == PLAYER_A ? A_HOME_SQUARE :
                                  partner == PLAYER_B ? B_HOME_SQUARE :
                                  partner == PLAYER_C ? C_HOME_SQUARE : D_HOME_SQUARE);
    const OctiSquare &sq = activeBoard->squareAt(bx, by);
    if (sq.getType() != partnerHome) {
        return false;
    }
    return activeBoard->getGame()->isPlayerEliminated(partner) &&
           activeBoard->getGame()->getCapturedPods(partner) > 0 &&
           sq.isOccupiedBy(turn);
}

bool OctiMoveComposer::canLiberateCapturedPodAt(int x, int y) const
{
    if (!_board || !_board->getGame()) {
        return false;
    }
    OctiGame *game = _board->getGame();
    if (!game->isGameStarted() || game->isGameOver() || game->getMode() == GAME_MODE_OFK) {
        return false;
    }
    int bx = x - 1;
    int by = y - 1;
    if (!_board->isValidCoord(bx, by)) {
        return false;
    }

    std::vector<OctiMoveOp> prefixOps;
    OctiBoard postBoard;
    if (!_stagedOps.empty() && !buildPostPreludeBoard(prefixOps, postBoard)) {
        return false;
    }

    if (canRevivePartnerPodAt(x, y)) {
        return true;
    }
    OctiMoveOp op;
    op.x0 = bx;
    op.y0 = by;
    op.addPod = true;
    op.liberate = true;
    prefixOps.push_back(op);
    return validateOpsOnScratch(prefixOps);
}

bool OctiMoveComposer::stageOrCommitAddPod(int targetX, int targetY, bool liberate)
{
    if (!_board || !_board->getGame()) {
        return false;
    }
    OctiGame *game = _board->getGame();
    if (!game->isGameStarted() || game->isGameOver() || game->getMode() == GAME_MODE_OFK) {
        return false;
    }

    int bx = targetX - 1;
    int by = targetY - 1;
    if (!_board->isValidCoord(bx, by)) {
        return false;
    }

    std::vector<OctiMoveOp> candidateOps;
    OctiBoard postBoard;
    if (!_stagedOps.empty()) {
        if (!buildPostPreludeBoard(candidateOps, postBoard)) {
            return false;
        }
    }

    OctiMoveOp op;
    op.x0 = bx;
    op.y0 = by;
    op.addPod = true;
    op.liberate = liberate;
    candidateOps.push_back(op);

    if (!validateOpsOnScratch(candidateOps)) {
        return false;
    }

    _stagedOps = std::move(candidateOps);
    _claimedStartPods.clear();
    _activeJumpHasChain = false;
    _guidedMoveActive = false;
    _guidedMidHop = false;
    _awaitingSuperProngBonusMove = false;
    _awaitingConfirmation = true;
    syncPreparedStateToGame();

    if (!_confirmAllMoves && _stagedOps.size() <= 1) {
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
        if (superProng) {
            return false;
        }
    }

    std::vector<OctiMoveOp> candidateOps;
    OctiBoard postBoard;
    const OctiBoard *activeBoard = _board;
    int removedFromSq = 0;
    if (!_stagedOps.empty()) {
        if (!buildPostPreludeBoard(candidateOps, postBoard)) {
            return false;
        }
        activeBoard = &postBoard;
        for (const auto &pOp : candidateOps) {
            if (pOp.x0 == bx && pOp.y0 == by) {
                ++removedFromSq;
            }
        }
    }

    const OctiSquare &sq = activeBoard->squareAt(bx, by);
    int resolvedIdx = resolvePodIndexOnBoard(
        sq, x, y, podIndex, turn, false, removedFromSq);
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
    candidateOps.push_back(op);

    if (!validateOpsOnScratch(candidateOps)) {
        return false;
    }

    _stagedOps = std::move(candidateOps);
    _claimedStartPods.clear();
    _activeJumpHasChain = false;
    _guidedOriginBx = bx;
    _guidedOriginBy = by;
    _guidedPodIndex = resolvedIdx;
    _guidedMoveActive = false;
    _guidedMidHop = false;
    _awaitingSuperProngBonusMove = false;
    _awaitingConfirmation = true;
    syncPreparedStateToGame();

    if (!_confirmAllMoves && _stagedOps.size() <= 1) {
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

    int bx = x - 1;
    int by = y - 1;
    if (!_board->isValidCoord(bx, by)) {
        return false;
    }

    std::vector<OctiMoveOp> candidateOps;
    OctiBoard postBoard;
    const OctiBoard *activeBoard = _board;
    int removedFromSq = 0;
    if (!_stagedOps.empty()) {
        if (!buildPostPreludeBoard(candidateOps, postBoard)) {
            return false;
        }
        activeBoard = &postBoard;
        for (const auto &pOp : candidateOps) {
            if (pOp.x0 == bx && pOp.y0 == by) {
                ++removedFromSq;
            }
        }
    }

    OctiPlayer turn = game->getTurn();
    const OctiSquare &sq = activeBoard->squareAt(bx, by);
    int resolvedIdx = resolvePodIndexOnBoard(
        sq, x, y, podIndex, turn, false, removedFromSq);
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
    candidateOps.push_back(op);

    if (!validateOpsOnScratch(candidateOps)) {
        return false;
    }

    _stagedOps = std::move(candidateOps);
    _claimedStartPods.clear();
    _activeJumpHasChain = false;
    _guidedMoveActive = false;
    _guidedMidHop = false;
    _awaitingSuperProngBonusMove = false;
    _awaitingConfirmation = true;
    syncPreparedStateToGame();

    if (!_confirmAllMoves && _stagedOps.size() <= 1) {
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

    // Case B: Appending a continuation hop to an active staged jump chain
    if (_activeJumpHasChain && !_stagedOps.empty() &&
        fbx == _activeJumpCurBx && fby == _activeJumpCurBy) {
        OctiBoard chainRefBoard;
        std::size_t chainBonusCount = 0;
        const OctiBoard *activeChainBoard = _board;
        if (buildBoardAfterBonusOps(_stagedOps, chainRefBoard, &chainBonusCount) &&
            chainBonusCount > 0) {
            activeChainBoard = &chainRefBoard;
        }

        OctiMoveOp &activeOp = _stagedOps.back();
        const OctiSquare &startSq =
            activeChainBoard->squareAt(_activeJumpStartBx, _activeJumpStartBy);
        std::vector<OctiPod> ghostPods = startSq.pods();

        std::vector<OctiHop> extraHops;
        std::vector<std::pair<int, int>> nextJumped;
        bool chainJumpFound = findJumpPathBfs(
            activeChainBoard, edgeless, turn,
            _activeJumpStartBx, _activeJumpStartBy,
            _activeJumpCurBx, _activeJumpCurBy,
            tbx, tby, activeOp.pod, ghostPods,
            _activeJumpedCoords, extraHops, nextJumped);

        if (chainJumpFound) {
            std::vector<OctiMoveOp> candidateOps = _stagedOps;
            std::vector<std::size_t> matchingStackOpIndices;
            for (std::size_t idx = chainBonusCount; idx < candidateOps.size(); ++idx) {
                const OctiMoveOp &cop = candidateOps[idx];
                if (cop.x0 == _activeJumpStartBx && cop.y0 == _activeJumpStartBy &&
                    !cop.hops.empty()) {
                    const OctiHop &lastHop = cop.hops.back();
                    if (lastHop.x == _activeJumpCurBx && lastHop.y == _activeJumpCurBy) {
                        bool canFollow = true;
                        int cx = _activeJumpCurBx;
                        int cy = _activeJumpCurBy;
                        for (const auto &eh : extraHops) {
                            int hdx = 0, hdy = 0;
                            if (!computeDisplacement(cx, cy, eh.x, eh.y, edgeless, w, h, hdx, hdy)) {
                                canFollow = false;
                                break;
                            }
                            OctiDir reqDir = deltaToDir(hdx / 2, hdy / 2);
                            if (reqDir == DIR_UNDEF || cop.pod.prongAt(reqDir) == NO_PRONG) {
                                canFollow = false;
                                break;
                            }
                            cx = eh.x;
                            cy = eh.y;
                        }
                        if (canFollow) {
                            matchingStackOpIndices.push_back(idx);
                        }
                    }
                }
            }
            if (matchingStackOpIndices.empty()) {
                matchingStackOpIndices.push_back(candidateOps.size() - 1);
            }
            for (std::size_t opIdxToExtend : matchingStackOpIndices) {
                for (const auto &eh : extraHops) {
                    candidateOps[opIdxToExtend].hops.push_back(eh);
                }
            }

            if (validateOpsOnScratch(candidateOps)) {
                std::size_t prevJumpedCount = _activeJumpedCoords.size();
                for (std::size_t opIdxToExtend : matchingStackOpIndices) {
                    OctiMoveOp &targetOp = _stagedOps[opIdxToExtend];
                    for (std::size_t i = 0; i < extraHops.size(); ++i) {
                        std::size_t hopIdx = targetOp.hops.size();
                        targetOp.hops.push_back(extraHops[i]);
                        std::pair<int, int> jc = nextJumped[prevJumpedCount + i];
                        const OctiSquare &jsq = activeChainBoard->squareAt(jc.first, jc.second);
                        if (jsq.getNumPods() > 0) {
                            bool hasEnemy = jsq.hasEnemyOf(turn);
                            int jx1 = jc.first + 1;
                            int jy1 = jc.second + 1;
                            _hopCaptureLinks.push_back({opIdxToExtend, hopIdx, jx1, jy1});
                            bool alreadyListed = false;
                            for (const auto &cc : _captureChoices) {
                                if (cc.jumpedX == jx1 && cc.jumpedY == jy1) {
                                    alreadyListed = true;
                                    break;
                                }
                            }
                            if (!alreadyListed) {
                                _captureChoices.push_back({
                                    jx1, jy1, hasEnemy, false
                                });
                            }
                        }
                    }
                }

                _activeJumpedCoords = nextJumped;
                _activeJumpCurBx = tbx;
                _activeJumpCurBy = tby;
                _guidedCurBx = tbx;
                _guidedCurBy = tby;

                bool hasContinuation = hasContinuationJumpFrom(
                    activeChainBoard, tbx, tby, activeOp.pod,
                    _activeJumpStartBx, _activeJumpStartBy, ghostPods, nextJumped);
                if (hasContinuation) {
                    _guidedMidHop = true;
                    _guidedMoveActive = true;
                } else {
                    auto unclaimed = unclaimedMovablePodIndicesAtOrigin();
                    if (!unclaimed.empty() && !hasDistinctUnclaimedPodsAtOrigin()) {
                        _guidedCurBx = _guidedOriginBx;
                        _guidedCurBy = _guidedOriginBy;
                        _guidedPodIndex = unclaimed.front();
                        _guidedMoveAllStack = false;
                        _guidedMidHop = false;
                        _guidedMoveActive = true;
                    } else {
                        _guidedMidHop = false;
                        _guidedMoveActive = false;
                    }
                }
                _awaitingSuperProngBonusMove = canPromoteStagedToSuperProngPreludes();
                _awaitingConfirmation = true;
                syncPreparedStateToGame();
                return true;
            }
        }

        if (!canPromoteStagedToSuperProngPreludes()) {
            return false;
        }
    }

    if (fbx == tbx && fby == tby) {
        return false;
    }

    // Case C: Starting a new move/jump op (initial, ',' composite, or ';' superprong continuation)
    struct MoveCandidatePlan {
        const OctiBoard *refBoard = nullptr;
        std::vector<OctiMoveOp> prefixOps;
        std::vector<OctiMoveOp> fullOps;
        std::vector<int> podIndicesToMove;
        std::vector<OctiHop> chosenHops;
        std::vector<std::pair<int, int>> chosenJumped;
        std::vector<OctiPod> ghostPods;
        OctiPod movingPod;
        int resolvedIdx = -1;
        bool isDist1 = false;
        bool clearClaimsFirst = false;
    };

    bool moveStack = moveAllMatchingStack || _moveEntireMatchingStack;

    auto tryBuildPlan = [&](const OctiBoard *refBoard,
                            const std::vector<OctiMoveOp> &prefixOps,
                            bool excludeClaimed,
                            int removedFromSq,
                            bool clearClaimsFirst,
                            MoveCandidatePlan &outPlan) -> bool {
        if (!refBoard || !refBoard->isValidCoord(fbx, fby)) {
            return false;
        }
        const OctiSquare &startSq = refBoard->squareAt(fbx, fby);
        int resolvedIdx = resolvePodIndexOnBoard(
            startSq, fromX, fromY, podIndex, turn, excludeClaimed, removedFromSq);
        if (resolvedIdx < 0 || resolvedIdx >= startSq.getNumPods()) {
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
            if (!findJumpPathBfs(refBoard, edgeless, turn,
                                 fbx, fby, fbx, fby, tbx, tby,
                                 movingPod, ghostPods, {},
                                 chosenHops, chosenJumped)) {
                return false;
            }
        }

        std::vector<int> podIndicesToMove;
        podIndicesToMove.push_back(resolvedIdx);

        if (moveStack && refBoard->getGameMode() != GAME_MODE_OFK) {
            for (int i = 0; i < startSq.getNumPods(); ++i) {
                if (i == resolvedIdx || (excludeClaimed && isPodClaimed(fbx, fby, i))) {
                    continue;
                }
                const OctiPod &otherPod = startSq.getPodAt(i);
                if (otherPod.getType() != playerToPodType(turn)) {
                    continue;
                }
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

        std::vector<OctiMoveOp> candidateOps = prefixOps;
        for (int pIdx : podIndicesToMove) {
            OctiMoveOp op;
            op.x0 = fbx;
            op.y0 = fby;
            op.pod = startSq.getPodAt(pIdx);
            op.hops = chosenHops;
            op.isSuperProngBonusMove = false;
            candidateOps.push_back(op);
        }

        if (!validateOpsOnScratch(candidateOps)) {
            return false;
        }

        outPlan.refBoard = refBoard;
        outPlan.prefixOps = prefixOps;
        outPlan.fullOps = std::move(candidateOps);
        outPlan.podIndicesToMove = std::move(podIndicesToMove);
        outPlan.chosenHops = std::move(chosenHops);
        outPlan.chosenJumped = std::move(chosenJumped);
        outPlan.ghostPods = std::move(ghostPods);
        outPlan.movingPod = movingPod;
        outPlan.resolvedIdx = resolvedIdx;
        outPlan.isDist1 = isDist1;
        outPlan.clearClaimsFirst = clearClaimsFirst;
        return true;
    };

    OctiBoard postBoard;
    OctiBoard bonusRefBoard;
    MoveCandidatePlan chosenPlan;
    bool foundPlan = false;

    if (_stagedOps.empty()) {
        foundPlan = tryBuildPlan(_board, {}, true, 0, false, chosenPlan);
    } else {
        std::vector<OctiMoveOp> promotedOps;
        MoveCandidatePlan semiPlan;
        bool semiOk = false;
        if (buildPostPreludeBoard(promotedOps, postBoard)) {
            int removedFromSq = 0;
            for (const auto &pOp : promotedOps) {
                if (pOp.x0 == fbx && pOp.y0 == fby) {
                    ++removedFromSq;
                }
            }
            semiOk = tryBuildPlan(&postBoard, promotedOps, false, removedFromSq, true, semiPlan);
        }

        MoveCandidatePlan commaPlan;
        bool commaOk = false;
        std::size_t bonusCount = 0;
        if (buildBoardAfterBonusOps(_stagedOps, bonusRefBoard, &bonusCount) &&
            _stagedOps.size() > bonusCount) {
            bool sameRegularOrigin = true;
            for (std::size_t idx = bonusCount; idx < _stagedOps.size(); ++idx) {
                const OctiMoveOp &existingOp = _stagedOps[idx];
                if (existingOp.hops.empty() ||
                    existingOp.x0 != fbx || existingOp.y0 != fby) {
                    sameRegularOrigin = false;
                    break;
                }
            }
            if (sameRegularOrigin) {
                const OctiBoard *commaBoard = (bonusCount > 0) ? &bonusRefBoard : _board;
                commaOk = tryBuildPlan(commaBoard, _stagedOps, true, 0, false, commaPlan);
            }
        }

        // Disambiguation: if valid ONLY as ',', keep composite ','; otherwise promote to ';'
        if (semiOk) {
            chosenPlan = std::move(semiPlan);
            foundPlan = true;
        } else if (commaOk) {
            chosenPlan = std::move(commaPlan);
            foundPlan = true;
        }
    }

    if (!foundPlan) {
        return false;
    }

    if (chosenPlan.clearClaimsFirst) {
        _claimedStartPods.clear();
    }
    _stagedOps = chosenPlan.prefixOps;

    const OctiSquare &chosenStartSq = chosenPlan.refBoard->squareAt(fbx, fby);
    int startKey = fbx * 10 + fby;
    _guidedOriginBx = fbx;
    _guidedOriginBy = fby;
    for (int pIdx : chosenPlan.podIndicesToMove) {
        std::size_t opIdx = _stagedOps.size();
        OctiMoveOp op;
        op.x0 = fbx;
        op.y0 = fby;
        op.pod = chosenStartSq.getPodAt(pIdx);
        op.hops = chosenPlan.chosenHops;
        op.isSuperProngBonusMove = false;
        _stagedOps.push_back(op);
        _claimedStartPods.push_back({startKey, pIdx});

        if (!chosenPlan.isDist1) {
            for (std::size_t hi = 0; hi < chosenPlan.chosenJumped.size(); ++hi) {
                std::pair<int, int> jc = chosenPlan.chosenJumped[hi];
                const OctiSquare &jsq = chosenPlan.refBoard->squareAt(jc.first, jc.second);
                if (jsq.getNumPods() > 0) {
                    bool hasEnemy = jsq.hasEnemyOf(turn);
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
                            jx1, jy1, hasEnemy, false
                        });
                    }
                }
            }
        }
    }

    _guidedOriginBx = fbx;
    _guidedOriginBy = fby;
    _activeJumpHasChain = !chosenPlan.isDist1;
    _activeJumpStartBx = fbx;
    _activeJumpStartBy = fby;
    _activeJumpCurBx = tbx;
    _activeJumpCurBy = tby;
    _activeJumpedCoords = chosenPlan.chosenJumped;

    bool hasMorePodsOnStart = hasOtherMovableFriendlyPods(fbx, fby, turn);
    bool hasContinuation = !chosenPlan.isDist1 &&
        hasContinuationJumpFrom(chosenPlan.refBoard, tbx, tby, chosenPlan.movingPod,
                                fbx, fby, chosenPlan.ghostPods, chosenPlan.chosenJumped);
    bool hasOptionalCapture = false;
    for (const auto &cc : _captureChoices) {
        if (!cc.mandatory) {
            hasOptionalCapture = true;
            break;
        }
    }

    if (hasContinuation) {
        _guidedMidHop = true;
        _guidedMoveActive = true;
        _guidedCurBx = tbx;
        _guidedCurBy = tby;
        _guidedPodIndex = chosenPlan.resolvedIdx;
    } else {
        auto unclaimed = unclaimedMovablePodIndicesAtOrigin();
        if (!unclaimed.empty() && !hasDistinctUnclaimedPodsAtOrigin()) {
            _guidedCurBx = _guidedOriginBx;
            _guidedCurBy = _guidedOriginBy;
            _guidedPodIndex = unclaimed.front();
            _guidedMoveAllStack = false;
            _guidedMidHop = false;
            _guidedMoveActive = true;
        } else {
            _guidedMidHop = false;
            _guidedMoveActive = false;
        }
    }

    _awaitingSuperProngBonusMove = canPromoteStagedToSuperProngPreludes();
    bool shouldPause = _confirmAllMoves ||
                       !chosenPlan.isDist1 ||
                       hasMorePodsOnStart ||
                       hasContinuation ||
                       hasOptionalCapture ||
                       _stagedOps.size() > 1 ||
                       _awaitingSuperProngBonusMove;

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

    OctiPlayer turn = game->getTurn();
    int chosenGuidedPodIdx = -1;

    if (_stagedOps.empty()) {
        const OctiSquare &sq = _board->squareAt(bx, by);
        chosenGuidedPodIdx = resolvePodIndex(sq, x, y, podIndex, turn, true);
        if (chosenGuidedPodIdx < 0 || chosenGuidedPodIdx >= sq.getNumPods()) {
            return false;
        }
    } else {
        bool foundPod = false;

        std::size_t bonusCount = 0;
        OctiBoard bonusRefBoard;
        if (buildBoardAfterBonusOps(_stagedOps, bonusRefBoard, &bonusCount) &&
            _stagedOps.size() > bonusCount) {
            bool sameRegularOrigin = true;
            for (std::size_t idx = bonusCount; idx < _stagedOps.size(); ++idx) {
                const OctiMoveOp &op = _stagedOps[idx];
                if (op.hops.empty() || op.x0 != bx || op.y0 != by) {
                    sameRegularOrigin = false;
                    break;
                }
            }
            if (sameRegularOrigin) {
                const OctiBoard *commaBoard = (bonusCount > 0) ? &bonusRefBoard : _board;
                const OctiSquare &sq = commaBoard->squareAt(bx, by);
                int cIdx = resolvePodIndexOnBoard(sq, x, y, podIndex, turn, true, 0);
                if (cIdx >= 0 && cIdx < sq.getNumPods()) {
                    chosenGuidedPodIdx = cIdx;
                    foundPod = true;
                }
            }
        }

        if (!foundPod) {
            std::vector<OctiMoveOp> promotedOps;
            OctiBoard postBoard;
            if (buildPostPreludeBoard(promotedOps, postBoard)) {
                int removedFromSq = 0;
                for (const auto &pOp : promotedOps) {
                    if (pOp.x0 == bx && pOp.y0 == by) {
                        ++removedFromSq;
                    }
                }
                const OctiSquare &sq = postBoard.squareAt(bx, by);
                int sIdx = resolvePodIndexOnBoard(sq, x, y, podIndex, turn, false, removedFromSq);
                if (sIdx >= 0 && sIdx < sq.getNumPods()) {
                    chosenGuidedPodIdx = sIdx;
                    foundPod = true;
                }
            }
        }

        if (!foundPod) {
            return false;
        }
    }

    _guidedOriginBx = bx;
    _guidedOriginBy = by;
    _guidedCurBx = bx;
    _guidedCurBy = by;
    _guidedPodIndex = chosenGuidedPodIdx;
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
    if (_guidedOriginBx >= 0 && _guidedOriginBy >= 0) {
        return {_guidedOriginBx + 1, _guidedOriginBy + 1};
    }
    if (!_stagedOps.empty()) {
        return {_stagedOps.front().x0 + 1, _stagedOps.front().y0 + 1};
    }
    return {0, 0};
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

    auto collectOriginTargetsOnBoard = [&](const OctiBoard *refBoard,
                                           const std::vector<OctiMoveOp> &prefixOps,
                                           int originBx, int originBy,
                                           bool excludeClaimed,
                                           int removedFromSq) {
        if (!refBoard || !refBoard->isValidCoord(originBx, originBy)) {
            return;
        }
        const OctiSquare &startSq = refBoard->squareAt(originBx, originBy);
        int resolvedIdx = resolvePodIndexOnBoard(
            startSq, originBx + 1, originBy + 1, _guidedPodIndex,
            turn, excludeClaimed, removedFromSq);
        if (resolvedIdx < 0 || resolvedIdx >= startSq.getNumPods()) {
            return;
        }
        const OctiPod &pod = startSq.getPodAt(resolvedIdx);
        if (pod.getType() != playerToPodType(turn)) {
            return;
        }
        std::vector<OctiPod> ghostPods = startSq.pods();

        // 1. Distance-1 steps
        for (int d = DIR_N; d <= DIR_NW; ++d) {
            OctiDir dir = static_cast<OctiDir>(d);
            if (pod.prongAt(dir) == NO_PRONG) {
                continue;
            }
            int jdx = dirDeltaX(dir);
            int jdy = dirDeltaY(dir);
            int sx = edgeless ? (((originBx + jdx) % w + w) % w) : (originBx + jdx);
            int sy = edgeless ? (((originBy + jdy) % h + h) % h) : (originBy + jdy);
            if (!refBoard->isValidCoord(sx, sy)) {
                continue;
            }
            const OctiSquare &destSq = refBoard->squareAt(sx, sy);
            if (refBoard->getGameMode() == GAME_MODE_OFK) {
                if (!destSq.isEmpty()) continue;
            } else {
                if (destSq.hasEnemyOf(turn)) continue;
                if (destSq.getNumPods() >= 7) continue;
            }
            OctiHop hop; hop.x = sx; hop.y = sy; hop.capture = false;
            OctiMoveOp op; op.x0 = originBx; op.y0 = originBy;
            op.pod = pod; op.hops.push_back(hop);
            op.isSuperProngBonusMove = false;
            std::vector<OctiMoveOp> cand = prefixOps;
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
            int jdx = dirDeltaX(dir);
            int jdy = dirDeltaY(dir);
            int dx = jdx * 2;
            int dy = jdy * 2;
            int jx = edgeless ? (((originBx + jdx) % w + w) % w) : (originBx + jdx);
            int jy = edgeless ? (((originBy + jdy) % h + h) % h) : (originBy + jdy);
            int lx = edgeless ? (((originBx + dx) % w + w) % w) : (originBx + dx);
            int ly = edgeless ? (((originBy + dy) % h + h) % h) : (originBy + dy);
            if (!refBoard->isValidCoord(jx, jy) || !refBoard->isValidCoord(lx, ly)) {
                continue;
            }
            const OctiSquare &jumpedSq = refBoard->squareAt(jx, jy);
            bool jumpedEmpty = (jx == originBx && jy == originBy)
                ? jumpedSq.isEmptyWithout(ghostPods) : jumpedSq.isEmpty();
            if (jumpedEmpty) continue;

            const OctiSquare &landSq = refBoard->squareAt(lx, ly);
            bool landEmpty = (lx == originBx && ly == originBy)
                ? landSq.isEmptyWithout(ghostPods) : landSq.isEmpty();
            if (!landEmpty) continue;

            OctiHop hop; hop.x = lx; hop.y = ly;
            hop.capture = jumpedSq.hasEnemyOf(turn);
            OctiMoveOp op; op.x0 = originBx; op.y0 = originBy;
            op.pod = pod; op.hops.push_back(hop);
            op.isSuperProngBonusMove = false;
            std::vector<OctiMoveOp> cand = prefixOps;
            cand.push_back(op);
            if (validateOpsOnScratch(cand)) {
                if (seen.insert({lx + 1, ly + 1}).second) {
                    result.push_back({lx + 1, ly + 1});
                }
            }
        }
    };

    if (!_guidedMidHop) {
        if (!_board->isValidCoord(_guidedOriginBx, _guidedOriginBy)) {
            return result;
        }
        if (_stagedOps.empty()) {
            collectOriginTargetsOnBoard(_board, {}, _guidedOriginBx, _guidedOriginBy, true, 0);
        } else {
            std::vector<OctiMoveOp> promotedOps;
            OctiBoard postBoard;
            if (buildPostPreludeBoard(promotedOps, postBoard)) {
                int removedFromSq = 0;
                for (const auto &pOp : promotedOps) {
                    if (pOp.x0 == _guidedOriginBx && pOp.y0 == _guidedOriginBy) {
                        ++removedFromSq;
                    }
                }
                collectOriginTargetsOnBoard(
                    &postBoard, promotedOps, _guidedOriginBx, _guidedOriginBy, false, removedFromSq);
            }

            std::size_t bonusCount = 0;
            OctiBoard bonusRefBoard;
            if (buildBoardAfterBonusOps(_stagedOps, bonusRefBoard, &bonusCount) &&
                _stagedOps.size() > bonusCount) {
                bool sameRegularOrigin = true;
                for (std::size_t idx = bonusCount; idx < _stagedOps.size(); ++idx) {
                    const OctiMoveOp &op = _stagedOps[idx];
                    if (op.hops.empty() ||
                        op.x0 != _guidedOriginBx || op.y0 != _guidedOriginBy) {
                        sameRegularOrigin = false;
                        break;
                    }
                }
                if (sameRegularOrigin) {
                    const OctiBoard *commaBoard = (bonusCount > 0) ? &bonusRefBoard : _board;
                    collectOriginTargetsOnBoard(
                        commaBoard, _stagedOps, _guidedOriginBx, _guidedOriginBy, true, 0);
                }
            }
        }
    } else {
        // Mid-jump continuation from (_guidedCurBx, _guidedCurBy)
        if (_stagedOps.empty()) {
            return result;
        }
        if (!_board->isValidCoord(_guidedOriginBx, _guidedOriginBy) ||
            !_board->isValidCoord(_guidedCurBx, _guidedCurBy)) {
            return result;
        }

        OctiBoard chainRefBoard;
        std::size_t chainBonusCount = 0;
        const OctiBoard *activeChainBoard = _board;
        if (buildBoardAfterBonusOps(_stagedOps, chainRefBoard, &chainBonusCount) &&
            chainBonusCount > 0) {
            activeChainBoard = &chainRefBoard;
        }

        const OctiMoveOp &curOp = _stagedOps.back();
        const OctiPod &pod = curOp.pod;
        const OctiSquare &startSq =
            activeChainBoard->squareAt(_guidedOriginBx, _guidedOriginBy);
        std::vector<OctiPod> ghostPods = startSq.pods();

        for (int d = DIR_N; d <= DIR_NW; ++d) {
            OctiDir dir = static_cast<OctiDir>(d);
            if (pod.prongAt(dir) == NO_PRONG) {
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
            if (!activeChainBoard->isValidCoord(jx, jy) ||
                !activeChainBoard->isValidCoord(lx, ly)) {
                continue;
            }
            if (std::find(_activeJumpedCoords.begin(), _activeJumpedCoords.end(),
                          std::make_pair(jx, jy)) != _activeJumpedCoords.end()) {
                continue;
            }
            const OctiSquare &jumpedSq = activeChainBoard->squareAt(jx, jy);
            bool jumpedEmpty = (jx == _guidedOriginBx && jy == _guidedOriginBy)
                ? jumpedSq.isEmptyWithout(ghostPods) : jumpedSq.isEmpty();
            if (jumpedEmpty) continue;

            const OctiSquare &landSq = activeChainBoard->squareAt(lx, ly);
            bool landEmpty = (lx == _guidedOriginBx && ly == _guidedOriginBy)
                ? landSq.isEmptyWithout(ghostPods) : landSq.isEmpty();
            if (!landEmpty) continue;

            std::vector<OctiMoveOp> cand = _stagedOps;
            OctiHop hop; hop.x = lx; hop.y = ly;
            hop.capture = jumpedSq.hasEnemyOf(turn);
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

    int fromBx = _guidedMidHop ? _guidedCurBx : _guidedOriginBx;
    int fromBy = _guidedMidHop ? _guidedCurBy : _guidedOriginBy;

    if (!stageOrCommitPodMoveOrJump(fromBx + 1, fromBy + 1, _guidedPodIndex,
                                    toX, toY, _guidedMoveAllStack)) {
        return false;
    }
    if (_guidedMoveActive && _guidedMidHop && guidedNextSquares().empty()) {
        auto unclaimed = unclaimedMovablePodIndicesAtOrigin();
        if (!unclaimed.empty() && !hasDistinctUnclaimedPodsAtOrigin()) {
            _guidedCurBx = _guidedOriginBx;
            _guidedCurBy = _guidedOriginBy;
            _guidedPodIndex = unclaimed.front();
            _guidedMoveAllStack = false;
            _guidedMidHop = false;
            _guidedMoveActive = true;
        } else {
            _guidedMidHop = false;
            _guidedMoveActive = false;
        }
    }
    return true;
}

bool OctiMoveComposer::finishGuidedMoveHere()
{
    if (!_guidedMoveActive) {
        return false;
    }
    _guidedMoveActive = false;
    _activeJumpHasChain = false;
    _guidedMidHop = false;
    return true;
}

std::vector<int> OctiMoveComposer::unclaimedMovablePodIndicesAtOrigin() const
{
    std::vector<int> result;
    if (_stagedOps.empty() || !_board || !_board->getGame()) {
        return result;
    }
    OctiBoard refBoard;
    std::size_t bonusCount = 0;
    if (!buildBoardAfterBonusOps(_stagedOps, refBoard, &bonusCount) ||
        _stagedOps.size() <= bonusCount) {
        return result;
    }
    const OctiMoveOp &firstRegOp = _stagedOps[bonusCount];
    if (firstRegOp.hops.empty()) {
        return result;
    }
    int origBx = firstRegOp.x0;
    int origBy = firstRegOp.y0;
    const OctiBoard *activeBoard = (bonusCount > 0) ? &refBoard : _board;
    if (!activeBoard->isValidCoord(origBx, origBy)) {
        return result;
    }
    OctiPlayer turn = _board->getGame()->getTurn();
    const OctiSquare &sq = activeBoard->squareAt(origBx, origBy);
    OctiPodType desiredType = playerToPodType(turn);
    auto sorted = sortedPodIndicesMostProngsFirst(sq);
    for (int idx : sorted) {
        if (isPodClaimed(origBx, origBy, idx)) {
            continue;
        }
        const OctiPod &pod = sq.getPodAt(idx);
        if (pod.getType() != desiredType) {
            continue;
        }
        if (pod.getNumProngs() > 0) {
            auto dests = legalDestinationsFrom(origBx + 1, origBy + 1, idx);
            if (!dests.empty()) {
                result.push_back(idx);
            }
        }
    }
    return result;
}

bool OctiMoveComposer::hasDistinctUnclaimedPodsAtOrigin() const
{
    auto indices = unclaimedMovablePodIndicesAtOrigin();
    if (indices.size() < 2 || _stagedOps.empty() || !_board) {
        return false;
    }
    OctiBoard refBoard;
    std::size_t bonusCount = 0;
    if (!buildBoardAfterBonusOps(_stagedOps, refBoard, &bonusCount) ||
        _stagedOps.size() <= bonusCount) {
        return false;
    }
    int origBx = _stagedOps[bonusCount].x0;
    int origBy = _stagedOps[bonusCount].y0;
    const OctiBoard *activeBoard = (bonusCount > 0) ? &refBoard : _board;
    const OctiSquare &sq = activeBoard->squareAt(origBx, origBy);
    std::uint32_t firstConf = (sq.getPodAt(indices[0]).rawConfig() & 0xFFFFu);
    for (std::size_t i = 1; i < indices.size(); ++i) {
        if ((sq.getPodAt(indices[i]).rawConfig() & 0xFFFFu) != firstConf) {
            return true;
        }
    }
    return false;
}

bool OctiMoveComposer::hasOtherMovablePodAtOrigin() const
{
    return !unclaimedMovablePodIndicesAtOrigin().empty();
}

bool OctiMoveComposer::isSquarePodClaimed(int x, int y, int podIndex) const
{
    if (!_board || !_board->isValidCoord(x - 1, y - 1) || podIndex < 0) {
        return false;
    }
    return isPodClaimed(x - 1, y - 1, podIndex);
}

int OctiMoveComposer::firstUnclaimedFriendlyPodIndex(int x, int y) const
{
    if (!_board || !_board->getGame() || !_board->isValidCoord(x - 1, y - 1)) {
        return -1;
    }
    int bx = x - 1;
    int by = y - 1;
    OctiBoard refBoard;
    std::size_t bonusCount = 0;
    const OctiBoard *activeBoard = _board;
    if (!_stagedOps.empty() &&
        buildBoardAfterBonusOps(_stagedOps, refBoard, &bonusCount) &&
        bonusCount > 0) {
        activeBoard = &refBoard;
    }
    const OctiSquare &sq = activeBoard->squareAt(bx, by);
    OctiPodType desiredType = playerToPodType(_board->getGame()->getTurn());
    auto sorted = sortedPodIndicesMostProngsFirst(sq);
    for (int idx : sorted) {
        if (sq.getPodAt(idx).getType() == desiredType && !isPodClaimed(bx, by, idx)) {
            return idx;
        }
    }
    return -1;
}

bool OctiMoveComposer::canAddMoreMoves() const
{
    return hasOtherMovablePodAtOrigin() ||
           isAwaitingSuperProngBonusMove() ||
           (_guidedMoveActive && _guidedMidHop && !guidedNextSquares().empty());
}

bool OctiMoveComposer::hasEighthProngBonusHop() const
{
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

    _awaitingSuperProngBonusMove = canPromoteStagedToSuperProngPreludes();
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

    _awaitingSuperProngBonusMove = canPromoteStagedToSuperProngPreludes();
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
    return _awaitingConfirmation && canPromoteStagedToSuperProngPreludes();
}

std::string OctiMoveComposer::stagedMoveNotation() const
{
    if (_stagedOps.empty()) {
        return "";
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
        outBoard.getGame()->applyUncommittedTurnForPreview(_stagedOps);
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
    std::set<std::pair<int, int>> seen;

    // If querying from the current end of an active jump chain
    if (_activeJumpHasChain && !_stagedOps.empty() &&
        bx == _activeJumpCurBx && by == _activeJumpCurBy) {
        OctiBoard chainRefBoard;
        std::size_t chainBonusCount = 0;
        const OctiBoard *activeChainBoard = _board;
        if (buildBoardAfterBonusOps(_stagedOps, chainRefBoard, &chainBonusCount) &&
            chainBonusCount > 0) {
            activeChainBoard = &chainRefBoard;
        }

        const OctiPod &pod = _stagedOps.back().pod;
        const OctiSquare &startSq =
            activeChainBoard->squareAt(_activeJumpStartBx, _activeJumpStartBy);
        std::vector<OctiPod> ghostPods = startSq.pods();

        for (int ty = 0; ty < h; ++ty) {
            for (int tx = 0; tx < w; ++tx) {
                std::vector<OctiHop> extraHops;
                std::vector<std::pair<int, int>> nextJumped;
                if (findJumpPathBfs(activeChainBoard, edgeless, turn,
                                    _activeJumpStartBx, _activeJumpStartBy,
                                    _activeJumpCurBx, _activeJumpCurBy,
                                    tx, ty, pod, ghostPods,
                                    _activeJumpedCoords, extraHops, nextJumped)) {
                    std::pair<int, int> dest(tx + 1, ty + 1);
                    if (seen.insert(dest).second) {
                        result.push_back(dest);
                    }
                }
            }
        }
        if (!canPromoteStagedToSuperProngPreludes()) {
            return result;
        }
    }

    auto collectFromBoard = [&](const OctiBoard *refBoard,
                                const std::vector<OctiMoveOp> &prefixOps,
                                bool excludeClaimed,
                                int removedFromSq) {
        if (!refBoard || !refBoard->getGame()) {
            return;
        }
        const OctiSquare &sq = refBoard->squareAt(bx, by);
        int resolvedIdx = resolvePodIndexOnBoard(
            sq, x, y, podIndex, turn, excludeClaimed, removedFromSq);
        if (podIndex >= 0 && resolvedIdx < 0) {
            return;
        }
        bool filterByPod = (podIndex >= 0 && resolvedIdx >= 0);
        std::uint32_t targetPodConf = filterByPod
            ? (sq.getPodAt(resolvedIdx).rawConfig() & 0xFFFFu) : 0u;

        std::vector<OctiMove> legal = refBoard->getGame()->generateLegalMoves();
        for (const auto &mv : legal) {
            const auto &ops = mv.getOps();
            if (ops.size() != 1 || ops[0].hops.empty() || ops[0].isSuperProngBonusMove) {
                continue;
            }
            const OctiMoveOp &op = ops[0];
            if (op.x0 != bx || op.y0 != by) {
                continue;
            }
            if (filterByPod && (op.pod.rawConfig() & 0xFFFFu) != targetPodConf) {
                continue;
            }
            if (!prefixOps.empty()) {
                std::vector<OctiMoveOp> cand = prefixOps;
                cand.push_back(op);
                if (!validateOpsOnScratch(cand)) {
                    continue;
                }
            }
            const OctiHop &lastHop = op.hops.back();
            std::pair<int, int> dest(lastHop.x + 1, lastHop.y + 1);
            if (seen.insert(dest).second) {
                result.push_back(dest);
            }
        }
    };

    if (_stagedOps.empty()) {
        collectFromBoard(_board, {}, true, 0);
    } else {
        std::vector<OctiMoveOp> promotedOps;
        OctiBoard postBoard;
        if (buildPostPreludeBoard(promotedOps, postBoard)) {
            int removedFromSq = 0;
            for (const auto &pOp : promotedOps) {
                if (pOp.x0 == bx && pOp.y0 == by) {
                    ++removedFromSq;
                }
            }
            collectFromBoard(&postBoard, promotedOps, false, removedFromSq);
        }

        std::size_t bonusCount = 0;
        OctiBoard bonusRefBoard;
        if (buildBoardAfterBonusOps(_stagedOps, bonusRefBoard, &bonusCount) &&
            _stagedOps.size() > bonusCount) {
            bool sameRegularOrigin = true;
            for (std::size_t idx = bonusCount; idx < _stagedOps.size(); ++idx) {
                const OctiMoveOp &op = _stagedOps[idx];
                if (op.hops.empty() || op.x0 != bx || op.y0 != by) {
                    sameRegularOrigin = false;
                    break;
                }
            }
            if (sameRegularOrigin) {
                const OctiBoard *commaBoard = (bonusCount > 0) ? &bonusRefBoard : _board;
                collectFromBoard(commaBoard, _stagedOps, true, 0);
            }
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

    std::vector<OctiMoveOp> prefixOps;
    OctiBoard postBoard;
    const OctiBoard *activeBoard = _board;
    int removedFromSq = 0;
    if (!_stagedOps.empty()) {
        if (!buildPostPreludeBoard(prefixOps, postBoard)) {
            return result;
        }
        activeBoard = &postBoard;
        for (const auto &pOp : prefixOps) {
            if (pOp.x0 == bx && pOp.y0 == by) {
                ++removedFromSq;
            }
        }
    }

    const OctiGame *activeGame = activeBoard->getGame();
    OctiPlayer turn = activeGame->getTurn();
    if (superProng) {
        if (!activeGame->useSuperProngs() || activeGame->getFreeSuperProngs(turn) <= 0) {
            return result;
        }
    } else {
        if (activeGame->getFreeProngs(turn) <= 0) {
            return result;
        }
    }

    const OctiSquare &sq = activeBoard->squareAt(bx, by);
    int resolvedIdx = resolvePodIndexOnBoard(
        sq, x, y, podIndex, turn, false, removedFromSq);
    if (resolvedIdx < 0) {
        return result;
    }

    const OctiPod &pod = sq.getPodAt(resolvedIdx);
    for (int d = DIR_N; d <= DIR_NW; ++d) {
        OctiDir dir = static_cast<OctiDir>(d);
        if (pod.prongAt(dir) != NO_PRONG) {
            continue;
        }
        OctiMoveOp op;
        op.x0 = bx;
        op.y0 = by;
        op.pod = pod;
        op.addProngDir = dir;
        op.superProng = superProng;
        std::vector<OctiMoveOp> cand = prefixOps;
        cand.push_back(op);
        if (validateOpsOnScratch(cand)) {
            result.push_back(dir);
        }
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
    if (game->getMode() == GAME_MODE_OFK && liberate) {
        return result;
    }

    std::vector<OctiMoveOp> prefixOps;
    OctiBoard postBoard;
    const OctiBoard *activeBoard = _board;
    if (!_stagedOps.empty()) {
        if (!buildPostPreludeBoard(prefixOps, postBoard)) {
            return result;
        }
        activeBoard = &postBoard;
    }

    const OctiGame *activeGame = activeBoard->getGame();
    OctiPlayer turn = activeGame->getTurn();
    if (liberate) {
        bool canLibOwn = (activeGame->getCapturedPods(turn) > 0 &&
                          activeBoard->canLiberate(turn));
        bool canRevive = false;
        if (activeBoard->getGameMode() == GAME_MODE_4) {
            OctiPlayer partner = (turn == PLAYER_A ? PLAYER_C :
                                  turn == PLAYER_C ? PLAYER_A :
                                  turn == PLAYER_B ? PLAYER_D : PLAYER_B);
            if (activeGame->isPlayerEliminated(partner) &&
                activeGame->getCapturedPods(partner) > 0) {
                canRevive = true;
            }
        }
        if (!canLibOwn && !canRevive) {
            return result;
        }
    } else {
        if (activeGame->getFreePods(turn) <= 0 &&
            activeGame->getCapturedPods(turn) <= 0) {
            return result;
        }
    }

    int w = activeBoard->getBoardWidth();
    int h = activeBoard->getBoardHeight();
    for (int by = 0; by < h; ++by) {
        for (int bx = 0; bx < w; ++bx) {
            OctiMoveOp op;
            op.x0 = bx;
            op.y0 = by;
            op.addPod = true;
            op.liberate = liberate;
            std::vector<OctiMoveOp> cand = prefixOps;
            cand.push_back(op);
            if (validateOpsOnScratch(cand)) {
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
    std::string committedNotation = stagedMoveNotation();

    game->rollbackPreparedMove();

    try {
        OctiMove mv(opsToCommit);
        game->move(mv);
        committedNotation = mv.asString();
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
