/*************************************************************************
 * $Id: Boolean.hxx,v 1.2 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created:  1/16/99 by Charles Chiou
 * Comments:
 *     A boolean class: 'unsigned char' is used to store the boolean value.
 *     Use "boolean" as the primitive boolean type.
 *     Use the class Boolean as a wrapper for boolean operations.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _BOOLEAN_HXX_
#define _BOOLEAN_HXX_

#include <iostream>
#include "utilapi.h"
#include "BaseObject.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class Boolean;

extern UTILAPI const char *TrueFalseStr(boolean b);
extern UTILAPI const char *TrueFalseStr(const Boolean &b);
extern UTILAPI const char *YesNoStr(boolean b);
extern UTILAPI const char *YesNoStr(const Boolean &b);
extern UTILAPI const char *OneZeroStr(boolean b);
extern UTILAPI const char *OneZeroStr(const Boolean &b);
extern UTILAPI const char *OnOffStr(boolean b);
extern UTILAPI const char *OnOffStr(const Boolean &b);
extern UTILAPI boolean IsTrueFalse(const char *s);

class UTILAPI Boolean : public BaseObject {

  DECLARE_RUNTIME_SERIALIZABLE(Boolean);

public:

  Boolean();  // Default constructor (value = FALSE).
  Boolean(boolean b);
  ~Boolean();

  // Straight assignment.
  boolean value() const;
  void value(boolean b);
  Boolean &operator=(const Boolean &b);
  Boolean &operator=(boolean b);

  // Logical and bitwise AND.
  boolean operator&&(const Boolean &b) const;
  boolean operator&&(boolean b) const;
  boolean operator&(const Boolean &b) const;
  boolean operator&(boolean b) const;
  Boolean &operator&=(const Boolean &b);
  Boolean &operator&=(boolean b);

  // Logical and bitwise OR.
  boolean operator||(const Boolean &b) const;
  boolean operator||(boolean b) const;
  boolean operator|(const Boolean &b) const;
  boolean operator|(boolean b) const;
  Boolean &operator|=(const Boolean &b);
  Boolean &operator|=(boolean b);

  // Logical XOR.
  boolean operator^(const Boolean &b) const;
  boolean operator^(boolean b) const;
  Boolean &operator^=(const Boolean &b);
  Boolean &operator^=(boolean b);

  // Comparison
  boolean operator==(const Boolean &b) const;
  boolean operator==(boolean b) const;
  boolean operator!=(const Boolean &b) const;
  boolean operator!=(boolean b) const;

  // Logical NOT.
  boolean operator!() const;

  friend ostream &operator<<(ostream &os, const Boolean &b);

private:

  boolean _b;

};

// Inline functions for class are below here.

inline Boolean::Boolean() {
  _b = FALSE;
}

inline Boolean::Boolean(boolean b) {
  _b = (b == TRUE);
}

inline Boolean::~Boolean() {
}

inline boolean Boolean::value() const {
  return _b;
}

inline void Boolean::value(boolean b) {
  _b = (b == TRUE);
}

inline Boolean &Boolean::operator=(const Boolean &b) {
  _b = b.value();
  return *this;
}

inline Boolean &Boolean::operator=(boolean b) {
  _b = (b == TRUE);
  return *this;
}

inline boolean Boolean::operator&&(const Boolean &b) const {
  return _b && b.value();
}

inline boolean Boolean::operator&&(boolean b) const {
  return _b && b;
}

inline boolean Boolean::operator&(const Boolean &b) const {
  return _b & b.value();
}

inline boolean Boolean::operator&(boolean b) const {
  return _b & b;
}

inline Boolean &Boolean::operator&=(const Boolean &b) {
  _b &= b.value();
  return *this;
}

inline Boolean &Boolean::operator&=(boolean b) {
  _b &= b;
  return *this;
}

inline boolean Boolean::operator||(const Boolean &b) const {
  return _b || b.value();
}

inline boolean Boolean::operator||(boolean b) const {
  return _b || b;
}
inline boolean Boolean::operator|(const Boolean &b) const {
  return _b | b.value();
}

inline boolean Boolean::operator|(boolean b) const {
  return _b | b;
}

inline Boolean &Boolean::operator|=(const Boolean &b) {
  _b |= b.value();
  return *this;
}

inline Boolean &Boolean::operator|=(boolean b) {
  _b |= b;
  return *this;
}

inline boolean Boolean::operator^(const Boolean &b) const {
  return _b ^ b.value();
}

inline boolean Boolean::operator^(boolean b) const {
  return _b ^ b;
}

inline Boolean &Boolean::operator^=(const Boolean &b) {
  _b ^= b.value();
  return *this;
}

inline Boolean &Boolean::operator^=(boolean b) {
  _b ^= b;
  return *this;
}

inline boolean Boolean::operator==(const Boolean &b) const {
  return _b == b.value();
}

inline boolean Boolean::operator==(boolean b) const {
  return _b == b;
}

inline boolean Boolean::operator!=(const Boolean &b) const {
  return _b != b.value();
}

inline boolean Boolean::operator!=(boolean b) const {
  return _b != b;
}

inline boolean Boolean::operator!() const {
  return !_b;
}

inline ostream &operator<<(ostream &o, const Boolean &b) {
  return o << (b.value() ? "TRUE" : "FALSE");
}

__END_NAMESPACE(SELFSOFT);

#endif
