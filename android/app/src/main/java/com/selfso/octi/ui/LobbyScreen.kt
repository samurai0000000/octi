/*
 * LobbyScreen.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Checkbox
import androidx.compose.material3.FilterChip
import androidx.compose.material3.IconButton
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
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import com.selfso.octi.OctiBotEngine
import com.selfso.octi.OctiGameMode
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
        val titleText = "Delete Table"
        val bodyText = "Are you sure you want to delete table #$roomId? All players will be disconnected and the table removed."
        val btnText = "Delete"
        AlertDialog(
            onDismissRequest = { purgeConfirmTableId = null },
            containerColor = OctiTheme.colors.panelCardBg,
            titleContentColor = OctiTheme.colors.accentBrass,
            title = { Text(titleText) },
            text = { Text(bodyText) },
            confirmButton = {
                Button(
                    onClick = {
                        viewModel.purgeOnlineTableAsync(roomId)
                        purgeConfirmTableId = null
                    },
                    colors = ButtonDefaults.buttonColors(containerColor = MaterialTheme.colorScheme.error)
                ) {
                    Text(btnText)
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
        // 1. Open & Live Tables Card (weight 1f)
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
                // Header row
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
                    Row(
                        horizontalArrangement = Arrangement.spacedBy(6.dp),
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        OutlinedButton(
                            onClick = { viewModel.refreshLobbyStateAsync() },
                            modifier = Modifier.height(32.dp),
                            contentPadding = PaddingValues(horizontal = 8.dp, vertical = 0.dp)
                        ) {
                            Text("↻", fontWeight = FontWeight.Bold)
                        }
                        Button(
                            onClick = {
                                onCreateTableClick()
                                showInlineCreateTableDialog = true
                            },
                            enabled = isAuthenticated,
                            modifier = Modifier.height(32.dp),
                            contentPadding = PaddingValues(horizontal = 10.dp, vertical = 0.dp),
                            colors = ButtonDefaults.buttonColors(
                                containerColor = OctiTheme.colors.accentBrass,
                                contentColor = OctiTheme.colors.windowBg
                            )
                        ) {
                            Text("+ Create Table", maxLines = 1, softWrap = false, style = MaterialTheme.typography.labelSmall, fontWeight = FontWeight.Bold)
                        }
                    }
                }

                // Compact active-table banner if seated or activeRoomId is not empty
                val showMyActiveTable = seatedTableInLobby != null || activeRoomId.isNotEmpty()
                if (showMyActiveTable) {
                    val tableRoomId = seatedTableInLobby?.roomId ?: activeRoomId
                    val shortTableRoomId = if (tableRoomId.length > 16) tableRoomId.take(16) else tableRoomId
                    val tablePhase = seatedTableInLobby?.phase ?: activeGameView?.phase ?: "WAITING_FOR_PLAYERS"
                    val tableMode = seatedTableInLobby?.mode ?: activeGameView?.mode ?: "2P"
                    val phaseLabel = if (tablePhase == "WAITING_FOR_PLAYERS") "Waiting" else "In Progress"

                    Spacer(modifier = Modifier.height(6.dp))
                    Surface(
                        modifier = Modifier.fillMaxWidth(),
                        shape = RoundedCornerShape(8.dp),
                        color = OctiTheme.colors.windowBg,
                        border = BorderStroke(1.5.dp, OctiTheme.colors.accentBrass)
                    ) {
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(horizontal = 8.dp, vertical = 6.dp),
                            horizontalArrangement = Arrangement.SpaceBetween,
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Column(modifier = Modifier.weight(1f)) {
                                Text(
                                    text = "Active Table #$shortTableRoomId ($tableMode • $phaseLabel)",
                                    style = MaterialTheme.typography.bodySmall,
                                    fontWeight = FontWeight.Bold,
                                    color = OctiTheme.colors.accentBrass,
                                    maxLines = 1,
                                    overflow = TextOverflow.Ellipsis
                                )
                            }
                            Row(
                                horizontalArrangement = Arrangement.spacedBy(6.dp),
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                Button(
                                    onClick = { viewModel.selectTab(OctiAppViewModel.AppTab.BOARD) },
                                    modifier = Modifier.height(28.dp),
                                    contentPadding = PaddingValues(horizontal = 8.dp, vertical = 0.dp),
                                    colors = ButtonDefaults.buttonColors(
                                        containerColor = OctiTheme.colors.accentBrass,
                                        contentColor = OctiTheme.colors.windowBg
                                    )
                                ) {
                                    Text("Open Board", style = MaterialTheme.typography.labelSmall, fontWeight = FontWeight.Bold)
                                }
                                OutlinedButton(
                                    onClick = { viewModel.leaveOnlineTableAsync(tableRoomId) },
                                    modifier = Modifier.height(28.dp),
                                    contentPadding = PaddingValues(horizontal = 8.dp, vertical = 0.dp)
                                ) {
                                    Text("Leave", style = MaterialTheme.typography.labelSmall, fontWeight = FontWeight.SemiBold)
                                }
                            }
                        }
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
                                            modifier = Modifier
                                                .weight(1f)
                                                .padding(end = 6.dp)
                                        ) {
                                            val shortId = if (t.roomId.length > 16) t.roomId.take(16) else t.roomId
                                            Text(
                                                text = "Table #$shortId • @${t.hostHandle}",
                                                style = MaterialTheme.typography.titleSmall,
                                                fontWeight = FontWeight.Bold,
                                                color = OctiTheme.colors.accentBrass,
                                                maxLines = 1,
                                                overflow = TextOverflow.Ellipsis,
                                                modifier = Modifier.weight(1f, fill = false)
                                            )
                                            if (t.hasPasscode) {
                                                Text(
                                                    text = "🔒",
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
                                        Button(
                                            onClick = {
                                                if (t.hasPasscode) {
                                                    joinPasscodeTargetTable = t
                                                    joinPasscodeInput = ""
                                                } else {
                                                    viewModel.observeOnlineTableAsync(t.roomId)
                                                }
                                            },
                                            colors = ButtonDefaults.buttonColors(
                                                containerColor = OctiTheme.colors.accentBrass,
                                                contentColor = OctiTheme.colors.windowBg
                                            )
                                        ) {
                                            Text("Join")
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 2. Lobby Chat Card (weight 1f)
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
                    .padding(10.dp),
                verticalArrangement = Arrangement.spacedBy(6.dp)
            ) {
                // Header row
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .clickable { viewModel.openLobbyChatZoomDialog() },
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = "Lobby Chat",
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold,
                        color = OctiTheme.colors.accentBrass
                    )
                    Text(
                        text = "Global • Tap to zoom ⤢",
                        style = MaterialTheme.typography.labelSmall,
                        color = OctiTheme.colors.accentBrass,
                        fontWeight = FontWeight.SemiBold
                    )
                }

                // Chat Messages Log
                val lobbyChatMessages by viewModel.lobbyChatMessages.collectAsState()
                val formattedChatList = if (lobbyChatMessages.isNotEmpty()) {
                    lobbyChatMessages
                } else {
                    lobbyState.recentChat.map { OctiAppViewModel.formatLobbyChatEntry(it) }
                }
                val lobbyChatListState = rememberLazyListState()
                LaunchedEffect(formattedChatList.size) {
                    if (formattedChatList.isNotEmpty()) {
                        lobbyChatListState.scrollToItem(formattedChatList.size - 1)
                    }
                }

                Surface(
                    modifier = Modifier
                        .fillMaxWidth()
                        .weight(1f)
                        .clickable { viewModel.openLobbyChatZoomDialog() },
                    color = OctiTheme.colors.windowBg,
                    shape = RoundedCornerShape(6.dp),
                    border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
                ) {
                    if (formattedChatList.isEmpty()) {
                        Box(
                            modifier = Modifier
                                .fillMaxSize()
                                .padding(8.dp),
                            contentAlignment = Alignment.Center
                        ) {
                            Text(
                                text = "No messages in lobby chat yet. Say hello!",
                                style = MaterialTheme.typography.bodySmall,
                                color = OctiTheme.colors.textSecondary
                            )
                        }
                    } else {
                        LazyColumn(
                            state = lobbyChatListState,
                            modifier = Modifier
                                .fillMaxSize()
                                .padding(6.dp),
                            verticalArrangement = Arrangement.spacedBy(3.dp)
                        ) {
                            items(formattedChatList) { line ->
                                Text(
                                    text = line,
                                    style = MaterialTheme.typography.bodySmall,
                                    color = OctiTheme.colors.textPrimary
                                )
                            }
                        }
                    }
                }

                // Input Row
                var chatInputText by remember { mutableStateOf("") }
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(6.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    OutlinedTextField(
                        value = chatInputText,
                        onValueChange = { chatInputText = it },
                        placeholder = {
                            Text(
                                if (isAuthenticated) "Message lobby..." else "Sign in to chat...",
                                style = MaterialTheme.typography.bodySmall
                            )
                        },
                        enabled = isAuthenticated,
                        singleLine = true,
                        modifier = Modifier.weight(1f)
                    )
                    Button(
                        onClick = {
                            if (chatInputText.isNotBlank()) {
                                viewModel.sendLobbyChatMessage(chatInputText.trim())
                                chatInputText = ""
                            }
                        },
                        enabled = isAuthenticated && chatInputText.isNotBlank(),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = OctiTheme.colors.accentBrass,
                            contentColor = OctiTheme.colors.windowBg
                        )
                    ) {
                        Text("Send", fontWeight = FontWeight.Bold)
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
                        viewModel.observeOnlineTableAsync(roomId, code)
                    },
                    colors = ButtonDefaults.buttonColors(
                        containerColor = OctiTheme.colors.accentBrass,
                        contentColor = OctiTheme.colors.windowBg
                    )
                ) {
                    Text("Join")
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
                Column(
                    modifier = Modifier.verticalScroll(rememberScrollState()),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
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

@Composable
fun LobbyChatScreen(
    viewModel: OctiAppViewModel,
    modifier: Modifier = Modifier
) {
    val lobbyState by viewModel.lobbyState.collectAsState()
    val lobbyChatMessages by viewModel.lobbyChatMessages.collectAsState()
    val isAuthenticated by viewModel.isAuthenticated.collectAsState()
    var chatDraft by remember { mutableStateOf("") }

    LaunchedEffect(Unit) {
        viewModel.refreshLobbyStateAsync()
    }

    Card(
        modifier = modifier
            .fillMaxSize()
            .padding(12.dp),
        colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
        border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
    ) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(12.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column {
                    Text(
                        text = "Lobby Chat",
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold,
                        color = OctiTheme.colors.accentBrass
                    )
                    Text(
                        text = "6h history • Local time",
                        style = MaterialTheme.typography.labelSmall,
                        color = OctiTheme.colors.textSecondary
                    )
                }
                Row(
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    OutlinedButton(
                        onClick = { viewModel.selectTab(OctiAppViewModel.AppTab.ONLINE_LOBBY) },
                        contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                    ) {
                        Text("Online Lobby", maxLines = 1, softWrap = false)
                    }
                    OutlinedButton(
                        onClick = { viewModel.refreshLobbyStateAsync() },
                        contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp)
                    ) {
                        Text("↻", maxLines = 1, softWrap = false)
                    }
                }
            }

            LazyColumn(
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f),
                verticalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                val formattedList = if (lobbyChatMessages.isNotEmpty()) {
                    lobbyChatMessages
                } else {
                    lobbyState.recentChat.map { OctiAppViewModel.formatLobbyChatEntry(it) }
                }
                if (formattedList.isEmpty()) {
                    item {
                        Text(
                            text = "No messages in lobby chat yet. Say hello!",
                            style = MaterialTheme.typography.bodySmall,
                            color = OctiTheme.colors.textSecondary,
                            modifier = Modifier.padding(vertical = 12.dp)
                        )
                    }
                } else {
                    items(formattedList) { line ->
                        Text(
                            text = line,
                            style = MaterialTheme.typography.bodyMedium,
                            color = OctiTheme.colors.textPrimary
                        )
                    }
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

@Composable
fun ConnectedUsersAndRobotsDialog(
    viewModel: OctiAppViewModel,
    onDismiss: () -> Unit
) {
    val lobbyState by viewModel.lobbyState.collectAsState()
    val humanPlayers = remember(lobbyState.players) {
        lobbyState.players.filter {
            it.handle != "anonymous-127.0.0.1" && it.handle != "anonymous-::1" && !it.isRobot && it.robotUuid.isEmpty()
        }
    }
    val robotPlayers = remember(lobbyState.players) {
        lobbyState.players.filter {
            it.handle != "anonymous-127.0.0.1" && it.handle != "anonymous-::1" && (it.isRobot || it.robotUuid.isNotEmpty())
        }
    }

    Dialog(
        onDismissRequest = onDismiss,
        properties = DialogProperties(usePlatformDefaultWidth = false)
    ) {
        Card(
            modifier = Modifier
                .fillMaxWidth(0.94f)
                .fillMaxHeight(0.85f)
                .padding(8.dp),
            shape = RoundedCornerShape(16.dp),
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
            border = BorderStroke(2.dp, OctiTheme.colors.accentBrass)
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(14.dp),
                verticalArrangement = Arrangement.spacedBy(10.dp)
            ) {
                // Header
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = "Connected Users & Robots",
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold,
                        color = OctiTheme.colors.accentBrass
                    )
                    IconButton(onClick = onDismiss) {
                        Text(
                            text = "✕",
                            style = MaterialTheme.typography.titleMedium,
                            color = OctiTheme.colors.textSecondary
                        )
                    }
                }

                LazyColumn(
                    modifier = Modifier
                        .fillMaxWidth()
                        .weight(1f),
                    verticalArrangement = Arrangement.spacedBy(12.dp)
                ) {
                    // Connected Users Section
                    item {
                        Text(
                            text = "Connected Users (${humanPlayers.size})",
                            style = MaterialTheme.typography.titleSmall,
                            fontWeight = FontWeight.Bold,
                            color = OctiTheme.colors.accentBrass
                        )
                    }
                    if (humanPlayers.isEmpty()) {
                        item {
                            Text(
                                text = "No other users connected right now.",
                                style = MaterialTheme.typography.bodySmall,
                                color = OctiTheme.colors.textSecondary,
                                modifier = Modifier.padding(vertical = 4.dp)
                            )
                        }
                    } else {
                        items(humanPlayers) { p ->
                            val cleanHandle = p.handle.removePrefix("@")
                            Surface(
                                modifier = Modifier.fillMaxWidth(),
                                shape = RoundedCornerShape(10.dp),
                                color = OctiTheme.colors.windowBg,
                                border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
                            ) {
                                Row(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(8.dp),
                                    horizontalArrangement = Arrangement.SpaceBetween,
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    Row(
                                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                                        verticalAlignment = Alignment.CenterVertically,
                                        modifier = Modifier.weight(1f)
                                    ) {
                                        OctiPlayerAvatar(
                                            avatarUrl = p.avatarUrl,
                                            handle = p.handle,
                                            size = 28.dp,
                                            netClient = viewModel.networkClient()
                                        )
                                        Column {
                                            Text(
                                                text = "@$cleanHandle",
                                                style = MaterialTheme.typography.bodyMedium,
                                                fontWeight = FontWeight.Bold,
                                                color = OctiTheme.colors.accentBrass
                                            )
                                            val onlineText = "On ${formatCompactDurationSec(p.onlineSec)}"
                                            val idleText = if (p.idleSec < 10L) "Active" else "Idle ${formatCompactDurationSec(p.idleSec)}"
                                            Text(
                                                text = "$onlineText • $idleText",
                                                style = MaterialTheme.typography.labelSmall,
                                                color = OctiTheme.colors.textSecondary
                                            )
                                        }
                                    }
                                    Button(
                                        onClick = {
                                            onDismiss()
                                            if (p.roomId.isNotEmpty()) {
                                                viewModel.observeOnlineTableAsync(p.roomId, "")
                                            } else {
                                                viewModel.setShowCreateTableDialog(true)
                                            }
                                        },
                                        modifier = Modifier.height(30.dp),
                                        contentPadding = PaddingValues(horizontal = 10.dp, vertical = 0.dp),
                                        colors = ButtonDefaults.buttonColors(
                                            containerColor = OctiTheme.colors.accentBrass,
                                            contentColor = OctiTheme.colors.windowBg
                                        )
                                    ) {
                                        Text("Challenge", style = MaterialTheme.typography.labelSmall, fontWeight = FontWeight.Bold)
                                    }
                                }
                            }
                        }
                    }

                    // Connected Robots Section
                    item {
                        Spacer(modifier = Modifier.height(4.dp))
                        Text(
                            text = "Connected Robots (${robotPlayers.size})",
                            style = MaterialTheme.typography.titleSmall,
                            fontWeight = FontWeight.Bold,
                            color = OctiTheme.colors.accentBrass
                        )
                    }
                    if (robotPlayers.isEmpty()) {
                        item {
                            Text(
                                text = "No automated robots connected right now.",
                                style = MaterialTheme.typography.bodySmall,
                                color = OctiTheme.colors.textSecondary,
                                modifier = Modifier.padding(vertical = 4.dp)
                            )
                        }
                    } else {
                        items(robotPlayers) { p ->
                            val cleanHandle = p.handle.removePrefix("@")
                            Surface(
                                modifier = Modifier.fillMaxWidth(),
                                shape = RoundedCornerShape(10.dp),
                                color = OctiTheme.colors.windowBg,
                                border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
                            ) {
                                Row(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(8.dp),
                                    horizontalArrangement = Arrangement.SpaceBetween,
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    Row(
                                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                                        verticalAlignment = Alignment.CenterVertically,
                                        modifier = Modifier.weight(1f)
                                    ) {
                                        OctiPlayerAvatar(
                                            avatarUrl = p.avatarUrl,
                                            handle = p.handle,
                                            size = 28.dp,
                                            netClient = viewModel.networkClient()
                                        )
                                        Column {
                                            Text(
                                                text = "@$cleanHandle",
                                                style = MaterialTheme.typography.bodyMedium,
                                                fontWeight = FontWeight.Bold,
                                                color = OctiTheme.colors.accentBrass
                                            )
                                            Text(
                                                text = "Automated bot instance",
                                                style = MaterialTheme.typography.labelSmall,
                                                color = OctiTheme.colors.textSecondary
                                            )
                                        }
                                    }
                                    Button(
                                        onClick = {
                                            onDismiss()
                                            viewModel.startNewLocalGame(
                                                mode = OctiGameMode.GAME_MODE_2,
                                                vsBot = true,
                                                difficulty = OctiBotEngine.BotDifficulty.TACTICAL
                                            )
                                        },
                                        modifier = Modifier.height(30.dp),
                                        contentPadding = PaddingValues(horizontal = 10.dp, vertical = 0.dp),
                                        colors = ButtonDefaults.buttonColors(
                                            containerColor = OctiTheme.colors.accentBrass,
                                            contentColor = OctiTheme.colors.windowBg
                                        )
                                    ) {
                                        Text("Play vs Bot", style = MaterialTheme.typography.labelSmall, fontWeight = FontWeight.Bold)
                                    }
                                }
                            }
                        }
                    }
                }

                // Close Button
                Button(
                    onClick = onDismiss,
                    modifier = Modifier.fillMaxWidth(),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = OctiTheme.colors.accentBrass,
                        contentColor = OctiTheme.colors.windowBg
                    )
                ) {
                    Text("Close", fontWeight = FontWeight.Bold)
                }
            }
        }
    }
}
