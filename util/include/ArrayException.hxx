/*************************************************************************
 * $Id: ArrayException.hxx,v 1.3 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created:  1/25/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _ARRAYEXCEPTION_HXX_
#define _ARRAYEXCEPTION_HXX_

#include "Exception.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI ArrayException : public Exception {

  DECLARE_RUNTIME_SERIALIZABLE(ArrayException);

public:

  ArrayException();
  ArrayException(const char *msg);

};

// Inline functions for ArrayException class are below here.

inline ArrayException::ArrayException() : Exception() {

}

inline ArrayException::ArrayException(const char *msg) : Exception(msg) {

}

__END_NAMESPACE(SELFSOFT);

#endif
