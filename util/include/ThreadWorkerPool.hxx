/*************************************************************************
 * $Id: ThreadWorkerPool.hxx,v 1.1 2000/11/28 10:18:53 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/28 10:18:53 $
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

#ifndef _THREADWORKERPOOL_HXX_
#define _THREADWORKERPOOL_HXX_

#include "Thread.hxx"
#include "WorkerPool.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI ThreadWorker : public Worker, public Thread {

public:

  ThreadWorker();
  ~ThreadWorker();

  // Implements the Runnable interface
  void run();

protected:

  // Implements the Worker interface
  void serviceWorkUnit(WorkUnit &unit, void *args);

private:

  friend class ThreadWorkerPool;

  void stopRun();

private:

  boolean _running;
  WorkUnit *_unit;
  void *_args;

};

class UTILAPI ThreadWorkerPool : public WorkerPool {

  DECLARE_RUNTIME_DISCOVERABLE(ThreadWorkerPool);

public:

  ThreadWorkerPool(int poolsize);

protected:

  void initWorkers();
  void destroyWorkers();

private:
  
  ThreadGroup _thrGroup;

};

// Inline functions

inline ThreadWorker::ThreadWorker() {
  _unit = NULL;
  _args = NULL;
  _running = FALSE;
}

inline ThreadWorker::~ThreadWorker() {
  stopRun();
}

inline ThreadWorkerPool::ThreadWorkerPool(int poolsize)
  : WorkerPool(poolsize), _thrGroup("ThreadWorkerPool") {
  initWorkers();
}

__END_NAMESPACE(SELFSOFT);

#endif
