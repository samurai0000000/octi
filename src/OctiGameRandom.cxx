/*************************************************************************
 * $Id: OctiGameRandom.cxx,v 1.4 2001/08/09 04:30:56 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 04:30:56 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created: 3/9/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2001 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _OCTIAPI_IMPL_

#include "OctiDefs.hxx"
#include "OctiGame.hxx"
#include "OctiGameImpl.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

struct octiboard_t {

};

void Octi2PlayerGame::randomize() {
  reset();
  start(PLAYER_A);
  move("33+b");
  move("37+e");
  move("33");
  move("37");
  move("33+c");
  move("37+d");
  move("53+a");
  move("57+f");
  move("33b+g");
  move("37d+f");
  move("73+h");
  move("37df-46,37e-36");
  move("53a+f");
  move("57f-35");
  move("33c-43,33bg-44");
  move("36e+c");
  move("53af-54");
  move("35f+d");
}

void Octi4PlayerGame::randomize() {
  // NOT YET IMPLEMENTED
}

__END_NAMESPACE(SELFSOFT);
