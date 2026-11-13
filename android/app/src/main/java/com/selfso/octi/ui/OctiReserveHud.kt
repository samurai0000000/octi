/*
 * OctiReserveHud.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.shape.RoundedCornerShape
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
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.StrokeJoin
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.luminance
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.selfso.octi.OctiGameMode
import com.selfso.octi.OctiMoveComposer
import com.selfso.octi.OctiPalette
import com.selfso.octi.OctiPlayer
import com.selfso.octi.ui.theme.OctiTheme
import kotlin.math.cos
import kotlin.math.min
import kotlin.math.roundToInt
import kotlin.math.sin

const val RESERVE_HUD_CONTROLS_HORIZONTAL_PADDING_DP: Int = 6
const val RESERVE_HUD_CHAT_HORIZONTAL_PADDING_DP: Int = 0
const val RESERVE_HUD_CHAT_MIN_HEIGHT_DP: Int = 44
const val RESERVE_HUD_CHAT_STACK_SPACING_DP: Int = 4
const val CHAT_LUMINANCE_LIGHT_THRESHOLD: Float = 0.55f

fun isVerticalFullWidthChatStack(): Boolean = true

fun chatSectionWidthFraction(): Float = 1.0f

fun chatZoomDialogWidthFraction(): Float = 1.0f

fun chatStackOrder(isOnlineRoom: Boolean): List<String> =
    if (isOnlineRoom) listOf("TABLE_CHAT", "LOBBY_CHAT") else listOf("LOBBY_CHAT")

data class WebChatStylePalette(
    val lightSurface: Boolean,
    val pageBgArgb: Int,
    val cardBgArgb: Int,
    val wellBgArgb: Int,
    val cardBorderArgb: Int,
    val inputBorderArgb: Int,
    val headerTitleArgb: Int,
    val timestampArgb: Int,
    val teamScopeArgb: Int,
    val peerHandleArgb: Int,
    val selfHandleArgb: Int,
    val botHandleArgb: Int,
    val systemTagArgb: Int,
    val systemEventBodyArgb: Int,
    val messageBodyArgb: Int,
    val dangerOutlineArgb: Int,
    val dangerTextArgb: Int
) {
    val pageBg: Color get() = Color(pageBgArgb.toLong() and 0xFFFFFFFFL)
    val cardBg: Color get() = Color(cardBgArgb.toLong() and 0xFFFFFFFFL)
    val wellBg: Color get() = Color(wellBgArgb.toLong() and 0xFFFFFFFFL)
    val cardBorder: Color get() = Color(cardBorderArgb.toLong() and 0xFFFFFFFFL)
    val inputBorder: Color get() = Color(inputBorderArgb.toLong() and 0xFFFFFFFFL)
    val headerTitle: Color get() = Color(headerTitleArgb.toLong() and 0xFFFFFFFFL)
    val timestamp: Color get() = Color(timestampArgb.toLong() and 0xFFFFFFFFL)
    val teamScope: Color get() = Color(teamScopeArgb.toLong() and 0xFFFFFFFFL)
    val peerHandle: Color get() = Color(peerHandleArgb.toLong() and 0xFFFFFFFFL)
    val selfHandle: Color get() = Color(selfHandleArgb.toLong() and 0xFFFFFFFFL)
    val botHandle: Color get() = Color(botHandleArgb.toLong() and 0xFFFFFFFFL)
    val systemTag: Color get() = Color(systemTagArgb.toLong() and 0xFFFFFFFFL)
    val systemEventBody: Color get() = Color(systemEventBodyArgb.toLong() and 0xFFFFFFFFL)
    val messageBody: Color get() = Color(messageBodyArgb.toLong() and 0xFFFFFFFFL)
    val dangerOutline: Color get() = Color(dangerOutlineArgb.toLong() and 0xFFFFFFFFL)
    val dangerText: Color get() = Color(dangerTextArgb.toLong() and 0xFFFFFFFFL)
}

data class ResolvedChatSpan(
    val text: String,
    val start: Int,
    val end: Int,
    val colorArgb: Int,
    val isBold: Boolean,
    val isItalic: Boolean,
    val isMonospace: Boolean
)

fun computeColorLuminanceArgb(argb: Int): Float {
    fun linearize(channel8: Int): Float {
        val s = (channel8 and 0xFF) / 255.0f
        return if (s <= 0.04045f) {
            s / 12.92f
        } else {
            Math.pow(((s + 0.055) / 1.055), 2.4).toFloat()
        }
    }
    val r = linearize(argb ushr 16)
    val g = linearize(argb ushr 8)
    val b = linearize(argb)
    return 0.2126f * r + 0.7152f * g + 0.0722f * b
}

fun isLightChatSurfaceArgb(windowBgArgb: Int): Boolean =
    computeColorLuminanceArgb(windowBgArgb) >= CHAT_LUMINANCE_LIGHT_THRESHOLD

fun isLightChatSurface(windowBg: Color): Boolean =
    windowBg.luminance() >= CHAT_LUMINANCE_LIGHT_THRESHOLD

fun resolveWebChatStylePalette(lightSurface: Boolean): WebChatStylePalette {
    return if (lightSurface) {
        WebChatStylePalette(
            lightSurface = true,
            pageBgArgb = 0xFFFBFBFA.toInt(),
            cardBgArgb = 0xFFFFFFFF.toInt(),
            wellBgArgb = 0xFFFBFBFA.toInt(),
            cardBorderArgb = 0xFFE4E4E7.toInt(),
            inputBorderArgb = 0xFFD4D4D8.toInt(),
            headerTitleArgb = 0xFF0F172A.toInt(),
            timestampArgb = 0xFF64748B.toInt(),
            teamScopeArgb = 0xFF7C3AED.toInt(),
            peerHandleArgb = 0xFF0284C7.toInt(),
            selfHandleArgb = 0xFF059669.toInt(),
            botHandleArgb = 0xFFD97706.toInt(),
            systemTagArgb = 0xFFB45309.toInt(),
            systemEventBodyArgb = 0xFF92400E.toInt(),
            messageBodyArgb = 0xFF0F172A.toInt(),
            dangerOutlineArgb = 0xFFDC2626.toInt(),
            dangerTextArgb = 0xFFDC2626.toInt()
        )
    } else {
        WebChatStylePalette(
            lightSurface = false,
            pageBgArgb = 0xFF0B0F17.toInt(),
            cardBgArgb = 0xFF101622.toInt(),
            wellBgArgb = 0xFF0B0F17.toInt(),
            cardBorderArgb = 0xFF1E293B.toInt(),
            inputBorderArgb = 0xFF334155.toInt(),
            headerTitleArgb = 0xFFC8A253.toInt(),
            timestampArgb = 0xFF94A3B8.toInt(),
            teamScopeArgb = 0xFFA78BFA.toInt(),
            peerHandleArgb = 0xFF38BDF8.toInt(),
            selfHandleArgb = 0xFF34D399.toInt(),
            botHandleArgb = 0xFFFBBF24.toInt(),
            systemTagArgb = 0xFFC8A253.toInt(),
            systemEventBodyArgb = 0xFFFDE68A.toInt(),
            messageBodyArgb = 0xFFF1F5F9.toInt(),
            dangerOutlineArgb = 0xFFDC2626.toInt(),
            dangerTextArgb = 0xFFF87171.toInt()
        )
    }
}

fun resolveWebChatStylePaletteForWindowBg(windowBgArgb: Int): WebChatStylePalette =
    resolveWebChatStylePalette(isLightChatSurfaceArgb(windowBgArgb))

fun resolveWebChatStylePaletteForColor(windowBg: Color): WebChatStylePalette =
    resolveWebChatStylePalette(isLightChatSurface(windowBg))

private val CHAT_TIMESTAMP_REGEX = Regex("^\\[(\\d{1,2}:\\d{2}(?::\\d{2})?)\\]\\s*")
private val CHAT_TEAM_SCOPE_REGEX = Regex("^\\[TEAM\\]\\s*", RegexOption.IGNORE_CASE)
private val CHAT_SYSTEM_PREFIX_REGEX = Regex("^(\\[System\\]:?|System:)\\s*", RegexOption.IGNORE_CASE)
private val CHAT_TRAILING_BOT_TAG_REGEX = Regex("\\s*\\[BOT\\]$", RegexOption.IGNORE_CASE)

private fun isBotSenderToken(rawSender: String, cleanHandle: String): Boolean {
    if (rawSender.contains("[BOT]", ignoreCase = true)) {
        return true
    }
    val lower = cleanHandle.lowercase()
    return lower == "bot" ||
        lower == "stock" ||
        lower.startsWith("octibot") ||
        lower.startsWith("bot_") ||
        lower.startsWith("bot-")
}

private fun colorToArgbInt(c: Color): Int {
    val a = (c.alpha * 255.0f).roundToInt().coerceIn(0, 255)
    val r = (c.red * 255.0f).roundToInt().coerceIn(0, 255)
    val g = (c.green * 255.0f).roundToInt().coerceIn(0, 255)
    val b = (c.blue * 255.0f).roundToInt().coerceIn(0, 255)
    return (a shl 24) or (r shl 16) or (g shl 8) or b
}

fun extractResolvedChatSpans(annotated: AnnotatedString): List<ResolvedChatSpan> {
    return annotated.spanStyles.map { range ->
        val item = range.item
        ResolvedChatSpan(
            text = annotated.text.substring(range.start, range.end),
            start = range.start,
            end = range.end,
            colorArgb = colorToArgbInt(item.color),
            isBold = item.fontWeight == FontWeight.Bold,
            isItalic = item.fontStyle == FontStyle.Italic,
            isMonospace = item.fontFamily == FontFamily.Monospace
        )
    }
}

@JvmOverloads
fun formatWebChatLineAnnotatedString(
    rawLine: String,
    viewerHandle: String = "",
    lightSurface: Boolean = false
): AnnotatedString {
    val pal = resolveWebChatStylePalette(lightSurface)
    var rest = rawLine
    val cleanViewer = viewerHandle.trim().removePrefix("@")

    return buildAnnotatedString {
        val tsMatch = CHAT_TIMESTAMP_REGEX.find(rest)
        if (tsMatch != null) {
            val tsToken = "[${tsMatch.groupValues[1]}] "
            withStyle(SpanStyle(color = pal.timestamp, fontFamily = FontFamily.Monospace)) {
                append(tsToken)
            }
            rest = rest.substring(tsMatch.value.length)
        }

        val teamMatch = CHAT_TEAM_SCOPE_REGEX.find(rest)
        if (teamMatch != null) {
            withStyle(SpanStyle(color = pal.teamScope, fontWeight = FontWeight.Bold)) {
                append("[TEAM] ")
            }
            rest = rest.substring(teamMatch.value.length)
        }

        val sysMatch = CHAT_SYSTEM_PREFIX_REGEX.find(rest)
        if (sysMatch != null) {
            withStyle(SpanStyle(color = pal.systemTag, fontWeight = FontWeight.Bold)) {
                append("[System] ")
            }
            val eventBody = rest.substring(sysMatch.value.length)
            if (eventBody.isNotEmpty()) {
                withStyle(SpanStyle(color = pal.systemEventBody, fontStyle = FontStyle.Italic)) {
                    append(eventBody)
                }
            }
            return@buildAnnotatedString
        }

        val colonIdx = rest.indexOf(": ")
        if (colonIdx in 1..48) {
            val rawSender = rest.substring(0, colonIdx).trim()
            val body = rest.substring(colonIdx + 2)
            val cleanSender = rawSender
                .removePrefix("@")
                .replace(CHAT_TRAILING_BOT_TAG_REGEX, "")
                .trim()

            if (cleanSender.equals("System", ignoreCase = true) ||
                rawSender.equals("[System]", ignoreCase = true)
            ) {
                withStyle(SpanStyle(color = pal.systemTag, fontWeight = FontWeight.Bold)) {
                    append("[System] ")
                }
                withStyle(SpanStyle(color = pal.systemEventBody, fontStyle = FontStyle.Italic)) {
                    append(body)
                }
                return@buildAnnotatedString
            }

            val isBot = isBotSenderToken(rawSender, cleanSender)
            val isSelf = !isBot && (
                cleanSender.equals("You", ignoreCase = true) ||
                    (cleanViewer.isNotEmpty() && cleanSender.equals(cleanViewer, ignoreCase = true))
                )

            val senderLabel: String
            val senderColor: Color
            when {
                isBot -> {
                    senderLabel = "@${cleanSender.ifEmpty { "bot" }} [BOT]:"
                    senderColor = pal.botHandle
                }
                isSelf -> {
                    senderLabel = if (cleanSender.equals("You", ignoreCase = true)) "You:" else "@$cleanSender:"
                    senderColor = pal.selfHandle
                }
                else -> {
                    senderLabel = "@${cleanSender.ifEmpty { "anon" }}:"
                    senderColor = pal.peerHandle
                }
            }

            withStyle(SpanStyle(color = senderColor, fontWeight = FontWeight.Bold)) {
                append(senderLabel)
            }
            withStyle(SpanStyle(color = pal.messageBody)) {
                append(" ")
                append(body)
            }
        } else {
            withStyle(SpanStyle(color = pal.messageBody)) {
                append(rest)
            }
        }
    }
}

@JvmOverloads
fun formatGameChatEntryAnnotatedString(
    msg: OctiAppViewModel.GameChatMessage,
    viewerUuid: String = "",
    viewerHandle: String = "",
    lightSurface: Boolean = false,
    includeTimestamp: Boolean = true,
    isRobotSeat: Boolean = false
): AnnotatedString {
    val pal = resolveWebChatStylePalette(lightSurface)
    val cleanViewer = viewerHandle.trim().removePrefix("@")
    val rawHandle = msg.senderHandle.trim()
    val cleanHandle = rawHandle
        .removePrefix("@")
        .replace(CHAT_TRAILING_BOT_TAG_REGEX, "")
        .trim()

    val isSystem = msg.scope.equals("SYSTEM", ignoreCase = true) ||
        msg.scope.equals("EVENT", ignoreCase = true) ||
        cleanHandle.equals("System", ignoreCase = true) ||
        rawHandle.equals("[System]", ignoreCase = true) ||
        (rawHandle.isEmpty() && msg.senderUuid.isEmpty() && msg.seat == 0 &&
            CHAT_SYSTEM_PREFIX_REGEX.containsMatchIn(msg.text))

    return buildAnnotatedString {
        if (includeTimestamp) {
            val tsStr = OctiAppViewModel.formatLobbyChatTimestamp(msg.atMs)
            withStyle(SpanStyle(color = pal.timestamp, fontFamily = FontFamily.Monospace)) {
                append("[$tsStr] ")
            }
        }

        if (msg.scope.equals("TEAM", ignoreCase = true)) {
            withStyle(SpanStyle(color = pal.teamScope, fontWeight = FontWeight.Bold)) {
                append("[TEAM] ")
            }
        }

        if (isSystem) {
            withStyle(SpanStyle(color = pal.systemTag, fontWeight = FontWeight.Bold)) {
                append("[System] ")
            }
            val cleanedEventText = msg.text.replaceFirst(CHAT_SYSTEM_PREFIX_REGEX, "")
            withStyle(SpanStyle(color = pal.systemEventBody, fontStyle = FontStyle.Italic)) {
                append(cleanedEventText)
            }
            return@buildAnnotatedString
        }

        val isBot = isRobotSeat ||
            msg.senderUuid.startsWith("bot", ignoreCase = true) ||
            isBotSenderToken(rawHandle, cleanHandle)

        val isSelf = !isBot && (
            (rawHandle.isEmpty() && msg.senderUuid.isEmpty()) ||
                cleanHandle.equals("You", ignoreCase = true) ||
                (viewerUuid.isNotEmpty() && msg.senderUuid == viewerUuid) ||
                (cleanViewer.isNotEmpty() && cleanHandle.isNotEmpty() &&
                    cleanHandle.equals(cleanViewer, ignoreCase = true))
            )

        val senderLabel: String
        val senderColor: Color
        when {
            isBot -> {
                senderLabel = "@${cleanHandle.ifEmpty { "bot" }} [BOT]:"
                senderColor = pal.botHandle
            }
            isSelf -> {
                senderLabel = if (cleanHandle.isEmpty() || cleanHandle.equals("You", ignoreCase = true)) {
                    "You:"
                } else {
                    "@$cleanHandle:"
                }
                senderColor = pal.selfHandle
            }
            else -> {
                senderLabel = "@${cleanHandle.ifEmpty { "player" }}:"
                senderColor = pal.peerHandle
            }
        }

        withStyle(SpanStyle(color = senderColor, fontWeight = FontWeight.Bold)) {
            append(senderLabel)
        }
        withStyle(SpanStyle(color = pal.messageBody)) {
            append(" ")
            append(msg.text)
        }
    }
}

/**
 * Tabletop reserve HUD, move confirmation dock, VCR move-history scrubber,
 * and full-screen-width vertically stacked Table Chat & Lobby Chat surfaces.
 */
@OptIn(ExperimentalLayoutApi::class)
@Composable
fun OctiReserveHud(
    viewModel: OctiAppViewModel,
    modifier: Modifier = Modifier,
    onRequestSitSeat: ((Int) -> Unit)? = null
) {
    val boardRev by viewModel.boardRevision.collectAsState()
    val clockTickRevision by viewModel.clockTickRevision.collectAsState()
    val currentBoardRev = boardRev
    val awaitingConfirmation by viewModel.awaitingConfirmation.collectAsState()
    val stagedMoveNotation by viewModel.stagedMoveNotation.collectAsState()
    val canAddMoreMoves by viewModel.canAddMoreMoves.collectAsState()
    val viewingPly by viewModel.viewingPly.collectAsState()
    val confirmAll by viewModel.confirmAllMoves.collectAsState()
    val moveStack by viewModel.moveEntireMatchingStack.collectAsState()
    val matchType by viewModel.matchType.collectAsState()
    val activeRoomId by viewModel.activeRoomId.collectAsState()
    val activeGameView by viewModel.activeGameView.collectAsState()
    val gameChatLog by viewModel.gameChatLog.collectAsState()
    val lobbyChatMessages by viewModel.lobbyChatMessages.collectAsState()
    val lobbyState by viewModel.lobbyState.collectAsState()
    val currentProfile by viewModel.currentProfile.collectAsState()
    val themeId by viewModel.theme.collectAsState()
    val pal = OctiPalette.forTheme(themeId)
    val webPal = resolveWebChatStylePaletteForColor(OctiTheme.colors.windowBg)
    val myUuid = currentProfile?.uuid ?: ""
    val myHandle = currentProfile?.handle ?: ""
    val isInteractiveTurn = viewModel.isInteractiveOnlineTurn()

    val activeBoard = if (viewModel.isViewingHistory()) {
        viewModel.displayBoard
    } else {
        viewModel.board
    }
    val game = activeBoard.game
    val turn = game?.turn ?: OctiPlayer.PLAYER_A
    val useSuperProngs = game?.useSuperProngs() == true
    val isGameOver = (game?.isGameOver == true) || viewModel.isOnlineGameOver()
    val awaitingConfirm = (awaitingConfirmation || viewModel.composer.isAwaitingConfirmation) && currentBoardRev >= 0L
    val stagedNotation = if (awaitingConfirm) {
        stagedMoveNotation.ifEmpty { viewModel.composer.stagedMoveNotation() }
    } else {
        ""
    }
    val dragKind = viewModel.composer.activeDragKind()
    val totalPlies = viewModel.livePlyCount()
    val compactBtnPad = PaddingValues(horizontal = 10.dp, vertical = 0.dp)

    Surface(
        modifier = modifier,
        color = webPal.cardBg,
        shape = RoundedCornerShape(0.dp),
        border = BorderStroke(1.dp, webPal.cardBorder)
    ) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(top = 4.dp),
            verticalArrangement = Arrangement.spacedBy(4.dp)
        ) {
            val gameMode = game?.mode ?: OctiGameMode.GAME_MODE_2
            val players = if (gameMode == OctiGameMode.GAME_MODE_4) {
                listOf(OctiPlayer.PLAYER_A, OctiPlayer.PLAYER_B, OctiPlayer.PLAYER_C, OctiPlayer.PLAYER_D)
            } else {
                listOf(OctiPlayer.PLAYER_A, OctiPlayer.PLAYER_B)
            }

            val chunked = players.chunked(2)
            for (rowPlayers in chunked) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(horizontal = RESERVE_HUD_CONTROLS_HORIZONTAL_PADDING_DP.dp),
                    horizontalArrangement = Arrangement.spacedBy(6.dp)
                ) {
                    for (p in rowPlayers) {
                        val oppForCap = when (p) {
                            OctiPlayer.PLAYER_A -> OctiPlayer.PLAYER_B
                            OctiPlayer.PLAYER_B -> OctiPlayer.PLAYER_A
                            OctiPlayer.PLAYER_C -> OctiPlayer.PLAYER_D
                            OctiPlayer.PLAYER_D -> OctiPlayer.PLAYER_C
                            else -> OctiPlayer.PLAYER_B
                        }
                        val capPodsHeld = game?.getCapturedPods(oppForCap) ?: 0
                        val ownCapPods = game?.getCapturedPods(p) ?: 0
                        val is4P = game?.mode == OctiGameMode.GAME_MODE_4
                        val partner = if (is4P) {
                            when (turn) {
                                OctiPlayer.PLAYER_A -> OctiPlayer.PLAYER_C
                                OctiPlayer.PLAYER_C -> OctiPlayer.PLAYER_A
                                OctiPlayer.PLAYER_B -> OctiPlayer.PLAYER_D
                                OctiPlayer.PLAYER_D -> OctiPlayer.PLAYER_B
                                else -> null
                            }
                        } else null
                        val partnerRevivable = is4P && partner != null && (p == partner) &&
                            (game?.isPlayerEliminated(partner) == true) &&
                            ((game?.getCapturedPods(partner) ?: 0) > 0)
                        val activePlayerTurn = !isGameOver && (p == turn) && !viewModel.isViewingHistory() && isInteractiveTurn
                        val canActiveLiberateFromThisCard = !isGameOver && !viewModel.isViewingHistory() && isInteractiveTurn &&
                            (((oppForCap == turn) && (capPodsHeld > 0)) ||
                             ((p == turn) && ((game?.getCapturedPods(turn) ?: 0) > 0)) ||
                             partnerRevivable)
                        val resolvedSeat = activeGameView?.seats?.firstOrNull { it.seat == p.value }
                        val seatLetter = p.name.removePrefix("PLAYER_")
                        val seatLabel = if (matchType == OctiAppViewModel.MatchType.ONLINE_ROOM && resolvedSeat != null && resolvedSeat.handle.isNotEmpty()) {
                            val cleanHandle = resolvedSeat.handle.removePrefix("@")
                            val isBot = resolvedSeat.isRobot || cleanHandle.equals("stock", ignoreCase = true) || resolvedSeat.playerUuid.startsWith("bot")
                            val tag = if (isBot) "@$cleanHandle [BOT]" else "@$cleanHandle"
                            "$tag ($seatLetter)"
                        } else {
                            "PLAYER $seatLetter"
                        }
                        val clockText = if (matchType == OctiAppViewModel.MatchType.ONLINE_ROOM && clockTickRevision >= 0L) {
                            if (activeGameView?.timeControlEnabled == true) {
                                val clockMs = resolvedSeat?.clockMs ?: 0L
                                val totalSec = maxOf(0L, clockMs / 1000L)
                                val mm = totalSec / 60L
                                val ss = totalSec % 60L
                                String.format(java.util.Locale.US, "%02d:%02d", mm, ss)
                            } else {
                                "Untimed"
                            }
                        } else null
                        val isWaiting = activeGameView?.phase == "WAITING_FOR_PLAYERS"
                        val viewerSeat = activeGameView?.viewerSeat ?: 0
                        val isSeatOccupied = resolvedSeat != null && (resolvedSeat.playerUuid.isNotEmpty() || resolvedSeat.handle.isNotEmpty())
                        val onSit = if (matchType == OctiAppViewModel.MatchType.ONLINE_ROOM &&
                            isWaiting && viewerSeat == 0 && !isSeatOccupied) {
                            {
                                if (onRequestSitSeat != null) {
                                    onRequestSitSeat(p.value)
                                } else {
                                    viewModel.joinOnlineTableAsync(activeRoomId, p.value, "")
                                }
                            }
                        } else null
                        PlayerReserveCard(
                            player = p,
                            displayName = seatLabel,
                            clockText = clockText,
                            onSit = onSit,
                            isTurn = !isGameOver && (p == turn),
                            freePods = game?.getFreePods(p) ?: 0,
                            freeProngs = game?.getFreeProngs(p) ?: 0,
                            freeSuper = game?.getFreeSuperProngs(p) ?: 0,
                            capPods = capPodsHeld,
                            useSuperProngs = useSuperProngs,
                            pal = pal,
                            modifier = Modifier.weight(1f),
                            ownCapPods = ownCapPods,
                            gameMode = game?.mode ?: OctiGameMode.GAME_MODE_2,
                            onStartPodPlacement = if (activePlayerTurn) {
                                {
                                    viewModel.clearLastMoveIndicators()
                                    if (viewModel.composer.beginAddPodFromReserve()) {
                                        viewModel.bumpBoardRevision()
                                    }
                                }
                            } else null,
                            onStartLiberatePlacement = if (canActiveLiberateFromThisCard) {
                                {
                                    viewModel.clearLastMoveIndicators()
                                    if (viewModel.composer.beginLiberatePodFromReserve()) {
                                        viewModel.bumpBoardRevision()
                                    }
                                }
                            } else null,
                            onStartProngPlacement = if (activePlayerTurn && (game?.getFreeProngs(p) ?: 0) > 0) {
                                {
                                    viewModel.clearLastMoveIndicators()
                                    viewModel.composer.beginAddProngFromReserve(false)
                                    val orig = viewModel.composer.guidedOriginSquare()
                                    if (orig != null && orig.size >= 2 && orig[0] >= 1 && orig[1] >= 1) {
                                        viewModel.openZoomedSquare(orig[0], orig[1])
                                    }
                                    viewModel.bumpBoardRevision()
                                }
                            } else null,
                            onStartSuperProngPlacement = if (activePlayerTurn && useSuperProngs && (game?.getFreeSuperProngs(p) ?: 0) > 0) {
                                {
                                    viewModel.clearLastMoveIndicators()
                                    viewModel.composer.beginAddProngFromReserve(true)
                                    val orig = viewModel.composer.guidedOriginSquare()
                                    if (orig != null && orig.size >= 2 && orig[0] >= 1 && orig[1] >= 1) {
                                        viewModel.openZoomedSquare(orig[0], orig[1])
                                    }
                                    viewModel.bumpBoardRevision()
                                }
                            } else null
                        )
                    }
                }
            }

            val isLocalMatch = matchType == OctiAppViewModel.MatchType.LOCAL_PASS_AND_PLAY ||
                matchType == OctiAppViewModel.MatchType.LOCAL_VS_BOT
            val canUndo = isLocalMatch && (viewModel.board.game?.canUndo() == true)
            val isGuidedCancelVisible = viewModel.composer.isGuidedMoveActive && boardRev >= 0L

            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = RESERVE_HUD_CONTROLS_HORIZONTAL_PADDING_DP.dp),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Row(
                    horizontalArrangement = Arrangement.spacedBy(3.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    OutlinedButton(
                        onClick = { viewModel.vcrFirst() },
                        enabled = viewingPly > 0,
                        modifier = Modifier.height(28.dp),
                        contentPadding = PaddingValues(horizontal = 8.dp, vertical = 0.dp)
                    ) {
                        Text("|<<", style = MaterialTheme.typography.labelSmall)
                    }
                    OutlinedButton(
                        onClick = { viewModel.vcrPrev() },
                        enabled = viewingPly > 0,
                        modifier = Modifier.height(28.dp),
                        contentPadding = PaddingValues(horizontal = 8.dp, vertical = 0.dp)
                    ) {
                        Text("<", style = MaterialTheme.typography.labelSmall)
                    }
                    Text(
                        text = "Ply $viewingPly/$totalPlies",
                        style = MaterialTheme.typography.labelSmall,
                        color = OctiTheme.colors.textSecondary,
                        modifier = Modifier.padding(horizontal = 2.dp)
                    )
                    OutlinedButton(
                        onClick = { viewModel.vcrNext() },
                        enabled = viewingPly < totalPlies,
                        modifier = Modifier.height(28.dp),
                        contentPadding = PaddingValues(horizontal = 8.dp, vertical = 0.dp)
                    ) {
                        Text(">", style = MaterialTheme.typography.labelSmall)
                    }
                    OutlinedButton(
                        onClick = { viewModel.vcrLast() },
                        enabled = viewingPly < totalPlies,
                        modifier = Modifier.height(28.dp),
                        contentPadding = PaddingValues(horizontal = 8.dp, vertical = 0.dp)
                    ) {
                        Text(">>|", style = MaterialTheme.typography.labelSmall)
                    }
                }
                Row(
                    horizontalArrangement = Arrangement.spacedBy(4.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    if (isGuidedCancelVisible) {
                        OutlinedButton(
                            onClick = { viewModel.rollbackStagedMove() },
                            modifier = Modifier.height(28.dp),
                            contentPadding = compactBtnPad
                        ) {
                            Text("Cancel", style = MaterialTheme.typography.labelSmall)
                        }
                    }
                    if (isLocalMatch) {
                        OutlinedButton(
                            onClick = { viewModel.undoLocalMove() },
                            enabled = canUndo && !viewModel.isViewingHistory(),
                            modifier = Modifier.height(28.dp),
                            contentPadding = compactBtnPad
                        ) {
                            Text("Undo", style = MaterialTheme.typography.labelSmall)
                        }
                        OutlinedButton(
                            onClick = { viewModel.resignLocalGame() },
                            enabled = !isGameOver && !viewModel.isViewingHistory(),
                            modifier = Modifier.height(28.dp),
                            contentPadding = compactBtnPad
                        ) {
                            Text("Resign", style = MaterialTheme.typography.labelSmall)
                        }
                    }
                    val context = LocalContext.current
                    OutlinedButton(
                        onClick = {
                            viewModel.clearLastMoveIndicators()
                            val text = viewModel.exportMoveHistoryText()
                            val clipboard = context.getSystemService(Context.CLIPBOARD_SERVICE) as? ClipboardManager
                            val clip = ClipData.newPlainText("OCTI Transcript", text)
                            clipboard?.setPrimaryClip(clip)
                            viewModel.setShowExportMovesDialog(true)
                        },
                        modifier = Modifier.size(32.dp, 28.dp),
                        contentPadding = PaddingValues(0.dp)
                    ) {
                        ExportMovesIcon(
                            color = OctiTheme.colors.accentBrass,
                            modifier = Modifier.size(14.dp)
                        )
                    }
                }
            }

            val viewerSeat = activeGameView?.viewerSeat ?: 0
            val isOnlineSeatedPlayer = matchType == OctiAppViewModel.MatchType.ONLINE_ROOM && activeRoomId.isNotEmpty() && viewerSeat > 0
            val isOnlineActivePhase = activeGameView?.phase == "IN_PROGRESS" || activeGameView?.phase == "PAUSED"
            if (isOnlineSeatedPlayer && isOnlineActivePhase) {
                FlowRow(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(horizontal = RESERVE_HUD_CONTROLS_HORIZONTAL_PADDING_DP.dp),
                    horizontalArrangement = Arrangement.spacedBy(6.dp),
                    verticalArrangement = Arrangement.spacedBy(4.dp)
                ) {
                    val anyOpponentForfeitClaimable = activeGameView?.seats?.any { it.seat != viewerSeat && it.forfeitClaimable } == true

                    if (viewModel.isOnlineUndoVisible) {
                        OutlinedButton(
                            onClick = { viewModel.requestOrAcceptOnlineUndo() },
                            modifier = Modifier.height(28.dp),
                            contentPadding = compactBtnPad
                        ) {
                            Text("Undo", style = MaterialTheme.typography.labelSmall)
                        }
                    }
                    OutlinedButton(
                        onClick = { viewModel.proposeDraw() },
                        modifier = Modifier.height(28.dp),
                        contentPadding = compactBtnPad
                    ) {
                        Text("Draw", style = MaterialTheme.typography.labelSmall)
                    }
                    OutlinedButton(
                        onClick = { viewModel.proposePauseOrResume() },
                        modifier = Modifier.height(28.dp),
                        contentPadding = compactBtnPad
                    ) {
                        Text(
                            text = if (activeGameView?.paused == true) "Resume" else "Pause",
                            style = MaterialTheme.typography.labelSmall
                        )
                    }
                    OutlinedButton(
                        onClick = { viewModel.resignOnlineGameAsync() },
                        modifier = Modifier.height(28.dp),
                        contentPadding = compactBtnPad
                    ) {
                        Text("Resign", style = MaterialTheme.typography.labelSmall)
                    }
                    if (anyOpponentForfeitClaimable) {
                        Button(
                            onClick = { viewModel.claimOpponentForfeit() },
                            modifier = Modifier.height(28.dp),
                            contentPadding = compactBtnPad,
                            colors = ButtonDefaults.buttonColors(
                                containerColor = MaterialTheme.colorScheme.error,
                                contentColor = Color.White
                            )
                        ) {
                            Text("Claim Forfeit", style = MaterialTheme.typography.labelSmall, fontWeight = FontWeight.Bold)
                        }
                    }
                }
            }

            val isOnlineRoom = matchType == OctiAppViewModel.MatchType.ONLINE_ROOM
            val effectiveLobbyMessages = if (lobbyChatMessages.isNotEmpty()) {
                lobbyChatMessages
            } else {
                lobbyState.recentChat.map { OctiAppViewModel.formatLobbyChatEntry(it) }
            }
            val tableChatListState = rememberLazyListState()
            val lobbyChatListState = rememberLazyListState()

            LaunchedEffect(gameChatLog.size) {
                if (gameChatLog.isNotEmpty()) {
                    tableChatListState.scrollToItem(gameChatLog.size - 1)
                }
            }
            LaunchedEffect(effectiveLobbyMessages.size) {
                if (effectiveLobbyMessages.isNotEmpty()) {
                    lobbyChatListState.scrollToItem(effectiveLobbyMessages.size - 1)
                }
            }

            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f),
                verticalArrangement = Arrangement.spacedBy(RESERVE_HUD_CHAT_STACK_SPACING_DP.dp)
            ) {
                if (isOnlineRoom) {
                    // 1. Table Chat (Top, 100% full screen width)
                    Surface(
                        modifier = Modifier
                            .fillMaxWidth()
                            .weight(1f)
                            .heightIn(min = RESERVE_HUD_CHAT_MIN_HEIGHT_DP.dp)
                            .clickable { viewModel.openBoardChatZoomDialog() },
                        color = webPal.wellBg,
                        shape = RoundedCornerShape(0.dp),
                        border = BorderStroke(1.dp, webPal.cardBorder)
                    ) {
                        Column(
                            modifier = Modifier
                                .fillMaxSize()
                                .padding(horizontal = 8.dp, vertical = 3.dp),
                            verticalArrangement = Arrangement.spacedBy(2.dp)
                        ) {
                            Row(
                                modifier = Modifier.fillMaxWidth(),
                                horizontalArrangement = Arrangement.SpaceBetween,
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                Text(
                                    text = "Table Chat",
                                    style = MaterialTheme.typography.labelSmall,
                                    fontWeight = FontWeight.Bold,
                                    color = webPal.headerTitle
                                )
                                Row(
                                    horizontalArrangement = Arrangement.spacedBy(4.dp),
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    if (activeRoomId.isNotEmpty()) {
                                        OutlinedButton(
                                            onClick = { viewModel.leaveOnlineTableAsync(activeRoomId) },
                                            modifier = Modifier.height(20.dp),
                                            border = BorderStroke(1.dp, webPal.dangerOutline),
                                            contentPadding = PaddingValues(horizontal = 6.dp, vertical = 0.dp)
                                        ) {
                                            Text(
                                                text = "Leave",
                                                style = MaterialTheme.typography.labelSmall,
                                                fontSize = 9.sp,
                                                color = webPal.dangerText
                                            )
                                        }
                                    }
                                    Surface(
                                        color = webPal.cardBg,
                                        shape = RoundedCornerShape(4.dp),
                                        border = BorderStroke(1.dp, webPal.headerTitle)
                                    ) {
                                        Text(
                                            text = "⤢",
                                            style = MaterialTheme.typography.labelSmall,
                                            color = webPal.headerTitle,
                                            fontWeight = FontWeight.Bold,
                                            modifier = Modifier.padding(horizontal = 5.dp, vertical = 1.dp)
                                        )
                                    }
                                }
                            }
                            if (gameChatLog.isEmpty()) {
                                Box(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .weight(1f),
                                    contentAlignment = Alignment.CenterStart
                                ) {
                                    Text(
                                        text = "Table Chat • Tap to open",
                                        style = MaterialTheme.typography.labelSmall,
                                        color = webPal.timestamp,
                                        maxLines = 2,
                                        overflow = TextOverflow.Ellipsis
                                    )
                                }
                            } else {
                                LazyColumn(
                                    state = tableChatListState,
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .weight(1f),
                                    verticalArrangement = Arrangement.spacedBy(1.dp)
                                ) {
                                    items(gameChatLog) { msg ->
                                        val isSeatBot = activeGameView?.seats
                                            ?.firstOrNull { it.seat == msg.seat }
                                            ?.isRobot == true
                                        val styled = formatGameChatEntryAnnotatedString(
                                            msg = msg,
                                            viewerUuid = myUuid,
                                            viewerHandle = myHandle,
                                            lightSurface = webPal.lightSurface,
                                            includeTimestamp = true,
                                            isRobotSeat = isSeatBot
                                        )
                                        Text(
                                            text = styled,
                                            style = MaterialTheme.typography.labelSmall,
                                            maxLines = 2,
                                            overflow = TextOverflow.Ellipsis,
                                            modifier = Modifier
                                                .fillMaxWidth()
                                                .clickable { viewModel.openBoardChatZoomDialog() }
                                        )
                                    }
                                }
                            }
                        }
                    }
                }

                // 2. Lobby Chat (Bottom when online, full chat stack when local; 100% full screen width)
                Surface(
                    modifier = Modifier
                        .fillMaxWidth()
                        .weight(1f)
                        .heightIn(min = RESERVE_HUD_CHAT_MIN_HEIGHT_DP.dp)
                        .clickable { viewModel.openLobbyChatZoomDialog() },
                    color = webPal.wellBg,
                    shape = RoundedCornerShape(0.dp),
                    border = BorderStroke(1.dp, webPal.cardBorder)
                ) {
                    Column(
                        modifier = Modifier
                            .fillMaxSize()
                            .padding(horizontal = 8.dp, vertical = 3.dp),
                        verticalArrangement = Arrangement.spacedBy(2.dp)
                    ) {
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.SpaceBetween,
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Row(
                                horizontalArrangement = Arrangement.spacedBy(6.dp),
                                verticalAlignment = Alignment.CenterVertically
                            ) {
                                Text(
                                    text = "Lobby Chat",
                                    style = MaterialTheme.typography.labelSmall,
                                    fontWeight = FontWeight.Bold,
                                    color = webPal.headerTitle
                                )
                                Text(
                                    text = "Global",
                                    style = MaterialTheme.typography.labelSmall,
                                    color = webPal.timestamp
                                )
                            }
                            Surface(
                                color = webPal.cardBg,
                                shape = RoundedCornerShape(4.dp),
                                border = BorderStroke(1.dp, webPal.headerTitle)
                            ) {
                                Text(
                                    text = "⤢",
                                    style = MaterialTheme.typography.labelSmall,
                                    color = webPal.headerTitle,
                                    fontWeight = FontWeight.Bold,
                                    modifier = Modifier.padding(horizontal = 5.dp, vertical = 1.dp)
                                )
                            }
                        }
                        if (effectiveLobbyMessages.isEmpty()) {
                            Box(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .weight(1f),
                                contentAlignment = Alignment.CenterStart
                            ) {
                                Text(
                                    text = "Global Lobby Chat • Tap to open & chat",
                                    style = MaterialTheme.typography.labelSmall,
                                    color = webPal.timestamp,
                                    maxLines = 2,
                                    overflow = TextOverflow.Ellipsis
                                )
                            }
                        } else {
                            LazyColumn(
                                state = lobbyChatListState,
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .weight(1f),
                                verticalArrangement = Arrangement.spacedBy(1.dp)
                            ) {
                                items(effectiveLobbyMessages) { msgLine ->
                                    val styled = formatWebChatLineAnnotatedString(
                                        rawLine = msgLine,
                                        viewerHandle = myHandle,
                                        lightSurface = webPal.lightSurface
                                    )
                                    Text(
                                        text = styled,
                                        style = MaterialTheme.typography.labelSmall,
                                        maxLines = 2,
                                        overflow = TextOverflow.Ellipsis,
                                        modifier = Modifier
                                            .fillMaxWidth()
                                            .clickable { viewModel.openLobbyChatZoomDialog() }
                                    )
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
fun PlayerReserveCard(
    player: OctiPlayer,
    isTurn: Boolean,
    freePods: Int,
    freeProngs: Int,
    freeSuper: Int,
    capPods: Int,
    useSuperProngs: Boolean,
    pal: OctiPalette,
    modifier: Modifier = Modifier,
    ownCapPods: Int = 0,
    gameMode: OctiGameMode = OctiGameMode.GAME_MODE_2,
    displayName: String? = null,
    clockText: String? = null,
    onSit: (() -> Unit)? = null,
    onStartPodPlacement: (() -> Unit)? = null,
    onStartLiberatePlacement: (() -> Unit)? = null,
    onStartProngPlacement: (() -> Unit)? = null,
    onStartSuperProngPlacement: (() -> Unit)? = null
) {
    val faction = pal.factionForPlayer(player)
    val oppPlayer = when (player) {
        OctiPlayer.PLAYER_A -> OctiPlayer.PLAYER_B
        OctiPlayer.PLAYER_B -> OctiPlayer.PLAYER_A
        OctiPlayer.PLAYER_C -> OctiPlayer.PLAYER_D
        OctiPlayer.PLAYER_D -> OctiPlayer.PLAYER_C
        else -> OctiPlayer.PLAYER_B
    }
    val oppFaction = pal.factionForPlayer(oppPlayer)
    val letter = player.name.removePrefix("PLAYER_")

    Card(
        modifier = modifier,
        colors = CardDefaults.cardColors(containerColor = OctiTheme.colors.windowBg),
        border = if (isTurn) {
            BorderStroke(1.5.dp, OctiTheme.colors.accentBrass)
        } else {
            BorderStroke(1.dp, OctiTheme.colors.panelCardBorder)
        }
    ) {
        Column(
            modifier = Modifier.padding(horizontal = 8.dp, vertical = 4.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = displayName ?: "PLAYER $letter",
                    style = MaterialTheme.typography.labelSmall,
                    fontWeight = FontWeight.Bold,
                    color = if (isTurn) OctiTheme.colors.accentBrass else OctiTheme.colors.textSecondary,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                    modifier = Modifier
                        .weight(1f)
                        .padding(end = 4.dp)
                )
                Row(
                    horizontalArrangement = Arrangement.spacedBy(4.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    if (clockText != null) {
                        Text(
                            text = clockText,
                            style = MaterialTheme.typography.labelSmall,
                            fontWeight = FontWeight.Bold,
                            maxLines = 1,
                            softWrap = false,
                            color = if (isTurn) OctiTheme.colors.legalStep else OctiTheme.colors.textSecondary
                        )
                    }
                    if (onSit != null) {
                        Button(
                            onClick = onSit,
                            contentPadding = PaddingValues(horizontal = 8.dp, vertical = 0.dp),
                            modifier = Modifier.height(22.dp),
                            colors = ButtonDefaults.buttonColors(
                                containerColor = OctiTheme.colors.accentBrass,
                                contentColor = OctiTheme.colors.windowBg
                            )
                        ) {
                            Text("Sit", fontSize = 10.sp, fontWeight = FontWeight.Bold, maxLines = 1)
                        }
                    }
                }
            }
            Spacer(modifier = Modifier.height(2.dp))
            FlowRow(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp),
                verticalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                // 1. Pod icon + count (hidden in OFK)
                if (gameMode != OctiGameMode.GAME_MODE_OFK) {
                    val canStartPod = isTurn && (freePods > 0 || ownCapPods > 0) && onStartPodPlacement != null
                    Row(
                        modifier = if (canStartPod) {
                            Modifier.clickable { onStartPodPlacement?.invoke() }
                        } else {
                            Modifier
                        },
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(3.dp)
                    ) {
                        PodIcon(faction = faction, letter = letter, modifier = Modifier.size(20.dp))
                        Text(
                            text = "$freePods",
                            style = MaterialTheme.typography.labelMedium,
                            fontWeight = FontWeight.Bold,
                            color = OctiTheme.colors.textPrimary
                        )
                    }
                }

                // 2. Prong icon + count
                val canStartProng = isTurn && freeProngs > 0 && onStartProngPlacement != null
                Row(
                    modifier = if (canStartProng) {
                        Modifier.clickable { onStartProngPlacement?.invoke() }
                    } else {
                        Modifier
                    },
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(3.dp)
                ) {
                    ProngIcon(pal = pal, modifier = Modifier.size(13.dp, 20.dp))
                    Text(
                        text = "$freeProngs",
                        style = MaterialTheme.typography.labelMedium,
                        fontWeight = FontWeight.Bold,
                        color = OctiTheme.colors.textPrimary
                    )
                }

                // 3. Superprong icon + count (if enabled)
                if (useSuperProngs) {
                    val canStartSuper = isTurn && freeSuper > 0 && onStartSuperProngPlacement != null
                    Row(
                        modifier = if (canStartSuper) {
                            Modifier.clickable { onStartSuperProngPlacement?.invoke() }
                        } else {
                            Modifier
                        },
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(3.dp)
                    ) {
                        SuperProngIcon(pal = pal, modifier = Modifier.size(15.dp, 20.dp))
                        Text(
                            text = "$freeSuper",
                            style = MaterialTheme.typography.labelMedium,
                            fontWeight = FontWeight.Bold,
                            color = OctiTheme.colors.accentBrass
                        )
                    }
                }

                // 4. Captured Pod icon + count (hidden when capPods == 0, disabled in OFK)
                if (capPods > 0) {
                    val canStartLiberate = onStartLiberatePlacement != null && gameMode != OctiGameMode.GAME_MODE_OFK
                    Row(
                        modifier = if (canStartLiberate) {
                            Modifier
                                .border(1.dp, Color(pal.legalCaptureGlow.toLong() and 0xFFFFFFFFL), RoundedCornerShape(4.dp))
                                .padding(horizontal = 3.dp, vertical = 1.dp)
                                .clickable { onStartLiberatePlacement?.invoke() }
                        } else {
                            Modifier
                        },
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(3.dp)
                    ) {
                        CapturedPodIcon(oppFaction = oppFaction, pal = pal, modifier = Modifier.size(20.dp))
                        Text(
                            text = "$capPods",
                            style = MaterialTheme.typography.labelMedium,
                            fontWeight = FontWeight.Bold,
                            color = Color(pal.legalCaptureGlow.toLong() and 0xFFFFFFFFL)
                        )
                    }
                }
            }
        }
    }
}

@Composable
fun ExportMovesIcon(
    color: Color,
    modifier: Modifier = Modifier
) {
    Canvas(modifier = modifier) {
        val w = size.width
        val h = size.height
        val stroke = 1.6.dp.toPx()

        // Open tray at bottom
        val trayPath = Path().apply {
            moveTo(w * 0.14f, h * 0.58f)
            lineTo(w * 0.14f, h * 0.86f)
            lineTo(w * 0.86f, h * 0.86f)
            lineTo(w * 0.86f, h * 0.58f)
        }
        drawPath(
            path = trayPath,
            color = color,
            style = Stroke(width = stroke, cap = StrokeCap.Round, join = StrokeJoin.Round)
        )

        // Upward arrow shaft
        drawLine(
            color = color,
            start = Offset(w * 0.50f, h * 0.66f),
            end = Offset(w * 0.50f, h * 0.14f),
            strokeWidth = stroke,
            cap = StrokeCap.Round
        )

        // Upward arrow head
        val headPath = Path().apply {
            moveTo(w * 0.28f, h * 0.36f)
            lineTo(w * 0.50f, h * 0.14f)
            lineTo(w * 0.72f, h * 0.36f)
        }
        drawPath(
            path = headPath,
            color = color,
            style = Stroke(width = stroke, cap = StrokeCap.Round, join = StrokeJoin.Round)
        )
    }
}

@Composable
fun PodIcon(
    faction: OctiPalette.PodFactionPalette,
    letter: String,
    modifier: Modifier = Modifier
) {
    Box(modifier = modifier, contentAlignment = Alignment.Center) {
        Canvas(modifier = Modifier.fillMaxSize()) {
            val w = size.width
            val h = size.height
            val cx = w * 0.5f
            val cy = h * 0.5f
            val r = min(cx, cy) * 0.92f
            val octoPath = Path()
            for (i in 0 until 8) {
                val angle = (i * 45.0 + 22.5) * Math.PI / 180.0
                val px = cx + cos(angle).toFloat() * r
                val py = cy + sin(angle).toFloat() * r
                if (i == 0) octoPath.moveTo(px, py) else octoPath.lineTo(px, py)
            }
            octoPath.close()

            drawPath(
                path = octoPath,
                brush = Brush.radialGradient(
                    colors = listOf(
                        Color(faction.highlight.toLong() and 0xFFFFFFFFL),
                        Color(faction.core.toLong() and 0xFFFFFFFFL),
                        Color(faction.shadow.toLong() and 0xFFFFFFFFL)
                    ),
                    center = Offset(cx - r * 0.2f, cy - r * 0.2f),
                    radius = r * 1.2f
                )
            )
            drawPath(
                path = octoPath,
                color = Color(faction.rim.toLong() and 0xFFFFFFFFL),
                style = Stroke(width = 1.2.dp.toPx())
            )
        }
        Text(
            text = letter,
            fontSize = 9.sp,
            fontWeight = FontWeight.ExtraBold,
            color = Color(faction.emblem.toLong() and 0xFFFFFFFFL)
        )
    }
}

@Composable
fun ProngIcon(
    pal: OctiPalette,
    modifier: Modifier = Modifier
) {
    Canvas(modifier = modifier) {
        val w = size.width
        val h = size.height
        val cx = w * 0.5f
        val tipY = 1.dp.toPx()
        val baseY = h - 1.dp.toPx()
        val midY = tipY + (baseY - tipY) * 0.55f
        val baseHalfW = w * 0.45f
        val midHalfW = w * 0.28f

        val path = Path().apply {
            moveTo(cx - baseHalfW, baseY)
            lineTo(cx - midHalfW, midY)
            lineTo(cx, tipY)
            lineTo(cx + midHalfW, midY)
            lineTo(cx + baseHalfW, baseY)
            close()
        }
        drawPath(
            path = path,
            brush = Brush.linearGradient(
                colors = listOf(
                    Color(pal.prongNormalTip.toLong() and 0xFFFFFFFFL),
                    Color(pal.prongNormalMetal.toLong() and 0xFFFFFFFFL)
                ),
                start = Offset(cx, tipY),
                end = Offset(cx, baseY)
            )
        )
        drawPath(
            path = path,
            color = Color(0xFF1E293B),
            style = Stroke(width = 1.dp.toPx())
        )
        drawLine(
            color = Color.White.copy(alpha = 0.55f),
            start = Offset(cx, tipY + 1.dp.toPx()),
            end = Offset(cx, baseY - 1.dp.toPx()),
            strokeWidth = 1.dp.toPx()
        )
    }
}

@Composable
fun SuperProngIcon(
    pal: OctiPalette,
    modifier: Modifier = Modifier
) {
    Canvas(modifier = modifier) {
        val w = size.width
        val h = size.height
        val cx = w * 0.5f
        val tipY = 1.dp.toPx()
        val baseY = h - 1.dp.toPx()
        val midY = tipY + (baseY - tipY) * 0.50f
        val baseHalfW = w * 0.48f
        val midHalfW = w * 0.35f

        val path = Path().apply {
            moveTo(cx - baseHalfW, baseY)
            lineTo(cx - midHalfW, midY)
            lineTo(cx, tipY)
            lineTo(cx + midHalfW, midY)
            lineTo(cx + baseHalfW, baseY)
            close()
        }
        drawPath(
            path = path,
            brush = Brush.linearGradient(
                colors = listOf(
                    Color(pal.prongSuperCore.toLong() and 0xFFFFFFFFL),
                    Color(pal.prongSuperGold.toLong() and 0xFFFFFFFFL)
                ),
                start = Offset(cx, tipY),
                end = Offset(cx, baseY)
            )
        )
        drawPath(
            path = path,
            color = Color(0xFF78350F),
            style = Stroke(width = 1.2.dp.toPx())
        )
        drawLine(
            color = Color(0xFFFFFBEB),
            start = Offset(cx, tipY + 1.dp.toPx()),
            end = Offset(cx, baseY - 1.dp.toPx()),
            strokeWidth = 1.2.dp.toPx()
        )
    }
}

@Composable
fun CapturedPodIcon(
    oppFaction: OctiPalette.PodFactionPalette,
    pal: OctiPalette,
    modifier: Modifier = Modifier
) {
    Canvas(modifier = modifier) {
        val w = size.width
        val h = size.height
        val cx = w * 0.5f
        val cy = h * 0.5f
        val r = min(cx, cy) * 0.78f

        val octoPath = Path()
        for (i in 0 until 8) {
            val angle = (i * 45.0 + 22.5) * Math.PI / 180.0
            val px = cx + cos(angle).toFloat() * r
            val py = cy + sin(angle).toFloat() * r
            if (i == 0) octoPath.moveTo(px, py) else octoPath.lineTo(px, py)
        }
        octoPath.close()

        drawPath(
            path = octoPath,
            brush = Brush.radialGradient(
                colors = listOf(
                    Color(oppFaction.highlight.toLong() and 0xFFFFFFFFL),
                    Color(oppFaction.core.toLong() and 0xFFFFFFFFL),
                    Color(oppFaction.shadow.toLong() and 0xFFFFFFFFL)
                ),
                center = Offset(cx - r * 0.2f, cy - r * 0.2f),
                radius = r * 1.2f
            )
        )
        drawPath(
            path = octoPath,
            color = Color(oppFaction.rim.toLong() and 0xFFFFFFFFL),
            style = Stroke(width = 1.dp.toPx())
        )

        val captureColor = Color(pal.legalCaptureGlow.toLong() and 0xFFFFFFFFL)
        drawCircle(
            color = captureColor,
            radius = min(cx, cy) * 0.94f,
            center = Offset(cx, cy),
            style = Stroke(width = 1.4.dp.toPx())
        )
        val strikeR = min(cx, cy) * 0.75f
        drawLine(
            color = captureColor,
            start = Offset(cx - strikeR, cy - strikeR),
            end = Offset(cx + strikeR, cy + strikeR),
            strokeWidth = 1.8.dp.toPx()
        )
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
