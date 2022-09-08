/*************************************************************************
 * $Id: OctiWidgetColors.hxx,v 1.1 2000/12/10 19:41:31 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/12/10 19:41:31 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created: 12/8/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _OCTIWIDGETCOLORS_HXX_
#define _OCTIWIDGETCOLORS_HXX_

#include <qcolor.h>
#include "octiapi.h"

__BEGIN_NAMESPACE(SELFSOFT);

class OCTIAPI OctiWidgetColors : public Root {

public:

    OctiWidgetColors();
    ~OctiWidgetColors();

    QColor &getLineColor();
    QColor &getLabelColor();
    QColor &getSquareColor();
    QColor &getAPodColor();
    QColor &getBPodColor();
    QColor &getCPodColor();
    QColor &getDPodColor();

private:

    QColor _lineColor;
    QColor _labelColor;
    QColor _squareColor;
    QColor _contrastColor;
    QColor _aPodColor;
    QColor _bPodColor;
    QColor _cPodColor;
    QColor _dPodColor;

};

// Inline functions

inline QColor &OctiWidgetColors::getLineColor() {
    return _lineColor;
}

inline QColor &OctiWidgetColors::getLabelColor() {
    return _labelColor;
}

inline QColor &OctiWidgetColors::getSquareColor() {
    return _squareColor;
}

inline QColor &OctiWidgetColors::getAPodColor() {
    return _aPodColor;
}

inline QColor &OctiWidgetColors::getBPodColor() {
    return _bPodColor;
}

inline QColor &OctiWidgetColors::getCPodColor() {
    return _cPodColor;
}

inline QColor &OctiWidgetColors::getDPodColor() {
    return _dPodColor;
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
