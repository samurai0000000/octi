/****************************************************************************
 ** Form interface generated from reading ui file 'ui/OctiPreferencesDialogBase.ui'
 **
 ** Created: Thu Aug 1 17:35:11 2002
 **      by:  The User Interface Compiler (uic)
 **
 ** WARNING! All changes made in this file will be lost!
 ****************************************************************************/
#ifndef OCTIPREFERENCESDIALOGBASE_H
#define OCTIPREFERENCESDIALOGBASE_H

#include <qvariant.h>
#include <qdialog.h>
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QButtonGroup;
class QCheckBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QTabWidget;
class QWidget;

class OctiPreferencesDialogBase : public QDialog
{
    Q_OBJECT

public:
    OctiPreferencesDialogBase( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~OctiPreferencesDialogBase();

    QTabWidget* tabWidget;
    QWidget* Widget2;
    QGroupBox* GroupBox1;
    QLabel* TextLabel1;
    QLabel* TextLabel2;
    QLabel* TextLabel3;
    QLabel* TextLabel4;
    QLabel* TextLabel5;
    QLabel* TextLabel6;
    QLabel* TextLabel7;
    QLabel* TextLabel8;
    QWidget* Widget3;
    QGroupBox* GroupBox3;
    QCheckBox* CheckBox5;
    QCheckBox* CheckBox8;
    QLabel* TextLabel12;
    QSpinBox* SpinBox5;
    QButtonGroup* ButtonGroup1;
    QButtonGroup* ButtonGroup2;
    QRadioButton* RadioButton1;
    QRadioButton* RadioButton2;
    QRadioButton* RadioButton3;
    QRadioButton* RadioButton4;
    QLabel* TextLabel10;
    QSpinBox* SpinBox2;
    QGroupBox* GroupBox4;
    QCheckBox* CheckBox6;
    QCheckBox* CheckBox7;
    QSpinBox* SpinBox3;
    QLabel* TextLabel11;
    QSpinBox* SpinBox4;
    QWidget* tab;
    QGroupBox* GroupBox5;
    QLabel* TextLabel13;
    QLineEdit* LineEdit2;
    QLabel* TextLabel14;
    QLineEdit* LineEdit3;
    QGroupBox* GroupBox6;
    QCheckBox* CheckBox9;
    QLabel* TextLabel15;
    QLineEdit* LineEdit4;
    QLabel* TextLabel16;
    QLineEdit* LineEdit5;
    QWidget* tab_2;
    QPushButton* buttonHelp;
    QPushButton* buttonApply;
    QPushButton* buttonOk;
    QPushButton* buttonCancel;


protected:
    QVBoxLayout* OctiPreferencesDialogBaseLayout;
    QVBoxLayout* GroupBox1Layout;
    QGridLayout* Widget3Layout;
    QVBoxLayout* GroupBox3Layout;
    QHBoxLayout* Layout8;
    QVBoxLayout* ButtonGroup1Layout;
    QVBoxLayout* ButtonGroup2Layout;
    QHBoxLayout* Layout4;
    QVBoxLayout* GroupBox4Layout;
    QHBoxLayout* Layout9;
    QHBoxLayout* Layout10;
    QVBoxLayout* tabLayout;
    QHBoxLayout* Layout20;
    QVBoxLayout* GroupBox5Layout;
    QHBoxLayout* Layout15;
    QHBoxLayout* Layout16;
    QHBoxLayout* Layout21;
    QVBoxLayout* GroupBox6Layout;
    QHBoxLayout* Layout19;
    QHBoxLayout* Layout17;
    QHBoxLayout* Layout18;
    QHBoxLayout* Layout1;
};

#endif // OCTIPREFERENCESDIALOGBASE_H

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
