/*
 * OctiBoardEngineTest.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.Test;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Random;

public class OctiBoardEngineTest {

    // -------------------------------------------------------------------------
    // 1. OctiPod Tests
    // -------------------------------------------------------------------------

    @Test
    public void testOctiPodDefaultConstructionAndClear() {
        OctiPod pod = new OctiPod();
        assertEquals(OctiPodType.POD_UNDEF, pod.getType());
        assertEquals(0, pod.getNumProngs());
        assertEquals(0, pod.getNumNormalProngs());
        assertEquals(0, pod.getNumSuperProngs());
        assertFalse(pod.hasSuperProng());
        assertEquals("", pod.getConfigString());

        pod.setType(OctiPodType.A_POD);
        pod.addProng(OctiDir.DIR_N, false);
        assertEquals(1, pod.getNumProngs());

        pod.clear();
        assertEquals(OctiPodType.POD_UNDEF, pod.getType());
        assertEquals(0, pod.getNumProngs());
        assertEquals("", pod.getConfigString());
    }

    @Test
    public void testOctiPodOwnerTypeManipulation() {
        OctiPod pod = new OctiPod(OctiPodType.B_POD);
        assertEquals(OctiPodType.B_POD, pod.getType());

        pod.setType(OctiPodType.C_POD);
        assertEquals(OctiPodType.C_POD, pod.getType());

        pod.setType(OctiPodType.D_POD);
        assertEquals(OctiPodType.D_POD, pod.getType());

        pod.setType(OctiPodType.A_POD);
        assertEquals(OctiPodType.A_POD, pod.getType());
    }

    @Test
    public void testOctiPodProngAddRemoveAndInspection() {
        OctiPod pod = new OctiPod(OctiPodType.A_POD);

        pod.addProng(OctiDir.DIR_N, false);
        pod.addProng(OctiDir.DIR_E, false);
        pod.addProng(OctiDir.DIR_S, false);

        assertEquals(OctiProngType.NORMAL_PRONG, pod.prongAt(OctiDir.DIR_N));
        assertEquals(OctiProngType.NO_PRONG, pod.prongAt(OctiDir.DIR_NE));
        assertEquals(OctiProngType.NORMAL_PRONG, pod.prongAt(OctiDir.DIR_E));
        assertEquals(OctiProngType.NORMAL_PRONG, pod.prongAt(OctiDir.DIR_S));
        assertEquals(3, pod.getNumProngs());
        assertEquals(3, pod.getNumNormalProngs());
        assertEquals(0, pod.getNumSuperProngs());
        assertEquals("ace", pod.getConfigString());

        pod.removeProng(OctiDir.DIR_E);
        assertEquals(OctiProngType.NO_PRONG, pod.prongAt(OctiDir.DIR_E));
        assertEquals(2, pod.getNumProngs());
        assertEquals("ae", pod.getConfigString());
    }

    @Test
    public void testOctiPodSuperProngHandling() {
        OctiPod pod = new OctiPod(OctiPodType.B_POD);

        pod.addProng(OctiDir.DIR_N, false);
        pod.addProng(OctiDir.DIR_E, true);
        assertTrue(pod.hasSuperProng());
        assertEquals(OctiProngType.SUPER_PRONG, pod.prongAt(OctiDir.DIR_E));
        assertEquals(2, pod.getNumProngs());
        assertEquals(1, pod.getNumNormalProngs());
        assertEquals(1, pod.getNumSuperProngs());
        assertEquals("ac!", pod.getConfigString());

        pod.removeProng(OctiDir.DIR_E);
        assertFalse(pod.hasSuperProng());
        assertEquals(1, pod.getNumProngs());
        assertEquals(1, pod.getNumNormalProngs());
        assertEquals(0, pod.getNumSuperProngs());
        assertEquals("a", pod.getConfigString());
    }

    @Test
    public void testOctiPodWeightComparisonAndStrictOrdering() {
        OctiPod p0 = new OctiPod(OctiPodType.A_POD);
        OctiPod p1 = new OctiPod(OctiPodType.A_POD);
        p1.addProng(OctiDir.DIR_N, false);
        OctiPod p1s = new OctiPod(OctiPodType.A_POD);
        p1s.addProng(OctiDir.DIR_N, true);
        OctiPod p2 = new OctiPod(OctiPodType.A_POD);
        p2.addProng(OctiDir.DIR_N, false);
        p2.addProng(OctiDir.DIR_E, false);

        assertTrue(p0.compareTo(p1) < 0);
        assertTrue(p1.compareTo(p1s) < 0);
        assertTrue(p1s.compareTo(p2) < 0);
        assertTrue(p0.compareTo(p1) <= 0);
        assertTrue(p2.compareTo(p1) > 0);
        assertTrue(p2.compareTo(p1s) >= 0);
        assertTrue(p1.equals(p1));
        assertFalse(p1.equals(p2));
    }

    @Test
    public void testOctiPodStringOutput() {
        OctiPod podA = new OctiPod(OctiPodType.A_POD);
        podA.addProng(OctiDir.DIR_N, false);
        podA.addProng(OctiDir.DIR_NE, true);
        assertEquals("Aab!", podA.toString());

        OctiPod undefPod = new OctiPod();
        assertEquals("?", undefPod.toString());
    }

    @Test
    public void testOctiPodBoundaryAllProngsSuperAndNormal() {
        OctiPod allNormal = new OctiPod(OctiPodType.A_POD);
        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            allNormal.addProng(OctiDir.fromIndex(d), false);
        }
        assertEquals(8, allNormal.getNumProngs());
        assertEquals(8, allNormal.getNumNormalProngs());
        assertEquals(0, allNormal.getNumSuperProngs());
        assertEquals("abcdefgh", allNormal.getConfigString());

        OctiPod allSuper = new OctiPod(OctiPodType.B_POD);
        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            allSuper.addProng(OctiDir.fromIndex(d), true);
        }
        assertEquals(8, allSuper.getNumProngs());
        assertEquals(0, allSuper.getNumNormalProngs());
        assertEquals(8, allSuper.getNumSuperProngs());
        assertEquals("a!b!c!d!e!f!g!h!", allSuper.getConfigString());
    }

    @Test
    public void testOctiPodPseudoRandomProngInvariants() {
        Random rand = new Random(42L);
        for (int iter = 0; iter < 500; iter++) {
            int ownerVal = rand.nextInt(5);
            OctiPodType owner = OctiPodType.fromValue(ownerVal);
            OctiPod pod = new OctiPod(owner);

            int normalCount = 0;
            int superCount = 0;

            for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                int pchoice = rand.nextInt(3);
                OctiDir dir = OctiDir.fromIndex(d);
                if (pchoice == 1) {
                    pod.addProng(dir, false);
                    normalCount++;
                } else if (pchoice == 2) {
                    pod.addProng(dir, true);
                    superCount++;
                }
            }

            assertEquals(owner, pod.getType());
            assertEquals(normalCount, pod.getNumNormalProngs());
            assertEquals(superCount, pod.getNumSuperProngs());
            assertEquals(normalCount + superCount, pod.getNumProngs());
            assertEquals(superCount > 0, pod.hasSuperProng());

            OctiPod pod2 = new OctiPod(pod.rawConfig());
            assertEquals(pod, pod2);
            assertEquals(pod.getConfigString(), pod2.getConfigString());
        }
    }

    // -------------------------------------------------------------------------
    // 2. OctiSquare Tests
    // -------------------------------------------------------------------------

    @Test
    public void testOctiSquareInitialConfigurationAndAccessors() {
        OctiSquare sq = new OctiSquare(OctiGameMode.GAME_MODE_2, OctiSquareType.A_HOME_SQUARE, 2, 2);
        assertEquals(OctiGameMode.GAME_MODE_2, sq.getGameMode());
        assertEquals(OctiSquareType.A_HOME_SQUARE, sq.getType());
        assertEquals(2, sq.getLocX());
        assertEquals(2, sq.getLocY());
        assertEquals(33, sq.getLocId());
        assertTrue(sq.isEmpty());
        assertEquals(0, sq.getNumPods());
    }

    @Test
    public void testOctiSquareAddRemovePodsStacking() {
        OctiSquare sq = new OctiSquare(OctiGameMode.GAME_MODE_2, OctiSquareType.NEUTRAL_SQUARE, 4, 4);

        OctiPod pod1 = new OctiPod(OctiPodType.A_POD);
        pod1.addProng(OctiDir.DIR_N, false);
        OctiPod pod2 = new OctiPod(OctiPodType.A_POD);
        pod2.addProng(OctiDir.DIR_E, false);

        sq.addPod(pod1);
        sq.addPod(pod2);

        assertFalse(sq.isEmpty());
        assertEquals(2, sq.getNumPods());
        assertTrue(sq.containsPod(pod1));
        assertTrue(sq.containsPod(pod2));
        assertEquals(pod1, sq.getPodAt(0));
        assertEquals(pod2, sq.getPodAt(1));

        assertTrue(sq.removePod(pod2));
        assertEquals(1, sq.getNumPods());
        assertFalse(sq.containsPod(pod2));
        assertTrue(sq.containsPod(pod1));

        assertFalse(sq.removePod(pod2));
    }

    @Test
    public void testOctiSquareEmptyWithoutGhostPods() {
        OctiSquare sq = new OctiSquare(OctiGameMode.GAME_MODE_2, OctiSquareType.NEUTRAL_SQUARE, 3, 3);
        OctiPod pA = new OctiPod(OctiPodType.A_POD);
        OctiPod pB = new OctiPod(OctiPodType.A_POD);
        pB.addProng(OctiDir.DIR_N, false);

        sq.addPod(pA);
        sq.addPod(pB);

        assertFalse(sq.isEmptyWithout(Collections.emptyList()));
        assertFalse(sq.isEmptyWithout(Collections.singletonList(pA)));
        assertTrue(sq.isEmptyWithout(Arrays.asList(pA, pB)));

        OctiPod pC = new OctiPod(OctiPodType.B_POD);
        assertFalse(sq.isEmptyWithout(Arrays.asList(pA, pC)));
    }

    @Test
    public void testOctiSquareIsCapturedMode2() {
        OctiSquare aHome = new OctiSquare(OctiGameMode.GAME_MODE_2, OctiSquareType.A_HOME_SQUARE, 2, 2);
        assertFalse(aHome.isCaptured());

        OctiPod podA = new OctiPod(OctiPodType.A_POD);
        aHome.addPod(podA);
        assertFalse(aHome.isCaptured());
        assertFalse(aHome.isCapturedBy(OctiPlayer.PLAYER_B));

        OctiPod podB = new OctiPod(OctiPodType.B_POD);
        aHome.addPod(podB);
        assertTrue(aHome.isCaptured());
        assertTrue(aHome.isCapturedBy(OctiPlayer.PLAYER_B));
        assertFalse(aHome.isCapturedBy(OctiPlayer.PLAYER_A));

        OctiSquare neutralSq = new OctiSquare(OctiGameMode.GAME_MODE_2, OctiSquareType.NEUTRAL_SQUARE, 4, 4);
        neutralSq.addPod(podB);
        assertFalse(neutralSq.isCaptured());
    }

    @Test
    public void testOctiSquareIsCapturedMode4() {
        OctiSquare aHome = new OctiSquare(OctiGameMode.GAME_MODE_4, OctiSquareType.A_HOME_SQUARE, 2, 2);

        OctiPod podC = new OctiPod(OctiPodType.C_POD);
        aHome.addPod(podC);
        assertFalse(aHome.isCaptured());

        OctiPod podD = new OctiPod(OctiPodType.D_POD);
        aHome.addPod(podD);
        assertTrue(aHome.isCaptured());
        assertTrue(aHome.isCapturedBy(OctiPlayer.PLAYER_D));
        assertTrue(aHome.isCapturedByTeamOf(OctiPlayer.PLAYER_B));
        assertFalse(aHome.isCapturedByTeamOf(OctiPlayer.PLAYER_A));
    }

    @Test
    public void testOctiSquareIsCapturedModeOFK() {
        OctiSquare ofkSq = new OctiSquare(OctiGameMode.GAME_MODE_OFK, OctiSquareType.B_HOME_SQUARE, 1, 5);
        OctiPod podA = new OctiPod(OctiPodType.A_POD);
        ofkSq.addPod(podA);

        assertTrue(ofkSq.isCaptured());
        assertTrue(ofkSq.isCapturedBy(OctiPlayer.PLAYER_A));
    }

    @Test
    public void testOctiSquareHasEnemyOfAndIsEnemyOf() {
        OctiSquare sq = new OctiSquare(OctiGameMode.GAME_MODE_2, OctiSquareType.B_HOME_SQUARE, 2, 6);
        assertTrue(sq.isEnemyOf(OctiPlayer.PLAYER_A));
        assertFalse(sq.isEnemyOf(OctiPlayer.PLAYER_B));

        OctiPod podB = new OctiPod(OctiPodType.B_POD);
        sq.addPod(podB);
        assertTrue(sq.hasEnemyOf(OctiPlayer.PLAYER_A));
        assertFalse(sq.hasEnemyOf(OctiPlayer.PLAYER_B));
    }

    @Test
    public void testOctiSquareHasSameMaterialsAs() {
        OctiSquare s1 = new OctiSquare(OctiGameMode.GAME_MODE_2, OctiSquareType.NEUTRAL_SQUARE, 0, 0);
        OctiSquare s2 = new OctiSquare(OctiGameMode.GAME_MODE_2, OctiSquareType.NEUTRAL_SQUARE, 1, 1);

        OctiPod p1 = new OctiPod(OctiPodType.A_POD);
        OctiPod p2 = new OctiPod(OctiPodType.B_POD);

        s1.addPod(p1);
        s1.addPod(p2);

        s2.addPod(p2);
        s2.addPod(p1);

        assertTrue(s1.hasSameMaterialsAs(s2));

        s2.addPod(p1);
        assertFalse(s1.hasSameMaterialsAs(s2));
    }

    // -------------------------------------------------------------------------
    // 3. OctiMove Tests
    // -------------------------------------------------------------------------

    @Test
    public void testOctiMoveParsePodPlacementAndExplicitLiberation() {
        OctiMove m1 = new OctiMove("33");
        assertEquals(1, m1.size());
        OctiMoveOp op1 = m1.getOps().get(0);
        assertEquals(2, op1.x0);
        assertEquals(2, op1.y0);
        assertTrue(op1.addPod);
        assertFalse(op1.liberate);
        assertEquals("33", m1.asString());

        OctiMove m2 = new OctiMove("37*");
        assertEquals(1, m2.size());
        OctiMoveOp op2 = m2.getOps().get(0);
        assertEquals(2, op2.x0);
        assertEquals(6, op2.y0);
        assertTrue(op2.addPod);
        assertTrue(op2.liberate);
        assertEquals("37*", m2.asString());
    }

    @Test
    public void testOctiMoveParseProngAddNormalAndSuper() {
        OctiMove m1 = new OctiMove("33+b");
        assertEquals(1, m1.size());
        OctiMoveOp op1 = m1.getOps().get(0);
        assertEquals(OctiDir.DIR_NE, op1.addProngDir);
        assertFalse(op1.superProng);
        assertEquals(0, op1.pod.getNumProngs());
        assertEquals("33+b", m1.asString());

        OctiMove m2 = new OctiMove("33b+a!");
        assertEquals(1, m2.size());
        OctiMoveOp op2 = m2.getOps().get(0);
        assertEquals(OctiDir.DIR_N, op2.addProngDir);
        assertTrue(op2.superProng);
        assertEquals(1, op2.pod.getNumProngs());
        assertEquals(OctiProngType.NORMAL_PRONG, op2.pod.prongAt(OctiDir.DIR_NE));
        assertEquals("33b+a!", m2.asString());
    }

    @Test
    public void testOctiMoveParseProngRepositionCanonicalAndReversed() {
        OctiMove m1 = new OctiMove("33a+b-a");
        assertEquals(1, m1.size());
        OctiMoveOp op1 = m1.getOps().get(0);
        assertEquals(OctiDir.DIR_NE, op1.addProngDir);
        assertEquals(OctiDir.DIR_N, op1.rmProngDir);
        assertEquals("33a+b-a", m1.asString());

        OctiMove m2 = new OctiMove("33a-a+b");
        assertEquals(1, m2.size());
        OctiMoveOp op2 = m2.getOps().get(0);
        assertEquals(OctiDir.DIR_NE, op2.addProngDir);
        assertEquals(OctiDir.DIR_N, op2.rmProngDir);
        assertEquals("33a+b-a", m2.asString());
    }

    @Test
    public void testOctiMoveParseSingleStepMoveAndMultiHopJump() {
        OctiMove m1 = new OctiMove("33a-34");
        assertEquals(1, m1.size());
        OctiMoveOp op1 = m1.getOps().get(0);
        assertEquals(1, op1.hops.size());
        assertEquals(2, op1.hops.get(0).x);
        assertEquals(3, op1.hops.get(0).y);
        assertFalse(op1.hops.get(0).capture);
        assertEquals("33a-34", m1.asString());

        OctiMove m2 = new OctiMove("33a-35x");
        assertEquals(1, m2.size());
        OctiMoveOp op2 = m2.getOps().get(0);
        assertEquals(1, op2.hops.size());
        assertEquals(2, op2.hops.get(0).x);
        assertEquals(4, op2.hops.get(0).y);
        assertTrue(op2.hops.get(0).capture);
        assertEquals("33a-35x", m2.asString());

        OctiMove m3 = new OctiMove("33ac-35x-55");
        assertEquals(1, m3.size());
        OctiMoveOp op3 = m3.getOps().get(0);
        assertEquals(2, op3.hops.size());
        assertEquals(2, op3.hops.get(0).x);
        assertEquals(4, op3.hops.get(0).y);
        assertTrue(op3.hops.get(0).capture);
        assertEquals(4, op3.hops.get(1).x);
        assertEquals(4, op3.hops.get(1).y);
        assertFalse(op3.hops.get(1).capture);
        assertEquals("33ac-35x-55", m3.asString());
    }

    @Test
    public void testOctiMoveParseCompositeStackMoves() {
        OctiMove m = new OctiMove("37df-46,37e-36");
        assertEquals(2, m.size());
        assertEquals(2, m.getOps().get(0).x0);
        assertEquals(6, m.getOps().get(0).y0);
        assertEquals(2, m.getOps().get(1).x0);
        assertEquals(6, m.getOps().get(1).y0);
        assertEquals("37df-46,37e-36", m.asString());
    }

    @Test
    public void testOctiMoveParseSuperprongBonusMovePrelude() {
        OctiMove m = new OctiMove("33a!-34;34a!+b");
        assertEquals(2, m.size());
        assertTrue(m.getOps().get(0).isSuperProngBonusMove);
        assertFalse(m.getOps().get(1).isSuperProngBonusMove);
        assertEquals("33a!-34;34a!+b", m.asString());

        OctiMove m2 = new OctiMove("33a!-34;34a!-35;35a+c");
        assertEquals(3, m2.size());
        assertTrue(m2.getOps().get(0).isSuperProngBonusMove);
        assertTrue(m2.getOps().get(1).isSuperProngBonusMove);
        assertFalse(m2.getOps().get(2).isSuperProngBonusMove);
        assertEquals("33a!-34;34a!-35;35a+c", m2.asString());
    }

    @Test
    public void testOctiMoveConstructFromOpsList() {
        OctiMoveOp op = new OctiMoveOp();
        op.x0 = 2;
        op.y0 = 2;
        op.pod.addProng(OctiDir.DIR_N, false);
        op.hops.add(new OctiHop(2, 3, false));

        OctiMove move = new OctiMove(Collections.singletonList(op));
        assertEquals(1, move.size());
        assertEquals("33a-34", move.asString());
    }

    @Test
    public void testOctiMoveFaultNullAndEmptyString() {
        assertThrowsIllegalMoveSyntax(() -> new OctiMove((String) null));
        assertThrowsIllegalMoveSyntax(() -> new OctiMove(""));
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("   \t  "));
    }

    @Test
    public void testOctiMoveFaultInvalidCoordinates() {
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("00"));
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("90"));
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("3a"));
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("3"));
    }

    @Test
    public void testOctiMoveFaultDuplicateProngsInConfig() {
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("33aa+b"));
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("33a!a+b"));
    }

    @Test
    public void testOctiMoveFaultProngAddAlreadyExisting() {
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("33a+a"));
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("33a!+a"));
    }

    @Test
    public void testOctiMoveFaultProngRepositionIllegalOperations() {
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("33a+a-a"));
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("33a+b-c"));
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("33a!+b-a"));
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("33a+b!-a"));
    }

    @Test
    public void testOctiMoveFaultCompositeMismatchSourceCoordinates() {
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("33a-34,34a-35"));
    }

    @Test
    public void testOctiMoveFaultCompositeMixingProngOrPodOps() {
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("33a-34,33+b"));
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("33,33a-34"));
    }

    @Test
    public void testOctiMoveFaultSuperprongPreludeSyntaxErrors() {
        assertThrowsIllegalMoveSyntax(() -> new OctiMove(";33+b"));
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("33a!-34;"));
        assertThrowsIllegalMoveSyntax(() -> new OctiMove("33a!-34;34a!-35;35a!-36;36a+b"));
    }

    // -------------------------------------------------------------------------
    // 4. OctiBoard Tests
    // -------------------------------------------------------------------------

    @Test
    public void testOctiBoardDimensionsAndModes() {
        OctiBoard b2 = new OctiBoard(OctiGameMode.GAME_MODE_2);
        assertEquals(9, b2.getBoardWidth());
        assertEquals(9, b2.getBoardHeight());
        assertEquals(OctiGameMode.GAME_MODE_2, b2.getGameMode());

        OctiBoard b4 = new OctiBoard(OctiGameMode.GAME_MODE_4);
        assertEquals(9, b4.getBoardWidth());
        assertEquals(9, b4.getBoardHeight());
        assertEquals(OctiGameMode.GAME_MODE_4, b4.getGameMode());

        OctiBoard bOfk = new OctiBoard(OctiGameMode.GAME_MODE_OFK);
        assertEquals(6, bOfk.getBoardWidth());
        assertEquals(7, bOfk.getBoardHeight());
        assertEquals(OctiGameMode.GAME_MODE_OFK, bOfk.getGameMode());
    }

    @Test
    public void testOctiBoardSquareTypesAndCoordinates() {
        OctiBoard b2 = new OctiBoard(OctiGameMode.GAME_MODE_2);
        assertEquals(OctiSquareType.A_HOME_SQUARE, b2.squareAt(2, 2).getType());
        assertEquals(OctiSquareType.A_HOME_SQUARE, b2.squareAt(4, 2).getType());
        assertEquals(OctiSquareType.A_HOME_SQUARE, b2.squareAt(6, 2).getType());
        assertEquals(OctiSquareType.B_HOME_SQUARE, b2.squareAt(2, 6).getType());
        assertEquals(OctiSquareType.B_HOME_SQUARE, b2.squareAt(4, 6).getType());
        assertEquals(OctiSquareType.B_HOME_SQUARE, b2.squareAt(6, 6).getType());
        assertEquals(OctiSquareType.NEUTRAL_SQUARE, b2.squareAt(0, 0).getType());

        assertThrowsIllegalMove(() -> b2.squareAt(-1, 0));
        assertThrowsIllegalMove(() -> b2.squareAt(9, 9));

        OctiBoard b4 = new OctiBoard(OctiGameMode.GAME_MODE_4);
        assertEquals(OctiSquareType.A_HOME_SQUARE, b4.squareAt(2, 2).getType());
        assertEquals(OctiSquareType.B_HOME_SQUARE, b4.squareAt(2, 6).getType());
        assertEquals(OctiSquareType.C_HOME_SQUARE, b4.squareAt(6, 6).getType());
        assertEquals(OctiSquareType.D_HOME_SQUARE, b4.squareAt(6, 2).getType());
        assertEquals(OctiSquareType.NEUTRAL_OCTI_SQUARE, b4.squareAt(4, 2).getType());
        assertEquals(OctiSquareType.NEUTRAL_OCTI_SQUARE, b4.squareAt(4, 6).getType());

        OctiBoard bOfk = new OctiBoard(OctiGameMode.GAME_MODE_OFK);
        assertEquals(OctiSquareType.A_HOME_SQUARE, bOfk.squareAt(1, 1).getType());
        assertEquals(OctiSquareType.A_HOME_SQUARE, bOfk.squareAt(4, 1).getType());
        assertEquals(OctiSquareType.B_HOME_SQUARE, bOfk.squareAt(1, 5).getType());
        assertEquals(OctiSquareType.B_HOME_SQUARE, bOfk.squareAt(4, 5).getType());
        assertThrowsIllegalMove(() -> bOfk.squareAt(6, 0));
        assertThrowsIllegalMove(() -> bOfk.squareAt(0, 7));
    }

    @Test
    public void testOctiBoardCanLiberateLogic() {
        OctiBoard b2 = new OctiBoard(OctiGameMode.GAME_MODE_2);
        assertFalse(b2.canLiberate(OctiPlayer.PLAYER_A));
        assertFalse(b2.canLiberate(OctiPlayer.PLAYER_B));

        b2.squareAt(2, 6).addPod(new OctiPod(OctiPodType.A_POD));
        assertTrue(b2.canLiberate(OctiPlayer.PLAYER_A));
        assertFalse(b2.canLiberate(OctiPlayer.PLAYER_B));

        OctiBoard bOfk = new OctiBoard(OctiGameMode.GAME_MODE_OFK);
        bOfk.squareAt(1, 5).addPod(new OctiPod(OctiPodType.A_POD));
        assertFalse(bOfk.canLiberate(OctiPlayer.PLAYER_A));

        OctiBoard b4 = new OctiBoard(OctiGameMode.GAME_MODE_4);
        assertFalse(b4.canLiberate(OctiPlayer.PLAYER_A));
        b4.squareAt(4, 2).addPod(new OctiPod(OctiPodType.A_POD));
        assertTrue(b4.canLiberate(OctiPlayer.PLAYER_A));
    }

    @Test
    public void testOctiBoardAsciiStreamOutput() {
        OctiBoard b2 = new OctiBoard(OctiGameMode.GAME_MODE_2);
        String sOcc = b2.toString();
        assertTrue(sOcc.contains("A:"));

        b2.clear();
        String s2 = b2.toString();
        assertTrue(s2.contains("(A)"));
        assertTrue(s2.contains("(B)"));

        OctiBoard bOfk = new OctiBoard(OctiGameMode.GAME_MODE_OFK);
        String sOfk = bOfk.toString();
        assertTrue(sOfk.contains("6"));
    }

    // -------------------------------------------------------------------------
    // 5. Octi2PlayerGame Tests
    // -------------------------------------------------------------------------

    @Test
    public void testOcti2PlayerGameResetAndInitialReserves() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.reset();
        game.start(OctiPlayer.PLAYER_A);

        assertTrue(game.isGameStarted());
        assertFalse(game.isGameOver());
        assertEquals(OctiPlayer.PLAYER_A, game.getTurn());
        assertEquals(4, game.getFreePods(OctiPlayer.PLAYER_A));
        assertEquals(4, game.getFreePods(OctiPlayer.PLAYER_B));
        assertEquals(0, game.getCapturedPods(OctiPlayer.PLAYER_A));
        assertEquals(0, game.getCapturedPods(OctiPlayer.PLAYER_B));
        assertEquals(25, game.getFreeProngs(OctiPlayer.PLAYER_A));
        assertEquals(25, game.getFreeProngs(OctiPlayer.PLAYER_B));
        assertEquals(0, game.getFreeSuperProngs(OctiPlayer.PLAYER_A));
        assertEquals(0, game.getFreeSuperProngs(OctiPlayer.PLAYER_B));
    }

    @Test
    public void testOcti2PlayerGameTurnOptionI_AddNormalAndSuperProng() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, true, false, OctiGameBases.GAME_BASES_3);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        assertEquals(24, game.getFreeProngs(OctiPlayer.PLAYER_A));
        assertEquals(1, game.getFreeSuperProngs(OctiPlayer.PLAYER_A));

        game.move("33+b");
        assertEquals(23, game.getFreeProngs(OctiPlayer.PLAYER_A));
        assertEquals(OctiPlayer.PLAYER_B, game.getTurn());
        assertEquals(OctiProngType.NORMAL_PRONG, board.squareAt(2, 2).getPodAt(0).prongAt(OctiDir.DIR_NE));

        game.move("37+e!");
        assertEquals(0, game.getFreeSuperProngs(OctiPlayer.PLAYER_B));
        assertEquals(OctiPlayer.PLAYER_A, game.getTurn());
        assertEquals(OctiProngType.SUPER_PRONG, board.squareAt(2, 6).getPodAt(0).prongAt(OctiDir.DIR_S));
    }

    @Test
    public void testOcti2PlayerGameTurnOptionII_RepositionNormalProng() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        game.move("33+b");
        game.move("37+e");
        game.move("33b+c-b");
        assertEquals(OctiProngType.NO_PRONG, board.squareAt(2, 2).getPodAt(0).prongAt(OctiDir.DIR_NE));
        assertEquals(OctiProngType.NORMAL_PRONG, board.squareAt(2, 2).getPodAt(0).prongAt(OctiDir.DIR_E));
    }

    @Test
    public void testOcti2PlayerGameTurnOptionII_RejectSuperProngReposition() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, true, false, OctiGameBases.GAME_BASES_3);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        game.move("33+b!");
        game.move("37+e");

        assertThrowsIllegalMoveSyntax(() -> game.move("33b!+c-b"));
    }

    @Test
    public void testOcti2PlayerGameTurnOptionIII_Distance1Move() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        game.move("33+a");
        game.move("37+e");
        game.move("33a-34");
        assertEquals(0, board.squareAt(2, 2).getNumPods());
        assertEquals(1, board.squareAt(2, 3).getNumPods());

        game.move("57+d");
        assertThrowsIllegalMove(() -> game.move("34a-35x"));
    }

    @Test
    public void testOcti2PlayerGameTurnOptionIV_Distance2JumpAndCapture() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        game.move("33+a");
        game.move("37+e");
        game.move("53+c");
        game.move("37e-36");
        game.move("53c+b");
        game.move("36e-35");
        game.move("53bc+d");
        game.move("35e-34");

        assertEquals(1, board.squareAt(2, 3).getNumPods());
        assertEquals(0, game.getCapturedPods(OctiPlayer.PLAYER_B));
        int prongsBefore = game.getFreeProngs(OctiPlayer.PLAYER_A);

        game.move("33a-35x");
        assertEquals(0, board.squareAt(2, 3).getNumPods());
        assertEquals(1, board.squareAt(2, 4).getNumPods());
        assertEquals(1, game.getCapturedPods(OctiPlayer.PLAYER_B));
        assertEquals(prongsBefore + 1, game.getFreeProngs(OctiPlayer.PLAYER_A));
    }

    @Test
    public void testOcti2PlayerGameTurnOptionV_ReservePodAndLiberation() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        assertEquals(4, game.getFreePods(OctiPlayer.PLAYER_A));
        game.move("33");
        assertEquals(3, game.getFreePods(OctiPlayer.PLAYER_A));
        assertEquals(2, board.squareAt(2, 2).getNumPods());

        board.squareAt(2, 6).clear();
        board.squareAt(2, 6).addPod(new OctiPod(OctiPodType.A_POD));
        OctiGameSnapshot snap = game.captureSnapshot();
        snap.players[OctiPlayer.PLAYER_A.getValue()].capturedPods = 1;
        game.restoreSnapshot(snap);

        game.move("57+e");
        game.move("37*");
        assertEquals(0, game.getCapturedPods(OctiPlayer.PLAYER_A));
    }

    @Test
    public void testOcti2PlayerGameUndoRedoStack() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        String fen0 = board.toPositionString();
        game.move("33+b");
        String fen1 = board.toPositionString();
        game.move("37+e");
        String fen2 = board.toPositionString();

        assertTrue(game.canUndo());
        game.undo();
        assertEquals(fen1, board.toPositionString());

        game.undo();
        assertEquals(fen0, board.toPositionString());
        assertFalse(game.canUndo());

        assertTrue(game.canRedo());
        game.redo();
        assertEquals(fen1, board.toPositionString());

        game.redo();
        assertEquals(fen2, board.toPositionString());
        assertFalse(game.canRedo());
    }

    @Test
    public void testOcti2PlayerGameVictoryConditions() {
        OctiBoard b1 = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false, OctiGameBases.GAME_BASES_1);
        OctiGame g1 = b1.getGame();
        g1.start(OctiPlayer.PLAYER_A);

        b1.squareAt(2, 6).addPod(new OctiPod(OctiPodType.A_POD));
        g1.move("33+b");
        assertTrue(g1.isGameOver());
        assertEquals(OctiPlayer.PLAYER_A, g1.getWinner());

        OctiBoard bResign = new OctiBoard(OctiGameMode.GAME_MODE_2);
        OctiGame gResign = bResign.getGame();
        gResign.start(OctiPlayer.PLAYER_A);
        gResign.resign(OctiPlayer.PLAYER_A);
        assertTrue(gResign.isGameOver());
        assertTrue(gResign.isGameResigned());
        assertEquals(OctiPlayer.PLAYER_B, gResign.getWinner());
    }

    @Test
    public void testOcti2PlayerGamePreparatoryWorkflow() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        assertFalse(game.hasPreparedMove());
        game.prepareAddProng(2, 2, new OctiPod(OctiPodType.A_POD), OctiDir.DIR_NE, false);
        assertTrue(game.hasPreparedMove());
        assertEquals("33+b", game.getPreparedMoveString());

        game.rollbackPreparedMove();
        assertFalse(game.hasPreparedMove());
        assertEquals("", game.getPreparedMoveString());

        game.prepareAddProng(2, 2, new OctiPod(OctiPodType.A_POD), OctiDir.DIR_NE, false);
        game.commitPreparedMove();
        assertFalse(game.hasPreparedMove());
        assertEquals(OctiPlayer.PLAYER_B, game.getTurn());
        assertEquals(1, game.getNumMoves());
    }

    // -------------------------------------------------------------------------
    // 6. Octi4PlayerGame Tests
    // -------------------------------------------------------------------------

    @Test
    public void testOcti4PlayerGameSetupAndCyclicTurns() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_4);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        assertEquals(OctiPlayer.PLAYER_A, game.getTurn());
        game.move("33+a");
        assertEquals(OctiPlayer.PLAYER_B, game.getTurn());
        game.move("37+e");
        assertEquals(OctiPlayer.PLAYER_C, game.getTurn());
        game.move("77+e");
        assertEquals(OctiPlayer.PLAYER_D, game.getTurn());
        game.move("73+a");
        assertEquals(OctiPlayer.PLAYER_A, game.getTurn());
    }

    @Test
    public void testOcti4PlayerGameAlliedStacking() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_4);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        board.squareAt(2, 2).addPod(new OctiPod(OctiPodType.C_POD));
        assertEquals(2, board.squareAt(2, 2).getNumPods());
        assertFalse(board.squareAt(2, 2).hasEnemyOf(OctiPlayer.PLAYER_A));
        assertFalse(board.squareAt(2, 2).hasEnemyOf(OctiPlayer.PLAYER_C));
    }

    @Test
    public void testOcti4PlayerGameHomeBaseEliminationAndTurnSkipping() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_4);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        board.squareAt(2, 6).addPod(new OctiPod(OctiPodType.A_POD));
        game.move("33+b");

        assertTrue(game.isPlayerEliminated(OctiPlayer.PLAYER_B));
        assertFalse(game.isPlayerEliminated(OctiPlayer.PLAYER_A));
        assertFalse(game.isPlayerEliminated(OctiPlayer.PLAYER_C));
        assertFalse(game.isPlayerEliminated(OctiPlayer.PLAYER_D));

        assertEquals(OctiPlayer.PLAYER_C, game.getTurn());
    }

    // -------------------------------------------------------------------------
    // 7. OctiForKidsGame Tests
    // -------------------------------------------------------------------------

    @Test
    public void testOctiForKidsGameDimensionsAndPodSetup() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_OFK);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        assertEquals(6, board.getBoardWidth());
        assertEquals(7, board.getBoardHeight());
        assertEquals(0, game.getFreePods(OctiPlayer.PLAYER_A));
        assertEquals(12, game.getFreeProngs(OctiPlayer.PLAYER_A));
        assertEquals(0, game.getFreeSuperProngs(OctiPlayer.PLAYER_A));

        assertEquals(1, board.squareAt(1, 1).getNumPods());
        assertEquals(1, board.squareAt(2, 1).getNumPods());
        assertEquals(1, board.squareAt(3, 1).getNumPods());
        assertEquals(1, board.squareAt(4, 1).getNumPods());
        assertEquals(1, board.squareAt(1, 5).getNumPods());
        assertEquals(1, board.squareAt(2, 5).getNumPods());
        assertEquals(1, board.squareAt(3, 5).getNumPods());
        assertEquals(1, board.squareAt(4, 5).getNumPods());
    }

    @Test
    public void testOctiForKidsGameDisallowPodPlacementAndReposition() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_OFK);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        assertThrowsIllegalMove(() -> game.move("22"));

        game.move("22+a");
        game.move("26+e");
        assertThrowsIllegalMove(() -> game.move("22a+b-a"));
    }

    @Test
    public void testOctiForKidsGameDisallowStacking() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_OFK);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        game.move("22+c");
        game.move("26+e");
        assertThrowsIllegalMove(() -> game.move("22c-32"));
    }

    @Test
    public void testOctiForKidsGameSingleBaseVictory() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_OFK);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        board.squareAt(1, 5).addPod(new OctiPod(OctiPodType.A_POD));
        game.move("22+a");
        assertTrue(game.isGameOver());
        assertEquals(OctiPlayer.PLAYER_A, game.getWinner());
    }

    // -------------------------------------------------------------------------
    // 8. OctiRulebookExamples Tests
    // -------------------------------------------------------------------------

    @Test
    public void testOctiRulebookExamplesExample1_MovingStackedPods() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        board.squareAt(2, 2).clear();
        OctiPod pBottom = new OctiPod(OctiPodType.A_POD);
        pBottom.addProng(OctiDir.DIR_NW, false);
        pBottom.addProng(OctiDir.DIR_E, false);
        OctiPod pMiddle = new OctiPod(OctiPodType.A_POD);
        pMiddle.addProng(OctiDir.DIR_N, false);
        OctiPod pTop = new OctiPod(OctiPodType.A_POD);
        pTop.addProng(OctiDir.DIR_NW, false);

        board.squareAt(2, 2).addPod(pBottom);
        board.squareAt(2, 2).addPod(pMiddle);
        board.squareAt(2, 2).addPod(pTop);

        game.move("33h-24,33a-34");
        assertEquals(1, board.squareAt(1, 3).getNumPods());
        assertEquals(1, board.squareAt(2, 3).getNumPods());
        assertEquals(1, board.squareAt(2, 2).getNumPods());
    }

    @Test
    public void testOctiRulebookExamplesExample2_JumpingWithStackedPods() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        board.clear();
        OctiPod pBottom = new OctiPod(OctiPodType.A_POD);
        pBottom.addProng(OctiDir.DIR_NW, false);
        pBottom.addProng(OctiDir.DIR_E, false);
        OctiPod pTop = new OctiPod(OctiPodType.A_POD);
        pTop.addProng(OctiDir.DIR_N, false);

        board.squareAt(2, 0).addPod(pBottom);
        board.squareAt(2, 0).addPod(pTop);

        board.squareAt(1, 1).addPod(new OctiPod(OctiPodType.B_POD));
        board.squareAt(2, 1).addPod(new OctiPod(OctiPodType.A_POD));
        board.squareAt(1, 2).addPod(new OctiPod(OctiPodType.A_POD));
        board.squareAt(2, 3).addPod(new OctiPod(OctiPodType.B_POD));

        game.move("31ch-13-33,31a-33-35");
        assertEquals(1, board.squareAt(2, 2).getNumPods());
        assertEquals(1, board.squareAt(2, 4).getNumPods());
    }

    @Test
    public void testOctiRulebookExamplesExample3_EdgelessMovesAndJumps() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, false, true, OctiGameBases.GAME_BASES_3);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        board.clear();
        OctiPod p1 = new OctiPod(OctiPodType.A_POD);
        p1.addProng(OctiDir.DIR_NW, false);
        board.squareAt(0, 8).addPod(p1);

        game.move("19h-91");
        assertEquals(0, board.squareAt(0, 8).getNumPods());
        assertEquals(1, board.squareAt(8, 0).getNumPods());
    }

    @Test
    public void testOctiRulebookExamplesRandomize18PlySequence() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.randomize();

        assertEquals(18, game.getNumMoves());
        assertTrue(game.isGameStarted());
        assertFalse(game.isGameOver());
        assertEquals(OctiPlayer.PLAYER_A, game.getTurn());
    }

    // -------------------------------------------------------------------------
    // 9. OctiSerialization Tests
    // -------------------------------------------------------------------------

    @Test
    public void testOctiSerializationOctiFenRoundTrip2Player() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        String fenStart = board.toPositionString();
        assertEquals("2:3:0:0:P:A:-:0:4,0,25,0:4,0,25,0:A33,A53,A73,B37,B57,B77", fenStart);

        OctiBoard checkBoard = new OctiBoard();
        checkBoard.fromPositionString(fenStart);
        assertEquals(fenStart, checkBoard.toPositionString());
    }

    @Test
    public void testOctiSerializationOctiFenRoundTripOFK() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_OFK);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        String fenStart = board.toPositionString();
        assertEquals("OFK:1:0:0:P:A:-:0:0,0,12,0:0,0,12,0:A22,A32,A42,A52,B26,B36,B46,B56", fenStart);

        OctiBoard checkBoard = new OctiBoard();
        checkBoard.fromPositionString(fenStart);
        assertEquals(fenStart, checkBoard.toPositionString());
    }

    @Test
    public void testOctiSerializationGameTranscriptMinimalAndAnnotated() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        game.move("33+b");
        game.move("37+e");
        game.move("33");
        game.move("37");

        String minimal = board.serializeGame();
        assertTrue(minimal.contains("MOVES: 4"));

        OctiBoard replayMinimal = new OctiBoard();
        replayMinimal.deserializeGame(minimal);
        assertEquals(4, replayMinimal.getGame().getNumMoves());
        assertEquals(board.toPositionString(), replayMinimal.toPositionString());

        String annotated =
                "; OCTI Game Transcript v2.0\n"
                + "GAME_MODE: 2\n"
                + "GAME_BASES: 3\n"
                + "USE_SUPER_PRONGS: No\n"
                + "USE_EDGELESS_BOARD: No\n"
                + "START_PLAYER: A\n"
                + "STATUS: IN_PROGRESS\n"
                + "WINNER: NONE\n"
                + "EVENT: Test Tournament\n"
                + "TIME_CONTROL: 600+5\n"
                + "MOVES: 4\n"
                + "1. A 33+b { clk=600000 emt=1200 eval=0.15 depth=12 }\n"
                + "1. B 37+e { clk=598500 emt=2100 eval=-0.10 depth=11 }\n"
                + "2. A 33 { clk=595200 emt=800 }\n"
                + "2. B 37 { clk=596000 emt=1400 }\n";

        OctiBoard replayAnnotated = new OctiBoard();
        replayAnnotated.deserializeGame(annotated);
        assertEquals(4, replayAnnotated.getGame().getNumMoves());
        assertEquals(board.toPositionString(), replayAnnotated.toPositionString());
    }

    // -------------------------------------------------------------------------
    // 10. Randomized Multi-Ply Walk Tests (50 games across 2P, 4P, OFK)
    // -------------------------------------------------------------------------

    @Test
    public void testRandomLegalMoveWalk2P() {
        Random rand = new Random(42L);
        for (int gameIdx = 0; gameIdx < 50; gameIdx++) {
            OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2);
            OctiGame game = board.getGame();
            assertNotNull(game);
            game.start(OctiPlayer.PLAYER_A);

            String initialFen = board.toPositionString();
            int numSteps = 5 + rand.nextInt(12);

            for (int step = 0; step < numSteps; step++) {
                if (game.isGameOver()) {
                    break;
                }
                List<OctiMove> legalMoves = game.generateLegalMoves();
                if (legalMoves.isEmpty()) {
                    break;
                }
                int moveIdx = rand.nextInt(legalMoves.size());
                OctiMove selected = legalMoves.get(moveIdx);

                game.move(selected);

                String fenAfter = board.toPositionString();
                OctiBoard checkBoard = new OctiBoard();
                checkBoard.fromPositionString(fenAfter);
                assertEquals(fenAfter, checkBoard.toPositionString());
            }

            String transcript = board.serializeGame();
            OctiBoard replayBoard = new OctiBoard();
            replayBoard.deserializeGame(transcript);
            assertEquals(board.toPositionString(), replayBoard.toPositionString());
            assertEquals(game.getNumMoves(), replayBoard.getGame().getNumMoves());
            assertEquals(game.getTurn(), replayBoard.getGame().getTurn());

            int movesMade = game.getNumMoves();
            for (int u = 0; u < movesMade; u++) {
                game.undo();
            }
            assertEquals(initialFen, board.toPositionString());

            for (int r = 0; r < movesMade; r++) {
                game.redo();
            }
            assertEquals(replayBoard.toPositionString(), board.toPositionString());
        }
    }

    @Test
    public void testRandomLegalMoveWalk4P() {
        Random rand = new Random(42L);
        for (int gameIdx = 0; gameIdx < 50; gameIdx++) {
            OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_4);
            OctiGame game = board.getGame();
            assertNotNull(game);
            game.start(OctiPlayer.PLAYER_A);

            String initialFen = board.toPositionString();
            int numSteps = 4 + rand.nextInt(9);

            for (int step = 0; step < numSteps; step++) {
                if (game.isGameOver()) {
                    break;
                }
                List<OctiMove> legalMoves = game.generateLegalMoves();
                if (legalMoves.isEmpty()) {
                    break;
                }
                int moveIdx = rand.nextInt(legalMoves.size());
                OctiMove selected = legalMoves.get(moveIdx);

                game.move(selected);

                String fenAfter = board.toPositionString();
                OctiBoard checkBoard = new OctiBoard();
                checkBoard.fromPositionString(fenAfter);
                assertEquals(fenAfter, checkBoard.toPositionString());
            }

            String transcript = board.serializeGame();
            OctiBoard replayBoard = new OctiBoard();
            replayBoard.deserializeGame(transcript);
            assertEquals(board.toPositionString(), replayBoard.toPositionString());

            int movesMade = game.getNumMoves();
            for (int u = 0; u < movesMade; u++) {
                game.undo();
            }
            assertEquals(initialFen, board.toPositionString());

            for (int r = 0; r < movesMade; r++) {
                game.redo();
            }
            assertEquals(replayBoard.toPositionString(), board.toPositionString());
        }
    }

    @Test
    public void testRandomLegalMoveWalkOFK() {
        Random rand = new Random(42L);
        for (int gameIdx = 0; gameIdx < 50; gameIdx++) {
            OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_OFK);
            OctiGame game = board.getGame();
            assertNotNull(game);
            game.start(OctiPlayer.PLAYER_A);

            String initialFen = board.toPositionString();
            int numSteps = 4 + rand.nextInt(9);

            for (int step = 0; step < numSteps; step++) {
                if (game.isGameOver()) {
                    break;
                }
                List<OctiMove> legalMoves = game.generateLegalMoves();
                if (legalMoves.isEmpty()) {
                    break;
                }
                int moveIdx = rand.nextInt(legalMoves.size());
                OctiMove selected = legalMoves.get(moveIdx);

                game.move(selected);

                String fenAfter = board.toPositionString();
                OctiBoard checkBoard = new OctiBoard();
                checkBoard.fromPositionString(fenAfter);
                assertEquals(fenAfter, checkBoard.toPositionString());
            }

            String transcript = board.serializeGame();
            OctiBoard replayBoard = new OctiBoard();
            replayBoard.deserializeGame(transcript);
            assertEquals(board.toPositionString(), replayBoard.toPositionString());

            int movesMade = game.getNumMoves();
            for (int u = 0; u < movesMade; u++) {
                game.undo();
            }
            assertEquals(initialFen, board.toPositionString());

            for (int r = 0; r < movesMade; r++) {
                game.redo();
            }
            assertEquals(replayBoard.toPositionString(), board.toPositionString());
        }
    }

    // -------------------------------------------------------------------------
    // Ghost-pod regression tests
    //
    // Ghost (vacated) pods exist only on the moving stack's own starting
    // square. A different square that merely holds an own pod with identical
    // prongs must be treated as an ordinary occupied square for jump-over and
    // landing checks.
    // -------------------------------------------------------------------------

    private static final OctiGameMode[] GHOST_MODES = {
        OctiGameMode.GAME_MODE_2, OctiGameMode.GAME_MODE_4, OctiGameMode.GAME_MODE_OFK
    };

    private static OctiPod ghostPod(OctiPodType type, OctiDir... prongs) {
        OctiPod pod = new OctiPod(type);
        for (OctiDir d : prongs) {
            pod.addProng(d, false);
        }
        return pod;
    }

    private static OctiMoveOp ghostJumpOp(int x0, int y0, OctiPod pod, int[][] hops) {
        OctiMoveOp op = new OctiMoveOp();
        op.x0 = x0;
        op.y0 = y0;
        op.pod = pod.copy();
        for (int[] h : hops) {
            op.hops.add(new OctiHop(h[0], h[1], false));
        }
        return op;
    }

    private static OctiMove ghostJump(int x0, int y0, OctiPod pod, int[][] hops) {
        return new OctiMove(Collections.singletonList(ghostJumpOp(x0, y0, pod, hops)));
    }

    private static boolean ghostAccepts(OctiGame game, OctiMove m) {
        try {
            game.move(m);
        } catch (IllegalMoveException e) {
            return false;
        }
        return true;
    }

    private static void collectGhostFailures(OctiGameMode mode, List<String> failures,
                                             List<String> allFailures) {
        for (String f : failures) {
            allFailures.add(mode + ": " + f);
        }
    }

    // (i) Landing on a DIFFERENT square holding an identical own pod is illegal.
    @Test
    public void testGhostPodLandingOnIdenticalOwnPodIsIllegal() {
        List<String> allFailures = new ArrayList<>();
        for (OctiGameMode mode : GHOST_MODES) {
            OctiBoard board = new OctiBoard(mode);
            OctiGame game = board.getGame();
            game.start(OctiPlayer.PLAYER_A);
            board.clear();

            OctiPod mover = ghostPod(OctiPodType.A_POD, OctiDir.DIR_E);
            board.squareAt(0, 3).addPod(mover.copy());
            board.squareAt(1, 3).addPod(ghostPod(OctiPodType.A_POD));
            board.squareAt(2, 3).addPod(mover.copy());

            String before = board.toPositionString();
            OctiMove jump = ghostJump(0, 3, mover, new int[][] {{2, 3}});

            List<String> failures = new ArrayList<>();
            if (game.generateLegalMoves().contains(jump)) {
                failures.add("generateLegalMoves() emitted a jump landing on an identical own pod");
            }
            if (ghostAccepts(game, jump)) {
                failures.add("move() accepted a jump landing on an identical own pod");
            } else if (!before.equals(board.toPositionString())) {
                failures.add("rejected move() changed the position");
            }
            collectGhostFailures(mode, failures, allFailures);
        }
        assertTrue(allFailures.toString(), allFailures.isEmpty());
    }

    // (ii) Jumping OVER a DIFFERENT square holding an identical own pod is legal
    // when the landing square is empty.
    @Test
    public void testGhostPodJumpOverIdenticalOwnPodIsLegal() {
        List<String> allFailures = new ArrayList<>();
        for (OctiGameMode mode : GHOST_MODES) {
            OctiBoard board = new OctiBoard(mode);
            OctiGame game = board.getGame();
            game.start(OctiPlayer.PLAYER_A);
            board.clear();

            OctiPod mover = ghostPod(OctiPodType.A_POD, OctiDir.DIR_E);
            board.squareAt(0, 3).addPod(mover.copy());
            board.squareAt(1, 3).addPod(mover.copy());

            OctiMove jump = ghostJump(0, 3, mover, new int[][] {{2, 3}});

            List<String> failures = new ArrayList<>();
            if (!game.generateLegalMoves().contains(jump)) {
                failures.add("generateLegalMoves() omitted a jump over an identical own pod");
            }
            if (!ghostAccepts(game, jump)) {
                failures.add("move() rejected a jump over an identical own pod");
            } else if (board.squareAt(0, 3).getNumPods() != 0
                    || board.squareAt(1, 3).getNumPods() != 1
                    || board.squareAt(2, 3).getNumPods() != 1) {
                failures.add("unexpected pod counts after jump over identical own pod");
            }
            collectGhostFailures(mode, failures, allFailures);
        }
        assertTrue(allFailures.toString(), allFailures.isEmpty());
    }

    // (iii) Looping back to the vacated start square stays legal.
    @Test
    public void testGhostPodLoopBackToVacatedStartStaysLegal() {
        List<String> allFailures = new ArrayList<>();
        for (OctiGameMode mode : GHOST_MODES) {
            OctiBoard board = new OctiBoard(mode);
            OctiGame game = board.getGame();
            game.start(OctiPlayer.PLAYER_A);
            board.clear();

            OctiPod mover = ghostPod(OctiPodType.A_POD,
                    OctiDir.DIR_E, OctiDir.DIR_N, OctiDir.DIR_SW);
            board.squareAt(1, 3).addPod(mover.copy());
            board.squareAt(2, 3).addPod(ghostPod(OctiPodType.A_POD));
            board.squareAt(3, 4).addPod(ghostPod(OctiPodType.A_POD));
            board.squareAt(2, 4).addPod(ghostPod(OctiPodType.A_POD));

            OctiMove loop = ghostJump(1, 3, mover, new int[][] {{3, 3}, {3, 5}, {1, 3}});

            List<String> failures = new ArrayList<>();
            if (!game.generateLegalMoves().contains(loop)) {
                failures.add("generateLegalMoves() omitted a loop back to the vacated start square");
            }
            if (!ghostAccepts(game, loop)) {
                failures.add("move() rejected a loop back to the vacated start square");
            } else if (board.squareAt(1, 3).getNumPods() != 1
                    || board.squareAt(3, 3).getNumPods() != 0
                    || board.squareAt(3, 5).getNumPods() != 0) {
                failures.add("unexpected pod counts after loop back to start");
            }
            collectGhostFailures(mode, failures, allFailures);
        }
        assertTrue(allFailures.toString(), allFailures.isEmpty());
    }

    // Composite stack move: two identical pods on one start square both jump to
    // the same destination over one stationary pod. Stacks do not exist in
    // OCTI for Kids.
    @Test
    public void testGhostPodStackJumpsToSameDestination() {
        OctiGameMode[] stackModes = {OctiGameMode.GAME_MODE_2, OctiGameMode.GAME_MODE_4};
        for (OctiGameMode mode : stackModes) {
            OctiBoard board = new OctiBoard(mode);
            OctiGame game = board.getGame();
            game.start(OctiPlayer.PLAYER_A);
            board.clear();

            OctiPod mover = ghostPod(OctiPodType.A_POD, OctiDir.DIR_E);
            board.squareAt(0, 3).addPod(mover.copy());
            board.squareAt(0, 3).addPod(mover.copy());
            board.squareAt(1, 3).addPod(ghostPod(OctiPodType.A_POD));

            OctiMoveOp op = ghostJumpOp(0, 3, mover, new int[][] {{2, 3}});
            OctiMove both = new OctiMove(Arrays.asList(op, new OctiMoveOp(op)));

            assertTrue(mode + ": move() rejected a stack jumping to the same destination",
                    ghostAccepts(game, both));
            assertEquals(0, board.squareAt(0, 3).getNumPods());
            assertEquals(1, board.squareAt(1, 3).getNumPods());
            assertEquals(2, board.squareAt(2, 3).getNumPods());
        }
    }

    // (iv) Every generated legal move must be accepted by move() and must be
    // fully reversible (position string and transcript restored by undo()).
    @Test
    public void testGhostPodGeneratedMovesAcceptedAndReversible() {
        for (OctiGameMode mode : GHOST_MODES) {
            Random rand = new Random(20260622L + mode.ordinal());
            for (int gameIdx = 0; gameIdx < 150; gameIdx++) {
                OctiBoard board = new OctiBoard(mode);
                OctiGame game = board.getGame();
                game.start(OctiPlayer.PLAYER_A);

                int numSteps = 5 + rand.nextInt(11);
                for (int step = 0; step < numSteps; step++) {
                    if (game.isGameOver()) {
                        break;
                    }
                    List<OctiMove> legalMoves = game.generateLegalMoves();
                    if (legalMoves.isEmpty()) {
                        break;
                    }

                    String fenBefore = board.toPositionString();
                    String transcriptBefore = board.serializeGame();
                    int movesBefore = game.getNumMoves();

                    for (OctiMove m : legalMoves) {
                        String context = mode + " game " + gameIdx + " step " + step
                                + " move " + m.asString() + " position " + fenBefore;
                        try {
                            game.move(m);
                        } catch (IllegalMoveException e) {
                            fail(context + " rejected: " + e.getMessage());
                        }
                        assertEquals(context, movesBefore + 1, game.getNumMoves());
                        game.undo();
                        assertEquals(context, movesBefore, game.getNumMoves());
                        assertEquals(context, fenBefore, board.toPositionString());
                        assertEquals(context, transcriptBefore, board.serializeGame());
                    }

                    game.move(legalMoves.get(rand.nextInt(legalMoves.size())));
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // Exception Assert Helpers
    // -------------------------------------------------------------------------

    @FunctionalInterface
    private interface Executable {
        void execute();
    }

    private static void assertThrowsIllegalMoveSyntax(Executable executable) {
        try {
            executable.execute();
            fail("Expected IllegalMoveSyntaxException to be thrown");
        } catch (IllegalMoveSyntaxException e) {
            // Success
        }
    }

    private static void assertThrowsIllegalMove(Executable executable) {
        try {
            executable.execute();
            fail("Expected IllegalMoveException to be thrown");
        } catch (IllegalMoveException e) {
            // Success
        }
    }
}
