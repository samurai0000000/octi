/*************************************************************************
 * $Id: OctiTrainerWindow.cxx,v 1.2 2001/08/16 02:21:50 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/16 02:21:50 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created: 12/20/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _OCTIAPI_IMPL_

#include <qlayout.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qpopupmenu.h>
#include <qmenubar.h>
#include <qkeycode.h>
#include <qmovie.h>
#include <qmultilineedit.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qstatusbar.h>
#include <qmessagebox.h>
#include <qapplication.h>
#include <qpushbutton.h>
#include <qvbuttongroup.h>
#include <qaccel.h>
#include <qtextstream.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qwhatsthis.h>
#include <qobjectlist.h>
#include <qvbox.h>
#include "OctiGame.hxx"
#include "Octi2PlayerGameSearchState.hxx"
#include "OctiBoardDispWidget.hxx"
#include "OctiTrainerWindow.hxx"
#include "xpm/tb/tb_exit.xpm"
#include "octiuires/icon_octi.xpm"

__BEGIN_NAMESPACE(SELFSOFT);

OctiTrainerWindow::OctiTrainerWindow(QWidget * parent, const char *name, WFlags f) {
  QPixmap exitIcon;

  exitIcon = QPixmap(tb_exit_xpm);

  /////////////////////////////////////////////////////////////////////////
  // Initialize toolbar
  _toolbar = new QToolBar(this, "Tool Bar");
  addToolBar(_toolbar, "Tool Bar", Top, TRUE);
  
  QToolButton *tbRand
    = new QToolButton(exitIcon, "Randomize", QString::null,
		      this, SLOT(rand()), _toolbar, "generate a random board");
  QToolButton *tbCont
    = new QToolButton(exitIcon, "Continue", QString::null,
		      this, SLOT(cont()), _toolbar, "continue training");
  QToolButton *tbStat
    = new QToolButton(exitIcon, "Statistics", QString::null,
		      this, SLOT(stat()), _toolbar, "generate statistics");
  _toolbar->addSeparator();
  QToolButton *tbAnalyzeCF
    = new QToolButton(exitIcon, "Analyze Curve-Fitting", QString::null,
		      this, SLOT(analyzeCF()), _toolbar, "analyze curve-fitting");
  QToolButton *tbAnalyzeNN
    = new QToolButton(exitIcon, "Analyze Neural-Net", QString::null,
		      this, SLOT(analyzeNN()), _toolbar, "analyze neural-net");
  QToolButton *appExit
    = new QToolButton(exitIcon, "Exit Application", QString::null,
		      qApp, SLOT(closeAllWindows()),
		      _toolbar, "exit application");
  (void)QWhatsThis::whatsThisButton(_toolbar);

  ////////////////////////////////////////////////////////////////////////
  // Initialize the menu bar
  QPopupMenu *trainer = new QPopupMenu(this);
  menuBar()->insertItem("&Trainer", trainer);
  trainer->insertItem("&Randomize", this, SLOT(rand()), CTRL+Key_R);
  trainer->insertItem("&Continue", this, SLOT(cont()), CTRL+Key_C);
  trainer->insertItem("&Statistics", this, SLOT(stat()), CTRL+Key_S);
  trainer->insertItem("&Quit", qApp, SLOT(closeAllWindows()), CTRL+Key_Q);
  
  QPopupMenu *analyze = new QPopupMenu(this);
  menuBar()->insertItem("&Analyze", analyze);
  analyze->insertItem("&Curve-Fitting", this, SLOT(analyzeCF()));
  analyze->insertItem("&Neural-Net", this, SLOT(analyzeNN()));

  menuBar()->insertSeparator();
  QPopupMenu *help = new QPopupMenu(this);
  menuBar()->insertItem("&Help", help);
  
  help->insertItem("&About", this, SLOT(about()), Key_F1);
  help->insertSeparator();
  help->insertItem("What's &This", this, SLOT(whatsThis()), SHIFT+Key_F1);

  /////////////////////////////////////////////////////////////////////////
  // Layout the rest
  QWidget *widget = new QWidget(this);
  setCentralWidget(widget);

  QHBoxLayout *hlayout1 = new QHBoxLayout(widget);
  QVBoxLayout *vlayout1 = new QVBoxLayout();

  _bw = new OctiBoardWidget(widget);
  QLabel *label1 = new QLabel("Chose Move", widget, "move");
  label1->setAlignment(AlignBottom | AlignHCenter);
  _listbox = new QListBox(widget);
  QVButtonGroup *buttongroup1 = new QVButtonGroup(widget);
  QPushButton *randButton = new QPushButton("Randomize", buttongroup1, "randomize");
  QPushButton *contButton = new QPushButton("Continue", buttongroup1, "continue");
  label1->setFont(QFont("Helvetica", 14, QFont::Bold));
  randButton->setFont(QFont("Helvetica", 10));
  contButton->setFont(QFont("Helvetica", 10));

  connect(randButton, SIGNAL(clicked()), this, SLOT(rand()));
  connect(contButton, SIGNAL(clicked()), this, SLOT(cont()));
  connect(_listbox, SIGNAL(highlighted(int)), this, SLOT(moveSelected(int)));

  hlayout1->addWidget(_bw, 9);
  hlayout1->addLayout(vlayout1, 2);
  vlayout1->addWidget(label1, 1);
  vlayout1->addWidget(_listbox, 5);
  vlayout1->addWidget(buttongroup1);

  statusBar()->message("Ready", 2000);
  setMinimumWidth(900);
  setMinimumHeight(600);

  _moved = FALSE;
}

OctiTrainerWindow::~OctiTrainerWindow() {
  delete _bw;
  delete _listbox;
}

void OctiTrainerWindow::rand() {
  statusBar()->message("New random board", 2000);

  _bw->getBoard()->getGame()->randomize();

  Octi2PlayerGameSearchState search(_bw->getBoard()), *current;;
  search.expand();
  _listbox->clear();
  _listbox->insertItem("----------");
  for(current = (Octi2PlayerGameSearchState *) search.getHeadSuccessors();
      current != NULL; current = (Octi2PlayerGameSearchState *) current->getNext()) {
    _listbox->insertItem(current->getDescription());
  }
  _moved = FALSE;
  _listbox->setSelected(0, TRUE);
  moveSelected(0);
}

void OctiTrainerWindow::cont() {
  if(_listbox->currentItem() == 0) {
    statusBar()->message("Please choose move first", 2000);
  } else {
    Octi2PlayerGameSearchState search(_bw->getBoard()), *current;;
    search.expand();
    _listbox->clear();
    _listbox->insertItem("----------");
    for(current = (Octi2PlayerGameSearchState *) search.getHeadSuccessors();
	current != NULL; current = (Octi2PlayerGameSearchState *) current->getNext()) {
      _listbox->insertItem(current->getDescription());
    }
    _moved = FALSE;
    _listbox->setSelected(0, TRUE);
    moveSelected(0);
  }
}

void OctiTrainerWindow::stat() {

}

void OctiTrainerWindow::analyzeCF() {

}

void OctiTrainerWindow::analyzeNN() {

}

void OctiTrainerWindow::about() {

}

void OctiTrainerWindow::moveSelected(int i) {
  if(_lastitem != i) {
    if(_moved == TRUE) {
      _bw->getBoard()->getGame()->undo();
      _moved = FALSE;
    }
    
    if(i != 0) {
      try {
	OctiMove m(_listbox->currentText());
	_bw->getBoard()->getGame()->move(m);
      } catch(IllegalMoveException e) {
	statusBar()->message(e.getMessage(), 2000);
      }
      _moved = TRUE;
    }
    _lastitem = i;
  }
}

__END_NAMESPACE(SELFSOFT);
