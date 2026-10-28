/*
 * MainActivity.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi

import android.os.Bundle
import android.view.KeyEvent
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
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
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import com.selfso.octi.BuildConfig
import com.selfso.selfupdate.UpdateState
import com.selfso.octi.ui.AccountIdentitiesDialog
import com.selfso.octi.ui.ArchiveScreen
import com.selfso.octi.ui.CapturePromptDialog
import com.selfso.octi.ui.CompositePodMoveDialog
import com.selfso.octi.ui.CreateTableDialog
import com.selfso.octi.ui.GameOverDialog
import com.selfso.octi.ui.LobbyScreen
import com.selfso.octi.ui.NewGameSetupDialog
import com.selfso.octi.ui.OctiAppViewModel
import com.selfso.octi.ui.OctiBoardCanvas
import com.selfso.octi.ui.OctiReserveHud
import com.selfso.octi.ui.OctiSplashOverlay
import com.selfso.octi.ui.PlyConfirmDialog
import com.selfso.octi.ui.SsoSignInDialog
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

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val configFile = ClientConfigStore.configFileForDirectory(filesDir ?: File("."))
        appViewModel = OctiAppViewModel(ClientConfigStore(configFile))

        // Extract intent extras while strictly ignoring any server_url / server_host / server_port override
        val extrasMap = LinkedHashMap<String, String>()
        intent?.extras?.keySet()?.forEach { key ->
            val v = intent?.extras?.getString(key)
            if (v != null) {
                extrasMap[key] = v
            }
        }
        appViewModel.applyExternalArguments(extrasMap)

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

    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
        if (isSplashActive && (keyCode == KeyEvent.KEYCODE_ESCAPE || keyCode == KeyEvent.KEYCODE_BACK)) {
            isSplashActive = false
            return true
        }
        if (::appViewModel.isInitialized && appViewModel.onHardwareKeyEvent(keyCode, true)) {
            return true
        }
        return super.onKeyDown(keyCode, event)
    }

    override fun onKeyUp(keyCode: Int, event: KeyEvent?): Boolean {
        if (::appViewModel.isInitialized && appViewModel.onHardwareKeyEvent(keyCode, false)) {
            return true
        }
        return super.onKeyUp(keyCode, event)
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

    val currentTab by viewModel.currentTab.collectAsState()
    val themeId by viewModel.theme.collectAsState()
    val sfxMuted by viewModel.sfxMuted.collectAsState()
    val statusBanner by viewModel.statusBanner.collectAsState()
    val showNewGameDialog by viewModel.showNewGameDialog.collectAsState()
    val showSsoDialog by viewModel.showSsoDialog.collectAsState()
    val showAccountDialog by viewModel.showAccountDialog.collectAsState()
    val showCreateTableDialog by viewModel.showCreateTableDialog.collectAsState()
    val showGameOverDialog by viewModel.showGameOverDialog.collectAsState()
    val boardRev by viewModel.boardRevision.collectAsState()
    val zoomedSquare by viewModel.zoomedSquare.collectAsState()
    val capturePrompt by viewModel.capturePrompt.collectAsState()
    val isContinuingGuidedJump by viewModel.isContinuingGuidedJump.collectAsState()
    var showSplash by remember { mutableStateOf(true) }
    var showTopMenu by remember { mutableStateOf(false) }

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
                        Row(
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.spacedBy(8.dp)
                        ) {
                            Text(
                                text = "OCTI",
                                style = MaterialTheme.typography.headlineMedium,
                                fontWeight = FontWeight.ExtraBold,
                                color = OctiTheme.colors.accentBrass,
                                maxLines = 1
                            )
                            if (statusBanner.isNotEmpty()) {
                                Text(
                                    text = statusBanner,
                                    style = MaterialTheme.typography.labelMedium,
                                    color = OctiTheme.colors.textSecondary,
                                    maxLines = 1,
                                    overflow = TextOverflow.Ellipsis
                                )
                            }
                        }
                    },
                    actions = {
                        OutlinedButton(
                            onClick = { viewModel.setShowNewGameDialog(true) },
                            modifier = Modifier.padding(end = 4.dp)
                        ) {
                            Text("New Game", maxLines = 1)
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
                                            "Theme: " + if (themeId == OctiThemeId.OBSIDIAN_BRASS) {
                                                "Obsidian"
                                            } else {
                                                "Rosewood"
                                            }
                                        )
                                    },
                                    onClick = {
                                        showTopMenu = false
                                        viewModel.toggleTheme()
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
                                    text = { Text("About OCTI (Splash)") },
                                    onClick = {
                                        showTopMenu = false
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
        },
        bottomBar = {
            NavigationBar(
                containerColor = OctiTheme.colors.panelCardBg,
                contentColor = OctiTheme.colors.textPrimary
            ) {
                NavigationBarItem(
                    selected = currentTab == OctiAppViewModel.AppTab.BOARD,
                    onClick = { viewModel.selectTab(OctiAppViewModel.AppTab.BOARD) },
                    icon = { Text("◈", color = OctiTheme.colors.accentBrass) },
                    label = { Text("Tabletop Board") }
                )
                NavigationBarItem(
                    selected = currentTab == OctiAppViewModel.AppTab.ONLINE_LOBBY,
                    onClick = {
                        viewModel.selectTab(OctiAppViewModel.AppTab.ONLINE_LOBBY)
                        viewModel.refreshLobbyState()
                    },
                    icon = { Text("⚡", color = OctiTheme.colors.accentBrass) },
                    label = { Text("Online Lobby") }
                )
                NavigationBarItem(
                    selected = currentTab == OctiAppViewModel.AppTab.ARCHIVE_LEADERBOARD,
                    onClick = {
                        viewModel.selectTab(OctiAppViewModel.AppTab.ARCHIVE_LEADERBOARD)
                        viewModel.refreshLeaderboard("2P", 50)
                        viewModel.refreshArchivedGames("", "", 20, 0)
                    },
                    icon = { Text("★", color = OctiTheme.colors.accentBrass) },
                    label = { Text("Archive & Elo") }
                )
            }
        }
    ) { innerPadding ->
        Box(
            modifier = Modifier
                .fillMaxSize()
                .padding(innerPadding)
                .background(OctiTheme.colors.windowBg)
        ) {
            when (currentTab) {
                OctiAppViewModel.AppTab.BOARD -> {
                    Column(
                        modifier = Modifier.fillMaxSize(),
                        verticalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        val isGameOver = viewModel.board.game?.isGameOver == true && boardRev >= 0L
                        if (isGameOver) {
                            Card(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .padding(horizontal = 8.dp, vertical = 2.dp),
                                shape = RoundedCornerShape(10.dp),
                                colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.panelCardBg),
                                border = BorderStroke(1.5.dp, OctiTheme.colors.accentBrass)
                            ) {
                                Row(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(horizontal = 12.dp, vertical = 6.dp),
                                    horizontalArrangement = Arrangement.SpaceBetween,
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    Text(
                                        text = "GAME OVER — Winner: ${viewModel.board.game?.winner}",
                                        style = MaterialTheme.typography.titleMedium,
                                        fontWeight = FontWeight.ExtraBold,
                                        color = OctiTheme.colors.accentBrass
                                    )
                                    Button(
                                        onClick = { viewModel.setShowNewGameDialog(true) },
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
                        val cols = viewModel.board.boardWidth
                        val rows = viewModel.board.boardHeight
                        val aspectRatio = cols.toFloat() / rows.toFloat()
                        OctiBoardCanvas(
                            viewModel = viewModel,
                            modifier = Modifier
                                .fillMaxWidth()
                                .aspectRatio(aspectRatio)
                        )
                        OctiReserveHud(
                            viewModel = viewModel,
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(horizontal = 8.dp)
                        )
                    }
                }
                OctiAppViewModel.AppTab.ONLINE_LOBBY -> {
                    LobbyScreen(
                        viewModel = viewModel,
                        modifier = Modifier.fillMaxSize()
                    )
                }
                OctiAppViewModel.AppTab.ARCHIVE_LEADERBOARD -> {
                    ArchiveScreen(
                        viewModel = viewModel,
                        modifier = Modifier.fillMaxSize()
                    )
                }
            }

            val isAwaitingConfirm = viewModel.composer.isAwaitingConfirmation &&
                    boardRev >= 0L && zoomedSquare == null && capturePrompt == null
            if (isAwaitingConfirm) {
                PlyConfirmDialog(
                    stagedNotation = viewModel.composer.stagedMoveNotation(),
                    canMoreMoves = viewModel.composer.canAddMoreMoves(),
                    onCommit = {
                        if (viewModel.composer.isGuidedMoveActive) {
                            viewModel.composer.finishGuidedMoveHere()
                        }
                        viewModel.commitStagedMove()
                    },
                    onCancel = { viewModel.rollbackStagedMove() },
                    onMoreMoves = {
                        if (viewModel.composer.hasUnlockedSuperProngBonus() && !viewModel.composer.isGuidedMoveActive) {
                            val orig = viewModel.composer.eighthProngOriginSquare()
                            viewModel.composer.beginGuidedPodMove(orig[0], orig[1], -1, false)
                            viewModel.bumpBoardRevision()
                        } else if (viewModel.composer.hasContinuationJump()) {
                            viewModel.resumeGuidedJumpSelection()
                        } else if (viewModel.composer.hasOtherMovablePodAtOrigin()) {
                            val orig = viewModel.composer.guidedOriginSquare()
                            viewModel.openZoomedSquare(orig[0], orig[1])
                        }
                    },
                    modifier = Modifier
                        .align(Alignment.BottomCenter)
                        .padding(bottom = 8.dp)
                )
            }

            if (showSplash) {
                OctiSplashOverlay(
                    onDismiss = {
                        showSplash = false
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

        zoomedSquare?.let { (zx, zy) ->
            ZoomedSquareDialog(
                board = viewModel.board,
                composer = viewModel.composer,
                squareX = zx,
                squareY = zy,
                rotation = viewModel.boardRotation.collectAsState().value,
                themeId = themeId,
                onDismiss = { viewModel.closeZoomedSquare() },
                onBeginGuidedMove = { podIndex, moveAllStack ->
                    if (viewModel.composer.isAwaitingConfirmation) {
                        viewModel.composer.cancelAndRollback()
                    }
                    if (viewModel.composer.beginGuidedPodMove(zx, zy, podIndex, moveAllStack)) {
                        viewModel.closeZoomedSquare()
                        viewModel.bumpBoardRevision()
                    }
                },
                onAddProng = { podIndex, dir, superProng ->
                    if (viewModel.composer.isAwaitingConfirmation) {
                        viewModel.composer.cancelAndRollback()
                    }
                    viewModel.composer.selectStackPodIndex(zx, zy, podIndex)
                    viewModel.composer.beginAddProngFromReserve(superProng)
                    if (viewModel.composer.dropOnCompassSector(zx, zy, podIndex, dir)) {
                        viewModel.closeZoomedSquare()
                        viewModel.notifyComposerUpdated()
                    }
                },
                onRepositionProng = { podIndex, srcDir, dstDir ->
                    if (viewModel.composer.isAwaitingConfirmation) {
                        viewModel.composer.cancelAndRollback()
                    }
                    viewModel.composer.selectStackPodIndex(zx, zy, podIndex)
                    viewModel.composer.beginRepositionProng(zx, zy, podIndex, srcDir)
                    if (viewModel.composer.dropOnCompassSector(zx, zy, podIndex, dstDir)) {
                        viewModel.closeZoomedSquare()
                        viewModel.notifyComposerUpdated()
                    }
                },
                onPlaceReservePod = { liberate ->
                    if (viewModel.composer.isAwaitingConfirmation) {
                        viewModel.composer.cancelAndRollback()
                    }
                    val canStart = if (liberate) {
                        viewModel.composer.beginLiberatePodFromReserve()
                    } else {
                        viewModel.composer.beginAddPodFromReserve()
                    }
                    if (canStart && viewModel.composer.dropOnSquare(zx, zy)) {
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

        if (showGameOverDialog && viewModel.board.game?.isGameOver == true) {
            GameOverDialog(
                winner = viewModel.board.game?.winner ?: OctiPlayer.PLAYER_A,
                matchLabel = statusBanner,
                totalPlies = viewModel.livePlyCount(),
                resigned = viewModel.board.game?.isGameResigned == true,
                onNewGame = {
                    viewModel.dismissGameOverDialog()
                    viewModel.setShowNewGameDialog(true)
                },
                onReviewBoard = {
                    viewModel.dismissGameOverDialog()
                }
            )
        }

        updateModalMessage?.let { msg ->
            UpToDateModalDialog(
                message = msg,
                onDismiss = { updateModalMessage = null },
                onReinstall = {
                    updateModalMessage = null
                    updateController.reinstall()
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
