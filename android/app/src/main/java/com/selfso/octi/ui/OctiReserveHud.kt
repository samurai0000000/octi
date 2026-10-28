/*
 * OctiReserveHud.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
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
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.selfso.octi.OctiMoveComposer
import com.selfso.octi.OctiPlayer
import com.selfso.octi.ui.theme.OctiTheme

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

    val activeBoard = if (viewModel.isViewingHistory()) {
        viewModel.displayBoard
    } else {
        viewModel.board
    }
    val game = activeBoard.game
    val turn = game?.turn ?: OctiPlayer.PLAYER_A
    val freePods = game?.getFreePods(turn) ?: 0
    val capPods = game?.getCapturedPods(turn) ?: 0
    val freeProngs = game?.getFreeProngs(turn) ?: 0
    val freeSuper = game?.getFreeSuperProngs(turn) ?: 0
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
                Text(
                    text = "Pods:$freePods  Cap:$capPods  Prongs:$freeProngs" +
                        if (useSuperProngs) "  Super:$freeSuper" else "",
                    style = MaterialTheme.typography.labelLarge,
                    color = OctiTheme.colors.textSecondary
                )
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
                Row(horizontalArrangement = Arrangement.spacedBy(4.dp)) {
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

/*
 * Local variables:
 * mode: Kotlin
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
