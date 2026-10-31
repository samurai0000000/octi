/*
 * OctiReservePanel.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiReservePanel.hxx"
#include "QtCompat.hxx"
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>
#include <QFont>
#include <QPen>

namespace octi {

namespace {

QPolygonF makeRegularOctagon(const QPointF &center, double radius)
{
    QPolygonF poly;
    for (int i = 0; i < 8; ++i) {
        double angleRad = (static_cast<double>(i) * 45.0 + 22.5) * 3.14159265358979323846 / 180.0;
        poly << QPointF(center.x() + std::cos(angleRad) * radius,
                        center.y() + std::sin(angleRad) * radius);
    }
    return poly;
}

enum class WellIconKind {
    POD,
    CAPTURED,
    PRONG,
    SUPERPRONG
};

} // namespace

OctiReservePanel::OctiReservePanel(QWidget *parent)
    : QWidget(parent)
    , _board(nullptr)
    , _composer(nullptr)
    , _canvas(nullptr)
    , _soundSynth(nullptr)
    , _theme(ThemeId::OBSIDIAN_BRASS)
    , _pttActive(false)
    , _voiceMutedByRoom(false)
{
    setMinimumSize(280, 480);
}

OctiReservePanel::~OctiReservePanel() = default;

void OctiReservePanel::attach(OctiBoard *board,
                              OctiMoveComposer *composer,
                              OctiBoardCanvas *canvas,
                              OctiSoundSynth *soundSynth)
{
    _board = board;
    _composer = composer;
    _canvas = canvas;
    _soundSynth = soundSynth;
    update();
}

void OctiReservePanel::setTheme(ThemeId theme)
{
    _theme = theme;
    update();
}

ThemeId OctiReservePanel::theme() const
{
    return _theme;
}

void OctiReservePanel::refresh()
{
    update();
}

void OctiReservePanel::setPushToTalkCallback(std::function<void(bool talking)> cb)
{
    _pttCallback = std::move(cb);
}

void OctiReservePanel::setPushToTalkActive(bool talking)
{
    bool nextState = talking && !_voiceMutedByRoom;
    if (_pttActive != nextState) {
        _pttActive = nextState;
        if (_pttCallback) {
            _pttCallback(_pttActive);
        }
        update();
    }
}

bool OctiReservePanel::isPushToTalkActive() const
{
    return _pttActive;
}

void OctiReservePanel::setVoiceMutedByRoom(bool muted)
{
    _voiceMutedByRoom = muted;
    if (_voiceMutedByRoom && _pttActive) {
        _pttActive = false;
        if (_pttCallback) {
            _pttCallback(false);
        }
    }
    update();
}

bool OctiReservePanel::isVoiceMutedByRoom() const
{
    return _voiceMutedByRoom;
}

QString OctiReservePanel::formatClockMs(long ms)
{
    long clamped = std::max(0L, ms);
    long totalSec = clamped / 1000L;
    long mins = totalSec / 60L;
    long secs = totalSec % 60L;
    long tenths = (clamped % 1000L) / 100L;
    return QString("%1:%2.%3")
        .arg(mins, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'))
        .arg(tenths);
}

QImage OctiReservePanel::renderToImage(int width, int height)
{
    int w = std::max(220, width);
    int h = std::max(360, height);
    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(paletteForTheme(_theme).windowBg);

    QPainter painter(&img);
    paintReserveScene(painter, w, h);
    painter.end();

    return img;
}

void OctiReservePanel::paintReserveScene(QPainter &p, int width, int height)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    _iconGlyphCountRendered = 0;
    for (int i = 0; i < 5; ++i) {
        _playerCardRects[i] = QRectF();
    }

    const OctiPalette &pal = paletteForTheme(_theme);
    p.fillRect(0, 0, width, height, pal.windowBg);

    const OctiGame *game = _board ? _board->getGame() : nullptr;
    OctiGameMode mode = _board ? _board->getGameMode() : GAME_MODE_2;
    bool isGameOver = game ? game->isGameOver() : false;
    OctiPlayer winner = isGameOver ? game->getWinner() : PLAYER_UNDEF;
    OctiPlayer activeTurn = game ? game->getTurn() : PLAYER_A;
    int numPlayers = (mode == GAME_MODE_4) ? 4 : 2;

    double topEdgeY = 10.0;
    double bottomEdgeY = static_cast<double>(height) - 10.0;
    if (_canvas != nullptr) {
        QRectF boardOuter = _canvas->geometry().boardOuterRect();
        if (_canvas->height() > 0 && std::abs(height - _canvas->height()) <= 8 &&
            boardOuter.height() > 100.0 && boardOuter.bottom() <= height) {
            topEdgeY = boardOuter.top();
            bottomEdgeY = boardOuter.bottom();
        } else {
            int bw = _board ? _board->getBoardWidth() : 9;
            int bh = _board ? _board->getBoardHeight() : 9;
            OctiBoardGeometry geom(height, height, bw, bh, _canvas->boardRotation());
            QRectF geomOuter = geom.boardOuterRect();
            if (geomOuter.height() > 100.0 && geomOuter.bottom() <= height) {
                topEdgeY = geomOuter.top();
                bottomEdgeY = geomOuter.bottom();
            }
        }
    }
    topEdgeY = std::max(4.0, topEdgeY);
    bottomEdgeY = std::min(static_cast<double>(height) - 4.0, bottomEdgeY);

    double margin = 10.0;
    double cardW = static_cast<double>(width) - 2.0 * margin;
    std::vector<std::pair<OctiPlayer, QRectF>> cardsToDraw;

    if (numPlayers == 2) {
        double cardH = 132.0;
        QRectF cardRectB(margin, topEdgeY, cardW, cardH);
        QRectF cardRectA(margin, bottomEdgeY - cardH, cardW, cardH);
        _playerCardRects[PLAYER_B] = cardRectB;
        _playerCardRects[PLAYER_A] = cardRectA;
        cardsToDraw.push_back({PLAYER_B, cardRectB});
        cardsToDraw.push_back({PLAYER_A, cardRectA});
    } else {
        double cardH = 94.0;
        double availH = bottomEdgeY - topEdgeY;
        double totalCardsH = 4.0 * cardH;
        double spacing = (availH > totalCardsH) ? (availH - totalCardsH) / 3.0 : 6.0;
        OctiPlayer pOrder[4] = {PLAYER_B, PLAYER_C, PLAYER_D, PLAYER_A};
        for (int k = 0; k < 4; ++k) {
            OctiPlayer pl = pOrder[k];
            double y = topEdgeY + k * (cardH + spacing);
            QRectF cRect(margin, y, cardW, cardH);
            _playerCardRects[pl] = cRect;
            cardsToDraw.push_back({pl, cRect});
        }
    }

    for (const auto &item : cardsToDraw) {
        OctiPlayer player = item.first;
        const QRectF &cardRect = item.second;
        double cardH = cardRect.height();
        const PodFactionPalette &fp = pal.factionForPlayer(player);
        bool isTurn = !isGameOver && (player == activeTurn);
        bool isWinner = isGameOver && (player == winner ||
            (mode == GAME_MODE_4 &&
             (((winner == PLAYER_A || winner == PLAYER_C) && (player == PLAYER_A || player == PLAYER_C)) ||
              ((winner == PLAYER_B || winner == PLAYER_D) && (player == PLAYER_B || player == PLAYER_D)))));
        bool isElim = game ? game->isPlayerEliminated(player) : false;

        p.setPen(QPen((isTurn || isWinner) ? pal.prongSuperGold : pal.panelCardBorder,
                      (isTurn || isWinner) ? 2.4 : 1.2));
        p.setBrush(pal.panelCardBg);
        p.drawRoundedRect(cardRect, 8.0, 8.0);

        // Player header & Fischer digital clock
        int pLetterIdx = player - PLAYER_A;
        QString pName = QString("Player %1").arg(QChar('A' + pLetterIdx));
        if (mode == GAME_MODE_4) {
            pName += (player == PLAYER_A || player == PLAYER_C)
                ? " (Team A+C)" : " (Team B+D)";
        }
        if (isWinner) {
            pName += " [WINNER]";
        } else if (isElim) {
            pName += " [ELIMINATED]";
        }

        QFont hf = p.font();
        hf.setBold(true);
        hf.setPixelSize(13);
        p.setFont(hf);
        p.setPen(isWinner ? pal.prongSuperGold : (isElim ? pal.legalCaptureGlow : fp.highlight));
        p.drawText(QRectF(cardRect.left() + 10.0, cardRect.top() + 6.0,
                          cardRect.width() * 0.65, 22.0),
                   Qt::AlignVCenter | Qt::AlignLeft, pName);

        long remMs = game ? game->getTimeRemaining(player) : 600000L;
        if (remMs <= 0) {
            remMs = 600000L;
        }
        p.setPen(pal.textPrimary);
        p.drawText(QRectF(cardRect.right() - 100.0, cardRect.top() + 6.0,
                          90.0, 22.0),
                   Qt::AlignVCenter | Qt::AlignRight, formatClockMs(remMs));

        // Resource wells (Free Pods, Captured, Normal Prongs, Superprongs)
        int freePods = game ? game->getFreePods(player) : 4;
        int capPods = game ? game->getCapturedPods(player) : 0;
        int freeProngs = game ? game->getFreeProngs(player) : 25;
        int superProngs = game ? game->getFreeSuperProngs(player) : 0;
        bool canLib = (_board && _board->canLiberate(player) && capPods > 0);
        bool hasSuper = (game && game->useSuperProngs());
        bool showCaptured = (capPods > 0);

        double wellY = cardRect.top() + 32.0;
        double wellH = cardH - 40.0;

        QRectF rFreePod;
        QRectF rLibPod;
        QRectF rNormProng;
        QRectF rSupProng;

        int numWells = 2 + (showCaptured ? 1 : 0) + (hasSuper ? 1 : 0);
        double totalGap = 6.0 * static_cast<double>(numWells + 1);
        double wellW = (cardW - totalGap) / static_cast<double>(numWells);
        double curX = cardRect.left() + 6.0;

        rFreePod = QRectF(curX, wellY, wellW, wellH);
        curX = rFreePod.right() + 6.0;

        if (showCaptured) {
            rLibPod = QRectF(curX, wellY, wellW, wellH);
            curX = rLibPod.right() + 6.0;
        } else {
            rLibPod = QRectF();
        }

        rNormProng = QRectF(curX, wellY, wellW, wellH);
        curX = rNormProng.right() + 6.0;

        if (hasSuper) {
            rSupProng = QRectF(curX, wellY, wellW, wellH);
        } else {
            rSupProng = QRectF();
        }

        if (isTurn || (isGameOver && player == activeTurn)) {
            _freePodWellRect = rFreePod;
            _liberatePodWellRect = rLibPod;
            _normalProngWellRect = rNormProng;
            _superProngWellRect = rSupProng;
        }

        auto drawWell = [&](const QRectF &wr, const QString &title,
                            int count, bool highlightGlow, WellIconKind iconKind) {
            p.setPen(QPen(highlightGlow ? pal.legalStepGuide : pal.panelCardBorder,
                          highlightGlow ? 2.0 : 1.0));
            p.setBrush(pal.windowBg);
            p.drawRoundedRect(wr, 5.0, 5.0);

            // Vector Icon
            if (iconKind == WellIconKind::POD) {
                double podR = std::min(wr.width() * 0.28, wr.height() * 0.22);
                podR = std::clamp(podR, 8.0, 14.0);
                QPointF podCenter(wr.center().x(), wr.top() + podR + 4.0);
                QPolygonF oct = makeRegularOctagon(podCenter, podR);
                QRadialGradient rg(podCenter - QPointF(podR * 0.2, podR * 0.2), podR * 1.2);
                rg.setColorAt(0.0, fp.highlight);
                rg.setColorAt(0.5, fp.core);
                rg.setColorAt(1.0, fp.shadow);
                p.setPen(QPen(fp.rim, 1.4));
                p.setBrush(rg);
                p.drawPolygon(oct);

                QFont ef("SansSerif", std::max(8, static_cast<int>(podR * 0.95)), QFont::Bold);
                p.setFont(ef);
                p.setPen(fp.emblem);
                p.drawText(QRectF(podCenter.x() - podR, podCenter.y() - podR, podR * 2.0, podR * 2.0),
                           Qt::AlignCenter, QString(QChar('A' + player - 1)));
                _iconGlyphCountRendered++;
            } else if (iconKind == WellIconKind::CAPTURED) {
                OctiPlayer opp = (player == PLAYER_A) ? PLAYER_B : PLAYER_A;
                const PodFactionPalette &ofp = pal.factionForPlayer(opp);
                double capR = std::min(wr.width() * 0.26, wr.height() * 0.20);
                capR = std::clamp(capR, 7.0, 13.0);
                QPointF capCenter(wr.center().x(), wr.top() + capR + 4.0);
                QPolygonF oct = makeRegularOctagon(capCenter, capR);
                QRadialGradient rg(capCenter - QPointF(capR * 0.2, capR * 0.2), capR * 1.2);
                rg.setColorAt(0.0, ofp.highlight);
                rg.setColorAt(0.5, ofp.core);
                rg.setColorAt(1.0, ofp.shadow);
                p.setPen(QPen(ofp.rim, 1.0));
                p.setBrush(rg);
                p.drawPolygon(oct);

                QFont ef("SansSerif", std::max(7, static_cast<int>(capR * 0.90)), QFont::Bold);
                p.setFont(ef);
                p.setPen(ofp.emblem);
                p.drawText(QRectF(capCenter.x() - capR, capCenter.y() - capR, capR * 2.0, capR * 2.0),
                           Qt::AlignCenter, QString(QChar('A' + opp - 1)));

                p.setPen(QPen(pal.legalCaptureGlow, 1.5));
                p.setBrush(Qt::NoBrush);
                p.drawEllipse(capCenter, capR * 1.25, capR * 1.25);
                p.drawLine(QPointF(capCenter.x() - capR * 1.1, capCenter.y() + capR * 1.1),
                           QPointF(capCenter.x() + capR * 1.1, capCenter.y() - capR * 1.1));
                _iconGlyphCountRendered++;
            } else if (iconKind == WellIconKind::PRONG) {
                double prongH = std::min(wr.height() * 0.38, 22.0);
                double prongW = prongH * 0.65;
                QPointF pCenter(wr.center().x(), wr.top() + prongH * 0.5 + 4.0);
                double cx = pCenter.x();
                double tipY = pCenter.y() - prongH * 0.5;
                double baseY = pCenter.y() + prongH * 0.5;
                double midY = tipY + (baseY - tipY) * 0.55;
                double baseHalfW = prongW * 0.45;
                double midHalfW = prongW * 0.28;
                QPolygonF blade;
                blade << QPointF(cx - baseHalfW, baseY)
                      << QPointF(cx - midHalfW, midY)
                      << QPointF(cx, tipY)
                      << QPointF(cx + midHalfW, midY)
                      << QPointF(cx + baseHalfW, baseY);
                QLinearGradient grad(cx, tipY, cx, baseY);
                grad.setColorAt(0.0, pal.prongNormalTip);
                grad.setColorAt(1.0, pal.prongNormalMetal);
                p.setPen(QPen(QColor(30, 41, 59), 1.0));
                p.setBrush(grad);
                p.drawPolygon(blade);
                p.setPen(QPen(QColor(255, 255, 255, 140), 1.0));
                p.drawLine(QPointF(cx, tipY + 1.0), QPointF(cx, baseY - 1.0));
                _iconGlyphCountRendered++;
            } else if (iconKind == WellIconKind::SUPERPRONG) {
                double sprongH = std::min(wr.height() * 0.38, 22.0);
                double sprongW = sprongH * 0.72;
                QPointF spCenter(wr.center().x(), wr.top() + sprongH * 0.5 + 4.0);
                double cx = spCenter.x();
                double tipY = spCenter.y() - sprongH * 0.5;
                double baseY = spCenter.y() + sprongH * 0.5;
                double midY = tipY + (baseY - tipY) * 0.50;
                double baseHalfW = sprongW * 0.48;
                double midHalfW = sprongW * 0.35;
                QPolygonF sBlade;
                sBlade << QPointF(cx - baseHalfW, baseY)
                       << QPointF(cx - midHalfW, midY)
                       << QPointF(cx, tipY)
                       << QPointF(cx + midHalfW, midY)
                       << QPointF(cx + baseHalfW, baseY);
                QLinearGradient sGrad(cx, tipY, cx, baseY);
                sGrad.setColorAt(0.0, pal.prongSuperCore);
                sGrad.setColorAt(1.0, pal.prongSuperGold);
                p.setPen(QPen(QColor(120, 53, 15), 1.2));
                p.setBrush(sGrad);
                p.drawPolygon(sBlade);
                p.setPen(QPen(QColor(255, 251, 235), 1.2));
                p.drawLine(QPointF(cx, tipY + 1.0), QPointF(cx, baseY - 1.0));
                _iconGlyphCountRendered++;
            }

            // Title label
            QFont wf = p.font();
            wf.setBold(true);
            wf.setPixelSize(10);
            p.setFont(wf);
            p.setPen(pal.textSecondary);
            double labelY = wr.top() + (wr.height() > 68.0 ? 32.0 : 25.0);
            p.drawText(QRectF(wr.left(), labelY, wr.width(), 13.0),
                       Qt::AlignCenter, title);

            // Count
            wf.setPixelSize(13);
            p.setFont(wf);
            p.setPen(highlightGlow ? pal.legalStepGuide : pal.textPrimary);
            p.drawText(QRectF(wr.left(), wr.bottom() - 19.0, wr.width(), 16.0),
                       Qt::AlignCenter, QString::number(count));
        };

        DragKind activeDrag = (_canvas && isTurn) ? _canvas->activeDragKind()
                                                  : DragKind::NONE;
        drawWell(rFreePod, "Pods", freePods,
                 activeDrag == DragKind::ADD_POD_FROM_RESERVE, WellIconKind::POD);
        if (showCaptured) {
            drawWell(rLibPod, "Captured", capPods,
                     canLib || activeDrag == DragKind::LIBERATE_POD_FROM_RESERVE, WellIconKind::CAPTURED);
        }
        drawWell(rNormProng, "Prongs", freeProngs,
                 activeDrag == DragKind::ADD_PRONG_FROM_RESERVE, WellIconKind::PRONG);
        if (hasSuper) {
            drawWell(rSupProng, "Super", superProngs,
                     superProngs > 0 ||
                     activeDrag == DragKind::ADD_SUPERPRONG_FROM_RESERVE, WellIconKind::SUPERPRONG);
        }
    }

    _pttButtonRect = QRectF();

    // Quick Guide Card in 2-player mode (centered in gap between Player B and Player A)
    if (numPlayers == 2) {
        double gap = _playerCardRects[PLAYER_A].top() - _playerCardRects[PLAYER_B].bottom();
        if (gap >= 110.0) {
            double guideH = std::min(144.0, gap - 20.0);
            double guideY = _playerCardRects[PLAYER_B].bottom() + 10.0 + (gap - 20.0 - guideH) * 0.5;
            QRectF guideRect(margin, guideY, cardW, guideH);
            p.setPen(QPen(pal.panelCardBorder, 1.1));
            p.setBrush(pal.panelCardBg);
            p.drawRoundedRect(guideRect, 8.0, 8.0);

            QFont gf = p.font();
            gf.setBold(true);
            gf.setPixelSize(12);
            p.setFont(gf);
            p.setPen(pal.accentBrass);
            p.drawText(QRectF(guideRect.left() + 10.0, guideRect.top() + 6.0,
                              guideRect.width() - 20.0, 18.0),
                       Qt::AlignVCenter | Qt::AlignLeft,
                       QString::fromUtf8("Quick Guide \xE2\x80\xA2 Zoomed Square Controls"));

            gf.setBold(false);
            gf.setPixelSize(10);
            p.setFont(gf);
            p.setPen(pal.textSecondary);
            QString guideText = QString::fromUtf8(
                "\xE2\x80\xA2 Empty Home Square: Click square to Place Reserve Pod or Liberate (*).\n"
                "\xE2\x80\xA2 Friendly Pod: Click pod to open Zoom View \xE2\x80\x94 click socket (a\xE2\x80\x93h) "
                "to add/reposition prong, or click 'Move / Jump This Pod'.\n"
                "\xE2\x80\xA2 Multi-Pod Stack: Scroll mouse wheel or click Pod tabs in Zoom View.\n"
                "\xE2\x80\xA2 Guided Jump: Click green target dots, then Commit or Cancel.");
            p.drawText(QRectF(guideRect.left() + 10.0, guideRect.top() + 26.0,
                              guideRect.width() - 20.0, guideH - 30.0),
                       Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap,
                       guideText);
        }
    }

    p.restore();
}

QRectF OctiReservePanel::playerCardRect(OctiPlayer player) const
{
    if (player >= PLAYER_A && player <= PLAYER_D) {
        return _playerCardRects[player];
    }
    return QRectF();
}

QRectF OctiReservePanel::freePodWellRect() const
{
    return _freePodWellRect;
}

QRectF OctiReservePanel::liberatePodWellRect() const
{
    return _liberatePodWellRect;
}

QRectF OctiReservePanel::normalProngWellRect() const
{
    return _normalProngWellRect;
}

QRectF OctiReservePanel::superProngWellRect() const
{
    return _superProngWellRect;
}

void OctiReservePanel::paintEvent(QPaintEvent *event)
{
    (void)event;
    QPainter p(this);
    paintReserveScene(p, width(), height());
}

void OctiReservePanel::mousePressEvent(QMouseEvent *event)
{
    if (!event) {
        return;
    }
    QPointF pos = mousePosF(event);

    if (_pttButtonRect.contains(pos)) {
        setPushToTalkActive(true);
        return;
    }

    if (_canvas) {
        OctiGame *game = _board ? _board->getGame() : nullptr;
        OctiPlayer turn = game ? game->getTurn() : PLAYER_A;
        int freePods = game ? game->getFreePods(turn) : 0;
        int capPods = game ? game->getCapturedPods(turn) : 0;
        if (_freePodWellRect.contains(pos) && (freePods > 0 || capPods > 0)) {
            _canvas->clearLastMoveModifiedSquares();
            _canvas->startExternalReserveDrag(DragKind::ADD_POD_FROM_RESERVE);
        } else if (!_liberatePodWellRect.isEmpty() && _liberatePodWellRect.contains(pos) && capPods > 0) {
            _canvas->clearLastMoveModifiedSquares();
            _canvas->startExternalReserveDrag(DragKind::LIBERATE_POD_FROM_RESERVE);
        }
        update();
    }
}

void OctiReservePanel::mouseMoveEvent(QMouseEvent *event)
{
    if (!event || !_canvas || _canvas->activeDragKind() == DragKind::NONE) {
        return;
    }
    QPoint globalPt = mapToGlobal(mousePosF(event).toPoint());
    QPointF canvasPos = _canvas->mapFromGlobal(globalPt);
    _canvas->updateExternalReserveHover(canvasPos);
}

void OctiReservePanel::mouseReleaseEvent(QMouseEvent *event)
{
    if (_pttActive) {
        setPushToTalkActive(false);
    }
    if (event && _canvas && _canvas->activeDragKind() != DragKind::NONE) {
        QPointF localPos = mousePosF(event);
        if (!rect().contains(localPos.toPoint())) {
            QPoint globalPt = mapToGlobal(localPos.toPoint());
            QPointF canvasPos = _canvas->mapFromGlobal(globalPt);
            if (_canvas->rect().contains(canvasPos.toPoint())) {
                _canvas->finishExternalReserveDrop(canvasPos);
                update();
            }
        }
    }
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
