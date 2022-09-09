/*************************************************************************
 * $Id: Condition.cxx,v 1.13 2001/08/21 05:14:42 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/21 05:14:42 $
 * $Locker:  $
 * $Revision: 1.13 $
 *
 * Created:  9/10/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include <errno.h>
#include "debug.h"
#include "Error.hxx"
#include "Mutex.hxx"
#include "Condition.hxx"
#include "Monitor.hxx"
#include "BaseObject.hxx"
#include "ThreadException.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

Synchronized::Synchronized(Mutex &mutex) {
    _sync.sclass = MUTEX;
    _sync.object.mutex = &mutex;
    _sync.object.mutex->lock();
}

Synchronized::Synchronized(RecursiveMutex &mutex) {
    _sync.sclass = RECURSIVE_MUTEX;
    _sync.object.rmutex = &mutex;
    _sync.object.rmutex->lock();
}

Synchronized::Synchronized(Monitor &monitor, boolean internal) {
    _sync.sclass = MONITOR;
    _sync.object.monitor = &monitor;
    _sync.object.monitor->lock(internal);
}

Synchronized::Synchronized(BaseObject &object) {
    _sync.sclass = OBJECT;
    _sync.object.object = &object;
    _sync.object.object->ensureMonitorInit();
    _sync.object.object->_monitor->lock(FALSE);
}

Synchronized::~Synchronized() {
    switch(_sync.sclass) {
    case MUTEX:
        _sync.object.mutex->unlock();
        break;
    case RECURSIVE_MUTEX:
        _sync.object.rmutex->unlock();
        break;
    case MONITOR:
        _sync.object.monitor->unlock();
        break;
    case OBJECT:
        _sync.object.object->_monitor->unlock();
        break;
    default:
        Error::fatal("~Synchronized() called wirh unknown class");
    }
}

#if defined(__USE_WINTHREAD__)

int Condition::win_cond_timedwait(cond_t *cond, RecursiveMutex &mutex, long timeout) {
    ASSERT(cond != NULL);

    int ret;
    BOOLEAN is_released;
    struct cond_thread_list el, **el_ptr;

    cond->list_mutex.lock();

    el.id = GetCurrentThreadId();
    el.next = NULL;
    if(cond->list_tl != NULL) {
        cond->list_tl->next = &el;
    } else {
        cond->list_hd = &el;
    }
    cond->list_tl = &el;

    cond->list_mutex.unlock();
    ReleaseMutex(mutex._critical);

    do {
        ret = WaitForSingleObject(cond->cond, timeout);

        cond->list_mutex.lock();

        for(el_ptr = &cond->released; *el_ptr != NULL;
            el_ptr = &(*el_ptr)->next) {
            if(*el_ptr == &el) {
                break;
            }
        }

        if(*el_ptr != NULL) {
            *el_ptr = el.next;
            is_released = TRUE;
        } else if(*el_ptr == NULL && ret == WAIT_TIMEOUT) {
            for(el_ptr = &cond->list_hd; *el_ptr != NULL;
                el_ptr = &(*el_ptr)->next) {
                if((*el_ptr) != NULL && (*el_ptr)->next == &el) {
                    break;
                }
            }

            if(*el_ptr != NULL) {
                (*el_ptr)->next = el.next;
            } else {
                cond->list_hd = el.next;
            }

            if(cond->list_tl == &el) {
                cond->list_tl = *el_ptr;
            }

            is_released = TRUE;
        } else {
            is_released = FALSE;
        }

        if(cond->released == NULL) {
            ResetEvent(cond->cond);
        }

        cond->list_mutex.unlock();

    } while(!is_released);

    WaitForSingleObject(mutex._critical, INFINITE);

    return ret;
}

int Condition::win_cond_signal(cond_t *cond) {
    ASSERT(cond != NULL);

    struct cond_thread_list *el;

    cond->list_mutex.lock();
    if((el = cond->list_hd) != NULL) {
        if((cond->list_hd = cond->list_hd->next) == NULL) {
            cond->list_tl = NULL;
        }
        el->next = cond->released;
        cond->released = el;
        SetEvent(cond->cond);
    }
    cond->list_mutex.unlock();

    return 0;
}

int Condition::win_cond_broadcast(cond_t *cond) {
    ASSERT(cond != NULL);

    cond->list_mutex.lock();

    if(cond->list_hd != NULL) {
        cond->list_tl->next = cond->released;
        cond->released = cond->list_hd;
        cond->list_hd = cond->list_tl = NULL;
        SetEvent(cond->cond);
    }
    cond->list_mutex.unlock();

    return 0;
}

#endif

void Condition::wait() throw(InterruptedException) {
    wait(-1);
}

void Condition::wait(long timeout) throw(InterruptedException) {
    int count;
    {
        Synchronized guard(_mutex._safeguard);
        count = _mutex._count;
        _mutex._count = 0;
        _mutex._owner = ThreadId();
    }

#if defined(__USE_POSIXTHREAD__)
    struct timespec abstime;
    if(timeout > 0) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        abstime.tv_sec = tv.tv_sec + (timeout / 1000);
        abstime.tv_nsec = (tv.tv_usec * 1000) + ((timeout % 1000) * 1000000);
        if(abstime.tv_nsec > 1000000000) {
            ++abstime.tv_sec;
            abstime.tv_nsec -= 1000000000;
        }
    }
#endif

    for(;;) {
        int ret = 0;
#if defined(__USE_WINTHREAD__)
        if(timeout < 0) {
            ret = Condition::win_cond_wait(&_cond, _mutex);
        } else {
            ret = Condition::win_cond_timedwait(&_cond, _mutex, timeout);
        }
#elif defined(__USE_POSIXTHREAD__)
        if(timeout < 0) {
            ret = pthread_cond_wait(&_cond, &_mutex._critical);
        } else {
            ret = pthread_cond_timedwait(&_cond, &_mutex._critical, &abstime);
        }
#endif

        // Handle possible error in wait
        if(ret != 0) {
            try {
#if defined(__USE_WINTHREAD__)
                ReleaseMutex(_mutex._critical);
#elif defined(__USE_POSIXTHREAD__)
                pthread_mutex_unlock(&_mutex._critical);
#endif
                _mutex.lock();
            } catch(...) {
            }

#if defined(__USE_WINTHREAD__)
            if(ret == WAIT_TIMEOUT) {
                return;
            }
#elif defined(__USE_POSIXTHREAD__)
            if(ret == ETIMEDOUT || ret == EAGAIN) {
                return;
            }
#endif

            throw InterruptedException();
        }
        break;
    }

#if defined(__USE_WINTHREAD__)
    ReleaseMutex(_mutex._critical);
#elif defined(__USE_POSIXTHREAD__)
    pthread_mutex_unlock(&_mutex._critical);
#endif
    _mutex.lock(count);
}

void Condition::signal() {
#if defined(__USE_WINTHREAD__)
    Condition::win_cond_signal(&_cond);
#elif defined(__USE_POSIXTHREAD__)
    pthread_cond_signal(&_cond);
#endif
}

void Condition::broadcast() {
#if defined(__USE_WINTHREAD__)
    Condition::win_cond_broadcast(&_cond);
#elif defined(__USE_POSIXTHREAD__)
    pthread_cond_broadcast(&_cond);
#endif
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
