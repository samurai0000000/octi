/*************************************************************************
 * $Id: OctiWidgetOptions.hxx,v 1.2 2001/06/19 00:45:46 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/06/19 00:45:46 $
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

#ifndef _OCTIWIDGETOPTIONS_HXX_
#define _OCTIWIDGETOPTIONS_HXX_

#include "OctiDefs.hxx"
#include "OctiWidgetColors.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class OCTIAPI OctiOptionsModel;

extern OctiOptionsModel *G_OctiOptions;
extern void InitOctiOptionsModel();

class OCTIAPI OctiOptionsModel : public Root {

public:

    OctiOptionsModel();
    ~OctiOptionsModel();

    OctiWidgetColors &getWidgetColors();

private:

    OctiGameMode _mode;
    boolean _useSuperProngs;
    boolean _useEdgelessBoard;

    boolean _isServerEnabled;
    int _serverListeningPort;

    OctiWidgetColors _colors;

};

// Inline functions

inline OctiWidgetColors &OctiOptionsModel::getWidgetColors() {
    return _colors;
}

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
