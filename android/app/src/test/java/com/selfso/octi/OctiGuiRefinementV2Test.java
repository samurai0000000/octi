/*
 * OctiGuiRefinementV2Test.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

import java.io.File;
import java.nio.file.Files;

public class OctiGuiRefinementV2Test {

    private static File resolveRepoRoot() {
        File cwd = new File(System.getProperty("user.dir", ".")).getAbsoluteFile();
        File cur = cwd;
        while (cur != null) {
            if (new File(cur, "plan/plan_gui_refinement_v2.md").exists()
                    || (new File(cur, "android").isDirectory() && new File(cur, "core").isDirectory())) {
                return cur;
            }
            cur = cur.getParentFile();
        }
        return cwd;
    }

    @Test
    public void testSplashDurationMsConstant() {
        assertEquals(10000L, com.selfso.octi.ui.OctiDialogs.SPLASH_DURATION_MS);
    }

    @Test
    public void testZoomedSquareSocketOrientationAndHighDpiHitTesting() {
        // Socket orientation at ROT_0
        OctiBoardGeometry geo0 = new OctiBoardGeometry(840, 840, 9, 9,
                OctiBoardGeometry.BoardRotation.ROT_0);

        // Socket 'a' (DIR_N) is at 12 o'clock top (u0 = 0, u1 = -1)
        double[] uA = geo0.dirUnitVectorScreen(OctiDir.DIR_N);
        assertEquals(0.0, uA[0], 1e-6);
        assertEquals(-1.0, uA[1], 1e-6);

        // Socket 'c' (DIR_E) is at 3 o'clock right (u0 = 1, u1 = 0)
        double[] uC = geo0.dirUnitVectorScreen(OctiDir.DIR_E);
        assertEquals(1.0, uC[0], 1e-6);
        assertEquals(0.0, uC[1], 1e-6);

        // Socket 'e' (DIR_S) is at 6 o'clock bottom (u0 = 0, u1 = 1)
        double[] uE = geo0.dirUnitVectorScreen(OctiDir.DIR_S);
        assertEquals(0.0, uE[0], 1e-6);
        assertEquals(1.0, uE[1], 1e-6);

        // Socket 'g' (DIR_W) is at 9 o'clock left (u0 = -1, u1 = 0)
        double[] uG = geo0.dirUnitVectorScreen(OctiDir.DIR_W);
        assertEquals(-1.0, uG[0], 1e-6);
        assertEquals(0.0, uG[1], 1e-6);

        // Rotation parity verification across all 4 rotations
        for (int deg : new int[]{0, 90, 180, 270}) {
            OctiBoardGeometry.BoardRotation rot = OctiBoardGeometry.BoardRotation.fromDegrees(deg);
            OctiBoardGeometry geoRot = new OctiBoardGeometry(840, 840, 9, 9, rot);
            for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                OctiDir dir = OctiDir.fromIndex(d);
                double[] u = geoRot.dirUnitVectorScreen(dir);
                double len = Math.hypot(u[0], u[1]);
                assertEquals(1.0, len, 1e-5);
            }
        }

        // High-DPI pixel accuracy hit testing on 840x840 canvas
        float wPx = 840f;
        float hPx = 840f;
        float cx = wPx * 0.5f;
        float cy = hPx * 0.5f;
        float podR = Math.min(wPx, hPx) * 0.30f;
        float socketDist = podR * 0.88f;

        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            OctiDir dir = OctiDir.fromIndex(d);
            double[] u = geo0.dirUnitVectorScreen(dir);
            float touchX = cx + (float) (u[0] * socketDist);
            float touchY = cy + (float) (u[1] * socketDist);

            OctiDir hit = com.selfso.octi.ui.OctiDialogs.hitTestZoomedSocket(
                    wPx, hPx, touchX, touchY, OctiBoardGeometry.BoardRotation.ROT_0);
            assertEquals("Socket " + (char) ('a' + dir.getIndex()) + " must be hit accurately at high DPI", dir, hit);
        }
    }

    @Test
    public void testBuildPreviewBoardStagingAndRollback() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_3);
        board.getGame().start(OctiPlayer.PLAYER_A);

        OctiMoveComposer composer = new OctiMoveComposer(board);
        assertFalse(composer.isAwaitingConfirmation());
        assertEquals("When not awaiting confirmation, buildPreviewBoard must return active board",
                board, composer.buildPreviewBoard());

        // Stage adding a normal prong at (3, 3) facing North
        boolean staged = composer.stageOrCommitAddProng(3, 3, 0, OctiDir.DIR_N, false);
        assertTrue("Move staging must succeed", staged);
        assertTrue("Composer must be awaiting confirmation", composer.isAwaitingConfirmation());

        // Live board must NOT be modified yet
        assertEquals(OctiProngType.NO_PRONG,
                board.squareAt(2, 2).getPodAt(0).prongAt(OctiDir.DIR_N));

        // Preview board must reflect the staged prong
        OctiBoard preview = composer.buildPreviewBoard();
        assertNotNull(preview);
        assertEquals(OctiProngType.NORMAL_PRONG,
                preview.squareAt(2, 2).getPodAt(0).prongAt(OctiDir.DIR_N));

        // Rolling back cancels the staged move
        composer.rollbackMove();
        assertFalse("Composer must no longer be awaiting confirmation after rollback",
                composer.isAwaitingConfirmation());
        assertEquals(OctiProngType.NO_PRONG,
                board.squareAt(2, 2).getPodAt(0).prongAt(OctiDir.DIR_N));
        assertEquals(board, composer.buildPreviewBoard());
    }

    @Test
    public void testSelectStackPodIndexDoesNotLeaveStackFanOpen() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_3);
        board.getGame().start(OctiPlayer.PLAYER_A);

        OctiSquare sq = board.squareAt(2, 2);
        sq.addPod(new OctiPod(OctiPodType.A_POD));

        OctiMoveComposer composer = new OctiMoveComposer(board);
        assertFalse(composer.isStackFanOpen());

        // Selecting a pod in a stack must NOT open the stack fan
        composer.selectStackPodIndex(3, 3, 1);
        assertFalse("selectStackPodIndex must not leave stack fan open!", composer.isStackFanOpen());

        // Only explicit openStackFan opens it
        composer.openStackFan(3, 3);
        assertTrue(composer.isStackFanOpen());

        composer.closeStackFan();
        assertFalse(composer.isStackFanOpen());
    }

    @Test
    public void testRenderZoomedPodPixelsWithStackUnderlay() {
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
        assertEquals(2, sq.getNumPods());

        int[] pixels = OctiBoardRenderer.renderZoomedPodPixels(
                board, 3, 3, 1, 0, OctiThemeId.OBSIDIAN_BRASS, 600, -1);
        assertNotNull(pixels);
        assertEquals(600 * 600, pixels.length);
    }

    @Test
    public void testGenerateVisualVerificationPngArtifacts() throws Exception {
        File repoRoot = resolveRepoRoot();
        File artifactDir = new File(repoRoot, "plan/done/plan_gui_refinement_v2.md").exists()
                ? new File(repoRoot, "plan/done/plan_gui_refinement_v2.artifacts")
                : new File(repoRoot, "plan/plan_gui_refinement_v2.artifacts");
        if (!artifactDir.exists()) {
            assertTrue(artifactDir.mkdirs());
        }

        // 1. android_zoomed_green_pod_v2.png (800 x 800)
        OctiBoard board1 = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_3);
        board1.getGame().start(OctiPlayer.PLAYER_A);
        board1.getGame().move("33+a");

        int[] zoomPixels = OctiBoardRenderer.renderZoomedPodPixels(
                board1, 3, 3, 0, 0, OctiThemeId.OBSIDIAN_BRASS, 800, -1);
        File zoomedPng = new File(artifactDir, "android_zoomed_green_pod_v2.png");
        OctiAppIconRenderer.writeArgbPng(800, 800, zoomPixels, zoomedPng);
        assertTrue(zoomedPng.isFile());
        assertTrue(zoomedPng.length() > 4096);

        // 2. android_zoomed_stack_underlay_v2.png (800 x 800)
        OctiBoard boardStack = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_3);
        boardStack.getGame().start(OctiPlayer.PLAYER_A);
        OctiSquare sq = boardStack.squareAt(2, 2);
        sq.clear();

        OctiPod underPod = new OctiPod(OctiPodType.A_POD);
        underPod.addProng(OctiDir.DIR_E, false);
        underPod.addProng(OctiDir.DIR_W, false);

        OctiPod topPod = new OctiPod(OctiPodType.A_POD);
        topPod.addProng(OctiDir.DIR_N, false);
        topPod.addProng(OctiDir.DIR_S, true);

        sq.addPod(underPod);
        sq.addPod(topPod);

        int[] stackPixels = OctiBoardRenderer.renderZoomedPodPixels(
                boardStack, 3, 3, 1, 0, OctiThemeId.OBSIDIAN_BRASS, 800, -1);
        File stackPng = new File(artifactDir, "android_zoomed_stack_underlay_v2.png");
        OctiAppIconRenderer.writeArgbPng(800, 800, stackPixels, stackPng);
        assertTrue(stackPng.isFile());
        assertTrue(stackPng.length() > 4096);

        // 3. android_board_staged_prong_preview_v2.png (1080 x 1080)
        OctiBoard boardPreview = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_3);
        boardPreview.getGame().start(OctiPlayer.PLAYER_A);
        OctiMoveComposer composer = new OctiMoveComposer(boardPreview);
        composer.stageOrCommitAddProng(3, 3, 0, OctiDir.DIR_N, false);
        assertTrue(composer.isAwaitingConfirmation());

        OctiBoard activePreviewBoard = composer.buildPreviewBoard();
        OctiBoardGeometry geoBoard = new OctiBoardGeometry(9, 9, 1080.0, 1080.0,
                OctiBoardGeometry.BoardRotation.ROT_0);
        int[] boardPreviewPixels = OctiBoardRenderer.renderBoardPixels(
                activePreviewBoard, composer, geoBoard, OctiThemeId.OBSIDIAN_BRASS);
        File boardPreviewPng = new File(artifactDir, "android_board_staged_prong_preview_v2.png");
        OctiAppIconRenderer.writeArgbPng(1080, 1080, boardPreviewPixels, boardPreviewPng);
        assertTrue(boardPreviewPng.isFile());
        assertTrue(boardPreviewPng.length() > 4096);

        // Validate PNG magic bytes across all 3 artifacts
        for (File f : new File[]{zoomedPng, stackPng, boardPreviewPng}) {
            byte[] bytes = Files.readAllBytes(f.toPath());
            assertEquals((byte) 0x89, bytes[0]);
            assertEquals((byte) 0x50, bytes[1]);
            assertEquals((byte) 0x4E, bytes[2]);
            assertEquals((byte) 0x47, bytes[3]);
        }
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
