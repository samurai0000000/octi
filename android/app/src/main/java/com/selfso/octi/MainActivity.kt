/*
 * MainActivity.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi

import android.app.Activity
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import android.content.pm.ActivityInfo
import android.os.Bundle
import android.view.KeyEvent
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.wrapContentHeight
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.gestures.detectHorizontalDragGestures
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.ui.input.pointer.PointerEventPass
import androidx.compose.ui.input.pointer.pointerInput
import kotlinx.coroutines.delay
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
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
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.selfso.octi.BuildConfig
import com.selfso.selfupdate.UpdateState
import com.selfso.octi.ui.AccountIdentitiesDialog
import com.selfso.octi.ui.ArchiveScreen
import com.selfso.octi.ui.BoardChatZoomDialog
import com.selfso.octi.ui.CapturePromptDialog
import com.selfso.octi.ui.CompositePodMoveDialog
import com.selfso.octi.ui.ConnectedUsersAndRobotsDialog
import com.selfso.octi.ui.CreateTableDialog
import com.selfso.octi.ui.ExportMovesDialog
import com.selfso.octi.ui.GameOverDialog
import com.selfso.octi.ui.LobbyChatScreen
import com.selfso.octi.ui.LobbyChatZoomDialog
import com.selfso.octi.ui.LobbyScreen
import com.selfso.octi.ui.NewGameSetupDialog
import com.selfso.octi.ui.OctiAppViewModel
import com.selfso.octi.ui.OctiBoardCanvas
import com.selfso.octi.ui.OctiPlayerAvatar
import com.selfso.octi.ui.OctiReserveHud
import com.selfso.octi.ui.OctiSplashOverlay
import com.selfso.octi.ui.OctiTelemetryConsentDialog
import com.selfso.octi.ui.OctiTelemetrySettingsDialog
import com.selfso.octi.ui.OctiThinkingDialog
import com.selfso.octi.ui.PlyConfirmDialog
import com.selfso.octi.ui.SsoSignInDialog
import com.selfso.octi.ui.StartMatchOverlayDialog
import com.selfso.octi.ui.SuperProngBonusDialog
import com.selfso.octi.ui.UpToDateModalDialog
import com.selfso.octi.ui.ZoomedSquareDialog
import com.selfso.octi.ui.theme.OctiMaterialTheme
import com.selfso.octi.ui.theme.OctiTheme
import com.selfso.selfupdate.ui.SelfUpdateBanner
import com.selfso.selfupdate.ui.rememberSelfUpdateController
import java.io.File

class MainActivity : ComponentActivity() {

    lateinit var appViewModel: OctiAppViewModel
        private set

    var isSplashActive: Boolean = true
    private val isFacebookAuthEnabled: Boolean = false

    private val facebookNativeAuthLauncher = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (!::appViewModel.isInitialized) return@registerForActivityResult
        val extras = result.data?.extras
        val resultArgs = extras?.getBundle("com.facebook.platform.protocol.RESULT_ARGS")
        val accessToken = extras?.getString("access_token")
            ?: extras?.getString("com.facebook.platform.extra.ACCESS_TOKEN")
            ?: resultArgs?.getString("access_token")
            ?: resultArgs?.getString("com.facebook.platform.extra.ACCESS_TOKEN")
            ?: ""
        val errorMsg = extras?.getString("error_message")
            ?: extras?.getString("error_description")
            ?: extras?.getString("com.facebook.platform.status.ERROR_DESCRIPTION")
            ?: resultArgs?.getString("com.facebook.platform.status.ERROR_DESCRIPTION")
            ?: resultArgs?.getString("error_message")
            ?: extras?.getString("error")
            ?: ""

        if (accessToken.isNotEmpty()) {
            appViewModel.completeNativeSocialAuth("facebook", accessToken)
        } else if (errorMsg.isNotEmpty()) {
            appViewModel.failNativeSocialAuth("Facebook sign-in failed: $errorMsg")
        } else if (result.resultCode == Activity.RESULT_CANCELED) {
            appViewModel.cancelBrowserAuth()
        } else {
            appViewModel.failNativeSocialAuth("Could not complete Facebook sign-in.")
        }
    }

    private fun launchNativeFacebookProxyAuth(clientId: String): Boolean {
        if (!isFacebookAuthEnabled) return false
        val fbPackages = listOf("com.facebook.katana", "com.facebook.wakizashi")
        for (pkg in fbPackages) {
            val intent = Intent().apply {
                setClassName(pkg, "com.facebook.katana.ProxyAuth")
                putExtra("client_id", clientId)
                putExtra("scope", "public_profile,email")
                putExtra("e2e", "{}")
                putExtra("response_type", "token,signed_request,graph_domain")
                putExtra("return_scopes", "true")
                putExtra("default_audience", "friends")
                putExtra("legacy_override", "v19.0")
                putExtra("auth_type", "rerequest")
            }
            try {
                if (packageManager.resolveActivity(intent, 0) != null) {
                    facebookNativeAuthLauncher.launch(intent)
                    return true
                }
            } catch (_: Throwable) {
            }
        }
        return false
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
        volumeControlStream = android.media.AudioManager.STREAM_MUSIC

        val configFile = ClientConfigStore.configFileForDirectory(filesDir ?: File("."))
        appViewModel = OctiAppViewModel(ClientConfigStore(configFile))
        appViewModel.setPreferDeepLinkRedirect(true)
        appViewModel.nativeSocialAuthLauncher = { provider, _, clientId, _ ->
            if (isFacebookAuthEnabled && provider.equals("facebook", ignoreCase = true) && clientId.isNotEmpty()) {
                launchNativeFacebookProxyAuth(clientId)
            } else {
                false
            }
        }
        appViewModel.setAsyncBotThinkingEnabled(true)

        val storageRoot = filesDir ?: File(".")
        val appVersion = try { BuildConfig.VERSION_NAME } catch (_: Throwable) { "2.1.2" }
        OctiCrashReporter.installGlobalHandler(storageRoot, appVersion, appViewModel.installId.value)
        appViewModel.setStorageRootDir(storageRoot)
        appViewModel.flushPendingTelemetry(async = true)
        appViewModel.setRequireStartMatchPrompt(true)
        appViewModel.beginAwaitingMatchStart()

        // Extract intent extras while strictly ignoring any server_url / server_host / server_port override
        val extrasMap = LinkedHashMap<String, String>()
        intent?.extras?.keySet()?.forEach { key ->
            val v = intent?.extras?.getString(key)
            if (v != null) {
                extrasMap[key] = v
            }
        }
        appViewModel.applyExternalArguments(extrasMap)
        intent?.dataString?.let { uriStr ->
            if (uriStr.startsWith("octi://")) {
                appViewModel.handleOAuthRedirectUri(uriStr)
            }
        }
        if (appViewModel.isAuthenticated.value) {
            appViewModel.refreshMyProfileAsync()
        }

        setContent {
            val themeId by appViewModel.theme.collectAsState()
            OctiMaterialTheme(themeId = themeId) {
                OctiMainAppContent(
                    viewModel = appViewModel,
                    onSplashDismissed = { isSplashActive = false }
                )
            }
        }
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        setIntent(intent)
        if (::appViewModel.isInitialized) {
            intent.dataString?.let { uriStr ->
                if (uriStr.startsWith("octi://")) {
                    appViewModel.handleOAuthRedirectUri(uriStr)
                }
            }
        }
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
        if (isSplashActive && (keyCode == KeyEvent.KEYCODE_ESCAPE || keyCode == KeyEvent.KEYCODE_BACK)) {
            isSplashActive = false
            return true
        }
        return super.onKeyDown(keyCode, event)
    }

    override fun onResume() {
        super.onResume()
        if (::appViewModel.isInitialized) {
            appViewModel.flushPendingTelemetry(async = true)
            if (appViewModel.isAuthenticated.value) {
                appViewModel.refreshMyProfileAsync()
            }
        }
    }

    override fun onDestroy() {
        if (isFinishing && ::appViewModel.isInitialized) {
            appViewModel.close()
        }
        super.onDestroy()
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun OctiMainAppContent(
    viewModel: OctiAppViewModel,
    onSplashDismissed: () -> Unit = {}
) {
    val updateController = rememberSelfUpdateController(checkOnLaunch = true)
    val updateState by updateController.state.collectAsState()
    var manualUpdateCheckPending by remember { mutableStateOf(false) }
    var updateModalMessage by remember { mutableStateOf<String?>(null) }

    LaunchedEffect(updateState) {
        if (manualUpdateCheckPending) {
            when (val s = updateState) {
                is UpdateState.UpToDate -> {
                    manualUpdateCheckPending = false
                    updateModalMessage = "OCTI v${BuildConfig.VERSION_NAME} is up to date. No newer update is available."
                }
                is UpdateState.Error -> {
                    manualUpdateCheckPending = false
                    updateModalMessage = s.message
                }
                is UpdateState.Available, is UpdateState.ReadyToInstall, is UpdateState.Downloading, is UpdateState.Installing -> {
                    manualUpdateCheckPending = false
                }
                else -> {}
            }
        }
    }

    LaunchedEffect(Unit) {
        while (true) {
            delay(1000L)
            viewModel.tickClock()
        }
    }

    val context = LocalContext.current
    val currentTab by viewModel.currentTab.collectAsState()
    val matchType by viewModel.matchType.collectAsState()
    val activeRoomId by viewModel.activeRoomId.collectAsState()
    val activeGameView by viewModel.activeGameView.collectAsState()

    LaunchedEffect(matchType, activeRoomId) {
        if (matchType == OctiAppViewModel.MatchType.ONLINE_ROOM && activeRoomId.isNotEmpty()) {
            while (true) {
                delay(3000L)
                viewModel.refreshActiveGameViewAsync()
            }
        }
    }

    val themeId by viewModel.theme.collectAsState()
    val sfxMuted by viewModel.sfxMuted.collectAsState()
    val statusBanner by viewModel.statusBanner.collectAsState()
    val showNewGameDialog by viewModel.showNewGameDialog.collectAsState()
    val showSsoDialog by viewModel.showSsoDialog.collectAsState()
    val showAccountDialog by viewModel.showAccountDialog.collectAsState()
    val currentProfile by viewModel.currentProfile.collectAsState()
    val isAuthenticated by viewModel.isAuthenticated.collectAsState()
    val showCreateTableDialog by viewModel.showCreateTableDialog.collectAsState()
    val showGameOverDialog by viewModel.showGameOverDialog.collectAsState()
    val showExportMovesDialog by viewModel.showExportMovesDialog.collectAsState()
    val showBoardChatZoomDialog by viewModel.showBoardChatZoomDialog.collectAsState()
    val showConnectedUsersDialog by viewModel.showConnectedUsersDialog.collectAsState()
    val showLobbyChatZoomDialog by viewModel.showLobbyChatZoomDialog.collectAsState()
    val showThemePickerDialog by viewModel.showThemePickerDialog.collectAsState()
    val isBotThinking by viewModel.isBotThinking.collectAsState()
    val botDifficulty by viewModel.botDifficulty.collectAsState()
    val boardRev by viewModel.boardRevision.collectAsState()
    val currentBoardRev = boardRev
    val awaitingConfirmation by viewModel.awaitingConfirmation.collectAsState()
    val stagedMoveNotation by viewModel.stagedMoveNotation.collectAsState()
    val canAddMoreMoves by viewModel.canAddMoreMoves.collectAsState()
    val zoomedSquare by viewModel.zoomedSquare.collectAsState()
    val capturePrompt by viewModel.capturePrompt.collectAsState()
    val isContinuingGuidedJump by viewModel.isContinuingGuidedJump.collectAsState()
    val apkSha256Prefix = remember(context) { OctiAppViewModel.computeInstalledApkSha256Prefix(context, 12) }
    var showSplash by remember { mutableStateOf(true) }
    var splashLoopIndefinitely by remember { mutableStateOf(false) }
    var showTopMenu by remember { mutableStateOf(false) }
    var pendingPasscodeSeat by remember { mutableStateOf<Int?>(null) }
    var joinSeatPasscodeInput by remember { mutableStateOf("") }

    Scaffold(
        containerColor = OctiTheme.colors.windowBg,
        topBar = {
            Column(modifier = Modifier.fillMaxWidth()) {
                TopAppBar(
                    colors = TopAppBarDefaults.topAppBarColors(
                        containerColor = OctiTheme.colors.panelCardBg,
                        titleContentColor = OctiTheme.colors.accentBrass
                    ),
                    title = {
                        Text(
                            text = "OCTI",
                            style = MaterialTheme.typography.headlineMedium,
                            fontWeight = FontWeight.ExtraBold,
                            color = OctiTheme.colors.accentBrass,
                            maxLines = 1
                        )
                    },
                    actions = {
                        if (!isAuthenticated) {
                            TextButton(
                                onClick = { viewModel.setShowSsoDialog(true) },
                                colors = ButtonDefaults.textButtonColors(
                                    contentColor = OctiTheme.colors.accentBrass
                                ),
                                modifier = Modifier.padding(end = 4.dp)
                            ) {
                                Text("Sign In", fontWeight = FontWeight.Bold, maxLines = 1)
                            }
                        } else {
                            val prof = currentProfile
                            if (prof != null) {
                                OctiPlayerAvatar(
                                    avatarUrl = prof.avatarUrl,
                                    handle = prof.handle,
                                    size = 28.dp,
                                    viewModel = viewModel,
                                    modifier = Modifier
                                        .padding(end = 6.dp)
                                        .clickable { viewModel.setShowAccountDialog(true) }
                                )
                            }
                        }
                        Box {
                            TextButton(
                                onClick = { showTopMenu = true },
                                colors = ButtonDefaults.textButtonColors(
                                    contentColor = OctiTheme.colors.accentBrass
                                )
                            ) {
                                Text("Menu ▾", fontWeight = FontWeight.Bold)
                            }
                            DropdownMenu(
                                expanded = showTopMenu,
                                onDismissRequest = { showTopMenu = false }
                            ) {
                                DropdownMenuItem(
                                    text = {
                                        Text(
                                            "Online",
                                            fontWeight = if (currentTab == OctiAppViewModel.AppTab.ONLINE_LOBBY) FontWeight.Bold else FontWeight.Normal,
                                            color = if (currentTab == OctiAppViewModel.AppTab.ONLINE_LOBBY) OctiTheme.colors.accentBrass else Color.Unspecified
                                        )
                                    },
                                    onClick = {
                                        showTopMenu = false
                                        viewModel.selectTab(OctiAppViewModel.AppTab.ONLINE_LOBBY)
                                        viewModel.refreshLobbyStateAsync()
                                    }
                                )
                                DropdownMenuItem(
                                    text = {
                                        Text(
                                            "Board",
                                            fontWeight = if (currentTab == OctiAppViewModel.AppTab.BOARD) FontWeight.Bold else FontWeight.Normal,
                                            color = if (currentTab == OctiAppViewModel.AppTab.BOARD) OctiTheme.colors.accentBrass else Color.Unspecified
                                        )
                                    },
                                    onClick = {
                                        showTopMenu = false
                                        viewModel.selectTab(OctiAppViewModel.AppTab.BOARD)
                                    }
                                )
                                DropdownMenuItem(
                                    text = { Text("New Game") },
                                    onClick = {
                                        showTopMenu = false
                                        viewModel.openNewGameDialog()
                                    }
                                )
                                DropdownMenuItem(
                                    text = { Text("Connected Users & Robots") },
                                    onClick = {
                                        showTopMenu = false
                                        viewModel.openConnectedUsersDialog()
                                        viewModel.refreshLobbyStateAsync()
                                    }
                                )
                                DropdownMenuItem(
                                    text = {
                                        Text(
                                            "Archive & Elo",
                                            fontWeight = if (currentTab == OctiAppViewModel.AppTab.ARCHIVE_LEADERBOARD) FontWeight.Bold else FontWeight.Normal,
                                            color = if (currentTab == OctiAppViewModel.AppTab.ARCHIVE_LEADERBOARD) OctiTheme.colors.accentBrass else Color.Unspecified
                                        )
                                    },
                                    onClick = {
                                        showTopMenu = false
                                        viewModel.selectTab(OctiAppViewModel.AppTab.ARCHIVE_LEADERBOARD)
                                        viewModel.refreshLeaderboardAsync("2P", 50)
                                        viewModel.refreshArchivedGamesAsync("", "", 20, 0)
                                    }
                                )
                                HorizontalDivider(color = OctiTheme.colors.panelCardBorder)
                                DropdownMenuItem(
                                    text = { Text("Export Moves") },
                                    onClick = {
                                        showTopMenu = false
                                        val text = viewModel.exportMoveHistoryText()
                                        val clipboard = context.getSystemService(Context.CLIPBOARD_SERVICE) as? ClipboardManager
                                        val clip = ClipData.newPlainText("OCTI Moves", text)
                                        clipboard?.setPrimaryClip(clip)
                                        viewModel.setShowExportMovesDialog(true)
                                    }
                                )
                                DropdownMenuItem(
                                    text = { Text("Theme: ${themeId.displayName()}...") },
                                    onClick = {
                                        showTopMenu = false
                                        viewModel.setShowThemePickerDialog(true)
                                    }
                                )
                                DropdownMenuItem(
                                    text = {
                                        Text("Sound Effects: " + if (sfxMuted) "Off" else "On")
                                    },
                                    onClick = {
                                        showTopMenu = false
                                        viewModel.toggleSfxMuted()
                                    }
                                )
                                DropdownMenuItem(
                                    text = { Text("Check for Updates") },
                                    onClick = {
                                        showTopMenu = false
                                        manualUpdateCheckPending = true
                                        updateController.checkForUpdates()
                                    }
                                )
                                DropdownMenuItem(
                                    text = { Text("Telemetry & Privacy") },
                                    onClick = {
                                        showTopMenu = false
                                        viewModel.setShowTelemetrySettingsDialog(true)
                                    }
                                )
                                DropdownMenuItem(
                                    text = { Text("About OCTI (Splash)") },
                                    onClick = {
                                        showTopMenu = false
                                        splashLoopIndefinitely = true
                                        showSplash = true
                                    }
                                )
                            }
                        }
                    }
                )
                SelfUpdateBanner(
                    controller = updateController,
                    modifier = Modifier.fillMaxWidth()
                )
            }
        }
    ) { innerPadding ->
        val swipeModifier = Modifier.pointerInput(currentTab) {
            if (currentTab == OctiAppViewModel.AppTab.ONLINE_LOBBY || currentTab == OctiAppViewModel.AppTab.BOARD) {
                val thresholdPx = 60.dp.toPx()
                awaitEachGesture {
                    val down = awaitFirstDown(pass = PointerEventPass.Initial)
                    var totalDx = 0f
                    var totalDy = 0f
                    var gestureDecided = false
                    var isHorizontalSwipe = false
                    while (true) {
                        val event = awaitPointerEvent(pass = PointerEventPass.Initial)
                        val change = event.changes.firstOrNull { it.id == down.id } ?: break
                        if (!change.pressed) break
                        val dragAmount = change.position - change.previousPosition
                        totalDx += dragAmount.x
                        totalDy += dragAmount.y
                        if (!gestureDecided) {
                            if (kotlin.math.abs(totalDx) > thresholdPx / 3 || kotlin.math.abs(totalDy) > thresholdPx / 3) {
                                gestureDecided = true
                                isHorizontalSwipe = kotlin.math.abs(totalDx) > kotlin.math.abs(totalDy) * 1.2f
                            }
                        }
                    }
                    if (gestureDecided && isHorizontalSwipe) {
                        if (totalDx < -thresholdPx) {
                            viewModel.swipeToNextVirtualScreen()
                        } else if (totalDx > thresholdPx) {
                            viewModel.swipeToPreviousVirtualScreen()
                        }
                    }
                }
            }
        }
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(innerPadding)
                .background(OctiTheme.colors.windowBg)
                .then(swipeModifier)
        ) {
            when (currentTab) {
                OctiAppViewModel.AppTab.BOARD -> {
                    Column(
                        modifier = Modifier.fillMaxSize(),
                        verticalArrangement = Arrangement.spacedBy(4.dp)
                    ) {
                        val isOnlineRoom = matchType == OctiAppViewModel.MatchType.ONLINE_ROOM
                        val isGameOver = viewModel.isOnlineGameOver() && boardRev >= 0L
                        if (isGameOver) {
                            Card(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .padding(horizontal = 8.dp, vertical = 2.dp),
                                shape = RoundedCornerShape(8.dp),
                                colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
                                border = BorderStroke(1.5.dp, OctiTheme.colors.accentBrass)
                            ) {
                                Row(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(horizontal = 10.dp, vertical = 4.dp),
                                    horizontalArrangement = Arrangement.SpaceBetween,
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    val titleText = if (isOnlineRoom) {
                                        viewModel.describeOnlineGameOverBanner()
                                    } else {
                                        "GAME OVER — Winner: ${viewModel.board.game?.winner}"
                                    }
                                    Text(
                                        text = titleText,
                                        style = MaterialTheme.typography.labelLarge,
                                        fontWeight = FontWeight.ExtraBold,
                                        color = OctiTheme.colors.accentBrass,
                                        maxLines = 1,
                                        overflow = TextOverflow.Ellipsis,
                                        modifier = Modifier
                                            .weight(1f)
                                            .padding(end = 6.dp)
                                    )
                                    Row(
                                        horizontalArrangement = Arrangement.spacedBy(6.dp),
                                        verticalAlignment = Alignment.CenterVertically
                                    ) {
                                        OutlinedButton(
                                            onClick = { viewModel.openGameOverDialog() },
                                            modifier = Modifier.height(28.dp),
                                            contentPadding = PaddingValues(horizontal = 10.dp, vertical = 0.dp)
                                        ) {
                                            Text("Score Sheet", style = MaterialTheme.typography.labelSmall, fontWeight = FontWeight.SemiBold)
                                        }
                                    }
                                }
                            }
                        }
                        val cols = viewModel.board.boardWidth
                        val rows = viewModel.board.boardHeight
                        val boardRot by viewModel.boardRotation.collectAsState()
                        val isSideways = (boardRot == OctiBoardGeometry.BoardRotation.ROT_90 ||
                                          boardRot == OctiBoardGeometry.BoardRotation.ROT_270)
                        val aspectRatio = if (isSideways) rows.toFloat() / cols.toFloat() else cols.toFloat() / rows.toFloat()
                        val lastMoveModifiedSquares by viewModel.lastMoveModifiedSquares.collectAsState()
                        val awaitingMatchStart by viewModel.awaitingMatchStart.collectAsState()
                        val gameOver = viewModel.isOnlineGameOver()
                        val humanSeat by viewModel.humanSeat.collectAsState()
                        val vsBot = matchType != OctiAppViewModel.MatchType.LOCAL_PASS_AND_PLAY
                        val botDifficulty by viewModel.botDifficulty.collectAsState()

                        if (matchType == OctiAppViewModel.MatchType.ONLINE_ROOM && activeGameView != null) {
                            val phase = activeGameView?.phase ?: "WAITING_FOR_PLAYERS"
                            val isWaiting = phase == "WAITING_FOR_PLAYERS"
                            val ttlMs = activeGameView?.ttlRemainingMs ?: -1L
                            val ttlStr = if (ttlMs >= 0) {
                                val totalSec = ttlMs / 1000
                                val mm = totalSec / 60
                                val ss = totalSec % 60
                                " • TTL %02d:%02d".format(mm, ss)
                            } else ""
                            val rawRoomId = activeRoomId.ifEmpty { activeGameView?.roomId ?: "" }
                            val shortRoomId = if (rawRoomId.length > 16) rawRoomId.take(16) else rawRoomId
                            val modeTag = activeGameView?.mode ?: "2P"
                            val statusText = if (isWaiting) {
                                "Table #$shortRoomId ($modeTag) • Waiting$ttlStr"
                            } else {
                                "Table #$shortRoomId ($modeTag)$ttlStr"
                            }
                            Card(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .padding(horizontal = 8.dp, vertical = 1.dp),
                                shape = RoundedCornerShape(8.dp),
                                colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
                                border = BorderStroke(1.dp, OctiTheme.colors.accentBrass)
                            ) {
                                Row(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(horizontal = 10.dp, vertical = 4.dp),
                                    horizontalArrangement = Arrangement.SpaceBetween,
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    Text(
                                        text = statusText,
                                        style = MaterialTheme.typography.labelMedium,
                                        fontWeight = FontWeight.Bold,
                                        color = OctiTheme.colors.accentBrass,
                                        maxLines = 1,
                                        overflow = TextOverflow.Ellipsis,
                                        modifier = Modifier
                                            .weight(1f)
                                            .padding(end = 6.dp)
                                    )
                                    Row(
                                        horizontalArrangement = Arrangement.spacedBy(6.dp),
                                        verticalAlignment = Alignment.CenterVertically
                                    ) {
                                        OutlinedButton(
                                            onClick = { viewModel.exitOnlineTableViewAsync() },
                                            modifier = Modifier.height(26.dp),
                                            contentPadding = PaddingValues(horizontal = 8.dp, vertical = 0.dp)
                                        ) {
                                            Text(
                                                text = "Leave",
                                                style = MaterialTheme.typography.labelSmall,
                                                fontWeight = FontWeight.SemiBold
                                            )
                                        }
                                    }
                                }
                            }
                        }

                        // Proposals Banner (Draw, Pause — No Undo or Rematch in Online Games per GDB-10.1a)
                        if (matchType == OctiAppViewModel.MatchType.ONLINE_ROOM && activeGameView?.proposals?.isNotEmpty() == true) {
                            val prop = activeGameView?.proposals?.firstOrNull {
                                it.type != "UNDO_REQUEST" && it.type != "UNDO" &&
                                it.type != "REMATCH_REQUEST" && it.type != "REMATCH"
                            }
                            if (prop != null) {
                                val myUuid = currentProfile?.uuid ?: ""
                                val viewerSeat = activeGameView?.viewerSeat ?: 0
                                val proposerSeat = activeGameView?.seats?.firstOrNull { it.playerUuid == prop.proposerUuid }
                                val proposerHandle = if (proposerSeat != null && proposerSeat.handle.isNotEmpty()) {
                                    proposerSeat.handle
                                } else {
                                    "Opponent"
                                }
                                val cleanHandle = proposerHandle.removePrefix("@")
                                val propTitle = when (prop.type) {
                                    "DRAW_OFFER" -> "Draw offer from @$cleanHandle"
                                    "PAUSE_REQUEST" -> "Pause request from @$cleanHandle"
                                    else -> "Proposal from @$cleanHandle"
                                }
                                Card(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(horizontal = 8.dp, vertical = 1.dp),
                                    shape = RoundedCornerShape(8.dp),
                                    colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
                                    border = BorderStroke(1.dp, OctiTheme.colors.accentBrass)
                                ) {
                                    Row(
                                        modifier = Modifier
                                            .fillMaxWidth()
                                            .padding(horizontal = 10.dp, vertical = 4.dp),
                                        horizontalArrangement = Arrangement.SpaceBetween,
                                        verticalAlignment = Alignment.CenterVertically
                                    ) {
                                        Text(
                                            text = propTitle,
                                            style = MaterialTheme.typography.labelMedium,
                                            fontWeight = FontWeight.Bold,
                                            color = OctiTheme.colors.accentBrass,
                                            maxLines = 1,
                                            overflow = TextOverflow.Ellipsis,
                                            modifier = Modifier
                                                .weight(1f)
                                                .padding(end = 6.dp)
                                        )
                                        Row(
                                            horizontalArrangement = Arrangement.spacedBy(6.dp),
                                            verticalAlignment = Alignment.CenterVertically
                                        ) {
                                            if (viewerSeat > 0 && prop.proposerUuid != myUuid) {
                                                Button(
                                                    onClick = { viewModel.acceptActiveProposal() },
                                                    modifier = Modifier.height(26.dp),
                                                    contentPadding = PaddingValues(horizontal = 8.dp, vertical = 0.dp),
                                                    colors = ButtonDefaults.buttonColors(
                                                        containerColor = OctiTheme.colors.accentBrass,
                                                        contentColor = OctiTheme.colors.windowBg
                                                    )
                                                ) {
                                                    Text("Accept", style = MaterialTheme.typography.labelSmall, fontWeight = FontWeight.Bold)
                                                }
                                                OutlinedButton(
                                                    onClick = { viewModel.declineActiveProposal() },
                                                    modifier = Modifier.height(26.dp),
                                                    contentPadding = PaddingValues(horizontal = 8.dp, vertical = 0.dp)
                                                ) {
                                                    Text("Decline", style = MaterialTheme.typography.labelSmall)
                                                }
                                            } else if (prop.proposerUuid == myUuid) {
                                                OutlinedButton(
                                                    onClick = { viewModel.declineActiveProposal() },
                                                    modifier = Modifier.height(26.dp),
                                                    contentPadding = PaddingValues(horizontal = 8.dp, vertical = 0.dp)
                                                ) {
                                                    Text("Withdraw", style = MaterialTheme.typography.labelSmall)
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Box(
                            modifier = Modifier
                                .fillMaxWidth()
                                .wrapContentHeight(),
                            contentAlignment = Alignment.Center
                        ) {
                            OctiBoardCanvas(
                                viewModel = viewModel,
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .aspectRatio(aspectRatio),
                                lastMoveModifiedSquares = lastMoveModifiedSquares,
                                onUserInteraction = { viewModel.clearLastMoveIndicators() },
                                awaitingMatchStart = awaitingMatchStart
                            )
                            if (awaitingMatchStart && matchType != OctiAppViewModel.MatchType.ONLINE_ROOM && !gameOver) {
                                StartMatchOverlayDialog(
                                    mode = viewModel.board.gameMode,
                                    bases = viewModel.board.game?.bases ?: OctiGameBases.GAME_BASES_3,
                                    useSuperProngs = viewModel.board.game?.useSuperProngs() == true,
                                    useEdgeless = viewModel.board.game?.useEdgelessBoard() == true,
                                    humanSeat = humanSeat,
                                    vsBot = vsBot,
                                    difficulty = botDifficulty,
                                    onChangeType = { viewModel.setShowNewGameDialog(true) },
                                    onStartMatch = { viewModel.confirmStartMatch() }
                                )
                            }
                        }
                        OctiReserveHud(
                            viewModel = viewModel,
                            modifier = Modifier
                                .fillMaxWidth()
                                .weight(1f)
                                .padding(horizontal = 8.dp, vertical = 2.dp),
                            onRequestSitSeat = { seatNum ->
                                if (activeGameView?.hasPasscode == true) {
                                    pendingPasscodeSeat = seatNum
                                } else {
                                    viewModel.joinOnlineTableAsync(activeRoomId, seatNum, "")
                                }
                            }
                        )
                    }
                }
                OctiAppViewModel.AppTab.ONLINE_LOBBY -> {
                    LobbyScreen(
                        viewModel = viewModel,
                        modifier = Modifier.fillMaxSize().then(swipeModifier)
                    )
                }
                OctiAppViewModel.AppTab.LOBBY_CHAT -> {
                    LobbyChatScreen(
                        viewModel = viewModel,
                        modifier = Modifier.fillMaxSize().then(swipeModifier)
                    )
                }
                OctiAppViewModel.AppTab.ARCHIVE_LEADERBOARD -> {
                    ArchiveScreen(
                        viewModel = viewModel,
                        modifier = Modifier.fillMaxSize()
                    )
                }
            }

            val isAwaitingConfirm = (awaitingConfirmation || viewModel.composer.isAwaitingConfirmation) &&
                    currentBoardRev >= 0L &&
                    zoomedSquare == null && capturePrompt == null
            if (isAwaitingConfirm) {
                val hint = if (viewModel.composer.hasContinuationJump()) {
                    "Tap board square to continue jumping"
                } else if (viewModel.composer.hasOtherMovablePodAtOrigin()) {
                    "Tap origin square to move another pod"
                } else null
                PlyConfirmDialog(
                    stagedNotation = stagedMoveNotation.ifEmpty { viewModel.composer.stagedMoveNotation() },
                    subtitleHint = hint,
                    onCommit = {
                        if (viewModel.composer.isGuidedMoveActive) {
                            viewModel.composer.finishGuidedMoveHere()
                        }
                        viewModel.commitStagedMove()
                    },
                    onCancel = { viewModel.rollbackStagedMove() },
                    modifier = Modifier
                        .align(Alignment.BottomCenter)
                        .padding(bottom = 8.dp)
                )
            }

            if (isBotThinking) {
                val botThinkingProgress by viewModel.botThinkingProgress.collectAsState()
                val botThinkingRemainingSec by viewModel.botThinkingRemainingSec.collectAsState()
                val botThinkingDepth by viewModel.botThinkingDepth.collectAsState()
                val botThinkingNodes by viewModel.botThinkingNodes.collectAsState()
                OctiThinkingDialog(
                    level = botDifficulty.level,
                    progress = botThinkingProgress,
                    remainingSec = botThinkingRemainingSec,
                    depth = botThinkingDepth,
                    nodes = botThinkingNodes,
                    budgetMs = viewModel.botEngine.timeBudgetMs
                )
            }

            if (showSplash) {
                OctiSplashOverlay(
                    apkSha256Prefix = apkSha256Prefix,
                    loopIndefinitely = splashLoopIndefinitely,
                    onDismiss = {
                        showSplash = false
                        splashLoopIndefinitely = false
                        onSplashDismissed()
                    }
                )
            }
        }

        if (showNewGameDialog) {
            NewGameSetupDialog(
                viewModel = viewModel,
                onDismiss = { viewModel.setShowNewGameDialog(false) }
            )
        }
        if (showSsoDialog) {
            SsoSignInDialog(
                viewModel = viewModel,
                onDismiss = { viewModel.setShowSsoDialog(false) }
            )
        }
        if (showAccountDialog) {
            AccountIdentitiesDialog(
                viewModel = viewModel,
                onDismiss = { viewModel.setShowAccountDialog(false) }
            )
        }
        if (showCreateTableDialog) {
            CreateTableDialog(
                viewModel = viewModel,
                onDismiss = { viewModel.setShowCreateTableDialog(false) }
            )
        }
        if (showBoardChatZoomDialog) {
            BoardChatZoomDialog(
                viewModel = viewModel,
                onDismiss = { viewModel.setShowBoardChatZoomDialog(false) }
            )
        }
        if (showConnectedUsersDialog) {
            ConnectedUsersAndRobotsDialog(
                viewModel = viewModel,
                onDismiss = { viewModel.dismissConnectedUsersDialog() }
            )
        }
        if (showLobbyChatZoomDialog) {
            LobbyChatZoomDialog(
                viewModel = viewModel,
                onDismiss = { viewModel.dismissLobbyChatZoomDialog() }
            )
        }

        zoomedSquare?.let { (zx, zy) ->
            ZoomedSquareDialog(
                board = viewModel.board,
                composer = viewModel.composer,
                squareX = zx,
                squareY = zy,
                rotation = viewModel.boardRotation.collectAsState().value,
                themeId = themeId,
                canInteract = viewModel.isInteractiveOnlineTurn() && !viewModel.isViewingHistory(),
                onDismiss = { viewModel.closeZoomedSquare() },
                onBeginGuidedMove = { podIndex, moveAllStack ->
                    viewModel.clearLastMoveIndicators()
                    val isOriginWithOther = viewModel.composer.isAwaitingConfirmation &&
                            viewModel.composer.hasOtherMovablePodAtOrigin() &&
                            viewModel.composer.guidedOriginSquare().let { it[0] == zx && it[1] == zy }
                    if (viewModel.composer.isAwaitingConfirmation && !isOriginWithOther) {
                        viewModel.composer.cancelAndRollback()
                    }
                    if (viewModel.composer.beginGuidedPodMove(zx, zy, podIndex, moveAllStack)) {
                        viewModel.closeZoomedSquare()
                        viewModel.bumpBoardRevision()
                    }
                },
                onAddProng = { podIndex, dir, superProng ->
                    viewModel.clearLastMoveIndicators()
                    if (viewModel.composer.isAwaitingConfirmation) {
                        viewModel.composer.cancelAndRollback()
                    }
                    viewModel.composer.selectStackPodIndex(zx, zy, podIndex)
                    viewModel.composer.beginAddProngFromReserve(superProng)
                    if (viewModel.composer.dropOnCompassSector(zx, zy, podIndex, dir)) {
                        if (viewModel.composer.isAwaitingConfirmation) {
                            OctiSoundSynth.playCue(OctiSoundSynth.Cue.PRONG_SNAP, !viewModel.sfxMuted.value)
                        }
                        viewModel.closeZoomedSquare()
                        viewModel.notifyComposerUpdated()
                    }
                },
                onRepositionProng = { podIndex, srcDir, dstDir ->
                    viewModel.clearLastMoveIndicators()
                    if (viewModel.composer.isAwaitingConfirmation) {
                        viewModel.composer.cancelAndRollback()
                    }
                    viewModel.composer.selectStackPodIndex(zx, zy, podIndex)
                    viewModel.composer.beginRepositionProng(zx, zy, podIndex, srcDir)
                    if (viewModel.composer.dropOnCompassSector(zx, zy, podIndex, dstDir)) {
                        if (viewModel.composer.isAwaitingConfirmation) {
                            OctiSoundSynth.playCue(OctiSoundSynth.Cue.PRONG_SNAP, !viewModel.sfxMuted.value)
                        }
                        viewModel.closeZoomedSquare()
                        viewModel.notifyComposerUpdated()
                    }
                },
                onPlaceReservePod = { liberate ->
                    viewModel.clearLastMoveIndicators()
                    if (viewModel.composer.isAwaitingConfirmation) {
                        viewModel.composer.cancelAndRollback()
                    }
                    val canStart = if (liberate) {
                        viewModel.composer.beginLiberatePodFromReserve()
                    } else {
                        viewModel.composer.beginAddPodFromReserve()
                    }
                    if (canStart && viewModel.composer.dropOnSquare(zx, zy)) {
                        if (viewModel.composer.isAwaitingConfirmation) {
                            OctiSoundSynth.playCue(OctiSoundSynth.Cue.POD_PLACE, !viewModel.sfxMuted.value)
                        }
                        viewModel.closeZoomedSquare()
                        viewModel.notifyComposerUpdated()
                    }
                }
            )
        }

        capturePrompt?.let { cp ->
            CapturePromptDialog(
                jumpedX = cp.jumpedX,
                jumpedY = cp.jumpedY,
                player = cp.player,
                onChoice = { capture ->
                    viewModel.resolveCapturePrompt(capture)
                },
                onDismiss = {
                    viewModel.resolveCapturePrompt(false)
                }
            )
        }

        if (showGameOverDialog && (viewModel.board.game?.isGameOver == true || viewModel.isOnlineGameOver())) {
            val isOnline = matchType == OctiAppViewModel.MatchType.ONLINE_ROOM
            val game = viewModel.board.game
            val termReason = if (isOnline) {
                viewModel.describeOnlineTerminationReason()
            } else {
                com.selfso.octi.ui.OctiDialogs.describeTerminationReason(viewModel.board)
            }
            val customTitle = if (isOnline) viewModel.describeOnlineGameOverBanner() else null
            GameOverDialog(
                winner = game?.winner ?: OctiPlayer.PLAYER_A,
                matchLabel = if (isOnline) "" else statusBanner,
                totalPlies = viewModel.livePlyCount(),
                resigned = game?.isGameResigned == true,
                gameMode = viewModel.board.gameMode,
                terminationReason = termReason,
                customTitle = customTitle,
                reviewBoardButtonText = "Review Board",
                onReviewBoard = {
                    viewModel.dismissGameOverDialog()
                }
            )
        }

        pendingPasscodeSeat?.let { seatNum ->
            AlertDialog(
                onDismissRequest = {
                    pendingPasscodeSeat = null
                    joinSeatPasscodeInput = ""
                },
                containerColor = OctiTheme.colors.panelCardBg,
                titleContentColor = OctiTheme.colors.accentBrass,
                textContentColor = OctiTheme.colors.textPrimary,
                title = { Text("Enter Table Passcode") },
                text = {
                    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                        Text("This table requires a passcode to sit.")
                        OutlinedTextField(
                            value = joinSeatPasscodeInput,
                            onValueChange = { joinSeatPasscodeInput = it },
                            label = { Text("Passcode") },
                            singleLine = true,
                            modifier = Modifier.fillMaxWidth()
                        )
                    }
                },
                confirmButton = {
                    Button(
                        onClick = {
                            val code = joinSeatPasscodeInput.trim()
                            pendingPasscodeSeat = null
                            joinSeatPasscodeInput = ""
                            viewModel.joinOnlineTableAsync(activeRoomId, seatNum, code)
                        },
                        colors = ButtonDefaults.buttonColors(
                            containerColor = OctiTheme.colors.accentBrass,
                            contentColor = OctiTheme.colors.windowBg
                        )
                    ) {
                        Text("Sit")
                    }
                },
                dismissButton = {
                    OutlinedButton(onClick = {
                        pendingPasscodeSeat = null
                        joinSeatPasscodeInput = ""
                    }) {
                        Text("Cancel")
                    }
                }
            )
        }

        updateModalMessage?.let { msg ->
            UpToDateModalDialog(
                message = msg,
                apkSha256Prefix = apkSha256Prefix,
                onDismiss = { updateModalMessage = null },
                onReinstall = {
                    updateModalMessage = null
                    updateController.reinstall()
                }
            )
        }

        if (showExportMovesDialog) {
            val movesText = viewModel.exportMoveHistoryText()
            ExportMovesDialog(
                movesText = movesText,
                onCopy = {
                    val clipboard = context.getSystemService(Context.CLIPBOARD_SERVICE) as? ClipboardManager
                    val clip = ClipData.newPlainText("OCTI Transcript", movesText)
                    clipboard?.setPrimaryClip(clip)
                },
                onShare = {
                    val sendIntent = Intent(Intent.ACTION_SEND).apply {
                        type = "application/json"
                        putExtra(Intent.EXTRA_TEXT, movesText)
                    }
                    val shareIntent = Intent.createChooser(sendIntent, "Share OCTI Transcript")
                    context.startActivity(shareIntent)
                },
                onDismiss = { viewModel.setShowExportMovesDialog(false) }
            )
        }

        val showTelemetryConsentDialog by viewModel.showTelemetryConsentDialog.collectAsState()
        val showTelemetrySettingsDialog by viewModel.showTelemetrySettingsDialog.collectAsState()
        val telemetryConsent by viewModel.telemetryConsent.collectAsState()

        if (showTelemetryConsentDialog) {
            OctiTelemetryConsentDialog(
                onChoice = { choice ->
                    viewModel.setTelemetryConsentChoice(choice)
                }
            )
        }

        if (showTelemetrySettingsDialog) {
            val installId = viewModel.configStore.preferences().installId ?: ""
            OctiTelemetrySettingsDialog(
                currentConsent = telemetryConsent,
                installId = installId,
                onToggleShare = { share ->
                    viewModel.setTelemetryConsentChoice(if (share) "share" else "decline")
                },
                onResetInstallId = {
                    viewModel.resetInstallIdAndRequestDeletion()
                },
                onDismiss = {
                    viewModel.setShowTelemetrySettingsDialog(false)
                }
            )
        }

        if (showThemePickerDialog) {
            AlertDialog(
                onDismissRequest = { viewModel.setShowThemePickerDialog(false) },
                containerColor = OctiTheme.colors.panelCardBg,
                title = {
                    Text(
                        text = "Select Board Theme",
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold,
                        color = OctiTheme.colors.accentBrass
                    )
                },
                text = {
                    Column(
                        modifier = Modifier
                            .fillMaxWidth()
                            .verticalScroll(rememberScrollState()),
                        verticalArrangement = Arrangement.spacedBy(6.dp)
                    ) {
                        for (th in OctiThemeId.values()) {
                            val isSelected = (th == themeId)
                            Surface(
                                onClick = {
                                    viewModel.setTheme(th)
                                    viewModel.setShowThemePickerDialog(false)
                                },
                                shape = RoundedCornerShape(8.dp),
                                color = if (isSelected) OctiTheme.colors.accentBrass.copy(alpha = 0.2f) else OctiTheme.colors.windowBg,
                                border = BorderStroke(
                                    1.dp,
                                    if (isSelected) OctiTheme.colors.accentBrass else OctiTheme.colors.panelCardBorder
                                ),
                                modifier = Modifier.fillMaxWidth()
                            ) {
                                Row(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(horizontal = 12.dp, vertical = 10.dp),
                                    horizontalArrangement = Arrangement.SpaceBetween,
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    Text(
                                        text = th.displayName(),
                                        style = MaterialTheme.typography.bodyMedium,
                                        fontWeight = if (isSelected) FontWeight.Bold else FontWeight.Normal,
                                        color = if (isSelected) OctiTheme.colors.accentBrass else OctiTheme.colors.textPrimary
                                    )
                                    if (isSelected) {
                                        Text(
                                            text = "✓",
                                            color = OctiTheme.colors.accentBrass,
                                            fontWeight = FontWeight.Bold
                                        )
                                    }
                                }
                            }
                        }
                    }
                },
                confirmButton = {
                    TextButton(onClick = { viewModel.setShowThemePickerDialog(false) }) {
                        Text("Close", color = OctiTheme.colors.accentBrass)
                    }
                }
            )
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
