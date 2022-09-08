/*************************************************************************
 * $Id: OctiGameSelectDialog.hxx,v 1.1 2001/08/09 04:30:07 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 04:30:07 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created: 7/5/01 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2001 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _OCTIGAMESELECTDIALOG_HXX_
#define _OCTIGAMESELECTDIALOG_HXX_

#include "octiapi.h"
#include "OctiDefs.hxx"
#include "OctiGameSelectDialogBase.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class OCTIAPI OctiGameSelectDialog : public OctiGameSelectDialogBase {

    Q_OBJECT;

public:

    OctiGameSelectDialog(QWidget* parent = 0, const char* name = 0, bool modal = TRUE, WFlags fl = 0);

    ~OctiGameSelectDialog();

    OctiGameMode getGameMode() const;
    OctiGameBases getGameBases() const;
    boolean useSuperProngs() const;
    boolean edgeless() const;
    boolean useTimeLimit() const;
    int getTimeLimit() const;  // In seconds

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
