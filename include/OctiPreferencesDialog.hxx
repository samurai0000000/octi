/*************************************************************************
 * $Id: OctiPreferencesDialog.hxx,v 1.1 2001/08/09 04:30:07 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 04:30:07 $
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

#ifndef __OCTIPREFERENCESDIALOG_HXX_
#define __OCTIPREFERENCESDIALOG_HXX_

#include "octiapi.h"
#include "OctiDefs.hxx"
#include "OctiPreferencesDialogBase.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class OCTIAPI OctiPreferencesDialog : public OctiPreferencesDialogBase { 
  Q_OBJECT;

public:

    OctiPreferencesDialog(QWidget *parent = 0, const char *name = 0, bool modal = TRUE, WFlags fl = 0);
    ~OctiPreferencesDialog();

};

__END_NAMESPACE(SELFSOFT);

#endif
