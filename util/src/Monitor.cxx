/*************************************************************************
 * $Id: Monitor.cxx,v 1.6 2001/07/29 07:30:45 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/07/29 07:30:45 $
 * $Locker:  $
 * $Revision: 1.6 $
 *
 * Created:  10/23/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include "Mutex.hxx"
#include "Condition.hxx"
#include "Monitor.hxx"
#include "String.hxx"
#include "ThreadException.hxx"
#include "Thread.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

Monitor::Monitor() : _cond(_mutex), _internal(FALSE) {

}

Monitor::~Monitor() {

}

void Monitor::validateMutexOwner(const char *caller)
  throw(IllegalMonitorStateException) {
  if(_mutex.getOwnerId() != ThreadId::self()) {
    String msg = "In Monitor::";
    msg += caller;
    msg += ", thread does not own mutex";
    throw IllegalMonitorStateException(msg);
  }
}

void Monitor::wait() throw(InterruptedException, IllegalMonitorStateException) {
  Thread::checkRunningStatusStatic();
  validateMutexOwner("wait()");

  Thread::setMonitorStatic(this);
  try {
    _cond.wait();
  } catch(...) {
    Thread::setMonitorStatic(NULL);
    Thread::checkRunningStatusStatic();
    throw;
  }

  Thread::setMonitorStatic(NULL);
  Thread::checkRunningStatusStatic();
}

void Monitor::wait(long timeout) throw(InterruptedException, IllegalMonitorStateException) {
  Thread::checkRunningStatusStatic();
  validateMutexOwner("wait(timeout)");
  
  Thread::setMonitorStatic(this);
  try {
    _cond.wait(timeout);
  } catch(...) {
    Thread::setMonitorStatic(NULL);
    Thread::checkRunningStatusStatic();
    throw;
  }
  
  Thread::setMonitorStatic(NULL);
  Thread::checkRunningStatusStatic();
}

void Monitor::notify() throw(IllegalMonitorStateException) {
  validateMutexOwner("notify()");
  _cond.signal();
}

void Monitor::notifyAll() throw(IllegalMonitorStateException) {
  validateMutexOwner("notifyAll()");
  _cond.broadcast();
}

void Monitor::lock(boolean internal) throw(ThreadException) {
#if defined(__USE_THREAD_FORCESTOP__)
  if(!internal) {
    Thread::checkRunningStatusStatic();
  }
  _internal = internal;
#endif

  _mutex.lock();
}

void Monitor::unlock() throw(ThreadException) {
#if defined(__USE_THREAD_FORCESTOP__)
  boolean internal = _internal;
#endif

  _mutex.unlock();

#if defined(__USE_THREAD_FORCESTOP__)
  if(!internal) {
    Thread::checkRunningStatusStatic();
  }
#endif
}

__END_NAMESPACE(SELFSOFT);
