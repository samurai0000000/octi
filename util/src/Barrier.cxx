/*************************************************************************
 * $Id: Barrier.cxx,v 1.1 2000/11/04 18:07:29 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/04 18:07:29 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created:  11/3/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done as part of courses taken in Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include "Barrier.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

void Barrier::wait() {
  _mutex.lock();

  if(++_count >= _nthrs) {
    /* The count of threads reaching the barrier has reached nthrs. */
    /* Wake up all other threads. */
    _cond.broadcast();
    _count = 0;  /* Reset the count to zero. */
  } else {
    /* Block on the condition indifinitely until the last thread to reach */
    /* the barrier to perform a broadcast. */
    _cond.wait();
  }

  _mutex.unlock();
}

__END_NAMESPACE(SELFSOFT);
