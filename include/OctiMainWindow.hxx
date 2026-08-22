#ifndef _OCTIMAINWINDOW_HXX_
#define _OCTIMAINWINDOW_HXX_

#include "qt2compat.h"
#include <QMainWindow>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>
#include <QPrinter>
#include "OctiGame.hxx"
#include "OctiBoardFrame.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class OCTIAPI OctiMainWindow : public QMainWindow,
                               private OctiBoardListener,
                               private OctiGameListener {

    Q_OBJECT;

public:

    OctiMainWindow(QWidget *parent = 0, const char *name = 0,
                   Qt::WindowFlags f = Qt::Window);
    ~OctiMainWindow();

protected slots:

    void newGame();
    void load();
    void save();
    void saveAs();
    void closeGame();
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
    void windowsMenuActivated();
    void windowActivated(QMdiSubWindow *w);
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

    OctiBoardFrame *activeBoardFrame() const;

private:

    int _windowCount;
    QMdiArea *_ws;
    QToolBar *_toolbar;
    QMenu *_windowsMenu;
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
