/*
 * OctiDesktopParityTest.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

import com.selfso.octi.ui.OctiAppViewModel;

import java.io.File;

public class OctiDesktopParityTest {

    private static File resolveRepoRoot() {
        File cur = new File(System.getProperty("user.dir", ".")).getAbsoluteFile();
        while (cur != null) {
            if (new File(cur, "plan").isDirectory() && new File(cur, "android").isDirectory()) {
                return cur;
            }
            cur = cur.getParentFile();
        }
        return new File(System.getProperty("user.dir", ".")).getAbsoluteFile();
    }

    @Test
    public void testComputeInstalledApkSha256PrefixReturnsDevLocalForNullContext() {
        String prefix = OctiAppViewModel.computeInstalledApkSha256Prefix(null, 12);
        assertEquals("dev-local", prefix);
    }

    @Test
    public void testAwaitingConfirmationStateFlowTransitionsOnProngAdditionAndBoardMove() {
        OctiAppViewModel vm = new OctiAppViewModel();
        try {
            vm.startNewLocalGame(
                    OctiGameMode.GAME_MODE_2,
                    false,
                    false,
                    OctiGameBases.GAME_BASES_3,
                    false
            );

            assertFalse("Initially should not be awaiting confirmation",
                    vm.getAwaitingConfirmation().getValue());

            // 1. Stage a prong addition via composer and refresh display
            boolean prongStaged = vm.getComposer().stageOrCommitAddProng(3, 3, 0, OctiDir.DIR_N, false);
            assertTrue("stageOrCommitAddProng should succeed", prongStaged);
            vm.refreshDisplayBoard();

            assertTrue("viewModel.awaitingConfirmation should be true after staging prong",
                    vm.getAwaitingConfirmation().getValue());
            assertTrue("composer.isAwaitingConfirmation should be true",
                    vm.getComposer().isAwaitingConfirmation());
            assertEquals("33+a", vm.getStagedMoveNotation().getValue());

            // Commit the prong move
            vm.commitStagedMove();
            assertFalse("Awaiting confirmation should be false after commit",
                    vm.getAwaitingConfirmation().getValue());
            assertEquals(1, vm.livePlyCount());

            // 2. Player B turn: stage a prong addition
            assertEquals(OctiPlayer.PLAYER_B, vm.getBoard().getGame().getTurn());
            boolean bProngStaged = vm.getComposer().stageOrCommitAddProng(3, 7, 0, OctiDir.DIR_S, false);
            assertTrue("Player B stage prong should succeed", bProngStaged);
            vm.refreshDisplayBoard();
            assertTrue(vm.getAwaitingConfirmation().getValue());
            vm.commitStagedMove();
            assertFalse(vm.getAwaitingConfirmation().getValue());
            assertEquals(2, vm.livePlyCount());

            // 3. Player A turn: move pod from (3,3) to (3,4) using the added DIR_N prong
            assertEquals(OctiPlayer.PLAYER_A, vm.getBoard().getGame().getTurn());
            boolean stepStaged = vm.getComposer().stageOrCommitPodMoveOrJump(3, 3, 0, 3, 4);
            assertTrue("Step move from 3,3 to 3,4 should succeed", stepStaged);
            vm.refreshDisplayBoard();

            assertTrue("viewModel.awaitingConfirmation should be true after staging pod move",
                    vm.getAwaitingConfirmation().getValue());
            assertTrue(vm.getStagedMoveNotation().getValue().contains("33"));
            assertTrue(vm.getStagedMoveNotation().getValue().contains("34"));

            // 4. Test auto-rollback on new interaction
            vm.getComposer().cancelAndRollback();
            vm.refreshDisplayBoard();
            assertFalse("Awaiting confirmation should be false after rollback",
                    vm.getAwaitingConfirmation().getValue());
        } finally {
            vm.close();
        }
    }

    @Test
    public void testRenderSplashPixelsWithSha256AndExportArtifact() throws Exception {
        File repoRoot = resolveRepoRoot();
        File artifactDir = new File(repoRoot, "plan/liberation_rules_and_commit_dialog.artifacts");
        artifactDir.mkdirs();

        int w = 720;
        int h = 1280;
        String testSha = "c8a253fe98ba";
        int[] splashPixels = OctiBoardRenderer.renderSplashPixels(
                w, h, OctiThemeId.OBSIDIAN_BRASS, 0.75, testSha);
        assertNotNull("renderSplashPixels must produce pixels", splashPixels);
        assertEquals(w * h, splashPixels.length);

        File splashFile = new File(artifactDir, "android_splash_sha256.png");
        OctiAppIconRenderer.writeArgbPng(w, h, splashPixels, splashFile);

        assertTrue("android_splash_sha256.png artifact must exist", splashFile.isFile());
        assertTrue("android_splash_sha256.png must be non-empty", splashFile.length() > 4096);
    }
}

/*
 * Local variables:
 * mode: Java
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
