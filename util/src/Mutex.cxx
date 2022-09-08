/*************************************************************************
 * $Id: Mutex.cxx,v 1.9 2001/07/29 07:30:45 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/07/29 07:30:45 $
 * $Locker:  $
 * $Revision: 1.9 $
 *
 * Created:  9/10/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include "Error.hxx"
#include "ThreadException.hxx"
#include "Mutex.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

void Mutex::lock() throw(ThreadException) {
#if defined(__USE_WINTHREAD__)
    if(WaitForSingleObject(_mutex, INFINITE) == WAIT_ABANDONED) {
        throw ThreadException("Mutex::lock()");
    }
#elif defined(__USE_POSIXTHREAD__)
    if(pthread_mutex_lock(&_mutex) != 0) {
        throw ThreadException("Mutex::lock()");
    }
#endif
}

void Mutex::unlock() throw(ThreadException) {
#if defined(__USE_WINTHREAD__)
    ReleaseMutex(_mutex);
#elif defined(__USE_POSIXTHREAD__)
    if(pthread_mutex_unlock(&_mutex) != 0) {
        throw ThreadException("Mutex::unlock()");
    }
#endif
}

void RecursiveMutex::lock(int count) throw(ThreadException) {
    boolean locked = FALSE;
    while(!locked) {
        _safeguard.lock();  // Lock the safety mutex
    
        if(_count == 0) {
            // No locks on the mutex yet, safe to proceed
            _count = count;
            _owner = ThreadId::self();
            locked = TRUE;

            int ret = 0;
#if defined(__USE_WINTHREAD__)
            ret = WaitForSingleObject(_critical, INFINITE);
#elif defined(__USE_POSIXTHREAD__)
            ret = pthread_mutex_lock(&_critical);
#endif

#if defined(__USE_WINTHREAD__)
            if(ret == WAIT_ABANDONED) {
#elif defined(__USE_POSIXTHREAD__)
                if(ret != 0) {
#endif
                    try {
                        _safeguard.unlock();
                    } catch(...) {
                        Error::warning("Unable to unlock safeguard mutex in "
                                       "RecursiveMutex::lock()");
                    }
	
                    throw ThreadException("Undetermined error in RecursiveMutex::lock()");;
                }     
            } else if(_owner == ThreadId::self()) {
                // This mutex has been locked by the same thread before, simply
                // increase the count and return
                _count += count;
                locked = TRUE;
            }
    
            _safeguard.unlock();  // Unlock the safety mutex
    
            if(!locked) {
                // If a lock on the critical section was not obtained, then wait for the
                // lock and quickly unlock it and re-attempt to lock
#if defined(__USE_WINTHREAD__)
                WaitForSingleObject(_critical, INFINITE);
                ReleaseMutex(_critical);
#elif defined(__USE_POSIXTHREAD__)
                pthread_mutex_lock(&_critical);
                pthread_mutex_unlock(&_critical);
#endif
            }
        }

    }

    void RecursiveMutex::unlock() throw(ThreadException) {
        boolean unlocked = FALSE;
        _safeguard.lock();

        if(--_count == 0) {
            _owner = ThreadId();
#if defined(__USE_WINTHREAD__)
            ReleaseMutex(_critical);
#elif defined(__USE_POSIXTHREAD__)
            pthread_mutex_unlock(&_critical);
#endif
            unlocked = TRUE;
        }

        _safeguard.unlock();
  
        if(unlocked) {
#if defined(__USE_WINTHREAD__)
            Yield();
#elif defined(__USE_POSIXTHREAD__)
            sched_yield();
#endif
        }
    }

    ThreadId RecursiveMutex::getOwnerId() const {
        RecursiveMutex *rmt = (RecursiveMutex *) this;
        rmt->_safeguard.lock();

        ThreadId id;
        if(_count > 0) {
            id = _owner;
        }

        rmt->_safeguard.unlock();
  
        return id;
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
