/*************************************************************************
 * $Id: OctiGameImpl.hxx,v 1.8 2001/08/09 04:30:52 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 04:30:52 $
 * $Locker:  $
 * $Revision: 1.8 $
 *
 * Created: 11/13/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _OCTIGAMEIMPL_HXX_
#define _OCTIGAMEIMPL_HXX_

#include "BaseObject.hxx"
#include "Stack.hxx"
#include "Dlist.hxx"
#include "Event.hxx"
#include "OctiPod.hxx"
#include "OctiSquare.hxx"
#include "OctiMove.hxx"
#include "OctiBoard.hxx"
#include "OctiGame.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class OCTIAPI OctiBoard;

/**
 * Base implementation class of the OctiGame interface. The common
 * methods shared by 2- and 4-player games are defined here.
 */
class OCTIAPI OctiGameBaseImpl : public OctiGame, public BaseObject {

  DECLARE_RUNTIME_DISCOVERABLE(OctiGameBaseImpl);

protected:

  OctiGameBaseImpl();

public:

  OctiGameBaseImpl(OctiBoard *board, boolean useSuperProngs = FALSE,
		   boolean edgeless = FALSE);
  ~OctiGameBaseImpl();

  virtual OctiGameMode getMode() const = 0;
  boolean useSuperProngs() const;
  boolean useEdgelessBoard() const;

  void addOctiGameListener(OctiGameListener *l);
  void removeOctiGameListener(OctiGameListener *l);

  void reset();
  void start(OctiPlayer player);
  virtual void randomize() = 0;
  void resign(OctiPlayer player);
  void move(OctiMove &move) throw(IllegalMoveException);
  void move(const char *move) throw(IllegalMoveException);
  void move(OctiPlayer player, OctiMove &move) throw(IllegalMoveException);
  void move(OctiPlayer player, const char *move) throw(IllegalMoveException);
  void undo();
  void redo();

  void prepareAddPod(int x, int y) throw (IllegalMoveException);
  void prepareAddProng(int x, int y, OctiPod pod, OctiDir dir) throw(IllegalMoveException);
  void prepareReposition(int x, int y, OctiPod pod, OctiDir srcDir, OctiDir tgtDir) throw(IllegalMoveException);
  void prepareHops(int x, int y, OctiPod pod, Dlist<OctiHop> hops) throw(IllegalMoveException);
  void prepareCapture(int x, int y);
  void commitPreparedMove() throw(IllegalMoveException);
  void rollbackPreparedMove() throw(IllegalMoveException);
  boolean isPrepareLocked() const;

  int isUndoAllowed() const;
  int isRedoAllowed() const;
  boolean isMakeMoveAllowed() const;
  boolean isCancelMoveAllowed() const;
  boolean isPauseGameAllowed() const;
  boolean isResignAllowed() const;

  boolean isStarted() const;
  boolean isGameOver() const;
  OctiPlayer getStartPlayer() const;
  OctiPlayer getPlayerToMove() const;
  int getMoveSize() const;
  Slist<OctiMove> getAllMoves() const;

  int getNumFreePods(OctiPlayer player) const;
  int getNumCapturedPods(OctiPlayer player) const;
  int getNumFreeProngs(OctiPlayer player) const;
  int getNumFreeSuperProngs(OctiPlayer player) const;

protected:

  virtual void initGameCondition() = 0;
  virtual boolean checkIsGameOver() const = 0;
  virtual void nextPlayer() = 0;
  virtual void prevPlayer() = 0;
  virtual boolean liberate(OctiPlayer player, Ptr<OctiMoveOp> op, OctiPod &pod) = 0;

private:

  void ensureGameCondition(OctiPlayer player) const throw(IllegalMoveException);
  void makeMove(OctiPlayer player, OctiMove &move, boolean redo)
    throw(IllegalMoveException);
  void doAddRemoveProng(OctiPlayer player, Ptr<OctiMoveOp> op)
    throw(IllegalMoveException);
  void doAddPod(OctiPlayer player, Ptr<OctiMoveOp> op)
    throw(IllegalMoveException);
  void doMovePod(OctiPlayer player, Ptr<OctiMoveOp> op)
    throw(IllegalMoveException);
  void undoAddRemoveProng(OctiPlayer player, Ptr<OctiMoveOp> op)
    throw(IllegalMoveException);
  void undoAddPod(OctiPlayer player, Ptr<OctiMoveOp> op)
    throw(IllegalMoveException);
  void undoMovePod(OctiPlayer player, Ptr<OctiMoveOp> op)
    throw(IllegalMoveException);

  void canUndo(boolean b);
  void canRedo(boolean b);

  void ensureLock() const throw(IllegalMoveException);
  void lockPrepare() throw(IllegalMoveException);
  void unlockPrepare() throw(IllegalMoveException);
  
protected:

  OctiBoard *_board;
  boolean _useSuperProngs;
  boolean _edgeless;
  Stack<OctiMove> _moveStack;
  Stack<OctiMove> _redoStack;
  boolean _isStarted;
  boolean _isOver;
  OctiPlayer _startPlayer;
  OctiPlayer _playerToMove;

  int _freePods[4];
  int _capPods[4];
  int _freeProngs[4];
  int _freeSuperProngs[4];

  EventListenerList _gameListeners;
  Slist<OctiSquare *> _cannotJumpOverSquares;
  Slist<OctiPod> _ghostPods;
  boolean _prepare;
  Dlist<Ptr<OctiMoveOp> > _preparedMoveOps;
  OctiMove _preparedMove;

};


/**
 * Implementation class of OCTI 2 player game.
 */
class OCTIAPI Octi2PlayerGame : public OctiGameBaseImpl {

  DECLARE_RUNTIME_SERIALIZABLE(Octi2PlayerGame);

protected:

  Octi2PlayerGame();

public:

  Octi2PlayerGame(OctiBoard *board, boolean useSuperProngs = FALSE,
		  boolean edgeless = FALSE);
  ~Octi2PlayerGame();

  void randomize();
  OctiGameMode getMode() const;

protected:

  void initGameCondition();
  boolean checkIsGameOver() const;
  void nextPlayer();
  void prevPlayer();
  boolean liberate(OctiPlayer player, Ptr<OctiMoveOp> op, OctiPod &pod);

};


/**
 * Implementation class of OCTI 4 player game.
 */
class OCTIAPI Octi4PlayerGame : public OctiGameBaseImpl {

  DECLARE_RUNTIME_SERIALIZABLE(Octi4PlayerGame);

protected:

  Octi4PlayerGame();

public:

  Octi4PlayerGame(OctiBoard *board, boolean useSuperProngs = FALSE,
		  boolean edgeless = FALSE);
  ~Octi4PlayerGame();

  void randomize();
  OctiGameMode getMode() const;

protected:

  void initGameCondition();
  boolean checkIsGameOver() const;
  void nextPlayer();
  void prevPlayer();
  boolean liberate(OctiPlayer player, Ptr<OctiMoveOp> op, OctiPod &pod);

};

// Inline functions

inline OctiGameBaseImpl::OctiGameBaseImpl() {
  _board = NULL;
  _useSuperProngs = 0;
  _edgeless = 0;
}

inline OctiGameBaseImpl::OctiGameBaseImpl(OctiBoard *board,
					  boolean useSuperProngs,
					  boolean edgeless) {
  _board = board;
  _useSuperProngs = useSuperProngs;
  _edgeless = edgeless;
}

inline OctiGameBaseImpl::~OctiGameBaseImpl() {

}

inline boolean OctiGameBaseImpl::useSuperProngs() const {
  return _useSuperProngs;
}

inline boolean OctiGameBaseImpl::useEdgelessBoard() const {
  return _edgeless;
}

inline void OctiGameBaseImpl::addOctiGameListener(OctiGameListener *l) {
  _gameListeners.addEventListener(l);
}

inline void OctiGameBaseImpl::removeOctiGameListener(OctiGameListener *l) {
  _gameListeners.removeEventListener(l);
}

inline boolean OctiGameBaseImpl::isStarted() const {
  return _isStarted;
}

inline boolean OctiGameBaseImpl::isGameOver() const {
  return _isOver;
}

inline OctiPlayer OctiGameBaseImpl::getStartPlayer() const {
  return _startPlayer;
}

inline OctiPlayer OctiGameBaseImpl::getPlayerToMove() const {
  return _playerToMove;
}

inline void OctiGameBaseImpl::move(OctiMove &move) throw(IllegalMoveException) {
  makeMove(_playerToMove, move, FALSE);
  _board->boardUpdated(&move, FALSE);
}

inline void OctiGameBaseImpl::move(const char *move) throw(IllegalMoveException) {
  OctiMove m(move);
  makeMove(_playerToMove, m, FALSE);
  _board->boardUpdated(&m, FALSE);
}

inline void OctiGameBaseImpl::move(OctiPlayer player, OctiMove &move)
  throw(IllegalMoveException) {
  makeMove(player, move, FALSE);
  _board->boardUpdated(&move, FALSE);
}

inline void OctiGameBaseImpl::move(OctiPlayer player, const char *move)
  throw(IllegalMoveException) {
  OctiMove m(move);
  makeMove(player, m, FALSE);
  _board->boardUpdated(&m, FALSE);
}

inline int OctiGameBaseImpl::isUndoAllowed() const {
  return _moveStack.size();
}

inline int OctiGameBaseImpl::isRedoAllowed() const {
  return _redoStack.size();
}

inline boolean OctiGameBaseImpl::isMakeMoveAllowed() const {
  return _isStarted && _prepare;
}

inline boolean OctiGameBaseImpl::isCancelMoveAllowed() const {
  return _isStarted && _prepare;
}

inline boolean OctiGameBaseImpl::isPauseGameAllowed() const {
  return _isStarted;
}

inline boolean OctiGameBaseImpl::isResignAllowed() const {
  return _isStarted;
}

inline int OctiGameBaseImpl::getMoveSize() const {
  return _moveStack.size();
}

inline Slist<OctiMove> OctiGameBaseImpl::getAllMoves() const {
  // this is very inefficient...
  // don't call too often!
  Stack<OctiMove> dup(_moveStack);
  Slist<OctiMove> list;
  OctiMove move;
  while(!dup.isEmpty()) {
    dup.pop(move);
    list.prepend(move);
  }
  return list;
}

inline int OctiGameBaseImpl::getNumFreePods(OctiPlayer player) const {
  return _freePods[(int) player - 1];
}

inline int OctiGameBaseImpl::getNumCapturedPods(OctiPlayer player) const {
  return _capPods[(int) player - 1];
}

inline int OctiGameBaseImpl::getNumFreeProngs(OctiPlayer player) const {
  return _freeProngs[(int) player - 1];
}

inline int OctiGameBaseImpl::getNumFreeSuperProngs(OctiPlayer player) const {
  return _freeSuperProngs[(int) player - 1];
}

inline Octi2PlayerGame::Octi2PlayerGame() : OctiGameBaseImpl() {

}

inline Octi2PlayerGame::Octi2PlayerGame(OctiBoard *board,
					boolean useSuperProngs,
					boolean edgeless)
  : OctiGameBaseImpl(board, useSuperProngs, edgeless) {

}

inline Octi2PlayerGame::~Octi2PlayerGame() {

}

inline Octi4PlayerGame::Octi4PlayerGame() : OctiGameBaseImpl() {

}

inline OctiGameMode Octi2PlayerGame::getMode() const {
  return GAME_MODE_2;
}

inline Octi4PlayerGame::Octi4PlayerGame(OctiBoard *board,
					boolean useSuperProngs,
					boolean edgeless)
  : OctiGameBaseImpl(board, useSuperProngs, edgeless) {

}

inline Octi4PlayerGame::~Octi4PlayerGame() {

}

inline OctiGameMode Octi4PlayerGame::getMode() const {
  return GAME_MODE_4;
}

__END_NAMESPACE(SELFSOFT);

#endif
