/*************************************************************************
 * $Id: Barrier.hxx,v 1.2 2000/12/03 00:16:07 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/12/03 00:16:07 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created:  11/3/00 by Charles Chiou
 * Comments: Barrier synchronization.
 * Status:
 * Bugs:
 *
 * Work done as part of courses taken in Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _BARRIER_HXX_
#define _BARRIER_HXX_

#include "utilapi.h"
#include "PtrReference.hxx"
#include "Mutex.hxx"
#include "Condition.hxx"

#if defined(__USE_WINTHREAD__)
#include <windows.h>
#elif defined(__USE_POSIXTHREAD__)
#include <pthread.h>
#else
#error ERROR: No thread library specified
#endif /* __USE_WINTHREAD__ */

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI Barrier : public PtrReference {

public:

  Barrier(int nthrs);
  ~Barrier();

  void wait();

private:

  int _nthrs;
  int _count;
  Condition _cond;
  Mutex _lock;
  RecursiveMutex _mutex;

};

// Inline functions

inline Barrier::Barrier(int nthrs) : _cond(_mutex) {
  _nthrs = nthrs;
  _count = 0;
}

inline Barrier::~Barrier() {

}

__END_NAMESPACE(SELFSOFT);

#endif
