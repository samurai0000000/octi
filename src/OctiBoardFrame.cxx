/*************************************************************************
 * $Id: OctiBoardFrame.cxx,v 1.5 2001/08/09 04:30:56 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 04:30:56 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created: 11/19/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _OCTIAPI_IMPL_

#include <qlayout.h>
#include "OctiGame.hxx"
#include "OctiBoardFrame.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

OctiBoardFrame::OctiBoardFrame(QWidget *parent, const char *name, WFlags f)
  : QFrame(parent, name, f) {
  _bw = new OctiBoardWidget(this);

  QVBoxLayout *vlayout1 = new QVBoxLayout(this);
  vlayout1->addWidget(_bw);
  setFrameStyle(StyledPanel | Sunken);
}

OctiBoardFrame::~OctiBoardFrame() {

}

__END_NAMESPACE(SELFSOFT);

