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
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.ButtonDefaults
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
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.selfso.octi.ui.AccountIdentitiesDialog
import com.selfso.octi.ui.ArchiveScreen
import com.selfso.octi.ui.CreateTableDialog
import com.selfso.octi.ui.LobbyScreen
import com.selfso.octi.ui.NewGameSetupDialog
import com.selfso.octi.ui.OctiAppViewModel
import com.selfso.octi.ui.OctiBoardCanvas
import com.selfso.octi.ui.OctiReserveHud
import com.selfso.octi.ui.SsoSignInDialog
import com.selfso.octi.ui.theme.OctiMaterialTheme
import com.selfso.octi.ui.theme.OctiTheme
import com.selfso.selfupdate.ui.SelfUpdateBanner
import com.selfso.selfupdate.ui.rememberSelfUpdateController
import java.io.File

class MainActivity : ComponentActivity() {

    lateinit var appViewModel: OctiAppViewModel
        private set

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
                OctiMainAppContent(viewModel = appViewModel)
            }
        }
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
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
fun OctiMainAppContent(viewModel: OctiAppViewModel) {
    val updateController = rememberSelfUpdateController(checkOnLaunch = true)
    val currentTab by viewModel.currentTab.collectAsState()
    val themeId by viewModel.theme.collectAsState()
    val sfxMuted by viewModel.sfxMuted.collectAsState()
    val statusBanner by viewModel.statusBanner.collectAsState()
    val showNewGameDialog by viewModel.showNewGameDialog.collectAsState()
    val showSsoDialog by viewModel.showSsoDialog.collectAsState()
    val showAccountDialog by viewModel.showAccountDialog.collectAsState()
    val showCreateTableDialog by viewModel.showCreateTableDialog.collectAsState()

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
                                color = OctiTheme.colors.accentBrass
                            )
                            if (statusBanner.isNotEmpty()) {
                                Text(
                                    text = statusBanner,
                                    style = MaterialTheme.typography.labelMedium,
                                    color = OctiTheme.colors.textSecondary
                                )
                            }
                        }
                    },
                    actions = {
                        OutlinedButton(
                            onClick = { viewModel.setShowNewGameDialog(true) },
                            modifier = Modifier.padding(end = 4.dp)
                        ) {
                            Text("New Game")
                        }
                        TextButton(
                            onClick = { viewModel.toggleTheme() },
                            colors = ButtonDefaults.textButtonColors(
                                contentColor = OctiTheme.colors.textPrimary
                            )
                        ) {
                            Text(
                                if (themeId == OctiThemeId.OBSIDIAN_BRASS) {
                                    "Obsidian"
                                } else {
                                    "Rosewood"
                                }
                            )
                        }
                        TextButton(
                            onClick = { viewModel.toggleSfxMuted() },
                            colors = ButtonDefaults.textButtonColors(
                                contentColor = OctiTheme.colors.textPrimary
                            )
                        ) {
                            Text(if (sfxMuted) "SFX:Off" else "SFX:On")
                        }
                        TextButton(
                            onClick = { updateController.checkForUpdates() },
                            colors = ButtonDefaults.textButtonColors(
                                contentColor = OctiTheme.colors.accentBrass
                            )
                        ) {
                            Text("Check for Updates")
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
                        modifier = Modifier
                            .fillMaxSize()
                            .padding(8.dp),
                        verticalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        OctiBoardCanvas(
                            viewModel = viewModel,
                            modifier = Modifier
                                .fillMaxWidth()
                                .weight(1f)
                        )
                        OctiReserveHud(
                            viewModel = viewModel,
                            modifier = Modifier.fillMaxWidth()
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
    }
}
