/*
 * LobbyScreen.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Checkbox
import androidx.compose.material3.FilterChip
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Surface
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
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.selfso.octi.OctiNetworkClient
import com.selfso.octi.ui.theme.OctiTheme

private fun formatCompactDurationSec(sec: Long): String {
    val s = if (sec < 0L) 0L else sec
    return when {
        s < 60L -> "${s}s"
        s < 3600L -> "${s / 60L}m"
        else -> "${s / 3600L}h ${(s % 3600L) / 60L}m"
    }
}

@Composable
fun ClientTypeSmallIcons(
    clients: List<String>,
    accentColor: Color,
    textSec: Color,
    modifier: Modifier = Modifier
) {
    Row(
        modifier = modifier,
        horizontalArrangement = Arrangement.spacedBy(3.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        for (c in clients) {
            when (c) {
                "web" -> {
                    Canvas(modifier = Modifier.size(12.dp)) {
                        val stroke = Stroke(width = 1.2f)
                        val radius = size.minDimension / 2f - 1f
                        val center = Offset(size.width / 2f, size.height / 2f)
                        drawCircle(color = Color(0xFF38BDF8), radius = radius, center = center, style = stroke)
                        drawLine(
                            color = Color(0xFF38BDF8),
                            start = Offset(center.x - radius, center.y),
                            end = Offset(center.x + radius, center.y),
                            strokeWidth = 1f
                        )
                        drawLine(
                            color = Color(0xFF38BDF8),
                            start = Offset(center.x, center.y - radius),
                            end = Offset(center.x, center.y + radius),
                            strokeWidth = 1f
                        )
                    }
                }
                "android" -> {
                    Canvas(modifier = Modifier.size(12.dp)) {
                        val stroke = Stroke(width = 1.2f)
                        val w = size.width - 2f
                        val h = size.height - 2f
                        drawRoundRect(
                            color = Color(0xFF4ADE80),
                            topLeft = Offset(1f, 1f),
                            size = Size(w, h),
                            cornerRadius = androidx.compose.ui.geometry.CornerRadius(2.5f, 2.5f),
                            style = stroke
                        )
                        drawCircle(
                            color = Color(0xFF4ADE80),
                            radius = 1.2f,
                            center = Offset(size.width / 2f, size.height - 3.5f)
                        )
                    }
                }
                "desktop" -> {
                    Canvas(modifier = Modifier.size(12.dp)) {
                        val stroke = Stroke(width = 1.2f)
                        val screenW = size.width - 2f
                        val screenH = size.height * 0.65f
                        drawRoundRect(
                            color = Color(0xFFC8A253),
                            topLeft = Offset(1f, 1f),
                            size = Size(screenW, screenH),
                            cornerRadius = androidx.compose.ui.geometry.CornerRadius(1.5f, 1.5f),
                            style = stroke
                        )
                        val centerX = size.width / 2f
                        drawLine(
                            color = Color(0xFFC8A253),
                            start = Offset(centerX, screenH + 1f),
                            end = Offset(centerX, size.height - 1f),
                            strokeWidth = 1.2f
                        )
                        drawLine(
                            color = Color(0xFFC8A253),
                            start = Offset(centerX - 3f, size.height - 1f),
                            end = Offset(centerX + 3f, size.height - 1f),
                            strokeWidth = 1.2f
                        )
                    }
                }
            }
        }
    }
}

/**
 * Online Lobby, Custom Table Browser, My Active Table status card,
 * and Live Lobby Chat screen.
 *
 * Strictly zero editable connection host or port fields are exposed.
 */
@OptIn(ExperimentalLayoutApi::class)
@Composable
fun LobbyScreen(
    viewModel: OctiAppViewModel,
    modifier: Modifier = Modifier,
    onCreateTableClick: () -> Unit = {}
) {
    val isAuthenticated by viewModel.isAuthenticated.collectAsState()
    val profile by viewModel.currentProfile.collectAsState()
    val lobbyState by viewModel.lobbyState.collectAsState()
    val lobbyChatMessages by viewModel.lobbyChatMessages.collectAsState()
    val activeRoomId by viewModel.activeRoomId.collectAsState()
    val activeGameView by viewModel.activeGameView.collectAsState()

    var selectedFilterMode by remember { mutableStateOf("All") }
    var chatDraft by remember { mutableStateOf("") }
    var showInlineCreateTableDialog by remember { mutableStateOf(false) }
    var joinPasscodeTargetTable by remember { mutableStateOf<OctiNetworkClient.TableSummaryInfo?>(null) }
    var joinPasscodeInput by remember { mutableStateOf("") }
    var expandedPlayerUuid by remember { mutableStateOf<String?>(null) }
    var purgeConfirmTableId by remember { mutableStateOf<String?>(null) }

    LaunchedEffect(Unit) {
        while (true) {
            viewModel.refreshLobbyStateAsync()
            kotlinx.coroutines.delay(5000L)
        }
    }

    val myUuid = profile?.uuid ?: ""
    val myHandle = profile?.handle ?: ""

    fun isUserSeatedInTable(t: OctiNetworkClient.TableSummaryInfo): Boolean {
        if (myUuid.isEmpty() && myHandle.isEmpty()) {
            return false
        }
        if (myUuid.isNotEmpty() && t.hostUuid == myUuid) {
            return true
        }
        if (myHandle.isNotEmpty() && t.hostHandle.equals(myHandle, ignoreCase = true)) {
            return true
        }
        for (s in t.seats) {
            if (myUuid.isNotEmpty() && s.playerUuid == myUuid) {
                return true
            }
            if (myHandle.isNotEmpty() && s.handle.equals(myHandle, ignoreCase = true)) {
                return true
            }
        }
        for (p in t.players) {
            if (myHandle.isNotEmpty() && p.equals(myHandle, ignoreCase = true)) {
                return true
            }
        }
        return false
    }

    fun seatLetterForIndex(seatNumber: Int): String {
        return when (seatNumber) {
            1 -> "A"
            2 -> "B"
            3 -> "C"
            4 -> "D"
            else -> seatNumber.toString()
        }
    }

    val seatedTableInLobby = lobbyState.tables.firstOrNull { t ->
        isUserSeatedInTable(t) || (activeRoomId.isNotEmpty() && t.roomId == activeRoomId)
    }

    val filteredTables = remember(lobbyState.tables, selectedFilterMode) {
        if (selectedFilterMode == "All") {
            lobbyState.tables
        } else {
            lobbyState.tables.filter { it.mode.equals(selectedFilterMode, ignoreCase = true) }
        }
    }

    purgeConfirmTableId?.let { roomId ->
        AlertDialog(
            onDismissRequest = { purgeConfirmTableId = null },
            containerColor = OctiTheme.colors.panelCardBg,
            titleContentColor = OctiTheme.colors.accentBrass,
            title = { Text("Purge Table") },
            text = { Text("Are you sure you want to permanently purge table #$roomId? All players will be disconnected and the table removed immediately.") },
            confirmButton = {
                Button(
                    onClick = {
                        viewModel.purgeTable(roomId)
                        purgeConfirmTableId = null
                    },
                    colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.error)
                ) {
                    Text("Purge")
                }
            },
            dismissButton = {
                OutlinedButton(onClick = { purgeConfirmTableId = null }) {
                    Text("Cancel")
                }
            }
        )
    }

    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(12.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp)
    ) {
        // Header Row
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
                    Column(modifier = Modifier.weight(1f)) {
                        Text(
                            text = "OCTI Online Arena",
                            style = MaterialTheme.typography.titleLarge,
                            color = OctiTheme.colors.accentBrass,
                            maxLines = 1,
                            overflow = TextOverflow.Ellipsis
                        )
                    }
                    Row(
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        if (!isAuthenticated) {
                            Button(
                                onClick = { viewModel.setShowSsoDialog(true) },
                                colors = ButtonDefaults.buttonColors(
                                    containerColor = OctiTheme.colors.accentBrass,
                                    contentColor = OctiTheme.colors.windowBg
                                )
                            ) {
                                Text("Sign In (SSO)", maxLines = 1, softWrap = false)
                            }
                        } else {
                            OutlinedButton(onClick = { viewModel.setShowAccountDialog(true) }) {
                                val prof = profile
                                if (prof != null) {
                                    OctiPlayerAvatar(
                                        avatarUrl = prof.avatarUrl,
                                        label = prof.handle,
                                        netClient = viewModel.networkClient(),
                                        size = 20.dp,
                                        modifier = Modifier.padding(end = 6.dp)
                                    )
                                    val handleText = if (prof.handle.isNotEmpty()) "@${prof.handle}" else "Account"
                                    Text(
                                        text = handleText,
                                        maxLines = 1,
                                        overflow = TextOverflow.Ellipsis
                                    )
                                } else {
                                    Text("Account", maxLines = 1, overflow = TextOverflow.Ellipsis)
                                }
                            }
                        }
                        OutlinedButton(
                            onClick = { viewModel.refreshLobbyStateAsync() },
                            contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                        ) {
                            Text("↻", maxLines = 1, softWrap = false)
                        }
                    }
                }
            }
        }

        // 1. My Active Table Card
        val showMyActiveTable = seatedTableInLobby != null || activeRoomId.isNotEmpty()
        if (showMyActiveTable) {
            val tableRoomId = seatedTableInLobby?.roomId ?: activeRoomId
            val tablePhase = seatedTableInLobby?.phase ?: activeGameView?.phase ?: "WAITING_FOR_PLAYERS"
            val tableMode = seatedTableInLobby?.mode ?: activeGameView?.mode ?: "2P"
            val tableBases = seatedTableInLobby?.bases ?: activeGameView?.bases ?: 3
            val tableTimeControl = seatedTableInLobby?.timeControl?.ifEmpty { "10+5" }
                ?: activeGameView?.timeControl?.ifEmpty { "10+5" } ?: "10+5"
            val phaseLabel = if (tablePhase == "WAITING_FOR_PLAYERS") {
                "Waiting for Players"
            } else {
                "Match in Progress"
            }

            Card(
                modifier = Modifier.fillMaxWidth(),
                colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
                border = BorderStroke(1.5.dp, OctiTheme.colors.accentBrass)
            ) {
                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(12.dp),
                    verticalArrangement = Arrangement.spacedBy(6.dp)
                ) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text(
                            text = "My Active Table • Table #$tableRoomId",
                            style = MaterialTheme.typography.titleMedium,
                            fontWeight = FontWeight.Bold,
                            color = OctiTheme.colors.accentBrass
                        )
                        Text(
                            text = phaseLabel,
                            style = MaterialTheme.typography.labelMedium,
                            fontWeight = FontWeight.SemiBold,
                            color = OctiTheme.colors.windowBg,
                            modifier = Modifier
                                .background(
                                    color = OctiTheme.colors.accentBrass,
                                    shape = RoundedCornerShape(6.dp)
                                )
                                .padding(horizontal = 8.dp, vertical = 3.dp)
                        )
                    }

                    Text(
                        text = "$tableMode • $tableBases-Base • $tableTimeControl",
                        style = MaterialTheme.typography.bodySmall,
                        color = OctiTheme.colors.textSecondary
                    )

                    FlowRow(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                        verticalArrangement = Arrangement.spacedBy(4.dp)
                    ) {
                        val seats = seatedTableInLobby?.seats ?: emptyList()
                        val seatsTotal = seatedTableInLobby?.seatsTotal
                            ?: if (tableMode.equals("4P", ignoreCase = true)) 4 else 2
                        if (seats.isNotEmpty()) {
                            for (seatIdx in 1..seatsTotal) {
                                val s = seats.firstOrNull { it.seat == seatIdx }
                                val seatLabel = if (s != null && s.handle.isNotEmpty()) {
                                    "Seat $seatIdx (${seatLetterForIndex(seatIdx)}): @${s.handle}"
                                } else {
                                    "Seat $seatIdx (${seatLetterForIndex(seatIdx)}): Waiting..."
                                }
                                Text(
                                    text = seatLabel,
                                    style = MaterialTheme.typography.bodySmall,
                                    color = OctiTheme.colors.textPrimary,
                                    modifier = Modifier
                                        .background(
                                            color = OctiTheme.colors.windowBg,
                                            shape = RoundedCornerShape(6.dp)
                                        )
                                        .padding(horizontal = 8.dp, vertical = 4.dp)
                                )
                            }
                        } else {
                            val players = seatedTableInLobby?.players ?: activeGameView?.players ?: emptyList()
                            for (seatIdx in 1..seatsTotal) {
                                val handle = players.getOrNull(seatIdx - 1) ?: ""
                                val seatLabel = if (handle.isNotEmpty()) {
                                    "Seat $seatIdx (${seatLetterForIndex(seatIdx)}): @$handle"
                                } else {
                                    "Seat $seatIdx (${seatLetterForIndex(seatIdx)}): Waiting..."
                                }
                                Text(
                                    text = seatLabel,
                                    style = MaterialTheme.typography.bodySmall,
                                    color = OctiTheme.colors.textPrimary,
                                    modifier = Modifier
                                        .background(
                                            color = OctiTheme.colors.windowBg,
                                            shape = RoundedCornerShape(6.dp)
                                        )
                                        .padding(horizontal = 8.dp, vertical = 4.dp)
                                )
                            }
                        }
                    }

                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(8.dp, Alignment.End),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Button(
                            onClick = { viewModel.selectTab(OctiAppViewModel.AppTab.BOARD) },
                            colors = ButtonDefaults.buttonColors(
                                containerColor = OctiTheme.colors.accentBrass,
                                contentColor = OctiTheme.colors.windowBg
                            )
                        ) {
                            Text("Open Board")
                        }
                        OutlinedButton(
                            onClick = { viewModel.leaveOnlineTableAsync(tableRoomId) }
                        ) {
                            Text("Leave Table")
                        }
                    }
                }
            }
        }

        // 2. Open & Live Tables Card
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
                        text = "Open & Live Tables (${filteredTables.size})",
                        style = MaterialTheme.typography.titleMedium,
                        color = OctiTheme.colors.textPrimary,
                        fontWeight = FontWeight.Bold
                    )
                    Button(
                        onClick = {
                            onCreateTableClick()
                            showInlineCreateTableDialog = true
                        },
                        enabled = isAuthenticated,
                        colors = ButtonDefaults.buttonColors(
                            containerColor = OctiTheme.colors.accentBrass,
                            contentColor = OctiTheme.colors.windowBg
                        )
                    ) {
                        Text("+ Create Table", maxLines = 1, softWrap = false)
                    }
                }

                Spacer(modifier = Modifier.height(6.dp))

                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(6.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    listOf("All", "2P", "4P", "OFK").forEach { modeChip ->
                        FilterChip(
                            selected = selectedFilterMode == modeChip,
                            onClick = { selectedFilterMode = modeChip },
                            label = { Text(modeChip) }
                        )
                    }
                }

                Spacer(modifier = Modifier.height(8.dp))

                if (filteredTables.isEmpty()) {
                    Box(
                        modifier = Modifier
                            .fillMaxSize()
                            .padding(16.dp),
                        contentAlignment = Alignment.Center
                    ) {
                        Text(
                            text = "No tables open right now. Tap '+ Create Table' to open a new table.",
                            style = MaterialTheme.typography.bodyMedium,
                            color = OctiTheme.colors.textSecondary,
                            textAlign = TextAlign.Center
                        )
                    }
                } else {
                    LazyColumn(
                        modifier = Modifier.fillMaxSize(),
                        verticalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        items(filteredTables) { t ->
                            val isSeatedHere = isUserSeatedInTable(t) ||
                                (activeRoomId.isNotEmpty() && t.roomId == activeRoomId)
                            val isOpenPhase = t.phase == "WAITING_FOR_PLAYERS"
                            val ttlStr = if (t.ttlRemainingMs >= 0) {
                                val totalSec = t.ttlRemainingMs / 1000
                                val mm = totalSec / 60
                                val ss = totalSec % 60
                                " [TTL %02d:%02d]".format(mm, ss)
                            } else ""
                            val badgeText = (if (isOpenPhase) "OPEN" else "IN PROGRESS") + ttlStr
                            val badgeColor = if (isOpenPhase) Color(0xFF2E7D32) else Color(0xFFF57C00)
                            val tcText = t.timeControl.ifEmpty { "10+5" }

                            Card(
                                modifier = Modifier.fillMaxWidth(),
                                colors = CardDefaults.cardColors(
                                    containerColor = OctiTheme.colors.windowBg
                                ),
                                border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
                            ) {
                                Column(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(10.dp),
                                    verticalArrangement = Arrangement.spacedBy(6.dp)
                                ) {
                                    // Top Row: Table #id • Host • Lock • Phase badge
                                    Row(
                                        modifier = Modifier.fillMaxWidth(),
                                        horizontalArrangement = Arrangement.SpaceBetween,
                                        verticalAlignment = Alignment.CenterVertically
                                    ) {
                                        Row(
                                            horizontalArrangement = Arrangement.spacedBy(6.dp),
                                            verticalAlignment = Alignment.CenterVertically,
                                            modifier = Modifier.weight(1f)
                                        ) {
                                            Text(
                                                text = "Table #${t.roomId} • Host: @${t.hostHandle}",
                                                style = MaterialTheme.typography.titleMedium,
                                                color = OctiTheme.colors.accentBrass,
                                                maxLines = 1,
                                                overflow = TextOverflow.Ellipsis
                                            )
                                            if (t.hasPasscode) {
                                                Text(
                                                    text = "🔒 Passcode",
                                                    style = MaterialTheme.typography.labelSmall,
                                                    color = OctiTheme.colors.textSecondary
                                                )
                                            }
                                        }
                                        Text(
                                            text = badgeText,
                                            style = MaterialTheme.typography.labelSmall,
                                            fontWeight = FontWeight.Bold,
                                            color = Color.White,
                                            modifier = Modifier
                                                .background(
                                                    color = badgeColor,
                                                    shape = RoundedCornerShape(4.dp)
                                                )
                                                .padding(horizontal = 6.dp, vertical = 2.dp)
                                        )
                                    }

                                    // Ruleset Row
                                    Text(
                                        text = "${t.mode} • ${t.bases}-Base • $tcText" +
                                            (if (t.rated) " • Rated" else " • Casual") +
                                            (if (t.superProngs) " • SuperProngs" else "") +
                                            (if (t.edgeless) " • Edgeless" else ""),
                                        style = MaterialTheme.typography.bodySmall,
                                        color = OctiTheme.colors.textSecondary
                                    )

                                    // Seats Row
                                    FlowRow(
                                        modifier = Modifier.fillMaxWidth(),
                                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                                        verticalArrangement = Arrangement.spacedBy(4.dp)
                                    ) {
                                        val totalSeats = if (t.seatsTotal > 0) t.seatsTotal else 2
                                        for (seatIdx in 1..totalSeats) {
                                            val seatEntry = t.seats.firstOrNull { it.seat == seatIdx }
                                            val fallbackHandle = t.players.getOrNull(seatIdx - 1) ?: ""
                                            val seatText = when {
                                                seatEntry != null && seatEntry.handle.isNotEmpty() ->
                                                    "Seat $seatIdx (${seatLetterForIndex(seatIdx)}): @${seatEntry.handle} (${seatEntry.rating})"
                                                fallbackHandle.isNotEmpty() ->
                                                    "Seat $seatIdx (${seatLetterForIndex(seatIdx)}): @$fallbackHandle"
                                                else ->
                                                    "Seat $seatIdx (${seatLetterForIndex(seatIdx)}): [Open Seat]"
                                            }
                                            Text(
                                                text = seatText,
                                                style = MaterialTheme.typography.bodySmall,
                                                color = OctiTheme.colors.textPrimary
                                            )
                                        }
                                    }

                                    // Action Row
                                    Row(
                                        modifier = Modifier.fillMaxWidth(),
                                        horizontalArrangement = Arrangement.spacedBy(8.dp, Alignment.End),
                                        verticalAlignment = Alignment.CenterVertically
                                    ) {
                                        if (isSeatedHere && t.phase == "WAITING_FOR_PLAYERS") {
                                            OutlinedButton(
                                                onClick = { viewModel.leaveOnlineTableAsync(t.roomId) }
                                            ) {
                                                Text("Leave Table")
                                            }
                                            Button(
                                                onClick = { viewModel.selectTab(OctiAppViewModel.AppTab.BOARD) },
                                                colors = ButtonDefaults.buttonColors(
                                                    containerColor = OctiTheme.colors.accentBrass,
                                                    contentColor = OctiTheme.colors.windowBg
                                                )
                                            ) {
                                                Text("Board")
                                            }
                                        } else if (t.phase == "WAITING_FOR_PLAYERS" && t.seatsFilled < t.seatsTotal && !isSeatedHere) {
                                            Button(
                                                onClick = {
                                                    if (t.hasPasscode) {
                                                        joinPasscodeTargetTable = t
                                                        joinPasscodeInput = ""
                                                    } else {
                                                        viewModel.joinOnlineTableAsync(t.roomId, "")
                                                    }
                                                },
                                                enabled = isAuthenticated,
                                                colors = ButtonDefaults.buttonColors(
                                                    containerColor = OctiTheme.colors.accentBrass,
                                                    contentColor = OctiTheme.colors.windowBg
                                                )
                                            ) {
                                                Text("Join Seat")
                                            }
                                        }
                                        OutlinedButton(
                                            onClick = { viewModel.observeOnlineTableAsync(t.roomId) },
                                            enabled = isAuthenticated || !t.hasPasscode
                                        ) {
                                            Text("Observe")
                                        }
                                        if (profile?.isAdmin == true) {
                                            Button(
                                                onClick = { purgeConfirmTableId = t.roomId },
                                                colors = ButtonDefaults.buttonColors(
                                                    containerColor = MaterialTheme.colorScheme.error,
                                                    contentColor = Color.White
                                                )
                                            ) {
                                                Text("Purge")
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

        // 2b. Connected Users Card (directly below Open & Live Tables Card)
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
            border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
        ) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 10.dp, vertical = 8.dp),
                verticalArrangement = Arrangement.spacedBy(6.dp)
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = "Connected Users (${lobbyState.players.size})",
                        style = MaterialTheme.typography.titleSmall,
                        fontWeight = FontWeight.Bold,
                        color = OctiTheme.colors.accentBrass
                    )
                    Text(
                        text = "Tap user to show @handle",
                        style = MaterialTheme.typography.labelSmall,
                        color = OctiTheme.colors.textSecondary
                    )
                }

                if (lobbyState.players.isEmpty()) {
                    Text(
                        text = "No users connected right now.",
                        style = MaterialTheme.typography.bodySmall,
                        color = OctiTheme.colors.textSecondary,
                        modifier = Modifier.padding(vertical = 4.dp)
                    )
                } else {
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .horizontalScroll(rememberScrollState()),
                        horizontalArrangement = Arrangement.spacedBy(6.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        for (p in lobbyState.players) {
                            if (p.handle == "anonymous-127.0.0.1" || p.handle == "anonymous-::1") {
                                continue
                            }
                            val playerKey = p.playerUuid.ifEmpty { p.handle }
                            val isExpanded = expandedPlayerUuid == playerKey
                            val cleanHandle = p.handle.removePrefix("@")
                            Surface(
                                onClick = {
                                    expandedPlayerUuid = if (isExpanded) null else playerKey
                                },
                                shape = RoundedCornerShape(16.dp),
                                color = OctiTheme.colors.windowBg,
                                border = BorderStroke(
                                    1.dp,
                                    if (isExpanded) OctiTheme.colors.accentBrass else OctiTheme.colors.panelCardBorder
                                )
                            ) {
                                Row(
                                    modifier = Modifier.padding(horizontal = 8.dp, vertical = 4.dp),
                                    horizontalArrangement = Arrangement.spacedBy(5.dp),
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    OctiPlayerAvatar(
                                        avatarUrl = p.avatarUrl,
                                        handle = p.handle,
                                        size = 22.dp,
                                        netClient = viewModel.networkClient()
                                    )

                                    if (isExpanded) {
                                        Text(
                                            text = "@$cleanHandle",
                                            style = MaterialTheme.typography.bodySmall,
                                            fontWeight = FontWeight.Bold,
                                            fontSize = 11.sp,
                                            color = OctiTheme.colors.accentBrass
                                        )
                                        if (p.roomId.isNotBlank()) {
                                            Text(
                                                text = "#${p.roomId}",
                                                style = MaterialTheme.typography.labelSmall,
                                                fontSize = 10.sp,
                                                color = OctiTheme.colors.accentBrass,
                                                modifier = Modifier
                                                    .background(
                                                        color = OctiTheme.colors.accentBrass.copy(alpha = 0.2f),
                                                        shape = RoundedCornerShape(8.dp)
                                                    )
                                                    .padding(horizontal = 4.dp, vertical = 1.dp)
                                            )
                                        }
                                    }

                                    if (p.clients.isNotEmpty()) {
                                        ClientTypeSmallIcons(
                                            clients = p.clients,
                                            accentColor = OctiTheme.colors.accentBrass,
                                            textSec = OctiTheme.colors.textSecondary
                                        )
                                    }

                                    val onlineText = "On ${formatCompactDurationSec(p.onlineSec)}"
                                    val idleText = if (p.idleSec < 10L) "Active" else "Idle ${formatCompactDurationSec(p.idleSec)}"
                                    Text(
                                        text = "$onlineText • $idleText",
                                        style = MaterialTheme.typography.labelSmall,
                                        fontSize = 10.sp,
                                        color = OctiTheme.colors.textSecondary
                                    )
                                }
                            }
                        }
                    }
                }
            }
        }

        // 3. Titled Lobby Chat Card
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .height(185.dp),
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
            border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(10.dp),
                verticalArrangement = Arrangement.spacedBy(6.dp)
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = "Lobby Chat",
                        style = MaterialTheme.typography.titleSmall,
                        fontWeight = FontWeight.Bold,
                        color = OctiTheme.colors.accentBrass
                    )
                    Text(
                        text = "6h history • Local time",
                        style = MaterialTheme.typography.labelSmall,
                        color = OctiTheme.colors.textSecondary
                    )
                }

                LazyColumn(
                    modifier = Modifier
                        .fillMaxWidth()
                        .weight(1f)
                ) {
                    val formattedList = if (lobbyChatMessages.isNotEmpty()) {
                        lobbyChatMessages
                    } else {
                        lobbyState.recentChat.map { OctiAppViewModel.formatLobbyChatEntry(it) }
                    }
                    items(formattedList) { line ->
                        Text(
                            text = line,
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
                        placeholder = {
                            Text(if (isAuthenticated) "Lobby chat message..." else "Sign in to chat")
                        },
                        enabled = isAuthenticated,
                        singleLine = true,
                        modifier = Modifier.weight(1f)
                    )
                    Button(
                        onClick = {
                            if (chatDraft.isNotBlank()) {
                                viewModel.sendLobbyChatAsync(chatDraft.trim())
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

    // Passcode Modal Dialog when joining a locked table
    val targetTable = joinPasscodeTargetTable
    if (targetTable != null) {
        AlertDialog(
            onDismissRequest = { joinPasscodeTargetTable = null },
            containerColor = OctiTheme.colors.panelCardBg,
            titleContentColor = OctiTheme.colors.accentBrass,
            textContentColor = OctiTheme.colors.textPrimary,
            title = { Text("Enter Table Passcode") },
            text = {
                Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    Text("Table #${targetTable.roomId} hosted by @${targetTable.hostHandle} requires a passcode.")
                    OutlinedTextField(
                        value = joinPasscodeInput,
                        onValueChange = { joinPasscodeInput = it },
                        label = { Text("Passcode") },
                        singleLine = true,
                        modifier = Modifier.fillMaxWidth()
                    )
                }
            },
            confirmButton = {
                Button(
                    onClick = {
                        val roomId = targetTable.roomId
                        val code = joinPasscodeInput.trim()
                        joinPasscodeTargetTable = null
                        viewModel.joinOnlineTableAsync(roomId, code)
                    },
                    colors = ButtonDefaults.buttonColors(
                        containerColor = OctiTheme.colors.accentBrass,
                        contentColor = OctiTheme.colors.windowBg
                    )
                ) {
                    Text("Join Seat")
                }
            },
            dismissButton = {
                OutlinedButton(onClick = { joinPasscodeTargetTable = null }) {
                    Text("Cancel")
                }
            }
        )
    }

    // Inline Create Online Table Dialog
    if (showInlineCreateTableDialog) {
        var createMode by remember { mutableStateOf("2P") }
        var createBases by remember { mutableStateOf(3) }
        var createSuperProngs by remember { mutableStateOf(false) }
        var createEdgeless by remember { mutableStateOf(false) }
        var createTimeControl by remember { mutableStateOf("10+5") }
        var createRated by remember { mutableStateOf(true) }
        var createAllowTakeback by remember { mutableStateOf(false) }
        var createPasscode by remember { mutableStateOf("") }
        val profileActionBusy by viewModel.profileActionBusy.collectAsState()

        AlertDialog(
            onDismissRequest = { showInlineCreateTableDialog = false },
            containerColor = OctiTheme.colors.panelCardBg,
            titleContentColor = OctiTheme.colors.accentBrass,
            textContentColor = OctiTheme.colors.textPrimary,
            title = { Text("Create Online Table") },
            text = {
                Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    Text("Mode", fontWeight = FontWeight.SemiBold, color = OctiTheme.colors.accentBrass)
                    FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                        listOf("2P", "4P", "OFK").forEach { m ->
                            FilterChip(
                                selected = createMode == m,
                                onClick = {
                                    createMode = m
                                    createBases = if (m == "4P" || m == "OFK") 1 else 3
                                    if (m == "OFK") {
                                        createSuperProngs = false
                                        createEdgeless = false
                                    }
                                },
                                label = { Text(m) }
                            )
                        }
                    }
                    if (createMode != "OFK") {
                        Text("Bases", fontWeight = FontWeight.SemiBold, color = OctiTheme.colors.accentBrass)
                        FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                            listOf(3 to "3-Base", 1 to "1-Base").forEach { (b, lbl) ->
                                FilterChip(
                                    selected = createBases == b,
                                    onClick = { createBases = b },
                                    label = { Text(lbl) }
                                )
                            }
                        }
                    }
                    Text("Time Control", fontWeight = FontWeight.SemiBold, color = OctiTheme.colors.accentBrass)
                    FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                        listOf("Untimed", "5+0", "10+5", "15+10").forEach { tc ->
                            FilterChip(
                                selected = createTimeControl == tc,
                                onClick = { createTimeControl = tc },
                                label = { Text(tc) }
                            )
                        }
                    }
                    if (createMode != "OFK") {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Checkbox(checked = createSuperProngs, onCheckedChange = { createSuperProngs = it })
                            Text("Super-Prongs")
                        }
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Checkbox(checked = createEdgeless, onCheckedChange = { createEdgeless = it })
                            Text("Edgeless")
                        }
                    }
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Checkbox(checked = createRated, onCheckedChange = { createRated = it })
                        Text(if (createRated) "Rated" else "Casual")
                    }
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Checkbox(checked = createAllowTakeback, onCheckedChange = { createAllowTakeback = it })
                        Text("Allow Mutual Takebacks")
                    }
                    OutlinedTextField(
                        value = createPasscode,
                        onValueChange = { createPasscode = it },
                        label = { Text("Optional Passcode") },
                        singleLine = true,
                        modifier = Modifier.fillMaxWidth()
                    )
                }
            },
            confirmButton = {
                Button(
                    onClick = {
                        val req = OctiNetworkClient.CreateTableRequest().apply {
                            this.mode = createMode
                            this.bases = if (createMode == "OFK") 1 else createBases
                            this.superProngs = if (createMode == "OFK") false else createSuperProngs
                            this.edgeless = if (createMode == "OFK") false else createEdgeless
                            this.timeControl = createTimeControl
                            this.rated = createRated
                            this.allowTakeback = createAllowTakeback
                            this.passcode = createPasscode.trim()
                        }
                        viewModel.createOnlineTableAsync(req) { res ->
                            if (res.ok) {
                                showInlineCreateTableDialog = false
                            }
                        }
                    },
                    enabled = !profileActionBusy,
                    colors = ButtonDefaults.buttonColors(
                        containerColor = OctiTheme.colors.accentBrass,
                        contentColor = OctiTheme.colors.windowBg
                    )
                ) {
                    Text(if (profileActionBusy) "Creating..." else "Create Table")
                }
            },
            dismissButton = {
                OutlinedButton(onClick = { showInlineCreateTableDialog = false }) {
                    Text("Cancel")
                }
            }
        )
    }
}
