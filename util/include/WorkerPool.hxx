/*************************************************************************
 * $Id: WorkerPool.hxx,v 1.5 2000/12/03 00:16:07 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/12/03 00:16:07 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created: 11/20/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _WORKERPOOL_HXX_
#define _WORKERPOOL_HXX_

#include <iostream>
#include "BaseObject.hxx"
#include "String.hxx"
#include "Queue.hxx"
#include "Slist.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

enum WorkerStatus {
    WK_IDLE,
    WK_BUSY
};

/**
 * The WorkUnit interface is to be implemented by classes that have work that
 * could be divided into units for distributed or parallel computation.
 */
class UTILAPI WorkUnit : public Root {

public:

    virtual void doWork(void *args) = 0;

};

class WorkerPool;

/**
 * The worker class is an interface for classes that implements primitives
 * for supporting distributed or parallel work.
 * Example implementations:
 *   - ThreadWorker
 *   - MPIWorker
 */
class UTILAPI Worker : public PtrReference {

public:

    Worker(const char *name = NULL);
    ~Worker();

    WorkerStatus getCurrentStatus() const;

protected:

    // To be overridden by the implementing class
    virtual void serviceWorkUnit(WorkUnit &unit, void *args) = 0;

private:

    friend class WorkerPool;

    // Methos called by the WorkerPool class
    void setPoolAssociation(WorkerPool *pool);
    void assignWork(WorkUnit &unit, void *args);

    WorkerPool *_pool;
    WorkerStatus _status;
    unsigned long _createTime;
    unsigned long _idleTime;
    unsigned long _busyTime;

protected:

    String _name;

};

/**
 * The WorkerPool interface is used for managing a pool of Worker resources.
 * Usually, the mapping is one worker per Proccessor Element.
 * This base interface class implements all the neccessary resource management,
 * ie. picking idle workers and dispatching work units to them, and adding
 * them back into the free pool after work units have completed.
 *
 * The subclass should implement the initWorkers() function for initial
 * Worker allocation.
 *
 * Example implementations:
 *   - ThreadWorkerPool
 *   - MPIWorkerPool
 */
class UTILAPI WorkerPool : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(WorkerPool);

public:

    WorkerPool(int poolsize);
    virtual ~WorkerPool();

    int getNumWorkers() const;
    int getNumIdleWorkers() const;
    int getNumBusyWorkers() const;

    void doWork(WorkUnit &unit, void *args);

protected:

    // To be overridden by the implementing class
    virtual void initWorkers() = 0;
    virtual void destroyWorkers() = 0;

private:

    friend class Worker;

    // Call back functions from the Worker class
    void workerBeginService(Ptr<Worker> worker);
    void workerEndService(Ptr<Worker> worker);

    // Internal Worker class management
    void dispatchWork(WorkUnit &unit, void *args);

protected:

    Ptr<Worker> *_workers;              // All registered workers
    Slist<Ptr<Worker> > _busyWorkers;   // Busy worker list
    Queue<Ptr<Worker> > _freeWorkers;   // Idle worker queue

    int _poolsize;
    int _numIdleWorkers;
    int _numBusyWorkers;
    int _numWorkUnitsServiced;
    int _totalServiceTime;

};

// Inline functions

inline Worker::Worker(const char *name) {
    _name = name;
}

inline Worker::~Worker() {

}

inline WorkerStatus Worker::getCurrentStatus() const {
    return _status;
}

inline void Worker::setPoolAssociation(WorkerPool *pool) {
    _pool = pool;
}

inline WorkerPool::~WorkerPool() {
    if(_workers != NULL) {
        delete [] _workers;
    }
}

inline int WorkerPool::getNumWorkers() const {
    WorkerPool *self = (WorkerPool *) (const WorkerPool *) this;
    synchronized(*self, {
            return _poolsize;
        });
}

inline int WorkerPool::getNumIdleWorkers() const {
    WorkerPool *self = (WorkerPool *) (const WorkerPool *) this;
    synchronized(*self, {
            return _numIdleWorkers;
        });
}

inline int WorkerPool::getNumBusyWorkers() const {
    WorkerPool *self = (WorkerPool *) (const WorkerPool *) this;
    synchronized(*self, {
            return _numBusyWorkers;
        });
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
