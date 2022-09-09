/*************************************************************************
 * $Id: OctiMainWindow.hxx,v 1.3 2001/07/31 05:11:04 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/07/31 05:11:04 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created: 12/20/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _OCTIMAINWINDOW_HXX_
#define _OCTIMAINWINDOW_HXX_

#include <qtoolbar.h>
#include <qtoolbutton.h>
//#include <qpopupmenu.h>
#include <qmenubar.h>
//#include <qworkspace.h>
#include <qmainwindow.h>
#include "OctiGame.hxx"
#include "OctiBoardFrame.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class OCTIAPI OctiMainWindow : public QMainWindow,
                               private OctiBoardListener,
                               private OctiGameListener {

    Q_OBJECT;

public:

    OctiMainWindow(QWidget * parent = 0, const char *name = 0, WFlags f = WType_TopLevel);
    ~OctiMainWindow();

protected slots:

    void newGame();
    void load();
    void save();
    void saveAs();
    void close();
    void print();
    void undo();
    void redo();
    void move();
    void cancel();
    void pause();
    void resign();
    void about();
    void pref();
    void client();
    void server();
    void windowsMenuAboutToShow();
    void windowsMenuActivated(int id);
    void windowActivated(QWidget *w);
    void windowDestroyed();

private:

    void gameChanged(OctiBoard *src, OctiGame *oldGame, OctiGame *newGame);
    void boardUpdated(OctiBoard *src, const OctiMove *move, boolean undo);
    void gameResetted(OctiGame *src);
    void gameStarted(OctiGame *src);
    void gameResigned(OctiGame *src);
    void gameUpdated(OctiGame *src);
    void gameOver(OctiGame *src);
    void canUndo(OctiGame *src, int steps);
    void canRedo(OctiGame *src, int steps);
    void canMakeMove(OctiGame *src, boolean b);
    void canCancelMove(OctiGame *src, boolean b);
    void canPauseGame(OctiGame *src, boolean b);
    void canResign(OctiGame *src, boolean b);

private:

    int _windowCount;
    QWorkspace *_ws;
    QToolBar *_toolbar;
    QPopupMenu *_windowsMenu;
    QToolButton *_gameNew;
    QToolButton *_gameLoad;
    QToolButton *_gameSave;
    QToolButton *_winClose;
#ifndef QT_NO_PRINTER
    QPrinter *_printer;
    QToolButton *_gamePrint;
#endif
    QToolButton *_gameUndo;
    QToolButton *_gameRedo;
    QToolButton *_gameMove;
    QToolButton *_gameCancel;
    QToolButton *_gamePause;
    QToolButton *_gameResign;
    QToolButton *_appExit;

};

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
