/*
 * OctiBoardCanvas.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiBoardCanvas.hxx"
#include "OctiDialogs.hxx"
#include "QtCompat.hxx"
#include <algorithm>
#include <cmath>
#include <QApplication>
#include <QCoreApplication>
#include <QFont>
#include <QLineF>
#include <QLinearGradient>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>
#include <QRadialGradient>

namespace octi {

OctiBoardCanvas::OctiBoardCanvas(QWidget *parent)
    : QWidget(parent)
    , _board(nullptr)
    , _composer(nullptr)
    , _soundSynth(nullptr)
    , _theme(ThemeId::OBSIDIAN_BRASS)
    , _rotation(BoardRotation::ROT_0)
    , _geometry(800, 800, 9, 9, BoardRotation::ROT_0)
    , _dragKind(DragKind::NONE)
    , _dragStartX(-1)
    , _dragStartY(-1)
    , _dragPodIndex(-1)
    , _dragSrcProngDir(DIR_UNDEF)
    , _dragHoverProngDir(DIR_UNDEF)
    , _dragShiftHeld(false)
    , _hoverSquareX(-1)
    , _hoverSquareY(-1)
    , _selectedSquareX(-1)
    , _selectedSquareY(-1)
    , _selectedPodIndex(-1)
    , _stackFanX(-1)
    , _stackFanY(-1)
    , _dragJumpedSquareX(-1)
    , _dragJumpedSquareY(-1)
    , _animActive(false)
    , _animProgress(0.0)
    , _animPodType(A_POD)
    , _boardZoomScale(1.0)
    , _boardZoomFocus(400.0, 400.0)
    , _boardPanActive(false)
    , _lastPanRawPos(0.0, 0.0)
    , _handRotateActive(false)
    , _leftMouseHeld(false)
    , _liveRotationOffsetDeg(0.0)
    , _pressRawPos(0.0, 0.0)
    , _handRotateRefAngleDeg(0.0)
{
    setMinimumSize(420, 420);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    _holdRotateTimer.setSingleShot(true);
    QObject::connect(&_holdRotateTimer, &QTimer::timeout, [this]() {
        if (!_leftMouseHeld) {
            return;
        }
        _handRotateActive = true;
        _liveRotationOffsetDeg = 0.0;
        if (_composer && _composer->isGuidedMoveActive() &&
            !_composer->isAwaitingConfirmation() &&
            _composer->guidedCurrentSquare() == _composer->guidedOriginSquare()) {
            _composer->cancelAndRollback();
        }
        _dragKind = DragKind::NONE;
        _dragStartX = -1;
        _dragStartY = -1;
        _dragPodIndex = -1;
        setCursor(Qt::OpenHandCursor);
        QPointF center = _geometry.boardOuterRect().center();
        _handRotateRefAngleDeg = std::atan2(
            _pressRawPos.y() - center.y(),
            _pressRawPos.x() - center.x()) * 180.0 / 3.14159265358979323846;
        update();
    });

    QObject::connect(&_animTimer, &QTimer::timeout, [this]() {
        _animProgress += 0.12;
        if (_animProgress >= 1.0) {
            _animProgress = 1.0;
            _animActive = false;
            _animTimer.stop();
        }
        update();
    });
}

OctiBoardCanvas::~OctiBoardCanvas()
{
    _animTimer.stop();
    _holdRotateTimer.stop();
}

void OctiBoardCanvas::cancelHandRotateHold()
{
    _holdRotateTimer.stop();
    _leftMouseHeld = false;
    if (_handRotateActive || std::abs(_liveRotationOffsetDeg) > 0.001) {
        _handRotateActive = false;
        _liveRotationOffsetDeg = 0.0;
        unsetCursor();
    }
}

void OctiBoardCanvas::attachBoard(OctiBoard *board,
                                  OctiMoveComposer *composer,
                                  OctiSoundSynth *soundSynth)
{
    _board = board;
    _composer = composer;
    _soundSynth = soundSynth;
    _stackFanX = -1;
    _stackFanY = -1;
    _selectedSquareX = -1;
    _selectedSquareY = -1;
    _selectedPodIndex = -1;
    _dragKind = DragKind::NONE;
    syncGeometryToBoard(width(), height());
    update();
}

OctiBoard *OctiBoardCanvas::board() const
{
    return _board;
}

OctiMoveComposer *OctiBoardCanvas::composer() const
{
    return _composer;
}

OctiSoundSynth *OctiBoardCanvas::soundSynth() const
{
    return _soundSynth;
}

void OctiBoardCanvas::setTheme(ThemeId theme)
{
    _theme = theme;
    update();
}

ThemeId OctiBoardCanvas::theme() const
{
    return _theme;
}

void OctiBoardCanvas::setBoardRotation(BoardRotation rot)
{
    _rotation = rot;
    syncGeometryToBoard(width(), height());
    update();
}

BoardRotation OctiBoardCanvas::boardRotation() const
{
    return _rotation;
}

void OctiBoardCanvas::rotateClockwise()
{
    setBoardRotation(OctiBoardGeometry::nextRotationClockwise(_rotation));
}

void OctiBoardCanvas::autoOrientForSeat(OctiPlayer seat)
{
    OctiGameMode mode = _board ? _board->getGameMode() : GAME_MODE_2;
    setBoardRotation(OctiBoardGeometry::rotationForSeat(mode, seat));
}

double OctiBoardCanvas::boardZoomScale() const
{
    return _boardZoomScale;
}

void OctiBoardCanvas::setBoardZoom(double scale, const QPointF &focusPos)
{
    double clamped = std::clamp(scale, 1.0, 3.0);
    if (clamped <= 1.001) {
        resetBoardZoom();
        return;
    }
    _boardZoomScale = clamped;
    if (!focusPos.isNull()) {
        _boardZoomFocus = focusPos;
    } else if (_boardZoomFocus.isNull()) {
        _boardZoomFocus = QPointF(width() * 0.5, height() * 0.5);
    }
    update();
}

void OctiBoardCanvas::resetBoardZoom()
{
    _boardZoomScale = 1.0;
    _boardZoomFocus = QPointF(width() * 0.5, height() * 0.5);
    update();
}

QPointF OctiBoardCanvas::rawMousePosF(const QMouseEvent *event) const
{
    return octi::mousePosF(event);
}

QPointF OctiBoardCanvas::rawMousePosF(const QWheelEvent *event) const
{
    if (!event) {
        return QPointF(0.0, 0.0);
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return event->position();
#else
    return event->posF();
#endif
}

QPointF OctiBoardCanvas::mousePosF(const QMouseEvent *event) const
{
    QPointF rawPos = rawMousePosF(event);
    if (_boardZoomScale > 1.001) {
        return (rawPos - _boardZoomFocus) / _boardZoomScale + _boardZoomFocus;
    }
    return rawPos;
}

const OctiBoardGeometry &OctiBoardCanvas::geometry() const
{
    return _geometry;
}

void OctiBoardCanvas::syncGeometryToBoard(int w, int h)
{
    int gw = _board ? _board->getBoardWidth() : 9;
    int gh = _board ? _board->getBoardHeight() : 9;
    _geometry.configure(std::max(120, w), std::max(120, h), gw, gh, _rotation);
}

void OctiBoardCanvas::openStackFanAt(int x, int y)
{
    if (_board && _board->isValidCoord(x - 1, y - 1) &&
        _board->squareAt(x - 1, y - 1).getNumPods() >= 2) {
        _stackFanX = x;
        _stackFanY = y;
        update();
    }
}

void OctiBoardCanvas::closeStackFan()
{
    _stackFanX = -1;
    _stackFanY = -1;
    update();
}

std::pair<int, int> OctiBoardCanvas::activeStackFanSquare() const
{
    return {_stackFanX, _stackFanY};
}

void OctiBoardCanvas::startExternalReserveDrag(DragKind kind)
{
    _dragKind = kind;
    _dragStartX = -1;
    _dragStartY = -1;
    _dragPodIndex = -1;
    _selectedSquareX = -1;
    _selectedSquareY = -1;
    _selectedPodIndex = -1;
    update();
}

DragKind OctiBoardCanvas::activeDragKind() const
{
    return _dragKind;
}

void OctiBoardCanvas::updateExternalReserveHover(const QPointF &canvasPos)
{
    _dragCursorPos = canvasPos;
    int sx = -1, sy = -1;
    if (_geometry.screenToSquare(canvasPos, sx, sy)) {
        _hoverSquareX = sx;
        _hoverSquareY = sy;
        QPointF center = _geometry.squareCenter(sx, sy);
        double cell = _geometry.cellSize();
        _dragHoverProngDir = _geometry.hitTestCompassSector(
            center, cell * 0.04, cell * 0.68, canvasPos);
    } else {
        _hoverSquareX = -1;
        _hoverSquareY = -1;
        _dragHoverProngDir = DIR_UNDEF;
    }
    update();
}

bool OctiBoardCanvas::finishExternalReserveDrop(const QPointF &canvasPos)
{
    if (_dragKind == DragKind::NONE || !_composer) {
        _dragKind = DragKind::NONE;
        return false;
    }

    int tx = -1, ty = -1;
    bool onBoard = _geometry.screenToSquare(canvasPos, tx, ty);
    bool ok = false;

    if (onBoard) {
        if (_dragKind == DragKind::ADD_POD_FROM_RESERVE ||
            _dragKind == DragKind::LIBERATE_POD_FROM_RESERVE) {
            ok = simulateDragFromReserveToSquare(_dragKind, tx, ty);
        }
    }

    _dragKind = DragKind::NONE;
    _dragStartX = -1;
    _dragStartY = -1;
    _dragPodIndex = -1;
    _dragSrcProngDir = DIR_UNDEF;
    _dragHoverProngDir = DIR_UNDEF;
    if (isVisible() && _composer && _composer->isAwaitingConfirmation()) {
        presentPlyConfirmationIfNeeded();
    }
    update();
    return ok;
}

void OctiBoardCanvas::triggerMoveAnimation(const QPointF &fromPt,
                                           const QPointF &toPt,
                                           OctiPodType podType,
                                           const OctiPod &pod)
{
    _animFromPt = fromPt;
    _animToPt = toPt;
    _animPodType = podType;
    _animPod = pod;
    _animProgress = 0.0;
    _animActive = true;
    _animTimer.start(16);
}

bool OctiBoardCanvas::simulateDragFromReserveToSquare(DragKind kind,
                                                      int targetX, int targetY)
{
    if (!_composer) {
        return false;
    }
    bool liberate = (kind == DragKind::LIBERATE_POD_FROM_RESERVE);
    bool ok = _composer->stageOrCommitAddPod(targetX, targetY, liberate);
    if (ok && _soundSynth) {
        _soundSynth->triggerCue(SoundCue::POD_PLACE);
    }
    update();
    return ok;
}

bool OctiBoardCanvas::simulateDragProngToPod(int x, int y,
                                             int podIndex,
                                             OctiDir dir,
                                             bool superProng)
{
    if (!_composer) {
        return false;
    }
    bool ok = _composer->stageOrCommitAddProng(x, y, podIndex, dir, superProng);
    if (ok && _soundSynth) {
        _soundSynth->triggerCue(SoundCue::PRONG_SNAP);
    }
    update();
    return ok;
}

bool OctiBoardCanvas::simulateDragPodToSquare(int fromX, int fromY,
                                              int podIndex,
                                              int toX, int toY,
                                              bool moveAllStack)
{
    if (!_composer || !_board) {
        return false;
    }

    QPointF fromPt = _geometry.squareCenter(fromX, fromY);
    QPointF toPt = _geometry.squareCenter(toX, toY);
    OctiPod animPod;
    OctiPodType animType = A_POD;
    if (_board->isValidCoord(fromX - 1, fromY - 1) &&
        !_board->squareAt(fromX - 1, fromY - 1).isEmpty()) {
        const OctiSquare &sq = _board->squareAt(fromX - 1, fromY - 1);
        int idx = std::clamp(podIndex < 0 ? sq.getNumPods() - 1 : podIndex,
                             0, sq.getNumPods() - 1);
        animPod = sq.getPodAt(idx);
        animType = animPod.getType();
    }

    bool ok = _composer->stageOrCommitPodMoveOrJump(
        fromX, fromY, podIndex, toX, toY, moveAllStack);
    if (ok) {
        triggerMoveAnimation(fromPt, toPt, animType, animPod);
        if (_soundSynth) {
            int dist = std::max(std::abs(toX - fromX), std::abs(toY - fromY));
            if (!_composer->captureChoices().empty()) {
                _soundSynth->triggerCue(SoundCue::POD_CAPTURE);
            } else if (dist >= 2) {
                _soundSynth->triggerCue(SoundCue::POD_JUMP);
            } else {
                _soundSynth->triggerCue(SoundCue::POD_STEP);
            }
        }
    }
    update();
    return ok;
}

bool OctiBoardCanvas::simulateRepositionProngOnPod(int x, int y,
                                                   int podIndex,
                                                   OctiDir srcDir,
                                                   OctiDir tgtDir)
{
    if (!_composer) {
        return false;
    }
    bool ok = _composer->stageOrCommitRepositionProng(x, y, podIndex, srcDir, tgtDir);
    if (ok && _soundSynth) {
        _soundSynth->triggerCue(SoundCue::PRONG_SNAP);
    }
    update();
    return ok;
}

QImage OctiBoardCanvas::renderToImage(int width, int height)
{
    int w = std::max(120, width);
    int h = std::max(120, height);
    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(paletteForTheme(_theme).windowBg);

    QPainter painter(&img);
    if (std::abs(_liveRotationOffsetDeg) > 0.01) {
        QPointF c = _geometry.boardOuterRect().center();
        painter.translate(c);
        painter.rotate(_liveRotationOffsetDeg);
        painter.translate(-c);
    }
    paintBoardScene(painter, w, h);
    painter.end();

    return img;
}

void OctiBoardCanvas::paintEdgelessPortals(QPainter &p,
                                           const OctiBoardGeometry &geom,
                                           const OctiPalette &pal) const
{
    p.save();
    QRectF grid = geom.gridInnerRect();
    double cell = geom.cellSize();

    QPen portalPen(pal.edgelessPortalGlow, 2.2, Qt::DashDotLine);
    p.setPen(portalPen);
    p.setBrush(Qt::NoBrush);
    p.drawRect(grid.adjusted(-3.0, -3.0, 3.0, 3.0));

    // Draw portal chevrons along all 4 edges
    int gw = geom.gridWidth();
    int gh = geom.gridHeight();
    p.setPen(QPen(pal.edgelessPortalGlow, 1.8));
    for (int x = 1; x <= gw; ++x) {
        QRectF rBot = geom.squareRect(x, 1);
        QRectF rTop = geom.squareRect(x, gh);
        p.drawEllipse(QPointF(rBot.center().x(), grid.bottom() + cell * 0.10), 3.0, 3.0);
        p.drawEllipse(QPointF(rTop.center().x(), grid.top() - cell * 0.10), 3.0, 3.0);
    }
    for (int y = 1; y <= gh; ++y) {
        QRectF rLeft = geom.squareRect(1, y);
        QRectF rRight = geom.squareRect(gw, y);
        p.drawEllipse(QPointF(grid.left() - cell * 0.10, rLeft.center().y()), 3.0, 3.0);
        p.drawEllipse(QPointF(grid.right() + cell * 0.10, rRight.center().y()), 3.0, 3.0);
    }
    p.restore();
}

void OctiBoardCanvas::paintHomeSquareBadge(QPainter &p,
                                           const QRectF &sqRect,
                                           const OctiSquare &sq,
                                           const OctiPalette &pal) const
{
    p.save();
    OctiPlayer owner = PLAYER_A;
    switch (sq.getType()) {
    case A_HOME_SQUARE: owner = PLAYER_A; break;
    case B_HOME_SQUARE: owner = PLAYER_B; break;
    case C_HOME_SQUARE: owner = PLAYER_C; break;
    case D_HOME_SQUARE: owner = PLAYER_D; break;
    default:            break;
    }

    bool isRosewood = (_theme == ThemeId::ROSEWOOD_IVORY);
    QColor outerBg = isRosewood ? QColor(20, 14, 11) : QColor(10, 14, 22);
    QColor innerBg = isRosewood ? QColor(14, 10, 8) : QColor(7, 10, 16);

    p.fillRect(sqRect.adjusted(1.0, 1.0, -1.0, -1.0), outerBg);

    QRectF inner = sqRect.adjusted(3.5, 3.5, -3.5, -3.5);
    if (sq.isCaptured()) {
        p.setPen(QPen(pal.legalCaptureGlow, 2.4));
    } else {
        p.setPen(QPen(pal.boardBevelDark, 1.2));
    }
    p.setBrush(innerBg);
    p.drawRoundedRect(inner, 4.0, 4.0);

    if (sq.isEmpty()) {
        OctiGameMode mode = (_board && _board->getGame()) ? _board->getGameMode() : GAME_MODE_2;
        BoardRotation seatRot = OctiBoardGeometry::rotationForSeat(mode, owner);
        int textAngleCwDeg = ((static_cast<int>(seatRot) - static_cast<int>(_rotation)) % 360 + 360) % 360;

        p.save();
        p.translate(sqRect.center());
        p.rotate(textAngleCwDeg);
        QFont f = p.font();
        f.setBold(true);
        f.setPixelSize(std::max(8, static_cast<int>(sqRect.height() * 0.20)));
        p.setFont(f);
        p.setPen(pal.coordLabel);
        p.drawText(QRectF(-inner.width() * 0.5, -inner.height() * 0.5, inner.width(), inner.height()),
                   Qt::AlignCenter, "OCTI");
        p.restore();
    }

    p.restore();
}

void OctiBoardCanvas::paintNeutralOctiMedallion(QPainter &p,
                                                const QRectF &sqRect,
                                                const OctiSquare &sq,
                                                const OctiPalette &pal) const
{
    p.save();
    QRectF inner = sqRect.adjusted(4.0, 4.0, -4.0, -4.0);
    p.setPen(QPen(pal.neutralOctiRim, 2.0, Qt::DashLine));
    p.setBrush(pal.neutralOctiFill);
    p.drawRoundedRect(inner, 6.0, 6.0);

    if (sq.isEmpty()) {
        QFont f = p.font();
        f.setBold(true);
        f.setPixelSize(std::max(8, static_cast<int>(sqRect.height() * 0.18)));
        p.setFont(f);
        p.setPen(pal.neutralOctiRim);
        p.drawText(inner, Qt::AlignCenter, "OCTI*");
    }
    p.restore();
}

void OctiBoardCanvas::paintCompassProngWheel(QPainter &p,
                                             const OctiBoardGeometry &geom,
                                             int x, int y, int podIndex,
                                             bool superProng,
                                             OctiDir hoverDir,
                                             const OctiPalette &pal) const
{
    if (!_board || !_board->isValidCoord(x - 1, y - 1)) {
        return;
    }
    const OctiSquare &sq = _board->squareAt(x - 1, y - 1);
    if (sq.isEmpty()) {
        return;
    }

    p.save();
    QPointF center = geom.squareCenter(x, y);
    double cell = geom.cellSize();
    double slotDist = cell * 0.36;

    std::vector<OctiDir> legalDirs;
    if (_composer) {
        legalDirs = _composer->legalProngAdditionsAt(x, y, podIndex, superProng);
    }

    p.setPen(QPen(pal.accentBrass, 1.2, Qt::DotLine));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(center, slotDist, slotDist);

    for (int d = DIR_N; d <= DIR_NW; ++d) {
        OctiDir dir = static_cast<OctiDir>(d);
        QPointF u = geom.dirUnitVectorScreen(dir);
        QPointF slotPt = center + u * slotDist;
        bool isLegal = std::find(legalDirs.begin(), legalDirs.end(), dir) != legalDirs.end();
        bool isHover = (dir == hoverDir);

        double dotR = isHover ? cell * 0.11 : cell * 0.088;
        if (isHover && isLegal) {
            p.setPen(QPen(pal.prongSuperCore, 1.8));
            p.setBrush(superProng ? pal.prongSuperGold : pal.legalStepGuide);
        } else if (isLegal) {
            p.setPen(QPen(pal.accentBrass, 1.2));
            p.setBrush(QColor(22, 42, 62, 215));
        } else {
            p.setPen(QPen(pal.textSecondary, 0.8));
            p.setBrush(QColor(20, 26, 36, 110));
        }
        p.drawEllipse(slotPt, dotR, dotR);

        QFont f = p.font();
        f.setBold(true);
        f.setPixelSize(std::max(8, static_cast<int>(cell * 0.13)));
        p.setFont(f);
        p.setPen(isHover && isLegal ? QColor(12, 18, 28) : pal.textPrimary);
        QString dStr(QChar(dirToChar(dir)));
        p.drawText(QRectF(slotPt.x() - dotR, slotPt.y() - dotR, dotR * 2.0, dotR * 2.0),
                   Qt::AlignCenter, dStr);
    }

    p.restore();
}

void OctiBoardCanvas::paintStackFanOverlay(QPainter &p,
                                           const OctiBoardGeometry &geom,
                                           int x, int y,
                                           const OctiPalette &pal) const
{
    if (!_board || !_board->isValidCoord(x - 1, y - 1)) {
        return;
    }
    const OctiSquare &sq = _board->squareAt(x - 1, y - 1);
    int n = sq.getNumPods();
    if (n < 2) {
        return;
    }

    p.save();
    QRectF overlay = geom.stackFanOverlayRect(x, y, n);
    p.setPen(QPen(pal.accentBrass, 1.8));
    p.setBrush(QColor(15, 22, 34, 235));
    p.drawRoundedRect(overlay, 8.0, 8.0);

    auto sorted = sortedPodIndicesMostProngsFirst(sq);
    int selIdx = _composer ? _composer->selectedStackPodIndex(x, y) : (n > 0 ? sorted[0] : -1);
    for (int i = 0; i < n; ++i) {
        int podIdx = (i < static_cast<int>(sorted.size())) ? sorted[i] : i;
        QRectF slot = geom.stackFanSlotRect(x, y, n, i);
        bool selected = (podIdx == selIdx);
        if (selected) {
            p.setPen(QPen(pal.prongSuperGold, 2.0));
            p.setBrush(QColor(255, 255, 255, 28));
            p.drawRoundedRect(slot, 5.0, 5.0);
        }
        const OctiPod &pod = sq.getPodAt(podIdx);
        paintOctagonalPod(p, slot, pod.getType(), pod, pal,
                          geom.rotationDegrees(), selected, 1.0);
    }
    p.restore();
}

void OctiBoardCanvas::paintCaptureChoicePills(QPainter &p,
                                              const OctiBoardGeometry &geom,
                                              const OctiPalette &pal)
{
    if (!_composer) {
        return;
    }
    const auto &choices = _composer->captureChoices();
    if (choices.empty()) {
        return;
    }

    p.save();
    for (const auto &cc : choices) {
        QRectF sqRect = geom.squareRect(cc.jumpedX, cc.jumpedY);
        if (sqRect.isEmpty()) {
            continue;
        }
        QRectF pill(sqRect.left() + 2.0,
                    sqRect.top() + 2.0,
                    sqRect.width() - 4.0,
                    std::max(16.0, sqRect.height() * 0.28));

        QColor bg = cc.capture ? pal.legalCaptureGlow : pal.panelCardBg;
        p.setPen(QPen(pal.prongSuperCore, 1.2));
        p.setBrush(bg);
        p.drawRoundedRect(pill, 4.0, 4.0);

        QFont f = p.font();
        f.setBold(true);
        f.setPixelSize(std::max(8, static_cast<int>(pill.height() * 0.65)));
        p.setFont(f);
        p.setPen(Qt::white);
        QString text = cc.capture ? "Capture [x]" : "Keep [ ]";
        p.drawText(pill, Qt::AlignCenter, text);
    }
    p.restore();
}

void OctiBoardCanvas::paintCommitBanner(QPainter &p,
                                        const OctiBoardGeometry &geom,
                                        const OctiPalette &pal)
{
    _commitBtnRect = QRectF();
    _rollbackBtnRect = QRectF();
    _finishBtnRect = QRectF();
    if (!_composer || !_composer->isAwaitingConfirmation()) {
        return;
    }

    p.save();
    QRectF outer = geom.boardOuterRect();
    double barW = std::min(outer.width() * 0.92, 580.0);
    double barH = 38.0;
    QRectF barRect(outer.center().x() - barW * 0.5,
                   outer.bottom() - barH - 6.0,
                   barW, barH);

    p.setPen(QPen(pal.accentBrass, 1.8));
    p.setBrush(QColor(14, 20, 32, 238));
    p.drawRoundedRect(barRect, 7.0, 7.0);

    QString staged = QString::fromStdString(_composer->stagedMoveNotation());
    if (_composer->isAwaitingSuperProngBonusMove()) {
        staged += " (8th Superprong Bonus Move Ready)";
    } else if (_composer->hasOtherMovablePodAtOrigin()) {
        staged += " (Click Origin Square to Move Next Pod)";
    } else if (_composer->isGuidedMoveActive()) {
        staged += " (Click Destination to Jump or Commit)";
    }

    QFont f = p.font();
    f.setBold(true);
    f.setPixelSize(11);
    p.setFont(f);
    p.setPen(pal.textPrimary);
    double textW = barRect.width() * 0.48;
    QRectF textRect(barRect.left() + 10.0, barRect.top(),
                    textW, barRect.height());
    p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
               QString("Staged: %1").arg(staged));

    double btnH = barH - 10.0;
    double btnW = barRect.width() * 0.22;
    _commitBtnRect = QRectF(barRect.right() - btnW * 2.0 - 16.0,
                            barRect.top() + 5.0, btnW, btnH);
    _rollbackBtnRect = QRectF(barRect.right() - btnW - 8.0,
                              barRect.top() + 5.0, btnW, btnH);

    p.setPen(QPen(pal.prongSuperCore, 1.2));
    p.setBrush(pal.legalStepGuide.darker(135));
    p.drawRoundedRect(_commitBtnRect, 5.0, 5.0);
    p.setPen(Qt::white);
    p.drawText(_commitBtnRect, Qt::AlignCenter, "Commit [Enter]");

    p.setPen(QPen(pal.accentBrass, 1.2));
    p.setBrush(pal.legalCaptureGlow.darker(150));
    p.drawRoundedRect(_rollbackBtnRect, 5.0, 5.0);
    p.setPen(Qt::white);
    p.drawText(_rollbackBtnRect, Qt::AlignCenter, "Rollback [Esc]");

    p.restore();
}

void OctiBoardCanvas::paintBoardScene(QPainter &p, int width, int height)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const OctiPalette &pal = paletteForTheme(_theme);
    p.fillRect(0, 0, width, height, pal.windowBg);

    const OctiBoard *displayBoard = _board;
    OctiBoard previewBoard;
    if (_composer && _composer->isAwaitingConfirmation()) {
        if (_composer->populatePreviewBoard(previewBoard)) {
            displayBoard = &previewBoard;
        }
    }

    int gw = displayBoard ? displayBoard->getBoardWidth() : 9;
    int gh = displayBoard ? displayBoard->getBoardHeight() : 9;
    OctiBoardGeometry geom(width, height, gw, gh, _rotation);

    // 1. Luxury board outer frame with 3D brass bevel
    QRectF outer = geom.boardOuterRect();
    QRectF grid = geom.gridInnerRect();

    QLinearGradient frameGrad(outer.topLeft(), outer.bottomRight());
    frameGrad.setColorAt(0.0, pal.boardOuterFrame.lighter(120));
    frameGrad.setColorAt(0.5, pal.boardOuterFrame);
    frameGrad.setColorAt(1.0, pal.boardOuterFrame.darker(130));

    p.setPen(QPen(pal.boardBevelLight, 2.6));
    p.setBrush(frameGrad);
    p.drawRoundedRect(outer, 10.0, 10.0);

    p.setPen(QPen(pal.boardBevelDark, 1.6));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(outer.adjusted(3.0, 3.0, -3.0, -3.0), 8.0, 8.0);

    // 2. Coordinate gutter labels (1..gw, 1..gh) rotated with board
    double gutter = (outer.width() - grid.width()) * 0.5;
    QFont coordFont = p.font();
    coordFont.setBold(true);
    coordFont.setPixelSize(std::max(9, static_cast<int>(geom.cellSize() * 0.24)));
    p.setFont(coordFont);
    p.setPen(pal.coordLabel);

    for (int x = 1; x <= gw; ++x) {
        QRectF rBot = geom.squareRect(x, 1);
        QRectF rTop = geom.squareRect(x, gh);
        if (_rotation == BoardRotation::ROT_0 || _rotation == BoardRotation::ROT_180) {
            QRectF lblBot(rBot.left(), grid.bottom() + 2.0, rBot.width(), gutter - 4.0);
            QRectF lblTop(rTop.left(), outer.top() + 2.0, rTop.width(), gutter - 4.0);
            p.drawText(lblBot, Qt::AlignCenter, QString::number(x));
            p.drawText(lblTop, Qt::AlignCenter, QString::number(x));
        } else {
            QRectF lblL(outer.left() + 2.0, rBot.top(), gutter - 4.0, rBot.height());
            QRectF lblR(grid.right() + 2.0, rTop.top(), gutter - 4.0, rTop.height());
            p.drawText(lblL, Qt::AlignCenter, QString::number(x));
            p.drawText(lblR, Qt::AlignCenter, QString::number(x));
        }
    }

    for (int y = 1; y <= gh; ++y) {
        QRectF rLeft = geom.squareRect(1, y);
        QRectF rRight = geom.squareRect(gw, y);
        if (_rotation == BoardRotation::ROT_0 || _rotation == BoardRotation::ROT_180) {
            QRectF lblL(outer.left() + 2.0, rLeft.top(), gutter - 4.0, rLeft.height());
            QRectF lblR(grid.right() + 2.0, rRight.top(), gutter - 4.0, rRight.height());
            p.drawText(lblL, Qt::AlignCenter, QString::number(y));
            p.drawText(lblR, Qt::AlignCenter, QString::number(y));
        } else {
            QRectF lblBot(rLeft.left(), grid.bottom() + 2.0, rLeft.width(), gutter - 4.0);
            QRectF lblTop(rRight.left(), outer.top() + 2.0, rRight.width(), gutter - 4.0);
            p.drawText(lblBot, Qt::AlignCenter, QString::number(y));
            p.drawText(lblTop, Qt::AlignCenter, QString::number(y));
        }
    }

    // 4. Edgeless board portal glow
    if (displayBoard && displayBoard->getGame() && displayBoard->getGame()->useEdgelessBoard()) {
        paintEdgelessPortals(p, geom, pal);
    }

    // 5. Grid squares, recessed sockets, home base shields, and neutral OCTI medallions
    for (int y = 1; y <= gh; ++y) {
        for (int x = 1; x <= gw; ++x) {
            QRectF sqRect = geom.squareRect(x, y);
            bool isDark = ((x + y) % 2 == 0);
            p.setPen(QPen(pal.gridLine, 1.0));
            p.setBrush(isDark ? pal.gridCellDark : pal.gridCellLight);
            p.drawRect(sqRect);

            // Subtle recessed socket ring
            double socketR = sqRect.width() * 0.34;
            p.setPen(QPen(pal.gridSocketShadow, 1.1));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(sqRect.center(), socketR, socketR);

            if (displayBoard && displayBoard->isValidCoord(x - 1, y - 1)) {
                const OctiSquare &sq = displayBoard->squareAt(x - 1, y - 1);
                OctiSquareType st = sq.getType();
                if (st == A_HOME_SQUARE || st == B_HOME_SQUARE ||
                    st == C_HOME_SQUARE || st == D_HOME_SQUARE) {
                    paintHomeSquareBadge(p, sqRect, sq, pal);
                } else if (st == NEUTRAL_OCTI_SQUARE) {
                    paintNeutralOctiMedallion(p, sqRect, sq, pal);
                }
            }
        }
    }

    // 6. Legal destination guides when dragging or guided move active (GUI2-06)
    if (_composer) {
        std::vector<std::pair<int, int>> guides;
        if (_composer->isGuidedMoveActive()) {
            guides = _composer->guidedNextSquares();
        } else if (_dragKind == DragKind::ADD_POD_FROM_RESERVE) {
            guides = _composer->legalPodDropSquares(false);
        } else if (_dragKind == DragKind::LIBERATE_POD_FROM_RESERVE) {
            guides = _composer->legalPodDropSquares(true);
        } else if (_dragKind == DragKind::MOVE_OR_JUMP_POD &&
                   _dragStartX >= 1 && _dragStartY >= 1) {
            guides = _composer->legalDestinationsFrom(_dragStartX, _dragStartY, _dragPodIndex);
        }

        for (const auto &dest : guides) {
            QRectF r = geom.squareRect(dest.first, dest.second);
            if (!r.isEmpty()) {
                double rad = r.width() * 0.16;
                p.setPen(QPen(pal.legalStepGuide, 2.0));
                QColor fill = pal.legalStepGuide;
                fill.setAlpha(95);
                p.setBrush(fill);
                p.drawEllipse(r.center(), rad, rad);
            }
        }
    }

    if (_composer && _composer->isAwaitingConfirmation() && _composer->hasOtherMovablePodAtOrigin()) {
        std::pair<int, int> orig = _composer->guidedOriginSquare();
        QRectF origRect = geom.squareRect(orig.first, orig.second);
        if (!origRect.isEmpty()) {
            p.save();
            QPen ringPen(pal.accentBrass.lighter(130), 2.4, Qt::DashLine);
            p.setPen(ringPen);
            p.setBrush(QColor(0, 220, 255, 30));
            double rad = origRect.width() * 0.44;
            p.drawEllipse(origRect.center(), rad, rad);
            p.restore();
        }
    }

    // 7. Pods, prongs, superprongs, and layered 3D-offset multi-pod stacks with count badges
    if (displayBoard) {
        for (int y = 1; y <= gh; ++y) {
            for (int x = 1; x <= gw; ++x) {
                const OctiSquare &sq = displayBoard->squareAt(x - 1, y - 1);
                int numPods = sq.getNumPods();
                if (numPods <= 0) {
                    continue;
                }

                QRectF sqRect = geom.squareRect(x, y);
                auto sorted = sortedPodIndicesMostProngsFirst(sq);
                int maxLayers = std::min(numPods, 4);

                for (int layer = 0; layer < maxLayers; ++layer) {
                    int podIdx = (layer == maxLayers - 1) ? sorted.back() : sorted[layer];
                    podIdx = std::clamp(podIdx, 0, numPods - 1);
                    const OctiPod &pod = sq.getPodAt(podIdx);
                    double offset = static_cast<double>(maxLayers - 1 - layer) * 2.6;
                    QRectF layerRect = sqRect.translated(-offset, offset);
                    bool hi = (x == _hoverSquareX && y == _hoverSquareY &&
                               layer == maxLayers - 1);
                    QString centerOverride;
                    if (numPods >= 2 && layer == maxLayers - 1) {
                        centerOverride = QString("x%1").arg(numPods);
                    }
                    paintOctagonalPod(p, layerRect, pod.getType(), pod, pal,
                                      geom.rotationDegrees(), hi, 1.0, centerOverride);
                }
            }
        }
    }

    if (_composer && _composer->isAwaitingConfirmation()) {
        for (const auto &op : _composer->stagedOps()) {
            if (op.addPod) {
                QRectF r = geom.squareRect(op.x0 + 1, op.y0 + 1);
                if (!r.isEmpty()) {
                    p.save();
                    p.setPen(QPen(pal.prongSuperGold, 2.2, Qt::DashLine));
                    p.setBrush(Qt::NoBrush);
                    p.drawRoundedRect(r.adjusted(3, 3, -3, -3), 6.0, 6.0);
                    p.restore();
                }
            }
        }
    }

    // 8. Smooth move transition animation
    if (_animActive) {
        QPointF curPt = _animFromPt + (_animToPt - _animFromPt) * _animProgress;
        double cs = geom.cellSize();
        QRectF animRect(curPt.x() - cs * 0.5, curPt.y() - cs * 0.5, cs, cs);
        paintOctagonalPod(p, animRect, _animPodType, _animPod, pal,
                          geom.rotationDegrees(), true, 0.92);
    }

    // 9. Floating Capture [x] / Keep [ ] pills & Commit Move / Rollback banner
    paintCaptureChoicePills(p, geom, pal);
    paintCommitBanner(p, geom, pal);

    // 10. Persistent Game Over Victory Banner (PCE-D06)
    if (displayBoard && displayBoard->getGame() && displayBoard->getGame()->isGameOver()) {
        double banW = std::min(outer.width() * 0.88, 520.0);
        double banH = 44.0;
        QRectF banRect(outer.center().x() - banW * 0.5,
                       outer.top() + 10.0,
                       banW, banH);
        p.setPen(QPen(pal.prongSuperGold, 2.2));
        p.setBrush(QColor(12, 16, 26, 235));
        p.drawRoundedRect(banRect, 8.0, 8.0);

        QFont vf("SansSerif", 13, QFont::Bold);
        p.setFont(vf);
        p.setPen(pal.prongSuperGold);
        p.drawText(banRect, Qt::AlignCenter,
                   GameOverDialog::describeWinnerBanner(displayBoard));
    }

    p.restore();
}

void OctiBoardCanvas::paintEvent(QPaintEvent *event)
{
    (void)event;
    syncGeometryToBoard(width(), height());
    QPainter p(this);
    if (_boardZoomScale > 1.001) {
        p.fillRect(0, 0, width(), height(), paletteForTheme(_theme).windowBg);
        p.translate(_boardZoomFocus);
        p.scale(_boardZoomScale, _boardZoomScale);
        p.translate(-_boardZoomFocus);
    }
    if (std::abs(_liveRotationOffsetDeg) > 0.01) {
        QPointF c = _geometry.boardOuterRect().center();
        p.translate(c);
        p.rotate(_liveRotationOffsetDeg);
        p.translate(-c);
    }
    paintBoardScene(p, width(), height());
}

void OctiBoardCanvas::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    syncGeometryToBoard(width(), height());
}

bool OctiBoardCanvas::openZoomedSquareAt(int sx, int sy)
{
    cancelHandRotateHold();
    if (!_board || !_board->isValidCoord(sx - 1, sy - 1)) {
        return false;
    }
    const OctiSquare &sq = _board->squareAt(sx - 1, sy - 1);
    bool isNonEmpty = !sq.isEmpty();
    bool isEligibleHome = false;
    if (_board->getGame() && _board->getGame()->isGameStarted() &&
        _board->getGameMode() != GAME_MODE_OFK) {
        OctiPlayer turn = _board->getGame()->getTurn();
        bool isFriendlyBase = false;
        if (_board->getNumPlayers() == 2) {
            isFriendlyBase = ((turn == PLAYER_A && sq.getType() == A_HOME_SQUARE) ||
                              (turn == PLAYER_B && sq.getType() == B_HOME_SQUARE));
        } else if (_board->getNumPlayers() == 4) {
            isFriendlyBase = (sq.getType() == static_cast<OctiSquareType>(turn));
        }
        if (isFriendlyBase && !sq.hasEnemyOf(turn)) {
            if (_board->getGame()->getFreePods(turn) > 0 ||
                (_board->getGame()->getCapturedPods(turn) > 0 && _board->canLiberate(turn))) {
                isEligibleHome = true;
            }
        }
    }

    if (!isNonEmpty && !isEligibleHome) {
        return false;
    }

    bool isOriginWithRemaining = (_composer && _composer->isAwaitingConfirmation() &&
                                  _composer->hasOtherMovablePodAtOrigin() &&
                                  std::make_pair(sx, sy) == _composer->guidedOriginSquare());
    if (!isOriginWithRemaining && _composer && _composer->isGuidedMoveActive() &&
        !_composer->isAwaitingConfirmation()) {
        _composer->cancelAndRollback();
    }
    ZoomedSquareDialog dlg(_board, _composer, sx, sy, _rotation, _theme, this);
    if (dlg.exec() == QDialog::Accepted) {
        if (dlg.shouldMovePod() && _composer) {
            _composer->beginGuidedPodMove(sx, sy, dlg.selectedPodIndex(), dlg.moveAllMatchingStack());
        } else if (dlg.actionCompleted() && _composer && _composer->isAwaitingConfirmation()) {
            presentPlyConfirmationIfNeeded();
        }
    }
    update();
    return true;
}

void OctiBoardCanvas::mousePressEvent(QMouseEvent *event)
{
    cancelHandRotateHold();
    if (!event) {
        return;
    }
    if (event->button() == Qt::RightButton && _boardZoomScale > 1.001) {
        resetBoardZoom();
        return;
    }
    if (!_board || !_composer) {
        return;
    }
    if (_board->getGame() && _board->getGame()->isGameOver()) {
        return;
    }

    QPointF pos = mousePosF(event);
    _dragCursorPos = pos;
    _dragShiftHeld = (event->modifiers() & Qt::ShiftModifier) != 0;
    _pressRawPos = rawMousePosF(event);
    _lastPanRawPos = _pressRawPos;
    _boardPanActive = false;

    if (event->button() == Qt::RightButton) {
        int rx = 0, ry = 0;
        if (_geometry.screenToSquare(pos, rx, ry)) {
            openZoomedSquareAt(rx, ry);
        }
        return;
    }

    // 1. Check Commit / Rollback buttons on bottom banner
    if (_composer->isAwaitingConfirmation()) {
        if (_commitBtnRect.contains(pos)) {
            cancelHandRotateHold();
            _composer->commitStagedMove();
            update();
            return;
        }
        if (_rollbackBtnRect.contains(pos)) {
            cancelHandRotateHold();
            _composer->cancelAndRollback();
            update();
            return;
        }
    }

    // 2. Check clicking staged origin square when multiple pods remain at origin
    if (_composer && _composer->isAwaitingConfirmation() && _composer->hasOtherMovablePodAtOrigin()) {
        int cx = 0, cy = 0;
        if (_geometry.screenToSquare(pos, cx, cy)) {
            std::pair<int, int> orig = _composer->guidedOriginSquare();
            if (cx == orig.first && cy == orig.second) {
                cancelHandRotateHold();
                _composer->finishGuidedMoveHere();
                if (!_composer->hasDistinctUnclaimedPodsAtOrigin()) {
                    int nextIdx = _composer->firstUnclaimedFriendlyPodIndex(orig.first, orig.second);
                    _composer->beginGuidedPodMove(orig.first, orig.second, nextIdx, false);
                    update();
                    return;
                } else {
                    openZoomedSquareAt(orig.first, orig.second);
                    return;
                }
            }
        }
    }

    // 3. Check Capture [x] / Keep [ ] pills
    for (const auto &cc : _composer->captureChoices()) {
        QRectF sqRect = _geometry.squareRect(cc.jumpedX, cc.jumpedY);
        QRectF pill(sqRect.left() + 2.0, sqRect.top() + 2.0,
                    sqRect.width() - 4.0, std::max(16.0, sqRect.height() * 0.28));
        if (pill.contains(pos)) {
            cancelHandRotateHold();
            _composer->toggleCaptureAtSquare(cc.jumpedX, cc.jumpedY);
            update();
            return;
        }
    }

    // 4. Check drag-off-board capture initiation during staged review
    if (_composer->isAwaitingConfirmation()) {
        int cx = 0, cy = 0;
        if (_geometry.screenToSquare(pos, cx, cy)) {
            for (const auto &cc : _composer->captureChoices()) {
                if (cc.jumpedX == cx && cc.jumpedY == cy) {
                    cancelHandRotateHold();
                    _dragJumpedSquareX = cx;
                    _dragJumpedSquareY = cy;
                    return;
                }
            }
        }
    }

    bool isGuidedDest = false;
    if (_composer->isGuidedMoveActive()) {
        int gx = 0, gy = 0;
        if (_geometry.screenToSquare(pos, gx, gy)) {
            for (const auto &sq : _composer->guidedNextSquares()) {
                if (sq.first == gx && sq.second == gy) {
                    isGuidedDest = true;
                    break;
                }
            }
        }
    }
    bool isReserveDrag = (_dragKind == DragKind::ADD_POD_FROM_RESERVE ||
                          _dragKind == DragKind::LIBERATE_POD_FROM_RESERVE);

    if (event->button() == Qt::LeftButton) {
        _leftMouseHeld = true;
        if (!isGuidedDest && !isReserveDrag) {
            _holdRotateTimer.start(200);
        }
    }

    // 5. Guided hop selection when guided move is active
    if (_composer->isGuidedMoveActive()) {
        int gx = 0, gy = 0;
        if (_geometry.screenToSquare(pos, gx, gy)) {
            auto nextList = _composer->guidedNextSquares();
            bool isNext = false;
            for (const auto &sq : nextList) {
                if (sq.first == gx && sq.second == gy) {
                    isNext = true;
                    break;
                }
            }
            if (isNext) {
                cancelHandRotateHold();
                _selectedSquareX = -1;
                _selectedSquareY = -1;
                _selectedPodIndex = -1;
                std::size_t prevCapCount = _composer->captureChoices().size();
                _composer->selectGuidedDestination(gx, gy);
                if (isVisible()) {
                    // Prompt only for newly added optional captures on this hop (PCE-D07)
                    const auto &choices = _composer->captureChoices();
                    for (std::size_t ci = prevCapCount; ci < choices.size(); ++ci) {
                        const auto &cc = choices[ci];
                        if (!cc.mandatory) {
                            OctiPlayer activeTurn = (_board->getGame())
                                ? _board->getGame()->getTurn() : PLAYER_A;
                            OctiPlayer jumpedOwner = (activeTurn == PLAYER_B)
                                ? PLAYER_A : PLAYER_B;
                            if (_board->isValidCoord(cc.jumpedX - 1, cc.jumpedY - 1)) {
                                const OctiSquare &jsq =
                                    _board->squareAt(cc.jumpedX - 1, cc.jumpedY - 1);
                                for (int pi = 0; pi < jsq.getNumPods(); ++pi) {
                                    OctiPodType pt = jsq.getPodAt(pi).getType();
                                    OctiPlayer owner = (pt == A_POD ? PLAYER_A :
                                                        pt == B_POD ? PLAYER_B :
                                                        pt == C_POD ? PLAYER_C : PLAYER_D);
                                    if (owner != activeTurn) {
                                        jumpedOwner = owner;
                                        break;
                                    }
                                }
                            }
                            cancelHandRotateHold();
                            CapturePromptDialog capDlg(
                                cc.jumpedX, cc.jumpedY, jumpedOwner, _theme, this);
                            if (capDlg.exec() == QDialog::Accepted) {
                                _composer->setCaptureAtSquare(
                                    cc.jumpedX, cc.jumpedY, capDlg.captureChosen());
                            }
                        }
                    }
                    if (_composer->isAwaitingConfirmation()) {
                        presentPlyConfirmationIfNeeded();
                    }
                }
                update();
                return;
            }
        }
        if (_composer->isAwaitingConfirmation()) {
            if (isVisible()) {
                presentPlyConfirmationIfNeeded();
            }
            update();
            return;
        }
        bool zeroHopsSoFar = (_composer->guidedCurrentSquare() == _composer->guidedOriginSquare());
        std::pair<int, int> oldOrigin = _composer->guidedOriginSquare();
        _composer->cancelAndRollback();
        if (zeroHopsSoFar && gx >= 1 && gy >= 1 &&
            (gx != oldOrigin.first || gy != oldOrigin.second) &&
            _board->isValidCoord(gx - 1, gy - 1)) {
            const OctiSquare &clickedSq = _board->squareAt(gx - 1, gy - 1);
            if (!clickedSq.isEmpty() && _board->getGame() &&
                _board->getGame()->isGameStarted() &&
                !clickedSq.hasEnemyOf(_board->getGame()->getTurn())) {
                if (!isVisible()) {
                    _composer->beginGuidedPodMove(
                        gx, gy,
                        _composer->selectedStackPodIndex(gx, gy),
                        _composer->moveEntireMatchingStack());
                    _dragStartX = gx;
                    _dragStartY = gy;
                    _dragPodIndex = _composer->selectedStackPodIndex(gx, gy);
                    _dragKind = DragKind::MOVE_OR_JUMP_POD;
                    update();
                    return;
                }
                // Fall through to step 7 when isVisible() is true
            } else {
                update();
                return;
            }
        } else {
            update();
            return;
        }
    }

    // 6. Check Stack Fan Pop-over slot selection
    if (_stackFanX >= 1 && _stackFanY >= 1 &&
        _board->isValidCoord(_stackFanX - 1, _stackFanY - 1)) {
        const OctiSquare &sq = _board->squareAt(_stackFanX - 1, _stackFanY - 1);
        int n = sq.getNumPods();
        int slot = _geometry.hitTestStackFanSlot(_stackFanX, _stackFanY, n, pos);
        if (slot >= 0) {
            auto sorted = sortedPodIndicesMostProngsFirst(sq);
            int podIdx = (slot < static_cast<int>(sorted.size())) ? sorted[slot] : slot;
            _composer->selectStackPodIndex(_stackFanX, _stackFanY, podIdx);
            _dragKind = DragKind::MOVE_OR_JUMP_POD;
            _dragStartX = _stackFanX;
            _dragStartY = _stackFanY;
            _dragPodIndex = podIdx;
            update();
            return;
        }
    }

    // 7. Hit-test board square: open ZoomedSquareDialog on clicking non-empty square or eligible home square
    int sx = 0, sy = 0;
    if (_geometry.screenToSquare(pos, sx, sy) &&
        _board->isValidCoord(sx - 1, sy - 1)) {
        _hoverSquareX = sx;
        _hoverSquareY = sy;

        // If an external Reserve drag/tool is active, preserve _dragKind so
        // mouseReleaseEvent places the reserve pod onto (sx, sy)!
        if (_dragKind == DragKind::ADD_POD_FROM_RESERVE ||
            _dragKind == DragKind::LIBERATE_POD_FROM_RESERVE) {
            update();
            return;
        }

        const OctiSquare &sq = _board->squareAt(sx - 1, sy - 1);
        _stackFanX = -1;
        _stackFanY = -1;

        if (!sq.isEmpty() && _board->getGame() && _board->getGame()->isGameStarted() &&
            !sq.hasEnemyOf(_board->getGame()->getTurn())) {
            _composer->beginGuidedPodMove(
                sx, sy, _composer->selectedStackPodIndex(sx, sy), _composer->moveEntireMatchingStack());
            _dragStartX = sx;
            _dragStartY = sy;
            _dragPodIndex = _composer->selectedStackPodIndex(sx, sy);
            _dragKind = DragKind::MOVE_OR_JUMP_POD;
            _dragSrcProngDir = DIR_UNDEF;
        } else if (!isVisible() && !sq.isEmpty()) {
            _dragStartX = sx;
            _dragStartY = sy;
            _dragPodIndex = _composer->selectedStackPodIndex(sx, sy);
            _dragKind = DragKind::MOVE_OR_JUMP_POD;
            _dragSrcProngDir = DIR_UNDEF;
        }

        update();
    }
}

void OctiBoardCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (!event) {
        return;
    }
    QPointF curRaw = rawMousePosF(event);

    if (_leftMouseHeld && _boardZoomScale > 1.001 &&
        _dragKind != DragKind::ADD_POD_FROM_RESERVE &&
        _dragKind != DragKind::LIBERATE_POD_FROM_RESERVE) {
        double moveDist = std::hypot(curRaw.x() - _pressRawPos.x(), curRaw.y() - _pressRawPos.y());
        if (moveDist > 6.0 || _boardPanActive) {
            _boardPanActive = true;
            _dragKind = DragKind::NONE;
            _holdRotateTimer.stop();
            setCursor(Qt::ClosedHandCursor);
            QPointF delta = curRaw - _lastPanRawPos;
            _lastPanRawPos = curRaw;
            _boardZoomFocus -= delta / (_boardZoomScale - 1.0);
            _boardZoomFocus.rx() = std::clamp(_boardZoomFocus.x(), 0.0, static_cast<double>(width()));
            _boardZoomFocus.ry() = std::clamp(_boardZoomFocus.y(), 0.0, static_cast<double>(height()));
            update();
            return;
        }
    }

    if (_handRotateActive) {
        if (isVisible() && !(event->buttons() & Qt::LeftButton)) {
            cancelHandRotateHold();
            update();
            return;
        }
        setCursor(Qt::ClosedHandCursor);
        QPointF center = _geometry.boardOuterRect().center();
        double curAngleDeg = std::atan2(
            curRaw.y() - center.y(),
            curRaw.x() - center.x()) * 180.0 / 3.14159265358979323846;
        auto normalizeDeg = [](double a) {
            while (a > 180.0) a -= 360.0;
            while (a < -180.0) a += 360.0;
            return a;
        };
        double diff = normalizeDeg(curAngleDeg - _handRotateRefAngleDeg);
        _liveRotationOffsetDeg = diff;
        update();
        return;
    }

    if (_leftMouseHeld && !_handRotateActive && _boardZoomScale <= 1.001) {
        double moveDist = std::hypot(curRaw.x() - _pressRawPos.x(), curRaw.y() - _pressRawPos.y());
        if (_dragKind == DragKind::NONE && moveDist > 6.0) {
            _handRotateActive = true;
            _holdRotateTimer.stop();
            setCursor(Qt::ClosedHandCursor);
            QPointF center = _geometry.boardOuterRect().center();
            _handRotateRefAngleDeg = std::atan2(_pressRawPos.y() - center.y(),
                                                _pressRawPos.x() - center.x()) * 180.0 / 3.14159265358979323846;
            double curAngleDeg = std::atan2(curRaw.y() - center.y(),
                                            curRaw.x() - center.x()) * 180.0 / 3.14159265358979323846;
            auto normalizeDeg = [](double a) {
                while (a > 180.0) a -= 360.0;
                while (a < -180.0) a += 360.0;
                return a;
            };
            _liveRotationOffsetDeg = normalizeDeg(curAngleDeg - _handRotateRefAngleDeg);
            update();
            return;
        }
    }

    QPointF pos = mousePosF(event);
    _dragCursorPos = pos;
    _dragShiftHeld = (event->modifiers() & Qt::ShiftModifier) != 0;

    int sx = -1, sy = -1;
    if (_geometry.screenToSquare(pos, sx, sy)) {
        _hoverSquareX = sx;
        _hoverSquareY = sy;
    } else {
        _hoverSquareX = -1;
        _hoverSquareY = -1;
    }
    _dragHoverProngDir = DIR_UNDEF;
    update();
}

void OctiBoardCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (_handRotateActive) {
        auto norm360 = [](double a) {
            while (a < 0.0) a += 360.0;
            while (a >= 360.0) a -= 360.0;
            return a;
        };
        double totalDeg = static_cast<double>(_rotation) + _liveRotationOffsetDeg;
        int snappedDeg = static_cast<int>(std::round(norm360(totalDeg) / 90.0)) * 90;
        if (snappedDeg >= 360) {
            snappedDeg = 0;
        }
        BoardRotation snapped = static_cast<BoardRotation>(snappedDeg);
        cancelHandRotateHold();
        setBoardRotation(snapped);
        return;
    }
    cancelHandRotateHold();

    if (_boardPanActive) {
        _boardPanActive = false;
        _leftMouseHeld = false;
        unsetCursor();
        update();
        return;
    }

    if (!event || !_composer || (_board && _board->getGame() && _board->getGame()->isGameOver())) {
        _dragKind = DragKind::NONE;
        return;
    }

    QPointF pos = mousePosF(event);

    if (_dragJumpedSquareX >= 1) {
        if (!_geometry.boardOuterRect().contains(pos)) {
            _composer->setCaptureAtSquare(_dragJumpedSquareX, _dragJumpedSquareY, true);
        }
        _dragJumpedSquareX = -1;
        _dragJumpedSquareY = -1;
        update();
        return;
    }

    int tx = -1, ty = -1;
    bool onBoard = _geometry.screenToSquare(pos, tx, ty);

    if (onBoard) {
        if (_dragKind == DragKind::ADD_POD_FROM_RESERVE ||
            _dragKind == DragKind::LIBERATE_POD_FROM_RESERVE) {
            simulateDragFromReserveToSquare(_dragKind, tx, ty);
            _selectedSquareX = -1;
            _selectedSquareY = -1;
            _selectedPodIndex = -1;
        } else if (_dragKind == DragKind::MOVE_OR_JUMP_POD &&
                   _dragStartX >= 1 && _dragStartY >= 1 &&
                   (tx != _dragStartX || ty != _dragStartY)) {
            simulateDragPodToSquare(
                _dragStartX, _dragStartY, _dragPodIndex,
                tx, ty, _dragShiftHeld);
            _selectedSquareX = -1;
            _selectedSquareY = -1;
            _selectedPodIndex = -1;
        } else if (_dragKind == DragKind::MOVE_OR_JUMP_POD &&
                   tx == _dragStartX && ty == _dragStartY &&
                   tx >= 1 && ty >= 1) {
            bool acted = false;
            if (_selectedSquareX >= 1 && _selectedSquareY >= 1 &&
                (tx != _selectedSquareX || ty != _selectedSquareY)) {
                auto dests = _composer->legalDestinationsFrom(
                    _selectedSquareX, _selectedSquareY, _selectedPodIndex);
                if (std::find(dests.begin(), dests.end(),
                              std::make_pair(tx, ty)) != dests.end()) {
                    acted = simulateDragPodToSquare(
                        _selectedSquareX, _selectedSquareY, _selectedPodIndex,
                        tx, ty, _dragShiftHeld);
                }
            }
            if (acted) {
                _selectedSquareX = -1;
                _selectedSquareY = -1;
                _selectedPodIndex = -1;
            } else {
                _selectedSquareX = tx;
                _selectedSquareY = ty;
                _selectedPodIndex = _dragPodIndex;
            }
        } else if (_dragKind == DragKind::NONE &&
                   _selectedSquareX >= 1 && _selectedSquareY >= 1 &&
                   (tx != _selectedSquareX || ty != _selectedSquareY)) {
            simulateDragPodToSquare(
                _selectedSquareX, _selectedSquareY, _selectedPodIndex,
                tx, ty, _dragShiftHeld);
            _selectedSquareX = -1;
            _selectedSquareY = -1;
            _selectedPodIndex = -1;
        }
    }

    _dragKind = DragKind::NONE;
    _dragStartX = -1;
    _dragStartY = -1;
    _dragPodIndex = -1;
    _dragSrcProngDir = DIR_UNDEF;
    if (isVisible() && _composer && _composer->isAwaitingConfirmation()) {
        presentPlyConfirmationIfNeeded();
    }
    update();
}

void OctiBoardCanvas::presentPlyConfirmationIfNeeded()
{
    update();
}

void OctiBoardCanvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    cancelHandRotateHold();
    if (!event) {
        return;
    }
    if (_boardZoomScale > 1.001) {
        resetBoardZoom();
        return;
    }
    if (!_board) {
        return;
    }
    if (_board->getGame() && _board->getGame()->isGameOver()) {
        return;
    }
    if (_composer && _composer->isAwaitingConfirmation()) {
        _composer->commitStagedMove();
        update();
        return;
    }

    int sx = 0, sy = 0;
    if (_geometry.screenToSquare(mousePosF(event), sx, sy)) {
        openZoomedSquareAt(sx, sy);
    }
}

void OctiBoardCanvas::wheelEvent(QWheelEvent *event)
{
    cancelHandRotateHold();
    if (!event) {
        return;
    }
    int deltaY = event->angleDelta().y();
    if (deltaY == 0) {
        return;
    }
    QPointF pos = rawMousePosF(event);
    if (deltaY > 0) {
        double oldScale = _boardZoomScale;
        double newScale = std::min(3.0, oldScale * 1.15);
        QPointF boardPt = (oldScale > 1.001)
            ? ((pos - _boardZoomFocus) / oldScale + _boardZoomFocus)
            : pos;
        if (newScale > 1.001) {
            _boardZoomFocus = (boardPt * newScale - pos) / (newScale - 1.0);
        } else {
            _boardZoomFocus = pos;
        }
        _boardZoomScale = newScale;
        update();
    } else {
        double oldScale = _boardZoomScale;
        double newScale = oldScale / 1.15;
        if (newScale <= 1.03) {
            resetBoardZoom();
        } else {
            QPointF boardPt = (oldScale > 1.001)
                ? ((pos - _boardZoomFocus) / oldScale + _boardZoomFocus)
                : pos;
            _boardZoomFocus = (boardPt * newScale - pos) / (newScale - 1.0);
            _boardZoomScale = newScale;
            update();
        }
    }
    event->accept();
}

void OctiBoardCanvas::keyPressEvent(QKeyEvent *event)
{
    if (event && event->key() == Qt::Key_Escape && _boardZoomScale > 1.001) {
        resetBoardZoom();
    }
    if (!event || !_composer || (_board && _board->getGame() && _board->getGame()->isGameOver())) {
        QWidget::keyPressEvent(event);
        return;
    }

    if (event->key() == Qt::Key_Return ||
        event->key() == Qt::Key_Enter ||
        event->key() == Qt::Key_Space) {
        if (_composer->isAwaitingConfirmation()) {
            _composer->commitStagedMove();
            update();
            return;
        }
    } else if (event->key() == Qt::Key_Escape) {
        _composer->cancelAndRollback();
        _dragKind = DragKind::NONE;
        _selectedSquareX = -1;
        _selectedSquareY = -1;
        _selectedPodIndex = -1;
        closeStackFan();
        update();
        return;
    }

    QWidget::keyPressEvent(event);
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
