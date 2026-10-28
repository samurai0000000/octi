/*
 * OctiTheme.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiTheme.hxx"
#include <algorithm>
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

const OctiPalette &paletteForTheme(ThemeId id)
{
    static const OctiPalette obsidian = makeObsidianBrassPalette();
    static const OctiPalette rosewood = makeRosewoodIvoryPalette();
    return (id == ThemeId::ROSEWOOD_IVORY) ? rosewood : obsidian;
}

QString buildAppStyleSheet(ThemeId id)
{
    const OctiPalette &p = paletteForTheme(id);
    return QString(
        "QMainWindow, QWidget { background-color: %1; color: %2; font-family: 'Segoe UI', 'DejaVu Sans', sans-serif; }\n"
        "QDialog { background-color: %3; color: %2; border: 1px solid %4; }\n"
        "QFrame { background-color: %3; border: 1px solid %4; border-radius: 6px; }\n"
        "QGroupBox { background-color: %3; border: 1px solid %4; border-radius: 6px; margin-top: 12px; padding-top: 16px; font-weight: bold; }\n"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 6px; color: %5; }\n"
        "QPushButton { background-color: %3; color: %2; border: 1px solid %5; border-radius: 5px; padding: 5px 10px; font-weight: bold; }\n"
        "QPushButton:hover { background-color: %5; color: %1; }\n"
        "QPushButton:disabled { color: %6; border-color: %6; }\n"
        "QLineEdit, QComboBox, QSpinBox, QTextEdit, QListWidget, QTableWidget { background-color: %1; color: %2; border: 1px solid %4; border-radius: 4px; padding: 4px; }\n"
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

void paintOctagonalPod(QPainter &p,
                       const QRectF &rect,
                       OctiPodType type,
                       const OctiPod &pod,
                       const OctiPalette &pal,
                       int boardRotationDeg,
                       bool highlighted,
                       double opacity)
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

    // 6. Faction letter emblem at center
    QString label = "?";
    switch (type) {
    case A_POD: label = "A"; break;
    case B_POD: label = "B"; break;
    case C_POD: label = "C"; break;
    case D_POD: label = "D"; break;
    default:    break;
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
