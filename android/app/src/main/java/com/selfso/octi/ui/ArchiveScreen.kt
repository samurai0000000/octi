/*
 * ArchiveScreen.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.FilterChip
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.selfso.octi.ui.theme.OctiTheme

/**
 * Completed Game Archive browser, `.octi` transcript VCR loader,
 * and Elo Leaderboard screen (`2P`, `4P`, `OFK`).
 */
@Composable
fun ArchiveScreen(
    viewModel: OctiAppViewModel,
    modifier: Modifier = Modifier
) {
    val leaderboard by viewModel.leaderboardRows.collectAsState()
    val archivePage by viewModel.archivedGamesPage.collectAsState()

    var selectedMode by remember { mutableStateOf("2P") }
    var showLeaderboard by remember { mutableStateOf(true) }

    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(12.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp)
    ) {
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
            border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
        ) {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(12.dp),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    FilterChip(
                        selected = showLeaderboard,
                        onClick = {
                            showLeaderboard = true
                            viewModel.refreshLeaderboard(selectedMode, 50)
                        },
                        label = { Text("Elo Leaderboard") }
                    )
                    FilterChip(
                        selected = !showLeaderboard,
                        onClick = {
                            showLeaderboard = false
                            viewModel.refreshArchivedGames(selectedMode, "", 20, 0)
                        },
                        label = { Text("Game Archive (${archivePage.total})") }
                    )
                }

                Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    listOf("2P", "4P", "OFK").forEach { mode ->
                        FilterChip(
                            selected = selectedMode == mode,
                            onClick = {
                                selectedMode = mode
                                if (showLeaderboard) {
                                    viewModel.refreshLeaderboard(mode, 50)
                                } else {
                                    viewModel.refreshArchivedGames(mode, "", 20, 0)
                                }
                            },
                            label = { Text(mode) }
                        )
                    }
                    OutlinedButton(
                        onClick = {
                            if (showLeaderboard) {
                                viewModel.refreshLeaderboard(selectedMode, 50)
                            } else {
                                viewModel.refreshArchivedGames(selectedMode, "", 20, 0)
                            }
                        }
                    ) {
                        Text("Refresh")
                    }
                }
            }
        }

        Card(
            modifier = Modifier
                .fillMaxWidth()
                .weight(1f),
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
            border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
        ) {
            if (showLeaderboard) {
                LazyColumn(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(12.dp),
                    verticalArrangement = Arrangement.spacedBy(6.dp)
                ) {
                    items(leaderboard) { row ->
                        Card(
                            modifier = Modifier.fillMaxWidth(),
                            colors = CardDefaults.cardColors(
                                containerColor = OctiTheme.colors.windowBg
                            ),
                            border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
                        ) {
                            Row(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .padding(10.dp),
                                horizontalArrangement = Arrangement.SpaceBetween,
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                Column {
                                    Text(
                                        text = "#${row.rank}  ${row.handle}",
                                        style = MaterialTheme.typography.titleMedium,
                                        color = OctiTheme.colors.accentBrass,
                                        fontWeight = FontWeight.Bold
                                    )
                                    Text(
                                        text = "W:${row.wins}  L:${row.losses}  D:${row.draws}  (${row.gamesPlayed} games)",
                                        style = MaterialTheme.typography.bodyMedium,
                                        color = OctiTheme.colors.textSecondary
                                    )
                                }
                                Text(
                                    text = "${row.elo} Elo",
                                    style = MaterialTheme.typography.titleLarge,
                                    color = OctiTheme.colors.textPrimary,
                                    fontWeight = FontWeight.ExtraBold
                                )
                            }
                        }
                    }
                }
            } else {
                LazyColumn(
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(12.dp),
                    verticalArrangement = Arrangement.spacedBy(6.dp)
                ) {
                    items(archivePage.games) { game ->
                        Card(
                            modifier = Modifier.fillMaxWidth(),
                            colors = CardDefaults.cardColors(
                                containerColor = OctiTheme.colors.windowBg
                            ),
                            border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
                        ) {
                            Row(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .padding(10.dp),
                                horizontalArrangement = Arrangement.SpaceBetween,
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                val playersLine = game.players.joinToString(" vs ") { p ->
                                    "S${p.seat}:${p.handle}"
                                }
                                Column(modifier = Modifier.weight(1f)) {
                                    Text(
                                        text = "${game.mode} (${game.bases}-Base) • $playersLine",
                                        style = MaterialTheme.typography.titleMedium,
                                        color = OctiTheme.colors.accentBrass
                                    )
                                    Spacer(modifier = Modifier.height(2.dp))
                                    Text(
                                        text = "Result: ${game.result} (${game.termination}) • ${game.plyCount} plies",
                                        style = MaterialTheme.typography.bodyMedium,
                                        color = OctiTheme.colors.textSecondary
                                    )
                                }
                                Button(
                                    onClick = { viewModel.openArchivedGameForReplay(game.gameId) },
                                    colors = ButtonDefaults.buttonColors(
                                        containerColor = OctiTheme.colors.accentBrass,
                                        contentColor = OctiTheme.colors.windowBg
                                    )
                                ) {
                                    Text("Replay in VCR")
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
