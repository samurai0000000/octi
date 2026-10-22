/*
 * OctiBoardGeometry.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiBoardGeometry.hxx"
#include <algorithm>
#include <cmath>

namespace octi {

OctiBoardGeometry::OctiBoardGeometry(int viewportWidth,
                                     int viewportHeight,
                                     int gridWidth,
                                     int gridHeight,
                                     BoardRotation rotation)
    : _viewportWidth(std::max(64, viewportWidth))
    , _viewportHeight(std::max(64, viewportHeight))
    , _gridWidth(std::clamp(gridWidth, 1, 9))
    , _gridHeight(std::clamp(gridHeight, 1, 9))
    , _rotation(rotation)
    , _cellSize(40.0)
{
    recomputeLayout();
}

void OctiBoardGeometry::configure(int viewportWidth,
                                  int viewportHeight,
                                  int gridWidth,
                                  int gridHeight,
                                  BoardRotation rotation)
{
    _viewportWidth = std::max(64, viewportWidth);
    _viewportHeight = std::max(64, viewportHeight);
    _gridWidth = std::clamp(gridWidth, 1, 9);
    _gridHeight = std::clamp(gridHeight, 1, 9);
    _rotation = rotation;
    recomputeLayout();
}

void OctiBoardGeometry::setViewportSize(int viewportWidth, int viewportHeight)
{
    _viewportWidth = std::max(64, viewportWidth);
    _viewportHeight = std::max(64, viewportHeight);
    recomputeLayout();
}

void OctiBoardGeometry::setGridDimensions(int gridWidth, int gridHeight)
{
    _gridWidth = std::clamp(gridWidth, 1, 9);
    _gridHeight = std::clamp(gridHeight, 1, 9);
    recomputeLayout();
}

void OctiBoardGeometry::setRotation(BoardRotation rotation)
{
    _rotation = rotation;
    recomputeLayout();
}

BoardRotation OctiBoardGeometry::rotation() const
{
    return _rotation;
}

int OctiBoardGeometry::rotationDegrees() const
{
    return static_cast<int>(_rotation);
}

int OctiBoardGeometry::viewportWidth() const
{
    return _viewportWidth;
}

int OctiBoardGeometry::viewportHeight() const
{
    return _viewportHeight;
}

int OctiBoardGeometry::gridWidth() const
{
    return _gridWidth;
}

int OctiBoardGeometry::gridHeight() const
{
    return _gridHeight;
}

double OctiBoardGeometry::cellSize() const
{
    return _cellSize;
}

BoardRotation OctiBoardGeometry::rotationForSeat(OctiGameMode mode, OctiPlayer seat)
{
    if (seat == PLAYER_A) {
        return BoardRotation::ROT_0;
    }
    if (mode == GAME_MODE_2 || mode == GAME_MODE_OFK) {
        if (seat == PLAYER_B) {
            return BoardRotation::ROT_180;
        }
        return BoardRotation::ROT_0;
    }
    if (mode == GAME_MODE_4) {
        switch (seat) {
        case PLAYER_A: return BoardRotation::ROT_0;
        case PLAYER_B: return BoardRotation::ROT_90;
        case PLAYER_C: return BoardRotation::ROT_180;
        case PLAYER_D: return BoardRotation::ROT_270;
        default:       return BoardRotation::ROT_0;
        }
    }
    return BoardRotation::ROT_0;
}

BoardRotation OctiBoardGeometry::rotationForSeat(OctiGameMode mode, int seat)
{
    if (seat < 1 || seat > 4) {
        return BoardRotation::ROT_0;
    }
    return rotationForSeat(mode, static_cast<OctiPlayer>(seat));
}

BoardRotation OctiBoardGeometry::nextRotationClockwise(BoardRotation r)
{
    switch (r) {
    case BoardRotation::ROT_0:   return BoardRotation::ROT_90;
    case BoardRotation::ROT_90:  return BoardRotation::ROT_180;
    case BoardRotation::ROT_180: return BoardRotation::ROT_270;
    case BoardRotation::ROT_270: return BoardRotation::ROT_0;
    }
    return BoardRotation::ROT_0;
}

int OctiBoardGeometry::screenCols() const
{
    if (_rotation == BoardRotation::ROT_90 || _rotation == BoardRotation::ROT_270) {
        return _gridHeight;
    }
    return _gridWidth;
}

int OctiBoardGeometry::screenRows() const
{
    if (_rotation == BoardRotation::ROT_90 || _rotation == BoardRotation::ROT_270) {
        return _gridWidth;
    }
    return _gridHeight;
}

void OctiBoardGeometry::recomputeLayout()
{
    int cols = std::max(1, screenCols());
    int rows = std::max(1, screenRows());

    double pad = std::max(12.0, std::min(_viewportWidth, _viewportHeight) * 0.045);
    double availW = std::max(32.0, static_cast<double>(_viewportWidth) - 2.0 * pad);
    double availH = std::max(32.0, static_cast<double>(_viewportHeight) - 2.0 * pad);

    // Reserve 1.1 cells total for the luxury brass bezel and coordinate gutters
    double cellW = availW / (static_cast<double>(cols) + 1.10);
    double cellH = availH / (static_cast<double>(rows) + 1.10);
    _cellSize = std::max(4.0, std::min(cellW, cellH));

    double gridW = _cellSize * static_cast<double>(cols);
    double gridH = _cellSize * static_cast<double>(rows);
    double gutter = _cellSize * 0.55;

    double gridLeft = (static_cast<double>(_viewportWidth) - gridW) * 0.5;
    double gridTop = (static_cast<double>(_viewportHeight) - gridH) * 0.5;

    _gridInnerRect = QRectF(gridLeft, gridTop, gridW, gridH);
    _boardOuterRect = _gridInnerRect.adjusted(-gutter, -gutter, gutter, gutter);
}

bool OctiBoardGeometry::boardToScreenGrid(int x, int y, int &outCol, int &outRow) const
{
    if (x < 1 || x > _gridWidth || y < 1 || y > _gridHeight) {
        return false;
    }

    switch (_rotation) {
    case BoardRotation::ROT_0:
        outCol = x - 1;
        outRow = _gridHeight - y;
        break;
    case BoardRotation::ROT_90:
        outCol = _gridHeight - y;
        outRow = _gridWidth - x;
        break;
    case BoardRotation::ROT_180:
        outCol = _gridWidth - x;
        outRow = y - 1;
        break;
    case BoardRotation::ROT_270:
        outCol = y - 1;
        outRow = x - 1;
        break;
    }
    return true;
}

void OctiBoardGeometry::screenGridToBoard(int col, int row, int &outX, int &outY) const
{
    switch (_rotation) {
    case BoardRotation::ROT_0:
        outX = col + 1;
        outY = _gridHeight - row;
        break;
    case BoardRotation::ROT_90:
        outX = _gridWidth - row;
        outY = _gridHeight - col;
        break;
    case BoardRotation::ROT_180:
        outX = _gridWidth - col;
        outY = row + 1;
        break;
    case BoardRotation::ROT_270:
        outX = row + 1;
        outY = col + 1;
        break;
    }
}

QRectF OctiBoardGeometry::boardOuterRect() const
{
    return _boardOuterRect;
}

QRectF OctiBoardGeometry::gridInnerRect() const
{
    return _gridInnerRect;
}

QRectF OctiBoardGeometry::squareRect(int x, int y) const
{
    int col = 0;
    int row = 0;
    if (!boardToScreenGrid(x, y, col, row)) {
        return QRectF();
    }

    double sx = _gridInnerRect.left() + static_cast<double>(col) * _cellSize;
    double sy = _gridInnerRect.top() + static_cast<double>(row) * _cellSize;
    return QRectF(sx, sy, _cellSize, _cellSize);
}

QPointF OctiBoardGeometry::squareCenter(int x, int y) const
{
    QRectF r = squareRect(x, y);
    if (r.isEmpty()) {
        return QPointF();
    }
    return r.center();
}

bool OctiBoardGeometry::screenToSquare(const QPointF &pt, int &outX, int &outY) const
{
    if (!_gridInnerRect.contains(pt)) {
        return false;
    }

    int cols = screenCols();
    int rows = screenRows();
    int col = static_cast<int>(std::floor((pt.x() - _gridInnerRect.left()) / _cellSize));
    int row = static_cast<int>(std::floor((pt.y() - _gridInnerRect.top()) / _cellSize));

    if (col < 0 || col >= cols || row < 0 || row >= rows) {
        return false;
    }

    screenGridToBoard(col, row, outX, outY);
    return (outX >= 1 && outX <= _gridWidth && outY >= 1 && outY <= _gridHeight);
}

QPointF OctiBoardGeometry::dirUnitVectorScreen(OctiDir dir) const
{
    if (dir < DIR_N || dir > DIR_NW) {
        return QPointF(0.0, 0.0);
    }

    double dx = static_cast<double>(dirDeltaX(dir));
    double dy = static_cast<double>(dirDeltaY(dir));
    double sx = 0.0;
    double sy = 0.0;

    switch (_rotation) {
    case BoardRotation::ROT_0:
        sx = dx;
        sy = -dy;
        break;
    case BoardRotation::ROT_90:
        sx = -dy;
        sy = -dx;
        break;
    case BoardRotation::ROT_180:
        sx = -dx;
        sy = dy;
        break;
    case BoardRotation::ROT_270:
        sx = dy;
        sy = dx;
        break;
    }

    double len = std::hypot(sx, sy);
    if (len <= 1e-9) {
        return QPointF(0.0, 0.0);
    }
    return QPointF(sx / len, sy / len);
}

OctiDir OctiBoardGeometry::hitTestCompassSector(const QPointF &podCenter,
                                                double innerRadius,
                                                double outerRadius,
                                                const QPointF &cursor) const
{
    if (innerRadius < 0.0 || outerRadius <= innerRadius) {
        return DIR_UNDEF;
    }

    double vx = cursor.x() - podCenter.x();
    double vy = cursor.y() - podCenter.y();
    double dist = std::hypot(vx, vy);
    if (dist < innerRadius || dist > outerRadius || dist < 1e-9) {
        return DIR_UNDEF;
    }

    double ux = vx / dist;
    double uy = vy / dist;

    OctiDir bestDir = DIR_UNDEF;
    double bestDot = -2.0;

    for (int d = DIR_N; d <= DIR_NW; ++d) {
        OctiDir dir = static_cast<OctiDir>(d);
        QPointF du = dirUnitVectorScreen(dir);
        double dot = ux * du.x() + uy * du.y();
        if (dot > bestDot) {
            bestDot = dot;
            bestDir = dir;
        }
    }

    return bestDir;
}

QRectF OctiBoardGeometry::stackFanOverlayRect(const QRectF &sqRect, int numPods) const
{
    if (sqRect.isEmpty() || numPods <= 0) {
        return QRectF();
    }

    int clampedPods = std::clamp(numPods, 1, 8);
    double slotW = sqRect.width() * 0.78;
    double slotH = sqRect.height() * 0.78;
    double pad = sqRect.width() * 0.08;
    double totalW = static_cast<double>(clampedPods) * slotW
                  + static_cast<double>(clampedPods + 1) * pad;
    double totalH = slotH + 2.0 * pad;

    double cx = sqRect.center().x();
    double left = std::clamp(cx - totalW * 0.5,
                             _boardOuterRect.left() + 4.0,
                             std::max(_boardOuterRect.left() + 4.0,
                                      _boardOuterRect.right() - totalW - 4.0));
    double top = sqRect.top() - totalH - 6.0;
    if (top < _boardOuterRect.top() + 4.0) {
        top = sqRect.bottom() + 6.0;
    }
    return QRectF(left, top, totalW, totalH);
}

QRectF OctiBoardGeometry::stackFanOverlayRect(int x, int y, int numPods) const
{
    return stackFanOverlayRect(squareRect(x, y), numPods);
}

QRectF OctiBoardGeometry::stackFanSlotRect(const QRectF &sqRect,
                                           int numPods,
                                           int slotIndex) const
{
    if (sqRect.isEmpty() || numPods <= 0 || slotIndex < 0 || slotIndex >= numPods) {
        return QRectF();
    }

    QRectF overlay = stackFanOverlayRect(sqRect, numPods);
    if (overlay.isEmpty()) {
        return QRectF();
    }

    double slotW = sqRect.width() * 0.78;
    double slotH = sqRect.height() * 0.78;
    double pad = sqRect.width() * 0.08;
    double sx = overlay.left() + pad + static_cast<double>(slotIndex) * (slotW + pad);
    double sy = overlay.top() + pad;
    return QRectF(sx, sy, slotW, slotH);
}

QRectF OctiBoardGeometry::stackFanSlotRect(int x, int y, int numPods, int slotIndex) const
{
    return stackFanSlotRect(squareRect(x, y), numPods, slotIndex);
}

int OctiBoardGeometry::hitTestStackFanSlot(const QRectF &sqRect,
                                           int numPods,
                                           const QPointF &cursor) const
{
    if (sqRect.isEmpty() || numPods <= 0) {
        return -1;
    }
    QRectF overlay = stackFanOverlayRect(sqRect, numPods);
    if (!overlay.contains(cursor)) {
        return -1;
    }

    for (int i = 0; i < numPods; ++i) {
        if (stackFanSlotRect(sqRect, numPods, i).contains(cursor)) {
            return i;
        }
    }
    return -1;
}

int OctiBoardGeometry::hitTestStackFanSlot(int x, int y, int numPods, const QPointF &cursor) const
{
    return hitTestStackFanSlot(squareRect(x, y), numPods, cursor);
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
