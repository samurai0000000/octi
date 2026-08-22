#include "OctiGameSelectDialogBase.hxx"

#include <QButtonGroup>

OctiGameSelectDialogBase::OctiGameSelectDialogBase(QWidget *parent, const char *name,
                                                   bool modal, WFlags fl)
    : QDialog(parent, fl)
{
    qt2InitWidget(this, name ? name : "OctiGameSelectDialogBase");
    setModal(modal);
    setWindowTitle(tr("New OCTI Game"));
    setSizeGripEnabled(true);
    resize(500, 280);

    OctiGameSelectDialogBaseLayout = new QGridLayout(this);
    OctiGameSelectDialogBaseLayout->setContentsMargins(11, 11, 11, 11);
    OctiGameSelectDialogBaseLayout->setSpacing(6);

    ButtonGroup1 = new QGroupBox(tr("Game Type"), this);
    QHBoxLayout *gameTypeLayout = new QHBoxLayout(ButtonGroup1);
    twoPlayerButton = new QRadioButton(tr("Two Players"), ButtonGroup1);
    fourPlayerButton = new QRadioButton(tr("Four Players"), ButtonGroup1);
    twoPlayerButton->setChecked(true);
    gameTypeLayout->addWidget(twoPlayerButton);
    gameTypeLayout->addWidget(fourPlayerButton);
    QButtonGroup *gameTypeGroup = new QButtonGroup(this);
    gameTypeGroup->addButton(twoPlayerButton);
    gameTypeGroup->addButton(fourPlayerButton);
    OctiGameSelectDialogBaseLayout->addWidget(ButtonGroup1, 0, 0);

    ButtonGroup2 = new QGroupBox(tr("Capture Bases"), this);
    QHBoxLayout *basesLayout = new QHBoxLayout(ButtonGroup2);
    oneBaseGameButton = new QRadioButton(tr("1 Base Game"), ButtonGroup2);
    threeBaseGameButton = new QRadioButton(tr("3 Base Game"), ButtonGroup2);
    threeBaseGameButton->setChecked(true);
    basesLayout->addWidget(oneBaseGameButton);
    basesLayout->addWidget(threeBaseGameButton);
    QButtonGroup *basesGroup = new QButtonGroup(this);
    basesGroup->addButton(oneBaseGameButton);
    basesGroup->addButton(threeBaseGameButton);
    OctiGameSelectDialogBaseLayout->addWidget(ButtonGroup2, 1, 0);

    GroupBox1 = new QGroupBox(tr("Options"), this);
    QHBoxLayout *optionsLayout = new QHBoxLayout(GroupBox1);
    superProngCheckBox = new QCheckBox(tr("Use Super Prong"), GroupBox1);
    edgelessCheckBox = new QCheckBox(tr("Edgeless"), GroupBox1);
    optionsLayout->addWidget(superProngCheckBox);
    optionsLayout->addWidget(edgelessCheckBox);
    OctiGameSelectDialogBaseLayout->addWidget(GroupBox1, 2, 0);

    GroupBox3 = new QGroupBox(tr("Time Limit"), this);
    QHBoxLayout *timeLayout = new QHBoxLayout(GroupBox3);
    useTimeLimitCheckBox = new QCheckBox(tr("Use Time Limit"), GroupBox3);
    timeLimitComboBox = new QComboBox(GroupBox3);
    timeLimitComboBox->setEnabled(false);
    timeLayout->addWidget(useTimeLimitCheckBox);
    timeLayout->addWidget(timeLimitComboBox);
    OctiGameSelectDialogBaseLayout->addWidget(GroupBox3, 3, 0);

    Layout1 = new QHBoxLayout();
    buttonHelp = new QPushButton(tr("&Help"), this);
    buttonHelp->setAutoDefault(true);
    Layout1->addWidget(buttonHelp);
    Layout1->addStretch();
    buttonOk = new QPushButton(tr("&OK"), this);
    buttonOk->setAutoDefault(true);
    buttonOk->setDefault(true);
    Layout1->addWidget(buttonOk);
    buttonCancel = new QPushButton(tr("&Cancel"), this);
    buttonCancel->setAutoDefault(true);
    Layout1->addWidget(buttonCancel);
    OctiGameSelectDialogBaseLayout->addLayout(Layout1, 4, 0);

    connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(useTimeLimitCheckBox, SIGNAL(toggled(bool)),
            timeLimitComboBox, SLOT(setEnabled(bool)));
}

OctiGameSelectDialogBase::~OctiGameSelectDialogBase()
{
}
