#include "OctiPreferencesDialogBase.hxx"

OctiPreferencesDialogBase::OctiPreferencesDialogBase(QWidget *parent, const char *name,
                                                     bool modal, WFlags fl)
    : QDialog(parent, fl)
{
    qt2InitWidget(this, name ? name : "OctiPreferencesDialogBase");
    setModal(modal);
    setWindowTitle(tr("Preferences"));
    setSizeGripEnabled(true);
    resize(480, 320);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(11, 11, 11, 11);
    layout->setSpacing(6);

    tabWidget = new QTabWidget(this);
    QLabel *appearances = new QLabel(tr("Appearance settings from the original Qt 2 dialog\n"
                                        "have not been fully restored yet."), tabWidget);
    appearances->setAlignment(Qt::AlignCenter);
    tabWidget->addTab(appearances, tr("Appearances"));
    QLabel *network = new QLabel(tr("Network settings from the original Qt 2 dialog\n"
                                    "have not been fully restored yet."), tabWidget);
    network->setAlignment(Qt::AlignCenter);
    tabWidget->addTab(network, tr("Network"));
    layout->addWidget(tabWidget);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttonHelp = new QPushButton(tr("&Help"), this);
    buttons->addWidget(buttonHelp);
    buttons->addStretch();
    buttonApply = new QPushButton(tr("&Apply"), this);
    buttons->addWidget(buttonApply);
    buttonOk = new QPushButton(tr("&OK"), this);
    buttonOk->setDefault(true);
    buttons->addWidget(buttonOk);
    buttonCancel = new QPushButton(tr("&Cancel"), this);
    buttons->addWidget(buttonCancel);
    layout->addLayout(buttons);

    connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttonApply, SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

OctiPreferencesDialogBase::~OctiPreferencesDialogBase()
{
}
