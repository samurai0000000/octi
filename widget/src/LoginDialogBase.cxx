#include "LoginDialogBase.hxx"

LoginDialogBase::LoginDialogBase(QWidget *parent, const char *name,
                                 bool modal, WFlags fl)
    : QDialog(parent, fl)
{
    qt2InitWidget(this, name ? name : "LoginDialogBase");
    setModal(modal);
    setWindowTitle(tr("Login"));
    setSizeGripEnabled(true);
    resize(380, 180);

    QGridLayout *layout = new QGridLayout(this);
    layout->setContentsMargins(11, 11, 11, 11);
    layout->setSpacing(6);

    QLabel *loginLabel = new QLabel(tr("&Login:"), this);
    loginLineEdit = new QLineEdit(this);
    loginLabel->setBuddy(loginLineEdit);
    layout->addWidget(loginLabel, 0, 0);
    layout->addWidget(loginLineEdit, 0, 1);

    QLabel *passwordLabel = new QLabel(tr("&Password:"), this);
    passwordLineEdit = new QLineEdit(this);
    passwordLineEdit->setEchoMode(QLineEdit::Password);
    passwordLabel->setBuddy(passwordLineEdit);
    layout->addWidget(passwordLabel, 1, 0);
    layout->addWidget(passwordLineEdit, 1, 1);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttonHelp = new QPushButton(tr("&Help"), this);
    buttons->addWidget(buttonHelp);
    buttons->addStretch();
    buttonOk = new QPushButton(tr("&OK"), this);
    buttonOk->setDefault(true);
    buttons->addWidget(buttonOk);
    buttonCancel = new QPushButton(tr("&Cancel"), this);
    buttons->addWidget(buttonCancel);
    layout->addLayout(buttons, 2, 0, 1, 2);

    connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
}

LoginDialogBase::~LoginDialogBase()
{
}
