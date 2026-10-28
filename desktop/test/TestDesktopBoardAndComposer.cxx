/*
 * TestDesktopBoardAndComposer.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QKeyEvent>
#include "OctiBoard.hxx"
#include "OctiBoardCanvas.hxx"
#include "OctiBoardGeometry.hxx"
#include "OctiBotEngine.hxx"
#include "OctiDefs.hxx"
#include "OctiDialogs.hxx"
#include "OctiGame.hxx"
#include "OctiMoveComposer.hxx"
#include "OctiReservePanel.hxx"
#include "OctiSoundSynth.hxx"
#include "OctiTheme.hxx"
#include "QtCompat.hxx"

#include <rapidcheck.h>
#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/TestHarness.h>

using namespace octi;

namespace {

bool isMoveInLegalList(const std::string &notation,
                       const std::vector<OctiMove> &legalMoves)
{
    for (const auto &m : legalMoves) {
        if (m.asString() == notation) {
            return true;
        }
    }
    return false;
}

double computePcmRms(const std::vector<std::int16_t> &samples)
{
    if (samples.empty()) {
        return 0.0;
    }
    double sumSq = 0.0;
    for (std::int16_t s : samples) {
        double v = static_cast<double>(s);
        sumSq += v * v;
    }
    return std::sqrt(sumSq / static_cast<double>(samples.size()));
}

bool imageHasDistinctColors(const QImage &img, int minDistinctColors = 12)
{
    if (img.isNull() || img.width() <= 0 || img.height() <= 0) {
        return false;
    }
    std::set<QRgb> uniqueColors;
    int stepX = std::max(1, img.width() / 48);
    int stepY = std::max(1, img.height() / 48);
    for (int y = 0; y < img.height(); y += stepY) {
        for (int x = 0; x < img.width(); x += stepX) {
            uniqueColors.insert(img.pixel(x, y));
            if (static_cast<int>(uniqueColors.size()) >= minDistinctColors) {
                return true;
            }
        }
    }
    return static_cast<int>(uniqueColors.size()) >= minDistinctColors;
}

} // namespace

// ============================================================================
// Gate 1 Group 1: OctiBoardGeometryTest
// ============================================================================

TEST_GROUP(OctiBoardGeometryTest)
{
};

TEST(OctiBoardGeometryTest, CanonicalServerEndpointConstants)
{
    STRCMP_EQUAL("trumpet.selfso.com", OCTI_SERVER_HOST);
    LONGS_EQUAL(6874, OCTI_SERVER_PORT);
}

TEST(OctiBoardGeometryTest, SquareCenterRoundTrip9x9And6x7AllRotations)
{
    const BoardRotation rotations[] = {
        BoardRotation::ROT_0,
        BoardRotation::ROT_90,
        BoardRotation::ROT_180,
        BoardRotation::ROT_270,
    };

    for (BoardRotation rot : rotations) {
        OctiBoardGeometry geom9(960, 960, 9, 9, rot);
        for (int x = 1; x <= 9; ++x) {
            for (int y = 1; y <= 9; ++y) {
                QPointF c = geom9.squareCenter(x, y);
                int rx = 0, ry = 0;
                CHECK_TRUE(geom9.screenToSquare(c, rx, ry));
                LONGS_EQUAL(x, rx);
                LONGS_EQUAL(y, ry);
            }
        }

        OctiBoardGeometry geom67(900, 820, 6, 7, rot);
        for (int x = 1; x <= 6; ++x) {
            for (int y = 1; y <= 7; ++y) {
                QPointF c = geom67.squareCenter(x, y);
                int rx = 0, ry = 0;
                CHECK_TRUE(geom67.screenToSquare(c, rx, ry));
                LONGS_EQUAL(x, rx);
                LONGS_EQUAL(y, ry);
            }
        }
    }
}

TEST(OctiBoardGeometryTest, OrientationAndCornerRelationships)
{
    // Under ROT_0 (Red/A at bottom): (1,1) is bottom-left, (9,9) is top-right
    OctiBoardGeometry g0(800, 800, 9, 9, BoardRotation::ROT_0);
    QPointF c11_0 = g0.squareCenter(1, 1);
    QPointF c99_0 = g0.squareCenter(9, 9);
    CHECK_TRUE(c11_0.x() < c99_0.x());
    CHECK_TRUE(c11_0.y() > c99_0.y());

    // Under ROT_180 (Green/B at bottom): (1,1) is top-right, (9,9) is bottom-left
    OctiBoardGeometry g180(800, 800, 9, 9, BoardRotation::ROT_180);
    QPointF c11_180 = g180.squareCenter(1, 1);
    QPointF c99_180 = g180.squareCenter(9, 9);
    CHECK_TRUE(c11_180.x() > c99_180.x());
    CHECK_TRUE(c11_180.y() < c99_180.y());

    // Seat auto-orientation
    LONGS_EQUAL(static_cast<int>(BoardRotation::ROT_0),
                static_cast<int>(OctiBoardGeometry::rotationForSeat(GAME_MODE_2, PLAYER_A)));
    LONGS_EQUAL(static_cast<int>(BoardRotation::ROT_180),
                static_cast<int>(OctiBoardGeometry::rotationForSeat(GAME_MODE_2, PLAYER_B)));
    LONGS_EQUAL(static_cast<int>(BoardRotation::ROT_0),
                static_cast<int>(OctiBoardGeometry::rotationForSeat(GAME_MODE_4, PLAYER_A)));
    LONGS_EQUAL(static_cast<int>(BoardRotation::ROT_90),
                static_cast<int>(OctiBoardGeometry::rotationForSeat(GAME_MODE_4, PLAYER_B)));
    LONGS_EQUAL(static_cast<int>(BoardRotation::ROT_180),
                static_cast<int>(OctiBoardGeometry::rotationForSeat(GAME_MODE_4, PLAYER_C)));
    LONGS_EQUAL(static_cast<int>(BoardRotation::ROT_270),
                static_cast<int>(OctiBoardGeometry::rotationForSeat(GAME_MODE_4, PLAYER_D)));
}

TEST(OctiBoardGeometryTest, EightSectorCompassHitTestAllRotations)
{
    const BoardRotation rotations[] = {
        BoardRotation::ROT_0,
        BoardRotation::ROT_90,
        BoardRotation::ROT_180,
        BoardRotation::ROT_270,
    };

    for (BoardRotation rot : rotations) {
        OctiBoardGeometry geom(800, 800, 9, 9, rot);
        QPointF center(400.0, 400.0);
        double innerR = 10.0;
        double outerR = 60.0;
        double testR = 35.0;

        for (int d = static_cast<int>(DIR_N); d <= static_cast<int>(DIR_NW); ++d) {
            OctiDir dir = static_cast<OctiDir>(d);
            QPointF uv = geom.dirUnitVectorScreen(dir);
            QPointF probe(center.x() + uv.x() * testR,
                          center.y() + uv.y() * testR);
            OctiDir hit = geom.hitTestCompassSector(center, innerR, outerR, probe);
            LONGS_EQUAL(static_cast<int>(dir), static_cast<int>(hit));
        }

        // Dead-zone inner radius and outside outer radius return DIR_UNDEF
        LONGS_EQUAL(static_cast<int>(DIR_UNDEF),
                    static_cast<int>(geom.hitTestCompassSector(
                        center, innerR, outerR, QPointF(402.0, 402.0))));
        LONGS_EQUAL(static_cast<int>(DIR_UNDEF),
                    static_cast<int>(geom.hitTestCompassSector(
                        center, innerR, outerR, QPointF(500.0, 500.0))));
    }
}

TEST(OctiBoardGeometryTest, StackFanSlotHitTestingAndBoundaryViewports)
{
    OctiBoardGeometry geom(900, 900, 9, 9, BoardRotation::ROT_0);
    for (int numPods = 1; numPods <= 7; ++numPods) {
        for (int slot = 0; slot < numPods; ++slot) {
            QRectF slotRect = geom.stackFanSlotRect(5, 5, numPods, slot);
            CHECK_TRUE(slotRect.width() > 0.0);
            CHECK_TRUE(slotRect.height() > 0.0);
            int hit = geom.hitTestStackFanSlot(5, 5, numPods, slotRect.center());
            LONGS_EQUAL(slot, hit);
        }
    }
    LONGS_EQUAL(-1, geom.hitTestStackFanSlot(5, 5, 3, QPointF(5.0, 5.0)));

    // Minimum viewport clamp and out-of-bounds screenToSquare
    OctiBoardGeometry tiny(50, 50, 9, 9, BoardRotation::ROT_0);
    CHECK_TRUE(tiny.cellSize() >= 4.0);
    int outX = 0, outY = 0;
    CHECK_FALSE(tiny.screenToSquare(QPointF(-100.0, -100.0), outX, outY));
    CHECK_FALSE(tiny.screenToSquare(QPointF(10000.0, 10000.0), outX, outY));
}

TEST(OctiBoardGeometryTest, RapidCheckBijectiveGeometryAndCompassProperties)
{
    bool ok = rc::check(
        "squareCenter -> screenToSquare and dirUnitVector -> hitTestCompassSector are bijective",
        []() {
            int vw = *rc::gen::inRange(320, 2560);
            int vh = *rc::gen::inRange(320, 2560);
            bool isOfk = *rc::gen::arbitrary<bool>();
            int gw = isOfk ? 6 : 9;
            int gh = isOfk ? 7 : 9;
            int rotIdx = *rc::gen::inRange(0, 4);
            const BoardRotation rots[] = {
                BoardRotation::ROT_0,
                BoardRotation::ROT_90,
                BoardRotation::ROT_180,
                BoardRotation::ROT_270,
            };
            OctiBoardGeometry geom(vw, vh, gw, gh, rots[rotIdx]);

            int x = *rc::gen::inRange(1, gw + 1);
            int y = *rc::gen::inRange(1, gh + 1);
            QPointF c = geom.squareCenter(x, y);
            int rx = 0, ry = 0;
            RC_ASSERT(geom.screenToSquare(c, rx, ry));
            RC_ASSERT(rx == x);
            RC_ASSERT(ry == y);

            int d = *rc::gen::inRange(0, 8);
            OctiDir dir = static_cast<OctiDir>(d);
            QPointF uv = geom.dirUnitVectorScreen(dir);
            QPointF probe(c.x() + uv.x() * 24.0, c.y() + uv.y() * 24.0);
            OctiDir hit = geom.hitTestCompassSector(c, 6.0, 48.0, probe);
            RC_ASSERT(hit == dir);
        });
    CHECK_TRUE(ok);
}

// ============================================================================
// Gate 1 Group 2: OctiMoveComposerTest
// ============================================================================

TEST_GROUP(OctiMoveComposerTest)
{
};

TEST(OctiMoveComposerTest, SmartAutoCommitProngAddPodDropStepAndConfirmAll)
{
    OctiBoard board(GAME_MODE_2);
    board.getGame()->start(PLAYER_A);
    OctiMoveComposer composer(&board);
    std::vector<std::string> committedMoves;
    composer.setMoveCommittedCallback([&](const std::string &m) {
        committedMoves.push_back(m);
    });

    // 1. Mandatory Move Confirmation (default confirmAllMoves == true):
    // Adding a normal prong on A's pod at (3,3) in DIR_N ('a') pauses for confirmation
    CHECK_TRUE(composer.confirmAllMoves());
    CHECK_TRUE(composer.stageOrCommitAddProng(3, 3, 0, DIR_N, false));
    CHECK_TRUE(composer.isAwaitingConfirmation());
    CHECK_TRUE(composer.commitStagedMove());
    LONGS_EQUAL(1, static_cast<long>(committedMoves.size()));
    STRCMP_EQUAL("33+a", committedMoves.back().c_str());
    LONGS_EQUAL(static_cast<int>(PLAYER_B), static_cast<int>(board.getGame()->getTurn()));

    // 2. B drops a reserve pod at (5,7) in Confirm-All mode -> pauses until commitStagedMove()
    CHECK_TRUE(composer.stageOrCommitAddPod(5, 7, false));
    CHECK_TRUE(composer.isAwaitingConfirmation());
    STRCMP_EQUAL("57", composer.stagedMoveNotation().c_str());
    LONGS_EQUAL(static_cast<int>(PLAYER_B), static_cast<int>(board.getGame()->getTurn()));
    CHECK_TRUE(composer.commitStagedMove());
    CHECK_FALSE(composer.isAwaitingConfirmation());
    LONGS_EQUAL(2, static_cast<long>(committedMoves.size()));
    STRCMP_EQUAL("57", committedMoves.back().c_str());
    LONGS_EQUAL(static_cast<int>(PLAYER_A), static_cast<int>(board.getGame()->getTurn()));

    // 3. Switch to Smart Auto-Commit explicitly (confirmAllMoves = false): A steps from (3,3) to (3,4) -> auto-commits immediately
    composer.setConfirmAllMoves(false);
    CHECK_FALSE(composer.confirmAllMoves());
    CHECK_TRUE(composer.stageOrCommitPodMoveOrJump(3, 3, 0, 3, 4, false));
    CHECK_FALSE(composer.isAwaitingConfirmation());
    LONGS_EQUAL(3, static_cast<long>(committedMoves.size()));
    STRCMP_EQUAL("33a-34", committedMoves.back().c_str());
    LONGS_EQUAL(static_cast<int>(PLAYER_B), static_cast<int>(board.getGame()->getTurn()));

    // 4. B adds prong on (3,7) in DIR_S ('e') -> auto-commits
    CHECK_TRUE(composer.stageOrCommitAddProng(3, 7, 0, DIR_S, false));
    STRCMP_EQUAL("37+e", committedMoves.back().c_str());

    // 5. A repositions prong on (3,4) from DIR_N ('a') to DIR_NE ('b') -> auto-commits
    CHECK_TRUE(composer.stageOrCommitRepositionProng(3, 4, 0, DIR_N, DIR_NE));
    STRCMP_EQUAL("34a+b-a", committedMoves.back().c_str());
}

TEST(OctiMoveComposerTest, MultiHopJumpSelectiveCaptureAndLiberation)
{
    OctiBoard board(GAME_MODE_2);
    board.getGame()->start(PLAYER_A);
    board.getGame()->move("33+a");
    board.getGame()->move("37+e");
    board.getGame()->move("53+a");
    board.getGame()->move("37e-36");
    board.getGame()->move("53a-54");
    board.getGame()->move("57+e");
    board.getGame()->move("54a-55");
    board.getGame()->move("57e-56");
    board.getGame()->move("55a-57x"); // A jumps over B at (5,6) capturing it and landing on B's empty base (5,7)

    // Give A 1 captured pod on A's turn while A occupies B's base (5,7) so A can liberate onto (5,7)
    OctiGameSnapshot snap = board.getGame()->captureSnapshot();
    snap.players[PLAYER_A].capturedPods = 1;
    snap.turn = PLAYER_A;
    board.getGame()->restoreSnapshot(snap);

    OctiMoveComposer composer(&board);
    std::string lastMove;
    composer.setMoveCommittedCallback([&](const std::string &m) { lastMove = m; });

    CHECK_TRUE(board.getGame()->getCapturedPods(PLAYER_A) >= 1);
    auto libSquares = composer.legalPodDropSquares(true);
    CHECK_FALSE(libSquares.empty());
    CHECK_TRUE(composer.stageOrCommitAddPod(5, 7, true));
    if (composer.isAwaitingConfirmation()) {
        CHECK_TRUE(composer.commitStagedMove());
    }
    STRCMP_EQUAL("57*", lastMove.c_str());

    // Now it's B's turn; let B step (3,6) -> (3,5) so B is at (3,5) and A is at (3,3) with 'a' prong
    board.getGame()->move("36e-35");
    // A steps (3,3) -> (3,4)
    board.getGame()->move("33a-34");
    // B adds prong on (7,7)
    board.getGame()->move("77+e");

    // Now it's A's turn: A is at (3,4) with 'a' prong, B is at (3,5), and (3,6) is empty!
    // A jumps from (3,4) over B at (3,5) to (3,6):
    CHECK_TRUE(composer.stageOrCommitPodMoveOrJump(3, 4, 0, 3, 6, false));
    // Must pause for confirmation because capture of (3,5) is optional in 2P!
    CHECK_TRUE(composer.isAwaitingConfirmation());
    STRCMP_EQUAL("34a-36x", composer.stagedMoveNotation().c_str());
    LONGS_EQUAL(1, static_cast<long>(composer.captureChoices().size()));
    CHECK_TRUE(composer.captureChoices()[0].capture);

    // Toggle capture off -> notation becomes "34a-36"
    CHECK_TRUE(composer.toggleCaptureAtSquare(3, 5));
    CHECK_FALSE(composer.captureChoices()[0].capture);
    STRCMP_EQUAL("34a-36", composer.stagedMoveNotation().c_str());

    // Toggle capture back on via hop index -> notation becomes "34a-36x"
    CHECK_TRUE(composer.toggleHopCapture(0));
    CHECK_TRUE(composer.captureChoices()[0].capture);
    STRCMP_EQUAL("34a-36x", composer.stagedMoveNotation().c_str());

    // Commit the staged move
    CHECK_TRUE(composer.commitStagedMove());
    STRCMP_EQUAL("34a-36x", lastMove.c_str());
}

TEST(OctiMoveComposerTest, StackFanSelectionShiftStackAndCompositeMove)
{
    OctiBoard board(GAME_MODE_2);
    board.getGame()->start(PLAYER_A);
    // Drop a second pod on (3,3) and give both pods a N prong so they can move together or separately
    board.getGame()->move("33");
    board.getGame()->move("37+e");
    board.getGame()->move("33+a");
    board.getGame()->move("57+e");
    board.getGame()->move("33+a");
    board.getGame()->move("77+e");

    // Square (3,3) now has 2 A pods, both with N ('a') prong.
    OctiMoveComposer composer(&board);
    std::string committed;
    composer.setMoveCommittedCallback([&](const std::string &m) { committed = m; });

    // 1. Shift-drag (moveAllMatchingStack = true) moves both pods together ("33a-34,33a-34")
    // Since _stagedOps.size() == 2, composer pauses for confirmation so user can inspect composite stack move
    CHECK_TRUE(composer.stageOrCommitPodMoveOrJump(3, 3, 0, 3, 4, true));
    if (composer.isAwaitingConfirmation()) {
        CHECK_TRUE(composer.commitStagedMove());
    }
    STRCMP_EQUAL("33a-34,33a-34", committed.c_str());

    // B moves
    board.getGame()->move("37e-36");

    // 2. Now on (3,4) we have 2 pods. Give the second pod an E ('c') prong so they have distinct prongs.
    board.getGame()->move("34a+c");
    board.getGame()->move("57e-56");

    // Select pod 1 via Stack Fan and move single pod from (3,4) to (3,5).
    // Since (3,4) has another movable friendly pod, Smart Auto-Commit pauses to allow composite move!
    composer.selectStackPodIndex(3, 4, 1);
    LONGS_EQUAL(1, composer.selectedStackPodIndex(3, 4));
    CHECK_TRUE(composer.stageOrCommitPodMoveOrJump(3, 4, 1, 3, 5, false));
    CHECK_TRUE(composer.isAwaitingConfirmation());

    // Now move the remaining pod on (3,4) to (3,5) as the second leg of the composite move
    CHECK_TRUE(composer.stageOrCommitPodMoveOrJump(3, 4, 0, 3, 5, false));
    CHECK_TRUE(composer.commitStagedMove());
    CHECK_FALSE(committed.empty());
}

TEST(OctiMoveComposerTest, SuperprongEighthProngBonusMoveAndEdgelessWrap)
{
    // Test edgeless wrap-around step from x=1 to x=9
    OctiBoard edgelessBoard(GAME_MODE_2, true, true, GAME_BASES_3);
    edgelessBoard.getGame()->start(PLAYER_A);
    edgelessBoard.getGame()->move("33+g"); // W prong on (3,3)
    edgelessBoard.getGame()->move("37+e");
    edgelessBoard.getGame()->move("33g-23");
    edgelessBoard.getGame()->move("57+e");
    edgelessBoard.getGame()->move("23g-13");
    edgelessBoard.getGame()->move("77+e");

    OctiMoveComposer edgelessComposer(&edgelessBoard);
    std::string edgelessCommitted;
    edgelessComposer.setMoveCommittedCallback([&](const std::string &m) {
        edgelessCommitted = m;
    });
    // Step West from (1,3) wraps around to (9,3)!
    CHECK_TRUE(edgelessComposer.stageOrCommitPodMoveOrJump(1, 3, 0, 9, 3, false));
    if (edgelessComposer.isAwaitingConfirmation()) {
        CHECK_TRUE(edgelessComposer.commitStagedMove());
    }
    STRCMP_EQUAL("13g-93", edgelessCommitted.c_str());

    // Test 8th prong added as superprong pauses for bonus move and then commits "33abcdefg+h!,33-34"
    OctiBoard spBoard(GAME_MODE_2, true, false, GAME_BASES_3);
    spBoard.getGame()->start(PLAYER_A);
    OctiMoveComposer spComposer(&spBoard);
    std::string spCommitted;
    spComposer.setMoveCommittedCallback([&](const std::string &m) {
        spCommitted = m;
    });

    const OctiDir dirs[] = { DIR_N, DIR_NE, DIR_E, DIR_SE, DIR_S, DIR_SW, DIR_W };
    const int bSquaresX[] = { 3, 5, 7, 3, 5, 7, 3 };
    const OctiDir bDirs[] = { DIR_S, DIR_S, DIR_S, DIR_SE, DIR_SE, DIR_SE, DIR_SW };
    for (int i = 0; i < 7; ++i) {
        CHECK_TRUE(spComposer.stageOrCommitAddProng(3, 3, 0, dirs[i], false));
        if (spComposer.isAwaitingConfirmation()) {
            CHECK_TRUE(spComposer.commitStagedMove());
        }
        CHECK_TRUE(spComposer.stageOrCommitAddProng(bSquaresX[i], 7, 0, bDirs[i], false));
        if (spComposer.isAwaitingConfirmation()) {
            CHECK_TRUE(spComposer.commitStagedMove());
        }
    }

    // Adding the 8th prong on (3,3) as a superprong in DIR_NW ('h') must pause for bonus move
    CHECK_TRUE(spComposer.stageOrCommitAddProng(3, 3, 0, DIR_NW, true));
    CHECK_TRUE(spComposer.isAwaitingConfirmation());
    CHECK_TRUE(spComposer.isAwaitingSuperProngBonusMove());
    STRCMP_EQUAL("33abcdefg+h!", spComposer.stagedMoveNotation().c_str());

    // Stage the bonus step from (3,3) to (3,4) and commit
    CHECK_TRUE(spComposer.stageOrCommitPodMoveOrJump(3, 3, 0, 3, 4, false));
    STRCMP_EQUAL("33abcdefg+h!,33-34", spComposer.stagedMoveNotation().c_str());
    CHECK_TRUE(spComposer.commitStagedMove());
    STRCMP_EQUAL("33abcdefg+h!,33-34", spCommitted.c_str());
}

TEST(OctiMoveComposerTest, FaultInjectionIllegalMovesAndRollback)
{
    OctiBoard board(GAME_MODE_2);
    board.getGame()->start(PLAYER_A);
    OctiMoveComposer composer(&board);

    // 1. Dropping a pod on a non-home square (4,4) fails
    CHECK_FALSE(composer.stageOrCommitAddPod(4, 4, false));
    CHECK_FALSE(composer.isAwaitingConfirmation());

    // 2. Dragging an enemy pod at (3,7) on A's turn fails
    CHECK_FALSE(composer.stageOrCommitPodMoveOrJump(3, 7, 0, 3, 6, false));

    // 3. Moving an unpronged pod at (3,3) fails
    CHECK_FALSE(composer.stageOrCommitPodMoveOrJump(3, 3, 0, 3, 4, false));

    // 4. Adding a backward prong in OFK fails
    OctiBoard ofkBoard(GAME_MODE_OFK);
    ofkBoard.getGame()->start(PLAYER_A);
    OctiMoveComposer ofkComposer(&ofkBoard);
    CHECK_FALSE(ofkComposer.stageOrCommitAddProng(2, 2, 0, DIR_S, false));
    CHECK_TRUE(ofkComposer.stageOrCommitAddProng(2, 2, 0, DIR_N, false));

    // 5. Mid-staging rollback leaves board untouched
    composer.setConfirmAllMoves(true);
    CHECK_TRUE(composer.stageOrCommitAddProng(3, 3, 0, DIR_N, false));
    CHECK_TRUE(composer.isAwaitingConfirmation());
    composer.cancelAndRollback();
    CHECK_FALSE(composer.isAwaitingConfirmation());
    LONGS_EQUAL(static_cast<int>(PLAYER_A), static_cast<int>(board.getGame()->getTurn()));
    LONGS_EQUAL(static_cast<int>(NO_PRONG),
                static_cast<int>(board.squareAt(2, 2).getPodAt(0).prongAt(DIR_N)));
}

TEST(OctiMoveComposerTest, GuidedStepAndMultiHopJumpWorkflow)
{
    OctiBoard board(GAME_MODE_2);
    board.getGame()->start(PLAYER_A);
    OctiMoveComposer composer(&board);
    std::string committed;
    composer.setMoveCommittedCallback([&](const std::string &m) { committed = m; });

    // 1. Pod at (3,3) has no prongs yet -> cannot begin guided move
    CHECK_FALSE(composer.beginGuidedPodMove(3, 3, 0));
    CHECK_FALSE(composer.isGuidedMoveActive());

    // 2. Add North prong 'a' at (3,3) and commit
    CHECK_TRUE(composer.stageOrCommitAddProng(3, 3, 0, DIR_N, false));
    CHECK_TRUE(composer.commitStagedMove());

    // B adds South prong 'e' at (3,7) and commits
    CHECK_TRUE(composer.stageOrCommitAddProng(3, 7, 0, DIR_S, false));
    CHECK_TRUE(composer.commitStagedMove());

    // 3. Now A's turn: begin guided move from (3,3)
    CHECK_TRUE(composer.beginGuidedPodMove(3, 3, 0));
    CHECK_TRUE(composer.isGuidedMoveActive());
    CHECK_EQUAL(3, composer.guidedOriginSquare().first);
    CHECK_EQUAL(3, composer.guidedOriginSquare().second);
    CHECK_EQUAL(3, composer.guidedCurrentSquare().first);
    CHECK_EQUAL(3, composer.guidedCurrentSquare().second);

    auto nextHops = composer.guidedNextSquares();
    CHECK_FALSE(nextHops.empty());
    bool found34 = false;
    for (const auto &sq : nextHops) {
        if (sq.first == 3 && sq.second == 4) {
            found34 = true;
            break;
        }
    }
    CHECK_TRUE(found34);

    // 4. Select destination (3,4) -> distance-1 step finishes guided selection and awaits confirmation
    CHECK_TRUE(composer.selectGuidedDestination(3, 4));
    CHECK_FALSE(composer.isGuidedMoveActive());
    CHECK_TRUE(composer.isAwaitingConfirmation());
    STRCMP_EQUAL("33a-34", composer.stagedMoveNotation().c_str());
    CHECK_TRUE(composer.commitStagedMove());
    STRCMP_EQUAL("33a-34", committed.c_str());

    // B moves pod from (3,7) to (3,6)
    CHECK_TRUE(composer.stageOrCommitPodMoveOrJump(3, 7, 0, 3, 6, false));
    CHECK_TRUE(composer.commitStagedMove());

    // A adds prong 'b' (NE) to pod at (3,4)
    CHECK_TRUE(composer.stageOrCommitAddProng(3, 4, 0, DIR_NE, false));
    CHECK_TRUE(composer.commitStagedMove());

    // B moves pod from (3,6) to (3,5)
    CHECK_TRUE(composer.stageOrCommitPodMoveOrJump(3, 6, 0, 3, 5, false));
    if (composer.isAwaitingConfirmation()) {
        CHECK_TRUE(composer.commitStagedMove());
    }

    // 5. A is at (3,4) with 'a' (N) prong, B is at (3,5), (3,6) is empty!
    // A starts guided move at (3,4)
    CHECK_TRUE(composer.beginGuidedPodMove(3, 4, 0));
    CHECK_TRUE(composer.isGuidedMoveActive());
    auto jumpHops = composer.guidedNextSquares();
    bool found36 = false;
    for (const auto &sq : jumpHops) {
        if (sq.first == 3 && sq.second == 6) {
            found36 = true;
            break;
        }
    }
    CHECK_TRUE(found36);

    // Select jump landing (3,6)
    CHECK_TRUE(composer.selectGuidedDestination(3, 6));
    CHECK_TRUE(composer.isAwaitingConfirmation());
    STRCMP_EQUAL("34ab-36x", composer.stagedMoveNotation().c_str());

    // Test setCaptureAtSquare
    CHECK_TRUE(composer.setCaptureAtSquare(3, 5, false));
    STRCMP_EQUAL("34ab-36", composer.stagedMoveNotation().c_str());
    CHECK_TRUE(composer.setCaptureAtSquare(3, 5, true));
    STRCMP_EQUAL("34ab-36x", composer.stagedMoveNotation().c_str());

    // Test finishGuidedMoveHere
    composer.finishGuidedMoveHere();
    CHECK_FALSE(composer.isGuidedMoveActive());
    CHECK_TRUE(composer.isAwaitingConfirmation());
    CHECK_TRUE(composer.commitStagedMove());
}

TEST(OctiMoveComposerTest, RapidCheckComposerLegalMovesInvariant)
{
    bool ok = rc::check(
        "Random sequences of OctiMoveComposer actions never corrupt OctiBoard and only commit legal moves",
        []() {
            OctiBoard board(GAME_MODE_2);
            board.getGame()->start(PLAYER_A);
            OctiMoveComposer composer(&board);
            bool confirmAll = *rc::gen::arbitrary<bool>();
            composer.setConfirmAllMoves(confirmAll);

            std::vector<OctiMove> preMoveLegal = board.getGame()->generateLegalMoves();
            composer.setMoveCommittedCallback([&](const std::string &notation) {
                RC_ASSERT(isMoveInLegalList(notation, preMoveLegal));
            });

            int steps = *rc::gen::inRange(4, 16);
            for (int i = 0; i < steps && !board.getGame()->isGameOver(); ++i) {
                preMoveLegal = board.getGame()->generateLegalMoves();
                int action = *rc::gen::inRange(0, 5);
                if (action == 0) {
                    int x = *rc::gen::inRange(1, 10);
                    int y = *rc::gen::inRange(1, 10);
                    int d = *rc::gen::inRange(0, 8);
                    composer.stageOrCommitAddProng(x, y, 0, static_cast<OctiDir>(d), false);
                } else if (action == 1) {
                    int x = *rc::gen::inRange(1, 10);
                    int y = *rc::gen::inRange(1, 10);
                    composer.stageOrCommitAddPod(x, y, false);
                } else if (action == 2) {
                    int fx = *rc::gen::inRange(1, 10);
                    int fy = *rc::gen::inRange(1, 10);
                    auto dests = composer.legalDestinationsFrom(fx, fy, 0);
                    if (!dests.empty()) {
                        int idx = *rc::gen::inRange(0, static_cast<int>(dests.size()));
                        composer.stageOrCommitPodMoveOrJump(
                            fx, fy, 0, dests[idx].first, dests[idx].second, false);
                    }
                } else if (action == 3) {
                    if (composer.isAwaitingConfirmation()) {
                        composer.commitStagedMove();
                    }
                } else {
                    composer.cancelAndRollback();
                }
            }
        });
    CHECK_TRUE(ok);
}

// ============================================================================
// Gate 1 Group 3: OctiBoardCanvasRenderTest
// ============================================================================

TEST_GROUP(OctiBoardCanvasRenderTest)
{
};

TEST(OctiBoardCanvasRenderTest, RenderBothThemesAllRotationsAllVariantsAndSaveArtifacts)
{
    QString repoRootStr = QString::fromUtf8(OCTI_REPO_ROOT_DIR);
    QString artifactDir = QFileInfo::exists(repoRootStr + "/plan/done/plan_octi_desktop.md")
                              ? (repoRootStr + "/plan/done/plan_octi_desktop.artifacts")
                              : (repoRootStr + "/plan/plan_octi_desktop.artifacts");
    QDir().mkpath(artifactDir);

    // 1. 2P Standard + Superprongs + Edgeless in Obsidian & Brass
    {
        OctiBoard board2p(GAME_MODE_2, true, true, GAME_BASES_3);
        board2p.getGame()->start(PLAYER_A);
        board2p.getGame()->move("33+a");
        board2p.getGame()->move("37+e");
        board2p.getGame()->move("33");
        board2p.getGame()->move("57+e!");
        OctiMoveComposer composer(&board2p);
        OctiSoundSynth synth;
        OctiBoardCanvas canvas;
        canvas.attachBoard(&board2p, &composer, &synth);
        canvas.setTheme(ThemeId::OBSIDIAN_BRASS);

        const BoardRotation rots[] = {
            BoardRotation::ROT_0,
            BoardRotation::ROT_90,
            BoardRotation::ROT_180,
            BoardRotation::ROT_270,
        };
        for (BoardRotation r : rots) {
            canvas.setBoardRotation(r);
            QImage img = canvas.renderToImage(960, 960);
            CHECK_FALSE(img.isNull());
            LONGS_EQUAL(960, img.width());
            LONGS_EQUAL(960, img.height());
            CHECK_TRUE(imageHasDistinctColors(img, 16));
        }

        // Open Stack Fan on (3,3) and stage a Confirm-All move so commit bar renders too
        canvas.setBoardRotation(BoardRotation::ROT_0);
        canvas.openStackFanAt(3, 3);
        composer.setConfirmAllMoves(true);
        composer.stageOrCommitPodMoveOrJump(3, 3, 0, 3, 4, false);
        QImage preview2p = canvas.renderToImage(1024, 1024);
        QString path2p = artifactDir + "/board_obsidian_brass_2p.png";
        CHECK_TRUE(preview2p.save(path2p));
        CHECK_TRUE(QFileInfo(path2p).size() > 10000);
        composer.cancelAndRollback();
        canvas.closeStackFan();

        // Also render and save OctiReservePanel preview
        OctiReservePanel reservePanel;
        reservePanel.attach(&board2p, &composer, &canvas, &synth);
        reservePanel.setTheme(ThemeId::OBSIDIAN_BRASS);
        QImage reserveImg = reservePanel.renderToImage(360, 760);
        CHECK_FALSE(reserveImg.isNull());
        CHECK_TRUE(imageHasDistinctColors(reserveImg, 10));
        QString reservePath = artifactDir + "/reserve_panel_preview.png";
        CHECK_TRUE(reserveImg.save(reservePath));
        CHECK_TRUE(QFileInfo(reservePath).size() > 5000);
    }

    // 2. 4P Partnership in Grandmaster Rosewood & Ivory
    {
        OctiBoard board4p(GAME_MODE_4, false, false, GAME_BASES_3);
        board4p.getGame()->start(PLAYER_A);
        board4p.getGame()->move("33+a");
        board4p.getGame()->move("37+e");
        board4p.getGame()->move("77+e");
        board4p.getGame()->move("73+a");
        OctiMoveComposer composer(&board4p);
        OctiBoardCanvas canvas;
        canvas.attachBoard(&board4p, &composer, nullptr);
        canvas.setTheme(ThemeId::ROSEWOOD_IVORY);
        canvas.autoOrientForSeat(PLAYER_B);
        LONGS_EQUAL(static_cast<int>(BoardRotation::ROT_90),
                    static_cast<int>(canvas.boardRotation()));

        QImage preview4p = canvas.renderToImage(1024, 1024);
        CHECK_TRUE(imageHasDistinctColors(preview4p, 16));
        QString path4p = artifactDir + "/board_rosewood_ivory_4p.png";
        CHECK_TRUE(preview4p.save(path4p));
        CHECK_TRUE(QFileInfo(path4p).size() > 10000);
    }

    // 3. OCTI for Kids (6x7) in Obsidian & Brass
    {
        OctiBoard boardOfk(GAME_MODE_OFK);
        boardOfk.getGame()->start(PLAYER_A);
        boardOfk.getGame()->move("22+a");
        boardOfk.getGame()->move("26+e");
        OctiMoveComposer composer(&boardOfk);
        OctiBoardCanvas canvas;
        canvas.attachBoard(&boardOfk, &composer, nullptr);
        canvas.setTheme(ThemeId::OBSIDIAN_BRASS);
        QImage previewOfk = canvas.renderToImage(960, 960);
        CHECK_TRUE(imageHasDistinctColors(previewOfk, 14));
        QString pathOfk = artifactDir + "/board_obsidian_brass_ofk.png";
        CHECK_TRUE(previewOfk.save(pathOfk));
        CHECK_TRUE(QFileInfo(pathOfk).size() > 10000);
    }
}

TEST(OctiBoardCanvasRenderTest, CanvasSimulateDragAndEscapeRollback)
{
    OctiBoard board(GAME_MODE_2);
    board.getGame()->start(PLAYER_A);
    OctiMoveComposer composer(&board);
    OctiSoundSynth synth;
    OctiBoardCanvas canvas;
    canvas.attachBoard(&board, &composer, &synth);
    composer.setConfirmAllMoves(false);

    CHECK_TRUE(canvas.simulateDragProngToPod(3, 3, 0, DIR_N, false));
    CHECK_TRUE(canvas.simulateDragFromReserveToSquare(DragKind::ADD_POD_FROM_RESERVE, 5, 7));
    CHECK_TRUE(canvas.simulateDragPodToSquare(3, 3, 0, 3, 4, false));
    CHECK_TRUE(canvas.simulateDragProngToPod(3, 7, 0, DIR_S, false));
    CHECK_TRUE(canvas.simulateRepositionProngOnPod(3, 4, 0, DIR_N, DIR_NE));

    // Confirm-All mode + Escape key rollback
    composer.setConfirmAllMoves(true);
    CHECK_TRUE(canvas.simulateDragProngToPod(3, 7, 0, DIR_SE, false));
    CHECK_TRUE(composer.isAwaitingConfirmation());
    QKeyEvent escEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &escEvent);
    CHECK_FALSE(composer.isAwaitingConfirmation());

    // Real QMouseEvent verification for direct compass click, Reserve well click, and click-to-move
    OctiBoard mouseBoard(GAME_MODE_2);
    mouseBoard.getGame()->start(PLAYER_A);
    OctiMoveComposer mouseComposer(&mouseBoard);
    mouseComposer.setConfirmAllMoves(false);
    OctiBoardCanvas mouseCanvas;
    mouseCanvas.resize(800, 800);
    mouseCanvas.attachBoard(&mouseBoard, &mouseComposer, &synth);
    OctiReservePanel mouseReserve;
    mouseReserve.resize(340, 760);
    mouseReserve.attach(&mouseBoard, &mouseComposer, &mouseCanvas, &synth);
    mouseReserve.renderToImage(340, 760);

    auto sendClick = [](QWidget *w, const QPointF &pt) {
        QMouseEvent press(QEvent::MouseButtonPress, pt, pt, pt,
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(w, &press);
        QMouseEvent release(QEvent::MouseButtonRelease, pt, pt, pt,
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(w, &release);
    };

    // 1. Direct click on pod (3,3) North ('a') compass socket adds a prong ("33+a")
    double cell = mouseCanvas.geometry().cellSize();
    QPointF pt33N = mouseCanvas.geometry().squareCenter(3, 3) +
                    mouseCanvas.geometry().dirUnitVectorScreen(DIR_N) * (cell * 0.35);
    sendClick(&mouseCanvas, pt33N);
    LONGS_EQUAL(1, mouseBoard.getGame()->getNumMoves());
    LONGS_EQUAL(static_cast<long>(NORMAL_PRONG),
                static_cast<long>(mouseBoard.squareAt(2, 2).getPodAt(0).prongAt(DIR_N)));

    // 2. Click "Prongs" well in OctiReservePanel then click pod (3,7) South ('e') socket ("37+e")
    mouseReserve.renderToImage(340, 760);
    sendClick(&mouseReserve, mouseReserve.normalProngWellRect().center());
    LONGS_EQUAL(static_cast<long>(DragKind::ADD_PRONG_FROM_RESERVE),
                static_cast<long>(mouseCanvas.activeDragKind()));
    QPointF pt37S = mouseCanvas.geometry().squareCenter(3, 7) +
                    mouseCanvas.geometry().dirUnitVectorScreen(DIR_S) * (cell * 0.35);
    sendClick(&mouseCanvas, pt37S);
    LONGS_EQUAL(2, mouseBoard.getGame()->getNumMoves());
    LONGS_EQUAL(static_cast<long>(NORMAL_PRONG),
                static_cast<long>(mouseBoard.squareAt(2, 6).getPodAt(0).prongAt(DIR_S)));

    // 3. Click pod (3,3) center to select, then click destination square (3,4) to step ("33a-34")
    sendClick(&mouseCanvas, mouseCanvas.geometry().squareCenter(3, 3));
    sendClick(&mouseCanvas, mouseCanvas.geometry().squareCenter(3, 4));
    LONGS_EQUAL(3, mouseBoard.getGame()->getNumMoves());
    CHECK_FALSE(mouseBoard.squareAt(2, 3).isEmpty());
}

// ============================================================================
// Gate 1 Group 4: OctiBotEngineTest
// ============================================================================

TEST_GROUP(OctiBotEngineTest)
{
};

TEST(OctiBotEngineTest, BotPlaysAcrossVariantsAndFindsWinningAndBlockingMoves)
{
    OctiBotEngine bot(BotDifficulty::CASUAL, 20261022ULL);

    // 1. Immediate 1-ply base capture win in 1-base mode (B vacates 57 so 56a-57 is open)
    {
        OctiBoard board(GAME_MODE_2, false, false, GAME_BASES_1);
        board.getGame()->start(PLAYER_A);
        board.getGame()->move("53+a");
        board.getGame()->move("57+f");
        board.getGame()->move("53a-54");
        board.getGame()->move("57f-46");
        board.getGame()->move("54a-55");
        board.getGame()->move("46f-35");
        board.getGame()->move("55a-56");
        board.getGame()->move("35f-24");
        bot.setDifficulty(BotDifficulty::CASUAL);
        bot.setTimeBudgetMs(200);
        std::string winMove = bot.chooseMove(board);
        STRCMP_EQUAL("56a-57", winMove.c_str());
    }

    // 2. Blocking opponent's 1-ply win threat by capturing the threatening pod
    {
        OctiBoard threatBoard(GAME_MODE_2, false, false, GAME_BASES_1);
        threatBoard.getGame()->start(PLAYER_A);
        threatBoard.clear();

        // Player B at 34 (0-based (2,3)) with 'e' prong threatens A's empty home base 33 (0-based (2,2)).
        // Player A at 35 (0-based (2,4)) with 'e' prong can jump 35e-33x capturing B's threatening pod.
        OctiPod podA(A_POD);
        podA.addProng(DIR_S, false);
        threatBoard.squareAt(2, 4).addPod(podA);

        OctiPod podB(B_POD);
        podB.addProng(DIR_S, false);
        threatBoard.squareAt(2, 3).addPod(podB);

        OctiPod podB2(B_POD);
        threatBoard.squareAt(6, 6).addPod(podB2);

        bot.setDifficulty(BotDifficulty::TACTICAL);
        bot.setTimeBudgetMs(300);
        std::string defenseMove = bot.chooseMove(threatBoard);
        STRCMP_EQUAL("35e-33x", defenseMove.c_str());
    }

    // 3. 4P Partnership (Team A+C vs Team B+D), OFK, Superprongs, Edgeless across all 3 tiers
    {
        OctiBoard board4p(GAME_MODE_4, false, false, GAME_BASES_3);
        board4p.getGame()->start(PLAYER_A);
        for (int ply = 0; ply < 4; ++ply) {
            BotDifficulty diff = (ply == 0) ? BotDifficulty::CASUAL
                : (ply == 1) ? BotDifficulty::TACTICAL : BotDifficulty::MASTER;
            bot.setDifficulty(diff);
            bot.setTimeBudgetMs(120);
            std::string mv = bot.chooseMove(board4p);
            CHECK_FALSE(mv.empty());
            CHECK_TRUE(isMoveInLegalList(mv, board4p.getGame()->generateLegalMoves()));
            board4p.getGame()->move(mv);
        }

        OctiBoard boardOfk(GAME_MODE_OFK);
        boardOfk.getGame()->start(PLAYER_A);
        bot.setDifficulty(BotDifficulty::TACTICAL);
        bot.setTimeBudgetMs(120);
        std::string ofkMv = bot.chooseMove(boardOfk);
        CHECK_FALSE(ofkMv.empty());
        CHECK_TRUE(isMoveInLegalList(ofkMv, boardOfk.getGame()->generateLegalMoves()));
    }
}

TEST(OctiBotEngineTest, RapidCheckBotAlwaysReturnsLegalMoveWithoutMutatingBoard)
{
    bool ok = rc::check(
        "OctiBotEngine::chooseMove returns a legal move and preserves caller board state",
        []() {
            OctiBoard board(GAME_MODE_2, false, false, GAME_BASES_3);
            board.getGame()->start(PLAYER_A);
            int plies = *rc::gen::inRange(0, 6);
            for (int i = 0; i < plies && !board.getGame()->isGameOver(); ++i) {
                auto legal = board.getGame()->generateLegalMoves();
                if (legal.empty()) {
                    break;
                }
                int idx = *rc::gen::inRange(0, static_cast<int>(legal.size()));
                board.getGame()->move(legal[idx]);
            }

            std::string posBefore = board.toPositionString();
            auto legalBefore = board.getGame()->generateLegalMoves();

            OctiBotEngine bot(BotDifficulty::CASUAL, 999ULL);
            bot.setTimeBudgetMs(50);
            std::string chosen = bot.chooseMove(board);
            RC_ASSERT(board.toPositionString() == posBefore);
            if (!board.getGame()->isGameOver() && !legalBefore.empty()) {
                RC_ASSERT(isMoveInLegalList(chosen, legalBefore));
            }
        });
    CHECK_TRUE(ok);
}

// ============================================================================
// Gate 1 Group 5: OctiSoundSynthTest
// ============================================================================

TEST_GROUP(OctiSoundSynthTest)
{
};

TEST(OctiSoundSynthTest, SynthesizeAllSevenCuesAndVerifyMuteGate)
{
    OctiSoundSynth synth;
    const SoundCue cues[] = {
        SoundCue::POD_PLACE,
        SoundCue::PRONG_SNAP,
        SoundCue::POD_STEP,
        SoundCue::POD_JUMP,
        SoundCue::POD_CAPTURE,
        SoundCue::CLOCK_LOW_TICK,
        SoundCue::GAME_VICTORY,
    };

    for (SoundCue cue : cues) {
        std::vector<std::int16_t> pcm = synth.synthesizeCuePcm(cue);
        CHECK_TRUE(pcm.size() >= 300);
        double rms = computePcmRms(pcm);
        CHECK_TRUE(rms > 150.0);
    }

    int callbackCount = 0;
    synth.setPlaybackCallback([&](const std::vector<std::int16_t> &pcm) {
        if (!pcm.empty()) {
            ++callbackCount;
        }
    });

    CHECK_FALSE(synth.isMuted());
    synth.triggerCue(SoundCue::PRONG_SNAP);
    LONGS_EQUAL(1, callbackCount);
    LONGS_EQUAL(1, static_cast<long>(synth.triggeredCueCount()));
    LONGS_EQUAL(static_cast<int>(SoundCue::PRONG_SNAP),
                static_cast<int>(synth.lastTriggeredCue()));

    // Muting blocks playback callback and counter increments
    synth.setMuted(true);
    CHECK_TRUE(synth.isMuted());
    CHECK_TRUE(synth.isSfxMuted());
    synth.triggerCue(SoundCue::GAME_VICTORY);
    LONGS_EQUAL(1, callbackCount);
    LONGS_EQUAL(1, static_cast<long>(synth.triggeredCueCount()));

    // UiDispatcher executes posted lambdas on the GUI thread
    UiDispatcher dispatcher;
    bool dispatched = false;
    dispatcher.post([&]() { dispatched = true; });
    QCoreApplication::processEvents();
    CHECK_TRUE(dispatched);
}

TEST_GROUP(OctiDialogsRefinementTest)
{
    void setup() override
    {
    }

    void teardown() override
    {
    }
};

TEST(OctiDialogsRefinementTest, SplashDialogTenSecondDurationAndEarlyDismissal)
{
    OctiSplashDialog splash(nullptr, ThemeId::OBSIDIAN_BRASS);
    CHECK_TRUE(splash.windowFlags() & Qt::FramelessWindowHint);

    QImage f0 = OctiSplashDialog::renderSplashFrame(960, 560, 0.0);
    CHECK_FALSE(f0.isNull());
    LONGS_EQUAL(960, f0.width());
    LONGS_EQUAL(560, f0.height());

    QImage fMid = OctiSplashDialog::renderSplashFrame(960, 560, 0.5);
    CHECK_FALSE(fMid.isNull());

    QImage fEnd = OctiSplashDialog::renderSplashFrame(960, 560, 1.0);
    CHECK_FALSE(fEnd.isNull());

    QString repoRootStr = QString::fromUtf8(OCTI_REPO_ROOT_DIR);
    QString planArtifactDir = repoRootStr + "/plan/liberation_rules_and_commit_dialog.artifacts";
    QDir().mkpath(planArtifactDir);
    QString splashArtifactPath = planArtifactDir + "/desktop_splash_sha256.png";
    CHECK_TRUE(fMid.save(splashArtifactPath));
    CHECK_TRUE(QFileInfo(splashArtifactPath).size() > 10000);

    // ESC dismisses splash early
    QKeyEvent escEv(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QCoreApplication::sendEvent(&splash, &escEv);
}

TEST(OctiDialogsRefinementTest, ZoomedSquareDialogSocketOrientationAtRot0)
{
    QString repoRootStr = QString::fromUtf8(OCTI_REPO_ROOT_DIR);
    QString artifactDir = QFileInfo::exists(repoRootStr + "/plan/done/plan_gui_refinement_v2.md")
                              ? (repoRootStr + "/plan/done/plan_gui_refinement_v2.artifacts")
                              : (repoRootStr + "/plan/plan_gui_refinement_v2.artifacts");
    QDir().mkpath(artifactDir);

    OctiBoard board(GAME_MODE_2, true, true, GAME_BASES_3);
    board.getGame()->start(PLAYER_A);
    OctiMoveComposer composer(&board);

    ZoomedSquareDialog dlg(&board, &composer, 3, 3, BoardRotation::ROT_0, ThemeId::OBSIDIAN_BRASS);
    CHECK_TRUE(dlg.windowFlags() & Qt::FramelessWindowHint);
    CHECK_TRUE(dlg.windowFlags() & Qt::Popup);

    QImage img = dlg.renderToImage(560, 530);
    CHECK_FALSE(img.isNull());
    LONGS_EQUAL(560, img.width());
    LONGS_EQUAL(530, img.height());

    QString podArtifact = artifactDir + "/desktop_zoomed_green_pod_v2.png";
    CHECK_TRUE(img.save(podArtifact));
    CHECK_TRUE(QFileInfo(podArtifact).size() > 10000);

    // Center of pod canvas in ZoomedSquareDialog:
    // _podCanvasRect is QRectF(120, 76, 320, 320) when numPods == 1
    // pc = center = (280, 236)
    QPointF pc(280.0, 236.0);
    const auto &sockets = dlg.socketCenters();
    LONGS_EQUAL(8, static_cast<long>(sockets.size()));

    // At ROT_0:
    // DIR_N ('a'): 12 o'clock TOP (sc.y < pc.y, sc.x == pc.x)
    CHECK_TRUE(sockets.at(DIR_N).y() < pc.y() - 50.0);
    DOUBLES_EQUAL(pc.x(), sockets.at(DIR_N).x(), 1.0);

    // DIR_S ('e'): 6 o'clock BOTTOM (sc.y > pc.y, sc.x == pc.x)
    CHECK_TRUE(sockets.at(DIR_S).y() > pc.y() + 50.0);
    DOUBLES_EQUAL(pc.x(), sockets.at(DIR_S).x(), 1.0);

    // DIR_E ('c'): 3 o'clock RIGHT (sc.x > pc.x, sc.y == pc.y)
    CHECK_TRUE(sockets.at(DIR_E).x() > pc.x() + 50.0);
    DOUBLES_EQUAL(pc.y(), sockets.at(DIR_E).y(), 1.0);

    // DIR_W ('g'): 9 o'clock LEFT (sc.x < pc.x, sc.y == pc.y)
    CHECK_TRUE(sockets.at(DIR_W).x() < pc.x() - 50.0);
    DOUBLES_EQUAL(pc.y(), sockets.at(DIR_W).y(), 1.0);

    // Diagonals
    CHECK_TRUE(sockets.at(DIR_NE).x() > pc.x() + 30.0);
    CHECK_TRUE(sockets.at(DIR_NE).y() < pc.y() - 30.0); // 1:30

    CHECK_TRUE(sockets.at(DIR_SE).x() > pc.x() + 30.0);
    CHECK_TRUE(sockets.at(DIR_SE).y() > pc.y() + 30.0); // 4:30

    CHECK_TRUE(sockets.at(DIR_SW).x() < pc.x() - 30.0);
    CHECK_TRUE(sockets.at(DIR_SW).y() > pc.y() + 30.0); // 7:30

    CHECK_TRUE(sockets.at(DIR_NW).x() < pc.x() - 30.0);
    CHECK_TRUE(sockets.at(DIR_NW).y() < pc.y() - 30.0); // 10:30
}

TEST(OctiDialogsRefinementTest, ZoomedSquareDialogStackUnderlayAndMouseWheelCycling)
{
    QString repoRootStr = QString::fromUtf8(OCTI_REPO_ROOT_DIR);
    QString artifactDir = QFileInfo::exists(repoRootStr + "/plan/done/plan_gui_refinement_v2.md")
                              ? (repoRootStr + "/plan/done/plan_gui_refinement_v2.artifacts")
                              : (repoRootStr + "/plan/plan_gui_refinement_v2.artifacts");
    QDir().mkpath(artifactDir);

    OctiBoard board(GAME_MODE_2, true, true, GAME_BASES_3);
    board.getGame()->start(PLAYER_A);

    // Add a second pod to (3,3) to create a multi-pod stack
    OctiPod podB(B_POD);
    podB.addProng(DIR_N);
    board.squareAt(2, 2).addPod(podB);
    LONGS_EQUAL(2, board.squareAt(2, 2).getNumPods());

    OctiMoveComposer composer(&board);
    ZoomedSquareDialog dlg(&board, &composer, 3, 3, BoardRotation::ROT_0, ThemeId::OBSIDIAN_BRASS);

    LONGS_EQUAL(1, dlg.selectedPodIndex());

    // Cycle stack forwards and backwards
    dlg.cycleSelectedPod(1);
    LONGS_EQUAL(0, dlg.selectedPodIndex());

    dlg.cycleSelectedPod(-1);
    LONGS_EQUAL(1, dlg.selectedPodIndex());

    QImage stackImg = dlg.renderToImage(560, 530);
    CHECK_FALSE(stackImg.isNull());
    QString stackArtifact = artifactDir + "/desktop_zoomed_stack_underlay_v2.png";
    CHECK_TRUE(stackImg.save(stackArtifact));
    CHECK_TRUE(QFileInfo(stackArtifact).size() > 10000);
}

TEST(OctiDialogsRefinementTest, ZoomedSquareDialogPodPlacementAndLiberationButtonsOnOccupiedAndEmptyHome)
{
    QString repoRootStr = QString::fromUtf8(OCTI_REPO_ROOT_DIR);
    QString artifactDir = repoRootStr + "/plan/pod_placement_and_liberation.artifacts";
    QDir().mkpath(artifactDir);

    OctiBoard board(GAME_MODE_2, true, true, GAME_BASES_3);
    board.getGame()->start(PLAYER_A);
    board.squareAt(2, 6).addPod(OctiPod(A_POD));
    OctiGameSnapshot snap = board.getGame()->captureSnapshot();
    snap.players[PLAYER_A].capturedPods = 1;
    snap.turn = PLAYER_A;
    board.getGame()->restoreSnapshot(snap);
    OctiMoveComposer composer(&board);

    // 1. Occupied home square at (3, 3) (0-based (2, 2))
    ZoomedSquareDialog dlgOccupied(&board, &composer, 3, 3, BoardRotation::ROT_0, ThemeId::OBSIDIAN_BRASS);
    QImage imgOccupied = dlgOccupied.renderToImage(560, 530);
    CHECK_FALSE(imgOccupied.isNull());
    LONGS_EQUAL(560, imgOccupied.width());
    LONGS_EQUAL(530, imgOccupied.height());

    // Both Move Pod and Pod Placement buttons visible and non-overlapping
    QRectF moveRect = dlgOccupied.movePodBtnRect();
    QRectF placeRect = dlgOccupied.placeReserveBtnRect();
    QRectF libRect = dlgOccupied.liberateBtnRect();
    CHECK_FALSE(moveRect.isEmpty());
    CHECK_FALSE(placeRect.isEmpty());
    CHECK_FALSE(libRect.isEmpty());
    CHECK_FALSE(moveRect.intersects(placeRect));
    CHECK_FALSE(placeRect.intersects(libRect));

    QString occArtifact = artifactDir + "/desktop_zoomed_occupied_home_v4.png";
    CHECK_TRUE(imgOccupied.save(occArtifact));
    CHECK_TRUE(QFileInfo(occArtifact).size() > 5000);

    // 2. Empty home square at (5, 3) (0-based (4, 2))
    board.squareAt(4, 2).clear();
    CHECK_TRUE(board.squareAt(4, 2).isEmpty());

    ZoomedSquareDialog dlgEmpty(&board, &composer, 5, 3, BoardRotation::ROT_0, ThemeId::OBSIDIAN_BRASS);
    QImage imgEmpty = dlgEmpty.renderToImage(560, 530);
    CHECK_FALSE(imgEmpty.isNull());

    CHECK_TRUE(dlgEmpty.movePodBtnRect().isEmpty());
    CHECK_FALSE(dlgEmpty.placeReserveBtnRect().isEmpty());
    CHECK_TRUE(dlgEmpty.liberateBtnRect().isEmpty());

    QString emptyArtifact = artifactDir + "/desktop_zoomed_empty_home_v4.png";
    CHECK_TRUE(imgEmpty.save(emptyArtifact));
    CHECK_TRUE(QFileInfo(emptyArtifact).size() > 5000);
}

TEST(OctiDialogsRefinementTest, SingleClickMoveVsDoubleClickZoomDirectInteraction)
{
    OctiBoard board(GAME_MODE_2);
    board.getGame()->start(PLAYER_A);
    // Pod at 0-based (2, 2) corresponds to 1-based (3, 3); give it a North prong so it has legal moves
    board.squareAt(2, 2).podAt(0).addProng(DIR_N, false);
    OctiMoveComposer composer(&board);
    OctiSoundSynth synth;

    OctiBoardCanvas canvas;
    canvas.resize(800, 800);
    canvas.attachBoard(&board, &composer, &synth);
    canvas.setVisible(true);

    auto sendSingleClick = [](QWidget *w, const QPointF &pt) {
        QMouseEvent press(QEvent::MouseButtonPress, pt, pt, pt,
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(w, &press);
        QMouseEvent release(QEvent::MouseButtonRelease, pt, pt, pt,
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(w, &release);
    };

    // 1. Single click on friendly occupied square (3, 3) starts guided move directly
    QPointF pt33 = canvas.geometry().squareCenter(3, 3);
    sendSingleClick(&canvas, pt33);
    CHECK_TRUE(composer.isGuidedMoveActive());
    CHECK_FALSE(composer.isAwaitingConfirmation());

    // 2. Click outside (e.g. square (1, 1)) rolls back guided move
    QPointF pt11 = canvas.geometry().squareCenter(1, 1);
    sendSingleClick(&canvas, pt11);
    CHECK_FALSE(composer.isGuidedMoveActive());

    // 3. AZG-D02: 1-click friendly pod switching during 0-hop guided move
    board.squareAt(4, 2).podAt(0).addProng(DIR_N, false); // Friendly pod at 1-based (5, 3)
    composer.refreshLegalMoves();
    sendSingleClick(&canvas, pt33);
    CHECK_TRUE(composer.isGuidedMoveActive());
    LONGS_EQUAL(3, composer.guidedOriginSquare().first);
    LONGS_EQUAL(3, composer.guidedOriginSquare().second);

    QPointF pt53 = canvas.geometry().squareCenter(5, 3);
    sendSingleClick(&canvas, pt53);
    CHECK_TRUE(composer.isGuidedMoveActive());
    LONGS_EQUAL(5, composer.guidedOriginSquare().first);
    LONGS_EQUAL(3, composer.guidedOriginSquare().second);

    sendSingleClick(&canvas, pt11);
    CHECK_FALSE(composer.isGuidedMoveActive());

    // 4. AZG-D01: Board zoom via setBoardZoom / wheelEvent and snap-out via ESC, Right-Click, Double-Click
    DOUBLES_EQUAL(1.0, canvas.boardZoomScale(), 1e-6);
    QWheelEvent wheelIn(pt33, pt33, QPoint(0, 0), QPoint(0, 120),
                        Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(&canvas, &wheelIn);
    CHECK_TRUE(canvas.boardZoomScale() > 1.1);

    // ESC snaps out of board zoom to 1.0
    QKeyEvent escEv(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &escEv);
    DOUBLES_EQUAL(1.0, canvas.boardZoomScale(), 1e-6);

    // Right-click while zoomed in snaps out of board zoom without opening dialog
    canvas.setBoardZoom(2.0, pt33);
    DOUBLES_EQUAL(2.0, canvas.boardZoomScale(), 1e-6);
    QMouseEvent rightPress(QEvent::MouseButtonPress, pt33, pt33, pt33,
                           Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &rightPress);
    DOUBLES_EQUAL(1.0, canvas.boardZoomScale(), 1e-6);

    // Double-click while zoomed in snaps out of board zoom without opening dialog
    canvas.setBoardZoom(1.8, pt33);
    DOUBLES_EQUAL(1.8, canvas.boardZoomScale(), 1e-6);
    QMouseEvent dblClick(QEvent::MouseButtonDblClick, pt33, pt33, pt33,
                         Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &dblClick);
    DOUBLES_EQUAL(1.0, canvas.boardZoomScale(), 1e-6);

    // 5. AZG-D02: Click-and-hold (~380ms) hand cursor 90-degree quadrant rotation
    LONGS_EQUAL(static_cast<int>(BoardRotation::ROT_0),
                static_cast<int>(canvas.boardRotation()));
    QPointF center = canvas.geometry().boardOuterRect().center();
    QPointF topPt(center.x(), center.y() - 150.0);   // -90 deg
    QPointF rightPt(center.x() + 150.0, center.y()); // 0 deg (+90 deg CW from topPt)

    QMouseEvent holdPress(QEvent::MouseButtonPress, topPt, topPt, topPt,
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &holdPress);
    for (int i = 0; i < 45; ++i) {
        QCoreApplication::processEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    QMouseEvent dragCw(QEvent::MouseMove, rightPt, rightPt, rightPt,
                       Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &dragCw);
    DOUBLES_EQUAL(90.0, canvas.liveRotationOffsetDeg(), 2.0);

    QMouseEvent dragCcw(QEvent::MouseMove, topPt, topPt, topPt,
                        Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &dragCcw);
    DOUBLES_EQUAL(0.0, canvas.liveRotationOffsetDeg(), 2.0);

    QMouseEvent holdRelease(QEvent::MouseButtonRelease, topPt, topPt, topPt,
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &holdRelease);
    LONGS_EQUAL(static_cast<int>(BoardRotation::ROT_0),
                static_cast<int>(canvas.boardRotation()));
    DOUBLES_EQUAL(0.0, canvas.liveRotationOffsetDeg(), 0.001);
}

TEST(OctiDialogsRefinementTest, PlyConfirmDialogCommitCancelAndMoreMoves)
{
    QString repoRootStr = QString::fromUtf8(OCTI_REPO_ROOT_DIR);
    QString artifactDir = QFileInfo::exists(repoRootStr + "/plan/done/plan_gui_refinement_v2.md")
                              ? (repoRootStr + "/plan/done/plan_gui_refinement_v2.artifacts")
                              : (repoRootStr + "/plan/plan_gui_refinement_v2.artifacts");
    QDir().mkpath(artifactDir);

    OctiBoard board(GAME_MODE_2, true, true, GAME_BASES_3);
    board.getGame()->start(PLAYER_A);
    OctiMoveComposer composer(&board);

    // 1. Stage adding normal prong at 33+a
    CHECK_TRUE(composer.stageOrCommitAddProng(3, 3, 0, DIR_N, false));
    CHECK_TRUE(composer.isAwaitingConfirmation());
    STRCMP_EQUAL("33+a", composer.stagedMoveNotation().c_str());

    // 2. Verify populatePreviewBoard shows prong on preview while real board is clean
    OctiBoard previewBoard;
    CHECK_TRUE(composer.populatePreviewBoard(previewBoard));
    CHECK_EQUAL(NORMAL_PRONG, previewBoard.squareAt(2, 2).getPodAt(0).prongAt(DIR_N));
    CHECK_EQUAL(NO_PRONG, board.squareAt(2, 2).getPodAt(0).prongAt(DIR_N));

    // 3. Borderless PlyConfirmDialog
    PlyConfirmDialog dlg(&composer, ThemeId::OBSIDIAN_BRASS);
    CHECK_TRUE(dlg.windowFlags() & Qt::FramelessWindowHint);
    CHECK_TRUE(dlg.windowFlags() & Qt::Popup);
    CHECK_TRUE(dlg.stagedMoveText().contains("33+a"));
    CHECK_FALSE(dlg.isMoreMovesVisible());

    QImage dlgImg = dlg.renderToImage(460, 150);
    CHECK_FALSE(dlgImg.isNull());
    QString confirmArtifact = artifactDir + "/desktop_ply_confirm_dialog_v2.png";
    CHECK_TRUE(dlgImg.save(confirmArtifact));
    CHECK_TRUE(QFileInfo(confirmArtifact).size() > 5000);

    // 4. Cancel and rollback restores board
    composer.cancelAndRollback();
    CHECK_FALSE(composer.isAwaitingConfirmation());
    CHECK_EQUAL(NO_PRONG, board.squareAt(2, 2).getPodAt(0).prongAt(DIR_N));

    // 5. Verify More Moves visibility when origin has other movable pods
    board.squareAt(2, 2).clear();
    OctiPod pod1(A_POD);
    pod1.addProng(DIR_N);
    OctiPod pod2(A_POD);
    pod2.addProng(DIR_N);
    board.squareAt(2, 2).addPod(pod1);
    board.squareAt(2, 2).addPod(pod2);
    LONGS_EQUAL(2, board.squareAt(2, 2).getNumPods());

    CHECK_TRUE(composer.stageOrCommitPodMoveOrJump(3, 3, 1, 3, 4, false));
    CHECK_TRUE(composer.isAwaitingConfirmation());
    CHECK_TRUE(composer.hasOtherMovablePodAtOrigin());

    PlyConfirmDialog dlgMore(&composer, ThemeId::OBSIDIAN_BRASS);
    CHECK_TRUE(dlgMore.isMoreMovesVisible());
    composer.cancelAndRollback();
}

TEST(OctiDialogsRefinementTest, PlyConfirmMultiMoveAndGameOverEndgameV3)
{
    QString repoRootStr = QString::fromUtf8(OCTI_REPO_ROOT_DIR);
    QString artifactDir = QFileInfo::exists(repoRootStr + "/plan/done/plan_ply_confirm_and_endgame.md")
                              ? (repoRootStr + "/plan/done/ply_confirm_and_endgame.artifacts")
                              : (repoRootStr + "/plan/ply_confirm_and_endgame.artifacts");
    QDir().mkpath(artifactDir);

    // 1. PCE-D01: ZoomedSquareDialog on empty home square shows Place Reserve & Liberate, NOT Move/Jump
    OctiBoard board(GAME_MODE_2, true, true, GAME_BASES_3);
    board.getGame()->start(PLAYER_A);
    board.squareAt(2, 2).clear(); // Empty Player A home square at (3,3)
    board.squareAt(2, 6).addPod(OctiPod(A_POD));
    OctiGameSnapshot snap = board.getGame()->captureSnapshot();
    snap.players[PLAYER_A].capturedPods = 1;
    snap.turn = PLAYER_A;
    board.getGame()->restoreSnapshot(snap);
    OctiMoveComposer composer(&board);

    ZoomedSquareDialog emptyHomeDlg(&board, &composer, 3, 3,
                                    BoardRotation::ROT_0, ThemeId::OBSIDIAN_BRASS);
    QImage emptyHomeImg = emptyHomeDlg.renderToImage(560, 530);
    CHECK_FALSE(emptyHomeImg.isNull());
    CHECK_TRUE(emptyHomeDlg.movePodBtnRect().isEmpty());
    CHECK_FALSE(emptyHomeDlg.placeReserveBtnRect().isEmpty());
    CHECK_TRUE(emptyHomeDlg.liberateBtnRect().isEmpty());

    QString emptyHomePath = artifactDir + "/desktop_zoomed_empty_home_v3.png";
    CHECK_TRUE(emptyHomeImg.save(emptyHomePath));
    CHECK_TRUE(QFileInfo(emptyHomePath).size() > 10000);

    // 2. PCE-D02 & PCE-D03: Multi-pod stack More Moves preserves guidedOriginSquare & auto-selects unclaimed pod
    board.squareAt(2, 2).clear();
    OctiPod p0(A_POD);
    p0.addProng(DIR_N);
    OctiPod p1(A_POD);
    p1.addProng(DIR_E);
    board.squareAt(2, 2).addPod(p0);
    board.squareAt(2, 2).addPod(p1);
    composer.refreshLegalMoves();

    CHECK_TRUE(composer.beginGuidedPodMove(3, 3, 1, false));
    CHECK_TRUE(composer.selectGuidedDestination(4, 3));
    CHECK_FALSE(composer.isGuidedMoveActive());
    CHECK_TRUE(composer.isAwaitingConfirmation());
    LONGS_EQUAL(3, composer.guidedOriginSquare().first);
    LONGS_EQUAL(3, composer.guidedOriginSquare().second);
    CHECK_TRUE(composer.isSquarePodClaimed(3, 3, 1));
    CHECK_FALSE(composer.isSquarePodClaimed(3, 3, 0));
    LONGS_EQUAL(0, composer.firstUnclaimedFriendlyPodIndex(3, 3));

    // Reopening ZoomedSquareDialog on (3,3) auto-selects pod 0 (unclaimed)
    ZoomedSquareDialog stackDlg(&board, &composer, 3, 3,
                                BoardRotation::ROT_0, ThemeId::OBSIDIAN_BRASS);
    LONGS_EQUAL(0, stackDlg.selectedPodIndex());
    stackDlg.renderToImage(560, 530);
    CHECK_FALSE(stackDlg.movePodBtnRect().isEmpty());
    composer.cancelAndRollback();

    // 3. PCE-D04: 8th-prong superprong bonus hop interactive completion
    board.squareAt(2, 2).clear();
    OctiPod almostFull(A_POD);
    for (int d = DIR_N; d <= DIR_W; ++d) {
        almostFull.addProng(static_cast<OctiDir>(d));
    }
    board.squareAt(2, 2).addPod(almostFull);
    composer.refreshLegalMoves();

    CHECK_TRUE(composer.stageOrCommitAddProng(3, 3, 0, DIR_NW, true));
    CHECK_TRUE(composer.isAwaitingSuperProngBonusMove());
    CHECK_FALSE(composer.hasEighthProngBonusHop());
    CHECK_TRUE(composer.canAddMoreMoves());
    CHECK_TRUE(composer.beginGuidedPodMove(3, 3, 0, false));
    CHECK_FALSE(composer.guidedNextSquares().empty());
    CHECK_TRUE(composer.selectGuidedDestination(3, 4));
    CHECK_TRUE(composer.hasEighthProngBonusHop());
    CHECK_TRUE(composer.stagedMoveNotation().find("+h!,33-34") != std::string::npos);
    CHECK_TRUE(composer.commitStagedMove());

    // 4. PCE-D05: Multi-hop jump allows early commit via PlyConfirmDialog after first hop
    OctiBoard jumpBoard(GAME_MODE_2, false, false, GAME_BASES_3);
    jumpBoard.getGame()->start(PLAYER_A);
    jumpBoard.squareAt(2, 2).clear();
    jumpBoard.squareAt(2, 6).clear(); // Clear B's initial pod at (3,7) so landing square is empty
    OctiPod jumper(A_POD);
    jumper.addProng(DIR_N);
    jumpBoard.squareAt(2, 2).addPod(jumper);
    jumpBoard.squareAt(2, 3).addPod(OctiPod(B_POD)); // Enemy at (3,4)
    jumpBoard.squareAt(2, 5).addPod(OctiPod(B_POD)); // Enemy at (3,6)
    OctiMoveComposer jumpComposer(&jumpBoard);

    CHECK_TRUE(jumpComposer.beginGuidedPodMove(3, 3, 0, false));
    CHECK_TRUE(jumpComposer.selectGuidedDestination(3, 5)); // First jump to (3,5)
    CHECK_TRUE(jumpComposer.isGuidedMoveActive());          // Second jump to (3,7) available
    CHECK_TRUE(jumpComposer.isAwaitingConfirmation());
    CHECK_TRUE(jumpComposer.canAddMoreMoves());

    PlyConfirmDialog jumpConfirmDlg(&jumpComposer, ThemeId::OBSIDIAN_BRASS);
    CHECK_TRUE(jumpConfirmDlg.isMoreMovesVisible());
    jumpConfirmDlg.renderToImage(460, 150);
    // Simulate clicking Commit after first hop
    QKeyEvent enterEv(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QCoreApplication::sendEvent(&jumpConfirmDlg, &enterEv);
    CHECK_TRUE(jumpConfirmDlg.chosenAction() == PlyConfirmDialog::ResultAction::COMMIT);
    CHECK_FALSE(jumpComposer.isGuidedMoveActive());
    CHECK_FALSE(jumpComposer.isAwaitingConfirmation());
    LONGS_EQUAL(1, jumpBoard.squareAt(2, 4).getNumPods()); // Landed at (3,5)

    // 5. PCE-D06: GameOverDialog luxury borderless card & endgame state
    jumpBoard.getGame()->resign(jumpBoard.getGame()->getTurn());
    CHECK_TRUE(jumpBoard.getGame()->isGameOver());

    GameOverDialog goDlg(&jumpBoard, ThemeId::OBSIDIAN_BRASS);
    CHECK_TRUE(goDlg.windowFlags() & Qt::FramelessWindowHint);
    CHECK_TRUE(goDlg.winnerBannerText().contains("PLAYER A WINS"));
    CHECK_TRUE(goDlg.terminationText().contains("Resignation"));

    QImage goImg = goDlg.renderToImage(480, 280);
    CHECK_FALSE(goImg.isNull());
    QString goPath = artifactDir + "/desktop_gameover_dialog_v3.png";
    CHECK_TRUE(goImg.save(goPath));
    CHECK_TRUE(QFileInfo(goPath).size() > 10000);
}

// ============================================================================
// Gate 2 Envelope 1: DesktopCommitDialogFixTest (ACF-D01 .. ACF-D04)
// ============================================================================

TEST_GROUP(DesktopCommitDialogFixTest)
{
};

TEST(DesktopCommitDialogFixTest, DirectProngAdditionOnBoardAndGuidedMove)
{
    OctiBoard board(GAME_MODE_2, true, true, GAME_BASES_3);
    board.getGame()->start(PLAYER_A);
    OctiMoveComposer composer(&board);
    OctiBoardCanvas canvas;
    canvas.resize(800, 800);
    canvas.attachBoard(&board, &composer);

    const OctiBoardGeometry &geom = canvas.geometry();

    // 1. Direct prong addition on pod with 0 prongs at (3, 3) -> 33+a
    QPointF c33 = geom.squareCenter(3, 3);
    double cell = geom.cellSize();
    QPointF ptA = c33 + geom.dirUnitVectorScreen(DIR_N) * (cell * 0.35);

    QMouseEvent prA(QEvent::MouseButtonPress, ptA, ptA, ptA,
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &prA);
    QMouseEvent relA(QEvent::MouseButtonRelease, ptA, ptA, ptA,
                     Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &relA);

    CHECK_TRUE(composer.isAwaitingConfirmation());
    STRCMP_EQUAL("33+a", composer.stagedMoveNotation().c_str());
    CHECK_TRUE(composer.commitStagedMove());

    const OctiSquare &sqAfterA = board.squareAt(2, 2);
    LONGS_EQUAL(1, sqAfterA.getNumPods());
    LONGS_EQUAL(static_cast<int>(NORMAL_PRONG), static_cast<int>(sqAfterA.getPodAt(0).prongAt(DIR_N)));

    // Player B's turn: make a dummy move for B
    CHECK_TRUE(composer.stageOrCommitAddProng(3, 7, 0, DIR_S, false));
    CHECK_TRUE(composer.commitStagedMove());

    // 2. Direct prong addition on pod with 1 prong (already has 'a') at (3, 3) -> 33a+b
    QPointF ptB = c33 + geom.dirUnitVectorScreen(DIR_NE) * (cell * 0.35);

    QMouseEvent prB(QEvent::MouseButtonPress, ptB, ptB, ptB,
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &prB);
    QMouseEvent relB(QEvent::MouseButtonRelease, ptB, ptB, ptB,
                     Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &relB);

    CHECK_TRUE(composer.isAwaitingConfirmation());
    STRCMP_EQUAL("33a+b", composer.stagedMoveNotation().c_str());
    CHECK_TRUE(composer.commitStagedMove());
    LONGS_EQUAL(static_cast<int>(NORMAL_PRONG), static_cast<int>(sqAfterA.getPodAt(0).prongAt(DIR_NE)));

    // Player B's turn: make a dummy move for B
    CHECK_TRUE(composer.stageOrCommitAddProng(3, 7, 0, DIR_SE, false));
    CHECK_TRUE(composer.commitStagedMove());

    // 3. Click pod center at (3, 3) activates guided move and allows destination click at (3, 4)
    QMouseEvent prC(QEvent::MouseButtonPress, c33, c33, c33,
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &prC);
    QMouseEvent relC(QEvent::MouseButtonRelease, c33, c33, c33,
                     Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &relC);

    CHECK_TRUE(composer.isGuidedMoveActive());
    LONGS_EQUAL(3, composer.guidedCurrentSquare().first);
    LONGS_EQUAL(3, composer.guidedCurrentSquare().second);

    // Click destination (3, 4) to step North
    QPointF c34 = geom.squareCenter(3, 4);
    QMouseEvent prDest(QEvent::MouseButtonPress, c34, c34, c34,
                       Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &prDest);
    QMouseEvent relDest(QEvent::MouseButtonRelease, c34, c34, c34,
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &relDest);

    CHECK_TRUE(composer.isAwaitingConfirmation());
    STRCMP_EQUAL("33ab-34", composer.stagedMoveNotation().c_str());
    CHECK_TRUE(composer.commitStagedMove());
}

TEST(DesktopCommitDialogFixTest, ContinuousHandDragRotationAndReleaseSnap)
{
    OctiBoard board(GAME_MODE_2, true, true, GAME_BASES_3);
    board.getGame()->start(PLAYER_A);
    OctiMoveComposer composer(&board);
    OctiBoardCanvas canvas;
    canvas.resize(800, 800);
    canvas.attachBoard(&board, &composer);

    LONGS_EQUAL(static_cast<int>(BoardRotation::ROT_0), static_cast<int>(canvas.boardRotation()));
    DOUBLES_EQUAL(0.0, canvas.liveRotationOffsetDeg(), 0.001);

    QPointF center = canvas.geometry().boardOuterRect().center();
    QPointF p0(center.x(), center.y() - 200.0); // angle = -90 deg

    // Press and hold for > 250ms to activate hand rotate
    QMouseEvent pressEv(QEvent::MouseButtonPress, p0, p0, p0,
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &pressEv);

    for (int i = 0; i < 35; ++i) {
        QCoreApplication::processEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Drag to p1 (angle = -55 deg) -> continuous arbitrary liveRotationOffsetDeg ~ +35 deg
    double r = 200.0;
    double rad1 = (-55.0) * 3.14159265358979323846 / 180.0;
    QPointF p1(center.x() + r * std::cos(rad1), center.y() + r * std::sin(rad1));
    QMouseEvent moveEv1(QEvent::MouseMove, p1, p1, p1,
                        Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &moveEv1);
    DOUBLES_EQUAL(35.0, canvas.liveRotationOffsetDeg(), 1.5);

    // Drag further to p2 (angle = -18 deg) -> liveRotationOffsetDeg ~ +72 deg
    double rad2 = (-18.0) * 3.14159265358979323846 / 180.0;
    QPointF p2(center.x() + r * std::cos(rad2), center.y() + r * std::sin(rad2));
    QMouseEvent moveEv2(QEvent::MouseMove, p2, p2, p2,
                        Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &moveEv2);
    DOUBLES_EQUAL(72.0, canvas.liveRotationOffsetDeg(), 1.5);

    // Drag to p3 (angle = -8 deg) -> liveRotationOffsetDeg ~ +82 deg
    double rad3 = (-8.0) * 3.14159265358979323846 / 180.0;
    QPointF p3(center.x() + r * std::cos(rad3), center.y() + r * std::sin(rad3));
    QMouseEvent moveEv3(QEvent::MouseMove, p3, p3, p3,
                        Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &moveEv3);
    DOUBLES_EQUAL(82.0, canvas.liveRotationOffsetDeg(), 1.5);

    // Release mouse at p3: snaps to nearest 90 deg -> ROT_90
    QMouseEvent relEv(QEvent::MouseButtonRelease, p3, p3, p3,
                      Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &relEv);

    LONGS_EQUAL(static_cast<int>(BoardRotation::ROT_90), static_cast<int>(canvas.boardRotation()));
    DOUBLES_EQUAL(0.0, canvas.liveRotationOffsetDeg(), 0.001);

    // Moving mouse after release without buttons pressed must NOT rotate the board
    QPointF p4(300.0, 200.0);
    QMouseEvent moveEv4(QEvent::MouseMove, p4, p4, p4,
                        Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&canvas, &moveEv4);

    LONGS_EQUAL(static_cast<int>(BoardRotation::ROT_90), static_cast<int>(canvas.boardRotation()));
    DOUBLES_EQUAL(0.0, canvas.liveRotationOffsetDeg(), 0.001);
}

TEST(DesktopCommitDialogFixTest, ZoomedSquareDialogLiberateVisibilityAndArtifacts)
{
    QString repoRootStr = QString::fromUtf8(OCTI_REPO_ROOT_DIR);
    QString artifactDir = repoRootStr + "/plan/android_commit_dialog_fix.artifacts";
    QDir().mkpath(artifactDir);

    OctiBoard board(GAME_MODE_2, true, true, GAME_BASES_3);
    board.getGame()->start(PLAYER_A);
    OctiMoveComposer composer(&board);

    // 1. When capturedPods == 0: "Liberate (*)" is hidden and Place Reserve is full width
    ZoomedSquareDialog zd0(&board, &composer, 3, 3, BoardRotation::ROT_0, ThemeId::OBSIDIAN_BRASS);
    QImage zdImg0 = zd0.renderToImage(560, 530);
    CHECK_FALSE(zdImg0.isNull());
    CHECK_TRUE(zd0.liberateButtonRect().isEmpty());
    CHECK_TRUE(zd0.liberateBtnRect().isEmpty());
    CHECK_TRUE(zd0.placeReserveBtnRect().width() >= 300.0);

    // 2. Add prongs to pod at (3, 3) for prominent rendering artifact:
    board.getGame()->move("33+a");
    board.getGame()->move("37+e");
    board.getGame()->move("33a+b");
    board.getGame()->move("37e+f");
    board.getGame()->move("33ab+c");
    board.getGame()->move("37ef+g");
    board.getGame()->move("33abc+d!");

    // Give A 1 captured pod on A's base (3, 3) and enemy base pod so canLiberate is true
    board.squareAt(2, 6).addPod(OctiPod(A_POD));
    OctiGameSnapshot snap = board.getGame()->captureSnapshot();
    snap.players[PLAYER_A].capturedPods = 1;
    snap.turn = PLAYER_A;
    board.getGame()->restoreSnapshot(snap);

    ZoomedSquareDialog zd1(&board, &composer, 3, 3, BoardRotation::ROT_0, ThemeId::OBSIDIAN_BRASS);
    QImage zdImg1 = zd1.renderToImage(560, 530);
    CHECK_FALSE(zdImg1.isNull());
    CHECK_FALSE(zd1.liberateButtonRect().isEmpty());
    CHECK_FALSE(zd1.liberateBtnRect().isEmpty());

    // Save desktop_zoomed_pod_prominent_prongs.png
    QString zdPath = artifactDir + "/desktop_zoomed_pod_prominent_prongs.png";
    CHECK_TRUE(zdImg1.save(zdPath));
    CHECK_TRUE(QFileInfo(zdPath).size() > 10000);

    // 3. desktop_board_prominent_prongs.png
    OctiBoardCanvas canvas;
    canvas.resize(960, 960);
    canvas.attachBoard(&board, &composer);
    QImage bImg = canvas.renderToImage(960, 960);
    CHECK_FALSE(bImg.isNull());
    QString bPath = artifactDir + "/desktop_board_prominent_prongs.png";
    CHECK_TRUE(bImg.save(bPath));
    CHECK_TRUE(QFileInfo(bPath).size() > 10000);

    // 4. desktop_splash_no_bar.png
    OctiSplashDialog splash(nullptr, ThemeId::OBSIDIAN_BRASS);
    QImage sImg = splash.renderSplashFrame(560, 480, 0.50);
    CHECK_FALSE(sImg.isNull());
    QString sPath = artifactDir + "/desktop_splash_no_bar.png";
    CHECK_TRUE(sImg.save(sPath));
    CHECK_TRUE(QFileInfo(sPath).size() > 10000);

    // 5. desktop_reserve_panel_captured.png
    // In standard 2P mode without superprongs:
    OctiBoard boardStandard(GAME_MODE_2, false, false, GAME_BASES_3);
    boardStandard.getGame()->start(PLAYER_A);
    OctiMoveComposer compStandard(&boardStandard);
    OctiBoardCanvas canvStandard;
    canvStandard.attachBoard(&boardStandard, &compStandard);

    OctiReservePanel panel;
    panel.attach(&boardStandard, &compStandard, &canvStandard);
    panel.setTheme(ThemeId::OBSIDIAN_BRASS);
    QImage rImg = panel.renderToImage(340, 720);
    CHECK_FALSE(rImg.isNull());
    CHECK_TRUE(panel.superProngWellRect().isEmpty()); // 3 wells mode!
    CHECK_FALSE(panel.liberatePodWellRect().isEmpty());
    QString rPath = artifactDir + "/desktop_reserve_panel_captured.png";
    CHECK_TRUE(rImg.save(rPath));
    CHECK_TRUE(QFileInfo(rPath).size() > 10000);
}

extern "C" const char *__lsan_default_suppressions()
{
    return "leak:libqoffscreen.so\n";
}

int main(int argc, char **argv)
{
    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();
    QApplication app(argc, argv);
    return CommandLineTestRunner::RunAllTests(argc, argv);
}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
