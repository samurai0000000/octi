/*************************************************************************
 * $Id: OctiGame.hxx,v 1.8 2001/08/09 04:30:52 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 04:30:52 $
 * $Locker:  $
 * $Revision: 1.8 $
 *
 * Created: 10/1/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _OCTIGAME_HXX_
#define _OCTIGAME_HXX_

#include "BaseObject.hxx"
#include "Dlist.hxx"
#include "Event.hxx"
#include "GameExceptions.hxx"
#include "OctiDefs.hxx"
#include "OctiPod.hxx"
#include "OctiSquare.hxx"
#include "OctiMove.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class OctiGame;

/**
 * Listener interface for objects that are interested in receiving
 * events from the game control.
 */
class OCTIAPI OctiGameListener : public EventListener {

public:

  virtual void gameResetted(OctiGame *src) = 0;
  virtual void gameStarted(OctiGame *src) = 0;
  virtual void gameResigned(OctiGame *src) = 0;
  virtual void gameUpdated(OctiGame *src) = 0;
  virtual void gameOver(OctiGame *src) = 0;
  virtual void canUndo(OctiGame *src, int steps) = 0;
  virtual void canRedo(OctiGame *src, int steps) = 0;
  virtual void canMakeMove(OctiGame *src, boolean b) = 0;
  virtual void canCancelMove(OctiGame *src, boolean b) = 0;
  virtual void canPauseGame(OctiGame *src, boolean b) = 0;
  virtual void canResign(OctiGame *src, boolean b) = 0;

};

/**
 * OctiHop is a utility container class for representing 'hops' or 'jumps'
 * over squares (and pods contained therein). The x, y member variables
 * denote the square location.
 */
class OCTIAPI OctiHop {

public:

  OctiHop();
  OctiHop(int x, int y);

  int x;
  int y;

  boolean operator==(const OctiHop &other) const;

};

/**
 * Interface to OCTI game control.
 * It is used to modify the states of an OCTI game.
 */
class OCTIAPI OctiGame {

public:

  virtual OctiGameMode getMode() const = 0;
  virtual boolean useSuperProngs() const = 0;
  virtual boolean useEdgelessBoard() const = 0;
  
  virtual void addOctiGameListener(OctiGameListener *l) = 0;
  virtual void removeOctiGameListener(OctiGameListener *l) = 0;

  virtual void reset() = 0;
  virtual void start(OctiPlayer player = PLAYER_A) = 0;
  virtual void randomize() = 0;
  virtual void resign(OctiPlayer player) = 0;
  virtual void move(OctiMove &move) throw(IllegalMoveException) = 0;
  virtual void move(const char *move) throw(IllegalMoveException) = 0;
  virtual void move(OctiPlayer player, OctiMove &move) throw(IllegalMoveException) = 0;
  virtual void move(OctiPlayer player, const char *move) throw(IllegalMoveException) = 0;
  virtual void undo() = 0;
  virtual void redo() = 0;

  virtual boolean isStarted() const = 0;
  virtual boolean isGameOver() const = 0;
  virtual OctiPlayer getStartPlayer() const = 0;
  virtual OctiPlayer getPlayerToMove() const = 0;
  virtual int getMoveSize() const = 0;
  virtual Slist<OctiMove> getAllMoves() const = 0;

  virtual int getNumFreePods(OctiPlayer player) const = 0;
  virtual int getNumCapturedPods(OctiPlayer player) const = 0;
  virtual int getNumFreeProngs(OctiPlayer player) const = 0;
  virtual int getNumFreeSuperProngs(OctiPlayer player) const = 0;

  virtual int isUndoAllowed() const = 0;
  virtual int isRedoAllowed() const = 0;
  virtual boolean isMakeMoveAllowed() const = 0;
  virtual boolean isCancelMoveAllowed() const = 0;
  virtual boolean isPauseGameAllowed() const = 0;
  virtual boolean isResignAllowed() const = 0;

  // Prepatory move operations used by GUI
  virtual void prepareAddPod(int x, int y) throw (IllegalMoveException) = 0;
  virtual void prepareAddProng(int x, int y, OctiPod pod, OctiDir dir) throw(IllegalMoveException) = 0;
  virtual void prepareReposition(int x, int y, OctiPod pod, OctiDir srcDir, OctiDir tgtDir) throw(IllegalMoveException) = 0;
  virtual void prepareHops(int x, int y, OctiPod pod, Dlist<OctiHop> hops) throw(IllegalMoveException) = 0;
  virtual void prepareCapture(int x, int y) = 0;
  virtual void commitPreparedMove() throw(IllegalMoveException) = 0;
  virtual void rollbackPreparedMove() throw(IllegalMoveException) = 0;
  virtual boolean isPrepareLocked() const = 0;

};

// Inline functions

inline OctiHop::OctiHop() {
  x = y = -1;
}

inline OctiHop::OctiHop(int xx, int yy) : x(xx), y(yy) {

}

inline boolean OctiHop::operator==(const OctiHop &other) const {
  return x == other.x && y == other.y;
}

__END_NAMESPACE(SELFSOFT);

#endif
