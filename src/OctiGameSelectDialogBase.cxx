/****************************************************************************
 ** Form implementation generated from reading ui file 'ui/OctiGameSelectDialogBase.ui'
 **
 ** Created: Thu Aug 1 17:35:11 2002
 **      by:  The User Interface Compiler (uic)
 **
 ** WARNING! All changes made in this file will be lost!
 ****************************************************************************/
#include "OctiGameSelectDialogBase.hxx"

#include <qvariant.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

/* 
 *  Constructs a OctiGameSelectDialogBase which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
OctiGameSelectDialogBase::OctiGameSelectDialogBase( QWidget* parent,  const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
        setName( "OctiGameSelectDialogBase" );
    resize( 499, 279 ); 
    setCaption( trUtf8( "New OCTI Game" ) );
    setSizeGripEnabled( TRUE );
    OctiGameSelectDialogBaseLayout = new QGridLayout( this, 1, 1, 11, 6, "OctiGameSelectDialogBaseLayout"); 

    Layout1 = new QHBoxLayout( 0, 0, 6, "Layout1"); 

    buttonHelp = new QPushButton( this, "buttonHelp" );
    buttonHelp->setText( trUtf8( "&Help" ) );
    buttonHelp->setAutoDefault( TRUE );
    Layout1->addWidget( buttonHelp );
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout1->addItem( spacer );

    buttonOk = new QPushButton( this, "buttonOk" );
    buttonOk->setText( trUtf8( "&OK" ) );
    buttonOk->setAutoDefault( TRUE );
    buttonOk->setDefault( TRUE );
    Layout1->addWidget( buttonOk );

    buttonCancel = new QPushButton( this, "buttonCancel" );
    buttonCancel->setText( trUtf8( "&Cancel" ) );
    buttonCancel->setAutoDefault( TRUE );
    Layout1->addWidget( buttonCancel );

    OctiGameSelectDialogBaseLayout->addLayout( Layout1, 4, 0 );

    ButtonGroup1 = new QButtonGroup( this, "ButtonGroup1" );
    ButtonGroup1->setTitle( trUtf8( "Game Type" ) );

    fourPlayerButton = new QRadioButton( ButtonGroup1, "fourPlayerButton" );
    fourPlayerButton->setGeometry( QRect( 210, 20, 100, 19 ) ); 
    fourPlayerButton->setText( trUtf8( "Four Players" ) );

    twoPlayerButton = new QRadioButton( ButtonGroup1, "twoPlayerButton" );
    twoPlayerButton->setGeometry( QRect( 50, 20, 100, 19 ) ); 
    twoPlayerButton->setText( trUtf8( "Two Players" ) );
    twoPlayerButton->setChecked( TRUE );

    OctiGameSelectDialogBaseLayout->addWidget( ButtonGroup1, 0, 0 );

    GroupBox3 = new QGroupBox( this, "GroupBox3" );
    GroupBox3->setTitle( trUtf8( "Time Limit" ) );

    useTimeLimitCheckBox = new QCheckBox( GroupBox3, "useTimeLimitCheckBox" );
    useTimeLimitCheckBox->setGeometry( QRect( 50, 20, 110, 19 ) ); 
    useTimeLimitCheckBox->setText( trUtf8( "Use Time Limit" ) );
    useTimeLimitCheckBox->setChecked( FALSE );

    timeLimitComboBox = new QComboBox( FALSE, GroupBox3, "timeLimitComboBox" );
    timeLimitComboBox->setEnabled( FALSE );
    timeLimitComboBox->setGeometry( QRect( 210, 20, 120, 20 ) ); 

    OctiGameSelectDialogBaseLayout->addWidget( GroupBox3, 3, 0 );

    ButtonGroup2 = new QButtonGroup( this, "ButtonGroup2" );
    ButtonGroup2->setTitle( trUtf8( "Capture Bases" ) );

    threeBaseGameButton = new QRadioButton( ButtonGroup2, "threeBaseGameButton" );
    threeBaseGameButton->setGeometry( QRect( 210, 20, 100, 19 ) ); 
    threeBaseGameButton->setText( trUtf8( "3 Base Game" ) );
    threeBaseGameButton->setChecked( TRUE );

    oneBaseGameButton = new QRadioButton( ButtonGroup2, "oneBaseGameButton" );
    oneBaseGameButton->setGeometry( QRect( 50, 20, 100, 19 ) ); 
    oneBaseGameButton->setText( trUtf8( "1 Base Game" ) );

    OctiGameSelectDialogBaseLayout->addWidget( ButtonGroup2, 1, 0 );

    GroupBox1 = new QGroupBox( this, "GroupBox1" );
    GroupBox1->setTitle( trUtf8( "Options" ) );

    edgelessCheckBox = new QCheckBox( GroupBox1, "edgelessCheckBox" );
    edgelessCheckBox->setGeometry( QRect( 210, 20, 100, 19 ) ); 
    edgelessCheckBox->setText( trUtf8( "Edgeless" ) );

    superProngCheckBox = new QCheckBox( GroupBox1, "superProngCheckBox" );
    superProngCheckBox->setGeometry( QRect( 50, 20, 120, 19 ) ); 
    superProngCheckBox->setText( trUtf8( "Use Super Prong" ) );

    OctiGameSelectDialogBaseLayout->addWidget( GroupBox1, 2, 0 );

    // signals and slots connections
    connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
    connect( useTimeLimitCheckBox, SIGNAL( toggled(bool) ), timeLimitComboBox, SLOT( setEnabled(bool) ) );
}

/*  
 *  Destroys the object and frees any allocated resources
 */
OctiGameSelectDialogBase::~OctiGameSelectDialogBase()
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
