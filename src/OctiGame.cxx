/*************************************************************************
 * $Id: OctiGame.cxx,v 1.10 2001/08/24 03:00:35 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:00:35 $
 * $Locker:  $
 * $Revision: 1.10 $
 *
 * Created: 10/1/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _OCTIAPI_IMPL_

#include "Error.hxx"
#include "OctiDefs.hxx"
#include "OctiGame.hxx"
#include "OctiGameImpl.hxx"

#define MODE_2_INIT_FREE_PODS            7
#define MODE_2_INIT_FREE_PRONGS         25
#define MODE_2_INIT_FREE_SUPERPRONGS     1
#define MODE_4_INIT_FREE_PODS            4
#define MODE_4_INIT_FREE_PRONGS         12
#define MODE_4_INIT_FREE_SUPERPRONGS     1

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_DISCOVERABLE(OctiGameBaseImpl, BaseObject);
IMPLEMENT_RUNTIME_SERIALIZABLE(Octi2PlayerGame, OctiGameBaseImpl, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(Octi4PlayerGame, OctiGameBaseImpl, 1);

void OctiGameBaseImpl::reset() {
  _board->clear();

  _freePods[0] = 0;
  _freePods[1] = 0;
  _freePods[2] = 0;
  _freePods[3] = 0;

  _capPods[0] = 0;
  _capPods[1] = 0;
  _capPods[2] = 0;
  _capPods[3] = 0;
  
  _freeProngs[0] = 0;
  _freeProngs[1] = 0;
  _freeProngs[2] = 0;
  _freeProngs[3] = 0;
  
  _freeSuperProngs[0] = 0;
  _freeSuperProngs[1] = 0;
  _freeSuperProngs[2] = 0;
  _freeSuperProngs[3] = 0;
  
  initGameCondition();

  _isStarted = false;
  _moveStack.clear();
  _redoStack.clear();
  canUndo(TRUE);
  canRedo(TRUE);
  _board->boardUpdated(NULL, FALSE);

  for(int i = _gameListeners.getNumListeners() - 1; i >= 0; i--) {
    OctiGameListener *listener =
      (OctiGameListener *) _gameListeners.getListenerAt(i);
    listener->gameResetted(this);
  }

  _prepare = FALSE;
  _preparedMoveOps.clear();
  _preparedMove = OctiMove();
}

void OctiGameBaseImpl::start(OctiPlayer player) {
  reset();
  _startPlayer = player;
  _playerToMove = player;
  _isStarted = TRUE;
  _isOver = FALSE;

  for(int i = _gameListeners.getNumListeners() - 1; i >= 0; i--) {
    OctiGameListener *listener =
      (OctiGameListener *) _gameListeners.getListenerAt(i);
    listener->gameStarted(this);
  }
}

void OctiGameBaseImpl::resign(OctiPlayer player) {
  for(int i = _gameListeners.getNumListeners() - 1; i >= 0; i--) {
    OctiGameListener *listener =
      (OctiGameListener *) _gameListeners.getListenerAt(i);
    listener->gameResigned(this);
  }
}

void OctiGameBaseImpl::ensureGameCondition(OctiPlayer player) const throw(IllegalMoveException) {
  if(!_isStarted) {
    throw IllegalMoveException("game not started");
  }
  if(_isOver) {
    throw IllegalMoveException("game over");
  }
  if(player != _playerToMove) {
    throw IllegalMoveException("not the player's turn");
  }
}

void OctiGameBaseImpl::makeMove(OctiPlayer player, OctiMove &move, boolean redo) throw(IllegalMoveException) {
  int i, sourceX, sourceY;
  IllegalMoveException exception;
  boolean failedMakeMove;
  Ptr<OctiMoveOp> op;
  Dlist<Ptr<OctiMoveOp> > ops;

  ensureGameCondition(player);
  ensureLock();

  ops = move.getOps();
  sourceX = move.getSourceX();
  sourceY = move.getSourceY();
  _ghostPods.clear();

  failedMakeMove = FALSE;
  Ptr<DlistIterator<Ptr<OctiMoveOp> > > iterator =
    (DlistIterator<Ptr<OctiMoveOp> > *) ops.getIterator();
  for(iterator->first(); iterator->isNotNull(); iterator->next()) {
    op = *iterator->current();

    if(sourceX == op->x0 && sourceY == op->y0) {
      _cannotJumpOverSquares.clear();
    }
	    
    if(op->prongAddDir != DIR_UNDEF) {
      try {
	doAddRemoveProng(player, op);
      } catch(IllegalMoveException e) {
	failedMakeMove = TRUE;
	exception = e;
	break;
      }
    } else if(op->x0 == op->x1 && op->y0 == op->y1) {
      try {
	doAddPod(player, op);
      } catch(IllegalMoveException e) {
	failedMakeMove = TRUE;
	exception = e;
	break;
      }
    } else {
      try {
	doMovePod(player, op);
      } catch(IllegalMoveException e) {
	failedMakeMove = TRUE;
	exception = e;
	break;
      }
    }
  }

  if(failedMakeMove) {
    // Undo the ops
    while(iterator->isPrevNotNull()) {
      op = *iterator->prev();

      if(op->prongAddDir != DIR_UNDEF) {
	try {
	  undoAddRemoveProng(player, op);
	} catch(IllegalMoveException e) {
	  Error::warning("Unrecoverable error: %s\n", e.getMessage());
	}
      } else if(op->x0 == op->x1 && op->y0 == op->y1) {
	try {
	  undoAddPod(player, op);
	} catch(IllegalMoveException e) {
	  Error::warning("Unrecoverable error: %s\n", e.getMessage());
	}
      } else {
	try {
	  undoMovePod(player, op);
	} catch(IllegalMoveException e) {
	  Error::warning("Unrecoverable error: %s\n", e.getMessage());
	}
      }
    }

    String msg;
    msg.format("%s: %s", move.getNotation(), exception.getMessage());
    throw IllegalMoveException(msg);
  }

  if(!redo) {
    nextPlayer(); // Switch player's turn
    _redoStack.clear();
  } else {
    prevPlayer();  // Switch to previous player's turn
  }
  _moveStack.push(move);
  canRedo(TRUE);
  canUndo(TRUE);

  if(checkIsGameOver()) {
    _isOver = TRUE;
    for(i = _gameListeners.getNumListeners() - 1; i >= 0; i--) {
      OctiGameListener *listener =
	(OctiGameListener *) _gameListeners.getListenerAt(i);
      listener->gameOver(this);
    }
  } else {
    for(i = _gameListeners.getNumListeners() - 1; i >= 0; i--) {
      OctiGameListener *listener =
	(OctiGameListener *) _gameListeners.getListenerAt(i);
      listener->gameUpdated(this);
    }
  }
}

void OctiGameBaseImpl::doAddRemoveProng(OctiPlayer player, Ptr<OctiMoveOp> op)
  throw(IllegalMoveException) {
  OctiSquare *square;
  OctiPod *pod;
  int numPods;
  int testFreeProngs, testFreeSuperProngs;
  boolean passRemoveProngTest;

  square = &_board->squareAt(op->x0, op->y0);
  numPods = square->getNumPods();

  // Check if the square is empty
  if(square->isEmpty()) {
    String msg;
    msg.format("square[%d, %d] is empty", op->x0 + 1, op->y0 + 1);
    throw IllegalMoveException(msg);
  }	

  for(int i = numPods - 1; i >= 0; i--) {
    pod = square->getPodAt(i);

    // Match the prong configuration
    if(((int) pod->getType() == (int) player) && pod->hasSameConfig(op->pod)) {
      // Check if the add and remove prong operation is valid
      if(pod->prongAt(op->prongAddDir) == NO_PRONG) {
	testFreeProngs = _freeProngs[player - 1];
	testFreeSuperProngs = _freeSuperProngs[player - 1];
	// Test if there are prongs available for the add
	passRemoveProngTest = FALSE;
	if(op->prongRemoveDir != DIR_UNDEF) {
	  if(pod->prongAt(op->prongRemoveDir) != 0) {
	    switch(pod->prongAt(op->prongRemoveDir)) {
	    case NORMAL_PRONG:
	      testFreeProngs++;
	      break;
	    case SUPER_PRONG:
	      testFreeSuperProngs++;
	      break;
	    }
	    passRemoveProngTest = TRUE;
	  }
	} else {
	  passRemoveProngTest = TRUE;
	}
		    
	// Continue test and if passed, perform the operation
	if(passRemoveProngTest &&
	   ((op->addSuperProng && testFreeSuperProngs > 0) ||
	    (!op->addSuperProng && testFreeProngs > 0))) {
	  if(op->prongRemoveDir != DIR_UNDEF) {
	    pod->removeProng(op->prongRemoveDir);
	    _freeProngs[player - 1] = testFreeProngs;
	    _freeSuperProngs[player - 1] = testFreeSuperProngs;
	  }
	  pod->addProng(op->prongAddDir, op->addSuperProng);
	  if(op->addSuperProng) {
	    _freeSuperProngs[player - 1]--;
	  } else {
	    _freeProngs[player - 1]--;
	  }
	  return;  // All passed! Return the function.
	}
      }
    }
  }

  throw IllegalMoveException("no valid pods found in square");
}

void OctiGameBaseImpl::doAddPod(OctiPlayer player, Ptr<OctiMoveOp> op)
  throw(IllegalMoveException) {
  OctiSquare *square;
  OctiSquareType type;
  OctiPod pod;

  square = &_board->squareAt(op->x0, op->y0);
  type = square->getType();
  if(type == NEUTRAL_SQUARE) {
    throw IllegalMoveException("square is neutral");
  } else if((int) type == (int) player) {
    if(square->isCaptured()) {
      throw IllegalMoveException("square has been captured");
    } else if(_freePods[player - 1] == 0) {
      throw IllegalMoveException("no more pods left");
    } else {
      _freePods[player - 1]--;
      square->addPod(OctiPod((OctiPodType) player));
    }
  } else if((int) type != (int) player) {
    if(!square->isCaptured()) {
      throw IllegalMoveException("not a friendly square");
    } else {
      if(liberate(player, op, pod)) {
	square->addPod(OctiPod((OctiPodType) player));
      } else {
	if(_freePods[player - 1] == 0) {
	  throw IllegalMoveException("no more pods left");
	}
	_freePods[player - 1]--;
	square->addPod(OctiPod((OctiPodType) player));
      }
    }
  }
}

void OctiGameBaseImpl::doMovePod(OctiPlayer player, Ptr<OctiMoveOp> op)
  throw(IllegalMoveException) {
  OctiSquare *startSquare;
  OctiSquare *targetSquare;
  OctiSquare *jumpSquare = NULL;
  OctiPod movePod, capPod;
  int numPods, numCapPods;
  int xdisplacement, ydisplacement;
  int i, j, k;
  OctiProngType t;
  OctiDir dir = DIR_UNDEF;

  startSquare = &_board->squareAt(op->x0, op->y0);
  targetSquare = &_board->squareAt(op->x1, op->y1);

  // Check if the start square is empty
  if(startSquare->isEmpty()) {
    String msg;
    msg.format("square [%d, %d] is empty", op->x0 + 1, op->y0 + 1);
    throw IllegalMoveException(msg);
  }

  // Check if the end square is occupied by enemy
  if(targetSquare->hasEnemyOf(player)) {
    String msg;
    msg.format("square [%d, %d] is occupied", op->x0 + 1, op->y0 + 1);
    throw IllegalMoveException(msg);
  }

  xdisplacement = op->x1 - op->x0;
  ydisplacement = op->y1 - op->y0;

  if(_edgeless) {
    if(xdisplacement < -2) {
      xdisplacement += 9;
    } else if(xdisplacement > 2) {
      xdisplacement -= 9;
    }
    if(ydisplacement < -2) {
      ydisplacement += 9;
    } else if(ydisplacement > 2) {
      ydisplacement -= 9;
    }
  }
  
  if((xdisplacement > 2 || xdisplacement < -2) ||
     (ydisplacement > 2 || ydisplacement < -2)) {
    throw IllegalMoveException("moving beyond range");
  }

  if(!_edgeless) {
    if((op->x0 + xdisplacement < 0) || (op->x0 + xdisplacement > 8) ||
       (op->y0 + ydisplacement < 0) || (op->y0 + ydisplacement > 8)) {
      throw IllegalMoveException("moving beyong range");
    }
  }

  if(xdisplacement == 0) {
    if(ydisplacement > 0) {
      dir = DIR_N;
    } else if(ydisplacement < 0) {
      dir = DIR_S;
    } else {
      Error::warning("Unrecoverable error: no direction");
    }
  } else if(xdisplacement > 0) {
    if(ydisplacement == 0) {
      dir = DIR_E;
    } else if(ydisplacement > 0) {
      dir = DIR_NE;
    } else {
      dir = DIR_SE;
    }
  } else if(xdisplacement < 0) {
    if(ydisplacement == 0) {
      dir = DIR_W;
    } else if(ydisplacement > 0) {
      dir = DIR_NW;
    } else {
      dir = DIR_SW;
    }
  }

  if((xdisplacement == 2 || xdisplacement == -2) ||
     (ydisplacement == 2 || ydisplacement == -2)) {
    jumpSquare = &_board->squareAt((op->x0 + (xdisplacement / 2)) % 9,
				   (op->y0 + (ydisplacement / 2)) % 9);
    if(jumpSquare->isEmptyWithout(_ghostPods)) {
      throw IllegalMoveException("jumping over empty square");
    }

    if(op->capture && jumpSquare->isEmpty()) {
      throw IllegalMoveException("no capture");
    }
    Ptr<SlistIterator<OctiSquare *> > iterator =
      (SlistIterator<OctiSquare *> *) _cannotJumpOverSquares.getIterator();
    for(iterator->first(); iterator->isNotNull(); iterator->next()) {
      if(jumpSquare == *iterator->current()) {
	throw IllegalMoveException("jumping over same square more than once");
      }
    }
    _cannotJumpOverSquares.append(jumpSquare);
  }

  // Now locate the pod in the square to be moved
  numPods = startSquare->getNumPods();
  for(i = numPods - 1; i >= 0; i--) {
    movePod = *startSquare->getPodAt(i);
    if(((int) movePod.getType() == (int) player) &&
       movePod.hasSameConfig(op->pod) &&
       movePod.prongAt(dir) != NO_PRONG) { 
      // Do the move and update
      startSquare->removePod(movePod);
      targetSquare->addPod(movePod);

      // Capture if specified
      if(op->capture) {
	numCapPods = jumpSquare->getNumPods();
	op->capturedPods.clear();
	for(j = numCapPods - 1; j >= 0; j--) {
	  capPod = *jumpSquare->getPodAt(j);
	  // Save extra info into the op for undo later!
	  op->capturedPods.append(capPod);
	  _capPods[capPod.getType() - 1]++;

	  jumpSquare->removePod(capPod);
	  for(k = 0; k < 8; k++) {
	    t = capPod.prongAt((OctiDir) k);
	    if(t == NORMAL_PRONG) {
	      _freeProngs[player - 1]++;
	    } else if(t == SUPER_PRONG) {
	      _freeSuperProngs[player - 1]++;
	    }
	  }
	}
      }

      if(_ghostPods.firstIndexOf(movePod) < 0) {
	_ghostPods.append(movePod);
      }

      return;
    }
  }

  throw IllegalMoveException("no valid pods found in square");
}

void OctiGameBaseImpl::undoAddRemoveProng(OctiPlayer player, Ptr<OctiMoveOp> op)
  throw(IllegalMoveException) {
  OctiSquare *square;
  OctiPod *pod;
  int numPods;
  OctiPod inferredPod;
  int i;
  boolean isSuperProng;

  square = &_board->squareAt(op->x0, op->y0);
  numPods = square->getNumPods();

  // Check if the square is empty
  if(square->isEmpty()) {
    String msg;
    msg.format("square [%d, %d] is empty", op->x0 + 1, op->y0 + 1);
    throw IllegalMoveException(msg);
  }

  // Infer the prong configuration after the previous add/remove operation
  inferredPod = op->pod;
  if(op->prongRemoveDir != DIR_UNDEF) {
    inferredPod.removeProng(op->prongRemoveDir);
  }
  if(op->prongAddDir != DIR_UNDEF) {
    inferredPod.addProng(op->prongAddDir, op->addSuperProng);
  }

  for(i = numPods - 1; i >= 0; i--) {
    pod = square->getPodAt(i);
    // Match the prong configuration
    if(((int) pod->getType() == (int) player) &&
       pod->hasSameConfig(inferredPod)) {
      // Restore
      isSuperProng = (pod->prongAt(op->prongAddDir) == SUPER_PRONG);
      pod->removeProng(op->prongAddDir);
      if(isSuperProng) {
	_freeSuperProngs[player - 1]++;
      } else {
	_freeProngs[player - 1]++;
      }
      if(op->prongRemoveDir != DIR_UNDEF) {
	pod->addProng(op->prongRemoveDir, isSuperProng);
	if(isSuperProng) {
	  _freeSuperProngs[player - 1]--;
	} else {
	  _freeProngs[player - 1]--;
	}
      }
      return;  // Done.
    }
  }
	
  throw IllegalMoveException("no valid pods found in square");
}

void OctiGameBaseImpl::undoAddPod(OctiPlayer player, Ptr<OctiMoveOp> op)
  throw(IllegalMoveException) {
  OctiSquare *square;
  OctiPod pod;
  int numPods;
  int i;

  square = &_board->squareAt(op->x0, op->y0);
  numPods = square->getNumPods();

  // Check if the square is empty
  if(square->isEmpty()) {
    String msg;
    msg.format("square [%d, %d] is empty", op->x0 + 1, op->y0 + 1);
    throw IllegalMoveException(msg);
  }

  for(i = numPods - 1; i >= 0; i--) {
    pod = *square->getPodAt(i);
    // Match the prong configuration
    if(((int) pod.getType() == (int) player) &&
       pod.hasSameConfig(OctiPod::EMPTY_POD)) {
      // Remove the empty pod from the square
      square->removePod(pod);

      if(op->liberatedFromPlayer != PLAYER_UNDEF) {
	_capPods[op->liberatedPod.getType() - 1]++;
      } else {
	// Credit the player
	_freePods[player - 1]++;
      }
      return;  // Done.
    }
  }
	
  throw IllegalMoveException("no valid pods found in square");
}

void OctiGameBaseImpl::undoMovePod(OctiPlayer player, Ptr<OctiMoveOp> op)
  throw(IllegalMoveException) {
  OctiSquare *sourceSquare, *targetSquare, *jumpSquare;
  OctiPod pod;
  int numPods;
  int i, j, k;
  OctiProngType t;
  int xdisplacement, ydisplacement;

  targetSquare = &_board->squareAt(op->x1, op->y1);
  sourceSquare = &_board->squareAt(op->x0, op->y0);
  numPods = targetSquare->getNumPods();

  // Check if the square is empty
  if(targetSquare->isEmpty()) {
    String msg;
    msg.format("square [%d, %d] is empty", op->x0 + 1, op->y0 + 1);
    throw IllegalMoveException(msg);
  }

  for(i = numPods - 1; i >= 0; i--) {
    pod = *targetSquare->getPodAt(i);
    // Match the prong configuration
    if(((int) pod.getType() == (int) player) && pod.hasSameConfig(op->pod)) {
      // Restore
      targetSquare->removePod(pod);
      sourceSquare->addPod(pod);
      if(op->capture) {
	xdisplacement = op->x1 - op->x0;
	ydisplacement = op->y1 - op->y0;
		    
	if(_edgeless) {
	  if(xdisplacement < -2) {
	    xdisplacement += 9;
	  } else if(xdisplacement > 2) {
	    xdisplacement -= 9;
	  }
	  if(ydisplacement < -2) {
	    ydisplacement += 9;
	  } else if(ydisplacement > 2) {
	    ydisplacement -= 9;
	  }
	}

	jumpSquare =
	  &_board->squareAt((op->x0 + (xdisplacement / 2)) % 9,
			    (op->y0 + (ydisplacement / 2)) % 9);

	for(j = 0; j < op->capturedPods.size(); j++) {
	  if((int) op->capturedPods[j].getType() != (int) player) {
	    _capPods[op->capturedPods[j].getType() - 1]--;
	  } else {
	    _capPods[player - 1]--;
	  }

	  jumpSquare->addPod(op->capturedPods[j]);
			
	  for(k = 0; k < 8; k++) {
	    t = op->capturedPods[j].prongAt((OctiDir) k);
	    if(t == NORMAL_PRONG) {
	      _freeProngs[player - 1]--;
	    } else if(t == SUPER_PRONG) {
	      _freeSuperProngs[player - 1]--;
	    }
	  }
	}
      }

      return;  // Done.
    }
  }
	
  throw IllegalMoveException("no valid pods found in square");
}

void OctiGameBaseImpl::undo() {
  OctiMove move;
  Ptr<OctiMoveOp> op;
  Dlist<Ptr<OctiMoveOp> > ops;
  
  if(!_moveStack.isEmpty()) {
    _moveStack.pop(move);
    ops = move.getOps();
    
    prevPlayer();

    Ptr<DlistIterator<Ptr<OctiMoveOp> > > iterator =
      (DlistIterator<Ptr<OctiMoveOp> > *) ops.getIterator();
    for(iterator->last(); iterator->isNotNull(); iterator->prev()) {
      op = *iterator->current();
      try {
	if(op->prongAddDir != DIR_UNDEF) {
	  undoAddRemoveProng(_playerToMove, op);
	} else if(op->x0 == op->x1 && op->y0 == op->y1) {
	  undoAddPod(_playerToMove, op);
	} else {
	  undoMovePod(_playerToMove, op);
	}
      } catch(IllegalMoveException e) {
	Error::warning("Error in undo \"%s\": %s\n", move.getNotation(), e.getMessage());
      }
    }
    
    _redoStack.push(move);

    if(_isOver) {
      _isOver = FALSE;
    }
    canUndo(TRUE);
    canRedo(TRUE);
    _board->boardUpdated(&move, TRUE);
    for(int i = _gameListeners.getNumListeners() - 1; i >= 0; i--) {
      OctiGameListener *listener =
	(OctiGameListener *) _gameListeners.getListenerAt(i);
      listener->gameUpdated(this);
    }
  }
}

void OctiGameBaseImpl::redo() {
  OctiMove move;
  if(!_redoStack.isEmpty()) {
    try {
      _redoStack.pop(move);
      makeMove(_playerToMove, move, TRUE);
    } catch(IllegalMoveException e) {
      Error::warning("Error in redo \"%s\": %s\n", move.getNotation(), e.getMessage());
    }

    canUndo(TRUE);
    canRedo(TRUE);
    _board->boardUpdated(&move, FALSE);
    for(int i = _gameListeners.getNumListeners() - 1; i >= 0; i--) {
      OctiGameListener *listener =
	(OctiGameListener *) _gameListeners.getListenerAt(i);
      listener->gameUpdated(this);
    }
  }
}

void OctiGameBaseImpl::ensureLock() const throw(IllegalMoveException) {
  if(_prepare) {
    throw IllegalMoveException("prepared-move in progress");
  }
}

void OctiGameBaseImpl::lockPrepare() throw(IllegalMoveException) {
  _prepare = TRUE;
  for(int i = _gameListeners.getNumListeners() - 1; i >= 0; i--) {
    OctiGameListener *listener =
      (OctiGameListener *) _gameListeners.getListenerAt(i);
    listener->canUndo(this, 0);
    listener->canRedo(this, 0);
    listener->canMakeMove(this, TRUE);
    listener->canCancelMove(this, TRUE);
    listener->canPauseGame(this, FALSE);
    listener->canResign(this, FALSE);
  }
}
					   
void OctiGameBaseImpl::unlockPrepare() throw(IllegalMoveException) {
  _prepare = FALSE;

  for(int i = _gameListeners.getNumListeners() - 1; i >= 0; i--) {
    OctiGameListener *listener =
      (OctiGameListener *) _gameListeners.getListenerAt(i);
    listener->canUndo(this, _moveStack.size());
    listener->canRedo(this, _redoStack.size());
    listener->canMakeMove(this, FALSE);
    listener->canCancelMove(this, FALSE);
    listener->canPauseGame(this, _isOver);
    listener->canResign(this, _isOver);
  }
}

boolean OctiGameBaseImpl::isPrepareLocked() const {
  return _prepare;
}

void OctiGameBaseImpl::canUndo(boolean b) {
  for(int i = _gameListeners.getNumListeners() - 1; i >= 0; i--) {
    OctiGameListener *listener =
      (OctiGameListener *) _gameListeners.getListenerAt(i);
    listener->canUndo(this, b ? _moveStack.size() : 0);
  }
}

void OctiGameBaseImpl::canRedo(boolean b) {
  for(int i = _gameListeners.getNumListeners() - 1; i >= 0; i--) {
    OctiGameListener *listener =
      (OctiGameListener *) _gameListeners.getListenerAt(i);
    listener->canRedo(this, b ? _redoStack.size() : 0);
  }
}

void OctiGameBaseImpl::prepareAddPod(int x, int y) throw (IllegalMoveException) {
  if(isPrepareLocked()) {
    throw IllegalMoveException("OctiGameBaseImpl::prepareAddPod() - already locked");
  }

  Ptr<OctiMoveOp> op = new OctiMoveOp();
  op->x0 = op->x1 = x;
  op->y0 = op->y1 = y;

  _preparedMoveOps.clear();
  _preparedMoveOps.append(op);
  try {
    _preparedMove = OctiMove(_preparedMoveOps);
    doAddPod(_playerToMove, op);
    lockPrepare();
    _board->boardUpdated(&_preparedMove, FALSE);
  } catch(IllegalMoveSyntaxException e) {
    cerr << e.getMessage() << endl;
  }
}
void OctiGameBaseImpl::prepareAddProng(int x, int y, OctiPod pod, OctiDir dir) throw(IllegalMoveException) {
  if(isPrepareLocked()) {
    throw IllegalMoveException("OctiGameBaseImpl::prepareAddProng() - already locked");
  }

  Ptr<OctiMoveOp> op = new OctiMoveOp();
  op->x0 = op->x1 = x;
  op->y0 = op->y1 = y;
  op->pod = pod;
  op->prongAddDir = dir;

  _preparedMoveOps.clear();
  _preparedMoveOps.append(op);
  try {
    _preparedMove = OctiMove(_preparedMoveOps);
    doAddRemoveProng(_playerToMove, op);
    lockPrepare();
    _board->boardUpdated(&_preparedMove, FALSE);
  } catch(IllegalMoveSyntaxException e) {
    cerr << e.getMessage() << endl;
  }
}

void OctiGameBaseImpl::prepareReposition(int x, int y, OctiPod pod, OctiDir srcDir, OctiDir tgtDir) throw(IllegalMoveException) {
  if(isPrepareLocked()) {
    throw IllegalMoveException("OctiGameBaseImpl::prepareReposition() - already locked");
  }

  Ptr<OctiMoveOp> op = new OctiMoveOp();
  op->x0 = op->x1 = x;
  op->y0 = op->x1 = y;
  op->pod = pod;
  op->prongAddDir = srcDir;
  op->prongRemoveDir = tgtDir;

  _preparedMoveOps.clear();
  _preparedMoveOps.append(op);
  try {
    _preparedMove = OctiMove(_preparedMoveOps);
    
    lockPrepare();
    _board->boardUpdated(&_preparedMove, FALSE);
  } catch(IllegalMoveSyntaxException e) {
    cerr << e.getMessage() << endl;
  }
}

void OctiGameBaseImpl::prepareHops(int x, int y, OctiPod pod, Dlist<OctiHop> hops) throw(IllegalMoveException) {
  OctiHop anchor, hop;
  Dlist<Ptr<OctiMoveOp> > newOps = _preparedMoveOps;

  anchor.x = x;
  anchor.y = y;
  Ptr<DlistIterator<OctiHop> > it =
    (DlistIterator<OctiHop> *) hops.getIterator();
  for(it->first(); it->isNotNull(); it->next()) {
    Ptr<OctiMoveOp> op = new OctiMoveOp();
    hop = *it->current();
    op->x0 = anchor.x;
    op->y0 = anchor.y;
    op->x1 = anchor.x = hop.x;
    op->y1 = anchor.y = hop.y;
    op->pod = pod;
    newOps.append(op);
  }

  try {
    OctiMove newMove(newOps);
    int sourceX = newMove.getSourceX();
    int sourceY = newMove.getSourceY();

    if(isPrepareLocked()) {
      rollbackPreparedMove();
    }

    Ptr<OctiMoveOp> op;
    Ptr<DlistIterator<Ptr<OctiMoveOp> > > iterator =
      (DlistIterator<Ptr<OctiMoveOp> > *) newOps.getIterator();
    for(iterator->first(); iterator->isNotNull(); iterator->next()) {
      op = *iterator->current();
      try {
	if(sourceX == op->x0 && sourceY == op->y0) {
	  _cannotJumpOverSquares.clear();
	}

	doMovePod(_playerToMove, op);
      } catch(IllegalMoveException e) {
	try {
	  // Undo the damage...
	  for(iterator->prev(); iterator->isNotNull(); iterator->prev()) {
	    op = *iterator->current();
	    undoMovePod(_playerToMove, op);
	  }
	} catch(IllegalMoveException e) {
	  Error::warning("Unrecoverable error: %s\n", e.getMessage());
	}
	throw e;
      }
    }

    _preparedMoveOps = newOps;
    _preparedMove = newMove;
    lockPrepare();
    _board->boardUpdated(&_preparedMove, FALSE);
  } catch(IllegalMoveSyntaxException e) {
    cerr << e.getMessage() << endl;
  }
}

void OctiGameBaseImpl::prepareCapture(int x, int y) {
  boolean origLock = isPrepareLocked(); 
  lockPrepare();
  try {
  } catch(IllegalMoveException e) {
    if(!origLock) {
      unlockPrepare();
    }
  }
}

void OctiGameBaseImpl::commitPreparedMove() throw(IllegalMoveException) {
  if(!isPrepareLocked()) {
    throw IllegalMoveException("OctiGameBaseImpl::commitPreparedMove() - nothing to commit");
  }

  Ptr<OctiMoveOp> op;
  Ptr<DlistIterator<Ptr<OctiMoveOp> > > iterator =
    (DlistIterator<Ptr<OctiMoveOp> > *) _preparedMoveOps.getIterator();
  for(iterator->last(); iterator->isNotNull(); iterator->prev()) {
    op = *iterator->current();
    try {
      if(op->prongAddDir != DIR_UNDEF) {
	undoAddRemoveProng(_playerToMove, op);
      } else if(op->x0 == op->x1 && op->y0 == op->y1) {
	undoAddPod(_playerToMove, op);
      } else {
	undoMovePod(_playerToMove, op);
      }
    } catch(IllegalMoveException e) {
      Error::warning("Error in commit \"%s\": %s\n",
		     _preparedMove.getNotation(),
		     e.getMessage());
    }
  }

  unlockPrepare();
  makeMove(_playerToMove, _preparedMove, FALSE);
  _board->boardUpdated(&_preparedMove, FALSE);
  _preparedMoveOps.clear();
  _preparedMove = OctiMove();
}

void OctiGameBaseImpl::rollbackPreparedMove() throw(IllegalMoveException) {
  if(!isPrepareLocked()) {
    throw IllegalMoveException("OctiGameBaseImpl::rollbackPreparedMove() - nothing to rollback");
  }

  Ptr<OctiMoveOp> op;
  Ptr<DlistIterator<Ptr<OctiMoveOp> > > iterator =
    (DlistIterator<Ptr<OctiMoveOp> > *) _preparedMoveOps.getIterator();
  for(iterator->last(); iterator->isNotNull(); iterator->prev()) {
    op = *iterator->current();
    try {
      if(op->prongAddDir != DIR_UNDEF) {
	undoAddRemoveProng(_playerToMove, op);
      } else if(op->x0 == op->x1 && op->y0 == op->y1) {
	undoAddPod(_playerToMove, op);
      } else {
	undoMovePod(_playerToMove, op);
      }
    } catch(IllegalMoveException e) {
      Error::warning("Error in rollback \"%s\": %s\n",
		     _preparedMove.getNotation(),
		     e.getMessage());
    }
  }

  unlockPrepare();
  _board->boardUpdated(&_preparedMove, TRUE);
  _preparedMoveOps.clear();
  _preparedMove = OctiMove();
}

void Octi2PlayerGame::initGameCondition() {
  // Initialize the player pods
  _freePods[0] = MODE_2_INIT_FREE_PODS - 3;
  _freePods[1] = MODE_2_INIT_FREE_PODS - 3;
  
  // Initialize the free prongs
  _freeProngs[0] = MODE_2_INIT_FREE_PRONGS;
  _freeProngs[1] = MODE_2_INIT_FREE_PRONGS;
  _freeSuperProngs[0] = _useSuperProngs ? MODE_2_INIT_FREE_SUPERPRONGS : 0;
  _freeSuperProngs[1] = _useSuperProngs ? MODE_2_INIT_FREE_SUPERPRONGS : 0;
  
  // Insert 3 pods of each player into his square
  _board->squareAt(2, 2).addPod(OctiPod(A_POD));
  _board->squareAt(4, 2).addPod(OctiPod(A_POD));
  _board->squareAt(6, 2).addPod(OctiPod(A_POD));
  _board->squareAt(2, 6).addPod(OctiPod(B_POD));
  _board->squareAt(4, 6).addPod(OctiPod(B_POD));
  _board->squareAt(6, 6).addPod(OctiPod(B_POD));
}

void Octi2PlayerGame::nextPlayer() {
  _playerToMove = (OctiPlayer) (((int) _playerToMove % 2) + 1);
}

void Octi2PlayerGame::prevPlayer() {
  _playerToMove = (OctiPlayer) (((int) _playerToMove % 2) + 1);
}

boolean Octi2PlayerGame::liberate(OctiPlayer player, Ptr<OctiMoveOp> op,
				  OctiPod &pod) {
  if(_capPods[player - 1] > 0) {
    _capPods[player - 1]--;
    op->liberatedPod = OctiPod();
    pod = op->liberatedPod;
    return TRUE;
  }
  
  return FALSE;
}

boolean Octi2PlayerGame::checkIsGameOver() const {
  return
    ((_board->squareAt(2, 2)).isCaptured()       &&
     (_board->squareAt(4, 2)).isCaptured()       &&
     (_board->squareAt(6, 2)).isCaptured())      ||
    ((_board->squareAt(2, 6)).isCaptured()       &&
     (_board->squareAt(4, 6)).isCaptured()       &&
     (_board->squareAt(6, 6)).isCaptured())      ||
    (_capPods[0] >= MODE_2_INIT_FREE_PODS)  ||
    (_capPods[1] >= MODE_2_INIT_FREE_PODS);
}

void Octi4PlayerGame::initGameCondition() {
  // Initialize the player pods
  _freePods[0] = MODE_4_INIT_FREE_PODS - 1;
  _freePods[1] = MODE_4_INIT_FREE_PODS - 1;
  _freePods[2] = MODE_4_INIT_FREE_PODS - 1;
  _freePods[3] = MODE_4_INIT_FREE_PODS - 1;
  
  // Initialize the free prongs
  _freeProngs[0] = MODE_4_INIT_FREE_PRONGS;
  _freeProngs[1] = MODE_4_INIT_FREE_PRONGS;
  _freeProngs[2] = MODE_4_INIT_FREE_PRONGS;
  _freeProngs[3] = MODE_4_INIT_FREE_PRONGS;
  _freeSuperProngs[0] = _useSuperProngs ? MODE_4_INIT_FREE_SUPERPRONGS : 0;
  _freeSuperProngs[1] = _useSuperProngs ? MODE_4_INIT_FREE_SUPERPRONGS : 0;
  _freeSuperProngs[2] = _useSuperProngs ? MODE_4_INIT_FREE_SUPERPRONGS : 0;
  _freeSuperProngs[3] = _useSuperProngs ? MODE_4_INIT_FREE_SUPERPRONGS : 0;
  
  // Insert pods of each player into his square

  _board->squareAt(2, 2).addPod(OctiPod(A_POD));
  _board->squareAt(2, 6).addPod(OctiPod(B_POD));
  _board->squareAt(6, 6).addPod(OctiPod(C_POD));
  _board->squareAt(6, 2).addPod(OctiPod(D_POD));
}

void Octi4PlayerGame::nextPlayer() {
  _playerToMove = (OctiPlayer) (((int) _playerToMove % 4) + 1);
}

void Octi4PlayerGame::prevPlayer() {
    _playerToMove = (OctiPlayer) ((((int) _playerToMove - 1) % 4) + 1);
}

boolean Octi4PlayerGame::liberate(OctiPlayer player, Ptr<OctiMoveOp> op,
				  OctiPod &pod) {
  OctiPod liberatedPod;
  OctiPlayer takenFrom1 = PLAYER_UNDEF;
  OctiPlayer takenFrom2 = PLAYER_UNDEF;
	
  switch(player) {
  case PLAYER_A:
    takenFrom1 = PLAYER_B;
    takenFrom2 = PLAYER_D;
    break;
  case PLAYER_B:
    takenFrom1 = PLAYER_A;
    takenFrom2 = PLAYER_C;
    break;
  case PLAYER_C:
    takenFrom1 = PLAYER_D;
    takenFrom2 = PLAYER_B;
    break;
  case PLAYER_D:
    takenFrom1 = PLAYER_C;
    takenFrom2 = PLAYER_A;
    break;
  }

  if(_capPods[player - 1] > 0) {
    _capPods[player - 1]--;
    op->liberatedPod = OctiPod();
    pod = op->liberatedPod;
    return TRUE;
  }

  return FALSE;
}

boolean Octi4PlayerGame::checkIsGameOver() const {
  return
    (_board->squareAt(2, 2).isCaptured()  &&
     _board->squareAt(6, 2).isCaptured()) ||
    (_board->squareAt(6, 6).isCaptured()  &&
     _board->squareAt(2, 6).isCaptured()) ||
    (_capPods[0] + _capPods[2] >= (2 * MODE_4_INIT_FREE_PODS)) ||
    (_capPods[1] + _capPods[3] >= (2 * MODE_4_INIT_FREE_PODS));
}

__END_NAMESPACE(SELFSOFT);
