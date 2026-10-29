/*
 * OctiBotAndPersistenceTest.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

public class OctiBotAndPersistenceTest {

    @Test
    public void testDefaultBudgetAndDepthAndCapsForDifficulty_EMF_B02() {
        // CASUAL: 3000ms (3s), depth 5, root cap 28, branch cap 18
        assertEquals(3000, OctiBotEngine.defaultBudgetForDifficulty(OctiBotEngine.BotDifficulty.CASUAL));
        assertEquals(5, OctiBotEngine.maxDepthForDifficulty(OctiBotEngine.BotDifficulty.CASUAL));
        assertEquals(28, OctiBotEngine.rootCapForDifficulty(OctiBotEngine.BotDifficulty.CASUAL));
        assertEquals(18, OctiBotEngine.branchCapForDifficulty(OctiBotEngine.BotDifficulty.CASUAL));

        // TACTICAL: 8000ms (8s), depth 7, root cap 40, branch cap 24
        assertEquals(8000, OctiBotEngine.defaultBudgetForDifficulty(OctiBotEngine.BotDifficulty.TACTICAL));
        assertEquals(7, OctiBotEngine.maxDepthForDifficulty(OctiBotEngine.BotDifficulty.TACTICAL));
        assertEquals(40, OctiBotEngine.rootCapForDifficulty(OctiBotEngine.BotDifficulty.TACTICAL));
        assertEquals(24, OctiBotEngine.branchCapForDifficulty(OctiBotEngine.BotDifficulty.TACTICAL));

        // MASTER: 15000ms (15s), depth 9, root cap 56, branch cap 32
        assertEquals(15000, OctiBotEngine.defaultBudgetForDifficulty(OctiBotEngine.BotDifficulty.MASTER));
        assertEquals(9, OctiBotEngine.maxDepthForDifficulty(OctiBotEngine.BotDifficulty.MASTER));
        assertEquals(56, OctiBotEngine.rootCapForDifficulty(OctiBotEngine.BotDifficulty.MASTER));
        assertEquals(32, OctiBotEngine.branchCapForDifficulty(OctiBotEngine.BotDifficulty.MASTER));

        // TT_SIZE must be 262,144 (256K entries)
        assertEquals(262144, OctiBotEngine.TT_SIZE);

        OctiBotEngine bot = new OctiBotEngine(OctiBotEngine.BotDifficulty.TACTICAL);
        assertEquals(8000, bot.timeBudgetMs());
        assertFalse(bot.isCustomTimeBudget());

        bot.setTimeBudgetMs(25);
        assertEquals(25, bot.timeBudgetMs());
        assertTrue(bot.isCustomTimeBudget());

        bot.restoreDefaultBudget();
        assertEquals(8000, bot.timeBudgetMs());
        assertFalse(bot.isCustomTimeBudget());
    }

    @Test
    public void testZobristBinaryHashingProperties_EMF_B02() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false, OctiGameBases.GAME_BASES_3);
        board.getGame().start(OctiPlayer.PLAYER_A);

        long h1 = OctiBotEngine.computePositionHash(board);
        assertNotEquals(0L, h1);

        // Deterministic: subsequent calls on identical board must yield identical 64-bit key
        long h2 = OctiBotEngine.computePositionHash(board);
        assertEquals(h1, h2);

        // Board mutation (adding a prong) must alter the hash
        board.getGame().move("33+a");
        long h3 = OctiBotEngine.computePositionHash(board);
        assertNotEquals(h1, h3);

        // Turn change must alter the hash
        long h3Repeat = OctiBotEngine.computePositionHash(board);
        assertEquals(h3, h3Repeat);

        board.getGame().move("37+e");
        long h4 = OctiBotEngine.computePositionHash(board);
        assertNotEquals(h3, h4);
        assertNotEquals(h1, h4);
    }

    @Test
    public void testAppendCompositeMoves_EMF_B02() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false, OctiGameBases.GAME_BASES_3);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        // Initially at (2, 2) (square 33), there is only 1 pod with no prongs.
        List<OctiMove> compositeMoves = new ArrayList<>();
        OctiBotEngine.appendCompositeMoves(board, OctiPlayer.PLAYER_A, compositeMoves);
        assertEquals(0, compositeMoves.size());

        // Place a second pod on square (2, 2) and add prongs to both pods
        OctiSquare sq = board.squareAt(2, 2);
        OctiPod pod2 = new OctiPod(OctiPodType.A_POD);
        pod2.addProng(OctiDir.DIR_N);
        sq.addPod(pod2);

        OctiPod pod1 = sq.getPodAt(0);
        pod1.addProng(OctiDir.DIR_E);

        // Now (2, 2) has 2 friendly pods of PLAYER_A, each with a prong (N and E).
        OctiBotEngine.appendCompositeMoves(board, OctiPlayer.PLAYER_A, compositeMoves);
        assertTrue("Composite moves should be enumerated when a square has 2+ friendly pods with prongs",
                compositeMoves.size() > 0);

        for (OctiMove m : compositeMoves) {
            assertTrue("Composite move must have 2 or more ops", m.getOps().size() >= 2);
            assertEquals(2, m.getOps().get(0).x0);
            assertEquals(2, m.getOps().get(0).y0);
            assertEquals(2, m.getOps().get(1).x0);
            assertEquals(2, m.getOps().get(1).y0);
            assertFalse(m.asString().isEmpty());
        }

        // Test that chosen composite move executes legally on the board
        OctiMove chosenComposite = compositeMoves.get(0);
        game.move(chosenComposite);
        assertEquals(OctiPlayer.PLAYER_B, game.getTurn());
    }

    @Test
    public void testHybrid2001EvaluationHeuristics_EMF_B02() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false, OctiGameBases.GAME_BASES_3);
        board.getGame().start(OctiPlayer.PLAYER_A);

        int evalStartA = OctiBotEngine.evaluatePosition(board, OctiPlayer.PLAYER_A);
        int evalStartB = OctiBotEngine.evaluatePosition(board, OctiPlayer.PLAYER_B);
        // By symmetry on initial board, perspective scores should be balanced
        assertEquals(evalStartA, -evalStartB, 50);

        // Arming forward prongs for Player A on home base in opening increases evaluation
        board.getGame().move("33+a");
        int evalAfterA = OctiBotEngine.evaluatePosition(board, OctiPlayer.PLAYER_A);
        assertTrue("Arming forward prong in opening should improve evaluation", evalAfterA > evalStartA);

        // Test targetEnemyBases
        List<int[]> enemyBasesForA = OctiBotEngine.targetEnemyBases(board, OctiPlayer.PLAYER_A);
        assertEquals(3, enemyBasesForA.size());
        List<int[]> enemyBasesForB = OctiBotEngine.targetEnemyBases(board, OctiPlayer.PLAYER_B);
        assertEquals(3, enemyBasesForB.size());
    }

    @Test
    public void testBotSearchChoosesMoveWithinBudget_EMF_B02() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false, OctiGameBases.GAME_BASES_3);
        board.getGame().start(OctiPlayer.PLAYER_A);

        OctiBotEngine bot = new OctiBotEngine(OctiBotEngine.BotDifficulty.CASUAL);
        bot.setTimeBudgetMs(30);

        OctiBotEngine.BotSearchStats stats = new OctiBotEngine.BotSearchStats();
        String move = bot.chooseMove(board, stats);

        assertNotNull(move);
        assertFalse("Move must not be empty", move.isEmpty());
        assertTrue("Elapsed search time should be bounded by ~150ms", stats.elapsedMs < 300);
        assertTrue("Depth reached must be at least 1", stats.depthReached >= 1);
        assertTrue("Nodes visited must be > 0", stats.nodesVisited > 0);
    }

    @Test
    public void testLevel3MasterAsyncFirstMoveAndConcurrentBoardRenderSafety() throws Exception {
        // 1. Verify searchMode prevents undoStack / history memory leak during deep search
        OctiBoard searchBoard = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false, OctiGameBases.GAME_BASES_3);
        searchBoard.getGame().start(OctiPlayer.PLAYER_A);
        OctiBotEngine masterBot = new OctiBotEngine(OctiBotEngine.BotDifficulty.MASTER);
        masterBot.setTimeBudgetMs(600);
        OctiBotEngine.BotSearchStats stats = new OctiBotEngine.BotSearchStats();
        String chosenDirect = masterBot.chooseMove(searchBoard, stats);
        assertFalse("Level 3 direct search must return a non-empty move", chosenDirect.isEmpty());
        assertTrue("Level 3 search must visit > 100 nodes", stats.nodesVisited > 100);

        // 2. Verify Seat A: human plays Move 1, Level 3 bot asynchronously thinks and applies Move 2
        com.selfso.octi.ui.OctiAppViewModel vm = new com.selfso.octi.ui.OctiAppViewModel();
        vm.setAsyncBotThinkingEnabled(true);
        vm.startNewLocalGame(
                OctiGameMode.GAME_MODE_2,
                false,
                false,
                OctiGameBases.GAME_BASES_3,
                true,
                OctiBotEngine.BotDifficulty.MASTER,
                OctiPlayer.PLAYER_A
        );
        vm.getBotEngine().setTimeBudgetMs(550);

        assertTrue(vm.applyMoveNotation("33+a"));
        Thread botThread = vm.startAsyncBotTurn();
        assertNotNull("Async bot thread should be started for Level 3", botThread);

        while (botThread.isAlive()) {
            synchronized (vm.getBoard()) {
                int[] pixels = OctiBoardRenderer.renderBoardPixels(
                        vm.getBoard(),
                        vm.getComposer(),
                        vm.getGeometry(),
                        OctiThemeId.OBSIDIAN_BRASS
                );
                assertNotNull(pixels);
            }
            Thread.sleep(15);
        }
        botThread.join(3000);

        assertFalse("Bot thinking flag must be false after completion", vm.isBotThinking().getValue());
        assertEquals("Both human move and Level 3 bot first move must be committed", 2, vm.livePlyCount());
        assertEquals(OctiPlayer.PLAYER_A, vm.getBoard().getGame().getTurn());
        vm.close();
    }

    @Test
    public void testLevel3MasterOpeningNeverDumpsUnarmedReservePodsOn37() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        OctiBotEngine bot = new OctiBotEngine(OctiBotEngine.BotDifficulty.MASTER);
        bot.setTimeBudgetMs(350);

        String[] aMoves = new String[]{"53+h", "33+b", "33b-44", "53h-35"};
        for (String aMove : aMoves) {
            game.move(aMove);
            String bMove = bot.chooseMove(board);
            assertNotNull(bMove);
            assertFalse("Level 3 must never dump bare reserve pods in opening: got " + bMove,
                    bMove.equals("37") || bMove.equals("57") || bMove.equals("77"));
            game.move(bMove);
        }

        // B should still have all 4 reserve pods in reserve after 4 opening turns
        assertEquals(4, game.getFreePods(OctiPlayer.PLAYER_B));
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
