/*************************************************************************
 * $Id: Error.cxx,v 1.4 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created:  9/10/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <iostream.h>
#include "Error.hxx"

#define MAX_ERRMSG_LEN 1024

__BEGIN_NAMESPACE(SELFSOFT);

static void default_warning_handler(const char *message) {
  cerr << "WARNING: " << message << endl;
}

static void default_fatal_warning_handler(const char *message) {
  cerr << "FATAL: " << message << endl;
  exit(EXIT_FAILURE);
}

static errhndl_fn GS_warnhndl = default_warning_handler;
static errhndl_fn GS_fatalhndl = default_fatal_warning_handler;

void Error::warning (const char *fmt, ...) {
  va_list args;
  char msg[MAX_ERRMSG_LEN];
  
  va_start(args, fmt);
  vsprintf(msg, fmt,  args);
  (*GS_warnhndl) (msg);
}

void Error::fatal (const char *fmt, ...) {
  va_list args;
  char msg[MAX_ERRMSG_LEN];
  
  va_start(args, fmt);
  vsprintf(msg, fmt,  args);
  (*GS_fatalhndl) (msg);
}

errhndl_fn Error::setWarningHandler(errhndl_fn fn) {
  if(fn == NULL) {
    return GS_warnhndl;
  }

  errhndl_fn old = GS_warnhndl;
  GS_warnhndl = fn;
  return old;
}

errhndl_fn Error::setFatalErrorHandler(errhndl_fn fn) {
  if(fn == NULL) {
    return GS_fatalhndl;
  }

  errhndl_fn old = GS_fatalhndl;
  GS_fatalhndl = fn;
  return old;
} 

__END_NAMESPACE(SELFSOFT);
