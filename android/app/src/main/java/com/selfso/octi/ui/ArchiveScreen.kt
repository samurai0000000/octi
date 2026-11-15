/*
 * ArchiveScreen.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.FilterChip
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.selfso.octi.OctiGameMode
import com.selfso.octi.ui.theme.OctiTheme

enum class ArchiveSubTab {
    LEADERBOARD,
    ARCHIVE,
    ROBOTS
}

/**
 * Completed Game Archive browser (13 canonical variants), `.octi` transcript VCR loader,
 * Pool 1 Elo Leaderboard screen (`2P_3B`, `2P_1B`, `OFK`), and merged Pool 2 Arena Directory.
 */
@Composable
fun ArchiveScreen(
    viewModel: OctiAppViewModel,
    modifier: Modifier = Modifier
) {
    val leaderboard by viewModel.leaderboardRows.collectAsState()
    val archivePage by viewModel.archivedGamesPage.collectAsState()
    val robots by viewModel.robotsDirectory.collectAsState()
    val selectedRobotProfile by viewModel.selectedRobotProfile.collectAsState()
    val currentProfile by viewModel.currentProfile.collectAsState()

    var selectedLadder by remember { mutableStateOf("2P_3B") }
    var selectedVariantFilter by remember { mutableStateOf("") }
    var selectedParticipantFilter by remember { mutableStateOf("all") }
    var selectedArenaSort by remember { mutableStateOf("elo_2p_3b") }
    var currentSubTab by remember { mutableStateOf(ArchiveSubTab.LEADERBOARD) }

    val filteredArenaEntries = remember(robots, selectedParticipantFilter) {
        when (selectedParticipantFilter) {
            "robots" -> robots.filter { !it.entryType.equals("human", ignoreCase = true) }
            "humans" -> robots.filter { it.entryType.equals("human", ignoreCase = true) }
            else -> robots
        }
    }

    LaunchedEffect(Unit) {
        viewModel.refreshRobotsDirectoryAsync("", selectedArenaSort)
    }

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
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(12.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Row(
                        modifier = Modifier
                            .weight(1f)
                            .horizontalScroll(rememberScrollState()),
                        horizontalArrangement = Arrangement.spacedBy(6.dp)
                    ) {
                        FilterChip(
                            selected = currentSubTab == ArchiveSubTab.LEADERBOARD,
                            onClick = {
                                currentSubTab = ArchiveSubTab.LEADERBOARD
                                viewModel.refreshLeaderboardAsync(selectedLadder, 50)
                            },
                            label = { Text("Elo Leaderboard") }
                        )
                        FilterChip(
                            selected = currentSubTab == ArchiveSubTab.ARCHIVE,
                            onClick = {
                                currentSubTab = ArchiveSubTab.ARCHIVE
                                viewModel.refreshArchivedGamesAsync(selectedVariantFilter, "", 20, 0)
                            },
                            label = { Text("Game Archive (${archivePage.total})") }
                        )
                        FilterChip(
                            selected = currentSubTab == ArchiveSubTab.ROBOTS,
                            onClick = {
                                currentSubTab = ArchiveSubTab.ROBOTS
                                viewModel.refreshRobotsDirectoryAsync("", selectedArenaSort)
                            },
                            label = { Text("Arena (${filteredArenaEntries.size})") }
                        )
                    }

                    Spacer(modifier = Modifier.width(6.dp))

                    OutlinedButton(
                        onClick = {
                            when (currentSubTab) {
                                ArchiveSubTab.LEADERBOARD -> viewModel.refreshLeaderboardAsync(selectedLadder, 50)
                                ArchiveSubTab.ARCHIVE -> viewModel.refreshArchivedGamesAsync(selectedVariantFilter, "", 20, 0)
                                ArchiveSubTab.ROBOTS -> viewModel.refreshRobotsDirectoryAsync("", selectedArenaSort)
                            }
                        }
                    ) {
                        Text("Refresh")
                    }
                }

                when (currentSubTab) {
                    ArchiveSubTab.LEADERBOARD -> {
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .horizontalScroll(rememberScrollState()),
                            horizontalArrangement = Arrangement.spacedBy(6.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            OctiDialogs.leaderboardLadders().forEach { ladder ->
                                FilterChip(
                                    selected = selectedLadder == ladder,
                                    onClick = {
                                        selectedLadder = ladder
                                        viewModel.refreshLeaderboardAsync(ladder, 50)
                                    },
                                    label = { Text(ladder) }
                                )
                            }
                        }
                    }
                    ArchiveSubTab.ARCHIVE -> {
                        val variantOptions = remember {
                            listOf(
                                "" to "All",
                                "2P_ALL" to "2P (All)",
                                "4P_ALL" to "4P (All)"
                            ) + OctiGameMode.allVariantCodes().map { it to it }
                        }
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .horizontalScroll(rememberScrollState()),
                            horizontalArrangement = Arrangement.spacedBy(6.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            variantOptions.forEach { (code, label) ->
                                FilterChip(
                                    selected = selectedVariantFilter == code,
                                    onClick = {
                                        selectedVariantFilter = code
                                        viewModel.refreshArchivedGamesAsync(code, "", 20, 0)
                                    },
                                    label = { Text(label) }
                                )
                            }
                        }
                    }
                    ArchiveSubTab.ROBOTS -> {
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .horizontalScroll(rememberScrollState()),
                            horizontalArrangement = Arrangement.spacedBy(6.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            OctiDialogs.arenaParticipantFilters().forEach { pType ->
                                val pLabel = when (pType) {
                                    "robots" -> "Bots Only"
                                    "humans" -> "Humans Only"
                                    else -> "All Challengers"
                                }
                                FilterChip(
                                    selected = selectedParticipantFilter == pType,
                                    onClick = { selectedParticipantFilter = pType },
                                    label = { Text(pLabel) }
                                )
                            }
                            listOf(
                                "elo_2p_3b" to "Sort: 2P_3B",
                                "elo_2p_1b" to "Sort: 2P_1B",
                                "elo_ofk" to "Sort: OFK",
                                "wins" to "Sort: Wins"
                            ).forEach { (sortKey, sortLabel) ->
                                FilterChip(
                                    selected = selectedArenaSort == sortKey,
                                    onClick = {
                                        selectedArenaSort = sortKey
                                        viewModel.refreshRobotsDirectoryAsync("", sortKey)
                                    },
                                    label = { Text(sortLabel) }
                                )
                            }
                        }
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
            when (currentSubTab) {
                ArchiveSubTab.LEADERBOARD -> {
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
                }
                ArchiveSubTab.ARCHIVE -> {
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
                                            text = "${game.variantCode()} • $playersLine",
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
                                    Row(
                                        horizontalArrangement = Arrangement.spacedBy(6.dp),
                                        verticalAlignment = Alignment.CenterVertically
                                    ) {
                                        Button(
                                            onClick = { viewModel.openArchivedGameForReplayAsync(game.gameId) },
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
                ArchiveSubTab.ROBOTS -> {
                    val p = selectedRobotProfile
                    if (p != null) {
                        val isHumanProfile = !p.isRobot || p.entryType.equals("human", ignoreCase = true)
                        Column(
                            modifier = Modifier
                                .fillMaxSize()
                                .padding(12.dp),
                            verticalArrangement = Arrangement.spacedBy(10.dp)
                        ) {
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.SpaceBetween,
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                Row(
                                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    Text(
                                        text = if (isHumanProfile) "@${p.handle} [HUMAN]" else "@${p.handle} [BOT]",
                                        style = MaterialTheme.typography.titleLarge,
                                        fontWeight = FontWeight.Bold,
                                        color = OctiTheme.colors.accentBrass
                                    )
                                    if (!isHumanProfile) {
                                        val isRetired = p.retired || p.revoked || p.status.equals("retired", ignoreCase = true)
                                        Text(
                                            text = if (isRetired) "[Retired]" else "[Active]",
                                            style = MaterialTheme.typography.labelMedium,
                                            fontWeight = FontWeight.Bold,
                                            color = if (isRetired) OctiTheme.colors.textSecondary else OctiTheme.colors.accentBrass
                                        )
                                    }
                                }
                                OutlinedButton(onClick = { viewModel.clearSelectedRobotProfile() }) {
                                    Text("Back to Directory")
                                }
                            }

                            if (!isHumanProfile && p.ownerHandle.isNotEmpty()) {
                                Text(
                                    text = "Owner: @${p.ownerHandle}",
                                    style = MaterialTheme.typography.bodyMedium,
                                    color = OctiTheme.colors.textSecondary
                                )
                            }
                            if (p.description.isNotEmpty()) {
                                Text(
                                    text = p.description,
                                    style = MaterialTheme.typography.bodyMedium,
                                    color = OctiTheme.colors.textPrimary
                                )
                            }

                            Card(
                                modifier = Modifier.fillMaxWidth(),
                                colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.windowBg),
                                border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
                            ) {
                                Column(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(10.dp),
                                    verticalArrangement = Arrangement.spacedBy(6.dp)
                                ) {
                                    Row(
                                        modifier = Modifier.fillMaxWidth(),
                                        horizontalArrangement = Arrangement.SpaceEvenly,
                                        verticalAlignment = Alignment.CenterVertically
                                    ) {
                                        Text("2P_3B: ${p.elo2p3b}", fontWeight = FontWeight.Bold, color = OctiTheme.colors.accentBrass)
                                        Text("2P_1B: ${p.elo2p1b}", fontWeight = FontWeight.Bold, color = OctiTheme.colors.accentBrass)
                                        Text("OFK: ${p.eloOfk}", fontWeight = FontWeight.Bold, color = OctiTheme.colors.accentBrass)
                                    }
                                    Row(
                                        modifier = Modifier.fillMaxWidth(),
                                        horizontalArrangement = Arrangement.SpaceEvenly,
                                        verticalAlignment = Alignment.CenterVertically
                                    ) {
                                        Text("Wins: ${p.wins}", fontWeight = FontWeight.Bold, color = OctiTheme.colors.accentBrass)
                                        Text("Losses: ${p.losses}", color = OctiTheme.colors.textSecondary)
                                        Text("Draws: ${p.draws}", color = OctiTheme.colors.textSecondary)
                                        Text("Total: ${p.gamesPlayed}", color = OctiTheme.colors.textPrimary)
                                        Text("Win Rate: ${"%.1f".format(p.winRate)}%", fontWeight = FontWeight.Bold, color = OctiTheme.colors.accentBrass)
                                    }
                                }
                            }

                            Text(
                                text = "Recent Match History",
                                style = MaterialTheme.typography.titleMedium,
                                fontWeight = FontWeight.Bold,
                                color = OctiTheme.colors.accentBrass
                            )

                            if (p.recentGames.isEmpty()) {
                                Box(
                                    modifier = Modifier.fillMaxSize(),
                                    contentAlignment = Alignment.Center
                                ) {
                                    Text("No recorded Arena match history yet.", color = OctiTheme.colors.textSecondary)
                                }
                            } else {
                                LazyColumn(
                                    modifier = Modifier.weight(1f),
                                    verticalArrangement = Arrangement.spacedBy(6.dp)
                                ) {
                                    items(p.recentGames) { game ->
                                        Card(
                                            modifier = Modifier.fillMaxWidth(),
                                            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.windowBg),
                                            border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
                                        ) {
                                            Row(
                                                modifier = Modifier
                                                    .fillMaxWidth()
                                                    .padding(10.dp),
                                                horizontalArrangement = Arrangement.SpaceBetween,
                                                verticalAlignment = Alignment.CenterVertically
                                            ) {
                                                val playersLine = game.players.joinToString(" vs ") { pl ->
                                                    "S${pl.seat}:${pl.handle}"
                                                }
                                                Column(modifier = Modifier.weight(1f)) {
                                                    Text(
                                                        text = "${game.variantCode()} • $playersLine",
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
                                                    onClick = { viewModel.openArchivedGameForReplayAsync(game.gameId) },
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
                    } else {
                        if (filteredArenaEntries.isEmpty()) {
                            Box(
                                modifier = Modifier
                                    .fillMaxSize()
                                    .padding(16.dp),
                                contentAlignment = Alignment.Center
                            ) {
                                Text(
                                    text = "No Arena participants found. Robots and human challengers appear here after completing at least 1 Arena match.",
                                    style = MaterialTheme.typography.bodyMedium,
                                    color = OctiTheme.colors.textSecondary
                                )
                            }
                        } else {
                            LazyColumn(
                                modifier = Modifier
                                    .fillMaxSize()
                                    .padding(12.dp),
                                verticalArrangement = Arrangement.spacedBy(6.dp)
                            ) {
                                items(filteredArenaEntries) { r ->
                                    val isHumanEntry = r.entryType.equals("human", ignoreCase = true)
                                    val targetId = if (isHumanEntry) {
                                        r.uuid.ifEmpty { r.handle }
                                    } else {
                                        r.robotUuid.ifEmpty { r.uuid.ifEmpty { r.handle } }
                                    }
                                    Card(
                                        modifier = Modifier
                                            .fillMaxWidth()
                                            .clickable {
                                                viewModel.selectRobotProfileAsync(targetId)
                                            },
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
                                            Column(modifier = Modifier.weight(1f)) {
                                                Row(
                                                    horizontalArrangement = Arrangement.spacedBy(6.dp),
                                                    verticalAlignment = Alignment.CenterVertically
                                                ) {
                                                    Text(
                                                        text = if (isHumanEntry) "@${r.handle} [HUMAN]" else "@${r.handle} [BOT]",
                                                        style = MaterialTheme.typography.titleMedium,
                                                        color = OctiTheme.colors.accentBrass,
                                                        fontWeight = FontWeight.Bold
                                                    )
                                                    if (!isHumanEntry) {
                                                        val isRetired = r.retired || r.revoked || r.status.equals("retired", ignoreCase = true)
                                                        Text(
                                                            text = if (isRetired) "[Retired]" else "[Active]",
                                                            style = MaterialTheme.typography.labelSmall,
                                                            fontWeight = FontWeight.Bold,
                                                            color = if (isRetired) OctiTheme.colors.textSecondary else OctiTheme.colors.accentBrass
                                                        )
                                                    }
                                                }
                                                Text(
                                                    text = "2P_3B: ${r.elo2p3b}  •  2P_1B: ${r.elo2p1b}  •  OFK: ${r.eloOfk}",
                                                    style = MaterialTheme.typography.bodyMedium,
                                                    color = OctiTheme.colors.textPrimary,
                                                    fontWeight = FontWeight.SemiBold
                                                )
                                                val ownerStr = if (!isHumanEntry && r.ownerHandle.isNotEmpty()) "Owner: @${r.ownerHandle} • " else ""
                                                Text(
                                                    text = "${ownerStr}W:${r.wins}  L:${r.losses}  D:${r.draws}  (${r.gamesPlayed} games)",
                                                    style = MaterialTheme.typography.bodySmall,
                                                    color = OctiTheme.colors.textSecondary
                                                )
                                            }
                                            Text(
                                                text = "${"%.1f".format(r.winRate)}% Win",
                                                style = MaterialTheme.typography.titleMedium,
                                                color = OctiTheme.colors.textPrimary,
                                                fontWeight = FontWeight.ExtraBold
                                            )
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
