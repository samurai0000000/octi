/*************************************************************************
 * $Id: StringTokenizer.hxx,v 1.2 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created:  9/5/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _STRINGTOKENIZER_HXX_
#define _STRINGTOKENIZER_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"

#define DEFAULT_DELIMETERS " \t\n"  // White-spaces as default delimeters.
#define MAX_TOKEN_BUFFER   2048

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI StringTokenizer : public BaseObject {

  DECLARE_RUNTIME_SERIALIZABLE(StringTokenizer);

public:

  StringTokenizer(const char *s = NULL,
		  const char *delims = DEFAULT_DELIMETERS);
  ~StringTokenizer();

  void setString(const char *s);
  void setDelimeters(const char *delims = DEFAULT_DELIMETERS);

  const char *getString() const;
  const char *getDelimeters() const;

  const char *nextToken();
  int countTokens() const;
  void reset();

private:

  boolean isDelimeter(char c) const;

private:

  char *_string;
  char *_delims;
  char _token[MAX_TOKEN_BUFFER];

  int _prevIdx;
  int _curIdx;

};

// Inline functions.

inline StringTokenizer::StringTokenizer(const char *s, const char *delims) {
  _string = _delims = NULL;
  setDelimeters(delims);
  setString(s);
}

inline const char *StringTokenizer::getString() const {
  return _string;
}

inline const char *StringTokenizer::getDelimeters() const {
  return _delims;
}

__END_NAMESPACE(SELFSOFT);

#endif
