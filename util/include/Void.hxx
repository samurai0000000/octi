/*************************************************************************
 * $Id: Void.hxx,v 1.3 2000/12/03 00:16:07 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/12/03 00:16:07 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created:  2/27/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _VOID_HXX_
#define _VOID_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI Void : public BaseObject {

  DECLARE_RUNTIME_DISCOVERABLE(Void);

public:

  Void();
  Void(void *ptr);
  ~Void();

  void *getPointer();
  void setPointer(void *ptr);

  operator void *();

  void *operator=(const Void &v);
  void *operator=(void *ptr);
  boolean operator==(const Void &v) const;
  boolean operator==(void *ptr) const;

private:

  void *_ptr;

};

// Inline functions

inline Void::Void() {
  _ptr = NULL;
}

inline Void::Void(void *ptr) {
  _ptr = ptr;
}

inline Void::~Void() {
  _ptr = NULL;
}

inline void *Void::getPointer() {
  return _ptr;
}

inline void Void::setPointer(void *ptr) {
  _ptr = ptr;
}

inline Void::operator void *() {
  return _ptr;
}

inline void *Void::operator=(const Void &v) {
  return (_ptr = v._ptr);
}

inline void *Void::operator=(void *ptr) {
  return (_ptr = ptr);
}

inline boolean Void::operator==(const Void &v) const {
  return _ptr == v._ptr;
}

inline boolean Void::operator==(void *ptr) const {
  return _ptr == ptr;
}

__END_NAMESPACE(SELFSOFT);

#endif
