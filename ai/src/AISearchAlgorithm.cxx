/*************************************************************************
 * $Id: AISearchAlgorithm.cxx,v 1.6 2001/08/25 09:16:56 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/25 09:16:56 $
 * $Locker:  $
 * $Revision: 1.6 $
 *
 * Created: 11/7/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _AIAPI_IMPL_

#include "debug.h"
#include "Exception.hxx"
#include "AISearchAlgorithm.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_DISCOVERABLE(SearchState, BaseObject);
IMPLEMENT_RUNTIME_DISCOVERABLE(SearchParameters, BaseObject);
IMPLEMENT_RUNTIME_DISCOVERABLE(MinMaxAlgorithm, BaseObject);

void SearchState::expand() {
    if(!_isExpanded && !isTerminal()) {
        _numSuccessors = 0;
        expandState();
    }

    _isExpanded = TRUE;
}

void SearchState::collapse() {
    SearchState *current = _headSuccessors, *prev;

    while(current != NULL) {
        prev = current;
        current->collapse();
        current = current->_next;
        delete prev;
    }

    _headSuccessors = NULL;
    _tailSuccessors = NULL;
    _isExpanded = FALSE;
}

int SearchState::prioritize(const SearchState *successor) {
    SearchState *current;

    if(successor == NULL || _headSuccessors == NULL || _tailSuccessors == NULL) {
        return -1;
    }

    if(*successor == *_headSuccessors) {
        return 0;  // Do nothing
    }

    if(*successor == *_tailSuccessors) {
        current = _tailSuccessors;
        current->_next = _headSuccessors;
        _tailSuccessors = current->_prev;
        _tailSuccessors->_next = NULL;
        _headSuccessors->_prev = current;
        _headSuccessors = current;
        return 0;
    }

    for(current = _headSuccessors->_next;
        current != NULL; current = current->_next) {
        if(*successor == *current) {
            current->_next->_prev = current->_prev;
            current->_prev->_next = current->_next;
            _headSuccessors->_prev = current;
            current->_next = _headSuccessors;
            _headSuccessors = current;
            return 0;
        }
    }

    return -1;
}

#define SAME_HASH(x, y) (x) == (y)->hashCode()

int SearchState::prioritize(unsigned long hashcode) {
    SearchState *current;

    if(_headSuccessors == NULL || _tailSuccessors == NULL) {
        return -1;
    }

    if(SAME_HASH(hashcode, _headSuccessors)) {
        return 0;  // Do nothing
    }

    if(SAME_HASH(hashcode, _tailSuccessors)) {
        current = _tailSuccessors;
        current->_next = _headSuccessors;
        _tailSuccessors = current->_prev;
        _tailSuccessors->_next = NULL;
        _headSuccessors->_prev = current;
        _headSuccessors = current;
        return 0;
    }

    for(current = _headSuccessors->_next;
        current != NULL; current = current->_next) {
        if(SAME_HASH(hashcode, current)) {
            current->_next->_prev = current->_prev;
            current->_prev->_next = current->_next;
            _headSuccessors->_prev = current;
            current->_next = _headSuccessors;
            _headSuccessors = current;
            return 0;
        }
    }

    return -1;
}

void SearchState::addSuccessors(SearchState *successors) {
    if(successors == NULL) {
        return;
    }
  
    if(_headSuccessors == NULL) {
        _headSuccessors = successors;
        successors->_prev = NULL;
        while(successors != NULL) {
            successors->_parent = this;
            successors->_headSuccessors = successors->_tailSuccessors = NULL;
            _tailSuccessors = successors;
            successors = successors->_next;
            _numSuccessors++;
        }
    } else {
        _tailSuccessors->_next = successors;
        successors->_prev = _tailSuccessors;
        while(successors != NULL) {
            successors->_parent = this;
            successors->_headSuccessors = successors->_tailSuccessors = NULL;
            _tailSuccessors = successors;
            successors = successors->_next;
            _numSuccessors++;
        }
    }
}

MinMaxAlgorithm::MinMaxAlgorithm() {
    _timer.addTimerListener(this);
}

MinMaxAlgorithm::~MinMaxAlgorithm() {
    _timer.removeTimerListener(this);
}

boolean MinMaxAlgorithm::retrieve(SearchState *state) {
    return FALSE;  // Not yet implemented
}

boolean MinMaxAlgorithm::store(SearchState *) {
    return FALSE;  // Not yet implemented
}

void MinMaxAlgorithm::timeTick(TimerEvent *e) {
    _timer.stop();
    _signal |= MM_TIMER_EXPIRED;
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
