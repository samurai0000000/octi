/*************************************************************************
 * $Id: OctiWidgetColors.cxx,v 1.2 2000/12/23 05:29:16 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/12/23 05:29:16 $
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

#include "OctiWidgetColors.hxx"

#define DEFAULT_WC_LINE       "black"
#define DEFAULT_WC_LABEL      "white"
#define DEFAULT_WC_SQUARE     "orange"
#define DEFAULT_WC_CONTRAST   "white"
#define DEFAULT_WC_APOD       "red"
#define DEFAULT_WC_BPOD       "blue"
#define DEFAULT_WC_CPOD       "magenta"
#define DEFAULT_WC_DPOD       "green"

__BEGIN_NAMESPACE(SELFSOFT);

OctiWidgetColors::OctiWidgetColors() :
  _lineColor(DEFAULT_WC_LINE),
  _labelColor(DEFAULT_WC_LABEL),
  _squareColor(DEFAULT_WC_SQUARE),
  _contrastColor(DEFAULT_WC_CONTRAST),
  _aPodColor(DEFAULT_WC_APOD),
  _bPodColor(DEFAULT_WC_BPOD),
  _cPodColor(DEFAULT_WC_CPOD),
  _dPodColor(DEFAULT_WC_DPOD) {

}

OctiWidgetColors::~OctiWidgetColors() {

}

__END_NAMESPACE(SELFSOFT);

