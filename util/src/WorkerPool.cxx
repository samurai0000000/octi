/*************************************************************************
 * $Id: WorkerPool.cxx,v 1.1 2000/11/28 10:19:13 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/28 10:19:13 $
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
#include "Thread.hxx"
#include "Date.hxx"
#include "WorkerPool.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_DISCOVERABLE(WorkerPool, BaseObject);

void Worker::assignWork(WorkUnit &unit, void *args) {
  Ptr<Worker> worker(this);
  _pool->workerBeginService(worker);
  serviceWorkUnit(unit, args);
  _pool->workerEndService(worker);
}

WorkerPool::WorkerPool(int poolsize) {
  _poolsize = poolsize;
  _workers = new Ptr<Worker>[poolsize];
}

void WorkerPool::doWork(WorkUnit &unit, void *args) {
  dispatchWork(unit, args);
}

void WorkerPool::dispatchWork(WorkUnit &unit, void *args) {
  Ptr<Worker> avbWorker;

  synchronized(_freeWorkers, {
    if(_freeWorkers.size() <= 0) {
      _freeWorkers.wait();  // Wait on the free queue to have one available
    }

    _freeWorkers.remove(avbWorker);
  });

  ASSERT(avbWorker != NULL);
  avbWorker->assignWork(unit, args);
}

void WorkerPool::workerBeginService(Ptr<Worker> worker) {
  synchronized(_busyWorkers, {
    // Add the worker to the busy list
    _busyWorkers.append(worker);
    _busyWorkers.notify();
  });
}

void WorkerPool::workerEndService(Ptr<Worker> worker) {
  synchronized(_busyWorkers, {
    // Remove the worker from the busy list
    _busyWorkers.removeElement(worker);
    _busyWorkers.notify();
  });

  synchronized(_freeWorkers, {
    // Add the worker to the free queue
    _freeWorkers.append(worker);
    _freeWorkers.notify();
  });
}

__END_NAMESPACE(SELFSOFT);
