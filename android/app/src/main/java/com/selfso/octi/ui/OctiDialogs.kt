/*
 * OctiDialogs.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Checkbox
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
import com.selfso.octi.OctiBotEngine
import com.selfso.octi.OctiGameBases
import com.selfso.octi.OctiGameMode
import com.selfso.octi.OctiNetworkClient
import com.selfso.octi.OctiPlayer
import com.selfso.octi.ui.theme.OctiTheme

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
