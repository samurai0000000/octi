/*************************************************************************
 * $Id: Monitor.hxx,v 1.7 2001/07/29 07:30:43 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/07/29 07:30:43 $
 * $Locker:  $
 * $Revision: 1.7 $
 *
 * Created:  10/23/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _MONITOR_HXX_
#define _MONITOR_HXX_

#include "utilapi.h"
#include "PtrReference.hxx"
#include "ThreadException.hxx"

#if defined(__USE_WINTHREAD__)
#include <windows.h>
#elif defined(__USE_POSIXTHREAD__)
#include <pthread.h>
#else
#error ERROR: No thread library specified
#endif /* __USE_WINTHREAD__ */

__BEGIN_NAMESPACE(SELFSOFT);

class Mutex;
class RecursiveMutex;
class Condition;

class UTILAPI Monitor : public PtrReference {

public:

  Monitor();
  ~Monitor();

  void wait() throw(InterruptedException, IllegalMonitorStateException);
  void wait(long timeout) throw(InterruptedException, IllegalMonitorStateException);
  void notify() throw(IllegalMonitorStateException);
  void notifyAll() throw(IllegalMonitorStateException);

private:

  Monitor(const Monitor &monitor);
  Monitor &operator=(const Monitor &monitor);

  void validateMutexOwner(const char *caller)
    throw(IllegalMonitorStateException);

private:

  friend class Synchronized;
  friend class Condition;

  void lock(boolean internal) throw(ThreadException);
  void unlock() throw(ThreadException);

  RecursiveMutex _mutex;
  Condition _cond;
  boolean _internal;

};

__END_NAMESPACE(SELFSOFT);

#endif
