/*************************************************************************
 * $Id: ThreadAlphaBetaSearch.hxx,v 1.4 2001/08/24 03:01:44 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:01:44 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created:  11/3/00 by Charles Chiou
 * Comments: Parallel Alpha-Beta search algorithm using pthreads.
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _THREADALPHABETASEARCH_HXX_
#define _THREADALPHABETASEARCH_HXX_

#include "AlphaBetaSearch.hxx"
#include "ThreadWorkerPool.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class AIAPI ThreadAlphaBetaSearch : public AlphaBetaSearch,
				    private WorkUnit {

  DECLARE_RUNTIME_DISCOVERABLE(ThreadAlphaBetaSearch);

public:

  ThreadAlphaBetaSearch();
  ~ThreadAlphaBetaSearch();

  // The setPoolSize() method should be called only once after the object
  // is constructed. Subsequent calls have no effect.
  void setPoolSize(int poolSize);

protected:

  enum ThreadAlphaBetaCallType {
    THR_AB_CALL_MAX,
    THR_AB_CALL_MIN,
  };

  typedef struct _WorkUnitArg {
    // Argument data
    ThreadAlphaBetaCallType type;
    SearchState *state;
    int depth;
    int cutoffDepth;
    float alpha;
    float beta;
    Ptr<SearchState> *path;
    // Return data
    float eval;
  } WorkUnitArg;

  float maxValue(SearchState *state, int depth, int cutoffDepth,
		 float alpha, float beta, Ptr<SearchState> path[]);
  float minValue(SearchState *state, int depth, int cutoffDepth,
		 float alpha, float beta, Ptr<SearchState> path[]);
private:

  // Implement the WorkUnit interface
  void doWork(void *args);

private:

  ThreadWorkerPool *_pool;

};

// Inline functions

inline ThreadAlphaBetaSearch::ThreadAlphaBetaSearch() {
  _pool = NULL;
}

inline ThreadAlphaBetaSearch::~ThreadAlphaBetaSearch() {
  if(_pool) {
    delete _pool;
  }
}

__END_NAMESPACE(SELFSOFT);

#endif
