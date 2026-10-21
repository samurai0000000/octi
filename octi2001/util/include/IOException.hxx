/*************************************************************************
 * $Id: Exception.hxx,v 1.3 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created:  8/30/01 by Charles Chiou
 * Comments: Basic I/O exceptions.
 * Status:
 * Bugs:
 *
 * Copyright (c) 2001 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _IOEXCEPTION_HXX_
#define _IOEXCEPTION_HXX_

#include <iostream>
#include "utilapi.h"
#include "Exception.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

/**
 * The base class of all IO-related exceptions.
 */
class UTILAPI IOException : public Exception {

    DECLARE_RUNTIME_SERIALIZABLE(IOException);

public:

    IOException();
    IOException(const char *msg);

};

/**
 * File not found.
 */
class UTILAPI FileNotFoundException : public IOException {

    DECLARE_RUNTIME_SERIALIZABLE(FileNotFoundException);

public:

    FileNotFoundException();
    FileNotFoundException(const char *msg);

};

// Inline functions

inline IOException::IOException() : Exception() {

}

inline IOException::IOException(const char *msg) : Exception(msg) {

}

inline FileNotFoundException::FileNotFoundException() : IOException() {

}

inline FileNotFoundException::FileNotFoundException(const char *msg) : IOException(msg) {

}

__END_NAMESPACE(SELFSOFT);

#endif

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
