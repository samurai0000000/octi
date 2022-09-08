/*************************************************************************
 * $Id: OctiWidgetOptions.cxx,v 1.2 2001/06/19 00:45:50 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/06/19 00:45:50 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created: 12/8/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _OCTIAPI_IMPL_

#include "OctiWidgetOptions.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

OctiOptionsModel *G_OctiOptions = NULL;

void InitOctiOptionsModel() {
    G_OctiOptions = new OctiOptionsModel();
}

OctiOptionsModel::OctiOptionsModel() {
    _mode = GAME_MODE_2;
    _useSuperProngs = FALSE;
    _useEdgelessBoard = FALSE;
    _isServerEnabled = TRUE;
    _serverListeningPort = 5570;
}

OctiOptionsModel::~OctiOptionsModel() {

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
