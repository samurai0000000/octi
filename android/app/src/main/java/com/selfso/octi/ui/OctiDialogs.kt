/*
 * OctiDialogs.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import android.graphics.Bitmap
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectTapGestures
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
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import com.selfso.octi.OctiBoard
import com.selfso.octi.OctiBoardGeometry
import com.selfso.octi.OctiBoardRenderer
import com.selfso.octi.OctiBotEngine
import com.selfso.octi.OctiDefs
import com.selfso.octi.OctiDir
import com.selfso.octi.OctiGameBases
import com.selfso.octi.OctiGameMode
import com.selfso.octi.OctiMoveComposer
import com.selfso.octi.OctiNetworkClient
import com.selfso.octi.OctiPlayer
import com.selfso.octi.OctiProngType
import com.selfso.octi.OctiSquare
import com.selfso.octi.OctiThemeId
import com.selfso.octi.ui.theme.OctiTheme
import kotlinx.coroutines.delay

private val CANONICAL_SSO_PROVIDERS = listOf(
    "google" to "Google",
    "apple" to "Apple",
    "microsoft" to "Microsoft",
    "github" to "GitHub",
    "facebook" to "Facebook"
)

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun NewGameSetupDialog(
    viewModel: OctiAppViewModel,
    onDismiss: () -> Unit
) {
    var mode by remember { mutableStateOf(OctiGameMode.GAME_MODE_2) }
    var threeBases by remember { mutableStateOf(true) }
    var useSuperProngs by remember { mutableStateOf(false) }
    var edgeless by remember { mutableStateOf(false) }
    var vsBot by remember { mutableStateOf(false) }
    var difficulty by remember { mutableStateOf(OctiBotEngine.BotDifficulty.TACTICAL) }

    AlertDialog(
        onDismissRequest = onDismiss,
        containerColor = OctiTheme.colors.panelCardBg,
        titleContentColor = OctiTheme.colors.accentBrass,
        textContentColor = OctiTheme.colors.textPrimary,
        title = { Text("New Local OCTI Match") },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                Text("Board Mode:", fontWeight = FontWeight.SemiBold)
                FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    FilterChip(
                        selected = mode == OctiGameMode.GAME_MODE_2,
                        onClick = {
                            mode = OctiGameMode.GAME_MODE_2
                            threeBases = true
                        },
                        label = { Text("2-Player (9x9)") }
                    )
                    FilterChip(
                        selected = mode == OctiGameMode.GAME_MODE_4,
                        onClick = {
                            mode = OctiGameMode.GAME_MODE_4
                            threeBases = false
                        },
                        label = { Text("4-Player (9x9)") }
                    )
                    FilterChip(
                        selected = mode == OctiGameMode.GAME_MODE_OFK,
                        onClick = {
                            mode = OctiGameMode.GAME_MODE_OFK
                            threeBases = false
                        },
                        label = { Text("OCTI for Kids (6x7)") }
                    )
                }

                if (mode == OctiGameMode.GAME_MODE_2) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Checkbox(checked = threeBases, onCheckedChange = { threeBases = it })
                        Text("3 Bases per Player (Tournament Standard)")
                    }
                }
                if (mode != OctiGameMode.GAME_MODE_OFK) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Checkbox(checked = useSuperProngs, onCheckedChange = { useSuperProngs = it })
                        Text("Enable Super-Prongs")
                    }
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Checkbox(checked = edgeless, onCheckedChange = { edgeless = it })
                        Text("Edgeless Toroidal Board Wrap")
                    }
                }

                Row(verticalAlignment = Alignment.CenterVertically) {
                    Checkbox(checked = vsBot, onCheckedChange = { vsBot = it })
                    Text("Play vs. Built-in AI Bot")
                }

                if (vsBot) {
                    Text("Bot Difficulty:", fontWeight = FontWeight.SemiBold)
                    FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                        OctiBotEngine.BotDifficulty.values().forEach { diff ->
                            FilterChip(
                                selected = difficulty == diff,
                                onClick = { difficulty = diff },
                                label = { Text(diff.name) }
                            )
                        }
                    }
                }
            }
        },
        confirmButton = {
            Button(
                onClick = {
                    val bases = if (mode == OctiGameMode.GAME_MODE_OFK) {
                        OctiGameBases.GAME_BASES_1
                    } else if (threeBases) {
                        OctiGameBases.GAME_BASES_3
                    } else {
                        OctiGameBases.GAME_BASES_1
                    }
                    viewModel.startNewLocalGame(
                        mode = mode,
                        useSuperProngs = (mode != OctiGameMode.GAME_MODE_OFK) && useSuperProngs,
                        useEdgeless = (mode != OctiGameMode.GAME_MODE_OFK) && edgeless,
                        bases = bases,
                        vsBot = vsBot,
                        difficulty = difficulty,
                        humanPlayer = OctiPlayer.PLAYER_A
                    )
                    onDismiss()
                },
                colors = ButtonDefaults.buttonColors(
                    containerColor = OctiTheme.colors.accentBrass,
                    contentColor = OctiTheme.colors.windowBg
                )
            ) {
                Text("Start Match")
            }
        },
        dismissButton = {
            OutlinedButton(onClick = onDismiss) {
                Text("Cancel")
            }
        }
    )
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun SsoSignInDialog(
    viewModel: OctiAppViewModel,
    onDismiss: () -> Unit
) {
    val onboardingRequired by viewModel.onboardingRequired.collectAsState()
    var selectedProvider by remember { mutableStateOf("google") }
    var tokenInput by remember { mutableStateOf("") }
    var desiredHandle by remember { mutableStateOf("") }

    AlertDialog(
        onDismissRequest = onDismiss,
        containerColor = OctiTheme.colors.panelCardBg,
        titleContentColor = OctiTheme.colors.accentBrass,
        textContentColor = OctiTheme.colors.textPrimary,
        title = {
            Text(
                if (onboardingRequired) {
                    "Choose Your OCTI Handle"
                } else {
                    "Sign In — 5-Provider SSO"
                }
            )
        },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                if (!onboardingRequired) {
                    FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                        CANONICAL_SSO_PROVIDERS.forEach { (id, label) ->
                            FilterChip(
                                selected = selectedProvider == id,
                                onClick = { selectedProvider = id },
                                label = { Text(label) }
                            )
                        }
                    }
                    OutlinedTextField(
                        value = tokenInput,
                        onValueChange = { tokenInput = it },
                        label = { Text("OAuth ID/Access Token") },
                        singleLine = true,
                        modifier = Modifier.fillMaxWidth()
                    )
                }
                OutlinedTextField(
                    value = desiredHandle,
                    onValueChange = { desiredHandle = it },
                    label = { Text("Player Handle (3..20 chars)") },
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth()
                )
            }
        },
        confirmButton = {
            Button(
                onClick = {
                    if (onboardingRequired) {
                        val res = viewModel.completeHandleOnboarding(desiredHandle.trim())
                        if (res.ok) {
                            onDismiss()
                        }
                    } else {
                        val res = viewModel.loginWithSso(
                            selectedProvider,
                            tokenInput.trim(),
                            desiredHandle.trim()
                        )
                        if (res.ok && !viewModel.onboardingRequired.value) {
                            onDismiss()
                        }
                    }
                },
                colors = ButtonDefaults.buttonColors(
                    containerColor = OctiTheme.colors.accentBrass,
                    contentColor = OctiTheme.colors.windowBg
                )
            ) {
                Text(if (onboardingRequired) "Save Handle" else "Sign In")
            }
        },
        dismissButton = {
            OutlinedButton(onClick = onDismiss) {
                Text("Close")
            }
        }
    )
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun AccountIdentitiesDialog(
    viewModel: OctiAppViewModel,
    onDismiss: () -> Unit
) {
    val profile by viewModel.currentProfile.collectAsState()
    val identities by viewModel.linkedIdentities.collectAsState()
    val conflictProfile by viewModel.linkConflictProfile.collectAsState()

    var linkProvider by remember { mutableStateOf("github") }
    var linkToken by remember { mutableStateOf("") }

    AlertDialog(
        onDismissRequest = onDismiss,
        containerColor = OctiTheme.colors.panelCardBg,
        titleContentColor = OctiTheme.colors.accentBrass,
        textContentColor = OctiTheme.colors.textPrimary,
        title = { Text("Account & Linked SSO Identities") },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                val p = profile
                if (p != null) {
                    Text(
                        text = "Handle: ${p.handle}  (UUID: ${p.uuid})",
                        fontWeight = FontWeight.Bold
                    )
                    Text(
                        text = "Elo 2P: ${p.elo2p}  •  4P: ${p.elo4p}  •  OFK: ${p.eloOfk}",
                        style = MaterialTheme.typography.bodyMedium,
                        color = OctiTheme.colors.textSecondary
                    )
                }

                Spacer(modifier = Modifier.height(4.dp))
                Text("Linked Providers (${identities.size}):", fontWeight = FontWeight.SemiBold)
                identities.forEach { idInfo ->
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text("${idInfo.provider}: ${idInfo.providerEmail}")
                        OutlinedButton(
                            onClick = { viewModel.unlinkIdentity(idInfo.provider) },
                            enabled = identities.size > 1
                        ) {
                            Text("Unlink")
                        }
                    }
                }

                if (conflictProfile != null) {
                    Spacer(modifier = Modifier.height(6.dp))
                    Text(
                        text = "409 Conflict: Identity is already linked to account '${conflictProfile?.handle}'.",
                        color = OctiTheme.colors.legalCapture,
                        fontWeight = FontWeight.Bold
                    )
                    Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        Button(
                            onClick = { viewModel.confirmSwitchToConflictAccount() },
                            colors = ButtonDefaults.buttonColors(
                                containerColor = OctiTheme.colors.accentBrass,
                                contentColor = OctiTheme.colors.windowBg
                            )
                        ) {
                            Text("Switch to ${conflictProfile?.handle}")
                        }
                        OutlinedButton(onClick = { viewModel.dismissLinkConflict() }) {
                            Text("Cancel")
                        }
                    }
                } else {
                    Spacer(modifier = Modifier.height(6.dp))
                    Text("Link Additional SSO Provider:", fontWeight = FontWeight.SemiBold)
                    FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                        CANONICAL_SSO_PROVIDERS.forEach { (id, label) ->
                            FilterChip(
                                selected = linkProvider == id,
                                onClick = { linkProvider = id },
                                label = { Text(label) }
                            )
                        }
                    }
                    OutlinedTextField(
                        value = linkToken,
                        onValueChange = { linkToken = it },
                        label = { Text("Provider Token to Link") },
                        singleLine = true,
                        modifier = Modifier.fillMaxWidth()
                    )
                    Button(
                        onClick = {
                            if (linkToken.isNotBlank()) {
                                viewModel.linkIdentity(linkProvider, linkToken.trim())
                                linkToken = ""
                            }
                        },
                        colors = ButtonDefaults.buttonColors(
                            containerColor = OctiTheme.colors.accentBrass,
                            contentColor = OctiTheme.colors.windowBg
                        )
                    ) {
                        Text("Link $linkProvider")
                    }
                }
            }
        },
        confirmButton = {
            OutlinedButton(
                onClick = {
                    viewModel.logoutOnline()
                    onDismiss()
                }
            ) {
                Text("Sign Out")
            }
        },
        dismissButton = {
            OutlinedButton(onClick = onDismiss) {
                Text("Close")
            }
        }
    )
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun CreateTableDialog(
    viewModel: OctiAppViewModel,
    onDismiss: () -> Unit
) {
    var mode by remember { mutableStateOf("2P") }
    var bases by remember { mutableStateOf(3) }
    var superProngs by remember { mutableStateOf(false) }
    var edgeless by remember { mutableStateOf(false) }
    var rated by remember { mutableStateOf(true) }
    var allowTakeback by remember { mutableStateOf(false) }
    var passcode by remember { mutableStateOf("") }

    AlertDialog(
        onDismissRequest = onDismiss,
        containerColor = OctiTheme.colors.panelCardBg,
        titleContentColor = OctiTheme.colors.accentBrass,
        textContentColor = OctiTheme.colors.textPrimary,
        title = { Text("Create Online Table") },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    listOf("2P", "4P", "OFK").forEach { m ->
                        FilterChip(
                            selected = mode == m,
                            onClick = {
                                mode = m
                                bases = if (m == "4P") 1 else 3
                            },
                            label = { Text(m) }
                        )
                    }
                }
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Checkbox(checked = superProngs, onCheckedChange = { superProngs = it })
                    Text("Super-Prongs")
                }
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Checkbox(checked = edgeless, onCheckedChange = { edgeless = it })
                    Text("Edgeless Board")
                }
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Checkbox(checked = rated, onCheckedChange = { rated = it })
                    Text("Elo Rated Match")
                }
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Checkbox(checked = allowTakeback, onCheckedChange = { allowTakeback = it })
                    Text("Allow Mutual Takebacks")
                }
                OutlinedTextField(
                    value = passcode,
                    onValueChange = { passcode = it },
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
                        this.mode = mode
                        this.bases = bases
                        this.superProngs = superProngs
                        this.edgeless = edgeless
                        this.rated = rated
                        this.allowTakeback = allowTakeback
                        this.passcode = passcode.trim()
                    }
                    val res = viewModel.createOnlineTable(req)
                    if (res.ok) {
                        onDismiss()
                    }
                },
                colors = ButtonDefaults.buttonColors(
                    containerColor = OctiTheme.colors.accentBrass,
                    contentColor = OctiTheme.colors.windowBg
                )
            ) {
                Text("Create Table")
            }
        },
        dismissButton = {
            OutlinedButton(onClick = onDismiss) {
                Text("Cancel")
            }
        }
    )
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun ZoomedSquareDialog(
    board: OctiBoard,
    composer: OctiMoveComposer,
    squareX: Int,
    squareY: Int,
    rotation: OctiBoardGeometry.BoardRotation,
    themeId: OctiThemeId,
    onDismiss: () -> Unit,
    onBeginGuidedMove: (podIndex: Int, moveAllStack: Boolean) -> Unit,
    onAddProng: (podIndex: Int, dir: OctiDir, superProng: Boolean) -> Unit,
    onRepositionProng: (podIndex: Int, srcDir: OctiDir, dstDir: OctiDir) -> Unit,
    onPlaceReservePod: (liberate: Boolean) -> Unit
) {
    val sq = if (board.isValidCoord(squareX - 1, squareY - 1)) {
        board.squareAt(squareX - 1, squareY - 1)
    } else {
        null
    }
    val numPods = sq?.numPods ?: 0
    val isEmptySquare = sq == null || sq.isEmpty
    val turn = board.game?.turn ?: OctiPlayer.PLAYER_A
    val isTurn = board.game != null && board.game.isGameStarted && !board.game.isGameOver
    val hasEnemy = sq?.hasEnemyOf(turn) == true
    val isFriendlyTurn = !hasEnemy && isTurn && !isEmptySquare

    var selectedPodIndex by remember {
        mutableStateOf(
            if (numPods > 0) Math.max(0, Math.min(numPods - 1, composer.selectedStackPodIndex(squareX, squareY))) else 0
        )
    }
    var moveAllStack by remember { mutableStateOf(false) }
    var selectedProngTray by remember { mutableStateOf<String?>(null) }
    var selectedRepositionSrcDir by remember { mutableStateOf<OctiDir?>(null) }
    var hoverSocketDir by remember { mutableStateOf(-1) }

    val freeProngs = board.game?.getFreeProngs(turn) ?: 0
    val freeSuper = board.game?.getFreeSuperProngs(turn) ?: 0
    val useSuperProngs = board.game?.useSuperProngs() == true

    val zoomedBitmap = remember(squareX, squareY, selectedPodIndex, rotation, themeId, hoverSocketDir, composer.stagedMoveNotation()) {
        val sizePx = 480
        val pixels = OctiBoardRenderer.renderZoomedPodPixels(
            board,
            squareX,
            squareY,
            selectedPodIndex,
            rotation.degrees,
            themeId,
            sizePx,
            hoverSocketDir
        )
        Bitmap.createBitmap(pixels, sizePx, sizePx, Bitmap.Config.ARGB_8888).asImageBitmap()
    }

    Dialog(
        onDismissRequest = onDismiss,
        properties = DialogProperties(usePlatformDefaultWidth = false)
    ) {
        Card(
            modifier = Modifier
                .fillMaxWidth(0.94f)
                .padding(8.dp),
            shape = RoundedCornerShape(12.dp),
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg)
        ) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(14.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = "Square ($squareX, $squareY)",
                        style = MaterialTheme.typography.titleLarge,
                        fontWeight = FontWeight.Bold,
                        color = OctiTheme.colors.accentBrass
                    )
                    Text(
                        text = if (hasEnemy) "Opponent Inspect" else if (isEmptySquare) "Home Square" else "Friendly Pod",
                        style = MaterialTheme.typography.labelMedium,
                        color = if (hasEnemy) OctiTheme.colors.legalCapture else OctiTheme.colors.textSecondary
                    )
                }

                if (numPods >= 2) {
                    FlowRow(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(6.dp)
                    ) {
                        for (i in 0 until numPods) {
                            FilterChip(
                                selected = i == selectedPodIndex,
                                onClick = {
                                    selectedPodIndex = i
                                    composer.selectStackPodIndex(squareX, squareY, i)
                                    selectedProngTray = null
                                    selectedRepositionSrcDir = null
                                },
                                label = { Text("Pod ${i + 1}/$numPods") }
                            )
                        }
                    }
                    if (isFriendlyTurn) {
                        Row(
                            verticalAlignment = Alignment.CenterVertically,
                            modifier = Modifier.fillMaxWidth()
                        ) {
                            Checkbox(
                                checked = moveAllStack,
                                onCheckedChange = { moveAllStack = it }
                            )
                            Text("All Matching Pods (Stack)", style = MaterialTheme.typography.bodyMedium)
                        }
                    }
                }

                if (isFriendlyTurn) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceEvenly,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        FilterChip(
                            selected = selectedProngTray == "NORMAL",
                            onClick = {
                                selectedProngTray = if (selectedProngTray == "NORMAL") null else "NORMAL"
                                selectedRepositionSrcDir = null
                            },
                            enabled = freeProngs > 0,
                            label = { Text("Normal Prong ($freeProngs)") }
                        )
                        if (useSuperProngs) {
                            FilterChip(
                                selected = selectedProngTray == "SUPER",
                                onClick = {
                                    selectedProngTray = if (selectedProngTray == "SUPER") null else "SUPER"
                                    selectedRepositionSrcDir = null
                                },
                                enabled = freeSuper > 0,
                                label = { Text("Superprong (!) ($freeSuper)") }
                            )
                        }
                    }
                    if (selectedProngTray != null) {
                        Text(
                            text = "Tap any socket (a..h) to add $selectedProngTray prong",
                            style = MaterialTheme.typography.bodySmall,
                            color = OctiTheme.colors.legalStep
                        )
                    } else if (selectedRepositionSrcDir != null) {
                        Text(
                            text = "Reposition prong: Tap empty target socket",
                            style = MaterialTheme.typography.bodySmall,
                            color = OctiTheme.colors.accentBrass
                        )
                    } else {
                        Text(
                            text = "Tap a prong socket to select, or select from tray above",
                            style = MaterialTheme.typography.bodySmall,
                            color = OctiTheme.colors.textSecondary
                        )
                    }
                }

                Box(
                    modifier = Modifier
                        .size(280.dp)
                        .pointerInput(selectedPodIndex, rotation, isFriendlyTurn, selectedProngTray, selectedRepositionSrcDir) {
                            detectTapGestures(
                                onTap = { offset ->
                                    if (!isFriendlyTurn || sq == null || sq.isEmpty) return@detectTapGestures
                                    val pod = sq.getPodAt(Math.max(0, Math.min(sq.numPods - 1, selectedPodIndex)))
                                    val sizePx = 280f
                                    val cx = sizePx * 0.5f
                                    val cy = sizePx * 0.5f
                                    val podR = sizePx * 0.30f
                                    val socketDist = podR * 0.88f
                                    val socketR = sizePx * 0.052f
                                    val radRot = Math.toRadians(rotation.degrees.toDouble())

                                    var hitDir = -1
                                    for (d in 0 until OctiDir.DIR_MAX) {
                                        val dir = OctiDir.fromIndex(d)
                                        val dx = dir.deltaX().toDouble()
                                        val dy = -dir.deltaY().toDouble()
                                        val u0 = dx * Math.cos(radRot) - dy * Math.sin(radRot)
                                        val u1 = dx * Math.sin(radRot) + dy * Math.cos(radRot)
                                        val scx = cx + u0 * socketDist
                                        val scy = cy + u1 * socketDist
                                        if (Math.hypot(offset.x - scx, offset.y - scy) <= socketR * 1.6) {
                                            hitDir = d
                                            break
                                        }
                                    }

                                    if (hitDir >= 0) {
                                        val targetDir = OctiDir.fromIndex(hitDir)
                                        if (selectedProngTray == "NORMAL") {
                                            onAddProng(selectedPodIndex, targetDir, false)
                                            selectedProngTray = null
                                            hoverSocketDir = -1
                                        } else if (selectedProngTray == "SUPER") {
                                            onAddProng(selectedPodIndex, targetDir, true)
                                            selectedProngTray = null
                                            hoverSocketDir = -1
                                        } else if (selectedRepositionSrcDir != null) {
                                            onRepositionProng(selectedPodIndex, selectedRepositionSrcDir!!, targetDir)
                                            selectedRepositionSrcDir = null
                                            hoverSocketDir = -1
                                        } else {
                                            if (pod.prongAt(targetDir) != OctiProngType.NO_PRONG) {
                                                selectedRepositionSrcDir = targetDir
                                                hoverSocketDir = hitDir
                                            }
                                        }
                                    }
                                }
                            )
                        }
                ) {
                    Image(
                        bitmap = zoomedBitmap,
                        contentDescription = "Zoomed Square Pod",
                        modifier = Modifier.fillMaxSize()
                    )
                }

                if (isFriendlyTurn) {
                    Button(
                        onClick = {
                            onBeginGuidedMove(selectedPodIndex, moveAllStack)
                            onDismiss()
                        },
                        modifier = Modifier.fillMaxWidth(),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = OctiTheme.colors.accentBrass,
                            contentColor = OctiTheme.colors.windowBg
                        )
                    ) {
                        Text("Move / Jump This Pod", fontWeight = FontWeight.Bold)
                    }
                }

                if (isEmptySquare && isTurn) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        Button(
                            onClick = {
                                onPlaceReservePod(false)
                                onDismiss()
                            },
                            modifier = Modifier.weight(1f)
                        ) {
                            Text("Place Reserve (+)")
                        }
                        OutlinedButton(
                            onClick = {
                                onPlaceReservePod(true)
                                onDismiss()
                            },
                            modifier = Modifier.weight(1f)
                        ) {
                            Text("Liberate (*)")
                        }
                    }
                }

                OutlinedButton(
                    onClick = onDismiss,
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("Close Inspector [Esc]")
                }
            }
        }
    }
}

@Composable
fun CapturePromptDialog(
    jumpedX: Int,
    jumpedY: Int,
    player: OctiPlayer,
    onChoice: (Boolean) -> Unit,
    onDismiss: () -> Unit
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        containerColor = OctiTheme.colors.panelCardBg,
        titleContentColor = OctiTheme.colors.accentBrass,
        textContentColor = OctiTheme.colors.textPrimary,
        title = { Text("Capture Jumped Pod?") },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                Text("You jumped over a pod at ($jumpedX, $jumpedY).")
                Text("Would you like to capture this pod or leave it on the board?")
                Text(
                    "You can also drag a jumped pod off the board canvas to signal capture.",
                    style = MaterialTheme.typography.bodySmall,
                    color = OctiTheme.colors.textSecondary
                )
            }
        },
        confirmButton = {
            Button(
                onClick = { onChoice(true) },
                colors = ButtonDefaults.buttonColors(
                    containerColor = OctiTheme.colors.legalCapture,
                    contentColor = OctiTheme.colors.windowBg
                )
            ) {
                Text("Capture (Remove from Board)")
            }
        },
        dismissButton = {
            OutlinedButton(onClick = { onChoice(false) }) {
                Text("Leave on Board")
            }
        }
    )
}

@Composable
fun SuperProngBonusDialog(
    onMoveNow: () -> Unit,
    onSkip: () -> Unit
) {
    AlertDialog(
        onDismissRequest = onSkip,
        containerColor = OctiTheme.colors.panelCardBg,
        titleContentColor = OctiTheme.colors.accentBrass,
        textContentColor = OctiTheme.colors.textPrimary,
        title = { Text("8th-Prong Superprong Bonus Move Available!") },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(6.dp)) {
                Text("Adding this superprong filled the 8th prong socket!")
                Text("You earned an immediate bonus move or jump with this pod.")
            }
        },
        confirmButton = {
            Button(
                onClick = onMoveNow,
                colors = ButtonDefaults.buttonColors(
                    containerColor = OctiTheme.colors.accentBrass,
                    contentColor = OctiTheme.colors.windowBg
                )
            ) {
                Text("Move / Jump Now")
            }
        },
        dismissButton = {
            OutlinedButton(onClick = onSkip) {
                Text("Skip Bonus Move")
            }
        }
    )
}

@Composable
fun CompositePodMoveDialog(
    originX: Int,
    originY: Int,
    onSelectNextPod: () -> Unit,
    onProceedToConfirm: () -> Unit
) {
    AlertDialog(
        onDismissRequest = onProceedToConfirm,
        containerColor = OctiTheme.colors.panelCardBg,
        titleContentColor = OctiTheme.colors.accentBrass,
        textContentColor = OctiTheme.colors.textPrimary,
        title = { Text("Move Another Pod from ($originX, $originY)?") },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(6.dp)) {
                Text("Another friendly pod on the starting square can also move this turn.")
                Text("Would you like to move another pod as part of a composite move?")
            }
        },
        confirmButton = {
            Button(
                onClick = onSelectNextPod,
                colors = ButtonDefaults.buttonColors(
                    containerColor = OctiTheme.colors.accentBrass,
                    contentColor = OctiTheme.colors.windowBg
                )
            ) {
                Text("Select Next Pod")
            }
        },
        dismissButton = {
            OutlinedButton(onClick = onProceedToConfirm) {
                Text("Proceed to Confirm Move")
            }
        }
    )
}

@Composable
fun OctiSplashOverlay(
    onDismiss: () -> Unit
) {
    var progress by remember { mutableStateOf(0f) }

    LaunchedEffect(Unit) {
        val start = System.currentTimeMillis()
        while (progress < 1f) {
            delay(16)
            val elapsed = System.currentTimeMillis() - start
            progress = (elapsed / 5000f).coerceIn(0f, 1f)
        }
        onDismiss()
    }

    val splashBitmap = remember(progress) {
        val w = 480
        val h = 800
        val pixels = OctiBoardRenderer.renderSplashPixels(w, h, progress.toDouble())
        Bitmap.createBitmap(pixels, w, h, Bitmap.Config.ARGB_8888).asImageBitmap()
    }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(OctiTheme.colors.windowBg)
            .clickable { onDismiss() },
        contentAlignment = Alignment.Center
    ) {
        Image(
            bitmap = splashBitmap,
            contentDescription = "OCTI Splash",
            modifier = Modifier.fillMaxSize()
        )

        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(24.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.SpaceBetween
        ) {
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                modifier = Modifier.padding(top = 40.dp)
            ) {
                Text(
                    text = "OCTI",
                    style = MaterialTheme.typography.displayMedium,
                    fontWeight = FontWeight.ExtraBold,
                    color = OctiTheme.colors.accentBrass
                )
                Text(
                    text = "TACTICAL POD STRATEGY",
                    style = MaterialTheme.typography.titleSmall,
                    letterSpacing = 2.sp,
                    color = OctiTheme.colors.textSecondary
                )
            }

            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.spacedBy(4.dp),
                modifier = Modifier.padding(bottom = 36.dp)
            ) {
                Text(
                    text = "Author: Charles Chiou (selfso.com)",
                    style = MaterialTheme.typography.bodyMedium,
                    color = OctiTheme.colors.textPrimary
                )
                Text(
                    text = "Original OCTI Game Designed by Don Green",
                    style = MaterialTheme.typography.bodySmall,
                    color = OctiTheme.colors.textSecondary
                )
                Text(
                    text = "Copyright (C) 2026, Charles Chiou",
                    style = MaterialTheme.typography.bodySmall,
                    color = OctiTheme.colors.textSecondary
                )
                Text(
                    text = "Version ${OctiDefs.OCTI_APP_VERSION} (Protocol v${OctiDefs.OCTI_PROTOCOL_VERSION})",
                    style = MaterialTheme.typography.labelMedium,
                    color = OctiTheme.colors.accentBrass,
                    fontWeight = FontWeight.Bold
                )
                Spacer(modifier = Modifier.height(12.dp))
                Text(
                    text = "Press ESC or tap anywhere to continue",
                    style = MaterialTheme.typography.labelSmall,
                    color = OctiTheme.colors.textSecondary
                )
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
