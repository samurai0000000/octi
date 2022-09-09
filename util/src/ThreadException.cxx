/*************************************************************************
 * $Id: ThreadException.cxx,v 1.3 2001/07/29 07:30:45 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/07/29 07:30:45 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created:  3/2/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include <stdlib.h>
#include <string.h>
#include "ThreadException.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

ThreadException::ThreadException() {
    _msg = NULL;
}

ThreadException::ThreadException(const char *msg) {
    if(msg == NULL) {
        _msg = NULL;
    } else {
        int len = strlen(msg);
        _msg = (char *) malloc(len + 1);
        memcpy(_msg, msg, len);
        _msg[len] = '\0';
    }
}

ThreadException::ThreadException(const ThreadException &e) {
    if(e._msg == NULL) {
        _msg = NULL;
    } else {
        int len = strlen(e._msg);
        _msg = (char *) malloc(len + 1);
        memcpy(_msg, e._msg, len);
        _msg[len] = '\0';
    }
}

ThreadException::~ThreadException() {
    if(_msg) {
        free(_msg);
    }
}

ThreadException &ThreadException::operator=(const ThreadException &e) {
    if(this != &e) {
        if(_msg) {
            free(_msg);
        }

        if(e._msg == NULL) {
            _msg = NULL;
        } else {
            int len = strlen(e._msg);
            _msg = (char *) malloc(len + 1);
            memcpy(_msg, e._msg, len);
            _msg[len] = '\0';
        }
    }

    return *this;
}

const char *ThreadException::getMessage() const {
    return _msg;
}

ostream &operator<<(ostream &o, const ThreadException &e) {
    return o << e._msg;
}

__END_NAMESPACE(SELFSOFT);

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
