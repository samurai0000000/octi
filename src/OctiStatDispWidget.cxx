/*************************************************************************
 * $Id: OctiStatDispWidget.cxx,v 1.4 2001/08/09 04:30:56 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 04:30:56 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created: 12/10/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _OCTIAPI_IMPL_

#include <qpainter.h>
#include <qdragobject.h>
#include <qdropsite.h>
#include "OctiWidgetColors.hxx"
#include "OctiWidgetOptions.hxx"
#include "OctiStatDispWidget.hxx"
#include "OctiGame.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

OctiStatDispWidget::OctiStatDispWidget(QWidget *parent, const char *name, WFlags f) : QWidget(parent, name, f) {
  _board = NULL;
  _player = PLAYER_UNDEF;
  _squareSize = 0;
  _nfprong = 0;
  _nfpod = 0;
  _ncpod = 0;
  _showInverted = FALSE;

  setAcceptDrops(TRUE);
}

OctiStatDispWidget::~OctiStatDispWidget() {
  if(_board) {
    _board->removeOctiBoardListener(this);
  }
}

void OctiStatDispWidget::calcRects() {
  //int sl = (int) (_squareSize * 0.100);
  int sp1 = (int) (_squareSize * 0.083);
  //int sp2 = (int) (_squareSize * 0.125);
  int sp3 = (int) (_squareSize * 0.250);
  int sp4 = (int) (_squareSize * 0.500);
  int xc1, yc1, xc2, yc2, xc3, yc3;

  yc1 = yc2 = yc3 = (int) (height() * 2 / 3);
  xc1 = (int) (width() * 1 / 6);
  xc2 = (int) (width() * 3 / 6);
  xc3 = (int) (width() * 5 / 6);

  _prongRect.setCoords(xc1 - sp3, yc1 - sp4,
		       xc1 + sp3, yc1 + sp1);
  _freePodRect.setCoords(xc2 - sp4, yc2 - sp4,
			 xc2 + sp3, yc2 + sp3);
  _jailPodRect.setCoords(xc3 - sp4, yc3 - sp4,
			 xc3 + sp3, yc3 + sp3);
  _highlightJail = FALSE;
}

void OctiStatDispWidget::paintEvent(QPaintEvent *e) {
  if(_player == PLAYER_UNDEF) {
    return;
  }

  OctiWidgetColors &colors = G_OctiOptions->getWidgetColors();
  int i;
  //int sl = (int) (_squareSize * 0.100);
  int sp1 = (int) (_squareSize * 0.083);
  int sp2 = (int) (_squareSize * 0.125);
  int sp3 = (int) (_squareSize * 0.250);
  int sp4 = (int) (_squareSize * 0.500);
  int xc1, yc1, xc2, yc2, xc3, yc3;
  int overlapoffet = (int) (_squareSize * 0.05);

  OctiPod pod((OctiPodType) _player);
  QPainter p(this);
  QString num;
  p.setFont(QFont("times", (int) (_squareSize * 0.25)));

  yc1 = yc2 = yc3 = (int) (height() * 0.667);
  xc1 = (int) (width() * 0.167);
  xc2 = (int) (width() * 0.500);
  xc3 = (int) (width() * 0.833);

  // Draw free prongs
  p.setPen(QPen(colors.getLineColor()));
  p.setBrush(QBrush(white));
  QPointArray rectangle;
  rectangle.setPoints(4,
		      xc1 - sp1, yc1 - sp4,
		      xc1 + sp1, yc1 - sp4,
		      xc1 + sp1, yc1 + sp1,
		      xc1 - sp1, yc1 + sp1);
  p.drawPolygon(rectangle);
  p.setPen(QPen(colors.getLineColor()));
  num.setNum(_nfprong);
  p.drawText(xc1 + sp2, yc1 - sp4, num);

  // Draw free pods
  for(i = 0; i < _nfpod; i++) {
    paint_pod_at(p, _squareSize, _showInverted, pod, xc2, yc2);
    xc2 -= overlapoffet;
    yc2 -= overlapoffet;
  }

  xc2 = (int) (width() * 0.500);
  yc2 = (int) (height() * 0.667);
  num.setNum(_nfpod);
  p.setPen(QPen(colors.getLineColor()));
  p.drawText(xc2 + sp1, yc2 - sp4, num);

  // Draw captured pods
  for(i = 0; i < _ncpod; i++) {
    paint_pod_at(p, _squareSize, _showInverted, pod, xc3, yc3);
    xc3 -= overlapoffet;
    yc3 -= overlapoffet;
  }

  xc3 = (int) (width() * 0.833);
  yc3 = (int) (height() * 0.667);
  if(_board->canLiberate(_player)) {
    p.setPen(QPen(colors.getLineColor()));
    p.setBrush(NoBrush);
    p.drawRect(xc3 - (int) (_squareSize * 0.45), yc3 - (int) (_squareSize * 0.45), _squareSize, _squareSize);
  } else {
    p.setPen(QPen(colors.getLineColor(), 2));
    p.drawLine(xc3 - sp4, yc3 - sp3, xc3 + sp4, yc3 - sp3);
    p.drawLine(xc3 - sp4, yc3 + sp3, xc3 + sp4, yc3 + sp3);
    p.drawLine(xc3 - sp3, yc3 - sp4, xc3 - sp3, yc3 + sp4);
    p.drawLine(xc3, yc3 - sp4, xc3, yc3 + sp4);
    p.drawLine(xc3 + sp3, yc3 - sp4, xc3 + sp3, yc3 + sp4);
    p.drawLine(xc3 + sp3, yc3 - sp4, xc3 + sp3, yc3 + sp4);
  }

  if(_highlightJail) {
    p.setBrush(NoBrush);
    p.setPen(QPen(red, 2));
    p.drawRect(xc3 - (int) (_squareSize * 0.45), yc3 - (int) (_squareSize * 0.45), _squareSize, _squareSize);
  }

  num.setNum(_ncpod);
  p.setPen(QPen(colors.getLineColor()));
  p.drawText(xc3 + sp1, yc3 - sp4, num);
}

void OctiStatDispWidget::mousePressEvent(QMouseEvent *e) {
   _pressPos = e->pos();
}

void OctiStatDispWidget::resizeEvent(QResizeEvent *e) {
  calcRects();
  repaint();
}

void OctiStatDispWidget::mouseReleaseEvent(QMouseEvent *e) {

}

void OctiStatDispWidget::mouseMoveEvent(QMouseEvent *e) {
  if((e->state() & LeftButton) &&
     _player == _board->getGame()->getPlayerToMove()) {
    if(QABS(e->pos().x() - _pressPos.x()) >= 3 ||
       QABS(e->pos().y() - _pressPos.y()) >= 3) {
      boolean inProngRect = _prongRect.contains(_pressPos);
      boolean inFreePodRect = _freePodRect.contains(_pressPos);
      boolean inJailPodRect = _jailPodRect.contains(_pressPos);

      if(inProngRect && _nfprong > 0) {
	QRect rect(0, 0, (int) (_squareSize * 0.166), (int) (_squareSize * 0.583));
	QPixmap pixmap(rect.size());
	pixmap.fill();
	QPainter p(&pixmap);
	p.drawRect(rect);

	QTextDrag *drag = new QTextDrag("PRONG", this);
	drag->setPixmap(pixmap, rect.center());
	drag->dragMove();
      } else if((inFreePodRect && _nfpod > 0) || (inJailPodRect && _ncpod > 0)) {
	QBitmap mask(_squareSize, _squareSize);
	QPixmap pixmap(_squareSize, _squareSize);
	pixmap.fill();
	QPainter p(&pixmap);
	OctiPod pod((OctiPodType) _player);
	paint_pod_at(p, _squareSize, _showInverted,
		     pod, _squareSize >> 1, _squareSize >> 1, &mask);
	pixmap.setMask(mask);

	QTextDrag *drag = new QTextDrag("POD", this);
	drag->setPixmap(pixmap, QPoint((int) (_squareSize * 0.5), (int) (_squareSize * 0.5)));
	drag->dragMove();
      }
    }
  }
}

void OctiStatDispWidget::dragEnterEvent(QDragEnterEvent *e) {

}

void OctiStatDispWidget::dragMoveEvent(QDragMoveEvent *e) {
  if(QTextDrag::canDecode(e)) {
    e->accept();

    QString s;
    QTextDrag::decode(e, s);

    if(s == "CAPTURE") {
      int x = e->pos().x();
      int y = e->pos().y();
      int x0 = (int) (width() * 0.833 - _squareSize * 0.45);
      int x1 = x0 + _squareSize;
      int y0 = (int) (height() * 2 / 3 - _squareSize * 0.45);
      int y1 = y0 + _squareSize;

      if(x > x0 && x < x1 && y > y0 && y < y1) {
	if(_highlightJail == FALSE) {
	  _highlightJail = TRUE;
	  repaint(FALSE);
	}
      } else {
	if(_highlightJail == TRUE) {
	  _highlightJail = FALSE;
	  repaint(TRUE);
	}
      }
    }
  }
}

void OctiStatDispWidget::dragLeaveEvent(QDragLeaveEvent *e) {
  if(_highlightJail == TRUE) {
    _highlightJail = FALSE;
    repaint(TRUE);
  }
}

void OctiStatDispWidget::dropEvent(QDropEvent *e) {
  if(_highlightJail == TRUE) {
    _highlightJail = FALSE;
    repaint(TRUE);
  }
}

void OctiStatDispWidget::gameChanged(OctiBoard *src, OctiGame *oldGame, OctiGame *newGame) {

}

void OctiStatDispWidget::boardUpdated(OctiBoard *src, const OctiMove *move,
				      boolean undo) {
  if(_board) {
    _nfprong = _board->getNumFreeProngs(_player);
    _nfpod = _board->getNumFreePods(_player);
    _ncpod = _board->getNumCapturedPods(_player);
    repaint();
  }
}

__END_NAMESPACE(SELFSOFT);
