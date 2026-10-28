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
            for (Method m : dialogsKt.getDeclaredMethods()) {
                if (m.getName().startsWith("PlyConfirmDialog")) {
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
            assertEquals("", vm.exportMoveHistoryText());
            assertFalse(vm.getShowExportMovesDialog().getValue());

            vm.setShowExportMovesDialog(true);
            assertTrue(vm.getShowExportMovesDialog().getValue());
            vm.setShowExportMovesDialog(false);
            assertFalse(vm.getShowExportMovesDialog().getValue());

            // Apply 3 moves and verify numbered chat format
            assertTrue(vm.applyMoveNotation("33+a"));
            assertTrue(vm.applyMoveNotation("37+e"));
            assertTrue(vm.applyMoveNotation("33a-34"));

            String exported = vm.exportMoveHistoryText();
            assertTrue(exported.contains("1. A: 33+a"));
            assertTrue(exported.contains("2. B: 37+e"));
            assertTrue(exported.contains("3. A: 33a-34"));
            assertEquals("1. A: 33+a\n2. B: 37+e\n3. A: 33a-34", exported.trim());
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
}

/*
 * Local variables:
 * mode: Java
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
