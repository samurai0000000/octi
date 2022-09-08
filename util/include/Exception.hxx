/*************************************************************************
 * $Id: Exception.hxx,v 1.3 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created:  1/16/99 by Charles Chiou
 * Comments: This is the base class for all exceptions.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _EXCEPTION_HXX_
#define _EXCEPTION_HXX_

#include <iostream>
#include "utilapi.h"
#include "BaseObject.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI Exception : public BaseObject {

  DECLARE_RUNTIME_SERIALIZABLE(Exception);

public:

  Exception();
  Exception(const char *msg);
  Exception(const Exception &e);
  virtual ~Exception();

  Exception &operator=(const Exception &e);
  const char *getMessage() const;
  friend ostream &operator<<(ostream &out, const Exception &e);

private:

  char *_msg;

};

__END_NAMESPACE(SELFSOFT);

#endif
