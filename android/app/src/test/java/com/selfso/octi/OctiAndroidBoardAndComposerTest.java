/*
 * OctiAndroidBoardAndComposerTest.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Random;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Envelope 1 JUnit 4 + 500-trial Seeded Property-Based Qualification Suite covering:
 * - Hard-coded endpoint constants in {@link OctiDefs}
 * - Luxury Adaptive Launcher Icon XML drawables and {@link OctiAppIconRenderer} (512x512 PNG export)
 * - Dual luxury themes ({@link OctiThemeId}, {@link OctiPalette})
 * - 4-Quadrant Board Geometry ({@link OctiBoardGeometry})
 * - Smart Auto-Commit & Confirm-All Touch Move Composer ({@link OctiMoveComposer})
 * - 2D Tabletop Board Renderer ({@link OctiBoardRenderer})
 * - On-Device Alpha-Beta AI Bot ({@link OctiBotEngine})
 * - Procedural 16 kHz PCM Sound Synthesizer ({@link OctiSoundSynth})
 */
public class OctiAndroidBoardAndComposerTest {

    private static File resolveRepoRoot() {
        File cwd = new File(System.getProperty("user.dir", ".")).getAbsoluteFile();
        File cur = cwd;
        while (cur != null) {
            if (new File(cur, "plan/plan_octi_android.md").exists()
                    || (new File(cur, "android").isDirectory() && new File(cur, "core").isDirectory())) {
                return cur;
            }
            cur = cur.getParentFile();
        }
        return cwd;
    }

    private static OctiBoard createStartedBoard(OctiGameMode mode,
                                                boolean superProngs,
                                                boolean edgeless,
                                                OctiGameBases bases) {
        OctiBoard board = new OctiBoard(mode, superProngs, edgeless, bases);
        board.getGame().start(OctiPlayer.PLAYER_A);
        return board;
    }

    private static OctiBoard createStartedBoard(OctiGameMode mode) {
        OctiGameBases bases = (mode == OctiGameMode.GAME_MODE_OFK)
                ? OctiGameBases.GAME_BASES_1
                : OctiGameBases.GAME_BASES_3;
        return createStartedBoard(mode, false, false, bases);
    }

    private static List<String> legalMoveStrings(OctiBoard board) {
        List<String> list = new ArrayList<>();
        if (board != null && board.getGame() != null
                && board.getGame().isGameStarted()
                && !board.getGame().isGameOver()) {
            List<OctiMove> all = board.getGame().generateLegalMoves();
            OctiBotEngine.appendCompositeMoves(board, board.getGame().getTurn(), all);
            for (OctiMove mv : all) {
                list.add(mv.asString());
            }
        }
        return list;
    }

    // -------------------------------------------------------------------------
    // 1. Canonical Endpoint Constants & Luxury Adaptive Icon Verification
    // -------------------------------------------------------------------------

    @Test
    public void testCanonicalServerHostAndPortConstants() {
        assertEquals("octi.selfso.com", OctiDefs.OCTI_SERVER_HOST);
        assertEquals(443, OctiDefs.OCTI_SERVER_PORT);
        assertEquals("trumpet.selfso.com", OctiDefs.OCTI_SERVER_FALLBACK_HOST);
        assertEquals(6874, OctiDefs.OCTI_SERVER_FALLBACK_PORT);
        assertEquals(6874, OctiDefs.OCTI_SERVER_DAEMON_PORT);
    }

    @Test
    public void testAdaptiveLauncherIconXmlDrawablesAnd512PngExport() throws Exception {
        File repoRoot = resolveRepoRoot();
        File resDir = new File(repoRoot, "android/app/src/main/res");

        File bgXml = new File(resDir, "drawable/ic_launcher_background.xml");
        File fgXml = new File(resDir, "drawable/ic_launcher_foreground.xml");
        File monoXml = new File(resDir, "drawable/ic_launcher_monochrome.xml");
        File mipmapXml = new File(resDir, "mipmap-anydpi-v26/ic_launcher.xml");
        File mipmapRoundXml = new File(resDir, "mipmap-anydpi-v26/ic_launcher_round.xml");

        assertTrue("ic_launcher_background.xml must exist", bgXml.isFile());
        assertTrue("ic_launcher_foreground.xml must exist", fgXml.isFile());
        assertTrue("ic_launcher_monochrome.xml must exist", monoXml.isFile());
        assertTrue("ic_launcher.xml must exist", mipmapXml.isFile());
        assertTrue("ic_launcher_round.xml must exist", mipmapRoundXml.isFile());

        String bgContent = new String(Files.readAllBytes(bgXml.toPath()), StandardCharsets.UTF_8);
        String fgContent = new String(Files.readAllBytes(fgXml.toPath()), StandardCharsets.UTF_8);
        String monoContent = new String(Files.readAllBytes(monoXml.toPath()), StandardCharsets.UTF_8);
        String mipmapContent = new String(Files.readAllBytes(mipmapXml.toPath()), StandardCharsets.UTF_8);

        assertTrue(bgContent.contains("C8A253"));
        assertTrue(fgContent.contains("10B981"));
        assertTrue(fgContent.contains("FDE047"));
        assertTrue(monoContent.contains("<vector"));
        assertTrue(mipmapContent.contains("@drawable/ic_launcher_monochrome"));

        int[] pixels = OctiAppIconRenderer.renderPixels(512);
        assertEquals(512 * 512, pixels.length);

        Set<Integer> distinctColors = new HashSet<>();
        for (int px : pixels) {
            distinctColors.add(px);
        }
        for (int y = 128; y < 384; y++) {
            for (int x = 128; x < 384; x++) {
                assertEquals("Interior icon pixel must be fully opaque",
                        0xFF000000, pixels[y * 512 + x] & 0xFF000000);
            }
        }
        assertTrue("Rendered 512x512 icon must have rich multi-layer shading (>500 colors)",
                distinctColors.size() > 500);

        File artifactDir = new File(repoRoot, "plan/done/plan_octi_android.md").exists()
                ? new File(repoRoot, "plan/done/plan_octi_android.artifacts")
                : new File(repoRoot, "plan/plan_octi_android.artifacts");
        File artifactPng = new File(artifactDir, "octi_app_icon_512.png");
        OctiAppIconRenderer.writePng(512, artifactPng);
        assertTrue("Exported PNG must exist", artifactPng.isFile());
        assertTrue("Exported PNG must be non-empty (>4 KB)", artifactPng.length() > 4096);

        byte[] pngBytes = Files.readAllBytes(artifactPng.toPath());
        assertEquals((byte) 0x89, pngBytes[0]);
        assertEquals((byte) 0x50, pngBytes[1]);
        assertEquals((byte) 0x4E, pngBytes[2]);
        assertEquals((byte) 0x47, pngBytes[3]);
    }

    // -------------------------------------------------------------------------
    // 2. Dual Luxury Theme Palettes (Obsidian & Brass + Rosewood & Ivory)
    // -------------------------------------------------------------------------

    @Test
    public void testDualLuxuryThemePalettesMatchDesktopSpec() {
        OctiPalette obsidian = OctiPalette.forTheme(OctiThemeId.OBSIDIAN_BRASS);
        assertEquals(OctiThemeId.OBSIDIAN_BRASS, obsidian.themeId);
        assertEquals(0xFF0B0F17, obsidian.windowBg);
        assertEquals(0xFF141B29, obsidian.panelCardBg);
        assertEquals(0xFFB49148, obsidian.panelCardBorder);
        assertEquals(0xFFC8A253, obsidian.accentBrass);
        assertEquals(0xFF151E30, obsidian.gridCellDark);
        assertEquals(0xFF1D2940, obsidian.gridCellLight);
        assertEquals(0xFF10B981, obsidian.podFill(OctiPlayer.PLAYER_A));
        assertEquals(0xFFF43F5E, obsidian.podFill(OctiPlayer.PLAYER_B));
        assertEquals(0xFF3B82F6, obsidian.podFill(OctiPlayer.PLAYER_C));
        assertEquals(0xFFF59E0B, obsidian.podFill(OctiPlayer.PLAYER_D));

        OctiPalette rosewood = OctiPalette.forTheme(OctiThemeId.ROSEWOOD_IVORY);
        assertEquals(OctiThemeId.ROSEWOOD_IVORY, rosewood.themeId);
        assertEquals(0xFF2A1812, rosewood.windowBg);
        assertEquals(0xFF3E241B, rosewood.panelCardBg);
        assertEquals(0xFFCEA660, rosewood.panelCardBorder);
        assertEquals(0xFF623626, rosewood.gridCellDark);
        assertEquals(0xFF7C4832, rosewood.gridCellLight);
        assertEquals(0xFFEEE4CC, rosewood.podRim(OctiPlayer.PLAYER_A));
        assertEquals(0xFFEEE4CC, rosewood.podRim(OctiPlayer.PLAYER_B));
    }

    // -------------------------------------------------------------------------
    // 3. 4-Quadrant Board Geometry, Seat Auto-Rotation & Compass Hit-Testing
    // -------------------------------------------------------------------------

    @Test
    public void testSeatAutoRotationAndClockwiseCycling() {
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_0,
                OctiBoardGeometry.rotationForSeat(OctiGameMode.GAME_MODE_2, OctiPlayer.PLAYER_A));
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_180,
                OctiBoardGeometry.rotationForSeat(OctiGameMode.GAME_MODE_2, OctiPlayer.PLAYER_B));

        assertEquals(OctiBoardGeometry.BoardRotation.ROT_0,
                OctiBoardGeometry.rotationForSeat(OctiGameMode.GAME_MODE_OFK, OctiPlayer.PLAYER_A));
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_180,
                OctiBoardGeometry.rotationForSeat(OctiGameMode.GAME_MODE_OFK, OctiPlayer.PLAYER_B));

        assertEquals(OctiBoardGeometry.BoardRotation.ROT_0,
                OctiBoardGeometry.rotationForSeat(OctiGameMode.GAME_MODE_4, OctiPlayer.PLAYER_A));
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_90,
                OctiBoardGeometry.rotationForSeat(OctiGameMode.GAME_MODE_4, OctiPlayer.PLAYER_B));
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_180,
                OctiBoardGeometry.rotationForSeat(OctiGameMode.GAME_MODE_4, OctiPlayer.PLAYER_C));
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_270,
                OctiBoardGeometry.rotationForSeat(OctiGameMode.GAME_MODE_4, OctiPlayer.PLAYER_D));

        OctiBoardGeometry.BoardRotation r = OctiBoardGeometry.BoardRotation.ROT_0;
        r = OctiBoardGeometry.nextRotationClockwise(r);
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_90, r);
        r = OctiBoardGeometry.nextRotationClockwise(r);
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_180, r);
        r = OctiBoardGeometry.nextRotationClockwise(r);
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_270, r);
        r = OctiBoardGeometry.nextRotationClockwise(r);
        assertEquals(OctiBoardGeometry.BoardRotation.ROT_0, r);
    }

    @Test
    public void testCoordinateRoundTripAndOrientationAcrossAllRotations() {
        int[][] grids = {{9, 9}, {6, 7}};
        for (int[] grid : grids) {
            int gw = grid[0];
            int gh = grid[1];
            for (OctiBoardGeometry.BoardRotation rot : OctiBoardGeometry.BoardRotation.values()) {
                OctiBoardGeometry geom = new OctiBoardGeometry(gw, gh, 900.0, 900.0, rot);
                int[] outXY = new int[2];
                for (int x = 1; x <= gw; x++) {
                    for (int y = 1; y <= gh; y++) {
                        double[] center = geom.squareCenter(x, y);
                        assertTrue(geom.screenToSquare(center[0], center[1], outXY));
                        assertEquals("X mismatch under " + rot, x, outXY[0]);
                        assertEquals("Y mismatch under " + rot, y, outXY[1]);
                    }
                }
            }
        }

        // Verify Seat A home (3,3) is near bottom in ROT_0 and near top in ROT_180
        OctiBoardGeometry geom0 = new OctiBoardGeometry(9, 9, 800.0, 800.0,
                OctiBoardGeometry.BoardRotation.ROT_0);
        OctiBoardGeometry geom180 = new OctiBoardGeometry(9, 9, 800.0, 800.0,
                OctiBoardGeometry.BoardRotation.ROT_180);
        assertTrue(geom0.squareCenter(3, 3)[1] > geom0.squareCenter(3, 7)[1]);
        assertTrue(geom180.squareCenter(3, 7)[1] > geom180.squareCenter(3, 3)[1]);
    }

    @Test
    public void testCompassSectorHitTestingAcrossAllRotations() {
        OctiDir[] dirs = {
            OctiDir.DIR_N, OctiDir.DIR_NE, OctiDir.DIR_E, OctiDir.DIR_SE,
            OctiDir.DIR_S, OctiDir.DIR_SW, OctiDir.DIR_W, OctiDir.DIR_NW
        };
        for (OctiBoardGeometry.BoardRotation rot : OctiBoardGeometry.BoardRotation.values()) {
            OctiBoardGeometry geom = new OctiBoardGeometry(9, 9, 800.0, 800.0, rot);
            double cx = 400.0;
            double cy = 400.0;
            double innerR = 16.0;
            double outerR = 64.0;
            double sampleR = 40.0;

            for (OctiDir dir : dirs) {
                double[] uv = geom.dirUnitVectorScreen(dir);
                double tx = cx + uv[0] * sampleR;
                double ty = cy + uv[1] * sampleR;
                OctiDir hit = geom.hitTestCompassSector(cx, cy, innerR, outerR, tx, ty);
                assertEquals("Direction hit-test mismatch for " + dir + " at " + rot, dir, hit);
            }

            // Inside inner dead-zone or outside outer radius returns DIR_UNDEF
            assertEquals(OctiDir.DIR_UNDEF, geom.hitTestCompassSector(cx, cy, innerR, outerR, cx + 4.0, cy));
            assertEquals(OctiDir.DIR_UNDEF, geom.hitTestCompassSector(cx, cy, innerR, outerR, cx + 90.0, cy));
        }
    }

    @Test
    public void testStackFanBoundary1To7PodsHitTesting() {
        OctiBoardGeometry geom = new OctiBoardGeometry(9, 9, 900.0, 900.0,
                OctiBoardGeometry.BoardRotation.ROT_0);
        int[][] testSquares = {{1, 1}, {5, 5}, {9, 9}};
        for (int[] sq : testSquares) {
            for (int numPods = 1; numPods <= 7; numPods++) {
                for (int slot = 0; slot < numPods; slot++) {
                    double[] rect = geom.stackFanSlotRect(sq[0], sq[1], numPods, slot);
                    assertNotNull(rect);
                    double midX = rect[0] + rect[2] * 0.5;
                    double midY = rect[1] + rect[3] * 0.5;
                    int hitSlot = geom.hitTestStackFanSlot(sq[0], sq[1], numPods, midX, midY);
                    assertEquals("Failed Stack Fan slot hit for numPods=" + numPods + " slot=" + slot,
                            slot, hitSlot);
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // 4. Smart Auto-Commit & Confirm-All Touch Move Composer
    // -------------------------------------------------------------------------

    @Test
    public void testComposerSmartAutoCommitVsConfirmAllForPodAndProngOps() {
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        AtomicReference<String> lastCommitted = new AtomicReference<>();
        OctiMoveComposer composer = new OctiMoveComposer(board);
        composer.setMoveCommitListener(lastCommitted::set);

        // 1. Confirm-All (default confirmAllMoves = true per GUI-02): Adding a prong to (3,3) pauses for confirmation
        assertTrue(composer.isConfirmAllMoves());
        assertTrue(composer.beginAddProngFromReserve(false));
        assertTrue(composer.dropOnCompassSector(3, 3, 0, OctiDir.DIR_N));
        assertNull("Confirm-All must pause before committing", lastCommitted.get());
        assertTrue(composer.shouldPauseForConfirmation());
        assertEquals("33+a", composer.getStagedMoveNotation());
        assertTrue(composer.commitMove());
        assertEquals("33+a", lastCommitted.get());
        assertEquals(OctiPlayer.PLAYER_B, game.getTurn());

        // 2. Smart Auto-Commit mode (confirmAllMoves = false): Adding a pod for Player B on (5,7) auto-commits immediately
        lastCommitted.set(null);
        composer.setConfirmAllMoves(false);
        assertTrue(composer.beginAddPodFromReserve());
        assertTrue(composer.dropOnSquare(5, 7));
        assertEquals("57", lastCommitted.get());
        assertEquals(OctiPlayer.PLAYER_A, game.getTurn());

        // 3. Reposition prong on (3,3) from N (a) to NE (b) under Smart Auto-Commit
        composer.setConfirmAllMoves(false);
        lastCommitted.set(null);
        assertTrue(composer.beginRepositionProng(3, 3, 0, OctiDir.DIR_N));
        assertTrue(composer.dropOnCompassSector(3, 3, 0, OctiDir.DIR_NE));
        assertTrue(lastCommitted.get().contains("+b") && lastCommitted.get().contains("-a"));
        assertEquals(OctiPlayer.PLAYER_B, game.getTurn());

        // 4. Single step by Player A after Player B adds a prong
        composer.setConfirmAllMoves(false);
        assertTrue(composer.beginAddProngFromReserve(false));
        assertTrue(composer.dropOnCompassSector(3, 7, 0, OctiDir.DIR_S));
        assertEquals("37+e", lastCommitted.get());
        assertEquals(OctiPlayer.PLAYER_A, game.getTurn());

        // Player A steps (3,3) -> (4,4) (NE)
        lastCommitted.set(null);
        assertTrue(composer.beginMoveOrJumpPod(3, 3, 0, false));
        assertTrue(composer.dropOnSquare(4, 4));
        assertTrue(lastCommitted.get().endsWith("-44"));
    }

    @Test
    public void testComposerMultiHopJumpAndSelectiveCaptureToggle() {
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.move("33+a");
        game.move("37+e");
        game.move("53+c");
        game.move("37e-36");
        game.move("53c+b");
        game.move("36e-35");
        game.move("53bc+d");
        game.move("35e-34");

        // Now A is at (3,3) with N prong, B is at (3,4). Jump (3,3) -> (3,5) crosses occupied (3,4)!
        AtomicReference<String> committed = new AtomicReference<>();
        OctiMoveComposer composer = new OctiMoveComposer(board);
        composer.setConfirmAllMoves(false);
        composer.setMoveCommitListener(committed::set);

        assertTrue(composer.beginMoveOrJumpPod(3, 3, 0, false));
        assertTrue(composer.dropOnSquare(3, 5));

        // Because the jump crossed an occupied square where capture is optional in 2P,
        // Smart Auto-Commit MUST pause so the player can toggle capture vs non-capture!
        assertNull("Jump over occupied square must pause for capture confirmation", committed.get());
        assertTrue(composer.shouldPauseForConfirmation());
        assertTrue(composer.getStagedMoveNotation().endsWith("-35x"));

        // Toggle capture off -> no trailing 'x'
        assertTrue(composer.toggleHopCapture(0));
        assertTrue(composer.getStagedMoveNotation().endsWith("-35"));

        // Toggle capture back on -> trailing 'x'
        assertTrue(composer.toggleCaptureAtSquare(3, 4));
        assertTrue(composer.getStagedMoveNotation().endsWith("-35x"));

        assertTrue(composer.commitMove());
        assertNotNull(committed.get());
        assertTrue(committed.get().endsWith("-35x"));
        assertEquals(1, game.getCapturedPods(OctiPlayer.PLAYER_B));
    }

    @Test
    public void testComposerLiberatePodFromCapturedPool() {
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();

        // Set up Player A occupying Player B's home square (3,7) with 1 captured pod in Player A's captured pool
        board.squareAt(2, 6).clear();
        board.squareAt(2, 6).addPod(new OctiPod(OctiPodType.A_POD));
        OctiGameSnapshot snap = game.captureSnapshot();
        snap.players[OctiPlayer.PLAYER_A.getValue()].capturedPods = 1;
        game.restoreSnapshot(snap);

        assertEquals(OctiPlayer.PLAYER_A, game.getTurn());
        assertEquals(1, game.getCapturedPods(OctiPlayer.PLAYER_A));
        assertTrue(board.canLiberate(OctiPlayer.PLAYER_A));

        OctiMoveComposer composer = new OctiMoveComposer(board);
        List<int[]> libSquares = composer.legalPodDropSquares(true);
        assertFalse("Must have legal liberation squares", libSquares.isEmpty());

        AtomicReference<String> committed = new AtomicReference<>();
        composer.setConfirmAllMoves(false);
        composer.setMoveCommitListener(committed::set);
        int[] target = libSquares.get(0);
        assertTrue(composer.beginLiberatePodFromReserve());
        assertTrue(composer.dropOnSquare(target[0], target[1]));
        assertEquals(target[0] + "" + target[1] + "*", committed.get());
        assertEquals(0, game.getCapturedPods(OctiPlayer.PLAYER_A));
    }

    @Test
    public void testComposerStackFanAndCompositeMultiPodMove() {
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.move("33+c"); // A adds E prong at 3,3
        game.move("37+c"); // B adds E prong at 3,7
        game.move("53+a"); // A adds N prong at 5,3
        game.move("57+e"); // B adds S prong at 5,7
        game.move("33c-43"); // A steps 3,3 -> 4,3
        game.move("37c-47"); // B steps 3,7 -> 4,7
        game.move("43c-53"); // A steps 4,3 -> 5,3 (now (5,3) has 2 pods: one with E, one with N)
        game.move("47c-57"); // B steps 4,7 -> 5,7
        game.move("53a+c"); // A adds E prong to the N-pod so both pods on (5,3) have E prong
        game.move("57e+c"); // B adds E prong

        assertEquals(2, board.squareAt(4, 2).getNumPods());

        // Moving a single pod from the 2-pod square pauses because the other pod on (5,3) can join a composite move
        OctiMoveComposer composer = new OctiMoveComposer(board);
        composer.openStackFan(5, 3);
        assertTrue(composer.isStackFanOpen());
        composer.selectStackPodIndex(0);
        assertEquals(0, composer.getSelectedStackPodIndex());

        assertTrue(composer.beginMoveOrJumpPod(5, 3, 0, false));
        assertTrue(composer.dropOnSquare(6, 3));
        assertTrue("Single pod move from multi-pod square with another movable pod must pause for composite option",
                composer.shouldPauseForConfirmation());
        assertTrue(composer.commitMove());
    }

    @Test
    public void testComposerWholeStackMove() {
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.move("33+c");
        game.move("37+c");
        game.move("53+c");
        game.move("57+c");
        game.move("33c-43");
        game.move("37c-47");
        game.move("43c-53"); // Now (5,3) has two identical pods with 'c' (E) prong!
        game.move("47c-57");

        AtomicReference<String> committed = new AtomicReference<>();
        OctiMoveComposer composer = new OctiMoveComposer(board);
        composer.setConfirmAllMoves(false);
        composer.setMoveCommitListener(committed::set);

        assertTrue(composer.beginMoveOrJumpPod(5, 3, 0, true));
        assertTrue(composer.dropOnSquare(6, 3));
        assertNotNull(committed.get());
        assertTrue(committed.get().contains(","));
        assertEquals(2, board.squareAt(5, 2).getNumPods());
    }

    @Test
    public void testComposerSuperProng8thProngBonusMove() {
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_2, true, false, OctiGameBases.GAME_BASES_3);
        OctiGame game = board.getGame();
        AtomicReference<String> committed = new AtomicReference<>();
        OctiMoveComposer composer = new OctiMoveComposer(board);
        composer.setConfirmAllMoves(false);
        composer.setMoveCommitListener(committed::set);

        OctiDir[] firstSevenDirs = {
            OctiDir.DIR_N, OctiDir.DIR_NE, OctiDir.DIR_E, OctiDir.DIR_SE,
            OctiDir.DIR_S, OctiDir.DIR_SW, OctiDir.DIR_W
        };
        for (OctiDir d : firstSevenDirs) {
            assertTrue(composer.beginAddProngFromReserve(false));
            assertTrue(composer.dropOnCompassSector(3, 3, 0, d));
            assertTrue(composer.beginAddProngFromReserve(false));
            assertTrue(composer.dropOnCompassSector(3, 7, 0, d));
        }
        assertEquals(7, board.squareAt(2, 2).getPodAt(0).getNumProngs());

        committed.set(null);

        // Adding the 8th prong as a superprong ('h!') triggers the bonus move state
        assertTrue(composer.beginAddProngFromReserve(true));
        assertTrue(composer.dropOnCompassSector(3, 3, 0, OctiDir.DIR_NW));
        assertTrue("8th prong as superprong must pause for bonus move", composer.isAwaitingSuperProngBonusMove());
        assertTrue(composer.shouldPauseForConfirmation());
        assertNull(committed.get());

        // Perform the bonus move step (3,3) -> (3,4) and commit
        assertTrue(composer.beginMoveOrJumpPod(3, 3, 0, false));
        assertTrue(composer.dropOnSquare(3, 4));
        if (composer.shouldPauseForConfirmation()) {
            assertTrue(composer.commitMove());
        }
        assertNotNull(committed.get());
        assertEquals(OctiPlayer.PLAYER_B, game.getTurn());
        assertEquals(1, board.squareAt(2, 3).getNumPods());
    }

    @Test
    public void testEdgelessWrapAroundStepAndJump() {
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_2, false, true, OctiGameBases.GAME_BASES_3);
        OctiGame game = board.getGame();
        game.move("33+g"); // W prong
        game.move("37+c");
        game.move("33g-23");
        game.move("37c-47");
        game.move("23g-13");
        game.move("47c-57");

        AtomicReference<String> committed = new AtomicReference<>();
        OctiMoveComposer composer = new OctiMoveComposer(board);
        composer.setConfirmAllMoves(false);
        composer.setMoveCommitListener(committed::set);

        assertTrue(composer.beginMoveOrJumpPod(1, 3, 0, false));
        assertTrue(composer.dropOnSquare(9, 3));
        assertNotNull(committed.get());
        assertTrue(committed.get().endsWith("-93"));
        assertEquals(1, board.squareAt(8, 2).getNumPods());
    }

    @Test
    public void testFaultInjectionIllegalDropsAndRollbackLeaveBoardUnchanged() {
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_OFK);
        OctiGame game = board.getGame();
        String initialPos = board.toPositionString();
        OctiMoveComposer composer = new OctiMoveComposer(board);

        // 1. Attempt to drag an enemy pod (Player B pod on (2,6) during Player A's turn)
        composer.beginMoveOrJumpPod(2, 6, 0, false);
        assertFalse(composer.dropOnSquare(2, 5));
        assertEquals(initialPos, board.toPositionString());

        // 2. Attempt to add a backward prong in OFK (Player A cannot add South 'e' prong in OFK)
        assertTrue(composer.beginAddProngFromReserve(false));
        assertFalse(composer.dropOnCompassSector(2, 2, 0, OctiDir.DIR_S));
        composer.rollbackMove();
        assertEquals(initialPos, board.toPositionString());

        // 3. In OFK, reserve pod placement is forbidden; in 2P, dropping a reserve pod on a non-home square (4,4) is rejected
        assertFalse(composer.beginAddPodFromReserve());
        OctiBoard board2p = createStartedBoard(OctiGameMode.GAME_MODE_2);
        String initial2p = board2p.toPositionString();
        OctiMoveComposer comp2p = new OctiMoveComposer(board2p);
        assertTrue(comp2p.beginAddPodFromReserve());
        assertFalse(comp2p.dropOnSquare(4, 4));
        comp2p.rollbackMove();
        assertEquals(initial2p, board2p.toPositionString());

        // 4. Stage a valid move in Confirm-All mode, then rollbackMove()
        composer.setConfirmAllMoves(true);
        assertTrue(composer.beginAddProngFromReserve(false));
        assertTrue(composer.dropOnCompassSector(2, 2, 0, OctiDir.DIR_N));
        assertEquals("22+a", composer.getStagedMoveNotation());
        composer.rollbackMove();
        assertEquals("", composer.getStagedMoveNotation());
        assertEquals(initialPos, board.toPositionString());
        assertEquals(OctiPlayer.PLAYER_A, game.getTurn());
    }

    // -------------------------------------------------------------------------
    // 5. On-Device AI Bot Engine (CASUAL, TACTICAL, MASTER)
    // -------------------------------------------------------------------------

    @Test
    public void testBotEngineFindsImmediateWinningBaseCaptureAndHandlesTerminalBoard() {
        // Construct a position where Player A is 1 jump away from capturing Player B's home base (3,7)
        // in 1-base capture victory mode.
        OctiBoard winBoard = createStartedBoard(OctiGameMode.GAME_MODE_2, false, false, OctiGameBases.GAME_BASES_1);
        OctiGame winGame = winBoard.getGame();
        winGame.move("33+a");
        winGame.move("37+e");
        winGame.move("33a-34");
        winGame.move("37e-36"); // B leaves (3,7) empty and stands on (3,6)
        winGame.move("34a-35");
        winGame.move("57+a");

        // Now it is Player A's turn: A at (3,5) with N prong can jump over B at (3,6) onto B's home base (3,7) for instant win!
        OctiBotEngine bot = new OctiBotEngine(OctiBotEngine.BotDifficulty.TACTICAL);
        bot.setTimeBudgetMs(25);
        OctiBotEngine.BotSearchStats stats = new OctiBotEngine.BotSearchStats();
        String chosen = bot.chooseMove(winBoard, stats);
        assertTrue("Bot must choose winning jump onto (3,7), got: " + chosen,
                chosen.contains("35") && chosen.contains("-37"));
        assertTrue(stats.nodesVisited > 0);

        winGame.move(chosen);
        assertTrue(winGame.isGameOver());
        assertEquals(OctiPlayer.PLAYER_A, winGame.getWinner());

        // On a terminal/finished board, chooseMove must return "" cleanly
        assertEquals("", bot.chooseMove(winBoard, stats));
    }

    @Test
    public void testBotEngineAcrossAllModesVariantsAndDifficulties() {
        OctiGameMode[] modes = {
            OctiGameMode.GAME_MODE_2,
            OctiGameMode.GAME_MODE_4,
            OctiGameMode.GAME_MODE_OFK
        };
        for (OctiGameMode mode : modes) {
            for (OctiBotEngine.BotDifficulty diff : OctiBotEngine.BotDifficulty.values()) {
                OctiGameBases bases = (mode == OctiGameMode.GAME_MODE_OFK)
                        ? OctiGameBases.GAME_BASES_1
                        : OctiGameBases.GAME_BASES_3;
                OctiBoard board = createStartedBoard(mode, mode != OctiGameMode.GAME_MODE_OFK, false, bases);
                OctiBotEngine bot = new OctiBotEngine(diff);
                bot.setTimeBudgetMs(25);
                OctiBotEngine.BotSearchStats stats = new OctiBotEngine.BotSearchStats();
                String snapshotBefore = board.toPositionString();
                String move = bot.chooseMove(board, stats);
                assertEquals("Bot must never mutate caller's board", snapshotBefore, board.toPositionString());
                assertFalse("Bot must return non-empty move on non-terminal board", move.isEmpty());
                List<String> legal = legalMoveStrings(board);
                assertTrue("Bot move '" + move + "' must be in generateLegalMoves()", legal.contains(move));
                board.getGame().move(move);
            }
        }
    }

    // -------------------------------------------------------------------------
    // 6. Procedural 16 kHz PCM Sound Synthesizer
    // -------------------------------------------------------------------------

    @Test
    public void testSoundSynthWaveformsAndMuteToggle() {
        assertEquals(16000, OctiSoundSynth.SAMPLE_RATE);
        OctiSoundSynth synth = new OctiSoundSynth();
        AtomicInteger playbackCount = new AtomicInteger(0);
        synth.setPlaybackCallback(pcm -> {
            assertTrue(pcm.length > 400);
            playbackCount.incrementAndGet();
        });

        for (OctiSoundSynth.SoundCue cue : OctiSoundSynth.SoundCue.values()) {
            short[] pcm = synth.synthesizeCuePcm(cue);
            assertNotNull(pcm);
            assertTrue("PCM buffer must have positive length for " + cue, pcm.length >= 600);
            long sumSq = 0;
            int peak = 0;
            for (short s : pcm) {
                int abs = Math.abs((int) s);
                if (abs > peak) {
                    peak = abs;
                }
                sumSq += (long) s * (long) s;
            }
            double rms = Math.sqrt((double) sumSq / pcm.length);
            assertTrue("Waveform peak must be audible for " + cue, peak > 1500);
            assertTrue("Waveform RMS must be non-trivial for " + cue, rms > 150.0);

            synth.triggerCue(cue);
        }
        assertEquals(OctiSoundSynth.SoundCue.values().length, playbackCount.get());

        // Muting suppresses playback callback
        synth.setMuted(true);
        assertTrue(synth.isMuted());
        synth.triggerCue(OctiSoundSynth.SoundCue.POD_CAPTURE);
        assertEquals(OctiSoundSynth.SoundCue.values().length, playbackCount.get());

        // Verify Cue alias, playCue static method and shutdown
        assertEquals(OctiSoundSynth.SoundCue.values().length, OctiSoundSynth.Cue.values().length);
        assertEquals(OctiSoundSynth.SoundCue.POD_PLACE, OctiSoundSynth.Cue.POD_PLACE);
        assertEquals(OctiSoundSynth.SoundCue.PRONG_SNAP, OctiSoundSynth.Cue.PRONG_SNAP);
        assertEquals(OctiSoundSynth.SoundCue.POD_STEP, OctiSoundSynth.Cue.POD_STEP);
        assertEquals(OctiSoundSynth.SoundCue.POD_JUMP, OctiSoundSynth.Cue.POD_JUMP);
        assertEquals(OctiSoundSynth.SoundCue.POD_CAPTURE, OctiSoundSynth.Cue.POD_CAPTURE);

        OctiSoundSynth.playCue(OctiSoundSynth.Cue.POD_PLACE, false);
        OctiSoundSynth.playCue(OctiSoundSynth.Cue.POD_PLACE, true);
        synth.shutdown();
    }

    // -------------------------------------------------------------------------
    // 7. 2D Tabletop Board Renderer Across Both Themes & Variants
    // -------------------------------------------------------------------------

    @Test
    public void testBoardRendererAcrossThemesRotationsAndOverlays() {
        OctiBoard board2p = createStartedBoard(
                OctiGameMode.GAME_MODE_2, true, true, OctiGameBases.GAME_BASES_3);
        board2p.getGame().move("33+a");
        board2p.getGame().move("37+e");
        board2p.getGame().move("53+b!");

        OctiMoveComposer composer = new OctiMoveComposer(board2p);
        composer.setConfirmAllMoves(true);
        composer.beginAddProngFromReserve(false);
        composer.dropOnCompassSector(5, 7, 0, OctiDir.DIR_S);

        for (OctiThemeId theme : OctiThemeId.values()) {
            for (OctiBoardGeometry.BoardRotation rot : OctiBoardGeometry.BoardRotation.values()) {
                int[] pixels = OctiBoardRenderer.renderBoardPixels(
                        board2p, composer, theme, rot, 360, 360);
                assertEquals(360 * 360, pixels.length);
                Set<Integer> unique = new HashSet<>();
                for (int px : pixels) {
                    unique.add(px);
                }
                assertTrue("Rendered board must contain rich visual elements", unique.size() > 64);
            }
        }

        OctiBoard board4p = createStartedBoard(OctiGameMode.GAME_MODE_4);
        int[] px4p = OctiBoardRenderer.renderBoardPixels(
                board4p, null, OctiThemeId.OBSIDIAN_BRASS,
                OctiBoardGeometry.BoardRotation.ROT_90, 360, 360);
        assertEquals(360 * 360, px4p.length);

        OctiBoard boardOfk = createStartedBoard(OctiGameMode.GAME_MODE_OFK);
        int[] pxOfk = OctiBoardRenderer.renderBoardPixels(
                boardOfk, null, OctiThemeId.ROSEWOOD_IVORY,
                OctiBoardGeometry.BoardRotation.ROT_180, 360, 360);
        assertEquals(360 * 360, pxOfk.length);
    }

    // -------------------------------------------------------------------------
    // 8. 500-Trial Seeded Property-Based Fuzzing Suite
    // -------------------------------------------------------------------------

    @Test
    public void testProperty500TrialsGeometryComposerAndBotInvariants() {
        Random rng = new Random(0x0C712026L);
        OctiDir[] allDirs = {
            OctiDir.DIR_N, OctiDir.DIR_NE, OctiDir.DIR_E, OctiDir.DIR_SE,
            OctiDir.DIR_S, OctiDir.DIR_SW, OctiDir.DIR_W, OctiDir.DIR_NW
        };
        OctiBoardGeometry.BoardRotation[] allRots = OctiBoardGeometry.BoardRotation.values();

        OctiBoard board = createStartedBoard(
                OctiGameMode.GAME_MODE_2, true, false, OctiGameBases.GAME_BASES_3);
        List<String> committedByComposer = new ArrayList<>();
        OctiMoveComposer composer = new OctiMoveComposer(board);
        composer.setMoveCommitListener(committedByComposer::add);
        OctiBotEngine casualBot = new OctiBotEngine(OctiBotEngine.BotDifficulty.CASUAL);
        casualBot.setTimeBudgetMs(25);

        int[] outXY = new int[2];

        for (int trial = 0; trial < 500; trial++) {
            // Property A: Bijective squareCenter <-> screenToSquare and dirUnitVectorScreen <-> hitTestCompassSector
            boolean ofkGrid = rng.nextBoolean();
            int gw = ofkGrid ? 6 : 9;
            int gh = ofkGrid ? 7 : 9;
            double vw = 240.0 + rng.nextDouble() * 1400.0;
            double vh = 240.0 + rng.nextDouble() * 1400.0;
            OctiBoardGeometry.BoardRotation rot = allRots[rng.nextInt(allRots.length)];
            OctiBoardGeometry geom = new OctiBoardGeometry(gw, gh, vw, vh, rot);

            int sx = 1 + rng.nextInt(gw);
            int sy = 1 + rng.nextInt(gh);
            double[] sqRect = geom.squareRect(sx, sy);
            // Sample a random interior point inside [15%..85%] of the square
            double px = sqRect[0] + sqRect[2] * (0.15 + rng.nextDouble() * 0.70);
            double py = sqRect[1] + sqRect[3] * (0.15 + rng.nextDouble() * 0.70);
            assertTrue("Trial " + trial + ": interior point must hit square", geom.screenToSquare(px, py, outXY));
            assertEquals("Trial " + trial + ": X mismatch", sx, outXY[0]);
            assertEquals("Trial " + trial + ": Y mismatch", sy, outXY[1]);

            OctiDir expectedDir = allDirs[rng.nextInt(allDirs.length)];
            double[] uv = geom.dirUnitVectorScreen(expectedDir);
            // Perturb angle within +/- 15 degrees (well inside the 45-degree sector)
            double jitterRad = Math.toRadians((rng.nextDouble() - 0.5) * 30.0);
            double cosJ = Math.cos(jitterRad);
            double sinJ = Math.sin(jitterRad);
            double jx = uv[0] * cosJ - uv[1] * sinJ;
            double jy = uv[0] * sinJ + uv[1] * cosJ;
            double radius = 25.0 + rng.nextDouble() * 30.0;
            OctiDir actualDir = geom.hitTestCompassSector(200.0, 200.0, 15.0, 65.0,
                    200.0 + jx * radius, 200.0 + jy * radius);
            assertEquals("Trial " + trial + ": compass sector mismatch", expectedDir, actualDir);

            // Property B: Random composer interactions never corrupt OctiBoard state,
            // and any move committed by OctiMoveComposer is strictly legal.
            if (board.getGame().isGameOver() || trial % 25 == 0) {
                OctiGameMode mode = (trial % 3 == 0) ? OctiGameMode.GAME_MODE_OFK
                        : ((trial % 3 == 1) ? OctiGameMode.GAME_MODE_4 : OctiGameMode.GAME_MODE_2);
                OctiGameBases bases = (mode == OctiGameMode.GAME_MODE_OFK)
                        ? OctiGameBases.GAME_BASES_1
                        : OctiGameBases.GAME_BASES_3;
                board = createStartedBoard(mode, mode != OctiGameMode.GAME_MODE_OFK, trial % 6 == 0, bases);
                composer.setBoard(board);
            }

            List<String> legalBefore = legalMoveStrings(board);
            if (legalBefore.isEmpty()) {
                continue;
            }
            committedByComposer.clear();
            composer.setConfirmAllMoves(rng.nextBoolean());

            int action = rng.nextInt(5);
            int rx = 1 + rng.nextInt(board.getBoardWidth());
            int ry = 1 + rng.nextInt(board.getBoardHeight());
            OctiDir rDir = allDirs[rng.nextInt(allDirs.length)];

            if (action == 0) {
                if (composer.beginAddPodFromReserve()) {
                    composer.dropOnSquare(rx, ry);
                }
            } else if (action == 1) {
                if (composer.beginAddProngFromReserve(rng.nextBoolean())) {
                    composer.dropOnCompassSector(rx, ry, 0, rDir);
                }
            } else if (action == 2) {
                if (composer.beginMoveOrJumpPod(rx, ry, 0, rng.nextBoolean())) {
                    int tx = 1 + rng.nextInt(board.getBoardWidth());
                    int ty = 1 + rng.nextInt(board.getBoardHeight());
                    composer.dropOnSquare(tx, ty);
                }
            } else if (action == 3) {
                if (composer.beginRepositionProng(rx, ry, 0, rDir)) {
                    OctiDir dstDir = allDirs[rng.nextInt(allDirs.length)];
                    composer.dropOnCompassSector(rx, ry, 0, dstDir);
                }
            } else {
                composer.rollbackMove();
            }

            if (composer.shouldPauseForConfirmation() && rng.nextBoolean()) {
                composer.commitMove();
            } else if (composer.shouldPauseForConfirmation()
                    || composer.getDragKind() != OctiMoveComposer.DragKind.NONE) {
                composer.rollbackMove();
            }

            for (String cm : committedByComposer) {
                assertTrue("Trial " + trial + ": composer committed illegal move '" + cm + "'",
                        legalBefore.contains(cm));
            }

            // Property C: Periodically verify OctiBotEngine chooses a legal move without mutating board
            if (trial % 10 == 0 && !board.getGame().isGameOver()) {
                String snap = board.toPositionString();
                List<String> legalNow = legalMoveStrings(board);
                if (!legalNow.isEmpty()) {
                    String botMove = casualBot.chooseMove(board, null);
                    assertEquals("Trial " + trial + ": bot mutated board", snap, board.toPositionString());
                    assertTrue("Trial " + trial + ": bot chose illegal move '" + botMove + "'",
                            legalNow.contains(botMove));
                    board.getGame().move(botMove);
                    composer.refreshLegalMoves();
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // 8. Envelope 2: Full-Width Borderless Board, Guided Moves & Visual Artifacts
    // -------------------------------------------------------------------------

    @Test
    public void testOctiAppVersionAndProtocolConstants() {
        assertEquals("2.1.0", OctiDefs.OCTI_APP_VERSION);
        assertEquals(1, OctiDefs.OCTI_PROTOCOL_VERSION);
    }

    @Test
    public void testZeroBorderFullWidthBoardGeometry() {
        // 9x9 square viewport (1080 x 1080)
        OctiBoardGeometry geom = new OctiBoardGeometry(9, 9, 1080.0, 1080.0,
                OctiBoardGeometry.BoardRotation.ROT_0);
        double[] outer = geom.boardOuterRect();
        assertEquals(0.0, outer[0], 0.001);
        assertEquals(0.0, outer[1], 0.001);
        assertEquals(1080.0, outer[2], 0.001);
        assertEquals(1080.0, outer[3], 0.001);

        double[] inner = geom.gridInnerRect();
        assertEquals(0.0, inner[0], 0.001);
        assertEquals(0.0, inner[1], 0.001);
        assertEquals(1080.0, inner[2], 0.001);
        assertEquals(1080.0, inner[3], 0.001);

        assertEquals(120.0, geom.cellSize(), 0.001);
        assertEquals(48.0, geom.podOuterRadius(), 0.001); // 0.40 * cellSize

        // 9x9 tall portrait viewport (1080 x 1920)
        OctiBoardGeometry geomPortrait = new OctiBoardGeometry(9, 9, 1080.0, 1920.0,
                OctiBoardGeometry.BoardRotation.ROT_0);
        assertEquals(120.0, geomPortrait.cellSize(), 0.001);
        assertEquals(48.0, geomPortrait.podOuterRadius(), 0.001);

        // OFK 6x7 tall portrait viewport (1080 x 1920)
        OctiBoardGeometry geomOfk = new OctiBoardGeometry(6, 7, 1080.0, 1920.0,
                OctiBoardGeometry.BoardRotation.ROT_0);
        assertEquals(180.0, geomOfk.cellSize(), 0.001); // 1080 / 6 = 180.0
        assertEquals(72.0, geomOfk.podOuterRadius(), 0.001);
    }

    @Test
    public void testMoveComposerGuidedMoveWorkflowStep() {
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.move("33+c"); // Player A adds East prong at 3,3
        game.move("37+c"); // Player B adds East prong at 3,7

        OctiMoveComposer composer = new OctiMoveComposer(board);
        assertFalse(composer.isGuidedMoveActive());

        // Begin guided pod move on (3,3) pod 0
        assertTrue(composer.beginGuidedPodMove(3, 3, 0, false));
        assertTrue(composer.isGuidedMoveActive());

        int[] cur = composer.guidedCurrentSquare();
        assertEquals(3, cur[0]);
        assertEquals(3, cur[1]);

        int[] orig = composer.guidedOriginSquare();
        assertEquals(3, orig[0]);
        assertEquals(3, orig[1]);

        List<int[]> nexts = composer.guidedNextSquares();
        assertFalse(nexts.isEmpty());
        boolean has43 = false;
        for (int[] sq : nexts) {
            if (sq[0] == 4 && sq[1] == 3) {
                has43 = true;
                break;
            }
        }
        assertTrue("Destination (4,3) must be in legal next squares", has43);

        // Selecting a single step completes the guided hop sequence and pauses for confirmation
        assertTrue(composer.selectGuidedDestination(4, 3));
        assertFalse(composer.isGuidedMoveActive());
        assertTrue(composer.isAwaitingConfirmation());
        assertEquals("33c-43", composer.stagedMoveNotation());

        // Commit move
        assertTrue(composer.commitStagedMove());
        assertEquals(OctiPlayer.PLAYER_B, game.getTurn());
        assertEquals(1, board.squareAt(3, 2).getNumPods());
    }

    @Test
    public void testMoveComposerGuidedJumpChainAndFinishHere() {
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.move("33+c"); // A: adds East prong at (3,3)
        game.move("37+c"); // B: adds East prong at (3,7)
        game.move("33c-43"); // A: steps to (4,3)
        game.move("37c-47"); // B: steps to (4,7)

        OctiMoveComposer composer = new OctiMoveComposer(board);
        assertFalse(composer.isGuidedMoveActive());

        // Cancel and rollback cleanly resets guided state
        assertTrue(composer.beginGuidedPodMove(4, 3, 0, false));
        assertTrue(composer.isGuidedMoveActive());
        composer.cancelAndRollback();
        assertFalse(composer.isGuidedMoveActive());
    }

    @Test
    public void testRenderZoomedPodPixelsAndSplashPixels() {
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_2);
        int[] zoomed = OctiBoardRenderer.renderZoomedPodPixels(
                board, 3, 3, 0, 0, OctiThemeId.OBSIDIAN_BRASS, 480, -1);
        assertNotNull(zoomed);
        assertEquals(480 * 480, zoomed.length);

        int[] splash = OctiBoardRenderer.renderSplashPixels(480, 800, 0.5);
        assertNotNull(splash);
        assertEquals(480 * 800, splash.length);
    }

    @Test
    public void testExportAndroidVisualArtifactPngs() throws Exception {
        File repoRoot = resolveRepoRoot();
        File artifactDir = new File(repoRoot, "plan/done/plan_gui_refinement.artifacts");
        if (!artifactDir.isDirectory()) {
            artifactDir = new File(repoRoot, "plan/plan_gui_refinement.artifacts");
        }
        if (!artifactDir.isDirectory()) {
            assertTrue(artifactDir.mkdirs());
        }

        // 1. android_board_fullwidth_2p.png (1080 x 1080)
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_2);
        OctiMoveComposer composer = new OctiMoveComposer(board);
        OctiBoardGeometry geom = new OctiBoardGeometry(9, 9, 1080.0, 1080.0,
                OctiBoardGeometry.BoardRotation.ROT_0);
        int[] boardPixels = OctiBoardRenderer.renderBoardPixels(
                board, composer, geom, OctiThemeId.OBSIDIAN_BRASS);
        File boardPng = new File(artifactDir, "android_board_fullwidth_2p.png");
        OctiAppIconRenderer.writeArgbPng(1080, 1080, boardPixels, boardPng);
        assertTrue("android_board_fullwidth_2p.png must exist", boardPng.isFile());
        assertTrue("android_board_fullwidth_2p.png must be > 4KB", boardPng.length() > 4096);

        // 2. android_splash.png (1080 x 1920)
        int[] splashPixels = OctiBoardRenderer.renderSplashPixels(1080, 1920, 0.65);
        File splashPng = new File(artifactDir, "android_splash.png");
        OctiAppIconRenderer.writeArgbPng(1080, 1920, splashPixels, splashPng);
        assertTrue("android_splash.png must exist", splashPng.isFile());
        assertTrue("android_splash.png must be > 4KB", splashPng.length() > 4096);

        // 3. android_zoomed_square.png (800 x 800)
        board.getGame().move("33+a");
        board.getGame().move("37+e");
        int[] zoomPixels = OctiBoardRenderer.renderZoomedPodPixels(
                board, 3, 3, 0, 0, OctiThemeId.OBSIDIAN_BRASS, 800, -1);
        File zoomPng = new File(artifactDir, "android_zoomed_square.png");
        OctiAppIconRenderer.writeArgbPng(800, 800, zoomPixels, zoomPng);
        assertTrue("android_zoomed_square.png must exist", zoomPng.isFile());
        assertTrue("android_zoomed_square.png must be > 4KB", zoomPng.length() > 4096);

        // Validate PNG magic bytes across all three artifacts
        for (File f : new File[]{boardPng, splashPng, zoomPng}) {
            byte[] bytes = Files.readAllBytes(f.toPath());
            assertEquals((byte) 0x89, bytes[0]);
            assertEquals((byte) 0x50, bytes[1]);
            assertEquals((byte) 0x4E, bytes[2]);
            assertEquals((byte) 0x47, bytes[3]);
        }
    }

    @Test
    public void testPodPlacementAndMandatoryProngConfirmation_PPL_A02() {
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_2);
        OctiMoveComposer composer = new OctiMoveComposer(board);
        assertTrue(composer.confirmAllMoves());

        // 1. Placing pod via stageOrCommitAddPod on occupied home square (3, 3)
        // With confirmAllMoves=true, it stages the move and does NOT auto-commit
        assertTrue(composer.stageOrCommitAddPod(3, 3, false));
        assertTrue(composer.isAwaitingConfirmation());
        assertEquals("33", composer.stagedMoveNotation());
        assertEquals(OctiPlayer.PLAYER_A, board.getGame().getTurn());

        // Commit the staged pod placement
        assertTrue(composer.commitStagedMove());
        assertFalse(composer.isAwaitingConfirmation());
        assertEquals(OctiPlayer.PLAYER_B, board.getGame().getTurn());
        assertEquals(2, board.squareAt(2, 2).getNumPods());

        // 2. Mandatory confirmation for adding a prong:
        // Adding a prong must stage the move (isAwaitingConfirmation == true)
        // and NOT auto-commit until commitStagedMove is called.
        assertTrue(composer.stageOrCommitAddProng(5, 7, 0, OctiDir.DIR_N, false));
        assertTrue(composer.isAwaitingConfirmation());
        assertEquals("57+a", composer.stagedMoveNotation());
        assertEquals(OctiPlayer.PLAYER_B, board.getGame().getTurn());

        // Confirming the move commits it
        assertTrue(composer.commitStagedMove());
        assertFalse(composer.isAwaitingConfirmation());
        assertEquals(OctiPlayer.PLAYER_A, board.getGame().getTurn());

        // 3. Single-tap board move via beginGuidedPodMove + selectGuidedDestination:
        // Direct move without opening zoomed square dialog
        assertTrue(composer.stageOrCommitAddProng(3, 3, 0, OctiDir.DIR_E, false));
        assertTrue(composer.commitStagedMove());
        assertTrue(composer.stageOrCommitAddProng(7, 7, 0, OctiDir.DIR_N, false));
        assertTrue(composer.commitStagedMove());

        // A single-taps pod at (3, 3): starts guided pod move
        assertTrue(composer.beginGuidedPodMove(3, 3, 0, false));
        assertTrue(composer.isGuidedMoveActive());
        List<int[]> nextSquares = composer.guidedNextSquares();
        boolean hasDest43 = false;
        for (int[] sq : nextSquares) {
            if (sq[0] == 4 && sq[1] == 3) {
                hasDest43 = true;
                break;
            }
        }
        assertTrue("Destination (4, 3) must be in guided next squares", hasDest43);

        // A single-taps destination (4, 3) on the board:
        assertTrue(composer.selectGuidedDestination(4, 3));
        assertFalse(composer.isGuidedMoveActive());
        assertTrue(composer.isAwaitingConfirmation());
        assertEquals("33c-43", composer.stagedMoveNotation());

        // Must require confirmation
        assertTrue(composer.commitStagedMove());
        assertEquals(OctiPlayer.PLAYER_B, board.getGame().getTurn());
        assertEquals(1, board.squareAt(3, 2).getNumPods());
    }

    @Test
    public void testGuidedMoveZeroHopFriendlyPodSwitchAndCancel_AZG_A02() {
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_2);
        board.getGame().move("33+a");
        board.getGame().move("37+e");
        board.getGame().move("53+c");
        board.getGame().move("57+e");

        OctiMoveComposer composer = new OctiMoveComposer(board);

        // Start guided move on friendly pod at (3, 3) (0 hops taken so far)
        assertTrue(composer.beginGuidedPodMove(3, 3, 0, false));
        assertTrue(composer.isGuidedMoveActive());
        assertEquals(3, composer.guidedOriginSquare()[0]);
        assertEquals(3, composer.guidedOriginSquare()[1]);
        assertEquals(3, composer.guidedCurrentSquare()[0]);
        assertEquals(3, composer.guidedCurrentSquare()[1]);

        // Switching to another friendly pod at (5, 3) when 0 hops have been taken:
        // cancelAndRollback() followed by beginGuidedPodMove(5, 3, 0, false)
        composer.cancelAndRollback();
        assertFalse(composer.isGuidedMoveActive());
        assertTrue(composer.beginGuidedPodMove(5, 3, 0, false));
        assertTrue(composer.isGuidedMoveActive());
        assertEquals(5, composer.guidedOriginSquare()[0]);
        assertEquals(3, composer.guidedOriginSquare()[1]);
        assertEquals(5, composer.guidedCurrentSquare()[0]);
        assertEquals(3, composer.guidedCurrentSquare()[1]);

        // Select destination (6, 3) for pod at (5, 3)
        assertTrue(composer.selectGuidedDestination(6, 3));
        assertFalse(composer.isGuidedMoveActive());
        assertTrue(composer.isAwaitingConfirmation());
        assertEquals("53c-63", composer.stagedMoveNotation());
    }

    @Test
    public void testStageActionsResetGuidedMoveActiveAndSetAwaitingConfirmation_ACF_A01() {
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_2);
        // Pod at (3, 3) starts with DIR_N so it can move
        board.squareAt(2, 2).getPodAt(0).addProng(OctiDir.DIR_N, false);
        OctiMoveComposer composer = new OctiMoveComposer(board);
        assertTrue(composer.confirmAllMoves());

        // 1. Adding prong resets guidedMoveActive
        assertTrue(composer.beginGuidedPodMove(3, 3, 0, false));
        assertTrue(composer.isGuidedMoveActive());
        assertFalse(composer.isAwaitingConfirmation());

        assertTrue(composer.stageOrCommitAddProng(3, 3, 0, OctiDir.DIR_E, false));
        assertFalse("stageOrCommitAddProng must reset guidedMoveActive", composer.isGuidedMoveActive());
        assertTrue("stageOrCommitAddProng must set awaitingConfirmation", composer.isAwaitingConfirmation());
        assertEquals("33a+c", composer.stagedMoveNotation());
        assertTrue(composer.commitStagedMove());

        // B's turn: B adds prong and commits
        assertTrue(composer.stageOrCommitAddProng(3, 7, 0, OctiDir.DIR_S, false));
        assertTrue(composer.commitStagedMove());

        // 2. Repositioning prong resets guidedMoveActive
        assertTrue(composer.beginGuidedPodMove(3, 3, 0, false));
        assertTrue(composer.isGuidedMoveActive());
        assertFalse(composer.isAwaitingConfirmation());

        assertTrue(composer.stageOrCommitRepositionProng(3, 3, 0, OctiDir.DIR_N, OctiDir.DIR_W));
        assertFalse("stageOrCommitRepositionProng must reset guidedMoveActive", composer.isGuidedMoveActive());
        assertTrue("stageOrCommitRepositionProng must set awaitingConfirmation", composer.isAwaitingConfirmation());
        assertTrue(composer.commitStagedMove());

        // B's turn: B adds prong and commits
        assertTrue(composer.stageOrCommitAddProng(5, 7, 0, OctiDir.DIR_S, false));
        assertTrue(composer.commitStagedMove());

        // 3. Adding pod resets guidedMoveActive
        assertTrue(composer.beginGuidedPodMove(3, 3, 0, false));
        assertTrue(composer.isGuidedMoveActive());
        assertFalse(composer.isAwaitingConfirmation());

        assertTrue(composer.stageOrCommitAddPod(3, 3, false));
        assertFalse("stageOrCommitAddPod must reset guidedMoveActive", composer.isGuidedMoveActive());
        assertTrue("stageOrCommitAddPod must set awaitingConfirmation", composer.isAwaitingConfirmation());
        assertEquals("33", composer.stagedMoveNotation());
        assertTrue(composer.commitStagedMove());
    }

    @Test
    public void testDirectProngAdditionOnPodWithExistingProngs_ACF_A03() {
        OctiBoard board = createStartedBoard(OctiGameMode.GAME_MODE_2);
        OctiMoveComposer composer = new OctiMoveComposer(board);

        // Player A starts by adding DIR_N ('a') to pod at (3, 3)
        assertTrue(composer.stageOrCommitAddProng(3, 3, 0, OctiDir.DIR_N, false));
        assertTrue(composer.commitStagedMove());

        // Player B turn: add prong on (3, 7)
        assertTrue(composer.stageOrCommitAddProng(3, 7, 0, OctiDir.DIR_S, false));
        assertTrue(composer.commitStagedMove());

        // Player A's turn again: Pod at (3, 3) already has DIR_N.
        // Direct addition on empty socket DIR_E ('c') must succeed cleanly
        List<OctiDir> legals = composer.legalProngAdditionsAt(3, 3, 0, false);
        assertFalse(legals.contains(OctiDir.DIR_N));
        assertTrue(legals.contains(OctiDir.DIR_E));
        assertTrue(legals.contains(OctiDir.DIR_W));

        assertTrue(composer.stageOrCommitAddProng(3, 3, 0, OctiDir.DIR_E, false));
        assertTrue(composer.isAwaitingConfirmation());
        assertEquals("33a+c", composer.stagedMoveNotation());

        // Preview board should show both prongs
        OctiBoard preview = composer.buildPreviewBoard();
        assertNotNull(preview);
        OctiPod previewPod = preview.squareAt(2, 2).getPodAt(0);
        assertEquals(OctiProngType.NORMAL_PRONG, previewPod.prongAt(OctiDir.DIR_N));
        assertEquals(OctiProngType.NORMAL_PRONG, previewPod.prongAt(OctiDir.DIR_E));
        assertEquals(OctiProngType.NO_PRONG, previewPod.prongAt(OctiDir.DIR_S));

        // Commit move
        assertTrue(composer.commitStagedMove());
        assertEquals(OctiPlayer.PLAYER_B, board.getGame().getTurn());
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
