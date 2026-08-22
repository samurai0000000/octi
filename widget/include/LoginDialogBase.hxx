#ifndef _LOGINDIALOGBASE_HXX_
#define _LOGINDIALOGBASE_HXX_

#include "qt2compat.h"
#include "basecfg.h"
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>

class LoginDialogBase : public QDialog
{
    Q_OBJECT

public:
    LoginDialogBase(QWidget *parent = 0, const char *name = 0,
                    bool modal = false, WFlags fl = Qt::WindowFlags());
    ~LoginDialogBase();

    QLineEdit *loginLineEdit;
    QLineEdit *passwordLineEdit;
    QPushButton *buttonHelp;
    QPushButton *buttonOk;
    QPushButton *buttonCancel;
};

#endif
