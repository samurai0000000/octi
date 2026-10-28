/*
 * OctiMoveComposerAndRendererTest.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Test;

import java.util.List;

public class OctiMoveComposerAndRendererTest {

    private OctiBoard board;
    private OctiGame game;
    private OctiMoveComposer composer;
    private OctiPalette palette;

    @Before
    public void setUp() {
        board = new OctiBoard(OctiGameMode.GAME_MODE_2, true, false, OctiGameBases.GAME_BASES_3);
        game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);
        composer = new OctiMoveComposer(board);
        composer.setConfirmAllMoves(true);
        palette = OctiPalette.forTheme(OctiThemeId.OBSIDIAN_BRASS);
    }

    @Test
    public void testSortedPodIndicesMostProngsFirst() {
        // Retrieve an empty square
        OctiSquare sq = board.squareAt(4, 4);
        sq.clear();

        OctiPod podFew = new OctiPod(OctiPodType.A_POD);
        podFew.addProng(OctiDir.DIR_N, false);

        OctiPod podMany = new OctiPod(OctiPodType.A_POD);
        podMany.addProng(OctiDir.DIR_N, false);
        podMany.addProng(OctiDir.DIR_NE, false);
        podMany.addProng(OctiDir.DIR_E, false);
        podMany.addProng(OctiDir.DIR_SE, false);

        OctiPod podMid = new OctiPod(OctiPodType.A_POD);
        podMid.addProng(OctiDir.DIR_N, false);
        podMid.addProng(OctiDir.DIR_S, false);

        sq.addPod(podFew);  // index 0: 1 prong
        sq.addPod(podMany); // index 1: 4 prongs
        sq.addPod(podMid);  // index 2: 2 prongs

        int[] sorted = OctiBoardRenderer.sortedPodIndicesMostProngsFirst(sq);
        assertEquals(3, sorted.length);
        assertEquals(1, sorted[0]); // 4 prongs
        assertEquals(2, sorted[1]); // 2 prongs
        assertEquals(0, sorted[2]); // 1 prong
    }

    @Test
    public void testSortedPodIndicesTieBreakerWithSuperProngs() {
        OctiSquare sq = board.squareAt(4, 4);
        sq.clear();

        OctiPod podNormal = new OctiPod(OctiPodType.A_POD);
        podNormal.addProng(OctiDir.DIR_N, false);
        podNormal.addProng(OctiDir.DIR_S, false);

        OctiPod podSuper = new OctiPod(OctiPodType.A_POD);
        podSuper.addProng(OctiDir.DIR_N, true);
        podSuper.addProng(OctiDir.DIR_S, false);

        sq.addPod(podNormal); // index 0: 2 prongs, 0 super
        sq.addPod(podSuper);  // index 1: 2 prongs, 1 super

        int[] sorted = OctiBoardRenderer.sortedPodIndicesMostProngsFirst(sq);
        assertEquals(2, sorted.length);
        assertEquals(1, sorted[0]); // Superprong breaks tie
        assertEquals(0, sorted[1]);
    }

    @Test
    public void testOriginSquareUnclaimedPodsDetection() {
        OctiSquare sq = board.squareAt(2, 2);
        sq.clear();
        OctiPod pod0 = new OctiPod(OctiPodType.A_POD);
        pod0.addProng(OctiDir.DIR_E, false);
        OctiPod pod1 = new OctiPod(OctiPodType.A_POD);
        pod1.addProng(OctiDir.DIR_N, false);
        sq.addPod(pod0);
        sq.addPod(pod1);

        assertTrue(composer.beginGuidedPodMove(3, 3, 1, false));
        assertTrue(composer.isGuidedMoveActive());
        assertTrue(composer.selectGuidedDestination(3, 4));
        assertFalse(composer.isGuidedMoveActive());
        assertTrue(composer.isAwaitingConfirmation());

        List<Integer> unclaimed = composer.unclaimedMovablePodIndicesAtOrigin();
        assertEquals(1, unclaimed.size());
        assertEquals(Integer.valueOf(0), unclaimed.get(0));
        assertTrue(composer.hasOtherMovablePodAtOrigin());
    }

    @Test
    public void testOriginSquareDistinctUnclaimedPods() {
        OctiSquare sq = board.squareAt(2, 2);
        sq.clear();
        OctiPod pod0 = new OctiPod(OctiPodType.A_POD);
        pod0.addProng(OctiDir.DIR_E, false);
        OctiPod pod1 = new OctiPod(OctiPodType.A_POD);
        pod1.addProng(OctiDir.DIR_N, false);
        OctiPod pod2 = new OctiPod(OctiPodType.A_POD);
        pod2.addProng(OctiDir.DIR_S, false);
        sq.addPod(pod0);
        sq.addPod(pod1);
        sq.addPod(pod2);

        assertTrue(composer.beginGuidedPodMove(3, 3, 2, false));
        assertTrue(composer.selectGuidedDestination(3, 2));

        List<Integer> unclaimed = composer.unclaimedMovablePodIndicesAtOrigin();
        assertEquals(2, unclaimed.size());
        assertTrue(composer.hasDistinctUnclaimedPodsAtOrigin());
    }

    @Test
    public void testOriginSquareIdenticalPodsNotDistinct() {
        OctiSquare sq = board.squareAt(2, 2);
        sq.clear();
        OctiPod pod0 = new OctiPod(OctiPodType.A_POD);
        pod0.addProng(OctiDir.DIR_E, false);
        OctiPod pod1 = new OctiPod(OctiPodType.A_POD);
        pod1.addProng(OctiDir.DIR_E, false);
        OctiPod pod2 = new OctiPod(OctiPodType.A_POD);
        pod2.addProng(OctiDir.DIR_N, false);
        sq.addPod(pod0);
        sq.addPod(pod1);
        sq.addPod(pod2);

        assertTrue(composer.beginGuidedPodMove(3, 3, 2, false));
        assertTrue(composer.selectGuidedDestination(3, 4));

        List<Integer> unclaimed = composer.unclaimedMovablePodIndicesAtOrigin();
        assertEquals(2, unclaimed.size());
        assertFalse(composer.hasDistinctUnclaimedPodsAtOrigin());
    }

    @Test
    public void testSuperProngEighthProngBonusGuidedActivation() {
        OctiSquare sq = board.squareAt(2, 2);
        assertNotNull(sq);
        OctiPod pod = sq.getPodAt(0);
        assertNotNull(pod);

        // Add 7 prongs
        for (int i = 0; i < 7; i++) {
            pod.addProng(OctiDir.fromIndex(i), false);
        }
        assertEquals(7, pod.getNumProngs());

        // Stage the 8th prong via composer at (3, 3) 1-indexed
        boolean staged = composer.stageOrCommitAddProng(3, 3, 0, OctiDir.DIR_NW, false);
        assertTrue(staged);

        // If awaiting superprong bonus move, guided move is automatically active
        if (composer.isAwaitingSuperProngBonusMove()) {
            assertTrue(composer.isGuidedMoveActive());
        }
    }

    @Test
    public void testRenderBoardPixelsWithDashedOriginHighlight() {
        OctiBoardGeometry geom = new OctiBoardGeometry(9, 9, 540.0, 540.0, OctiBoardGeometry.BoardRotation.ROT_0);

        // Render standard board
        int[] out = OctiBoardRenderer.renderBoardPixels(board, composer, geom, OctiThemeId.OBSIDIAN_BRASS);
        assertNotNull(out);
        assertEquals(540 * 540, out.length);

        // Render with active guided move
        composer.beginGuidedPodMove(3, 3, 0, false);
        int[] outGuided = OctiBoardRenderer.renderBoardPixels(board, composer, geom, OctiThemeId.OBSIDIAN_BRASS);
        assertNotNull(outGuided);
        assertEquals(540 * 540, outGuided.length);
    }

    @Test
    public void testRenderZoomedPodPixelsWithHoverProngPreview() {
        int size = 280;

        // Hover over empty socket DIR_N
        int[] out = OctiBoardRenderer.renderZoomedPodPixels(
                board, 3, 3, 0, 0,
                OctiThemeId.OBSIDIAN_BRASS, size, OctiDir.DIR_N.getIndex()
        );
        assertNotNull(out);
        assertEquals(size * size, out.length);
    }

    @Test
    public void testPaintPlayerReserveCardRendering() {
        int w = 600;
        int h = 200;
        int[] pixels = new int[w * h];

        OctiBoardRenderer.paintPlayerReserveCard(
                pixels, w, h, 20.0, 20.0, 260.0, 100.0,
                OctiPlayer.PLAYER_A, true,
                4, 1, 22, 1, true, palette
        );

        OctiBoardRenderer.paintPlayerReserveCard(
                pixels, w, h, 300.0, 20.0, 260.0, 100.0,
                OctiPlayer.PLAYER_B, false,
                3, 0, 24, 0, true, palette
        );

        boolean nonZeroFound = false;
        for (int p : pixels) {
            if (p != 0) {
                nonZeroFound = true;
                break;
            }
        }
        assertTrue(nonZeroFound);
    }

    @Test
    public void testDrawVectorStringRotatedAngles() {
        int w = 120;
        int h = 120;
        int[] pixels = new int[w * h];

        int[] angles = {0, 90, 180, 270};
        for (int angle : angles) {
            java.util.Arrays.fill(pixels, 0);
            OctiBoardRenderer.drawVectorStringRotated(
                    pixels, w, h, 60.0, 60.0, 14.0, "OCTI",
                    0xFFFFFFFF, 1.5, angle
            );
            boolean painted = false;
            for (int p : pixels) {
                if (p != 0) {
                    painted = true;
                    break;
                }
            }
            assertTrue("Expected painted pixels at rotation " + angle, painted);
        }
    }

    @Test
    public void testRenderZoomBoardAndReserveHudPixels() {
        // 1. Zoomed hover preview blade render
        int zoomSize = 600;
        OctiSquare sqZoom = board.squareAt(2, 2);
        sqZoom.clear();
        OctiPod zoomPod = new OctiPod(OctiPodType.A_POD);
        zoomPod.addProng(OctiDir.DIR_E, false);
        zoomPod.addProng(OctiDir.DIR_S, false);
        sqZoom.addPod(zoomPod);
        int[] zoomedPixels = OctiBoardRenderer.renderZoomedPodPixels(
                board, 3, 3, 0, 0,
                OctiThemeId.OBSIDIAN_BRASS, zoomSize, OctiDir.DIR_N.getIndex()
        );
        assertEquals(zoomSize * zoomSize, zoomedPixels.length);

        // 2. Stacked pods sorted on board render
        OctiSquare sqStack = board.squareAt(4, 4);
        sqStack.clear();
        OctiPod p1 = new OctiPod(OctiPodType.A_POD);
        p1.addProng(OctiDir.DIR_N, false);
        OctiPod p4 = new OctiPod(OctiPodType.A_POD);
        p4.addProng(OctiDir.DIR_N, false);
        p4.addProng(OctiDir.DIR_E, false);
        p4.addProng(OctiDir.DIR_S, false);
        p4.addProng(OctiDir.DIR_W, false);
        OctiPod p2 = new OctiPod(OctiPodType.A_POD);
        p2.addProng(OctiDir.DIR_NE, false);
        p2.addProng(OctiDir.DIR_SW, false);
        sqStack.addPod(p1);
        sqStack.addPod(p4);
        sqStack.addPod(p2);

        int boardSize = 800;
        OctiBoardGeometry geom = new OctiBoardGeometry(boardSize, boardSize, 9, 9, OctiBoardGeometry.BoardRotation.ROT_0);
        int[] boardStackPixels = OctiBoardRenderer.renderBoardPixels(board, null, geom, OctiThemeId.OBSIDIAN_BRASS);
        assertEquals(boardSize * boardSize, boardStackPixels.length);

        // 3. Reserve HUD cards render
        int hudW = 640;
        int hudH = 140;
        int[] hudPixels = new int[hudW * hudH];
        OctiPalette pal = OctiPalette.forTheme(OctiThemeId.OBSIDIAN_BRASS);
        OctiBoardRenderer.paintPlayerReserveCard(
                hudPixels, hudW, hudH, 16.0, 16.0, 290.0, 108.0,
                OctiPlayer.PLAYER_A, true,
                4, 1, 22, 1, true, pal
        );
        OctiBoardRenderer.paintPlayerReserveCard(
                hudPixels, hudW, hudH, 334.0, 16.0, 290.0, 108.0,
                OctiPlayer.PLAYER_B, false,
                3, 0, 24, 0, true, pal
        );
        boolean nonZero = false;
        for (int px : hudPixels) {
            if (px != 0) {
                nonZero = true;
                break;
            }
        }
        assertTrue(nonZero);
    }

    @Test
    public void testThreeAndFourJumpGuidedAndDragChain() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false, OctiGameBases.GAME_BASES_3);
        board.getGame().start(OctiPlayer.PLAYER_A);

        // Clear board and set up a 4-jump chain for Player A:
        // Jumper at (2, 2) [1-based], with N and E prongs:
        // Hop 1: jump over (2, 3) -> land at (2, 4)
        // Hop 2: jump over (2, 5) -> land at (2, 6)
        // Hop 3: jump over (3, 6) -> land at (4, 6)
        // Hop 4: jump over (5, 6) -> land at (6, 6)
        for (int x = 0; x < 9; x++) {
            for (int y = 0; y < 9; y++) {
                board.squareAt(x, y).clear();
            }
        }
        OctiPod jumper = new OctiPod(OctiPodType.A_POD);
        jumper.addProng(OctiDir.DIR_N, false);
        jumper.addProng(OctiDir.DIR_E, false);
        board.squareAt(1, 1).addPod(jumper); // (2, 2)

        board.squareAt(1, 2).addPod(new OctiPod(OctiPodType.A_POD)); // (2, 3)
        board.squareAt(1, 4).addPod(new OctiPod(OctiPodType.B_POD)); // (2, 5)
        board.squareAt(2, 5).addPod(new OctiPod(OctiPodType.A_POD)); // (3, 6)
        board.squareAt(4, 5).addPod(new OctiPod(OctiPodType.B_POD)); // (5, 6)

        OctiMoveComposer composer = new OctiMoveComposer(board);
        composer.setConfirmAllMoves(true);

        // Start guided move at (2, 2)
        assertTrue(composer.beginGuidedPodMove(2, 2, 0, false));
        assertTrue(composer.isGuidedLegalDestination(2, 4));

        // 1st jump: (2, 2) -> (2, 4)
        assertTrue(composer.selectGuidedDestination(2, 4));
        assertTrue("Must remain active after 1st jump", composer.isGuidedMoveActive());
        assertTrue("Must be mid-hop after 1st jump", composer.isGuidedMidHop());
        assertEquals(2, composer.guidedCurrentSquare()[0]);
        assertEquals(4, composer.guidedCurrentSquare()[1]);
        assertTrue("2nd jump (2, 6) must be legal", composer.isGuidedLegalDestination(2, 6));

        // 2nd jump: (2, 4) -> (2, 6)
        assertTrue(composer.selectGuidedDestination(2, 6));
        assertTrue("Must remain active after 2nd jump", composer.isGuidedMoveActive());
        assertTrue("Must be mid-hop after 2nd jump", composer.isGuidedMidHop());
        assertEquals(2, composer.guidedCurrentSquare()[0]);
        assertEquals(6, composer.guidedCurrentSquare()[1]);
        assertTrue("3rd jump (4, 6) must be legal", composer.isGuidedLegalDestination(4, 6));

        // 3rd jump: (2, 6) -> (4, 6)
        assertTrue(composer.selectGuidedDestination(4, 6));
        assertTrue("Must remain active after 3rd jump", composer.isGuidedMoveActive());
        assertTrue("Must be mid-hop after 3rd jump", composer.isGuidedMidHop());
        assertEquals(4, composer.guidedCurrentSquare()[0]);
        assertEquals(6, composer.guidedCurrentSquare()[1]);
        assertTrue("4th jump (6, 6) must be legal", composer.isGuidedLegalDestination(6, 6));

        // 4th jump: (4, 6) -> (6, 6)
        assertTrue(composer.selectGuidedDestination(6, 6));
        assertEquals("22ac-24-26x-46-66x", composer.stagedMoveNotation());
        assertTrue(composer.commitMove());
        assertEquals(1, board.squareAt(5, 5).getNumPods());
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
