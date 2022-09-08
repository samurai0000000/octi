/*************************************************************************
 * $Id: ThreadWorkerPool.cxx,v 1.1 2000/11/28 10:18:54 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/28 10:18:54 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created: 11/20/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include "debug.h"
#include "Error.hxx"
#include "Thread.hxx"
#include "ThreadWorkerPool.hxx"

#define DEFAULT_THR_WAIT_TIMEOUT 5000

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_DISCOVERABLE(ThreadWorkerPool, WorkerPool);

void ThreadWorker::serviceWorkUnit(WorkUnit &unit, void *args) {
    synchronized(*this, {
            _unit = &unit;
            _args = args;
            notify();
        });
}

void ThreadWorker::run() {
    _running = TRUE;
    while(_running) {
        try {
            synchronized(*this, {
                    wait(DEFAULT_THR_WAIT_TIMEOUT);
                    if(_unit != NULL) {
                        try {
                            _unit->doWork(_args);
                        } catch(...) {
                            Error::warning("error occured on servicing unit\n");
                        }
                    }
                });
        } catch(InterruptedException e) {
        }
    }
}

void ThreadWorker::stopRun() {
    _running = FALSE;
    interrupt();
    join();
}

void ThreadWorkerPool::initWorkers() {
    for(int i = 0; i < _poolsize; i++) {
        ThreadWorker *w = new ThreadWorker();
        w->start();
        _workers[i] = w;
        _freeWorkers.append(_workers[i]);
    }
}

void ThreadWorkerPool::destroyWorkers() {
    synchronized(*this, {
            // Unfinished
        });
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
