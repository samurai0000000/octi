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

import org.junit.Test;

import java.io.File;
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
            vm.startNewLocalGame(
                    OctiGameMode.GAME_MODE_2,
                    false,
                    false,
                    OctiGameBases.GAME_BASES_3,
                    true,
                    OctiBotEngine.BotDifficulty.TACTICAL,
                    OctiPlayer.PLAYER_A
            );
            assertEquals(OctiAppViewModel.MatchType.LOCAL_VS_BOT, vm.getMatchType().getValue());
            assertEquals(OctiPlayer.PLAYER_A, vm.getBoard().getGame().getTurn());
            assertEquals(0, vm.livePlyCount());

            // Stage a human move (add prong at 33+a) and commit
            assertTrue(vm.getComposer().stageOrCommitAddProng(3, 3, 0, OctiDir.DIR_N, false));
            assertTrue(vm.commitStagedMove());

            // Both the human move (ply 1) and the AI bot's reply (ply 2) must be applied automatically!
            assertEquals(2, vm.livePlyCount());
            assertEquals(OctiPlayer.PLAYER_A, vm.getBoard().getGame().getTurn());
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
