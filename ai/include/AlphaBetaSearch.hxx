/*************************************************************************
 * $Id: AlphaBetaSearch.hxx,v 1.11 2001/08/25 09:16:54 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/25 09:16:54 $
 * $Locker:  $
 * $Revision: 1.11 $
 *
 * Created: 10/3/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _ALPHABETASEARCH_HXX_
#define _ALPHABETASEARCH_HXX_

#include "BaseObject.hxx"
#include "Thread.hxx"
#include "AISearchAlgorithm.hxx"
#include "TranspositionTable.hxx"

#define DEFAULT_AB_ITERATIVE     TRUE
#define DEFAULT_AB_DEPTH         1000
#define DEFAULT_AB_TIMEOUT       0ul

__BEGIN_NAMESPACE(SELFSOFT);

/**
 * Parameters to the AlphaBetaSearch algorithm.
 *
 * The following parameters are supported:
 *    - hash table size
 *    - iterative deepening search
 *    - depth of search
 *    - use timeout for cut-off
 */
class AIAPI AlphaBetaSearchParameters : public SearchParameters {

  DECLARE_RUNTIME_SERIALIZABLE(AlphaBetaSearchParameters);

public:

  static const AlphaBetaSearchParameters DEFAULT;

  AlphaBetaSearchParameters();
  ~AlphaBetaSearchParameters();

  boolean iterative;
  int depth;
  int timeout;

};

/**
 * A sequential implementation of the alpha-beta search algorithm.
 */
class AIAPI AlphaBetaSearch : public MinMaxAlgorithm {
  
  DECLARE_RUNTIME_DISCOVERABLE(AlphaBetaSearch);

public:

  AlphaBetaSearch(unsigned long tableSize = 0);
  ~AlphaBetaSearch();

  const SearchState *searchMax(SearchState *state,
			       const SearchParameters *params = NULL);
  const SearchState *searchMin(SearchState *state,
			       const SearchParameters *params = NULL);

  void speculateMax(SearchState *state,
		    const SearchParameters *params = NULL);
  void speculateMin(SearchState *state,
		    const SearchParameters *params = NULL);
  void stopSpeculation();
  

  void printLastSearchStatistics(ostream &out) const;
  void printTotalStatistics(ostream &out) const;
  void resetStatistics();

protected:

  virtual const SearchState *search(MinMaxSearchType type,
				    SearchState *state,
				    const AlphaBetaSearchParameters *params);

  static void *s_start_speculation(void *args);

  virtual void speculate(MinMaxSearchType type,
			 SearchState *state,
			 const AlphaBetaSearchParameters *params);

  virtual float alphabeta(MinMaxSearchType type,
			  SearchState *state, Ptr<SearchState> &best,
			  int depth, int depinit, float alpha, float beta);

  struct spec_thread_args_t {
    AlphaBetaSearch *alg;
    MinMaxSearchType type;
    SearchState *state;
    const AlphaBetaSearchParameters *params;
  };

  struct spec_thread_args_t _spec_thread_args;
  boolean _speculateOn;
  Ptr<Thread> _speculateThread;

protected:
  
  TranspositionTable _tbl;
  Ptr<SearchState> _best;
  
  // Algorithm accounting information
  int _numMinCalls;
  int _numMaxCalls;
  unsigned long _lastHashHit;
  unsigned long _lastHashMiss;
  unsigned long _lastSearchTime;
  unsigned long _lastDepthReached;
  unsigned long _lastStatesExpanded;
  unsigned long _lastStatesEvaluated;
  unsigned long _totalSearchTime;
  unsigned long _totalStatesExpanded;
  unsigned long _totalStatesEvaluated;

};

// Inline functions

inline AlphaBetaSearchParameters::AlphaBetaSearchParameters() {
  iterative = DEFAULT_AB_ITERATIVE;
  depth = DEFAULT_AB_DEPTH;
  timeout = DEFAULT_AB_TIMEOUT;
};

inline AlphaBetaSearchParameters::~AlphaBetaSearchParameters() {

}

inline AlphaBetaSearch::AlphaBetaSearch(unsigned long tableSize)
  : _tbl(tableSize) {
  _best = NULL;
  _speculateThread = NULL;
  _speculateOn = FALSE;
  resetStatistics();
}

inline AlphaBetaSearch::~AlphaBetaSearch() {

}

inline const SearchState *AlphaBetaSearch::searchMax(SearchState *state, const SearchParameters *params) {
  return search(MM_SEARCH_MAX, state,
		(params == NULL ?
		 &AlphaBetaSearchParameters::DEFAULT :
		 (const AlphaBetaSearchParameters *) params));
}

inline const SearchState *AlphaBetaSearch::searchMin(SearchState *state, const SearchParameters *params) {
  return search(MM_SEARCH_MIN, state,
		(params == NULL ?
		 &AlphaBetaSearchParameters::DEFAULT :
		 (const AlphaBetaSearchParameters *) params));
}

inline void AlphaBetaSearch::resetStatistics() {
  _numMinCalls = 0;
  _numMaxCalls = 0;
  _lastHashHit = 0;
  _lastHashMiss = 0;
  _lastSearchTime = 0;
  _lastDepthReached = 0;
  _lastStatesExpanded = 0;
  _lastStatesEvaluated = 0;
  _totalSearchTime = 0;
  _totalStatesExpanded = 0;
  _totalStatesEvaluated = 0;
}

__END_NAMESPACE(SELFSOFT);

#endif
