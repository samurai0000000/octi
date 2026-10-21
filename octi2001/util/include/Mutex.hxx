/*************************************************************************
 * $Id: Mutex.hxx,v 1.6 2001/07/29 07:30:43 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/07/29 07:30:43 $
 * $Locker:  $
 * $Revision: 1.6 $
 *
 * Created:  1/8/99 by Charles Chiou
 * Comments: This is a wrapper class for mutexes. It isuUsed for thread
 *           synchronizations.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _MUTEX_HXX_
#define _MUTEX_HXX_

#include "utilapi.h"
#include "PtrReference.hxx"
#include "ThreadId.hxx"
#include "ThreadException.hxx"

#if defined(__USE_WINTHREAD__)
#include <windows.h>
#elif defined(__USE_POSIXTHREAD__)
#include <pthread.h>
#else
#error ERROR: No thread library specified
#endif  /* __USE_WINTHREAD__ */

__BEGIN_NAMESPACE(SELFSOFT);

class Condition;

class UTILAPI Mutex : public PtrReference {

public:

    Mutex();
    ~Mutex();

    void lock() throw(ThreadException);
    void unlock() throw(ThreadException);

private:

    friend class Condition;

#if defined(__USE_WINTHREAD__)

    HANDLE _mutex;
#elif defined(__USE_POSIXTHREAD__)
    pthread_mutex_t _mutex;
#endif

};

class UTILAPI RecursiveMutex : PtrReference {

public:

    RecursiveMutex();
    ~RecursiveMutex();

    void lock() throw(ThreadException);
    void unlock() throw(ThreadException);

    ThreadId getOwnerId() const;  // Gets the thread ID of the owner

private:

    friend class Condition;

    void lock(int count) throw(ThreadException);

#if defined(__USE_WINTHREAD__)
    HANDLE _critical;            // The critical section mutex
#elif defined(__USE_POSIXTHREAD__)
    pthread_mutex_t _critical;   // The critical section mutex
#endif
    Mutex _safeguard;  // The safeguard mutex to prevent self-locks
    int _count;        // The number of times the mutex has been acquired
    ThreadId _owner;   // The owner of the mutex

};

// Inline functions for class are below here.

inline Mutex::Mutex() {
#if defined(__USE_WINTHREAD__)
    _mutex = CreateMutex(NULL, FALSE, NULL);
#elif defined(__USE_POSIXTHREAD__)
    //pthread_mutexattr_t attr;
    //pthread_mutexattr_init(&attr);
    pthread_mutex_init(&_mutex, NULL);
#endif
}

inline Mutex::~Mutex() {
#if defined(__USE_WINTHREAD__)
    CloseHandle(_mutex);
#elif defined(__USE_POSIXTHREAD__)
    pthread_mutex_destroy(&_mutex);
#endif
}

inline RecursiveMutex::RecursiveMutex() {
    _count = 0;
    _owner = ThreadId();
#if defined(__USE_WINTHREAD__)
    _critical = CreateMutex(NULL, FALSE, NULL);
#elif defined(__USE_POSIXTHREAD__)
    //pthread_mutexattr_t attr;
    //pthread_mutexattr_init(&attr);
    pthread_mutex_init(&_critical, NULL);
#endif
}

inline RecursiveMutex::~RecursiveMutex() {
#if defined(__USE_WINTHREAD__)
    CloseHandle(_critical);
#elif defined(__USE_POSIXTHREAD__)
    pthread_mutex_destroy(&_critical);
#endif
}

inline void RecursiveMutex::lock() throw(ThreadException) {
    lock(1);
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
