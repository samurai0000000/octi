/*************************************************************************
 * $Id: ThreadException.hxx,v 1.3 2001/07/29 07:30:43 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/07/29 07:30:43 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created:  3/2/00 by Charles Chiou
 * Comments:
 * Status:  Changed the inheritance hierarchy.
 *          GCC-3.0 has trouble with forward class declarations that
 *          has classes undefined. It became more strict and does not
 *          allow undefined classes in the throw(...) body, which means
 *          the exceptions thrown in BaseObject must not inherit from
 *          the generic Exception class which is a subclass of BaseObject
 *          itself!
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _THREADEXCEPTION_HXX_
#define _THREADEXCEPTION_HXX_

#include <iostream.h>
#include "utilapi.h"
#include "PtrReference.hxx"
#include "BaseRuntimeClass.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI ThreadException {

public:
  
  ThreadException();
  ThreadException(const char *msg);
  ThreadException(const ThreadException &e);
  virtual ~ThreadException();
  
  ThreadException &operator=(const ThreadException &e);
  const char *getMessage() const;
  friend ostream &operator<<(ostream &out, const ThreadException &e);
  
private:
  
  char *_msg;

};

class UTILAPI InterruptedException : public ThreadException {
  
public:
  
  InterruptedException();
  InterruptedException(const char *msg);

};

class UTILAPI IllegalMonitorStateException : public ThreadException {
  
public:
  
  IllegalMonitorStateException();
  IllegalMonitorStateException(const char *msg);

};

class UTILAPI IllegalThreadStateException : public ThreadException {

public:

  IllegalThreadStateException();
  IllegalThreadStateException(const char *msg);

};

class UTILAPI ThreadDeathException : public ThreadException {

public:

  ThreadDeathException();
  ThreadDeathException(const char *msg);

};

// Inline functions

inline InterruptedException::InterruptedException() : ThreadException() {

}

inline InterruptedException::InterruptedException(const char *msg) : ThreadException(msg) {

}

inline IllegalMonitorStateException::IllegalMonitorStateException() : ThreadException() {

}

inline IllegalMonitorStateException::IllegalMonitorStateException(const char *msg) : ThreadException(msg) {

}

inline IllegalThreadStateException::IllegalThreadStateException() : ThreadException() {

}

inline IllegalThreadStateException::IllegalThreadStateException(const char *msg) : ThreadException(msg) {

}

inline ThreadDeathException::ThreadDeathException() : ThreadException() {

}

inline ThreadDeathException::ThreadDeathException(const char *msg) : ThreadException(msg) {

}

__END_NAMESPACE(SELFSOFT);

#endif
