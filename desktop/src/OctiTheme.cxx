/*
 * OctiTheme.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiTheme.hxx"
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <QBrush>
#include <QFont>
#include <QLinearGradient>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPolygonF>
#include <QRadialGradient>

namespace octi {

namespace {

constexpr double PI_VAL = 3.14159265358979323846;

OctiPalette makeObsidianBrassPalette()
{
    OctiPalette pal;
    pal.windowBg           = QColor(11, 15, 23);     // #0B0F17
    pal.panelCardBg        = QColor(20, 27, 41);     // #141B29
    pal.panelCardBorder    = QColor(180, 145, 72);   // #B49148
    pal.textPrimary        = QColor(242, 236, 224);  // #F2ECE0
    pal.textSecondary      = QColor(168, 178, 198);  // #A8B2C6
    pal.accentBrass        = QColor(200, 162, 83);   // #C8A253

    pal.boardOuterFrame    = QColor(16, 22, 34);     // #101622
    pal.boardBevelLight    = QColor(228, 192, 112);  // #E4C070
    pal.boardBevelDark     = QColor(102, 76, 30);    // #664C1E
    pal.gridCellDark       = QColor(21, 30, 48);     // #151E30
    pal.gridCellLight      = QColor(29, 41, 64);     // #1D2940
    pal.gridSocketShadow   = QColor(8, 12, 20, 190);
    pal.gridLine           = QColor(184, 148, 76, 115);
    pal.coordLabel         = QColor(216, 182, 108);

    pal.neutralOctiFill    = QColor(45, 58, 84);
    pal.neutralOctiRim     = QColor(220, 184, 105);
    pal.edgelessPortalGlow = QColor(56, 189, 248);   // #38BDF8
    pal.legalStepGuide     = QColor(52, 211, 153);   // #34D399
    pal.legalJumpGuide     = QColor(251, 191, 36);   // #FBBF24
    pal.legalCaptureGlow   = QColor(244, 63, 94);    // #F43F5E

    pal.prongNormalMetal   = QColor(214, 220, 232);
    pal.prongNormalTip     = QColor(248, 250, 252);
    pal.prongSuperGold     = QColor(253, 224, 71);
    pal.prongSuperCore     = QColor(255, 251, 235);

    // Player A: Emerald Jade
    pal.podA.core      = QColor(16, 185, 129);  // #10B981
    pal.podA.rim       = QColor(5, 150, 105);   // #059669
    pal.podA.highlight = QColor(110, 231, 183); // #6EE7B7
    pal.podA.shadow    = QColor(6, 78, 59);     // #064E3B
    pal.podA.emblem    = QColor(236, 253, 245);

    // Player B: Crimson Ruby
    pal.podB.core      = QColor(244, 63, 94);   // #F43F5E
    pal.podB.rim       = QColor(190, 18, 60);   // #BE123C
    pal.podB.highlight = QColor(253, 164, 175); // #FDA4AF
    pal.podB.shadow    = QColor(136, 19, 55);   // #881337
    pal.podB.emblem    = QColor(255, 241, 242);

    // Player C: Royal Sapphire
    pal.podC.core      = QColor(59, 130, 246);  // #3B82F6
    pal.podC.rim       = QColor(29, 78, 216);   // #1D4ED8
    pal.podC.highlight = QColor(147, 197, 253); // #93C5FD
    pal.podC.shadow    = QColor(30, 58, 138);   // #1E3A8A
    pal.podC.emblem    = QColor(239, 246, 255);

    // Player D: Imperial Gold
    pal.podD.core      = QColor(245, 158, 11);  // #F59E0B
    pal.podD.rim       = QColor(180, 83, 9);    // #B45309
    pal.podD.highlight = QColor(253, 230, 138); // #FDE68A
    pal.podD.shadow    = QColor(120, 53, 15);   // #78350F
    pal.podD.emblem    = QColor(255, 251, 235);

    pal.homeBadgeOuterBg   = QColor(10, 14, 22);
    pal.homeBadgeInnerBg   = QColor(7, 10, 16);
    pal.podStyle           = PodStyle{PodFinish::CLASSIC, PodPattern::NONE};
    pal.prongStyle         = ProngStyle::CLASSIC;
    pal.boardTexture       = BoardTexture::NONE;
    pal.cellShape          = CellShape::SQUARE;
    pal.glowStrength       = 0.0;

    return pal;
}

OctiPalette makeRosewoodIvoryPalette()
{
    OctiPalette pal;
    pal.windowBg           = QColor(42, 24, 18);     // Warm dark walnut
    pal.panelCardBg        = QColor(62, 36, 27);     // Rosewood card
    pal.panelCardBorder    = QColor(206, 166, 96);   // Antique gold
    pal.textPrimary        = QColor(251, 245, 232);  // Warm ivory
    pal.textSecondary      = QColor(214, 194, 168);  // Parchment muted
    pal.accentBrass        = QColor(214, 172, 92);

    pal.boardOuterFrame    = QColor(74, 38, 26);     // Deep rosewood frame
    pal.boardBevelLight    = QColor(238, 204, 132);
    pal.boardBevelDark     = QColor(112, 68, 28);
    pal.gridCellDark       = QColor(98, 54, 38);     // Inlaid rosewood square
    pal.gridCellLight      = QColor(124, 72, 50);    // Inlaid burl maple/mahogany square
    pal.gridSocketShadow   = QColor(34, 16, 10, 185);
    pal.gridLine           = QColor(222, 186, 116, 130);
    pal.coordLabel         = QColor(244, 222, 176);

    pal.neutralOctiFill    = QColor(146, 96, 62);
    pal.neutralOctiRim     = QColor(240, 206, 132);
    pal.edgelessPortalGlow = QColor(45, 212, 191);
    pal.legalStepGuide     = QColor(74, 222, 128);
    pal.legalJumpGuide     = QColor(250, 204, 21);
    pal.legalCaptureGlow   = QColor(251, 113, 133);

    pal.prongNormalMetal   = QColor(232, 224, 208);
    pal.prongNormalTip     = QColor(255, 252, 245);
    pal.prongSuperGold     = QColor(253, 224, 71);
    pal.prongSuperCore     = QColor(255, 255, 240);

    // Player A: Imperial Jade on Ivory
    pal.podA.core      = QColor(13, 148, 104);
    pal.podA.rim       = QColor(238, 228, 204);
    pal.podA.highlight = QColor(110, 231, 183);
    pal.podA.shadow    = QColor(6, 68, 49);
    pal.podA.emblem    = QColor(252, 248, 236);

    // Player B: Garnet Ruby on Ivory
    pal.podB.core      = QColor(216, 46, 78);
    pal.podB.rim       = QColor(238, 228, 204);
    pal.podB.highlight = QColor(253, 164, 175);
    pal.podB.shadow    = QColor(118, 16, 42);
    pal.podB.emblem    = QColor(252, 248, 236);

    // Player C: Cobalt Sapphire on Ivory
    pal.podC.core      = QColor(37, 99, 235);
    pal.podC.rim       = QColor(238, 228, 204);
    pal.podC.highlight = QColor(147, 197, 253);
    pal.podC.shadow    = QColor(24, 48, 118);
    pal.podC.emblem    = QColor(252, 248, 236);

    // Player D: Amber Citrine on Ivory
    pal.podD.core      = QColor(217, 119, 6);
    pal.podD.rim       = QColor(238, 228, 204);
    pal.podD.highlight = QColor(253, 230, 138);
    pal.podD.shadow    = QColor(108, 48, 12);
    pal.podD.emblem    = QColor(252, 248, 236);

    pal.homeBadgeOuterBg   = QColor(20, 14, 11);
    pal.homeBadgeInnerBg   = QColor(14, 10, 8);
    pal.podStyle           = PodStyle{PodFinish::CLASSIC, PodPattern::NONE};
    pal.prongStyle         = ProngStyle::CLASSIC;
    pal.boardTexture       = BoardTexture::NONE;
    pal.cellShape          = CellShape::SQUARE;
    pal.glowStrength       = 0.0;

    return pal;
}

OctiPalette makeNeonCircuitPalette()
{
    OctiPalette pal;
    pal.windowBg           = QColor(8, 11, 16);     // #080B10
    pal.panelCardBg        = QColor(14, 20, 32);     // #0E1420
    pal.panelCardBorder    = QColor(0, 229, 255);     // #00E5FF
    pal.textPrimary        = QColor(240, 253, 254);     // #F0FDFE
    pal.textSecondary      = QColor(125, 211, 252);     // #7DD3FC
    pal.accentBrass        = QColor(0, 229, 255);     // #00E5FF

    pal.boardOuterFrame    = QColor(10, 15, 26);     // #0A0F1A
    pal.boardBevelLight    = QColor(0, 229, 255);     // #00E5FF
    pal.boardBevelDark     = QColor(6, 50, 72);     // #063248
    pal.gridCellDark       = QColor(11, 16, 29);     // #0B101D
    pal.gridCellLight      = QColor(18, 26, 46);     // #121A2E
    pal.gridSocketShadow   = QColor(3, 6, 12, 184);     // #03060CB8
    pal.gridLine           = QColor(0, 229, 255, 102);     // #00E5FF66
    pal.coordLabel         = QColor(56, 189, 248);     // #38BDF8
    pal.homeBadgeOuterBg   = QColor(7, 10, 18);     // #070A12
    pal.homeBadgeInnerBg   = QColor(4, 6, 11);     // #04060B

    pal.neutralOctiFill    = QColor(30, 41, 59);     // #1E293B
    pal.neutralOctiRim     = QColor(0, 229, 255);     // #00E5FF
    pal.edgelessPortalGlow = QColor(0, 240, 255);     // #00F0FF
    pal.legalStepGuide     = QColor(0, 255, 157);     // #00FF9D
    pal.legalJumpGuide     = QColor(255, 230, 0);     // #FFE600
    pal.legalCaptureGlow   = QColor(255, 0, 85);     // #FF0055

    pal.prongNormalMetal   = QColor(224, 242, 254);     // #E0F2FE
    pal.prongNormalTip     = QColor(255, 255, 255);     // #FFFFFF
    pal.prongSuperGold     = QColor(255, 215, 0);     // #FFD700
    pal.prongSuperCore     = QColor(255, 255, 238);     // #FFFFEE

    pal.podStyle           = PodStyle{PodFinish::GLASS, PodPattern::CIRCUIT};
    pal.prongStyle         = ProngStyle::NEON_TUBE;
    pal.boardTexture       = BoardTexture::PCB;
    pal.cellShape          = CellShape::SQUARE;
    pal.glowStrength       = 0.85;

    // Neon Cyan
    pal.podA.core      = QColor(0, 122, 135);     // #007A87
    pal.podA.rim       = QColor(0, 229, 255);     // #00E5FF
    pal.podA.highlight = QColor(128, 242, 255);     // #80F2FF
    pal.podA.shadow    = QColor(0, 54, 61);     // #00363D
    pal.podA.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podA.pattern   = PodPattern::CIRCUIT;

    // Neon Magenta
    pal.podB.core      = QColor(184, 20, 70);     // #B81446
    pal.podB.rim       = QColor(255, 0, 127);     // #FF007F
    pal.podB.highlight = QColor(255, 128, 191);     // #FF80BF
    pal.podB.shadow    = QColor(77, 0, 26);     // #4D001A
    pal.podB.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podB.pattern   = PodPattern::CIRCUIT;

    // Electric Lime
    pal.podC.core      = QColor(77, 124, 15);     // #4D7C0F
    pal.podC.rim       = QColor(118, 255, 3);     // #76FF03
    pal.podC.highlight = QColor(191, 255, 128);     // #BFFF80
    pal.podC.shadow    = QColor(27, 51, 0);     // #1B3300
    pal.podC.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podC.pattern   = PodPattern::CIRCUIT;

    // Neon Amber
    pal.podD.core      = QColor(161, 92, 7);     // #A15C07
    pal.podD.rim       = QColor(255, 179, 0);     // #FFB300
    pal.podD.highlight = QColor(255, 217, 128);     // #FFD980
    pal.podD.shadow    = QColor(64, 34, 0);     // #402200
    pal.podD.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podD.pattern   = PodPattern::CIRCUIT;

    return pal;
}

OctiPalette makeJadeDynastyPalette()
{
    OctiPalette pal;
    pal.windowBg           = QColor(11, 19, 14);     // #0B130E
    pal.panelCardBg        = QColor(18, 31, 23);     // #121F17
    pal.panelCardBorder    = QColor(197, 160, 89);     // #C5A059
    pal.textPrimary        = QColor(247, 244, 235);     // #F7F4EB
    pal.textSecondary      = QColor(163, 184, 153);     // #A3B899
    pal.accentBrass        = QColor(212, 175, 55);     // #D4AF37

    pal.boardOuterFrame    = QColor(19, 34, 25);     // #132219
    pal.boardBevelLight    = QColor(223, 186, 92);     // #DFBA5C
    pal.boardBevelDark     = QColor(92, 68, 24);     // #5C4418
    pal.gridCellDark       = QColor(22, 41, 30);     // #16291E
    pal.gridCellLight      = QColor(31, 56, 42);     // #1F382A
    pal.gridSocketShadow   = QColor(9, 17, 12, 180);     // #09110CB4
    pal.gridLine           = QColor(197, 160, 89, 96);     // #C5A05960
    pal.coordLabel         = QColor(229, 198, 120);     // #E5C678
    pal.homeBadgeOuterBg   = QColor(15, 29, 20);     // #0F1D14
    pal.homeBadgeInnerBg   = QColor(10, 20, 14);     // #0A140E

    pal.neutralOctiFill    = QColor(56, 77, 62);     // #384D3E
    pal.neutralOctiRim     = QColor(212, 175, 55);     // #D4AF37
    pal.edgelessPortalGlow = QColor(94, 234, 212);     // #5EEAD4
    pal.legalStepGuide     = QColor(52, 211, 153);     // #34D399
    pal.legalJumpGuide     = QColor(251, 191, 36);     // #FBBF24
    pal.legalCaptureGlow   = QColor(244, 63, 94);     // #F43F5E

    pal.prongNormalMetal   = QColor(230, 215, 184);     // #E6D7B8
    pal.prongNormalTip     = QColor(255, 253, 245);     // #FFFDF5
    pal.prongSuperGold     = QColor(245, 158, 11);     // #F59E0B
    pal.prongSuperCore     = QColor(255, 251, 235);     // #FFFBEB

    pal.podStyle           = PodStyle{PodFinish::CARVED, PodPattern::CLOUD_SCROLL};
    pal.prongStyle         = ProngStyle::FILIGREE;
    pal.boardTexture       = BoardTexture::LACQUER_SCROLL;
    pal.cellShape          = CellShape::SQUARE;
    pal.glowStrength       = 0.25;

    // Imperial Jade
    pal.podA.core      = QColor(8, 105, 68);     // #086944
    pal.podA.rim       = QColor(16, 185, 129);     // #10B981
    pal.podA.highlight = QColor(110, 231, 183);     // #6EE7B7
    pal.podA.shadow    = QColor(3, 51, 33);     // #033321
    pal.podA.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podA.pattern   = PodPattern::CLOUD_SCROLL;

    // Cinnabar Vermilion
    pal.podB.core      = QColor(179, 27, 44);     // #B31B2C
    pal.podB.rim       = QColor(239, 68, 68);     // #EF4444
    pal.podB.highlight = QColor(252, 165, 165);     // #FCA5A5
    pal.podB.shadow    = QColor(84, 11, 19);     // #540B13
    pal.podB.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podB.pattern   = PodPattern::CLOUD_SCROLL;

    // Lapis Lazuli
    pal.podC.core      = QColor(30, 64, 175);     // #1E40AF
    pal.podC.rim       = QColor(59, 130, 246);     // #3B82F6
    pal.podC.highlight = QColor(147, 197, 253);     // #93C5FD
    pal.podC.shadow    = QColor(14, 31, 89);     // #0E1F59
    pal.podC.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podC.pattern   = PodPattern::CLOUD_SCROLL;

    // Imperial Saffron Gold
    pal.podD.core      = QColor(162, 101, 5);     // #A26505
    pal.podD.rim       = QColor(245, 158, 11);     // #F59E0B
    pal.podD.highlight = QColor(253, 230, 138);     // #FDE68A
    pal.podD.shadow    = QColor(74, 46, 0);     // #4A2E00
    pal.podD.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podD.pattern   = PodPattern::CLOUD_SCROLL;

    return pal;
}

OctiPalette makeArcticAuroraPalette()
{
    OctiPalette pal;
    pal.windowBg           = QColor(11, 17, 26);     // #0B111A
    pal.panelCardBg        = QColor(18, 28, 43);     // #121C2B
    pal.panelCardBorder    = QColor(94, 234, 212);     // #5EEAD4
    pal.textPrimary        = QColor(240, 253, 244);     // #F0FDF4
    pal.textSecondary      = QColor(148, 163, 184);     // #94A3B8
    pal.accentBrass        = QColor(56, 189, 248);     // #38BDF8

    pal.boardOuterFrame    = QColor(14, 23, 36);     // #0E1724
    pal.boardBevelLight    = QColor(125, 211, 252);     // #7DD3FC
    pal.boardBevelDark     = QColor(12, 37, 64);     // #0C2540
    pal.gridCellDark       = QColor(17, 28, 45);     // #111C2D
    pal.gridCellLight      = QColor(26, 41, 66);     // #1A2942
    pal.gridSocketShadow   = QColor(5, 10, 18, 192);     // #050A12C0
    pal.gridLine           = QColor(125, 211, 252, 80);     // #7DD3FC50
    pal.coordLabel         = QColor(147, 197, 253);     // #93C5FD
    pal.homeBadgeOuterBg   = QColor(12, 20, 32);     // #0C1420
    pal.homeBadgeInnerBg   = QColor(8, 13, 21);     // #080D15

    pal.neutralOctiFill    = QColor(42, 59, 84);     // #2A3B54
    pal.neutralOctiRim     = QColor(125, 211, 252);     // #7DD3FC
    pal.edgelessPortalGlow = QColor(56, 189, 248);     // #38BDF8
    pal.legalStepGuide     = QColor(52, 211, 153);     // #34D399
    pal.legalJumpGuide     = QColor(251, 191, 36);     // #FBBF24
    pal.legalCaptureGlow   = QColor(244, 63, 94);     // #F43F5E

    pal.prongNormalMetal   = QColor(224, 242, 254);     // #E0F2FE
    pal.prongNormalTip     = QColor(255, 255, 255);     // #FFFFFF
    pal.prongSuperGold     = QColor(253, 224, 71);     // #FDE047
    pal.prongSuperCore     = QColor(255, 251, 235);     // #FFFBEB

    pal.podStyle           = PodStyle{PodFinish::GLASS, PodPattern::FACET};
    pal.prongStyle         = ProngStyle::ICICLE;
    pal.boardTexture       = BoardTexture::FROST;
    pal.cellShape          = CellShape::ROUNDED;
    pal.glowStrength       = 0.6;

    // Aurora Teal-Green
    pal.podA.core      = QColor(15, 118, 110);     // #0F766E
    pal.podA.rim       = QColor(45, 212, 191);     // #2DD4BF
    pal.podA.highlight = QColor(153, 246, 228);     // #99F6E4
    pal.podA.shadow    = QColor(5, 55, 51);     // #053733
    pal.podA.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podA.pattern   = PodPattern::FACET;

    // Arctic Rose-Crimson
    pal.podB.core      = QColor(157, 23, 77);     // #9D174D
    pal.podB.rim       = QColor(244, 63, 94);     // #F43F5E
    pal.podB.highlight = QColor(254, 205, 211);     // #FECDD3
    pal.podB.shadow    = QColor(76, 5, 35);     // #4C0523
    pal.podB.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podB.pattern   = PodPattern::FACET;

    // Glacier Cobalt-Violet
    pal.podC.core      = QColor(55, 48, 163);     // #3730A3
    pal.podC.rim       = QColor(99, 102, 241);     // #6366F1
    pal.podC.highlight = QColor(199, 210, 254);     // #C7D2FE
    pal.podC.shadow    = QColor(24, 21, 82);     // #181552
    pal.podC.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podC.pattern   = PodPattern::FACET;

    // Solar Gold-Amber
    pal.podD.core      = QColor(154, 96, 6);     // #9A6006
    pal.podD.rim       = QColor(251, 191, 36);     // #FBBF24
    pal.podD.highlight = QColor(254, 240, 138);     // #FEF08A
    pal.podD.shadow    = QColor(70, 43, 1);     // #462B01
    pal.podD.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podD.pattern   = PodPattern::FACET;

    return pal;
}

OctiPalette makeVolcanicForgePalette()
{
    OctiPalette pal;
    pal.windowBg           = QColor(15, 12, 11);     // #0F0C0B
    pal.panelCardBg        = QColor(28, 22, 20);     // #1C1614
    pal.panelCardBorder    = QColor(217, 119, 6);     // #D97706
    pal.textPrimary        = QColor(255, 247, 237);     // #FFF7ED
    pal.textSecondary      = QColor(168, 154, 149);     // #A89A95
    pal.accentBrass        = QColor(245, 158, 11);     // #F59E0B

    pal.boardOuterFrame    = QColor(22, 17, 15);     // #16110F
    pal.boardBevelLight    = QColor(249, 115, 22);     // #F97316
    pal.boardBevelDark     = QColor(58, 25, 12);     // #3A190C
    pal.gridCellDark       = QColor(23, 18, 16);     // #171210
    pal.gridCellLight      = QColor(34, 27, 23);     // #221B17
    pal.gridSocketShadow   = QColor(10, 7, 6, 184);     // #0A0706B8
    pal.gridLine           = QColor(255, 87, 34, 128);     // #FF572280
    pal.coordLabel         = QColor(251, 146, 60);     // #FB923C
    pal.homeBadgeOuterBg   = QColor(17, 12, 10);     // #110C0A
    pal.homeBadgeInnerBg   = QColor(11, 8, 6);     // #0B0806

    pal.neutralOctiFill    = QColor(56, 42, 36);     // #382A24
    pal.neutralOctiRim     = QColor(251, 146, 60);     // #FB923C
    pal.edgelessPortalGlow = QColor(249, 115, 22);     // #F97316
    pal.legalStepGuide     = QColor(52, 211, 153);     // #34D399
    pal.legalJumpGuide     = QColor(251, 191, 36);     // #FBBF24
    pal.legalCaptureGlow   = QColor(255, 23, 68);     // #FF1744

    pal.prongNormalMetal   = QColor(209, 199, 189);     // #D1C7BD
    pal.prongNormalTip     = QColor(255, 228, 214);     // #FFE4D6
    pal.prongSuperGold     = QColor(245, 158, 11);     // #F59E0B
    pal.prongSuperCore     = QColor(255, 247, 237);     // #FFF7ED

    pal.podStyle           = PodStyle{PodFinish::HAMMERED, PodPattern::EMBER};
    pal.prongStyle         = ProngStyle::FORGED;
    pal.boardTexture       = BoardTexture::BASALT_LAVA;
    pal.cellShape          = CellShape::SQUARE;
    pal.glowStrength       = 0.7;

    // Verdigris Malachite
    pal.podA.core      = QColor(6, 107, 78);     // #066B4E
    pal.podA.rim       = QColor(5, 150, 105);     // #059669
    pal.podA.highlight = QColor(110, 231, 183);     // #6EE7B7
    pal.podA.shadow    = QColor(2, 51, 37);     // #023325
    pal.podA.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podA.pattern   = PodPattern::EMBER;

    // Molten Crimson-Orange
    pal.podB.core      = QColor(168, 21, 32);     // #A81520
    pal.podB.rim       = QColor(220, 38, 38);     // #DC2626
    pal.podB.highlight = QColor(252, 165, 165);     // #FCA5A5
    pal.podB.shadow    = QColor(78, 5, 11);     // #4E050B
    pal.podB.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podB.pattern   = PodPattern::EMBER;

    // Cobalt Tempered Steel
    pal.podC.core      = QColor(30, 58, 138);     // #1E3A8A
    pal.podC.rim       = QColor(37, 99, 235);     // #2563EB
    pal.podC.highlight = QColor(147, 197, 253);     // #93C5FD
    pal.podC.shadow    = QColor(14, 27, 66);     // #0E1B42
    pal.podC.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podC.pattern   = PodPattern::EMBER;

    // Sulphur Yellow-Gold
    pal.podD.core      = QColor(158, 104, 0);     // #9E6800
    pal.podD.rim       = QColor(234, 179, 8);     // #EAB308
    pal.podD.highlight = QColor(254, 240, 138);     // #FEF08A
    pal.podD.shadow    = QColor(71, 47, 0);     // #472F00
    pal.podD.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podD.pattern   = PodPattern::EMBER;

    return pal;
}

OctiPalette makeCandyPopPalette()
{
    OctiPalette pal;
    pal.windowBg           = QColor(26, 16, 28);     // #1A101C
    pal.panelCardBg        = QColor(38, 23, 41);     // #261729
    pal.panelCardBorder    = QColor(255, 105, 180);     // #FF69B4
    pal.textPrimary        = QColor(255, 245, 249);     // #FFF5F9
    pal.textSecondary      = QColor(226, 184, 214);     // #E2B8D6
    pal.accentBrass        = QColor(255, 105, 180);     // #FF69B4

    pal.boardOuterFrame    = QColor(34, 18, 38);     // #221226
    pal.boardBevelLight    = QColor(255, 182, 193);     // #FFB6C1
    pal.boardBevelDark     = QColor(74, 21, 75);     // #4A154B
    pal.gridCellDark       = QColor(36, 20, 43);     // #24142B
    pal.gridCellLight      = QColor(51, 30, 61);     // #331E3D
    pal.gridSocketShadow   = QColor(18, 7, 22, 168);     // #120716A8
    pal.gridLine           = QColor(255, 182, 193, 85);     // #FFB6C155
    pal.coordLabel         = QColor(251, 207, 232);     // #FBCFE8
    pal.homeBadgeOuterBg   = QColor(27, 13, 33);     // #1B0D21
    pal.homeBadgeInnerBg   = QColor(19, 9, 23);     // #130917

    pal.neutralOctiFill    = QColor(77, 44, 84);     // #4D2C54
    pal.neutralOctiRim     = QColor(255, 128, 191);     // #FF80BF
    pal.edgelessPortalGlow = QColor(56, 189, 248);     // #38BDF8
    pal.legalStepGuide     = QColor(52, 211, 153);     // #34D399
    pal.legalJumpGuide     = QColor(251, 191, 36);     // #FBBF24
    pal.legalCaptureGlow   = QColor(255, 20, 147);     // #FF1493

    pal.prongNormalMetal   = QColor(253, 226, 228);     // #FDE2E4
    pal.prongNormalTip     = QColor(255, 255, 255);     // #FFFFFF
    pal.prongSuperGold     = QColor(253, 224, 71);     // #FDE047
    pal.prongSuperCore     = QColor(255, 251, 235);     // #FFFBEB

    pal.podStyle           = PodStyle{PodFinish::GLOSS, PodPattern::FACTION_SPECIFIC};
    pal.prongStyle         = ProngStyle::LOLLIPOP;
    pal.boardTexture       = BoardTexture::SPRINKLES;
    pal.cellShape          = CellShape::ROUNDED;
    pal.glowStrength       = 0.2;

    // Mint-Apple Green
    pal.podA.core      = QColor(5, 122, 85);     // #057A55
    pal.podA.rim       = QColor(16, 185, 129);     // #10B981
    pal.podA.highlight = QColor(167, 243, 208);     // #A7F3D0
    pal.podA.shadow    = QColor(2, 56, 39);     // #023827
    pal.podA.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podA.pattern   = PodPattern::STRIPES;

    // Strawberry-Cherry Red
    pal.podB.core      = QColor(185, 28, 71);     // #B91C47
    pal.podB.rim       = QColor(244, 63, 94);     // #F43F5E
    pal.podB.highlight = QColor(254, 205, 211);     // #FECDD3
    pal.podB.shadow    = QColor(86, 11, 31);     // #560B1F
    pal.podB.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podB.pattern   = PodPattern::DOTS;

    // Blueberry-Razz Blue
    pal.podC.core      = QColor(29, 78, 216);     // #1D4ED8
    pal.podC.rim       = QColor(59, 130, 246);     // #3B82F6
    pal.podC.highlight = QColor(191, 219, 254);     // #BFDBFE
    pal.podC.shadow    = QColor(12, 37, 112);     // #0C2570
    pal.podC.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podC.pattern   = PodPattern::CHECKS;

    // Lemon-Tangerine Gold
    pal.podD.core      = QColor(161, 98, 7);     // #A16207
    pal.podD.rim       = QColor(245, 158, 11);     // #F59E0B
    pal.podD.highlight = QColor(253, 230, 138);     // #FDE68A
    pal.podD.shadow    = QColor(71, 43, 2);     // #472B02
    pal.podD.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podD.pattern   = PodPattern::SWIRL;

    return pal;
}

OctiPalette makeOceanPearlPalette()
{
    OctiPalette pal;
    pal.windowBg           = QColor(7, 16, 24);     // #071018
    pal.panelCardBg        = QColor(13, 28, 42);     // #0D1C2A
    pal.panelCardBorder    = QColor(56, 189, 248);     // #38BDF8
    pal.textPrimary        = QColor(240, 253, 244);     // #F0FDF4
    pal.textSecondary      = QColor(125, 211, 252);     // #7DD3FC
    pal.accentBrass        = QColor(45, 212, 191);     // #2DD4BF

    pal.boardOuterFrame    = QColor(10, 22, 36);     // #0A1624
    pal.boardBevelLight    = QColor(94, 234, 212);     // #5EEAD4
    pal.boardBevelDark     = QColor(6, 40, 61);     // #06283D
    pal.gridCellDark       = QColor(13, 30, 45);     // #0D1E2D
    pal.gridCellLight      = QColor(19, 43, 64);     // #132B40
    pal.gridSocketShadow   = QColor(4, 10, 18, 188);     // #040A12BC
    pal.gridLine           = QColor(56, 189, 248, 80);     // #38BDF850
    pal.coordLabel         = QColor(103, 232, 249);     // #67E8F9
    pal.homeBadgeOuterBg   = QColor(8, 20, 31);     // #08141F
    pal.homeBadgeInnerBg   = QColor(5, 13, 21);     // #050D15

    pal.neutralOctiFill    = QColor(31, 58, 77);     // #1F3A4D
    pal.neutralOctiRim     = QColor(45, 212, 191);     // #2DD4BF
    pal.edgelessPortalGlow = QColor(34, 211, 238);     // #22D3EE
    pal.legalStepGuide     = QColor(52, 211, 153);     // #34D399
    pal.legalJumpGuide     = QColor(251, 191, 36);     // #FBBF24
    pal.legalCaptureGlow   = QColor(244, 63, 94);     // #F43F5E

    pal.prongNormalMetal   = QColor(224, 242, 254);     // #E0F2FE
    pal.prongNormalTip     = QColor(255, 255, 255);     // #FFFFFF
    pal.prongSuperGold     = QColor(253, 224, 71);     // #FDE047
    pal.prongSuperCore     = QColor(255, 251, 235);     // #FFFBEB

    pal.podStyle           = PodStyle{PodFinish::NACRE, PodPattern::SHELL};
    pal.prongStyle         = ProngStyle::CORAL;
    pal.boardTexture       = BoardTexture::CAUSTICS;
    pal.cellShape          = CellShape::ROUNDED;
    pal.glowStrength       = 0.45;

    // Seafoam Emerald
    pal.podA.core      = QColor(5, 122, 95);     // #057A5F
    pal.podA.rim       = QColor(20, 184, 166);     // #14B8A6
    pal.podA.highlight = QColor(153, 246, 228);     // #99F6E4
    pal.podA.shadow    = QColor(2, 56, 44);     // #02382C
    pal.podA.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podA.pattern   = PodPattern::SHELL;

    // Living Coral Crimson
    pal.podB.core      = QColor(184, 25, 56);     // #B81938
    pal.podB.rim       = QColor(244, 63, 94);     // #F43F5E
    pal.podB.highlight = QColor(254, 205, 211);     // #FECDD3
    pal.podB.shadow    = QColor(84, 10, 24);     // #540A18
    pal.podB.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podB.pattern   = PodPattern::SHELL;

    // Abyssal Sapphire
    pal.podC.core      = QColor(23, 67, 163);     // #1743A3
    pal.podC.rim       = QColor(59, 130, 246);     // #3B82F6
    pal.podC.highlight = QColor(191, 219, 254);     // #BFDBFE
    pal.podC.shadow    = QColor(9, 31, 82);     // #091F52
    pal.podC.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podC.pattern   = PodPattern::SHELL;

    // Sunlit Amber-Pearl
    pal.podD.core      = QColor(158, 101, 5);     // #9E6505
    pal.podD.rim       = QColor(245, 158, 11);     // #F59E0B
    pal.podD.highlight = QColor(254, 240, 138);     // #FEF08A
    pal.podD.shadow    = QColor(71, 46, 1);     // #472E01
    pal.podD.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podD.pattern   = PodPattern::SHELL;

    return pal;
}

OctiPalette makeSteampunkBrassPalette()
{
    OctiPalette pal;
    pal.windowBg           = QColor(20, 15, 10);     // #140F0A
    pal.panelCardBg        = QColor(34, 24, 16);     // #221810
    pal.panelCardBorder    = QColor(200, 138, 58);     // #C88A3A
    pal.textPrimary        = QColor(251, 244, 230);     // #FBF4E6
    pal.textSecondary      = QColor(191, 159, 128);     // #BF9F80
    pal.accentBrass        = QColor(221, 161, 94);     // #DDA15E

    pal.boardOuterFrame    = QColor(31, 20, 12);     // #1F140C
    pal.boardBevelLight    = QColor(224, 159, 62);     // #E09F3E
    pal.boardBevelDark     = QColor(82, 43, 8);     // #522B08
    pal.gridCellDark       = QColor(36, 24, 16);     // #241810
    pal.gridCellLight      = QColor(51, 34, 22);     // #332216
    pal.gridSocketShadow   = QColor(16, 9, 5, 180);     // #100905B4
    pal.gridLine           = QColor(221, 161, 94, 96);     // #DDA15E60
    pal.coordLabel         = QColor(242, 190, 116);     // #F2BE74
    pal.homeBadgeOuterBg   = QColor(24, 16, 10);     // #18100A
    pal.homeBadgeInnerBg   = QColor(16, 10, 6);     // #100A06

    pal.neutralOctiFill    = QColor(71, 50, 35);     // #473223
    pal.neutralOctiRim     = QColor(221, 161, 94);     // #DDA15E
    pal.edgelessPortalGlow = QColor(56, 189, 248);     // #38BDF8
    pal.legalStepGuide     = QColor(52, 211, 153);     // #34D399
    pal.legalJumpGuide     = QColor(251, 191, 36);     // #FBBF24
    pal.legalCaptureGlow   = QColor(244, 63, 94);     // #F43F5E

    pal.prongNormalMetal   = QColor(230, 204, 178);     // #E6CCB2
    pal.prongNormalTip     = QColor(255, 241, 230);     // #FFF1E6
    pal.prongSuperGold     = QColor(245, 158, 11);     // #F59E0B
    pal.prongSuperCore     = QColor(255, 251, 235);     // #FFFBEB

    pal.podStyle           = PodStyle{PodFinish::BRASS, PodPattern::GEARS};
    pal.prongStyle         = ProngStyle::PIPE;
    pal.boardTexture       = BoardTexture::RIVET_PLATE;
    pal.cellShape          = CellShape::SQUARE;
    pal.glowStrength       = 0.3;

    // Patina Verdigris Green
    pal.podA.core      = QColor(14, 116, 98);     // #0E7462
    pal.podA.rim       = QColor(16, 185, 129);     // #10B981
    pal.podA.highlight = QColor(167, 243, 208);     // #A7F3D0
    pal.podA.shadow    = QColor(5, 59, 50);     // #053B32
    pal.podA.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podA.pattern   = PodPattern::GEARS;

    // Oxblood Furnace Red
    pal.podB.core      = QColor(163, 24, 40);     // #A31828
    pal.podB.rim       = QColor(239, 68, 68);     // #EF4444
    pal.podB.highlight = QColor(252, 165, 165);     // #FCA5A5
    pal.podB.shadow    = QColor(71, 8, 17);     // #470811
    pal.podB.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podB.pattern   = PodPattern::GEARS;

    // Aether Prussian Blue
    pal.podC.core      = QColor(25, 61, 135);     // #193D87
    pal.podC.rim       = QColor(59, 130, 246);     // #3B82F6
    pal.podC.highlight = QColor(191, 219, 254);     // #BFDBFE
    pal.podC.shadow    = QColor(11, 27, 61);     // #0B1B3D
    pal.podC.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podC.pattern   = PodPattern::GEARS;

    // Burnished Clockwork Gold
    pal.podD.core      = QColor(156, 99, 4);     // #9C6304
    pal.podD.rim       = QColor(217, 119, 6);     // #D97706
    pal.podD.highlight = QColor(253, 230, 138);     // #FDE68A
    pal.podD.shadow    = QColor(69, 44, 1);     // #452C01
    pal.podD.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podD.pattern   = PodPattern::GEARS;

    return pal;
}

OctiPalette makeCosmicNebulaPalette()
{
    OctiPalette pal;
    pal.windowBg           = QColor(8, 7, 18);     // #080712
    pal.panelCardBg        = QColor(17, 14, 36);     // #110E24
    pal.panelCardBorder    = QColor(139, 92, 246);     // #8B5CF6
    pal.textPrimary        = QColor(248, 250, 252);     // #F8FAFC
    pal.textSecondary      = QColor(165, 180, 252);     // #A5B4FC
    pal.accentBrass        = QColor(192, 132, 252);     // #C084FC

    pal.boardOuterFrame    = QColor(14, 12, 33);     // #0E0C21
    pal.boardBevelLight    = QColor(168, 85, 247);     // #A855F7
    pal.boardBevelDark     = QColor(46, 16, 101);     // #2E1065
    pal.gridCellDark       = QColor(16, 13, 38);     // #100D26
    pal.gridCellLight      = QColor(26, 22, 59);     // #1A163B
    pal.gridSocketShadow   = QColor(5, 3, 15, 168);     // #05030FA8
    pal.gridLine           = QColor(139, 92, 246, 85);     // #8B5CF655
    pal.coordLabel         = QColor(196, 181, 253);     // #C4B5FD
    pal.homeBadgeOuterBg   = QColor(11, 8, 26);     // #0B081A
    pal.homeBadgeInnerBg   = QColor(6, 4, 16);     // #060410

    pal.neutralOctiFill    = QColor(43, 36, 86);     // #2B2456
    pal.neutralOctiRim     = QColor(192, 132, 252);     // #C084FC
    pal.edgelessPortalGlow = QColor(56, 189, 248);     // #38BDF8
    pal.legalStepGuide     = QColor(52, 211, 153);     // #34D399
    pal.legalJumpGuide     = QColor(251, 191, 36);     // #FBBF24
    pal.legalCaptureGlow   = QColor(244, 63, 94);     // #F43F5E

    pal.prongNormalMetal   = QColor(224, 231, 255);     // #E0E7FF
    pal.prongNormalTip     = QColor(255, 255, 255);     // #FFFFFF
    pal.prongSuperGold     = QColor(253, 224, 71);     // #FDE047
    pal.prongSuperCore     = QColor(255, 251, 235);     // #FFFBEB

    pal.podStyle           = PodStyle{PodFinish::PLANETARY, PodPattern::BANDS};
    pal.prongStyle         = ProngStyle::COMET;
    pal.boardTexture       = BoardTexture::STARFIELD;
    pal.cellShape          = CellShape::ROUNDED;
    pal.glowStrength       = 0.8;

    // Exoplanet Emerald
    pal.podA.core      = QColor(5, 122, 85);     // #057A55
    pal.podA.rim       = QColor(16, 185, 129);     // #10B981
    pal.podA.highlight = QColor(110, 231, 183);     // #6EE7B7
    pal.podA.shadow    = QColor(2, 56, 39);     // #023827
    pal.podA.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podA.pattern   = PodPattern::BANDS;

    // Red-Dwarf Nova Crimson
    pal.podB.core      = QColor(181, 23, 63);     // #B5173F
    pal.podB.rim       = QColor(244, 63, 94);     // #F43F5E
    pal.podB.highlight = QColor(253, 164, 175);     // #FDA4AF
    pal.podB.shadow    = QColor(84, 9, 28);     // #54091C
    pal.podB.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podB.pattern   = PodPattern::BANDS;

    // Pulsar Azure-Violet
    pal.podC.core      = QColor(67, 56, 202);     // #4338CA
    pal.podC.rim       = QColor(99, 102, 241);     // #6366F1
    pal.podC.highlight = QColor(199, 210, 254);     // #C7D2FE
    pal.podC.shadow    = QColor(30, 25, 97);     // #1E1961
    pal.podC.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podC.pattern   = PodPattern::BANDS;

    // Helios Corona Gold
    pal.podD.core      = QColor(158, 100, 4);     // #9E6404
    pal.podD.rim       = QColor(245, 158, 11);     // #F59E0B
    pal.podD.highlight = QColor(253, 230, 138);     // #FDE68A
    pal.podD.shadow    = QColor(71, 44, 0);     // #472C00
    pal.podD.emblem    = QColor(255, 255, 255);     // #FFFFFF
    pal.podD.pattern   = PodPattern::BANDS;

    return pal;
}

// Converts canonical OctiDir + boardRotationDeg into screen-space unit vector (dx, dy)
// Consistent with OctiBoardGeometry::dirUnitVectorScreen:
// ROT_0 (0):     (dx0, -dy0)
// ROT_90 (90):   (-dy0, -dx0)   [90 deg CCW so Seat B West is at bottom]
// ROT_180 (180): (-dx0, dy0)    [180 deg so North is at bottom]
// ROT_270 (270): (dy0, dx0)     [270 deg CCW / 90 deg CW so Seat D East is at bottom]
QPointF screenUnitForDir(OctiDir dir, int boardRotationDeg)
{
    double bx = static_cast<double>(dirDeltaX(dir));
    double by = static_cast<double>(dirDeltaY(dir));
    double sx0 = bx;
    double sy0 = -by;
    double len = std::hypot(sx0, sy0);
    if (len > 1e-9) {
        sx0 /= len;
        sy0 /= len;
    }

    int normRot = ((boardRotationDeg % 360) + 360) % 360;
    if (normRot == 90) {
        return QPointF(sy0, -sx0);
    }
    if (normRot == 180) {
        return QPointF(-sx0, -sy0);
    }
    if (normRot == 270) {
        return QPointF(-sy0, sx0);
    }
    return QPointF(sx0, sy0);
}

QPolygonF regularOctagon(const QPointF &center, double radius)
{
    QPolygonF poly;
    poly.reserve(8);
    for (int i = 0; i < 8; ++i) {
        double angle = (static_cast<double>(i) * 45.0 + 22.5) * PI_VAL / 180.0;
        poly.append(QPointF(center.x() + radius * std::cos(angle),
                            center.y() + radius * std::sin(angle)));
    }
    return poly;
}

} // namespace

const PodFactionPalette &OctiPalette::factionForPodType(OctiPodType type) const
{
    switch (type) {
    case A_POD: return podA;
    case B_POD: return podB;
    case C_POD: return podC;
    case D_POD: return podD;
    default:    return podA;
    }
}

const PodFactionPalette &OctiPalette::factionForPlayer(OctiPlayer player) const
{
    return factionForPodType(playerToPodType(player));
}

const char *themeIdToConfigKey(ThemeId id)
{
    switch (id) {
    case ThemeId::OBSIDIAN_BRASS:  return "obsidian_brass";
    case ThemeId::ROSEWOOD_IVORY:  return "rosewood_ivory";
    case ThemeId::NEON_CIRCUIT:    return "neon_circuit";
    case ThemeId::JADE_DYNASTY:    return "jade_dynasty";
    case ThemeId::ARCTIC_AURORA:   return "arctic_aurora";
    case ThemeId::VOLCANIC_FORGE:  return "volcanic_forge";
    case ThemeId::CANDY_POP:       return "candy_pop";
    case ThemeId::OCEAN_PEARL:     return "ocean_pearl";
    case ThemeId::STEAMPUNK_BRASS: return "steampunk_brass";
    case ThemeId::COSMIC_NEBULA:   return "cosmic_nebula";
    default:                       return "obsidian_brass";
    }
}

const char *themeIdToWebKey(ThemeId id)
{
    switch (id) {
    case ThemeId::OBSIDIAN_BRASS:  return "obsidian";
    case ThemeId::ROSEWOOD_IVORY:  return "rosewood";
    case ThemeId::NEON_CIRCUIT:    return "neon";
    case ThemeId::JADE_DYNASTY:    return "jade";
    case ThemeId::ARCTIC_AURORA:   return "aurora";
    case ThemeId::VOLCANIC_FORGE:  return "forge";
    case ThemeId::CANDY_POP:       return "candy";
    case ThemeId::OCEAN_PEARL:     return "pearl";
    case ThemeId::STEAMPUNK_BRASS: return "steam";
    case ThemeId::COSMIC_NEBULA:   return "nebula";
    default:                       return "obsidian";
    }
}

QString themeDisplayName(ThemeId id)
{
    switch (id) {
    case ThemeId::OBSIDIAN_BRASS:  return QStringLiteral("Obsidian & Brass");
    case ThemeId::ROSEWOOD_IVORY:  return QStringLiteral("Rosewood & Ivory");
    case ThemeId::NEON_CIRCUIT:    return QStringLiteral("Neon Circuit");
    case ThemeId::JADE_DYNASTY:    return QStringLiteral("Jade Dynasty");
    case ThemeId::ARCTIC_AURORA:   return QStringLiteral("Arctic Aurora");
    case ThemeId::VOLCANIC_FORGE:  return QStringLiteral("Volcanic Forge");
    case ThemeId::CANDY_POP:       return QStringLiteral("Candy Pop");
    case ThemeId::OCEAN_PEARL:     return QStringLiteral("Ocean Pearl");
    case ThemeId::STEAMPUNK_BRASS: return QStringLiteral("Steampunk Brass");
    case ThemeId::COSMIC_NEBULA:   return QStringLiteral("Cosmic Nebula");
    default:                       return QStringLiteral("Obsidian & Brass");
    }
}

const OctiPalette &paletteForTheme(ThemeId id)
{
    static const OctiPalette obsidian = makeObsidianBrassPalette();
    static const OctiPalette rosewood = makeRosewoodIvoryPalette();
    static const OctiPalette neon     = makeNeonCircuitPalette();
    static const OctiPalette jade     = makeJadeDynastyPalette();
    static const OctiPalette aurora   = makeArcticAuroraPalette();
    static const OctiPalette forge    = makeVolcanicForgePalette();
    static const OctiPalette candy    = makeCandyPopPalette();
    static const OctiPalette pearl    = makeOceanPearlPalette();
    static const OctiPalette steam    = makeSteampunkBrassPalette();
    static const OctiPalette nebula   = makeCosmicNebulaPalette();

    switch (id) {
    case ThemeId::OBSIDIAN_BRASS:  return obsidian;
    case ThemeId::ROSEWOOD_IVORY:  return rosewood;
    case ThemeId::NEON_CIRCUIT:    return neon;
    case ThemeId::JADE_DYNASTY:    return jade;
    case ThemeId::ARCTIC_AURORA:   return aurora;
    case ThemeId::VOLCANIC_FORGE:  return forge;
    case ThemeId::CANDY_POP:       return candy;
    case ThemeId::OCEAN_PEARL:     return pearl;
    case ThemeId::STEAMPUNK_BRASS: return steam;
    case ThemeId::COSMIC_NEBULA:   return nebula;
    default:                       return obsidian;
    }
}

QString buildAppStyleSheet(ThemeId id)
{
    const OctiPalette &p = paletteForTheme(id);
    return QString(
        "QMainWindow, QWidget { background-color: %1; color: %2; font-family: 'Segoe UI', 'DejaVu Sans', sans-serif; }\n"
        "QDialog { background-color: %3; color: %2; border: 1px solid %4; }\n"
        "QFrame { background-color: %3; border: 1px solid %4; border-radius: 6px; }\n"
        "QGroupBox { background-color: %3; border: 1px solid %4; border-radius: 6px; margin-top: 8px; padding-top: 12px; font-weight: bold; }\n"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 6px; color: %5; }\n"
        "QPushButton { background-color: %3; color: %2; border: 1px solid %5; border-radius: 5px; padding: 5px 10px; font-weight: bold; }\n"
        "QPushButton:hover { background-color: %5; color: %1; }\n"
        "QPushButton:disabled { color: %6; border-color: %6; }\n"
        "QLineEdit, QComboBox, QSpinBox, QTextEdit, QListWidget, QTableWidget { background-color: %1; color: %2; border: 1px solid %4; border-radius: 4px; padding: 4px; }\n"
        "QTableWidget, QListWidget { selection-background-color: %5; selection-color: %1; outline: 0; }\n"
        "QTableWidget::item:selected, QTableWidget::item:selected:!active, QListWidget::item:selected, QListWidget::item:selected:!active { background-color: %5; color: %1; font-weight: bold; }\n"
        "QLabel { background: transparent; color: %2; border: none; }\n"
        "QCheckBox { color: %2; spacing: 6px; }\n"
        "QProgressBar { background-color: %1; color: %2; border: 1px solid %4; border-radius: 4px; text-align: center; font-weight: bold; }\n"
        "QProgressBar::chunk { background-color: %5; border-radius: 3px; }\n"
    )
    .arg(p.windowBg.name())
    .arg(p.textPrimary.name())
    .arg(p.panelCardBg.name())
    .arg(p.panelCardBorder.name())
    .arg(p.accentBrass.name())
    .arg(p.textSecondary.name());
}

// Bounded Procedural Pod Pattern Cache
struct PodPatternKey {
    int pattern = 0;
    int size = 0;
    QRgb highlight = 0;
    QRgb shadow = 0;
    QRgb rim = 0;

    bool operator==(const PodPatternKey &o) const {
        return pattern == o.pattern && size == o.size &&
               highlight == o.highlight && shadow == o.shadow && rim == o.rim;
    }
};

struct PodPatternKeyHash {
    std::size_t operator()(const PodPatternKey &k) const {
        std::size_t h = std::hash<int>()(k.pattern);
        h ^= std::hash<int>()(k.size) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<unsigned int>()(k.highlight) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<unsigned int>()(k.shadow) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<unsigned int>()(k.rim) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

static QPixmap renderPodPatternTile(PodPattern pat, int size,
                                   const QColor &highlight,
                                   const QColor &shadow,
                                   const QColor &rim)
{
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    double s = static_cast<double>(size);
    QPointF c(s * 0.5, s * 0.5);

    switch (pat) {
    case PodPattern::CIRCUIT: {
        QPen pen(highlight, std::max(1.2, s * 0.04));
        p.setPen(pen);
        p.drawLine(QPointF(s * 0.15, s * 0.35), QPointF(s * 0.45, s * 0.35));
        p.drawLine(QPointF(s * 0.45, s * 0.35), QPointF(s * 0.65, s * 0.15));
        p.drawLine(QPointF(s * 0.35, s * 0.85), QPointF(s * 0.35, s * 0.60));
        p.drawLine(QPointF(s * 0.35, s * 0.60), QPointF(s * 0.70, s * 0.60));
        p.drawLine(QPointF(s * 0.70, s * 0.60), QPointF(s * 0.85, s * 0.45));
        p.drawLine(QPointF(s * 0.15, s * 0.75), QPointF(s * 0.25, s * 0.75));

        p.setPen(QPen(rim, 1.0));
        p.setBrush(highlight);
        double padR = std::max(1.8, s * 0.05);
        p.drawEllipse(QPointF(s * 0.15, s * 0.35), padR, padR);
        p.drawEllipse(QPointF(s * 0.65, s * 0.15), padR, padR);
        p.drawEllipse(QPointF(s * 0.35, s * 0.85), padR, padR);
        p.drawEllipse(QPointF(s * 0.85, s * 0.45), padR, padR);

        p.setPen(Qt::NoPen);
        p.setBrush(shadow);
        double holeR = padR * 0.4;
        p.drawEllipse(QPointF(s * 0.15, s * 0.35), holeR, holeR);
        p.drawEllipse(QPointF(s * 0.65, s * 0.15), holeR, holeR);
        p.drawEllipse(QPointF(s * 0.35, s * 0.85), holeR, holeR);
        p.drawEllipse(QPointF(s * 0.85, s * 0.45), holeR, holeR);
        break;
    }
    case PodPattern::CLOUD_SCROLL: {
        p.setPen(QPen(highlight, std::max(1.2, s * 0.045)));
        p.setBrush(Qt::NoBrush);
        QRectF arc1(s * 0.15, s * 0.2, s * 0.35, s * 0.35);
        QRectF arc2(s * 0.50, s * 0.45, s * 0.35, s * 0.35);
        QRectF arc3(s * 0.20, s * 0.55, s * 0.30, s * 0.30);
        p.drawArc(arc1, 30 * 16, 270 * 16);
        p.drawArc(arc2, 180 * 16, 260 * 16);
        p.drawArc(arc3, 0 * 16, 240 * 16);
        p.drawLine(QPointF(s * 0.325, s * 0.55), QPointF(s * 0.50, s * 0.625));
        break;
    }
    case PodPattern::FACET: {
        p.setPen(QPen(highlight, std::max(1.0, s * 0.035)));
        p.drawLine(c, QPointF(0, 0));
        p.drawLine(c, QPointF(s, 0));
        p.drawLine(c, QPointF(s, s));
        p.drawLine(c, QPointF(0, s));
        p.drawLine(c, QPointF(s * 0.5, 0));
        p.drawLine(c, QPointF(s * 0.5, s));
        p.drawLine(c, QPointF(0, s * 0.5));
        p.drawLine(c, QPointF(s, s * 0.5));

        QColor f1 = highlight; f1.setAlpha(60);
        QColor f2 = shadow; f2.setAlpha(60);
        p.setPen(Qt::NoPen);
        p.setBrush(f1);
        QPolygonF tri1; tri1 << c << QPointF(0, 0) << QPointF(s * 0.5, 0);
        QPolygonF tri2; tri2 << c << QPointF(s, s * 0.5) << QPointF(s, s);
        p.drawPolygon(tri1);
        p.drawPolygon(tri2);
        p.setBrush(f2);
        QPolygonF tri3; tri3 << c << QPointF(s * 0.5, 0) << QPointF(s, 0);
        QPolygonF tri4; tri4 << c << QPointF(0, s * 0.5) << QPointF(0, s);
        p.drawPolygon(tri3);
        p.drawPolygon(tri4);
        break;
    }
    case PodPattern::EMBER: {
        p.setPen(Qt::NoPen);
        QColor dimple = shadow; dimple.setAlpha(90);
        p.setBrush(dimple);
        p.drawEllipse(QPointF(s * 0.3, s * 0.35), s * 0.12, s * 0.10);
        p.drawEllipse(QPointF(s * 0.7, s * 0.4), s * 0.14, s * 0.11);
        p.drawEllipse(QPointF(s * 0.45, s * 0.7), s * 0.13, s * 0.12);

        QColor ember = highlight; ember.setAlpha(180);
        p.setBrush(ember);
        p.drawEllipse(QPointF(s * 0.28, s * 0.33), s * 0.04, s * 0.04);
        p.drawEllipse(QPointF(s * 0.68, s * 0.38), s * 0.05, s * 0.05);
        p.drawEllipse(QPointF(s * 0.43, s * 0.68), s * 0.04, s * 0.04);
        p.drawEllipse(QPointF(s * 0.8, s * 0.75), s * 0.035, s * 0.035);
        p.drawEllipse(QPointF(s * 0.2, s * 0.75), s * 0.035, s * 0.035);
        break;
    }
    case PodPattern::STRIPES: {
        p.setPen(Qt::NoPen);
        QColor strCol = highlight; strCol.setAlpha(140);
        p.setBrush(strCol);
        double step = s * 0.25;
        double w = step * 0.45;
        for (double d = -s; d < s * 2; d += step) {
            QPolygonF stripe;
            stripe << QPointF(d, 0) << QPointF(d + w, 0)
                   << QPointF(d + w + s, s) << QPointF(d + s, s);
            p.drawPolygon(stripe);
        }
        break;
    }
    case PodPattern::DOTS: {
        p.setPen(Qt::NoPen);
        QColor dotCol = highlight; dotCol.setAlpha(160);
        p.setBrush(dotCol);
        double r = std::max(2.0, s * 0.07);
        double step = s * 0.28;
        for (double y = s * 0.15; y < s; y += step) {
            for (double x = s * 0.15; x < s; x += step) {
                p.drawEllipse(QPointF(x, y), r, r);
            }
        }
        for (double y = s * 0.15 + step * 0.5; y < s; y += step) {
            for (double x = s * 0.15 + step * 0.5; x < s; x += step) {
                p.drawEllipse(QPointF(x, y), r * 0.8, r * 0.8);
            }
        }
        break;
    }
    case PodPattern::CHECKS: {
        p.setPen(Qt::NoPen);
        QColor chkCol = highlight; chkCol.setAlpha(110);
        p.setBrush(chkCol);
        double step = s * 0.22;
        int count = 6;
        for (int i = 0; i < count; ++i) {
            for (int j = 0; j < count; ++j) {
                if ((i + j) % 2 == 0) {
                    p.fillRect(QRectF(i * step, j * step, step, step), chkCol);
                }
            }
        }
        break;
    }
    case PodPattern::SWIRL: {
        p.setPen(Qt::NoPen);
        QColor swirlCol = highlight; swirlCol.setAlpha(130);
        p.setBrush(swirlCol);
        int petals = 6;
        for (int i = 0; i < petals; ++i) {
            double a0 = (i * 2.0 * M_PI) / petals;
            double a1 = a0 + (M_PI / petals);
            QPainterPath path;
            path.moveTo(c);
            path.quadTo(c + QPointF(std::cos(a0 + 0.3) * s * 0.35, std::sin(a0 + 0.3) * s * 0.35),
                        c + QPointF(std::cos(a0) * s * 0.5, std::sin(a0) * s * 0.5));
            path.arcTo(QRectF(0, 0, s, s), -a0 * 180.0 / M_PI, -(a1 - a0) * 180.0 / M_PI);
            path.quadTo(c + QPointF(std::cos(a1 + 0.3) * s * 0.35, std::sin(a1 + 0.3) * s * 0.35), c);
            p.drawPath(path);
        }
        break;
    }
    case PodPattern::SHELL: {
        p.setPen(QPen(highlight, std::max(1.2, s * 0.035)));
        p.setBrush(Qt::NoBrush);
        QPointF origin(s * 0.5, s * 0.9);
        int rays = 9;
        for (int i = 0; i < rays; ++i) {
            double angle = M_PI * (1.1 + (0.8 * i) / (rays - 1));
            QPointF end(origin.x() + std::cos(angle) * s * 0.8,
                        origin.y() + std::sin(angle) * s * 0.8);
            p.drawLine(origin, end);
        }
        p.drawArc(QRectF(s * 0.2, s * 0.2, s * 0.6, s * 0.7), 30 * 16, 120 * 16);
        p.drawArc(QRectF(s * 0.3, s * 0.4, s * 0.4, s * 0.5), 30 * 16, 120 * 16);
        break;
    }
    case PodPattern::GEARS: {
        p.setPen(QPen(highlight, std::max(1.3, s * 0.045)));
        p.setBrush(Qt::NoBrush);
        double gr = s * 0.35;
        p.drawEllipse(c, gr, gr);
        p.drawEllipse(c, gr * 0.65, gr * 0.65);
        int teeth = 8;
        for (int i = 0; i < teeth; ++i) {
            double a = (i * 2.0 * M_PI) / teeth;
            QPointF t0 = c + QPointF(std::cos(a) * (gr - s * 0.04), std::sin(a) * (gr - s * 0.04));
            QPointF t1 = c + QPointF(std::cos(a) * (gr + s * 0.06), std::sin(a) * (gr + s * 0.06));
            p.drawLine(t0, t1);
        }
        p.setPen(Qt::NoPen);
        p.setBrush(rim);
        for (int i = 0; i < teeth; ++i) {
            double a = ((i + 0.5) * 2.0 * M_PI) / teeth;
            QPointF rp = c + QPointF(std::cos(a) * gr * 0.82, std::sin(a) * gr * 0.82);
            p.drawEllipse(rp, std::max(1.5, s * 0.03), std::max(1.5, s * 0.03));
        }
        break;
    }
    case PodPattern::BANDS: {
        p.setPen(Qt::NoPen);
        QColor b1 = highlight; b1.setAlpha(90);
        QColor b2 = shadow; b2.setAlpha(90);
        p.fillRect(QRectF(0, s * 0.22, s, s * 0.12), b1);
        p.fillRect(QRectF(0, s * 0.42, s, s * 0.16), b2);
        p.fillRect(QRectF(0, s * 0.66, s, s * 0.10), b1);
        p.setBrush(rim);
        p.drawEllipse(QPointF(s * 0.65, s * 0.50), s * 0.14, s * 0.07);
        break;
    }
    case PodPattern::NONE:
    default:
        break;
    }
    p.end();
    return pix;
}

static QPixmap getCachedPodPatternTile(PodPattern pat, int size,
                                       const QColor &highlight,
                                       const QColor &shadow,
                                       const QColor &rim)
{
    static std::unordered_map<PodPatternKey, QPixmap, PodPatternKeyHash> s_cache;
    static std::vector<PodPatternKey> s_order;

    PodPatternKey key{static_cast<int>(pat), size, highlight.rgba(), shadow.rgba(), rim.rgba()};
    auto it = s_cache.find(key);
    if (it != s_cache.end()) {
        return it->second;
    }

    QPixmap pix = renderPodPatternTile(pat, size, highlight, shadow, rim);
    if (s_cache.size() >= 64) {
        if (!s_order.empty()) {
            s_cache.erase(s_order.front());
            s_order.erase(s_order.begin());
        }
    }
    s_cache[key] = pix;
    s_order.push_back(key);
    return pix;
}


void paintOctagonalPod(QPainter &p,
                       const QRectF &rect,
                       OctiPodType type,
                       const OctiPod &pod,
                       const OctiPalette &pal,
                       int boardRotationDeg,
                       bool highlighted,
                       double opacity,
                       const QString &centerLabelOverride)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setOpacity(opacity);

    const PodFactionPalette &fp = pal.factionForPodType(type);
    QPointF center = rect.center();
    double radius = std::min(rect.width(), rect.height()) * 0.36;

    // 1. Draw prongs extending from pod body out to socket perimeter
    double prongInnerR = radius * 0.65;
    double prongOuterR = radius * 1.46;
    double superOuterR = radius * 1.56;

    for (int d = DIR_N; d <= DIR_NW; ++d) {
        OctiDir dir = static_cast<OctiDir>(d);
        OctiProngType pt = pod.prongAt(dir);
        if (pt == NO_PRONG) {
            continue;
        }

        QPointF u = screenUnitForDir(dir, boardRotationDeg);
        QPointF perp(-u.y(), u.x());

        if (pal.prongStyle == ProngStyle::CLASSIC) {
            if (pt == NORMAL_PRONG) {
                double halfW = std::max(2.8, radius * 0.20);
                QPolygonF shaft;
                shaft << (center + u * prongInnerR - perp * halfW)
                      << (center + u * (prongOuterR * 0.86) - perp * (halfW * 0.85))
                      << (center + u * prongOuterR)
                      << (center + u * (prongOuterR * 0.86) + perp * (halfW * 0.85))
                      << (center + u * prongInnerR + perp * halfW);

                QLinearGradient grad(center + u * prongOuterR, center + u * prongInnerR);
                grad.setColorAt(0.0, QColor(255, 255, 255));
                grad.setColorAt(0.45, pal.prongNormalTip);
                grad.setColorAt(1.0, pal.prongNormalMetal);

                p.setPen(QPen(QColor(15, 20, 28, 240), std::max(1.3, radius * 0.055)));
                p.setBrush(grad);
                p.drawPolygon(shaft);

                // Specular ridge line along the shaft
                p.setPen(QPen(QColor(255, 255, 255, 230), std::max(1.0, radius * 0.04)));
                p.drawLine(center + u * (prongInnerR + radius * 0.08), center + u * (prongOuterR * 0.92));
            } else if (pt == SUPER_PRONG) {
                double halfW = std::max(3.6, radius * 0.26);
                QPolygonF crown;
                crown << (center + u * prongInnerR - perp * (halfW * 0.75))
                      << (center + u * (superOuterR * 0.72) - perp * (halfW * 1.25))
                      << (center + u * superOuterR)
                      << (center + u * (superOuterR * 0.72) + perp * (halfW * 1.25))
                      << (center + u * prongInnerR + perp * (halfW * 0.75));

                QRadialGradient sgrad(center + u * (superOuterR * 0.78), radius * 0.5);
                sgrad.setColorAt(0.0, pal.prongSuperCore);
                sgrad.setColorAt(0.55, pal.prongSuperGold);
                sgrad.setColorAt(1.0, QColor(217, 119, 6));

                p.setPen(QPen(pal.prongSuperCore, std::max(1.4, radius * 0.07)));
                p.setBrush(sgrad);
                p.drawPolygon(crown);
            }
        } else {
            // Non-classic prong styles
            double halfW = std::max(2.8, radius * 0.20);
            if (pt == NORMAL_PRONG) {
                switch (pal.prongStyle) {
                case ProngStyle::NEON_TUBE: {
                    p.setPen(QPen(pal.accentBrass, halfW * 2.2, Qt::SolidLine, Qt::RoundCap));
                    p.drawLine(center + u * (prongInnerR + radius * 0.04), center + u * (prongOuterR * 0.95));
                    p.setPen(QPen(pal.prongNormalTip, halfW * 0.7, Qt::SolidLine, Qt::RoundCap));
                    p.drawLine(center + u * (prongInnerR + radius * 0.06), center + u * (prongOuterR * 0.92));
                    break;
                }
                case ProngStyle::FILIGREE: {
                    QPolygonF shaft;
                    shaft << (center + u * prongInnerR - perp * halfW)
                          << (center + u * (prongOuterR * 0.86) - perp * (halfW * 0.85))
                          << (center + u * prongOuterR)
                          << (center + u * (prongOuterR * 0.86) + perp * (halfW * 0.85))
                          << (center + u * prongInnerR + perp * halfW);
                    QLinearGradient grad(center + u * prongOuterR, center + u * prongInnerR);
                    grad.setColorAt(0.0, pal.prongNormalTip);
                    grad.setColorAt(0.5, pal.prongNormalMetal);
                    grad.setColorAt(1.0, pal.accentBrass);
                    p.setPen(QPen(pal.accentBrass, std::max(1.2, radius * 0.05)));
                    p.setBrush(grad);
                    p.drawPolygon(shaft);
                    p.setPen(QPen(pal.prongSuperGold, std::max(1.3, radius * 0.055)));
                    p.drawLine(center + u * (prongOuterR * 0.60) - perp * (halfW * 0.9),
                               center + u * (prongOuterR * 0.60) + perp * (halfW * 0.9));
                    p.drawLine(center + u * (prongOuterR * 0.78) - perp * (halfW * 0.6),
                               center + u * (prongOuterR * 0.78) + perp * (halfW * 0.6));
                    break;
                }
                case ProngStyle::ICICLE: {
                    QPolygonF icicle;
                    icicle << (center + u * prongInnerR - perp * (halfW * 0.8))
                           << (center + u * (prongOuterR * 0.55) - perp * (halfW * 0.45))
                           << (center + u * (prongOuterR * 1.05))
                           << (center + u * (prongOuterR * 0.55) + perp * (halfW * 0.45))
                           << (center + u * prongInnerR + perp * (halfW * 0.8));
                    QLinearGradient grad(center + u * (prongOuterR * 1.05), center + u * prongInnerR);
                    grad.setColorAt(0.0, pal.prongNormalTip);
                    grad.setColorAt(0.5, pal.prongNormalMetal);
                    grad.setColorAt(1.0, QColor(20, 35, 60));
                    p.setPen(QPen(QColor(220, 245, 255), std::max(1.0, radius * 0.04)));
                    p.setBrush(grad);
                    p.drawPolygon(icicle);
                    break;
                }
                case ProngStyle::FORGED: {
                    QPolygonF shaft;
                    shaft << (center + u * prongInnerR - perp * halfW)
                          << (center + u * (prongOuterR * 0.86) - perp * (halfW * 0.85))
                          << (center + u * prongOuterR)
                          << (center + u * (prongOuterR * 0.86) + perp * (halfW * 0.85))
                          << (center + u * prongInnerR + perp * halfW);
                    QLinearGradient grad(center + u * prongOuterR, center + u * prongInnerR);
                    grad.setColorAt(0.0, pal.prongNormalTip);
                    grad.setColorAt(0.35, QColor(249, 115, 22));
                    grad.setColorAt(0.70, QColor(180, 50, 15));
                    grad.setColorAt(1.0, QColor(35, 25, 20));
                    p.setPen(QPen(QColor(20, 15, 12), std::max(1.3, radius * 0.055)));
                    p.setBrush(grad);
                    p.drawPolygon(shaft);
                    break;
                }
                case ProngStyle::LOLLIPOP: {
                    QPolygonF shaft;
                    shaft << (center + u * prongInnerR - perp * halfW)
                          << (center + u * (prongOuterR * 0.86) - perp * (halfW * 0.85))
                          << (center + u * prongOuterR)
                          << (center + u * (prongOuterR * 0.86) + perp * (halfW * 0.85))
                          << (center + u * prongInnerR + perp * halfW);
                    p.setPen(QPen(QColor(120, 40, 80), std::max(1.2, radius * 0.05)));
                    p.setBrush(pal.prongNormalTip);
                    p.drawPolygon(shaft);
                    p.setPen(QPen(pal.accentBrass, std::max(1.6, radius * 0.065)));
                    p.drawLine(center + u * (prongOuterR * 0.50) - perp * halfW,
                               center + u * (prongOuterR * 0.62) + perp * halfW);
                    p.drawLine(center + u * (prongOuterR * 0.70) - perp * (halfW * 0.8),
                               center + u * (prongOuterR * 0.82) + perp * (halfW * 0.8));
                    break;
                }
                case ProngStyle::CORAL: {
                    QPolygonF shaft;
                    shaft << (center + u * prongInnerR - perp * (halfW * 0.6))
                          << (center + u * prongOuterR)
                          << (center + u * prongInnerR + perp * (halfW * 0.6));
                    p.setPen(QPen(pal.accentBrass, std::max(1.2, radius * 0.05)));
                    p.setBrush(pal.prongNormalMetal);
                    p.drawPolygon(shaft);
                    p.setPen(QPen(pal.accentBrass, 1.0));
                    p.setBrush(pal.prongNormalTip);
                    p.drawEllipse(center + u * (prongOuterR * 0.60), halfW * 0.75, halfW * 0.75);
                    p.drawEllipse(center + u * (prongOuterR * 0.90), halfW * 0.65, halfW * 0.65);
                    break;
                }
                case ProngStyle::PIPE: {
                    QPolygonF pipe;
                    pipe << (center + u * prongInnerR - perp * (halfW * 0.9))
                         << (center + u * (prongOuterR * 0.88) - perp * (halfW * 0.9))
                         << (center + u * (prongOuterR * 0.88) + perp * (halfW * 0.9))
                         << (center + u * prongInnerR + perp * (halfW * 0.9));
                    QLinearGradient pgrad(center + u * prongInnerR - perp * halfW,
                                          center + u * prongInnerR + perp * halfW);
                    pgrad.setColorAt(0.0, pal.prongNormalMetal.darker(140));
                    pgrad.setColorAt(0.5, pal.prongNormalTip);
                    pgrad.setColorAt(1.0, pal.prongNormalMetal.darker(140));
                    p.setPen(QPen(QColor(40, 25, 15), std::max(1.2, radius * 0.05)));
                    p.setBrush(pgrad);
                    p.drawPolygon(pipe);
                    QPolygonF collar;
                    collar << (center + u * (prongOuterR * 0.88) - perp * (halfW * 1.3))
                           << (center + u * prongOuterR - perp * (halfW * 1.3))
                           << (center + u * prongOuterR + perp * (halfW * 1.3))
                           << (center + u * (prongOuterR * 0.88) + perp * (halfW * 1.3));
                    p.setBrush(pal.accentBrass);
                    p.drawPolygon(collar);
                    break;
                }
                case ProngStyle::COMET: {
                    QPainterPath tail;
                    tail.moveTo(center + u * prongInnerR);
                    tail.lineTo(center + u * (prongOuterR * 0.85) - perp * halfW);
                    tail.lineTo(center + u * (prongOuterR * 0.85) + perp * halfW);
                    tail.closeSubpath();
                    QLinearGradient tgrad(center + u * prongOuterR, center + u * prongInnerR);
                    tgrad.setColorAt(0.0, pal.accentBrass);
                    tgrad.setColorAt(1.0, QColor(0, 0, 0, 0));
                    p.setPen(Qt::NoPen);
                    p.setBrush(tgrad);
                    p.drawPath(tail);
                    p.setPen(QPen(pal.prongSuperGold, 1.2));
                    p.setBrush(pal.prongNormalTip);
                    p.drawEllipse(center + u * (prongOuterR * 0.88), halfW * 0.95, halfW * 0.95);
                    break;
                }
                case ProngStyle::CLASSIC:
                default:
                    break;
                }
            } else if (pt == SUPER_PRONG) {
                double sHalfW = std::max(3.6, radius * 0.26);
                QPolygonF crown;
                crown << (center + u * prongInnerR - perp * (sHalfW * 0.75))
                      << (center + u * (superOuterR * 0.72) - perp * (sHalfW * 1.25))
                      << (center + u * superOuterR)
                      << (center + u * (superOuterR * 0.72) + perp * (sHalfW * 1.25))
                      << (center + u * prongInnerR + perp * (sHalfW * 0.75));

                QRadialGradient sgrad(center + u * (superOuterR * 0.78), radius * 0.5);
                sgrad.setColorAt(0.0, pal.prongSuperCore);
                sgrad.setColorAt(0.55, pal.prongSuperGold);
                sgrad.setColorAt(1.0, pal.accentBrass.darker(140));

                p.setPen(QPen(pal.prongSuperCore, std::max(1.4, radius * 0.07)));
                p.setBrush(sgrad);
                p.drawPolygon(crown);
            }
        }
    }

    // 2. Drop shadow beneath octagonal pod
    QPolygonF shadowOct = regularOctagon(center + QPointF(radius * 0.07, radius * 0.10), radius * 1.02);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 125));
    p.drawPolygon(shadowOct);

    // 3. Outer bevel octagon
    QPolygonF outerOct = regularOctagon(center, radius);
    QLinearGradient rimGrad(rect.topLeft(), rect.bottomRight());
    rimGrad.setColorAt(0.0, fp.highlight);
    rimGrad.setColorAt(0.45, fp.rim);
    rimGrad.setColorAt(1.0, fp.shadow);

    QPen outerPen(highlighted ? pal.prongSuperGold : pal.accentBrass,
                  highlighted ? std::max(2.2, radius * 0.11) : std::max(1.2, radius * 0.06));
    p.setPen(outerPen);
    p.setBrush(rimGrad);
    p.drawPolygon(outerOct);

    // 4. Inner gem dome octagon
    QPolygonF innerOct = regularOctagon(center, radius * 0.78);
    QRadialGradient domeGrad(center - QPointF(radius * 0.22, radius * 0.22), radius * 0.95);
    domeGrad.setColorAt(0.0, fp.highlight);
    domeGrad.setColorAt(0.50, fp.core);
    domeGrad.setColorAt(1.0, fp.shadow);

    p.setPen(QPen(fp.highlight.lighter(115), std::max(0.8, radius * 0.04)));
    p.setBrush(domeGrad);
    p.drawPolygon(innerOct);

    bool isClassicDome = (pal.podStyle.finish == PodFinish::CLASSIC &&
                          fp.pattern == PodPattern::NONE &&
                          pal.podStyle.pattern == PodPattern::NONE);
    if (!isClassicDome) {
        PodPattern pat = (fp.pattern != PodPattern::NONE) ? fp.pattern : pal.podStyle.pattern;
        int tileSizePx = std::clamp(((static_cast<int>(radius * 1.6) + 7) / 8) * 8, 32, 128);

        if (pat != PodPattern::NONE && pat != PodPattern::FACTION_SPECIFIC) {
            p.save();
            QPainterPath clip;
            clip.addPolygon(innerOct);
            p.setClipPath(clip);
            QPixmap tile = getCachedPodPatternTile(pat, tileSizePx, fp.highlight, fp.shadow, fp.rim);
            QRectF domeRect = innerOct.boundingRect();
            p.setOpacity(opacity * 0.45);
            p.drawPixmap(domeRect.toRect(), tile);
            p.restore();
        }

        switch (pal.podStyle.finish) {
        case PodFinish::GLASS: {
            p.setPen(QPen(QColor(255, 255, 255, 175), std::max(1.0, radius * 0.045)));
            p.setBrush(Qt::NoBrush);
            p.drawPolygon(innerOct);
            QLinearGradient arcGrad(center - QPointF(0, radius * 0.6), center);
            arcGrad.setColorAt(0.0, QColor(255, 255, 255, 160));
            arcGrad.setColorAt(1.0, QColor(255, 255, 255, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(arcGrad);
            p.drawEllipse(center - QPointF(0, radius * 0.35), radius * 0.45, radius * 0.18);
            break;
        }
        case PodFinish::CARVED: {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(fp.highlight, std::max(1.1, radius * 0.04)));
            for (int i = 0; i < 4; ++i) {
                p.drawLine(innerOct[i], innerOct[(i + 1) % 8]);
            }
            p.setPen(QPen(fp.shadow.darker(150), std::max(1.1, radius * 0.04)));
            for (int i = 4; i < 8; ++i) {
                p.drawLine(innerOct[i], innerOct[(i + 1) % 8]);
            }
            break;
        }
        case PodFinish::HAMMERED: {
            p.setPen(QPen(pal.accentBrass, std::max(1.2, radius * 0.05)));
            p.setBrush(Qt::NoBrush);
            p.drawPolygon(innerOct);
            break;
        }
        case PodFinish::GLOSS: {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 255, 255, 210));
            p.drawEllipse(center - QPointF(radius * 0.22, radius * 0.28), radius * 0.22, radius * 0.12);
            break;
        }
        case PodFinish::NACRE: {
            QLinearGradient sheen(center - QPointF(radius * 0.6, radius * 0.6),
                                  center + QPointF(radius * 0.6, radius * 0.6));
            sheen.setColorAt(0.0, QColor(140, 240, 255, 75));
            sheen.setColorAt(0.5, QColor(255, 255, 255, 110));
            sheen.setColorAt(1.0, QColor(255, 180, 225, 75));
            p.setPen(Qt::NoPen);
            p.setBrush(sheen);
            p.drawPolygon(innerOct);
            break;
        }
        case PodFinish::BRASS: {
            p.setPen(QPen(pal.accentBrass, std::max(1.2, radius * 0.05)));
            p.setBrush(Qt::NoBrush);
            p.drawPolygon(innerOct);
            p.setPen(Qt::NoPen);
            p.setBrush(pal.accentBrass.lighter(130));
            for (int i = 0; i < 8; ++i) {
                p.drawEllipse(innerOct[i], std::max(1.2, radius * 0.035), std::max(1.2, radius * 0.035));
            }
            break;
        }
        case PodFinish::PLANETARY: {
            QRadialGradient limb(center, radius * 0.78);
            limb.setColorAt(0.0, QColor(0, 0, 0, 0));
            limb.setColorAt(0.70, QColor(0, 0, 0, 0));
            QColor limbCol = pal.accentBrass;
            limbCol.setAlpha(150);
            limb.setColorAt(1.0, limbCol);
            p.setPen(Qt::NoPen);
            p.setBrush(limb);
            p.drawPolygon(innerOct);
            break;
        }
        case PodFinish::CLASSIC:
        default:
            break;
        }
    }

    // 5. Recessed 8-direction prong sockets around bevel ring
    double socketDist = radius * 0.86;
    double socketR = std::max(1.4, radius * 0.085);
    for (int d = DIR_N; d <= DIR_NW; ++d) {
        OctiDir dir = static_cast<OctiDir>(d);
        QPointF u = screenUnitForDir(dir, boardRotationDeg);
        QPointF sc = center + u * socketDist;
        OctiProngType pt = pod.prongAt(dir);
        if (pt == NO_PRONG) {
            p.setPen(QPen(fp.shadow.darker(140), 0.8));
            p.setBrush(QColor(12, 16, 24, 180));
        } else if (pt == SUPER_PRONG) {
            p.setPen(QPen(pal.prongSuperCore, 1.0));
            p.setBrush(pal.prongSuperGold);
        } else {
            p.setPen(QPen(pal.prongNormalTip, 0.9));
            p.setBrush(pal.prongNormalMetal);
        }
        p.drawEllipse(sc, socketR, socketR);
    }

    // 6. Faction letter emblem at center (or stack count override like "x2")
    QString label = centerLabelOverride;
    if (label.isEmpty()) {
        switch (type) {
        case A_POD: label = "A"; break;
        case B_POD: label = "B"; break;
        case C_POD: label = "C"; break;
        case D_POD: label = "D"; break;
        default:    label = "?"; break;
        }
    }

    QFont font = p.font();
    font.setBold(true);
    font.setPixelSize(std::max(9, static_cast<int>(radius * 0.72)));
    p.setFont(font);
    p.setPen(fp.emblem);
    p.drawText(QRectF(center.x() - radius * 0.6, center.y() - radius * 0.6,
                      radius * 1.2, radius * 1.2),
               Qt::AlignCenter, label);

    p.restore();
}

void paintProngGlyph(QPainter &p,
                     const QRectF &rect,
                     bool superProng,
                     const OctiPalette &pal)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    QPointF c = rect.center();
    double r = std::min(rect.width(), rect.height()) * 0.40;

    if (pal.prongStyle == ProngStyle::CLASSIC) {
        if (!superProng) {
            QPolygonF peg;
            peg << QPointF(c.x() - r * 0.28, c.y() + r * 0.75)
                << QPointF(c.x() - r * 0.22, c.y() - r * 0.45)
                << QPointF(c.x(),            c.y() - r * 0.85)
                << QPointF(c.x() + r * 0.22, c.y() - r * 0.45)
                << QPointF(c.x() + r * 0.28, c.y() + r * 0.75);

            QLinearGradient grad(c.x() - r * 0.3, c.y() - r * 0.8,
                                 c.x() + r * 0.3, c.y() + r * 0.8);
            grad.setColorAt(0.0, pal.prongNormalTip);
            grad.setColorAt(0.6, pal.prongNormalMetal);
            grad.setColorAt(1.0, pal.prongNormalMetal.darker(135));

            p.setPen(QPen(pal.accentBrass, 1.2));
            p.setBrush(grad);
            p.drawPolygon(peg);
        } else {
            QPolygonF star;
            star << QPointF(c.x(),            c.y() - r * 0.92)
                 << QPointF(c.x() + r * 0.34, c.y() - r * 0.25)
                 << QPointF(c.x() + r * 0.82, c.y() - r * 0.10)
                 << QPointF(c.x() + r * 0.38, c.y() + r * 0.28)
                 << QPointF(c.x() + r * 0.48, c.y() + r * 0.85)
                 << QPointF(c.x(),            c.y() + r * 0.52)
                 << QPointF(c.x() - r * 0.48, c.y() + r * 0.85)
                 << QPointF(c.x() - r * 0.38, c.y() + r * 0.28)
                 << QPointF(c.x() - r * 0.82, c.y() - r * 0.10)
                 << QPointF(c.x() - r * 0.34, c.y() - r * 0.25);

            QRadialGradient sgrad(c, r);
            sgrad.setColorAt(0.0, pal.prongSuperCore);
            sgrad.setColorAt(0.6, pal.prongSuperGold);
            sgrad.setColorAt(1.0, QColor(217, 119, 6));

            p.setPen(QPen(pal.prongSuperCore, 1.4));
            p.setBrush(sgrad);
            p.drawPolygon(star);
        }
    } else {
        if (!superProng) {
            switch (pal.prongStyle) {
            case ProngStyle::NEON_TUBE: {
                p.setPen(QPen(pal.accentBrass, r * 0.55, Qt::SolidLine, Qt::RoundCap));
                p.drawLine(QPointF(c.x(), c.y() + r * 0.7), QPointF(c.x(), c.y() - r * 0.7));
                p.setPen(QPen(pal.prongNormalTip, r * 0.2, Qt::SolidLine, Qt::RoundCap));
                p.drawLine(QPointF(c.x(), c.y() + r * 0.65), QPointF(c.x(), c.y() - r * 0.65));
                break;
            }
            case ProngStyle::FILIGREE: {
                QPolygonF peg;
                peg << QPointF(c.x() - r * 0.28, c.y() + r * 0.75)
                    << QPointF(c.x() - r * 0.22, c.y() - r * 0.45)
                    << QPointF(c.x(),            c.y() - r * 0.85)
                    << QPointF(c.x() + r * 0.22, c.y() - r * 0.45)
                    << QPointF(c.x() + r * 0.28, c.y() + r * 0.75);
                p.setPen(QPen(pal.accentBrass, 1.3));
                p.setBrush(pal.prongNormalMetal);
                p.drawPolygon(peg);
                p.setPen(QPen(pal.prongSuperGold, 1.4));
                p.drawLine(c.x() - r * 0.25, c.y() - r * 0.1, c.x() + r * 0.25, c.y() - r * 0.1);
                break;
            }
            case ProngStyle::ICICLE: {
                QPolygonF icicle;
                icicle << QPointF(c.x() - r * 0.26, c.y() + r * 0.75)
                       << QPointF(c.x() - r * 0.15, c.y() - r * 0.2)
                       << QPointF(c.x(),            c.y() - r * 0.9)
                       << QPointF(c.x() + r * 0.15, c.y() - r * 0.2)
                       << QPointF(c.x() + r * 0.26, c.y() + r * 0.75);
                QLinearGradient igrad(c.x(), c.y() - r * 0.9, c.x(), c.y() + r * 0.75);
                igrad.setColorAt(0.0, pal.prongNormalTip);
                igrad.setColorAt(1.0, pal.prongNormalMetal);
                p.setPen(QPen(QColor(220, 245, 255), 1.2));
                p.setBrush(igrad);
                p.drawPolygon(icicle);
                break;
            }
            case ProngStyle::FORGED: {
                QPolygonF shaft;
                shaft << QPointF(c.x() - r * 0.28, c.y() + r * 0.75)
                      << QPointF(c.x() - r * 0.22, c.y() - r * 0.45)
                      << QPointF(c.x(),            c.y() - r * 0.85)
                      << QPointF(c.x() + r * 0.22, c.y() - r * 0.45)
                      << QPointF(c.x() + r * 0.28, c.y() + r * 0.75);
                QLinearGradient fgrad(c.x(), c.y() - r * 0.85, c.x(), c.y() + r * 0.75);
                fgrad.setColorAt(0.0, pal.prongNormalTip);
                fgrad.setColorAt(0.4, QColor(249, 115, 22));
                fgrad.setColorAt(1.0, QColor(40, 28, 22));
                p.setPen(QPen(QColor(20, 15, 12), 1.2));
                p.setBrush(fgrad);
                p.drawPolygon(shaft);
                break;
            }
            case ProngStyle::LOLLIPOP: {
                QPolygonF shaft;
                shaft << QPointF(c.x() - r * 0.28, c.y() + r * 0.75)
                      << QPointF(c.x() - r * 0.22, c.y() - r * 0.45)
                      << QPointF(c.x(),            c.y() - r * 0.85)
                      << QPointF(c.x() + r * 0.22, c.y() - r * 0.45)
                      << QPointF(c.x() + r * 0.28, c.y() + r * 0.75);
                p.setPen(QPen(QColor(140, 50, 90), 1.2));
                p.setBrush(pal.prongNormalTip);
                p.drawPolygon(shaft);
                p.setPen(QPen(pal.accentBrass, 1.6));
                p.drawLine(c.x() - r * 0.25, c.y() + r * 0.2, c.x() + r * 0.25, c.y() + r * 0.4);
                p.drawLine(c.x() - r * 0.20, c.y() - r * 0.2, c.x() + r * 0.20, c.y());
                break;
            }
            case ProngStyle::CORAL: {
                p.setPen(QPen(pal.accentBrass, 1.2));
                p.setBrush(pal.prongNormalMetal);
                p.drawLine(QPointF(c.x(), c.y() + r * 0.7), QPointF(c.x(), c.y() - r * 0.7));
                p.setBrush(pal.prongNormalTip);
                p.drawEllipse(QPointF(c.x(), c.y() + r * 0.2), r * 0.28, r * 0.28);
                p.drawEllipse(QPointF(c.x(), c.y() - r * 0.4), r * 0.24, r * 0.24);
                break;
            }
            case ProngStyle::PIPE: {
                p.setPen(QPen(QColor(40, 25, 15), 1.2));
                p.setBrush(pal.prongNormalMetal);
                p.drawRect(QRectF(c.x() - r * 0.25, c.y() - r * 0.65, r * 0.5, r * 1.4));
                p.setBrush(pal.accentBrass);
                p.drawRoundedRect(QRectF(c.x() - r * 0.38, c.y() - r * 0.85, r * 0.76, r * 0.28), 2.0, 2.0);
                break;
            }
            case ProngStyle::COMET: {
                QPainterPath tail;
                tail.moveTo(c.x(), c.y() + r * 0.75);
                tail.lineTo(c.x() - r * 0.25, c.y() - r * 0.3);
                tail.lineTo(c.x() + r * 0.25, c.y() - r * 0.3);
                tail.closeSubpath();
                QLinearGradient tgrad(c.x(), c.y() - r * 0.5, c.x(), c.y() + r * 0.75);
                tgrad.setColorAt(0.0, pal.accentBrass);
                tgrad.setColorAt(1.0, QColor(0, 0, 0, 0));
                p.setPen(Qt::NoPen);
                p.setBrush(tgrad);
                p.drawPath(tail);
                p.setPen(QPen(pal.prongSuperGold, 1.2));
                p.setBrush(pal.prongNormalTip);
                p.drawEllipse(QPointF(c.x(), c.y() - r * 0.45), r * 0.32, r * 0.32);
                break;
            }
            case ProngStyle::CLASSIC:
            default:
                break;
            }
        } else {
            QPolygonF star;
            star << QPointF(c.x(),            c.y() - r * 0.92)
                 << QPointF(c.x() + r * 0.34, c.y() - r * 0.25)
                 << QPointF(c.x() + r * 0.82, c.y() - r * 0.10)
                 << QPointF(c.x() + r * 0.38, c.y() + r * 0.28)
                 << QPointF(c.x() + r * 0.48, c.y() + r * 0.85)
                 << QPointF(c.x(),            c.y() + r * 0.52)
                 << QPointF(c.x() - r * 0.48, c.y() + r * 0.85)
                 << QPointF(c.x() - r * 0.38, c.y() + r * 0.28)
                 << QPointF(c.x() - r * 0.82, c.y() - r * 0.10)
                 << QPointF(c.x() - r * 0.34, c.y() - r * 0.25);

            QRadialGradient sgrad(c, r);
            sgrad.setColorAt(0.0, pal.prongSuperCore);
            sgrad.setColorAt(0.6, pal.prongSuperGold);
            sgrad.setColorAt(1.0, pal.accentBrass.darker(130));

            p.setPen(QPen(pal.prongSuperCore, 1.4));
            p.setBrush(sgrad);
            p.drawPolygon(star);
        }
    }

    p.restore();
}


void paintCompassRose(QPainter &p,
                      const QRectF &rect,
                      int boardRotationDeg,
                      const OctiPalette &pal)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    QPointF c = rect.center();
    double r = std::min(rect.width(), rect.height()) * 0.44;

    p.setPen(QPen(pal.accentBrass, 1.4));
    p.setBrush(pal.panelCardBg);
    p.drawEllipse(c, r, r);

    QPointF northU = screenUnitForDir(DIR_N, boardRotationDeg);
    QPointF perp(-northU.y(), northU.x());

    // North needle ('a')
    QPolygonF northNeedle;
    northNeedle << (c + northU * (r * 0.82))
                << (c + perp * (r * 0.20))
                << (c - perp * (r * 0.20));
    p.setPen(QPen(pal.prongSuperCore, 1.0));
    p.setBrush(pal.legalCaptureGlow);
    p.drawPolygon(northNeedle);

    // South needle ('e')
    QPolygonF southNeedle;
    southNeedle << (c - northU * (r * 0.72))
                << (c + perp * (r * 0.18))
                << (c - perp * (r * 0.18));
    p.setPen(QPen(pal.accentBrass, 1.0));
    p.setBrush(pal.textSecondary);
    p.drawPolygon(southNeedle);

    QFont f = p.font();
    f.setBold(true);
    f.setPixelSize(std::max(8, static_cast<int>(r * 0.46)));
    p.setFont(f);
    p.setPen(pal.textPrimary);
    QPointF labelPt = c + northU * (r * 0.52);
    p.drawText(QRectF(labelPt.x() - r * 0.35, labelPt.y() - r * 0.35,
                      r * 0.70, r * 0.70),
               Qt::AlignCenter, "a");

    p.restore();
}

void paintProviderIcon(QPainter &p,
                       const QString &provider,
                       const QRectF &rect)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    QString prov = provider.trimmed().toLower();
    QColor bg(59, 130, 246);
    QString glyph = "ID";

    if (prov == "google") {
        bg = QColor(234, 67, 53);
        glyph = "G";
    } else if (prov == "apple") {
        bg = QColor(36, 41, 47);
        glyph = "A";
    } else if (prov == "microsoft") {
        bg = QColor(0, 164, 239);
        glyph = "M";
    } else if (prov == "facebook") {
        bg = QColor(24, 119, 242);
        glyph = "f";
    } else if (prov == "line") {
        bg = QColor(6, 199, 85);
        glyph = "L";
    }

    p.setPen(QPen(QColor(255, 255, 255, 190), 1.2));
    p.setBrush(bg);
    p.drawRoundedRect(rect.adjusted(1.0, 1.0, -1.0, -1.0), 5.0, 5.0);

    QFont f = p.font();
    f.setBold(true);
    f.setPixelSize(std::max(10, static_cast<int>(rect.height() * 0.56)));
    p.setFont(f);
    p.setPen(Qt::white);
    p.drawText(rect, Qt::AlignCenter, glyph);

    p.restore();
}

void paintVoiceMicIcon(QPainter &p,
                       const QRectF &rect,
                       bool active,
                       bool muted,
                       const OctiPalette &pal)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    QPointF c = rect.center();
    double r = std::min(rect.width(), rect.height()) * 0.42;

    QColor ringColor = muted ? pal.legalCaptureGlow :
                       active ? pal.legalStepGuide : pal.accentBrass;

    p.setPen(QPen(ringColor, 1.6));
    p.setBrush(active ? ringColor.darker(240) : pal.panelCardBg);
    p.drawEllipse(c, r, r);

    // Microphone capsule
    QRectF capRect(c.x() - r * 0.24, c.y() - r * 0.48, r * 0.48, r * 0.62);
    p.setBrush(ringColor);
    p.drawRoundedRect(capRect, r * 0.22, r * 0.22);

    // Stand & base
    p.setBrush(Qt::NoBrush);
    p.drawLine(QPointF(c.x(), c.y() + r * 0.18),
               QPointF(c.x(), c.y() + r * 0.52));
    p.drawLine(QPointF(c.x() - r * 0.30, c.y() + r * 0.52),
               QPointF(c.x() + r * 0.30, c.y() + r * 0.52));

    if (muted) {
        p.setPen(QPen(pal.legalCaptureGlow, 2.2));
        p.drawLine(QPointF(c.x() - r * 0.58, c.y() - r * 0.58),
                   QPointF(c.x() + r * 0.58, c.y() + r * 0.58));
    }

    p.restore();
}

void paintOctiAppIcon(QPainter &p, const QRectF &rect)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    QPointF c = rect.center();
    double s = std::min(rect.width(), rect.height());
    double rOuter = s * 0.455;
    double rDial  = rOuter * 0.835;
    double rPod   = rOuter * 0.455;

    // 1. Soft ambient drop shadow beneath the octagonal crest
    QPolygonF shadowOct = regularOctagon(
        c + QPointF(0.0, s * 0.022), rOuter * 1.02);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 155));
    p.drawPolygon(shadowOct);

    // 2. 3D Faceted Imperial Brass & Gold Outer Octagonal Bezel
    QPolygonF outerOct = regularOctagon(c, rOuter);
    QPolygonF dialOct  = regularOctagon(c, rDial);

    // Base gold fill
    QLinearGradient baseRimGrad(c - QPointF(rOuter, rOuter),
                                c + QPointF(rOuter, rOuter));
    baseRimGrad.setColorAt(0.0, QColor(254, 240, 138));
    baseRimGrad.setColorAt(0.35, QColor(234, 179, 8));
    baseRimGrad.setColorAt(0.70, QColor(180, 83, 9));
    baseRimGrad.setColorAt(1.0, QColor(92, 45, 12));
    p.setPen(Qt::NoPen);
    p.setBrush(baseRimGrad);
    p.drawPolygon(outerOct);

    // Individual 8 bevel trapezoid facets lit from upper-left (-135 deg)
    static const QColor facetColors[8] = {
        QColor(180, 110, 24),   // 0: E-SE (shaded warm bronze)
        QColor(120, 62, 14),    // 1: S-SE (deep shadow bronze)
        QColor(92, 45, 12),     // 2: S-SW (deepest shadow)
        QColor(168, 102, 22),   // 3: W-SW (mid bronze)
        QColor(234, 179, 8),    // 4: W-NW (warm gold)
        QColor(254, 243, 199),  // 5: N-NW (brightest specular gold)
        QColor(253, 224, 71),   // 6: N-NE (bright imperial gold)
        QColor(217, 155, 26),   // 7: E-NE (rich brass)
    };
    for (int i = 0; i < 8; ++i) {
        int next = (i + 1) % 8;
        QPolygonF facet;
        facet << outerOct[i] << outerOct[next]
              << dialOct[next] << dialOct[i];
        QLinearGradient fGrad(outerOct[i], dialOct[next]);
        fGrad.setColorAt(0.0, facetColors[i].lighter(112));
        fGrad.setColorAt(1.0, facetColors[i].darker(115));
        p.setPen(QPen(QColor(254, 240, 138, 95), std::max(0.6, s * 0.005)));
        p.setBrush(fGrad);
        p.drawPolygon(facet);
    }

    // Crisp outer rim stroke
    p.setPen(QPen(QColor(45, 23, 6, 230), std::max(1.0, s * 0.012)));
    p.setBrush(Qt::NoBrush);
    p.drawPolygon(outerOct);

    // 3. Deep Obsidian-Sapphire Starburst Dial
    QRadialGradient dialGrad(c - QPointF(rDial * 0.18, rDial * 0.18), rDial * 1.15);
    dialGrad.setColorAt(0.0, QColor(30, 44, 72));
    dialGrad.setColorAt(0.55, QColor(15, 23, 42));
    dialGrad.setColorAt(1.0, QColor(6, 10, 19));

    p.setPen(QPen(QColor(253, 224, 71, 210), std::max(1.0, s * 0.013)));
    p.setBrush(dialGrad);
    p.drawPolygon(dialOct);

    // Subtle inlaid golden tactical grid & inner octagonal track (for sizes >= 28px)
    if (s >= 28.0) {
        p.save();
        QPainterPath clipPath;
        clipPath.addPolygon(dialOct);
        p.setClipPath(clipPath);

        double gridStep = rDial * 0.42;
        QPen gridPen(QColor(212, 175, 55, 58), std::max(0.8, s * 0.007));
        p.setPen(gridPen);
        for (int k = -1; k <= 1; k += 2) {
            double off = k * gridStep;
            p.drawLine(QPointF(c.x() - rDial, c.y() + off),
                       QPointF(c.x() + rDial, c.y() + off));
            p.drawLine(QPointF(c.x() + off, c.y() - rDial),
                       QPointF(c.x() + off, c.y() + rDial));
        }

        QPolygonF trackOct = regularOctagon(c, rDial * 0.84);
        p.setPen(QPen(QColor(234, 179, 8, 85), std::max(0.8, s * 0.008)));
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(trackOct);
        p.restore();
    }

    // 8 recessed perimeter target sockets on the dial
    if (s >= 24.0) {
        double ringR = rDial * 0.84;
        double sockR = std::max(1.2, s * 0.020);
        for (int d = DIR_N; d <= DIR_NW; ++d) {
            QPointF u = screenUnitForDir(static_cast<OctiDir>(d), 0);
            QPointF pt = c + u * ringR;
            bool cardinal = (d % 2 == 0);
            p.setPen(QPen(cardinal ? QColor(253, 224, 71, 200)
                                   : QColor(203, 213, 225, 180),
                          std::max(0.7, s * 0.006)));
            p.setBrush(QColor(8, 12, 22, 220));
            p.drawEllipse(pt, sockR, sockR);
        }
    }

    // 4. 8 Radiating Prongs (4 Diagonal Platinum-Silver Prongs + 4 Cardinal Gold Superprongs)
    double rProngBase = rPod * 0.72;
    double rSilverTip = rDial * 0.85;
    double rGoldTip   = rDial * 0.91;

    // First draw the 4 diagonal Platinum-Silver Prongs (NE, SE, SW, NW)
    for (int d = DIR_N; d <= DIR_NW; ++d) {
        if (d % 2 == 0) {
            continue;
        }
        QPointF u = screenUnitForDir(static_cast<OctiDir>(d), 0);
        QPointF perp(-u.y(), u.x());
        double hwBase = std::max(1.4, s * 0.032);
        double hwShoulder = std::max(1.8, s * 0.040);
        double shoulderDist = rSilverTip * 0.78;

        QPointF baseCenter = c + u * rProngBase;
        QPointF tipPt      = c + u * rSilverTip;
        QPointF leftBase   = baseCenter - perp * hwBase;
        QPointF rightBase  = baseCenter + perp * hwBase;
        QPointF leftSh     = c + u * shoulderDist - perp * hwShoulder;
        QPointF rightSh    = c + u * shoulderDist + perp * hwShoulder;

        // Lit left facet
        QPolygonF leftFacet;
        leftFacet << baseCenter << leftBase << leftSh << tipPt;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawPolygon(leftFacet);

        // Shaded right facet
        QPolygonF rightFacet;
        rightFacet << baseCenter << tipPt << rightSh << rightBase;
        p.setBrush(QColor(148, 163, 184));
        p.drawPolygon(rightFacet);

        // Full prong outline
        QPolygonF fullProng;
        fullProng << leftBase << leftSh << tipPt << rightSh << rightBase;
        p.setPen(QPen(QColor(15, 23, 42, 220), std::max(0.8, s * 0.009)));
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(fullProng);
    }

    // Next draw the 4 cardinal Golden Crown Superprongs (N, E, S, W)
    for (int d = DIR_N; d <= DIR_NW; ++d) {
        if (d % 2 != 0) {
            continue;
        }
        QPointF u = screenUnitForDir(static_cast<OctiDir>(d), 0);
        QPointF perp(-u.y(), u.x());
        double hwBase  = std::max(1.6, s * 0.036);
        double hwFlare = std::max(2.4, s * 0.058);
        double flareDist = rGoldTip * 0.72;

        QPointF baseCenter = c + u * rProngBase;
        QPointF tipPt      = c + u * rGoldTip;
        QPointF leftBase   = baseCenter - perp * hwBase;
        QPointF rightBase  = baseCenter + perp * hwBase;
        QPointF leftFlare  = c + u * flareDist - perp * hwFlare;
        QPointF rightFlare = c + u * flareDist + perp * hwFlare;

        // Lit golden facet
        QPolygonF leftFacet;
        leftFacet << baseCenter << leftBase << leftFlare << tipPt;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(254, 240, 138));
        p.drawPolygon(leftFacet);

        // Rich amber shaded facet
        QPolygonF rightFacet;
        rightFacet << baseCenter << tipPt << rightFlare << rightBase;
        p.setBrush(QColor(217, 119, 6));
        p.drawPolygon(rightFacet);

        // Full superprong crown outline
        QPolygonF fullCrown;
        fullCrown << leftBase << leftFlare << tipPt << rightFlare << rightBase;
        p.setPen(QPen(QColor(255, 251, 235, 235), std::max(0.9, s * 0.010)));
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(fullCrown);
    }

    // 5. Central Sculpted Emerald-Jade Octagonal Pod
    QPolygonF podShadow = regularOctagon(
        c + QPointF(0.0, s * 0.016), rPod * 1.06);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 175));
    p.drawPolygon(podShadow);

    QPolygonF podOuter = regularOctagon(c, rPod);
    QPolygonF podInner = regularOctagon(c, rPod * 0.74);

    // Emerald pod bevel collar with 8 directional facets
    static const QColor podFacetColors[8] = {
        QColor(5, 150, 105),    // 0: E-SE
        QColor(6, 95, 70),      // 1: S-SE
        QColor(6, 78, 59),      // 2: S-SW
        QColor(4, 120, 87),     // 3: W-SW
        QColor(16, 185, 129),   // 4: W-NW
        QColor(167, 243, 208),  // 5: N-NW (specular highlight)
        QColor(110, 231, 183),  // 6: N-NE
        QColor(52, 211, 153),   // 7: E-NE
    };
    for (int i = 0; i < 8; ++i) {
        int next = (i + 1) % 8;
        QPolygonF pFacet;
        pFacet << podOuter[i] << podOuter[next]
               << podInner[next] << podInner[i];
        p.setPen(QPen(QColor(167, 243, 208, 70), std::max(0.5, s * 0.004)));
        p.setBrush(podFacetColors[i]);
        p.drawPolygon(pFacet);
    }

    // Crisp gold outer rim on the central pod
    p.setPen(QPen(QColor(253, 224, 71), std::max(1.2, s * 0.015)));
    p.setBrush(Qt::NoBrush);
    p.drawPolygon(podOuter);

    // Inner Emerald Cabochon Dome
    QRadialGradient domeGrad(c - QPointF(rPod * 0.25, rPod * 0.25), rPod * 0.95);
    domeGrad.setColorAt(0.0, QColor(167, 243, 208));
    domeGrad.setColorAt(0.38, QColor(16, 185, 129));
    domeGrad.setColorAt(0.78, QColor(5, 150, 105));
    domeGrad.setColorAt(1.0, QColor(6, 78, 59));

    p.setPen(QPen(QColor(209, 250, 229, 210), std::max(0.9, s * 0.010)));
    p.setBrush(domeGrad);
    p.drawPolygon(podInner);

    // 8 recessed collar sockets on the pod bevel ring
    if (s >= 24.0) {
        double collarR = rPod * 0.87;
        double cSockR = std::max(1.1, s * 0.018);
        for (int d = DIR_N; d <= DIR_NW; ++d) {
            QPointF u = screenUnitForDir(static_cast<OctiDir>(d), 0);
            QPointF sc = c + u * collarR;
            bool cardinal = (d % 2 == 0);
            p.setPen(QPen(QColor(6, 46, 34, 220), std::max(0.6, s * 0.005)));
            p.setBrush(cardinal ? QColor(253, 224, 71) : QColor(241, 245, 249));
            p.drawEllipse(sc, cSockR, cSockR);
        }
    }

    // 6. Sculpted Golden Octagonal Crest & Center Jewel inside the Pod Dome
    QPolygonF emblemOuter = regularOctagon(c, rPod * 0.46);
    QPolygonF emblemInner = regularOctagon(c, rPod * 0.25);
    QPainterPath emblemRingPath;
    emblemRingPath.addPolygon(emblemOuter);
    emblemRingPath.addPolygon(emblemInner);

    QLinearGradient emblemGrad(c - QPointF(rPod * 0.46, rPod * 0.46),
                               c + QPointF(rPod * 0.46, rPod * 0.46));
    emblemGrad.setColorAt(0.0, QColor(255, 251, 235));
    emblemGrad.setColorAt(0.5, QColor(253, 224, 71));
    emblemGrad.setColorAt(1.0, QColor(217, 119, 6));

    p.setPen(QPen(QColor(6, 78, 59, 190), std::max(0.7, s * 0.008)));
    p.setBrush(emblemGrad);
    p.drawPath(emblemRingPath);

    // Glowing white-gold center pip
    double pipR = std::max(1.2, rPod * 0.11);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 251, 235));
    p.drawEllipse(c, pipR, pipR);

    p.restore();
}

QImage renderOctiIconImage(int sizePx)
{
    int sz = std::clamp(sizePx, 16, 1024);
    QImage img(sz, sz, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter painter(&img);
    paintOctiAppIcon(painter, QRectF(0.0, 0.0, sz, sz));
    painter.end();

    return img.convertToFormat(QImage::Format_RGBA8888);
}

QIcon createOctiAppIcon()
{
    QIcon icon;
    static const int sizes[] = {16, 24, 32, 48, 64, 128, 256};
    for (int sz : sizes) {
        QImage img = renderOctiIconImage(sz);
        icon.addPixmap(QPixmap::fromImage(img));
    }
    return icon;
}

} // namespace octi

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
