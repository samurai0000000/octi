/*************************************************************************
 * $Id: OctiBoardDispWidget.cxx,v 1.5 2001/08/24 03:00:34 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:00:34 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created: 12/8/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _OCTIAPI_IMPL_

#include <qlayout.h>
#include <qpainter.h>
#include <qpen.h>
#include <qdragobject.h>
#include <qdropsite.h>
#include "OctiWidgetColors.hxx"
#include "OctiWidgetOptions.hxx"
#include "OctiBoardDispWidget.hxx"
#include "OctiBoard.hxx"
#include "OctiGame.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

OctiBoardDispWidget::OctiBoardDispWidget(QWidget *parent, const char *name, WFlags f) : QWidget(parent, name, f) {
    int i;
    _showInverted = FALSE;
    _animate = TRUE;
    _move = NULL;
    _board = NULL;
    _sqrDisp = NULL;
    for(i = 0; i < 28; i++) {
        _pods[i].sqrX = _pods[i].sqrY = _pods[i].locX = _pods[i].locY = -1;
    }
    _numPods = 0;

    setAcceptDrops(TRUE);
    calcDispUnits();
    _supressOn = FALSE;
    _highlightX = -1;
    _highlightY = -1;
    _supressX = -1;
    _supressY = -1;
    _numSupressPods = 0;
}

OctiBoardDispWidget::~OctiBoardDispWidget() {
    if(_board) {
        _board->removeOctiBoardListener(this);
    }
}

void OctiBoardDispWidget::setShowInverted(boolean inverted) {
    if(_showInverted != inverted) {
        _showInverted = inverted;
        repaint(FALSE);
        if(_sqrDisp) {
            _sqrDisp->setShowInverted(inverted);
        }
    }

    switch(_board->getGameMode()) {
    case GAME_MODE_2:
        if(_showInverted) {
            _statDisp1->setPlayer(PLAYER_A, _showInverted);
            _statDisp2->setPlayer(PLAYER_UNDEF, _showInverted);
            _statDisp3->setPlayer(PLAYER_UNDEF, _showInverted);
            _statDisp4->setPlayer(PLAYER_B, _showInverted);
        } else {
            _statDisp1->setPlayer(PLAYER_B, _showInverted);
            _statDisp2->setPlayer(PLAYER_UNDEF, _showInverted);
            _statDisp3->setPlayer(PLAYER_UNDEF, _showInverted);
            _statDisp4->setPlayer(PLAYER_A, _showInverted);
        }
        break;
    case GAME_MODE_4:
        if(_showInverted) {
            _statDisp1->setPlayer(PLAYER_A, _showInverted);
            _statDisp2->setPlayer(PLAYER_C, _showInverted);
            _statDisp3->setPlayer(PLAYER_D, _showInverted);
            _statDisp4->setPlayer(PLAYER_B, _showInverted);
        } else {
            _statDisp1->setPlayer(PLAYER_B, _showInverted);
            _statDisp2->setPlayer(PLAYER_D, _showInverted);
            _statDisp3->setPlayer(PLAYER_C, _showInverted);
            _statDisp4->setPlayer(PLAYER_A, _showInverted);
        }
        break;
    }
}

int OctiBoardDispWidget::heightForWidth(int w) const {
    return w;
}

void OctiBoardDispWidget::paintEvent(QPaintEvent *e) {
    QPixmap pm(size());
    pm.fill(backgroundColor());
    QPainter p;
    p.begin(&pm, this);
    if(!_showInverted) {
        paintNormal(p);
        paintNormalPods(p);
        plotNormalMove(p);
    } else {
        paintInverted(p);
        paintInvertedPods(p);
        plotInvertedMove(p);
    }
    p.end();
    bitBlt(this, 0, 0, &pm);
}

void OctiBoardDispWidget::paintNormal(QPainter &p) {
    OctiWidgetColors &colors = G_OctiOptions->getWidgetColors();
    int i, j;
    int x, y, xc, yc;

    // Draw the boarder
    p.setClipRect(_xOffset, _yOffset, _boardSize, _boardSize);
    p.setFont(QFont("times", (int) (_squareSize * 0.5), QFont::Bold));
    p.setPen(QPen(colors.getLineColor()));
    p.drawRect(_xOffset, _yOffset, _boardSize, _boardSize);
    
    // Draw y-axis labels
    for(i = 1; i <= 9; i++) {
        y = i * _squareSize + _yOffset;
        p.setPen(QPen(colors.getSquareColor()));
        p.setBrush(QBrush(colors.getSquareColor()));
        p.drawRect(_xOffset, y - _squareSize, _squareSize, _squareSize);
        p.setPen(QPen(colors.getLabelColor()));
        p.drawText(_xOffset, y - _squareSize, _squareSize, _squareSize,
                   AlignCenter, QString(QChar('0' + 10 - i)));
    }
    
    // Draw x-axis labels
    y = 10 * _squareSize + _yOffset;
    for(i = 1; i <= 9; i++) {
        x = i * _squareSize + _xOffset;
        p.setPen(QPen(colors.getSquareColor()));
        p.setBrush(QBrush(colors.getSquareColor()));
        p.drawRect(x, y - _squareSize, _squareSize, _squareSize);
        p.setPen(QPen(colors.getLabelColor()));
        p.drawText(x, y - _squareSize, _squareSize, _squareSize,
                   AlignCenter, QString(QChar('0' + i)));
    }
    
    // Fill the unmarked label
    p.setFont(QFont("times", (int) (_squareSize * 0.25)));
    p.setPen(QPen(colors.getLabelColor()));
    p.setBrush(QBrush(colors.getSquareColor()));
    p.drawRect(_xOffset, 9 * _squareSize + _yOffset, _squareSize + 1, _squareSize);
    p.setPen(QPen(colors.getLabelColor()));
    p.drawText(_xOffset, 9 * _squareSize + _yOffset, _squareSize, _squareSize,
               AlignCenter, QString("FLIP"));


    // Draw each square
    p.setFont(QFont("times", (int) (_squareSize * 0.35), QFont::Bold));
    for(i = 0; i < 9; i++) {
        for(j = 0; j < 9; j++) {
            y = _squareSize * i + _yOffset;
            x = _squareSize * j + _squareSize + _xOffset;
            yc = y + (_squareSize >> 1);
            xc = x + (_squareSize >> 1);
	
            if((i == 2 && (j == 2 || j == 4 || j == 6)) ||
               (i == 6 && (j == 2 || j == 4 || j == 6))) {
                // Draw home squares
                p.setPen(QPen(colors.getLineColor()));
                p.setBrush(QBrush(colors.getLineColor()));
                p.drawRect(x, y, _squareSize, _squareSize);
                p.setPen(QPen(colors.getSquareColor()));
                p.drawText(x, y, _squareSize, _squareSize, AlignCenter,
                           QString("OCTI"));
            } else {
                // Draw dash lines in each square
                p.setPen(QPen(colors.getLineColor()));
                p.setBrush(NoBrush);
                p.drawRect(x, y, _squareSize + 1, _squareSize + 1);
                p.drawLine(xc, y, xc, y + _sl);
                p.drawLine(xc, y + _squareSize, xc, y + _squareSize - _sl);
                p.drawLine(x, yc, x + _sl, yc);
                p.drawLine(x + _squareSize, yc, x + _squareSize - _sl, yc);
                p.drawLine(x + _sl, y + _sl, x + _sl + _sl, y + _sl  + _sl);
                p.drawLine(x + _sl, y + _squareSize - _sl,
                           x + _sl + _sl, y + _squareSize - _sl - _sl);
                p.drawLine(x + _squareSize - _sl, y + _sl,
                           x + _squareSize - _sl - _sl, y + _sl + _sl);
                p.drawLine(x + _squareSize - _sl, y + _squareSize - _sl,
                           x + _squareSize - _sl - _sl, y + _squareSize - _sl - _sl);
            }
        }
    }
}

void OctiBoardDispWidget::paintInverted(QPainter &p) {
    OctiWidgetColors &colors = G_OctiOptions->getWidgetColors();
    int i, j;
    int x, y, xc, yc;

    // Draw the boarder
    p.setClipRect(_xOffset, _yOffset, _boardSize, _boardSize);
    p.setFont(QFont("times", (int) (_squareSize * 0.5), QFont::Bold));
    p.setPen(QPen(colors.getLineColor()));
    p.drawRect(_xOffset, _yOffset, _boardSize, _boardSize);
    
    // Draw y-axis labels
    for(i = 1; i <= 9; i++) {
        y = i * _squareSize + _yOffset;
        p.setPen(QPen(colors.getSquareColor()));
        p.setBrush(QBrush(colors.getSquareColor()));
        p.drawRect(_xOffset, y, _squareSize, _squareSize);
        p.setPen(QPen(colors.getLabelColor()));
        p.drawText(_xOffset, y, _squareSize, _squareSize, AlignCenter,
                   QString(QChar('0' + i)));
    }
    
    // Draw x-axis labels
    y = _yOffset;
    for(i = 1; i <= 9; i++) {
        x = i * _squareSize + _xOffset;
        p.setPen(QPen(colors.getSquareColor()));
        p.setBrush(QBrush(colors.getSquareColor()));
        p.drawRect(x, y, _squareSize, _squareSize);
        p.setPen(QPen(colors.getLabelColor()));
        p.drawText(x, y, _squareSize, _squareSize, AlignCenter,
                   QString(QChar('0' + i)));
    }
    
    // Fill the unmarked label
    p.setFont(QFont("times", (int) (_squareSize * 0.25)));
    p.setPen(QPen(colors.getLabelColor()));
    p.setBrush(QBrush(colors.getSquareColor()));
    p.drawRect(_xOffset, _yOffset, _squareSize + 1, _squareSize);
    p.setPen(QPen(colors.getLabelColor()));
    p.drawText(_xOffset, _yOffset, _squareSize, _squareSize, AlignCenter,
               QString("FLIP"));

    // Draw each square
    p.setFont(QFont("times", (int) (_squareSize * 0.35), QFont::Bold));
    for(i = 0; i < 9; i++) {
        for(j = 0; j < 9; j++) {
            y = _squareSize * i + _squareSize + _yOffset;
            x = _squareSize * j + _squareSize + _xOffset;
            yc = y + (_squareSize >> 1);
            xc = x + (_squareSize >> 1);
	
            if((i == 2 && (j == 2 || j == 4 || j == 6)) ||
               (i == 6 && (j == 2 || j == 4 || j == 6))) {
                // Draw home squares
                p.setPen(QPen(colors.getLineColor()));
                p.setBrush(QBrush(colors.getLineColor()));
                p.drawRect(x, y, _squareSize, _squareSize);
                p.setPen(QPen(colors.getSquareColor()));
                p.drawText(x, y, _squareSize, _squareSize, AlignCenter,
                           QString("OCTI"));
            } else {
                // Draw dash lines in each square
                p.setPen(QPen(colors.getLineColor()));
                p.setBrush(NoBrush);
                p.drawRect(x, y, _squareSize + 1, _squareSize + 1);
                p.drawLine(xc, y, xc, y + _sl);
                p.drawLine(xc, y + _squareSize, xc, y + _squareSize - _sl);
                p.drawLine(x, yc, x + _sl, yc);
                p.drawLine(x + _squareSize, yc, x + _squareSize - _sl, yc);
                p.drawLine(x + _sl, y + _sl, x + _sl + _sl, y + _sl  + _sl);
                p.drawLine(x + _sl, y + _squareSize - _sl,
                           x + _sl + _sl, y + _squareSize - _sl - _sl);
                p.drawLine(x + _squareSize - _sl, y + _sl,
                           x + _squareSize - _sl - _sl, y + _sl + _sl);
                p.drawLine(x + _squareSize - _sl, y + _squareSize - _sl,
                           x + _squareSize - _sl - _sl, y + _squareSize - _sl - _sl);

            }
        }
    }
}

void OctiBoardDispWidget::plotNormalMove(QPainter &p) {
    int i, j, x, y;

    if(_move != NULL) {
        p.setPen(QPen(yellow, (int) (_squareSize * 0.05), DashLine));
        int xc0, yc0, xc1, yc1;
        Ptr<OctiMoveOp> op;
        Dlist<Ptr<OctiMoveOp> > ops = _move->getOps();
        Ptr<DlistIterator<Ptr<OctiMoveOp> > > iterator =
            (DlistIterator<Ptr<OctiMoveOp> > *) ops.getIterator();
        for(iterator->first(); iterator->isNotNull(); iterator->next()) {
            op = *iterator->current();
            if(!(op->x0 == op->x1 && op->y0 == op->y1) &&
               op->prongAddDir == -1) {
                xc0 = _squareSize * (op->x0 + 1) + (_squareSize >> 1) + _xOffset;
                yc0 = _squareSize * (9 - op->y0) - (_squareSize >> 1) + _yOffset;
                xc1 = _squareSize * (op->x1 + 1) + (_squareSize >> 1) + _xOffset;
                yc1 = _squareSize * (9 - op->y1) - (_squareSize >> 1) + _yOffset;
                p.drawLine(xc0, yc0, xc1, yc1);
            }
        }
    }

    // Draw highlighted squares
    p.setBrush(NoBrush);
    for(i = 0; i < 9; i++) {
        for(j = 0; j < 9; j++) {
            y = _squareSize * (8 - i) + _yOffset;
            x = _squareSize * j + _squareSize + _xOffset;
            if(_highlightX == j && _highlightY == i &&
               _map.getMark(j, i) != DSM_JUMP) {
                p.setPen(QPen(yellow, 2));
                p.drawRect(x + 2, y + 2, _squareSize - 2, _squareSize - 2);
            } else {
                switch(_map.getMark(j, i)) {
                case DSM_HOP:
                case DSM_DEST:
                    p.setPen(QPen(yellow, 2));
                    p.drawRect(x + 2, y + 2, _squareSize - 2, _squareSize - 2);
                    break;
                case DSM_JUMP:
                    p.setPen(QPen(red, 2));
                    p.drawRect(x + 2, y + 2, _squareSize - 2, _squareSize - 2);
                    break;
                }
            }
        }
    }
}

void OctiBoardDispWidget::plotInvertedMove(QPainter &p) {
    int i, j, x, y;

    if(_move != NULL) {
        p.setPen(QPen(yellow, (int) (_squareSize * 0.05), DashLine));
        int xc0, yc0, xc1, yc1;
        Ptr<OctiMoveOp> op;
        Dlist<Ptr<OctiMoveOp> > ops = _move->getOps();
        Ptr<DlistIterator<Ptr<OctiMoveOp> > > iterator =
            (DlistIterator<Ptr<OctiMoveOp> > *) ops.getIterator();
        for(iterator->first(); iterator->isNotNull(); iterator->next()) {
            op = *iterator->current();
            if(!(op->x0 == op->x1 && op->y0 == op->y1) &&
               op->prongAddDir == -1) {
                xc0 = _squareSize * (op->x0 + 1) + (_squareSize >> 1) + _xOffset;
                yc0 = _squareSize * (op->y0 + 1) + (_squareSize >> 1) + _yOffset;
                xc1 = _squareSize * (op->x1 + 1) + (_squareSize >> 1) + _xOffset;
                yc1 = _squareSize * (op->y1 + 1) + (_squareSize >> 1) + _yOffset;
                p.drawLine(xc0, yc0, xc1, yc1);
            }
        }
    }

    // Draw highlighted squares
    p.setBrush(NoBrush);
    for(i = 0; i < 9; i++) {
        for(j = 0; j < 9; j++) {
            y = _squareSize * i + _squareSize + _yOffset;
            x = _squareSize * j + _squareSize + _xOffset;
            if(_highlightX == j && _highlightY == i) {
                p.setPen(QPen(yellow, 2));
                p.drawRect(x + 2, y + 2, _squareSize - 2, _squareSize - 2);
            } else {
                switch(_map.getMark(j, i)) {
                case DSM_HOP:
                case DSM_DEST:
                    p.setPen(QPen(yellow, 2));
                    p.drawRect(x + 2, y + 2, _squareSize - 2, _squareSize - 2);
                    break;
                case DSM_JUMP:
                    p.setPen(QPen(red, 2));
                    p.drawRect(x + 2, y + 2, _squareSize - 2, _squareSize - 2);
                    break;
                }
            }
        }
    }
}

boolean OctiBoardDispWidget::okToPaintPod(int i) const {
    if(_supressOn && _pods[i].sqrX == _supressX && _pods[i].sqrY == _supressY) {
        for(int j = 0; j < _numSupressPods; j++) {
            if(_pods[i].pod == _supressPods[j]) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

void OctiBoardDispWidget::paintNormalPods(QPainter &p) {
    int i;
    int overlapoffet = (int) (_squareSize * 0.05);
    for(i = 0; i < _numPods; i++) {
        _pods[i].locX = _squareSize * (_pods[i].sqrX + 1) + (_squareSize >> 1) +
            _xOffset;
        _pods[i].locY = _squareSize * (9 - _pods[i].sqrY) - (_squareSize >> 1) +
            _yOffset;
        if(okToPaintPod(i)) {
            paint_pod_at(p, _squareSize, _showInverted, _pods[i].pod,
                         _pods[i].locX - overlapoffet * _pods[i].offset,
                         _pods[i].locY - overlapoffet * _pods[i].offset);
        }
    }
}

void OctiBoardDispWidget::paintInvertedPods(QPainter &p) {
    int i;
    int overlapoffet = (int) (_squareSize * 0.05);
    for(i = 0; i < _numPods; i++) {
        _pods[i].locX = _squareSize * (_pods[i].sqrX + 1) + (_squareSize >> 1) +
            _xOffset;
        _pods[i].locY = _squareSize * (_pods[i].sqrY + 1) + (_squareSize >> 1) +
            _yOffset;
        if(okToPaintPod(i)) {
            paint_pod_at(p, _squareSize, _showInverted, _pods[i].pod,
                         _pods[i].locX - overlapoffet * _pods[i].offset,
                         _pods[i].locY - overlapoffet * _pods[i].offset);
        }
    }
}

void OctiBoardDispWidget::calcDispUnits() {
    int width = this->width();
    int height = this->height();
    _squareSize = ((width < height) ? (width / 10) : (height / 10));
    _boardSize = _squareSize * 10;
    _sl = (int) (_squareSize * 0.100);
    _xOffset = (width / 2) - (_squareSize * 5);
    _yOffset = (height / 2) - (_squareSize * 5);
}

void OctiBoardDispWidget::resizeEvent(QResizeEvent *e) {
    calcDispUnits();
    if(_sqrDisp != NULL) {
        _sqrDisp->setSquareSize(_squareSize);
    }

    if(_statDisp1 != NULL) {
        _statDisp1->setSquareSize(_squareSize);
    }

    if(_statDisp2 != NULL) {
        _statDisp2->setSquareSize(_squareSize);
    }

    if(_statDisp3 != NULL) {
        _statDisp3->setSquareSize(_squareSize);
    }

    if(_statDisp4 != NULL) {
        _statDisp4->setSquareSize(_squareSize);
    }
}

void OctiBoardDispWidget::getGridPos(int x, int y, int &gridX, int &gridY) {
    if(x < _xOffset || x > _boardSize + _xOffset) {
        x = -1;
    } else {
        x -= _xOffset;
        gridX = (x - (x % _squareSize)) * 10 / _boardSize - 1;
    }

    if(y < _yOffset || y > _boardSize + _yOffset) {
        y = -1;
    } else {
        y -= _yOffset;
        if(!_showInverted) {
            gridY =  9 - (y - (y % _squareSize)) * 10 / _boardSize - 1;
        } else {
            gridY = (y - (y % _squareSize)) * 10 / _boardSize - 1;
        }
    }
}

boolean OctiBoardDispWidget::acceptAddPodAt(int targetX, int targetY) const {
    OctiGameMode gameMode = _board->getGameMode();
    OctiPlayer playerToMove = _board->getGame()->getPlayerToMove();
  
    if(gameMode == GAME_MODE_2) {
        if(playerToMove == PLAYER_A) {
            if(targetY == 2 && (targetX == 2 || targetX == 4 || targetX == 6)) {
                return TRUE;
            } else if(targetY == 6 && (targetX == 2 || targetX == 4 || targetX == 6) &&
                      _board->squareAt(targetX, targetY).isCapturedBy(playerToMove)) {
                return TRUE;
            } else {
                return FALSE;
            }
        } else {
            if(targetY == 6 && (targetX == 2 || targetX == 4 || targetX == 6)) {
                return TRUE;
            } else if(targetY == 2 && (targetX == 2 || targetX == 4 || targetX == 6) &&
                      _board->squareAt(targetX, targetY).isCapturedBy(playerToMove)) {
                return TRUE;
            } else {
                return FALSE;
            }
        }
    } else {
        // NOT IMPLEMENTED
    }
    return FALSE;
}

int OctiBoardDispWidget::acceptMovePodsAt(int targetX, int targetY,
                                          int &jumpX, int &jumpY) const {
    if(targetX < 0 || targetX >= 9 || targetY < 0 || targetY >= 9) {
        return 0;
    }

    //OctiGameMode gameMode = _board->getGameMode();
    OctiPlayer playerToMove = _board->getGame()->getPlayerToMove();
    OctiSquare *targetSquare = &_board->squareAt(targetX, targetY);
    OctiSquare *jumpSquare = NULL;
    boolean edgeless = _board->getGame()->useEdgelessBoard();

    OctiHop hop = _map.getLastMarkedOrigOrHop();
    int startX = hop.x;
    int startY = hop.y;
    int xdisplacement = targetX - startX;
    int ydisplacement = targetY - startY;
  
    if((startX == targetX && startY == targetY) ||
       targetSquare->hasEnemyOf(playerToMove)) {
        return 0;
    }

    if(edgeless) {
        if(xdisplacement < -2) {
            xdisplacement += 9;
        } else if(xdisplacement > 2) {
            xdisplacement -= 9;
        }
        if(ydisplacement < -2) {
            ydisplacement += 9;
        } else if(ydisplacement > 2) {
            ydisplacement -= 9;
        }
    }

    if(_map.numHops() == 0 &&
       xdisplacement <= 1 && xdisplacement >= -1 &&
       ydisplacement <= 1 && ydisplacement >= -1) {
        if((xdisplacement == 0 && ydisplacement == 1 &&
            _generalizedPod.prongAt(DIR_N) != NO_PRONG) ||
           (xdisplacement == 1 && ydisplacement == 1 &&
            _generalizedPod.prongAt(DIR_NE) != NO_PRONG) ||
           (xdisplacement == 1 && ydisplacement == 0 &&
            _generalizedPod.prongAt(DIR_E) != NO_PRONG) ||
           (xdisplacement == 1 && ydisplacement == -1 &&
            _generalizedPod.prongAt(DIR_SE) != NO_PRONG) ||
           (xdisplacement == 0 && ydisplacement == -1 &&
            _generalizedPod.prongAt(DIR_S) != NO_PRONG) ||
           (xdisplacement == -1 && ydisplacement == -1 &&
            _generalizedPod.prongAt(DIR_SW) != NO_PRONG) ||
           (xdisplacement == -1 && ydisplacement == 0 &&
            _generalizedPod.prongAt(DIR_W) != NO_PRONG) ||
           (xdisplacement == -1 && ydisplacement == 1 &&
            _generalizedPod.prongAt(DIR_NW) != NO_PRONG)) {
            return 1;
        }
    } else if(xdisplacement <= 2 && xdisplacement >= -2 &&
              ydisplacement <= 2 && ydisplacement >= -2) {
        jumpX = (startX + (xdisplacement / 2)) % 9;
        jumpY = (startY + (ydisplacement / 2)) % 9;
        jumpSquare = &_board->squareAt(jumpX, jumpY);
        if(!jumpSquare->isEmpty()) {
            if((xdisplacement == 0 && ydisplacement == 2 &&
                _generalizedPod.prongAt(DIR_N) != NO_PRONG) ||
               (xdisplacement == 2 && ydisplacement == 2 &&
                _generalizedPod.prongAt(DIR_NE) != NO_PRONG) ||
               (xdisplacement == 2 && ydisplacement == 0 &&
                _generalizedPod.prongAt(DIR_E) != NO_PRONG) ||
               (xdisplacement == 2 && ydisplacement == -2 &&
                _generalizedPod.prongAt(DIR_SE) != NO_PRONG) ||
               (xdisplacement == 0 && ydisplacement == -2 &&
                _generalizedPod.prongAt(DIR_S) != NO_PRONG) ||
               (xdisplacement == -2 && ydisplacement == -2 &&
                _generalizedPod.prongAt(DIR_SW) != NO_PRONG) ||
               (xdisplacement == -2 && ydisplacement == 0 &&
                _generalizedPod.prongAt(DIR_W) != NO_PRONG) ||
               (xdisplacement == -2 && ydisplacement == 2 &&
                _generalizedPod.prongAt(DIR_NW) != NO_PRONG)) {
                return 2;
            }
        }
    }

    return 0;
}

void OctiBoardDispWidget::updateGeneralizedPod() {
    _numSupressPods = _sqrDisp->getSelectedPods(_supressPods);
    _generalizedPod.addProng(DIR_N, FALSE);
    _generalizedPod.addProng(DIR_NE, FALSE);
    _generalizedPod.addProng(DIR_E, FALSE);
    _generalizedPod.addProng(DIR_SE, FALSE);
    _generalizedPod.addProng(DIR_S, FALSE);
    _generalizedPod.addProng(DIR_SW, FALSE);
    _generalizedPod.addProng(DIR_W, FALSE);
    _generalizedPod.addProng(DIR_NW, FALSE);
    for(int i = 0; i < _numSupressPods; i++) {
        for(int j = 0; j < 8; j++) {
            if(_supressPods[i].prongAt((OctiDir) j) == NO_PRONG) {
                _generalizedPod.removeProng((OctiDir) j);
            }
        }
    }
}

void OctiBoardDispWidget::mousePressEvent(QMouseEvent *e) {
    _pressPos = e->pos();
}

void OctiBoardDispWidget::mouseReleaseEvent(QMouseEvent *e) {
    int gridX = 10, gridY = 10;
    int pressX, pressY;
    getGridPos(e->x(), e->y(), gridX, gridY);
    getGridPos(_pressPos.x(), _pressPos.y(), pressX, pressY);

    if(gridX == pressX && gridY == pressY) {
        if(gridX > -1 && gridX < 9 && gridY > -1 && gridY < 9) {
            _sqrDisp->setDisplaySquare(gridX, gridY);
            _highlightX = gridX;
            _highlightY = gridY;
            _move = NULL;
            repaint(FALSE);
        } else if(gridX == -1 && gridY == -1) {
            setShowInverted(!_showInverted);
        }
    }
}

void OctiBoardDispWidget::mouseMoveEvent(QMouseEvent *e) {
    if(e->state() & LeftButton) {
        if(QABS(e->pos().x() - _pressPos.x()) >= 3 ||
           QABS(e->pos().y() - _pressPos.y()) >= 3) {
            int overlapoffet = (int) (_squareSize * 0.05);
            int gridX = 10, gridY = 10;
            OctiPod pods[7];
            int i, numPods;
            getGridPos(_pressPos.x(), _pressPos.y(), gridX, gridY);

            if(!_board->getGame()->isPrepareLocked()) {
                _sqrDisp->setDisplaySquare(gridX, gridY);
                _move = NULL;
                _map.markOrigin(gridX, gridY);
                _highlightX = gridX;
                _highlightY = gridY;
                updateGeneralizedPod();
                repaint(FALSE);
            }

            if(gridX > -1 && gridX < 9 && gridY > -1 && gridY < 9 &&
               (numPods = _sqrDisp->getSelectedPods(pods)) > 0 &&
               _board->getGame()->getPlayerToMove() == (OctiPlayer) pods[0].getType()) {
                QBitmap mask(_squareSize, _squareSize);
                QPixmap pixmap(_squareSize, _squareSize);
                pixmap.fill();
                mask.fill(Qt::black);
                QPainter p(&pixmap);
                for(i = 0; i < numPods; i++) {
                    paint_pod_at(p, _squareSize, _showInverted, pods[i],
                                 (_squareSize >> 1) - (overlapoffet * i),
                                 (_squareSize >> 1) - (overlapoffet * i),
                                 &mask, FALSE);
                }
                pixmap.setMask(mask);

                DispSquareMark mark = _map.getMark(gridX, gridY);
                QString s;
	
                if(mark == DSM_JUMP) {
                    _supressOn = TRUE;
                    _supressX = gridX;
                    _supressY = gridY;
                    repaint(FALSE);

                    s = "CAPTURE";
                    QTextDrag *drag = new QTextDrag(s, this);
                    drag->setPixmap(pixmap, QPoint((int) (_squareSize * 0.5), (int) (_squareSize * 0.5)));
                    drag->dragMove();
                } else if(mark == DSM_ORIGIN) {
                    _supressOn = TRUE;
                    _supressX = gridX;
                    _supressY = gridY;
                    repaint(FALSE);

                    s = "MOVE";
                    QTextDrag *drag = new QTextDrag(s, this);
                    drag->setPixmap(pixmap, QPoint((int) (_squareSize * 0.5), (int) (_squareSize * 0.5)));
                    drag->dragMove();
                }
            }
        }
    }
}

void OctiBoardDispWidget::dragEnterEvent(QDragEnterEvent *e) {

}

void OctiBoardDispWidget::dragMoveEvent(QDragMoveEvent *e) {
    if(QTextDrag::canDecode(e)) {
        e->accept();

        QString s;
        QTextDrag::decode(e, s);

        int targetX = 10, targetY = 10;
        getGridPos(e->pos().x(), e->pos().y(), targetX, targetY);

        if(s == "POD") {
            if(acceptAddPodAt(targetX, targetY)) {
                _sqrDisp->setDisplaySquare(targetX, targetY);
                _map.clearHops();
                _map.markDest(targetX, targetY);
                repaint(FALSE);
            }
        } else if(s == "MOVE") {
            int jumpX, jumpY;
            int acceptMove = acceptMovePodsAt(targetX, targetY, jumpX, jumpY);
            if(acceptMove == 1) {
                _map.clearHops();
                _map.markDest(targetX, targetY);
                repaint(FALSE);
            } else if(acceptMove == 2) {
                _map.markJump(jumpX, jumpY);
                _map.markHop(targetX, targetY);
                repaint(FALSE);
            }
        }
    }
}

void OctiBoardDispWidget::dragLeaveEvent(QDragLeaveEvent *e) {
    _map.clearHops();
    repaint(FALSE);
}

void OctiBoardDispWidget::dropEvent(QDropEvent *e) {
    if(QTextDrag::canDecode(e)) {
        e->accept();

        //OctiPlayer playerToMove = _board->getGame()->getPlayerToMove();
        QString s;
        QTextDrag::decode(e, s);

        int targetX = 10, targetY = 10;
        getGridPos(e->pos().x(), e->pos().y(), targetX, targetY);

        if(s == "POD") {
            if(acceptAddPodAt(targetX, targetY)) {
                try {
                    _board->getGame()->prepareAddPod(targetX, targetY);
                } catch(IllegalMoveException e) {
                    _map.clearAll();
                    cerr << e.getMessage() << endl;
                }
            }
        } else if(s == "MOVE") {
            int i, numHops = _map.numHops();
            OctiHop orig = _map.getOrigin();
            OctiHop dest = _map.getDest();
            Dlist<OctiHop> hops;
            if(dest.x == targetX && dest.y == targetY) {
                if(numHops == 0) {
                    hops.append(dest);
                } else {
                    for(i = 0; i < numHops; i++) {
                        hops.append(_map.getHop(i));
                    }
                }

                for(i = 0; i < _numSupressPods; i++) {
                    try {
                        _map.saveJumps();
                        _board->getGame()->prepareHops(orig.x, orig.y, _supressPods[i], hops);
                    } catch(IllegalMoveException e) {
                        _map.clearAll();
                    }
                }
            }
        }
    }

    _map.clearHops();
    _supressOn = FALSE;
    repaint(FALSE);
}

void OctiBoardDispWidget::gameChanged(OctiBoard *src, OctiGame *oldGame, OctiGame *newGame) {
    switch(_board->getGameMode()) {
    case GAME_MODE_2:
        if(_showInverted) {
            _statDisp1->setPlayer(PLAYER_A, _showInverted);
            _statDisp2->setPlayer(PLAYER_UNDEF, _showInverted);
            _statDisp3->setPlayer(PLAYER_UNDEF, _showInverted);
            _statDisp4->setPlayer(PLAYER_B, _showInverted);
        } else {
            _statDisp1->setPlayer(PLAYER_B, _showInverted);
            _statDisp2->setPlayer(PLAYER_UNDEF, _showInverted);
            _statDisp3->setPlayer(PLAYER_UNDEF, _showInverted);
            _statDisp4->setPlayer(PLAYER_A, _showInverted);
        }
        break;
    case GAME_MODE_4:
        if(_showInverted) {
            _statDisp1->setPlayer(PLAYER_A, _showInverted);
            _statDisp2->setPlayer(PLAYER_C, _showInverted);
            _statDisp3->setPlayer(PLAYER_D, _showInverted);
            _statDisp4->setPlayer(PLAYER_B, _showInverted);
        } else {
            _statDisp1->setPlayer(PLAYER_B, _showInverted);
            _statDisp2->setPlayer(PLAYER_D, _showInverted);
            _statDisp3->setPlayer(PLAYER_C, _showInverted);
            _statDisp4->setPlayer(PLAYER_A, _showInverted);
        }
        break;
    }

    _move = NULL;
    _map.clearAll();
    _highlightX = -1;
    _highlightY = -1;
}

void OctiBoardDispWidget::boardUpdated(OctiBoard *src, const OctiMove *move,
                                       boolean undo) {
    int i, j, k;
    OctiSquare *square;
    if(_board->getGame()->isPrepareLocked()) {
        cout << "here" << endl;
        _map.clearHops();
    } else {
        cout << "destroy" << endl;
        _map.clearAll();
    }

    if(move) {
        _move = new OctiMove(*move);
        Ptr<OctiMoveOp> op;
        Dlist<Ptr<OctiMoveOp> > ops = _move->getOps();
        Ptr<DlistIterator<Ptr<OctiMoveOp> > > iterator =
            (DlistIterator<Ptr<OctiMoveOp> > *) ops.getIterator();
        iterator->last();
        if(iterator->isNotNull()) {
            op = *iterator->current();
            if(!(op->x0 == op->x1 && op->y0 == op->y1) &&
               op->prongAddDir == DIR_UNDEF && !undo) {
                _highlightX = op->x1;
                _highlightY = op->y1;
                _sqrDisp->setDisplaySquare(_highlightX, _highlightY);
            } else {
                op = *iterator->first();
                _highlightX = op->x0;
                _highlightY = op->y0;
                _sqrDisp->setDisplaySquare(_highlightX, _highlightY);
            }
        }
    }

    _numPods = 0;
    for(i = 0; i < 9; i++) {
        for(j = 0; j < 9; j++) {
            square = &_board->squareAt(i, j);
            for(k = 0; k < square->getNumPods(); k++) {
                _pods[_numPods].pod = *square->getPodAt(k);
                _pods[_numPods].sqrX = i;
                _pods[_numPods].sqrY = j;
                _pods[_numPods].offset = k;
                _numPods++;
            }
        }
    }
    repaint(FALSE);
}

OctiBoardWidget::OctiBoardWidget(QWidget *parent, const char *name, WFlags f) : QWidget(parent, name, f) {
    _squareDisp = new OctiSquareDispWidget(this);
    _statDisp1 = new OctiStatDispWidget(this);
    _statDisp2 = new OctiStatDispWidget(this);
    _statDisp3 = new OctiStatDispWidget(this);
    _statDisp4 = new OctiStatDispWidget(this);
    _boardDisp = new OctiBoardDispWidget(this);
    _statDisp1->setPlayer(PLAYER_B, FALSE);
    _statDisp2->setPlayer(PLAYER_UNDEF, FALSE);
    _statDisp3->setPlayer(PLAYER_UNDEF, FALSE);
    _statDisp4->setPlayer(PLAYER_A, FALSE);
    _boardDisp->setChildDisp(_squareDisp, _statDisp1, _statDisp2, _statDisp3, _statDisp4);

    QHBoxLayout *hlayout1 = new QHBoxLayout(this);
    QVBoxLayout *vlayout1 = new QVBoxLayout();
  
    hlayout1->addWidget(_boardDisp, 10);
    hlayout1->addWidget(_squareDisp, 1);
    hlayout1->addLayout(vlayout1, 4);
    vlayout1->addWidget(_statDisp1);
    vlayout1->addWidget(_statDisp2);
    vlayout1->addWidget(_statDisp3);
    vlayout1->addWidget(_statDisp4);

    _squareDisp->setBoard(&_board);
    _statDisp1->setBoard(&_board);
    _statDisp2->setBoard(&_board);
    _statDisp3->setBoard(&_board);
    _statDisp4->setBoard(&_board);
    _boardDisp->setBoard(&_board);
}

OctiBoardWidget::~OctiBoardWidget() {
    delete _squareDisp;
    delete _statDisp1;
    delete _statDisp2;
    delete _statDisp3;
    delete _statDisp4;
    delete _boardDisp;
}

__END_NAMESPACE(SELFSOFT);

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
