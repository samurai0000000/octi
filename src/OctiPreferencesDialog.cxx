/*************************************************************************
 * $Id: OctiPreferencesDialog.cxx,v 1.1 2001/08/09 04:30:27 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 04:30:27 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created: 8/7/01 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2001 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _OCTIAPI_IMPL_

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qtabwidget.h>
#include <qwidget.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include "OctiPreferencesDialog.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

OctiPreferencesDialog::OctiPreferencesDialog(QWidget *parent, const char *name, bool modal, WFlags fl) : OctiPreferencesDialogBase(parent, name, modal, fl) {
  
}

OctiPreferencesDialog::~OctiPreferencesDialog() {

}

__END_NAMESPACE(SELFSOFT);

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
