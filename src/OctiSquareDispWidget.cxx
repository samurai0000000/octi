/*************************************************************************
 * $Id: OctiSquareDispWidget.cxx,v 1.5 2001/08/24 03:00:35 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:00:35 $
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

#include <qwmatrix.h>
#include <qpainter.h>
#include <qdragobject.h>
#include <qdropsite.h>
#include "OctiWidgetColors.hxx"
#include "OctiWidgetOptions.hxx"
#include "OctiSquareDispWidget.hxx"
#include "OctiGame.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

void paint_pod_at(QPainter &p, int sqsize, boolean invert,
		  OctiPod pod, int x, int y, QBitmap *mask, boolean fillMask) {
  OctiWidgetColors &colors = G_OctiOptions->getWidgetColors();
  int i;
  int sl = (int) (sqsize * 0.1);
  int sp1 = (int) (sqsize * 0.083);
  int sp2 = (int) (sqsize * 0.125);
  int sp3 = (int) (sqsize * 0.250);
  int sp4 = (int) (sqsize * 0.400);
  OctiPodType podType = pod.getType();
  OctiProngType prongType;
  QPointArray octagon, rotRect, prongRect;
  QWMatrix m;
  QPainter mp;

  if(mask) {
    if(fillMask) {
      mask->fill(Qt::black);
    }
    mp.begin(mask);
    mp.setPen(QPen(Qt::white));
    mp.setBrush(QBrush(Qt::white));
  }

  octagon.setPoints(8,
		    x + sp2, y - sp3,
		    x + sp3, y - sp2,
		    x + sp3, y + sp2,
		    x + sp2, y + sp3,
		    x - sp2, y + sp3,
		    x - sp3, y + sp2,
		    x - sp3, y - sp2,
		    x - sp2, y - sp3);

  prongRect.setPoints(4, -sp1, -sp4, sp1, -sp4, sp1, 0, -sp1, 0);
  
  m.translate(x, y);
  if(invert) {
    m.rotate(180);
  }
  for(i = 0; i < 8; i++) {
    prongType = pod.prongAt((OctiDir) i);
    if(prongType != NO_PRONG) {
      rotRect = m.map(prongRect);
      switch(prongType) {
      case NORMAL_PRONG:
	p.setPen(QPen(Qt::black));
	p.setBrush(QBrush(Qt::white));
	p.drawPolygon(rotRect);
	break;
      case SUPER_PRONG:
	p.setPen(QPen(Qt::black));
	p.setBrush(QBrush(Qt::white));
	p.drawPolygon(rotRect);
	break;
      }
      if(mask) {
	mp.drawPolygon(rotRect);
      }
    }
    m.rotate(invert ? -45 : 45);
  }

  p.setPen(QPen(Qt::white));
  switch(pod.getType()) {
  case A_POD:
    p.setBrush(QBrush(colors.getAPodColor()));
    break;
  case B_POD:
    p.setBrush(QBrush(colors.getBPodColor()));
    break;
  case C_POD:
    p.setBrush(QBrush(colors.getCPodColor()));
    break;
  case D_POD:
    p.setBrush(QBrush(colors.getDPodColor()));
    break;
  }
  p.drawPolygon(octagon);
  p.drawLine(x, y - sp2, x, y + sp2);
  if((invert && (podType == A_POD || podType == C_POD)) ||
     (!invert && (podType == B_POD || podType == D_POD))) {
    p.drawLine(x, y + sp2,
	       x + sl, y + sp2 - sl);
    p.drawLine(x, y + sp2,
	       x - sl, y + sp2 - sl);
  } else {
    p.drawLine(x, y - sp2,
	       x + sl, y - sp2 + sl);
    p.drawLine(x, y - sp2,
	       x - sl, y - sp2 + sl);
  }

  if(mask) {
    mp.drawPolygon(octagon);
    mp.end();
  }
}

void paint_pronghighlight_at(QPainter &p, int sqsize, boolean invert,
			     OctiDir dir, int x, int y) {
  int halfsq = sqsize >> 1;
  int sp2 = (int) (sqsize * 0.125);
  QRect rect;

  if(invert) {
    switch(dir) {
    case DIR_N:  dir = DIR_S;  break;
    case DIR_NE: dir = DIR_SE; break;
    case DIR_SE: dir = DIR_NE; break;
    case DIR_S:  dir = DIR_N;  break;
    case DIR_SW: dir = DIR_NW; break;
    case DIR_NW: dir = DIR_SW; break;
    }
  }

  switch(dir) {
  case DIR_N:
    rect.setCoords(x - sp2, y - halfsq, x + sp2, y - sp2);
    break;
  case DIR_NE:
    rect.setCoords(x + sp2, y - halfsq, x + halfsq, y - sp2);
    break;
  case DIR_E:
    rect.setCoords(x + sp2, y - sp2, x + halfsq, y + sp2);
    break;
  case DIR_SE:
    rect.setCoords(x + sp2, y + sp2, x + halfsq, y + halfsq);
    break;
  case DIR_S:
    rect.setCoords(x - sp2, y + sp2, x + sp2, y + halfsq);
    break;
  case DIR_SW:
    rect.setCoords(x - halfsq, y + sp2, x - sp2, y + halfsq);
    break;
  case DIR_W:
    rect.setCoords(x - halfsq, y - sp2, x - sp2, y + sp2);
    break;
  case DIR_NW:
    rect.setCoords(x - halfsq, y - halfsq, x - sp2, y - sp2);
    break;
  default:
    return;
  }

  p.setPen(QPen(Qt::yellow));
  p.setBrush(QBrush(Qt::yellow));
  p.drawRect(rect);
}

boolean prong_pos(int sqsize, boolean invert,
		  int &sq, OctiDir &dir, int x, int y, int posX, int posY) {
  int halfsq = sqsize >> 1;
  int sp2 = (int) (sqsize * 0.125);

  int xn0 = x - sp2, yn0 = y - halfsq, xn1 = x + sp2, yn1 = y - sp2;
  int xne0 = x + sp2, yne0 = y - halfsq, xne1 = x + halfsq, yne1 = y - sp2;
  int xe0 = x + sp2, ye0 = y - sp2, xe1 = x + halfsq, ye1 = y + sp2;
  int xse0 = x + sp2, yse0 = y + sp2, xse1 = x + halfsq, yse1 = y + halfsq;
  int xs0 = x - sp2, ys0 = y + sp2, xs1 = x + sp2, ys1 = y + halfsq;
  int xsw0 = x - halfsq, ysw0 = y + sp2, xsw1 = x - sp2, ysw1 = y + halfsq;
  int xw0 = x - halfsq, yw0 = y - sp2, xw1 = x - sp2, yw1 = y + sp2;
  int xnw0 = x - halfsq, ynw0 = y - halfsq, xnw1 = x - sp2, ynw1 = y - sp2;

  sq = -1;
  dir = DIR_UNDEF;

  for(int i = 0; i < 7; i++) {
    if(posX > xn0 && posX < xn1 && posY > yn0 && posY < yn1) {
      sq = i;
      dir = invert ? DIR_S : DIR_N;
      return TRUE;
    }
    if(posX > xne0 && posX < xne1 && posY > yne0 && posY < yne1) {
      sq = i;
      dir = invert ? DIR_SE : DIR_NE;
      return TRUE;
    }
    if(posX > xe0 && posX < xe1 && posY > ye0 && posY < ye1) {
      sq = i;
      dir = DIR_E;
      return TRUE;
    }
    if(posX > xse0 && posX < xse1 && posY > yse0 && posY < yse1) {
      sq = i;
      dir = invert ? DIR_NE : DIR_SE;
      return TRUE;
    }
    if(posX > xs0 && posX < xs1 && posY > ys0 && posY < ys1) {
      sq = i;
      dir = invert ? DIR_N : DIR_S;
      return TRUE;
    }
    if(posX > xsw0 && posX < xsw1 && posY > ysw0 && posY < ysw1) {
      sq = i;
      dir = invert ? DIR_NW : DIR_SW;
      return TRUE;
    }
    if(posX > xw0 && posX < xw1 && posY > yw0 && posY < yw1) {
      sq = i;
      dir = DIR_W;
      return TRUE;
    }
    if(posX > xnw0 && posX < xnw1 && posY > ynw0 && posY < ynw1) {
      sq = i;
      dir = invert ? DIR_SW : DIR_NW;
      return TRUE;
    }
    posY -= sqsize;
  }

  return FALSE;
}

OctiSquareDispWidget::OctiSquareDispWidget(QWidget *parent, const char *name, WFlags f) : QWidget(parent, name, f) {
  _board = NULL;
  _numPods = 0;
  _squareSize = 0;
  _x = -1;
  _y = -1;
  _highlightSqr = -1;
  _highlightDir = DIR_UNDEF;
  _supressSqr = -1;
  _supressDir = DIR_UNDEF;
  _supressOn = FALSE;

  setAcceptDrops(TRUE);
}

OctiSquareDispWidget::~OctiSquareDispWidget() {
  if(_board) {
    _board->removeOctiBoardListener(this);
  }
}

void OctiSquareDispWidget::setDisplaySquare(int x, int y, boolean forceload) {
  int i;

  if(_x != x || _y != y || forceload) {
    _x = x;
    _y = y;

    if(_board != NULL && x > -1 && _x < 9 && _y > -1 && _y < 9) {
      OctiSquare *square = NULL;
      square = &_board->squareAt(_x, _y);
      _numPods = square->getNumPods();
      for(i = 0; i < 7; i++) {
	if(i < _numPods) {
	  _pods[i] = *square->getPodAt(i);
	  _selected[i] = TRUE;
	} else {
	  _selected[i] = FALSE;
	}
      }
    }

    repaint(FALSE);
  }
}

int OctiSquareDispWidget::getSquareLoc(int x, int y) {
  int xOrig = (width() - _squareSize) >> 1;
  int yOrig = (height() >> 1) - (_squareSize << 2);

  if(x >= xOrig && x <= xOrig + _squareSize &&
     y >= yOrig && y <= yOrig + _squareSize * 7) {
    return (y - yOrig) / _squareSize ;
  }

  return -1;
}

int OctiSquareDispWidget::getSelectedPods(OctiPod pods[]) const {
  int i, count = 0;

  for(i = 0; i < _numPods; i++) {
    if(_selected[i]) {
      pods[i] = _pods[i];
      count++;
    }
  }

  return count;
}

OctiPlayer OctiSquareDispWidget::getSelectedPodsOwner() const {
  OctiPlayer owner = PLAYER_UNDEF;

  for(int i = 0; i < _numPods; i++) {
    if(_selected[i]) {
      if(owner != (OctiPlayer) _pods[i].getType()) {
	if(owner != PLAYER_UNDEF) {
	  return PLAYER_UNDEF;
	} else {
	  owner = (OctiPlayer) _pods[i].getType();
	}
      }
    }
  }

  return owner;
}

void OctiSquareDispWidget::paintEvent(QPaintEvent *pe) {
  OctiWidgetColors &colors = G_OctiOptions->getWidgetColors();
  int sl = (int) (_squareSize * 0.100);
  int i, x, y, xc, yc, yOrig;
  QPixmap pm(size());
  pm.fill(backgroundColor());
  QPainter p;
  p.begin(&pm, this);
  p.setFont(QFont("times", (int) (_squareSize * 0.35), QFont::Bold));

  xc = (width() >> 1);
  x = xc - (_squareSize >> 1);
  yOrig = (height() >> 1) - (_squareSize << 2);

  // Draw boxes
  p.setPen(QPen(colors.getLineColor()));
  p.drawRect(x, yOrig, _squareSize, _squareSize * 7);
  for(i = 0; i < 7; i++) {
    p.drawLine(x, yOrig + _squareSize * i,
	       x + _squareSize, yOrig + _squareSize * i);
  }
  
  for(i = 0; i < 7; i++) {
    y = yOrig + _squareSize * i;
    yc = y + (_squareSize >> 1);
    
    if((_y == 2 && (_x == 2 || _x == 4 || _x == 6)) ||
       (_y == 6 && (_x == 2 || _x == 4 || _x == 6))) {
      // Draw home squares
      p.setPen(QPen(colors.getLineColor()));
      p.setBrush(QBrush(colors.getLineColor()));
      p.drawRect(x, y, _squareSize, _squareSize);
      p.setPen(QPen(colors.getSquareColor()));
      p.drawText(x, y, _squareSize, _squareSize, AlignCenter,
		 QString("OCTI"));
      p.setPen(QPen(gray));
      p.drawLine(x, y, x + _squareSize, y);
    } else {
      // Draw dash lines
      p.setPen(QPen(colors.getLineColor()));
      p.drawLine(xc, y, xc, y + sl);
      p.drawLine(xc, y + _squareSize, xc, y + _squareSize - sl);
      p.drawLine(x, yc, x + sl, yc);
      p.drawLine(x + _squareSize, yc, x + _squareSize - sl, yc);
      p.drawLine(x + sl, y + sl, x + sl + sl, y + sl  + sl);
      p.drawLine(x + sl, y + _squareSize - sl,
		 x + sl + sl, y + _squareSize - sl - sl);
      p.drawLine(x + _squareSize - sl, y + sl,
		 x + _squareSize - sl - sl, y + sl + sl);
      p.drawLine(x + _squareSize - sl, y + _squareSize - sl,
		 x + _squareSize - sl - sl, y + _squareSize - sl - sl);
    }

    // Draw highlights
    if(i == _highlightSqr ) {
      paint_pronghighlight_at(p, _squareSize, _showInverted, _highlightDir,
			      xc, yc);
    }
    if(_selected[i]) {
      p.setPen(QPen(yellow, 2));
      p.setBrush(NoBrush);
      p.drawRect(x + 1, y + 1, _squareSize - 1, _squareSize - 1);
    }

    // Draw pods
    if(i < _numPods) {
      if(_supressOn && i == _supressSqr) {
	OctiPod supressedPod = _pods[i];
	supressedPod.removeProng(_supressDir);
	paint_pod_at(p, _squareSize, _showInverted, supressedPod, xc, yc);
      } else {
	paint_pod_at(p, _squareSize, _showInverted, _pods[i], xc, yc);
      }
    }
  }

  // Draw coordinate
  p.setPen(QPen(colors.getLineColor()));
  y = yOrig + _squareSize * i;
  yc = y + (_squareSize >> 1);
  QString str;
  str += "[";
  str += ('1' + _x);
  str += ", ";
  str += ('1' + _y);
  str += "]";
  p.drawText(x, y, _squareSize, _squareSize, AlignCenter, str);

  p.end();
  bitBlt(this, 0, 0, &pm);
}

void OctiSquareDispWidget::mousePressEvent(QMouseEvent *e) {
  _pressPos = e->pos();
}

void OctiSquareDispWidget::mouseReleaseEvent(QMouseEvent *e) {
  int loc = getSquareLoc(e->pos().x(), e->pos().y());
  if(loc > - 1 && loc < _numPods) {
    _selected[loc] = !_selected[loc];
    repaint(FALSE);
  }
}

void OctiSquareDispWidget::mouseMoveEvent(QMouseEvent *e) {
  if(e->state() & LeftButton) {
    if(QABS(e->pos().x() - _pressPos.x()) >= 3 ||
       QABS(e->pos().y() - _pressPos.y()) >= 3) {
      int xc = (width() >> 1);
      int yOrig = (height() >> 1) - (_squareSize << 2);
      int yc = yOrig + (_squareSize >> 1);
      int sqr;
      OctiDir dir;

      if(prong_pos(_squareSize, _showInverted, sqr, dir,
		   xc, yc, _pressPos.x(), _pressPos.y())) {
	if(_board->getGame()->getPlayerToMove() == (OctiPlayer) _pods[sqr].getType() &&
	   sqr < _numPods && _pods[sqr].prongAt(dir) != NO_PRONG) {
	  _supressSqr = sqr;
	  _supressDir = dir;
	  _supressOn = TRUE;

	  QRect rect(0, 0, (int) (_squareSize * 0.166), (int) (_squareSize * 0.583));
	  QPixmap pixmap(rect.size());
	  pixmap.fill();
	  QPainter p(&pixmap);
	  p.drawRect(rect);
	  
	  QTextDrag *drag = new QTextDrag("REPOSITION", this);
	  drag->setPixmap(pixmap, rect.center());
	  drag->dragMove();
	}
      }
    }
  }
}

void OctiSquareDispWidget::dragEnterEvent(QDragEnterEvent *e) {

}

void OctiSquareDispWidget::dragMoveEvent(QDragMoveEvent *e) {
  if(QTextDrag::canDecode(e)) {
    e->accept();

    QString s;
    QTextDrag::decode(e, s);
    
    if(s == "REPOSITION") {
      _supressOn = TRUE;
    }

    if(s == "REPOSITION" || s == "PRONG") {
      int xc = (width() >> 1);
      int yOrig = (height() >> 1) - (_squareSize << 2);
      int yc = yOrig + (_squareSize >> 1);
      int sqr;
      OctiDir dir;
      
      if(prong_pos(_squareSize, _showInverted, sqr, dir,
		   xc, yc, e->pos().x(), e->pos().y())) {
	if(_board->getGame()->getPlayerToMove() == (OctiPlayer) _pods[sqr].getType() &&
	   sqr < _numPods && _pods[sqr].prongAt(dir) == NO_PRONG &&
	   (sqr != _highlightSqr || dir != _highlightDir)) {
	  _highlightSqr = sqr;
	  _highlightDir = dir;
	  repaint(FALSE);
	}
      }
    }
  }
}

void OctiSquareDispWidget::dragLeaveEvent(QDragLeaveEvent *e) {
  if(_highlightSqr >= 0) {
    _highlightSqr = -1;
    _highlightDir = DIR_UNDEF;
    _supressOn = FALSE;
    repaint(FALSE);
  }
}

void OctiSquareDispWidget::dropEvent(QDropEvent *e) {
  if(QTextDrag::canDecode(e)) {
    e->accept();

    QString s;
    QTextDrag::decode(e, s);
    
    if(s == "REPOSITION" || s == "PRONG") {
      int xc = (width() >> 1);
      int yOrig = (height() >> 1) - (_squareSize << 2);
      int yc = yOrig + (_squareSize >> 1);
      int sqr;
      OctiDir dir;
      
      if(prong_pos(_squareSize, _showInverted, sqr, dir,
		   xc, yc, e->pos().x(), e->pos().y())) {
	if(_board->getGame()->getPlayerToMove() == (OctiPlayer) _pods[sqr].getType() &&
	   sqr < _numPods && _pods[sqr].prongAt(dir) == NO_PRONG) {
	  try {
	    _board->getGame()->prepareAddProng(_x, _y, _pods[sqr], dir);
	  } catch(IllegalMoveException e) {
	    cerr << e.getMessage() << endl;
	  }
	}
      }
    }
  }

  if(_highlightSqr >= 0) {
    _highlightSqr = -1;
    _highlightDir = DIR_UNDEF;
    _supressOn = FALSE;
    repaint(FALSE);
  }
}

void OctiSquareDispWidget::gameChanged(OctiBoard *src, OctiGame *oldGame, OctiGame *newGame) {

}

void OctiSquareDispWidget::boardUpdated(OctiBoard *src, const OctiMove *move,
					boolean undo) {
  setDisplaySquare(_x, _y, TRUE);
}

__END_NAMESPACE(SELFSOFT);

