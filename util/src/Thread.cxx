/*************************************************************************
 * $Id: Thread.cxx,v 1.10 2001/07/30 04:30:06 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/07/30 04:30:06 $
 * $Locker:  $
 * $Revision: 1.10 $
 *
 * Created:  1/7/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#ifndef _WIN32
#include <unistd.h>
#endif
#include <stdio.h>
#include <errno.h>
#include "Error.hxx"
#include "Thread.hxx"
#include "String.hxx"
#include "Date.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_DISCOVERABLE(ThreadGroupSet, BaseObject);
IMPLEMENT_RUNTIME_DISCOVERABLE(ThreadSet, BaseObject);
IMPLEMENT_RUNTIME_DISCOVERABLE(ThreadGroup, BaseObject);
IMPLEMENT_RUNTIME_DISCOVERABLE(ThreadLocal, BaseObject);
IMPLEMENT_RUNTIME_DISCOVERABLE(Thread, BaseObject);

/****************************************************************************/
/* ThreadGroupSet                                                           */
/****************************************************************************/

void ThreadGroupSet::addGroup(ThreadGroup *group) {
  synchronized(*this, {
    if(group == NULL) {
      return;
    }
    
    int i;
    for(i = 0; i < _numGroups && _groups[i] != group; i++);
    
    if(i >= _numGroups) {
      ThreadGroup **newGroups = (ThreadGroup **) calloc(_numGroups + 1, sizeof(ThreadGroup *));
      memcpy(newGroups, _groups, _numGroups * sizeof(ThreadGroup *));
      newGroups[_numGroups] = group;
      if(_groups) {
	free(_groups);
      }
      _numGroups++;
      _groups = newGroups;
    }
  });
}

void ThreadGroupSet::removeGroup(ThreadGroup *group) {
  synchronized(*this, {
    if(group == NULL) {
      return;
    }

    int i;
    for(i = 0; i < _numGroups && _groups[i] != group; i++);
    
    if(i < _numGroups) {
      ThreadGroup **newGroups = NULL;
      if(_numGroups - 1 > 0) {
	newGroups= (ThreadGroup **) calloc(_numGroups - 1, sizeof(ThreadGroup *));
	memcpy(newGroups, _groups, i * sizeof(ThreadGroup *));
	memcpy(newGroups + i, &_groups[i + 1], (_numGroups - i - 1) * sizeof(ThreadGroup *));
      }

      if(_groups) {
	free(_groups);
      }

      _numGroups--;
      _groups = newGroups;
    }
  });
}

/****************************************************************************/
/* ThreadSet                                                                */
/****************************************************************************/

void ThreadSet::addThread(Thread *thread) {
  synchronized(*this, {
    if(thread == NULL) {
      return;
    }

    int i;
    for(i = 0; i < _numThreads && _threads[i] != thread; i++);
    
    if(i >= _numThreads) {
      Thread **newThreads = (Thread **) calloc(_numThreads + 1, sizeof(Thread *));
      memcpy(newThreads, _threads, _numThreads * sizeof(Thread *));
      newThreads[_numThreads] = thread;
      if(_threads) {
	free(_threads);
      }
      _numThreads++;
      _threads = newThreads;
    }
  });
}

void ThreadSet::removeThread(Thread *thread) {
  synchronized(*this, {
    if(thread == NULL) {
      return;
    }

    int i;
    for(i = 0; i < _numThreads && _threads[i] != thread; i++);
    
    if(i < _numThreads) {
      Thread **newThreads = NULL;
      if(_numThreads - 1 > 0) {
	newThreads = (Thread **) calloc(_numThreads - 1, sizeof(Thread *));
	memcpy(newThreads, _threads, i * sizeof(Thread *));
	memcpy(newThreads + i, &_threads[i + 1], (_numThreads - i - 1) * sizeof(Thread *));
      }

      if(_threads) {
	free(_threads);
      }
      _numThreads--;
      _threads = newThreads;
    }
  });
}

/****************************************************************************/
/* ThreadGroup                                                              */
/****************************************************************************/

ThreadGroupSet ThreadGroup::s_allThreadGroups;

ThreadGroup ThreadGroup::s_mainThreadGroup(MAIN_THREAD_GROUP);

ThreadGroup ThreadGroup::s_systemThreadGroup(SYSTEM_THREAD_GROUP);

ThreadGroup::~ThreadGroup() {
  synchronized(s_allThreadGroups, {
    s_allThreadGroups.removeGroup(this);
  });
}

void ThreadGroup::initialize(ThreadGroup *parent, const char *name) {
  _parent = parent;
  _name = name;
  _daemon = (parent == NULL ? FALSE : parent->_daemon);
  _destroyed = FALSE;
  _maxPriority = (parent == NULL ? MAX_THREAD_PRIORITY : parent->_maxPriority);
  
  if(parent != NULL) {
    _parent->addGroup(this);
  }

  synchronized(s_allThreadGroups, {
    s_allThreadGroups.addGroup(this);
  });
}

void ThreadGroup::addGroup(ThreadGroup *group) throw(IllegalThreadStateException) {
  synchronized(*this, {
    if(_destroyed) {
      throw IllegalThreadStateException("ThreadGroup destroyed");
    }
    
    _childGroups.addGroup(group);
  });
}

void ThreadGroup::removeGroup(ThreadGroup *group) throw(IllegalThreadStateException) {
  synchronized(*this, {
    if(_destroyed) {
      throw IllegalThreadStateException("ThreadGroup destroyed");
    }
    
    _childGroups.removeGroup(group);
  });
}

void ThreadGroup::add(Thread *thread)  throw(IllegalThreadStateException){
  synchronized(*this, {
    if(_destroyed) {
      throw IllegalThreadStateException("ThreadGroup destroyed");
    }

    _threads.addThread(thread);
  });
}

void ThreadGroup::remove(Thread *thread) throw(IllegalThreadStateException) {
  synchronized(*this, {
    if(_destroyed) {
      throw IllegalThreadStateException("ThreadGroup destroyed");
    }

    _threads.removeThread(thread);
  });
}

#if defined(__USE_THREAD_FORCESTOP__)

void ThreadGroup::stop() {
  ThreadGroupSet groupSnapshot;
  synchronized(*this, {
    for(int i = 0; i < _threads.length(); i++) {
      _threads[i].stop();
    }
    groupSnapshot = _childGroups;
  });

  for(int i = 0; i < groupSnapshot.length(); i++) {
    groupSnapshot[i].stop();
  }
}

#endif

void ThreadGroup::resume() {
  ThreadGroupSet groupSnapshot;
  synchronized(*this, {
    for(int i = 0; i < _threads.length(); i++) {
      _threads[i].resume();
    }
    groupSnapshot = _childGroups;
  });

  for(int i = 0; i < groupSnapshot.length(); i++) {
    groupSnapshot[i].resume();
  }
}

void ThreadGroup::suspend() {
  ThreadGroupSet groupSnapshot;
  synchronized(*this, {
    for(int i = 0; i < _threads.length(); i++) {
      _threads[i].suspend();
    }
    groupSnapshot = _childGroups;
  });

  for(int i = 0; i < groupSnapshot.length(); i++) {
    groupSnapshot[i].suspend();
  }
}

void ThreadGroup::setMaxPriority(int priority) {
  ThreadGroupSet groupSnapshot;
  synchronized(*this, {
    _maxPriority = priority;
    groupSnapshot = _childGroups;
  });

  for(int i = 0; i < groupSnapshot.length(); i++) {
    groupSnapshot[i].setMaxPriority(priority);
  }
}

int ThreadGroup::getMaxPriority() const {
  return _maxPriority;
}

void ThreadGroup::destroy() throw(IllegalThreadStateException) {
  ThreadGroupSet groupSnapshot;
  synchronized(*this, {
    if(_destroyed) {
      throw IllegalThreadStateException("Thread already destroyed");
    } else if(_threads.length() > 0) {
      String msg;
      msg.format("Thread(s) still active: %d", _threads.length());
      throw IllegalThreadStateException(msg);
    }
    
    _destroyed = TRUE;
    _threads = ThreadSet();
    _childGroups = ThreadGroupSet();
    groupSnapshot = _childGroups;
  });

  for(int i = 0; i < groupSnapshot.length(); i++) {
    groupSnapshot[i].destroy();
  }

  if(_parent != NULL) {
    _parent->removeGroup(this);
  }
}

boolean ThreadGroup::isDestroyed() const {
  return _destroyed;
}

int ThreadGroup::activeCount() const {
  int count = 0;
  ThreadGroupSet groupSnapshot;
  ThreadGroup *group = (ThreadGroup *) this;
  synchronized(*group, {
    if(_destroyed) {
      return 0;
    }

    count = _threads.length();
    groupSnapshot = _childGroups;
  });

  for(int i = 0; i < groupSnapshot.length(); i++) {
    count += groupSnapshot[i].activeCount();
  }

  return count;
}

int ThreadGroup::activeGroupCount() const {
  int count = 0;
  ThreadGroupSet groupSnapshot;
  ThreadGroup *group = (ThreadGroup *) this;
  synchronized(*group, {
    if(_destroyed) {
      return 0;
    }

    groupSnapshot = _childGroups;
  });

  count = groupSnapshot.length();
  for(int i = 0; i < groupSnapshot.length(); i++) {
    count += groupSnapshot[i].activeCount();
  }

  return count;
}

boolean ThreadGroup::isParentOf(const ThreadGroup &group) const {
  ThreadGroup *g = (ThreadGroup *) &group;
  for(; g != NULL; g = (ThreadGroup *) g->_parent) {
    if(g == this) {
      return TRUE;
    }
  }

  return FALSE;
}

ThreadSet ThreadGroup::enumerateThreads(boolean recursive) {
  ThreadGroupSet groupSnapshot;
  ThreadSet threads;
  synchronized(*this, {
    threads = _threads;
    groupSnapshot = _childGroups;
  });

  if(recursive) {
    for(int i = 0; i < groupSnapshot.length(); i++) {
      threads += groupSnapshot[i].enumerateThreads(recursive);
    }
  }

  return threads;
}

ThreadGroupSet ThreadGroup::enumerateGroups(boolean recursive) {
  ThreadGroupSet groupSnapshot;
  ThreadGroupSet groups;
  synchronized(*this, {
    groups = _childGroups;
    groupSnapshot = _childGroups;
  });

  if(recursive) {
    for(int i = 0; i < groupSnapshot.length(); i++) {
      groups += groupSnapshot[i].enumerateGroups(recursive);
    }
  }

  return groups;
}

/****************************************************************************/
/* Thread                                                                   */
/****************************************************************************/

static const char *THREAD_STATE_STRING[] = {
  "NEW",
  "RUNNING",
  "SUSPENDED",
  "DEAD"
};

Thread Thread::s_mainThread(ThreadId::self());

ThreadLocal Thread::s_systemThreadLocal;

int Thread::s_defaultStackSize = 0;

void *Thread::startRun(void *args) {
  ASSERT(args != NULL);

  void *ret = NULL;
  StartRunArg *startRunArg = (StartRunArg *) args;
  Thread *currentThread;
  startRunArg->mutex.lock();
  currentThread = startRunArg->thread;
  currentThread->threadStarted();
  startRunArg->mutex.unlock();

  try {
    ThreadFunc runfunc = currentThread->_startfunc;
    void *runargs = currentThread->_args;
    ret = runfunc(runargs);
  } catch(ThreadDeathException e) {
    Error::warning("Thread terminated");
  } catch(Exception e) {
    String msg = e.getRuntimeClass()->getClassName();
    msg += " in thread: ";
    msg += e.getMessage();
    Error::warning(msg);
  } catch(...) {
    Error::warning("Exception in thread: Unknown");
  }

  // Get the current thread again because it's possible that a thread
  // object became an orphane
  startRunArg->mutex.lock();
  if(currentThread == startRunArg->thread) {
    currentThread = startRunArg->thread;
    ASSERT(currentThread != NULL);
    currentThread->threadExited();
  }

  // Test and delete orphaned thread
  if(currentThread != &Thread::s_mainThread && currentThread->_orphaned) {
    delete currentThread;
  }
  startRunArg->mutex.unlock();

  delete startRunArg;  // Delete after no longer needed!

  return ret;
}

void *Thread::startThreadRun(void *args) {
  ((Thread *) args)->run();
  return NULL;
}

void *Thread::startRunnableRun(void *args) {
  ((Runnable *) args)->run();
  return NULL;
}

Thread::Thread() : _threadBeginCondition(_threadBeginMutex),
		   _resumeCondition(_resumeMutex),
		   _joinCondition(_joinMutex) {
  _startfunc = Thread::startThreadRun;
  _args = this;
  initialize(NULL, NULL);
}

Thread::Thread(const char *name) : _threadBeginCondition(_threadBeginMutex),
				   _resumeCondition(_resumeMutex),
				   _joinCondition(_joinMutex) {
  _startfunc = Thread::startThreadRun;
  _args = this;
  initialize(NULL, name);
}

Thread::Thread(ThreadFunc startfunc, void *args, const char *name) :
  _threadBeginCondition(_threadBeginMutex), 
  _resumeCondition(_resumeMutex),
  _joinCondition(_joinMutex) {
  _startfunc = startfunc;
  _args = args;
  initialize(NULL, name);
}

Thread::Thread(Runnable &target, const char *name) :
  _threadBeginCondition(_threadBeginMutex),
  _resumeCondition(_resumeMutex),
  _joinCondition(_joinMutex) {
  _startfunc = Thread::startRunnableRun;
  _args = &target;
  initialize(NULL, name);
}

Thread::Thread(ThreadGroup &group) :
  _threadBeginCondition(_threadBeginMutex),
  _resumeCondition(_resumeMutex),
  _joinCondition(_joinMutex) {
  _startfunc = Thread::startThreadRun;
  _args = this;
  initialize(&group, NULL);
}

Thread::Thread(ThreadGroup &group, const char *name) :
  _threadBeginCondition(_threadBeginMutex),
  _resumeCondition(_resumeMutex),
  _joinCondition(_joinMutex) {
  _startfunc = Thread::startThreadRun;
  initialize(&group, name);
}

Thread::Thread(ThreadGroup &group, ThreadFunc startfunc, void *args, const char *name) :
  _threadBeginCondition(_threadBeginMutex),
  _resumeCondition(_resumeMutex),
  _joinCondition(_joinMutex) {
  _startfunc = startfunc;
  _args = args;
  initialize(&group, name);
}

Thread::Thread(ThreadGroup &group, Runnable &target, const char *name) :
  _threadBeginCondition(_threadBeginMutex),
  _resumeCondition(_resumeMutex),
  _joinCondition(_joinMutex) {
  _startfunc = Thread::startRunnableRun;
  _args = &target;
  initialize(&group, name);
}

// This is the system initialization constructor that is called EXACTLY once
// when the system is initialized
Thread::Thread(ThreadId id) :
  _threadBeginCondition(_threadBeginMutex),
  _resumeCondition(_resumeMutex),
  _joinCondition(_joinMutex) {
  setName(MAIN_THREAD);
  _orphaned = FALSE;
  _terminated = FALSE;
  _suspended = FALSE;
  _state = THREAD_RUNNING;
  _monitor = NULL;
  _threadId = ThreadId::self();

  s_systemThreadLocal.set(this);

  _group = &ThreadGroup::s_mainThreadGroup;
  _group->add(this);
}

// This constructor is called when a normal thread object is destructed while
// the thread hasn't exited yet. They become "orphaned" threads.
Thread::Thread(Thread &thread) :
  _threadBeginCondition(_threadBeginMutex),
  _resumeCondition(_resumeMutex),
  _joinCondition(_joinMutex) {
  _startfunc = NULL;
  _args = NULL;

  _name = thread._name;
  _orphaned = TRUE;
  _terminated = thread._terminated;
  _suspended = thread._suspended;
  _state = thread._state;
  _monitor = NULL;  // ???
  _group = thread._group;

  ASSERT(_group != NULL);
  _group->add(this);

  _startRunArg = thread._startRunArg;
  ASSERT(_startRunArg != NULL);
  _startRunArg->thread = this;  // Switch the pointer to this (orphane)
}

void Thread::initialize(ThreadGroup *group, const char *name) {
  setName(name == NULL ? ANONYMOUS_THREAD : name);
  _orphaned = FALSE;
  _terminated = FALSE;
  _suspended = FALSE;
  _state = THREAD_NEW;
  _monitor = NULL;
  _group = group;

  if(_group == NULL) {
    Thread *currentThread = (Thread *) s_systemThreadLocal.get();
    if(currentThread != NULL) {
      _group = currentThread->getThreadGroup();
    } else {
      _group = &ThreadGroup::s_systemThreadGroup;
    }
  }

  ASSERT(_group != NULL);
  _group->add(this);

  // The _startRunArg variable is created for passing into the thread entry
  // function Thread::startRun, It should be deleted explicitly in two places:
  // 1) If Thread::startRun is called, it will be deleted after it's no longer
  //    required.
  // 2) If Thread::startRun is never called, it will be deleted by the
  //    destructor
  _startRunArg = new StartRunArg;
  _startRunArg->thread = this;
}

Thread::~Thread() {
  if(this != &s_mainThread) {
    if(_state == THREAD_NEW) {
      // If this is a new thread when the destructor is called, then
      // free the memory allocated for StartRunArg since Thread::startRun
      // was never called
      ASSERT(_startRunArg != NULL);
      delete _startRunArg;
    } else if(_state != THREAD_NEW && _state != THREAD_DEAD) {
      // Create an orphane thread and remove ourself
      _startRunArg->mutex.lock();
      _startRunArg->thread = NULL;
      ASSERT(_group != NULL);
      _group->remove(this);
      Thread *orphane = new Thread(*this);
      orphane->getThreadId();
      _startRunArg->mutex.unlock();
    }
  }
}

void Thread::threadStarted() {
  s_systemThreadLocal.set(this);
  _state = THREAD_RUNNING;
  _threadBeginCondition.broadcast();
}

void Thread::threadExited() {
  synchronized(_joinMutex, {
    synchronized(_resumeMutex, {
      _state = THREAD_DEAD;
      _joinCondition.broadcast();
      
      ASSERT(_group != NULL);
      _group->remove(this);
      
      s_systemThreadLocal.set(NULL);
    });
  });
}

void Thread::start() throw(IllegalThreadStateException) {
  if(_state != THREAD_NEW) {
    throw IllegalThreadStateException("Not a new thread");
  }

  int stackSize = s_defaultStackSize;

#if defined(__USE_WINTHREAD__)
  _handle = CreateThread(NULL,
			 stackSize > 0 ? stackSize : 0,
			 (LPTHREAD_START_ROUTINE) Thread::startRun,
			 (LPVOID) _startRunArg,
			 0,
			 &_thread);
  if(_handle == INVALID_HANDLE_VALUE) {
    int errnum = GetLastError();
    if(errnum == ERROR_NOT_ENOUGH_MEMORY || errno == ERROR_OUTOFMEMORY) {
      throw Exception("Unable to create thread: out of memory");
    } else {
      throw Exception("Unable to create thread: unknown");
    }
  }
  _threadId = ThreadId(_thread, _handle);
#elif defined(__USE_POSIXTHREAD__)
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  if(stackSize > 0) {
    pthread_attr_setstacksize(&attr, stackSize);
  }

  int ret = pthread_create(&_thread, &attr, Thread::startRun, _startRunArg);
  if(ret != 0) {
    pthread_attr_destroy(&attr);
    if(ret == EAGAIN || ret == ENOMEM) {
      throw Exception("Unable to create thread: out of memory");
    } else {
      throw Exception("Unable to create thread: unknown");
    }
  }

  pthread_attr_destroy(&attr);
  _threadId = ThreadId(_thread);
#endif

  // Wait for the condition
  while(_state != THREAD_RUNNING) {
    try {
      _threadBeginCondition.wait(1000);
    } catch(...) {
    }
  }
}

#if defined(__USE_THREAD_FORCESTOP__)
void Thread::stop() throw(IllegalThreadStateException) {
  Monitor* currentMonitor = NULL;
  synchronized(_resumeMutex, {
    if(_state != THREAD_RUNNING && _state != THREAD_SUSPENDED) {
      String msg = "Cannot stop thread in ";
      msg += THREAD_STATE_STRING[_state];
      msg += " state";
      throw IllegalThreadStateException(msg);
    }

    if(!_terminated) {
      _terminated = TRUE;
      if(_suspended) {
	resume();
      } else {
	currentMonitor = getMonitor();
      }
    }
  });

  if(currentMonitor != NULL) {
    Synchronized monitorGuard(*currentMonitor, true);
    currentMonitor->notifyAll();
  }
}
#endif

void Thread::suspend() throw(IllegalThreadStateException) {
  synchronized(_resumeMutex, {
    if(_state != THREAD_RUNNING && _state != THREAD_SUSPENDED) {
      String msg = "Cannot suspend thread in ";
      msg += THREAD_STATE_STRING[_state];
      msg += " state";
      throw IllegalThreadStateException(msg);
    }

    checkRunningStatus();
    if(!_suspended) {
      _suspended = TRUE;

      if(_threadId == ThreadId::self()) {
	_resumeCondition.wait();
	checkRunningStatus();
      }
    }
  });
}

void Thread::resume() throw(IllegalThreadStateException) {
  synchronized(_resumeMutex, {
    if(_state != THREAD_RUNNING && _state != THREAD_SUSPENDED) {
      String msg = "Cannot resume thread in ";
      msg += THREAD_STATE_STRING[_state];
      msg += " state";
      throw IllegalThreadStateException(msg);
    }

    if(_suspended) {
      _resumeCondition.broadcast();
      _suspended = FALSE;
    }
  });
}

void Thread::join(long timeout) throw(InterruptedException) {
  if(timeout < 0) {
    timeout = 0;
  }

  checkRunningStatus();

  unsigned long timeStart = Date::currentTimeMillis();
  unsigned long timeNow = 0;
  synchronized(_joinMutex, {
    if(timeout == 0) {
      while(isAlive()) {
	_joinCondition.wait();
      }
    } else {
      while(isAlive()) {
	long delay = timeout - timeNow;
	if(delay <= 0) {
	  break;
	}

	_joinCondition.wait(delay);
	timeNow = Date::currentTimeMillis() - timeStart;
      }
    }
  });

  checkRunningStatus();
}

boolean Thread::isAlive() const {
  Thread *thread = (Thread *) this;
  synchronized(thread->_resumeMutex, {
    return _state != THREAD_DEAD && _state != THREAD_NEW;
  });
}

void Thread::interrupt() {
  // Not supported yet.
}

void Thread::sleep(long timeout) throw(InterruptedException) {
  checkRunningStatusStatic();
  if(timeout <= 0) {
    return;
  }

#if defined(__USE_WINTHREAD__)
  Sleep(timeout);
#elif defined(__USE_POSIXTHREAD__)
  usleep(timeout * 1000);
#endif  

  checkRunningStatusStatic();
}

void Thread::setPriority(int priority) {
#if defined(__USE_WINTHREAD__)
  SetThreadPriority(_threadId._handle, priority);
#elif defined(__USE_POSIXTHREAD__)
  sched_param param;
  int policy = 0;
  param.sched_priority = priority;
  pthread_setschedparam(_threadId, policy, &param);
#endif
}

int Thread::getPriority() const{
#if defined(__USE_WINTHREAD__)
  return GetThreadPriority(_threadId._handle);
#elif defined(__USE_POSIXTHREAD__)
  sched_param param;
  int policy;
  pthread_getschedparam(_threadId, &policy, &param);
  return param.sched_priority;
#endif
}

void Thread::checkRunningStatus() {
  synchronized(_resumeMutex, {
    if(_threadId == ThreadId::self() && _state != THREAD_DEAD) {
      if(_suspended) {
	_resumeCondition.wait();
      }

      if(_terminated) {
	// ******* NOTE ************
	// Throwing exceptions is causing problems in Linux
	//
	//cout << "throwing death!!" << endl;
	//throw ThreadDeathException();
      }
    }
  });
}

void Thread::checkRunningStatusStatic() {
  Thread *current = (Thread *) s_systemThreadLocal.get();
  if(current != NULL) {
    current->checkRunningStatus();
  }
}

void Thread::setMonitor(Monitor *monitor) {
  synchronized(_monitorMutex, {
    _monitor = monitor;
  });
}

Monitor *Thread::getMonitor() {
  synchronized(_monitorMutex, {
    return _monitor;
  });
}

void Thread::setMonitorStatic(Monitor *monitor) {
  Thread *current = (Thread *) s_systemThreadLocal.get();
  if(current != NULL) {
    current->setMonitor(monitor);
  }
}

// Dumps all the thread groups and threads to the 
void Thread::dumpThreadUsage() {
  synchronized(ThreadGroup::s_allThreadGroups, {
    ThreadGroupSet allGroups = ThreadGroup::s_allThreadGroups;
    
    cout << "--- Active Thread dump ---" << endl;
    for(int i = 0; i < allGroups.length(); i++) {
      cout << allGroups[i].getName() << ":" << endl;
      ThreadSet threads = allGroups[i].enumerateThreads();
      for(int j = 0; j < threads.length(); j++) {
	cout << " +-- " << threads[j].getName();
	if(threads[j]._orphaned) {
	  cout << " [orphaned] ";
	} else {
	  cout << " [normal] ";
	}
	cout << THREAD_STATE_STRING[threads[j]._state] << endl;
      }
    }
    cout << "--------------------------" << endl;
  });
}

__END_NAMESPACE(SELFSOFT);
