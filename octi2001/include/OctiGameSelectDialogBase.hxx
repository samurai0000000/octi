#ifndef OCTIGAMESELECTDIALOGBASE_H
#define OCTIGAMESELECTDIALOGBASE_H

#include "qt2compat.h"
#include "basecfg.h"
#include <QDialog>
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QRadioButton>
#include <QGridLayout>
#include <QHBoxLayout>

class OctiGameSelectDialogBase : public QDialog
{
    Q_OBJECT

public:

    OctiGameSelectDialogBase(QWidget *parent = 0, const char *name = 0,
                             bool modal = false, WFlags fl = Qt::WindowFlags());
    ~OctiGameSelectDialogBase();

    QPushButton *buttonHelp;
    QPushButton *buttonOk;
    QPushButton *buttonCancel;
    QGroupBox *ButtonGroup1;
    QRadioButton *fourPlayerButton;
    QRadioButton *twoPlayerButton;
    QGroupBox *GroupBox3;
    QCheckBox *useTimeLimitCheckBox;
    QComboBox *timeLimitComboBox;
    QGroupBox *ButtonGroup2;
    QRadioButton *threeBaseGameButton;
    QRadioButton *oneBaseGameButton;
    QGroupBox *GroupBox1;
    QCheckBox *edgelessCheckBox;
    QCheckBox *superProngCheckBox;

protected:
    QGridLayout *OctiGameSelectDialogBaseLayout;
    QHBoxLayout *Layout1;
};

#endif
