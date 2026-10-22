/*
 * OctiBoardGeometry.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_OCTI_BOARD_GEOMETRY_HXX
#define OCTI_DESKTOP_OCTI_BOARD_GEOMETRY_HXX

#include <QPointF>
#include <QRectF>
#include "OctiDefs.hxx"

namespace octi {

enum class BoardRotation {
    ROT_0   = 0,
    ROT_90  = 90,
    ROT_180 = 180,
    ROT_270 = 270,
};

class OctiBoardGeometry {
public:
    OctiBoardGeometry(int viewportWidth = 800,
                      int viewportHeight = 800,
                      int gridWidth = 9,
                      int gridHeight = 9,
                      BoardRotation rotation = BoardRotation::ROT_0);

    void configure(int viewportWidth,
                   int viewportHeight,
                   int gridWidth,
                   int gridHeight,
                   BoardRotation rotation = BoardRotation::ROT_0);

    void setViewportSize(int viewportWidth, int viewportHeight);
    void setGridDimensions(int gridWidth, int gridHeight);
    void setRotation(BoardRotation rotation);

    BoardRotation rotation() const;
    int rotationDegrees() const;
    int viewportWidth() const;
    int viewportHeight() const;
    int gridWidth() const;
    int gridHeight() const;
    double cellSize() const;

    static BoardRotation rotationForSeat(OctiGameMode mode, OctiPlayer seat);
    static BoardRotation rotationForSeat(OctiGameMode mode, int seat);
    static BoardRotation nextRotationClockwise(BoardRotation r);

    QRectF boardOuterRect() const;
    QRectF gridInnerRect() const;
    QRectF squareRect(int x, int y) const;
    QPointF squareCenter(int x, int y) const;
    bool screenToSquare(const QPointF &pt, int &outX, int &outY) const;

    QPointF dirUnitVectorScreen(OctiDir dir) const;
    OctiDir hitTestCompassSector(const QPointF &podCenter,
                                 double innerRadius,
                                 double outerRadius,
                                 const QPointF &cursor) const;

    QRectF stackFanOverlayRect(int x, int y, int numPods) const;
    QRectF stackFanOverlayRect(const QRectF &sqRect, int numPods) const;
    QRectF stackFanSlotRect(int x, int y, int numPods, int slotIndex) const;
    QRectF stackFanSlotRect(const QRectF &sqRect, int numPods, int slotIndex) const;
    int hitTestStackFanSlot(int x, int y, int numPods, const QPointF &cursor) const;
    int hitTestStackFanSlot(const QRectF &sqRect, int numPods, const QPointF &cursor) const;

private:
    void recomputeLayout();
    int screenCols() const;
    int screenRows() const;
    bool boardToScreenGrid(int x, int y, int &outCol, int &outRow) const;
    void screenGridToBoard(int col, int row, int &outX, int &outY) const;

    int _viewportWidth;
    int _viewportHeight;
    int _gridWidth;
    int _gridHeight;
    BoardRotation _rotation;

    double _cellSize;
    QRectF _boardOuterRect;
    QRectF _gridInnerRect;
};

} // namespace octi

#endif // OCTI_DESKTOP_OCTI_BOARD_GEOMETRY_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
