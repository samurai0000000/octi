/*
 * Color.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui.theme

import androidx.compose.runtime.Immutable
import androidx.compose.ui.graphics.Color
import com.selfso.octi.OctiPalette
import com.selfso.octi.OctiThemeId

// Obsidian & Brass Luxury Tabletop Palette
val ObsidianWindowBg = Color(0xFF0B0F17)
val ObsidianPanelCardBg = Color(0xFF141B26)
val ObsidianPanelCardBorder = Color(0xFF283448)
val ObsidianTextPrimary = Color(0xFFF8FAFC)
val ObsidianTextSecondary = Color(0xFF94A3B8)
val ObsidianAccentBrass = Color(0xFFC8A253)
val ObsidianGridDark = Color(0xFF182232)
val ObsidianGridLight = Color(0xFF1E2B3E)

// Grandmaster Rosewood & Ivory Palette
val RosewoodWindowBg = Color(0xFF1A120E)
val RosewoodPanelCardBg = Color(0xFF2A1D17)
val RosewoodPanelCardBorder = Color(0xFF4A342A)
val RosewoodTextPrimary = Color(0xFFFDFBF7)
val RosewoodTextSecondary = Color(0xFFD2C4B4)
val RosewoodAccentBrass = Color(0xFFD4AF37)
val RosewoodGridDark = Color(0xFF54281C)
val RosewoodGridLight = Color(0xFFEBDDC6)

// Player Pod Identity Accents
val PodAlphaRed = Color(0xFFE13A3E)
val PodAlphaRedLight = Color(0xFFFF6B6E)
val PodBetaGreen = Color(0xFF1E9E6A)
val PodBetaGreenLight = Color(0xFF45D098)
val PodGammaAmber = Color(0xFFE09F24)
val PodGammaAmberLight = Color(0xFFFFC857)
val PodDeltaCobalt = Color(0xFF2E77D0)
val PodDeltaCobaltLight = Color(0xFF64ADFF)

// Status & Action Accents
val LegalStepEmerald = Color(0xFF2ED573)
val LegalJumpGold = Color(0xFFFFC048)
val LegalCaptureCrimson = Color(0xFFFF4757)
val PortalCyanGlow = Color(0xFF38BDF8)

@Immutable
data class OctiThemeColors(
    val themeId: OctiThemeId,
    val windowBg: Color,
    val panelCardBg: Color,
    val panelCardBorder: Color,
    val textPrimary: Color,
    val textSecondary: Color,
    val accentBrass: Color,
    val boardOuterFrame: Color,
    val gridCellDark: Color,
    val gridCellLight: Color,
    val coordLabel: Color,
    val podAlpha: Color,
    val podBeta: Color,
    val podGamma: Color,
    val podDelta: Color,
    val legalStep: Color,
    val legalJump: Color,
    val legalCapture: Color
) {
    companion object {
        fun fromThemeId(themeId: OctiThemeId): OctiThemeColors {
            val p = OctiPalette.forTheme(themeId)
            return OctiThemeColors(
                themeId = themeId,
                windowBg = Color(p.windowBg.toLong() and 0xFFFFFFFFL),
                panelCardBg = Color(p.panelCardBg.toLong() and 0xFFFFFFFFL),
                panelCardBorder = Color(p.panelCardBorder.toLong() and 0xFFFFFFFFL),
                textPrimary = Color(p.textPrimary.toLong() and 0xFFFFFFFFL),
                textSecondary = Color(p.textSecondary.toLong() and 0xFFFFFFFFL),
                accentBrass = Color(p.accentBrass.toLong() and 0xFFFFFFFFL),
                boardOuterFrame = Color(p.boardOuterFrame.toLong() and 0xFFFFFFFFL),
                gridCellDark = Color(p.gridCellDark.toLong() and 0xFFFFFFFFL),
                gridCellLight = Color(p.gridCellLight.toLong() and 0xFFFFFFFFL),
                coordLabel = Color(p.coordLabel.toLong() and 0xFFFFFFFFL),
                podAlpha = Color(p.podA.core.toLong() and 0xFFFFFFFFL),
                podBeta = Color(p.podB.core.toLong() and 0xFFFFFFFFL),
                podGamma = Color(p.podC.core.toLong() and 0xFFFFFFFFL),
                podDelta = Color(p.podD.core.toLong() and 0xFFFFFFFFL),
                legalStep = Color(p.legalStepGuide.toLong() and 0xFFFFFFFFL),
                legalJump = Color(p.legalJumpGuide.toLong() and 0xFFFFFFFFL),
                legalCapture = Color(p.legalCaptureGlow.toLong() and 0xFFFFFFFFL)
            )
        }
    }
}
