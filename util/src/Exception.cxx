/*************************************************************************
 * $Id: Exception.cxx,v 1.5 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created:  1/12/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include <stdlib.h>
#include <string.h>
#include "Exception.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(Exception, BaseObject, 1);

Exception::Exception() {
    _msg = NULL;
}

Exception::Exception(const char *msg) {
    if(msg == NULL) {
        _msg = NULL;
    } else {
        int len = strlen(msg);
        _msg = (char *) malloc(len + 1);
        memcpy(_msg, msg, len);
        _msg[len] = '\0';
    }
}

Exception::Exception(const Exception &e) {
    if(e._msg == NULL) {
        _msg = NULL;
    } else {
        int len = strlen(e._msg);
        _msg = (char *) malloc(len + 1);
        memcpy(_msg, e._msg, len);
        _msg[len] = '\0';
    }
}

Exception::~Exception() {
    if(_msg) {
        free(_msg);
    }
}

Exception &Exception::operator=(const Exception &e) {
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

const char *Exception::getMessage() const {
    return _msg;
}

ostream &operator<<(ostream &o, const Exception &e) {
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
