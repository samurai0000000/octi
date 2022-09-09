/*************************************************************************
 * $Id: ThreadTest.cxx,v 1.7 2001/08/09 03:11:11 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 03:11:11 $
 * $Locker:  $
 * $Revision: 1.7 $
 *
 * Created:  3/3/00 by Charles Chiou
 * Comments: Tests for threads.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#include "Condition.hxx"
#include "Barrier.hxx"
#include "Thread.hxx"

__USING_NAMESPACE(SELFSOFT);

BaseObject sig;

class ThreadTest : public Thread {

public:

    ThreadTest(ThreadGroup &group, int id, int interval) : Thread(group) {
        _id = id;
        _interval = interval;
    };

    void run() {
        cout << "Thread Id(" << _id << ") started" << endl;
        for(int i = 0; i < 10; i++) {
            int c = 0;
            try {
                synchronized(rmutex, {
                        synchronized(rmutex, {
                                c = count++;
                                sleep(_interval);
                            });
                    });
            } catch(InterruptedException e) {
            }
            cout << "Thread Id(" << _id << ") = " << c << endl;
        }

        synchronized(sig, {
                cout << "Thread Id(" << _id << ") waiting" << endl;
                sig.wait(12000);
                cout << "Thread Id(" << _id << ") finished waiting" << endl;
            });

        cout << "Thread Id(" << _id << ") ended" << endl;
    };

private:

    int _id;
    int _interval;
    static int count;
    static BaseObject lock;
    static Mutex mutex;
    static RecursiveMutex rmutex;

};

int ThreadTest::count = 0;

BaseObject ThreadTest::lock;
Mutex ThreadTest::mutex;
RecursiveMutex ThreadTest::rmutex;
static int staticfuncId = 0;

void *staticfunc(void *args) {
    int id = staticfuncId++;
    cout << "staticfunc " << id << " started" << endl;
    Thread::sleep(3000);
    cout << "staticfunc " << id << " ended" << endl;
    return NULL;
}

void *faststaticfunc(void *args) {
    cout << "faststaticfunc  started" << endl;
    cout << "faststaticfunc ended" << endl;
    return NULL;
}

class BarrierTest : public Thread {

public:

    BarrierTest(ThreadGroup &group, int id, int interval,
                Barrier *barrier) : Thread(group) {
        _id = id;
        _interval = interval;
        _barrier = barrier;
    };

    void run() {
        cout << "Thread Id(" << _id << ") started" << endl;

        sleep(_interval);
        cout << "Thread Id(" << _id << ") entered barrier" << endl;
        _barrier->wait();
        cout << "Thread Id(" << _id << ") exited barrier" << endl;

        cout << "Thread Id(" << _id << ") ended" << endl;
    };

private:

    int _id;
    int _interval;
    Barrier *_barrier;

};

void doThreadTest() {

    ////////////////////////////////////////////////////////////////////////
    // Thread Test
    cout << "Begin thread test" << endl;

    try {
        for(int i = 0; i < 10; i++) {
            Thread a(faststaticfunc, NULL);
            a.start();
        }
    } catch(Exception e) {
        cout << e.getMessage() << endl;
    }

    ThreadGroup mygroup("My Group");
    ThreadTest t1(mygroup, 1, 200), t2(mygroup, 2, 300), t3(mygroup, 3, 500);
    try {
        t1.start();
        t2.start();
        t3.start();
        t1.join(3000);
        cout << "HeyHey!!" << endl;
        cout << "HoHo!!" << endl;
        Thread::dumpThreadUsage();
        Thread::sleep(15000);
        synchronized(sig, {
                cout << "notify" << endl;
                sig.notify();
            });
        Thread::sleep(2000);
        synchronized(sig, {
                cout << "notify" << endl;
                sig.notify();
            });
        Thread::sleep(2000);
        Thread::dumpThreadUsage();
        synchronized(sig, {
                cout << "notifyAll" << endl;
                sig.notifyAll();
            });

        t2.join();
        t3.join();
    } catch(Exception e) {
        cout << e.getMessage() << endl;
    } catch(...) {
        cout << "huh???" << endl;
    }

    //////////////////////////////////////////////////////////////////////////
    // Thread-run Function Test

    try {
        for(int i = 0; i < 10; i++) {
            Thread a(staticfunc, NULL);
            a.start();
            Thread::dumpThreadUsage();
        }
        Thread::sleep(10000);
    } catch(Exception e) {
        cout << e.getMessage() << endl;
    }
    Thread::dumpThreadUsage();

    //////////////////////////////////////////////////////////////////////////
    // Barrier Test

    Barrier barrier(3);
    BarrierTest b1(mygroup, 1, 2000, &barrier), b2(mygroup, 2, 4000, &barrier),
        b3(mygroup, 3, 6000, &barrier);

    try {
        b1.start();
        b2.start();
        b3.start();
        b1.join();
        b2.join();
        b3.join();
    } catch(Exception e) {
        cout << e.getMessage() << endl;
    }

    cout << "End thread test" << endl;
}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
