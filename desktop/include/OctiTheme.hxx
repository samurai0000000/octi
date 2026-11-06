/*
 * OctiTheme.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_OCTI_THEME_HXX
#define OCTI_DESKTOP_OCTI_THEME_HXX

#include <QColor>
#include <QIcon>
#include <QImage>
#include <QPainter>
#include <QRectF>
#include <QString>
#include "OctiDefs.hxx"
#include "OctiPod.hxx"

namespace octi {

enum class ThemeId {
    OBSIDIAN_BRASS  = 0,
    ROSEWOOD_IVORY  = 1,
    NEON_CIRCUIT    = 2,
    JADE_DYNASTY    = 3,
    ARCTIC_AURORA   = 4,
    VOLCANIC_FORGE  = 5,
    CANDY_POP       = 6,
    OCEAN_PEARL     = 7,
    STEAMPUNK_BRASS = 8,
    COSMIC_NEBULA   = 9,
};

enum class PodFinish {
    CLASSIC = 0,
    GLASS,
    CARVED,
    HAMMERED,
    GLOSS,
    NACRE,
    BRASS,
    PLANETARY,
};

enum class PodPattern {
    NONE = 0,
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
    FACTION_SPECIFIC,
};

enum class ProngStyle {
    CLASSIC = 0,
    NEON_TUBE,
    FILIGREE,
    ICICLE,
    FORGED,
    LOLLIPOP,
    CORAL,
    PIPE,
    COMET,
};

enum class BoardTexture {
    NONE = 0,
    PCB,
    LACQUER_SCROLL,
    FROST,
    BASALT_LAVA,
    SPRINKLES,
    CAUSTICS,
    RIVET_PLATE,
    STARFIELD,
};

enum class CellShape {
    SQUARE = 0,
    ROUNDED,
};

struct PodStyle {
    PodFinish finish = PodFinish::CLASSIC;
    PodPattern pattern = PodPattern::NONE;
};

struct PodFactionPalette {
    QColor core;
    QColor rim;
    QColor highlight;
    QColor shadow;
    QColor emblem;
    PodPattern pattern = PodPattern::NONE;
};

struct OctiPalette {
    QColor windowBg;
    QColor panelCardBg;
    QColor panelCardBorder;
    QColor textPrimary;
    QColor textSecondary;
    QColor accentBrass;

    QColor boardOuterFrame;
    QColor boardBevelLight;
    QColor boardBevelDark;
    QColor gridCellDark;
    QColor gridCellLight;
    QColor gridSocketShadow;
    QColor gridLine;
    QColor coordLabel;

    QColor neutralOctiFill;
    QColor neutralOctiRim;
    QColor edgelessPortalGlow;
    QColor legalStepGuide;
    QColor legalJumpGuide;
    QColor legalCaptureGlow;

    QColor homeBadgeOuterBg = QColor(10, 14, 22);
    QColor homeBadgeInnerBg = QColor(7, 10, 16);

    QColor prongNormalMetal;
    QColor prongNormalTip;
    QColor prongSuperGold;
    QColor prongSuperCore;

    PodStyle podStyle;
    ProngStyle prongStyle = ProngStyle::CLASSIC;
    BoardTexture boardTexture = BoardTexture::NONE;
    CellShape cellShape = CellShape::SQUARE;
    double glowStrength = 0.0;

    PodFactionPalette podA;
    PodFactionPalette podB;
    PodFactionPalette podC;
    PodFactionPalette podD;

    const PodFactionPalette &factionForPodType(OctiPodType type) const;
    const PodFactionPalette &factionForPlayer(OctiPlayer player) const;
};

const char *themeIdToConfigKey(ThemeId id);
const char *themeIdToWebKey(ThemeId id);
QString themeDisplayName(ThemeId id);

const OctiPalette &paletteForTheme(ThemeId id);
QString buildAppStyleSheet(ThemeId id);

void paintOctagonalPod(QPainter &p,
                       const QRectF &rect,
                       OctiPodType type,
                       const OctiPod &pod,
                       const OctiPalette &pal,
                       int boardRotationDeg = 0,
                       bool highlighted = false,
                       double opacity = 1.0,
                       const QString &centerLabelOverride = QString());

inline void paintPodGlyph(QPainter &p,
                          const QRectF &rect,
                          OctiPodType type,
                          const OctiPod &pod,
                          const OctiPalette &pal,
                          int boardRotationDeg = 0,
                          bool highlighted = false,
                          double opacity = 1.0,
                          const QString &centerLabelOverride = QString())
{
    paintOctagonalPod(p, rect, type, pod, pal, boardRotationDeg, highlighted, opacity, centerLabelOverride);
}

void paintProngGlyph(QPainter &p,
                     const QRectF &rect,
                     bool superProng,
                     const OctiPalette &pal);

void paintCompassRose(QPainter &p,
                      const QRectF &rect,
                      int boardRotationDeg,
                      const OctiPalette &pal);

void paintProviderIcon(QPainter &p,
                       const QString &provider,
                       const QRectF &rect);

void paintVoiceMicIcon(QPainter &p,
                       const QRectF &rect,
                       bool active,
                       bool muted,
                       const OctiPalette &pal);

inline void paintVoiceIcon(QPainter &p,
                           const QRectF &rect,
                           bool active,
                           bool muted,
                           const OctiPalette &pal)
{
    paintVoiceMicIcon(p, rect, active, muted, pal);
}

void paintOctiAppIcon(QPainter &p, const QRectF &rect);
QImage renderOctiIconImage(int sizePx);
QIcon createOctiAppIcon();

} // namespace octi

#endif // OCTI_DESKTOP_OCTI_THEME_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
