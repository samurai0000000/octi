/*
 * OctiAndroidUiSmokeTest.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.List;

public class OctiAndroidUiSmokeTest {

    @Test
    public void testOctiSplashOverlayHasLoopIndefinitelyParameter() {
        // OctiDialogsKt holds top-level composables in OctiDialogs.kt
        Class<?> dialogsKt = null;
        try {
            dialogsKt = Class.forName("com.selfso.octi.ui.OctiDialogsKt");
        } catch (ClassNotFoundException e) {
            // Some compiler outputs may name it OctiDialogs
        }

        if (dialogsKt != null) {
            boolean foundSplash = false;
            boolean hasBooleanParam = false;
            for (Method m : dialogsKt.getDeclaredMethods()) {
                if (m.getName().equals("OctiSplashOverlay")) {
                    foundSplash = true;
                    List<Class<?>> paramTypes = Arrays.asList(m.getParameterTypes());
                    if (paramTypes.contains(boolean.class) || paramTypes.contains(Boolean.class)) {
                        hasBooleanParam = true;
                        break;
                    }
                }
            }
            assertTrue("Expected OctiSplashOverlay composable function in OctiDialogsKt", foundSplash);
            assertTrue("OctiSplashOverlay should take a boolean parameter for loopIndefinitely", hasBooleanParam);
        }
    }

    @Test
    public void testPlyConfirmDialogHasNoMoreMovesParameter() {
        Class<?> dialogsKt = null;
        try {
            dialogsKt = Class.forName("com.selfso.octi.ui.OctiDialogsKt");
        } catch (ClassNotFoundException ignored) {
        }

        if (dialogsKt != null) {
            boolean found = false;
            for (Method m : dialogsKt.getDeclaredMethods()) {
                if (m.getName().equals("PlyConfirmDialog")
                        || (m.getName().startsWith("PlyConfirmDialog") && !m.isSynthetic() && !m.getName().contains("$"))) {
                    found = true;
                    // Method signature should NOT have more than 2 function/lambda parameters (onCancel, onCommit)
                    // and should take subtitleHint (String)
                    int functionParamCount = 0;
                    for (Class<?> p : m.getParameterTypes()) {
                        if (kotlin.jvm.functions.Function0.class.isAssignableFrom(p)) {
                            functionParamCount++;
                        }
                    }
                    // Previously there were 3 callbacks: onCancel, onCommit, onMoreMoves
                    // Now there are only 2: onCancel, onCommit
                    assertEquals("PlyConfirmDialog should have exactly 2 action callbacks (Cancel & Commit)",
                            2, functionParamCount);
                }
            }
            assertTrue("Expected PlyConfirmDialog composable function in OctiDialogsKt", found);
        }
    }

    @Test
    public void testReserveHudIconsComposablesExist() {
        Class<?> hudKt = null;
        try {
            hudKt = Class.forName("com.selfso.octi.ui.OctiReserveHudKt");
        } catch (ClassNotFoundException ignored) {
        }

        if (hudKt != null) {
            boolean foundPodIcon = false;
            boolean foundProngIcon = false;
            boolean foundSuperProngIcon = false;
            boolean foundCapturedPodIcon = false;
            boolean foundPlayerReserveCard = false;

            for (Method m : hudKt.getDeclaredMethods()) {
                if (m.getName().startsWith("PodIcon")) foundPodIcon = true;
                if (m.getName().startsWith("ProngIcon")) foundProngIcon = true;
                if (m.getName().startsWith("SuperProngIcon")) foundSuperProngIcon = true;
                if (m.getName().startsWith("CapturedPodIcon")) foundCapturedPodIcon = true;
                if (m.getName().startsWith("PlayerReserveCard")) foundPlayerReserveCard = true;
            }

            assertTrue("Expected PodIcon composable", foundPodIcon);
            assertTrue("Expected ProngIcon composable", foundProngIcon);
            assertTrue("Expected SuperProngIcon composable", foundSuperProngIcon);
            assertTrue("Expected CapturedPodIcon composable", foundCapturedPodIcon);
            assertTrue("Expected PlayerReserveCard composable", foundPlayerReserveCard);
        }
    }

    @Test
    public void testOctiMoveComposerMovablePodsApi() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false, OctiGameBases.GAME_BASES_3);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);
        OctiMoveComposer composer = new OctiMoveComposer(board);

        assertNotNull(composer.unclaimedMovablePodIndicesAtOrigin());
        assertFalse(composer.hasDistinctUnclaimedPodsAtOrigin());
        assertFalse(composer.hasOtherMovablePodAtOrigin());
    }

    @Test
    public void testExportMovesDialogAndMoveHistoryFormatting_EMF_A01() {
        Class<?> dialogsKt = null;
        try {
            dialogsKt = Class.forName("com.selfso.octi.ui.OctiDialogsKt");
        } catch (ClassNotFoundException ignored) {
        }

        if (dialogsKt != null) {
            boolean foundExportDialog = false;
            for (Method m : dialogsKt.getDeclaredMethods()) {
                if (m.getName().startsWith("ExportMovesDialog")) {
                    foundExportDialog = true;
                    break;
                }
            }
            assertTrue("Expected ExportMovesDialog composable in OctiDialogsKt", foundExportDialog);
        }

        com.selfso.octi.ui.OctiAppViewModel vm = new com.selfso.octi.ui.OctiAppViewModel();
        try {
            String initialExport = vm.exportMoveHistoryText();
            assertTrue(initialExport.contains("\"format\": \"octi.transcript.v3\"") || initialExport.contains("\"format\":\"octi.transcript.v3\""));
            assertFalse(vm.getShowExportMovesDialog().getValue());

            vm.setShowExportMovesDialog(true);
            assertTrue(vm.getShowExportMovesDialog().getValue());
            vm.setShowExportMovesDialog(false);
            assertFalse(vm.getShowExportMovesDialog().getValue());

            // Apply 3 moves and verify canonical JSON v3 format
            assertTrue(vm.applyMoveNotation("33+a"));
            assertTrue(vm.applyMoveNotation("37+e"));
            assertTrue(vm.applyMoveNotation("33a-34"));

            String exported = vm.exportMoveHistoryText();
            assertTrue(exported.contains("\"format\": \"octi.transcript.v3\"") || exported.contains("\"format\":\"octi.transcript.v3\""));
            assertTrue(exported.contains("33+a"));
            assertTrue(exported.contains("37+e"));
            assertTrue(exported.contains("33a-34"));
            assertFalse("Export must omit '1. A:' prefixes", exported.contains("1. A:"));
            assertFalse("Export must omit client.extended", exported.contains("\"extended\""));
        } finally {
            vm.close();
        }
    }

    @Test
    public void testOctiThinkingDialogAndStateTracking_EMF_A02() {
        Class<?> dialogsKt = null;
        try {
            dialogsKt = Class.forName("com.selfso.octi.ui.OctiDialogsKt");
        } catch (ClassNotFoundException ignored) {
        }

        if (dialogsKt != null) {
            boolean foundThinkingDialog = false;
            for (Method m : dialogsKt.getDeclaredMethods()) {
                if (m.getName().startsWith("OctiThinkingDialog")) {
                    foundThinkingDialog = true;
                    break;
                }
            }
            assertTrue("Expected OctiThinkingDialog composable in OctiDialogsKt", foundThinkingDialog);
        }

        com.selfso.octi.ui.OctiAppViewModel vm = new com.selfso.octi.ui.OctiAppViewModel();
        try {
            assertFalse(vm.isBotThinking().getValue());
            assertEquals(0f, vm.getBotThinkingProgress().getValue(), 0.0001f);
            assertEquals(0f, vm.getBotThinkingRemainingSec().getValue(), 0.0001f);

            vm.updateBotThinkingProgressForTest(true, 0.45f, 4.4f);
            assertTrue(vm.isBotThinking().getValue());
            assertEquals(0.45f, vm.getBotThinkingProgress().getValue(), 0.0001f);
            assertEquals(4.4f, vm.getBotThinkingRemainingSec().getValue(), 0.0001f);

            vm.updateBotThinkingProgressForTest(false, 1.0f, 0.0f);
            assertFalse(vm.isBotThinking().getValue());
            assertEquals(1.0f, vm.getBotThinkingProgress().getValue(), 0.0001f);
            assertEquals(0.0f, vm.getBotThinkingRemainingSec().getValue(), 0.0001f);
        } finally {
            vm.close();
        }
    }

    @Test
    public void testAndroidManifestLocksMainActivityToPortraitOrientation() throws Exception {
        java.io.File manifestFile = new java.io.File("src/main/AndroidManifest.xml");
        if (!manifestFile.exists()) {
            manifestFile = new java.io.File("android/app/src/main/AndroidManifest.xml");
        }
        assertTrue("AndroidManifest.xml should exist", manifestFile.exists());
        String manifestContent = new String(java.nio.file.Files.readAllBytes(manifestFile.toPath()), java.nio.charset.StandardCharsets.UTF_8);
        assertTrue("AndroidManifest.xml must declare android:screenOrientation=\"portrait\"",
                manifestContent.contains("android:screenOrientation=\"portrait\""));
        assertFalse("AndroidManifest.xml must never declare landscape orientation",
                manifestContent.contains("landscape"));
        assertFalse("AndroidManifest.xml must never declare fullSensor orientation",
                manifestContent.contains("fullSensor"));
        assertFalse("AndroidManifest.xml must never declare sensorLandscape orientation",
                manifestContent.contains("sensorLandscape"));

        java.io.File mainActivityFile = new java.io.File("src/main/java/com/selfso/octi/MainActivity.kt");
        if (!mainActivityFile.exists()) {
            mainActivityFile = new java.io.File("android/app/src/main/java/com/selfso/octi/MainActivity.kt");
        }
        assertTrue("MainActivity.kt should exist", mainActivityFile.exists());
        String activityContent = new String(java.nio.file.Files.readAllBytes(mainActivityFile.toPath()), java.nio.charset.StandardCharsets.UTF_8);
        assertTrue("MainActivity.kt must enforce ActivityInfo.SCREEN_ORIENTATION_PORTRAIT in onCreate",
                activityContent.contains("ActivityInfo.SCREEN_ORIENTATION_PORTRAIT"));
    }

    @Test
    public void testReserveHudDisplaysOpponentPodsCapturedByPlayer() {
        OctiBoard board = new OctiBoard(OctiGameMode.GAME_MODE_2, false, false, OctiGameBases.GAME_BASES_3);
        OctiGame game = board.getGame();
        game.start(OctiPlayer.PLAYER_A);

        // Snapshot with Player B having 1 captured pod (i.e. Player A captured 1 of B's pods)
        OctiGameSnapshot snap = game.captureSnapshot();
        snap.players[OctiPlayer.PLAYER_A.ordinal()].capturedPods = 0;
        snap.players[OctiPlayer.PLAYER_B.ordinal()].capturedPods = 1;
        game.restoreSnapshot(snap);

        assertEquals("Player A card must display 1 captured pod (B pod held by A)",
                1, OctiBoardRenderer.getDisplayedCapturedPodsForPlayerCard(game, OctiPlayer.PLAYER_A));
        assertEquals("Player B card must display 0 captured pods (B has captured 0 A pods)",
                0, OctiBoardRenderer.getDisplayedCapturedPodsForPlayerCard(game, OctiPlayer.PLAYER_B));

        // When Player B captures 2 of Player A's pods
        snap.players[OctiPlayer.PLAYER_A.ordinal()].capturedPods = 2;
        snap.players[OctiPlayer.PLAYER_B.ordinal()].capturedPods = 1;
        game.restoreSnapshot(snap);

        assertEquals("Player A card displays 1 captured B pod",
                1, OctiBoardRenderer.getDisplayedCapturedPodsForPlayerCard(game, OctiPlayer.PLAYER_A));
        assertEquals("Player B card displays 2 captured A pods",
                2, OctiBoardRenderer.getDisplayedCapturedPodsForPlayerCard(game, OctiPlayer.PLAYER_B));
    }

    @Test
    public void testBoardZoomInspectUnblockedAndRefreshedDuringBotThinking() throws Exception {
        com.selfso.octi.ui.OctiAppViewModel vm = new com.selfso.octi.ui.OctiAppViewModel();
        try {
            vm.startNewLocalGame(
                    OctiGameMode.GAME_MODE_2,
                    false,
                    false,
                    OctiGameBases.GAME_BASES_3,
                    true,
                    OctiBotEngine.BotDifficulty.CASUAL,
                    OctiPlayer.PLAYER_A);

            assertTrue(vm.applyMoveNotation("33+a"));
            assertTrue(vm.applyMoveNotation("37+e"));
            assertTrue(vm.applyMoveNotation("33a-34"));
            assertEquals(OctiPlayer.PLAYER_B, vm.getBoard().getGame().getTurn());
            assertTrue(vm.getBoard().squareAt(2, 2).isEmpty());

            vm.updateBotThinkingProgressForTest(true, 0.40f, 3.0f);
            assertTrue(vm.isBotThinking().getValue());

            // Empty square (4, 4) and vacated home square (3, 3) cannot place pods while AI thinks
            assertFalse(vm.handleBoardSquareSingleTap(4, 4));
            assertFalse(vm.handleBoardSquareSingleTap(3, 3));
            assertFalse(vm.handleBoardSquareZoomInspect(3, 3));
            assertEquals(null, vm.getZoomedSquare().getValue());

            // Single-tap on occupied friendly square (3, 4) opens read-only zoomedSquare while AI thinks
            assertTrue(vm.handleBoardSquareSingleTap(3, 4));
            kotlin.Pair<Integer, Integer> zs1 = vm.getZoomedSquare().getValue();
            assertNotNull("Single-tap must open zoomedSquare while isBotThinking == true", zs1);
            assertEquals(Integer.valueOf(3), zs1.getFirst());
            assertEquals(Integer.valueOf(4), zs1.getSecond());
            assertFalse(vm.getComposer().isAwaitingConfirmation());
            assertFalse(vm.getComposer().isGuidedMoveActive());

            vm.closeZoomedSquare();
            assertEquals(null, vm.getZoomedSquare().getValue());

            // Long-press / double-tap zoom inspect on enemy pod (3, 7) opens zoomedSquare while AI thinks
            assertTrue(vm.handleBoardSquareZoomInspect(3, 7));
            kotlin.Pair<Integer, Integer> zs2 = vm.getZoomedSquare().getValue();
            assertNotNull("Zoom-inspect must open zoomedSquare while isBotThinking == true", zs2);
            assertEquals(Integer.valueOf(3), zs2.getFirst());
            assertEquals(Integer.valueOf(7), zs2.getSecond());

            // When AI finishes thinking and commits move on (3, 7), boardRevision advances
            // while zoomedSquare stays open for thread-safe live refresh
            long revBefore = vm.getBoardRevision().getValue();
            assertTrue(vm.applyMoveNotation("37e+f"));
            vm.updateBotThinkingProgressForTest(false, 1.0f, 0.0f);
            assertFalse(vm.isBotThinking().getValue());
            assertTrue("boardRevision must increment when AI move lands",
                    vm.getBoardRevision().getValue() > revBefore);
            kotlin.Pair<Integer, Integer> zsAfter = vm.getZoomedSquare().getValue();
            assertNotNull(zsAfter);
            assertEquals(Integer.valueOf(3), zsAfter.getFirst());
            assertEquals(Integer.valueOf(7), zsAfter.getSecond());
            assertEquals(2, vm.getBoard().squareAt(2, 6).getPodAt(0).getNumProngs());
        } finally {
            vm.close();
        }

        // Verify ZoomedSquareDialog takes boardRevision: Long and synchronizes on displayBoard,
        // and OctiThinkingDialog does not swallow full-screen board touches
        Class<?> dialogsKt = null;
        try {
            dialogsKt = Class.forName("com.selfso.octi.ui.OctiDialogsKt");
        } catch (ClassNotFoundException ignored) {
        }
        assertNotNull("Expected OctiDialogsKt class", dialogsKt);
        boolean foundZoomWithBoardRev = false;
        for (Method m : dialogsKt.getDeclaredMethods()) {
            if (m.getName().equals("ZoomedSquareDialog")) {
                for (Class<?> p : m.getParameterTypes()) {
                    if (p == long.class || p == Long.class) {
                        foundZoomWithBoardRev = true;
                        break;
                    }
                }
            }
        }
        assertTrue("ZoomedSquareDialog must accept boardRevision: Long parameter",
                foundZoomWithBoardRev);

        java.io.File dialogsFile = new java.io.File("src/main/java/com/selfso/octi/ui/OctiDialogs.kt");
        if (!dialogsFile.exists()) {
            dialogsFile = new java.io.File("android/app/src/main/java/com/selfso/octi/ui/OctiDialogs.kt");
        }
        assertTrue("OctiDialogs.kt should exist", dialogsFile.exists());
        String dialogsSrc = new String(
                java.nio.file.Files.readAllBytes(dialogsFile.toPath()),
                java.nio.charset.StandardCharsets.UTF_8);

        int thinkingIdx = dialogsSrc.indexOf("fun OctiThinkingDialog(");
        assertTrue("Expected OctiThinkingDialog in OctiDialogs.kt", thinkingIdx >= 0);
        int cardIdx = dialogsSrc.indexOf("Card(", thinkingIdx);
        assertTrue("Expected Card( inside OctiThinkingDialog", cardIdx > thinkingIdx);
        String beforeCard = dialogsSrc.substring(thinkingIdx, cardIdx);
        assertFalse("OctiThinkingDialog outer wrapper must not use .fillMaxSize() touch blocker",
                beforeCard.contains(".fillMaxSize()"));
        assertTrue("ZoomedSquareDialog must synchronize reads on displayBoard",
                dialogsSrc.contains("synchronized(displayBoard)"));

        java.io.File mainActivityFile = new java.io.File("src/main/java/com/selfso/octi/MainActivity.kt");
        if (!mainActivityFile.exists()) {
            mainActivityFile = new java.io.File("android/app/src/main/java/com/selfso/octi/MainActivity.kt");
        }
        assertTrue("MainActivity.kt should exist", mainActivityFile.exists());
        String mainActivitySrc = new String(
                java.nio.file.Files.readAllBytes(mainActivityFile.toPath()),
                java.nio.charset.StandardCharsets.UTF_8);
        assertTrue("MainActivity.kt must pass boardRevision = currentBoardRev to ZoomedSquareDialog",
                mainActivitySrc.contains("boardRevision = currentBoardRev"));
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
