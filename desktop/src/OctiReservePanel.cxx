/*
 * OctiReservePanel.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiReservePanel.hxx"
#include "QtCompat.hxx"
#include <algorithm>
#include <QFont>
#include <QPen>

namespace octi {

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

    const OctiPalette &pal = paletteForTheme(_theme);
    p.fillRect(0, 0, width, height, pal.windowBg);

    const OctiGame *game = _board ? _board->getGame() : nullptr;
    OctiGameMode mode = _board ? _board->getGameMode() : GAME_MODE_2;
    bool isGameOver = game ? game->isGameOver() : false;
    OctiPlayer winner = isGameOver ? game->getWinner() : PLAYER_UNDEF;
    OctiPlayer activeTurn = game ? game->getTurn() : PLAYER_A;
    int numPlayers = (mode == GAME_MODE_4) ? 4 : 2;

    double margin = 10.0;
    double yCursor = margin;
    double cardW = static_cast<double>(width) - 2.0 * margin;
    double cardH = (mode == GAME_MODE_4) ? 94.0 : 126.0;

    for (int pIdx = 1; pIdx <= numPlayers; ++pIdx) {
        OctiPlayer player = static_cast<OctiPlayer>(pIdx);
        const PodFactionPalette &fp = pal.factionForPlayer(player);
        bool isTurn = !isGameOver && (player == activeTurn);
        bool isWinner = isGameOver && (player == winner ||
            (mode == GAME_MODE_4 &&
             (((winner == PLAYER_A || winner == PLAYER_C) && (player == PLAYER_A || player == PLAYER_C)) ||
              ((winner == PLAYER_B || winner == PLAYER_D) && (player == PLAYER_B || player == PLAYER_D)))));
        bool isElim = game ? game->isPlayerEliminated(player) : false;

        QRectF cardRect(margin, yCursor, cardW, cardH);
        p.setPen(QPen((isTurn || isWinner) ? pal.prongSuperGold : pal.panelCardBorder,
                      (isTurn || isWinner) ? 2.4 : 1.2));
        p.setBrush(pal.panelCardBg);
        p.drawRoundedRect(cardRect, 8.0, 8.0);

        // Player header & Fischer digital clock
        QString pName = QString("Player %1").arg(QChar('A' + pIdx - 1));
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

        double wellY = cardRect.top() + 34.0;
        double wellH = cardH - 42.0;

        QRectF rFreePod;
        QRectF rLibPod;
        QRectF rNormProng;
        QRectF rSupProng;

        if (hasSuper) {
            double wellW = (cardW - 30.0) / 4.0;
            rFreePod = QRectF(cardRect.left() + 6.0, wellY, wellW, wellH);
            rLibPod = QRectF(rFreePod.right() + 6.0, wellY, wellW, wellH);
            rNormProng = QRectF(rLibPod.right() + 6.0, wellY, wellW, wellH);
            rSupProng = QRectF(rNormProng.right() + 6.0, wellY, wellW, wellH);
        } else {
            double wellW = (cardW - 24.0) / 3.0;
            rFreePod = QRectF(cardRect.left() + 6.0, wellY, wellW, wellH);
            rLibPod = QRectF(rFreePod.right() + 6.0, wellY, wellW, wellH);
            rNormProng = QRectF(rLibPod.right() + 6.0, wellY, wellW, wellH);
            rSupProng = QRectF();
        }

        if (isTurn || (isGameOver && player == activeTurn)) {
            _freePodWellRect = rFreePod;
            _liberatePodWellRect = rLibPod;
            _normalProngWellRect = rNormProng;
            _superProngWellRect = rSupProng;
        }

        auto drawWell = [&](const QRectF &wr, const QString &title,
                            int count, bool highlightGlow) {
            p.setPen(QPen(highlightGlow ? pal.legalStepGuide : pal.panelCardBorder,
                          highlightGlow ? 2.0 : 1.0));
            p.setBrush(pal.windowBg);
            p.drawRoundedRect(wr, 5.0, 5.0);

            QFont wf = p.font();
            wf.setBold(true);
            wf.setPixelSize(10);
            p.setFont(wf);
            p.setPen(pal.textSecondary);
            p.drawText(QRectF(wr.left(), wr.top() + 3.0, wr.width(), 14.0),
                       Qt::AlignCenter, title);

            wf.setPixelSize(14);
            p.setFont(wf);
            p.setPen(highlightGlow ? pal.legalStepGuide : pal.textPrimary);
            p.drawText(QRectF(wr.left(), wr.bottom() - 22.0, wr.width(), 18.0),
                       Qt::AlignCenter, QString::number(count));
        };

        DragKind activeDrag = (_canvas && isTurn) ? _canvas->activeDragKind()
                                                  : DragKind::NONE;
        drawWell(rFreePod, "Pods (+)", freePods,
                 activeDrag == DragKind::ADD_POD_FROM_RESERVE);
        drawWell(rLibPod, "Captured", capPods,
                 canLib || activeDrag == DragKind::LIBERATE_POD_FROM_RESERVE);
        drawWell(rNormProng, "Prongs", freeProngs,
                 activeDrag == DragKind::ADD_PRONG_FROM_RESERVE);
        if (hasSuper) {
            drawWell(rSupProng, "Super (!)", superProngs,
                     superProngs > 0 ||
                     activeDrag == DragKind::ADD_SUPERPRONG_FROM_RESERVE);
        }

        yCursor += cardH + 10.0;
    }

    _pttButtonRect = QRectF();

    // Tactical Controls Guide Card (shown when vertical room permits)
    double availGuideH = static_cast<double>(height) - yCursor - margin;
    if (availGuideH >= 86.0) {
        double guideH = std::min(144.0, availGuideH);
        QRectF guideRect(margin, yCursor, cardW, guideH);
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
            "\xE2\x80\xA2 Empty Home Square: Click square to Place Reserve (+) or Liberate (*).\n"
            "\xE2\x80\xA2 Friendly Pod: Click pod to open Zoom View \xE2\x80\x94 click socket (a\xE2\x80\x93h) "
            "to add/reposition prong, or click 'Move / Jump This Pod'.\n"
            "\xE2\x80\xA2 Multi-Pod Stack: Scroll mouse wheel or click Pod tabs in Zoom View.\n"
            "\xE2\x80\xA2 Guided Jump: Click green target dots, then Commit or Cancel.");
        p.drawText(QRectF(guideRect.left() + 10.0, guideRect.top() + 26.0,
                          guideRect.width() - 20.0, guideH - 30.0),
                   Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap,
                   guideText);
    }

    p.restore();
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
        if (_freePodWellRect.contains(pos)) {
            _canvas->startExternalReserveDrag(DragKind::ADD_POD_FROM_RESERVE);
        } else if (_liberatePodWellRect.contains(pos)) {
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
