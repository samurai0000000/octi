/*************************************************************************
 * $Id: OctiAgent.hxx,v 1.9 2001/08/25 09:17:31 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/25 09:17:31 $
 * $Locker:  $
 * $Revision: 1.9 $
 *
 * Created: 10/1/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _OCTIAGENT_HXX_
#define _OCTIAGENT_HXX_

#include "GameSession.hxx"
#include "AlphaBetaSearch.hxx"
#include "ThreadAlphaBetaSearch.hxx"
#include "MPIAlphaBetaSearch.hxx"
#include "ERSearch.hxx"
#include "OctiPod.hxx"
#include "OctiSquare.hxx"
#include "OctiBoard.hxx"
#include "OctiGame.hxx"
#include "Octi2PlayerGameSearchState.hxx"

#define DEFAULT_ITERATIVE  FALSE
#define DEFAULT_DEPTH      32
#define DEFAULT_TIMEOUT    0ul

__BEGIN_NAMESPACE(SELFSOFT);

enum OctiAgentSearchAlgorithm {
  OASA_SEQ_ALPHABETA,
  OASA_THREAD_ALPHABETA,
  OASA_MPI_ALPHABETA,
  OASA_SEQ_ER
};

class OCTIAPI OctiAgent : public BaseObject, private OctiBoardListener,
			  private OctiGameListener {
  
  DECLARE_RUNTIME_DISCOVERABLE(OctiAgent);

public:

  OctiAgent(OctiAgentSearchAlgorithm alg, int hashsize);
  ~OctiAgent();

  void setParameters(boolean iterative, int depth, unsigned long timeout);
  int execute();

  void setSpeculateOn(boolean speculate) {
    _speculate = speculate;
  }

private:
  
  void gameChanged(OctiBoard *src, OctiGame *oldGame, OctiGame *newGame);
  void boardUpdated(OctiBoard *src, const OctiMove *move, boolean undo);
  void gameResetted(OctiGame *src);
  void gameStarted(OctiGame *src);
  void gameResigned(OctiGame *src);
  void gameUpdated(OctiGame *src);
  void gameOver(OctiGame *src);
  void canUndo(OctiGame *src, int steps);
  void canRedo(OctiGame *src, int steps);
  void canMakeMove(OctiGame *src, boolean b);
  void canCancelMove(OctiGame *src, boolean b);
  void canPauseGame(OctiGame *src, boolean b);
  void canResign(OctiGame *src, boolean b);

private:

  OctiBoard _board;
  OctiGame *_game;

  MinMaxAlgorithm *_alg;
  AlphaBetaSearchParameters _params;
  boolean _speculate;

};

// Inline functions

inline OctiAgent::OctiAgent(OctiAgentSearchAlgorithm alg, int hashsize) {
  switch(alg) {
  case OASA_THREAD_ALPHABETA:
    _alg = new ThreadAlphaBetaSearch();
    ((ThreadAlphaBetaSearch *) _alg)->setPoolSize(2);
    break;
  case OASA_MPI_ALPHABETA:
    // _alg = new MPIAlphaBetaSearch();
    break;
  case OASA_SEQ_ER:
    _alg = new ERSearch();
    break;
  case OASA_SEQ_ALPHABETA:
  default:
    _alg = new AlphaBetaSearch(hashsize);
    break;
  }

  _game = _board.getGame();
  _board.addOctiBoardListener(this);
}

inline OctiAgent::~OctiAgent() {
  if(_alg) {
    delete _alg;
  }
  _board.removeOctiBoardListener(this);
  if(_game) {
    _game->removeOctiGameListener(this);
  }
}

inline void OctiAgent::setParameters(boolean iterative, int depth,
				     unsigned long timeout) {
  _params.iterative = iterative;
  _params.depth = depth;
  _params.timeout = timeout;
}

__END_NAMESPACE(SELFSOFT);

#endif
