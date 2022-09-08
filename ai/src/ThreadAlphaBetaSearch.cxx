/*************************************************************************
 * $Id: ThreadAlphaBetaSearch.cxx,v 1.4 2001/08/24 03:01:45 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:01:45 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created: 11/3/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _AIAPI_IMPL_

#include "ThreadAlphaBetaSearch.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_DISCOVERABLE(ThreadAlphaBetaSearch, AlphaBetaSearch);

void ThreadAlphaBetaSearch::setPoolSize(int poolSize) {
    if(_pool == NULL) {
        _pool = new ThreadWorkerPool(poolSize);
    }
}

float ThreadAlphaBetaSearch::maxValue(SearchState *state, int depth, int cutoffDepth,
                                      float alpha, float beta, Ptr<SearchState> path[]) {
    /*
      ASSERT(state != NULL);
      SearchState *current;
      float eval;

      if(depth > cutoffDepth || state->isTerminal()) {
      eval = state->evaluate();
      _lastStatesEvaluated++;
      return eval;
      }

      Ptr<SearchState> localPath[MAX_DEPTH];

      state->expand();
      _lastStatesExpanded++;
      current = state->getHeadSuccessors();
      while(current != NULL && !isTimeUp()) {
      if(_pool != NULL) {
      WorkUnitArg arg;
      arg.type = THR_AB_CALL_MIN;
      arg.state = state;
      arg.depth = depth + 1;
      arg.cutoffDepth = cutoffDepth;
      arg.alpha = alpha;
      arg.beta = beta;
      arg.path = localPath;
      #ifdef _DEBUG
      cout << "pool->doWork()" << endl;
      #endif
      _pool->doWork(*this, &arg);
      eval = arg.eval;
      } else {
      eval = minValue(current, depth + 1, cutoffDepth,
      alpha, beta, localPath);
      }

      if(eval > alpha) {
      alpha = eval;
      if(alpha < beta) {
      localPath[depth] = (SearchState *) current->clone();
      for(int p = depth; p <= cutoffDepth; p++) {
	  path[p] = localPath[p];
      }
      }
      }
    
      if(alpha >= beta) {
      current->collapse();
      return beta;
      }
    
      current->collapse();
      current = current->getNext();
      }
    */
    return alpha;
}

float ThreadAlphaBetaSearch::minValue(SearchState *state, int depth, int cutoffDepth,
                                      float alpha, float beta, Ptr<SearchState> path[]) {
    /*
      SearchState *current;
      float eval;

      if(depth > cutoffDepth || state->isTerminal()) {
      eval = state->evaluate();
      _lastStatesEvaluated++;
      return eval;
      }

      Ptr<SearchState> localPath[MAX_DEPTH];
  
      state->expand();
      _lastStatesExpanded++;
      current = state->getHeadSuccessors();
      while(current != NULL && !isTimeUp()) {
      if(_pool != NULL) {
      WorkUnitArg arg;
      arg.type = THR_AB_CALL_MIN;
      arg.state = state;
      arg.depth = depth + 1;
      arg.cutoffDepth = cutoffDepth;
      arg.alpha = alpha;
      arg.beta = beta;
      arg.path = localPath;
      #ifdef _DEBUG
      cout << "pool->doWork()" << endl;
      #endif
      _pool->doWork(*this, &arg);
      eval = arg.eval;
      } else {
      eval = maxValue(current, depth + 1, cutoffDepth,
      alpha, beta, localPath);
      }

      if(eval < beta) {
      beta = eval;
      if(beta > alpha) {
      localPath[depth] = (SearchState *) current->clone();
      for(int p = depth; p <= cutoffDepth; p++) {
	  path[p] = localPath[p];
      }
      }
      }

      if(beta <= alpha) {
      current->collapse();
      return alpha;
      }

      current->collapse();
      current = current->getNext();
      }
    */
    return beta;
}

// Called by a thread from the free thread pool
void ThreadAlphaBetaSearch::doWork(void *args) {
    ASSERT(args != NULL);

#ifdef _DEBUG
    cout << "doWork()" << endl;
#endif

    WorkUnitArg *thrABarg = (WorkUnitArg *) args;
    if(thrABarg->type == THR_AB_CALL_MAX) {
        thrABarg->eval =
            maxValue(thrABarg->state, thrABarg->depth, thrABarg->cutoffDepth,
                     thrABarg->alpha, thrABarg->beta, thrABarg->path);
    } else {
        thrABarg->eval =
            minValue(thrABarg->state, thrABarg->depth, thrABarg->cutoffDepth,
                     thrABarg->alpha, thrABarg->beta, thrABarg->path);
    }
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
