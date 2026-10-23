/*
 * OctiPalette.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public final class OctiPalette {

    public static final class PodFactionPalette {
        public final int core;
        public final int rim;
        public final int highlight;
        public final int shadow;
        public final int emblem;

        public PodFactionPalette(int core, int rim, int highlight, int shadow, int emblem) {
            this.core = core;
            this.rim = rim;
            this.highlight = highlight;
            this.shadow = shadow;
            this.emblem = emblem;
        }
    }

    public final OctiThemeId themeId;
    public final int windowBg;
    public final int panelCardBg;
    public final int panelCardBorder;
    public final int textPrimary;
    public final int textSecondary;
    public final int accentBrass;

    public final int boardOuterFrame;
    public final int boardBevelLight;
    public final int boardBevelDark;
    public final int gridCellDark;
    public final int gridCellLight;
    public final int gridSocketShadow;
    public final int gridLine;
    public final int gridBorder;
    public final int coordLabel;
    public final int coordText;

    public final int neutralOctiFill;
    public final int neutralOctiRim;
    public final int neutralOctiRing;
    public final int edgelessPortalGlow;
    public final int legalStepGuide;
    public final int legalJumpGuide;
    public final int legalCaptureGlow;
    public final int highlightLegal;
    public final int highlightCapture;

    public final int prongNormalMetal;
    public final int prongNormalTip;
    public final int prongNormal;
    public final int prongSuperGold;
    public final int prongSuperCore;
    public final int prongSuper;

    public final PodFactionPalette podA;
    public final PodFactionPalette podB;
    public final PodFactionPalette podC;
    public final PodFactionPalette podD;

    private static final OctiPalette OBSIDIAN_BRASS_INSTANCE = createObsidianBrass();
    private static final OctiPalette ROSEWOOD_IVORY_INSTANCE = createRosewoodIvory();

    private OctiPalette(
            OctiThemeId themeId,
            int windowBg,
            int panelCardBg,
            int panelCardBorder,
            int textPrimary,
            int textSecondary,
            int accentBrass,
            int boardOuterFrame,
            int boardBevelLight,
            int boardBevelDark,
            int gridCellDark,
            int gridCellLight,
            int gridSocketShadow,
            int gridLine,
            int coordLabel,
            int neutralOctiFill,
            int neutralOctiRim,
            int edgelessPortalGlow,
            int legalStepGuide,
            int legalJumpGuide,
            int legalCaptureGlow,
            int prongNormalMetal,
            int prongNormalTip,
            int prongSuperGold,
            int prongSuperCore,
            PodFactionPalette podA,
            PodFactionPalette podB,
            PodFactionPalette podC,
            PodFactionPalette podD) {
        this.themeId = themeId;
        this.windowBg = windowBg;
        this.panelCardBg = panelCardBg;
        this.panelCardBorder = panelCardBorder;
        this.textPrimary = textPrimary;
        this.textSecondary = textSecondary;
        this.accentBrass = accentBrass;
        this.boardOuterFrame = boardOuterFrame;
        this.boardBevelLight = boardBevelLight;
        this.boardBevelDark = boardBevelDark;
        this.gridCellDark = gridCellDark;
        this.gridCellLight = gridCellLight;
        this.gridSocketShadow = gridSocketShadow;
        this.gridLine = gridLine;
        this.gridBorder = gridLine;
        this.coordLabel = coordLabel;
        this.coordText = coordLabel;
        this.neutralOctiFill = neutralOctiFill;
        this.neutralOctiRim = neutralOctiRim;
        this.neutralOctiRing = neutralOctiRim;
        this.edgelessPortalGlow = edgelessPortalGlow;
        this.legalStepGuide = legalStepGuide;
        this.legalJumpGuide = legalJumpGuide;
        this.legalCaptureGlow = legalCaptureGlow;
        this.highlightLegal = legalStepGuide;
        this.highlightCapture = legalCaptureGlow;
        this.prongNormalMetal = prongNormalMetal;
        this.prongNormalTip = prongNormalTip;
        this.prongNormal = prongNormalMetal;
        this.prongSuperGold = prongSuperGold;
        this.prongSuperCore = prongSuperCore;
        this.prongSuper = prongSuperGold;
        this.podA = podA;
        this.podB = podB;
        this.podC = podC;
        this.podD = podD;
    }

    private static OctiPalette createObsidianBrass() {
        return new OctiPalette(
                OctiThemeId.OBSIDIAN_BRASS,
                0xFF0B0F17, // windowBg
                0xFF141B29, // panelCardBg
                0xFFB49148, // panelCardBorder
                0xFFF2ECE0, // textPrimary
                0xFFA8B2C6, // textSecondary
                0xFFC8A253, // accentBrass
                0xFF101622, // boardOuterFrame
                0xFFE4C070, // boardBevelLight
                0xFF664C1E, // boardBevelDark
                0xFF151E30, // gridCellDark
                0xFF1D2940, // gridCellLight
                0xBE080C14, // gridSocketShadow
                0x73B8944C, // gridLine
                0xFFD8B66C, // coordLabel
                0xFF2D3A54, // neutralOctiFill
                0xFFDCB869, // neutralOctiRim
                0xFF38BDF8, // edgelessPortalGlow
                0xFF34D399, // legalStepGuide
                0xFFFBBF24, // legalJumpGuide
                0xFFF43F5E, // legalCaptureGlow
                0xFFD6DCE8, // prongNormalMetal
                0xFFF8FAFC, // prongNormalTip
                0xFFFDE047, // prongSuperGold
                0xFFFFFBEB, // prongSuperCore
                new PodFactionPalette(0xFF10B981, 0xFF059669, 0xFF6EE7B7, 0xFF064E3B, 0xFFECFDF5),
                new PodFactionPalette(0xFFF43F5E, 0xFFBE123C, 0xFFFDA4AF, 0xFF881337, 0xFFFFF1F2),
                new PodFactionPalette(0xFF3B82F6, 0xFF1D4ED8, 0xFF93C5FD, 0xFF1E3A8A, 0xFFEFF6FF),
                new PodFactionPalette(0xFFF59E0B, 0xFFB45309, 0xFFFDE68A, 0xFF78350F, 0xFFFFFBEB));
    }

    private static OctiPalette createRosewoodIvory() {
        return new OctiPalette(
                OctiThemeId.ROSEWOOD_IVORY,
                0xFF2A1812, // windowBg
                0xFF3E241B, // panelCardBg
                0xFFCEA660, // panelCardBorder
                0xFFFBF5E8, // textPrimary
                0xFFD6C2A8, // textSecondary
                0xFFD6AC5C, // accentBrass
                0xFF4A261A, // boardOuterFrame
                0xFFEECC84, // boardBevelLight
                0xFF70441C, // boardBevelDark
                0xFF623626, // gridCellDark
                0xFF7C4832, // gridCellLight
                0xB922100A, // gridSocketShadow
                0x82DEBA74, // gridLine
                0xFFF4DEB0, // coordLabel
                0xFF92603E, // neutralOctiFill
                0xFFF0CE84, // neutralOctiRim
                0xFF2DD4BF, // edgelessPortalGlow
                0xFF4ADE80, // legalStepGuide
                0xFFFACC15, // legalJumpGuide
                0xFFFB7185, // legalCaptureGlow
                0xFFE8E0D0, // prongNormalMetal
                0xFFFFFCF5, // prongNormalTip
                0xFFFDE047, // prongSuperGold
                0xFFFFFFF0, // prongSuperCore
                new PodFactionPalette(0xFF0D9468, 0xFFEEE4CC, 0xFF6EE7B7, 0xFF064431, 0xFFFCF8EC),
                new PodFactionPalette(0xFFD82E4E, 0xFFEEE4CC, 0xFFFDA4AF, 0xFF76102A, 0xFFFCF8EC),
                new PodFactionPalette(0xFF2563EB, 0xFFEEE4CC, 0xFF93C5FD, 0xFF183076, 0xFFFCF8EC),
                new PodFactionPalette(0xFFD97706, 0xFFEEE4CC, 0xFFFDE68A, 0xFF6C300C, 0xFFFCF8EC));
    }

    public static OctiPalette forTheme(OctiThemeId id) {
        return (id == OctiThemeId.ROSEWOOD_IVORY)
                ? ROSEWOOD_IVORY_INSTANCE
                : OBSIDIAN_BRASS_INSTANCE;
    }

    public static OctiPalette paletteForTheme(OctiThemeId id) {
        return forTheme(id);
    }

    public PodFactionPalette factionForPodType(OctiPodType type) {
        if (type == null) {
            return podA;
        }
        switch (type) {
            case A_POD:
                return podA;
            case B_POD:
                return podB;
            case C_POD:
                return podC;
            case D_POD:
                return podD;
            default:
                return podA;
        }
    }

    public PodFactionPalette factionForPlayer(OctiPlayer player) {
        if (player == null) {
            return podA;
        }
        return factionForPodType(player.toPodType());
    }

    public int podFill(OctiPlayer player) {
        return factionForPlayer(player).core;
    }

    public int podFill(OctiPodType type) {
        return factionForPodType(type).core;
    }

    public int podRim(OctiPlayer player) {
        return factionForPlayer(player).rim;
    }

    public int podRim(OctiPodType type) {
        return factionForPodType(type).rim;
    }

    public int podHighlight(OctiPlayer player) {
        return factionForPlayer(player).highlight;
    }

    public int podShadow(OctiPlayer player) {
        return factionForPlayer(player).shadow;
    }

    public int podEmblem(OctiPlayer player) {
        return factionForPlayer(player).emblem;
    }
}
