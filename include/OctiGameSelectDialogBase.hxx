/****************************************************************************
** Form interface generated from reading ui file 'ui/OctiGameSelectDialogBase.ui'
**
** Created: Thu Aug 1 17:35:11 2002
**      by:  The User Interface Compiler (uic)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/
#ifndef OCTIGAMESELECTDIALOGBASE_H
#define OCTIGAMESELECTDIALOGBASE_H

#include <qvariant.h>
#include <qdialog.h>
class QVBoxLayout; 
class QHBoxLayout; 
class QGridLayout; 
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QPushButton;
class QRadioButton;

class OctiGameSelectDialogBase : public QDialog
{ 
    Q_OBJECT

public:
    OctiGameSelectDialogBase( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~OctiGameSelectDialogBase();

    QPushButton* buttonHelp;
    QPushButton* buttonOk;
    QPushButton* buttonCancel;
    QButtonGroup* ButtonGroup1;
    QRadioButton* fourPlayerButton;
    QRadioButton* twoPlayerButton;
    QGroupBox* GroupBox3;
    QCheckBox* useTimeLimitCheckBox;
    QComboBox* timeLimitComboBox;
    QButtonGroup* ButtonGroup2;
    QRadioButton* threeBaseGameButton;
    QRadioButton* oneBaseGameButton;
    QGroupBox* GroupBox1;
    QCheckBox* edgelessCheckBox;
    QCheckBox* superProngCheckBox;


protected:
    QGridLayout* OctiGameSelectDialogBaseLayout;
    QHBoxLayout* Layout1;
};

#endif // OCTIGAMESELECTDIALOGBASE_H
