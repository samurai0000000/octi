/*************************************************************************
 * $Id: OctiMainWindow.cxx,v 1.5 2001/08/16 02:21:50 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/16 02:21:50 $
 * $Locker:  $
 * $Revision: 1.5 $
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

#include <qimage.h>
#include <qpixmap.h>
#include <qkeycode.h>
#include <qmovie.h>
#include <qmultilineedit.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qstatusbar.h>
#include <qmessagebox.h>
#include <qprinter.h>
#include <qapplication.h>
#include <qpushbutton.h>
#include <qaccel.h>
#include <qtextstream.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qwhatsthis.h>
#include <qobjectlist.h>
#include <qvbox.h>
#include "LoginDialog.hxx"
#include "OctiMainWindow.hxx"
#include "OctiGameSelectDialog.hxx"
#include "OctiPreferencesDialog.hxx"
#include "xpm/tb/tb_new.xpm"
#include "xpm/tb/tb_open.xpm"
#include "xpm/tb/tb_save.xpm"
#include "xpm/tb/tb_print.xpm"
#include "xpm/tb/tb_winclose.xpm"
#include "xpm/tb/tb_undo.xpm"
#include "xpm/tb/tb_redo.xpm"
#include "xpm/tb/tb_ok.xpm"
#include "xpm/tb/tb_cancel.xpm"
#include "xpm/tb/tb_exit.xpm"
#include "xpm/tb/tb_prefs.xpm"
#include "octiuires/icon_octi.xpm"

__BEGIN_NAMESPACE(SELFSOFT);

const char *G_OMW_GAME_NEW_TEXT =
    "Click on this button to start a new game";

const char *G_OMW_GAME_LOAD_TEXT =
    "Click on this button to load a saved octi game.\n\n"
    "You can also select the <em>Load</em> command from the Game menu";

const char *G_OMW_GAME_SAVE_TEXT =
    "Click on this button to save the octi game into a file.\n\n"
    "You can also select the <em>Save</em> command from the Game menu";

OctiMainWindow::OctiMainWindow(QWidget * parent, const char *name, WFlags f) : _windowCount(0) {

    /////////////////////////////////////////////////////////////////////////
    // Initialize toolbar
    _toolbar = new QToolBar(this, "Tool Bar");
    addToolBar(_toolbar, "Tool Bar", Top, TRUE);

    _gameNew = new QToolButton(QPixmap(tb_new_xpm), "New Game", QString::null,
                               this, SLOT(newGame()), _toolbar, "new game");
    _gameLoad = new QToolButton(QPixmap(tb_open_xpm) , "Load Game", QString::null,
                                this, SLOT(load()), _toolbar, "load game");
    _gameSave = new QToolButton(QPixmap(tb_save_xpm), "Save Game", QString::null,
                                this, SLOT(save()), _toolbar, "save game");
    _winClose = new QToolButton(QPixmap(tb_winclose_xpm), "Close Game", QString::null,
                                this, SLOT(close()), _toolbar, "close game");
#ifndef QT_NO_PRINTER
    _printer = new QPrinter;
    _gamePrint = new QToolButton(QPixmap(tb_print_xpm), "Print Game", QString::null,
                                 this, SLOT(print()), _toolbar, "print game");
#endif
    _toolbar->addSeparator();
    _gameUndo = new QToolButton(QPixmap(tb_undo_xpm), "Undo Move", QString::null,
                                this, SLOT(undo()), _toolbar, "undo move");
    _gameRedo = new QToolButton(QPixmap(tb_redo_xpm), "Redo Move", QString::null,
                                this, SLOT(redo()), _toolbar, "redo move");
    _toolbar->addSeparator();
    _gameMove = new QToolButton(QPixmap(tb_ok_xpm), "Make Move", QString::null,
                                this, SLOT(move()), _toolbar, "make move");
    _gameCancel = new QToolButton(QPixmap(tb_cancel_xpm), "Cancel Move", QString::null,
                                  this, SLOT(cancel()), _toolbar, "cancel move");
    _gamePause = new QToolButton(QPixmap(), "Pause Game", QString::null,
                                 this, SLOT(pause()), _toolbar, "pause game");
    _gameResign = new QToolButton(QPixmap(),"Resign Game", QString::null,
                                  this, SLOT(resign()), _toolbar, "resign");
    _toolbar->addSeparator();
    _appExit = new QToolButton(QPixmap(tb_exit_xpm), "Exit Application", QString::null,
                               qApp, SLOT(closeAllWindows()),
                               _toolbar, "exit application");

#ifndef QT_NO_PRINTER
    _gamePrint->setEnabled(FALSE);
#endif
    _gameUndo->setEnabled(FALSE);
    _gameRedo->setEnabled(FALSE);
    _gameMove->setEnabled(FALSE);
    _gameCancel->setEnabled(FALSE);
    _gamePause->setEnabled(FALSE);
    _gameResign->setEnabled(FALSE);

    (void)QWhatsThis::whatsThisButton(_toolbar);

    ////////////////////////////////////////////////////////////////////////
    // Help messages

    QWhatsThis::add(_gameNew, G_OMW_GAME_NEW_TEXT);
    QWhatsThis::add(_gameLoad, G_OMW_GAME_LOAD_TEXT);
    QWhatsThis::add(_gameSave, G_OMW_GAME_SAVE_TEXT);
#ifndef QT_NO_PRINTER
    //QWhatsThis::add(filePrint, G_OMW_FILE_SAVE_TEXT);
#endif

    ////////////////////////////////////////////////////////////////////////
    // Initialize the menu bar
    QPopupMenu *game = new QPopupMenu(this);
    menuBar()->insertItem("&Game", game);
    game->insertItem(QPixmap(tb_new_xpm), "&New", this, SLOT(newGame()), CTRL+Key_N);
    game->insertItem(QPixmap(tb_open_xpm), "&Load", this, SLOT(load()), CTRL+Key_O);
    game->insertItem(QPixmap(tb_save_xpm), "&Save", this, SLOT(save()), CTRL+Key_S);
    game->insertItem(QPixmap(tb_save_xpm), "Save &as...", this, SLOT(saveAs()));
#ifndef QT_NO_PRINTER
    game->insertSeparator();
    game->insertItem(QPixmap(tb_print_xpm), "&Print", this, SLOT(print()), CTRL+Key_P);
#endif
    game->insertSeparator();
    game->insertItem(QPixmap(tb_undo_xpm), "&Undo", this, SLOT(undo()), CTRL+Key_Z);
    game->insertItem(QPixmap(tb_redo_xpm), "&Redo", this, SLOT(undo()), CTRL+Key_R);
    game->insertSeparator();
    game->insertItem(QPixmap(tb_ok_xpm), "&Make Move", this, SLOT(move()));
    game->insertItem(QPixmap(tb_cancel_xpm), "&Cancel Move", this, SLOT(cancel()));
    game->insertItem("&Paue Game", this, SLOT(pause()));
    game->insertItem("Resign", this, SLOT(resign()));
    game->insertSeparator();
    game->insertItem(QPixmap(tb_winclose_xpm), "&Close", this, SLOT(close()), CTRL+Key_W);

    game->insertItem(QPixmap(tb_exit_xpm), "&Quit", qApp, SLOT(closeAllWindows()), CTRL+Key_Q);

    QPopupMenu *settings = new QPopupMenu(this);
    menuBar()->insertItem("&Settings", settings);
    settings->insertItem("&Client Connect", this, SLOT(client()), CTRL+Key_C);
    settings->insertItem("Start Server", this, SLOT(server()));
    settings->insertSeparator();
    settings->insertItem("&Preferences", this, SLOT(pref()));

    _windowsMenu = new QPopupMenu(this);
    _windowsMenu->setCheckable(TRUE);
    connect(_windowsMenu, SIGNAL(aboutToShow()), this, SLOT(windowsMenuAboutToShow()));
    menuBar()->insertItem("&Windows", _windowsMenu);

    menuBar()->insertSeparator();
    QPopupMenu *help = new QPopupMenu(this);
    menuBar()->insertItem("&Help", help);

    help->insertItem("&About", this, SLOT(about()), Key_F1);
    help->insertSeparator();
    help->insertItem("What's &This", this, SLOT(whatsThis()), SHIFT+Key_F1);

    /////////////////////////////////////////////////////////////////////////
    // Layout the rest
    QVBox *vb = new QVBox(this);
    vb->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    _ws = new QWorkspace(vb);
    connect(_ws, SIGNAL(windowActivated(QWidget *)), this, SLOT(windowActivated(QWidget *)));
    setCentralWidget(vb);

    statusBar()->message("Ready", 2000);
    setMinimumWidth(700);
    setMinimumHeight(600);
}

OctiMainWindow::~OctiMainWindow() {
    _windowCount = -1;  // Prevent the tool button setEnabled() to be called...
}

void OctiMainWindow::newGame() {
    OctiGameSelectDialog d;
    d.show();

    if(d.result() == QDialog::Rejected) {
        return;
    }

    OctiBoardFrame *f = new OctiBoardFrame(_ws, 0, WDestructiveClose);
    /*
      connect(f, SIGNAL( message(const QString &, int)), statusBar(),
	  SLOT( message(const QString &, int)));
    */
    connect(f, SIGNAL(destroyed()), this, SLOT(windowDestroyed()));
    f->setCaption("Untitled Game");
    f->setIcon(QPixmap(icon_octi_xpm));

    // Start game
    OctiBoard *board = f->getBoard();
    board->setGameMode(d.getGameMode(), d.useSuperProngs(), d.edgeless());
    board->getGame()->addOctiGameListener(this);
    board->getGame()->start();

    if(_ws->windowList().isEmpty()) {
        f->showMaximized();
    } else {
        f->show();
    }

#ifndef QT_NO_PRINTER
    _gamePrint->setEnabled(TRUE);
    _windowCount++;
#endif
}

void OctiMainWindow::load() {
    QFileDialog d(this, "Load Game", TRUE);
    d.show();
}

void OctiMainWindow::save() {
    QFileDialog d(this, "Save Game", TRUE);
    d.show();
}

void OctiMainWindow::saveAs() {
    QFileDialog d(this, "Save Game As", TRUE);
    d.show();
}

void OctiMainWindow::close() {

}

void OctiMainWindow::print() {

}

void OctiMainWindow::undo() {
    OctiBoardFrame *f = (OctiBoardFrame *) _ws->activeWindow();
    if(f != NULL) {
        try {
            f->getBoard()->getGame()->undo();
        } catch(IllegalMoveException e) {
            cerr << e.getMessage() << endl;
        }
    }
}

void OctiMainWindow::redo() {
    OctiBoardFrame *f = (OctiBoardFrame *) _ws->activeWindow();
    if(f != NULL) {
        try {
            f->getBoard()->getGame()->redo();
        } catch(IllegalMoveException e) {
            cerr << e.getMessage() << endl;
        }
    }
}

void OctiMainWindow::move() {
    OctiBoardFrame *f = (OctiBoardFrame *) _ws->activeWindow();
    if(f != NULL) {
        try {
            f->getBoard()->getGame()->commitPreparedMove();
        } catch(IllegalMoveException e) {
            cerr << e.getMessage() << endl;
        }
    }
}

void OctiMainWindow::cancel() {
    OctiBoardFrame *f = (OctiBoardFrame *) _ws->activeWindow();
    if(f != NULL) {
        try {
            f->getBoard()->getGame()->rollbackPreparedMove();
        } catch(IllegalMoveException e) {
            cerr << e.getMessage() << endl;
        }
    }
}

void OctiMainWindow::pause() {

}

void OctiMainWindow::resign() {

}

void OctiMainWindow::about() {

}

void OctiMainWindow::pref() {
    OctiPreferencesDialog d(this, NULL, TRUE);
    d.show();

    if(d.result() == QDialog::Rejected) {
        return;
    }
}

void OctiMainWindow::client() {
    LoginDialog d(this, NULL, TRUE);
    d.show();

    if(d.result() == QDialog::Rejected) {
        return;
    }
}

void OctiMainWindow::server() {

}

void OctiMainWindow::windowsMenuAboutToShow() {
    _windowsMenu->clear();
    int cascadeId = _windowsMenu->insertItem("&Cascade", _ws, SLOT(cascade()));
    int tileId = _windowsMenu->insertItem("&Tile", _ws, SLOT(tile()));
    if(_ws->windowList().isEmpty()) {
        _windowsMenu->setItemEnabled(cascadeId, FALSE);
        _windowsMenu->setItemEnabled(tileId, FALSE);
    }
    _windowsMenu->insertSeparator();
    QWidgetList windows = _ws->windowList();
    for(int i = 0; i < int (windows.count()); ++i) {
        int id = _windowsMenu->insertItem(windows.at(i)->caption(),
                                          this, SLOT(windowsMenuActivated(int)));
        _windowsMenu->setItemParameter(id, i);
        _windowsMenu->setItemChecked(id, _ws->activeWindow() == windows.at(i));
    }
}

void OctiMainWindow::windowsMenuActivated(int id) {
    QWidget *w = _ws->windowList().at(id);
    if(w) {
        w->setFocus();
    }
}

void OctiMainWindow::windowActivated(QWidget *w) {
    OctiGame *game = ((OctiBoardFrame *) w)->getBoard()->getGame();
    _gameUndo->setEnabled(game->isUndoAllowed() > 0);
    _gameRedo->setEnabled(game->isRedoAllowed() > 0);
    _gameMove->setEnabled(game->isMakeMoveAllowed());
    _gameCancel->setEnabled(game->isCancelMoveAllowed());
    _gamePause->setEnabled(game->isPauseGameAllowed());
    _gameResign->setEnabled(game->isResignAllowed());
}

void OctiMainWindow::windowDestroyed() {
    if(--_windowCount == 0) {
#ifndef QT_NO_PRINTER
        _gamePrint->setEnabled(FALSE);
#endif
        _gameUndo->setEnabled(FALSE);
        _gameRedo->setEnabled(FALSE);
        _gameMove->setEnabled(FALSE);
        _gameCancel->setEnabled(FALSE);
        _gamePause->setEnabled(FALSE);
        _gameResign->setEnabled(FALSE);
    }
}

void OctiMainWindow::gameChanged(OctiBoard *src, OctiGame *oldGame, OctiGame *newGame) {
    if(oldGame) {
        oldGame->removeOctiGameListener(this);
    }
    newGame->addOctiGameListener(this);
}

void OctiMainWindow::boardUpdated(OctiBoard *src, const OctiMove *move, boolean undo) {

}

void OctiMainWindow::gameResetted(OctiGame *src) {

}

void OctiMainWindow::gameStarted(OctiGame *src) {

}

void OctiMainWindow::gameResigned(OctiGame *src) {

}

void OctiMainWindow::gameUpdated(OctiGame *src) {

}

void OctiMainWindow::gameOver(OctiGame *src) {

}

void OctiMainWindow::canUndo(OctiGame *src, int steps) {
    OctiBoardFrame *f = (OctiBoardFrame *) _ws->activeWindow();
    if(f != NULL && f->getBoard()->getGame() == src) {
        _gameUndo->setEnabled(steps > 0);
    }
}

void OctiMainWindow::canRedo(OctiGame *src, int steps) {
    OctiBoardFrame *f = (OctiBoardFrame *) _ws->activeWindow();
    if(f != NULL && f->getBoard()->getGame() == src) {
        _gameRedo->setEnabled(steps > 0);
    }
}

void OctiMainWindow::canMakeMove(OctiGame *src, boolean b) {
    OctiBoardFrame *f = (OctiBoardFrame *) _ws->activeWindow();
    if(f != NULL && f->getBoard()->getGame() == src) {
        _gameMove->setEnabled(b);
    }
}

void OctiMainWindow::canCancelMove(OctiGame *src, boolean b) {
    OctiBoardFrame *f = (OctiBoardFrame *) _ws->activeWindow();
    if(f != NULL && f->getBoard()->getGame() == src) {
        _gameCancel->setEnabled(b);
    }
}

void OctiMainWindow::canPauseGame(OctiGame *src, boolean b) {

}

void OctiMainWindow::canResign(OctiGame *src, boolean b) {

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
