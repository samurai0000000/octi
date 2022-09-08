/*************************************************************************
 * $Id: Thread.hxx,v 1.7 2000/11/14 06:16:14 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/14 06:16:14 $
 * $Locker:  $
 * $Revision: 1.7 $
 *
 * Created:  1/7/99 by Charles Chiou
 * Comments:
 *     This is a wrapper class for POSIX pthreads or Windows native threads.
 *     The MIT pthreads is supported by default.
 *
 *     ThreadException exceptions are thrown for some member functions. The
 *     exceptions should be caught accordingly.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _THREAD_HXX_
#define _THREAD_HXX_

#include <string.h>
#include "utilapi.h"
#include "debug.h"
#include "BaseObject.hxx"
#include "Boolean.hxx"
#include "Exception.hxx"
#include "Mutex.hxx"
#include "Condition.hxx"
#include "String.hxx"
#include "ThreadException.hxx"

#if defined(__USE_WINTHREAD__)
#include <windows.h>
#elif defined(__USE_POSIXTHREAD__)
#include <pthread.h>
#else
#error ERROR: No thread library specidied
#endif  /* __USE_WINTHREAD_ */

#define MAX_THREAD_PRIORITY  2
#define MAIN_THREAD_GROUP    "Main Thread Group"
#define SYSTEM_THREAD_GROUP  "System Thread Group"
#define MAIN_THREAD          "Main Thread"
#define ANONYMOUS_THREAD     "Anonymous Thread"
#define ORPHANED_THREAD      "Orphaned Thread"

__BEGIN_NAMESPACE(SELFSOFT);

typedef enum {
    THREAD_NEW        = 0,
    THREAD_RUNNING    = 1,
    THREAD_SUSPENDED  = 2,
    THREAD_DEAD       = 3
} THREAD_STATE;

typedef void *(*ThreadFunc)(void *);

class ThreadGroupSet;
class ThreadSet;
class ThreadGroup;
class ThreadLocal;
class Thread;

/**
 * Classes that needs a thread execution path should derive this interface
 * class and implement the run() method.
 *
 */
class UTILAPI Runnable : public Root {

public:

    virtual void run() = 0;

};

/**
 * This class is a container to a set of ThreadGroup objects. It is used
 * by ThreadGroup class to enumerate ThreadGroup objects contained therein.
 */
class ThreadGroupSet : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(ThreadGroupSet);

public:

    ThreadGroupSet();
    ThreadGroupSet(const ThreadGroupSet &groupSet);
    ~ThreadGroupSet();

    ThreadGroupSet &operator=(const ThreadGroupSet &groupSet);
    ThreadGroupSet &operator+=(const ThreadGroupSet &groupSet);

    ThreadGroup &operator[](int i);
    int length() const;

private:

    friend class ThreadGroup;

    void addGroup(ThreadGroup *group);
    void removeGroup(ThreadGroup *group);

    ThreadGroup **_groups;
    int _numGroups;

};

/**
 * This class is a container to a set of Thread objects. It is used by the
 * ThreadGroup class to enumerate Thread objects contained therein.
 */
class ThreadSet : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(ThreadSet);

public:

    ThreadSet();
    ThreadSet(const ThreadSet &threadSet);
    ~ThreadSet();

    ThreadSet &operator=(const ThreadSet &threadSet);
    ThreadSet &operator+=(const ThreadSet &threadSet);

    Thread &operator[](int i);
    int length() const;

private:

    friend class ThreadGroup;

    void addThread(Thread *thread);
    void removeThread(Thread *thread);

    Thread **_threads;
    int _numThreads;

};

/**
 * This class is a container for a set of thread objects that belong to a
 * common "logical" group.
 */
class UTILAPI ThreadGroup : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(ThreadGroup);

public:

    ThreadGroup();
    ThreadGroup(const char* name);
    ThreadGroup(ThreadGroup *parent, const char* name);

    ~ThreadGroup();

    ThreadGroup *getParent();

    void setName(const char *name);
    const char *getName() const;

    void setDaemon(boolean daemon);
    boolean isDaemon() const;

#if defined(__USE_THREAD_FORCESTOP__)
    void stop();
#endif
    void resume();
    void suspend();

    void setMaxPriority(int priority);
    int getMaxPriority() const;

    void destroy() throw(IllegalThreadStateException);
    boolean isDestroyed() const;

    int activeCount() const;
    int activeGroupCount() const;

    boolean isParentOf(const ThreadGroup &group) const;

    ThreadSet enumerateThreads(boolean recursive = FALSE);
    ThreadGroupSet enumerateGroups(boolean recursive = FALSE);

private:

    static ThreadGroupSet s_allThreadGroups;
    static ThreadGroup s_mainThreadGroup;
    static ThreadGroup s_systemThreadGroup;

private:

    friend class Thread;

    void addGroup(ThreadGroup *group) throw(IllegalThreadStateException);
    void removeGroup(ThreadGroup *group) throw(IllegalThreadStateException);
    void add(Thread *thread) throw(IllegalThreadStateException);
    void remove(Thread *thread) throw(IllegalThreadStateException);

    void initialize(ThreadGroup *parent, const char *name);

    RecursiveMutex _mutex;

    ThreadGroup *_parent;
    String _name;
    boolean _daemon;
    boolean _destroyed;
    int _maxPriority;

    ThreadSet _threads;
    ThreadGroupSet _childGroups;

};

/**
 * Thread local variables.
 */
class UTILAPI ThreadLocal : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(ThreadLocal);

public:

    ThreadLocal();
    ~ThreadLocal();

    void set(void *value);
    void *get();

private:

#if defined(__USE_WINTHREAD__)
    DWORD _key;
#elif defined(__USE_POSIXTHREAD__)
    pthread_key_t _key;
#endif

};

/**
 * This class encapsulates the OS or library specific supports for threads.
 *
 */
class UTILAPI Thread : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(Thread);

public:

    Thread();
    Thread(const char *name);
    Thread(ThreadFunc startfunc, void *args = NULL, const char *name = NULL);
    Thread(Runnable &target, const char *name = NULL);
    Thread(ThreadGroup &group);
    Thread(ThreadGroup &group, const char *name);
    Thread(ThreadGroup &group, ThreadFunc startfunc, void *args = NULL,
           const char *name = NULL);
    Thread(ThreadGroup &group, Runnable &target, const char *name = NULL);

    ~Thread();

    void start() throw(IllegalThreadStateException);   // Starts the thread
#if defined(__USE_THREAD_FORCESTOP__)
    void stop() throw(IllegalThreadStateException);    // Stops the thread
#endif
    void suspend() throw(IllegalThreadStateException); // Suspends the thread
    void resume() throw(IllegalThreadStateException);  // Resumes the thread

    void join() throw(InterruptedException);  // Waits for thread termination
    void join(long timeout) throw(InterruptedException);  // Waits for thread termination with timeout

    void interrupt();  // Interrupts the thread

    static void sleep(long timeout) throw(InterruptedException);
    static void yield();

    void setPriority(int priority);
    int getPriority() const;

    boolean isAlive() const;

    const char *getName() const;
    void setName(const char *name);
    ThreadId getThreadId() const;

    ThreadGroup *getThreadGroup();

    static void dumpThreadUsage();

public:

    // Gets and sets the default stack size used to initialize a thread
    static void setDefaultThreadStackSize(int stackSize);
    static int getDefaultThreadStackSize();

protected:

    // The run function is the execution path of the thread if this class
    // was constructed with the empty constructor.
    // Derived classes should implement this function for its own needs.
    virtual void run();

private:

    friend class Monitor;

    // Entry point function after a native thread has been created
    static void *startRun(void *args);

    // Function that starts the execution of the thread in Thread::run method
    static void *startThreadRun(void *args);
    // Function that starts the execution of the thread in Runnable::run method
    static void *startRunnableRun(void *args);

    void initialize(ThreadGroup *group, const char *name);
    void threadStarted();
    void threadExited();

    void checkRunningStatus();
    void setMonitor(Monitor *monitor);
    Monitor *getMonitor();

    static void checkRunningStatusStatic();
    static void setMonitorStatic(Monitor *monitor);

private:

    // Main thread initializtion constructor
    Thread(ThreadId id);
    // Orphaned thread constructor
    Thread(Thread &thread);

    // System and TSS
    static Thread s_mainThread;
    static ThreadLocal s_systemThreadLocal;
    static int s_defaultStackSize;

private:

#if defined(__USE_WINTHREAD__)
    DWORD _thread;
    HANDLE _handle;
#elif defined(__USE_POSIXTHREAD__)
    pthread_t _thread;
#endif

    typedef struct _StartRunArg {
        Mutex mutex;
        Thread *thread;
    } StartRunArg;

    ThreadFunc _startfunc;        // The start function
    void *_args;                  // Arguments to the start function
    StartRunArg *_startRunArg;    // Arguments to the static thread entry func

    ThreadGroup *_group;           // The group this thread belongs to
    ThreadId _threadId;           // The thread ID
    String _name;                 // The name of the thread
    THREAD_STATE _state;          // The state of the thread

    boolean _orphaned;
    boolean _terminated;
    boolean _suspended;

    RecursiveMutex _threadBeginMutex;
    Condition _threadBeginCondition;
    RecursiveMutex _resumeMutex;
    Condition _resumeCondition;
    RecursiveMutex _joinMutex;
    Condition _joinCondition;
    Mutex _monitorMutex;
    Monitor *_monitor;

};

// Inline functions

inline ThreadGroupSet::ThreadGroupSet() : _groups(NULL), _numGroups(0) {

}

inline ThreadGroupSet::ThreadGroupSet(const ThreadGroupSet &groupSet) : _groups(NULL), _numGroups(0) {
    operator=(groupSet);
}

inline ThreadGroupSet::~ThreadGroupSet() {
    if(_groups) {
        free(_groups);
    }
}

inline ThreadGroupSet &ThreadGroupSet::operator=(const ThreadGroupSet &groupSet) {
    if(_groups != NULL) {
        free(_groups);
    }

    _numGroups = groupSet._numGroups;
    if(_numGroups > 0) {
        _groups = (ThreadGroup **) calloc(_numGroups, sizeof(ThreadGroup *));
        memcpy(_groups, groupSet._groups, _numGroups * sizeof(ThreadGroup *));
    }

    return *this;
}

inline ThreadGroupSet &ThreadGroupSet::operator+=(const ThreadGroupSet &groupSet) {
    for(int i = 0; i < groupSet._numGroups; i++) {
        addGroup(groupSet._groups[i]);
    }

    return *this;
}

inline ThreadGroup &ThreadGroupSet::operator[](int i) {
    ASSERT(i >= 0 && i <= _numGroups);
    return *_groups[i];
}

inline int ThreadGroupSet::length() const {
    return _numGroups;
}

inline ThreadSet::ThreadSet() : _threads(NULL), _numThreads(0) {

}

inline ThreadSet::ThreadSet(const ThreadSet &threadSet) : _threads(NULL), _numThreads(0) {
    operator=(threadSet);
}

inline ThreadSet::~ThreadSet() {
    if(_threads) {
        free(_threads);
    }
}

inline ThreadSet &ThreadSet::operator=(const ThreadSet &threadSet) {
    if(_threads != NULL) {
        free(_threads);
    }

    _numThreads = threadSet._numThreads;
    if(_numThreads > 0) {
        _threads = (Thread **) calloc(_numThreads, sizeof(Thread *));
        memcpy(_threads, threadSet._threads, _numThreads * sizeof(Thread *));
    }

    return *this;
}

inline ThreadSet &ThreadSet::operator+=(const ThreadSet &threadSet) {
    for(int i = 0; i < threadSet._numThreads; i++) {
        addThread(threadSet._threads[i]);
    }

    return *this;
}

inline Thread &ThreadSet::operator[](int i) {
    ASSERT(i >= 0 && i <= _numThreads);
    return *_threads[i];
}

inline int ThreadSet::length() const {
    return _numThreads;
}

inline ThreadGroup::ThreadGroup() {
    initialize(NULL, NULL);
}

inline ThreadGroup::ThreadGroup(const char* name) {
    initialize(NULL, name);
}

inline ThreadGroup::ThreadGroup(ThreadGroup *parent, const char* name) {
    initialize(parent, name);
}

inline ThreadGroup *ThreadGroup::getParent() {
    return _parent;
}

inline void ThreadGroup::setName(const char *name) {
    _name = name;
}

inline const char *ThreadGroup::getName() const {
    return _name;
}

inline void ThreadGroup::setDaemon(boolean daemon) {
    _daemon = daemon;
}

inline boolean ThreadGroup::isDaemon() const {
    return _daemon;
}

inline ThreadLocal::ThreadLocal() {
#if defined(__USE_WINTHREAD__)
    _key = TlsAlloc();
#elif defined(__USE_POSIXTHREAD__)
    pthread_key_create(&_key, NULL);
#endif
}

inline ThreadLocal::~ThreadLocal() {
#if defined(__USE_WINTHREAD__)
    TlsFree(_key);
#elif defined(__USE_POSIXTHREAD__)
    pthread_key_delete(_key);
#endif
}

inline void ThreadLocal::set(void *value) {
#if defined(__USE_WINTHREAD__)
    TlsSetValue(_key, value);
#elif defined(__USE_POSIXTHREAD__)
    pthread_setspecific(_key, value);
#endif
}

inline void *ThreadLocal::get() {
#if defined(__USE_WINTHREAD__)
    return TlsGetValue(_key);
#elif defined(__USE_POSIXTHREAD__)
    return pthread_getspecific(_key);
#endif
}

inline void Thread::yield() {
    checkRunningStatusStatic();

#if defined(__USE_WINTHREAD__)
    Yield();
#elif defined(__USE_POSIXTHREAD__)
    sched_yield();
#endif

    checkRunningStatusStatic();
}

inline void Thread::run() {

}

inline void Thread::join() throw(InterruptedException) {
    join(0);
}

inline void Thread::setDefaultThreadStackSize(int stackSize) {
    s_defaultStackSize = (stackSize > 0 ? stackSize : 0);
}

inline int Thread::getDefaultThreadStackSize() {
    return s_defaultStackSize;
}

inline const char *Thread::getName() const {
    return _name;
}

inline void Thread::setName(const char *name) {
    _name = name;
}

inline ThreadId Thread::getThreadId() const {
    return ThreadId::self();
}

inline ThreadGroup *Thread::getThreadGroup() {
    return _group;
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
