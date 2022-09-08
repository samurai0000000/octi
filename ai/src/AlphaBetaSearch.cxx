/*************************************************************************
 * $Id: AlphaBetaSearch.cxx,v 1.14 2001/08/25 09:16:56 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/25 09:16:56 $
 * $Locker:  $
 * $Revision: 1.14 $
 *
 * Created: 10/1/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _AIAPI_IMPL_

#include <iostream>
#include "debug.h"
#include "Error.hxx"
#include "Date.hxx"
#include "Thread.hxx"
#include "AlphaBetaSearch.hxx"

#define START_ALPHA_VAL               TERMINAL_NEGATIVE
#define START_BETA_VAL                TERMINAL_POSITIVE
#define TTE_SET_EXACT(x)              (x)->flag = 0x3
#define TTE_SET_LB(x)                 (x)->flag = 0x1
#define TTE_SET_UB(x)                 (x)->flag = 0x2
#define TTE_IS_LB(x)                  ((x)->flag & 0x1) != 0x0
#define TTE_IS_UP(x)                  ((x)->flag & 0x2) != 0x0
#define TTE_DEPTH(x)                  (x)->depth
#define TTE_LOWERBOUND(x)             (x)->xxx
#define TTE_UPPERBOUND(x)             (x)->yyy
#define TTE_BESTMOVE(x)               (x)->zzz

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(AlphaBetaSearchParameters, SearchParameters, 1);
IMPLEMENT_RUNTIME_DISCOVERABLE(AlphaBetaSearch, BaseObject);

const AlphaBetaSearchParameters AlphaBetaSearchParameters::DEFAULT;

const SearchState *AlphaBetaSearch::search(MinMaxSearchType type,
					   SearchState *state,
					   const AlphaBetaSearchParameters *params) {
  ASSERT(state != NULL);
  ASSERT(params != NULL);

  Ptr<SearchState> best;
  int depth;
  unsigned timeStamp;

  if(_speculateThread != NULL) {
    _speculateThread->join(1000);
    if(_speculateOn) {
      Error::warning("unable to  wait for speculate thread to finish");
    }
  }

  // Reset last statistics
  _lastHashHit = 0;
  _lastHashMiss = 0;
  _lastSearchTime = 0;
  _lastDepthReached = 0;
  _lastStatesExpanded = 0;
  _lastStatesEvaluated = 0;
  resetTimerExpirationFlag();
  resetStopSpeculationFlag();

  if(params->timeout > 0) {
    startTimer(params->timeout);
  }

  if(type == MM_SEARCH_MAX) {
    _numMaxCalls++;
  } else {  // MM_SEARCH_MIN
    _numMinCalls++;
  }

  // Start the search
  float f;
  timeStamp = Date::currentTimeMillis();
  for(depth = (params->iterative ? 1 : params->depth);
      depth <= params->depth && !checkStopSearchFlag(MM_TIMER_EXPIRED);
      depth++) {
    f = alphabeta(type, state, best, depth, depth,
		  START_ALPHA_VAL, START_BETA_VAL);
    if(!checkStopSearchFlag(MM_TIMER_EXPIRED)) {
      if(best != NULL) {
	_best = best;
      } else {
	cout << "igore ";
      }
      _lastDepthReached = depth;
      cout << "depth " << depth << ": " <<_best->getDescription()
	   << ": " << f << endl;
    }
  }

  _lastSearchTime = Date::currentTimeMillis() - timeStamp;
  _totalSearchTime += _lastSearchTime;
  _totalStatesExpanded += _lastStatesExpanded;
  _totalStatesEvaluated += _lastStatesEvaluated;
  _tbl.setAllStale();
  //_tbl.clearAll();

  return _best;
}

void AlphaBetaSearch::speculateMax(SearchState *state,
				   const SearchParameters *params) {
  if(_speculateOn) {
    Error::warning("speculate thread already running");
    return;
  }

  resetStopSpeculationFlag();
  resetTimerExpirationFlag();
  _spec_thread_args.alg = this;
  _spec_thread_args.type = MM_SEARCH_MAX;
  _spec_thread_args.state = state;
  _spec_thread_args.params =
    (params == NULL ?
     &AlphaBetaSearchParameters::DEFAULT :
     (const AlphaBetaSearchParameters *) params);

  _speculateThread = new Thread(s_start_speculation, &_spec_thread_args);
  _speculateThread->setPriority(0);
  _speculateThread->start();
}

void AlphaBetaSearch::speculateMin(SearchState *state,
				   const SearchParameters *params) {
  if(_speculateOn) {
    Error::warning("speculate thread already running");
    return;
  }

  resetStopSpeculationFlag();
  resetTimerExpirationFlag();
  _spec_thread_args.alg = this;
  _spec_thread_args.type = MM_SEARCH_MIN;
  _spec_thread_args.state = state;
  _spec_thread_args.params =
    (params == NULL ?
     &AlphaBetaSearchParameters::DEFAULT :
     (const AlphaBetaSearchParameters *) params);

  _speculateThread = new Thread(s_start_speculation, &_spec_thread_args);
  _speculateThread->setPriority(0);
  _speculateThread->start();
}


void *AlphaBetaSearch::s_start_speculation(void *args) {
  spec_thread_args_t *spec = (spec_thread_args_t *) args;
  spec->alg->speculate(spec->type, spec->state, spec->params);
  return NULL;
}

void AlphaBetaSearch::speculate(MinMaxSearchType type,
				SearchState *state,
				const AlphaBetaSearchParameters *params) {
  ASSERT(state != NULL);
  ASSERT(params != NULL);

  Ptr<SearchState> best;
  int depth = 1;
  _speculateOn = TRUE;
  for(; !checkStopSearchFlag(MM_TIMER_EXPIRED | MM_STOP_SPECULATE);depth++) {
    alphabeta(type, state, best, depth, depth,
	      START_ALPHA_VAL, START_BETA_VAL);
    Thread::yield();
  }
  _speculateOn = FALSE;
};

void AlphaBetaSearch::stopSpeculation() {
  MinMaxAlgorithm::stopSpeculation();
  _speculateThread->join();
  _speculateThread = NULL;
}

///////////////////////////////////////////////////////////////////////////
// The Alpha-Beta Prunning algorithm
float AlphaBetaSearch::alphabeta(MinMaxSearchType type,
				 SearchState *state, Ptr<SearchState> &best,
				 int depth, int initdep,
				 float alpha, float beta) {
  ASSERT(state != NULL);

  const TranspositionTableEntry *tte;
  TranspositionTableEntry stte;
  SearchState *current;
  float a, b, g, eval;

  // Fetch from transposition table
  if((tte = _tbl.fetch(state)) != NULL && TTE_DEPTH(tte) >= depth) {
    _lastHashHit++;
    if(TTE_IS_LB(tte)) {
      if(TTE_LOWERBOUND(tte) >= beta && initdep != depth) {
	return TTE_LOWERBOUND(tte);
      }
      alpha = (alpha > TTE_LOWERBOUND(tte) ? alpha : TTE_LOWERBOUND(tte));
    }
    if(TTE_IS_UP(tte)) {
      if(TTE_UPPERBOUND(tte) <= alpha && initdep != depth) {
	return TTE_UPPERBOUND(tte);
      }
      beta = (beta < TTE_UPPERBOUND(tte) ? beta : TTE_UPPERBOUND(tte));
    }
  } else {
    _lastHashMiss++;
  }

  // Evaluate cut-off leaf or terminal states
  if(depth <= 0 || state->isTerminal()) {
    _lastStatesEvaluated++;
    return state->evaluate();
  } 

  // Traverse the child states
  state->expand();
  _lastStatesExpanded++;

  // Reorder according to the hash table
  if(tte) {
    state->prioritize(TTE_BESTMOVE(tte));
  }

  if(type == MM_SEARCH_MAX) {
    g = START_ALPHA_VAL;
    a = alpha;
    for(current = state->getHeadSuccessors();
	current != NULL && (initdep == depth || g < beta) &&
	!checkStopSearchFlag(MM_TIMER_EXPIRED | MM_STOP_SPECULATE);
	current = current->getNext()) {
      eval = alphabeta(MM_SEARCH_MIN, current, best,
		       depth - 1, initdep, a, beta);
      if(eval > g &&
      !checkStopSearchFlag(MM_TIMER_EXPIRED | MM_STOP_SPECULATE)) {
	g = eval;
	TTE_BESTMOVE(&stte) = current->hashCode();
	if(depth == initdep) {
	  best = (SearchState *) current->clone();
	}
      }
      a = (a > g ? a : g);
      current->collapse();
    }
  } else {
    g = START_BETA_VAL;
    b = beta;
    for(current = state->getHeadSuccessors();
	current != NULL && (initdep == depth || g > alpha) &&
	!checkStopSearchFlag(MM_TIMER_EXPIRED | MM_STOP_SPECULATE);
	current = current->getNext()) {
      eval = alphabeta(MM_SEARCH_MAX, current, best,
		       depth - 1, initdep, alpha, b);
      if(eval < g &&
      !checkStopSearchFlag(MM_TIMER_EXPIRED | MM_STOP_SPECULATE)) {
	g = eval;
	TTE_BESTMOVE(&stte) = current->hashCode();
	if(depth == initdep) {
	  best = (SearchState *) current->clone();
	}
      }
      b = (b < g ? b : g);
      current->collapse();
    }
  }

  // Store to transposition table
  if(!checkStopSearchFlag(MM_TIMER_EXPIRED | MM_STOP_SPECULATE)) {
    if(g <= alpha) {
      TTE_SET_UB(&stte);
      TTE_DEPTH(&stte) = depth;
      TTE_UPPERBOUND(&stte) = g;
      _tbl.store(state, &stte);
    } else if(g > alpha && g < beta) {
      TTE_SET_EXACT(&stte);
      TTE_DEPTH(&stte) = depth;
      TTE_UPPERBOUND(&stte) = g;
      TTE_LOWERBOUND(&stte) = g;
      _tbl.store(state, &stte);
    } else if(g >= beta) {
      TTE_SET_LB(&stte);
      TTE_DEPTH(&stte) = depth;
      TTE_LOWERBOUND(&stte) = beta;
      _tbl.store(state, &stte);
    }
  }

  return g;
}

void AlphaBetaSearch::printLastSearchStatistics(ostream &out) const {
  out << "search time = " << ((float) _lastSearchTime / 1000.0)
      << " sec " << endl;
  out << "depth reached = " << _lastDepthReached << endl;
  out << "hash hits = " << _lastHashHit << endl;
  out << "hash misses = " << _lastHashMiss << endl;
  out << "states expanded = " << _lastStatesExpanded << endl;
  out << "states evaluated = " << _lastStatesEvaluated << endl;
}

void AlphaBetaSearch::printTotalStatistics(ostream &out) const {
  out << "total search time = " << ((float) _totalSearchTime / 1000.0) 
      << " sec " << endl;
  out << "total states expanded = " << _totalStatesExpanded << endl;
  out << "total states evaluated = " << _totalStatesEvaluated << endl;
}

__END_NAMESPACE(SELFSOFT);
