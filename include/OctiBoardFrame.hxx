/*************************************************************************
 * $Id: OctiBoardFrame.hxx,v 1.4 2001/06/19 00:45:46 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/06/19 00:45:46 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created: 11/19/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _OCTIBOARDFRAME_HXX_
#define _OCTIBOARDFRAME_HXX_

#include <qframe.h>
#include "OctiBoard.hxx"
#include "OctiSquareDispWidget.hxx"
#include "OctiBoardDispWidget.hxx"
#include "OctiStatDispWidget.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class OCTIAPI OctiBoardFrame : public QFrame {

  Q_OBJECT;

public:
  
  OctiBoardFrame(QWidget *parent = NULL, const char *name = NULL, WFlags f = 0);
  ~OctiBoardFrame();

  OctiBoard *getBoard();

private:

  OctiBoardWidget *_bw;

};

// Inline functions

inline OctiBoard *OctiBoardFrame::getBoard() {
  return _bw->getBoard();
}

__END_NAMESPACE(SELFSOFT);

#endif
