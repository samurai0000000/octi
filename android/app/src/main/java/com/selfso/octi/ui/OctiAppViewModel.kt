/*
 * OctiAppViewModel.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import android.view.KeyEvent
import androidx.lifecycle.ViewModel
import com.selfso.octi.BuildConfig
import com.selfso.octi.ClientConfigStore
import com.selfso.octi.OctiBoard
import com.selfso.octi.OctiBoardGeometry
import com.selfso.octi.OctiBoardRenderer
import com.selfso.octi.OctiBotEngine
import com.selfso.octi.OctiCrashReporter
import com.selfso.octi.OctiDefs
import com.selfso.octi.OctiGame
import com.selfso.octi.OctiGameBases
import com.selfso.octi.OctiGameMode
import com.selfso.octi.OctiMove
import com.selfso.octi.OctiMoveComposer
import com.selfso.octi.OctiNetworkClient
import com.selfso.octi.OctiPlayer
import com.selfso.octi.OctiSoundSynth
import com.selfso.octi.OctiThemeId
import com.selfso.octi.P2pVoiceEngine
import com.selfso.selfupdate.ApkDownloader
import com.selfso.selfupdate.UpdateManifest
import java.io.File
import java.util.Collections
import java.util.Locale
import java.util.TimeZone
import java.util.concurrent.CopyOnWriteArrayList
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.runBlocking
import org.json.JSONObject

/**
 * Central application controller and state holder for the OCTI Android client.
 *
 * Manages local Pass-and-Play / AI bot games, 4-quadrant board rotation,
 * VCR move-history scrubbing, hard-coded `trumpet.selfso.com:6874` online
 * play (5-provider SSO, multi-provider identity linking, lobby, custom tables,
 * Elo matchmaking, live SSE games, `ALL` / `TEAM` chat, moderation, archive),
 * and Push-to-Talk peer-to-peer UDP voice chat.
 */
class OctiAppViewModel @JvmOverloads constructor(
    val configStore: ClientConfigStore = ClientConfigStore(),
    val netClient: OctiNetworkClient = OctiNetworkClient()
) : ViewModel(), AutoCloseable {

    enum class AppTab {
        BOARD,
        ONLINE_LOBBY,
        ARCHIVE_LEADERBOARD
    }

    enum class MatchType {
        LOCAL_PASS_AND_PLAY,
        LOCAL_VS_BOT,
        ONLINE_ROOM,
        ARCHIVE_REPLAY
    }

    data class GameChatMessage(
        val seq: Long,
        val scope: String,
        val senderUuid: String,
        val senderHandle: String,
        val seat: Int,
        val text: String,
        val atMs: Long
    )

    companion object {
        @JvmStatic
        fun canonicalServerEndpoint(): String {
            return "${OctiDefs.OCTI_SERVER_HOST}:${OctiDefs.OCTI_SERVER_PORT}"
        }

        @JvmStatic
        fun canonicalServerHost(): String {
            return OctiDefs.OCTI_SERVER_HOST
        }

        @JvmStatic
        fun canonicalServerPort(): Int {
            return OctiDefs.OCTI_SERVER_PORT
        }

        @JvmStatic
        fun hasEditableServerOrPortField(): Boolean {
            return false
        }

        @JvmStatic
        fun modeFromWireCode(code: String?): OctiGameMode {
            if (code == null) {
                return OctiGameMode.GAME_MODE_2
            }
            val u = code.trim().uppercase()
            return when {
                u.startsWith("4") -> OctiGameMode.GAME_MODE_4
                u == "OFK" || u == "KIDS" -> OctiGameMode.GAME_MODE_OFK
                else -> OctiGameMode.GAME_MODE_2
            }
        }

        @JvmStatic
        fun modeToWireCode(mode: OctiGameMode): String {
            return when (mode) {
                OctiGameMode.GAME_MODE_4 -> "4P"
                OctiGameMode.GAME_MODE_OFK -> "OFK"
                else -> "2P"
            }
        }

        @JvmStatic
        fun downloadAndVerifyUpdateBlocking(
            cacheRootDir: File,
            manifest: UpdateManifest
        ): File = runBlocking {
            ApkDownloader(cacheRootDir).downloadAndVerify(manifest)
        }

        @Volatile
        private var cachedApkSha256Prefix: String = ""

        @JvmStatic
        fun computeInstalledApkSha256Prefix(context: android.content.Context?, hexLen: Int = 12): String {
            if (cachedApkSha256Prefix.isNotEmpty()) {
                return cachedApkSha256Prefix.take(hexLen)
            }
            return try {
                val apkPath = context?.applicationInfo?.sourceDir ?: return "dev-local"
                val apkFile = File(apkPath)
                if (!apkFile.isFile || apkFile.length() <= 0L) {
                    return "dev-local"
                }
                val digest = java.security.MessageDigest.getInstance("SHA-256")
                apkFile.inputStream().use { input ->
                    val buf = ByteArray(16 * 1024)
                    var n: Int
                    while (input.read(buf).also { n = it } != -1) {
                        digest.update(buf, 0, n)
                    }
                }
                val fullHex = digest.digest().joinToString("") { "%02x".format(it) }
                cachedApkSha256Prefix = fullHex
                fullHex.take(hexLen)
            } catch (_: Throwable) {
                "dev-local"
            }
        }
    }

    val soundSynth: OctiSoundSynth = OctiSoundSynth.getDefaultInstance()
    val botEngine: OctiBotEngine = OctiBotEngine(OctiBotEngine.BotDifficulty.TACTICAL)
    val voiceEngine: P2pVoiceEngine = P2pVoiceEngine(netClient)

    val board: OctiBoard = OctiBoard()
    val displayBoard: OctiBoard = OctiBoard()
    val composer: OctiMoveComposer = OctiMoveComposer(board)
    val geometry: OctiBoardGeometry = OctiBoardGeometry()

    private val _fenHistory = CopyOnWriteArrayList<String>()
    private val _moveHistory = CopyOnWriteArrayList<String>()

    private val _currentTab = MutableStateFlow(AppTab.BOARD)
    val currentTab: StateFlow<AppTab> = _currentTab.asStateFlow()

    private val _matchType = MutableStateFlow(MatchType.LOCAL_VS_BOT)
    val matchType: StateFlow<MatchType> = _matchType.asStateFlow()

    private val _theme = MutableStateFlow(OctiThemeId.OBSIDIAN_BRASS)
    val theme: StateFlow<OctiThemeId> = _theme.asStateFlow()

    private val _confirmAllMoves = MutableStateFlow(true)
    val confirmAllMoves: StateFlow<Boolean> = _confirmAllMoves.asStateFlow()

    private val _moveEntireMatchingStack = MutableStateFlow(false)
    val moveEntireMatchingStack: StateFlow<Boolean> = _moveEntireMatchingStack.asStateFlow()

    private val _sfxMuted = MutableStateFlow(false)
    val sfxMuted: StateFlow<Boolean> = _sfxMuted.asStateFlow()

    private val _volumeKeyPtt = MutableStateFlow(true)
    val volumeKeyPtt: StateFlow<Boolean> = _volumeKeyPtt.asStateFlow()

    private val _boardRotation = MutableStateFlow(OctiBoardGeometry.BoardRotation.ROT_0)
    val boardRotation: StateFlow<OctiBoardGeometry.BoardRotation> = _boardRotation.asStateFlow()

    private val _autoRotateSeatView = MutableStateFlow(true)
    val autoRotateSeatView: StateFlow<Boolean> = _autoRotateSeatView.asStateFlow()

    private val _botDifficulty = MutableStateFlow(OctiBotEngine.BotDifficulty.TACTICAL)
    val botDifficulty: StateFlow<OctiBotEngine.BotDifficulty> = _botDifficulty.asStateFlow()

    private val _humanSeat = MutableStateFlow(OctiPlayer.PLAYER_A)
    val humanSeat: StateFlow<OctiPlayer> = _humanSeat.asStateFlow()

    private val _viewingPly = MutableStateFlow(0)
    val viewingPly: StateFlow<Int> = _viewingPly.asStateFlow()

    private val _boardRevision = MutableStateFlow(0L)
    val boardRevision: StateFlow<Long> = _boardRevision.asStateFlow()

    private val _awaitingConfirmation = MutableStateFlow(false)
    val awaitingConfirmation: StateFlow<Boolean> = _awaitingConfirmation.asStateFlow()

    private val _stagedMoveNotation = MutableStateFlow("")
    val stagedMoveNotation: StateFlow<String> = _stagedMoveNotation.asStateFlow()

    private val _canAddMoreMoves = MutableStateFlow(false)
    val canAddMoreMoves: StateFlow<Boolean> = _canAddMoreMoves.asStateFlow()

    private val _guidedMoveActive = MutableStateFlow(false)
    val guidedMoveActive: StateFlow<Boolean> = _guidedMoveActive.asStateFlow()

    val LAST_MOVE_INDICATOR_DURATION_MS: Long = OctiBoardRenderer.LAST_MOVE_INDICATOR_DURATION_MS

    private val _lastMoveModifiedSquares = MutableStateFlow<List<IntArray>>(emptyList())
    val lastMoveModifiedSquares: StateFlow<List<IntArray>> = _lastMoveModifiedSquares.asStateFlow()
    private var _lastMoveIndicatorSetAtMs: Long = 0L

    fun getLastMoveModifiedSquaresList(): List<IntArray> {
        return _lastMoveModifiedSquares.value
    }

    fun getLastMoveIndicatorSetAtMs(): Long = _lastMoveIndicatorSetAtMs

    fun expireLastMoveIndicatorsIfElapsed(nowMs: Long = System.currentTimeMillis()): Boolean {
        if (_lastMoveModifiedSquares.value.isEmpty()) {
            return false
        }
        if (nowMs - _lastMoveIndicatorSetAtMs >= LAST_MOVE_INDICATOR_DURATION_MS) {
            _lastMoveModifiedSquares.value = emptyList()
            _lastMoveIndicatorSetAtMs = 0L
            return true
        }
        return false
    }

    fun clearLastMoveIndicators() {
        if (_lastMoveModifiedSquares.value.isNotEmpty()) {
            _lastMoveModifiedSquares.value = emptyList()
            _lastMoveIndicatorSetAtMs = 0L
        }
    }

    private val _statusBanner = MutableStateFlow("")
    val statusBanner: StateFlow<String> = _statusBanner.asStateFlow()

    // Online Authentication & Account Linking State
    private val _isAuthenticated = MutableStateFlow(false)
    val isAuthenticated: StateFlow<Boolean> = _isAuthenticated.asStateFlow()

    private val _onboardingRequired = MutableStateFlow(false)
    val onboardingRequired: StateFlow<Boolean> = _onboardingRequired.asStateFlow()

    private val _currentProfile = MutableStateFlow<OctiNetworkClient.PlayerProfileInfo?>(null)
    val currentProfile: StateFlow<OctiNetworkClient.PlayerProfileInfo?> = _currentProfile.asStateFlow()

    private val _linkedIdentities =
        MutableStateFlow<List<OctiNetworkClient.LinkedIdentityInfo>>(emptyList())
    val linkedIdentities: StateFlow<List<OctiNetworkClient.LinkedIdentityInfo>> =
        _linkedIdentities.asStateFlow()

    private val _authProviders =
        MutableStateFlow<List<OctiNetworkClient.AuthProviderInfo>>(emptyList())
    val authProviders: StateFlow<List<OctiNetworkClient.AuthProviderInfo>> =
        _authProviders.asStateFlow()

    private val _linkConflictProfile =
        MutableStateFlow<OctiNetworkClient.PlayerProfileInfo?>(null)
    val linkConflictProfile: StateFlow<OctiNetworkClient.PlayerProfileInfo?> =
        _linkConflictProfile.asStateFlow()

    private var _pendingConflictProvider: String = ""
    private var _pendingConflictToken: String = ""

    // Online Lobby, Matchmaking, Live Room & Archive State
    private val _lobbyState = MutableStateFlow(OctiNetworkClient.LobbyStateData())
    val lobbyState: StateFlow<OctiNetworkClient.LobbyStateData> = _lobbyState.asStateFlow()

    private val _matchmakingStatus = MutableStateFlow(OctiNetworkClient.MatchmakingStatusData())
    val matchmakingStatus: StateFlow<OctiNetworkClient.MatchmakingStatusData> =
        _matchmakingStatus.asStateFlow()

    private val _activeRoomId = MutableStateFlow("")
    val activeRoomId: StateFlow<String> = _activeRoomId.asStateFlow()

    private val _activeGameView = MutableStateFlow<OctiNetworkClient.GameViewData?>(null)
    val activeGameView: StateFlow<OctiNetworkClient.GameViewData?> = _activeGameView.asStateFlow()

    private val _gameChatLog = MutableStateFlow<List<GameChatMessage>>(emptyList())
    val gameChatLog: StateFlow<List<GameChatMessage>> = _gameChatLog.asStateFlow()

    private val _isPttActive = MutableStateFlow(false)
    val isPttActive: StateFlow<Boolean> = _isPttActive.asStateFlow()

    private val _isInVoiceRoom = MutableStateFlow(false)
    val isInVoiceRoom: StateFlow<Boolean> = _isInVoiceRoom.asStateFlow()

    private val _leaderboardRows =
        MutableStateFlow<List<OctiNetworkClient.LeaderboardRow>>(emptyList())
    val leaderboardRows: StateFlow<List<OctiNetworkClient.LeaderboardRow>> =
        _leaderboardRows.asStateFlow()

    private val _archivedGamesPage = MutableStateFlow(OctiNetworkClient.ArchivedGamesPage())
    val archivedGamesPage: StateFlow<OctiNetworkClient.ArchivedGamesPage> =
        _archivedGamesPage.asStateFlow()

    // Dialog visibility flags for UI
    private val _showNewGameDialog = MutableStateFlow(false)
    val showNewGameDialog: StateFlow<Boolean> = _showNewGameDialog.asStateFlow()

    private val _showSsoDialog = MutableStateFlow(false)
    val showSsoDialog: StateFlow<Boolean> = _showSsoDialog.asStateFlow()

    private val _showAccountDialog = MutableStateFlow(false)
    val showAccountDialog: StateFlow<Boolean> = _showAccountDialog.asStateFlow()

    private val _showCreateTableDialog = MutableStateFlow(false)
    val showCreateTableDialog: StateFlow<Boolean> = _showCreateTableDialog.asStateFlow()

    data class CapturePromptData(val jumpedX: Int, val jumpedY: Int, val player: OctiPlayer)

    private val _zoomedSquare = MutableStateFlow<Pair<Int, Int>?>(null)
    val zoomedSquare: StateFlow<Pair<Int, Int>?> = _zoomedSquare.asStateFlow()

    private val _capturePrompt = MutableStateFlow<CapturePromptData?>(null)
    val capturePrompt: StateFlow<CapturePromptData?> = _capturePrompt.asStateFlow()

    private val _showSuperProngBonusDialog = MutableStateFlow(false)
    val showSuperProngBonusDialog: StateFlow<Boolean> = _showSuperProngBonusDialog.asStateFlow()

    private val _compositePodPrompt = MutableStateFlow<Pair<Int, Int>?>(null)
    val compositePodPrompt: StateFlow<Pair<Int, Int>?> = _compositePodPrompt.asStateFlow()

    private val _isContinuingGuidedJump = MutableStateFlow(false)
    val isContinuingGuidedJump: StateFlow<Boolean> = _isContinuingGuidedJump.asStateFlow()

    private val _showGameOverDialog = MutableStateFlow(false)
    val showGameOverDialog: StateFlow<Boolean> = _showGameOverDialog.asStateFlow()

    private val _showExportMovesDialog = MutableStateFlow(false)
    val showExportMovesDialog: StateFlow<Boolean> = _showExportMovesDialog.asStateFlow()

    private val _isBotThinking = MutableStateFlow(false)
    val isBotThinking: StateFlow<Boolean> = _isBotThinking.asStateFlow()

    private val _botThinkingProgress = MutableStateFlow(0f)
    val botThinkingProgress: StateFlow<Float> = _botThinkingProgress.asStateFlow()

    private val _botThinkingRemainingSec = MutableStateFlow(0f)
    val botThinkingRemainingSec: StateFlow<Float> = _botThinkingRemainingSec.asStateFlow()

    private val _botThinkingDepth = MutableStateFlow(0)
    val botThinkingDepth: StateFlow<Int> = _botThinkingDepth.asStateFlow()

    private val _botThinkingNodes = MutableStateFlow(0L)
    val botThinkingNodes: StateFlow<Long> = _botThinkingNodes.asStateFlow()

    private val _botThinkingScore = MutableStateFlow(0)
    val botThinkingScore: StateFlow<Int> = _botThinkingScore.asStateFlow()

    private val _telemetryConsent = MutableStateFlow(configStore.preferences().telemetryConsent ?: "unset")
    val telemetryConsent: StateFlow<String> = _telemetryConsent.asStateFlow()

    private val _telemetryConsentVersion = MutableStateFlow(configStore.preferences().telemetryConsentVersion)
    val telemetryConsentVersion: StateFlow<Int> = _telemetryConsentVersion.asStateFlow()

    private val _installId = MutableStateFlow(configStore.preferences().installId ?: "")
    val installId: StateFlow<String> = _installId.asStateFlow()

    private val _showTelemetryConsentDialog = MutableStateFlow(false)
    val showTelemetryConsentDialog: StateFlow<Boolean> = _showTelemetryConsentDialog.asStateFlow()

    private val _showTelemetrySettingsDialog = MutableStateFlow(false)
    val showTelemetrySettingsDialog: StateFlow<Boolean> = _showTelemetrySettingsDialog.asStateFlow()

    private val _awaitingMatchStart = MutableStateFlow(false)
    val awaitingMatchStart: StateFlow<Boolean> = _awaitingMatchStart.asStateFlow()
    private var _requireStartMatchPrompt: Boolean = false

    fun setRequireStartMatchPrompt(enabled: Boolean) {
        _requireStartMatchPrompt = enabled
    }

    fun isAwaitingMatchStart(): Boolean = _awaitingMatchStart.value

    private var _matchStartedEpochSec: Long = 0L
    private var _currentTurnStartEpochMs: Long = 0L
    private var _playerATotalThinkMs: Long = 0L
    private var _playerBTotalThinkMs: Long = 0L
    private val _turnElapsedSec = MutableStateFlow(0)
    val turnElapsedSec: StateFlow<Int> = _turnElapsedSec.asStateFlow()
    private val _totalGameSec = MutableStateFlow(0)
    val totalGameSec: StateFlow<Int> = _totalGameSec.asStateFlow()

    fun matchStartedEpochSec(): Long = _matchStartedEpochSec
    fun currentTurnStartEpochMs(): Long = _currentTurnStartEpochMs

    fun beginAwaitingMatchStart() {
        _awaitingMatchStart.value = true
        _matchStartedEpochSec = 0L
        _currentTurnStartEpochMs = 0L
        _playerATotalThinkMs = 0L
        _playerBTotalThinkMs = 0L
        _turnElapsedSec.value = 0
        _totalGameSec.value = 0
        cancelActiveBotTurn()
    }

    fun confirmStartMatch() {
        if (!_awaitingMatchStart.value) return
        _awaitingMatchStart.value = false
        _matchStartedEpochSec = System.currentTimeMillis() / 1000L
        _currentTurnStartEpochMs = System.currentTimeMillis()
        soundSynth.triggerCue(OctiSoundSynth.SoundCue.POD_PLACE)
        refreshSnapshots()
        val g = board.game
        if (_matchType.value == MatchType.LOCAL_VS_BOT && g != null && !g.isGameOver && g.turn != _humanSeat.value) {
            if (_asyncBotThinkingEnabled.value) {
                startAsyncBotTurn()
            } else {
                executeBotTurnIfNeeded()
            }
        }
    }

    fun tickClock() {
        if (_awaitingMatchStart.value) {
            _turnElapsedSec.value = 0
            _totalGameSec.value = 0
            return
        }
        val now = System.currentTimeMillis()
        if (_matchStartedEpochSec > 0L) {
            _totalGameSec.value = ((now / 1000L) - _matchStartedEpochSec).toInt().coerceAtLeast(0)
        }
        if (_currentTurnStartEpochMs > 0L) {
            _turnElapsedSec.value = ((now - _currentTurnStartEpochMs) / 1000L).toInt().coerceAtLeast(0)
        }
    }

    class BoardSnapshot(
        val moveCount: Int,
        val turn: OctiPlayer,
        val isGameOver: Boolean
    )

    fun currentBoardSnapshot(): BoardSnapshot {
        val g = board.game
        return BoardSnapshot(
            moveCount = g?.numMoves ?: 0,
            turn = g?.turn ?: OctiPlayer.PLAYER_A,
            isGameOver = g?.isGameOver ?: false
        )
    }

    fun refreshSnapshots() {
        displayBoard.fromPositionString(board.toPositionString())
        bumpBoardRevision()
    }

    private var _transcriptUploadedForCurrentMatch: Boolean = false
    private var _heldTranscriptPendingConsent: Boolean = false
    private var storageRootDir: File = configStore.configFile().parentFile ?: File(".")

    fun setStorageRootDir(dir: File) {
        storageRootDir = dir
        OctiCrashReporter.setStorageRootDir(dir)
    }

    fun getStorageRootDir(): File = storageRootDir

    fun loadConfig(file: File) {
        storageRootDir = file.parentFile ?: storageRootDir
        OctiCrashReporter.setStorageRootDir(storageRootDir)
        configStore.load()
    }

    fun setShowTelemetryConsentDialog(show: Boolean) {
        _showTelemetryConsentDialog.value = show
    }

    fun setShowTelemetrySettingsDialog(show: Boolean) {
        _showTelemetrySettingsDialog.value = show
    }

    fun setTelemetryConsentChoice(choice: String) {
        configStore.preferences().telemetryConsent = choice
        configStore.preferences().telemetryConsentVersion = ClientConfigStore.ClientPreferences.CURRENT_TELEMETRY_CONSENT_VERSION
        if (choice == "share" && configStore.preferences().installId.isEmpty()) {
            configStore.preferences().installId = ClientConfigStore.generateUuidV4()
        }
        configStore.save()
        _telemetryConsent.value = choice
        _telemetryConsentVersion.value = ClientConfigStore.ClientPreferences.CURRENT_TELEMETRY_CONSENT_VERSION
        _installId.value = configStore.preferences().installId ?: ""
        _showTelemetryConsentDialog.value = false

        if (_heldTranscriptPendingConsent) {
            _heldTranscriptPendingConsent = false
            uploadCompletedBotGameIfEligible()
        }
    }

    fun resetInstallIdAndRequestDeletion() {
        val oldId = configStore.preferences().installId
        if (!oldId.isNullOrEmpty()) {
            Thread({
                try {
                    netClient.deleteInstallArchiveData(oldId)
                } catch (_: Throwable) {
                }
            }, "OctiDeleteInstallDataThread").apply {
                isDaemon = true
                start()
            }
        }
        configStore.resetInstallId()
        configStore.save()
        _installId.value = configStore.preferences().installId ?: ""
    }

    private val _asyncBotThinkingEnabled = MutableStateFlow(false)
    val asyncBotThinkingEnabled: StateFlow<Boolean> = _asyncBotThinkingEnabled.asStateFlow()

    @Volatile
    private var _activeBotThinkingThread: Thread? = null

    private val _botTurnGeneration = java.util.concurrent.atomic.AtomicInteger(0)

    fun cancelActiveBotTurn() {
        _botTurnGeneration.incrementAndGet()
        _isBotThinking.value = false
        _botThinkingProgress.value = 0f
        _botThinkingRemainingSec.value = 0f
        _botThinkingDepth.value = 0
        _botThinkingNodes.value = 0L
        _botThinkingScore.value = 0
        val active = _activeBotThinkingThread
        _activeBotThinkingThread = null
        if (active != null && active !== Thread.currentThread()) {
            try {
                active.interrupt()
            } catch (_: Throwable) {
            }
        }
    }

    private fun runOnUiThreadOrSync(action: () -> Unit) {
        val vmName = (System.getProperty("java.vm.name") ?: "").lowercase()
        val isAndroidVm = vmName.contains("dalvik") || vmName.contains("art")
        if (isAndroidVm) {
            try {
                val mainLooper = android.os.Looper.getMainLooper()
                if (mainLooper != null) {
                    if (Thread.currentThread() === mainLooper.thread) {
                        synchronized(board) {
                            action()
                        }
                    } else {
                        android.os.Handler(mainLooper).post {
                            synchronized(board) {
                                action()
                            }
                        }
                    }
                    return
                }
            } catch (_: Throwable) {
            }
        }
        synchronized(board) {
            action()
        }
    }

    fun setShowExportMovesDialog(show: Boolean) {
        _showExportMovesDialog.value = show
    }

    fun setAsyncBotThinkingEnabled(enabled: Boolean) {
        _asyncBotThinkingEnabled.value = enabled
    }

    @JvmOverloads
    fun updateBotThinkingProgressForTest(
        thinking: Boolean,
        progress: Float,
        remainingSec: Float,
        depth: Int = 0,
        nodes: Long = 0L,
        score: Int = 0
    ) {
        _isBotThinking.value = thinking
        _botThinkingProgress.value = progress
        _botThinkingRemainingSec.value = remainingSec
        _botThinkingDepth.value = depth
        _botThinkingNodes.value = nodes
        _botThinkingScore.value = score
    }

    fun openZoomedSquare(x: Int, y: Int) {
        clearLastMoveIndicators()
        _isContinuingGuidedJump.value = false
        _zoomedSquare.value = Pair(x, y)
    }

    fun closeZoomedSquare() {
        _zoomedSquare.value = null
    }

    fun resumeGuidedJumpSelection() {
        _isContinuingGuidedJump.value = true
        bumpBoardRevision()
    }

    fun dismissGameOverDialog() {
        _showGameOverDialog.value = false
    }

    fun promptCapture(jumpedX: Int, jumpedY: Int, player: OctiPlayer) {
        _capturePrompt.value = CapturePromptData(jumpedX, jumpedY, player)
    }

    fun resolveCapturePrompt(capture: Boolean) {
        val p = _capturePrompt.value
        if (p != null) {
            composer.setCaptureAtSquare(p.jumpedX, p.jumpedY, capture)
            _capturePrompt.value = null
            bumpBoardRevision()
        }
    }

    fun setShowSuperProngBonusDialog(show: Boolean) {
        _showSuperProngBonusDialog.value = show
    }

    fun setCompositePodPrompt(square: Pair<Int, Int>?) {
        _compositePodPrompt.value = square
    }

    init {
        configStore.load()
        configStore.configFile().parentFile?.let {
            storageRootDir = it
        }
        OctiCrashReporter.setStorageRootDir(storageRootDir)
        OctiCrashReporter.updateGameContext(_matchType.value.name, livePlyCount(), board.toPositionString())
        val prefs = configStore.preferences()
        _theme.value = prefs.getTheme()
        _confirmAllMoves.value = true
        _sfxMuted.value = prefs.isSfxMuted()
        _volumeKeyPtt.value = prefs.isVolumeKeyPtt()
        soundSynth.setMuted(_sfxMuted.value)
        composer.setConfirmAllMoves(true)
        prefs.setConfirmAllMoves(true)
        if (prefs.sessionToken.isNotEmpty()) {
            netClient.setSessionToken(prefs.sessionToken)
            _isAuthenticated.value = true
            val cachedProfile = OctiNetworkClient.PlayerProfileInfo().apply {
                uuid = prefs.playerUuid
                handle = prefs.handle
                displayName = prefs.displayName
                avatarUrl = prefs.avatarUrl
            }
            _currentProfile.value = cachedProfile
        }
        _telemetryConsent.value = prefs.getTelemetryConsent()
        _telemetryConsentVersion.value = prefs.getTelemetryConsentVersion()
        _installId.value = prefs.getInstallId()
        if (prefs.getTelemetryConsent() == "unset" ||
            prefs.getTelemetryConsentVersion() < ClientConfigStore.ClientPreferences.CURRENT_TELEMETRY_CONSENT_VERSION
        ) {
            _showTelemetryConsentDialog.value = true
        }
        startNewLocalGame(
            OctiGameMode.GAME_MODE_2,
            false,
            false,
            OctiGameBases.GAME_BASES_3,
            true,
            OctiBotEngine.BotDifficulty.TACTICAL,
            OctiPlayer.PLAYER_A
        )
    }

    /**
     * Explicitly ignores any external `server_url`, `server_host`, `server_port`,
     * `host`, or `port` arguments or intent extras. The server endpoint is
     * permanently compiled as `trumpet.selfso.com:6874`.
     */
    fun applyExternalArguments(args: Map<String, String>?): Boolean {
        if (args == null || args.isEmpty()) {
            return false
        }
        val tabArg = args["tab"] ?: ""
        when (tabArg.uppercase()) {
            "LOBBY", "ONLINE" -> _currentTab.value = AppTab.ONLINE_LOBBY
            "ARCHIVE", "LEADERBOARD" -> _currentTab.value = AppTab.ARCHIVE_LEADERBOARD
            "BOARD" -> _currentTab.value = AppTab.BOARD
        }
        return true
    }

    fun selectTab(tab: AppTab) {
        _currentTab.value = tab
    }

    fun setShowNewGameDialog(visible: Boolean) {
        _showNewGameDialog.value = visible
    }

    fun setShowSsoDialog(visible: Boolean) {
        _showSsoDialog.value = visible
    }

    fun setShowAccountDialog(visible: Boolean) {
        _showAccountDialog.value = visible
    }

    fun setShowCreateTableDialog(visible: Boolean) {
        _showCreateTableDialog.value = visible
    }

    fun setTheme(newTheme: OctiThemeId) {
        _theme.value = newTheme
        configStore.preferences().setTheme(newTheme)
        configStore.save()
        bumpBoardRevision()
    }

    fun toggleTheme() {
        val next = if (_theme.value == OctiThemeId.OBSIDIAN_BRASS) {
            OctiThemeId.ROSEWOOD_IVORY
        } else {
            OctiThemeId.OBSIDIAN_BRASS
        }
        setTheme(next)
    }

    fun setConfirmAllMoves(confirm: Boolean) {
        _confirmAllMoves.value = confirm
        composer.setConfirmAllMoves(confirm)
        configStore.preferences().setConfirmAllMoves(confirm)
        configStore.save()
        bumpBoardRevision()
    }

    fun toggleConfirmAllMoves() {
        setConfirmAllMoves(!_confirmAllMoves.value)
    }

    fun setMoveEntireMatchingStack(moveAll: Boolean) {
        _moveEntireMatchingStack.value = moveAll
        composer.setMoveEntireMatchingStack(moveAll)
        bumpBoardRevision()
    }

    fun setSfxMuted(muted: Boolean) {
        _sfxMuted.value = muted
        soundSynth.setMuted(muted)
        configStore.preferences().setSfxMuted(muted)
        configStore.save()
    }

    fun toggleSfxMuted() {
        setSfxMuted(!_sfxMuted.value)
    }

    fun setVolumeKeyPtt(enabled: Boolean) {
        _volumeKeyPtt.value = enabled
        configStore.preferences().setVolumeKeyPtt(enabled)
        configStore.save()
    }

    fun setAutoRotateSeatView(enabled: Boolean) {
        _autoRotateSeatView.value = enabled
        if (enabled) {
            syncRotationToActiveSeat()
        }
        bumpBoardRevision()
    }

    fun setBoardRotation(rot: OctiBoardGeometry.BoardRotation) {
        _boardRotation.value = rot
        geometry.configure(
            Math.max(320, geometry.viewportWidth()),
            Math.max(320, geometry.viewportHeight()),
            displayBoard.boardWidth,
            displayBoard.boardHeight,
            rot
        )
        bumpBoardRevision()
    }

    fun rotateBoard90Clockwise(): OctiBoardGeometry.BoardRotation {
        _autoRotateSeatView.value = false
        val next = OctiBoardGeometry.nextRotationClockwise(_boardRotation.value)
        setBoardRotation(next)
        return next
    }

    fun rotateBoard90CounterClockwise(): OctiBoardGeometry.BoardRotation {
        _autoRotateSeatView.value = false
        val next = when (_boardRotation.value) {
            OctiBoardGeometry.BoardRotation.ROT_0 -> OctiBoardGeometry.BoardRotation.ROT_270
            OctiBoardGeometry.BoardRotation.ROT_90 -> OctiBoardGeometry.BoardRotation.ROT_0
            OctiBoardGeometry.BoardRotation.ROT_180 -> OctiBoardGeometry.BoardRotation.ROT_90
            OctiBoardGeometry.BoardRotation.ROT_270 -> OctiBoardGeometry.BoardRotation.ROT_180
        }
        setBoardRotation(next)
        return next
    }

    fun setBotDifficulty(difficulty: OctiBotEngine.BotDifficulty) {
        _botDifficulty.value = difficulty
        botEngine.setDifficulty(difficulty)
    }

    /* ================================================================== */
    /* Local Tabletop, AI Bot & VCR Move-History Scrubbing                */
    /* ================================================================== */

    @JvmOverloads
    fun startNewLocalGame(
        mode: OctiGameMode = OctiGameMode.GAME_MODE_2,
        useSuperProngs: Boolean = false,
        useEdgeless: Boolean = false,
        bases: OctiGameBases = if (mode == OctiGameMode.GAME_MODE_OFK) {
            OctiGameBases.GAME_BASES_1
        } else {
            OctiGameBases.GAME_BASES_3
        },
        vsBot: Boolean = true,
        difficulty: OctiBotEngine.BotDifficulty = OctiBotEngine.BotDifficulty.TACTICAL,
        humanPlayer: OctiPlayer = OctiPlayer.PLAYER_A
    ) {
        cancelActiveBotTurn()
        synchronized(board) {
            board.setGameMode(mode, useSuperProngs, useEdgeless, bases)
            board.game.start(OctiPlayer.PLAYER_A)

            val initialFen = board.toPositionString()
            displayBoard.fromPositionString(initialFen)

            composer.attachBoard(board)
            composer.setConfirmAllMoves(_confirmAllMoves.value)
            composer.setMoveEntireMatchingStack(_moveEntireMatchingStack.value)
            _matchType.value = if (vsBot) MatchType.LOCAL_VS_BOT else MatchType.LOCAL_PASS_AND_PLAY
            _botDifficulty.value = difficulty
            botEngine.setDifficulty(difficulty)
            _humanSeat.value = humanPlayer

            _fenHistory.clear()
            _moveHistory.clear()
            _fenHistory.add(initialFen)
        }
        _viewingPly.value = 0
        clearLastMoveIndicators()
        _matchStartedEpochSec = System.currentTimeMillis() / 1000L
        _currentTurnStartEpochMs = System.currentTimeMillis()
        _transcriptUploadedForCurrentMatch = false
        _heldTranscriptPendingConsent = false
        _isContinuingGuidedJump.value = false
        _showGameOverDialog.value = false
        _zoomedSquare.value = null
        _capturePrompt.value = null
        _showSuperProngBonusDialog.value = false
        _compositePodPrompt.value = null

        val initialRot = OctiBoardGeometry.rotationForSeat(mode, humanPlayer)
        _boardRotation.value = initialRot
        geometry.configure(
            Math.max(480, geometry.viewportWidth()),
            Math.max(480, geometry.viewportHeight()),
            board.boardWidth,
            board.boardHeight,
            initialRot
        )
        _statusBanner.value = if (vsBot) {
            "Local vs. Bot (${difficulty.name})"
        } else {
            "Pass & Play (${modeToWireCode(mode)})"
        }
        bumpBoardRevision()
        OctiCrashReporter.updateGameContext(_matchType.value.name, livePlyCount(), board.toPositionString())
        if (_requireStartMatchPrompt) {
            beginAwaitingMatchStart()
        } else {
            executeBotTurnIfNeeded()
        }
    }

    fun livePlyCount(): Int {
        return Math.max(0, _fenHistory.size - 1)
    }

    fun isViewingHistory(): Boolean {
        return _viewingPly.value < livePlyCount()
    }

    fun moveHistory(): List<String> {
        return Collections.unmodifiableList(ArrayList(_moveHistory))
    }

    fun fenHistory(): List<String> {
        return Collections.unmodifiableList(ArrayList(_fenHistory))
    }

    fun vcrFirst(): Int {
        return vcrSeekToPly(0)
    }

    fun vcrPrev(): Int {
        return vcrSeekToPly(_viewingPly.value - 1)
    }

    fun vcrNext(): Int {
        return vcrSeekToPly(_viewingPly.value + 1)
    }

    fun vcrLast(): Int {
        return vcrSeekToPly(livePlyCount())
    }

    fun vcrSeekToPly(targetPly: Int): Int {
        clearLastMoveIndicators()
        if (_fenHistory.isEmpty()) {
            _viewingPly.value = 0
            return 0
        }
        val clamped = Math.max(0, Math.min(livePlyCount(), targetPly))
        val fen = _fenHistory[clamped]
        displayBoard.fromPositionString(fen)
        _viewingPly.value = clamped
        bumpBoardRevision()
        return clamped
    }

    fun applyMoveNotation(notation: String): Boolean {
        if (notation.isBlank()) {
            return false
        }
        if (_matchType.value == MatchType.ONLINE_ROOM && _activeRoomId.value.isNotEmpty()) {
            val res = submitOnlineMove(notation)
            return res.ok
        }
        return synchronized(board) {
            if (isViewingHistory()) {
                vcrLast()
            }
            composer.rollbackMove()
            try {
                board.game.move(notation)
                recordAppliedBoardPly(notation)
                composer.attachBoard(board)
                composer.setConfirmAllMoves(_confirmAllMoves.value)
                syncRotationToActiveSeat()
                bumpBoardRevision()
                true
            } catch (e: RuntimeException) {
                _statusBanner.value = "Illegal move: ${e.message}"
                false
            }
        }
    }

    fun commitStagedMove(): Boolean {
        _isContinuingGuidedJump.value = false
        if (composer.isGuidedMoveActive()) {
            composer.finishGuidedMoveHere()
        }
        if (!composer.isAwaitingConfirmation()) {
            return false
        }
        val stagedNotation = composer.stagedMoveNotation()
        if (_matchType.value == MatchType.ONLINE_ROOM && _activeRoomId.value.isNotEmpty()) {
            composer.rollbackMove()
            val res = submitOnlineMove(stagedNotation)
            return res.ok
        }
        if (!composer.commitMove()) {
            return false
        }
        recordAppliedBoardPly(stagedNotation)
        syncRotationToActiveSeat()
        bumpBoardRevision()
        OctiCrashReporter.updateGameContext(_matchType.value.name, livePlyCount(), board.toPositionString())
        executeBotTurnIfNeeded()
        return true
    }

    fun rollbackStagedMove() {
        clearLastMoveIndicators()
        _isContinuingGuidedJump.value = false
        composer.rollbackMove()
        displayBoard.fromPositionString(board.toPositionString())
        bumpBoardRevision()
    }

    fun undoLocalMove(): Boolean {
        if (_matchType.value != MatchType.LOCAL_PASS_AND_PLAY &&
            _matchType.value != MatchType.LOCAL_VS_BOT
        ) {
            return false
        }
        clearLastMoveIndicators()
        cancelActiveBotTurn()
        return synchronized(board) {
            _isContinuingGuidedJump.value = false
            composer.rollbackMove()
            val g = board.game ?: return@synchronized false
            if (!g.canUndo()) {
                return@synchronized false
            }
            g.undo()
            if (_matchType.value == MatchType.LOCAL_VS_BOT &&
                g.turn != _humanSeat.value &&
                g.canUndo()
            ) {
                g.undo()
            }
            while (_fenHistory.size > g.numMoves + 1 && _fenHistory.size > 1) {
                _fenHistory.removeAt(_fenHistory.size - 1)
            }
            while (_moveHistory.size > g.numMoves && _moveHistory.isNotEmpty()) {
                _moveHistory.removeAt(_moveHistory.size - 1)
            }
            val currentFen = board.toPositionString()
            if (_fenHistory.isEmpty()) {
                _fenHistory.add(currentFen)
            } else {
                _fenHistory[_fenHistory.size - 1] = currentFen
            }
            displayBoard.fromPositionString(currentFen)
            _viewingPly.value = livePlyCount()
            _showGameOverDialog.value = g.isGameOver
            composer.attachBoard(board)
            composer.setConfirmAllMoves(_confirmAllMoves.value)
            syncRotationToActiveSeat()
            bumpBoardRevision()
            true
        }
    }

    fun resignLocalGame(): Boolean {
        if (_matchType.value != MatchType.LOCAL_PASS_AND_PLAY &&
            _matchType.value != MatchType.LOCAL_VS_BOT
        ) {
            return false
        }
        clearLastMoveIndicators()
        cancelActiveBotTurn()
        return synchronized(board) {
            val g = board.game ?: return@synchronized false
            if (!g.isGameStarted || g.isGameOver) {
                return@synchronized false
            }
            _isContinuingGuidedJump.value = false
            composer.rollbackMove()
            val resignPlayer = if (_matchType.value == MatchType.LOCAL_VS_BOT) {
                _humanSeat.value
            } else {
                g.turn
            }
            try {
                g.resign(resignPlayer)
                val snap = board.toPositionString()
                displayBoard.fromPositionString(snap)
                soundSynth.triggerCue(OctiSoundSynth.SoundCue.GAME_VICTORY)
                _statusBanner.value = "Game Over — Winner: ${g.winner}"
                _showGameOverDialog.value = true
                bumpBoardRevision()
                true
            } catch (e: RuntimeException) {
                false
            }
        }
    }

    /**
     * Synchronously computes and applies a single AI bot move on the live board.
     * Returns the formatted move notation, or empty string if game is over.
     */
    fun playBotTurnSynchronously(): String {
        if (board.game == null || board.game.isGameOver) {
            return ""
        }
        val stats = OctiBotEngine.BotSearchStats()
        val isCustom = botEngine.isCustomTimeBudget
        if (!isCustom) {
            botEngine.setTimeBudgetMs(25)
        }
        val chosen = try {
            botEngine.chooseMove(board, stats)
        } finally {
            if (!isCustom) {
                botEngine.restoreDefaultBudget()
            }
        }
        if (chosen.isEmpty() || !applyMoveNotation(chosen)) {
            return ""
        }
        val plyMeta = OctiGame.OctiPlyMetadata().apply {
            this.elapsedMs = stats.elapsedMs
            this.isAiMove = true
            this.aiDifficulty = _botDifficulty.value.level
            this.aiBudgetMs = botEngine.timeBudgetMs
            this.aiDepthReached = stats.depthReached
            this.aiNodesVisited = stats.nodesVisited
            this.aiBestScore = stats.bestScore
        }
        board.game?.setLastPlyMetadata(plyMeta)
        return chosen
    }

    fun startAsyncBotTurn(): Thread? {
        val rootFen: String
        synchronized(board) {
            val g = board.game ?: return null
            if (!g.isGameStarted || g.isGameOver || g.turn == _humanSeat.value) {
                return null
            }
            if (_isBotThinking.value) {
                return _activeBotThinkingThread
            }
            rootFen = board.toPositionString()
            _isBotThinking.value = true
            _botThinkingProgress.value = 0f
        }

        val turnGen = _botTurnGeneration.incrementAndGet()
        val budgetMs = botEngine.timeBudgetMs.toFloat().coerceAtLeast(10f)
        _botThinkingRemainingSec.value = (budgetMs / 1000f)

        val t = Thread({
            try {
                val startNs = System.nanoTime()
                val ticker = java.util.concurrent.Executors.newSingleThreadScheduledExecutor()
                ticker.scheduleAtFixedRate({
                    if (_botTurnGeneration.get() == turnGen) {
                        val elapsedMs = (System.nanoTime() - startNs) / 1_000_000f
                        val progress = (elapsedMs / budgetMs).coerceIn(0f, 1f)
                        val remaining = ((budgetMs - elapsedMs) / 1000f).coerceAtLeast(0f)
                        _botThinkingProgress.value = progress
                        _botThinkingRemainingSec.value = remaining
                        _botThinkingDepth.value = botEngine.liveDepthReached()
                        _botThinkingNodes.value = botEngine.liveNodesVisited()
                        _botThinkingScore.value = botEngine.liveBestScore()
                    }
                }, 50, 50, java.util.concurrent.TimeUnit.MILLISECONDS)

                var chosen = ""
                val stats = OctiBotEngine.BotSearchStats()
                try {
                    val searchBoard = OctiBoard()
                    searchBoard.suppressBoardUpdateNotification(true)
                    searchBoard.fromPositionString(rootFen)
                    chosen = botEngine.chooseMove(searchBoard, stats)
                } catch (_: Throwable) {
                    chosen = ""
                } finally {
                    ticker.shutdownNow()
                }

                if (_botTurnGeneration.get() == turnGen) {
                    var moveApplied = false
                    try {
                        synchronized(board) {
                            var candidate = chosen
                            if (candidate.isEmpty()) {
                                candidate = board.game?.generateLegalMoves()?.firstOrNull()?.asString() ?: ""
                            }
                            if (candidate.isNotEmpty()) {
                                moveApplied = applyMoveNotation(candidate)
                                if (!moveApplied) {
                                    val fallback = board.game?.generateLegalMoves()?.firstOrNull()?.asString() ?: ""
                                    if (fallback.isNotEmpty() && fallback != candidate) {
                                        moveApplied = applyMoveNotation(fallback)
                                    }
                                }
                                if (moveApplied) {
                                    val plyMeta = OctiGame.OctiPlyMetadata().apply {
                                        this.elapsedMs = stats.elapsedMs
                                        this.isAiMove = true
                                        this.aiDifficulty = _botDifficulty.value.level
                                        this.aiBudgetMs = botEngine.timeBudgetMs
                                        this.aiDepthReached = stats.depthReached
                                        this.aiNodesVisited = stats.nodesVisited
                                        this.aiBestScore = stats.bestScore
                                    }
                                    board.game?.setLastPlyMetadata(plyMeta)
                                }
                            }
                        }
                    } finally {
                        _isBotThinking.value = false
                        _botThinkingProgress.value = 1f
                        _botThinkingRemainingSec.value = 0f
                        _botThinkingDepth.value = 0
                        _botThinkingNodes.value = 0L
                        _botThinkingScore.value = 0
                        _activeBotThinkingThread = null
                    }
                    if (moveApplied) {
                        executeBotTurnIfNeeded()
                    }
                }
            } catch (t: Throwable) {
                _isBotThinking.value = false
                _botThinkingProgress.value = 1f
                _botThinkingRemainingSec.value = 0f
                _botThinkingDepth.value = 0
                _botThinkingNodes.value = 0L
                _botThinkingScore.value = 0
                _activeBotThinkingThread = null
                val shareExtra = (configStore.preferences().telemetryConsent == "share")
                val instId = if (shareExtra) (configStore.preferences().installId ?: "") else ""
                OctiCrashReporter.recordHandledException(
                    storageRootDir,
                    "OctiBotThinkingThread",
                    t,
                    try { BuildConfig.VERSION_NAME } catch (_: Throwable) { "2.1.2" },
                    instId
                )
                flushPendingTelemetry(async = true)
            }
        }, "OctiBotThinkingThread")

        _activeBotThinkingThread = t
        t.start()
        return t
    }

    fun executeBotTurnIfNeeded(): Boolean {
        if (_awaitingMatchStart.value) return false
        if (_matchType.value != MatchType.LOCAL_VS_BOT) return false
        val g = board.game ?: return false
        if (!g.isGameStarted || g.isGameOver) return false
        if (g.turn == _humanSeat.value) return false

        if (_asyncBotThinkingEnabled.value) {
            startAsyncBotTurn()
            return true
        }

        var safety = 0
        var moved = false
        while (!g.isGameOver && g.turn != _humanSeat.value && safety < 8) {
            safety++
            val chosen = playBotTurnSynchronously()
            if (chosen.isEmpty()) break
            moved = true
        }
        return moved
    }

    fun loadTranscriptForReplay(octiText: String): Boolean {
        return try {
            val loadedBoard = OctiBoard()
            loadedBoard.deserializeGame(octiText)

            _matchType.value = MatchType.ARCHIVE_REPLAY
            _fenHistory.clear()
            _moveHistory.clear()

            val replayWalker = OctiBoard(
                loadedBoard.gameMode,
                loadedBoard.game.useSuperProngs(),
                loadedBoard.game.useEdgelessBoard(),
                loadedBoard.game.bases
            )
            replayWalker.game.start(loadedBoard.game.startPlayer)
            _fenHistory.add(replayWalker.toPositionString())

            for (entry in loadedBoard.game.moves) {
                val mvStr = entry.move.asString()
                replayWalker.game.move(mvStr)
                _moveHistory.add(mvStr)
                _fenHistory.add(replayWalker.toPositionString())
            }

            board.fromPositionString(_fenHistory.last())
            displayBoard.fromPositionString(_fenHistory.last())
            composer.attachBoard(board)
            _viewingPly.value = livePlyCount()
            clearLastMoveIndicators()
            bumpBoardRevision()
            true
        } catch (e: RuntimeException) {
            _statusBanner.value = "Failed to load transcript: ${e.message}"
            false
        }
    }

    fun buildCurrentGameTranscriptJson(includeExtendedTelemetry: Boolean = false): String {
        val g = board.game ?: return ""
        val meta = OctiGame.OctiGameMetadata().apply {
            this.source = "ANDROID_LOCAL_AI"
            this.appVersion = OctiDefs.OCTI_VERSION_STRING
            val abi = try {
                android.os.Build.SUPPORTED_ABIS?.firstOrNull() ?: "generic"
            } catch (_: Throwable) {
                "generic"
            }
            this.platform = "android-$abi"
            this.buildType = "release"
            this.license = "CC-BY-4.0"
            this.attribution = "Octi game data, Charles Chiou (selfso.com)"

            val humanSeat = _humanSeat.value
            val botSeat = if (humanSeat == OctiPlayer.PLAYER_A) OctiPlayer.PLAYER_B else OctiPlayer.PLAYER_A
            val p1 = OctiGame.OctiPlayerMetadata().apply {
                this.seat = 1
                this.player = humanSeat
                this.type = "HUMAN"
                this.handle = "Player"
            }
            val p2 = OctiGame.OctiPlayerMetadata().apply {
                this.seat = 2
                this.player = botSeat
                this.type = "AI"
                this.handle = "AI_L" + _botDifficulty.value.level
                this.aiDifficulty = _botDifficulty.value.level
                this.aiBudgetMs = botEngine.timeBudgetMs
            }
            this.players.add(p1)
            this.players.add(p2)

            if (includeExtendedTelemetry) {
                this.extended.present = true
                this.extended.startedAt = if (_matchStartedEpochSec > 0) _matchStartedEpochSec else System.currentTimeMillis() / 1000L
                this.extended.endedAt = System.currentTimeMillis() / 1000L
                val tz = TimeZone.getDefault()
                this.extended.utcOffsetMin = tz.getOffset(System.currentTimeMillis()) / (60 * 1000)
                this.extended.locale = Locale.getDefault().toLanguageTag()
                val osRel = try {
                    android.os.Build.VERSION.RELEASE
                } catch (_: Throwable) {
                    "unknown"
                }
                this.extended.osVersion = "Android $osRel"
                this.extended.installId = configStore.preferences().installId ?: ""
            } else {
                this.extended.present = false
            }
        }
        return g.serializeJson(meta)
    }

    fun exportCurrentTranscript(): String {
        return buildCurrentGameTranscriptJson(false)
    }

    /**
     * Formats the committed move history as canonical JSON v3.0 transcript text.
     */
    fun exportMoveHistoryText(): String {
        return exportCurrentTranscript()
    }

    @JvmOverloads
    fun uploadCompletedBotGameIfEligible(
        userInitiatedPrompt: Boolean = false,
        synchronousForTest: Boolean = false
    ): Boolean {
        if (_matchType.value != MatchType.LOCAL_VS_BOT) {
            return false
        }
        val g = board.game ?: return false
        if (!g.isGameStarted || !g.isGameOver) {
            return false
        }
        if (g.numMoves == 0) {
            return false
        }
        if (_transcriptUploadedForCurrentMatch) {
            return false
        }
        val consent = configStore.preferences().telemetryConsent ?: "unset"
        if (consent == "unset" || configStore.preferences().telemetryConsentVersion < ClientConfigStore.ClientPreferences.CURRENT_TELEMETRY_CONSENT_VERSION) {
            _heldTranscriptPendingConsent = true
            if (userInitiatedPrompt) {
                _showTelemetryConsentDialog.value = true
            }
            return false
        }
        _transcriptUploadedForCurrentMatch = true
        _heldTranscriptPendingConsent = false

        val shareExtra = (consent == "share")
        val jsonStr = buildCurrentGameTranscriptJson(shareExtra)
        val installId = if (shareExtra) (configStore.preferences().installId ?: "") else ""

        val gameId = try { g.computeCanonicalGameId() } catch (_: Throwable) { "game_" + System.currentTimeMillis() }
        OctiCrashReporter.spoolTrainingTranscript(storageRootDir, gameId, jsonStr, installId)

        if (synchronousForTest) {
            val flushed = OctiCrashReporter.flushPendingTranscripts(storageRootDir, netClient, installId)
            OctiCrashReporter.flushPendingCrashReports(storageRootDir, netClient, installId)
            return flushed > 0
        } else {
            val thread = Thread({
                try {
                    OctiCrashReporter.flushPendingTranscripts(storageRootDir, netClient, installId)
                    OctiCrashReporter.flushPendingCrashReports(storageRootDir, netClient, installId)
                } catch (t: Throwable) {
                    OctiCrashReporter.recordHandledException(
                        storageRootDir,
                        "OctiTrainingUploadThread",
                        t,
                        try { BuildConfig.VERSION_NAME } catch (_: Throwable) { "2.1.2" },
                        installId
                    )
                }
            }, "OctiTrainingUploadThread")
            thread.isDaemon = true
            thread.start()
            return true
        }
    }

    @JvmOverloads
    fun flushPendingTelemetry(async: Boolean = true) {
        val shareExtra = (configStore.preferences().telemetryConsent == "share")
        val installId = if (shareExtra) (configStore.preferences().installId ?: "") else ""
        if (async) {
            val t = Thread({
                try {
                    OctiCrashReporter.flushPendingTranscripts(storageRootDir, netClient, installId)
                    OctiCrashReporter.flushPendingCrashReports(storageRootDir, netClient, installId)
                } catch (ex: Throwable) {
                    OctiCrashReporter.recordHandledException(
                        storageRootDir,
                        "OctiUploadThread",
                        ex,
                        try { BuildConfig.VERSION_NAME } catch (_: Throwable) { "2.1.2" },
                        installId
                    )
                }
            }, "OctiUploadThread")
            t.isDaemon = true
            t.start()
        } else {
            try {
                OctiCrashReporter.flushPendingTranscripts(storageRootDir, netClient, installId)
                OctiCrashReporter.flushPendingCrashReports(storageRootDir, netClient, installId)
            } catch (ex: Throwable) {
                OctiCrashReporter.recordHandledException(
                    storageRootDir,
                    "OctiUploadThread",
                    ex,
                    try { BuildConfig.VERSION_NAME } catch (_: Throwable) { "2.1.2" },
                    installId
                )
            }
        }
    }

    private fun recordAppliedBoardPly(notation: String) {
        val snap = board.toPositionString()
        _moveHistory.add(notation)
        _fenHistory.add(snap)
        displayBoard.fromPositionString(snap)
        _viewingPly.value = livePlyCount()
        OctiCrashReporter.updateGameContext(_matchType.value.name, livePlyCount(), snap)

        val lastMove = board.game?.moves?.lastOrNull()?.move ?: try {
            OctiMove(notation)
        } catch (_: Throwable) {
            null
        }
        val edgeless = board.game?.useEdgelessBoard() == true
        _lastMoveModifiedSquares.value = if (lastMove != null) {
            OctiBoardRenderer.extractModifiedSquares(lastMove, edgeless)
        } else {
            OctiBoardRenderer.extractModifiedSquares(notation, edgeless)
        }
        _lastMoveIndicatorSetAtMs = System.currentTimeMillis()

        val g = board.game
        if (g != null && g.numMoves > 0) {
            val lastMeta = g.getPlyMetadata(g.numMoves - 1)
            if (!lastMeta.isAiMove && lastMeta.elapsedMs == 0L) {
                val nowMs = System.currentTimeMillis()
                val thinkMs = Math.max(0L, nowMs - _currentTurnStartEpochMs)
                lastMeta.elapsedMs = thinkMs
                g.setLastPlyMetadata(lastMeta)
                _currentTurnStartEpochMs = nowMs
            }
        }

        if (board.game.isGameOver) {
            soundSynth.triggerCue(OctiSoundSynth.SoundCue.GAME_VICTORY)
            _statusBanner.value = "Game Over — Winner: ${board.game.winner}"
            _showGameOverDialog.value = true
            uploadCompletedBotGameIfEligible()
        } else {
            val cue = when {
                notation.contains("x") -> OctiSoundSynth.SoundCue.POD_CAPTURE
                notation.contains("+") -> OctiSoundSynth.SoundCue.PRONG_SNAP
                notation.contains("-") -> OctiSoundSynth.SoundCue.POD_STEP
                else -> OctiSoundSynth.SoundCue.POD_PLACE
            }
            soundSynth.triggerCue(cue)
        }
    }

    private fun syncRotationToActiveSeat() {
        if (!_autoRotateSeatView.value) {
            return
        }
        when (_matchType.value) {
            MatchType.LOCAL_PASS_AND_PLAY -> {
                val rot = OctiBoardGeometry.rotationForSeat(
                    board.gameMode,
                    board.game.turn
                )
                setBoardRotation(rot)
            }
            MatchType.LOCAL_VS_BOT -> {
                val rot = OctiBoardGeometry.rotationForSeat(
                    board.gameMode,
                    _humanSeat.value
                )
                setBoardRotation(rot)
            }
            MatchType.ONLINE_ROOM -> {
                val gv = _activeGameView.value
                if (gv != null && gv.viewerSeat >= 1) {
                    val mode = modeFromWireCode(gv.mode)
                    setBoardRotation(OctiBoardGeometry.rotationForSeat(mode, gv.viewerSeat))
                }
            }
            MatchType.ARCHIVE_REPLAY -> {
                // Keep viewer's current rotation during replay scrubbing.
            }
        }
    }

    fun notifyComposerUpdated() {
        _isContinuingGuidedJump.value = false
        val currentBoardFen = board.toPositionString()
        displayBoard.fromPositionString(currentBoardFen)
        val gameHistorySize = board.game?.numMoves ?: 0
        if (_fenHistory.isNotEmpty() && gameHistorySize > livePlyCount()) {
            val movesList = board.game.moves
            val lastMoveObj = if (movesList.isNotEmpty()) movesList.last().move else null
            val lastMoveStr = lastMoveObj?.asString() ?: ""
            _moveHistory.add(lastMoveStr)
            _fenHistory.add(currentBoardFen)
            _viewingPly.value = livePlyCount()
            val edgeless = board.game?.useEdgelessBoard() == true
            _lastMoveModifiedSquares.value = if (lastMoveObj != null) {
                OctiBoardRenderer.extractModifiedSquares(lastMoveObj, edgeless)
            } else {
                OctiBoardRenderer.extractModifiedSquares(lastMoveStr, edgeless)
            }
            _lastMoveIndicatorSetAtMs = System.currentTimeMillis()

            val g = board.game
            if (g != null && g.numMoves > 0) {
                val lastMeta = g.getPlyMetadata(g.numMoves - 1)
                if (!lastMeta.isAiMove && lastMeta.elapsedMs == 0L) {
                    val nowMs = System.currentTimeMillis()
                    val thinkMs = Math.max(0L, nowMs - _currentTurnStartEpochMs)
                    lastMeta.elapsedMs = thinkMs
                    g.setLastPlyMetadata(lastMeta)
                    _currentTurnStartEpochMs = nowMs
                }
            }

            if (board.game.isGameOver) {
                soundSynth.triggerCue(OctiSoundSynth.SoundCue.GAME_VICTORY)
                _statusBanner.value = "Game Over — Winner: ${board.game.winner}"
                _showGameOverDialog.value = true
                uploadCompletedBotGameIfEligible()
            } else {
                soundSynth.triggerCue(OctiSoundSynth.SoundCue.POD_STEP)
            }
            syncRotationToActiveSeat()
            if (!composer.isAwaitingConfirmation()) {
                executeBotTurnIfNeeded()
            }
        }
        bumpBoardRevision()
    }

    fun bumpBoardRevision() {
        _awaitingConfirmation.value = composer.isAwaitingConfirmation()
        _stagedMoveNotation.value = composer.stagedMoveNotation()
        _canAddMoreMoves.value = composer.canAddMoreMoves()
        _guidedMoveActive.value = composer.isGuidedMoveActive()
        _boardRevision.value = _boardRevision.value + 1L
    }

    fun bumpRevision() {
        bumpBoardRevision()
    }

    fun refreshDisplayBoard() {
        notifyComposerUpdated()
    }

    /* ================================================================== */
    /* Online SSO Authentication & Multi-Provider Account Linking         */
    /* ================================================================== */

    fun refreshAuthProviders(): OctiNetworkClient.ApiResult<List<OctiNetworkClient.AuthProviderInfo>> {
        val res = netClient.fetchAuthProviders()
        if (res.ok && res.data != null) {
            _authProviders.value = res.data
        }
        return res
    }

    fun attachAuthenticatedSession(
        sessionToken: String,
        playerUuid: String,
        handle: String
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.PlayerProfileInfo> {
        netClient.setSessionToken(sessionToken)
        _isAuthenticated.value = true
        val prefs = configStore.preferences()
        prefs.setSessionToken(sessionToken)
        prefs.setPlayerUuid(playerUuid)
        prefs.setHandle(handle)
        configStore.save()
        return refreshMyProfile()
    }

    @JvmOverloads
    fun loginWithSso(
        provider: String,
        token: String,
        desiredHandle: String = ""
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.SsoLoginData> {
        val res = netClient.loginWithSso(provider, token, desiredHandle)
        if (res.ok && res.data != null) {
            val d = res.data
            _isAuthenticated.value = true
            _onboardingRequired.value = d.onboardingRequired
            _currentProfile.value = d.profile
            _linkedIdentities.value = ArrayList(d.identities)

            val prefs = configStore.preferences()
            prefs.setSessionToken(d.sessionToken)
            prefs.setPlayerUuid(d.profile.uuid)
            prefs.setHandle(d.profile.handle)
            prefs.setDisplayName(d.profile.displayName)
            prefs.setAvatarUrl(d.profile.avatarUrl)
            prefs.setPreferredProvider(provider)
            configStore.save()
            _statusBanner.value = "Signed in as ${d.profile.handle}"
        } else {
            _statusBanner.value = "Sign-in failed: ${res.errorMessage}"
        }
        return res
    }

    @JvmOverloads
    fun completeHandleOnboarding(
        handle: String,
        displayName: String = ""
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.PlayerProfileInfo> {
        val res = netClient.updateMyProfile(handle, "", displayName)
        if (res.ok && res.data != null) {
            _currentProfile.value = res.data
            _onboardingRequired.value = false
            val prefs = configStore.preferences()
            prefs.setHandle(res.data.handle)
            prefs.setDisplayName(res.data.displayName)
            configStore.save()
            _statusBanner.value = "Handle updated to ${res.data.handle}"
        } else {
            _statusBanner.value = "Handle update failed: ${res.errorMessage}"
        }
        return res
    }

    fun linkIdentity(
        provider: String,
        token: String
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.LinkIdentityData> {
        val res = netClient.linkIdentity(provider, token)
        if (res.ok && res.data != null) {
            _linkedIdentities.value = ArrayList(res.data.identities)
            _linkConflictProfile.value = null
            _pendingConflictProvider = ""
            _pendingConflictToken = ""
            _statusBanner.value = "Linked $provider identity"
        } else if (res.httpStatus == 409 && res.data != null && res.data.conflictAlreadyLinked) {
            _linkConflictProfile.value = res.data.existingProfile
            _pendingConflictProvider = provider
            _pendingConflictToken = token
            _statusBanner.value =
                "Identity already linked to ${res.data.existingProfile.handle}"
        } else {
            _statusBanner.value = "Link failed: ${res.errorMessage}"
        }
        return res
    }

    /**
     * Resolves a `409 Conflict` during account linking by switching the active
     * session to the existing account that owns the SSO identity.
     */
    fun confirmSwitchToConflictAccount(): OctiNetworkClient.ApiResult<OctiNetworkClient.SsoLoginData> {
        val prov = _pendingConflictProvider
        val tok = _pendingConflictToken
        _linkConflictProfile.value = null
        _pendingConflictProvider = ""
        _pendingConflictToken = ""
        return loginWithSso(prov, tok)
    }

    fun dismissLinkConflict() {
        _linkConflictProfile.value = null
        _pendingConflictProvider = ""
        _pendingConflictToken = ""
    }

    fun unlinkIdentity(
        provider: String
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.UnlinkIdentityData> {
        val res = netClient.unlinkIdentity(provider)
        if (res.ok && res.data != null) {
            _linkedIdentities.value = ArrayList(res.data.identities)
            _statusBanner.value = "Unlinked $provider identity"
        } else {
            _statusBanner.value = "Unlink failed: ${res.errorMessage}"
        }
        return res
    }

    fun refreshMyProfile(): OctiNetworkClient.ApiResult<OctiNetworkClient.PlayerProfileInfo> {
        val res = netClient.fetchMyProfile()
        if (res.ok && res.data != null) {
            _currentProfile.value = res.data
            _linkedIdentities.value = ArrayList(res.data.identities)
        }
        return res
    }

    fun logoutOnline(): OctiNetworkClient.ApiResult<Boolean> {
        leaveVoiceChannel()
        netClient.stopEventStreams()
        val res = netClient.logout()
        configStore.clearSession()
        _isAuthenticated.value = false
        _onboardingRequired.value = false
        _currentProfile.value = null
        _linkedIdentities.value = emptyList()
        _activeRoomId.value = ""
        _activeGameView.value = null
        _statusBanner.value = "Signed out"
        return res
    }

    /* ================================================================== */
    /* Online Lobby, Tables, Matchmaking & Live Room                      */
    /* ================================================================== */

    fun refreshLobbyState(): OctiNetworkClient.ApiResult<OctiNetworkClient.LobbyStateData> {
        val res = netClient.fetchLobbyState()
        if (res.ok && res.data != null) {
            _lobbyState.value = res.data
        }
        return res
    }

    fun sendLobbyChat(text: String): OctiNetworkClient.ApiResult<Long> {
        val res = netClient.postLobbyChat(text)
        if (res.ok) {
            refreshLobbyState()
        }
        return res
    }

    fun createOnlineTable(
        req: OctiNetworkClient.CreateTableRequest
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.CreateTableResponse> {
        val res = netClient.createTable(req)
        if (res.ok && res.data != null && res.data.roomId.isNotEmpty()) {
            enterOnlineRoom(res.data.roomId)
        }
        return res
    }

    @JvmOverloads
    fun joinOnlineTable(
        roomId: String,
        seat: Int,
        passcode: String = ""
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.JoinTableResponse> {
        val res = netClient.joinTable(roomId, seat, passcode)
        if (res.ok && res.data != null) {
            enterOnlineRoom(roomId)
        }
        return res
    }

    @JvmOverloads
    fun observeOnlineTable(
        roomId: String,
        passcode: String = ""
    ): OctiNetworkClient.ApiResult<Boolean> {
        val res = netClient.observeTable(roomId, passcode)
        if (res.ok) {
            enterOnlineRoom(roomId)
        }
        return res
    }

    fun leaveOnlineTable(): OctiNetworkClient.ApiResult<Boolean> {
        val roomId = _activeRoomId.value
        if (roomId.isEmpty()) {
            return OctiNetworkClient.ApiResult(false)
        }
        leaveVoiceChannel()
        netClient.stopGameEventStream()
        val res = netClient.leaveTable(roomId)
        _activeRoomId.value = ""
        _activeGameView.value = null
        _gameChatLog.value = emptyList()
        _matchType.value = MatchType.LOCAL_PASS_AND_PLAY
        return res
    }

    fun kickSeatFromTable(seat: Int): OctiNetworkClient.ApiResult<Boolean> {
        val roomId = _activeRoomId.value
        if (roomId.isEmpty()) {
            return OctiNetworkClient.ApiResult(false)
        }
        val res = netClient.kickSeat(roomId, seat)
        if (res.ok) {
            refreshActiveGameView()
        }
        return res
    }

    fun enqueueMatchmaking(
        req: OctiNetworkClient.MatchmakingRequest
    ): OctiNetworkClient.ApiResult<Boolean> {
        val res = netClient.enqueueMatchmaking(req)
        if (res.ok) {
            pollMatchmakingStatus()
        }
        return res
    }

    fun dequeueMatchmaking(): OctiNetworkClient.ApiResult<Boolean> {
        val res = netClient.dequeueMatchmaking()
        if (res.ok) {
            _matchmakingStatus.value = OctiNetworkClient.MatchmakingStatusData()
        }
        return res
    }

    fun pollMatchmakingStatus(): OctiNetworkClient.ApiResult<OctiNetworkClient.MatchmakingStatusData> {
        val res = netClient.fetchMatchmakingStatus()
        if (res.ok && res.data != null) {
            _matchmakingStatus.value = res.data
            if (res.data.matchedRoomId.isNotEmpty()) {
                enterOnlineRoom(res.data.matchedRoomId)
            }
        }
        return res
    }

    fun enterOnlineRoom(roomId: String): OctiNetworkClient.ApiResult<OctiNetworkClient.GameViewData> {
        _activeRoomId.value = roomId
        _matchType.value = MatchType.ONLINE_ROOM
        _currentTab.value = AppTab.BOARD
        val gvRes = refreshActiveGameView()
        val sinceSeq = gvRes.data?.seq ?: 0L
        netClient.startGameEventStream(roomId, sinceSeq) { ev ->
            handleGameSseEvent(ev)
        }
        return gvRes
    }

    fun refreshActiveGameView(): OctiNetworkClient.ApiResult<OctiNetworkClient.GameViewData> {
        val roomId = _activeRoomId.value
        if (roomId.isEmpty()) {
            return OctiNetworkClient.ApiResult(OctiNetworkClient.GameViewData())
        }
        val res = netClient.fetchGameView(roomId)
        if (res.ok && res.data != null) {
            applyGameViewToBoard(res.data)
        }
        return res
    }

    private fun applyGameViewToBoard(gv: OctiNetworkClient.GameViewData) {
        _activeGameView.value = gv
        if (gv.fen.isNotEmpty()) {
            try {
                board.fromPositionString(gv.fen)
                composer.attachBoard(board)
                composer.setConfirmAllMoves(_confirmAllMoves.value)
                if (_fenHistory.isEmpty() || _fenHistory.last() != gv.fen) {
                    if (gv.moves.isEmpty()) {
                        _fenHistory.clear()
                        _moveHistory.clear()
                        _fenHistory.add(gv.fen)
                        clearLastMoveIndicators()
                    } else {
                        while (_moveHistory.size < gv.moves.size) {
                            _moveHistory.add(gv.moves[_moveHistory.size])
                        }
                        _fenHistory.add(gv.fen)
                        val latestMoveStr = gv.moves.lastOrNull() ?: ""
                        val edgeless = board.game?.useEdgelessBoard() == true
                        _lastMoveModifiedSquares.value = OctiBoardRenderer.extractModifiedSquares(latestMoveStr, edgeless)
                        _lastMoveIndicatorSetAtMs = System.currentTimeMillis()
                    }
                }
                if (!isViewingHistory() || _viewingPly.value >= livePlyCount() - 1) {
                    displayBoard.fromPositionString(gv.fen)
                    _viewingPly.value = livePlyCount()
                }
                if (board.game?.isGameOver == true) {
                    _showGameOverDialog.value = true
                }
            } catch (ignored: RuntimeException) {
            }
        }
        if (gv.viewerSeat >= 1 && _autoRotateSeatView.value) {
            val mode = modeFromWireCode(gv.mode)
            val rot = OctiBoardGeometry.rotationForSeat(mode, gv.viewerSeat)
            _boardRotation.value = rot
            geometry.configure(
                Math.max(480, geometry.viewportWidth()),
                Math.max(480, geometry.viewportHeight()),
                displayBoard.boardWidth,
                displayBoard.boardHeight,
                rot
            )
        }
        bumpBoardRevision()
    }

    fun submitOnlineMove(
        moveNotation: String
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.MoveSubmitData> {
        val roomId = _activeRoomId.value
        val res = netClient.submitMove(roomId, moveNotation)
        if (res.ok) {
            refreshActiveGameView()
        } else {
            _statusBanner.value = "Move rejected: ${res.errorMessage}"
        }
        return res
    }

    fun sendGameChat(scope: String, text: String): OctiNetworkClient.ApiResult<Long> {
        val roomId = _activeRoomId.value
        return netClient.postGameChat(roomId, scope, text)
    }

    fun submitGameAction(
        action: String
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.GameActionData> {
        val roomId = _activeRoomId.value
        val res = netClient.submitGameAction(roomId, action)
        if (res.ok) {
            refreshActiveGameView()
        }
        return res
    }

    fun resignOnlineGame(): OctiNetworkClient.ApiResult<Boolean> {
        val roomId = _activeRoomId.value
        val res = netClient.resignGame(roomId)
        if (res.ok) {
            refreshActiveGameView()
        }
        return res
    }

    fun muteParticipant(
        targetUuid: String,
        muteChat: Boolean,
        muteVoice: Boolean
    ): OctiNetworkClient.ApiResult<Boolean> {
        val roomId = _activeRoomId.value
        return netClient.muteParticipant(roomId, targetUuid, muteChat, muteVoice)
    }

    @JvmOverloads
    fun banObserver(
        targetUuid: String,
        reason: String = ""
    ): OctiNetworkClient.ApiResult<Boolean> {
        val roomId = _activeRoomId.value
        return netClient.banObserver(roomId, targetUuid, reason)
    }

    fun updateRoomSettings(
        observerPolicy: String,
        voicePolicy: String
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.RoomSettingsData> {
        val roomId = _activeRoomId.value
        val res = netClient.updateRoomSettings(roomId, observerPolicy, voicePolicy)
        if (res.ok) {
            refreshActiveGameView()
        }
        return res
    }

    fun handleLobbySseEvent(ev: OctiNetworkClient.SseEvent?) {
        if (ev == null) {
            return
        }
        refreshLobbyState()
    }

    fun handleGameSseEvent(ev: OctiNetworkClient.SseEvent?) {
        if (ev == null) {
            return
        }
        voiceEngine.handleGameSseEvent(ev)
        _isPttActive.value = voiceEngine.isPushToTalkActive()
        _isInVoiceRoom.value = voiceEngine.isInVoiceRoom()

        if ("CHAT" == ev.event || "CHAT_MESSAGE" == ev.event || "GAME_CHAT" == ev.event) {
            try {
                val j = JSONObject(ev.dataJson)
                val msg = GameChatMessage(
                    seq = j.optLong("seq", ev.id),
                    scope = j.optString("scope", "all"),
                    senderUuid = j.optString("player_uuid", ""),
                    senderHandle = j.optString("handle", ""),
                    seat = j.optInt("seat", 0),
                    text = j.optString("text", ""),
                    atMs = j.optLong("at_ms", System.currentTimeMillis())
                )
                val updated = ArrayList(_gameChatLog.value)
                updated.add(msg)
                _gameChatLog.value = updated
            } catch (ignored: Exception) {
            }
        }

        var evType = ""
        try {
            if (ev.dataJson.isNotEmpty() && ev.dataJson.startsWith("{")) {
                val j = JSONObject(ev.dataJson)
                evType = j.optString("type", "")
            }
        } catch (ignored: Exception) {
        }

        if ("MOVE" == ev.event ||
            "MOVE_APPLIED" == ev.event ||
            "GAME_STARTED" == ev.event ||
            "GAME_OVER" == ev.event ||
            "PROPOSAL" == ev.event ||
            "PROPOSAL_UPDATED" == ev.event ||
            "ACTION" == ev.event ||
            "ACTION_APPLIED" == ev.event ||
            "PLAYER_JOINED" == ev.event ||
            "PLAYER_LEFT" == ev.event ||
            "POLICY_UPDATED" == ev.event ||
            "reconnect" == ev.event ||
            "gap" == ev.event ||
            "PLAYER_DISCONNECTED" == ev.event ||
            "PLAYER_RECONNECTED" == ev.event ||
            "GAME_PAUSED" == ev.event ||
            "GAME_RESUMED" == ev.event ||
            "MOVE_UNDONE" == ev.event ||
            "UNDO_APPLIED" == ev.event ||
            "REMATCH_CREATED" == ev.event ||
            "PLAYER_DISCONNECTED" == evType ||
            "PLAYER_RECONNECTED" == evType ||
            "GAME_PAUSED" == evType ||
            "GAME_RESUMED" == evType ||
            "MOVE_UNDONE" == evType ||
            "REMATCH_CREATED" == evType
        ) {
            refreshActiveGameView()
        }
    }

    /* ================================================================== */
    /* Push-to-Talk Voice Controls & Volume Hardware Key Binding          */
    /* ================================================================== */

    fun joinVoiceChannel(): Boolean {
        val roomId = _activeRoomId.value
        val uuid = _currentProfile.value?.uuid ?: configStore.preferences().playerUuid
        if (roomId.isEmpty() || uuid.isEmpty()) {
            return false
        }
        val ok = voiceEngine.joinVoiceRoom(roomId, uuid)
        _isInVoiceRoom.value = voiceEngine.isInVoiceRoom()
        _isPttActive.value = voiceEngine.isPushToTalkActive()
        return ok
    }

    fun leaveVoiceChannel() {
        voiceEngine.leaveVoiceRoom()
        _isInVoiceRoom.value = false
        _isPttActive.value = false
    }

    fun setPushToTalkPressed(pressed: Boolean): Boolean {
        val ok = voiceEngine.setPushToTalkActive(pressed)
        _isPttActive.value = voiceEngine.isPushToTalkActive()
        return ok
    }

    /**
     * Handles physical hardware key events (`KEYCODE_VOLUME_DOWN` /
     * `KEYCODE_VOLUME_UP`) for Push-to-Talk when `volumeKeyPtt` is enabled
     * and the player is in a voice room.
     */
    fun onHardwareKeyEvent(keyCode: Int, isDown: Boolean): Boolean {
        if (!_volumeKeyPtt.value || !voiceEngine.isInVoiceRoom()) {
            return false
        }
        if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN ||
            keyCode == KeyEvent.KEYCODE_VOLUME_UP
        ) {
            setPushToTalkPressed(isDown)
            return true
        }
        return false
    }

    /* ================================================================== */
    /* Game Archive & Elo Leaderboard                                     */
    /* ================================================================== */

    @JvmOverloads
    fun refreshLeaderboard(
        mode: String = "2P",
        limit: Int = 50
    ): OctiNetworkClient.ApiResult<List<OctiNetworkClient.LeaderboardRow>> {
        val res = netClient.fetchLeaderboard(mode, limit)
        if (res.ok && res.data != null) {
            _leaderboardRows.value = res.data
        }
        return res
    }

    @JvmOverloads
    fun refreshArchivedGames(
        mode: String = "",
        playerUuid: String = "",
        limit: Int = 20,
        offset: Int = 0
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.ArchivedGamesPage> {
        val res = netClient.listArchivedGames(mode, playerUuid, limit, offset)
        if (res.ok && res.data != null) {
            _archivedGamesPage.value = res.data
        }
        return res
    }

    fun openArchivedGameForReplay(gameId: String): Boolean {
        val dl = netClient.downloadOctiTranscript(gameId)
        if (dl.ok && dl.data != null && dl.data.isNotEmpty()) {
            if (loadTranscriptForReplay(dl.data)) {
                _currentTab.value = AppTab.BOARD
                return true
            }
        }
        val detail = netClient.fetchArchivedGame(gameId)
        if (detail.ok && detail.data != null && detail.data.octiTranscript.isNotEmpty()) {
            if (loadTranscriptForReplay(detail.data.octiTranscript)) {
                _currentTab.value = AppTab.BOARD
                return true
            }
        }
        return false
    }

    override fun onCleared() {
        super.onCleared()
        close()
    }

    override fun close() {
        cancelActiveBotTurn()
        voiceEngine.close()
        netClient.close()
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
