/*************************************************************************
 * $Id: StringTokenizer.cxx,v 1.5 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created:  9/5/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include <string.h>
#include "StringTokenizer.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(StringTokenizer, BaseObject, 1);

StringTokenizer::~StringTokenizer() {
  if(_string != NULL) {
    delete [] _string;
  }

  if(_delims != NULL) {
    delete [] _delims;
  }
}

void StringTokenizer::setString(const char *s) {
  if(_string != NULL) {
    delete [] _string;
  }

  if(s != NULL) {
    _string = new char[strlen(s) + 1];
    strcpy(_string, s);
  } else {
    _string = NULL;
  }

  reset();
}

void StringTokenizer::setDelimeters(const char *delims) {
  if(_delims != NULL) {
    delete [] _delims;
  }

  if(delims != NULL) {
    _delims = new char[strlen(delims) + 1];
    strcpy(_delims, delims);
  } else {
    _delims = NULL;
  }
}

const char *StringTokenizer::nextToken() {
  if(_string == NULL) {
    return NULL;
  }

  // Scan to make sure that _curIdx is pointing to a non-delimeter.
  int scan;
  for(scan = _curIdx; isDelimeter(_string[scan]) && _string[scan] != '\0';
      scan++);
  if(scan != _curIdx) {
    _curIdx = scan;
  }

  // Scan until a delimeter or NULL is encountered.
  for(_prevIdx = _curIdx;
      !isDelimeter(_string[_curIdx]) && _string[_curIdx] != '\0';
      _curIdx++);
  
  if(_curIdx == _prevIdx) {
    return NULL;
  }

  // Set the token
  int tokenLen = _curIdx - _prevIdx;
  strncpy(_token, _string + _prevIdx, tokenLen);
  _token[tokenLen] = '\0';

  // Advance the _curIdx till it points to a non-delimeter.
  for(; isDelimeter(_string[_curIdx]) && _string[_curIdx] != '\0'; _curIdx++);

  return _token;
}

int StringTokenizer::countTokens() const {
  enum TOK_STATE {
    NON_DELIM,
    DELIM
  };

  int numTokens = 0;
  int idx = 0;
  TOK_STATE state = NON_DELIM;

  while(_string[idx] != '\0') {
    switch(state) {
    case NON_DELIM:
      for( ; !isDelimeter(_string[idx]) && _string[idx] != '\0'; idx++);
      state = DELIM;
      break;
    case DELIM:
      for( ; isDelimeter(_string[idx]) && _string[idx] != '\0'; idx++);
      state = NON_DELIM;
      break;
    }

    if(state == DELIM) {
      numTokens++;
    }
  }

  return numTokens;
}

void StringTokenizer::reset() {
  _prevIdx = _curIdx = 0;
}

boolean StringTokenizer::isDelimeter(char c) const {
  if(_delims == NULL) {
    return FALSE;
  }

  for(int i = 0; _delims[i] != '\0'; i++) {
    if(c == _delims[i]) {
      return TRUE;
    }
  }

  return FALSE;
}

__END_NAMESPACE(SELFSOFT);
