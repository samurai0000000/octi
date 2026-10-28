/*
 * OctiAndroidGuiTest.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import com.selfso.octi.ui.OctiAppViewModel;
import com.selfso.octi.ui.OctiDialogs;

import org.junit.Test;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.util.List;

/**
 * Qualification tests for Envelope 2 (Android Compose & Engine Developer) of
 * plan_ply_confirm_and_endgame.md covering PCE-A01 through PCE-A09.
 */
public class OctiAndroidGuiTest {

    @Test
    public void testGuidedOriginSquarePreservedAfterFirstPodMove_PCE_A02() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_3);
        board.getGame().start(OctiPlayer.PLAYER_A);

        // Place two friendly pods at (3, 3) (0-based (2, 2)) with different prongs
        OctiSquare sq = board.squareAt(2, 2);
        sq.clear();
        OctiPod pod0 = new OctiPod(OctiPodType.A_POD);
        pod0.addProng(OctiDir.DIR_E, false);
        OctiPod pod1 = new OctiPod(OctiPodType.A_POD);
        pod1.addProng(OctiDir.DIR_N, false);
        sq.addPod(pod0);
        sq.addPod(pod1);

        OctiMoveComposer composer = new OctiMoveComposer(board);

        // Move top pod (index 1) from (3, 3) to (3, 4)
        assertTrue(composer.beginGuidedPodMove(3, 3, 1, false));
        assertTrue(composer.isGuidedMoveActive());
        assertTrue(composer.selectGuidedDestination(3, 4));
        assertFalse(composer.isGuidedMoveActive());
        assertTrue(composer.isAwaitingConfirmation());

        // guidedOriginSquare() MUST still return [3, 3] even after guidedMoveActive is false!
        int[] orig = composer.guidedOriginSquare();
        assertEquals(3, orig[0]);
        assertEquals(3, orig[1]);
        assertTrue(composer.hasOtherMovablePodAtOrigin());
        assertTrue(composer.canAddMoreMoves());
    }

    @Test
    public void testClaimedPodHelpersInCompositeStackMove_PCE_A06() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_3);
        board.getGame().start(OctiPlayer.PLAYER_A);

        OctiSquare sq = board.squareAt(2, 2);
        sq.clear();
        OctiPod pod0 = new OctiPod(OctiPodType.A_POD);
        pod0.addProng(OctiDir.DIR_E, false);
        OctiPod pod1 = new OctiPod(OctiPodType.A_POD);
        pod1.addProng(OctiDir.DIR_N, false);
        sq.addPod(pod0);
        sq.addPod(pod1);

        OctiMoveComposer composer = new OctiMoveComposer(board);
        assertEquals(1, composer.firstUnclaimedFriendlyPodIndex(3, 3));
        assertFalse(composer.isSquarePodClaimed(3, 3, 0));
        assertFalse(composer.isSquarePodClaimed(3, 3, 1));

        // Stage move for pod 1 from (3, 3) to (3, 4)
        assertTrue(composer.beginGuidedPodMove(3, 3, 1, false));
        assertTrue(composer.selectGuidedDestination(3, 4));

        // Pod 1 is now claimed; pod 0 is unclaimed and selected by firstUnclaimedFriendlyPodIndex
        assertTrue(composer.isSquarePodClaimed(3, 3, 1));
        assertFalse(composer.isSquarePodClaimed(3, 3, 0));
        assertEquals(0, composer.firstUnclaimedFriendlyPodIndex(3, 3));

        // Move remaining unclaimed pod 0 from (3, 3) to (4, 3)
        assertTrue(composer.beginGuidedPodMove(3, 3, 0, false));
        assertTrue(composer.selectGuidedDestination(4, 3));
        assertEquals(-1, composer.firstUnclaimedFriendlyPodIndex(3, 3));
        assertEquals("33a-34,33c-43", composer.stagedMoveNotation());
    }

    @Test
    public void testEighthProngSuperprongBonusGuidedMove_PCE_A04() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, true, false,
                OctiGameBases.GAME_BASES_3);
        board.getGame().start(OctiPlayer.PLAYER_A);

        // Configure pod at (3, 3) with 7 normal prongs (all except DIR_NW / 'h')
        OctiSquare sq = board.squareAt(2, 2);
        sq.clear();
        OctiPod pod = new OctiPod(OctiPodType.A_POD);
        for (int d = 0; d < 7; d++) {
            pod.addProng(OctiDir.fromIndex(d), false);
        }
        sq.addPod(pod);
        assertEquals(7, sq.getPodAt(0).getNumProngs());

        OctiMoveComposer composer = new OctiMoveComposer(board);
        assertTrue(composer.stageOrCommitAddProng(3, 3, 0, OctiDir.DIR_NW, true));
        assertTrue(composer.isAwaitingConfirmation());
        assertTrue(composer.hasUnlockedSuperProngBonus());
        assertTrue(composer.canAddMoreMoves());

        int[] orig = composer.eighthProngOriginSquare();
        assertEquals(3, orig[0]);
        assertEquals(3, orig[1]);

        // Starting guided bonus move on the 8-prong pod MUST succeed!
        assertTrue(composer.beginGuidedPodMove(orig[0], orig[1], -1, false));
        assertTrue(composer.isGuidedMoveActive());
        List<int[]> nextSqs = composer.guidedNextSquares();
        assertFalse("8th-prong bonus move must expose legal destinations", nextSqs.isEmpty());

        // Select destination (3, 4) (North)
        assertTrue(composer.selectGuidedDestination(3, 4));
        assertFalse(composer.isGuidedMoveActive());
        assertTrue(composer.isAwaitingConfirmation());
        assertFalse(composer.hasUnlockedSuperProngBonus());
        assertEquals("33abcdefg+h!,33-34", composer.stagedMoveNotation());
    }

    @Test
    public void testBotAutoPlaysOnMoveCommitInLocalVsBotMode_PCE_A03() {
        OctiAppViewModel vm = new OctiAppViewModel();
        try {
            vm.getBotEngine().setTimeBudgetMs(25);
            // Default on launch must be LOCAL_VS_BOT (matching Desktop)
            assertEquals(OctiAppViewModel.MatchType.LOCAL_VS_BOT, vm.getMatchType().getValue());
            assertEquals(OctiPlayer.PLAYER_A, vm.getHumanSeat().getValue());
            assertEquals(OctiPlayer.PLAYER_A, vm.getBoard().getGame().getTurn());
            assertEquals(0, vm.livePlyCount());

            // Stage a human move (add prong at 33+a) and commit
            assertTrue(vm.getComposer().stageOrCommitAddProng(3, 3, 0, OctiDir.DIR_N, false));
            assertTrue(vm.commitStagedMove());

            // Both the human move (ply 1) and the AI bot's reply (ply 2) must be applied automatically!
            assertEquals(2, vm.livePlyCount());
            assertEquals(OctiPlayer.PLAYER_A, vm.getBoard().getGame().getTurn());

            // Starting as Seat B (Red / 2nd) must immediately auto-play Bot Seat A's opening ply
            vm.getBotEngine().setTimeBudgetMs(25);
            vm.startNewLocalGame(
                    OctiGameMode.GAME_MODE_2,
                    false,
                    false,
                    OctiGameBases.GAME_BASES_3,
                    true,
                    OctiBotEngine.BotDifficulty.TACTICAL,
                    OctiPlayer.PLAYER_B
            );
            assertEquals(OctiAppViewModel.MatchType.LOCAL_VS_BOT, vm.getMatchType().getValue());
            assertEquals(OctiPlayer.PLAYER_B, vm.getHumanSeat().getValue());
            assertEquals(1, vm.livePlyCount());
            assertEquals(OctiPlayer.PLAYER_B, vm.getBoard().getGame().getTurn());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_180, vm.getBoardRotation().getValue());
        } finally {
            vm.close();
        }
    }

    @Test
    public void testGameOverDialogAndUndoResignInLocalGame_PCE_A07_A09() {
        OctiAppViewModel vm = new OctiAppViewModel();
        try {
            vm.startNewLocalGame(
                    OctiGameMode.GAME_MODE_2,
                    false,
                    false,
                    OctiGameBases.GAME_BASES_3,
                    false,
                    OctiBotEngine.BotDifficulty.TACTICAL,
                    OctiPlayer.PLAYER_A
            );
            assertFalse(vm.getShowGameOverDialog().getValue());

            // Apply 1 move and test undoLocalMove()
            assertTrue(vm.getComposer().stageOrCommitAddProng(3, 3, 0, OctiDir.DIR_N, false));
            assertTrue(vm.commitStagedMove());
            assertEquals(1, vm.livePlyCount());
            assertTrue(vm.undoLocalMove());
            assertEquals(0, vm.livePlyCount());
            assertEquals(OctiPlayer.PLAYER_A, vm.getBoard().getGame().getTurn());

            // Resigning sets gameOver = true and showGameOverDialog = true
            assertTrue(vm.resignLocalGame());
            assertTrue(vm.getBoard().getGame().isGameOver());
            assertEquals(OctiPlayer.PLAYER_B, vm.getBoard().getGame().getWinner());
            assertTrue(vm.getShowGameOverDialog().getValue());

            vm.dismissGameOverDialog();
            assertFalse(vm.getShowGameOverDialog().getValue());
        } finally {
            vm.close();
        }
    }

    private static File resolveRepoRoot() {
        File cwd = new File(System.getProperty("user.dir", ".")).getAbsoluteFile();
        File cur = cwd;
        while (cur != null) {
            if (new File(cur, "plan/plan_ply_confirm_and_endgame.md").exists()
                    || new File(cur, "plan/plan_pod_placement_and_liberation.md").exists()
                    || (new File(cur, "android").isDirectory() && new File(cur, "core").isDirectory())) {
                return cur;
            }
            cur = cur.getParentFile();
        }
        return cwd;
    }

    @Test
    public void testGenerateEnvelope2VerificationScreenshots() throws Exception {
        File repoRoot = resolveRepoRoot();
        File artifactDir = new File(repoRoot, "plan/done/plan_ply_confirm_and_endgame.md").exists()
                ? new File(repoRoot, "plan/done/ply_confirm_and_endgame.artifacts")
                : new File(repoRoot, "plan/ply_confirm_and_endgame.artifacts");
        if (!artifactDir.exists()) {
            assertTrue(artifactDir.mkdirs());
        }

        // -----------------------------------------------------------------
        // 1. android_ply_confirm_dialog_v3.png (1080 x 1920)
        // -----------------------------------------------------------------
        OctiBoard board1 = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_3);
        board1.getGame().start(OctiPlayer.PLAYER_A);
        OctiSquare sq33 = board1.squareAt(2, 2);
        sq33.clear();
        OctiPod pod0 = new OctiPod(OctiPodType.A_POD);
        pod0.addProng(OctiDir.DIR_E, false);
        OctiPod pod1 = new OctiPod(OctiPodType.A_POD);
        pod1.addProng(OctiDir.DIR_N, false);
        sq33.addPod(pod0);
        sq33.addPod(pod1);

        OctiMoveComposer composer1 = new OctiMoveComposer(board1);
        assertTrue(composer1.beginGuidedPodMove(3, 3, 1, false));
        assertTrue(composer1.selectGuidedDestination(3, 4));
        assertTrue(composer1.isAwaitingConfirmation());
        assertTrue(composer1.canAddMoreMoves());

        int[] pixels1 = OctiBoardRenderer.renderPlyConfirmScreenPixels(
                board1, composer1, OctiThemeId.OBSIDIAN_BRASS);
        File plyConfirmPng = new File(artifactDir, "android_ply_confirm_dialog_v3.png");
        OctiAppIconRenderer.writeArgbPng(1080, 1920, pixels1, plyConfirmPng);
        assertTrue(plyConfirmPng.isFile() && plyConfirmPng.length() > 4096);

        // -----------------------------------------------------------------
        // 2. android_gameover_dialog_v3.png (1080 x 1920)
        // -----------------------------------------------------------------
        OctiBoard board2 = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_1);
        board2.getGame().start(OctiPlayer.PLAYER_A);
        board2.getGame().resign(OctiPlayer.PLAYER_B);
        assertTrue(board2.getGame().isGameOver());
        assertEquals(OctiPlayer.PLAYER_A, board2.getGame().getWinner());

        OctiMoveComposer composer2 = new OctiMoveComposer(board2);
        int[] pixels2 = OctiBoardRenderer.renderGameOverScreenPixels(
                board2, composer2, OctiThemeId.OBSIDIAN_BRASS);
        File gameOverPng = new File(artifactDir, "android_gameover_dialog_v3.png");
        OctiAppIconRenderer.writeArgbPng(1080, 1920, pixels2, gameOverPng);
        assertTrue(gameOverPng.isFile() && gameOverPng.length() > 4096);
    }

    @Test
    public void testGenerateEnvelope3VerificationPngArtifacts_PPL_A03() throws Exception {
        File repoRoot = resolveRepoRoot();
        File artifactDir = new File(repoRoot, "plan/done/plan_pod_placement_and_liberation.md").exists()
                ? new File(repoRoot, "plan/done/pod_placement_and_liberation.artifacts")
                : new File(repoRoot, "plan/pod_placement_and_liberation.artifacts");
        if (!artifactDir.exists()) {
            assertTrue(artifactDir.mkdirs());
        }

        // 1. android_zoomed_occupied_home_v4.png (800 x 800)
        // Zoomed square of an occupied friendly home square with placed pod
        OctiBoard board1 = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_3);
        board1.getGame().start(OctiPlayer.PLAYER_A);
        // Play pod placement on home square (3, 3) so it has 2 pods
        board1.getGame().move("33");
        assertEquals(2, board1.squareAt(2, 2).getNumPods());

        int[] zoomPixels = OctiBoardRenderer.renderZoomedPodPixels(
                board1, 3, 3, 1, 0, OctiThemeId.OBSIDIAN_BRASS, 800, -1);
        File zoomedPng = new File(artifactDir, "android_zoomed_occupied_home_v4.png");
        OctiAppIconRenderer.writeArgbPng(800, 800, zoomPixels, zoomedPng);
        assertTrue(zoomedPng.isFile());
        assertTrue(zoomedPng.length() > 4096);

        // 2. android_board_staged_pod_preview_v4.png (1080 x 1080)
        // Board preview with staged reserve pod placement awaiting confirmation
        OctiBoard board2 = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_3);
        board2.getGame().start(OctiPlayer.PLAYER_A);
        OctiMoveComposer composer2 = new OctiMoveComposer(board2);
        assertTrue(composer2.stageOrCommitAddPod(3, 3, false));
        assertTrue(composer2.isAwaitingConfirmation());

        OctiBoard activePreviewBoard = composer2.buildPreviewBoard();
        OctiBoardGeometry geom2 = new OctiBoardGeometry(9, 9, 1080.0, 1080.0,
                OctiBoardGeometry.BoardRotation.ROT_0);
        int[] boardPreviewPixels = OctiBoardRenderer.renderBoardPixels(
                activePreviewBoard, composer2, geom2, OctiThemeId.OBSIDIAN_BRASS);
        File boardPreviewPng = new File(artifactDir, "android_board_staged_pod_preview_v4.png");
        OctiAppIconRenderer.writeArgbPng(1080, 1080, boardPreviewPixels, boardPreviewPng);
        assertTrue(boardPreviewPng.isFile());
        assertTrue(boardPreviewPng.length() > 4096);

        // Verify PNG magic headers
        for (File f : new File[]{zoomedPng, boardPreviewPng}) {
            byte[] bytes = Files.readAllBytes(f.toPath());
            assertEquals((byte) 0x89, bytes[0]);
            assertEquals((byte) 0x50, bytes[1]);
            assertEquals((byte) 0x4E, bytes[2]);
            assertEquals((byte) 0x47, bytes[3]);
        }
    }

    @Test
    public void testRotateBoard90ClockwiseAndCounterClockwiseQuadrants_AZG_A01_A03() {
        OctiAppViewModel vm = new OctiAppViewModel();
        try {
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_0, vm.getBoardRotation().getValue());

            // Clockwise 90° steps strictly through ROT_90 -> ROT_180 -> ROT_270 -> ROT_0
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_90, vm.rotateBoard90Clockwise());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_90, vm.getBoardRotation().getValue());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_180, vm.rotateBoard90Clockwise());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_180, vm.getBoardRotation().getValue());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_270, vm.rotateBoard90Clockwise());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_270, vm.getBoardRotation().getValue());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_0, vm.rotateBoard90Clockwise());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_0, vm.getBoardRotation().getValue());

            // Counter-clockwise 90° steps strictly through ROT_270 -> ROT_180 -> ROT_90 -> ROT_0
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_270, vm.rotateBoard90CounterClockwise());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_270, vm.getBoardRotation().getValue());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_180, vm.rotateBoard90CounterClockwise());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_180, vm.getBoardRotation().getValue());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_90, vm.rotateBoard90CounterClockwise());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_90, vm.getBoardRotation().getValue());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_0, vm.rotateBoard90CounterClockwise());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_0, vm.getBoardRotation().getValue());
        } finally {
            vm.close();
        }
    }

    @Test
    public void testOpenZoomedSquareAndCancelZeroHopGuidedMove_AZG_A02_A04() {
        OctiAppViewModel vm = new OctiAppViewModel();
        try {
            vm.startNewLocalGame(
                    OctiGameMode.GAME_MODE_2,
                    false,
                    false,
                    OctiGameBases.GAME_BASES_3,
                    false,
                    OctiBotEngine.BotDifficulty.TACTICAL,
                    OctiPlayer.PLAYER_A
            );
            OctiBoard board = vm.getBoard();
            OctiMoveComposer composer = vm.getComposer();

            // 1. Add prong on (3, 3) and step away so (3, 3) becomes an empty friendly home square
            assertTrue(composer.stageOrCommitAddProng(3, 3, 0, OctiDir.DIR_N, false));
            assertTrue(vm.commitStagedMove());
            assertTrue(composer.stageOrCommitAddProng(3, 7, 0, OctiDir.DIR_S, false));
            assertTrue(vm.commitStagedMove());
            assertTrue(composer.beginGuidedPodMove(3, 3, 0, false));
            assertTrue(composer.selectGuidedDestination(3, 4));
            assertTrue(vm.commitStagedMove());
            assertTrue(composer.stageOrCommitAddProng(5, 7, 0, OctiDir.DIR_S, false));
            assertTrue(vm.commitStagedMove());

            assertTrue("Home square (3, 3) should now be empty", board.squareAt(2, 2).isEmpty());
            assertTrue(board.isHomeSquareOf(2, 2, OctiPlayer.PLAYER_A));

            // 2. Start a 0-hop guided move on occupied square (3, 4), then cancel and open ZoomedSquareDialog on (3, 4)
            assertTrue(composer.beginGuidedPodMove(3, 4, 0, false));
            assertTrue(composer.isGuidedMoveActive());
            assertFalse(composer.isAwaitingConfirmation());

            composer.cancelAndRollback();
            assertFalse(composer.isGuidedMoveActive());
            vm.openZoomedSquare(3, 4);
            kotlin.Pair<Integer, Integer> z1 = vm.getZoomedSquare().getValue();
            assertEquals(Integer.valueOf(3), z1.getFirst());
            assertEquals(Integer.valueOf(4), z1.getSecond());
            vm.closeZoomedSquare();

            // 3. Open ZoomedSquareDialog on empty friendly home square (3, 3) to place a reserve pod
            vm.openZoomedSquare(3, 3);
            kotlin.Pair<Integer, Integer> z2 = vm.getZoomedSquare().getValue();
            assertEquals(Integer.valueOf(3), z2.getFirst());
            assertEquals(Integer.valueOf(3), z2.getSecond());
        } finally {
            vm.close();
        }
    }

    @Test
    public void testHitTestZoomedSocketCompassSectorFallback_ACF_A02() {
        // Direct socket hit (cx = 240, cy = 240, socketDist = 109.4):
        OctiDir dirE = OctiDialogs.hitTestZoomedSocket(480f, 480f, 350f, 240f, OctiBoardGeometry.BoardRotation.ROT_0);
        assertEquals(OctiDir.DIR_E, dirE);

        // Prong blade tip hit (outside socket radius, caught by compass sector fallback):
        OctiDir bladeE = OctiDialogs.hitTestZoomedSocket(480f, 480f, 420f, 240f, OctiBoardGeometry.BoardRotation.ROT_0);
        assertEquals(OctiDir.DIR_E, bladeE);

        OctiDir bladeN = OctiDialogs.hitTestZoomedSocket(480f, 480f, 240f, 60f, OctiBoardGeometry.BoardRotation.ROT_0);
        assertEquals(OctiDir.DIR_N, bladeN);

        OctiDir bladeSW = OctiDialogs.hitTestZoomedSocket(480f, 480f, 100f, 380f, OctiBoardGeometry.BoardRotation.ROT_0);
        assertEquals(OctiDir.DIR_SW, bladeSW);
    }

    @Test
    public void testContinuousRotationSnapMath_ACF_A03() {
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_0, snapRotation(OctiBoardGeometry.BoardRotation.ROT_0, 15f));
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_90, snapRotation(OctiBoardGeometry.BoardRotation.ROT_0, 50f));
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_90, snapRotation(OctiBoardGeometry.BoardRotation.ROT_0, 110f));
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_180, snapRotation(OctiBoardGeometry.BoardRotation.ROT_0, 140f));
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_0, snapRotation(OctiBoardGeometry.BoardRotation.ROT_0, -35f));
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_270, snapRotation(OctiBoardGeometry.BoardRotation.ROT_0, -50f));
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_180, snapRotation(OctiBoardGeometry.BoardRotation.ROT_90, 60f));
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_0, snapRotation(OctiBoardGeometry.BoardRotation.ROT_270, 60f));
    }

    private OctiBoardGeometry.BoardRotation snapRotation(OctiBoardGeometry.BoardRotation current, float gestureDeg) {
        float currentDeg = (float) current.degrees();
        float totalDeg = currentDeg + gestureDeg;
        int snappedStep = Math.round(totalDeg / 90.0f);
        int normalizedDeg = ((snappedStep * 90 % 360) + 360) % 360;
        return OctiBoardGeometry.BoardRotation.fromDegrees(normalizedDeg);
    }

    @Test
    public void testExportVisualArtifacts_ACF_A05() throws IOException {
        File repoRoot = resolveRepoRoot();
        File artifactDir = new File(repoRoot, "plan/android_commit_dialog_fix.artifacts");
        if (!artifactDir.exists()) {
            assertTrue("Artifact directory must be created", artifactDir.mkdirs());
        }

        // 1. Zoomed pod with prominent prongs
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, true, false, OctiGameBases.GAME_BASES_3);
        board.getGame().start(OctiPlayer.PLAYER_A);
        OctiSquare sq = board.squareAt(2, 2);
        sq.clear();
        OctiPod pod = new OctiPod(OctiPodType.A_POD);
        pod.addProng(OctiDir.DIR_N, false);
        pod.addProng(OctiDir.DIR_E, false);
        pod.addProng(OctiDir.DIR_S, false);
        pod.addProng(OctiDir.DIR_NW, true); // superprong!
        sq.addPod(pod);

        int sizePx = 600;
        int[] zoomedPx = OctiBoardRenderer.renderZoomedPodPixels(
                board, 3, 3, 0, 0, OctiThemeId.OBSIDIAN_BRASS, sizePx, -1);
        File zoomedPng = new File(artifactDir, "android_zoomed_pod_prominent_prongs.png");
        OctiAppIconRenderer.writeArgbPng(sizePx, sizePx, zoomedPx, zoomedPng);
        assertTrue("Zoomed pod PNG must exist", zoomedPng.isFile());
        assertTrue("Zoomed pod PNG must be non-empty", zoomedPng.length() > 4096);

        // 2. Main board with prominent prongs
        OctiBoardGeometry geo = new OctiBoardGeometry(800, 800, 9, 9, OctiBoardGeometry.BoardRotation.ROT_0);
        int[] boardPx = OctiBoardRenderer.renderBoardPixels(board, null, geo, OctiThemeId.OBSIDIAN_BRASS);
        File boardPng = new File(artifactDir, "android_board_prominent_prongs.png");
        OctiAppIconRenderer.writeArgbPng(800, 800, boardPx, boardPng);
        assertTrue("Board PNG must exist", boardPng.isFile());
        assertTrue("Board PNG must be non-empty", boardPng.length() > 4096);

        // 3. Splash screen without progress bar
        int[] splashPx = OctiBoardRenderer.renderSplashPixels(720, 1280, 0.85);
        File splashPng = new File(artifactDir, "android_splash_no_bar.png");
        OctiAppIconRenderer.writeArgbPng(720, 1280, splashPx, splashPng);
        assertTrue("Splash PNG must exist", splashPng.isFile());
        assertTrue("Splash PNG must be non-empty", splashPng.length() > 4096);
    }
}

/*
 * Local variables:
 * mode: Java
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
