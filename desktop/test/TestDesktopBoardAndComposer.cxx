/*
 * TestDesktopBoardAndComposer.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <cmath>
#include <cstdint>
#include <cstring>
#include <set>
#include <string>
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
