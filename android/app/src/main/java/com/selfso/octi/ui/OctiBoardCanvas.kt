/*
 * OctiBoardCanvas.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import android.graphics.Bitmap
import android.os.SystemClock
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
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clipToBounds
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
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
import com.selfso.octi.OctiSoundSynth
import com.selfso.octi.ui.theme.OctiTheme
import kotlin.math.floor
import kotlinx.coroutines.delay

/**
 * Interactive high-DPI OCTI board canvas with 4-quadrant rotation,
 * stack fan overlay hit-testing, radial 8-direction prong compass sector
 * selection, two-finger pinch-to-zoom / 90° rotation, and Smart Auto-Commit /
 * Confirm-All move composition.
 */
@Composable
fun OctiBoardCanvas(
    viewModel: OctiAppViewModel,
    modifier: Modifier = Modifier,
    lastMoveModifiedSquares: List<IntArray> = emptyList(),
    onUserInteraction: () -> Unit = { viewModel.clearLastMoveIndicators() },
    awaitingMatchStart: Boolean = false
) {
    val themeId by viewModel.theme.collectAsState()
    val rotation by viewModel.boardRotation.collectAsState()
    val boardRev by viewModel.boardRevision.collectAsState()
    val viewingPly by viewModel.viewingPly.collectAsState()
    val awaitingConfirm by viewModel.awaitingConfirmation.collectAsState()
    val vmLastMoveSquares by viewModel.lastMoveModifiedSquares.collectAsState()
    val activeModifiedSquares = if (lastMoveModifiedSquares.isNotEmpty()) {
        lastMoveModifiedSquares
    } else {
        vmLastMoveSquares
    }
    val currentOnUserInteraction by rememberUpdatedState(onUserInteraction)
    var canvasSize by remember { mutableStateOf(IntSize(640, 640)) }
    var selectedOriginSquare by remember { mutableStateOf<Pair<Int, Int>?>(null) }
    var zoomScale by remember { mutableStateOf(1.0f) }
    var panOffset by remember { mutableStateOf(Offset.Zero) }
    var gestureRotationDeg by remember { mutableFloatStateOf(0f) }
    var multiTouchActive by remember { mutableStateOf(false) }
    var lastMultiTouchTimeMs by remember { mutableStateOf(0L) }
    var lastDragTimeMs by remember { mutableStateOf(0L) }

    val game = viewModel.board.game
    val isGameInProgress = game != null && game.isGameStarted && !game.isGameOver && !viewModel.isViewingHistory() && !awaitingMatchStart

    val lastMoveAnimPhase by produceState(
        initialValue = 0f,
        isGameInProgress,
        activeModifiedSquares.isNotEmpty(),
        awaitingConfirm,
        boardRev
    ) {
        val shouldAnimate = (isGameInProgress || activeModifiedSquares.isNotEmpty()) && !awaitingConfirm
        if (shouldAnimate) {
            val startMs = SystemClock.uptimeMillis()
            while (true) {
                viewModel.expireLastMoveIndicatorsIfElapsed()
                val elapsedMs = (SystemClock.uptimeMillis() - startMs) % OctiBoardRenderer.INDICATOR_CYCLE_PERIOD_MS
                value = elapsedMs.toFloat() / OctiBoardRenderer.INDICATOR_CYCLE_PERIOD_MS.toFloat()
                delay(16L)
            }
        } else {
            value = 0f
        }
    }

    val renderedBitmap = remember(themeId, rotation, boardRev, viewingPly, canvasSize, awaitingMatchStart) {
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
            val cfg = OctiBoardRenderer.RenderConfig()
            cfg.showTurnIndicator = !awaitingMatchStart
            cfg.lastMoveAnimPhase = lastMoveAnimPhase
            cfg.activeTurnAnimPhase = lastMoveAnimPhase
            OctiBoardRenderer.renderBoardPixels(
                activeBoard,
                viewModel.composer,
                viewModel.geometry,
                themeId,
                cfg
            )
        }
        Bitmap.createBitmap(pixels, vw, vh, Bitmap.Config.ARGB_8888).asImageBitmap()
    }
    val currentRenderedBitmap by rememberUpdatedState(renderedBitmap)

    fun openZoomForOffset(offset: Offset, viewportSize: IntSize) {
        if (awaitingMatchStart || multiTouchActive || System.currentTimeMillis() - lastMultiTouchTimeMs < 250L) {
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
            .pointerInput(awaitingMatchStart) {
                if (awaitingMatchStart) {
                    return@pointerInput
                }
                awaitEachGesture {
                    var hadMultiTouch = false
                    val down = awaitFirstDown(requireUnconsumed = false)
                    currentOnUserInteraction()
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
                                    val curSq = composer.guidedCurrentSquare()
                                    if (composer.clickBoardSquareForGuidedMove(endX, endY)) {
                                        viewModel.refreshDisplayBoard()
                                        val capChoices = composer.captureChoices()
                                        if (composer.isAwaitingConfirmation) {
                                            val dx = Math.abs(endX - curSq[0])
                                            val dy = Math.abs(endY - curSq[1])
                                            val isCap = capChoices.size > prevCapCount
                                            val cue = if (isCap) OctiSoundSynth.Cue.POD_CAPTURE
                                                else if (dx > 1 || dy > 1) OctiSoundSynth.Cue.POD_JUMP
                                                else OctiSoundSynth.Cue.POD_STEP
                                            OctiSoundSynth.playCue(cue, !viewModel.sfxMuted.value)
                                        }
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
                                        if (composer.isAwaitingConfirmation) {
                                            val dx = Math.abs(endX - startX)
                                            val dy = Math.abs(endY - startY)
                                            val isCap = capChoices.size > prevCapCount
                                            val cue = if (isCap) OctiSoundSynth.Cue.POD_CAPTURE
                                                else if (dx > 1 || dy > 1) OctiSoundSynth.Cue.POD_JUMP
                                                else OctiSoundSynth.Cue.POD_STEP
                                            OctiSoundSynth.playCue(cue, !viewModel.sfxMuted.value)
                                        }
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
            .pointerInput(awaitingMatchStart) {
                if (awaitingMatchStart) {
                    return@pointerInput
                }
                detectTapGestures(
                    onDoubleTap = { offset ->
                        currentOnUserInteraction()
                        openZoomForOffset(offset, size)
                    },
                    onLongPress = { offset ->
                        currentOnUserInteraction()
                        openZoomForOffset(offset, size)
                    },
                    onTap = { offset ->
                        currentOnUserInteraction()
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
                                val curSq = composer.guidedCurrentSquare()
                                if (composer.selectGuidedDestination(sqX, sqY)) {
                                    val capChoices = composer.captureChoices()
                                    if (composer.isAwaitingConfirmation) {
                                        val dx = Math.abs(sqX - curSq[0])
                                        val dy = Math.abs(sqY - curSq[1])
                                        val isCap = capChoices.size > prevCapCount
                                        val cue = if (isCap) OctiSoundSynth.Cue.POD_CAPTURE
                                            else if (dx > 1 || dy > 1) OctiSoundSynth.Cue.POD_JUMP
                                            else OctiSoundSynth.Cue.POD_STEP
                                        OctiSoundSynth.playCue(cue, !viewModel.sfxMuted.value)
                                    }
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
                                if (composer.isAwaitingConfirmation) {
                                    OctiSoundSynth.playCue(OctiSoundSynth.Cue.POD_PLACE, !viewModel.sfxMuted.value)
                                }
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
            if (isGameInProgress && !awaitingMatchStart) {
                val scaleX = size.width / Math.max(1, currentRenderedBitmap.width).toFloat()
                val scaleY = size.height / Math.max(1, currentRenderedBitmap.height).toFloat()
                val geo = viewModel.geometry
                val grid = geo.gridInnerRect()
                val activeTurn = game?.turn
                if (grid != null && grid.size >= 4 && grid[2] > 0.0 && grid[3] > 0.0 && activeTurn != null) {
                    val edge = OctiBoardRenderer.activeTurnScreenEdge(game.mode, activeTurn, rotation.degrees)
                    val inset = 1.0f
                    val left = (grid[0].toFloat() + inset) * scaleX
                    val top = (grid[1].toFloat() + inset) * scaleY
                    val right = (grid[0] + grid[2] - inset).toFloat() * scaleX
                    val bottom = (grid[1] + grid[3] - inset).toFloat() * scaleY

                    val x1: Float
                    val y1: Float
                    val x2: Float
                    val y2: Float
                    when (edge) {
                        0 -> { // Bottom edge: left to right
                            x1 = left; y1 = bottom; x2 = right; y2 = bottom
                        }
                        1 -> { // Left edge: top to bottom
                            x1 = left; y1 = top; x2 = left; y2 = bottom
                        }
                        2 -> { // Top edge: right to left
                            x1 = right; y1 = top; x2 = left; y2 = top
                        }
                        3 -> { // Right edge: bottom to top
                            x1 = right; y1 = bottom; x2 = right; y2 = top
                        }
                        else -> {
                            x1 = left; y1 = bottom; x2 = right; y2 = bottom
                        }
                    }

                    val segFrac = 0.30f
                    val strokeW = Math.max(3.5f, (geo.cellSize().toFloat() * scaleX) * 0.065f)
                    val normPhase = lastMoveAnimPhase - floor(lastMoveAnimPhase)
                    val s0 = normPhase
                    val s1 = normPhase + segFrac

                    fun drawTurnSeg(t0: Float, t1: Float) {
                        val sx = x1 + t0 * (x2 - x1)
                        val sy = y1 + t0 * (y2 - y1)
                        val ex = x1 + t1 * (x2 - x1)
                        val ey = y1 + t1 * (y2 - y1)
                        // Glow stroke
                        drawLine(
                            color = Color(0x55FF69B4),
                            start = Offset(sx, sy),
                            end = Offset(ex, ey),
                            strokeWidth = strokeW + 3.0f,
                            cap = StrokeCap.Round
                        )
                        // Core stroke
                        drawLine(
                            color = Color(0xFFFF69B4),
                            start = Offset(sx, sy),
                            end = Offset(ex, ey),
                            strokeWidth = strokeW,
                            cap = StrokeCap.Round
                        )
                    }

                    if (s1 <= 1.0f) {
                        drawTurnSeg(s0, s1)
                    } else {
                        drawTurnSeg(s0, 1.0f)
                        drawTurnSeg(0.0f, s1 - 1.0f)
                    }
                }
            }
            if (activeModifiedSquares.isNotEmpty() && !awaitingConfirm && !viewModel.composer.isAwaitingConfirmation) {
                val scaleX = size.width / Math.max(1, currentRenderedBitmap.width).toFloat()
                val scaleY = size.height / Math.max(1, currentRenderedBitmap.height).toFloat()
                val geo = viewModel.geometry
                val gw = geo.gridWidth()
                val gh = geo.gridHeight()
                val phase = lastMoveAnimPhase - floor(lastMoveAnimPhase)
                val s0 = phase * 4.0f
                val segLen = 1.2f
                val orangeColor = Color(OctiBoardRenderer.LAST_MOVE_INDICATOR_COLOR.toLong() and 0xFFFFFFFFL)
                for (sq in activeModifiedSquares) {
                    if (sq.size < 2) continue
                    val sx = sq[0]
                    val sy = sq[1]
                    if (sx < 1 || sx > gw || sy < 1 || sy > gh) continue
                    val sqRect = geo.squareRect(sx, sy)
                    if (sqRect.size < 4 || sqRect[2] <= 3.0 || sqRect[3] <= 3.0) continue
                    val left = (sqRect[0] + 1.5).toFloat() * scaleX
                    val top = (sqRect[1] + 1.5).toFloat() * scaleY
                    val right = (sqRect[0] + sqRect[2] - 1.5).toFloat() * scaleX
                    val bottom = (sqRect[1] + sqRect[3] - 1.5).toFloat() * scaleY
                    val strokeW = Math.max(3.0f, (sqRect[2].toFloat() * scaleX) * 0.055f)
                    var cur = s0
                    val end = s0 + segLen
                    while (cur < end - 1e-5f) {
                        val edgeBase = floor(cur)
                        val nextBoundary = Math.min(end, edgeBase + 1.0f)
                        var edgeIdx = edgeBase.toInt() % 4
                        if (edgeIdx < 0) edgeIdx += 4
                        val t0 = cur - edgeBase
                        val t1 = nextBoundary - edgeBase
                        val x1: Float
                        val y1: Float
                        val x2: Float
                        val y2: Float
                        when (edgeIdx) {
                            0 -> {
                                x1 = left + t0 * (right - left)
                                y1 = top
                                x2 = left + t1 * (right - left)
                                y2 = top
                            }
                            1 -> {
                                x1 = right
                                y1 = top + t0 * (bottom - top)
                                x2 = right
                                y2 = top + t1 * (bottom - top)
                            }
                            2 -> {
                                x1 = right - t0 * (right - left)
                                y1 = bottom
                                x2 = right - t1 * (right - left)
                                y2 = bottom
                            }
                            else -> {
                                x1 = left
                                y1 = bottom - t0 * (bottom - top)
                                x2 = left
                                y2 = bottom - t1 * (bottom - top)
                            }
                        }
                        drawLine(
                            color = orangeColor,
                            start = Offset(x1, y1),
                            end = Offset(x2, y2),
                            strokeWidth = strokeW,
                            cap = StrokeCap.Round
                        )
                        cur = nextBoundary
                    }
                }
            }
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
