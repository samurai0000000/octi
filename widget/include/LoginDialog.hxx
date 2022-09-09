/*************************************************************************
 * $Id: LoginDialog.hxx,v 1.2 2001/08/15 01:48:53 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/15 01:48:53 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created: 8/6/01 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2001 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _LOGINDIALOG_HXX_
#define _LOGINDIALOG_HXX_

#include "String.hxx"
#include "widgetapi.h"
#include "LoginDialogBase.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class WIDGETAPI LoginDialog : public LoginDialogBase {

    Q_OBJECT;

public:

    LoginDialog(QWidget *parent = 0, const char *name = 0, bool modal = TRUE, WFlags fl = 0);
    ~LoginDialog();

    const char *getLogin() const;
    const char *getPassword() const;

protected slots:

    void accept();

private:

    String _login;
    String _password;

};

__END_NAMESPACE(SELFSOFT);

#endif

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
