/*************************************************************************
 * $Id: OctiMainWindow.cxx,v 1.5 2001/08/16 02:21:50 samurai Exp $
 * Ported from Qt 2 to Qt 5.
 *************************************************************************/

#define _OCTIAPI_IMPL_

#include "qt2compat.h"
#include <QImage>
#include <QPixmap>
#include <QFileDialog>
#include <QLabel>
#include <QStatusBar>
#include <QMessageBox>
#include <QPrinter>
#include <QApplication>
#include <QPushButton>
#include <QTextStream>
#include <QPainter>
#include <QWhatsThis>
#include <QMenuBar>
#include <QAction>
#include <iostream>
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

OctiMainWindow::OctiMainWindow(QWidget *parent, const char *name, Qt::WindowFlags f)
    : QMainWindow(parent, f), _windowCount(0)
{
    qt2InitWidget(this, name ? name : "OctiMainWindow");
    setWindowTitle("OCTI");
    setWindowIcon(QIcon(QPixmap(icon_octi_xpm)));

    _toolbar = new QToolBar(tr("Tool Bar"), this);
    addToolBar(Qt::TopToolBarArea, _toolbar);

    _gameNew = qt2ToolButton(QPixmap(tb_new_xpm), tr("New Game"),
                             this, SLOT(newGame()), _toolbar);
    _gameLoad = qt2ToolButton(QPixmap(tb_open_xpm), tr("Load Game"),
                              this, SLOT(load()), _toolbar);
    _gameSave = qt2ToolButton(QPixmap(tb_save_xpm), tr("Save Game"),
                              this, SLOT(save()), _toolbar);
    _winClose = qt2ToolButton(QPixmap(tb_winclose_xpm), tr("Close Game"),
                              this, SLOT(closeGame()), _toolbar);
#ifndef QT_NO_PRINTER
    _printer = new QPrinter;
    _gamePrint = qt2ToolButton(QPixmap(tb_print_xpm), tr("Print Game"),
                               this, SLOT(print()), _toolbar);
#endif
    _toolbar->addSeparator();
    _gameUndo = qt2ToolButton(QPixmap(tb_undo_xpm), tr("Undo Move"),
                              this, SLOT(undo()), _toolbar);
    _gameRedo = qt2ToolButton(QPixmap(tb_redo_xpm), tr("Redo Move"),
                              this, SLOT(redo()), _toolbar);
    _toolbar->addSeparator();
    _gameMove = qt2ToolButton(QPixmap(tb_ok_xpm), tr("Make Move"),
                              this, SLOT(move()), _toolbar);
    _gameCancel = qt2ToolButton(QPixmap(tb_cancel_xpm), tr("Cancel Move"),
                                this, SLOT(cancel()), _toolbar);
    _gamePause = qt2ToolButton(QPixmap(), tr("Pause Game"),
                               this, SLOT(pause()), _toolbar);
    _gameResign = qt2ToolButton(QPixmap(), tr("Resign Game"),
                                this, SLOT(resign()), _toolbar);
    _toolbar->addSeparator();
    _appExit = qt2ToolButton(QPixmap(tb_exit_xpm), tr("Exit Application"),
                             qApp, SLOT(closeAllWindows()), _toolbar);

#ifndef QT_NO_PRINTER
    _gamePrint->setEnabled(false);
#endif
    _gameUndo->setEnabled(false);
    _gameRedo->setEnabled(false);
    _gameMove->setEnabled(false);
    _gameCancel->setEnabled(false);
    _gamePause->setEnabled(false);
    _gameResign->setEnabled(false);

    _toolbar->addAction(QWhatsThis::createAction(this));

    _gameNew->setWhatsThis(G_OMW_GAME_NEW_TEXT);
    _gameLoad->setWhatsThis(G_OMW_GAME_LOAD_TEXT);
    _gameSave->setWhatsThis(G_OMW_GAME_SAVE_TEXT);

    QMenu *game = menuBar()->addMenu(tr("&Game"));
    game->addAction(QIcon(QPixmap(tb_new_xpm)), tr("&New"), this, SLOT(newGame()),
                    QKeySequence::New);
    game->addAction(QIcon(QPixmap(tb_open_xpm)), tr("&Load"), this, SLOT(load()),
                    QKeySequence::Open);
    game->addAction(QIcon(QPixmap(tb_save_xpm)), tr("&Save"), this, SLOT(save()),
                    QKeySequence::Save);
    game->addAction(QIcon(QPixmap(tb_save_xpm)), tr("Save &as..."), this, SLOT(saveAs()));
#ifndef QT_NO_PRINTER
    game->addSeparator();
    game->addAction(QIcon(QPixmap(tb_print_xpm)), tr("&Print"), this, SLOT(print()),
                    QKeySequence::Print);
#endif
    game->addSeparator();
    game->addAction(QIcon(QPixmap(tb_undo_xpm)), tr("&Undo"), this, SLOT(undo()),
                    QKeySequence::Undo);
    game->addAction(QIcon(QPixmap(tb_redo_xpm)), tr("&Redo"), this, SLOT(redo()),
                    QKeySequence::Redo);
    game->addSeparator();
    game->addAction(QIcon(QPixmap(tb_ok_xpm)), tr("&Make Move"), this, SLOT(move()));
    game->addAction(QIcon(QPixmap(tb_cancel_xpm)), tr("&Cancel Move"), this, SLOT(cancel()));
    game->addAction(tr("&Pause Game"), this, SLOT(pause()));
    game->addAction(tr("Resign"), this, SLOT(resign()));
    game->addSeparator();
    game->addAction(QIcon(QPixmap(tb_winclose_xpm)), tr("&Close"), this, SLOT(closeGame()),
                    QKeySequence::Close);
    game->addAction(QIcon(QPixmap(tb_exit_xpm)), tr("&Quit"), qApp, SLOT(closeAllWindows()),
                    QKeySequence::Quit);

    QMenu *settings = menuBar()->addMenu(tr("&Settings"));
    settings->addAction(tr("&Client Connect"), this, SLOT(client()));
    settings->addAction(tr("Start Server"), this, SLOT(server()));
    settings->addSeparator();
    settings->addAction(tr("&Preferences"), this, SLOT(pref()));

    _windowsMenu = menuBar()->addMenu(tr("&Windows"));
    connect(_windowsMenu, SIGNAL(aboutToShow()), this, SLOT(windowsMenuAboutToShow()));

    QMenu *help = menuBar()->addMenu(tr("&Help"));
    help->addAction(tr("&About"), this, SLOT(about()), QKeySequence::HelpContents);
    help->addSeparator();
    help->addAction(QWhatsThis::createAction(this));

    _ws = new QMdiArea(this);
    _ws->setViewMode(QMdiArea::SubWindowView);
    connect(_ws, SIGNAL(subWindowActivated(QMdiSubWindow *)),
            this, SLOT(windowActivated(QMdiSubWindow *)));
    setCentralWidget(_ws);

    statusBar()->showMessage(tr("Ready"), 2000);
    setMinimumWidth(700);
    setMinimumHeight(600);
}

OctiMainWindow::~OctiMainWindow() {
    _windowCount = -1;
}

OctiBoardFrame *OctiMainWindow::activeBoardFrame() const {
    QMdiSubWindow *sub = _ws->activeSubWindow();
    if(!sub) {
        return NULL;
    }
    return qobject_cast<OctiBoardFrame *>(sub->widget());
}

void OctiMainWindow::newGame() {
    OctiGameSelectDialog d(this);
    if(d.exec() != QDialog::Accepted) {
        return;
    }

    OctiBoardFrame *f = new OctiBoardFrame;
    f->setAttribute(Qt::WA_DeleteOnClose);
    QMdiSubWindow *sub = _ws->addSubWindow(f);
    connect(f, SIGNAL(destroyed()), this, SLOT(windowDestroyed()));
    sub->setWindowTitle(tr("Untitled Game"));
    sub->setWindowIcon(QIcon(QPixmap(icon_octi_xpm)));

    OctiBoard *board = f->getBoard();
    board->setGameMode(d.getGameMode(), d.useSuperProngs(), d.edgeless());
    board->getGame()->addOctiGameListener(this);
    board->getGame()->start();

    if(_ws->subWindowList().count() <= 1) {
        sub->showMaximized();
    } else {
        sub->show();
    }

    _windowCount++;
#ifndef QT_NO_PRINTER
    _gamePrint->setEnabled(true);
#endif
}

void OctiMainWindow::load() {
    QFileDialog::getOpenFileName(this, tr("Load Game"));
}

void OctiMainWindow::save() {
    QFileDialog::getSaveFileName(this, tr("Save Game"));
}

void OctiMainWindow::saveAs() {
    QFileDialog::getSaveFileName(this, tr("Save Game As"));
}

void OctiMainWindow::closeGame() {
    if(QMdiSubWindow *sub = _ws->activeSubWindow()) {
        sub->close();
    }
}

void OctiMainWindow::print() {

}

void OctiMainWindow::undo() {
    OctiBoardFrame *f = activeBoardFrame();
    if(f != NULL) {
        try {
            f->getBoard()->getGame()->undo();
        } catch(IllegalMoveException e) {
            cerr << e.getMessage() << endl;
        }
    }
}

void OctiMainWindow::redo() {
    OctiBoardFrame *f = activeBoardFrame();
    if(f != NULL) {
        try {
            f->getBoard()->getGame()->redo();
        } catch(IllegalMoveException e) {
            cerr << e.getMessage() << endl;
        }
    }
}

void OctiMainWindow::move() {
    OctiBoardFrame *f = activeBoardFrame();
    if(f != NULL) {
        try {
            f->getBoard()->getGame()->commitPreparedMove();
        } catch(IllegalMoveException e) {
            cerr << e.getMessage() << endl;
        }
    }
}

void OctiMainWindow::cancel() {
    OctiBoardFrame *f = activeBoardFrame();
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
    QMessageBox::about(this, tr("About OCTI"),
                       tr("OCTI\n"
                          "A revival of the 2001 Selfsoft OCTI client.\n"
                          "Originally written by Charles Chiou."));
}

void OctiMainWindow::pref() {
    OctiPreferencesDialog d(this);
    d.exec();
}

void OctiMainWindow::client() {
    LoginDialog d(this);
    d.exec();
}

void OctiMainWindow::server() {

}

void OctiMainWindow::windowsMenuAboutToShow() {
    _windowsMenu->clear();
    QAction *cascadeAct = _windowsMenu->addAction(tr("&Cascade"), _ws, SLOT(cascadeSubWindows()));
    QAction *tileAct = _windowsMenu->addAction(tr("&Tile"), _ws, SLOT(tileSubWindows()));
    bool hasWindows = !_ws->subWindowList().isEmpty();
    cascadeAct->setEnabled(hasWindows);
    tileAct->setEnabled(hasWindows);
    _windowsMenu->addSeparator();
    QList<QMdiSubWindow *> windows = _ws->subWindowList();
    for(int i = 0; i < windows.count(); ++i) {
        QAction *a = _windowsMenu->addAction(windows.at(i)->windowTitle());
        a->setCheckable(true);
        a->setChecked(_ws->activeSubWindow() == windows.at(i));
        a->setData(i);
        connect(a, SIGNAL(triggered()), this, SLOT(windowsMenuActivated()));
    }
}

void OctiMainWindow::windowsMenuActivated() {
    QAction *a = qobject_cast<QAction *>(sender());
    if(!a) {
        return;
    }
    int id = a->data().toInt();
    QList<QMdiSubWindow *> windows = _ws->subWindowList();
    if(id >= 0 && id < windows.count()) {
        windows.at(id)->setFocus();
    }
}

void OctiMainWindow::windowActivated(QMdiSubWindow *w) {
    if(!w) {
        return;
    }
    OctiBoardFrame *frame = qobject_cast<OctiBoardFrame *>(w->widget());
    if(!frame) {
        return;
    }
    OctiGame *game = frame->getBoard()->getGame();
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
        _gamePrint->setEnabled(false);
#endif
        _gameUndo->setEnabled(false);
        _gameRedo->setEnabled(false);
        _gameMove->setEnabled(false);
        _gameCancel->setEnabled(false);
        _gamePause->setEnabled(false);
        _gameResign->setEnabled(false);
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
    OctiBoardFrame *f = activeBoardFrame();
    if(f != NULL && f->getBoard()->getGame() == src) {
        _gameUndo->setEnabled(steps > 0);
    }
}

void OctiMainWindow::canRedo(OctiGame *src, int steps) {
    OctiBoardFrame *f = activeBoardFrame();
    if(f != NULL && f->getBoard()->getGame() == src) {
        _gameRedo->setEnabled(steps > 0);
    }
}

void OctiMainWindow::canMakeMove(OctiGame *src, boolean b) {
    OctiBoardFrame *f = activeBoardFrame();
    if(f != NULL && f->getBoard()->getGame() == src) {
        _gameMove->setEnabled(b);
    }
}

void OctiMainWindow::canCancelMove(OctiGame *src, boolean b) {
    OctiBoardFrame *f = activeBoardFrame();
    if(f != NULL && f->getBoard()->getGame() == src) {
        _gameCancel->setEnabled(b);
    }
}

void OctiMainWindow::canPauseGame(OctiGame *src, boolean b) {

}

void OctiMainWindow::canResign(OctiGame *src, boolean b) {

}

__END_NAMESPACE(SELFSOFT);
