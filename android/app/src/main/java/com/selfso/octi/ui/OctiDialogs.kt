/*
 * OctiDialogs.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import android.content.Intent
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
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
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.imePadding
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Checkbox
import androidx.compose.material3.FilterChip
import androidx.compose.material3.IconButton
import androidx.compose.material3.Surface
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
import kotlinx.coroutines.delay
import kotlin.math.cos
import kotlin.math.hypot
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sin

object OctiDialogs {
    const val SPLASH_DURATION_MS: Long = 10000L
    const val PLACE_RESERVE_POD_LABEL = "Place Reserve Pod"
    const val CHANGE_TYPE_BUTTON_LABEL = "Change Type"
    const val START_MATCH_BUTTON_LABEL = "Start Match"
    const val START_MATCH_CARD_MIN_WIDTH_DP = 300
    const val START_MATCH_CARD_MAX_WIDTH_DP = 360
    const val START_MATCH_BUTTON_HORIZONTAL_PADDING_DP = 10

    fun describeTerminationReason(board: OctiBoard?): String {
        if (board == null || board.game == null) {
            return "Match Concluded"
        }
        val game = board.game
        if (game.isGameResigned) {
            return "Victory by Resignation"
        }
        val term = game.deduceTermination()
        return when (term) {
            "RESIGNATION" -> "Victory by Resignation"
            "BASE_OCCUPATION" -> "Captured Enemy Home Base"
            "TOTAL_CAPTURE" -> "Eliminated All Opponent Pods"
            "STALEMATE" -> "Victory by Stalemate (Opponent Has No Legal Moves)"
            else -> "Match Concluded"
        }
    }

    @JvmField
    val CANONICAL_SSO_PROVIDERS: List<Pair<String, String>> = listOf(
        "google" to "Google"
    )

    @JvmField
    val ALL_SSO_PROVIDERS: List<Pair<String, String>> = listOf(
        "google" to "Google",
        "facebook" to "Facebook"
    )

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
            (podR * 0.35f).toDouble(), (podR * 2.4f).toDouble(),
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

val CANONICAL_SSO_PROVIDERS: List<Pair<String, String>> = OctiDialogs.CANONICAL_SSO_PROVIDERS

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
                                OctiBotEngine.BotDifficulty.CASUAL -> "Lv1 (5s)"
                                OctiBotEngine.BotDifficulty.TACTICAL -> "Lv2 (10s)"
                                OctiBotEngine.BotDifficulty.MASTER -> "Lv3 (20s)"
                                OctiBotEngine.BotDifficulty.EXPERT -> "Lv4 (40s)"
                                OctiBotEngine.BotDifficulty.GRANDMASTER -> "Lv5 (60s)"
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
    val context = LocalContext.current
    val onboardingRequired by viewModel.onboardingRequired.collectAsState()
    val currentProfile by viewModel.currentProfile.collectAsState()
    val isBrowserAuthInProgress by viewModel.isBrowserAuthInProgress.collectAsState()
    val browserAuthStatusText by viewModel.browserAuthStatusText.collectAsState()
    val profileActionBusy by viewModel.profileActionBusy.collectAsState()
    val profileActionMessage by viewModel.profileActionMessage.collectAsState()
    var desiredHandle by remember(currentProfile?.handle) {
        mutableStateOf(currentProfile?.handle ?: "")
    }

    AlertDialog(
        onDismissRequest = {
            viewModel.cancelBrowserAuth()
            viewModel.clearProfileActionMessage()
            onDismiss()
        },
        containerColor = OctiTheme.colors.panelCardBg,
        titleContentColor = OctiTheme.colors.accentBrass,
        textContentColor = OctiTheme.colors.textPrimary,
        title = {
            Text(
                if (onboardingRequired) {
                    "Welcome to OCTI!"
                } else {
                    "Sign In to OCTI"
                }
            )
        },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                if (onboardingRequired) {
                    Text(
                        text = "Choose the player name others will see:",
                        style = MaterialTheme.typography.bodyMedium,
                        color = OctiTheme.colors.textSecondary
                    )
                    OutlinedTextField(
                        value = desiredHandle,
                        onValueChange = { desiredHandle = it },
                        label = { Text("Player Name") },
                        singleLine = true,
                        modifier = Modifier.fillMaxWidth()
                    )
                    val statusText = profileActionMessage.ifEmpty { browserAuthStatusText }
                    if (statusText.isNotEmpty()) {
                        Text(
                            text = statusText,
                            style = MaterialTheme.typography.bodySmall,
                            color = if (statusText.contains("already taken") ||
                                statusText.contains("must be") ||
                                statusText.contains("Failed")
                            ) OctiTheme.colors.legalCapture else OctiTheme.colors.textSecondary
                        )
                    }
                } else {
                    Text(
                        text = "Sign in to play online, chat with players, and track your rating.",
                        style = MaterialTheme.typography.bodyMedium,
                        color = OctiTheme.colors.textSecondary
                    )
                    CANONICAL_SSO_PROVIDERS.forEach { (id, label) ->
                        Button(
                            onClick = {
                                viewModel.startBrowserSignIn(id) { url ->
                                    try {
                                        val intent = Intent(Intent.ACTION_VIEW, Uri.parse(url))
                                        context.startActivity(intent)
                                    } catch (_: Throwable) {
                                    }
                                }
                            },
                            enabled = !isBrowserAuthInProgress,
                            modifier = Modifier
                                .fillMaxWidth()
                                .height(48.dp),
                            colors = ButtonDefaults.buttonColors(
                                containerColor = OctiTheme.colors.accentBrass,
                                contentColor = OctiTheme.colors.windowBg
                            )
                        ) {
                            Text(
                                text = "Continue with $label",
                                fontWeight = FontWeight.Bold
                            )
                        }
                    }
                    if (browserAuthStatusText.isNotEmpty()) {
                        Text(
                            text = browserAuthStatusText,
                            style = MaterialTheme.typography.bodySmall,
                            color = OctiTheme.colors.textSecondary
                        )
                    }
                }
            }
        },
        confirmButton = {
            if (onboardingRequired) {
                Button(
                    onClick = {
                        viewModel.completeHandleOnboardingAsync(desiredHandle.trim()) { res ->
                            if (res.ok) {
                                viewModel.clearProfileActionMessage()
                                onDismiss()
                            }
                        }
                    },
                    enabled = !profileActionBusy && desiredHandle.trim().length >= 3,
                    colors = ButtonDefaults.buttonColors(
                        containerColor = OctiTheme.colors.accentBrass,
                        contentColor = OctiTheme.colors.windowBg
                    )
                ) {
                    Text(if (profileActionBusy) "Saving..." else "Continue")
                }
            }
        },
        dismissButton = {
            OutlinedButton(
                onClick = {
                    viewModel.cancelBrowserAuth()
                    viewModel.clearProfileActionMessage()
                    onDismiss()
                }
            ) {
                Text(if (isBrowserAuthInProgress) "Cancel" else "Close")
            }
        }
    )
}

@Composable
fun OctiPlayerAvatar(
    avatarUrl: String,
    handle: String = "",
    size: androidx.compose.ui.unit.Dp,
    viewModel: OctiAppViewModel? = null,
    modifier: Modifier = Modifier,
    netClient: OctiNetworkClient? = viewModel?.netClient,
    label: String = handle
) {
    var avatarBitmap by remember(avatarUrl) { mutableStateOf<ImageBitmap?>(null) }

    LaunchedEffect(avatarUrl) {
        if (avatarUrl.isEmpty()) {
            avatarBitmap = null
            return@LaunchedEffect
        }
        withContext(Dispatchers.IO) {
            val bytes = OctiAppViewModel.decodeAvatarDataUriOrBytes(avatarUrl, netClient ?: viewModel?.netClient)
            if (bytes != null && bytes.isNotEmpty()) {
                try {
                    val bmp = BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
                    if (bmp != null) {
                        avatarBitmap = bmp.asImageBitmap()
                    } else {
                        avatarBitmap = null
                    }
                } catch (_: Throwable) {
                    avatarBitmap = null
                }
            } else {
                avatarBitmap = null
            }
        }
    }

    val displayHandle = label.ifEmpty { handle }
    val initialLetter = remember(displayHandle) {
        displayHandle.trim().takeIf { it.isNotEmpty() }?.first()?.uppercaseChar()?.toString() ?: "?"
    }

    val shape = CircleShape
    val bmp = avatarBitmap

    Box(
        modifier = modifier
            .size(size)
            .border(1.5.dp, OctiTheme.colors.accentBrass, shape)
            .clip(shape)
            .background(OctiTheme.colors.panelCardBg),
        contentAlignment = Alignment.Center
    ) {
        if (bmp != null) {
            Image(
                bitmap = bmp,
                contentDescription = "Avatar of $displayHandle",
                contentScale = ContentScale.Crop,
                modifier = Modifier.fillMaxSize()
            )
        } else {
            Text(
                text = initialLetter,
                style = MaterialTheme.typography.titleMedium.copy(
                    fontSize = (size.value * 0.45f).sp,
                    fontWeight = FontWeight.Bold,
                    color = OctiTheme.colors.accentBrass
                )
            )
        }
    }
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun AccountIdentitiesDialog(
    viewModel: OctiAppViewModel,
    onDismiss: () -> Unit,
    onMergeConflict: () -> Unit = { viewModel.mergeConflictingIdentityIntoCurrentAccount() }
) {
    val context = LocalContext.current
    val profile by viewModel.currentProfile.collectAsState()
    val identities by viewModel.linkedIdentities.collectAsState()
    val conflictProfile by viewModel.linkConflictProfile.collectAsState()
    val isBrowserAuthInProgress by viewModel.isBrowserAuthInProgress.collectAsState()
    val browserAuthStatusText by viewModel.browserAuthStatusText.collectAsState()
    val profileActionBusy by viewModel.profileActionBusy.collectAsState()
    val profileActionMessage by viewModel.profileActionMessage.collectAsState()

    var fieldsDirty by remember { mutableStateOf(false) }
    var savingProfile by remember { mutableStateOf(false) }
    var avatarChanged by remember { mutableStateOf(false) }
    var editableHandle by remember { mutableStateOf(profile?.handle ?: "") }
    var editableDisplayName by remember { mutableStateOf(profile?.displayName ?: "") }
    var editableLocation by remember { mutableStateOf(profile?.location ?: "") }
    var editableBio by remember { mutableStateOf(profile?.bio ?: "") }
    var pendingAvatarUrl by remember { mutableStateOf(profile?.avatarUrl ?: "") }
    var avatarInlineFeedback by remember { mutableStateOf("") }
    var statusMessage by remember { mutableStateOf("") }
    var showClearAvatarConfirm by remember { mutableStateOf(false) }
    var showSaveProfileConfirm by remember { mutableStateOf(false) }
    var showDiscardChangesConfirm by remember { mutableStateOf(false) }

    val prof = profile
    LaunchedEffect(prof?.handle, prof?.displayName, prof?.location, prof?.bio, prof?.avatarUrl) {
        if (!fieldsDirty && !savingProfile && prof != null) {
            editableHandle = prof.handle
            editableDisplayName = prof.displayName
            editableLocation = prof.location
            editableBio = prof.bio
            if (!avatarChanged) {
                pendingAvatarUrl = prof.avatarUrl
            }
        }
    }

    val imagePickerLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.GetContent()
    ) { uri: Uri? ->
        if (uri != null) {
            try {
                context.contentResolver.openInputStream(uri)?.use { stream ->
                    val bmp = BitmapFactory.decodeStream(stream)
                    if (bmp != null) {
                        val encoded = OctiAppViewModel.encodeAvatarBitmapToDataUri(bmp)
                        if (encoded.isNotEmpty()) {
                            pendingAvatarUrl = encoded
                            avatarChanged = true
                            fieldsDirty = true
                            viewModel.clearProfileActionMessage()
                            avatarInlineFeedback = "Avatar selected (auto-cropped square)."
                        } else {
                            avatarInlineFeedback = "Could not compress image under size limit."
                        }
                    } else {
                        avatarInlineFeedback = "Could not decode selected image."
                    }
                }
            } catch (e: Throwable) {
                avatarInlineFeedback = "Failed to open image: ${e.message}"
            }
        }
    }

    val hasUnsavedChanges = prof != null && (
        avatarChanged ||
        editableHandle.trim() != prof.handle ||
        editableDisplayName.trim() != prof.displayName ||
        editableLocation.trim() != prof.location ||
        editableBio.trim() != prof.bio
    )

    AlertDialog(
        onDismissRequest = {
            if (!savingProfile) {
                viewModel.cancelBrowserAuth()
                viewModel.clearProfileActionMessage()
                onDismiss()
            }
        },
        containerColor = OctiTheme.colors.panelCardBg,
        titleContentColor = OctiTheme.colors.accentBrass,
        textContentColor = OctiTheme.colors.textPrimary,
        title = { Text("My Account") },
        text = {
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .verticalScroll(rememberScrollState()),
                verticalArrangement = Arrangement.spacedBy(10.dp)
            ) {
                val p = profile
                if (p != null) {
                    Text("Profile & Avatar", fontWeight = FontWeight.Bold, color = OctiTheme.colors.accentBrass)

                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(16.dp)
                    ) {
                        OctiPlayerAvatar(
                            avatarUrl = pendingAvatarUrl,
                            handle = editableHandle.ifEmpty { p.handle },
                            size = 72.dp,
                            viewModel = viewModel
                        )
                        Column(
                            verticalArrangement = Arrangement.spacedBy(6.dp),
                            modifier = Modifier.weight(1f)
                        ) {
                            Button(
                                onClick = { imagePickerLauncher.launch("image/*") },
                                enabled = !profileActionBusy,
                                colors = ButtonDefaults.buttonColors(
                                    containerColor = OctiTheme.colors.accentBrass,
                                    contentColor = OctiTheme.colors.windowBg
                                ),
                                modifier = Modifier.fillMaxWidth()
                            ) {
                                Text("Upload Avatar")
                            }
                            OutlinedButton(
                                onClick = {
                                    showClearAvatarConfirm = true
                                },
                                enabled = !profileActionBusy && !savingProfile && (pendingAvatarUrl.isNotEmpty() || p.avatarUrl.isNotEmpty()),
                                modifier = Modifier.fillMaxWidth()
                            ) {
                                Text("Clear Avatar")
                            }
                        }
                    }

                    if (avatarInlineFeedback.isNotEmpty()) {
                        Text(
                            text = avatarInlineFeedback,
                            style = MaterialTheme.typography.bodySmall,
                            color = OctiTheme.colors.accentBrass
                        )
                    }

                    OutlinedTextField(
                        value = editableHandle,
                        onValueChange = {
                            editableHandle = it
                            fieldsDirty = true
                            viewModel.clearProfileActionMessage()
                        },
                        label = { Text("Player Handle") },
                        supportingText = { Text("3-32 chars • 1 handle change per 24h") },
                        singleLine = true,
                        modifier = Modifier.fillMaxWidth()
                    )

                    OutlinedTextField(
                        value = editableDisplayName,
                        onValueChange = {
                            editableDisplayName = it
                            fieldsDirty = true
                            viewModel.clearProfileActionMessage()
                        },
                        label = { Text("Display Name") },
                        singleLine = true,
                        modifier = Modifier.fillMaxWidth()
                    )

                    OutlinedTextField(
                        value = editableLocation,
                        onValueChange = {
                            editableLocation = it
                            fieldsDirty = true
                            viewModel.clearProfileActionMessage()
                        },
                        label = { Text("Location") },
                        singleLine = true,
                        modifier = Modifier.fillMaxWidth()
                    )

                    OutlinedTextField(
                        value = editableBio,
                        onValueChange = {
                            editableBio = it
                            fieldsDirty = true
                            viewModel.clearProfileActionMessage()
                        },
                        label = { Text("Bio") },
                        minLines = 2,
                        maxLines = 3,
                        modifier = Modifier.fillMaxWidth()
                    )

                    Button(
                        onClick = {
                            showSaveProfileConfirm = true
                        },
                        enabled = !profileActionBusy && !savingProfile && hasUnsavedChanges && editableHandle.trim().isNotEmpty(),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = OctiTheme.colors.accentBrass,
                            contentColor = OctiTheme.colors.windowBg
                        ),
                        modifier = Modifier.fillMaxWidth()
                    ) {
                        Text(if (profileActionBusy || savingProfile) "Saving Profile..." else "Save Profile")
                    }

                    if (profileActionMessage.isNotEmpty()) {
                        Text(
                            text = profileActionMessage,
                            style = MaterialTheme.typography.bodySmall,
                            color = if (profileActionMessage.startsWith("Profile saved") || profileActionMessage.startsWith("Handle updated"))
                                OctiTheme.colors.accentBrass else OctiTheme.colors.legalCapture
                        )
                    }

                    Text(
                        text = "Rating — 2P: ${p.elo2p}  •  4P: ${p.elo4p}  •  OFK: ${p.eloOfk}",
                        style = MaterialTheme.typography.bodyMedium,
                        color = OctiTheme.colors.textSecondary
                    )
                }

                Spacer(modifier = Modifier.height(4.dp))
                Text("Connected Accounts", fontWeight = FontWeight.SemiBold)
                CANONICAL_SSO_PROVIDERS.forEach { (provId, provLabel) ->
                    val linked = identities.firstOrNull {
                        it.provider.equals(provId, ignoreCase = true)
                    }
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        if (linked != null) {
                            val detail = linked.providerEmail.ifEmpty { linked.displayHint }
                            Text(
                                text = if (detail.isNotEmpty()) "$provLabel • $detail" else "$provLabel (Connected)",
                                modifier = Modifier.weight(1f)
                            )
                            OutlinedButton(
                                onClick = {
                                    viewModel.unlinkIdentityAsync(linked.provider) { res ->
                                        statusMessage = if (res.ok) "Unlinked $provLabel." else "Cannot unlink: ${res.error}"
                                    }
                                },
                                enabled = !profileActionBusy && identities.size > 1
                            ) {
                                Text("Disconnect")
                            }
                        } else {
                            Text(
                                text = provLabel,
                                modifier = Modifier.weight(1f),
                                color = OctiTheme.colors.textSecondary
                            )
                            Button(
                                onClick = {
                                    viewModel.startBrowserLink(provId) { url ->
                                        try {
                                            val intent = Intent(Intent.ACTION_VIEW, Uri.parse(url))
                                            context.startActivity(intent)
                                        } catch (_: Throwable) {
                                        }
                                    }
                                },
                                enabled = !isBrowserAuthInProgress,
                                colors = ButtonDefaults.buttonColors(
                                    containerColor = OctiTheme.colors.accentBrass,
                                    contentColor = OctiTheme.colors.windowBg
                                )
                            ) {
                                Text("Connect $provLabel")
                            }
                        }
                    }
                }

                if (browserAuthStatusText.isNotEmpty()) {
                    Text(
                        text = browserAuthStatusText,
                        style = MaterialTheme.typography.bodySmall,
                        color = OctiTheme.colors.textSecondary
                    )
                }

                if (statusMessage.isNotEmpty()) {
                    Text(
                        text = statusMessage,
                        style = MaterialTheme.typography.bodySmall,
                        color = OctiTheme.colors.textSecondary
                    )
                }

                if (conflictProfile != null) {
                    Spacer(modifier = Modifier.height(6.dp))
                    Text(
                        text = "This account is already connected to @${conflictProfile?.handle}.",
                        color = OctiTheme.colors.legalCapture,
                        fontWeight = FontWeight.Bold
                    )
                    FlowRow(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(8.dp),
                        verticalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        Button(
                            onClick = {
                                viewModel.confirmSwitchToConflictAccountAsync { ok ->
                                    statusMessage = if (ok) "Switched to existing account." else "Account switch failed."
                                }
                            },
                            enabled = !profileActionBusy,
                            colors = ButtonDefaults.buttonColors(
                                containerColor = OctiTheme.colors.accentBrass,
                                contentColor = OctiTheme.colors.windowBg
                            )
                        ) {
                            Text("Switch to @${conflictProfile?.handle}")
                        }
                        Button(
                            onClick = { onMergeConflict() },
                            colors = ButtonDefaults.buttonColors(
                                containerColor = OctiTheme.colors.accentBrass,
                                contentColor = OctiTheme.colors.windowBg
                            )
                        ) {
                            Text("Combine Accounts")
                        }
                        OutlinedButton(onClick = { viewModel.dismissLinkConflict() }) {
                            Text("Cancel")
                        }
                    }
                }
            }
        },
        confirmButton = {
            OutlinedButton(
                onClick = {
                    viewModel.logoutOnlineAsync {
                        onDismiss()
                    }
                },
                enabled = !profileActionBusy && !savingProfile
            ) {
                Text("Sign Out")
            }
        },
        dismissButton = {
            OutlinedButton(
                onClick = {
                    if (hasUnsavedChanges) {
                        showDiscardChangesConfirm = true
                    } else {
                        viewModel.cancelBrowserAuth()
                        viewModel.clearProfileActionMessage()
                        onDismiss()
                    }
                },
                enabled = !savingProfile && !profileActionBusy
            ) {
                Text(if (savingProfile) "Saving..." else "Done")
            }
        }
    )

    if (showClearAvatarConfirm) {
        AlertDialog(
            onDismissRequest = { showClearAvatarConfirm = false },
            containerColor = OctiTheme.colors.panelCardBg,
            titleContentColor = OctiTheme.colors.accentBrass,
            textContentColor = OctiTheme.colors.textPrimary,
            title = { Text("Clear Avatar?") },
            text = {
                Text("Are you sure you want to remove your profile avatar? You will still need to save your profile to apply.")
            },
            confirmButton = {
                Button(
                    onClick = {
                        showClearAvatarConfirm = false
                        pendingAvatarUrl = ""
                        avatarChanged = true
                        fieldsDirty = true
                        viewModel.clearProfileActionMessage()
                        avatarInlineFeedback = "Avatar cleared (save to commit)."
                    },
                    colors = ButtonDefaults.buttonColors(
                        containerColor = OctiTheme.colors.accentBrass,
                        contentColor = OctiTheme.colors.windowBg
                    )
                ) {
                    Text("Clear Avatar")
                }
            },
            dismissButton = {
                OutlinedButton(onClick = { showClearAvatarConfirm = false }) {
                    Text("Cancel")
                }
            }
        )
    }

    if (showSaveProfileConfirm) {
        AlertDialog(
            onDismissRequest = { showSaveProfileConfirm = false },
            containerColor = OctiTheme.colors.panelCardBg,
            titleContentColor = OctiTheme.colors.accentBrass,
            textContentColor = OctiTheme.colors.textPrimary,
            title = { Text("Apply Profile Settings?") },
            text = {
                Text("Are you sure you want to save and apply these changes to your profile?")
            },
            confirmButton = {
                Button(
                    onClick = {
                        showSaveProfileConfirm = false
                        savingProfile = true
                        viewModel.clearProfileActionMessage()
                        viewModel.updateMyProfileAsync(
                            editableHandle.trim(),
                            if (avatarChanged) pendingAvatarUrl else "",
                            editableDisplayName.trim(),
                            editableBio.trim(),
                            editableLocation.trim(),
                            avatarChanged
                        ) { ok: Boolean ->
                            savingProfile = false
                            if (ok) {
                                avatarChanged = false
                                fieldsDirty = false
                                avatarInlineFeedback = ""
                            }
                        }
                    },
                    colors = ButtonDefaults.buttonColors(
                        containerColor = OctiTheme.colors.accentBrass,
                        contentColor = OctiTheme.colors.windowBg
                    )
                ) {
                    Text("Apply Changes")
                }
            },
            dismissButton = {
                OutlinedButton(onClick = { showSaveProfileConfirm = false }) {
                    Text("Cancel")
                }
            }
        )
    }

    if (showDiscardChangesConfirm) {
        AlertDialog(
            onDismissRequest = { showDiscardChangesConfirm = false },
            containerColor = OctiTheme.colors.panelCardBg,
            titleContentColor = OctiTheme.colors.accentBrass,
            textContentColor = OctiTheme.colors.textPrimary,
            title = { Text("Unsaved Profile Changes") },
            text = {
                Text("You have unsaved changes to your profile. Do you want to apply them or discard them?")
            },
            confirmButton = {
                Button(
                    onClick = {
                        showDiscardChangesConfirm = false
                        if (editableHandle.trim().isNotEmpty()) {
                            savingProfile = true
                            viewModel.clearProfileActionMessage()
                            viewModel.updateMyProfileAsync(
                                editableHandle.trim(),
                                if (avatarChanged) pendingAvatarUrl else "",
                                editableDisplayName.trim(),
                                editableBio.trim(),
                                editableLocation.trim(),
                                avatarChanged
                            ) { ok: Boolean ->
                                savingProfile = false
                                if (ok) {
                                    avatarChanged = false
                                    fieldsDirty = false
                                    onDismiss()
                                }
                            }
                        }
                    },
                    enabled = editableHandle.trim().isNotEmpty(),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = OctiTheme.colors.accentBrass,
                        contentColor = OctiTheme.colors.windowBg
                    )
                ) {
                    Text("Apply & Close")
                }
            },
            dismissButton = {
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    OutlinedButton(
                        onClick = {
                            showDiscardChangesConfirm = false
                            viewModel.cancelBrowserAuth()
                            viewModel.clearProfileActionMessage()
                            onDismiss()
                        }
                    ) {
                        Text("Discard")
                    }
                    OutlinedButton(onClick = { showDiscardChangesConfirm = false }) {
                        Text("Keep Editing")
                    }
                }
            }
        )
    }
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
    var timeControl by remember { mutableStateOf("10+5") }
    var rated by remember { mutableStateOf(true) }
    var allowTakeback by remember { mutableStateOf(false) }
    var passcode by remember { mutableStateOf("") }
    val profileActionBusy by viewModel.profileActionBusy.collectAsState()

    AlertDialog(
        onDismissRequest = onDismiss,
        containerColor = OctiTheme.colors.panelCardBg,
        titleContentColor = OctiTheme.colors.accentBrass,
        textContentColor = OctiTheme.colors.textPrimary,
        title = { Text("Create Online Table") },
        text = {
            Column(
                modifier = Modifier.verticalScroll(rememberScrollState()),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Text("Game Mode", fontWeight = FontWeight.SemiBold, color = OctiTheme.colors.accentBrass)
                FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    listOf("2P", "4P", "OFK").forEach { m ->
                        FilterChip(
                            selected = mode == m,
                            onClick = {
                                mode = m
                                bases = if (m == "4P" || m == "OFK") 1 else 3
                                if (m == "OFK") {
                                    superProngs = false
                                    edgeless = false
                                }
                            },
                            label = { Text(m) }
                        )
                    }
                }
                if (mode != "OFK") {
                    Text("Bases", fontWeight = FontWeight.SemiBold, color = OctiTheme.colors.accentBrass)
                    FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                        listOf(3 to "3-Base", 1 to "1-Base").forEach { (b, lbl) ->
                            FilterChip(
                                selected = bases == b,
                                onClick = { bases = b },
                                label = { Text(lbl) }
                            )
                        }
                    }
                }
                Text("Time Control", fontWeight = FontWeight.SemiBold, color = OctiTheme.colors.accentBrass)
                FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    listOf("Untimed", "5+0", "10+5", "15+10").forEach { tc ->
                        FilterChip(
                            selected = timeControl == tc,
                            onClick = { timeControl = tc },
                            label = { Text(tc) }
                        )
                    }
                }
                if (mode != "OFK") {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Checkbox(checked = superProngs, onCheckedChange = { superProngs = it })
                        Text("Super-Prongs")
                    }
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Checkbox(checked = edgeless, onCheckedChange = { edgeless = it })
                        Text("Edgeless Board")
                    }
                }
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Checkbox(checked = rated, onCheckedChange = { rated = it })
                    Text("Elo Rated Match")
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
                        this.bases = if (mode == "OFK") 1 else bases
                        this.superProngs = if (mode == "OFK") false else superProngs
                        this.edgeless = if (mode == "OFK") false else edgeless
                        this.timeControl = timeControl
                        this.rated = rated
                        this.allowTakeback = allowTakeback
                        this.passcode = passcode.trim()
                    }
                    viewModel.createOnlineTableAsync(req) { res ->
                        if (res.ok) {
                            onDismiss()
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
    canInteract: Boolean = true,
    onDismiss: () -> Unit,
    onBeginGuidedMove: (podIndex: Int, moveAllStack: Boolean) -> Unit,
    onAddProng: (podIndex: Int, dir: OctiDir, superProng: Boolean) -> Unit,
    onRepositionProng: (podIndex: Int, srcDir: OctiDir, dstDir: OctiDir) -> Unit,
    onPlaceReservePod: (liberate: Boolean) -> Unit
) {
    val displayBoard = if (composer.isAwaitingConfirmation) composer.buildPreviewBoard() ?: board else board
    val displayGame = displayBoard.game
    val sq = if (displayBoard.isValidCoord(squareX - 1, squareY - 1)) {
        displayBoard.squareAt(squareX - 1, squareY - 1)
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
    val turn = displayGame?.turn ?: OctiPlayer.PLAYER_A
    val isTurn = canInteract && displayGame != null && displayGame.isGameStarted && !displayGame.isGameOver
    val hasEnemy = sq?.hasEnemyOf(turn) == true
    val isOwnOccupied = sq?.isOccupiedBy(turn) == true
    val isFriendlyTurn = isTurn && !isEmptySquare && isOwnOccupied
    val freePods = displayGame?.getFreePods(turn) ?: 0
    val capPods = displayGame?.getCapturedPods(turn) ?: 0

    val is4P = displayBoard.gameMode == OctiGameMode.GAME_MODE_4
    val partner = if (is4P) {
        when (turn) {
            OctiPlayer.PLAYER_A -> OctiPlayer.PLAYER_C
            OctiPlayer.PLAYER_C -> OctiPlayer.PLAYER_A
            OctiPlayer.PLAYER_B -> OctiPlayer.PLAYER_D
            OctiPlayer.PLAYER_D -> OctiPlayer.PLAYER_B
            else -> null
        }
    } else null
    val partnerHome = if (is4P && partner != null) {
        when (partner) {
            OctiPlayer.PLAYER_A -> OctiSquareType.A_HOME_SQUARE
            OctiPlayer.PLAYER_B -> OctiSquareType.B_HOME_SQUARE
            OctiPlayer.PLAYER_C -> OctiSquareType.C_HOME_SQUARE
            OctiPlayer.PLAYER_D -> OctiSquareType.D_HOME_SQUARE
            else -> null
        }
    } else null
    val isRevivingPartner = is4P && partner != null && (sq?.type == partnerHome) &&
        (displayGame?.isPlayerEliminated(partner) == true) && isOwnOccupied &&
        ((displayGame?.getCapturedPods(partner) ?: 0) > 0)

    val canPlaceReserve = composer.canPlaceReservePodAt(squareX, squareY)
    val canLiberateHere = composer.canLiberateCapturedPodAt(squareX, squareY)
    val canShowPodPlacement = isTurn && displayBoard.gameMode != OctiGameMode.GAME_MODE_OFK && (canPlaceReserve || canLiberateHere)
    var moveAllStack by remember(squareX, squareY) { mutableStateOf(false) }

    val freeProngs = displayGame?.getFreeProngs(turn) ?: 0
    val freeSuper = displayGame?.getFreeSuperProngs(turn) ?: 0
    val useSuperProngs = displayGame?.useSuperProngs() == true

    var selectedProngTray by remember(squareX, squareY) {
        mutableStateOf<String?>(if (isFriendlyTurn && freeProngs > 0) "NORMAL" else null)
    }
    var selectedRepositionSrcDir by remember(squareX, squareY) { mutableStateOf<OctiDir?>(null) }
    var hoverSocketDir by remember(squareX, squareY) { mutableStateOf(-1) }
    var podBoxPxSize by remember { mutableStateOf(IntSize(480, 480)) }

    val zoomedBitmap = remember(squareX, squareY, selectedPodIndex, rotation, themeId, hoverSocketDir, composer.stagedMoveNotation()) {
        val sizePx = 480
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
                        .padding(14.dp)
                        .verticalScroll(rememberScrollState()),
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
                            text = if (hasEnemy) "Opponent Inspect" else if (isEmptySquare) "Home Square" else if (isOwnOccupied) "Friendly Pod" else "Partner Pod",
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
                                    selectedProngTray = "NORMAL"
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
                            .pointerInput(selectedPodIndex, rotation, isFriendlyTurn, selectedProngTray, selectedRepositionSrcDir, podBoxPxSize, squareX, squareY, freeProngs, freeSuper, board.gameMode) {
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
                                                if (board.gameMode != OctiGameMode.GAME_MODE_OFK) {
                                                    selectedRepositionSrcDir = hitDir
                                                    hoverSocketDir = hitDir.index
                                                }
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
                            .pointerInput(selectedPodIndex, rotation, isFriendlyTurn, selectedProngTray, selectedRepositionSrcDir, podBoxPxSize, sortedPodIndices, squareX, squareY, freeProngs, freeSuper, board.gameMode) {
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
                                        if (hitDir != OctiDir.DIR_UNDEF) {
                                            hoverSocketDir = hitDir.index
                                        }
                                        if (isFriendlyTurn && hitDir != OctiDir.DIR_UNDEF && pod.prongAt(hitDir) == OctiProngType.NORMAL_PRONG && board.gameMode != OctiGameMode.GAME_MODE_OFK) {
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
                                                    (podR * 0.4).toDouble(), (podR * 2.4).toDouble(),
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
                                                    (podR * 0.4).toDouble(), (podR * 2.4).toDouble(),
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
                        val libButtonLabel = if (isRevivingPartner) {
                            "Liberate Partner's Captured Pod (${squareX}${squareY}*)"
                        } else {
                            "Liberate Captured Pod (*)"
                        }
                        if (canPlaceReserve && canLiberateHere) {
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.spacedBy(8.dp)
                            ) {
                                Button(
                                    onClick = {
                                        onPlaceReservePod(false)
                                    },
                                    enabled = true,
                                    modifier = Modifier.weight(1f)
                                ) {
                                    Text(OctiDialogs.PLACE_RESERVE_POD_LABEL)
                                }
                                OutlinedButton(
                                    onClick = {
                                        onPlaceReservePod(true)
                                    },
                                    enabled = true,
                                    modifier = Modifier.weight(1f)
                                ) {
                                    Text(libButtonLabel)
                                }
                            }
                        } else if (canLiberateHere) {
                            Button(
                                onClick = {
                                    onPlaceReservePod(true)
                                },
                                enabled = true,
                                modifier = Modifier.fillMaxWidth()
                            ) {
                                Text(libButtonLabel)
                            }
                        } else if (canPlaceReserve) {
                            Button(
                                onClick = {
                                    onPlaceReservePod(false)
                                },
                                enabled = true,
                                modifier = Modifier.fillMaxWidth()
                            ) {
                                Text(OctiDialogs.PLACE_RESERVE_POD_LABEL)
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
    var flashBright by remember {
        mutableStateOf(OctiBoardRenderer.isCommitButtonFlashBright(System.currentTimeMillis()))
    }
    LaunchedEffect(Unit) {
        while (true) {
            flashBright = OctiBoardRenderer.isCommitButtonFlashBright(System.currentTimeMillis())
            delay(50L)
        }
    }
    val stateTs = if (flashBright) 0L else 500L
    val commitBgColor = Color(OctiBoardRenderer.commitButtonBgColor(stateTs))
    val commitBorderColor = Color(OctiBoardRenderer.commitButtonBorderColor(stateTs))
    val commitTextColor = Color(OctiBoardRenderer.commitButtonTextColor(stateTs))
    val commitBorderWidth = if (flashBright) 2.2.dp else 1.2.dp

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
                        containerColor = commitBgColor,
                        contentColor = commitTextColor
                    ),
                    border = BorderStroke(commitBorderWidth, commitBorderColor)
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
    gameMode: OctiGameMode = OctiGameMode.GAME_MODE_2,
    terminationReason: String = "",
    customTitle: String? = null,
    newGameButtonText: String = "New Game",
    reviewBoardButtonText: String = "Review Board",
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
                val titleText = customTitle ?: when {
                    gameMode == OctiGameMode.GAME_MODE_4 -> {
                        if (winner == OctiPlayer.PLAYER_A || winner == OctiPlayer.PLAYER_C) {
                            "Team A & C Wins!"
                        } else {
                            "Team B & D Wins!"
                        }
                    }
                    else -> "VICTORY — ${winner.name} WINS!"
                }
                Text(
                    text = titleText,
                    style = MaterialTheme.typography.titleLarge,
                    fontWeight = FontWeight.ExtraBold,
                    color = OctiTheme.colors.accentBrass,
                    textAlign = TextAlign.Center
                )
                val concludedText = when {
                    resigned -> "Match concluded by resignation after $totalPlies plies"
                    terminationReason.isNotEmpty() -> "$terminationReason after $totalPlies plies"
                    else -> "Match concluded after $totalPlies plies"
                }
                Text(
                    text = concludedText,
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
                        Text(reviewBoardButtonText, fontWeight = FontWeight.SemiBold)
                    }
                    Button(
                        onClick = onNewGame,
                        modifier = Modifier.weight(1f),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = OctiTheme.colors.accentBrass,
                            contentColor = OctiTheme.colors.windowBg
                        )
                    ) {
                        Text(newGameButtonText, fontWeight = FontWeight.Bold)
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
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg.copy(alpha = 1.0f)),
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
    mode: OctiGameMode = OctiGameMode.GAME_MODE_2,
    bases: OctiGameBases = OctiGameBases.GAME_BASES_3,
    useSuperProngs: Boolean = false,
    useEdgeless: Boolean = false,
    humanSeat: OctiPlayer = OctiPlayer.PLAYER_A,
    vsBot: Boolean = true,
    difficulty: OctiBotEngine.BotDifficulty = OctiBotEngine.BotDifficulty.TACTICAL,
    onChangeType: () -> Unit = {},
    onStartMatch: () -> Unit
) {
    Card(
        modifier = Modifier
            .fillMaxWidth(0.92f)
            .widthIn(min = 300.dp, max = 360.dp)
            .padding(8.dp),
        shape = RoundedCornerShape(12.dp),
        colors = CardDefaults.cardColors(
            containerColor = Color(0xFF161D26)
        ),
        border = BorderStroke(2.dp, Color(0xFFC8A253)),
        elevation = CardDefaults.cardElevation(defaultElevation = 8.dp)
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 16.dp, vertical = 14.dp),
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
            val configSummary = OctiBoardRenderer.formatStartMatchConfigSummary(
                mode, bases, useSuperProngs, useEdgeless, vsBot, difficulty, humanSeat
            )
            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(
                        color = Color(0xFF0F151D),
                        shape = RoundedCornerShape(8.dp)
                    )
                    .border(
                        width = 1.dp,
                        color = Color(0xFF2E4057),
                        shape = RoundedCornerShape(8.dp)
                    )
                    .padding(horizontal = 10.dp, vertical = 8.dp),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    text = configSummary,
                    color = Color(0xFFC8D4E3),
                    fontSize = 12.sp,
                    lineHeight = 16.sp,
                    textAlign = TextAlign.Center
                )
            }
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(10.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Button(
                    onClick = onChangeType,
                    modifier = Modifier.weight(1f),
                    contentPadding = PaddingValues(horizontal = 10.dp, vertical = 10.dp),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = Color(0xFF2E4057),
                        contentColor = Color(0xFFE2E8F0)
                    ),
                    border = BorderStroke(1.dp, Color(0xFF4B6382)),
                    shape = RoundedCornerShape(6.dp)
                ) {
                    Text(
                        text = OctiDialogs.CHANGE_TYPE_BUTTON_LABEL,
                        fontWeight = FontWeight.Bold,
                        fontSize = 13.sp,
                        softWrap = false,
                        maxLines = 1
                    )
                }
                Button(
                    onClick = onStartMatch,
                    modifier = Modifier.weight(1f),
                    contentPadding = PaddingValues(horizontal = 10.dp, vertical = 10.dp),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = Color(0xFFD4AF37),
                        contentColor = Color(0xFF161D26)
                    ),
                    shape = RoundedCornerShape(6.dp)
                ) {
                    Text(
                        text = OctiDialogs.START_MATCH_BUTTON_LABEL,
                        fontWeight = FontWeight.Bold,
                        fontSize = 13.sp,
                        softWrap = false,
                        maxLines = 1
                    )
                }
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
        mode = OctiGameMode.GAME_MODE_2,
        bases = OctiGameBases.GAME_BASES_3,
        useSuperProngs = false,
        useEdgeless = false,
        humanSeat = humanSeat,
        vsBot = true,
        difficulty = OctiBotEngine.BotDifficulty.TACTICAL,
        onChangeType = {},
        onStartMatch = onStartMatch
    )
}

@Composable
fun BoardChatZoomDialog(
    viewModel: OctiAppViewModel,
    onDismiss: () -> Unit
) {
    val chatLog by viewModel.gameChatLog.collectAsState()
    val matchType by viewModel.matchType.collectAsState()
    val activeGameView by viewModel.activeGameView.collectAsState()
    val canTeamChat = matchType == OctiAppViewModel.MatchType.ONLINE_ROOM && activeGameView?.mode == "4P" && (activeGameView?.viewerSeat ?: 0) > 0
    var inputText by remember { mutableStateOf("") }
    var currentScope by remember { mutableStateOf("ALL") }
    if (!canTeamChat && currentScope != "ALL") {
        currentScope = "ALL"
    }
    var chatErrorText by remember { mutableStateOf("") }
    val listState = rememberLazyListState()

    LaunchedEffect(chatLog.size) {
        if (chatLog.isNotEmpty()) {
            listState.animateScrollToItem(chatLog.size - 1)
        }
    }

    Dialog(
        onDismissRequest = onDismiss,
        properties = DialogProperties(usePlatformDefaultWidth = false)
    ) {
        Card(
            modifier = Modifier
                .fillMaxWidth(0.92f)
                .imePadding()
                .navigationBarsPadding()
                .background(OctiTheme.colors.panelCardBg, RoundedCornerShape(12.dp))
                .border(1.5.dp, OctiTheme.colors.accentBrass, RoundedCornerShape(12.dp))
                .padding(14.dp),
            colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
            shape = RoundedCornerShape(12.dp)
        ) {
            Column(
                modifier = Modifier.fillMaxWidth(),
                verticalArrangement = Arrangement.spacedBy(10.dp)
            ) {
                // Header row
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = "TABLE CHAT",
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold,
                        color = OctiTheme.colors.accentBrass
                    )
                    IconButton(
                        onClick = onDismiss,
                        modifier = Modifier.size(28.dp)
                    ) {
                        Text(
                            text = "✕",
                            style = MaterialTheme.typography.titleMedium,
                            color = OctiTheme.colors.textSecondary
                        )
                    }
                }

                // Scrollable LazyColumn displaying gameChatLog entries
                Surface(
                    modifier = Modifier
                        .fillMaxWidth()
                        .heightIn(min = 160.dp, max = 260.dp),
                    color = OctiTheme.colors.windowBg,
                    shape = RoundedCornerShape(8.dp),
                    border = BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
                ) {
                    if (chatLog.isEmpty()) {
                        Box(
                            modifier = Modifier
                                .fillMaxSize()
                                .padding(12.dp),
                            contentAlignment = Alignment.Center
                        ) {
                            Text(
                                text = "No messages yet. Chat with players at the table.",
                                style = MaterialTheme.typography.bodySmall,
                                color = OctiTheme.colors.textSecondary
                            )
                        }
                    } else {
                        LazyColumn(
                            state = listState,
                            modifier = Modifier
                                .fillMaxSize()
                                .padding(8.dp),
                            verticalArrangement = Arrangement.spacedBy(6.dp)
                        ) {
                            items(chatLog) { msg ->
                                val sender = if (msg.senderHandle.isNotEmpty()) {
                                    val clean = msg.senderHandle.removePrefix("@")
                                    if (clean.equals("bot", ignoreCase = true) || clean.equals("stock", ignoreCase = true)) {
                                        "@$clean [BOT]"
                                    } else {
                                        "@$clean"
                                    }
                                } else {
                                    "You"
                                }
                                val scopePrefix = if (msg.scope.equals("TEAM", ignoreCase = true)) "[TEAM] " else ""
                                val sdf = remember { java.text.SimpleDateFormat("HH:mm:ss", java.util.Locale.getDefault()) }
                                val timeStr = sdf.format(java.util.Date(msg.atMs))
                                Row(
                                    modifier = Modifier.fillMaxWidth(),
                                    verticalAlignment = Alignment.Top
                                ) {
                                    Text(
                                        text = "[$timeStr] ",
                                        style = MaterialTheme.typography.bodySmall,
                                        color = OctiTheme.colors.textSecondary
                                    )
                                    Text(
                                        text = "$scopePrefix$sender: ",
                                        style = MaterialTheme.typography.bodySmall,
                                        fontWeight = FontWeight.Bold,
                                        color = if (sender.contains("[BOT]")) OctiTheme.colors.accentBrass else OctiTheme.colors.textPrimary
                                    )
                                    Text(
                                        text = msg.text,
                                        style = MaterialTheme.typography.bodySmall,
                                        color = OctiTheme.colors.textPrimary,
                                        modifier = Modifier.weight(1f)
                                    )
                                }
                            }
                        }
                    }
                }

                if (chatErrorText.isNotEmpty()) {
                    Text(
                        text = chatErrorText,
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.error,
                        modifier = Modifier.padding(horizontal = 4.dp)
                    )
                }

                // Input row with ALL/TEAM scope toggle button (if 4P seated online), OutlinedTextField, and Send button
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(6.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    if (canTeamChat) {
                        OutlinedButton(
                            onClick = {
                                currentScope = if (currentScope == "ALL") "TEAM" else "ALL"
                            },
                            contentPadding = PaddingValues(horizontal = 8.dp, vertical = 0.dp),
                            modifier = Modifier.height(48.dp)
                        ) {
                            Text(
                                text = currentScope,
                                style = MaterialTheme.typography.labelSmall,
                                fontWeight = FontWeight.Bold
                            )
                        }
                    }

                    OutlinedTextField(
                        value = inputText,
                        onValueChange = {
                            inputText = it
                            if (chatErrorText.isNotEmpty()) chatErrorText = ""
                        },
                        modifier = Modifier.weight(1f),
                        placeholder = {
                            Text("Type a table message...", style = MaterialTheme.typography.bodySmall)
                        },
                        singleLine = true,
                        keyboardOptions = KeyboardOptions(imeAction = ImeAction.Send),
                        keyboardActions = KeyboardActions(
                            onSend = {
                                val t = inputText.trim()
                                if (t.isNotEmpty()) {
                                    viewModel.sendGameChatAsync(currentScope, t) { res ->
                                        if (res.ok) {
                                            inputText = ""
                                            chatErrorText = ""
                                        } else {
                                            chatErrorText = res.errorMessage.ifEmpty { "Failed to send chat" }
                                        }
                                    }
                                }
                            }
                        )
                    )

                    Button(
                        onClick = {
                            val t = inputText.trim()
                            if (t.isNotEmpty()) {
                                viewModel.sendGameChatAsync(currentScope, t) { res ->
                                    if (res.ok) {
                                        inputText = ""
                                        chatErrorText = ""
                                    } else {
                                        chatErrorText = res.errorMessage.ifEmpty { "Failed to send chat" }
                                    }
                                }
                            }
                        },
                        colors = ButtonDefaults.buttonColors(containerColor = OctiTheme.colors.accentBrass),
                        contentPadding = PaddingValues(horizontal = 12.dp, vertical = 0.dp),
                        modifier = Modifier.height(48.dp)
                    ) {
                        Text("Send", color = OctiTheme.colors.windowBg, fontWeight = FontWeight.Bold)
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
