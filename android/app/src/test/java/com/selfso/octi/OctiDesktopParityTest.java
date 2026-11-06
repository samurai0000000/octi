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

    @Test
    public void testAllTenThemePalettesParityAndCompleteness() {
        OctiThemeId[] themes = OctiThemeId.values();
        assertEquals("Must support exactly 10 themes", 10, themes.length);

        for (OctiThemeId themeId : themes) {
            OctiPalette pal = OctiPalette.paletteForTheme(themeId);
            assertNotNull("Palette for " + themeId + " must not be null", pal);
            assertEquals("ThemeId must match", themeId, pal.themeId);

            // Verify chrome colors are non-zero
            assertTrue("windowBg non-zero", pal.windowBg != 0);
            assertTrue("panelCardBg non-zero", pal.panelCardBg != 0);
            assertTrue("panelCardBorder non-zero", pal.panelCardBorder != 0);
            assertTrue("textPrimary non-zero", pal.textPrimary != 0);
            assertTrue("textSecondary non-zero", pal.textSecondary != 0);
            assertTrue("accentBrass non-zero", pal.accentBrass != 0);

            // Verify board colors
            assertTrue("boardOuterFrame non-zero", pal.boardOuterFrame != 0);
            assertTrue("gridCellDark non-zero", pal.gridCellDark != 0);
            assertTrue("gridCellLight non-zero", pal.gridCellLight != 0);
            assertTrue("coordLabel non-zero", pal.coordLabel != 0);

            // Verify guide colors
            assertTrue("neutralOctiFill non-zero", pal.neutralOctiFill != 0);
            assertTrue("neutralOctiRim non-zero", pal.neutralOctiRim != 0);
            assertTrue("edgelessPortalGlow non-zero", pal.edgelessPortalGlow != 0);
            assertTrue("legalStepGuide non-zero", pal.legalStepGuide != 0);
            assertTrue("legalJumpGuide non-zero", pal.legalJumpGuide != 0);
            assertTrue("legalCaptureGlow non-zero", pal.legalCaptureGlow != 0);

            // Verify prongs
            assertTrue("prongNormalMetal non-zero", pal.prongNormalMetal != 0);
            assertTrue("prongNormalTip non-zero", pal.prongNormalTip != 0);
            assertTrue("prongSuperGold non-zero", pal.prongSuperGold != 0);
            assertTrue("prongSuperCore non-zero", pal.prongSuperCore != 0);

            // Verify style layers
            assertNotNull("podFinish non-null", pal.podFinish);
            assertNotNull("podPattern non-null", pal.podPattern);
            assertNotNull("prongStyle non-null", pal.prongStyle);
            assertNotNull("boardTexture non-null", pal.boardTexture);
            assertNotNull("cellShape non-null", pal.cellShape);
            assertTrue("glowStrength in [0.0, 1.0]", pal.glowStrength >= 0.0f && pal.glowStrength <= 1.0f);

            // Verify faction palettes
            OctiPalette.PodFactionPalette[] factions = new OctiPalette.PodFactionPalette[] {
                pal.podA, pal.podB, pal.podC, pal.podD
            };
            for (int f = 0; f < factions.length; f++) {
                OctiPalette.PodFactionPalette fp = factions[f];
                assertNotNull("PodFactionPalette for faction " + f + " non-null", fp);
                assertTrue("core non-zero", fp.core != 0);
                assertTrue("rim non-zero", fp.rim != 0);
                assertTrue("highlight non-zero", fp.highlight != 0);
                assertTrue("shadow non-zero", fp.shadow != 0);
                assertTrue("emblem non-zero", fp.emblem != 0);
                assertNotNull("pattern non-null", fp.pattern);
            }

            // Verify faction mapping helpers
            assertNotNull("factionForPodType A_POD", pal.factionForPodType(OctiPodType.A_POD));
            assertNotNull("factionForPodType B_POD", pal.factionForPodType(OctiPodType.B_POD));
            assertNotNull("factionForPodType C_POD", pal.factionForPodType(OctiPodType.C_POD));
            assertNotNull("factionForPodType D_POD", pal.factionForPodType(OctiPodType.D_POD));
            assertNotNull("factionForPlayer PLAYER_A", pal.factionForPlayer(OctiPlayer.PLAYER_A));
            assertNotNull("factionForPlayer PLAYER_B", pal.factionForPlayer(OctiPlayer.PLAYER_B));
            assertNotNull("factionForPlayer PLAYER_C", pal.factionForPlayer(OctiPlayer.PLAYER_C));
            assertNotNull("factionForPlayer PLAYER_D", pal.factionForPlayer(OctiPlayer.PLAYER_D));
        }
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
