/*************************************************************************
 * $Id: OctiBoardDispWidget.hxx,v 1.3 2001/06/19 00:45:46 samurai Exp $
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

#ifndef _OCTIBOARDDISPWIDGET_HXX_
#define _OCTIBOARDDISPWIDGET_HXX_

#include <qframe.h>
#include <qbitmap.h>
#include <qpixmap.h>
#include "OctiPod.hxx"
#include "BufferedArray.hxx"
#include "OctiSquare.hxx"
#include "OctiBoard.hxx"
#include "OctiGame.hxx"
#include "OctiMove.hxx"
#include "OctiSquareDispWidget.hxx"
#include "OctiStatDispWidget.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

enum DispSquareMark {
  DSM_UNMARKED = 0x0,
  DSM_ORIGIN   = 0x1,
  DSM_JUMP     = 0x2,
  DSM_HOP      = 0x4,
  DSM_DEST     = 0x8,
  DSM_OLD_JUMP = 0x10
};

class OCTIAPI OctiBoardDispDirtyMap {

public:

  OctiBoardDispDirtyMap();
  ~OctiBoardDispDirtyMap();

  void markOrigin(int x, int y);
  void markJump(int x, int y);
  void markHop(int x, int y);
  void markDest(int x, int y);
  void saveJumps();
  void clearHops();
  void clearAll();

  DispSquareMark getMark(int x, int y) const;
  OctiHop getLastMarkedOrigOrHop() const;
  OctiHop getOrigin() const;
  OctiHop getDest() const;
  OctiHop getHop(int i) const;
  int numHops() const;

  boolean mapJumped() const;
  boolean mapMoved() const;

private:

  int _map[9][9];
  OctiHop _orig;
  OctiHop _dest;
  BufferedArray<OctiHop> _hops;

};

class OCTIAPI OctiBoardDispWidget : public QWidget,
				    protected OctiBoardListener {

  Q_OBJECT;

public:

  OctiBoardDispWidget(QWidget *parent = NULL, const char *name = NULL, WFlags f = 0);
  ~OctiBoardDispWidget();

  int heightForWidth(int w) const;

  void setShowInverted(boolean inverted);
  boolean isShowInverted() const;
  void setAnimateMove(boolean animate);
  boolean isAnimateMove() const;

  int getSquareSize() const;
  int getBoardSize() const;
  
protected:

  void paintEvent(QPaintEvent *e);
  void resizeEvent(QResizeEvent *e);
  void getGridPos(int x, int y, int &gridX, int &gridY);
  void mousePressEvent(QMouseEvent *e);
  void mouseReleaseEvent(QMouseEvent *e);
  void mouseMoveEvent(QMouseEvent *e);
  void dragEnterEvent(QDragEnterEvent *e);
  void dragMoveEvent(QDragMoveEvent *e);
  void dragLeaveEvent(QDragLeaveEvent *e);
  void dropEvent(QDropEvent *e);

private:

  boolean acceptAddPodAt(int targetX, int targetY) const;
  int acceptMovePodsAt(int targetX, int targetY, int &jumpX, int &jumpY) const;
  void updateGeneralizedPod();

private:

  void paintNormal(QPainter &p);
  void paintInverted(QPainter &p);
  void plotNormalMove(QPainter &p);
  void plotInvertedMove(QPainter &p);
  boolean okToPaintPod(int i) const;
  void paintNormalPods(QPainter &p);
  void paintInvertedPods(QPainter &p);

private:

  friend class OctiBoardWidget;

  void setBoard(OctiBoard *board);
  void setChildDisp(OctiSquareDispWidget *sqrDisp,
		    OctiStatDispWidget *statDisp1,
		    OctiStatDispWidget *statDisp2,
		    OctiStatDispWidget *statDisp3,
		    OctiStatDispWidget *statDisp4);

  void calcDispUnits();

protected:

  void gameChanged(OctiBoard *src, OctiGame *oldGame, OctiGame *newGame);
  void boardUpdated(OctiBoard *src, const OctiMove *move, boolean undo);

private:

  typedef struct _pod_render_t {
    OctiPod pod;
    int sqrX, sqrY;
    int locX, locY;
    int offset;
  } pod_render_t;

  OctiBoard *_board;
  OctiSquareDispWidget *_sqrDisp;
  OctiStatDispWidget *_statDisp1, *_statDisp2, *_statDisp3, *_statDisp4;
  int _squareSize;
  int _boardSize;
  int _sl;
  int _xOffset;
  int _yOffset;
  boolean _showInverted;
  boolean _animate;
  pod_render_t _pods[28];
  int _numPods;
  Ptr<OctiMove> _move;
  OctiBoardDispDirtyMap _map;
  QPoint _pressPos;
  OctiPod _generalizedPod;
  int _highlightX;
  int _highlightY;
  boolean _supressOn;
  int _supressX;
  int _supressY;
  OctiPod _supressPods[7];
  int _numSupressPods;

};

class OCTIAPI OctiBoardWidget : public QWidget {
  Q_OBJECT;

public:

  OctiBoardWidget(QWidget *parent = NULL, const char *name = NULL, WFlags f = 0);
  ~OctiBoardWidget();

  OctiBoard *getBoard();

private:

  OctiSquareDispWidget *_squareDisp;
  OctiStatDispWidget *_statDisp1;
  OctiStatDispWidget *_statDisp2;
  OctiStatDispWidget *_statDisp3;
  OctiStatDispWidget *_statDisp4;
  OctiBoardDispWidget *_boardDisp;

  OctiBoard _board;

};

// Inline functions

inline OctiBoardDispDirtyMap::OctiBoardDispDirtyMap() {
  for(int x = 0; x < 9; x++) {
    for(int y = 0; y < 9; y++) {
      _map[x][y] = DSM_UNMARKED;
    }
  }
  _orig.x = _orig.y = _dest.x = _dest.y = -1;
  _hops.clear();
}

inline OctiBoardDispDirtyMap::~OctiBoardDispDirtyMap() {

}

inline void OctiBoardDispDirtyMap::markOrigin(int x, int y) {
  if(x >= 0 && x < 9 && y >= 0 && y < 9) {
    if(_orig.x >= 0 && _orig.x < 9 && _orig.y >= 0 && _orig.y < 9) {
      _map[_orig.x][_orig.y] &= DSM_OLD_JUMP;
    }
    _map[x][y] = (DSM_ORIGIN | (_map[x][y] & DSM_OLD_JUMP));
    _orig.x = x;
    _orig.y = y;
    _dest.x = _dest.y = -1;
  }
}

inline void OctiBoardDispDirtyMap::markJump(int x, int y) {
  if(x >= 0 && x < 9 && y >= 0 && y < 9) {
    _map[x][y] = (DSM_JUMP | (_map[x][y] & DSM_OLD_JUMP));
  }
}

inline void OctiBoardDispDirtyMap::markHop(int x, int y) {
  if(x >= 0 && x < 9 && y >= 0 && y < 9) {
    _map[x][y] = (DSM_HOP | (_map[x][y] & DSM_OLD_JUMP));
    _dest.x = x;
    _dest.y = y;
    _hops.append(OctiHop(x, y));
  }
}

inline void OctiBoardDispDirtyMap::markDest(int x, int y) {
  if(x >= 0 && x < 9 && y >= 0 && y < 9) {
    _map[x][y] = (DSM_DEST | (_map[x][y] & DSM_OLD_JUMP));
    _hops.clear();
    _dest.x = x;
    _dest.y = y;
  }
}

inline void OctiBoardDispDirtyMap::saveJumps() {
  for(int i = 0; i < 9; i++) {
    for(int j = 0; j < 9; j++) {
      if((_map[i][j] & DSM_JUMP) != 0x0) {
	_map[i][j] |= DSM_OLD_JUMP;
      }
    }
  }
}

inline void OctiBoardDispDirtyMap::clearHops() {
  for(int i = 0; i < 9; i++) {
    for(int j = 0; j < 9; j++) {
      _map[i][j] &= (DSM_ORIGIN | DSM_OLD_JUMP);
    }
  }
  _dest.x = _dest.y = -1;
  _hops.clear();
}

inline void OctiBoardDispDirtyMap::clearAll() {
  for(int x = 0; x < 9; x++) {
    for(int y = 0; y < 9; y++) {
      _map[x][y] = DSM_UNMARKED;
    }
  }
  _orig.x = _orig.y = _dest.x = _dest.y = -1;
  _hops.clear();
}

inline DispSquareMark OctiBoardDispDirtyMap::getMark(int x, int y) const {
  if((_map[x][y] & DSM_OLD_JUMP) != 0x0) {
    return (DispSquareMark) DSM_JUMP;
  }

  return (DispSquareMark) (_map[x][y] & 0xf);
}

inline OctiHop OctiBoardDispDirtyMap::getLastMarkedOrigOrHop() const {
  if(_hops.size() == 0) {
    return _orig;
  } else {
    return _hops[_hops.size() - 1];
  }
}

inline OctiHop OctiBoardDispDirtyMap::getOrigin() const {
  return _orig;
}

inline OctiHop OctiBoardDispDirtyMap::getDest() const {
  return _dest;
}

inline OctiHop OctiBoardDispDirtyMap::getHop(int i) const {
  return _hops[i];
}

inline int OctiBoardDispDirtyMap::numHops() const {
  return _hops.size();
}

inline boolean OctiBoardDispDirtyMap::mapJumped() const {
  return _hops.size() > 0;
}

inline boolean OctiBoardDispDirtyMap::mapMoved() const {
  return _dest.x != -1 && _dest.y != -1 && _hops.size() == 0;
}

inline void OctiBoardDispWidget::setBoard(OctiBoard *board) {
  if(board != NULL && _board != board) {
    if(_board != NULL) {
      _board->removeOctiBoardListener(this);
    }
    _board = board;
    _board->addOctiBoardListener(this);
  }
}

inline void OctiBoardDispWidget::setChildDisp(OctiSquareDispWidget *sqrDisp,
					      OctiStatDispWidget *statDisp1,
					      OctiStatDispWidget *statDisp2,
					      OctiStatDispWidget *statDisp3,
					      OctiStatDispWidget *statDisp4) {
  if(sqrDisp != NULL && _sqrDisp != sqrDisp) {
    _sqrDisp = sqrDisp;
    _sqrDisp->setSquareSize(_squareSize);
  }

  if(statDisp1 != NULL && _statDisp1 != statDisp1) {
    _statDisp1 = statDisp1;
    _statDisp1->setSquareSize(_squareSize);
  }

  if(statDisp2 != NULL && _statDisp2 != statDisp2) {
    _statDisp2 = statDisp2;
    _statDisp2->setSquareSize(_squareSize);
  }

  if(statDisp3 != NULL && _statDisp3 != statDisp3) {
    _statDisp3 = statDisp3;
    _statDisp3->setSquareSize(_squareSize);
  }

  if(statDisp4 != NULL && _statDisp4 != statDisp4) {
    _statDisp4 = statDisp4;
    _statDisp4->setSquareSize(_squareSize);
  }
}

inline boolean OctiBoardDispWidget::isShowInverted() const {
  return _showInverted;
}

inline void OctiBoardDispWidget::setAnimateMove(boolean animate) {
  _animate = animate;
}

inline boolean OctiBoardDispWidget::isAnimateMove() const {
  return _animate;
}

inline int OctiBoardDispWidget::getSquareSize() const {
  return _squareSize;
}

inline int OctiBoardDispWidget::getBoardSize() const {
  return _boardSize;
}

inline OctiBoard *OctiBoardWidget::getBoard() {
  return &_board;
}

__END_NAMESPACE(SELFSOFT);

#endif
