/*************************************************************************
 * $Id: ThreadId.hxx,v 1.3 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created:  2/29/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _THREADID_HXX_
#define _THREADID_HXX_

#include <iostream.h>
#include "utilapi.h"
#include "PtrReference.hxx"

#if defined(__USE_WINTHREAD__)
#include <windows.h>
#elif defined(__USE_POSIXTHREAD__)
#include <pthread.h>
#else
#error ERROR: No thread library specidied
#endif  /* __USE_WINTHREAD_ */

__BEGIN_NAMESPACE(SELFSOFT);

class Thread;

class UTILAPI ThreadId : public PtrReference {

public:

  ThreadId();

#if defined(__USE_WINTHREAD__)
  ThreadId(DWORD threadId, HANDLE handle);
#elif defined(__USE_POSIXTHREAD__)
  ThreadId(pthread_t id);
#endif

  ~ThreadId();

  boolean operator!=(const ThreadId &id) const;
  boolean operator==(const ThreadId &id) const;

  
  static ThreadId self();

#if defined(__USE_WINTHREAD__)
  operator DWORD () const;
#elif defined(__USE_POSIXTHREAD__)
  operator pthread_t () const;
#endif
  
private:

  friend class Thread;

#if defined(__USE_WINTHREAD__)
  HANDLE _handle;
  DWORD _id;
#elif defined(__USE_POSIXTHREAD__)
  pthread_t _id;
  boolean _nullId;
#endif

};

// Inline functions

inline ThreadId::ThreadId() {

}

inline ThreadId::~ThreadId() {

}

#if defined(__USE_WINTHREAD__)

inline ThreadId::ThreadId(DWORD id, HANDLE handle) {
  _id  = id;
  _handle = handle;
}

inline boolean ThreadId::operator==(const ThreadId &id) const {
  return _id == id._id;
}
    
inline boolean ThreadId::operator!=(const ThreadId &id) const {
  return !(*this == id);
}

inline ThreadId::operator DWORD() const {
  return _id;
}

inline ThreadId ThreadId::self() {
  return ThreadId(GetCurrentThreadId(), GetCurrentThread());
}

#elif defined(__USE_POSIXTHREAD__)

inline ThreadId::ThreadId(pthread_t id) {
  _id = id;
  _nullId = FALSE;
}

inline boolean ThreadId::operator!=(const ThreadId &id) const {
  return !(*this == id);
}

inline boolean ThreadId::operator==(const ThreadId &id) const {
  if(_nullId == FALSE && id._nullId == FALSE) {
    return pthread_equal(_id, id._id);
  } else if (_nullId == TRUE && id._nullId == TRUE) {
    return TRUE;
  } 

  return FALSE;
}

inline ThreadId ThreadId::self() {
  return ThreadId(pthread_self());
}

inline ThreadId::operator pthread_t() const {
  return _id;
}

#endif

__END_NAMESPACE(SELFSOFT);

#endif
