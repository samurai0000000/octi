/*************************************************************************
 * $Id: OctiStatDispWidget.hxx,v 1.3 2001/06/19 00:45:46 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/06/19 00:45:46 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created: 12/10/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _OCTISTATDISPWIDGET_HXX_
#define _OCTISTATDISPWIDGET_HXX_

#include <qframe.h>
#include "OctiBoard.hxx"
#include "OctiSquareDispWidget.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class OCTIAPI OctiStatDispWidget : public QWidget,
                                   protected OctiBoardListener {

    Q_OBJECT;

public:

    OctiStatDispWidget(QWidget *parent = NULL, const char *name = NULL, WFlags f = 0);
    ~OctiStatDispWidget();

    void setPlayer(OctiPlayer player, boolean inverted = FALSE);

private:

    friend class OctiBoardDispWidget;
    friend class OctiBoardWidget;

    void setBoard(OctiBoard *board);
    void setSquareSize(int size);

protected:

    void paintEvent(QPaintEvent *pe);
    void resizeEvent(QResizeEvent *e);
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

    void calcRects();

private:

    OctiBoard *_board;
    OctiPlayer _player;
    int _squareSize;
    int _nfprong;
    int _nfpod;
    int _ncpod;
    boolean _showInverted;

    QPoint _pressPos;
    QRect _prongRect;
    QRect _freePodRect;
    QRect _jailPodRect;
    boolean _highlightJail;

};

// Inline functions

inline void OctiStatDispWidget::setBoard(OctiBoard *board) {
    if(board != NULL && _board != board) {
        if(_board != NULL) {
            _board->removeOctiBoardListener(this);
        }
        _board = board;
        _board->addOctiBoardListener(this);
    }
}

inline void OctiStatDispWidget::setPlayer(OctiPlayer player, boolean inverted) {
    if(_player != player || _showInverted != inverted) {
        _player = player;
        _showInverted = inverted;
        repaint();
    }
}

inline void OctiStatDispWidget::setSquareSize(int size) {
    _squareSize = size;
    calcRects();
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
