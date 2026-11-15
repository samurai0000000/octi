/*
 * RulebookDialog.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui

import android.graphics.Color as AndroidColor
import android.view.ViewGroup
import android.webkit.WebResourceRequest
import android.webkit.WebResourceResponse
import android.webkit.WebView
import android.webkit.WebViewClient
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import com.selfso.octi.ui.theme.OctiTheme
import java.io.ByteArrayInputStream
import java.nio.charset.StandardCharsets

object OctiRulebookAssets {

    const val VIRTUAL_ORIGIN_BASE = "https://octi.local/"
    private const val RULEBOOK_ASSET_PREFIX = "https://octi.local/assets/rulebook/"

    @JvmField
    val IMAGE_FILES: List<String> = listOf(
        "rule_01_2p_3b_setup.png",
        "rule_02_2p_1b_setup.png",
        "rule_03_prongs_8dir.png",
        "rule_04_step_vs_jump.png",
        "rule_05_multihop_chain.png",
        "rule_06_stack_split_loop.png",
        "rule_07_reserve_liberate.png",
        "rule_08_edgeless_wrap.png",
        "rule_09_superprongs.png",
        "rule_10_4p_partnership.png",
        "rule_11_ofk_6x7.png",
        "rule_12_victory_infiltration.png"
    )

    class InterceptDecision(
        val statusCode: Int,
        val reasonPhrase: String,
        val mimeType: String,
        val bytes: ByteArray
    )

    @JvmStatic
    fun loadRulebookHtml(): String? {
        val stream = OctiRulebookAssets::class.java.getResourceAsStream("/rulebook/rulebook_mobile.html")
            ?: OctiRulebookAssets::class.java.classLoader?.getResourceAsStream("rulebook/rulebook_mobile.html")
            ?: return null
        return stream.use { it.readBytes().toString(StandardCharsets.UTF_8) }
    }

    @JvmStatic
    fun loadRulebookImageBytes(fileName: String?): ByteArray? {
        if (fileName.isNullOrEmpty() || !IMAGE_FILES.contains(fileName)) {
            return null
        }
        val stream = OctiRulebookAssets::class.java.getResourceAsStream("/rulebook/$fileName")
            ?: OctiRulebookAssets::class.java.classLoader?.getResourceAsStream("rulebook/$fileName")
            ?: return null
        return stream.use { it.readBytes() }
    }

    @JvmStatic
    fun resolveInterceptedUrl(url: String?): InterceptDecision {
        if (url.isNullOrEmpty() || !url.startsWith(RULEBOOK_ASSET_PREFIX)) {
            return forbiddenDecision()
        }
        val fileName = url.substring(RULEBOOK_ASSET_PREFIX.length)
        if (fileName.isEmpty() ||
            fileName.contains('/') ||
            fileName.contains('\\') ||
            fileName.contains('?') ||
            fileName.contains('#') ||
            fileName.contains('%') ||
            fileName.contains("..") ||
            !IMAGE_FILES.contains(fileName)
        ) {
            return forbiddenDecision()
        }
        val pngBytes = loadRulebookImageBytes(fileName)
        if (pngBytes == null || pngBytes.isEmpty()) {
            return forbiddenDecision()
        }
        return InterceptDecision(200, "OK", "image/png", pngBytes)
    }

    private fun forbiddenDecision(): InterceptDecision {
        return InterceptDecision(403, "Forbidden", "text/plain", ByteArray(0))
    }
}

@Composable
fun RulebookDialog(onDismiss: () -> Unit) {
    val htmlContent = remember {
        OctiRulebookAssets.loadRulebookHtml()
            ?: "<html><body style='background:#0b0f17;color:#f8fafc;padding:16px;'>Rulebook asset unavailable.</body></html>"
    }
    var webViewRef by remember { mutableStateOf<WebView?>(null) }

    DisposableEffect(Unit) {
        onDispose {
            webViewRef?.stopLoading()
            webViewRef?.destroy()
            webViewRef = null
        }
    }

    Dialog(
        onDismissRequest = onDismiss,
        properties = DialogProperties(
            usePlatformDefaultWidth = false,
            dismissOnBackPress = true,
            dismissOnClickOutside = false
        )
    ) {
        Surface(
            modifier = Modifier
                .fillMaxSize()
                .padding(horizontal = 8.dp, vertical = 12.dp),
            shape = RoundedCornerShape(12.dp),
            color = OctiTheme.colors.panelCardBg,
            border = BorderStroke(1.5.dp, OctiTheme.colors.accentBrass)
        ) {
            Column(modifier = Modifier.fillMaxSize()) {
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(horizontal = 12.dp, vertical = 8.dp),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = "OCTI Official Illustrated Rulebook",
                        style = MaterialTheme.typography.titleMedium,
                        fontWeight = FontWeight.Bold,
                        color = OctiTheme.colors.accentBrass,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis,
                        modifier = Modifier
                            .weight(1f)
                            .padding(end = 8.dp)
                    )
                    OutlinedButton(
                        onClick = onDismiss,
                        modifier = Modifier.height(32.dp),
                        contentPadding = PaddingValues(horizontal = 12.dp, vertical = 0.dp),
                        border = BorderStroke(1.dp, OctiTheme.colors.accentBrass)
                    ) {
                        Text(
                            text = "Close",
                            style = MaterialTheme.typography.labelMedium,
                            fontWeight = FontWeight.SemiBold,
                            color = OctiTheme.colors.accentBrass
                        )
                    }
                }
                HorizontalDivider(color = OctiTheme.colors.panelCardBorder)
                AndroidView(
                    modifier = Modifier
                        .fillMaxWidth()
                        .weight(1f),
                    factory = { ctx ->
                        WebView(ctx).apply {
                            layoutParams = ViewGroup.LayoutParams(
                                ViewGroup.LayoutParams.MATCH_PARENT,
                                ViewGroup.LayoutParams.MATCH_PARENT
                            )
                            setBackgroundColor(AndroidColor.parseColor("#0B0F17"))
                            settings.javaScriptEnabled = false
                            settings.allowFileAccess = false
                            settings.allowContentAccess = false
                            settings.blockNetworkLoads = false
                            settings.builtInZoomControls = true
                            settings.displayZoomControls = false
                            webViewClient = object : WebViewClient() {
                                override fun shouldInterceptRequest(
                                    view: WebView?,
                                    request: WebResourceRequest?
                                ): WebResourceResponse {
                                    val decision = OctiRulebookAssets.resolveInterceptedUrl(
                                        request?.url?.toString()
                                    )
                                    return WebResourceResponse(
                                        decision.mimeType,
                                        "UTF-8",
                                        decision.statusCode,
                                        decision.reasonPhrase,
                                        mapOf("Cache-Control" to "no-store"),
                                        ByteArrayInputStream(decision.bytes)
                                    )
                                }

                                override fun shouldOverrideUrlLoading(
                                    view: WebView?,
                                    request: WebResourceRequest?
                                ): Boolean {
                                    val target = request?.url?.toString() ?: return true
                                    return !target.startsWith("https://octi.local/#")
                                }
                            }
                            loadDataWithBaseURL(
                                OctiRulebookAssets.VIRTUAL_ORIGIN_BASE,
                                htmlContent,
                                "text/html",
                                "UTF-8",
                                null
                            )
                            webViewRef = this
                        }
                    }
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
