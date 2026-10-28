/*
 * OctiReserveHud.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.selfso.octi.OctiGameMode
import com.selfso.octi.OctiMoveComposer
import com.selfso.octi.OctiPalette
import com.selfso.octi.OctiPlayer
import com.selfso.octi.ui.theme.OctiTheme
import kotlin.math.cos
import kotlin.math.min
import kotlin.math.sin

/**
 * Tabletop reserve HUD, move confirmation dock, VCR move-history scrubber,
 * and Push-to-Talk voice controls.
 */
@OptIn(ExperimentalLayoutApi::class)
@Composable
fun OctiReserveHud(
    viewModel: OctiAppViewModel,
    modifier: Modifier = Modifier
) {
    val boardRev by viewModel.boardRevision.collectAsState()
    val currentBoardRev = boardRev
    val awaitingConfirmation by viewModel.awaitingConfirmation.collectAsState()
    val stagedMoveNotation by viewModel.stagedMoveNotation.collectAsState()
    val canAddMoreMoves by viewModel.canAddMoreMoves.collectAsState()
    val viewingPly by viewModel.viewingPly.collectAsState()
    val confirmAll by viewModel.confirmAllMoves.collectAsState()
    val moveStack by viewModel.moveEntireMatchingStack.collectAsState()
    val matchType by viewModel.matchType.collectAsState()
    val inVoiceRoom by viewModel.isInVoiceRoom.collectAsState()
    val isPttActive by viewModel.isPttActive.collectAsState()
    val activeRoomId by viewModel.activeRoomId.collectAsState()
    val themeId by viewModel.theme.collectAsState()
    val pal = OctiPalette.forTheme(themeId)

    val activeBoard = if (viewModel.isViewingHistory()) {
        viewModel.displayBoard
    } else {
        viewModel.board
    }
    val game = activeBoard.game
    val turn = game?.turn ?: OctiPlayer.PLAYER_A
    val useSuperProngs = game?.useSuperProngs() == true
    val isGameOver = game?.isGameOver == true
    val awaitingConfirm = (awaitingConfirmation || viewModel.composer.isAwaitingConfirmation) && currentBoardRev >= 0L
    val stagedNotation = if (awaitingConfirm) {
        stagedMoveNotation.ifEmpty { viewModel.composer.stagedMoveNotation() }
    } else {
        ""
    }
    val dragKind = viewModel.composer.activeDragKind()
    val totalPlies = viewModel.livePlyCount()

    Card(
        modifier = modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
        border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
    ) {
        Column(modifier = Modifier.padding(10.dp)) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = if (isGameOver) {
                        "Winner: ${game?.winner}"
                    } else {
                        "Turn: ${turn.name} (Ply $viewingPly/$totalPlies)"
                    },
                    style = MaterialTheme.typography.titleMedium,
                    color = OctiTheme.colors.accentBrass,
                    fontWeight = FontWeight.Bold
                )
            }

            Spacer(modifier = Modifier.height(6.dp))

            val gameMode = game?.mode ?: OctiGameMode.GAME_MODE_2
            val players = if (gameMode == OctiGameMode.GAME_MODE_4) {
                listOf(OctiPlayer.PLAYER_A, OctiPlayer.PLAYER_B, OctiPlayer.PLAYER_C, OctiPlayer.PLAYER_D)
            } else {
                listOf(OctiPlayer.PLAYER_A, OctiPlayer.PLAYER_B)
            }

            if (players.size == 2) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(6.dp)
                ) {
                    for (p in players) {
                        PlayerReserveCard(
                            player = p,
                            isTurn = !isGameOver && (p == turn),
                            freePods = game?.getFreePods(p) ?: 0,
                            freeProngs = game?.getFreeProngs(p) ?: 0,
                            freeSuper = game?.getFreeSuperProngs(p) ?: 0,
                            capPods = game?.getCapturedPods(p) ?: 0,
                            useSuperProngs = useSuperProngs,
                            pal = pal,
                            modifier = Modifier.weight(1f)
                        )
                    }
                }
            } else {
                Column(
                    modifier = Modifier.fillMaxWidth(),
                    verticalArrangement = Arrangement.spacedBy(4.dp)
                ) {
                    val chunked = players.chunked(2)
                    for (rowPlayers in chunked) {
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.spacedBy(6.dp)
                        ) {
                            for (p in rowPlayers) {
                                PlayerReserveCard(
                                    player = p,
                                    isTurn = !isGameOver && (p == turn),
                                    freePods = game?.getFreePods(p) ?: 0,
                                    freeProngs = game?.getFreeProngs(p) ?: 0,
                                    freeSuper = game?.getFreeSuperProngs(p) ?: 0,
                                    capPods = game?.getCapturedPods(p) ?: 0,
                                    useSuperProngs = useSuperProngs,
                                    pal = pal,
                                    modifier = Modifier.weight(1f)
                                )
                            }
                        }
                    }
                }
            }

            Spacer(modifier = Modifier.height(6.dp))

            val isLocalMatch = matchType == OctiAppViewModel.MatchType.LOCAL_PASS_AND_PLAY ||
                matchType == OctiAppViewModel.MatchType.LOCAL_VS_BOT
            val canUndo = isLocalMatch && (viewModel.board.game?.canUndo() == true)

            FlowRow(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(6.dp),
                verticalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                if (isLocalMatch) {
                    OutlinedButton(
                        onClick = { viewModel.undoLocalMove() },
                        enabled = canUndo && !viewModel.isViewingHistory()
                    ) {
                        Text("Undo")
                    }
                    OutlinedButton(
                        onClick = { viewModel.resignLocalGame() },
                        enabled = !isGameOver && !viewModel.isViewingHistory()
                    ) {
                        Text("Resign")
                    }
                }
                if (viewModel.composer.isGuidedMoveActive && boardRev >= 0L) {
                    OutlinedButton(
                        onClick = { viewModel.rollbackStagedMove() }
                    ) {
                        Text("Cancel Move")
                    }
                }
            }

            Spacer(modifier = Modifier.height(6.dp))

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Row(
                    horizontalArrangement = Arrangement.spacedBy(4.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    OutlinedButton(
                        onClick = { viewModel.vcrFirst() },
                        enabled = viewingPly > 0
                    ) {
                        Text("|<<")
                    }
                    OutlinedButton(
                        onClick = { viewModel.vcrPrev() },
                        enabled = viewingPly > 0
                    ) {
                        Text("<")
                    }
                    Text(
                        text = "Ply $viewingPly/$totalPlies",
                        style = MaterialTheme.typography.labelSmall,
                        color = OctiTheme.colors.textSecondary,
                        modifier = Modifier.padding(horizontal = 2.dp)
                    )
                    OutlinedButton(
                        onClick = { viewModel.vcrNext() },
                        enabled = viewingPly < totalPlies
                    ) {
                        Text(">")
                    }
                    OutlinedButton(
                        onClick = { viewModel.vcrLast() },
                        enabled = viewingPly < totalPlies
                    ) {
                        Text(">>|")
                    }
                }
                val context = LocalContext.current
                OutlinedButton(
                    onClick = {
                        val text = viewModel.exportMoveHistoryText()
                        val clipboard = context.getSystemService(Context.CLIPBOARD_SERVICE) as? ClipboardManager
                        val clip = ClipData.newPlainText("OCTI Moves", text)
                        clipboard?.setPrimaryClip(clip)
                        viewModel.setShowExportMovesDialog(true)
                    }
                ) {
                    Text("Export")
                }
            }

            if (matchType == OctiAppViewModel.MatchType.ONLINE_ROOM && activeRoomId.isNotEmpty()) {
                Spacer(modifier = Modifier.height(6.dp))
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                        if (!inVoiceRoom) {
                            OutlinedButton(onClick = { viewModel.joinVoiceChannel() }) {
                                Text("Join Voice")
                            }
                        } else {
                            Button(
                                onClick = { viewModel.setPushToTalkPressed(!isPttActive) },
                                colors = ButtonDefaults.buttonColors(
                                    containerColor = if (isPttActive) {
                                        OctiTheme.colors.legalStep
                                    } else {
                                        OctiTheme.colors.accentBrass
                                    },
                                    contentColor = OctiTheme.colors.windowBg
                                )
                            ) {
                                Text(if (isPttActive) "PTT: TALKING" else "PTT: Muted")
                            }
                            Spacer(modifier = Modifier.width(4.dp))
                            OutlinedButton(onClick = { viewModel.leaveVoiceChannel() }) {
                                Text("Leave Voice")
                            }
                        }
                    }

                    Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                        OutlinedButton(onClick = { viewModel.submitGameAction("DRAW_OFFER") }) {
                            Text("Draw")
                        }
                        OutlinedButton(onClick = { viewModel.resignOnlineGame() }) {
                            Text("Resign")
                        }
                    }
                }
            }
        }
    }
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun PlayerReserveCard(
    player: OctiPlayer,
    isTurn: Boolean,
    freePods: Int,
    freeProngs: Int,
    freeSuper: Int,
    capPods: Int,
    useSuperProngs: Boolean,
    pal: OctiPalette,
    modifier: Modifier = Modifier
) {
    val faction = pal.factionForPlayer(player)
    val oppPlayer = if (player == OctiPlayer.PLAYER_A) OctiPlayer.PLAYER_B else OctiPlayer.PLAYER_A
    val oppFaction = pal.factionForPlayer(oppPlayer)
    val letter = player.name.removePrefix("PLAYER_")

    Card(
        modifier = modifier,
        colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.windowBg),
        border = if (isTurn) {
            BorderStroke(1.5.dp, OctiTheme.colors.accentBrass)
        } else {
            BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
        }
    ) {
        Column(
            modifier = Modifier.padding(horizontal = 8.dp, vertical = 6.dp)
        ) {
            Text(
                text = "PLAYER $letter",
                style = MaterialTheme.typography.labelSmall,
                fontWeight = FontWeight.Bold,
                color = if (isTurn) OctiTheme.colors.accentBrass else OctiTheme.colors.textSecondary
            )
            Spacer(modifier = Modifier.height(4.dp))
            FlowRow(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
                verticalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                // 1. Pod icon + count
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(3.dp)
                ) {
                    PodIcon(faction = faction, letter = letter, modifier = Modifier.size(20.dp))
                    Text(
                        text = "$freePods",
                        style = MaterialTheme.typography.labelMedium,
                        fontWeight = FontWeight.Bold,
                        color = OctiTheme.colors.textPrimary
                    )
                }

                // 2. Prong icon + count
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(3.dp)
                ) {
                    ProngIcon(pal = pal, modifier = Modifier.size(13.dp, 20.dp))
                    Text(
                        text = "$freeProngs",
                        style = MaterialTheme.typography.labelMedium,
                        fontWeight = FontWeight.Bold,
                        color = OctiTheme.colors.textPrimary
                    )
                }

                // 3. Superprong icon + count (if enabled)
                if (useSuperProngs) {
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(3.dp)
                    ) {
                        SuperProngIcon(pal = pal, modifier = Modifier.size(15.dp, 20.dp))
                        Text(
                            text = "$freeSuper",
                            style = MaterialTheme.typography.labelMedium,
                            fontWeight = FontWeight.Bold,
                            color = OctiTheme.colors.accentBrass
                        )
                    }
                }

                // 4. Captured Pod icon + count
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(3.dp)
                ) {
                    CapturedPodIcon(oppFaction = oppFaction, pal = pal, modifier = Modifier.size(20.dp))
                    Text(
                        text = "$capPods",
                        style = MaterialTheme.typography.labelMedium,
                        fontWeight = FontWeight.Bold,
                        color = Color(pal.legalCaptureGlow.toLong() and 0xFFFFFFFFL)
                    )
                }
            }
        }
    }
}

@Composable
fun PodIcon(
    faction: OctiPalette.PodFactionPalette,
    letter: String,
    modifier: Modifier = Modifier
) {
    Box(modifier = modifier, contentAlignment = Alignment.Center) {
        Canvas(modifier = Modifier.fillMaxSize()) {
            val w = size.width
            val h = size.height
            val cx = w * 0.5f
            val cy = h * 0.5f
            val r = min(cx, cy) * 0.92f
            val octoPath = Path()
            for (i in 0 until 8) {
                val angle = (i * 45.0 + 22.5) * Math.PI / 180.0
                val px = cx + cos(angle).toFloat() * r
                val py = cy + sin(angle).toFloat() * r
                if (i == 0) octoPath.moveTo(px, py) else octoPath.lineTo(px, py)
            }
            octoPath.close()

            drawPath(
                path = octoPath,
                brush = Brush.radialGradient(
                    colors = listOf(
                        Color(faction.highlight.toLong() and 0xFFFFFFFFL),
                        Color(faction.core.toLong() and 0xFFFFFFFFL),
                        Color(faction.shadow.toLong() and 0xFFFFFFFFL)
                    ),
                    center = Offset(cx - r * 0.2f, cy - r * 0.2f),
                    radius = r * 1.2f
                )
            )
            drawPath(
                path = octoPath,
                color = Color(faction.rim.toLong() and 0xFFFFFFFFL),
                style = Stroke(width = 1.2.dp.toPx())
            )
        }
        Text(
            text = letter,
            fontSize = 9.sp,
            fontWeight = FontWeight.ExtraBold,
            color = Color(faction.emblem.toLong() and 0xFFFFFFFFL)
        )
    }
}

@Composable
fun ProngIcon(
    pal: OctiPalette,
    modifier: Modifier = Modifier
) {
    Canvas(modifier = modifier) {
        val w = size.width
        val h = size.height
        val cx = w * 0.5f
        val tipY = 1.dp.toPx()
        val baseY = h - 1.dp.toPx()
        val midY = tipY + (baseY - tipY) * 0.55f
        val baseHalfW = w * 0.45f
        val midHalfW = w * 0.28f

        val path = Path().apply {
            moveTo(cx - baseHalfW, baseY)
            lineTo(cx - midHalfW, midY)
            lineTo(cx, tipY)
            lineTo(cx + midHalfW, midY)
            lineTo(cx + baseHalfW, baseY)
            close()
        }
        drawPath(
            path = path,
            brush = Brush.linearGradient(
                colors = listOf(
                    Color(pal.prongNormalTip.toLong() and 0xFFFFFFFFL),
                    Color(pal.prongNormalMetal.toLong() and 0xFFFFFFFFL)
                ),
                start = Offset(cx, tipY),
                end = Offset(cx, baseY)
            )
        )
        drawPath(
            path = path,
            color = Color(0xFF1E293B),
            style = Stroke(width = 1.dp.toPx())
        )
        drawLine(
            color = Color.White.copy(alpha = 0.55f),
            start = Offset(cx, tipY + 1.dp.toPx()),
            end = Offset(cx, baseY - 1.dp.toPx()),
            strokeWidth = 1.dp.toPx()
        )
    }
}

@Composable
fun SuperProngIcon(
    pal: OctiPalette,
    modifier: Modifier = Modifier
) {
    Canvas(modifier = modifier) {
        val w = size.width
        val h = size.height
        val cx = w * 0.5f
        val tipY = 1.dp.toPx()
        val baseY = h - 1.dp.toPx()
        val midY = tipY + (baseY - tipY) * 0.50f
        val baseHalfW = w * 0.48f
        val midHalfW = w * 0.35f

        val path = Path().apply {
            moveTo(cx - baseHalfW, baseY)
            lineTo(cx - midHalfW, midY)
            lineTo(cx, tipY)
            lineTo(cx + midHalfW, midY)
            lineTo(cx + baseHalfW, baseY)
            close()
        }
        drawPath(
            path = path,
            brush = Brush.linearGradient(
                colors = listOf(
                    Color(pal.prongSuperCore.toLong() and 0xFFFFFFFFL),
                    Color(pal.prongSuperGold.toLong() and 0xFFFFFFFFL)
                ),
                start = Offset(cx, tipY),
                end = Offset(cx, baseY)
            )
        )
        drawPath(
            path = path,
            color = Color(0xFF78350F),
            style = Stroke(width = 1.2.dp.toPx())
        )
        drawLine(
            color = Color(0xFFFFFBEB),
            start = Offset(cx, tipY + 1.dp.toPx()),
            end = Offset(cx, baseY - 1.dp.toPx()),
            strokeWidth = 1.2.dp.toPx()
        )
    }
}

@Composable
fun CapturedPodIcon(
    oppFaction: OctiPalette.PodFactionPalette,
    pal: OctiPalette,
    modifier: Modifier = Modifier
) {
    Canvas(modifier = modifier) {
        val w = size.width
        val h = size.height
        val cx = w * 0.5f
        val cy = h * 0.5f
        val r = min(cx, cy) * 0.78f

        val octoPath = Path()
        for (i in 0 until 8) {
            val angle = (i * 45.0 + 22.5) * Math.PI / 180.0
            val px = cx + cos(angle).toFloat() * r
            val py = cy + sin(angle).toFloat() * r
            if (i == 0) octoPath.moveTo(px, py) else octoPath.lineTo(px, py)
        }
        octoPath.close()

        drawPath(
            path = octoPath,
            brush = Brush.radialGradient(
                colors = listOf(
                    Color(oppFaction.highlight.toLong() and 0xFFFFFFFFL),
                    Color(oppFaction.core.toLong() and 0xFFFFFFFFL),
                    Color(oppFaction.shadow.toLong() and 0xFFFFFFFFL)
                ),
                center = Offset(cx - r * 0.2f, cy - r * 0.2f),
                radius = r * 1.2f
            )
        )
        drawPath(
            path = octoPath,
            color = Color(oppFaction.rim.toLong() and 0xFFFFFFFFL),
            style = Stroke(width = 1.dp.toPx())
        )

        val captureColor = Color(pal.legalCaptureGlow.toLong() and 0xFFFFFFFFL)
        drawCircle(
            color = captureColor,
            radius = min(cx, cy) * 0.94f,
            center = Offset(cx, cy),
            style = Stroke(width = 1.4.dp.toPx())
        )
        val strikeR = min(cx, cy) * 0.75f
        drawLine(
            color = captureColor,
            start = Offset(cx - strikeR, cy - strikeR),
            end = Offset(cx + strikeR, cy + strikeR),
            strokeWidth = 1.8.dp.toPx()
        )
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
