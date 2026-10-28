/*
 * OctiMoveComposer.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_OCTI_MOVE_COMPOSER_HXX
#define OCTI_DESKTOP_OCTI_MOVE_COMPOSER_HXX

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "OctiBoard.hxx"
#include "OctiDefs.hxx"
#include "OctiGame.hxx"
#include "OctiMove.hxx"

namespace octi {

enum class DragKind {
    NONE                        = 0,
    ADD_POD_FROM_RESERVE        = 1,
    LIBERATE_POD_FROM_RESERVE   = 2,
    ADD_PRONG_FROM_RESERVE      = 3,
    ADD_SUPERPRONG_FROM_RESERVE = 4,
    REPOSITION_PRONG_ON_POD     = 5,
    MOVE_OR_JUMP_POD            = 6,
};

struct CaptureChoice {
    int jumpedX = 0;
    int jumpedY = 0;
    bool capture = true;
    bool mandatory = false;
};

std::vector<int> sortedPodIndicesMostProngsFirst(const OctiSquare &sq);

class OctiMoveComposer {
public:
    explicit OctiMoveComposer(OctiBoard *board = nullptr);
    ~OctiMoveComposer();

    void attachBoard(OctiBoard *board);
    OctiBoard *board() const;

    void setConfirmAllMoves(bool confirmAll);
    bool confirmAllMoves() const;

    void setMoveCommittedCallback(
        std::function<void(const std::string &moveNotation)> cb);

    void refreshLegalMoves();

    // Stack Fan & Pod selection on 1-based (x, y)
    void selectStackPodIndex(int x, int y, int podIndex);
    int selectedStackPodIndex(int x, int y) const;
    void setMoveEntireMatchingStack(bool moveAll);
    bool moveEntireMatchingStack() const;

    // Interactive Drag / Staging operations (1-based coordinates 1..W, 1..H)
    bool stageOrCommitAddPod(int targetX, int targetY, bool liberate);
    bool stageOrCommitAddProng(int x, int y, int podIndex, OctiDir dir, bool superProng);
    bool stageOrCommitRepositionProng(int x, int y, int podIndex, OctiDir srcDir, OctiDir tgtDir);
    bool stageOrCommitPodMoveOrJump(int fromX, int fromY, int podIndex,
                                    int toX, int toY,
                                    bool moveAllMatchingStack = false);

    // Guided Step / Multi-Hop Jump & Composite / Bonus Move Helpers (1-based coordinates)
    bool beginGuidedPodMove(int x, int y, int podIndex, bool moveAllMatchingStack = false);
    bool isGuidedMoveActive() const;
    std::pair<int, int> guidedCurrentSquare() const;
    std::pair<int, int> guidedOriginSquare() const;
    std::vector<std::pair<int, int>> guidedNextSquares() const;
    bool selectGuidedDestination(int toX, int toY);
    bool finishGuidedMoveHere();
    bool finishGuidedMove() { return finishGuidedMoveHere(); }
    bool hasOtherMovablePodAtOrigin() const;
    std::vector<int> unclaimedMovablePodIndicesAtOrigin() const;
    bool hasDistinctUnclaimedPodsAtOrigin() const;
    bool isSquarePodClaimed(int x, int y, int podIndex) const;
    int firstUnclaimedFriendlyPodIndex(int x, int y) const;
    bool canAddMoreMoves() const;
    bool hasEighthProngBonusHop() const;

    bool setCaptureAtSquare(int jumpedX, int jumpedY, bool capture);
    bool toggleCaptureAtSquare(int jumpedX, int jumpedY);
    bool toggleHopCapture(std::size_t hopIndex);

    bool canCommitStagedMove() const;
    bool isAwaitingConfirmation() const;
    bool isAwaitingSuperProngBonusMove() const;
    std::string stagedMoveNotation() const;
    const std::vector<OctiMoveOp> &stagedOps() const { return _stagedOps; }
    const std::vector<CaptureChoice> &captureChoices() const;
    bool populatePreviewBoard(OctiBoard &outBoard) const;

    std::vector<std::pair<int, int>> legalDestinationsFrom(int x, int y, int podIndex = -1) const;
    std::vector<OctiDir> legalProngAdditionsAt(int x, int y, int podIndex, bool superProng) const;
    std::vector<std::pair<int, int>> legalPodDropSquares(bool liberate) const;

    bool commitStagedMove();
    void cancelAndRollback();

private:
    struct HopCaptureLink {
        std::size_t opIndex = 0;
        std::size_t hopIndex = 0;
        int jumpedX = 0; // 1-based
        int jumpedY = 0; // 1-based
    };

    int resolvePodIndex(const OctiSquare &sq, int x, int y, int requestedIndex,
                        OctiPlayer turn, bool excludeClaimed) const;
    bool isPodClaimed(int bx, int by, int podIdx) const;
    bool validateOpsOnScratch(const std::vector<OctiMoveOp> &ops) const;
    bool hasContinuationJumpFrom(int bx, int by, const OctiPod &pod,
                                 int startBx, int startBy,
                                 const std::vector<OctiPod> &ghostPods,
                                 const std::vector<std::pair<int, int>> &alreadyJumped) const;
    bool hasOtherMovableFriendlyPods(int bx, int by, OctiPlayer turn) const;
    void syncPreparedStateToGame();

    OctiBoard *_board;
    bool _confirmAllMoves;
    bool _moveEntireMatchingStack;
    std::function<void(const std::string &moveNotation)> _onMoveCommitted;

    std::vector<OctiMove> _cachedLegalMoves;
    std::map<std::pair<int, int>, int> _selectedStackPods;

    bool _awaitingConfirmation;
    bool _awaitingSuperProngBonusMove;
    std::vector<OctiMoveOp> _stagedOps;
    std::vector<std::pair<int, int>> _claimedStartPods; // (bx * 10 + by, podIndex)
    std::vector<CaptureChoice> _captureChoices;
    std::vector<HopCaptureLink> _hopCaptureLinks;

    bool _activeJumpHasChain;
    int _activeJumpStartBx;
    int _activeJumpStartBy;
    int _activeJumpCurBx;
    int _activeJumpCurBy;
    std::vector<std::pair<int, int>> _activeJumpedCoords; // 0-based (jx, jy)

    // For 8th-prong superprong bonus hop staged after +dir!
    bool _hasEighthProngBonusHop;
    int _eighthProngTargetBx;
    int _eighthProngTargetBy;
    std::string _eighthProngBonusNotation;

    // Guided step / jump chain state (0-based internally)
    bool _guidedMoveActive;
    bool _guidedMidHop;
    int _guidedOriginBx;
    int _guidedOriginBy;
    int _guidedCurBx;
    int _guidedCurBy;
    int _guidedPodIndex;
    bool _guidedMoveAllStack;
};

} // namespace octi

#endif // OCTI_DESKTOP_OCTI_MOVE_COMPOSER_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
