/*************************************************************************
 * $Id: LoginDialog.cxx,v 1.1 2001/08/09 04:06:56 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 04:06:56 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created: 8/6/01 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2001 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _WIDGETAPI_IMPL_

#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include "LoginDialog.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

LoginDialog::LoginDialog(QWidget *parent, const char *name, bool modal, WFlags fl) : LoginDialogBase(parent, name, modal, fl) {

}

LoginDialog::~LoginDialog() {

}

const char *LoginDialog::getLogin() const {
  return _login;
}

const char *LoginDialog::getPassword() const {
  return _password;
}

void LoginDialog::accept() {
  _login = loginLineEdit->text();
  _password = passwordLineEdit->text();
  QDialog::accept();
}

__END_NAMESPACE(SELFSOFT);
