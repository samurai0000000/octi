/*************************************************************************
 * $Id: Date.cxx,v 1.10 2000/11/28 10:19:13 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/28 10:19:13 $
 * $Locker:  $
 * $Revision: 1.10 $
 *
 * Created:  11/15/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#ifdef _WIN32
#include <sys/types.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#include "Date.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(Date, BaseObject, 1);

void Date::captureCurrentTime() {
#ifdef _WIN32
    struct _timeb buffer;
    _ftime(&buffer);
    _time = buffer.time;
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    _time = tv.tv_sec;
#endif
}

unsigned long Date::currentTimeMillis() {
    unsigned long t;

#ifdef _WIN32
    struct _timeb buffer;
    _ftime(&buffer);
    t = buffer.time * 1000;
    t += buffer.millitm;
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    t = tv.tv_sec * 1000;
    t += (tv.tv_usec / 1000);
#endif

    return t;
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
