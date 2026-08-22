#ifndef OCTIPREFERENCESDIALOGBASE_H
#define OCTIPREFERENCESDIALOGBASE_H

#include "qt2compat.h"
#include "basecfg.h"
#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTabWidget>

class OctiPreferencesDialogBase : public QDialog
{
    Q_OBJECT

public:
    OctiPreferencesDialogBase(QWidget *parent = 0, const char *name = 0,
                              bool modal = false, WFlags fl = Qt::WindowFlags());
    ~OctiPreferencesDialogBase();

    QTabWidget *tabWidget;
    QPushButton *buttonHelp;
    QPushButton *buttonApply;
    QPushButton *buttonOk;
    QPushButton *buttonCancel;
};

#endif
