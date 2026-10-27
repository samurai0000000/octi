/*
 * OctiBoardCanvas.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_OCTI_BOARD_CANVAS_HXX
#define OCTI_DESKTOP_OCTI_BOARD_CANVAS_HXX

#include <utility>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QResizeEvent>
#include <QTimer>
#include <QWidget>
#include "OctiBoard.hxx"
#include "OctiBoardGeometry.hxx"
#include "OctiMoveComposer.hxx"
#include "OctiSoundSynth.hxx"
#include "OctiTheme.hxx"

namespace octi {

class OctiBoardCanvas : public QWidget {
public:
    explicit OctiBoardCanvas(QWidget *parent = nullptr);
    ~OctiBoardCanvas() override;

    void attachBoard(OctiBoard *board,
                     OctiMoveComposer *composer = nullptr,
                     OctiSoundSynth *soundSynth = nullptr);
    OctiBoard *board() const;
    OctiMoveComposer *composer() const;
    OctiSoundSynth *soundSynth() const;

    void setTheme(ThemeId theme);
    ThemeId theme() const;

    void setBoardRotation(BoardRotation rot);
    BoardRotation boardRotation() const;
    void rotateClockwise();
    void autoOrientForSeat(OctiPlayer seat);

    const OctiBoardGeometry &geometry() const;

    QImage renderToImage(int width = 960, int height = 960);
    void paintBoardScene(QPainter &p, int width, int height);

    void openStackFanAt(int x, int y);
    void closeStackFan();
    std::pair<int, int> activeStackFanSquare() const;

    void startExternalReserveDrag(DragKind kind);
    DragKind activeDragKind() const;
    void updateExternalReserveHover(const QPointF &canvasPos);
    bool finishExternalReserveDrop(const QPointF &canvasPos);

    // Deterministic programmatic drag simulation helpers for testing and HUD actions
    bool simulateDragFromReserveToSquare(DragKind kind, int targetX, int targetY);
    bool simulateDragProngToPod(int x, int y, int podIndex, OctiDir dir, bool superProng);
    bool simulateDragPodToSquare(int fromX, int fromY, int podIndex,
                                 int toX, int toY, bool moveAllStack = false);
    bool simulateRepositionProngOnPod(int x, int y, int podIndex,
                                      OctiDir srcDir, OctiDir tgtDir);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void syncGeometryToBoard(int w, int h);
    void triggerMoveAnimation(const QPointF &fromPt, const QPointF &toPt,
                              OctiPodType podType, const OctiPod &pod);
    void paintEdgelessPortals(QPainter &p, const OctiBoardGeometry &geom,
                              const OctiPalette &pal) const;
    void paintHomeSquareBadge(QPainter &p, const QRectF &sqRect,
                              const OctiSquare &sq, const OctiPalette &pal) const;
    void paintNeutralOctiMedallion(QPainter &p, const QRectF &sqRect,
                                   const OctiSquare &sq, const OctiPalette &pal) const;
    void paintCompassProngWheel(QPainter &p, const OctiBoardGeometry &geom,
                                int x, int y, int podIndex,
                                bool superProng, OctiDir hoverDir,
                                const OctiPalette &pal) const;
    void paintStackFanOverlay(QPainter &p, const OctiBoardGeometry &geom,
                              int x, int y, const OctiPalette &pal) const;
    void paintCaptureChoicePills(QPainter &p, const OctiBoardGeometry &geom,
                                 const OctiPalette &pal);
    void paintCommitBanner(QPainter &p, const OctiBoardGeometry &geom,
                           const OctiPalette &pal);
    void presentPlyConfirmationIfNeeded();

    OctiBoard *_board;
    OctiMoveComposer *_composer;
    OctiSoundSynth *_soundSynth;
    ThemeId _theme;
    BoardRotation _rotation;
    OctiBoardGeometry _geometry;

    DragKind _dragKind;
    int _dragStartX;
    int _dragStartY;
    int _dragPodIndex;
    OctiDir _dragSrcProngDir;
    OctiDir _dragHoverProngDir;
    QPointF _dragCursorPos;
    bool _dragShiftHeld;

    int _hoverSquareX;
    int _hoverSquareY;
    int _selectedSquareX;
    int _selectedSquareY;
    int _selectedPodIndex;
    int _stackFanX;
    int _stackFanY;

    QRectF _commitBtnRect;
    QRectF _rollbackBtnRect;
    QRectF _finishBtnRect;
    int _dragJumpedSquareX;
    int _dragJumpedSquareY;

    QTimer _animTimer;
    bool _animActive;
    double _animProgress;
    QPointF _animFromPt;
    QPointF _animToPt;
    OctiPodType _animPodType;
    OctiPod _animPod;
};

} // namespace octi

#endif // OCTI_DESKTOP_OCTI_BOARD_CANVAS_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
