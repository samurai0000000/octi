/*
 * LobbyScreen.kt
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
import androidx.compose.material3.OutlinedTextField
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
import com.selfso.octi.OctiNetworkClient
import com.selfso.octi.ui.theme.OctiTheme

/**
 * Online Lobby, Custom Table Browser, Elo Quick-Play Matchmaking Queue,
 * and Live Lobby Chat screen for `trumpet.selfso.com:6874`.
 *
 * Strictly zero editable server host or port fields are exposed.
 */
@Composable
fun LobbyScreen(
    viewModel: OctiAppViewModel,
    modifier: Modifier = Modifier
) {
    val isAuthenticated by viewModel.isAuthenticated.collectAsState()
    val profile by viewModel.currentProfile.collectAsState()
    val lobbyState by viewModel.lobbyState.collectAsState()
    val mmStatus by viewModel.matchmakingStatus.collectAsState()

    var selectedQueueMode by remember { mutableStateOf("2P") }
    var chatDraft by remember { mutableStateOf("") }
    var passcodeInput by remember { mutableStateOf("") }

    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(12.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp)
    ) {
        // Read-only Canonical Server Endpoint & Identity Header
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
            border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
        ) {
            Column(modifier = Modifier.padding(12.dp)) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Column {
                        Text(
                            text = "OCTI Online Arena",
                            style = MaterialTheme.typography.titleLarge,
                            color = OctiTheme.colors.accentBrass
                        )
                        Text(
                            text = "Endpoint: ${OctiAppViewModel.canonicalServerEndpoint()}",
                            style = MaterialTheme.typography.labelMedium,
                            color = OctiTheme.colors.textSecondary
                        )
                    }
                    Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        if (!isAuthenticated) {
                            Button(
                                onClick = { viewModel.setShowSsoDialog(true) },
                                colors = ButtonDefaults.buttonColors(
                                    containerColor = OctiTheme.colors.accentBrass,
                                    contentColor = OctiTheme.colors.windowBg
                                )
                            ) {
                                Text("Sign In (SSO)")
                            }
                        } else {
                            OutlinedButton(onClick = { viewModel.setShowAccountDialog(true) }) {
                                Text(profile?.handle?.ifEmpty { "Account" } ?: "Account")
                            }
                            OutlinedButton(onClick = { viewModel.refreshLobbyState() }) {
                                Text("Refresh")
                            }
                        }
                    }
                }
            }
        }

        // Elo Matchmaking & Custom Table Action Bar
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
            border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
        ) {
            Column(modifier = Modifier.padding(12.dp)) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                        listOf("2P", "4P", "OFK").forEach { m ->
                            FilterChip(
                                selected = selectedQueueMode == m,
                                onClick = { selectedQueueMode = m },
                                label = { Text(m) }
                            )
                        }
                    }
                    Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        if (!mmStatus.queued) {
                            Button(
                                onClick = {
                                    val req = OctiNetworkClient.MatchmakingRequest().apply {
                                        mode = selectedQueueMode
                                        bases = if (selectedQueueMode == "4P") 1 else 3
                                    }
                                    viewModel.enqueueMatchmaking(req)
                                },
                                enabled = isAuthenticated,
                                colors = ButtonDefaults.buttonColors(
                                    containerColor = OctiTheme.colors.accentBrass,
                                    contentColor = OctiTheme.colors.windowBg
                                )
                            ) {
                                Text("Quick Match ($selectedQueueMode)")
                            }
                        } else {
                            OutlinedButton(onClick = { viewModel.dequeueMatchmaking() }) {
                                Text("Cancel Queue (${mmStatus.waitedMs / 1000}s)")
                            }
                        }

                        OutlinedButton(
                            onClick = { viewModel.setShowCreateTableDialog(true) },
                            enabled = isAuthenticated
                        ) {
                            Text("+ Create Table")
                        }
                    }
                }
            }
        }

        // Active Tables List
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .weight(1f),
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
            border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(12.dp)
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = "Active Tables (${lobbyState.tables.size}) • Online (${lobbyState.players.size})",
                        style = MaterialTheme.typography.titleMedium,
                        color = OctiTheme.colors.textPrimary,
                        fontWeight = FontWeight.Bold
                    )
                    OutlinedTextField(
                        value = passcodeInput,
                        onValueChange = { passcodeInput = it },
                        label = { Text("Room Passcode (optional)") },
                        singleLine = true,
                        modifier = Modifier.fillMaxWidth(0.45f)
                    )
                }

                Spacer(modifier = Modifier.height(8.dp))

                LazyColumn(
                    modifier = Modifier.fillMaxSize(),
                    verticalArrangement = Arrangement.spacedBy(6.dp)
                ) {
                    items(lobbyState.tables) { table ->
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
                                        text = "Room ${table.roomId} • ${table.mode} (${table.bases}-Base)" +
                                            if (table.hasPasscode) " [Locked]" else "",
                                        style = MaterialTheme.typography.titleMedium,
                                        color = OctiTheme.colors.accentBrass
                                    )
                                    Text(
                                        text = "Host: ${table.hostHandle} • Seats: ${table.seatsFilled}/${table.seatsTotal} • Phase: ${table.phase}",
                                        style = MaterialTheme.typography.bodyMedium,
                                        color = OctiTheme.colors.textSecondary
                                    )
                                }
                                Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                                    if (table.seatsFilled < table.seatsTotal) {
                                        Button(
                                            onClick = {
                                                viewModel.joinOnlineTable(
                                                    table.roomId,
                                                    0,
                                                    passcodeInput
                                                )
                                            },
                                            enabled = isAuthenticated,
                                            colors = ButtonDefaults.buttonColors(
                                                containerColor = OctiTheme.colors.accentBrass,
                                                contentColor = OctiTheme.colors.windowBg
                                            )
                                        ) {
                                            Text("Join")
                                        }
                                    }
                                    OutlinedButton(
                                        onClick = {
                                            viewModel.observeOnlineTable(
                                                table.roomId,
                                                passcodeInput
                                            )
                                        },
                                        enabled = isAuthenticated
                                    ) {
                                        Text("Observe")
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Live Lobby Chat Panel
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .height(170.dp),
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
            border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(10.dp)
            ) {
                LazyColumn(
                    modifier = Modifier
                        .fillMaxWidth()
                        .weight(1f)
                ) {
                    items(lobbyState.recentChat) { msg ->
                        Text(
                            text = "[${msg.handle}]: ${msg.text}",
                            style = MaterialTheme.typography.bodyMedium,
                            color = OctiTheme.colors.textPrimary
                        )
                    }
                }
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    OutlinedTextField(
                        value = chatDraft,
                        onValueChange = { chatDraft = it },
                        placeholder = { Text("Lobby chat message...") },
                        singleLine = true,
                        modifier = Modifier.weight(1f)
                    )
                    Button(
                        onClick = {
                            if (chatDraft.isNotBlank()) {
                                viewModel.sendLobbyChat(chatDraft.trim())
                                chatDraft = ""
                            }
                        },
                        enabled = isAuthenticated && chatDraft.isNotBlank(),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = OctiTheme.colors.accentBrass,
                            contentColor = OctiTheme.colors.windowBg
                        )
                    ) {
                        Text("Send")
                    }
                }
            }
        }
    }
}
