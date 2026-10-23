/*
 * Theme.kt
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi.ui.theme

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.ReadOnlyComposable
import androidx.compose.runtime.staticCompositionLocalOf
import com.selfso.octi.OctiThemeId

private val ObsidianColorScheme = darkColorScheme(
    primary = ObsidianAccentBrass,
    onPrimary = ObsidianWindowBg,
    secondary = PodDeltaCobalt,
    onSecondary = ObsidianTextPrimary,
    tertiary = PodBetaGreen,
    background = ObsidianWindowBg,
    onBackground = ObsidianTextPrimary,
    surface = ObsidianPanelCardBg,
    onSurface = ObsidianTextPrimary,
    surfaceVariant = ObsidianGridDark,
    onSurfaceVariant = ObsidianTextSecondary,
    outline = ObsidianPanelCardBorder,
    error = LegalCaptureCrimson
)

private val RosewoodColorScheme = darkColorScheme(
    primary = RosewoodAccentBrass,
    onPrimary = RosewoodWindowBg,
    secondary = PodGammaAmber,
    onSecondary = RosewoodWindowBg,
    tertiary = PodAlphaRed,
    background = RosewoodWindowBg,
    onBackground = RosewoodTextPrimary,
    surface = RosewoodPanelCardBg,
    onSurface = RosewoodTextPrimary,
    surfaceVariant = RosewoodGridDark,
    onSurfaceVariant = RosewoodTextSecondary,
    outline = RosewoodPanelCardBorder,
    error = LegalCaptureCrimson
)

val LocalOctiThemeColors = staticCompositionLocalOf {
    OctiThemeColors.fromThemeId(OctiThemeId.OBSIDIAN_BRASS)
}

object OctiTheme {
    val colors: OctiThemeColors
        @Composable
        @ReadOnlyComposable
        get() = LocalOctiThemeColors.current
}

@Composable
fun OctiMaterialTheme(
    themeId: OctiThemeId = OctiThemeId.OBSIDIAN_BRASS,
    content: @Composable () -> Unit
) {
    val octiColors = OctiThemeColors.fromThemeId(themeId)
    val scheme = when (themeId) {
        OctiThemeId.OBSIDIAN_BRASS -> ObsidianColorScheme
        OctiThemeId.ROSEWOOD_IVORY -> RosewoodColorScheme
    }
    CompositionLocalProvider(LocalOctiThemeColors provides octiColors) {
        MaterialTheme(
            colorScheme = scheme,
            typography = OctiTypography,
            content = content
        )
    }
}
