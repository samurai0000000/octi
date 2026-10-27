/*
 * OctiBoardCanvas.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import android.graphics.Bitmap
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.IntSize
import com.selfso.octi.OctiBoardRenderer
import com.selfso.octi.OctiDir
import com.selfso.octi.OctiMoveComposer
import com.selfso.octi.OctiPlayer
import com.selfso.octi.ui.theme.OctiTheme

/**
 * Interactive high-DPI OCTI board canvas with 4-quadrant rotation,
 * stack fan overlay hit-testing, radial 8-direction prong compass sector
 * selection, and Smart Auto-Commit / Confirm-All move composition.
 */
@Composable
fun OctiBoardCanvas(
    viewModel: OctiAppViewModel,
    modifier: Modifier = Modifier
) {
    val themeId by viewModel.theme.collectAsState()
    val rotation by viewModel.boardRotation.collectAsState()
    val boardRev by viewModel.boardRevision.collectAsState()
    val viewingPly by viewModel.viewingPly.collectAsState()
    var canvasSize by remember { mutableStateOf(IntSize(640, 640)) }
    var selectedOriginSquare by remember { mutableStateOf<Pair<Int, Int>?>(null) }

    val renderedBitmap = remember(themeId, rotation, boardRev, viewingPly, canvasSize) {
        val vw = Math.max(320, Math.min(1280, canvasSize.width))
        val vh = Math.max(320, Math.min(1280, canvasSize.height))
        val activeBoard = if (viewModel.isViewingHistory()) {
            viewModel.displayBoard
        } else {
            viewModel.board
        }
        viewModel.geometry.configure(
            vw,
            vh,
            activeBoard.boardWidth,
            activeBoard.boardHeight,
            rotation
        )
        val pixels = OctiBoardRenderer.renderBoardPixels(
            activeBoard,
            viewModel.composer,
            viewModel.geometry,
            themeId
        )
        Bitmap.createBitmap(pixels, vw, vh, Bitmap.Config.ARGB_8888).asImageBitmap()
    }

    Box(
        modifier = modifier
            .background(OctiTheme.colors.windowBg)
            .onSizeChanged { newSize ->
                if (newSize.width > 0 && newSize.height > 0) {
                    canvasSize = newSize
                }
            }
            .pointerInput(rotation, boardRev, viewingPly) {
                detectTapGestures(
                    onTap = { offset ->
                        if (viewModel.isViewingHistory()) {
                            return@detectTapGestures
                        }
                        val composer = viewModel.composer
                        val geo = viewModel.geometry
                        val b = viewModel.board
                        val scaleX = renderedBitmap.width.toDouble() / Math.max(1, size.width).toDouble()
                        val scaleY = renderedBitmap.height.toDouble() / Math.max(1, size.height).toDouble()
                        val px = offset.x.toDouble() * scaleX
                        val py = offset.y.toDouble() * scaleY

                        // 1. Check Stack Fan Overlay first if open
                        if (composer.isStackFanOpen) {
                            val fanX = composer.stackFanX
                            val fanY = composer.stackFanY
                            if (b.isValidCoord(fanX - 1, fanY - 1)) {
                                val sq = b.squareAt(fanX - 1, fanY - 1)
                                val hitSlot = geo.hitTestStackFanSlot(
                                    fanX,
                                    fanY,
                                    sq.numPods,
                                    px,
                                    py
                                )
                                if (hitSlot >= 0) {
                                    composer.selectStackPodIndex(fanX, fanY, hitSlot)
                                    composer.closeStackFan()
                                    selectedOriginSquare = Pair(fanX, fanY)
                                    composer.beginMoveOrJumpPod(fanX, fanY, hitSlot)
                                    viewModel.notifyComposerUpdated()
                                    return@detectTapGestures
                                }
                            }
                            composer.closeStackFan()
                        }

                        // 2. Map screen point to board square (1-based)
                        val xy = IntArray(2)
                        if (!geo.screenToSquare(px, py, xy)) {
                            viewModel.notifyComposerUpdated()
                            return@detectTapGestures
                        }
                        val sqX = xy[0]
                        val sqY = xy[1]

                        // 3. If Move Composer is in guided move/jump chain mode
                        if (composer.isGuidedMoveActive) {
                            val nextSquares = composer.guidedNextSquares()
                            var isDestination = false
                            for (dest in nextSquares) {
                                if (dest[0] == sqX && dest[1] == sqY) {
                                    isDestination = true
                                    break
                                }
                            }
                            if (isDestination) {
                                if (composer.selectGuidedDestination(sqX, sqY)) {
                                    val capChoices = composer.captureChoices()
                                    val latestCap = capChoices.lastOrNull { !it.mandatory }
                                    if (latestCap != null) {
                                        val jsq = b.squareAt(latestCap.jumpedX - 1, latestCap.jumpedY - 1)
                                        val jumpedPlayer = if (!jsq.isEmpty) jsq.getPodAt(0).type.toPlayer() else OctiPlayer.PLAYER_B
                                        viewModel.promptCapture(latestCap.jumpedX, latestCap.jumpedY, jumpedPlayer)
                                    }
                                    if (composer.hasUnlockedSuperProngBonus()) {
                                        viewModel.setShowSuperProngBonusDialog(true)
                                    }
                                    if (!composer.isGuidedMoveActive && composer.hasOtherMovablePodAtOrigin()) {
                                        val orig = composer.guidedOriginSquare()
                                        viewModel.setCompositePodPrompt(Pair(orig[0], orig[1]))
                                    }
                                    viewModel.notifyComposerUpdated()
                                    return@detectTapGestures
                                }
                            }

                            // Tapping current square finishes guided jump chain here
                            val curSq = composer.guidedCurrentSquare()
                            if (curSq[0] == sqX && curSq[1] == sqY) {
                                composer.finishGuidedMoveHere()
                                if (composer.hasUnlockedSuperProngBonus()) {
                                    viewModel.setShowSuperProngBonusDialog(true)
                                }
                                if (composer.hasOtherMovablePodAtOrigin()) {
                                    val orig = composer.guidedOriginSquare()
                                    viewModel.setCompositePodPrompt(Pair(orig[0], orig[1]))
                                }
                                viewModel.notifyComposerUpdated()
                                return@detectTapGestures
                            }

                            // Tapping elsewhere during guided move does not open zoomed square
                            return@detectTapGestures
                        }

                        // 4. Check optional capture toggle on jumped square
                        if (composer.toggleCaptureAtSquare(sqX, sqY)) {
                            viewModel.notifyComposerUpdated()
                            return@detectTapGestures
                        }

                        val dragKind = composer.activeDragKind()
                        if (dragKind == OctiMoveComposer.DragKind.ADD_PRONG_FROM_RESERVE ||
                            dragKind == OctiMoveComposer.DragKind.ADD_SUPERPRONG_FROM_RESERVE ||
                            dragKind == OctiMoveComposer.DragKind.REPOSITION_PRONG_ON_POD
                        ) {
                            val center = geo.squareCenter(sqX, sqY)
                            val cellR = geo.cellSize() * 0.48
                            val dir = geo.hitTestCompassSector(
                                center[0],
                                center[1],
                                cellR * 0.12,
                                cellR * 1.15,
                                px,
                                py
                            )
                            val chosenDir = if (dir != OctiDir.DIR_UNDEF) dir else OctiDir.DIR_N
                            val podIdx = Math.max(0, composer.selectedStackPodIndex(sqX, sqY))
                            composer.dropOnCompassSector(sqX, sqY, podIdx, chosenDir)
                            if (composer.hasUnlockedSuperProngBonus()) {
                                viewModel.setShowSuperProngBonusDialog(true)
                            }
                            viewModel.notifyComposerUpdated()
                            return@detectTapGestures
                        }

                        if (dragKind == OctiMoveComposer.DragKind.ADD_POD_FROM_RESERVE ||
                            dragKind == OctiMoveComposer.DragKind.LIBERATE_POD_FROM_RESERVE
                        ) {
                            if (composer.dropOnSquare(sqX, sqY)) {
                                selectedOriginSquare = null
                                viewModel.notifyComposerUpdated()
                                return@detectTapGestures
                            }
                        }

                        // 5. Non-empty square or friendly home square opens Zoomed Square Dialog
                        val sq = b.squareAt(sqX - 1, sqY - 1)
                        val turn = b.game?.turn
                        val isHome = if (turn != null) b.isHomeSquareOf(sqX - 1, sqY - 1, turn) else false
                        if (!sq.isEmpty || isHome) {
                            selectedOriginSquare = Pair(sqX, sqY)
                            viewModel.openZoomedSquare(sqX, sqY)
                            viewModel.notifyComposerUpdated()
                            return@detectTapGestures
                        }
                    },
                    onLongPress = { offset ->
                        if (viewModel.isViewingHistory()) {
                            return@detectTapGestures
                        }
                        val geo = viewModel.geometry
                        val b = viewModel.board
                        val scaleX = renderedBitmap.width.toDouble() / Math.max(1, size.width).toDouble()
                        val scaleY = renderedBitmap.height.toDouble() / Math.max(1, size.height).toDouble()
                        val px = offset.x.toDouble() * scaleX
                        val py = offset.y.toDouble() * scaleY
                        val xy = IntArray(2)
                        if (geo.screenToSquare(px, py, xy)) {
                            val sqX = xy[0]
                            val sqY = xy[1]
                            val sq = b.squareAt(sqX - 1, sqY - 1)
                            if (sq.numPods >= 2) {
                                viewModel.composer.openStackFan(sqX, sqY)
                                viewModel.notifyComposerUpdated()
                            }
                        }
                    }
                )
            }
    ) {
        Canvas(modifier = Modifier.fillMaxSize()) {
            drawImage(
                image = renderedBitmap,
                dstOffset = IntOffset.Zero,
                dstSize = IntSize(size.width.toInt(), size.height.toInt())
            )
        }
    }
}

/*
 * Local variables:
 * mode: Kotlin
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
