/*
 * OctiCoreUnitTest.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.Test;

import java.util.List;

public class OctiCoreUnitTest {

    @Test
    public void testUserReported72MoveTranscriptRejectsEmptyBaseLiberationAtMove60() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        String[] moves = {
            "33+b", "37+d", "53+h", "37d+e", "33b-44", "37de+f", "44b+h", "37def-46",
            "44bh+a", "57+d", "53h+b", "57d+e", "33", "57de+f", "33+h", "77+d",
            "33h+b", "46def-55", "44abh-66x-88x", "37", "88abh+f", "37", "88abfh-77", "37",
            "77abfh+g", "37", "77abfgh-67", "57def-66", "67abfgh+e", "66def-75",
            "67abefgh-57", "37+d", "57abefgh-47", "37d+c", "47abefgh-27x", "75def+a",
            "27abefgh+c", "75adef+b", "27abcefgh-37", "75abdef+c", "37", "75abcdef+g",
            "37abcefgh-47", "75abcdefg+h", "47abcefgh-57", "75abcdefgh-66", "57abcefgh-47",
            "66abcdefgh-56", "47abcefgh-38", "56abcdefgh-65", "38abcefgh+d", "65abcdefgh-74",
            "38abcdefgh-47", "74abcdefgh-72x", "47abcdefgh-57", "72abcdefgh-73", "57",
            "73", "57abcdefgh-67"
        };

        assertEquals(59, moves.length);
        for (String m : moves) {
            game.move(m);
        }

        assertEquals(OctiPlayer.PLAYER_B, game.getTurn());
        assertEquals(0, game.getFreePods(OctiPlayer.PLAYER_B));
        assertEquals(5, game.getCapturedPods(OctiPlayer.PLAYER_B));
        assertTrue(board.squareAt(6, 2).isOccupiedBy(OctiPlayer.PLAYER_B));
        assertTrue(board.squareAt(6, 6).isEmpty());

        List<OctiMove> legalMoves = game.generateLegalMoves();
        boolean has73Star = false;
        boolean has77 = false;
        boolean has77Star = false;
        boolean has37 = false;
        boolean has37Star = false;
        boolean has57 = false;
        boolean has57Star = false;

        for (OctiMove lm : legalMoves) {
            String s = lm.format();
            if ("73*".equals(s)) has73Star = true;
            if ("77".equals(s)) has77 = true;
            if ("77*".equals(s)) has77Star = true;
            if ("37".equals(s)) has37 = true;
            if ("37*".equals(s)) has37Star = true;
            if ("57".equals(s)) has57 = true;
            if ("57*".equals(s)) has57Star = true;
        }

        assertTrue("Legal moves should contain 73* on occupied square 73", has73Star);
        assertFalse("Legal moves must NOT contain 77 on empty square 77", has77);
        assertFalse("Legal moves must NOT contain 77* on empty square 77", has77Star);
        assertFalse("Legal moves must NOT contain 37 on empty square 37", has37);
        assertFalse("Legal moves must NOT contain 37* on empty square 37", has37Star);
        assertFalse("Legal moves must NOT contain 57 on empty square 57", has57);
        assertFalse("Legal moves must NOT contain 57* on empty square 57", has57Star);

        try {
            game.move("77");
            fail("Expected IllegalMoveException for move 77 on empty square");
        } catch (IllegalMoveException expected) {
            // Success
        }

        try {
            game.move("77*");
            fail("Expected IllegalMoveException for move 77* on empty square");
        } catch (IllegalMoveException expected) {
            // Success
        }
    }

    @Test
    @SuppressWarnings("unchecked")
    public void testCanonical13VariantsBijectiveAndOfficialRatedPredicates() throws Exception {
        java.lang.reflect.Method allMethod;
        java.lang.reflect.Method codeMethod;
        java.lang.reflect.Method parseMethod;
        java.lang.reflect.Method ratedCodeMethod;
        try {
            allMethod = OctiGameMode.class.getMethod("allVariantCodes");
            codeMethod = OctiGameMode.class.getMethod(
                    "variantCode",
                    OctiGameMode.class,
                    OctiGameBases.class,
                    boolean.class,
                    boolean.class);
            parseMethod = OctiGameMode.class.getMethod("parseVariantCode", String.class);
            ratedCodeMethod = OctiGameMode.class.getMethod("isOfficialRatedVariantCode", String.class);
        } catch (NoSuchMethodException e) {
            fail("Missing OctiGameMode 13-variant helper: " + e.getMessage());
            return;
        }

        List<String> expected13 = java.util.Arrays.asList(
                "2P_3B", "2P_3B_SP", "2P_3B_ED", "2P_3B_SPED",
                "2P_1B", "2P_1B_SP", "2P_1B_ED", "2P_1B_SPED",
                "4P", "4P_SP", "4P_ED", "4P_SPED",
                "OFK");
        List<String> actualAll = (List<String>) allMethod.invoke(null);
        assertEquals(expected13, actualAll);

        for (String v : expected13) {
            Object parsed = parseMethod.invoke(null, v);
            assertTrue("Expected parseVariantCode non-null for " + v, parsed != null);
            OctiGameMode m = (OctiGameMode) parsed.getClass().getField("mode").get(parsed);
            OctiGameBases b = (OctiGameBases) parsed.getClass().getField("bases").get(parsed);
            boolean sp = parsed.getClass().getField("superProngs").getBoolean(parsed);
            boolean ed = parsed.getClass().getField("edgeless").getBoolean(parsed);
            if (v.startsWith("4P")) {
                assertEquals(OctiGameBases.GAME_BASES_3, b);
            }
            if ("OFK".equals(v)) {
                assertEquals(OctiGameBases.GAME_BASES_1, b);
                assertFalse(sp);
                assertFalse(ed);
            }
            String roundTrip = (String) codeMethod.invoke(null, m, b, sp, ed);
            assertEquals(v, roundTrip);

            boolean isRated = (Boolean) ratedCodeMethod.invoke(null, v);
            boolean shouldBeRated = "2P_3B".equals(v) || "2P_1B".equals(v) || "OFK".equals(v);
            assertEquals("Official rated mismatch for " + v, shouldBeRated, isRated);
        }

        assertEquals("4P", codeMethod.invoke(
                null, OctiGameMode.GAME_MODE_4, OctiGameBases.GAME_BASES_1, false, false));
        assertEquals("OFK", codeMethod.invoke(
                null, OctiGameMode.GAME_MODE_OFK, OctiGameBases.GAME_BASES_3, true, true));
        assertTrue(parseMethod.invoke(null, "BOGUS_VARIANT") == null);
        assertFalse((Boolean) ratedCodeMethod.invoke(null, "4P"));
        assertFalse((Boolean) ratedCodeMethod.invoke(null, "BOGUS_VARIANT"));
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
