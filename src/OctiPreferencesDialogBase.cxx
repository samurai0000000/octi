/****************************************************************************
 ** Form implementation generated from reading ui file 'ui/OctiPreferencesDialogBase.ui'
 **
 ** Created: Thu Aug 1 17:35:12 2002
 **      by:  The User Interface Compiler (uic)
 **
 ** WARNING! All changes made in this file will be lost!
 ****************************************************************************/
#include "OctiPreferencesDialogBase.hxx"

#include <qvariant.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qtabwidget.h>
#include <qwidget.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

/*
 *  Constructs a OctiPreferencesDialogBase which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
OctiPreferencesDialogBase::OctiPreferencesDialogBase( QWidget* parent,  const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
        setName( "OctiPreferencesDialogBase" );
    resize( 624, 356 );
    setCaption( trUtf8( "OctiPreferencesDialogBase" ) );
    setSizeGripEnabled( TRUE );
    OctiPreferencesDialogBaseLayout = new QVBoxLayout( this, 11, 6, "OctiPreferencesDialogBaseLayout");

    tabWidget = new QTabWidget( this, "tabWidget" );

    Widget2 = new QWidget( tabWidget, "Widget2" );

    GroupBox1 = new QGroupBox( Widget2, "GroupBox1" );
    GroupBox1->setGeometry( QRect( 40, 20, 69, 192 ) );
    GroupBox1->setTitle( trUtf8( "Colors" ) );
    GroupBox1->setColumnLayout(0, Qt::Vertical );
    GroupBox1->layout()->setSpacing( 6 );
    GroupBox1->layout()->setMargin( 11 );
    GroupBox1Layout = new QVBoxLayout( GroupBox1->layout() );
    GroupBox1Layout->setAlignment( Qt::AlignTop );

    TextLabel1 = new QLabel( GroupBox1, "TextLabel1" );
    TextLabel1->setText( trUtf8( "Line" ) );
    GroupBox1Layout->addWidget( TextLabel1 );

    TextLabel2 = new QLabel( GroupBox1, "TextLabel2" );
    TextLabel2->setText( trUtf8( "Label" ) );
    GroupBox1Layout->addWidget( TextLabel2 );

    TextLabel3 = new QLabel( GroupBox1, "TextLabel3" );
    TextLabel3->setText( trUtf8( "Square" ) );
    GroupBox1Layout->addWidget( TextLabel3 );

    TextLabel4 = new QLabel( GroupBox1, "TextLabel4" );
    TextLabel4->setText( trUtf8( "Contrast" ) );
    GroupBox1Layout->addWidget( TextLabel4 );

    TextLabel5 = new QLabel( GroupBox1, "TextLabel5" );
    TextLabel5->setText( trUtf8( "Player 1" ) );
    GroupBox1Layout->addWidget( TextLabel5 );

    TextLabel6 = new QLabel( GroupBox1, "TextLabel6" );
    TextLabel6->setText( trUtf8( "Player 2" ) );
    GroupBox1Layout->addWidget( TextLabel6 );

    TextLabel7 = new QLabel( GroupBox1, "TextLabel7" );
    TextLabel7->setText( trUtf8( "Player 3" ) );
    GroupBox1Layout->addWidget( TextLabel7 );

    TextLabel8 = new QLabel( GroupBox1, "TextLabel8" );
    TextLabel8->setText( trUtf8( "Player 4" ) );
    GroupBox1Layout->addWidget( TextLabel8 );
    tabWidget->insertTab( Widget2, trUtf8( "Appearances" ) );

    Widget3 = new QWidget( tabWidget, "Widget3" );
    Widget3Layout = new QGridLayout( Widget3, 1, 1, 11, 6, "Widget3Layout");

    GroupBox3 = new QGroupBox( Widget3, "GroupBox3" );
    GroupBox3->setTitle( trUtf8( "Hash Table" ) );
    GroupBox3->setColumnLayout(0, Qt::Vertical );
    GroupBox3->layout()->setSpacing( 6 );
    GroupBox3->layout()->setMargin( 11 );
    GroupBox3Layout = new QVBoxLayout( GroupBox3->layout() );
    GroupBox3Layout->setAlignment( Qt::AlignTop );

    CheckBox5 = new QCheckBox( GroupBox3, "CheckBox5" );
    CheckBox5->setText( trUtf8( "Use Hash Table" ) );
    GroupBox3Layout->addWidget( CheckBox5 );

    CheckBox8 = new QCheckBox( GroupBox3, "CheckBox8" );
    CheckBox8->setEnabled( FALSE );
    CheckBox8->setText( trUtf8( "Check Collision" ) );
    GroupBox3Layout->addWidget( CheckBox8 );

    Layout8 = new QHBoxLayout( 0, 0, 6, "Layout8");

    TextLabel12 = new QLabel( GroupBox3, "TextLabel12" );
    TextLabel12->setText( trUtf8( "Memory (KB)" ) );
    Layout8->addWidget( TextLabel12 );

    SpinBox5 = new QSpinBox( GroupBox3, "SpinBox5" );
    SpinBox5->setEnabled( FALSE );
    SpinBox5->setMaxValue( 999999999 );
    Layout8->addWidget( SpinBox5 );
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout8->addItem( spacer );
    GroupBox3Layout->addLayout( Layout8 );

    Widget3Layout->addWidget( GroupBox3, 1, 1 );

    ButtonGroup1 = new QButtonGroup( Widget3, "ButtonGroup1" );
    ButtonGroup1->setTitle( trUtf8( "AI Algorithm" ) );
    ButtonGroup1->setColumnLayout(0, Qt::Vertical );
    ButtonGroup1->layout()->setSpacing( 6 );
    ButtonGroup1->layout()->setMargin( 11 );
    ButtonGroup1Layout = new QVBoxLayout( ButtonGroup1->layout() );
    ButtonGroup1Layout->setAlignment( Qt::AlignTop );

    ButtonGroup2 = new QButtonGroup( ButtonGroup1, "ButtonGroup2" );
    ButtonGroup2->setTitle( trUtf8( "" ) );
    ButtonGroup2->setColumnLayout(0, Qt::Vertical );
    ButtonGroup2->layout()->setSpacing( 6 );
    ButtonGroup2->layout()->setMargin( 11 );
    ButtonGroup2Layout = new QVBoxLayout( ButtonGroup2->layout() );
    ButtonGroup2Layout->setAlignment( Qt::AlignTop );

    RadioButton1 = new QRadioButton( ButtonGroup2, "RadioButton1" );
    RadioButton1->setText( trUtf8( "AlphaBeta - Single Thread" ) );
    RadioButton1->setChecked( TRUE );
    ButtonGroup2Layout->addWidget( RadioButton1 );

    RadioButton2 = new QRadioButton( ButtonGroup2, "RadioButton2" );
    RadioButton2->setText( trUtf8( "AlphaBeta - Multi Thread" ) );
    ButtonGroup2Layout->addWidget( RadioButton2 );

    RadioButton3 = new QRadioButton( ButtonGroup2, "RadioButton3" );
    RadioButton3->setText( trUtf8( "ER (Evaluate-Refute) - Single Thread" ) );
    ButtonGroup2Layout->addWidget( RadioButton3 );

    RadioButton4 = new QRadioButton( ButtonGroup2, "RadioButton4" );
    RadioButton4->setText( trUtf8( "ER (Evaluate-Refute) - Multi Thread" ) );
    ButtonGroup2Layout->addWidget( RadioButton4 );
    ButtonGroup1Layout->addWidget( ButtonGroup2 );

    Layout4 = new QHBoxLayout( 0, 0, 6, "Layout4");

    TextLabel10 = new QLabel( ButtonGroup1, "TextLabel10" );
    TextLabel10->setEnabled( TRUE );
    TextLabel10->setText( trUtf8( "Thread Size" ) );
    Layout4->addWidget( TextLabel10 );

    SpinBox2 = new QSpinBox( ButtonGroup1, "SpinBox2" );
    SpinBox2->setEnabled( FALSE );
    SpinBox2->setValue( 0 );
    Layout4->addWidget( SpinBox2 );
    QSpacerItem* spacer_2 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout4->addItem( spacer_2 );
    ButtonGroup1Layout->addLayout( Layout4 );

    Widget3Layout->addMultiCellWidget( ButtonGroup1, 0, 1, 0, 0 );

    GroupBox4 = new QGroupBox( Widget3, "GroupBox4" );
    GroupBox4->setTitle( trUtf8( "Algorithm Parameters" ) );
    GroupBox4->setColumnLayout(0, Qt::Vertical );
    GroupBox4->layout()->setSpacing( 6 );
    GroupBox4->layout()->setMargin( 11 );
    GroupBox4Layout = new QVBoxLayout( GroupBox4->layout() );
    GroupBox4Layout->setAlignment( Qt::AlignTop );

    CheckBox6 = new QCheckBox( GroupBox4, "CheckBox6" );
    CheckBox6->setText( trUtf8( "Iterative Deepening" ) );
    CheckBox6->setChecked( TRUE );
    GroupBox4Layout->addWidget( CheckBox6 );

    Layout9 = new QHBoxLayout( 0, 0, 6, "Layout9");

    CheckBox7 = new QCheckBox( GroupBox4, "CheckBox7" );
    CheckBox7->setText( trUtf8( "Timeout (seconds)" ) );
    CheckBox7->setChecked( TRUE );
    Layout9->addWidget( CheckBox7 );

    SpinBox3 = new QSpinBox( GroupBox4, "SpinBox3" );
    SpinBox3->setMaxValue( 999999999 );
    SpinBox3->setValue( 0 );
    Layout9->addWidget( SpinBox3 );
    QSpacerItem* spacer_3 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout9->addItem( spacer_3 );
    GroupBox4Layout->addLayout( Layout9 );

    Layout10 = new QHBoxLayout( 0, 0, 6, "Layout10");

    TextLabel11 = new QLabel( GroupBox4, "TextLabel11" );
    TextLabel11->setText( trUtf8( "Depth Limit" ) );
    Layout10->addWidget( TextLabel11 );

    SpinBox4 = new QSpinBox( GroupBox4, "SpinBox4" );
    SpinBox4->setValue( 0 );
    Layout10->addWidget( SpinBox4 );
    QSpacerItem* spacer_4 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout10->addItem( spacer_4 );
    GroupBox4Layout->addLayout( Layout10 );

    Widget3Layout->addWidget( GroupBox4, 0, 1 );
    tabWidget->insertTab( Widget3, trUtf8( "AI Parameters" ) );

    tab = new QWidget( tabWidget, "tab" );
    tabLayout = new QVBoxLayout( tab, 11, 6, "tabLayout");

    Layout20 = new QHBoxLayout( 0, 0, 6, "Layout20");

    GroupBox5 = new QGroupBox( tab, "GroupBox5" );
    GroupBox5->setTitle( trUtf8( "OCTI Server Settings" ) );
    GroupBox5->setColumnLayout(0, Qt::Vertical );
    GroupBox5->layout()->setSpacing( 6 );
    GroupBox5->layout()->setMargin( 11 );
    GroupBox5Layout = new QVBoxLayout( GroupBox5->layout() );
    GroupBox5Layout->setAlignment( Qt::AlignTop );

    Layout15 = new QHBoxLayout( 0, 0, 6, "Layout15");
    QSpacerItem* spacer_5 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout15->addItem( spacer_5 );

    TextLabel13 = new QLabel( GroupBox5, "TextLabel13" );
    TextLabel13->setText( trUtf8( "Server Address" ) );
    Layout15->addWidget( TextLabel13 );

    LineEdit2 = new QLineEdit( GroupBox5, "LineEdit2" );
    Layout15->addWidget( LineEdit2 );
    GroupBox5Layout->addLayout( Layout15 );

    Layout16 = new QHBoxLayout( 0, 0, 6, "Layout16");
    QSpacerItem* spacer_6 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout16->addItem( spacer_6 );

    TextLabel14 = new QLabel( GroupBox5, "TextLabel14" );
    TextLabel14->setText( trUtf8( "Server Port" ) );
    Layout16->addWidget( TextLabel14 );

    LineEdit3 = new QLineEdit( GroupBox5, "LineEdit3" );
    Layout16->addWidget( LineEdit3 );
    GroupBox5Layout->addLayout( Layout16 );
    Layout20->addWidget( GroupBox5 );
    QSpacerItem* spacer_7 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout20->addItem( spacer_7 );
    tabLayout->addLayout( Layout20 );

    Layout21 = new QHBoxLayout( 0, 0, 6, "Layout21");

    GroupBox6 = new QGroupBox( tab, "GroupBox6" );
    GroupBox6->setTitle( trUtf8( "Local Server Settings (For Incoming Connections)" ) );
    GroupBox6->setColumnLayout(0, Qt::Vertical );
    GroupBox6->layout()->setSpacing( 6 );
    GroupBox6->layout()->setMargin( 11 );
    GroupBox6Layout = new QVBoxLayout( GroupBox6->layout() );
    GroupBox6Layout->setAlignment( Qt::AlignTop );

    Layout19 = new QHBoxLayout( 0, 0, 6, "Layout19");
    QSpacerItem* spacer_8 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout19->addItem( spacer_8 );

    CheckBox9 = new QCheckBox( GroupBox6, "CheckBox9" );
    CheckBox9->setText( trUtf8( "Enabled" ) );
    Layout19->addWidget( CheckBox9 );
    QSpacerItem* spacer_9 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout19->addItem( spacer_9 );
    GroupBox6Layout->addLayout( Layout19 );

    Layout17 = new QHBoxLayout( 0, 0, 6, "Layout17");
    QSpacerItem* spacer_10 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout17->addItem( spacer_10 );

    TextLabel15 = new QLabel( GroupBox6, "TextLabel15" );
    TextLabel15->setText( trUtf8( "Local Address" ) );
    Layout17->addWidget( TextLabel15 );

    LineEdit4 = new QLineEdit( GroupBox6, "LineEdit4" );
    Layout17->addWidget( LineEdit4 );
    GroupBox6Layout->addLayout( Layout17 );

    Layout18 = new QHBoxLayout( 0, 0, 6, "Layout18");
    QSpacerItem* spacer_11 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout18->addItem( spacer_11 );

    TextLabel16 = new QLabel( GroupBox6, "TextLabel16" );
    TextLabel16->setText( trUtf8( "Listening Port" ) );
    Layout18->addWidget( TextLabel16 );

    LineEdit5 = new QLineEdit( GroupBox6, "LineEdit5" );
    Layout18->addWidget( LineEdit5 );
    GroupBox6Layout->addLayout( Layout18 );
    Layout21->addWidget( GroupBox6 );
    QSpacerItem* spacer_12 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout21->addItem( spacer_12 );
    tabLayout->addLayout( Layout21 );
    tabWidget->insertTab( tab, trUtf8( "Networking" ) );

    tab_2 = new QWidget( tabWidget, "tab_2" );
    tabWidget->insertTab( tab_2, trUtf8( "Personal Settings" ) );
    OctiPreferencesDialogBaseLayout->addWidget( tabWidget );

    Layout1 = new QHBoxLayout( 0, 0, 6, "Layout1");

    buttonHelp = new QPushButton( this, "buttonHelp" );
    buttonHelp->setText( trUtf8( "&Help" ) );
    buttonHelp->setAutoDefault( TRUE );
    Layout1->addWidget( buttonHelp );
    QSpacerItem* spacer_13 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout1->addItem( spacer_13 );

    buttonApply = new QPushButton( this, "buttonApply" );
    buttonApply->setText( trUtf8( "&Apply" ) );
    buttonApply->setAutoDefault( TRUE );
    Layout1->addWidget( buttonApply );

    buttonOk = new QPushButton( this, "buttonOk" );
    buttonOk->setText( trUtf8( "&OK" ) );
    buttonOk->setAutoDefault( TRUE );
    buttonOk->setDefault( TRUE );
    Layout1->addWidget( buttonOk );

    buttonCancel = new QPushButton( this, "buttonCancel" );
    buttonCancel->setText( trUtf8( "&Cancel" ) );
    buttonCancel->setAutoDefault( TRUE );
    Layout1->addWidget( buttonCancel );
    OctiPreferencesDialogBaseLayout->addLayout( Layout1 );

    // signals and slots connections
    connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
    connect( RadioButton2, SIGNAL( toggled(bool) ), SpinBox2, SLOT( setEnabled(bool) ) );
    connect( RadioButton4, SIGNAL( toggled(bool) ), SpinBox2, SLOT( setEnabled(bool) ) );
    connect( RadioButton1, SIGNAL( toggled(bool) ), SpinBox2, SLOT( setDisabled(bool) ) );
    connect( RadioButton3, SIGNAL( toggled(bool) ), SpinBox2, SLOT( setDisabled(bool) ) );
    connect( CheckBox7, SIGNAL( toggled(bool) ), SpinBox3, SLOT( setEnabled(bool) ) );
    connect( CheckBox5, SIGNAL( toggled(bool) ), SpinBox5, SLOT( setEnabled(bool) ) );
    connect( CheckBox5, SIGNAL( toggled(bool) ), CheckBox8, SLOT( setEnabled(bool) ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
OctiPreferencesDialogBase::~OctiPreferencesDialogBase()
{
    // no need to delete child widgets, Qt does it all for us
}


/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
