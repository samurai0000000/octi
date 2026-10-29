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

    @Test
    public void testEnemySquarePodPlacementPrioritizesLiberationOverReserve() {
        // 1. 2P mode: placing an unadorned pod ("57") on an occupied enemy OCTI square
        // while holding both capturedPods > 0 and freePods > 0 must consume capturedPods first.
        OctiBoard board2p = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_3);
        OctiGame game2p = board2p.getGame();
        game2p.start(OctiPlayer.PLAYER_A);

        board2p.squareAt(4, 6).clear(); // (5, 7) is a B_HOME_SQUARE (enemy base for Player A)
        board2p.squareAt(4, 6).addPod(new OctiPod(OctiPodType.A_POD));
        OctiGameSnapshot snap2p = game2p.captureSnapshot();
        snap2p.players[OctiPlayer.PLAYER_A.getValue()].freePods = 3;
        snap2p.players[OctiPlayer.PLAYER_A.getValue()].capturedPods = 2;
        game2p.restoreSnapshot(snap2p);

        game2p.move("57");
        assertEquals(3, game2p.getFreePods(OctiPlayer.PLAYER_A));
        assertEquals(1, game2p.getCapturedPods(OctiPlayer.PLAYER_A));
        assertEquals(2, board2p.squareAt(4, 6).getNumPods());

        game2p.move("37+d"); // Pass B's turn

        // Next placement on enemy base "57" consumes the last captured pod (1 -> 0), freePods stays 3
        game2p.move("57");
        assertEquals(3, game2p.getFreePods(OctiPlayer.PLAYER_A));
        assertEquals(0, game2p.getCapturedPods(OctiPlayer.PLAYER_A));
        assertEquals(3, board2p.squareAt(4, 6).getNumPods());

        game2p.move("77+d"); // Pass B's turn

        // With capturedPods == 0, unadorned "57" falls back to consuming freePods (3 -> 2)
        game2p.move("57");
        assertEquals(2, game2p.getFreePods(OctiPlayer.PLAYER_A));
        assertEquals(0, game2p.getCapturedPods(OctiPlayer.PLAYER_A));
        assertEquals(4, board2p.squareAt(4, 6).getNumPods());

        // 2. 4P mode: placing an unadorned pod on a non-home OCTI square consumes capturedPods before freePods
        OctiBoard board4p = new OctiBoard(OctiGameMode.GAME_MODE_4, false, false,
                OctiGameBases.GAME_BASES_3);
        OctiGame game4p = board4p.getGame();
        game4p.start(OctiPlayer.PLAYER_A);

        board4p.squareAt(4, 6).clear();
        board4p.squareAt(4, 6).addPod(new OctiPod(OctiPodType.A_POD));
        OctiGameSnapshot snap4p = game4p.captureSnapshot();
        snap4p.players[OctiPlayer.PLAYER_A.getValue()].freePods = 2;
        snap4p.players[OctiPlayer.PLAYER_A.getValue()].capturedPods = 1;
        game4p.restoreSnapshot(snap4p);

        game4p.move("57");
        assertEquals(2, game4p.getFreePods(OctiPlayer.PLAYER_A));
        assertEquals(0, game4p.getCapturedPods(OctiPlayer.PLAYER_A));
        assertEquals(2, board4p.squareAt(4, 6).getNumPods());

        // 3. MoveComposer auto-promotes non-liberate addPod on enemy square to "57*" when capturedPods > 0
        OctiBoard compBoard = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_3);
        compBoard.getGame().start(OctiPlayer.PLAYER_A);
        compBoard.squareAt(4, 6).clear();
        compBoard.squareAt(4, 6).addPod(new OctiPod(OctiPodType.A_POD));
        OctiGameSnapshot snapComp = compBoard.getGame().captureSnapshot();
        snapComp.players[OctiPlayer.PLAYER_A.getValue()].freePods = 3;
        snapComp.players[OctiPlayer.PLAYER_A.getValue()].capturedPods = 2;
        compBoard.getGame().restoreSnapshot(snapComp);

        OctiMoveComposer composer = new OctiMoveComposer(compBoard);
        composer.setConfirmAllMoves(true);
        assertTrue(composer.stageOrCommitAddPod(5, 7, false));
        assertEquals("57*", composer.stagedMoveNotation());
        assertTrue(composer.commitStagedMove());
        assertEquals(3, compBoard.getGame().getFreePods(OctiPlayer.PLAYER_A));
        assertEquals(1, compBoard.getGame().getCapturedPods(OctiPlayer.PLAYER_A));

        compBoard.getGame().move("37+d");

        // When freePods == 0 and capturedPods > 0, legalPodDropSquares(false) and beginAddPodFromReserve()
        // still allow dropping onto the occupied enemy square (5, 7)
        OctiGameSnapshot snapZeroFree = compBoard.getGame().captureSnapshot();
        snapZeroFree.players[OctiPlayer.PLAYER_A.getValue()].freePods = 0;
        snapZeroFree.players[OctiPlayer.PLAYER_A.getValue()].capturedPods = 1;
        compBoard.getGame().restoreSnapshot(snapZeroFree);

        assertTrue(composer.beginAddPodFromReserve());
        java.util.List<int[]> drops = composer.legalPodDropSquares(false);
        boolean has57 = false;
        for (int[] sq : drops) {
            if (sq[0] == 5 && sq[1] == 7) {
                has57 = true;
                break;
            }
        }
        assertTrue("legalPodDropSquares(false) must include (5, 7) when freePods == 0 and capturedPods > 0", has57);
        assertTrue(composer.dropOnSquare(5, 7));
        assertEquals("57*", composer.stagedMoveNotation());
        assertTrue(composer.commitStagedMove());
        assertEquals(0, compBoard.getGame().getFreePods(OctiPlayer.PLAYER_A));
        assertEquals(0, compBoard.getGame().getCapturedPods(OctiPlayer.PLAYER_A));

        // 4. Replay the 72-move game sequence verifying move 57 ("57") liberates A's captured pod
        // so A still has 2 reserve pods remaining after move 57 (having used 2 earlier at moves 13 and 41)
        OctiBoard replayBoard = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_3);
        OctiGame replayGame = replayBoard.getGame();
        replayGame.start(OctiPlayer.PLAYER_A);
        String[] moves72 = {
            "33+b", "37+d", "53+h", "37d+e", "33b-44", "37de+f", "44b+h", "37def-46",
            "44bh+a", "57+d", "53h+b", "57d+e", "33", "57de+f", "33+h", "77+d",
            "33h+b", "46def-55", "44abh-66x-88x", "37", "88abh+f", "37", "88abfh-77", "37",
            "77abfh+g", "37", "77abfgh-67", "57def-66", "67abfgh+e", "66def-75",
            "67abefgh-57", "37+d", "57abefgh-47", "37d+c", "47abefgh-27x", "75def+a",
            "27abefgh+c", "75adef+b", "27abcefgh-37", "75abdef+c", "37", "75abcdef+g",
            "37abcefgh-47", "75abcdefg+h", "47abcefgh-57", "75abcdefgh-66", "57abcefgh-47",
            "66abcdefgh-56", "47abcefgh-38", "56abcdefgh-65", "38abcefgh+d", "65abcdefgh-74",
            "38abcdefgh-47", "74abcdefgh-72x", "47abcdefgh-57", "72abcdefgh-73", "57",
            "73", "57abcdefgh-67", "73*", "57+a", "73*", "57a+b", "73*",
            "57", "73*", "57", "73*", "57+c", "73+a", "57c+d", "73a+b"
        };
        assertEquals(72, moves72.length);
        for (int i = 0; i < moves72.length; i++) {
            replayGame.move(moves72[i]);
            if (i == 56) { // After move 57 ("57") by Player A
                assertEquals(2, replayGame.getFreePods(OctiPlayer.PLAYER_A));
                assertEquals(0, replayGame.getCapturedPods(OctiPlayer.PLAYER_A));
            }
            if (i == 57) { // After move 58 ("73") by Player B
                assertEquals(0, replayGame.getFreePods(OctiPlayer.PLAYER_B));
                assertEquals(5, replayGame.getCapturedPods(OctiPlayer.PLAYER_B));
            }
        }
        assertEquals(0, replayGame.getFreePods(OctiPlayer.PLAYER_A));
        assertEquals(0, replayGame.getCapturedPods(OctiPlayer.PLAYER_A));
        assertEquals(0, replayGame.getFreePods(OctiPlayer.PLAYER_B));
        assertEquals(0, replayGame.getCapturedPods(OctiPlayer.PLAYER_B));
    }

    @Test
    public void testExtractModifiedSquaresAndClockwiseOrangeBorderRendering() {
        // 1. Prong placement extracts (3, 3)
        java.util.List<int[]> modProng = OctiBoardRenderer.extractModifiedSquares(
                new OctiMove("33+a"), false);
        assertEquals(1, modProng.size());
        assertEquals(3, modProng.get(0)[0]);
        assertEquals(3, modProng.get(0)[1]);

        // 2. Multi-hop jump extracts origin, jumped-over squares, and landing squares
        java.util.List<int[]> modJump = OctiBoardRenderer.extractModifiedSquares(
                new OctiMove("33ac-35x-55x"), false);
        assertEquals(5, modJump.size()); // (3,3), (3,4), (3,5), (4,5), (5,5)
        assertEquals(3, modJump.get(0)[0]);
        assertEquals(3, modJump.get(0)[1]);
        assertEquals(3, modJump.get(1)[0]);
        assertEquals(4, modJump.get(1)[1]);
        assertEquals(3, modJump.get(2)[0]);
        assertEquals(5, modJump.get(2)[1]);
        assertEquals(4, modJump.get(3)[0]);
        assertEquals(5, modJump.get(3)[1]);
        assertEquals(5, modJump.get(4)[0]);
        assertEquals(5, modJump.get(4)[1]);

        // 3. Edgeless wrap jump extracts wrapped intermediate square (1, 5)
        java.util.List<int[]> modWrap = OctiBoardRenderer.extractModifiedSquares(
                new OctiMove("95c-25x"), true);
        assertEquals(3, modWrap.size()); // (9,5), (1,5), (2,5)
        assertEquals(1, modWrap.get(1)[0]);
        assertEquals(5, modWrap.get(1)[1]);

        // 4. Verify OctiBoardRenderer.paintBoard renders orange (0xFFFF8C00) border pixels
        // along the perimeter of lastMoveModifiedSquares across phases 0.0f, 0.25f, 0.5f, 0.75f
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_3);
        board.getGame().start(OctiPlayer.PLAYER_A);
        OctiBoardGeometry geo = new OctiBoardGeometry(720, 720, 9, 9,
                OctiBoardGeometry.BoardRotation.ROT_0);

        int[] basePixels = OctiBoardRenderer.paintBoard(
                board, null, geo, OctiThemeId.OBSIDIAN_BRASS, null);
        double[] sqRect = geo.squareRect(5, 5);
        int x0 = Math.max(0, (int) Math.floor(sqRect[0]));
        int x1 = Math.min(719, (int) Math.ceil(sqRect[0] + sqRect[2]) - 1);
        int y0 = Math.max(0, (int) Math.floor(sqRect[1]));
        int y1 = Math.min(719, (int) Math.ceil(sqRect[1] + sqRect[3]) - 1);

        for (float phase : new float[]{0.0f, 0.25f, 0.5f, 0.75f}) {
            OctiBoardRenderer.RenderConfig cfg = new OctiBoardRenderer.RenderConfig();
            cfg.lastMoveModifiedSquares.add(new int[]{5, 5});
            cfg.lastMoveAnimPhase = phase;
            int[] animPixels = OctiBoardRenderer.paintBoard(
                    board, null, geo, OctiThemeId.OBSIDIAN_BRASS, cfg);

            int orangePixelCount = 0;
            for (int y = y0; y <= y1; y++) {
                for (int x = x0; x <= x1; x++) {
                    int idx = y * 720 + x;
                    if (animPixels[idx] != basePixels[idx]
                            && animPixels[idx] == OctiBoardRenderer.LAST_MOVE_INDICATOR_COLOR) {
                        orangePixelCount++;
                    }
                }
            }
            assertTrue("Phase " + phase + " must render orange border pixels on modified square",
                    orangePixelCount > 20);
        }

        // 5. Verify ViewModel populates lastMoveModifiedSquares on move commit and clears on UI action
        com.selfso.octi.ui.OctiAppViewModel vm = new com.selfso.octi.ui.OctiAppViewModel();
        vm.startNewLocalGame(OctiGameMode.GAME_MODE_2, false, false,
                OctiGameBases.GAME_BASES_3, false,
                OctiBotEngine.BotDifficulty.CASUAL, OctiPlayer.PLAYER_A);
        assertTrue(vm.getLastMoveModifiedSquaresList().isEmpty());
        assertTrue(vm.applyMoveNotation("33+a"));
        assertFalse(vm.getLastMoveModifiedSquaresList().isEmpty());
        assertEquals(3, vm.getLastMoveModifiedSquaresList().get(0)[0]);
        assertEquals(3, vm.getLastMoveModifiedSquaresList().get(0)[1]);
        vm.clearLastMoveIndicators();
        assertTrue(vm.getLastMoveModifiedSquaresList().isEmpty());
        vm.close();
    }

    @Test
    public void testPaintPlayerReserveCardOmitsCapturedPodsWhenZero() {
        int w = 520;
        int h = 100;
        OctiPalette pal = OctiPalette.forTheme(OctiThemeId.OBSIDIAN_BRASS);

        int[] pixelsZeroCap = new int[w * h];
        OctiBoardRenderer.paintPlayerReserveCard(
                pixelsZeroCap, w, h, 10, 10, 496, 76,
                OctiPlayer.PLAYER_A, true,
                4, 0, 25, 0, false, pal);

        int[] pixelsWithCap = new int[w * h];
        OctiBoardRenderer.paintPlayerReserveCard(
                pixelsWithCap, w, h, 10, 10, 496, 76,
                OctiPlayer.PLAYER_A, true,
                4, 2, 25, 0, false, pal);

        // In the right half of the card where captured pods are drawn (x >= 160),
        // pixelsZeroCap must contain only background / border pixels and zero legalCaptureGlow pixels
        int glowZero = 0;
        int glowNonZero = 0;
        int diffPixels = 0;
        for (int y = 20; y < 80; y++) {
            for (int x = 140; x < 480; x++) {
                int idx = y * w + x;
                if (pixelsZeroCap[idx] == pal.legalCaptureGlow) {
                    glowZero++;
                }
                if (pixelsWithCap[idx] == pal.legalCaptureGlow) {
                    glowNonZero++;
                }
                if (pixelsZeroCap[idx] != pixelsWithCap[idx]) {
                    diffPixels++;
                }
            }
        }
        assertEquals("When capPods == 0, zero captured pod glow pixels may be drawn", 0, glowZero);
        assertTrue("When capPods > 0, captured pod glow pixels must be drawn", glowNonZero > 10);
        assertTrue("Card pixels must differ when capPods > 0 vs capPods == 0", diffPixels > 50);
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
