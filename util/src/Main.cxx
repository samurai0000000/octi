/*************************************************************************
 * $Id: Main.cxx,v 1.1 2001/08/24 03:04:44 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:04:44 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created:  8/6/01 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 2001 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include "Error.hxx"
#include "Exception.hxx"
#include "Main.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_DISCOVERABLE(Main, BaseObject);

boolean Main::G_started = FALSE;

Main *Main::G_impl = NULL;

int Main::start(int argc, char **argv) {
  int retcode;

  if(G_started) {
    Error::fatal("Main::start() entered twice");
  } else {
    G_started = TRUE;
  }
  
  if(G_impl != NULL) {
    try {
      retcode = G_impl->main(argc, argv);
    } catch(Exception e) {
      Error::fatal("uncaught exception in Main: %s", e.getMessage());
      retcode = 1;
    } catch(...) {
      Error::fatal("exception in Main: unknown");
      retcode = 2;
    }
  }

  return retcode;
}

Main::Main() {
  if(G_impl == NULL) {
    // Initialize for the first time
  }

  if(G_started) {
    Error::fatal("attempt to create another instance of subclass of Main\n"
		 "while one is already executing");
  }
  
  if(G_impl != NULL) {
    Error::warning("more than once instance of subclass of Main created\n"
		   "registering this one for start-up, previous instances\n"
		   "will be ignored!\n");
  }

  G_impl = this;
}

Main::~Main() {
  if(G_impl == this) {
    // Cleanup system resources
  }
}

__END_NAMESPACE(SELFSOFT);
