/*************************************************************************
 * $Id: GameExceptions.hxx,v 1.2 2000/11/18 10:25:09 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/18 10:25:09 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created:  11/13/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _GAMEEXCEPTIONS_HXX_
#define _GAMEEXCEPTIONS_HXX_

#include "gameapi.h"
#include "BaseObject.hxx"
#include "Exception.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class GAMEAPI IllegalMoveSyntaxException : public Exception {

  DECLARE_RUNTIME_SERIALIZABLE(IllegalMoveSyntaxException);

public:

  IllegalMoveSyntaxException();
  IllegalMoveSyntaxException(const char *msg);

};

class GAMEAPI IllegalMoveException : public Exception {

  DECLARE_RUNTIME_SERIALIZABLE(IllegalMoveException);

public:

  IllegalMoveException();
  IllegalMoveException(const char *msg);

};


class GAMEAPI GameSessionException : public Exception {

  DECLARE_RUNTIME_SERIALIZABLE(GameSessionException);

public:

  GameSessionException();
  GameSessionException(const char *msg);

};

// Inline functions

inline IllegalMoveSyntaxException::IllegalMoveSyntaxException()
  : Exception() {

}

inline IllegalMoveSyntaxException::IllegalMoveSyntaxException(const char *msg)
  : Exception(msg) {

}

inline IllegalMoveException::IllegalMoveException()
  : Exception()  {

}

inline IllegalMoveException::IllegalMoveException(const char *msg)
  : Exception(msg)  {

}

inline GameSessionException::GameSessionException()
  : Exception() {

}

inline GameSessionException::GameSessionException(const char *msg)
  : Exception(msg) {

}

__END_NAMESPACE(SELFSOFT);

#endif
