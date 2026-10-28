/*
 * OctiBoardCanvas.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import android.graphics.Bitmap
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.gestures.calculatePan
import androidx.compose.foundation.gestures.calculateRotation
import androidx.compose.foundation.gestures.calculateZoom
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clipToBounds
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.unit.IntOffset
import androidx.compose.ui.unit.IntSize
import com.selfso.octi.OctiBoardGeometry
import com.selfso.octi.OctiBoardRenderer
import com.selfso.octi.OctiDir
import com.selfso.octi.OctiMoveComposer
import com.selfso.octi.OctiPlayer
import com.selfso.octi.OctiProngType
import com.selfso.octi.ui.theme.OctiTheme

/**
 * Interactive high-DPI OCTI board canvas with 4-quadrant rotation,
 * stack fan overlay hit-testing, radial 8-direction prong compass sector
 * selection, two-finger pinch-to-zoom / 90° rotation, and Smart Auto-Commit /
 * Confirm-All move composition.
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
    var zoomScale by remember { mutableStateOf(1.0f) }
    var panOffset by remember { mutableStateOf(Offset.Zero) }
    var gestureRotationDeg by remember { mutableFloatStateOf(0f) }
    var multiTouchActive by remember { mutableStateOf(false) }
    var lastMultiTouchTimeMs by remember { mutableStateOf(0L) }
    var lastDragTimeMs by remember { mutableStateOf(0L) }

    val renderedBitmap = remember(themeId, rotation, boardRev, viewingPly, canvasSize) {
        val vw = Math.max(320, Math.min(1280, canvasSize.width))
        val vh = Math.max(320, Math.min(1280, canvasSize.height))
        val pixels = synchronized(viewModel.board) {
            val activeBoard = if (viewModel.isViewingHistory()) {
                viewModel.displayBoard
            } else if (viewModel.composer.isAwaitingConfirmation) {
                viewModel.composer.buildPreviewBoard() ?: viewModel.board
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
            OctiBoardRenderer.renderBoardPixels(
                activeBoard,
                viewModel.composer,
                viewModel.geometry,
                themeId
            )
        }
        Bitmap.createBitmap(pixels, vw, vh, Bitmap.Config.ARGB_8888).asImageBitmap()
    }
    val currentRenderedBitmap by rememberUpdatedState(renderedBitmap)

    fun openZoomForOffset(offset: Offset, viewportSize: IntSize) {
        if (multiTouchActive || System.currentTimeMillis() - lastMultiTouchTimeMs < 250L) {
            return
        }
        if (viewModel.isViewingHistory()) {
            return
        }
        val b = viewModel.board
        if (b.game?.isGameOver == true) {
            return
        }
        val geo = viewModel.geometry
        val scaleX = currentRenderedBitmap.width.toDouble() / Math.max(1, viewportSize.width).toDouble()
        val scaleY = currentRenderedBitmap.height.toDouble() / Math.max(1, viewportSize.height).toDouble()
        val px = offset.x.toDouble() * scaleX
        val py = offset.y.toDouble() * scaleY
        val xy = IntArray(2)
        if (!geo.screenToSquare(px, py, xy)) {
            return
        }
        val sqX = xy[0]
        val sqY = xy[1]
        val sq = b.squareAt(sqX - 1, sqY - 1)
        val turn = b.game?.turn
        val isHome = if (turn != null) b.isHomeSquareOf(sqX - 1, sqY - 1, turn) && !sq.hasEnemyOf(turn) else false
        if (!sq.isEmpty || isHome) {
            val composer = viewModel.composer
            val isOriginWithOther = composer.isAwaitingConfirmation &&
                    composer.hasOtherMovablePodAtOrigin() &&
                    composer.guidedOriginSquare().let { it[0] == sqX && it[1] == sqY }
            if (composer.isGuidedMoveActive && !composer.isAwaitingConfirmation && !isOriginWithOther) {
                composer.cancelAndRollback()
            }
            selectedOriginSquare = Pair(sqX, sqY)
            viewModel.openZoomedSquare(sqX, sqY)
            viewModel.notifyComposerUpdated()
        }
    }

    Box(
        modifier = modifier
            .background(OctiTheme.colors.windowBg)
            .clipToBounds()
            .onSizeChanged { newSize ->
                if (newSize.width > 0 && newSize.height > 0) {
                    canvasSize = newSize
                }
            }
            .pointerInput(Unit) {
                awaitEachGesture {
                    var hadMultiTouch = false
                    val down = awaitFirstDown(requireUnconsumed = false)
                    val startPos = down.position
                    var lastPos = startPos
                    var totalMoveDist = 0f
                    val dragSlop = viewConfiguration.touchSlop
                    do {
                        val event = awaitPointerEvent()
                        val pressedPointers = event.changes.filter { it.pressed }
                        if (pressedPointers.size >= 2) {
                            hadMultiTouch = true
                            multiTouchActive = true
                            lastMultiTouchTimeMs = System.currentTimeMillis()
                            val zoomChange = event.calculateZoom()
                            val rotChange = event.calculateRotation()
                            val panChange = event.calculatePan()

                            val newScale = (zoomScale * zoomChange).coerceIn(1.0f, 3.0f)
                            zoomScale = newScale
                            if (newScale > 1.0f) {
                                val maxPanX = (size.width * (newScale - 1.0f)) * 0.5f
                                val maxPanY = (size.height * (newScale - 1.0f)) * 0.5f
                                panOffset = Offset(
                                    (panOffset.x + panChange.x).coerceIn(-maxPanX, maxPanX),
                                    (panOffset.y + panChange.y).coerceIn(-maxPanY, maxPanY)
                                )
                            } else {
                                panOffset = Offset.Zero
                            }

                            gestureRotationDeg += rotChange

                            event.changes.forEach { it.consume() }
                        } else if (pressedPointers.size == 1 && !hadMultiTouch) {
                            val currPos = pressedPointers[0].position
                            totalMoveDist += (currPos - lastPos).getDistance()
                            lastPos = currPos
                        }
                    } while (event.changes.any { it.pressed })

                    if (hadMultiTouch || zoomScale != 1.0f || panOffset != Offset.Zero || gestureRotationDeg != 0f) {
                        val currentDeg = viewModel.boardRotation.value.degrees.toFloat()
                        val totalDeg = currentDeg + gestureRotationDeg
                        val snappedStep = Math.round(totalDeg / 90f).toInt()
                        val normalizedDeg = ((snappedStep * 90 % 360) + 360) % 360
                        val snapped = OctiBoardGeometry.BoardRotation.fromDegrees(normalizedDeg)
                        viewModel.setBoardRotation(snapped)

                        gestureRotationDeg = 0f
                        zoomScale = 1.0f
                        panOffset = Offset.Zero
                        multiTouchActive = false
                        lastMultiTouchTimeMs = System.currentTimeMillis()
                    } else if (totalMoveDist > dragSlop && !viewModel.isViewingHistory() && viewModel.board.game?.isGameOver != true) {
                        lastDragTimeMs = System.currentTimeMillis()
                        val scaleX = currentRenderedBitmap.width.toDouble() / Math.max(1, size.width).toDouble()
                        val scaleY = currentRenderedBitmap.height.toDouble() / Math.max(1, size.height).toDouble()
                        val geo = viewModel.geometry
                        val b = viewModel.board
                        val composer = viewModel.composer

                        val startPx = startPos.x.toDouble() * scaleX
                        val startPy = startPos.y.toDouble() * scaleY
                        val endPx = lastPos.x.toDouble() * scaleX
                        val endPy = lastPos.y.toDouble() * scaleY

                        val startXY = IntArray(2)
                        val endXY = IntArray(2)
                        val startOnBoard = geo.screenToSquare(startPx, startPy, startXY)
                        val endOnBoard = geo.screenToSquare(endPx, endPy, endXY)

                        if (startOnBoard && endOnBoard) {
                            val startX = startXY[0]
                            val startY = startXY[1]
                            val endX = endXY[0]
                            val endY = endXY[1]
                            val turn = b.game?.turn

                            if (endX != startX || endY != startY) {
                                val startSq = b.squareAt(startX - 1, startY - 1)
                                val hasFriendlyPod = !startSq.isEmpty && turn != null && !startSq.hasEnemyOf(turn)
                                if (composer.isGuidedMoveActive) {
                                    val prevCapCount = composer.captureChoices().size
                                    if (composer.clickBoardSquareForGuidedMove(endX, endY)) {
                                        viewModel.refreshDisplayBoard()
                                        val capChoices = composer.captureChoices()
                                        if (capChoices.size > prevCapCount) {
                                            val latestCap = capChoices.lastOrNull { !it.mandatory }
                                            if (latestCap != null) {
                                                val jsq = b.squareAt(latestCap.jumpedX - 1, latestCap.jumpedY - 1)
                                                val jumpedPlayer = if (!jsq.isEmpty) jsq.getPodAt(0).type.toPlayer() else OctiPlayer.PLAYER_B
                                                viewModel.promptCapture(latestCap.jumpedX, latestCap.jumpedY, jumpedPlayer)
                                            }
                                        }
                                    }
                                } else if (hasFriendlyPod) {
                                    val pIdx = composer.selectedStackPodIndex(startX, startY)
                                    val resolvedIdx = if (pIdx >= 0 && pIdx < startSq.numPods) pIdx else -1
                                    val prevCapCount = composer.captureChoices().size
                                    if (composer.stageOrCommitPodMoveOrJump(startX, startY, resolvedIdx, endX, endY, viewModel.moveEntireMatchingStack.value)) {
                                        viewModel.refreshDisplayBoard()
                                        val capChoices = composer.captureChoices()
                                        if (capChoices.size > prevCapCount) {
                                            val latestCap = capChoices.lastOrNull { !it.mandatory }
                                            if (latestCap != null) {
                                                val jsq = b.squareAt(latestCap.jumpedX - 1, latestCap.jumpedY - 1)
                                                val jumpedPlayer = if (!jsq.isEmpty) jsq.getPodAt(0).type.toPlayer() else OctiPlayer.PLAYER_B
                                                viewModel.promptCapture(latestCap.jumpedX, latestCap.jumpedY, jumpedPlayer)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            .pointerInput(Unit) {
                detectTapGestures(
                    onDoubleTap = { offset ->
                        openZoomForOffset(offset, size)
                    },
                    onLongPress = { offset ->
                        openZoomForOffset(offset, size)
                    },
                    onTap = { offset ->
                        if (multiTouchActive || System.currentTimeMillis() - lastMultiTouchTimeMs < 250L || System.currentTimeMillis() - lastDragTimeMs < 250L) {
                            return@detectTapGestures
                        }
                        if (viewModel.isViewingHistory()) {
                            return@detectTapGestures
                        }
                        val composer = viewModel.composer
                        val geo = viewModel.geometry
                        val b = viewModel.board
                        val scaleX = currentRenderedBitmap.width.toDouble() / Math.max(1, size.width).toDouble()
                        val scaleY = currentRenderedBitmap.height.toDouble() / Math.max(1, size.height).toDouble()
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
                            if (composer.isGuidedMoveActive) {
                                if (composer.isAwaitingConfirmation) {
                                    composer.finishGuidedMoveHere()
                                } else {
                                    composer.cancelAndRollback()
                                }
                            }
                            viewModel.notifyComposerUpdated()
                            return@detectTapGestures
                        }
                        val sqX = xy[0]
                        val sqY = xy[1]

                        val orig = composer.guidedOriginSquare()
                        val isOriginSquare = (orig != null && orig.size >= 2 && orig[0] == sqX && orig[1] == sqY)

                        if (composer.isAwaitingConfirmation) {
                            if (isOriginSquare && composer.hasOtherMovablePodAtOrigin()) {
                                composer.finishGuidedMoveHere()
                                if (composer.hasDistinctUnclaimedPodsAtOrigin()) {
                                    viewModel.openZoomedSquare(sqX, sqY)
                                } else {
                                    val nextPod = composer.firstUnclaimedFriendlyPodIndex(sqX, sqY)
                                    composer.beginGuidedPodMove(sqX, sqY, nextPod, viewModel.moveEntireMatchingStack.value)
                                }
                                viewModel.notifyComposerUpdated()
                                return@detectTapGestures
                            }
                        }

                        // 3. If Move Composer is in guided move/jump chain mode
                        if (composer.isGuidedMoveActive) {
                            val isDestination = composer.isGuidedLegalDestination(sqX, sqY)
                            if (isDestination) {
                                val prevCapCount = composer.captureChoices().size
                                if (composer.selectGuidedDestination(sqX, sqY)) {
                                    val capChoices = composer.captureChoices()
                                    if (capChoices.size > prevCapCount) {
                                        val latestCap = capChoices.lastOrNull { !it.mandatory }
                                        if (latestCap != null) {
                                            val jsq = b.squareAt(latestCap.jumpedX - 1, latestCap.jumpedY - 1)
                                            val jumpedPlayer = if (!jsq.isEmpty) jsq.getPodAt(0).type.toPlayer() else OctiPlayer.PLAYER_B
                                            viewModel.promptCapture(latestCap.jumpedX, latestCap.jumpedY, jumpedPlayer)
                                        }
                                    }
                                    viewModel.notifyComposerUpdated()
                                    return@detectTapGestures
                                }
                            }

                            // Not a legal destination!
                            if (composer.isAwaitingConfirmation) {
                                composer.finishGuidedMoveHere()
                                viewModel.notifyComposerUpdated()
                                return@detectTapGestures
                            } else {
                                composer.cancelAndRollback()
                                val sq = b.squareAt(sqX - 1, sqY - 1)
                                val turn = b.game?.turn
                                val isFriendlyPod = !sq.isEmpty && turn != null && !sq.hasEnemyOf(turn)
                                if (!isFriendlyPod) {
                                    viewModel.notifyComposerUpdated()
                                    return@detectTapGestures
                                }
                                // Friendly pod: fall through to process guided move start
                            }
                        } else if (composer.isAwaitingConfirmation) {
                            if (!isOriginSquare || !composer.hasOtherMovablePodAtOrigin()) {
                                composer.cancelAndRollback()
                                viewModel.refreshDisplayBoard()
                            }
                        }

                        if (b.game?.isGameOver == true) {
                            return@detectTapGestures
                        }

                        // 4. Check optional capture toggle on jumped square
                        if (composer.toggleCaptureAtSquare(sqX, sqY)) {
                            viewModel.notifyComposerUpdated()
                            return@detectTapGestures
                        }

                        val dragKind = composer.activeDragKind()
                        if (dragKind == OctiMoveComposer.DragKind.ADD_POD_FROM_RESERVE ||
                            dragKind == OctiMoveComposer.DragKind.LIBERATE_POD_FROM_RESERVE
                        ) {
                            if (composer.dropOnSquare(sqX, sqY)) {
                                selectedOriginSquare = null
                                viewModel.notifyComposerUpdated()
                                return@detectTapGestures
                            }
                        }

                        // 5. Friendly pod square interaction: guided move start
                        val sq = b.squareAt(sqX - 1, sqY - 1)
                        val turn = b.game?.turn
                        if (!sq.isEmpty && turn != null && !sq.hasEnemyOf(turn)) {
                            val pIdx = composer.selectedStackPodIndex(sqX, sqY)
                            val resolvedIdx = if (pIdx >= 0 && pIdx < sq.numPods) pIdx else 0
                            selectedOriginSquare = Pair(sqX, sqY)
                            composer.beginGuidedPodMove(sqX, sqY, resolvedIdx, viewModel.moveEntireMatchingStack.value)
                            viewModel.bumpRevision()
                            return@detectTapGestures
                        }
                    }
                )
            }
    ) {
        Canvas(
            modifier = Modifier
                .fillMaxSize()
                .graphicsLayer(
                    scaleX = zoomScale,
                    scaleY = zoomScale,
                    translationX = panOffset.x,
                    translationY = panOffset.y,
                    rotationZ = gestureRotationDeg
                )
        ) {
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
