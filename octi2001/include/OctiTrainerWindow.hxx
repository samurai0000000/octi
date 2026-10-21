/*************************************************************************
 * $Id: OctiTrainerWindow.hxx,v 1.1 2001/07/31 05:09:36 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/07/31 05:09:36 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created: 12/20/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _OCTITRAINERWINDOW_HXX_
#define _OCTITRAINERWINDOW_HXX_

#include <qlistbox.h>
#include <qworkspace.h>
#include <qmainwindow.h>
#include "OctiBoardDispWidget.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class OCTIAPI OctiTrainerWindow : public QMainWindow {

    Q_OBJECT;

public:

    OctiTrainerWindow(QWidget * parent = 0, const char *name = 0, WFlags f = WType_TopLevel);
    ~OctiTrainerWindow();

protected slots:

    void rand();
    void cont();
    void stat();
    void analyzeCF();
    void analyzeNN();
    void about();
    void moveSelected(int i);

private:

    QToolBar *_toolbar;
    QPopupMenu *_windowsMenu;
    QListBox *_listbox;
    OctiBoardWidget *_bw;
    boolean _moved;
    int _lastitem;

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
