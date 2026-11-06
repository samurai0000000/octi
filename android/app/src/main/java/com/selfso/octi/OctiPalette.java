/*
 * OctiPalette.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public final class OctiPalette {

    public enum PodFinish {
        CLASSIC,
        GLASS,
        CARVED,
        HAMMERED,
        GLOSS,
        NACRE,
        BRASS,
        PLANETARY
    }

    public enum PodPattern {
        NONE,
        CIRCUIT,
        CLOUD_SCROLL,
        FACET,
        EMBER,
        STRIPES,
        DOTS,
        CHECKS,
        SWIRL,
        SHELL,
        GEARS,
        BANDS,
        FACTION_SPECIFIC
    }

    public enum ProngStyle {
        CLASSIC,
        NEON_TUBE,
        FILIGREE,
        ICICLE,
        FORGED,
        LOLLIPOP,
        CORAL,
        PIPE,
        COMET
    }

    public enum BoardTexture {
        NONE,
        PCB,
        LACQUER_SCROLL,
        FROST,
        BASALT_LAVA,
        SPRINKLES,
        CAUSTICS,
        RIVET_PLATE,
        STARFIELD
    }

    public enum CellShape {
        SQUARE,
        ROUNDED
    }

    public static final class PodFactionPalette {
        public final int core;
        public final int rim;
        public final int highlight;
        public final int shadow;
        public final int emblem;
        public final PodPattern pattern;

        public PodFactionPalette(int core, int rim, int highlight, int shadow, int emblem, PodPattern pattern) {
            this.core = core;
            this.rim = rim;
            this.highlight = highlight;
            this.shadow = shadow;
            this.emblem = emblem;
            this.pattern = pattern != null ? pattern : PodPattern.NONE;
        }

        public PodFactionPalette(int core, int rim, int highlight, int shadow, int emblem) {
            this(core, rim, highlight, shadow, emblem, PodPattern.NONE);
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
    public final int homeBadgeOuterBg;
    public final int homeBadgeInnerBg;

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

    public final PodFinish podFinish;
    public final PodPattern podPattern;
    public final ProngStyle prongStyle;
    public final BoardTexture boardTexture;
    public final CellShape cellShape;
    public final float glowStrength;

    static final OctiPalette OBSIDIAN_BRASS_INSTANCE = createObsidianBrass();
    private static final OctiPalette ROSEWOOD_IVORY_INSTANCE = createRosewoodIvory();
    private static final OctiPalette NEON_CIRCUIT_INSTANCE = createNeonCircuit();
    private static final OctiPalette JADE_DYNASTY_INSTANCE = createJadeDynasty();
    private static final OctiPalette ARCTIC_AURORA_INSTANCE = createArcticAurora();
    private static final OctiPalette VOLCANIC_FORGE_INSTANCE = createVolcanicForge();
    private static final OctiPalette CANDY_POP_INSTANCE = createCandyPop();
    private static final OctiPalette OCEAN_PEARL_INSTANCE = createOceanPearl();
    private static final OctiPalette STEAMPUNK_BRASS_INSTANCE = createSteampunkBrass();
    private static final OctiPalette COSMIC_NEBULA_INSTANCE = createCosmicNebula();

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
            int homeBadgeOuterBg,
            int homeBadgeInnerBg,
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
            PodFactionPalette podD,
            PodFinish podFinish,
            PodPattern podPattern,
            ProngStyle prongStyle,
            BoardTexture boardTexture,
            CellShape cellShape,
            float glowStrength) {
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
        this.homeBadgeOuterBg = homeBadgeOuterBg;
        this.homeBadgeInnerBg = homeBadgeInnerBg;
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
        this.podFinish = podFinish;
        this.podPattern = podPattern;
        this.prongStyle = prongStyle;
        this.boardTexture = boardTexture;
        this.cellShape = cellShape;
        this.glowStrength = glowStrength;
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
                0xFF0A0E16, // homeBadgeOuterBg
                0xFF070A10, // homeBadgeInnerBg
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
                new PodFactionPalette(0xFF10B981, 0xFF059669, 0xFF6EE7B7, 0xFF064E3B, 0xFFECFDF5, PodPattern.NONE),
                new PodFactionPalette(0xFFF43F5E, 0xFFBE123C, 0xFFFDA4AF, 0xFF881337, 0xFFFFF1F2, PodPattern.NONE),
                new PodFactionPalette(0xFF3B82F6, 0xFF1D4ED8, 0xFF93C5FD, 0xFF1E3A8A, 0xFFEFF6FF, PodPattern.NONE),
                new PodFactionPalette(0xFFF59E0B, 0xFFB45309, 0xFFFDE68A, 0xFF78350F, 0xFFFFFBEB, PodPattern.NONE),
                PodFinish.CLASSIC,
                PodPattern.NONE,
                ProngStyle.CLASSIC,
                BoardTexture.NONE,
                CellShape.SQUARE,
                0.0f);
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
                0xFF140E0B, // homeBadgeOuterBg
                0xFF0E0A08, // homeBadgeInnerBg
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
                new PodFactionPalette(0xFF0D9468, 0xFFEEE4CC, 0xFF6EE7B7, 0xFF064431, 0xFFFCF8EC, PodPattern.NONE),
                new PodFactionPalette(0xFFD82E4E, 0xFFEEE4CC, 0xFFFDA4AF, 0xFF76102A, 0xFFFCF8EC, PodPattern.NONE),
                new PodFactionPalette(0xFF2563EB, 0xFFEEE4CC, 0xFF93C5FD, 0xFF183076, 0xFFFCF8EC, PodPattern.NONE),
                new PodFactionPalette(0xFFD97706, 0xFFEEE4CC, 0xFFFDE68A, 0xFF6C300C, 0xFFFCF8EC, PodPattern.NONE),
                PodFinish.CLASSIC,
                PodPattern.NONE,
                ProngStyle.CLASSIC,
                BoardTexture.NONE,
                CellShape.SQUARE,
                0.0f);
    }

    private static OctiPalette createNeonCircuit() {
        return new OctiPalette(
                OctiThemeId.NEON_CIRCUIT,
                0xFF080B10, // windowBg
                0xFF0E1420, // panelCardBg
                0xFF00E5FF, // panelCardBorder
                0xFFF0FDFE, // textPrimary
                0xFF7DD3FC, // textSecondary
                0xFF00E5FF, // accentBrass
                0xFF0A0F1A, // boardOuterFrame
                0xFF00E5FF, // boardBevelLight
                0xFF063248, // boardBevelDark
                0xFF0B101D, // gridCellDark
                0xFF121A2E, // gridCellLight
                0xB803060C, // gridSocketShadow
                0x6600E5FF, // gridLine
                0xFF38BDF8, // coordLabel
                0xFF070A12, // homeBadgeOuterBg
                0xFF04060B, // homeBadgeInnerBg
                0xFF1E293B, // neutralOctiFill
                0xFF00E5FF, // neutralOctiRim
                0xFF00F0FF, // edgelessPortalGlow
                0xFF00FF9D, // legalStepGuide
                0xFFFFE600, // legalJumpGuide
                0xFFFF0055, // legalCaptureGlow
                0xFFE0F2FE, // prongNormalMetal
                0xFFFFFFFF, // prongNormalTip
                0xFFFFD700, // prongSuperGold
                0xFFFFFFEE, // prongSuperCore
                new PodFactionPalette(0xFF007A87, 0xFF00E5FF, 0xFF80F2FF, 0xFF00363D, 0xFFFFFFFF, PodPattern.CIRCUIT),
                new PodFactionPalette(0xFFB81446, 0xFFFF007F, 0xFFFF80BF, 0xFF4D001A, 0xFFFFFFFF, PodPattern.CIRCUIT),
                new PodFactionPalette(0xFF4D7C0F, 0xFF76FF03, 0xFFBFFF80, 0xFF1B3300, 0xFFFFFFFF, PodPattern.CIRCUIT),
                new PodFactionPalette(0xFFA15C07, 0xFFFFB300, 0xFFFFD980, 0xFF402200, 0xFFFFFFFF, PodPattern.CIRCUIT),
                PodFinish.GLASS,
                PodPattern.CIRCUIT,
                ProngStyle.NEON_TUBE,
                BoardTexture.PCB,
                CellShape.SQUARE,
                0.85f);
    }

    private static OctiPalette createJadeDynasty() {
        return new OctiPalette(
                OctiThemeId.JADE_DYNASTY,
                0xFF0B130E, // windowBg
                0xFF121F17, // panelCardBg
                0xFFC5A059, // panelCardBorder
                0xFFF7F4EB, // textPrimary
                0xFFA3B899, // textSecondary
                0xFFD4AF37, // accentBrass
                0xFF132219, // boardOuterFrame
                0xFFDFBA5C, // boardBevelLight
                0xFF5C4418, // boardBevelDark
                0xFF16291E, // gridCellDark
                0xFF1F382A, // gridCellLight
                0xB409110C, // gridSocketShadow
                0x60C5A059, // gridLine
                0xFFE5C678, // coordLabel
                0xFF0F1D14, // homeBadgeOuterBg
                0xFF0A140E, // homeBadgeInnerBg
                0xFF384D3E, // neutralOctiFill
                0xFFD4AF37, // neutralOctiRim
                0xFF5EEAD4, // edgelessPortalGlow
                0xFF34D399, // legalStepGuide
                0xFFFBBF24, // legalJumpGuide
                0xFFF43F5E, // legalCaptureGlow
                0xFFE6D7B8, // prongNormalMetal
                0xFFFFFDF5, // prongNormalTip
                0xFFF59E0B, // prongSuperGold
                0xFFFFFBEB, // prongSuperCore
                new PodFactionPalette(0xFF086944, 0xFF10B981, 0xFF6EE7B7, 0xFF033321, 0xFFFFFFFF, PodPattern.CLOUD_SCROLL),
                new PodFactionPalette(0xFFB31B2C, 0xFFEF4444, 0xFFFCA5A5, 0xFF540B13, 0xFFFFFFFF, PodPattern.CLOUD_SCROLL),
                new PodFactionPalette(0xFF1E40AF, 0xFF3B82F6, 0xFF93C5FD, 0xFF0E1F59, 0xFFFFFFFF, PodPattern.CLOUD_SCROLL),
                new PodFactionPalette(0xFFA26505, 0xFFF59E0B, 0xFFFDE68A, 0xFF4A2E00, 0xFFFFFFFF, PodPattern.CLOUD_SCROLL),
                PodFinish.CARVED,
                PodPattern.CLOUD_SCROLL,
                ProngStyle.FILIGREE,
                BoardTexture.LACQUER_SCROLL,
                CellShape.SQUARE,
                0.25f);
    }

    private static OctiPalette createArcticAurora() {
        return new OctiPalette(
                OctiThemeId.ARCTIC_AURORA,
                0xFF0B111A, // windowBg
                0xFF121C2B, // panelCardBg
                0xFF5EEAD4, // panelCardBorder
                0xFFF0FDF4, // textPrimary
                0xFF94A3B8, // textSecondary
                0xFF38BDF8, // accentBrass
                0xFF0E1724, // boardOuterFrame
                0xFF7DD3FC, // boardBevelLight
                0xFF0C2540, // boardBevelDark
                0xFF111C2D, // gridCellDark
                0xFF1A2942, // gridCellLight
                0xC0050A12, // gridSocketShadow
                0x507DD3FC, // gridLine
                0xFF93C5FD, // coordLabel
                0xFF0C1420, // homeBadgeOuterBg
                0xFF080D15, // homeBadgeInnerBg
                0xFF2A3B54, // neutralOctiFill
                0xFF7DD3FC, // neutralOctiRim
                0xFF38BDF8, // edgelessPortalGlow
                0xFF34D399, // legalStepGuide
                0xFFFBBF24, // legalJumpGuide
                0xFFF43F5E, // legalCaptureGlow
                0xFFE0F2FE, // prongNormalMetal
                0xFFFFFFFF, // prongNormalTip
                0xFFFDE047, // prongSuperGold
                0xFFFFFBEB, // prongSuperCore
                new PodFactionPalette(0xFF0F766E, 0xFF2DD4BF, 0xFF99F6E4, 0xFF053733, 0xFFFFFFFF, PodPattern.FACET),
                new PodFactionPalette(0xFF9D174D, 0xFFF43F5E, 0xFFFECDD3, 0xFF4C0523, 0xFFFFFFFF, PodPattern.FACET),
                new PodFactionPalette(0xFF3730A3, 0xFF6366F1, 0xFFC7D2FE, 0xFF181552, 0xFFFFFFFF, PodPattern.FACET),
                new PodFactionPalette(0xFF9A6006, 0xFFFBBF24, 0xFFFEF08A, 0xFF462B01, 0xFFFFFFFF, PodPattern.FACET),
                PodFinish.GLASS,
                PodPattern.FACET,
                ProngStyle.ICICLE,
                BoardTexture.FROST,
                CellShape.ROUNDED,
                0.6f);
    }

    private static OctiPalette createVolcanicForge() {
        return new OctiPalette(
                OctiThemeId.VOLCANIC_FORGE,
                0xFF0F0C0B, // windowBg
                0xFF1C1614, // panelCardBg
                0xFFD97706, // panelCardBorder
                0xFFFFF7ED, // textPrimary
                0xFFA89A95, // textSecondary
                0xFFF59E0B, // accentBrass
                0xFF16110F, // boardOuterFrame
                0xFFF97316, // boardBevelLight
                0xFF3A190C, // boardBevelDark
                0xFF171210, // gridCellDark
                0xFF221B17, // gridCellLight
                0xB80A0706, // gridSocketShadow
                0x80FF5722, // gridLine
                0xFFFB923C, // coordLabel
                0xFF110C0A, // homeBadgeOuterBg
                0xFF0B0806, // homeBadgeInnerBg
                0xFF382A24, // neutralOctiFill
                0xFFFB923C, // neutralOctiRim
                0xFFF97316, // edgelessPortalGlow
                0xFF34D399, // legalStepGuide
                0xFFFBBF24, // legalJumpGuide
                0xFFFF1744, // legalCaptureGlow
                0xFFD1C7BD, // prongNormalMetal
                0xFFFFE4D6, // prongNormalTip
                0xFFF59E0B, // prongSuperGold
                0xFFFFF7ED, // prongSuperCore
                new PodFactionPalette(0xFF066B4E, 0xFF059669, 0xFF6EE7B7, 0xFF023325, 0xFFFFFFFF, PodPattern.EMBER),
                new PodFactionPalette(0xFFA81520, 0xFFDC2626, 0xFFFCA5A5, 0xFF4E050B, 0xFFFFFFFF, PodPattern.EMBER),
                new PodFactionPalette(0xFF1E3A8A, 0xFF2563EB, 0xFF93C5FD, 0xFF0E1B42, 0xFFFFFFFF, PodPattern.EMBER),
                new PodFactionPalette(0xFF9E6800, 0xFFEAB308, 0xFFFEF08A, 0xFF472F00, 0xFFFFFFFF, PodPattern.EMBER),
                PodFinish.HAMMERED,
                PodPattern.EMBER,
                ProngStyle.FORGED,
                BoardTexture.BASALT_LAVA,
                CellShape.SQUARE,
                0.7f);
    }

    private static OctiPalette createCandyPop() {
        return new OctiPalette(
                OctiThemeId.CANDY_POP,
                0xFF1A101C, // windowBg
                0xFF261729, // panelCardBg
                0xFFFF69B4, // panelCardBorder
                0xFFFFF5F9, // textPrimary
                0xFFE2B8D6, // textSecondary
                0xFFFF69B4, // accentBrass
                0xFF221226, // boardOuterFrame
                0xFFFFB6C1, // boardBevelLight
                0xFF4A154B, // boardBevelDark
                0xFF24142B, // gridCellDark
                0xFF331E3D, // gridCellLight
                0xA8120716, // gridSocketShadow
                0x55FFB6C1, // gridLine
                0xFFFBCFE8, // coordLabel
                0xFF1B0D21, // homeBadgeOuterBg
                0xFF130917, // homeBadgeInnerBg
                0xFF4D2C54, // neutralOctiFill
                0xFFFF80BF, // neutralOctiRim
                0xFF38BDF8, // edgelessPortalGlow
                0xFF34D399, // legalStepGuide
                0xFFFBBF24, // legalJumpGuide
                0xFFFF1493, // legalCaptureGlow
                0xFFFDE2E4, // prongNormalMetal
                0xFFFFFFFF, // prongNormalTip
                0xFFFDE047, // prongSuperGold
                0xFFFFFBEB, // prongSuperCore
                new PodFactionPalette(0xFF057A55, 0xFF10B981, 0xFFA7F3D0, 0xFF023827, 0xFFFFFFFF, PodPattern.STRIPES),
                new PodFactionPalette(0xFFB91C47, 0xFFF43F5E, 0xFFFECDD3, 0xFF560B1F, 0xFFFFFFFF, PodPattern.DOTS),
                new PodFactionPalette(0xFF1D4ED8, 0xFF3B82F6, 0xFFBFDBFE, 0xFF0C2570, 0xFFFFFFFF, PodPattern.CHECKS),
                new PodFactionPalette(0xFFA16207, 0xFFF59E0B, 0xFFFDE68A, 0xFF472B02, 0xFFFFFFFF, PodPattern.SWIRL),
                PodFinish.GLOSS,
                PodPattern.FACTION_SPECIFIC,
                ProngStyle.LOLLIPOP,
                BoardTexture.SPRINKLES,
                CellShape.ROUNDED,
                0.2f);
    }

    private static OctiPalette createOceanPearl() {
        return new OctiPalette(
                OctiThemeId.OCEAN_PEARL,
                0xFF071018, // windowBg
                0xFF0D1C2A, // panelCardBg
                0xFF38BDF8, // panelCardBorder
                0xFFF0FDF4, // textPrimary
                0xFF7DD3FC, // textSecondary
                0xFF2DD4BF, // accentBrass
                0xFF0A1624, // boardOuterFrame
                0xFF5EEAD4, // boardBevelLight
                0xFF06283D, // boardBevelDark
                0xFF0D1E2D, // gridCellDark
                0xFF132B40, // gridCellLight
                0xBC040A12, // gridSocketShadow
                0x5038BDF8, // gridLine
                0xFF67E8F9, // coordLabel
                0xFF08141F, // homeBadgeOuterBg
                0xFF050D15, // homeBadgeInnerBg
                0xFF1F3A4D, // neutralOctiFill
                0xFF2DD4BF, // neutralOctiRim
                0xFF22D3EE, // edgelessPortalGlow
                0xFF34D399, // legalStepGuide
                0xFFFBBF24, // legalJumpGuide
                0xFFF43F5E, // legalCaptureGlow
                0xFFE0F2FE, // prongNormalMetal
                0xFFFFFFFF, // prongNormalTip
                0xFFFDE047, // prongSuperGold
                0xFFFFFBEB, // prongSuperCore
                new PodFactionPalette(0xFF057A5F, 0xFF14B8A6, 0xFF99F6E4, 0xFF02382C, 0xFFFFFFFF, PodPattern.SHELL),
                new PodFactionPalette(0xFFB81938, 0xFFF43F5E, 0xFFFECDD3, 0xFF540A18, 0xFFFFFFFF, PodPattern.SHELL),
                new PodFactionPalette(0xFF1743A3, 0xFF3B82F6, 0xFFBFDBFE, 0xFF091F52, 0xFFFFFFFF, PodPattern.SHELL),
                new PodFactionPalette(0xFF9E6505, 0xFFF59E0B, 0xFFFEF08A, 0xFF472E01, 0xFFFFFFFF, PodPattern.SHELL),
                PodFinish.NACRE,
                PodPattern.SHELL,
                ProngStyle.CORAL,
                BoardTexture.CAUSTICS,
                CellShape.ROUNDED,
                0.45f);
    }

    private static OctiPalette createSteampunkBrass() {
        return new OctiPalette(
                OctiThemeId.STEAMPUNK_BRASS,
                0xFF140F0A, // windowBg
                0xFF221810, // panelCardBg
                0xFFC88A3A, // panelCardBorder
                0xFFFBF4E6, // textPrimary
                0xFFBF9F80, // textSecondary
                0xFFDDA15E, // accentBrass
                0xFF1F140C, // boardOuterFrame
                0xFFE09F3E, // boardBevelLight
                0xFF522B08, // boardBevelDark
                0xFF241810, // gridCellDark
                0xFF332216, // gridCellLight
                0xB4100905, // gridSocketShadow
                0x60DDA15E, // gridLine
                0xFFF2BE74, // coordLabel
                0xFF18100A, // homeBadgeOuterBg
                0xFF100A06, // homeBadgeInnerBg
                0xFF473223, // neutralOctiFill
                0xFFDDA15E, // neutralOctiRim
                0xFF38BDF8, // edgelessPortalGlow
                0xFF34D399, // legalStepGuide
                0xFFFBBF24, // legalJumpGuide
                0xFFF43F5E, // legalCaptureGlow
                0xFFE6CCB2, // prongNormalMetal
                0xFFFFF1E6, // prongNormalTip
                0xFFF59E0B, // prongSuperGold
                0xFFFFFBEB, // prongSuperCore
                new PodFactionPalette(0xFF0E7462, 0xFF10B981, 0xFFA7F3D0, 0xFF053B32, 0xFFFFFFFF, PodPattern.GEARS),
                new PodFactionPalette(0xFFA31828, 0xFFEF4444, 0xFFFCA5A5, 0xFF470811, 0xFFFFFFFF, PodPattern.GEARS),
                new PodFactionPalette(0xFF193D87, 0xFF3B82F6, 0xFFBFDBFE, 0xFF0B1B3D, 0xFFFFFFFF, PodPattern.GEARS),
                new PodFactionPalette(0xFF9C6304, 0xFFD97706, 0xFFFDE68A, 0xFF452C01, 0xFFFFFFFF, PodPattern.GEARS),
                PodFinish.BRASS,
                PodPattern.GEARS,
                ProngStyle.PIPE,
                BoardTexture.RIVET_PLATE,
                CellShape.SQUARE,
                0.3f);
    }

    private static OctiPalette createCosmicNebula() {
        return new OctiPalette(
                OctiThemeId.COSMIC_NEBULA,
                0xFF080712, // windowBg
                0xFF110E24, // panelCardBg
                0xFF8B5CF6, // panelCardBorder
                0xFFF8FAFC, // textPrimary
                0xFFA5B4FC, // textSecondary
                0xFFC084FC, // accentBrass
                0xFF0E0C21, // boardOuterFrame
                0xFFA855F7, // boardBevelLight
                0xFF2E1065, // boardBevelDark
                0xFF100D26, // gridCellDark
                0xFF1A163B, // gridCellLight
                0xA805030F, // gridSocketShadow
                0x558B5CF6, // gridLine
                0xFFC4B5FD, // coordLabel
                0xFF0B081A, // homeBadgeOuterBg
                0xFF060410, // homeBadgeInnerBg
                0xFF2B2456, // neutralOctiFill
                0xFFC084FC, // neutralOctiRim
                0xFF38BDF8, // edgelessPortalGlow
                0xFF34D399, // legalStepGuide
                0xFFFBBF24, // legalJumpGuide
                0xFFF43F5E, // legalCaptureGlow
                0xFFE0E7FF, // prongNormalMetal
                0xFFFFFFFF, // prongNormalTip
                0xFFFDE047, // prongSuperGold
                0xFFFFFBEB, // prongSuperCore
                new PodFactionPalette(0xFF057A55, 0xFF10B981, 0xFF6EE7B7, 0xFF023827, 0xFFFFFFFF, PodPattern.BANDS),
                new PodFactionPalette(0xFFB5173F, 0xFFF43F5E, 0xFFFDA4AF, 0xFF54091C, 0xFFFFFFFF, PodPattern.BANDS),
                new PodFactionPalette(0xFF4338CA, 0xFF6366F1, 0xFFC7D2FE, 0xFF1E1961, 0xFFFFFFFF, PodPattern.BANDS),
                new PodFactionPalette(0xFF9E6404, 0xFFF59E0B, 0xFFFDE68A, 0xFF472C00, 0xFFFFFFFF, PodPattern.BANDS),
                PodFinish.PLANETARY,
                PodPattern.BANDS,
                ProngStyle.COMET,
                BoardTexture.STARFIELD,
                CellShape.ROUNDED,
                0.8f);
    }

    public static OctiPalette forTheme(OctiThemeId id) {
        if (id == null) {
            return OBSIDIAN_BRASS_INSTANCE;
        }
        switch (id) {
            case OBSIDIAN_BRASS:
                return OBSIDIAN_BRASS_INSTANCE;
            case ROSEWOOD_IVORY:
                return ROSEWOOD_IVORY_INSTANCE;
            case NEON_CIRCUIT:
                return NEON_CIRCUIT_INSTANCE;
            case JADE_DYNASTY:
                return JADE_DYNASTY_INSTANCE;
            case ARCTIC_AURORA:
                return ARCTIC_AURORA_INSTANCE;
            case VOLCANIC_FORGE:
                return VOLCANIC_FORGE_INSTANCE;
            case CANDY_POP:
                return CANDY_POP_INSTANCE;
            case OCEAN_PEARL:
                return OCEAN_PEARL_INSTANCE;
            case STEAMPUNK_BRASS:
                return STEAMPUNK_BRASS_INSTANCE;
            case COSMIC_NEBULA:
                return COSMIC_NEBULA_INSTANCE;
            default:
                return OBSIDIAN_BRASS_INSTANCE;
        }
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

    public PodPattern podPattern(OctiPodType type) {
        return factionForPodType(type).pattern;
    }

    public PodPattern podPattern(OctiPlayer player) {
        return factionForPlayer(player).pattern;
    }
}

/*
 * Local variables:
 * mode: Java
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
