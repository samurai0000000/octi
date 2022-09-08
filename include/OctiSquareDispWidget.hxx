/*************************************************************************
 * $Id: OctiSquareDispWidget.hxx,v 1.3 2001/06/19 00:45:46 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/06/19 00:45:46 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created: 12/8/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _OCTISQUAREDISPWIDGET_HXX_
#define _OCTISQUAREDISPWIDGET_HXX_

#include <qbitmap.h>
#include <qpixmap.h>
#include <qframe.h>
#include "OctiPod.hxx"
#include "OctiSquare.hxx"
#include "OctiBoard.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

extern OCTIAPI void paint_pod_at(QPainter &p, int sqsize, boolean invert,
                                 OctiPod pod, int x, int y,
                                 QBitmap *mask = NULL, boolean fillMask = TRUE);

class OCTIAPI OctiSquareDispWidget : public QWidget,
                                     protected OctiBoardListener {

    Q_OBJECT;

public:

    OctiSquareDispWidget(QWidget *parent = NULL, const char *name = NULL, WFlags f = 0);
    ~OctiSquareDispWidget();

    void setShowInverted(boolean inverted);
    boolean isShowInverted() const;
    void setDisplaySquare(int x, int y, boolean forceload = FALSE);

    int getSelectedPods(OctiPod pods[]) const;
    OctiPlayer getSelectedPodsOwner() const;

private:

    friend class OctiBoardDispWidget;
    friend class OctiBoardWidget;

    void setBoard(OctiBoard *board);
    void setSquareSize(int size);

protected:

    void paintEvent(QPaintEvent *pe);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void dragEnterEvent(QDragEnterEvent *e);
    void dragMoveEvent(QDragMoveEvent *e);
    void dragLeaveEvent(QDragLeaveEvent *e);
    void dropEvent(QDropEvent *e);

protected:

    void gameChanged(OctiBoard *src, OctiGame *oldGame, OctiGame *newGame);
    void boardUpdated(OctiBoard *src, const OctiMove *move, boolean undo);

private:

    int getSquareLoc(int x, int y);

private:

    OctiBoard *_board;
    OctiPod _pods[7];
    int _numPods;
    boolean _selected[7];
    boolean _showInverted;
    int _squareSize;
    int _x, _y;
    int _highlightSqr;
    OctiDir _highlightDir;
    int _supressSqr;
    OctiDir _supressDir;
    boolean _supressOn;
    QPoint _pressPos;

};

// Inline functions

inline void OctiSquareDispWidget::setBoard(OctiBoard *board) {
    if(board != NULL && _board != board) {
        if(_board != NULL) {
            _board->removeOctiBoardListener(this);
        }
        _board = board;
        _board->addOctiBoardListener(this);
    }
}

inline void OctiSquareDispWidget::setShowInverted(boolean inverted) {
    if(_showInverted != inverted) {
        _showInverted = inverted;
        repaint();
    }
}

inline boolean OctiSquareDispWidget::isShowInverted() const {
    return _showInverted;
}

inline void OctiSquareDispWidget::setSquareSize(int size) {
    _squareSize = size;
    repaint();
}

__END_NAMESPACE(SELFSOFT);

#endif

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
