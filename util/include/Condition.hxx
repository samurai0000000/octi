/*************************************************************************
 * $Id: Condition.hxx,v 1.12 2001/07/29 07:30:43 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/07/29 07:30:43 $
 * $Locker:  $
 * $Revision: 1.12 $
 *
 * Created:  1/8/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _CONDITION_HXX_
#define _CONDITION_HXX_

#ifndef _WIN32
#include <sys/time.h>
#endif

#include "utilapi.h"
#include "PtrReference.hxx"
#include "Mutex.hxx"

#if defined(__USE_WINTHREAD__)
#include <windows.h>
#elif defined (__USE_POSIXTHREAD__)
#include <pthread.h>
#else
#error ERROR: No thread library specified
#endif  /* __USE_WINTHREAD__ */

#define synchronized(LOCK, STATEMENT) { Synchronized sync((LOCK)); STATEMENT; }

__BEGIN_NAMESPACE(SELFSOFT);

class Monitor;
class BaseObject;

class UTILAPI Synchronized : public PtrReference {

public:
  
  Synchronized(Mutex &mutex);
  Synchronized(RecursiveMutex &mutex);
  Synchronized(Monitor &monitor, boolean internal = FALSE);
  Synchronized(BaseObject &object);

  ~Synchronized();

private:

  typedef enum _SyncClass {
    MUTEX,
    RECURSIVE_MUTEX,
    MONITOR,
    OBJECT
  } SyncClass;
  
  typedef struct _SyncObject {
    SyncClass sclass;

    union {
      Mutex *mutex;
      RecursiveMutex *rmutex;
      Monitor *monitor;
      BaseObject *object;
    } object;
  } SyncObject;

  SyncObject _sync;

};

class UTILAPI Condition : public PtrReference {

public:
  
  Condition(RecursiveMutex &mutex);
  ~Condition();

  void wait() throw(InterruptedException);
  void wait(long timeout) throw(InterruptedException);
  void signal();
  void broadcast();

private:

  Condition(const Condition &condition);
  Condition &operator=(const Condition &condition);

private:

  RecursiveMutex &_mutex;

#if defined(__USE_WINTHREAD__)
  typedef struct cond_thread_list {
    struct cond_thread_list *next;
    DWORD id;
  } cond_thread_list_t;

  typedef struct {
    HANDLE cond;
    RecursiveMutex list_mutex;
    cond_thread_list_t *list_hd;
    cond_thread_list_t *list_tl;
    cond_thread_list_t *released;
  } cond_t;
  
  static int win_cond_wait(cond_t *cond, RecursiveMutex &mutex);
  static int win_cond_timedwait(cond_t *cond, RecursiveMutex &mutex, long timeout);
  static int win_cond_signal(cond_t *cond);
  static int win_cond_broadcast(cond_t *cond);

  cond_t _cond;

#elif defined(__USE_POSIXTHREAD__)
  pthread_cond_t _cond;
#endif

};

// Inline functions

inline Condition::Condition(RecursiveMutex &mutex) : _mutex(mutex) {
#if defined(__USE_WINTHREAD__)
  _cond.cond = CreateEvent(NULL, TRUE, FALSE, NULL);
  _cond.list_hd = NULL;
  _cond.list_tl = NULL;
  _cond.released = NULL;
#elif defined(__USE_POSIXTHREAD__)
  pthread_cond_init(&_cond, NULL);
#endif
}

inline Condition::~Condition() {
#if defined(__USE_WINTHREAD_)
  CloseHandle(_cond.cond);
#elif defined(__USE_POSIXTHREAD__)
  pthread_cond_destroy(&_cond);
#endif
}

#if defined(__USE_WINTHREAD__)

inline int Condition::win_cond_wait(cond_t *cond, RecursiveMutex &mutex) {
  return Condition::win_cond_timedwait(cond, mutex, INFINITE);
}

#endif

__END_NAMESPACE(SELFSOFT);

#endif
