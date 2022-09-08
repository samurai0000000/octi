/*************************************************************************
 * $Id: Error.hxx,v 1.2 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created:  9/10/99 by Charles Chiou
 * Comments: The default error handler. Usage examples:
 *           Error::warning("(%d): %s", 1001, "empty string");
 *           Error::fatal("(%d): %s", 501, "accessed invalid memory");
 *
 *           The default warning handler prints the error message and
 *           returns.
 *           The default fatal error handler prints the error message and
 *           calls exit(EXIT_FALIURE).
 *
 *           The defualt error handlers can be replaced by calling
 *           Error::setWarningHandler and Error::setFatalErrorHandler. In
 *           both cases, the previous handler function is returned.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _ERROR_HXX_
#define _ERROR_HXX_

#include "utilapi.h"

using namespace std;

__BEGIN_NAMESPACE(SELFSOFT);

// The error handler function prototype.
typedef void (* errhndl_fn)(const char *);

class UTILAPI Error {

public:

  static void warning(const char *fmt, ...);
  static void fatal(const char *fmt, ...);
  static errhndl_fn setWarningHandler(errhndl_fn fn);
  static errhndl_fn setFatalErrorHandler(errhndl_fn fn);

};

__END_NAMESPACE(SELFSOFT);

#endif
