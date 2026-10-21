/*************************************************************************
 * $Id: AISearchAlgorithm.hxx,v 1.9 2001/08/25 09:16:54 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/25 09:16:54 $
 * $Locker:  $
 * $Revision: 1.9 $
 *
 * Created: 10/3/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _AISEARCHALGORITHM_HXX_
#define _AISEARCHALGORITHM_HXX_

#include <iostream>
#include "aiapi.h"
#include "BaseObject.hxx"
#include "Thread.hxx"
#include "Timer.hxx"

#define TERMINAL_POSITIVE   3.40282346638528860e+37f
#define TERMINAL_NEGATIVE  -3.40282346638528860e+37f
#define MM_TIMER_EXPIRED    0x1
#define MM_STOP_SPECULATE   0x2

__BEGIN_NAMESPACE(SELFSOFT);

/**
 * A base class for storing parameters to search algorithms.
 */
class AIAPI SearchParameters : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(SearchParameters);

public:

    ~SearchParameters();

};

/**
 * A generic class representing a search state.
 *
 * There are many uses of a search state by different algorithms under
 * different requirements. This interface provides a general framework
 * for an implementation class. It is general enough to accomodate most
 * of the problem domains that may be encountered.
 *
 * The assumption made here is that each state is "expandable". Meaning
 * that it has successor states. Further more, the successors have their
 * own successors upon their own expansion. This results in a tree
 * view of the state space.
 *
 * This implementation uses a doubly linked list for successor states. It
 * is therefore optimized for sequential access to child states. I.e. the
 * normal usage is:
 *     SearchState *s = somefunc();  // somefunc() returns a state object
 *     SearchState *c;
 *     s->expand();
 *     c = s->getHeadSuccessors();
 *     while(c != NULL) {
 *       ... do work here ...
 *       c = c->getNext();
 *     }
 *
 * Hashing:  Three methods are relevant used:
 *           hashCode()
 *           hashChecksum()
 */
class AIAPI SearchState : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(SearchState);

protected:

    SearchState();

public:

    ~SearchState();

    void expand();     // Expand this state by one level
    void collapse();   // Collapse this state by freeing all the leaf nodes

    // prioritize() reorders the list by positioning the sucessor state as
    // the "head" successor and leaving the rest of the list 'intact'.
    int prioritize(const SearchState *successor);
    int prioritize(unsigned long hashcode);

    SearchState *getParent();   // get the parent state
    SearchState *getNext();     // get the next (sibling) state
    SearchState *getPrev();     // get the previous (sibling) state
    SearchState *getHeadSuccessors();  // get the head (highest priority) successor
    SearchState *getTailSuccessors();  // get the tail (lowest priority) successor
    int getNumSuccessors() const;  // get the number of successors of this state

    virtual const char *getDescription() const = 0;  // Description of state
    virtual boolean isTerminal() const = 0;  // Override for terminal testing
    virtual float evaluate() const = 0;      // Override for evaluation func
    virtual void printState(ostream &out) const= 0;

    virtual boolean operator==(const BaseObject &object) const = 0;
    virtual boolean operator!=(const BaseObject &object) const = 0;
    virtual BaseObject *clone() const = 0;  // duplicate this state (IMPORTANT)

    // The hashCode() and hashChecksum() methods are used by AI search
    // algorithms to store data associated with a state into a temporary
    // storage for usage in later pipelines. For example, it is used by
    // many minimax algorithms to maintain a transposition table.
    // NOTE:
    //      hashCode() is required to be implemented.
    //      hashChecksum() is optional. When used, it helps the algorithm
    //      to reduce the chance of a collision.
    virtual unsigned long hashCode() const = 0;
    virtual word32 hashChecksum() const;

protected:

    // The expanState function is called whenever one of the getXXX functions
    // are called. The deriving class should enumerate the successor state
    // and repeatedly call addSuccessors() to insert them into the list.
    virtual void expandState() = 0;
    void addSuccessors(SearchState *successors);

protected:

    boolean _isExpanded;
    int _numSuccessors;
    SearchState *_parent;           // pointer to the parent state
    SearchState *_next;             // pointer to the next sibling state
    SearchState *_prev;             // pointer to the previous sibling state
    SearchState *_headSuccessors;   // pointer to the head of the successors
    SearchState *_tailSuccessors;   // pointer to the tail of the successors

};

/**
 * Interface for general search algorithms.
 */
class AIAPI SearchAlgorithm {

public:

    virtual SearchState *search(SearchState *state,
                                const SearchParameters *params = NULL) = 0;

    virtual void printLastSearchStatistics(ostream &out) const = 0;
    virtual void printTotalStatistics(ostream &out) const = 0;
    virtual void resetStatistics() = 0;

};

/**
 * Interface for minimax-based algorithms.
 *
 * Methods that are requied to be implemented:
 *  searchMax()  - for searching a max move
 *  searchMin()  - for searching a min move
 *
 * Since most applications of minimax algorithms require the use of a
 * timer to cut-off a search effort. It is provided here for convenience
 * and reusability.
 *
 * The speculateMax() and speculateMin() are provided for searching game
 * states when the user application is idle in waiting for user/opponent
 * inputs. It is possible to start a low-priority thread to perform
 * speculative searches. However, it must end after a stopSpeculation() call
 * or a new searchMin() or searchMax(). The default implementation here
 * is to return the function call immediately.
 *
 * NOTE: In the methods searchMax() and searchMin(), a pointer to SerchState
 *       is returned. The algorithm SHOULD provide memory management of
 *       it. Users should NEVER delete that pointer or write to it.
 */
class AIAPI MinMaxAlgorithm : public BaseObject, public TimerListener {

    DECLARE_RUNTIME_DISCOVERABLE(MinMaxAlgorithm);

public:

    MinMaxAlgorithm();
    virtual ~MinMaxAlgorithm();

    virtual const SearchState *searchMax(SearchState *state,
                                         const SearchParameters *params = NULL)
        = 0;
    virtual const SearchState *searchMin(SearchState *state,
                                         const SearchParameters *params = NULL)
        = 0;

    virtual void speculateMax(SearchState *state,
                              const SearchParameters *params = NULL);
    virtual void speculateMin(SearchState *state,
                              const SearchParameters *params = NULL);
    virtual void stopSpeculation();

    virtual void printLastSearchStatistics(ostream &out) const = 0;
    virtual void printTotalStatistics(ostream &out) const = 0;
    virtual void resetStatistics() = 0;

protected:

    enum MinMaxSearchType {
        MM_SEARCH_MAX,
        MM_SEARCH_MIN
    };

    boolean retrieve(SearchState *state);
    boolean store(SearchState *);

    // A minimax search algorithm usually consumes a lot of CPU cycles
    // and takes a long time to complete. A timer utility is provided
    // in this class for utility purposes.
    // Instead of a hard interrupt design to preempt the current search,
    // a special flag is used for the implementation algorithm to poll
    // on. In strategic places, checkStopSearchFlag() should be called to
    // see if timer has expired or request has come in to stop speculative
    // searches. The return is checked agains the flag passed in.
    // The valid flags are:
    //    MM_TIMER_EXPIRED
    //    MM_STOP_SPECULATE

    void startTimer(unsigned long timeout);
    void resetTimerExpirationFlag();
    void resetStopSpeculationFlag();
    boolean checkStopSearchFlag(int flag) const;

private:

    void timeTick(TimerEvent *e);  // Implements the TimerListener interface

    Timer _timer;        // Timer for cutting off search
    int _signal;

};

// Inline functions

inline SearchParameters::~SearchParameters() {

}

inline SearchState::SearchState() {
    _isExpanded = FALSE;
    _numSuccessors = 0;
    _parent = NULL;
    _next = NULL;
    _prev = NULL;
    _headSuccessors = NULL;
    _tailSuccessors = NULL;
}

inline SearchState::~SearchState() {
    collapse();
}

inline SearchState *SearchState::getParent() {
    return _parent;
}

inline SearchState *SearchState::getNext() {
    return _next;
}

inline SearchState *SearchState::getPrev() {
    return _prev;
}

inline SearchState *SearchState::getHeadSuccessors() {
    return _headSuccessors;
}

inline SearchState *SearchState::getTailSuccessors() {
    return _tailSuccessors;
}

inline int SearchState::getNumSuccessors() const {
    return _numSuccessors;
}

inline word32 SearchState::hashChecksum() const {
    return (word32) hashCode();
}

inline void MinMaxAlgorithm::startTimer(unsigned long timeout) {
    if(!_timer.isRunning()) {
        resetTimerExpirationFlag();
        _timer.setDelay(timeout);
        _timer.start();
    }
}

inline void MinMaxAlgorithm::speculateMax(SearchState *state,
                                          const SearchParameters *params) {
    resetStopSpeculationFlag();
    return;
}

inline void MinMaxAlgorithm::speculateMin(SearchState *state,
                                          const SearchParameters *params) {
    resetStopSpeculationFlag();
    return;
}

inline void MinMaxAlgorithm::stopSpeculation() {
    _signal |= MM_STOP_SPECULATE;
}

inline void MinMaxAlgorithm::resetTimerExpirationFlag() {
    _signal &= (~MM_TIMER_EXPIRED);
}

inline void MinMaxAlgorithm::resetStopSpeculationFlag() {
    _signal &= (~MM_STOP_SPECULATE);
}

inline boolean MinMaxAlgorithm::checkStopSearchFlag(int flag) const {
    return (_signal & flag) != 0x0;
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
