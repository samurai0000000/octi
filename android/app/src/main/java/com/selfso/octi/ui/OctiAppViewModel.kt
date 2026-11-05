/*
 * OctiAppViewModel.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import android.graphics.Bitmap
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
 * VCR move-history scrubbing, and hard-coded `trumpet.selfso.com:6874` online
 * play (5-provider SSO, multi-provider identity linking, lobby, custom tables,
 * Elo matchmaking, live SSE games, `ALL` / `TEAM` chat, moderation, archive).
 */
class OctiAppViewModel @JvmOverloads constructor(
    val configStore: ClientConfigStore = ClientConfigStore(),
    val netClient: OctiNetworkClient = OctiNetworkClient()
) : ViewModel(), AutoCloseable {

    fun networkClient(): OctiNetworkClient = netClient

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
        @Volatile
        @JvmField
        var avatarEncoderForTest: java.util.function.Function<android.graphics.Bitmap, String>? = null

        @JvmStatic
        fun encodeAvatarBitmapToDataUri(src: android.graphics.Bitmap): String {
            val hook = avatarEncoderForTest
            if (hook != null) {
                val forced = hook.apply(src)
                if (forced != null && forced.isNotEmpty()) return forced
            }
            try {
                val edge = minOf(src.width, src.height)
                if (edge <= 0) return ""
                val x = (src.width - edge) / 2
                val y = (src.height - edge) / 2
                val cropped = if (src.width == edge && src.height == edge) {
                    src
                } else {
                    android.graphics.Bitmap.createBitmap(src, x, y, edge, edge)
                } ?: src

                val dims = intArrayOf(192, 128, 96)
                val jpegQualities = intArrayOf(92, 85, 75, 65, 50)
                val maxRawBytes = 200000

                for (dim in dims) {
                    val scaled = if (cropped.width == dim && cropped.height == dim) {
                        cropped
                    } else {
                        android.graphics.Bitmap.createScaledBitmap(cropped, dim, dim, true)
                    } ?: cropped

                    if (scaled.hasAlpha()) {
                        val baos = java.io.ByteArrayOutputStream()
                        val ok = try {
                            scaled.compress(android.graphics.Bitmap.CompressFormat.PNG, 100, baos)
                        } catch (_: Throwable) {
                            false
                        }
                        if (ok && baos.size() in 1..maxRawBytes) {
                            val b64 = java.util.Base64.getEncoder().encodeToString(baos.toByteArray())
                            return "data:image/png;base64,$b64"
                        }
                    }

                    for (q in jpegQualities) {
                        val baos = java.io.ByteArrayOutputStream()
                        val ok = try {
                            scaled.compress(android.graphics.Bitmap.CompressFormat.JPEG, q, baos)
                        } catch (_: Throwable) {
                            false
                        }
                        if (ok && baos.size() in 1..maxRawBytes) {
                            val b64 = java.util.Base64.getEncoder().encodeToString(baos.toByteArray())
                            return "data:image/jpeg;base64,$b64"
                        }
                    }
                }
            } catch (_: Throwable) {
            }
            return ""
        }

        private val avatarBytesCache = java.util.concurrent.ConcurrentHashMap<String, ByteArray>()

        @JvmStatic
        fun clearAvatarBytesCacheForTest() {
            avatarBytesCache.clear()
        }

        @JvmStatic
        @JvmOverloads
        fun decodeAvatarDataUriOrBytes(
            avatarUrl: String,
            netClient: OctiNetworkClient? = null
        ): ByteArray? {
            if (avatarUrl.isEmpty()) {
                return null
            }
            if (avatarUrl.startsWith("data:")) {
                val comma = avatarUrl.indexOf(',')
                if (comma != -1) {
                    val b64 = avatarUrl.substring(comma + 1).trim()
                    return try {
                        java.util.Base64.getDecoder().decode(b64)
                    } catch (_: Throwable) {
                        null
                    }
                }
                return null
            }
            val cached = avatarBytesCache[avatarUrl]
            if (cached != null) {
                return cached
            }
            if (netClient != null) {
                return try {
                    val bytes = netClient.fetchAvatarBytes(avatarUrl)
                    if (bytes != null && bytes.isNotEmpty()) {
                        avatarBytesCache[avatarUrl] = bytes
                    }
                    bytes
                } catch (_: Throwable) {
                    null
                }
            }
            return null
        }

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
        fun formatLobbyChatTimestamp(epochMs: Long): String {
            val ts = if (epochMs > 0L) epochMs else System.currentTimeMillis()
            val sdf = java.text.SimpleDateFormat("HH:mm", Locale.getDefault())
            sdf.timeZone = TimeZone.getDefault()
            return sdf.format(java.util.Date(ts))
        }

        @JvmStatic
        fun formatLobbyChatEntry(entry: OctiNetworkClient.LobbyChatEntry): String {
            val tsStr = formatLobbyChatTimestamp(entry.atMs)
            val handleStr = entry.handle.ifEmpty { "anon" }
            return "[$tsStr] $handleStr: ${entry.text}"
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

    private val _boardRotation = MutableStateFlow(OctiBoardGeometry.BoardRotation.ROT_0)
    val boardRotation: StateFlow<OctiBoardGeometry.BoardRotation> = _boardRotation.asStateFlow()

    private val _autoRotateSeatView = MutableStateFlow(true)
    val autoRotateSeatView: StateFlow<Boolean> = _autoRotateSeatView.asStateFlow()

    private val _botDifficulty = MutableStateFlow(OctiBotEngine.BotDifficulty.TACTICAL)
    val botDifficulty: StateFlow<OctiBotEngine.BotDifficulty> = _botDifficulty.asStateFlow()

    private val _botPlayStyle = MutableStateFlow("Tactical")
    val botPlayStyle: StateFlow<String> = _botPlayStyle.asStateFlow()

    private val _botTimeBudgetMs = MutableStateFlow(1500L)
    val botTimeBudgetMs: StateFlow<Long> = _botTimeBudgetMs.asStateFlow()

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

    fun setStatusBanner(msg: String) {
        _statusBanner.value = msg
    }

    // Online Authentication & Account Linking State
    private val _isAuthenticated = MutableStateFlow(false)
    val isAuthenticated: StateFlow<Boolean> = _isAuthenticated.asStateFlow()

    private val _onboardingRequired = MutableStateFlow(false)
    val onboardingRequired: StateFlow<Boolean> = _onboardingRequired.asStateFlow()

    private val _crashReporter: OctiCrashReporter = OctiCrashReporter.getInstance()
    val crashReporter: OctiCrashReporter get() = _crashReporter

    private val _profileActionBusy = MutableStateFlow(false)
    val profileActionBusy: StateFlow<Boolean> = _profileActionBusy.asStateFlow()

    private val _profileActionMessage = MutableStateFlow("")
    val profileActionMessage: StateFlow<String> = _profileActionMessage.asStateFlow()

    fun clearProfileActionMessage() {
        _profileActionMessage.value = ""
    }

    private val _currentProfile = MutableStateFlow<OctiNetworkClient.PlayerProfileInfo?>(null)
    val currentProfile: StateFlow<OctiNetworkClient.PlayerProfileInfo?> = _currentProfile.asStateFlow()
    val onlineProfile: StateFlow<OctiNetworkClient.PlayerProfileInfo?> = _currentProfile.asStateFlow()

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
    val linkConflict: StateFlow<OctiNetworkClient.PlayerProfileInfo?>
        get() = _linkConflictProfile.asStateFlow()

    private var _pendingConflictProvider: String = ""
    private var _pendingConflictToken: String = ""
    private var _pendingConflictIdToken: String = ""
    private var _pendingConflictAccessToken: String = ""
    private val _isBrowserAuthInProgress = MutableStateFlow(false)
    val isBrowserAuthInProgress: StateFlow<Boolean> = _isBrowserAuthInProgress.asStateFlow()
    private val _browserAuthStatusText = MutableStateFlow("")
    val browserAuthStatusText: StateFlow<String> = _browserAuthStatusText.asStateFlow()
    private val _loopbackAuthListener = OctiNetworkClient.OAuthLoopbackListener()
    @Volatile
    private var _browserAuthLinkMode: Boolean = false
    @Volatile
    private var _browserAuthProvider: String = "google"
    @Volatile
    private var _preferDeepLinkRedirect: Boolean = false
    @Volatile
    var nativeSocialAuthLauncher: ((provider: String, linkMode: Boolean, clientId: String, onFallbackToBrowser: () -> Unit) -> Boolean)? = null
    private val _backgroundExecutor = java.util.concurrent.Executors.newCachedThreadPool { r ->
        Thread(r, "OctiAppViewModelBackgroundWorker").apply { isDaemon = true }
    }

    // Online Lobby, Matchmaking, Live Room & Archive State
    private val _lobbyState = MutableStateFlow(OctiNetworkClient.LobbyStateData())
    val lobbyState: StateFlow<OctiNetworkClient.LobbyStateData> = _lobbyState.asStateFlow()

    private val _lobbyChatMessages = MutableStateFlow<List<String>>(emptyList())
    val lobbyChatMessages: StateFlow<List<String>> = _lobbyChatMessages.asStateFlow()

    private val _matchmakingStatus = MutableStateFlow(OctiNetworkClient.MatchmakingStatusData())
    val matchmakingStatus: StateFlow<OctiNetworkClient.MatchmakingStatusData> =
        _matchmakingStatus.asStateFlow()

    private val _activeRoomId = MutableStateFlow("")
    val activeRoomId: StateFlow<String> = _activeRoomId.asStateFlow()
    val onlineGameId: StateFlow<String> = _activeRoomId.asStateFlow()

    private val _activeGameView = MutableStateFlow<OctiNetworkClient.GameViewData?>(null)
    val activeGameView: StateFlow<OctiNetworkClient.GameViewData?> = _activeGameView.asStateFlow()

    private val _gameChatLog = MutableStateFlow<List<GameChatMessage>>(emptyList())
    val gameChatLog: StateFlow<List<GameChatMessage>> = _gameChatLog.asStateFlow()

    private val _leaderboardRows =
        MutableStateFlow<List<OctiNetworkClient.LeaderboardRow>>(emptyList())
    val leaderboardRows: StateFlow<List<OctiNetworkClient.LeaderboardRow>> =
        _leaderboardRows.asStateFlow()

    private val _archivedGamesPage = MutableStateFlow(OctiNetworkClient.ArchivedGamesPage())
    val archivedGamesPage: StateFlow<OctiNetworkClient.ArchivedGamesPage> =
        _archivedGamesPage.asStateFlow()

    private val _robotsDirectory =
        MutableStateFlow<List<OctiNetworkClient.RobotDirectoryEntry>>(emptyList())
    val robotsDirectory: StateFlow<List<OctiNetworkClient.RobotDirectoryEntry>> =
        _robotsDirectory.asStateFlow()

    private val _selectedRobotProfile =
        MutableStateFlow<OctiNetworkClient.RobotProfileData?>(null)
    val selectedRobotProfile: StateFlow<OctiNetworkClient.RobotProfileData?> =
        _selectedRobotProfile.asStateFlow()

    // Dialog visibility flags for UI
    private val _showNewGameDialog = MutableStateFlow(false)
    val showNewGameDialog: StateFlow<Boolean> = _showNewGameDialog.asStateFlow()

    private val _showSsoDialog = MutableStateFlow(false)
    val showSsoDialog: StateFlow<Boolean> = _showSsoDialog.asStateFlow()

    private val _showAccountDialog = MutableStateFlow(false)
    val showAccountDialog: StateFlow<Boolean> = _showAccountDialog.asStateFlow()

    private val _showCreateTableDialog = MutableStateFlow(false)
    val showCreateTableDialog: StateFlow<Boolean> = _showCreateTableDialog.asStateFlow()

    private val _showBoardChatZoomDialog = MutableStateFlow(false)
    val showBoardChatZoomDialog: StateFlow<Boolean> = _showBoardChatZoomDialog.asStateFlow()

    private var _lastSubmittedOnlinePly: Int = 0
    fun lastSubmittedOnlinePly(): Int = _lastSubmittedOnlinePly

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
    private var _gameOverDialogShownForCurrentOnlineMatch: Boolean = false

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

    private val _clockTickRevision = MutableStateFlow(0L)
    val clockTickRevision: StateFlow<Long> = _clockTickRevision.asStateFlow()
    private val _clockA = MutableStateFlow(-1L)
    val clockA: StateFlow<Long> = _clockA.asStateFlow()
    private val _clockB = MutableStateFlow(-1L)
    val clockB: StateFlow<Long> = _clockB.asStateFlow()

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
        val gv = _activeGameView.value
        if (gv != null) {
            if (_matchType.value == MatchType.ONLINE_ROOM) {
                if (gv.phase == "IN_PROGRESS" && !gv.paused && gv.timeControlEnabled) {
                    val turnSeat = gv.turnSeat
                    val seat = gv.seats.firstOrNull { it.seat == turnSeat }
                    if (seat != null && seat.clockMs > 0L) {
                        seat.clockMs = (seat.clockMs - 1000L).coerceAtLeast(0L)
                    }
                    val s1 = gv.seats.firstOrNull { it.seat == 1 }
                    val s2 = gv.seats.firstOrNull { it.seat == 2 }
                    _clockA.value = s1?.clockMs ?: -1L
                    _clockB.value = s2?.clockMs ?: -1L
                } else if (gv.phase == "WAITING_FOR_PLAYERS" && gv.ttlRemainingMs > 0L) {
                    gv.ttlRemainingMs = (gv.ttlRemainingMs - 1000L).coerceAtLeast(0L)
                }
                _clockTickRevision.value += 1L
                _activeGameView.value = gv
            } else if (gv.ttlRemainingMs > 0L) {
                gv.ttlRemainingMs = (gv.ttlRemainingMs - 1000L).coerceAtLeast(0L)
                _activeGameView.value = gv
            }
        }
        val lobby = _lobbyState.value
        if (lobby != null && lobby.tables.isNotEmpty()) {
            for (t in lobby.tables) {
                if (t.ttlRemainingMs > 0L) {
                    t.ttlRemainingMs = (t.ttlRemainingMs - 1000L).coerceAtLeast(0L)
                }
            }
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
        if (configStore.preferences().installId.isEmpty()) {
            configStore.preferences().installId = ClientConfigStore.generateUuidV4()
        }
        netClient.setInstallId(configStore.preferences().installId ?: "")
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
        netClient.setInstallId(configStore.preferences().installId ?: "")
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
        netClient.setInstallId(configStore.preferences().installId ?: "")
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

    private fun isMainThread(): Boolean {
        if (OctiNetworkClient.isCurrentThreadUiThread()) {
            return true
        }
        val vmName = (System.getProperty("java.vm.name") ?: "").lowercase()
        val isAndroidVm = vmName.contains("dalvik") || vmName.contains("art")
        if (!isAndroidVm) return false
        return try {
            val mainLooper = android.os.Looper.getMainLooper()
            mainLooper != null && mainLooper.thread === Thread.currentThread()
        } catch (_: Throwable) {
            false
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
        netClient.setCrashReporter(_crashReporter)
        OctiCrashReporter.updateGameContext(_matchType.value.name, livePlyCount(), board.toPositionString())
        val prefs = configStore.preferences()
        if (prefs.installId.isEmpty()) {
            prefs.installId = ClientConfigStore.generateUuidV4()
            configStore.save()
        }
        netClient.setInstallId(prefs.installId)
        _theme.value = prefs.getTheme()
        _confirmAllMoves.value = true
        _sfxMuted.value = prefs.isSfxMuted()
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
                bio = prefs.bio
                location = prefs.location
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
        if (visible && _isAuthenticated.value) {
            refreshMyProfileAsync()
        }
    }

    fun setShowCreateTableDialog(visible: Boolean) {
        _showCreateTableDialog.value = visible
    }

    fun setShowBoardChatZoomDialog(show: Boolean) {
        _showBoardChatZoomDialog.value = show
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
        _botPlayStyle.value = difficulty.name.lowercase(Locale.ROOT).replaceFirstChar { it.uppercase() }
        botEngine.setDifficulty(difficulty)
        _botTimeBudgetMs.value = botEngine.timeBudgetMs.toLong()
    }

    fun isInteractiveOnlineTurn(): Boolean {
        if (_matchType.value != MatchType.ONLINE_ROOM) {
            return true
        }
        val gv = _activeGameView.value ?: return false
        if (gv.phase != "IN_PROGRESS") {
            return false
        }
        val currentTurn = board.game?.turn ?: return false
        return currentTurn.value == gv.viewerSeat
    }

    val isOnlineUndoVisible: Boolean
        get() = false

    fun isOnlineRoomActive(): Boolean {
        return _matchType.value == MatchType.ONLINE_ROOM && _activeRoomId.value.isNotEmpty()
    }

    fun requestOrAcceptOnlineUndo(): Boolean {
        return false
    }

    fun handleBoardSquareSingleTap(sqX: Int, sqY: Int): Boolean {
        if (!isInteractiveOnlineTurn() || isViewingHistory() || _awaitingMatchStart.value) {
            return false
        }
        return synchronized(board) {
            val g = board.game ?: return@synchronized false
            if (!g.isGameStarted || g.isGameOver || !board.isValidCoord(sqX - 1, sqY - 1)) {
                return@synchronized false
            }
            if (composer.isGuidedMoveActive || composer.isAwaitingConfirmation) {
                return@synchronized false
            }
            val sq = board.squareAt(sqX - 1, sqY - 1)
            val activeTurn = g.turn ?: return@synchronized false
            if (sq.numPods == 0) {
                if (board.gameMode != OctiGameMode.GAME_MODE_OFK &&
                    board.isHomeSquareOf(sqX - 1, sqY - 1, activeTurn) &&
                    g.getFreePods(activeTurn) > 0
                ) {
                    if (composer.stageOrCommitAddPod(sqX, sqY, false)) {
                        soundSynth.triggerCue(OctiSoundSynth.SoundCue.POD_DROP)
                        notifyComposerUpdated()
                        return@synchronized true
                    }
                }
                return@synchronized false
            }
            if (!sq.isEmpty && !sq.hasEnemyOf(activeTurn)) {
                val pIdx = composer.selectedStackPodIndex(sqX, sqY)
                val resolvedIdx = if (pIdx >= 0 && pIdx < sq.numPods) pIdx else 0
                if (!composer.beginGuidedPodMove(sqX, sqY, resolvedIdx, _moveEntireMatchingStack.value)) {
                    openZoomedSquare(sqX, sqY)
                }
                bumpRevision()
                return@synchronized true
            }
            false
        }
    }

    fun handleBoardSquareZoomInspect(sqX: Int, sqY: Int): Boolean {
        if (_awaitingMatchStart.value && _matchType.value != MatchType.ONLINE_ROOM) {
            return false
        }
        if (isViewingHistory()) {
            return false
        }
        val isOnlineFinished = _matchType.value == MatchType.ONLINE_ROOM && _activeGameView.value?.phase == "FINISHED"
        return synchronized(board) {
            val g = board.game
            if ((g?.isGameOver == true) || isOnlineFinished || !board.isValidCoord(sqX - 1, sqY - 1)) {
                return@synchronized false
            }
            val sq = board.squareAt(sqX - 1, sqY - 1)
            val turn = g?.turn
            val canEdit = isInteractiveOnlineTurn()
            val isHome = if (canEdit && turn != null && board.gameMode != OctiGameMode.GAME_MODE_OFK) {
                board.isHomeSquareOf(sqX - 1, sqY - 1, turn) && !sq.hasEnemyOf(turn)
            } else {
                false
            }
            if (!sq.isEmpty || isHome) {
                if (canEdit) {
                    val isOriginWithOther = composer.isAwaitingConfirmation &&
                        composer.hasOtherMovablePodAtOrigin() &&
                        composer.guidedOriginSquare().let { it[0] == sqX && it[1] == sqY }
                    if (composer.isGuidedMoveActive && !composer.isAwaitingConfirmation && !isOriginWithOther) {
                        composer.cancelAndRollback()
                    }
                }
                openZoomedSquare(sqX, sqY)
                notifyComposerUpdated()
                return@synchronized true
            }
            false
        }
    }

    fun setOnlineMatchForTest(roomId: String, gv: OctiNetworkClient.GameViewData?) {
        _matchType.value = MatchType.ONLINE_ROOM
        _activeRoomId.value = roomId
        if (gv != null) {
            applyGameViewToBoard(gv)
        }
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
            _botPlayStyle.value = difficulty.name.lowercase(Locale.ROOT).replaceFirstChar { it.uppercase() }
            botEngine.setDifficulty(difficulty)
            _botTimeBudgetMs.value = botEngine.timeBudgetMs.toLong()
            _humanSeat.value = humanPlayer

            _fenHistory.clear()
            _moveHistory.clear()
            _fenHistory.add(initialFen)
            _lastSubmittedOnlinePly = 0
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

    fun fenHistorySize(): Int {
        return _fenHistory.size
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

    @JvmOverloads
    fun applyMoveNotation(
        notation: String,
        aiPlyMeta: OctiGame.OctiPlyMetadata? = null
    ): Boolean {
        if (notation.isBlank()) {
            return false
        }
        if (_matchType.value == MatchType.ONLINE_ROOM && _activeRoomId.value.isNotEmpty()) {
            if (isMainThread()) {
                _backgroundExecutor.submit {
                    submitOnlineMove(notation)
                }
                return true
            }
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
                recordAppliedBoardPly(notation, aiPlyMeta)
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
            if (isMainThread()) {
                _backgroundExecutor.submit {
                    submitOnlineMove(stagedNotation)
                }
                return true
            }
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

    fun undoMove(): Boolean {
        if (_matchType.value == MatchType.ONLINE_ROOM || isOnlineRoomActive()) {
            return false
        }
        return undoLocalMove()
    }

    fun resignMatch(): Boolean {
        if (_matchType.value == MatchType.ONLINE_ROOM && _activeRoomId.value.isNotEmpty()) {
            return resignOnlineGame().ok
        }
        return resignLocalGame()
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
                _statusBanner.value = formatGameOverBanner(g.winner)
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
        val plyMeta = OctiGame.OctiPlyMetadata().apply {
            this.elapsedMs = stats.elapsedMs
            this.isAiMove = true
            this.aiDifficulty = _botDifficulty.value.level
            this.aiBudgetMs = botEngine.timeBudgetMs
            this.aiDepthReached = stats.depthReached
            this.aiNodesVisited = stats.nodesVisited
            this.aiBestScore = stats.bestScore
        }
        if (chosen.isEmpty() || !applyMoveNotation(chosen, plyMeta)) {
            return ""
        }
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
                                val plyMeta = OctiGame.OctiPlyMetadata().apply {
                                    this.elapsedMs = stats.elapsedMs
                                    this.isAiMove = true
                                    this.aiDifficulty = _botDifficulty.value.level
                                    this.aiBudgetMs = botEngine.timeBudgetMs
                                    this.aiDepthReached = stats.depthReached
                                    this.aiNodesVisited = stats.nodesVisited
                                    this.aiBestScore = stats.bestScore
                                }
                                moveApplied = applyMoveNotation(candidate, plyMeta)
                                if (!moveApplied) {
                                    val fallback = board.game?.generateLegalMoves()?.firstOrNull()?.asString() ?: ""
                                    if (fallback.isNotEmpty() && fallback != candidate) {
                                        moveApplied = applyMoveNotation(fallback, plyMeta)
                                    }
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

            val signedInHandle = _currentProfile.value?.handle?.trim()?.takeIf { it.isNotEmpty() }
                ?: configStore.preferences().handle?.trim()?.takeIf { it.isNotEmpty() && _isAuthenticated.value }
            val humanHandle = signedInHandle ?: "Player"

            val is4P = (g.mode == OctiGameMode.GAME_MODE_4)
            val vsAi = (_matchType.value == MatchType.LOCAL_VS_BOT)
            val humanSeat = _humanSeat.value

            if (is4P) {
                val seatPlayers = listOf(
                    OctiPlayer.PLAYER_A,
                    OctiPlayer.PLAYER_B,
                    OctiPlayer.PLAYER_C,
                    OctiPlayer.PLAYER_D
                )
                for (seatP in seatPlayers) {
                    val pMeta = OctiGame.OctiPlayerMetadata().apply {
                        this.seat = seatP.value
                        this.player = seatP
                        if (seatP == humanSeat) {
                            this.type = "HUMAN"
                            this.handle = humanHandle
                        } else if (vsAi) {
                            this.type = "AI"
                            this.handle = "AI_L" + _botDifficulty.value.level
                            this.aiDifficulty = _botDifficulty.value.level
                            this.aiBudgetMs = botEngine.timeBudgetMs
                        } else {
                            this.type = "HUMAN"
                            this.engine = "ANDROID_LOCAL"
                            this.handle = "Player " + seatP.name.removePrefix("PLAYER_")
                        }
                    }
                    this.players.add(pMeta)
                }
            } else {
                val otherSeat = if (humanSeat == OctiPlayer.PLAYER_A) OctiPlayer.PLAYER_B else OctiPlayer.PLAYER_A
                val p1 = if (humanSeat == OctiPlayer.PLAYER_A) humanSeat else otherSeat
                val p2 = if (humanSeat == OctiPlayer.PLAYER_A) otherSeat else humanSeat

                val makeMeta = { seatP: OctiPlayer, seatNum: Int ->
                    OctiGame.OctiPlayerMetadata().apply {
                        this.seat = seatNum
                        this.player = seatP
                        if (seatP == humanSeat) {
                            this.type = "HUMAN"
                            this.handle = humanHandle
                        } else if (vsAi) {
                            this.type = "AI"
                            this.handle = "AI_L" + _botDifficulty.value.level
                            this.aiDifficulty = _botDifficulty.value.level
                            this.aiBudgetMs = botEngine.timeBudgetMs
                        } else {
                            this.type = "HUMAN"
                            this.engine = "ANDROID_LOCAL"
                            this.handle = if (humanSeat == OctiPlayer.PLAYER_A) "Player B" else "Player A"
                        }
                    }
                }
                this.players.add(makeMeta(p1, 1))
                this.players.add(makeMeta(p2, 2))
            }

            if (g.isGameOver && !g.isGameResigned) {
                this.termination = g.deduceTermination()
            }

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

    private fun recordAppliedBoardPly(
        notation: String,
        aiPlyMeta: OctiGame.OctiPlyMetadata? = null
    ) {
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
            if (aiPlyMeta != null) {
                g.setLastPlyMetadata(aiPlyMeta)
                _currentTurnStartEpochMs = System.currentTimeMillis()
            } else {
                val lastMeta = g.getPlyMetadata(g.numMoves - 1)
                if (!lastMeta.isAiMove && lastMeta.elapsedMs == 0L) {
                    val nowMs = System.currentTimeMillis()
                    val thinkMs = Math.max(0L, nowMs - _currentTurnStartEpochMs)
                    lastMeta.elapsedMs = thinkMs
                    g.setLastPlyMetadata(lastMeta)
                    _currentTurnStartEpochMs = nowMs
                }
            }
        }

        if (board.game.isGameOver) {
            soundSynth.triggerCue(OctiSoundSynth.SoundCue.GAME_VICTORY)
            _statusBanner.value = formatGameOverBanner(board.game.winner)
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
                _statusBanner.value = formatGameOverBanner(board.game.winner)
                _showGameOverDialog.value = true
                uploadCompletedBotGameIfEligible()
            } else {
                soundSynth.triggerCue(OctiSoundSynth.SoundCue.POD_STEP)
            }
            syncRotationToActiveSeat()
            if (_matchType.value == MatchType.ONLINE_ROOM && _activeRoomId.value.isNotEmpty() && !composer.hasActiveTurn && livePlyCount() > _lastSubmittedOnlinePly) {
                val moveList = board.game?.moveHistory ?: emptyList()
                val lastMoveStr = moveList.lastOrNull()?.asString() ?: ""
                _lastSubmittedOnlinePly = livePlyCount()
                if (lastMoveStr.isNotEmpty()) {
                    if (isMainThread()) {
                        _backgroundExecutor.submit {
                            val res = submitOnlineMove(lastMoveStr)
                            if (!res.ok) {
                                refreshActiveGameView()
                            }
                        }
                    } else {
                        val res = submitOnlineMove(lastMoveStr)
                        if (!res.ok) {
                            refreshActiveGameView()
                        }
                    }
                }
            }
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
            prefs.setBio(d.profile.bio)
            prefs.setLocation(d.profile.location)
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
    ): OctiNetworkClient.ProfileResult {
        val trimmed = handle.trim()
        val dName = if (displayName.isNotEmpty()) displayName else (_currentProfile.value?.displayName ?: "")
        val apiRes = netClient.updateMyProfile(trimmed, "", dName)
        val res = OctiNetworkClient.ProfileResult.fromApiResult(apiRes)
        if (res.ok && res.profile != null) {
            _currentProfile.value = res.profile
            _onboardingRequired.value = false
            _profileActionMessage.value = ""
            _browserAuthStatusText.value = ""
            val prefs = configStore.preferences()
            prefs.setHandle(res.profile.handle)
            prefs.setDisplayName(res.profile.displayName)
            prefs.setAvatarUrl(res.profile.avatarUrl)
            prefs.setBio(res.profile.bio)
            prefs.setLocation(res.profile.location)
            configStore.save()
            _statusBanner.value = "Handle updated to ${res.profile.handle}"
        } else {
            val errRaw = (res.errorMessage.ifEmpty { res.errorCode ?: "" }).lowercase()
            val userMsg = when {
                errRaw.contains("handle_taken") -> "Handle '@$trimmed' is already taken. Please choose another."
                errRaw.contains("invalid_handle") -> "Handle must be 3–32 characters (letters, numbers, _, ., or -)."
                res.errorMessage.isNotEmpty() -> res.errorMessage
                else -> "Failed to save handle: ${res.error.ifEmpty { "network error" }}"
            }
            _profileActionMessage.value = userMsg
            _browserAuthStatusText.value = userMsg
            _statusBanner.value = "Handle update failed: ${res.errorMessage}"
        }
        return res
    }

    @JvmOverloads
    fun updateMyProfile(
        desiredHandle: String,
        displayName: String,
        location: String,
        bio: String,
        avatarUrl: String,
        setAvatarUrl: Boolean = false
    ): OctiNetworkClient.ProfileResult {
        val handle = desiredHandle.trim()
        if (handle.isNotEmpty() && !handle.matches(Regex("^[A-Za-z0-9_.-]{3,32}$"))) {
            val res = OctiNetworkClient.ProfileResult()
            res.ok = false
            res.error = "invalid_handle"
            res.errorMessage = "Handle must be 3–32 characters (letters, numbers, _, ., or -)."
            _profileActionMessage.value = res.errorMessage
            return res
        }
        val oldHandle = _currentProfile.value?.handle ?: ""
        val apiRes = netClient.updateMyProfile(handle, avatarUrl, displayName, bio, location, setAvatarUrl)
        val res = OctiNetworkClient.ProfileResult.fromApiResult(apiRes)
        if (res.ok && res.profile != null) {
            runOnUiThreadOrSync {
                _currentProfile.value = res.profile
                _onboardingRequired.value = false
                val prefs = configStore.preferences()
                if (res.profile.handle.isNotEmpty()) {
                    prefs.setHandle(res.profile.handle)
                }
                prefs.setDisplayName(res.profile.displayName)
                prefs.setAvatarUrl(res.profile.avatarUrl)
                prefs.setBio(res.profile.bio)
                prefs.setLocation(res.profile.location)
                configStore.save()
                _profileActionMessage.value = if (res.profile.handle.isNotEmpty() && res.profile.handle != oldHandle) {
                    "Profile saved. Handle updated to @${res.profile.handle}"
                } else {
                    "Profile saved."
                }
                _browserAuthStatusText.value = ""
                _statusBanner.value = "Profile saved."
            }
        } else {
            val errRaw = (res.errorMessage.ifEmpty { res.errorCode ?: "" }).lowercase()
            val userMsg = when {
                errRaw.contains("handle_taken") -> "Handle '@$handle' is already taken. Please choose another."
                errRaw.contains("handle_change_rate_limited") || errRaw.contains("rate_limited") || errRaw.contains("24h") || errRaw.contains("24 hours") ->
                    if (res.errorMessage.isNotEmpty() && !res.errorMessage.equals("handle_change_rate_limited", ignoreCase = true)) res.errorMessage
                    else "Handle can only be changed once every 24 hours."
                errRaw.contains("invalid_handle") -> "Handle must be 3–32 characters (letters, numbers, _, ., or -)."
                errRaw.contains("invalid_avatar_url") || errRaw.contains("avatar") ->
                    if (res.errorMessage.isNotEmpty() && !res.errorMessage.equals("invalid_avatar_url", ignoreCase = true)) res.errorMessage
                    else "Invalid avatar image format or size."
                res.errorMessage.isNotEmpty() -> res.errorMessage
                else -> "Failed to save profile: ${res.error.ifEmpty { "network error" }}"
            }
            runOnUiThreadOrSync {
                _profileActionMessage.value = userMsg
                _statusBanner.value = "Profile update failed: ${res.errorMessage}"
            }
        }
        return res
    }

    fun updateMyProfileAsync(
        desiredHandle: String,
        displayName: String,
        location: String,
        bio: String,
        avatarUrl: String,
        setAvatarUrl: Boolean = false,
        onDone: ActionCallback<OctiNetworkClient.ProfileResult>? = null
    ) {
        _profileActionBusy.value = true
        _backgroundExecutor.submit {
            val res = updateMyProfile(desiredHandle, displayName, location, bio, avatarUrl, setAvatarUrl)
            runOnUiThreadOrSync {
                _profileActionBusy.value = false
                onDone?.onDone(res)
            }
        }
    }

    @JvmName("updateMyProfileWithBoolCallbackAsync")
    fun updateMyProfileAsync(
        handle: String,
        avatarUrl: String,
        displayName: String,
        bio: String,
        location: String,
        setAvatarUrl: Boolean,
        onDone: ActionCallback<Boolean>? = null
    ) {
        _profileActionBusy.value = true
        _backgroundExecutor.submit {
            val res = updateMyProfile(
                desiredHandle = handle,
                displayName = displayName,
                location = location,
                bio = bio,
                avatarUrl = avatarUrl,
                setAvatarUrl = setAvatarUrl
            )
            runOnUiThreadOrSync {
                _profileActionBusy.value = false
                onDone?.onComplete(res.ok)
            }
        }
    }

    fun updateMyHandle(desiredHandle: String): OctiNetworkClient.ProfileResult {
        val current = _currentProfile.value
        return updateMyProfile(
            desiredHandle = desiredHandle,
            displayName = current?.displayName ?: "",
            location = current?.location ?: "",
            bio = current?.bio ?: "",
            avatarUrl = current?.avatarUrl ?: "",
            setAvatarUrl = false
        )
    }

    fun updateMyHandleAsync(
        desiredHandle: String,
        onDone: ActionCallback<OctiNetworkClient.ProfileResult>? = null
    ) {
        val current = _currentProfile.value
        updateMyProfileAsync(
            desiredHandle = desiredHandle,
            displayName = current?.displayName ?: "",
            location = current?.location ?: "",
            bio = current?.bio ?: "",
            avatarUrl = current?.avatarUrl ?: "",
            setAvatarUrl = false,
            onDone = onDone
        )
    }

    fun interface ActionCallback<in T> {
        fun onDone(result: T)
    }

    fun <T> ActionCallback<T>.onComplete(result: T) {
        onDone(result)
    }

    fun completeHandleOnboardingAsync(
        desiredHandle: String,
        onDone: ActionCallback<OctiNetworkClient.ProfileResult>? = null
    ) {
        _profileActionBusy.value = true
        _backgroundExecutor.submit {
            val res = completeHandleOnboarding(desiredHandle)
            runOnUiThreadOrSync {
                _profileActionBusy.value = false
                onDone?.onDone(res)
            }
        }
    }

    fun unlinkIdentityAsync(
        provider: String,
        onDone: ActionCallback<OctiNetworkClient.IdentityUnlinkResult>? = null
    ) {
        _profileActionBusy.value = true
        _backgroundExecutor.submit {
            val apiRes = unlinkIdentity(provider)
            val res = OctiNetworkClient.IdentityUnlinkResult.fromApiResult(apiRes)
            runOnUiThreadOrSync {
                _profileActionBusy.value = false
                onDone?.onDone(res)
            }
        }
    }

    fun confirmSwitchToConflictAccountAsync(onDone: ActionCallback<Boolean>? = null) {
        _profileActionBusy.value = true
        _backgroundExecutor.submit {
            val res = confirmSwitchToConflictAccount()
            runOnUiThreadOrSync {
                _profileActionBusy.value = false
                onDone?.onDone(res.ok)
            }
        }
    }

    fun logoutOnlineAsync(onDone: ActionCallback<Boolean>? = null) {
        _profileActionBusy.value = true
        _backgroundExecutor.submit {
            val res = logoutOnline()
            runOnUiThreadOrSync {
                _profileActionBusy.value = false
                onDone?.onDone(res.ok)
            }
        }
    }

    fun refreshLobbyStateAsync() {
        _backgroundExecutor.submit {
            refreshLobbyState()
        }
    }

    fun sendLobbyChatAsync(text: String, onDone: ActionCallback<Boolean>? = null) {
        _backgroundExecutor.submit {
            val res = sendLobbyChat(text)
            runOnUiThreadOrSync {
                onDone?.onDone(res.ok)
            }
        }
    }

    fun createOnlineTableAsync(
        req: OctiNetworkClient.CreateTableRequest,
        onDone: ActionCallback<OctiNetworkClient.TableActionResult>? = null
    ) {
        _backgroundExecutor.submit {
            val apiRes = createOnlineTable(req)
            val res = OctiNetworkClient.TableActionResult.fromApiResult(apiRes, apiRes.data?.roomId ?: "")
            runOnUiThreadOrSync {
                onDone?.onDone(res)
            }
        }
    }

    @JvmOverloads
    fun createOnlineTableAsync(
        modeStr: String,
        timeControl: String = "15+10",
        rated: Boolean = true,
        allowSpectators: Boolean = true,
        botDifficulty: String = "",
        onDone: ActionCallback<OctiNetworkClient.TableActionResult>? = null
    ) {
        val req = OctiNetworkClient.CreateTableRequest().apply {
            this.mode = modeStr
            this.rated = rated
            this.observerPolicy = if (allowSpectators) "OPEN" else "NONE"
        }
        createOnlineTableAsync(req, onDone)
    }

    @JvmOverloads
    fun joinOnlineTableAsync(
        tableId: String,
        preferredSeat: Int = -1,
        passcode: String = "",
        onDone: ActionCallback<OctiNetworkClient.TableActionResult>? = null
    ) {
        _backgroundExecutor.submit {
            val apiRes = joinOnlineTable(tableId, preferredSeat, passcode)
            val res = OctiNetworkClient.TableActionResult.fromApiResult(apiRes, tableId)
            runOnUiThreadOrSync {
                if (!res.ok) {
                    _statusBanner.value = "Failed to sit: ${res.errorMessage}"
                }
                onDone?.onDone(res)
            }
        }
    }

    fun joinOnlineTableAsync(
        tableId: String,
        passcode: String,
        onDone: ActionCallback<OctiNetworkClient.TableActionResult>? = null
    ) {
        joinOnlineTableAsync(tableId, -1, passcode, onDone)
    }

    fun exitOnlineTableViewAsync(
        roomId: String,
        onDone: ActionCallback<Boolean>?
    ) {
        exitOnlineTableViewAsync(roomId, false, onDone)
    }

    @JvmOverloads
    fun exitOnlineTableViewAsync(
        roomId: String = _activeRoomId.value,
        leaveSeatOnServer: Boolean = false,
        onDone: ActionCallback<Boolean>? = null
    ) {
        val targetRoomId = roomId.trim()
        val isActiveRoom = (targetRoomId.isEmpty() || targetRoomId == _activeRoomId.value)
        val resolvedRoomId = if (targetRoomId.isNotEmpty()) targetRoomId else _activeRoomId.value
        val gv = _activeGameView.value
        val viewerSeat = gv?.viewerSeat ?: 0
        val shouldLeaveSeatOnServer = (leaveSeatOnServer || (viewerSeat > 0 && gv?.phase != "IN_PROGRESS"))
        _backgroundExecutor.submit {
            val res = if (viewerSeat == 0 && resolvedRoomId.isNotEmpty()) {
                netClient.unobserveTable(resolvedRoomId)
            } else if (shouldLeaveSeatOnServer && resolvedRoomId.isNotEmpty()) {
                netClient.leaveTable(resolvedRoomId)
            } else {
                OctiNetworkClient.ApiResult(true)
            }
            if (isActiveRoom) {
                netClient.stopGameEventStream()
            }
            refreshLobbyState()
            runOnUiThreadOrSync {
                if (isActiveRoom) {
                    _activeRoomId.value = ""
                    _activeGameView.value = null
                    _gameChatLog.value = emptyList()
                    _lastSubmittedOnlinePly = 0
                    _matchType.value = MatchType.LOCAL_VS_BOT
                    _currentTab.value = AppTab.ONLINE_LOBBY
                    _statusBanner.value = "Returned to lobby"
                }
                onDone?.onDone(res.ok)
            }
        }
    }

    @JvmOverloads
    fun leaveOnlineTableAsync(
        roomId: String = _activeRoomId.value,
        onDone: ActionCallback<Boolean>? = null
    ) {
        val targetRoomId = roomId.trim()
        _backgroundExecutor.submit {
            val res = leaveOnlineTable(targetRoomId)
            runOnUiThreadOrSync {
                onDone?.onDone(res.ok)
            }
        }
    }

    @JvmOverloads
    fun observeOnlineTableAsync(
        tableId: String,
        passcode: String = "",
        onDone: ActionCallback<OctiNetworkClient.TableActionResult>? = null
    ) {
        _backgroundExecutor.submit {
            val apiRes = observeOnlineTable(tableId, passcode)
            val res = OctiNetworkClient.TableActionResult.fromApiResult(apiRes, tableId)
            runOnUiThreadOrSync {
                onDone?.onDone(res)
            }
        }
    }

    fun enqueueMatchmakingAsync(
        req: OctiNetworkClient.MatchmakingRequest,
        onDone: ActionCallback<OctiNetworkClient.MatchmakingStatusResult>? = null
    ) {
        _backgroundExecutor.submit {
            val apiRes = enqueueMatchmaking(req)
            val statusRes = pollMatchmakingStatus()
            val res = OctiNetworkClient.MatchmakingStatusResult.fromApiResult(statusRes)
            if (!apiRes.ok) {
                res.ok = false
                res.error = apiRes.errorMessage
            }
            runOnUiThreadOrSync {
                onDone?.onDone(res)
            }
        }
    }

    fun dequeueMatchmakingAsync(onDone: ActionCallback<Boolean>? = null) {
        _backgroundExecutor.submit {
            val res = dequeueMatchmaking()
            runOnUiThreadOrSync {
                onDone?.onDone(res.ok)
            }
        }
    }

    @JvmOverloads
    fun refreshLeaderboardAsync(
        mode: String = "2P",
        limit: Int = 50
    ) {
        _backgroundExecutor.submit {
            refreshLeaderboard(mode, limit)
        }
    }

    @JvmOverloads
    fun refreshArchivedGamesAsync(
        mode: String = "",
        playerId: String = "",
        limit: Int = 20,
        offset: Int = 0
    ) {
        _backgroundExecutor.submit {
            refreshArchivedGames(mode, playerId, limit, offset)
        }
    }

    fun openArchivedGameForReplayAsync(
        gameId: String,
        onDone: ActionCallback<Boolean>? = null
    ) {
        _backgroundExecutor.submit {
            val ok = openArchivedGameForReplay(gameId)
            runOnUiThreadOrSync {
                onDone?.onDone(ok)
            }
        }
    }

    fun purgeArchivedGame(gameId: String) {
        _backgroundExecutor.submit {
            val res = netClient.purgeArchivedGame(gameId)
            if (res.ok) {
                refreshArchivedGamesAsync()
            }
        }
    }

    @JvmOverloads
    fun purgeOnlineTableAsync(
        roomId: String,
        onDone: ActionCallback<Boolean>? = null
    ) {
        val targetRoomId = roomId.trim()
        if (targetRoomId.isEmpty()) {
            onDone?.onDone(false)
            return
        }
        _backgroundExecutor.submit {
            val res = netClient.purgeTable(targetRoomId)
            val isTargetActive = (targetRoomId == _activeRoomId.value)
            if (res.ok) {
                if (isTargetActive) {
                    netClient.stopGameEventStream()
                }
                refreshLobbyState()
            }
            runOnUiThreadOrSync {
                if (res.ok) {
                    if (isTargetActive) {
                        _activeRoomId.value = ""
                        _activeGameView.value = null
                        _gameChatLog.value = emptyList()
                        _lastSubmittedOnlinePly = 0
                        _matchType.value = MatchType.LOCAL_PASS_AND_PLAY
                        _currentTab.value = AppTab.ONLINE_LOBBY
                        _statusBanner.value = "Table #$targetRoomId deleted"
                    }
                } else {
                    _statusBanner.value = "Delete table failed: ${res.errorMessage}"
                }
                onDone?.onDone(res.ok)
            }
        }
    }

    fun purgeTable(roomId: String) {
        purgeOnlineTableAsync(roomId)
    }

    @JvmOverloads
    fun refreshRobotsDirectoryAsync(
        query: String = "",
        sort: String = "wins"
    ) {
        _backgroundExecutor.submit {
            val res = netClient.fetchRobotsDirectory(query, sort)
            runOnUiThreadOrSync {
                if (res.ok && res.data != null) {
                    _robotsDirectory.value = res.data
                }
            }
        }
    }

    fun selectRobotProfileAsync(robotIdOrUuid: String) {
        if (robotIdOrUuid.isEmpty()) {
            _selectedRobotProfile.value = null
            return
        }
        _backgroundExecutor.submit {
            val res = netClient.fetchRobotProfile(robotIdOrUuid)
            runOnUiThreadOrSync {
                if (res.ok && res.data != null) {
                    _selectedRobotProfile.value = res.data
                }
            }
        }
    }

    fun clearSelectedRobotProfile() {
        _selectedRobotProfile.value = null
    }

    fun linkIdentity(
        provider: String,
        token: String
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.LinkIdentityData> {
        val isAccess = "facebook".equals(provider, ignoreCase = true)
        val idToken = if (isAccess) "" else token
        val accessToken = if (isAccess) token else ""
        return linkIdentity(provider, idToken, accessToken)
    }

    fun linkIdentity(
        provider: String,
        idToken: String,
        accessToken: String
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.LinkIdentityData> {
        val res = netClient.linkIdentity(provider, idToken, accessToken, false)
        if (res.ok && res.data != null) {
            _linkedIdentities.value = ArrayList(res.data.identities)
            _linkConflictProfile.value = null
            _pendingConflictProvider = ""
            _pendingConflictToken = ""
            _pendingConflictIdToken = ""
            _pendingConflictAccessToken = ""
            _statusBanner.value = "Linked $provider identity"
        } else if (res.httpStatus == 409 && res.data != null && res.data.conflictAlreadyLinked) {
            _linkConflictProfile.value = res.data.existingProfile
            _pendingConflictProvider = provider
            _pendingConflictToken = if (idToken.isNotEmpty()) idToken else accessToken
            _pendingConflictIdToken = idToken
            _pendingConflictAccessToken = accessToken
            _statusBanner.value =
                "Identity already linked to ${res.data.existingProfile.handle}"
        } else {
            _statusBanner.value = "Link failed: ${res.errorMessage}"
        }
        return res
    }

    fun mergeConflictingIdentityIntoCurrentAccount(): java.util.concurrent.Future<*>? {
        val prov = _pendingConflictProvider
        val idTok = _pendingConflictIdToken
        val accTok = _pendingConflictAccessToken
        if (prov.isEmpty()) {
            return null
        }
        return _backgroundExecutor.submit {
            val res = netClient.linkIdentity(prov, idTok, accTok, true)
            if (res.ok && res.data != null) {
                runOnUiThreadOrSync {
                    _linkedIdentities.value = ArrayList(res.data.identities)
                    _linkConflictProfile.value = null
                    _pendingConflictProvider = ""
                    _pendingConflictToken = ""
                    _pendingConflictIdToken = ""
                    _pendingConflictAccessToken = ""
                    _statusBanner.value = "Merged $prov identity into current account"
                }
            } else {
                runOnUiThreadOrSync {
                    _statusBanner.value = "Merge failed: ${res.errorMessage}"
                }
            }
        }
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
        _pendingConflictIdToken = ""
        _pendingConflictAccessToken = ""
        return loginWithSso(prov, tok)
    }

    fun dismissLinkConflict() {
        _linkConflictProfile.value = null
        _pendingConflictProvider = ""
        _pendingConflictToken = ""
        _pendingConflictIdToken = ""
        _pendingConflictAccessToken = ""
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
            runOnUiThreadOrSync {
                _currentProfile.value = res.data
                _linkedIdentities.value = ArrayList(res.data.identities)
                val prefs = configStore.preferences()
                prefs.setHandle(res.data.handle)
                prefs.setDisplayName(res.data.displayName)
                prefs.setAvatarUrl(res.data.avatarUrl)
                prefs.setBio(res.data.bio)
                prefs.setLocation(res.data.location)
                configStore.save()
            }
        }
        return res
    }

    fun refreshMyProfileAsync(onDone: ActionCallback<Boolean>? = null) {
        if (!_isAuthenticated.value || configStore.preferences().sessionToken.isEmpty()) {
            runOnUiThreadOrSync {
                onDone?.onComplete(false)
            }
            return
        }
        _backgroundExecutor.submit {
            val res = refreshMyProfile()
            runOnUiThreadOrSync {
                onDone?.onComplete(res.ok)
            }
        }
    }

    fun logoutOnline(): OctiNetworkClient.ApiResult<Boolean> {
        cancelBrowserAuth()
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

    fun setPreferDeepLinkRedirect(prefer: Boolean) {
        _preferDeepLinkRedirect = prefer
    }

    private fun resolveProviderClientId(provider: String): String {
        val existing = _authProviders.value.firstOrNull {
            it.name.equals(provider, ignoreCase = true)
        }?.clientIds?.firstOrNull()
        if (!existing.isNullOrBlank()) {
            return existing
        }
        val fetched = refreshAuthProviders()
        if (fetched.ok && fetched.data != null) {
            val found = fetched.data.firstOrNull {
                it.name.equals(provider, ignoreCase = true)
            }?.clientIds?.firstOrNull()
            if (!found.isNullOrBlank()) {
                return found
            }
        }
        return ""
    }

    fun completeNativeSocialAuth(provider: String, token: String) {
        val prov = provider.trim().lowercase().ifEmpty { _browserAuthProvider }
        val isLink = _browserAuthLinkMode
        _isBrowserAuthInProgress.value = true
        _browserAuthStatusText.value = "Signing in with ${if (prov == "facebook") "Facebook" else "Google"}..."
        _backgroundExecutor.submit {
            if (isLink) {
                val res = linkIdentity(prov, token)
                runOnUiThreadOrSync {
                    _isBrowserAuthInProgress.value = false
                    if (res.ok || res.httpStatus == 409) {
                        _browserAuthStatusText.value = ""
                    } else {
                        _browserAuthStatusText.value =
                            res.errorMessage.ifEmpty { "Could not connect account right now." }
                    }
                }
            } else {
                val res = loginWithSso(prov, token)
                runOnUiThreadOrSync {
                    _isBrowserAuthInProgress.value = false
                    if (res.ok) {
                        _browserAuthStatusText.value = ""
                        if (!_onboardingRequired.value) {
                            _showSsoDialog.value = false
                        }
                    } else {
                        _browserAuthStatusText.value =
                            res.errorMessage.ifEmpty { "Could not sign in right now. Please try again." }
                    }
                }
            }
        }
    }

    fun failNativeSocialAuth(errorMessage: String) {
        runOnUiThreadOrSync {
            _isBrowserAuthInProgress.value = false
            _browserAuthStatusText.value = errorMessage
        }
    }

    @JvmOverloads
    fun startBrowserSignIn(
        provider: String,
        openUrlCallback: ((String) -> Unit)? = null
    ): String {
        val prov = provider.trim().lowercase()
        _browserAuthLinkMode = false
        _browserAuthProvider = prov
        _loopbackAuthListener.stop()
        val provLabel = if (prov == "facebook") "Facebook" else "Google"

        val nativeLauncher = nativeSocialAuthLauncher
        if (nativeLauncher != null && prov == "facebook") {
            _isBrowserAuthInProgress.value = true
            _browserAuthStatusText.value = "Opening $provLabel app..."
            _backgroundExecutor.submit {
                val clientId = resolveProviderClientId(prov)
                runOnUiThreadOrSync {
                    val handled = if (clientId.isNotEmpty()) {
                        nativeLauncher.invoke(prov, false, clientId) {
                            val fallbackUrl = netClient.buildBrowserAuthorizeUrl(prov, "octi://auth/callback")
                            _browserAuthStatusText.value = "Completing sign-in with $provLabel..."
                            openUrlCallback?.invoke(fallbackUrl)
                        }
                    } else {
                        false
                    }
                    if (!handled) {
                        val fallbackUrl = netClient.buildBrowserAuthorizeUrl(prov, "octi://auth/callback")
                        _browserAuthStatusText.value = "Completing sign-in with $provLabel..."
                        openUrlCallback?.invoke(fallbackUrl)
                    }
                }
            }
            return netClient.buildBrowserAuthorizeUrl(prov, "octi://auth/callback")
        }

        val redirectUri = if (_preferDeepLinkRedirect) {
            "octi://auth/callback"
        } else {
            val errOut = StringBuilder()
            val started = _loopbackAuthListener.start({ cbProv, cbTok ->
                val resolvedProv = cbProv.ifEmpty { prov }
                _backgroundExecutor.submit {
                    val res = loginWithSso(resolvedProv, cbTok)
                    runOnUiThreadOrSync {
                        _isBrowserAuthInProgress.value = false
                        if (res.ok) {
                            _browserAuthStatusText.value = ""
                            if (!_onboardingRequired.value) {
                                _showSsoDialog.value = false
                            }
                        } else {
                            _browserAuthStatusText.value =
                                res.errorMessage.ifEmpty { "Could not sign in right now. Please try again." }
                        }
                    }
                }
            }, errOut)
            if (started && _loopbackAuthListener.callbackUrl().isNotEmpty()) {
                _loopbackAuthListener.callbackUrl()
            } else {
                "octi://auth/callback"
            }
        }
        val url = netClient.buildBrowserAuthorizeUrl(prov, redirectUri)
        _isBrowserAuthInProgress.value = true
        _browserAuthStatusText.value = "Completing sign-in with $provLabel in your browser..."
        openUrlCallback?.invoke(url)
        return url
    }

    @JvmOverloads
    fun startBrowserLink(
        provider: String,
        openUrlCallback: ((String) -> Unit)? = null
    ): String {
        val prov = provider.trim().lowercase()
        _browserAuthLinkMode = true
        _browserAuthProvider = prov
        _loopbackAuthListener.stop()
        val provLabel = if (prov == "facebook") "Facebook" else "Google"

        val nativeLauncher = nativeSocialAuthLauncher
        if (nativeLauncher != null && prov == "facebook") {
            _isBrowserAuthInProgress.value = true
            _browserAuthStatusText.value = "Opening $provLabel app..."
            _backgroundExecutor.submit {
                val clientId = resolveProviderClientId(prov)
                runOnUiThreadOrSync {
                    val handled = if (clientId.isNotEmpty()) {
                        nativeLauncher.invoke(prov, true, clientId) {
                            val fallbackUrl = netClient.buildBrowserAuthorizeUrl(prov, "octi://auth/callback")
                            _browserAuthStatusText.value = "Connecting $provLabel..."
                            openUrlCallback?.invoke(fallbackUrl)
                        }
                    } else {
                        false
                    }
                    if (!handled) {
                        val fallbackUrl = netClient.buildBrowserAuthorizeUrl(prov, "octi://auth/callback")
                        _browserAuthStatusText.value = "Connecting $provLabel..."
                        openUrlCallback?.invoke(fallbackUrl)
                    }
                }
            }
            return netClient.buildBrowserAuthorizeUrl(prov, "octi://auth/callback")
        }

        val redirectUri = if (_preferDeepLinkRedirect) {
            "octi://auth/callback"
        } else {
            val errOut = StringBuilder()
            val started = _loopbackAuthListener.start({ cbProv, cbTok ->
                val resolvedProv = cbProv.ifEmpty { prov }
                _backgroundExecutor.submit {
                    val res = linkIdentity(resolvedProv, cbTok)
                    runOnUiThreadOrSync {
                        _isBrowserAuthInProgress.value = false
                        if (res.ok) {
                            _browserAuthStatusText.value = ""
                        } else if (res.httpStatus == 409) {
                            _browserAuthStatusText.value = ""
                        } else {
                            _browserAuthStatusText.value =
                                res.errorMessage.ifEmpty { "Could not connect account right now." }
                        }
                    }
                }
            }, errOut)
            if (started && _loopbackAuthListener.callbackUrl().isNotEmpty()) {
                _loopbackAuthListener.callbackUrl()
            } else {
                "octi://auth/callback"
            }
        }
        val url = netClient.buildBrowserAuthorizeUrl(prov, redirectUri)
        _isBrowserAuthInProgress.value = true
        _browserAuthStatusText.value = "Connecting $provLabel in your browser..."
        openUrlCallback?.invoke(url)
        return url
    }

    fun cancelBrowserAuth() {
        _loopbackAuthListener.stop()
        _isBrowserAuthInProgress.value = false
        _browserAuthStatusText.value = ""
        _browserAuthLinkMode = false
    }

    fun loopbackAuthPort(): Int = _loopbackAuthListener.boundPort()

    fun handleOAuthRedirectUri(uriString: String?): Boolean {
        if (uriString.isNullOrBlank()) {
            return false
        }
        val qIdx = uriString.indexOf('?')
        val fIdx = uriString.indexOf('#')
        val queryPart = when {
            qIdx >= 0 && fIdx > qIdx -> uriString.substring(qIdx + 1, fIdx)
            qIdx >= 0 -> uriString.substring(qIdx + 1)
            fIdx >= 0 -> uriString.substring(fIdx + 1)
            else -> ""
        }
        if (queryPart.isEmpty()) {
            return false
        }
        val params = LinkedHashMap<String, String>()
        for (pair in queryPart.split("&")) {
            if (pair.isEmpty()) continue
            val eq = pair.indexOf('=')
            if (eq >= 0) {
                val k = java.net.URLDecoder.decode(pair.substring(0, eq), "UTF-8")
                val v = java.net.URLDecoder.decode(pair.substring(eq + 1), "UTF-8")
                params[k] = v
            }
        }
        val prov = params["provider"]?.ifEmpty { _browserAuthProvider } ?: _browserAuthProvider
        val authTok = params["auth_token"] ?: ""
        val idTok = params["id_token"] ?: params["access_token"] ?: params["token"] ?: ""
        val isNew = params["is_new_account"] == "true"
        val handle = params["handle"] ?: ""
        val playerUuid = params["player_uuid"] ?: ""

        _loopbackAuthListener.stop()

        if (_browserAuthLinkMode && idTok.isNotEmpty()) {
            _backgroundExecutor.submit {
                linkIdentity(prov, idTok)
                runOnUiThreadOrSync {
                    _isBrowserAuthInProgress.value = false
                    _browserAuthStatusText.value = ""
                }
            }
            return true
        }

        if (authTok.isNotEmpty() && !_browserAuthLinkMode) {
            _backgroundExecutor.submit {
                val res = attachAuthenticatedSession(authTok, playerUuid, handle)
                runOnUiThreadOrSync {
                    _isBrowserAuthInProgress.value = false
                    _browserAuthStatusText.value = ""
                    if (res.ok) {
                        _onboardingRequired.value = isNew
                        if (isNew) {
                            _showSsoDialog.value = true
                        } else {
                            _showSsoDialog.value = false
                        }
                    }
                }
            }
            return true
        }

        if (idTok.isNotEmpty()) {
            _backgroundExecutor.submit {
                val res = loginWithSso(prov, idTok)
                runOnUiThreadOrSync {
                    _isBrowserAuthInProgress.value = false
                    _browserAuthStatusText.value = ""
                    if (res.ok && !_onboardingRequired.value) {
                        _showSsoDialog.value = false
                    }
                }
            }
            return true
        }

        return false
    }

    /* ================================================================== */
    /* Online Lobby, Tables, Matchmaking & Live Room                      */
    /* ================================================================== */

    fun refreshLobby(): OctiNetworkClient.ApiResult<OctiNetworkClient.LobbyStateData> {
        return refreshLobbyState()
    }

    fun refreshLobbyState(): OctiNetworkClient.ApiResult<OctiNetworkClient.LobbyStateData> {
        val res = if (isMainThread()) {
            _backgroundExecutor.submit<OctiNetworkClient.ApiResult<OctiNetworkClient.LobbyStateData>> {
                netClient.fetchLobbyState()
            }.get()
        } else {
            netClient.fetchLobbyState()
        }
        if (res.ok && res.data != null) {
            _lobbyState.value = res.data
            _lobbyChatMessages.value = res.data.recentChat.map { formatLobbyChatEntry(it) }
        }
        return res
    }

    fun sendLobbyChat(text: String): OctiNetworkClient.ApiResult<Long> {
        if (!netClient.isAuthenticated) {
            val res = OctiNetworkClient.ApiResult<Long>(0L)
            res.ok = false
            res.httpStatus = 401
            res.errorCode = "UNAUTHENTICATED"
            res.errorMessage = "Sign in to chat"
            return res
        }
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
            val roomId = res.data.roomId
            val gvRes = enterOnlineRoom(roomId)
            refreshLobbyState()
            if (gvRes.data?.phase == "WAITING_FOR_PLAYERS") {
                _statusBanner.value = "Table #$roomId created — Waiting for players"
            }
        }
        return res
    }

    @JvmOverloads
    fun joinOnlineTable(
        roomId: String,
        seat: Int = -1,
        passcode: String = ""
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.JoinTableResponse> {
        val res = netClient.joinTable(roomId, seat, passcode)
        if (res.ok && res.data != null) {
            enterOnlineRoom(roomId)
            refreshLobbyState()
        }
        return res
    }

    @JvmOverloads
    fun observeTable(
        roomId: String,
        passcode: String = ""
    ): OctiNetworkClient.ApiResult<Boolean> {
        return observeOnlineTable(roomId, passcode)
    }

    @JvmOverloads
    fun observeOnlineTable(
        roomId: String,
        passcode: String = ""
    ): OctiNetworkClient.ApiResult<Boolean> {
        val res = netClient.observeTable(roomId, passcode)
        if (res.ok) {
            if (!netClient.isAuthenticated) {
                _statusBanner.value = "Observing as anonymous-<ip> — Sign in to play or chat"
            }
            enterOnlineRoom(roomId)
        }
        return res
    }

    @JvmOverloads
    fun leaveOnlineTable(roomId: String = _activeRoomId.value): OctiNetworkClient.ApiResult<Boolean> {
        val targetRoomId = roomId.trim()
        if (targetRoomId.isEmpty()) {
            return OctiNetworkClient.ApiResult(false)
        }
        val isActiveRoom = (targetRoomId == _activeRoomId.value)
        val res = if (isMainThread()) {
            _backgroundExecutor.submit<OctiNetworkClient.ApiResult<Boolean>> {
                if (isActiveRoom) {
                    netClient.stopGameEventStream()
                }
                val r = netClient.leaveTable(targetRoomId)
                val lobbyRes = netClient.fetchLobbyState()
                if (lobbyRes.ok && lobbyRes.data != null) {
                    _lobbyState.value = lobbyRes.data
                    _lobbyChatMessages.value = lobbyRes.data.recentChat.map { formatLobbyChatEntry(it) }
                }
                r
            }.get()
        } else {
            if (isActiveRoom) {
                netClient.stopGameEventStream()
            }
            val r = netClient.leaveTable(targetRoomId)
            refreshLobbyState()
            r
        }
        if (isActiveRoom) {
            runOnUiThreadOrSync {
                _activeRoomId.value = ""
                _activeGameView.value = null
                _gameChatLog.value = emptyList()
                _lastSubmittedOnlinePly = 0
                _matchType.value = MatchType.LOCAL_VS_BOT
                _currentTab.value = AppTab.ONLINE_LOBBY
                _statusBanner.value = "Returned to lobby"
            }
        }
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
        _awaitingMatchStart.value = false
        _gameOverDialogShownForCurrentOnlineMatch = false
        if (_activeRoomId.value != roomId) {
            _gameChatLog.value = emptyList()
        }
        _activeRoomId.value = roomId
        _matchType.value = MatchType.ONLINE_ROOM
        _currentTab.value = AppTab.BOARD
        val gvRes = refreshActiveGameView()
        val sinceSeq = 0L
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
        val res = if (isMainThread()) {
            _backgroundExecutor.submit<OctiNetworkClient.ApiResult<OctiNetworkClient.GameViewData>> {
                netClient.fetchGameView(roomId)
            }.get()
        } else {
            netClient.fetchGameView(roomId)
        }
        if (res.ok && res.data != null) {
            applyGameViewToBoard(res.data)
        }
        return res
    }

    @JvmOverloads
    fun refreshActiveGameViewAsync(
        onDone: ActionCallback<OctiNetworkClient.GameViewData>? = null
    ): java.util.concurrent.Future<OctiNetworkClient.ApiResult<OctiNetworkClient.GameViewData>> {
        return _backgroundExecutor.submit<OctiNetworkClient.ApiResult<OctiNetworkClient.GameViewData>> {
            val res = refreshActiveGameView()
            runOnUiThreadOrSync {
                if (res.ok && res.data != null) {
                    onDone?.onDone(res.data)
                }
            }
            res
        }
    }

    fun applyGameView(gv: OctiNetworkClient.GameViewData) {
        applyGameViewToBoard(gv)
    }

    private fun applyGameViewToBoard(gv: OctiNetworkClient.GameViewData) {
        _awaitingMatchStart.value = false
        val prevPhase = _activeGameView.value?.phase
        _activeGameView.value = gv
        _lastSubmittedOnlinePly = gv.moves.size

        if (gv.rematchRoomId.isNotEmpty() && gv.rematchRoomId != _activeRoomId.value) {
            enterOnlineRoom(gv.rematchRoomId)
            return
        }

        if (gv.phase == "IN_PROGRESS" && prevPhase == "WAITING_FOR_PLAYERS") {
            _currentTab.value = AppTab.BOARD
            _statusBanner.value = "Match #${gv.roomId} started!"
        } else if (gv.phase == "WAITING_FOR_PLAYERS") {
            if (!netClient.isAuthenticated && gv.viewerSeat == 0) {
                _statusBanner.value = "Observing as anonymous — Sign in to play or chat"
            } else {
                val filled = gv.seats.count { it.playerUuid.isNotEmpty() || it.handle.isNotEmpty() }
                val total = gv.seats.size.coerceAtLeast(2)
                _statusBanner.value = "Table #${gv.roomId} — Waiting for players ($filled/$total)"
            }
        }

        val s1 = gv.seats.firstOrNull { it.seat == 1 }
        val s2 = gv.seats.firstOrNull { it.seat == 2 }
        _clockA.value = s1?.clockMs ?: -1L
        _clockB.value = s2?.clockMs ?: -1L

        synchronized(board) {
            val needFullRebuild = (gv.moves != _moveHistory || _fenHistory.isEmpty() || _fenHistory.size != gv.moves.size + 1)
            if (needFullRebuild) {
                try {
                    val mode = modeFromWireCode(gv.mode)
                    val bases = OctiGameBases.fromValue(gv.bases)
                    board.setGameMode(mode, gv.superProngs, gv.edgeless, bases)
                    displayBoard.setGameMode(mode, gv.superProngs, gv.edgeless, bases)
                    board.game?.start(OctiPlayer.PLAYER_A)
                    displayBoard.game?.start(OctiPlayer.PLAYER_A)

                    _fenHistory.clear()
                    _moveHistory.clear()
                    _fenHistory.add(board.toPositionString())

                    var replayOk = true
                    for (m in gv.moves) {
                        try {
                            board.game?.move(m)
                            _moveHistory.add(m)
                            _fenHistory.add(board.toPositionString())
                        } catch (e: Exception) {
                            replayOk = false
                            break
                        }
                    }

                    if (!replayOk || (gv.fen.isNotEmpty() && board.toPositionString() != gv.fen)) {
                        if (gv.fen.isNotEmpty()) {
                            board.fromPositionString(gv.fen)
                            if (_fenHistory.isEmpty() || _fenHistory.last() != gv.fen) {
                                _fenHistory.add(gv.fen)
                            }
                        }
                    }

                    _viewingPly.value = livePlyCount()
                    displayBoard.fromPositionString(board.toPositionString())
                    composer.attachBoard(board)
                    composer.setConfirmAllMoves(_confirmAllMoves.value)

                    val latestMoveStr = gv.moves.lastOrNull() ?: ""
                    if (latestMoveStr.isNotEmpty()) {
                        val edgeless = board.game?.useEdgelessBoard() == true
                        _lastMoveModifiedSquares.value = OctiBoardRenderer.extractModifiedSquares(latestMoveStr, edgeless)
                        _lastMoveIndicatorSetAtMs = System.currentTimeMillis()
                    } else {
                        clearLastMoveIndicators()
                    }
                } catch (ignored: RuntimeException) {
                }
            } else {
                val stagedActive = (composer.hasStagedMove() || _awaitingConfirmation.value)
                if (gv.phase == "IN_PROGRESS" && !gv.paused && stagedActive) {
                    // Preserve staged move across non-move SSE refreshes
                } else {
                    if (gv.fen.isNotEmpty()) {
                        try {
                            board.fromPositionString(gv.fen)
                            displayBoard.fromPositionString(gv.fen)
                            composer.attachBoard(board)
                            composer.setConfirmAllMoves(_confirmAllMoves.value)
                        } catch (ignored: RuntimeException) {
                        }
                    }
                }
            }
        }
        if (gv.phase == "FINISHED") {
            if (!_gameOverDialogShownForCurrentOnlineMatch) {
                _gameOverDialogShownForCurrentOnlineMatch = true
                _showGameOverDialog.value = true
            }
        } else {
            _gameOverDialogShownForCurrentOnlineMatch = false
            if (board.game?.isGameOver == true) {
                _showGameOverDialog.value = true
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
            refreshActiveGameView()
        }
        return res
    }

    fun sendBoardChatMessage(
        scope: String,
        text: String
    ): OctiNetworkClient.ApiResult<Long> {
        val trimmed = text.trim()
        if (trimmed.isEmpty()) {
            val res = OctiNetworkClient.ApiResult<Long>(0L)
            res.ok = false
            return res
        }
        if (_matchType.value != MatchType.ONLINE_ROOM || _activeRoomId.value.isBlank()) {
            val handle = _currentProfile.value?.handle?.ifEmpty { "You" } ?: "You"
            val myUuid = _currentProfile.value?.uuid ?: ""
            val userMsg = GameChatMessage(
                seq = System.currentTimeMillis(),
                scope = scope,
                senderUuid = myUuid,
                senderHandle = handle,
                seat = 1,
                text = text,
                atMs = System.currentTimeMillis()
            )
            val updated = ArrayList(_gameChatLog.value)
            updated.add(userMsg)
            val lower = trimmed.lowercase(Locale.ROOT)
            if (lower.startsWith("@bot") || lower.startsWith("@stock")) {
                val botReplyText = when {
                    lower.contains("help") -> "Bot commands: @bot help, @bot eval, @bot stats"
                    lower.contains("eval") -> {
                        val evalScore = OctiBotEngine.evaluatePosition(board, board.game?.turn ?: OctiPlayer.PLAYER_A)
                        "Position eval score: $evalScore"
                    }
                    lower.contains("stats") -> "Local AI Bot (mode=${_botPlayStyle.value}, timeBudget=${_botTimeBudgetMs.value}ms)"
                    else -> "Bot commands: @bot help, @bot eval, @bot stats"
                }
                val botMsg = GameChatMessage(
                    seq = System.currentTimeMillis() + 1,
                    scope = scope,
                    senderUuid = "bot",
                    senderHandle = "bot",
                    seat = 2,
                    text = botReplyText,
                    atMs = System.currentTimeMillis()
                )
                updated.add(botMsg)
            }
            _gameChatLog.value = updated
            val res = OctiNetworkClient.ApiResult(1L)
            res.ok = true
            return res
        }
        return sendGameChat(scope, text)
    }

    fun sendGameChat(
        scope: String,
        text: String
    ): OctiNetworkClient.ApiResult<Long> {
        val trimmed = text.trim()
        if (trimmed.isEmpty()) {
            val res = OctiNetworkClient.ApiResult<Long>(0L)
            res.ok = false
            return res
        }
        if (_matchType.value != MatchType.ONLINE_ROOM || _activeRoomId.value.isBlank()) {
            return sendBoardChatMessage(scope, text)
        }
        if (!netClient.isAuthenticated) {
            val res = OctiNetworkClient.ApiResult<Long>(0L)
            res.ok = false
            res.httpStatus = 401
            res.errorCode = "UNAUTHENTICATED"
            res.errorMessage = "Sign in to chat"
            return res
        }
        val roomId = _activeRoomId.value
        val res = netClient.postGameChat(roomId, scope, text)
        if (res.ok) {
            val handle = _currentProfile.value?.handle?.ifEmpty { "You" } ?: "You"
            val myUuid = _currentProfile.value?.uuid ?: ""
            val userSeq = res.data ?: System.currentTimeMillis()
            val userMsg = GameChatMessage(
                seq = userSeq,
                scope = scope,
                senderUuid = myUuid,
                senderHandle = handle,
                seat = _activeGameView.value?.viewerSeat ?: 1,
                text = text,
                atMs = System.currentTimeMillis()
            )
            val updated = ArrayList(_gameChatLog.value)
            if (updated.none { (userSeq > 0L && it.seq == userSeq) || (it.seq == userSeq && it.text == userMsg.text) }) {
                updated.add(userMsg)
                _gameChatLog.value = updated
            }
        }
        return res
    }

    @JvmOverloads
    fun sendGameChatAsync(
        scope: String,
        text: String,
        onDone: ((OctiNetworkClient.ApiResult<Long>) -> Unit)? = null
    ) {
        val trimmed = text.trim()
        if (trimmed.isEmpty()) {
            val res = OctiNetworkClient.ApiResult<Long>(0L)
            res.ok = false
            onDone?.invoke(res)
            return
        }
        if (_matchType.value != MatchType.ONLINE_ROOM || _activeRoomId.value.isBlank()) {
            val res = sendBoardChatMessage(scope, text)
            onDone?.invoke(res)
            return
        }
        _backgroundExecutor.submit {
            val roomId = _activeRoomId.value
            val res = if (!netClient.isAuthenticated) {
                val r = OctiNetworkClient.ApiResult<Long>(0L)
                r.ok = false
                r.httpStatus = 401
                r.errorCode = "UNAUTHENTICATED"
                r.errorMessage = "Sign in to chat"
                r
            } else {
                netClient.postGameChat(roomId, scope, text)
            }
            runOnUiThreadOrSync {
                if (res.ok) {
                    val handle = _currentProfile.value?.handle?.ifEmpty { "You" } ?: "You"
                    val myUuid = _currentProfile.value?.uuid ?: ""
                    val userSeq = res.data ?: System.currentTimeMillis()
                    val userMsg = GameChatMessage(
                        seq = userSeq,
                        scope = scope,
                        senderUuid = myUuid,
                        senderHandle = handle,
                        seat = _activeGameView.value?.viewerSeat ?: 1,
                        text = text,
                        atMs = System.currentTimeMillis()
                    )
                    val cur = _gameChatLog.value
                    if (cur.none { (userSeq > 0L && it.seq == userSeq) || (it.seq == userSeq && it.text == userMsg.text) }) {
                        val updated = ArrayList(cur)
                        updated.add(userMsg)
                        _gameChatLog.value = updated
                    }
                } else {
                    _statusBanner.value = "Chat failed: ${res.errorMessage}"
                }
                onDone?.invoke(res)
            }
        }
    }

    @JvmOverloads
    fun sendGameChatAsync(
        text: String,
        onDone: ((OctiNetworkClient.ApiResult<Long>) -> Unit)? = null
    ) {
        sendGameChatAsync("ALL", text, onDone)
    }

    fun sendGameChat(text: String): OctiNetworkClient.ApiResult<Long> {
        return sendGameChat("ALL", text)
    }

    fun formatGameOverBanner(winner: OctiPlayer?): String {
        if (winner == null || winner == OctiPlayer.PLAYER_UNDEF) {
            return "Game Over — Stalemate"
        }
        return if (board.gameMode == OctiGameMode.GAME_MODE_4) {
            if (winner == OctiPlayer.PLAYER_A || winner == OctiPlayer.PLAYER_C) {
                "Team A & C Wins!"
            } else {
                "Team B & D Wins!"
            }
        } else {
            "Game Over — Winner: $winner"
        }
    }

    fun isOnlineGameOver(): Boolean {
        val gv = _activeGameView.value
        if (_matchType.value == MatchType.ONLINE_ROOM && gv?.phase == "FINISHED") {
            return true
        }
        return board.game?.isGameOver == true
    }

    fun describeOnlineGameOverBanner(): String {
        val gv = _activeGameView.value
        if (_matchType.value == MatchType.ONLINE_ROOM && gv?.phase == "FINISHED") {
            if (gv.winnerSeat > 0) {
                val winSeat = gv.winnerSeat
                val seatObj = gv.seats.firstOrNull { it.seat == winSeat }
                val handle = seatObj?.handle?.ifEmpty { "Seat $winSeat" } ?: "Seat $winSeat"
                val cleanHandle = handle.removePrefix("@")
                val isBot = seatObj?.isRobot == true || cleanHandle.equals("stock", ignoreCase = true) || (seatObj?.playerUuid?.startsWith("bot") == true)
                val tag = if (isBot) "@$cleanHandle [BOT]" else "@$cleanHandle"
                return if (gv.mode == "4P") {
                    if (winSeat == 1 || winSeat == 3) {
                        "Team A & C Wins!"
                    } else {
                        "Team B & D Wins!"
                    }
                } else {
                    "Winner: $tag"
                }
            } else if (gv.result == "DRAW") {
                return "Match Drawn"
            } else {
                return "Match Complete"
            }
        }
        val game = board.game
        val winner = game?.winner
        if (winner == null || winner == OctiPlayer.PLAYER_UNDEF) {
            return "Game Over — Stalemate"
        }
        return if (board.gameMode == OctiGameMode.GAME_MODE_4) {
            if (winner == OctiPlayer.PLAYER_A || winner == OctiPlayer.PLAYER_C) {
                "Team A & C Wins!"
            } else {
                "Team B & D Wins!"
            }
        } else {
            "Game Over — Winner: $winner"
        }
    }

    fun describeOnlineTerminationReason(): String {
        val gv = _activeGameView.value
        if (_matchType.value == MatchType.ONLINE_ROOM && gv?.phase == "FINISHED") {
            val term = gv.termination.ifEmpty { "CONCLUDED" }
            return when (term) {
                "RESIGNATION" -> "Match concluded by resignation"
                "AGREED_DRAW" -> "Draw agreed by mutual consent"
                "DISCONNECT_FORFEIT" -> "Opponent forfeited by disconnection"
                "TIMEOUT" -> "Match concluded by time expiration"
                "BASE_OCCUPATION" -> "Winning base occupied"
                "POD_ELIMINATION" -> "All opponent pods captured"
                else -> term.replace('_', ' ').lowercase().replaceFirstChar { it.uppercase() }
            }
        }
        return OctiDialogs.describeTerminationReason(board)
    }

    fun submitGameAction(
        action: String
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.GameActionData> {
        val roomId = _activeRoomId.value
        val res = if (isMainThread()) {
            _backgroundExecutor.submit<OctiNetworkClient.ApiResult<OctiNetworkClient.GameActionData>> {
                netClient.submitGameAction(roomId, action)
            }.get()
        } else {
            netClient.submitGameAction(roomId, action)
        }
        if (res.ok) {
            refreshActiveGameView()
        }
        return res
    }

    fun claimOpponentForfeit(): OctiNetworkClient.ApiResult<OctiNetworkClient.GameActionData> {
        return submitGameAction("claim_forfeit")
    }

    fun forfeitOnlineMatchAsync(): java.util.concurrent.Future<*>? {
        return _backgroundExecutor.submit {
            val roomId = _activeRoomId.value
            val res = netClient.submitGameAction(roomId, "claim_forfeit")
            if (res.ok) {
                refreshActiveGameView()
                runOnUiThreadOrSync {
                    val gv = _activeGameView.value
                    if (gv != null && gv.winnerSeat > 0) {
                        val wSeat = gv.winnerSeat
                        _statusBanner.value = if (board.gameMode == OctiGameMode.GAME_MODE_4) {
                            if (wSeat == 1 || wSeat == 3) "Team A & C Wins!" else "Team B & D Wins!"
                        } else {
                            val wPlayer = OctiPlayer.fromValue(wSeat)
                            "Game Over — Winner: $wPlayer"
                        }
                    }
                }
            }
        }
    }

    fun resignOnlineGame(): OctiNetworkClient.ApiResult<Boolean> {
        val roomId = _activeRoomId.value
        val res = if (isMainThread()) {
            _backgroundExecutor.submit<OctiNetworkClient.ApiResult<Boolean>> {
                netClient.resignGame(roomId)
            }.get()
        } else {
            netClient.resignGame(roomId)
        }
        if (res.ok) {
            refreshActiveGameView()
        }
        return res
    }

    @JvmOverloads
    fun submitGameActionAsync(
        action: String,
        onDone: ActionCallback<OctiNetworkClient.GameActionData>? = null
    ): java.util.concurrent.Future<OctiNetworkClient.ApiResult<OctiNetworkClient.GameActionData>> {
        return _backgroundExecutor.submit<OctiNetworkClient.ApiResult<OctiNetworkClient.GameActionData>> {
            val res = submitGameAction(action)
            runOnUiThreadOrSync {
                if (res.data != null) {
                    onDone?.onDone(res.data)
                }
            }
            res
        }
    }

    @JvmOverloads
    fun resignOnlineGameAsync(
        onDone: ActionCallback<Boolean>? = null
    ): java.util.concurrent.Future<OctiNetworkClient.ApiResult<Boolean>> {
        return _backgroundExecutor.submit<OctiNetworkClient.ApiResult<Boolean>> {
            val res = resignOnlineGame()
            runOnUiThreadOrSync {
                onDone?.onDone(res.ok)
            }
            res
        }
    }

    @JvmOverloads
    fun proposeDraw(
        onDone: ActionCallback<OctiNetworkClient.GameActionData>? = null
    ): java.util.concurrent.Future<OctiNetworkClient.ApiResult<OctiNetworkClient.GameActionData>> {
        val gv = _activeGameView.value
        val myUuid = _currentProfile.value?.uuid ?: configStore.preferences().playerUuid
        val hasOpponentDrawOffer = gv?.proposals?.any { it.type == "DRAW_OFFER" && it.proposerUuid != myUuid } == true
        val action = if (hasOpponentDrawOffer) "accept_draw" else "offer_draw"
        return submitGameActionAsync(action, onDone)
    }

    @JvmOverloads
    fun proposeUndo(
        onDone: ActionCallback<OctiNetworkClient.GameActionData>? = null
    ): java.util.concurrent.Future<OctiNetworkClient.ApiResult<OctiNetworkClient.GameActionData>> {
        return submitGameActionAsync("request_undo", onDone)
    }

    @JvmOverloads
    fun proposePauseOrResume(
        onDone: ActionCallback<OctiNetworkClient.GameActionData>? = null
    ): java.util.concurrent.Future<OctiNetworkClient.ApiResult<OctiNetworkClient.GameActionData>> {
        val gv = _activeGameView.value
        val action = if (gv?.paused == true) "resume" else "request_pause"
        return submitGameActionAsync(action, onDone)
    }

    @JvmOverloads
    fun proposeRematch(
        onDone: ActionCallback<OctiNetworkClient.GameActionData>? = null
    ): java.util.concurrent.Future<OctiNetworkClient.ApiResult<OctiNetworkClient.GameActionData>> {
        val gv = _activeGameView.value
        val myUuid = _currentProfile.value?.uuid ?: configStore.preferences().playerUuid
        val hasOpponentRematch = gv?.proposals?.any { it.type == "REMATCH_REQUEST" && it.proposerUuid != myUuid } == true
        val action = if (hasOpponentRematch) "accept_rematch" else "request_rematch"
        return submitGameActionAsync(action, onDone)
    }

    @JvmOverloads
    fun acceptActiveProposal(
        onDone: ActionCallback<OctiNetworkClient.GameActionData>? = null
    ): java.util.concurrent.Future<OctiNetworkClient.ApiResult<OctiNetworkClient.GameActionData>> {
        val gv = _activeGameView.value
        val prop = gv?.proposals?.firstOrNull()
        val action = when (prop?.type) {
            "DRAW_OFFER" -> "accept_draw"
            "PAUSE_REQUEST" -> "accept_pause"
            "UNDO_REQUEST" -> "accept_undo"
            "REMATCH_REQUEST" -> "accept_rematch"
            else -> "accept_draw"
        }
        return submitGameActionAsync(action, onDone)
    }

    @JvmOverloads
    fun declineActiveProposal(
        onDone: ActionCallback<OctiNetworkClient.GameActionData>? = null
    ): java.util.concurrent.Future<OctiNetworkClient.ApiResult<OctiNetworkClient.GameActionData>> {
        val gv = _activeGameView.value
        val prop = gv?.proposals?.firstOrNull()
        val action = when (prop?.type) {
            "DRAW_OFFER" -> "decline_draw"
            "PAUSE_REQUEST" -> "decline_pause"
            "UNDO_REQUEST" -> "decline_undo"
            "REMATCH_REQUEST" -> "decline_rematch"
            else -> "decline_draw"
        }
        return submitGameActionAsync(action, onDone)
    }

    fun muteParticipant(
        targetUuid: String,
        muteChat: Boolean
    ): OctiNetworkClient.ApiResult<Boolean> {
        val roomId = _activeRoomId.value
        return netClient.muteParticipant(roomId, targetUuid, muteChat)
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
        observerPolicy: String
    ): OctiNetworkClient.ApiResult<OctiNetworkClient.RoomSettingsData> {
        val roomId = _activeRoomId.value
        val res = netClient.updateRoomSettings(roomId, observerPolicy)
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

        if ("CHAT" == ev.event || "CHAT_MESSAGE" == ev.event || "GAME_CHAT" == ev.event) {
            try {
                val j = JSONObject(ev.dataJson)
                val seq = j.optLong("seq", ev.id)
                val cur = _gameChatLog.value
                if (seq > 0L && cur.any { it.seq == seq }) {
                    return
                }
                val msg = GameChatMessage(
                    seq = seq,
                    scope = j.optString("scope", "all"),
                    senderUuid = j.optString("player_uuid", ""),
                    senderHandle = j.optString("handle", ""),
                    seat = j.optInt("seat", 0),
                    text = j.optString("text", ""),
                    atMs = j.optLong("at_ms", System.currentTimeMillis())
                )
                val updated = ArrayList(cur)
                updated.add(msg)
                _gameChatLog.value = updated
            } catch (ignored: Exception) {
            }
        }

        var evType = ""
        var dataObj: JSONObject? = null
        try {
            if (ev.dataJson.isNotEmpty() && ev.dataJson.startsWith("{")) {
                dataObj = JSONObject(ev.dataJson)
                evType = dataObj.optString("type", "")
            }
        } catch (ignored: Exception) {
        }

        val eventName = if (ev.event.isNotEmpty()) ev.event else evType
        val upperEventName = eventName.uppercase(Locale.ROOT)

        if (upperEventName == "REMATCH_CREATED") {
            var newRoomId = ""
            if (dataObj != null) {
                newRoomId = dataObj.optString("room_id", dataObj.optString("text", ""))
            } else if (ev.dataJson.isNotEmpty()) {
                newRoomId = ev.dataJson.trim().removeSurrounding("\"")
            }
            if (newRoomId.isNotEmpty() && newRoomId != _activeRoomId.value) {
                enterOnlineRoom(newRoomId)
                return
            }
        }

        val isLifecycleEvent = when (upperEventName) {
            "SEAT_JOINED", "SEAT_LEFT", "SEAT_KICKED", "HOST_CHANGED",
            "OBSERVER_JOINED", "OBSERVER_LEFT", "GAME_STARTED",
            "MOVE", "MOVE_APPLIED", "UNDO_APPLIED", "MOVE_UNDONE",
            "PROPOSAL_OFFERED", "PROPOSAL_ACCEPTED", "PROPOSAL_DECLINED", "PROPOSAL_EXPIRED",
            "PROPOSAL", "PROPOSAL_UPDATED", "ACTION", "ACTION_APPLIED",
            "GAME_PAUSED", "GAME_RESUMED",
            "PLAYER_DISCONNECTED", "PLAYER_RECONNECTED",
            "GAME_OVER", "POLICY_UPDATED", "PLAYER_JOINED", "PLAYER_LEFT",
            "RECONNECT", "GAP" -> true
            else -> false
        }

        if (isLifecycleEvent) {
            refreshActiveGameView()
        }
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
        cancelBrowserAuth()
        cancelActiveBotTurn()
        if (isMainThread()) {
            try {
                _backgroundExecutor.submit {
                    netClient.close()
                }.get()
            } catch (_: Throwable) {
            }
            _backgroundExecutor.shutdownNow()
        } else {
            _backgroundExecutor.shutdownNow()
            netClient.close()
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
