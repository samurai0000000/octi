/*
 * OctiDialogs.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import android.graphics.Bitmap
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
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
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.withTransform
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.IntSize
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
import com.selfso.octi.OctiSquareType
import com.selfso.octi.OctiThemeId
import com.selfso.octi.ui.theme.OctiTheme
import kotlin.math.cos
import kotlin.math.hypot
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sin

object OctiDialogs {
    const val SPLASH_DURATION_MS: Long = 10000L

    @JvmStatic
    fun hitTestZoomedSocket(
        canvasWidthPx: Float,
        canvasHeightPx: Float,
        touchX: Float,
        touchY: Float,
        rotation: OctiBoardGeometry.BoardRotation
    ): OctiDir {
        val sizePx = min(canvasWidthPx, canvasHeightPx)
        if (sizePx <= 0f) return OctiDir.DIR_UNDEF
        val cx = canvasWidthPx * 0.5f
        val cy = canvasHeightPx * 0.5f
        val podR = sizePx * 0.30f
        val socketDist = podR * 0.76f
        val socketR = sizePx * 0.046f
        val geo = OctiBoardGeometry(480, 480, 9, 9, rotation)

        for (d in 0 until OctiDir.DIR_MAX) {
            val dir = OctiDir.fromIndex(d)
            val u = geo.dirUnitVectorScreen(dir)
            val scx = cx + (u[0] * socketDist).toFloat()
            val scy = cy + (u[1] * socketDist).toFloat()
            val dist = hypot((touchX - scx).toDouble(), (touchY - scy).toDouble()).toFloat()
            if (dist <= socketR * 1.45f) {
                return dir
            }
        }
        return geo.hitTestCompassSector(
            cx.toDouble(), cy.toDouble(),
            (podR * 0.35f).toDouble(), (podR * 1.65f).toDouble(),
            touchX.toDouble(), touchY.toDouble()
        )
    }
}

fun hitTestZoomedSocket(
    canvasWidthPx: Float,
    canvasHeightPx: Float,
    touchX: Float,
    touchY: Float,
    rotation: OctiBoardGeometry.BoardRotation
): OctiDir = OctiDialogs.hitTestZoomedSocket(canvasWidthPx, canvasHeightPx, touchX, touchY, rotation)

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
    var mode by remember { mutableStateOf(viewModel.board.gameMode) }
    var threeBases by remember {
        mutableStateOf(viewModel.board.game?.bases == OctiGameBases.GAME_BASES_3)
    }
    var useSuperProngs by remember {
        mutableStateOf(viewModel.board.game?.useSuperProngs() == true)
    }
    var edgeless by remember {
        mutableStateOf(viewModel.board.game?.useEdgelessBoard() == true)
    }
    var vsBot by remember {
        mutableStateOf(viewModel.matchType.value != OctiAppViewModel.MatchType.LOCAL_PASS_AND_PLAY)
    }
    var difficulty by remember { mutableStateOf(viewModel.botDifficulty.value) }
    var humanSeat by remember { mutableStateOf(viewModel.humanSeat.value) }

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
                            if (humanSeat != OctiPlayer.PLAYER_A && humanSeat != OctiPlayer.PLAYER_B) {
                                humanSeat = OctiPlayer.PLAYER_A
                            }
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
                            if (humanSeat != OctiPlayer.PLAYER_A && humanSeat != OctiPlayer.PLAYER_B) {
                                humanSeat = OctiPlayer.PLAYER_A
                            }
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
                            val label = when (diff) {
                                OctiBotEngine.BotDifficulty.CASUAL -> "Lv1 (3s)"
                                OctiBotEngine.BotDifficulty.TACTICAL -> "Lv2 (8s)"
                                OctiBotEngine.BotDifficulty.MASTER -> "Lv3 (15s)"
                            }
                            FilterChip(
                                selected = difficulty == diff,
                                onClick = { difficulty = diff },
                                label = { Text(label) }
                            )
                        }
                    }

                    Text("Your Seat:", fontWeight = FontWeight.SemiBold)
                    FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                        val availableSeats = if (mode == OctiGameMode.GAME_MODE_4) {
                            listOf(
                                OctiPlayer.PLAYER_A to "Seat A (Green)",
                                OctiPlayer.PLAYER_B to "Seat B (Red)",
                                OctiPlayer.PLAYER_C to "Seat C (Blue)",
                                OctiPlayer.PLAYER_D to "Seat D (Amber)"
                            )
                        } else {
                            listOf(
                                OctiPlayer.PLAYER_A to "Seat A (Green / 1st)",
                                OctiPlayer.PLAYER_B to "Seat B (Red / 2nd)"
                            )
                        }
                        availableSeats.forEach { (seat, label) ->
                            FilterChip(
                                selected = humanSeat == seat,
                                onClick = { humanSeat = seat },
                                label = { Text(label) }
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
                    val resolvedSeat = if (!vsBot) {
                        OctiPlayer.PLAYER_A
                    } else if (mode != OctiGameMode.GAME_MODE_4 &&
                        humanSeat != OctiPlayer.PLAYER_A &&
                        humanSeat != OctiPlayer.PLAYER_B
                    ) {
                        OctiPlayer.PLAYER_A
                    } else {
                        humanSeat
                    }
                    viewModel.startNewLocalGame(
                        mode = mode,
                        useSuperProngs = (mode != OctiGameMode.GAME_MODE_OFK) && useSuperProngs,
                        useEdgeless = (mode != OctiGameMode.GAME_MODE_OFK) && edgeless,
                        bases = bases,
                        vsBot = vsBot,
                        difficulty = difficulty,
                        humanPlayer = resolvedSeat
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
    val sortedPodIndices = remember(squareX, squareY, numPods, composer.stagedMoveNotation()) {
        if (sq != null) OctiBoardRenderer.sortedPodIndicesMostProngsFirst(sq).toList() else emptyList()
    }
    val defaultPodIdx = remember(sortedPodIndices) {
        val unclaimed = sortedPodIndices.firstOrNull { !composer.isSquarePodClaimed(squareX, squareY, it) }
        unclaimed ?: sortedPodIndices.firstOrNull() ?: 0
    }
    var selectedPodIndex by remember(squareX, squareY, defaultPodIdx) { mutableIntStateOf(defaultPodIdx) }
    val isEmptySquare = sq == null || sq.isEmpty
    val turn = board.game?.turn ?: OctiPlayer.PLAYER_A
    val isTurn = board.game != null && board.game.isGameStarted && !board.game.isGameOver
    val hasEnemy = sq?.hasEnemyOf(turn) == true
    val isFriendlyTurn = !hasEnemy && isTurn && !isEmptySquare
    val freePods = board.game?.getFreePods(turn) ?: 0
    val capPods = board.game?.getCapturedPods(turn) ?: 0
    val isFriendlyHome = turn != null && ((board.numPlayers == 2 && ((turn == OctiPlayer.PLAYER_A && sq?.type == OctiSquareType.A_HOME_SQUARE) || (turn == OctiPlayer.PLAYER_B && sq?.type == OctiSquareType.B_HOME_SQUARE))) || (board.numPlayers == 4 && sq?.type?.value == turn.value))
    val canShowPodPlacement = isTurn && !hasEnemy && board.gameMode != OctiGameMode.GAME_MODE_OFK && (isFriendlyHome || (!isEmptySquare && sq != null && sq.type != OctiSquareType.NEUTRAL_SQUARE))
    var moveAllStack by remember(squareX, squareY) { mutableStateOf(false) }

    val freeProngs = board.game?.getFreeProngs(turn) ?: 0
    val freeSuper = board.game?.getFreeSuperProngs(turn) ?: 0
    val useSuperProngs = board.game?.useSuperProngs() == true

    var selectedProngTray by remember(squareX, squareY) {
        mutableStateOf<String?>(if (isFriendlyTurn && freeProngs > 0) "NORMAL" else null)
    }
    var selectedRepositionSrcDir by remember(squareX, squareY) { mutableStateOf<OctiDir?>(null) }
    var hoverSocketDir by remember(squareX, squareY) { mutableStateOf(-1) }
    var podBoxPxSize by remember { mutableStateOf(IntSize(480, 480)) }

    val zoomedBitmap = remember(squareX, squareY, selectedPodIndex, rotation, themeId, hoverSocketDir, composer.stagedMoveNotation()) {
        val sizePx = 480
        val displayBoard = if (composer.isAwaitingConfirmation) composer.buildPreviewBoard() ?: board else board
        val pixels = OctiBoardRenderer.renderZoomedPodPixels(
            displayBoard,
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
        properties = DialogProperties(
            dismissOnBackPress = true,
            dismissOnClickOutside = true,
            usePlatformDefaultWidth = false
        )
    ) {
        Box(
            modifier = Modifier
                .fillMaxSize()
                .clickable(
                    interactionSource = remember { MutableInteractionSource() },
                    indication = null,
                    onClick = onDismiss
                ),
            contentAlignment = Alignment.Center
        ) {
            Card(
                modifier = Modifier
                    .fillMaxWidth(0.92f)
                    .padding(8.dp)
                    .clickable(
                        interactionSource = remember { MutableInteractionSource() },
                        indication = null,
                        onClick = {}
                    ),
                shape = RoundedCornerShape(16.dp),
                colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
                border = BorderStroke(1.5.dp, OctiTheme.colors.accentBrass)
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
                            sortedPodIndices.forEachIndexed { rank, i ->
                                val isClaimed = composer.isSquarePodClaimed(squareX, squareY, i)
                                FilterChip(
                                    selected = i == selectedPodIndex,
                                    onClick = {
                                        selectedPodIndex = i
                                        composer.selectStackPodIndex(squareX, squareY, i)
                                        selectedRepositionSrcDir = null
                                    },
                                    label = {
                                        Text(
                                            if (isClaimed) {
                                                "Pod ${i + 1}/$numPods (Moved)"
                                            } else {
                                                "Pod ${i + 1}/$numPods"
                                            }
                                        )
                                    }
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
                        if (selectedRepositionSrcDir != null) {
                            Text(
                                text = "Reposition prong: Tap or drag to empty socket",
                                style = MaterialTheme.typography.bodySmall,
                                color = OctiTheme.colors.accentBrass
                            )
                        } else if (selectedProngTray != null) {
                            Text(
                                text = "Tap socket (a..h) or drag outward from center to add $selectedProngTray prong",
                                style = MaterialTheme.typography.bodySmall,
                                color = OctiTheme.colors.legalStep
                            )
                        } else {
                            Text(
                                text = "Tap any socket to select/add, or select from tray above",
                                style = MaterialTheme.typography.bodySmall,
                                color = OctiTheme.colors.textSecondary
                            )
                        }
                    }

                    Box(
                        modifier = Modifier
                            .size(280.dp)
                            .onSizeChanged { newSize ->
                                if (newSize.width > 0 && newSize.height > 0) {
                                    podBoxPxSize = newSize
                                }
                            }
                            .pointerInput(selectedPodIndex, rotation, isFriendlyTurn, selectedProngTray, selectedRepositionSrcDir, podBoxPxSize) {
                                detectTapGestures(
                                    onTap = { offset ->
                                        if (!isFriendlyTurn || sq == null) return@detectTapGestures
                                        val pIdx = Math.max(0, Math.min(sq.numPods - 1, selectedPodIndex))
                                        val pod = sq.getPodAt(pIdx)
                                        val wPx = podBoxPxSize.width.toFloat()
                                        val hPx = podBoxPxSize.height.toFloat()
                                        val hitDir = hitTestZoomedSocket(wPx, hPx, offset.x, offset.y, rotation)
                                        if (hitDir != OctiDir.DIR_UNDEF) {
                                            val pt = pod.prongAt(hitDir)
                                            if (selectedRepositionSrcDir != null) {
                                                if (pt == OctiProngType.NO_PRONG) {
                                                    onRepositionProng(selectedPodIndex, selectedRepositionSrcDir!!, hitDir)
                                                    selectedRepositionSrcDir = null
                                                    hoverSocketDir = -1
                                                } else {
                                                    selectedRepositionSrcDir = hitDir
                                                    hoverSocketDir = hitDir.index
                                                }
                                            } else if (pt == OctiProngType.NORMAL_PRONG) {
                                                selectedRepositionSrcDir = hitDir
                                                hoverSocketDir = hitDir.index
                                            } else if (pt == OctiProngType.NO_PRONG) {
                                                if (selectedProngTray == "SUPER" && freeSuper > 0) {
                                                    onAddProng(selectedPodIndex, hitDir, true)
                                                } else if ((selectedProngTray == "NORMAL" || selectedProngTray == null) && freeProngs > 0) {
                                                    onAddProng(selectedPodIndex, hitDir, false)
                                                }
                                            }
                                        }
                                    }
                                )
                            }
                            .pointerInput(selectedPodIndex, rotation, isFriendlyTurn, selectedProngTray, selectedRepositionSrcDir, podBoxPxSize, sortedPodIndices) {
                                var isRepositionDrag = false
                                var verticalSlideAccumPx = 0f
                                val density = this
                                val stepThresholdPx = with(density) { 36.dp.toPx() }

                                detectDragGestures(
                                    onDragStart = { startOffset ->
                                        verticalSlideAccumPx = 0f
                                        isRepositionDrag = false
                                        if (sq == null) return@detectDragGestures
                                        val pIdx = Math.max(0, Math.min(sq.numPods - 1, selectedPodIndex))
                                        val pod = sq.getPodAt(pIdx)
                                        val wPx = podBoxPxSize.width.toFloat()
                                        val hPx = podBoxPxSize.height.toFloat()
                                        val hitDir = hitTestZoomedSocket(wPx, hPx, startOffset.x, startOffset.y, rotation)
                                        if (isFriendlyTurn && hitDir != OctiDir.DIR_UNDEF && pod.prongAt(hitDir) == OctiProngType.NORMAL_PRONG) {
                                            selectedRepositionSrcDir = hitDir
                                            isRepositionDrag = true
                                        }
                                    },
                                    onDrag = { change, dragAmount ->
                                        if (sq == null) return@detectDragGestures
                                        if (isRepositionDrag) {
                                            val wPx = podBoxPxSize.width.toFloat()
                                            val hPx = podBoxPxSize.height.toFloat()
                                            val hitDir = hitTestZoomedSocket(wPx, hPx, change.position.x, change.position.y, rotation)
                                            if (hitDir != OctiDir.DIR_UNDEF) {
                                                hoverSocketDir = hitDir.index
                                            } else {
                                                val cx = wPx * 0.5f
                                                val cy = hPx * 0.5f
                                                val sizeMin = min(wPx, hPx)
                                                val podR = sizeMin * 0.30f
                                                val geo = OctiBoardGeometry(480, 480, 9, 9, rotation)
                                                val sectorDir = geo.hitTestCompassSector(
                                                    cx.toDouble(), cy.toDouble(),
                                                    (podR * 0.4).toDouble(), (podR * 1.6).toDouble(),
                                                    change.position.x.toDouble(), change.position.y.toDouble()
                                                )
                                                if (sectorDir != OctiDir.DIR_UNDEF) {
                                                    hoverSocketDir = sectorDir.index
                                                }
                                            }
                                        } else if (numPods >= 2 && Math.abs(dragAmount.y) > Math.abs(dragAmount.x) * 1.15f) {
                                            verticalSlideAccumPx += dragAmount.y
                                            if (Math.abs(verticalSlideAccumPx) >= stepThresholdPx && sortedPodIndices.isNotEmpty()) {
                                                val curRank = sortedPodIndices.indexOf(selectedPodIndex).let { if (it >= 0) it else 0 }
                                                val step = if (verticalSlideAccumPx < 0f) -1 else 1
                                                val nextRank = ((curRank + step) % sortedPodIndices.size + sortedPodIndices.size) % sortedPodIndices.size
                                                val newPodIdx = sortedPodIndices[nextRank]
                                                selectedPodIndex = newPodIdx
                                                composer.selectStackPodIndex(squareX, squareY, newPodIdx)
                                                selectedRepositionSrcDir = null
                                                if (verticalSlideAccumPx < 0f) {
                                                    verticalSlideAccumPx += stepThresholdPx
                                                } else {
                                                    verticalSlideAccumPx -= stepThresholdPx
                                                }
                                            }
                                        } else if (isFriendlyTurn) {
                                            val wPx = podBoxPxSize.width.toFloat()
                                            val hPx = podBoxPxSize.height.toFloat()
                                            val hitDir = hitTestZoomedSocket(wPx, hPx, change.position.x, change.position.y, rotation)
                                            if (hitDir != OctiDir.DIR_UNDEF) {
                                                hoverSocketDir = hitDir.index
                                            } else {
                                                val cx = wPx * 0.5f
                                                val cy = hPx * 0.5f
                                                val sizeMin = min(wPx, hPx)
                                                val podR = sizeMin * 0.30f
                                                val geo = OctiBoardGeometry(480, 480, 9, 9, rotation)
                                                val sectorDir = geo.hitTestCompassSector(
                                                    cx.toDouble(), cy.toDouble(),
                                                    (podR * 0.4).toDouble(), (podR * 1.6).toDouble(),
                                                    change.position.x.toDouble(), change.position.y.toDouble()
                                                )
                                                if (sectorDir != OctiDir.DIR_UNDEF) {
                                                    hoverSocketDir = sectorDir.index
                                                }
                                            }
                                        }
                                    },
                                    onDragCancel = {
                                        hoverSocketDir = -1
                                        verticalSlideAccumPx = 0f
                                        isRepositionDrag = false
                                    },
                                    onDragEnd = {
                                        verticalSlideAccumPx = 0f
                                        if (isFriendlyTurn && sq != null) {
                                            val pIdx = Math.max(0, Math.min(sq.numPods - 1, selectedPodIndex))
                                            val pod = sq.getPodAt(pIdx)
                                            val dropDir = if (hoverSocketDir in 0 until OctiDir.DIR_MAX) OctiDir.fromIndex(hoverSocketDir) else OctiDir.DIR_UNDEF
                                            if (dropDir != OctiDir.DIR_UNDEF && pod.prongAt(dropDir) == OctiProngType.NO_PRONG) {
                                                if (selectedRepositionSrcDir != null && selectedRepositionSrcDir != dropDir) {
                                                    onRepositionProng(selectedPodIndex, selectedRepositionSrcDir!!, dropDir)
                                                    selectedRepositionSrcDir = null
                                                    hoverSocketDir = -1
                                                } else if (selectedProngTray == "SUPER" && freeSuper > 0) {
                                                    onAddProng(selectedPodIndex, dropDir, true)
                                                    hoverSocketDir = -1
                                                } else if ((selectedProngTray == "NORMAL" || selectedProngTray == null) && freeProngs > 0) {
                                                    onAddProng(selectedPodIndex, dropDir, false)
                                                    hoverSocketDir = -1
                                                }
                                            }
                                        }
                                        hoverSocketDir = -1
                                        isRepositionDrag = false
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
                        val isClaimedPod = composer.isSquarePodClaimed(squareX, squareY, selectedPodIndex)
                        val podProngs = if (selectedPodIndex in 0 until numPods) {
                            sq?.getPodAt(selectedPodIndex)?.numProngs ?: 0
                        } else {
                            0
                        }
                        Button(
                            onClick = {
                                onBeginGuidedMove(selectedPodIndex, moveAllStack)
                            },
                            enabled = !isClaimedPod && podProngs > 0,
                            modifier = Modifier.fillMaxWidth(),
                            colors = ButtonDefaults.buttonColors(
                                containerColor = OctiTheme.colors.accentBrass,
                                contentColor = OctiTheme.colors.windowBg
                            )
                        ) {
                            Text(
                                text = if (isClaimedPod) "Pod Already Moved" else "Move / Jump This Pod",
                                fontWeight = FontWeight.Bold
                            )
                        }
                    }

                    if (canShowPodPlacement) {
                        val canLiberateHere = (capPods > 0) && sq.isOccupiedBy(turn) &&
                            (if (isFriendlyHome) board.canLiberate(turn) else true)
                        if (!isFriendlyHome && canLiberateHere) {
                            Button(
                                onClick = {
                                    onPlaceReservePod(true)
                                },
                                enabled = true,
                                modifier = Modifier.fillMaxWidth()
                            ) {
                                Text("Liberate (*)")
                            }
                        } else if (canLiberateHere) {
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.spacedBy(8.dp)
                            ) {
                                Button(
                                    onClick = {
                                        onPlaceReservePod(false)
                                    },
                                    enabled = freePods > 0,
                                    modifier = Modifier.weight(1f)
                                ) {
                                    Text("Place Reserve (+)")
                                }
                                OutlinedButton(
                                    onClick = {
                                        onPlaceReservePod(true)
                                    },
                                    enabled = true,
                                    modifier = Modifier.weight(1f)
                                ) {
                                    Text("Liberate (*)")
                                }
                            }
                        } else {
                            Button(
                                onClick = {
                                    onPlaceReservePod(false)
                                },
                                enabled = freePods > 0,
                                modifier = Modifier.fillMaxWidth()
                            ) {
                                Text("Place Reserve (+)")
                            }
                        }
                    }
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
fun PlyConfirmDialog(
    stagedNotation: String,
    onCommit: () -> Unit,
    onCancel: () -> Unit,
    subtitleHint: String? = null,
    modifier: Modifier = Modifier
) {
    Card(
        modifier = modifier
            .fillMaxWidth()
            .padding(horizontal = 12.dp, vertical = 6.dp),
        shape = RoundedCornerShape(14.dp),
        colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
        border = BorderStroke(1.5.dp, OctiTheme.colors.accentBrass)
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 14.dp, vertical = 10.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Column(modifier = Modifier.fillMaxWidth()) {
                Text(
                    text = "Staged Ply: $stagedNotation",
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.Bold,
                    color = OctiTheme.colors.legalStep
                )
                if (subtitleHint != null && subtitleHint.isNotEmpty()) {
                    Text(
                        text = subtitleHint,
                        style = MaterialTheme.typography.bodySmall,
                        color = OctiTheme.colors.accentBrass
                    )
                }
            }
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                OutlinedButton(
                    onClick = onCancel,
                    modifier = Modifier.weight(1f),
                    colors = ButtonDefaults.outlinedButtonColors(
                        contentColor = OctiTheme.colors.legalCapture
                    ),
                    border = BorderStroke(1.dp, OctiTheme.colors.legalCapture)
                ) {
                    Text("Cancel", fontWeight = FontWeight.SemiBold)
                }

                Button(
                    onClick = onCommit,
                    modifier = Modifier.weight(1f),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = OctiTheme.colors.accentBrass,
                        contentColor = OctiTheme.colors.windowBg
                    )
                ) {
                    Text("Commit", fontWeight = FontWeight.Bold)
                }
            }
        }
    }
}

@Composable
fun GameOverDialog(
    winner: OctiPlayer,
    matchLabel: String,
    totalPlies: Int,
    resigned: Boolean,
    onNewGame: () -> Unit,
    onReviewBoard: () -> Unit
) {
    Dialog(
        onDismissRequest = onReviewBoard,
        properties = DialogProperties(
            dismissOnBackPress = true,
            dismissOnClickOutside = true,
            usePlatformDefaultWidth = false
        )
    ) {
        Card(
            modifier = Modifier
                .fillMaxWidth(0.90f)
                .padding(16.dp),
            shape = RoundedCornerShape(16.dp),
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
            border = BorderStroke(2.dp, OctiTheme.colors.accentBrass)
        ) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(20.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                Text(
                    text = "VICTORY — ${winner.name} WINS!",
                    style = MaterialTheme.typography.titleLarge,
                    fontWeight = FontWeight.ExtraBold,
                    color = OctiTheme.colors.accentBrass,
                    textAlign = TextAlign.Center
                )
                Text(
                    text = if (resigned) {
                        "Match concluded by resignation after $totalPlies plies"
                    } else {
                        "Match concluded after $totalPlies plies"
                    },
                    style = MaterialTheme.typography.bodyMedium,
                    color = OctiTheme.colors.textPrimary,
                    textAlign = TextAlign.Center
                )
                if (matchLabel.isNotEmpty()) {
                    Text(
                        text = matchLabel,
                        style = MaterialTheme.typography.labelLarge,
                        color = OctiTheme.colors.textSecondary,
                        textAlign = TextAlign.Center
                    )
                }
                Spacer(modifier = Modifier.height(4.dp))
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(10.dp)
                ) {
                    OutlinedButton(
                        onClick = onReviewBoard,
                        modifier = Modifier.weight(1f)
                    ) {
                        Text("Review Board", fontWeight = FontWeight.SemiBold)
                    }
                    Button(
                        onClick = onNewGame,
                        modifier = Modifier.weight(1f),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = OctiTheme.colors.accentBrass,
                            contentColor = OctiTheme.colors.windowBg
                        )
                    ) {
                        Text("New Game", fontWeight = FontWeight.Bold)
                    }
                }
            }
        }
    }
}

@Composable
fun UpToDateModalDialog(
    message: String,
    apkSha256Prefix: String = "dev-local",
    onDismiss: () -> Unit,
    onReinstall: (() -> Unit)? = null
) {
    Dialog(
        onDismissRequest = onDismiss,
        properties = DialogProperties(
            dismissOnBackPress = true,
            dismissOnClickOutside = true,
            usePlatformDefaultWidth = false
        )
    ) {
        Card(
            modifier = Modifier
                .fillMaxWidth(0.86f)
                .padding(16.dp),
            shape = RoundedCornerShape(16.dp),
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
            border = BorderStroke(1.5.dp, OctiTheme.colors.accentBrass)
        ) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(20.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.spacedBy(14.dp)
            ) {
                Text(
                    text = "Check for Updates",
                    style = MaterialTheme.typography.titleLarge,
                    fontWeight = FontWeight.Bold,
                    color = OctiTheme.colors.accentBrass
                )
                Text(
                    text = message,
                    style = MaterialTheme.typography.bodyMedium,
                    color = OctiTheme.colors.textPrimary
                )
                if (apkSha256Prefix.isNotEmpty()) {
                    Text(
                        text = "Installed SHA-256: $apkSha256Prefix",
                        style = MaterialTheme.typography.labelSmall,
                        color = OctiTheme.colors.textSecondary
                    )
                }
                if (onReinstall != null) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        OutlinedButton(
                            onClick = onReinstall,
                            modifier = Modifier.weight(1f)
                        ) {
                            Text("Reinstall", fontWeight = FontWeight.Bold)
                        }
                        Button(
                            onClick = onDismiss,
                            modifier = Modifier.weight(1f),
                            colors = ButtonDefaults.buttonColors(
                                containerColor = OctiTheme.colors.accentBrass,
                                contentColor = OctiTheme.colors.windowBg
                            )
                        ) {
                            Text("OK", fontWeight = FontWeight.Bold)
                        }
                    }
                } else {
                    Button(
                        onClick = onDismiss,
                        modifier = Modifier.fillMaxWidth(0.5f),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = OctiTheme.colors.accentBrass,
                            contentColor = OctiTheme.colors.windowBg
                        )
                    ) {
                        Text("OK", fontWeight = FontWeight.Bold)
                    }
                }
            }
        }
    }
}

@Composable
fun OctiSplashOverlay(
    apkSha256Prefix: String = "dev-local",
    loopIndefinitely: Boolean = false,
    onDismiss: () -> Unit
) {
    androidx.activity.compose.BackHandler(enabled = true) {
        onDismiss()
    }

    var progress by remember { mutableFloatStateOf(0f) }

    LaunchedEffect(loopIndefinitely) {
        val startNanos = withFrameNanos { it }
        val durationNanos = OctiDialogs.SPLASH_DURATION_MS * 1_000_000L
        if (loopIndefinitely) {
            while (true) {
                val nowNanos = withFrameNanos { it }
                val elapsed = nowNanos - startNanos
                progress = ((elapsed % durationNanos).toFloat() / durationNanos.toFloat()).coerceIn(0f, 1f)
            }
        } else {
            while (progress < 1f) {
                val nowNanos = withFrameNanos { it }
                val elapsed = nowNanos - startNanos
                progress = (elapsed.toFloat() / durationNanos.toFloat()).coerceIn(0f, 1f)
                if (progress >= 1f) {
                    break
                }
            }
            onDismiss()
        }
    }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .clickable(
                interactionSource = remember { MutableInteractionSource() },
                indication = null,
                onClick = onDismiss
            ),
        contentAlignment = Alignment.Center
    ) {
        Canvas(modifier = Modifier.fillMaxSize()) {
            val w = size.width
            val h = size.height
            val cx = w * 0.5f
            val cy = h * 0.36f
            val maxR = hypot(w.toDouble(), h.toDouble()).toFloat() * 0.70f

            // 1. Deep Obsidian radial gradient backdrop
            drawRect(
                brush = Brush.radialGradient(
                    colors = listOf(
                        Color(0xFF1E293B),
                        Color(0xFF0F172A),
                        Color(0xFF05070B)
                    ),
                    center = Offset(cx, cy),
                    radius = maxR
                )
            )

            // 2. Rotating outer brass compass ring
            val rOuter = min(w, h) * 0.30f
            withTransform({
                rotate(degrees = progress * 180f, pivot = Offset(cx, cy))
            }) {
                drawCircle(
                    color = Color(0xFFC8A253),
                    radius = rOuter,
                    center = Offset(cx, cy),
                    style = Stroke(width = 2.dp.toPx())
                )
                drawCircle(
                    color = Color(0x66C8A253),
                    radius = rOuter * 1.06f,
                    center = Offset(cx, cy),
                    style = Stroke(width = 1.dp.toPx())
                )
                for (i in 0 until 16) {
                    val angle = (i * 22.5 * Math.PI / 180.0)
                    val cosA = cos(angle).toFloat()
                    val sinA = sin(angle).toFloat()
                    val tLen = if (i % 2 == 0) 14.dp.toPx() else 7.dp.toPx()
                    drawLine(
                        color = if (i % 2 == 0) Color(0xFFFBBF24) else Color(0xFFC8A253),
                        start = Offset(cx + cosA * (rOuter - tLen), cy + sinA * (rOuter - tLen)),
                        end = Offset(cx + cosA * rOuter, cy + sinA * rOuter),
                        strokeWidth = if (i % 2 == 0) 2.dp.toPx() else 1.2.dp.toPx()
                    )
                }
            }

            // 3. Counter-rotating inner brass ring
            val rInner = rOuter * 0.62f
            withTransform({
                rotate(degrees = -progress * 240f, pivot = Offset(cx, cy))
            }) {
                drawCircle(
                    color = Color(0xFFE2C974),
                    radius = rInner,
                    center = Offset(cx, cy),
                    style = Stroke(width = 1.5.dp.toPx())
                )
                for (i in 0 until 8) {
                    val angle = (i * 45.0 * Math.PI / 180.0)
                    val cosA = cos(angle).toFloat()
                    val sinA = sin(angle).toFloat()
                    drawLine(
                        color = Color(0xFFE2C974),
                        start = Offset(cx + cosA * (rInner - 8.dp.toPx()), cy + sinA * (rInner - 8.dp.toPx())),
                        end = Offset(cx + cosA * rInner, cy + sinA * rInner),
                        strokeWidth = 1.4.dp.toPx()
                    )
                }
            }

            // 4. Central octagonal pod geometry
            val podR = min(w, h) * 0.13f
            val geo = OctiBoardGeometry(480, 480, 9, 9, OctiBoardGeometry.BoardRotation.ROT_0)

            // 5. Animated 8-prong beams + golden Superprong shooting in and snapping
            for (d in 0 until OctiDir.DIR_MAX) {
                val dir = OctiDir.fromIndex(d)
                val u = geo.dirUnitVectorScreen(dir)
                val u0 = u[0].toFloat()
                val u1 = u[1].toFloat()
                val isSuper = (d == 0)

                val prongProgress = ((progress - d * 0.07f) / 0.32f).coerceIn(0f, 1f)
                if (prongProgress > 0f) {
                    val startDist = rOuter * 1.35f
                    val endDist = podR * 0.90f
                    val currDist = startDist + (endDist - startDist) * prongProgress
                    val tipDist = currDist + podR * (if (isSuper) 0.50f else 0.38f)

                    val px = -u1
                    val py = u0
                    val halfW = podR * (if (isSuper) 0.12f else 0.08f)

                    val prongPath = Path().apply {
                        moveTo(cx + u0 * currDist - px * halfW, cy + u1 * currDist - py * halfW)
                        lineTo(cx + u0 * (tipDist * 0.85f) - px * (halfW * 0.85f), cy + u1 * (tipDist * 0.85f) - py * (halfW * 0.85f))
                        lineTo(cx + u0 * tipDist, cy + u1 * tipDist)
                        lineTo(cx + u0 * (tipDist * 0.85f) + px * (halfW * 0.85f), cy + u1 * (tipDist * 0.85f) + py * (halfW * 0.85f))
                        lineTo(cx + u0 * currDist + px * halfW, cy + u1 * currDist + py * halfW)
                        close()
                    }

                    if (isSuper) {
                        drawPath(
                            path = prongPath,
                            brush = Brush.linearGradient(
                                colors = listOf(Color(0xFFFEF08A), Color(0xFFFBBF24), Color(0xFFD97706)),
                                start = Offset(cx + u0 * currDist, cy + u1 * currDist),
                                end = Offset(cx + u0 * tipDist, cy + u1 * tipDist)
                            )
                        )
                        drawPath(
                            path = prongPath,
                            color = Color(0xFFFEF08A),
                            style = Stroke(width = 1.5.dp.toPx())
                        )
                    } else {
                        drawPath(
                            path = prongPath,
                            brush = Brush.linearGradient(
                                colors = listOf(Color(0xFFE2E8F0), Color(0xFF94A3B8), Color(0xFF64748B)),
                                start = Offset(cx + u0 * currDist, cy + u1 * currDist),
                                end = Offset(cx + u0 * tipDist, cy + u1 * tipDist)
                            )
                        )
                        drawPath(
                            path = prongPath,
                            color = Color(0xFFCBD5E1),
                            style = Stroke(width = 1.dp.toPx())
                        )
                    }
                }
            }

            // Draw central Emerald (#10B981) & Brass (#C8A253) octagonal pod
            val octoPath = Path()
            for (i in 0 until 8) {
                val angle = (i * 45.0 + 22.5) * Math.PI / 180.0
                val px = cx + cos(angle).toFloat() * podR
                val py = cy + sin(angle).toFloat() * podR
                if (i == 0) octoPath.moveTo(px, py) else octoPath.lineTo(px, py)
            }
            octoPath.close()

            // Outer drop shadow
            drawPath(
                path = octoPath,
                color = Color(0x80000000)
            )

            // Emerald radial gradient fill
            drawPath(
                path = octoPath,
                brush = Brush.radialGradient(
                    colors = listOf(
                        Color(0xFF34D399),
                        Color(0xFF10B981),
                        Color(0xFF064E3B)
                    ),
                    center = Offset(cx - podR * 0.2f, cy - podR * 0.2f),
                    radius = podR * 1.2f
                )
            )

            // Brass perimeter stroke
            drawPath(
                path = octoPath,
                color = Color(0xFFC8A253),
                style = Stroke(width = 3.dp.toPx())
            )

            // Inner core octagon
            val innerOcto = Path()
            for (i in 0 until 8) {
                val angle = (i * 45.0 + 22.5) * Math.PI / 180.0
                val px = cx + cos(angle).toFloat() * (podR * 0.76f)
                val py = cy + sin(angle).toFloat() * (podR * 0.76f)
                if (i == 0) innerOcto.moveTo(px, py) else innerOcto.lineTo(px, py)
            }
            innerOcto.close()
            drawPath(
                path = innerOcto,
                color = Color(0x40FFFFFF),
                style = Stroke(width = 1.2.dp.toPx())
            )

            // Central brass emblem
            drawCircle(
                color = Color(0xFF0F172A),
                radius = podR * 0.22f,
                center = Offset(cx, cy)
            )
            drawCircle(
                color = Color(0xFFC8A253),
                radius = podR * 0.22f,
                center = Offset(cx, cy),
                style = Stroke(width = 1.6.dp.toPx())
            )
        }

        // Overlay Typography
        Column(
            modifier = Modifier
                .fillMaxSize()
                .verticalScroll(rememberScrollState())
                .padding(horizontal = 24.dp, vertical = 16.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.SpaceBetween
        ) {
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                modifier = Modifier.padding(top = 16.dp)
            ) {
                Text(
                    text = "OCTI",
                    style = MaterialTheme.typography.displayMedium,
                    fontWeight = FontWeight.ExtraBold,
                    color = OctiTheme.colors.accentBrass
                )
                Text(
                    text = "TACTICAL POD & PRONG BOARD GAME",
                    style = MaterialTheme.typography.titleSmall,
                    letterSpacing = 2.sp,
                    color = OctiTheme.colors.textSecondary
                )
            }

            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.spacedBy(4.dp),
                modifier = Modifier.padding(bottom = 16.dp)
            ) {
                Text(
                    text = "Author: Charles Chiou",
                    style = MaterialTheme.typography.bodyMedium,
                    color = OctiTheme.colors.textPrimary
                )
                Text(
                    text = "Copyright (C) 2026, Charles Chiou",
                    style = MaterialTheme.typography.bodySmall,
                    color = OctiTheme.colors.textSecondary
                )
                Text(
                    text = "Version ${OctiDefs.OCTI_VERSION_STRING}  •  SHA-256: $apkSha256Prefix",
                    style = MaterialTheme.typography.labelMedium,
                    color = OctiTheme.colors.accentBrass,
                    fontWeight = FontWeight.Bold
                )
                Text(
                    text = "Public Game Archive: CC-BY 4.0 (Octi game data, Charles Chiou, selfso.com)",
                    style = MaterialTheme.typography.bodySmall,
                    color = OctiTheme.colors.textSecondary,
                    textAlign = TextAlign.Center
                )
                Spacer(modifier = Modifier.height(10.dp))
                Text(
                    text = "Tap anywhere or press ESC / Back to skip",
                    style = MaterialTheme.typography.labelSmall,
                    color = OctiTheme.colors.textSecondary
                )
            }
        }
    }
}

@Composable
fun ExportMovesDialog(
    movesText: String,
    onCopy: () -> Unit,
    onShare: () -> Unit,
    onDismiss: () -> Unit
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        containerColor = OctiTheme.colors.panelCardBg,
        titleContentColor = OctiTheme.colors.accentBrass,
        textContentColor = OctiTheme.colors.textPrimary,
        title = { Text("Export OCTI Game Notation") },
        text = {
            Column(
                modifier = Modifier.fillMaxWidth(),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Text(
                    text = "Canonical OCTI Transcript (JSON v3.0):",
                    style = MaterialTheme.typography.bodySmall,
                    color = OctiTheme.colors.textSecondary
                )
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(240.dp)
                        .background(
                            OctiTheme.colors.windowBg,
                            shape = RoundedCornerShape(6.dp)
                        )
                        .padding(8.dp)
                        .verticalScroll(rememberScrollState())
                ) {
                    Text(
                        text = movesText.ifEmpty { "(No moves yet)" },
                        style = MaterialTheme.typography.bodyMedium.copy(
                            fontFamily = androidx.compose.ui.text.font.FontFamily.Monospace,
                            fontSize = 12.sp
                        ),
                        color = OctiTheme.colors.textPrimary
                    )
                }
            }
        },
        confirmButton = {
            Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                Button(
                    onClick = onCopy,
                    colors = ButtonDefaults.buttonColors(
                        containerColor = OctiTheme.colors.accentBrass,
                        contentColor = OctiTheme.colors.windowBg
                    )
                ) {
                    Text("Copy Transcript", fontWeight = FontWeight.Bold)
                }
                Button(
                    onClick = onShare,
                    colors = ButtonDefaults.buttonColors(
                        containerColor = OctiTheme.colors.accentBrass,
                        contentColor = OctiTheme.colors.windowBg
                    )
                ) {
                    Text("Share...", fontWeight = FontWeight.Bold)
                }
            }
        },
        dismissButton = {
            OutlinedButton(onClick = onDismiss) {
                Text("Close")
            }
        }
    )
}

@Composable
fun OctiThinkingDialog(
    level: Int,
    progress: Float,
    remainingSec: Float,
    depth: Int = 0,
    nodes: Long = 0L,
    budgetMs: Int = 0,
    modifier: Modifier = Modifier
) {
    val clampedProgress = progress.coerceIn(0f, 1f)
    val totalBudgetSec = if (budgetMs > 0) {
        budgetMs / 1000f
    } else {
        max(1.0f, remainingSec / max(0.001f, 1f - clampedProgress))
    }
    val elapsedSec = max(0f, totalBudgetSec - remainingSec)
    val elapsedMs = (elapsedSec * 1000f).toInt()
    val effBudgetMs = (totalBudgetSec * 1000f).toInt()
    val overlayText = OctiBoardRenderer.formatThinkingProgressOverlay(depth, nodes, elapsedMs, effBudgetMs)

    Box(
        modifier = modifier
            .fillMaxSize()
            .clickable(
                interactionSource = remember { MutableInteractionSource() },
                indication = null,
                onClick = {}
            )
            .padding(bottom = 16.dp, start = 16.dp, end = 16.dp),
        contentAlignment = Alignment.BottomCenter
    ) {
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 8.dp),
            shape = RoundedCornerShape(12.dp),
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
            border = BorderStroke(1.5.dp, OctiTheme.colors.accentBrass)
        ) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 16.dp, vertical = 12.dp),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Text(
                    text = "AI Thinking (Level $level)",
                    style = MaterialTheme.typography.titleSmall,
                    fontWeight = FontWeight.Bold,
                    color = OctiTheme.colors.accentBrass
                )
                Box(
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(26.dp)
                        .background(
                            color = Color(0xFF14181E),
                            shape = RoundedCornerShape(6.dp)
                        )
                        .border(
                            width = 1.dp,
                            color = Color(0xFF3B4252),
                            shape = RoundedCornerShape(6.dp)
                        )
                ) {
                    if (clampedProgress > 0.001f) {
                        Box(
                            modifier = Modifier
                                .fillMaxWidth(fraction = clampedProgress)
                                .fillMaxSize()
                                .background(
                                    brush = Brush.horizontalGradient(
                                        listOf(
                                            Color(0xFFB8860B),
                                            Color(0xFFFFD700),
                                            Color(0xFFDAA520)
                                        )
                                    ),
                                    shape = RoundedCornerShape(6.dp)
                                )
                        )
                    }
                    Text(
                        text = overlayText,
                        style = MaterialTheme.typography.labelMedium.copy(
                            fontSize = 11.sp,
                            fontWeight = FontWeight.Bold,
                            color = Color.White
                        ),
                        textAlign = TextAlign.Center,
                        maxLines = 1,
                        modifier = Modifier.align(Alignment.Center)
                    )
                }
            }
        }
    }
}

@Composable
fun OctiTelemetryConsentDialog(
    onChoice: (String) -> Unit
) {
    androidx.activity.compose.BackHandler(enabled = true) {
        // Non-dismissible: ignore back press until choice is made
    }

    var showDetails by remember { mutableStateOf(false) }

    val sampleJsonPreview = remember {
        """{
  "client": {
    "extended": {
      "startedAt": 1730250000,
      "endedAt": 1730250120,
      "utcOffsetMin": 480,
      "locale": "en-US",
      "osVersion": "Android 14",
      "installId": "3fa85f64-5717-4562-b3fc-2c963f66afa6"
    }
  }
}"""
    }

    Dialog(
        onDismissRequest = {},
        properties = DialogProperties(
            dismissOnBackPress = false,
            dismissOnClickOutside = false
        )
    ) {
        Card(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            shape = RoundedCornerShape(12.dp),
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
            border = BorderStroke(1.5.dp, OctiTheme.colors.accentBrass)
        ) {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(20.dp)
                    .verticalScroll(rememberScrollState()),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                Text(
                    text = "Help Improve OCTI AI",
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.Bold,
                    color = OctiTheme.colors.accentBrass
                )

                Text(
                    text = "When you complete games against AI bots, OCTI can automatically upload anonymized transcripts to help train and refine bot heuristics.\n\n" +
                            "You can choose whether to include extra contextual telemetry (match duration, regional locale, OS version, and an anonymous install ID) with uploads.",
                    style = MaterialTheme.typography.bodyMedium,
                    color = OctiTheme.colors.textPrimary
                )

                Text(
                    text = "Public Game Archive License:\nCreative Commons Attribution 4.0 International (CC-BY 4.0)\nOcti game data, Charles Chiou (selfso.com)",
                    style = MaterialTheme.typography.bodySmall,
                    color = OctiTheme.colors.textSecondary
                )

                Row(
                    modifier = Modifier
                        .clickable { showDetails = !showDetails }
                        .padding(vertical = 4.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = if (showDetails) "▾ Hide what will be sent" else "▸ View what will be sent",
                        style = MaterialTheme.typography.labelMedium,
                        fontWeight = FontWeight.Bold,
                        color = OctiTheme.colors.accentBrass
                    )
                }

                if (showDetails) {
                    Box(
                        modifier = Modifier
                            .fillMaxWidth()
                            .background(OctiTheme.colors.windowBg, RoundedCornerShape(6.dp))
                            .padding(8.dp)
                    ) {
                        Text(
                            text = sampleJsonPreview,
                            style = MaterialTheme.typography.bodySmall.copy(
                                fontFamily = androidx.compose.ui.text.font.FontFamily.Monospace,
                                fontSize = 11.sp
                            ),
                            color = OctiTheme.colors.textSecondary
                        )
                    }
                }

                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(top = 8.dp),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    OutlinedButton(
                        modifier = Modifier.weight(1f),
                        onClick = { onChoice("decline") }
                    ) {
                        Text("Don't share", maxLines = 1)
                    }
                    Button(
                        modifier = Modifier.weight(1f),
                        onClick = { onChoice("share") },
                        colors = ButtonDefaults.buttonColors(
                            containerColor = OctiTheme.colors.accentBrass,
                            contentColor = OctiTheme.colors.windowBg
                        )
                    ) {
                        Text("Share extra info", fontWeight = FontWeight.Bold, maxLines = 1)
                    }
                }
            }
        }
    }
}

@Composable
fun OctiTelemetrySettingsDialog(
    currentConsent: String,
    installId: String,
    onToggleShare: (Boolean) -> Unit,
    onResetInstallId: () -> Unit,
    onDismiss: () -> Unit
) {
    val isSharing = (currentConsent == "share")

    AlertDialog(
        onDismissRequest = onDismiss,
        containerColor = OctiTheme.colors.panelCardBg,
        titleContentColor = OctiTheme.colors.accentBrass,
        textContentColor = OctiTheme.colors.textPrimary,
        title = { Text("Telemetry & Privacy") },
        text = {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .verticalScroll(rememberScrollState()),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                Text(
                    text = "OCTI automatically shares completed games against AI bots with the public research dataset to improve bot play.",
                    style = MaterialTheme.typography.bodySmall,
                    color = OctiTheme.colors.textPrimary
                )

                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .clickable { onToggleShare(!isSharing) }
                        .padding(vertical = 4.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Checkbox(
                        checked = isSharing,
                        onCheckedChange = { onToggleShare(it) }
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                    Text(
                        text = "Include extra info (time, region, install ID) with uploaded games",
                        style = MaterialTheme.typography.bodyMedium,
                        color = OctiTheme.colors.textPrimary
                    )
                }

                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .background(OctiTheme.colors.windowBg, RoundedCornerShape(6.dp))
                        .padding(8.dp),
                    verticalArrangement = Arrangement.spacedBy(4.dp)
                ) {
                    Text(
                        text = "Anonymous Install ID:",
                        style = MaterialTheme.typography.labelSmall,
                        color = OctiTheme.colors.textSecondary
                    )
                    Text(
                        text = installId.ifEmpty { "(None)" },
                        style = MaterialTheme.typography.bodySmall.copy(
                            fontFamily = androidx.compose.ui.text.font.FontFamily.Monospace,
                            fontSize = 11.sp
                        ),
                        color = OctiTheme.colors.accentBrass
                    )
                }

                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.End
                ) {
                    OutlinedButton(
                        onClick = onResetInstallId
                    ) {
                        Text("Reset Install ID")
                    }
                }

                Text(
                    text = "Public Dataset License:\nCreative Commons Attribution 4.0 (CC-BY 4.0)\nOcti game data, Charles Chiou (selfso.com)",
                    style = MaterialTheme.typography.labelSmall,
                    color = OctiTheme.colors.textSecondary
                )
            }
        },
        confirmButton = {
            Button(
                onClick = onDismiss,
                colors = ButtonDefaults.buttonColors(
                    containerColor = OctiTheme.colors.accentBrass,
                    contentColor = OctiTheme.colors.windowBg
                )
            ) {
                Text("Close", fontWeight = FontWeight.Bold)
            }
        }
    )
}

/**
 * Centered borderless modal card overlay prompting "Start Match"
 * before the game clock, turn indicators, or AI opening move starts.
 */
@Composable
fun StartMatchOverlayDialog(
    humanSeat: OctiPlayer = OctiPlayer.PLAYER_A,
    vsBot: Boolean = true,
    difficulty: OctiBotEngine.BotDifficulty = OctiBotEngine.BotDifficulty.TACTICAL,
    onStartMatch: () -> Unit
) {
    Card(
        modifier = Modifier
            .width(260.dp)
            .padding(8.dp),
        shape = RoundedCornerShape(12.dp),
        colors = CardDefaults.cardColors(
            containerColor = Color(0xF0161D26)
        ),
        border = BorderStroke(2.dp, Color(0xFF2E4057)),
        elevation = CardDefaults.cardElevation(defaultElevation = 8.dp)
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 20.dp, vertical = 16.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(10.dp)
        ) {
            Text(
                text = "Ready to Play?",
                color = Color.White,
                fontWeight = FontWeight.Bold,
                fontSize = 15.sp,
                textAlign = TextAlign.Center
            )
            val subtitle = if (vsBot) {
                if (humanSeat == OctiPlayer.PLAYER_A) "Playing as Player A (Red)"
                else "Playing as Player B (Green)"
            } else {
                "Local Pass-and-Play"
            }
            Text(
                text = subtitle,
                color = Color(0xFF8B9BB4),
                fontSize = 12.sp,
                textAlign = TextAlign.Center
            )
            Button(
                onClick = onStartMatch,
                colors = ButtonDefaults.buttonColors(
                    containerColor = Color(0xFFD4AF37),
                    contentColor = Color(0xFF161D26)
                ),
                shape = RoundedCornerShape(6.dp),
                modifier = Modifier.fillMaxWidth()
            ) {
                Text(
                    text = "Start Match",
                    fontWeight = FontWeight.Bold,
                    fontSize = 14.sp
                )
            }
        }
    }
}

@Composable
fun StartMatchOverlayDialog(
    humanSeat: OctiPlayer,
    onStartMatch: () -> Unit
) {
    StartMatchOverlayDialog(
        humanSeat = humanSeat,
        vsBot = true,
        difficulty = OctiBotEngine.BotDifficulty.TACTICAL,
        onStartMatch = onStartMatch
    )
}

/*
 * Local variables:
 * mode: Kotlin
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
