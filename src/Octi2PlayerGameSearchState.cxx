/*************************************************************************
 * $Id: Octi2PlayerGameSearchState.cxx,v 1.11 2001/08/24 03:00:34 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:00:34 $
 * $Locker:  $
 * $Revision: 1.11 $
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

#include <math.h>
#include "OctiDefs.hxx"
#include "Octi2PlayerGameSearchState.hxx"

#define O2PGSS Octi2PlayerGameSearchState
#define MAJOR_WEIGHT_MATERIAL         10.0f
#define MAJOR_WEIGHT_MOBILITY          1.0f
#define MAJOR_WEIGHT_THREAT            1.0f
#define MAJOR_WEIGHT_DISTANCE         16.0f
#define MINOR_WEIGHT_POD               1.0f
#define MINOR_WEIGHT_PRONG             1.0f
#define MINOR_WEIGHT_ONBOARD           2.0f
#define MINOR_WEIGHT_RESERVE           1.6f
#define MINOR_WEIGHT_THREAT_POD        1.0f
#define MINOR_WEIGHT_THREAT_PRONG      1.0f

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_DISCOVERABLE(Octi2PlayerGameSearchState, SearchState);

const word32 MJCMap::s_NEG_OD_COLUMN[] = {
    0xffffe,  // [x, 0]
    0xffffb,  // [x, 1]
    0xfffef,  // [x,.2]
    0xfffbf,  // [x, 3]
    0xffeff,  // [x, 4]
    0xffbff,  // [x, 5]
    0xfefff,  // [x, 6]
    0xfbfff,  // [x, 7]
    0xeffff   // [x, 8]
};

const word32 MJCMap::s_MASK_OD_COLUMN[] = {
    0x00001,  // [x, 0]
    0x00004,  // [x, 1]
    0x00010,  // [x,.2]
    0x00040,  // [x, 3]
    0x00100,  // [x, 4]
    0x00400,  // [x, 5]
    0x01000,  // [x, 6]
    0x04000,  // [x, 7]
    0x10000   // [x, 8]
};

const word32 MJCMap::s_NEG_JC_COLUMN[] = {
    0xffffd,  // [x, 0]
    0xffff7,  // [x, 1]
    0xfffdf,  // [x,.2]
    0xfff7f,  // [x, 3]
    0xffdff,  // [x, 4]
    0xff7ff,  // [x, 5]
    0xfdfff,  // [x, 6]
    0xf7fff,  // [x, 7]
    0xdffff   // [x, 8]
};

const word32 MJCMap::s_MASK_JC_COLUMN[] = {
    0x00002,  // [x, 0]
    0x00008,  // [x, 1]
    0x00020,  // [x,.2]
    0x00080,  // [x, 3]
    0x00200,  // [x, 4]
    0x00800,  // [x, 5]
    0x02000,  // [x, 6]
    0x08000,  // [x, 7]
    0x20000   // [x, 8]
};

const char *Octi2PlayerGameSearchState::s_STR_LOCS[9][9] = {
    { "11", "12", "13", "14", "15", "16", "17", "18", "19" },
    { "21", "22", "23", "24", "25", "26", "27", "28", "29" },
    { "31", "32", "33", "34", "35", "36", "37", "38", "39" },
    { "41", "42", "43", "44", "45", "46", "47", "48", "49" },
    { "51", "52", "53", "54", "55", "56", "57", "58", "59" },
    { "61", "62", "63", "64", "65", "66", "67", "68", "69" },
    { "71", "72", "73", "74", "75", "76", "77", "78", "79" },
    { "81", "82", "83", "84", "85", "86", "87", "88", "89" },
    { "91", "92", "93", "94", "95", "96", "97", "98", "99" },
};

const char Octi2PlayerGameSearchState::s_CHAR_DIRS[8] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'
};

const char *Octi2PlayerGameSearchState::s_STR_ADDPOD_MOVES[6] = {
    "33", "53", "73", "37", "57", "77"
};

TransitionOp &TransitionOp::operator=(const TransitionOp &trans) {
    if(_type != trans._type) {
        switch(_type) {
        case TRANS_ADD_POD:
            delete _u.addPodTrans;
            break;
        case TRANS_ADD_PRONG:
            delete _u.addProngTrans;
            break;
        case TRANS_REPOSITION:
            delete _u.repositionTrans;
            break;
        case TRANS_MOVE_POD:
            delete _u.movePodTrans;
            break;
        case TRANS_CAPTURE:
            delete _u.captureTrans;
            break;
        }
    
        switch(trans._type) {
        case TRANS_ADD_POD:
            _u.addPodTrans = new AddPodTransition();
            break;
        case TRANS_ADD_PRONG:
            _u.addProngTrans = new AddProngTransition();
            break;
        case TRANS_REPOSITION:
            _u.repositionTrans = new RepositionTransition();
            break;
        case TRANS_MOVE_POD:
            _u.movePodTrans = new MovePodTransition();
            break;
        case TRANS_CAPTURE:
            _u.captureTrans = new CaptureTransition();
            break;
        }
    }

    _type = trans._type;
    switch(trans._type) {
    case TRANS_ADD_POD:
        *_u.addPodTrans = *trans._u.addPodTrans;
        break;
    case TRANS_ADD_PRONG:
        *_u.addProngTrans = *trans._u.addProngTrans;
        break;
    case TRANS_REPOSITION:
        *_u.repositionTrans = *trans._u.repositionTrans;
        break;
    case TRANS_MOVE_POD:
        *_u.movePodTrans = *trans._u.movePodTrans;
        break;
    case TRANS_CAPTURE:
        *_u.captureTrans = *trans._u.captureTrans;
        break;
    }

    return *this;
}

//////////////////////////////////////////////////////////////////////////////
// Constructors

O2PGSS::Octi2PlayerGameSearchState(const Octi2PlayerGameSearchState *state) {
    Octi2PlayerGameSearchState *copy =
        (Octi2PlayerGameSearchState *) (const Octi2PlayerGameSearchState *) state;

    _moveDescription = state->_moveDescription;
    _playerToMove = copy->_playerToMove;
    _freePods[0] = copy->_freePods[0];
    _freePods[1] = copy->_freePods[1];
    _freeProngs[0] = copy->_freeProngs[0];
    _freeProngs[1] = copy->_freeProngs[1];
    _freeSuperProngs[0] = copy->_freeSuperProngs[0];
    _freeSuperProngs[1] = copy->_freeSuperProngs[1];
    _capturedPods[0] = copy->_capturedPods[0];
    _capturedPods[1] = copy->_capturedPods[1];
  
    Ptr<SlistIterator<ValidSquare> > iterator = 
        (SlistIterator<ValidSquare> *) copy->_validSquares.getIterator();
    const ValidSquare *current;
    const Octi2PlayerSquare *square;
    int x, y;
    for(iterator->first(); iterator->isNotNull(); iterator->next()) {
        current = iterator->current();
        x = current->loc.x;
        y = current->loc.y;
        square = current->square.ptr();
        _squares[x][y] = new Octi2PlayerSquare(*square);
        appendValidSquare(current->loc.x, current->loc.y);
    }

    _evaled = copy->_evaled;
    _eval = copy->_eval;
    _terminalTested = copy->_terminalTested;
    _terminalValue = copy->_terminalValue;
}

O2PGSS::Octi2PlayerGameSearchState(OctiBoard *board) {
    int x, y;
    OctiSquare *square;
    OctiGame *game = board->getGame();
  
    _playerToMove = game->getPlayerToMove();
    _freePods[0] = game->getNumFreePods(PLAYER_A);
    _freePods[1] = game->getNumFreePods(PLAYER_B);
    _capturedPods[0] = game->getNumCapturedPods(PLAYER_A);
    _capturedPods[1] = game->getNumCapturedPods(PLAYER_B);
    _freeProngs[0] = game->getNumFreeProngs(PLAYER_A);
    _freeProngs[1] = game->getNumFreeProngs(PLAYER_B);
    _freeSuperProngs[0] = game->getNumFreeSuperProngs(PLAYER_A);
    _freeSuperProngs[1] = game->getNumFreeSuperProngs(PLAYER_B);
    _evaled = FALSE;
    _eval = 0.0;
    _terminalTested = FALSE;
    _terminalValue = 0.0;

    for(x = 0; x < 9; x++) {
        for(y = 0; y < 9; y++) {
            square = &board->squareAt(x, y);
            if(!square->isEmpty()) {
                _squares[x][y] =
                    new Octi2PlayerSquare(*(Octi2PlayerSquare *) square);
                appendValidSquare(x, y);
            }
        }
    }
}

O2PGSS::Octi2PlayerGameSearchState(Octi2PlayerGameSearchState *parent,
                                   const char *moveDescription) {
    _parent = parent;
    _moveDescription = moveDescription;
    _evaled = FALSE;
    _eval = 0.0;
    _terminalTested = FALSE;
    _terminalValue = 0.0;
}

O2PGSS::Octi2PlayerGameSearchState(Octi2PlayerGameSearchState *parent,
                                   Octi2PlayerGameSearchState *copy,
                                   const char *moveDescription) {
    _parent = parent;
    _moveDescription = moveDescription;
    _evaled = FALSE;
    _eval = 0.0;
    _terminalTested = FALSE;
    _terminalValue = 0.0;
  
    // Copy the transition operators that contains a "TRANS_CAPTURE" type
    Ptr<SlistIterator<TransitionOp> > iterator = 
        (SlistIterator<TransitionOp> *) copy->_trans.getIterator();
    const TransitionOp *current;

    for(iterator->first(); iterator->isNotNull(); iterator->next()) {
        current = iterator->current();
        if(current->getTransitionType() == TRANS_CAPTURE) {
            appendTransitionOp(*current);
        }
    }
}

O2PGSS::Octi2PlayerGameSearchState(Octi2PlayerGameSearchState *parent,
                                   Octi2PlayerGameSearchState *states[],
                                   int statesCount,
                                   const char *moveDescription)
    throw(Exception) {
    _parent = parent;
    _moveDescription = moveDescription;
    _evaled = FALSE;
    _eval = 0.0;
    _terminalTested = FALSE;
    _terminalValue = 0.0;

    // Copy all transition operators from the mergeable states
    // check if there are more than two capture attempts on the same
    // square. If so, reject this state from being merged.
    // All permutations are possible except when two or more mergeable
    // states try to capture the same square!
    MJCMap map;
    int i;
    for(i = 0; i < statesCount; i++) {
        Ptr<SlistIterator<TransitionOp> > iterator = 
            (SlistIterator<TransitionOp> *) states[i]->_trans.getIterator();
        const TransitionOp *current;
    
        for(iterator->first(); iterator->isNotNull(); iterator->next()) {
            current = iterator->current();
            if(current->getTransitionType() == TRANS_CAPTURE) {
                const CaptureTransition *t = current->getCaptureTransition();
                if(!map.tryMarkJC(t->loc.x, t->loc.y)) {
                    throw Exception();
                }
            }

            appendTransitionOp(*current);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// Move enumerations

void O2PGSS::expandState() {
    const ValidSquare *current;
    const Octi2PlayerSquare *square;
    int x, y, k;
    OctiPod pod;
    int numPods;
    CompositeMove cm[7];
    int cmCount = 0;

    // Set the temporary references to null
    _headAddPodStates = NULL;
    _tailAddPodStates = NULL;
    _headAddProngStates = NULL;
    _tailAddProngStates = NULL;
    _headMoveJumpCaptureStates = NULL;
    _tailMoveJumpCaptureStates = NULL;
    _headCompositeStates = NULL;
    _tailCompositeStates = NULL;

    syncState();

    enumerateAddPodPlys();

    Ptr<SlistIterator<ValidSquare> > iterator = 
        (SlistIterator<ValidSquare> *) _validSquares.getIterator();
    for(iterator->first(); iterator->isNotNull(); iterator->next()) {
        current = iterator->current();
        x = current->loc.x;
        y = current->loc.y;
        square = current->square;
        numPods = square->getNumPods();

        if(numPods > 0 && ((int) square->getPodAt(0)->getType() == (int)_playerToMove)) {
            // Reset the composite move array
            cmCount = 0;

            for(k = numPods - 1; k >= 0; k--) {
                pod = *square->getPodAt(k);
                enumerateAddProngPlys(x, y, pod);
                cm[cmCount].head = _tailMoveJumpCaptureStates;
                enumerateMovePodPlys(x, y, pod);
                enumerateJumpPodPlys(x, y, pod);
                cm[cmCount].tail = _tailMoveJumpCaptureStates;
                cm[cmCount].pod = pod;
                if(cm[cmCount].tail != NULL &&
                   cm[cmCount].tail != cm[cmCount].head) {
                    if(cm[cmCount].head == NULL) {
                        cm[cmCount].head = _headMoveJumpCaptureStates;
                    } else if(cm[cmCount].head != cm[cmCount].tail) {
                        cm[cmCount].head = (Octi2PlayerGameSearchState *)
                            cm[cmCount].head->_next;
                    }
                    cmCount++;
                }
            }
            enumerateCompositePlys(x, y, cm, cmCount);
        }
    }

    orderListAndUpdateSuccessors();
}

void O2PGSS::enumerateAddPodPlys() {
    boolean square22Cap, square42Cap, square62Cap;
    boolean square26Cap, square46Cap, square66Cap;
    boolean canAddFree;
    boolean canLiberate;
    Octi2PlayerGameSearchState *s;
  
    square22Cap = (_squares[2][2] == NULL) ? FALSE : _squares[2][2]->isCaptured();
    square42Cap = (_squares[4][2] == NULL) ? FALSE : _squares[4][2]->isCaptured();
    square62Cap = (_squares[6][2] == NULL) ? FALSE : _squares[6][2]->isCaptured();
    square26Cap = (_squares[2][6] == NULL) ? FALSE : _squares[2][6]->isCaptured();
    square46Cap = (_squares[4][6] == NULL) ? FALSE : _squares[4][6]->isCaptured();
    square66Cap = (_squares[6][6] == NULL) ? FALSE : _squares[6][6]->isCaptured();
  
    canAddFree = (_freePods[_playerToMove - 1] > 0);
    canLiberate = (_capturedPods[_playerToMove - 1] > 0);
  
    if((_playerToMove == PLAYER_A && !square22Cap && canAddFree) ||
       (_playerToMove == PLAYER_B && square22Cap && (canAddFree || canLiberate))) {
        s = new Octi2PlayerGameSearchState(this, s_STR_ADDPOD_MOVES[0]);
        s->addPodTransitionOp(2, 2);
        appendAddPodState(s);
    }
  
    if((_playerToMove == PLAYER_A && !square42Cap && canAddFree) ||
       (_playerToMove == PLAYER_B && square42Cap && (canAddFree || canLiberate))) {
        s = new Octi2PlayerGameSearchState(this, s_STR_ADDPOD_MOVES[1]);
        s->addPodTransitionOp(4, 2);
        appendAddPodState(s);
    }
  
    if((_playerToMove == PLAYER_A && !square62Cap && canAddFree) ||
       (_playerToMove == PLAYER_B && square62Cap && (canAddFree || canLiberate))) {
        s = new Octi2PlayerGameSearchState(this, s_STR_ADDPOD_MOVES[2]);
        s->addPodTransitionOp(6, 2);
        appendAddPodState(s);
    }
  
    if((_playerToMove == PLAYER_A && square26Cap && (canAddFree || canLiberate)) ||
       (_playerToMove == PLAYER_B && !square26Cap && canAddFree)) {
        s = new Octi2PlayerGameSearchState(this, s_STR_ADDPOD_MOVES[3]);
        s->addPodTransitionOp(2, 6);
        appendAddPodState(s);
    }
  
    if((_playerToMove == PLAYER_A && square46Cap && (canAddFree || canLiberate)) ||
       (_playerToMove == PLAYER_B && !square46Cap && canAddFree)) {
        s = new Octi2PlayerGameSearchState(this, s_STR_ADDPOD_MOVES[4]);
        s->addPodTransitionOp(4, 6);
        appendAddPodState(s);
    }
  
    if((_playerToMove == PLAYER_A && square66Cap && (canAddFree || canLiberate)) ||
       (_playerToMove == PLAYER_B && !square66Cap && canAddFree)) {
        s = new Octi2PlayerGameSearchState(this, s_STR_ADDPOD_MOVES[5]);
        s->addPodTransitionOp(6, 6);
        appendAddPodState(s);
    }
}

void O2PGSS::enumerateAddProngPlys(int x, int y, OctiPod pod) {
    int k, l;
    OctiProngType prong;
    char str_prong_config[17];
    String buf;
    Octi2PlayerGameSearchState *s;

    pod.getProngConfigString(str_prong_config);
    if(_freeProngs[_playerToMove - 1] > 0) {
        // Has free prongs, then enumerate adding a prong from the
        // reserve
        for(k = DIR_N; k <= DIR_NW; k++) {
            buf = s_STR_LOCS[x][y];
            buf += str_prong_config;
            if(pod.prongAt((OctiDir) k) == NO_PRONG) {
                buf += '+';
                buf += s_CHAR_DIRS[k];
                s = new Octi2PlayerGameSearchState(this, (const char *) buf);
                s->addProngTransitionOp(x, y, pod, (OctiDir) k);
                appendAddProngState(s);
            }
        }
    } else {
        // No free prongs, then enumerate repositioning a prong
        for(k = DIR_N; k <= DIR_NW; k++) {
            if((prong = pod.prongAt((OctiDir) k)) != NO_PRONG) {
                for(l = DIR_N; l <= DIR_NW; l++) {
                    buf = s_STR_LOCS[x][y];
                    buf += str_prong_config;
                    if(l != k && pod.prongAt((OctiDir) l) == NO_PRONG) {
                        buf += '-';
                        buf += s_CHAR_DIRS[k];
                        buf += '+';
                        buf += s_CHAR_DIRS[l];
                        s = new Octi2PlayerGameSearchState(this, (const char *) buf);
                        s->repositionTransitionOp(x, y, pod, (OctiDir) k, (OctiDir) l);
                        appendAddProngState(s);
                    }
                }
            }
        }
    }
}

void O2PGSS::enumerateMovePodPlys(int x, int y, OctiPod pod) {
    char str_prong_config[17];
    String buf;
    Octi2PlayerGameSearchState *s;

    pod.getProngConfigString(str_prong_config);

    // North
    if(pod.prongAt(DIR_N) != NO_PRONG && y + 1 < 9 &&
       (_squares[x][y + 1] == NULL ||
        (!_squares[x][y + 1]->hasEnemyOf(_playerToMove)))) {
        buf = s_STR_LOCS[x][y];
        buf += str_prong_config;
        buf += '-';
        buf += s_STR_LOCS[x][y + 1];
        s = new Octi2PlayerGameSearchState(this, (const char *) buf);
        s->movePodTransitionOp(x, y, pod, x, y + 1);
        appendMoveJumpCaptureState(s);
    }
	
    // North East
    if(pod.prongAt(DIR_NE) != 0 && x + 1 < 9 && y + 1 < 9 &&
       (_squares[x + 1][y + 1] == NULL ||
        (!_squares[x + 1][y + 1]->hasEnemyOf(_playerToMove)))) {
        buf = s_STR_LOCS[x][y];
        buf += str_prong_config;
        buf += '-';
        buf += s_STR_LOCS[x + 1][y + 1];
        s = new Octi2PlayerGameSearchState(this, (const char *) buf);
        s->movePodTransitionOp(x, y, pod, x + 1, y + 1);
        appendMoveJumpCaptureState(s);
    }
	
    // East
    if(pod.prongAt(DIR_E) != 0 && x + 1 < 9 &&
       (_squares[x + 1][y] == NULL ||
        (!_squares[x + 1][y]->hasEnemyOf(_playerToMove)))) {
        buf = s_STR_LOCS[x][y];
        buf += str_prong_config;
        buf += '-';
        buf += s_STR_LOCS[x + 1][y];
        s = new Octi2PlayerGameSearchState(this, (const char *) buf);
        s->movePodTransitionOp(x, y, pod, x + 1, y);
        appendMoveJumpCaptureState(s);
    }
	
    // South East
    if(pod.prongAt(DIR_SE) != 0 && x + 1 < 9 && y - 1 >= 0 &&
       (_squares[x + 1][y - 1] == NULL ||
        (!_squares[x + 1][y - 1]->hasEnemyOf(_playerToMove)))) {
        buf = s_STR_LOCS[x][y];
        buf += str_prong_config;
        buf += '-';
        buf += s_STR_LOCS[x + 1][y - 1];
        s = new Octi2PlayerGameSearchState(this, (const char *) buf);
        s->movePodTransitionOp(x, y, pod, x + 1, y - 1);
        appendMoveJumpCaptureState(s);
    }
	
    // South
    if(pod.prongAt(DIR_S) != 0 && y - 1 >= 0 &&
       (_squares[x][y - 1] == NULL ||
        (!_squares[x][y - 1]->hasEnemyOf(_playerToMove)))) {
        buf = s_STR_LOCS[x][y];
        buf += str_prong_config;
        buf += '-';
        buf += s_STR_LOCS[x][y - 1];
        s = new Octi2PlayerGameSearchState(this, (const char *) buf);
        s->movePodTransitionOp(x, y, pod, x, y - 1);
        appendMoveJumpCaptureState(s);
    }
	
    // South West
    if(pod.prongAt(DIR_SW) != 0 && x - 1 >= 0 && y - 1 >= 0 &&
       (_squares[x - 1][y - 1] == NULL ||
        (!_squares[x - 1][y - 1]->hasEnemyOf(_playerToMove)))) {
        buf = s_STR_LOCS[x][y];
        buf += str_prong_config;
        buf += '-';
        buf += s_STR_LOCS[x - 1][y - 1];
        s = new Octi2PlayerGameSearchState(this, (const char *) buf);
        s->movePodTransitionOp(x, y, pod, x - 1, y - 1);
        appendMoveJumpCaptureState(s);
    }
	
    // West
    if(pod.prongAt(DIR_W) != 0 && x - 1 >= 0 &&
       (_squares[x - 1][y] == NULL ||
        (!_squares[x - 1][y]->hasEnemyOf(_playerToMove)))) {
        buf = s_STR_LOCS[x][y];
        buf += str_prong_config;
        buf += '-';
        buf += s_STR_LOCS[x - 1][y];
        s = new Octi2PlayerGameSearchState(this, (const char *) buf);
        s->movePodTransitionOp(x, y, pod, x - 1, y);
        appendMoveJumpCaptureState(s);
    }
	
    // North West
    if(pod.prongAt(DIR_NW) != 0 && x - 1 >= 0 && y + 1 < 9 &&
       (_squares[x - 1][y + 1] == NULL ||
        (!_squares[x - 1][y + 1]->hasEnemyOf(_playerToMove)))) {
        buf = s_STR_LOCS[x][y];
        buf += str_prong_config;
        buf += '-';
        buf += s_STR_LOCS[x - 1][y + 1];
        s = new Octi2PlayerGameSearchState(this, (const char *) buf);
        s->movePodTransitionOp(x, y, pod, x - 1, y + 1);
        appendMoveJumpCaptureState(s);
    }
}

void O2PGSS::enumerateJumpPodPlys(int x, int y, OctiPod pod) {
    char str_prong_config[17];
    String initJumpStr;
    pod.getProngConfigString(str_prong_config);
    initJumpStr = s_STR_LOCS[x][y];
    initJumpStr += str_prong_config;
    MJCMap map;
    recursiveEnumerateJumpPodPlys(x, y, x, y, pod, initJumpStr, &map, this);
}

void O2PGSS::recursiveEnumerateJumpPodPlys(int srcX, int srcY,
                                           int x, int y,
                                           OctiPod pod,
                                           const char *prevJumpStr,
                                           MJCMap *map,
                                           Octi2PlayerGameSearchState *pS) {
    String buf;
    Octi2PlayerGameSearchState *s;

    // North
    if(pod.prongAt(DIR_N) != NO_PRONG && y + 2 < 9 &&
       _squares[x][y + 1] != NULL && _squares[x][y + 2] == NULL &&
       map->tryMarkJC(x, y + 1)) {
        // Without capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x][y + 2];
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x, y + 2);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x, y + 2, pod,
                                      (const char *) buf, map, s);

        // With capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x][y + 2];
        buf += 'x';
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x, y + 2);
        s->captureTransitionOp(x, y + 1);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x, y + 2, pod,
                                      (const char *) buf, map, s);

        map->tryUnmarkJC(x, y + 1);
    }

    // North East
    if(pod.prongAt(DIR_NE) != NO_PRONG && x + 2 < 9 && y + 2 < 9 &&
       _squares[x + 1][y + 1] != NULL && _squares[x + 2][y + 2] == NULL &&
       map->tryMarkJC(x + 1, y + 1)) {
        // Without capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x + 2][y + 2];
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x + 2, y + 2);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x + 2, y + 2, pod,
                                      (const char *) buf, map, s);

        // With capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x + 2][y + 2];
        buf += 'x';
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x + 2, y + 2);
        s->captureTransitionOp(x + 1, y + 1);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x + 2, y + 2, pod,
                                      (const char *) buf, map, s);
        map->tryUnmarkJC(x + 1, y + 1);
    }

    // East
    if(pod.prongAt(DIR_E) != NO_PRONG && x + 2 < 9 &&
       _squares[x + 1][y] != NULL && _squares[x + 2][y] == NULL &&
       map->tryMarkJC(x + 1, y)) {
        // Without capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x + 2][y];
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x + 2, y);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x + 2, y, pod,
                                      (const char *) buf, map, s);
    
        // With capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x + 2][y];
        buf += 'x';
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x + 2, y);
        s->captureTransitionOp(x + 1, y);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x + 2, y, pod,
                                      (const char *) buf, map, s);

        map->tryUnmarkJC(x + 1, y);
    }

    // South East
    if(pod.prongAt(DIR_SE) != NO_PRONG && x + 2 < 9 && y - 2 >= 0 &&
       _squares[x + 1][y - 1] != NULL && _squares[x + 2][y - 2] == NULL &&
       map->tryMarkJC(x + 1, y - 1)) {
        // Without capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x + 2][y - 2];
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x + 2, y - 2);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x + 2, y - 2, pod,
                                      (const char *) buf, map, s);

        // With capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x + 2][y - 2];
        buf += 'x';
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x + 2, y - 2);
        s->captureTransitionOp(x + 1, y - 1);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x + 2, y - 2, pod,
                                      (const char *) buf, map, s);
    
        map->tryUnmarkJC(x + 1, y - 1);
    }

    // South
    if(pod.prongAt(DIR_S) != NO_PRONG && y - 2 >= 0 &&
       _squares[x][y - 1] != NULL && _squares[x][y - 2] == NULL &&
       map->tryMarkJC(x, y - 1)) {
        // Without capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x][y - 2];
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x, y - 2);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x, y - 2, pod,
                                      (const char *) buf, map, s);

        // With capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x][y - 2];
        buf += 'x';
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x, y - 2);
        s->captureTransitionOp(x, y - 1);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x, y - 2, pod,
                                      (const char *) buf, map, s);

        map->tryUnmarkJC(x, y - 1);
    }

    // South West
    if(pod.prongAt(DIR_SW) != NO_PRONG && x - 2 >= 0 && y - 2 >= 0 &&
       _squares[x - 1][y - 1] != NULL && _squares[x - 2][y - 2] == NULL &&
       map->tryMarkJC(x - 1, y - 1)) {
        // Without capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x - 2][y - 2];
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x - 2, y - 2);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x - 2, y - 2, pod,
                                      (const char *) buf, map, s);

        // With capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x - 2][y - 2];
        buf += 'x';
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x - 2, y - 2);
        s->captureTransitionOp(x - 1, y - 1);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x - 2, y - 2, pod,
                                      (const char *) buf, map, s);

        map->tryUnmarkJC(x - 1, y - 1);
    }

    // West
    if(pod.prongAt(DIR_W) != NO_PRONG && x - 2 >= 0 &&
       _squares[x - 1][y] != NULL && _squares[x - 2][y] == NULL &&
       map->tryMarkJC(x - 1, y)) {
        // Without capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x - 2][y];
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x - 2, y);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x - 2, y, pod,
                                      (const char *) buf, map, s);

        // With capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x - 2][y];
        buf += 'x';
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x - 2, y);
        s->captureTransitionOp(x - 1, y);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x - 2, y, pod,
                                      (const char *) buf, map, s);

        map->tryUnmarkJC(x - 1, y);
    }

    // North West
    if(pod.prongAt(DIR_NW) != NO_PRONG && x - 2 >= 0 && y + 2 < 9 &&
       _squares[x - 1][y + 1] != NULL && _squares[x - 2][y + 2] == NULL &&
       map->tryMarkJC(x - 1, y + 1)) {
        // Without capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x - 2][y + 2];
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x - 2, y + 2);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x - 2, y + 2, pod,
                                      (const char *) buf, map, s);

        // With capture
        buf = prevJumpStr;
        buf += '-';
        buf += s_STR_LOCS[x - 2][y + 2];
        buf += 'x';
        s = new Octi2PlayerGameSearchState(this, pS, (const char *) buf);
        s->movePodTransitionOp(srcX, srcY, pod, x - 2, y + 2);
        s->captureTransitionOp(x - 1, y + 1);
        appendMoveJumpCaptureState(s);
        recursiveEnumerateJumpPodPlys(srcX, srcY, x - 2, y + 2, pod,
                                      (const char *) buf, map, s);

        map->tryUnmarkJC(x - 1, y + 1);
    }
}

void O2PGSS::enumerateCompositePlys(int x, int y, CompositeMove *cm,
                                    int cmCount) {
    Octi2PlayerGameSearchState *s, *head[7], *tail[7], *current[7];
    boolean carry = FALSE;
    int i;
    String buf;
    Octi2PlayerGameSearchState *states[7];
    int statesCount = 0;

    for(i = 0; i < cmCount; i++) {
        current[i] = head[i] = cm[i].head;
        tail[i] = cm[i].tail;
    }

    // If the size is less than 2, then there are no other enumerable
    // composite moves
    if(cmCount >= 2) {
        while(!carry) {
            buf = (const char *) NULL;
		
            // Combine the results
            statesCount = 0;
            states[0] = states[1] = states[2] =
                states[3] = states[4] = states[5] = states[6] = NULL;
            for(i = 0; i < cmCount; i++) {
                if(current[i] != NULL) {
                    if(statesCount != 0) {
                        buf += ',';
                    }
                    buf += current[i]->_moveDescription;
                    states[statesCount] = current[i];
                    statesCount++;
                }

            }
            if(statesCount > 1) {
                try {
                    s = new Octi2PlayerGameSearchState(this, states,
                                                       statesCount,
                                                       (const char *) buf);
                    appendCompositeState(s);
                } catch(Exception e) {
                }
            }

            // Perform addition and carry operation, which rotates the
            // vectors like an odometer.
            for(i = cmCount - 1; i >= 0; i--) {
                // Advance
                if(current[i] != NULL) {
                    current[i] =
                        (Octi2PlayerGameSearchState *) current[i]->_next;
                } else {
                    current[i] =
                        (Octi2PlayerGameSearchState *) head[i];
                }
		    
                if(current[i] == tail[i]->_next) {
                    current[i] = NULL;
                }
		    
                // Carry
                if(current[i] == head[i]) {
                    carry = TRUE;
                } else {
                    carry = FALSE;
                    break;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// Operators for changing internal state

void O2PGSS::appendTransitionOp(const TransitionOp &op) {
    _trans.append(op);
}

void O2PGSS::addPodTransitionOp(int x, int y) {
    AddPodTransition trans;
    trans.loc.x = x;
    trans.loc.y = y;
    appendTransitionOp(TransitionOp(trans));
}

void O2PGSS::addProngTransitionOp(int x, int y, OctiPod pod, OctiDir dir) {
    AddProngTransition trans;
    trans.pod = pod;
    trans.loc.x = x;
    trans.loc.y = y;
    trans.addDir = dir;
    appendTransitionOp(TransitionOp(trans));
}

void O2PGSS::repositionTransitionOp(int x, int y, OctiPod pod,
                                    OctiDir addDir, OctiDir rmDir) {
    RepositionTransition trans;
    trans.pod = pod;
    trans.loc.x = x;
    trans.loc.y = y;
    trans.addDir = addDir;
    trans.rmDir = rmDir;
    appendTransitionOp(TransitionOp(trans));
}

void O2PGSS::movePodTransitionOp(int x0, int y0, OctiPod pod, int x1, int y1) {
    MovePodTransition trans;
    trans.pod = pod;
    trans.src.x = x0;
    trans.src.y = y0;
    trans.dest.x = x1;
    trans.dest.y = y1;
    appendTransitionOp(TransitionOp(trans));
}

void O2PGSS::captureTransitionOp(int x, int y) {
    CaptureTransition trans;
    trans.loc.x = x;
    trans.loc.y = y;
    appendTransitionOp(TransitionOp(trans));
}

void O2PGSS::syncState() {
    if(_trans.size() > 0) {
        // copy from parent first
        copyInternalStatesFromParent();
    
        // Reset evaluation
        _evaled = FALSE;
        _eval = 0.0;
        _terminalTested = FALSE;
        _terminalValue = 0.0;

        // apply the transition operations
        Ptr<SlistIterator<TransitionOp> > iterator = 
            (SlistIterator<TransitionOp> *) _trans.getIterator();
        const TransitionOp *op;
    
        for(iterator->first(); iterator->isNotNull(); iterator->next()) {
            op = iterator->current();
            switch(op->getTransitionType()) {
            case TRANS_ADD_POD:
            {
                const AddPodTransition *t = op->getAddPodTransition();
                addPod(t->loc.x, t->loc.y);
            }
            break;
            case TRANS_ADD_PRONG:
            {
                const AddProngTransition *t = op->getAddProngTransition();
                addProng(t->loc.x, t->loc.y, t->pod, t->addDir);
            }
            break;
            case TRANS_REPOSITION:
            {
                const RepositionTransition *t = op->getRepositionTransition();
                repositionProng(t->loc.x, t->loc.y, t->pod, t->rmDir, t->addDir);
            }
            break;
            case TRANS_MOVE_POD:
            {
                const MovePodTransition *t = op->getMovePodTransition();
                movePod(t->src.x, t->src.y, t->pod, t->dest.x, t->dest.y);
            }
            break;
            case TRANS_CAPTURE:
            {
                const CaptureTransition *t = op->getCaptureTransition();
                capture(t->loc.x, t->loc.y);
            }
            break;
            }
        }

        _trans.clear();  // release the transition operators
    }
}

void O2PGSS::addPod(int x, int y) {
    boolean canLiberate = false;
    OctiPlayer player = (OctiPlayer) (_playerToMove % 2 + 1);
  
    if(_squares[x][y] == NULL) {
        if((x == 2 || x == 4 || x == 6) && (y == 2)) {
            _squares[x][y] = new Octi2PlayerSquare(A_HOME_SQUARE, x, y);
        } else if((x == 2 || x == 4 || x == 6) && (y == 6)) {
            _squares[x][y] = new Octi2PlayerSquare(B_HOME_SQUARE, x, y);
        } else {
            _squares[x][y] = new Octi2PlayerSquare(NEUTRAL_SQUARE, x, y);
        }
        appendValidSquare(x, y);
    }
  
    if(x == 2 || x == 4 || x == 6) {
        if(player == PLAYER_A && y == 6) {
            canLiberate = TRUE;
        } else if(player == PLAYER_B && y == 2) {
            canLiberate = TRUE;
        }
    }
  
    _squares[x][y]->addPod(OctiPod((OctiPodType) player));
    if(canLiberate && _capturedPods[player - 1] != 0) {
        _capturedPods[player - 1]--;
    } else {
        _freePods[player - 1]--;
    }
}

void O2PGSS::addProng(int x, int y, OctiPod pod, OctiDir dir) {
    int k;
    OctiPod *matchPod;
    int numPods;
    OctiPlayer player = (OctiPlayer) (_playerToMove % 2 + 1);
  
    numPods = _squares[x][y]->getNumPods();

    for(k = numPods - 1; k >= 0; k--) {
        matchPod = _squares[x][y]->getPodAt(k);
        if(matchPod->hasSameConfig(pod)) {
            matchPod->addProng(dir, FALSE);
            _freeProngs[player - 1]--;
            break;
        }
    }
}

void O2PGSS::repositionProng(int x, int y, OctiPod pod,
                             OctiDir rmDir, OctiDir addDir) {
    int k;
    OctiPod *matchPod;
    int numPods;
    OctiPlayer player = (OctiPlayer) (_playerToMove % 2 + 1);
  
    numPods = _squares[x][y]->getNumPods();

    for(k = numPods - 1; k >= 0; k--) {
        matchPod = _squares[x][y]->getPodAt(k);
        if(matchPod->hasSameConfig(pod)) {
            matchPod->removeProng(rmDir);
            matchPod->addProng(addDir, FALSE);
            _freeProngs[player - 1]--;
            break;
        }
    }
}

void O2PGSS::movePod(int x0, int y0, OctiPod pod, int x1, int y1) {
    int k;
    OctiPod matchPod;
    int numPods;
    //OctiPlayer player = (OctiPlayer) (_playerToMove % 2 + 1);

    if(_squares[x1][y1] == NULL) {
        if((x1 == 2 || x1 == 4 || x1 == 6) && (y1 == 2)) {
            _squares[x1][y1] = new Octi2PlayerSquare(A_HOME_SQUARE, x1, y1);
        } else if((x1 == 2 || x1 == 4 || x1 == 6) && (y1 == 6)) {
            _squares[x1][y1] = new Octi2PlayerSquare(B_HOME_SQUARE, x1, y1);
        } else {
            _squares[x1][y1] = new Octi2PlayerSquare(NEUTRAL_SQUARE, x1, y1);
        }
        appendValidSquare(x1, y1);
    }

    numPods = _squares[x0][y0]->getNumPods();

    for(k = numPods - 1; k >= 0; k--) {
        matchPod = *_squares[x0][y0]->getPodAt(k);
        if(matchPod.hasSameConfig(pod)) {
            _squares[x0][y0]->removePod(matchPod);
            _squares[x1][y1]->addPod(matchPod);

            if(_squares[x0][y0]->isEmpty()) {
                _squares[x0][y0] = NULL;
                removeValidSquare(x0, y0);
            }
            break;
        }
    }
}

void O2PGSS::capture(int x, int y) {
    int k, l;
    OctiProngType p;
    int numCapProngs, numCapSuperProngs, numCapPods, capType;
    OctiPod capPod;
    OctiPlayer player = (OctiPlayer) (_playerToMove % 2 + 1);

    numCapPods = _squares[x][y]->getNumPods();
    numCapProngs = 0;
    numCapSuperProngs = 0;	
    capType = _squares[x][y]->getPodAt(0)->getType();

    for(k = numCapPods - 1; k >= 0; k--) {
        capPod = *_squares[x][y]->getPodAt(k);
        for(l = 0; l < 8; l++) {
            p = capPod.prongAt((OctiDir) l);
            if(p == NORMAL_PRONG) {
                numCapProngs++;
            } else if(p == SUPER_PRONG) {
                numCapSuperProngs++;
            }
        }
    }

    if(capType == player) {
        _freeProngs[player - 1] += numCapProngs;
        _freeSuperProngs[player - 1] += numCapSuperProngs;
        _freePods[player - 1] += numCapPods;
    } else {
        _freeProngs[player - 1] += numCapProngs;
        _freeSuperProngs[player - 1] += numCapSuperProngs;
        _capturedPods[capPod.getType() - 1] += numCapPods;
    }

    _squares[x][y] = NULL;
    removeValidSquare(x, y);
}

//////////////////////////////////////////////////////////////////////////////
// State transitions

void O2PGSS::copyInternalStatesFromParent() {
    // Copy and update the internal states
    Octi2PlayerGameSearchState *copy =
        (Octi2PlayerGameSearchState *) (const Octi2PlayerGameSearchState *) _parent;
    _playerToMove = (OctiPlayer) ((copy->_playerToMove % 2) + 1);
    _freePods[0] = copy->_freePods[0];
    _freePods[1] = copy->_freePods[1];
    _freeProngs[0] = copy->_freeProngs[0];
    _freeProngs[1] = copy->_freeProngs[1];
    _freeSuperProngs[0] = copy->_freeSuperProngs[0];
    _freeSuperProngs[1] = copy->_freeSuperProngs[1];
    _capturedPods[0] = copy->_capturedPods[0];
    _capturedPods[1] = copy->_capturedPods[1];
  
    // Copy squares and pods
    Ptr<SlistIterator<ValidSquare> > iterator = 
        (SlistIterator<ValidSquare> *) copy->_validSquares.getIterator();
    const ValidSquare *current;
    const Octi2PlayerSquare *square;
    int x, y;
    for(iterator->first(); iterator->isNotNull(); iterator->next()) {
        current = iterator->current();
        x = current->loc.x;
        y = current->loc.y;
        square = current->square.ptr();
        _squares[x][y] = new Octi2PlayerSquare(*square);
        appendValidSquare(current->loc.x, current->loc.y);
    }
}

void O2PGSS::appendValidSquare(int x, int y) {
    ValidSquare sq;
    sq.loc.x = x;
    sq.loc.y = y;
    sq.square = _squares[x][y];
    _validSquares.append(sq);
}

void O2PGSS::removeValidSquare(int x, int y) {
    ValidSquare sq;
    sq.loc.x = x;
    sq.loc.y = y;
    sq.square = _squares[x][y];
    _validSquares.removeElement(sq);
}

void O2PGSS::appendAddPodState(Octi2PlayerGameSearchState *s) {
    if(_headAddPodStates == NULL) {
        _headAddPodStates = s;
        _tailAddPodStates = s;
        s->_next = NULL;
        s->_prev = NULL;
    } else {
        s->_next = NULL;
        s->_prev = _tailAddPodStates;
        _tailAddPodStates->_next = s;
        _tailAddPodStates = s;
    }
}

void O2PGSS::appendAddProngState(Octi2PlayerGameSearchState *s) {
    if(_headAddProngStates == NULL) {
        _headAddProngStates = s;
        _tailAddProngStates = s;
        s->_next = NULL;
        s->_prev = NULL;
    } else {
        s->_next = NULL;
        s->_prev = _tailAddProngStates;
        _tailAddProngStates->_next = s;
        _tailAddProngStates = s;
    }
}

void O2PGSS::appendMoveJumpCaptureState(Octi2PlayerGameSearchState *s) {
    if(_headMoveJumpCaptureStates == NULL) {
        _headMoveJumpCaptureStates = s;
        _tailMoveJumpCaptureStates = s;
        s->_next = NULL;
        s->_prev = NULL;
    } else {
        s->_next = NULL;
        s->_prev = _tailMoveJumpCaptureStates;
        _tailMoveJumpCaptureStates->_next = s;
        _tailMoveJumpCaptureStates = s;
    }
}

void O2PGSS::appendCompositeState(Octi2PlayerGameSearchState *s) {
    if(_headCompositeStates == NULL) {
        _headCompositeStates = s;
        _tailCompositeStates = s;
        s->_next = NULL;
        s->_prev = NULL;
    } else {
        s->_next = NULL;
        s->_prev = _tailCompositeStates;
        _tailCompositeStates->_next = s;
        _tailCompositeStates = s;
    }
}

void O2PGSS::orderListAndUpdateSuccessors() {
    // The ordering is:
    // CompositeStates -> MoveJumpCaptureStates -> AddProngStates -> AddPodStates
    addSuccessors(_headCompositeStates);
    addSuccessors(_headMoveJumpCaptureStates);
    addSuccessors(_headAddProngStates);
    addSuccessors(_headAddPodStates);
  
    // Set the temporary pointers to null
    _headAddPodStates = NULL;
    _tailAddPodStates = NULL;
    _headAddProngStates = NULL;
    _tailAddProngStates = NULL;
    _headMoveJumpCaptureStates = NULL;
    _tailMoveJumpCaptureStates = NULL;
    _headCompositeStates = NULL;
    _tailCompositeStates = NULL;
}

//////////////////////////////////////////////////////////////////////////////
// State evaluation

boolean O2PGSS::isEnemySquareOf(int x, int y, OctiPlayer player) {
    if(((player == PLAYER_A &&
         ((x == 2 || x == 4 || x == 6) && (y == 6))) ||
        (player == PLAYER_B &&
         ((x == 2 || x == 4 || x == 6) && (y == 2))))) {
        return TRUE;
    }

    return FALSE;
}

float O2PGSS::getDistance(int x0, int y0, int x1, int y1) {
    int x = (x0 > x1) ? x0 - x1 : x1 - x0;
    int y = (y0 > y1) ? y0 - y1 : y1 - y0;

    return (float) sqrt((double) (x * x + y * y));
}

void O2PGSS::evaluateJumpCapture(int x, int y, OctiPodType type, OctiPod pod,
                                 PodMobilityEval *mob, MJCMap *map) const {
    int k, l, numPods;
    OctiPod capPod;

    // North
    if(pod.prongAt(DIR_N) != NO_PRONG && y + 2 < 9 &&
       _squares[x][y + 1] != NULL && _squares[x][y + 2] == NULL &&
       map->tryMarkJC(x, y + 1)) {
        numPods = _squares[x][y + 1]->getNumPods();
        if(_squares[x][y + 1]->getPodAt(0)->getType() != type) {
            for(k = numPods - 1; k >= 0; k--) {
                capPod = *_squares[x][y + 1]->getPodAt(k);
                for(l = DIR_N; l <= DIR_NW; l++) {
                    if(capPod.prongAt((OctiDir) l) == NORMAL_PRONG) {
                        mob->capturableEnemyProngs++;
                    }
                }
                mob->capturableEnemyPods++;
            }
        }
        if(map->tryMarkOD(x, y + 2)) {
            mob->reachableSquares++;
        }
        evaluateJumpCapture(x, y + 2, type, pod, mob, map);
        map->tryUnmarkJC(x, y + 1);
    }

    // North East
    if(pod.prongAt(DIR_NE) != NO_PRONG && x + 2 < 9 && y + 2 < 9 &&
       _squares[x + 1][y + 1] != NULL && _squares[x + 2][y + 2] == NULL &&
       map->tryMarkJC(x + 1, y + 1)) {
        numPods = _squares[x + 1][y + 1]->getNumPods();
        if(_squares[x + 1][y + 1]->getPodAt(0)->getType() != type) {
            for(k = numPods - 1; k >= 0; k--) {
                capPod = *_squares[x + 1][y + 1]->getPodAt(k);
                for(l = DIR_N; l <= DIR_NW; l++) {
                    if(capPod.prongAt((OctiDir) l) == NORMAL_PRONG) {
                        mob->capturableEnemyProngs++;
                    }
                }
                mob->capturableEnemyPods++;
            }
        }
        if(map->tryMarkOD(x + 2, y + 2)) {
            mob->reachableSquares++;
        }
        evaluateJumpCapture(x + 2, y + 2, type, pod, mob, map);
        map->tryUnmarkJC(x + 1, y + 1);
    }

    // East
    if(pod.prongAt(DIR_E) != NO_PRONG && x + 2 < 9 &&
       _squares[x + 1][y] != NULL && _squares[x + 2][y] == NULL &&
       map->tryMarkJC(x + 1, y)) {
        numPods = _squares[x + 1][y]->getNumPods();
        if(_squares[x + 1][y]->getPodAt(0)->getType() != type) {
            for(k = numPods - 1; k >= 0; k--) {
                capPod = *_squares[x + 1][y]->getPodAt(k);
                for(l = DIR_N; l <= DIR_NW; l++) {
                    if(capPod.prongAt((OctiDir) l) == NORMAL_PRONG) {
                        mob->capturableEnemyProngs++;
                    }
                }
                mob->capturableEnemyPods++;
            }
        }
        if(map->tryMarkOD(x + 2, y)) {
            mob->reachableSquares++;
        }
        evaluateJumpCapture(x + 2, y, type, pod, mob, map);
        map->tryUnmarkJC(x + 1, y);
    }

    // South East
    if(pod.prongAt(DIR_SE) != NO_PRONG && x + 2 < 9 && y - 2 >= 0 &&
       _squares[x + 1][y - 1] != NULL && _squares[x + 2][y - 2] == NULL &&
       map->tryMarkJC(x + 1, y - 1)) {
        numPods = _squares[x + 1][y - 1]->getNumPods();
        if(_squares[x + 1][y - 1]->getPodAt(0)->getType() != type) {
            for(k = numPods - 1; k >= 0; k--) {
                capPod = *_squares[x + 1][y - 1]->getPodAt(k);
                for(l = DIR_N; l <= DIR_NW; l++) {
                    if(capPod.prongAt((OctiDir) l) == NORMAL_PRONG) {
                        mob->capturableEnemyProngs++;
                    }
                }
                mob->capturableEnemyPods++;
            }
        }
        if(map->tryMarkOD(x + 2, y - 2)) {
            mob->reachableSquares++;
        }
        evaluateJumpCapture(x + 2, y - 2, type, pod, mob, map);
        map->tryUnmarkJC(x + 1, y - 1);
    }

    // South
    if(pod.prongAt(DIR_S) != NO_PRONG && y - 2 >= 0 &&
       _squares[x][y - 1] != NULL && _squares[x][y - 2] == NULL &&
       map->tryMarkJC(x, y - 1)) {
        numPods = _squares[x][y - 1]->getNumPods();
        if(_squares[x][y - 1]->getPodAt(0)->getType() != type) {
            for(k = numPods - 1; k >= 0; k--) {
                capPod = *_squares[x][y - 1]->getPodAt(k);
                for(l = DIR_N; l <= DIR_NW; l++) {
                    if(capPod.prongAt((OctiDir) l) == NORMAL_PRONG) {
                        mob->capturableEnemyProngs++;
                    }
                }
                mob->capturableEnemyPods++;
            }
        }
        if(map->tryMarkOD(x, y - 2)) {
            mob->reachableSquares++;
        }
        evaluateJumpCapture(x, y - 2, type, pod, mob, map);
        map->tryUnmarkJC(x, y - 1);
    }

    // South West
    if(pod.prongAt(DIR_SW) != NO_PRONG && x - 2 >= 0 && y - 2 >= 0 &&
       _squares[x - 1][y - 1] != NULL && _squares[x - 2][y - 2] == NULL &&
       map->tryMarkJC(x - 1, y - 1)) {
        numPods = _squares[x - 1][y - 1]->getNumPods();
        if(_squares[x - 1][y - 1]->getPodAt(0)->getType() != type) {
            for(k = numPods - 1; k >= 0; k--) {
                capPod = *_squares[x - 1][y - 1]->getPodAt(k);
                for(l = DIR_N; l <= DIR_NW; l++) {
                    if(capPod.prongAt((OctiDir) l) == NORMAL_PRONG) {
                        mob->capturableEnemyProngs++;
                    }
                }
                mob->capturableEnemyPods++;
            }
        }
        if(map->tryMarkOD(x - 2, y - 2)) {
            mob->reachableSquares++;
        }
        evaluateJumpCapture(x - 2, y - 2, type, pod, mob, map);
        map->tryUnmarkJC(x - 1, y - 1);
    }

    // West
    if(pod.prongAt(DIR_W) != NO_PRONG && x - 2 >= 0 &&
       _squares[x - 1][y] != NULL && _squares[x - 2][y] == NULL &&
       map->tryMarkJC(x - 1, y)) {
        numPods = _squares[x - 1][y]->getNumPods();
        if(_squares[x - 1][y]->getPodAt(0)->getType() != type) {
            for(k = numPods - 1; k >= 0; k--) {
                capPod = *_squares[x - 1][y]->getPodAt(k);
                for(l = DIR_N; l <= DIR_NW; l++) {
                    if(capPod.prongAt((OctiDir) l) == NORMAL_PRONG) {
                        mob->capturableEnemyProngs++;
                    }
                }
                mob->capturableEnemyPods++;
            }
        }
        if(map->tryMarkOD(x - 2, y)) {
            mob->reachableSquares++;
        }
        evaluateJumpCapture(x - 2, y, type, pod, mob, map);
        map->tryUnmarkJC(x - 1, y);
    }

    // North West
    if(pod.prongAt(DIR_NW) != NO_PRONG && x - 2 >= 0 && y + 2 < 9 &&
       _squares[x - 1][y + 1] != NULL && _squares[x - 2][y + 2] == NULL &&
       map->tryMarkJC(x - 1, y + 1)) {
        numPods = _squares[x - 1][y + 1]->getNumPods();
        if(_squares[x - 1][y + 1]->getPodAt(0)->getType() != type) {
            for(k = numPods - 1; k >= 0; k--) {
                capPod = *_squares[x - 1][y + 1]->getPodAt(k);
                for(l = DIR_N; l <= DIR_NW; l++) {
                    if(capPod.prongAt((OctiDir) l) == NORMAL_PRONG) {
                        mob->capturableEnemyProngs++;
                    }
                }
                mob->capturableEnemyPods++;
            }
        }
        if(map->tryMarkOD(x - 2, y + 2)) {
            mob->reachableSquares++;
        }
        evaluateJumpCapture(x - 2, y + 2, type, pod, mob, map);
        map->tryUnmarkJC(x - 1, y + 1);
    }
}

float O2PGSS::evaluate() const {
    int aPodsOnBoard = 0, bPodsOnBoard = 0;
    int aProngsOnBoard = 0, bProngsOnBoard = 0;
    int aPodsInReserve = 0, bPodsInReserve = 0;
    int aProngsInReserve = 0, bProngsInReserve = 0;
    int aMobility = 0, bMobility = 0;
    int aThreatToCapturePods = 0, bThreatToCapturePods = 0;
    int aThreatToCaptureProngs = 0, bThreatToCaptureProngs = 0;
    float aDistance1, aDistance2, aDistance3;
    float bDistance1, bDistance2, bDistance3;
    PodMobilityEval mob;
    float material, mobility, threat, distance;
    MJCMap map;
    const ValidSquare *current;
    const OctiSquare *square;
    int x, y, k;
    OctiPlayer type;
    int numPods;
    int prong;
    OctiPod pod;

    if(_evaled) {
        return _eval;
    }

    if(isTerminal()) {
        return _terminalValue;
    }

    Octi2PlayerGameSearchState *self =
        (Octi2PlayerGameSearchState *) (const Octi2PlayerGameSearchState *) this;
    self->syncState();

    // Calculate materials
    aPodsInReserve = _freePods[0];
    bPodsInReserve = _freePods[1];
    aProngsInReserve = _freeProngs[0];
    bProngsInReserve = _freeProngs[1];
    aPodsOnBoard = 7 - aPodsInReserve - _capturedPods[0];
    bPodsOnBoard = 7 - bPodsInReserve - _capturedPods[1];

    // For each pod in the board, determine its mobility and threat
    aDistance1 = aDistance2 = aDistance3 =
        bDistance1 = bDistance2 = bDistance3 = 99999;

    Ptr<SlistIterator<ValidSquare> > iterator = 
        (SlistIterator<ValidSquare> *) _validSquares.getIterator();
    for(iterator->first(); iterator->isNotNull(); iterator->next()) {
        current = iterator->current();
        x = current->loc.x;
        y = current->loc.y;
        square = current->square.ptr();
        numPods = square->getNumPods();

        type = (OctiPlayer) square->getPodAt(0)->getType();
        for(k = numPods - 1; k >= 0; k--) {
            pod = *square->getPodAt(k);
            map.reset();
		
            mob.numProngs = 0;
            mob.reachableSquares = 0;
            mob.capturableEnemyPods = 0;
            mob.capturableEnemyProngs = 0;
		
            // Go North
            prong = pod.prongAt(DIR_N);
            if(prong != NO_PRONG) {
                mob.numProngs++;
            }
            if(prong != NO_PRONG && y + 1 < 9 &&
               (_squares[x][y + 1] == NULL ||
                (!_squares[x][y + 1]->hasEnemyOf(type)))) {
                mob.reachableSquares++;
                map.tryMarkOD(x, y + 1);
            }
		
            // Go North East
            prong = pod.prongAt(DIR_NE);
            if(prong != NO_PRONG) {
                mob.numProngs++;
            }
            if(prong != NO_PRONG && x + 1 < 9 && y + 1 < 9 &&
               (_squares[x + 1][y + 1] == NULL ||
                (!_squares[x + 1][y + 1]->hasEnemyOf(type)))) {
                mob.reachableSquares++;
                map.tryMarkOD(x + 1, y + 1);
            }
		
            // Go East
            prong = pod.prongAt(DIR_E);
            if(prong != NO_PRONG) {
                mob.numProngs++;
            }
            if(prong != NO_PRONG && x + 1 < 9 &&
               (_squares[x + 1][y] == NULL ||
                (!_squares[x + 1][y]->hasEnemyOf(type)))) {
                mob.reachableSquares++;
                map.tryMarkOD(x + 1, y);
            }
		
            // Go South East
            prong = pod.prongAt(DIR_SE);
            if(prong != NO_PRONG) {
                mob.numProngs++;
            }
            if(prong != NO_PRONG && x + 1 < 9 && y - 1 >= 0 &&
               (_squares[x + 1][y - 1] == NULL ||
                (!_squares[x + 1][y - 1]->hasEnemyOf(type)))) {
                mob.reachableSquares++;
                map.tryMarkOD(x + 1, y - 1);
            }
		
            // Go South
            prong = pod.prongAt(DIR_S);
            if(prong != NO_PRONG) {
                mob.numProngs++;
            }
            if(prong != NO_PRONG && y - 1 >= 0 &&
               (_squares[x][y - 1] == NULL ||
                (!_squares[x][y - 1]->hasEnemyOf(type)))) {
                mob.reachableSquares++;
                map.tryMarkOD(x, y - 1);
            }
		
            // Go South West
            prong = pod.prongAt(DIR_SW);
            if(prong != NO_PRONG) {
                mob.numProngs++;
            }
            if(prong != NO_PRONG && x - 1 >= 0 && y - 1 >= 0 &&
               (_squares[x - 1][y - 1] == NULL ||
                (!_squares[x - 1][y - 1]->hasEnemyOf(type)))) {
                mob.reachableSquares++;
                map.tryMarkOD(x - 1, y - 1);
            }
		
            // Go West
            prong = pod.prongAt(DIR_W);
            if(prong != NO_PRONG) {
                mob.numProngs++;
            }
            if(prong != NO_PRONG && x - 1 >= 0 &&
               (_squares[x - 1][y] == NULL ||
                (!_squares[x - 1][y]->hasEnemyOf(type)))) {
                mob.reachableSquares++;
                map.tryMarkOD(x - 1, y);
            }
		
            // Go North West
            prong = pod.prongAt(DIR_NW);
            if(prong != NO_PRONG) {
                mob.numProngs++;
            }
            if(prong != NO_PRONG && x - 1 >= 0 && y + 1 < 9 &&
               (_squares[x - 1][y + 1] == NULL ||
                (!_squares[x - 1][y + 1]->hasEnemyOf(type)))) {
                mob.reachableSquares++;
                map.tryMarkOD(x - 1, y + 1);
            }

            //evaluateJumpCapture(x, y, (OctiPodType) type, pod, &mob, &map);
		
            // Calculate the distances to the enemy squares
            // and minimize
            switch(type) {
            case PLAYER_A:
                distance = getDistance(x, y, 2, 6);
                aDistance1 = (distance < aDistance1) ? distance : aDistance1;
                distance = getDistance(x, y, 4, 6);
                aDistance2 = (distance < aDistance2) ? distance : aDistance2;
                distance = getDistance(x, y, 6, 6);
                aDistance3 = (distance < aDistance3) ? distance : aDistance3;
                break;
            case PLAYER_B:
                distance = getDistance(x, y, 2, 2);
                bDistance1 = (distance < bDistance1) ? distance : bDistance1;
                distance = getDistance(x, y, 4, 2);
                bDistance2 = (distance < bDistance2) ? distance : bDistance2;
                distance = getDistance(x, y, 6, 2);
                bDistance3 = (distance < bDistance3) ? distance : bDistance3;
                break;
            }
		
            // Tally the result
            switch(type) {
            case A_POD:
                aProngsOnBoard += mob.numProngs;
                aMobility += mob.reachableSquares;
                aThreatToCapturePods += mob.capturableEnemyPods;
                aThreatToCaptureProngs += mob.capturableEnemyProngs;
                break;
            case B_POD:
                bProngsOnBoard += mob.numProngs;
                bMobility += mob.reachableSquares;
                bThreatToCapturePods += mob.capturableEnemyPods;
                bThreatToCaptureProngs += mob.capturableEnemyProngs;
                break;
            }
        }
    }

    material =
        MINOR_WEIGHT_ONBOARD *
        (MINOR_WEIGHT_POD * (aPodsOnBoard - bPodsOnBoard) +
         MINOR_WEIGHT_PRONG * (aProngsOnBoard - bProngsOnBoard)) +
        MINOR_WEIGHT_RESERVE *
        (MINOR_WEIGHT_POD * (aPodsInReserve - bPodsInReserve) +
         MINOR_WEIGHT_PRONG * (aProngsInReserve - bProngsInReserve));
  
    mobility = aMobility - bMobility;

    if(_playerToMove == PLAYER_A) {
        bThreatToCapturePods = 0;
        bThreatToCaptureProngs = 0;
    } else {
        aThreatToCapturePods = 0;
        aThreatToCaptureProngs = 0;
    }
  
    threat =
        MINOR_WEIGHT_THREAT_POD *
        (aThreatToCapturePods - bThreatToCapturePods) +
        MINOR_WEIGHT_THREAT_PRONG *
        (aThreatToCaptureProngs - bThreatToCaptureProngs);

    distance =
        (bDistance1 + bDistance2 + bDistance3) -
        (aDistance1 + aDistance2 + aDistance3);

    self->_eval =
        MAJOR_WEIGHT_MATERIAL * material +
        MAJOR_WEIGHT_MOBILITY * mobility +
        MAJOR_WEIGHT_THREAT * threat +
        MAJOR_WEIGHT_DISTANCE * distance;

    self->_evaled = TRUE;
    return _eval;
}

boolean O2PGSS::isTerminal() const {
    Octi2PlayerGameSearchState *self =
        (Octi2PlayerGameSearchState *) (const Octi2PlayerGameSearchState *) this;

    if(_terminalTested) {
        return _terminalValue != 0.0;
    }

    self->syncState();

    self->_terminalValue = 0.0;

    if(_capturedPods[0] == 7) {
        self->_terminalValue = TERMINAL_NEGATIVE;  // A loses
    } else if(_capturedPods[1] == 7) {
        self->_terminalValue = TERMINAL_POSITIVE;  // B loses
    } else if((_squares[2][2] != NULL && _squares[2][2]->isCaptured()) &&
              (_squares[4][2] != NULL && _squares[4][2]->isCaptured()) &&
              (_squares[6][2] != NULL && _squares[6][2]->isCaptured())) {
        self->_terminalValue = TERMINAL_NEGATIVE;  // A loses
    } else if((_squares[2][6] != NULL && _squares[2][6]->isCaptured()) &&
              (_squares[4][6] != NULL && _squares[4][6]->isCaptured()) &&
              (_squares[6][6] != NULL && _squares[6][6]->isCaptured())) {
        self->_terminalValue = TERMINAL_POSITIVE;  // B loses
    }
	
    self->_terminalTested = TRUE;
    return _terminalValue != 0.0;
}

// Hash a state into a value
// How it works:
// The following must be taken into consideration when hashing a board
// state:
//   1. The class OctiPod can hashes into the range 0x00000 - 0x3aaaa (18 bits)
//   2. 9x9 = 81 squares on board
//   3. For pods not on board, it is either on reserve or captured
//
// Methodology:
//   1. For each pod on the square get a hash code of the pod with its
//      configuration (18) bits.
//   2. Tag the Square Id to the hash value (7 bits, since 81 square is
//      representable by 2^7 = 128).
//   3. For each pod off the squares, get a hash code. If it is on reserve,
//      the square Id is 127; if it is captured, its square Id is 126.
//   4. Add the number of prongs on reserve for player A left-shifted by
//      25 bits.
//   5. Add the number of prongs on reserve for player B left-shifted by
//      26 bits.
//
// Hopefully, this is a low-collision hashing scheme... Hmm... ?!
// Let's revisit this crap later!
//
unsigned long O2PGSS::hashCode() const {
    Octi2PlayerGameSearchState *self =
        (Octi2PlayerGameSearchState *) (const Octi2PlayerGameSearchState *) this;
    self->syncState();

    register unsigned long h = 0;

    Ptr<SlistIterator<ValidSquare> > iterator = 
        (SlistIterator<ValidSquare> *) self->_validSquares.getIterator();
    const ValidSquare *current;
    const Octi2PlayerSquare *square;
    int id;
    for(iterator->first(); iterator->isNotNull(); iterator->next()) {
        current = iterator->current();
        square = current->square.ptr();
        id = square->getLocId();
        for(int i = square->getNumPods(); i > 0; i--) {
            h += ((id << 18) | (square->getPodAt(i - 1))->hashCode());
        }
    }

    static const OctiPod sl_O2PGSSaPod(A_POD);
    static const OctiPod sl_O2PGSSbPod(B_POD);

    h += ((0x8f << 18) | sl_O2PGSSaPod.hashCode()) * _freePods[0];
    h += ((0x8f << 18) | sl_O2PGSSbPod.hashCode()) * _freePods[1];
    h += ((0x8e << 18) | sl_O2PGSSaPod.hashCode()) * _capturedPods[0];
    h += ((0x8e << 18) | sl_O2PGSSbPod.hashCode()) * _capturedPods[1];
    h += (_freeProngs[0] << 25);
    h += (_freeProngs[1] << 26);

    return h;
}

// The idea here is same as the hashCode() method.
// However, instead of using addition (+), the XOR (^) operator is used.
// This means that two states call hashCode() to get the same value should
// call hasChecksum() to a different value.
// Well, not neccessarilly. However, the probability should be siginificantly
// reduced.
//
// (hmmm... the word 'should' is used twice here. Faith?! I guess...)
word32 O2PGSS::hashChecksum() const {
    Octi2PlayerGameSearchState *self =
        (Octi2PlayerGameSearchState *) (const Octi2PlayerGameSearchState *) this;
    self->syncState();

    register unsigned long h = 0;

    Ptr<SlistIterator<ValidSquare> > iterator = 
        (SlistIterator<ValidSquare> *) self->_validSquares.getIterator();
    const ValidSquare *current;
    const Octi2PlayerSquare *square;
    int id;
    for(iterator->first(); iterator->isNotNull(); iterator->next()) {
        current = iterator->current();
        square = current->square.ptr();
        id = square->getLocId();
        for(int i = square->getNumPods(); i > 0; i--) {
            h ^= ((id << 18) | (square->getPodAt(i - 1))->hashCode());
        }
    }

    static const OctiPod sl_O2PGSSaPod(A_POD);
    static const OctiPod sl_O2PGSSbPod(B_POD);

    h ^= ((0x8f << 18) | sl_O2PGSSaPod.hashCode()) * _freePods[0];
    h ^= ((0x8f << 18) | sl_O2PGSSbPod.hashCode()) * _freePods[1];
    h ^= ((0x8e << 18) | sl_O2PGSSaPod.hashCode()) * _capturedPods[0];
    h ^= ((0x8e << 18) | sl_O2PGSSbPod.hashCode()) * _capturedPods[1];
    h ^= (_freeProngs[0] << 25);
    h ^= (_freeProngs[1] << 26);

    return h;
}


void O2PGSS::printState(ostream &out) const {
    int x, y, k, l;
    OctiPod pod;
    int numPods;
    int prongs[9][8];
    char podOwners[9];

    Octi2PlayerGameSearchState *self =
        (Octi2PlayerGameSearchState *) (const Octi2PlayerGameSearchState *) this;
    self->syncState();

    out << "Move = " << _moveDescription << endl;
    out << "Current Player Move = " << (_playerToMove == PLAYER_A ? "A" : "B") << endl;
    out << "A's [Pods, Captured, Prongs, Super] = ";
    out << _freePods[0];
    out << ", " << _capturedPods[0];
    out << ", " << _freeProngs[0];
    out << ", " << _freeSuperProngs[0] << endl;
    out << "B's [Pods, Captured, Prongs, Super] = ";
    out << _freePods[1];
    out << ", " << _capturedPods[1];
    out << ", " << _freeProngs[1];
    out << ", " << _freeSuperProngs[1] << endl;

    out << "+-----+-----+-----+-----+-----+-----+-----+-----+-----+" << endl;
    for(y = 8; y >= 0; y--) {
        for(x = 0; x < 9; x++) {
            for(l = 0; l < 8; l++) {
                prongs[x][l] = 0;
            }
        }
    
        for(x = 0; x < 9; x++) {
            if(_squares[x][y] != NULL) {
                numPods = _squares[x][y]->getNumPods();
                if(_squares[x][y]->isEmpty()) {
                    podOwners[x] = ' ';
                } else {
                    podOwners[x] = _squares[x][y]->getPodAt(0)->getType() +
                        'A' - 1;
                }
                for(k = numPods - 1; k >= 0; k--) {
                    pod = *_squares[x][y]->getPodAt(k);
                    for(l = 0; l < 8; l++) {
                        prongs[x][l] |= (pod.prongAt((OctiDir) l) > prongs[x][l] ?
                                         pod.prongAt((OctiDir) l) : prongs[x][l]);
                    }
                }
            } else {
                podOwners[x] = ' ';
            }
        }

        // Print grid level 1
        for(x = 0; x < 9; x++) {
            out << "| ";
            out << (prongs[x][7] == 0 ? " " : "\\");
            out << (prongs[x][0] == 0 ? " " : "|");
            out << (prongs[x][1] == 0 ? " " : "/");
            out << " ";
        }
        out << "|" << endl;
	    
        // Print grid level 2
        for(x = 0; x < 9; x++) {
            out << "| ";
            out << (prongs[x][6] == 0 ? " " : "-");
            out << (podOwners[x]);
            out << (prongs[x][2] == 0 ? " " : "-");
            out << " ";
        }
        out << "|" << endl;
	    
        // Print grid level 1
        for(x = 0; x < 9; x++) {
            out << "| ";
            out << (prongs[x][5] == 0 ? " " : "/");
            out << (prongs[x][4] == 0 ? " " : "|");
            out << (prongs[x][3] == 0 ? " " : "\\");
            out << " ";
        }
        out << "|" << endl;
        out << "+-----+-----+-----+-----+-----+-----+-----+-----+-----+" << endl;
    }
}

boolean O2PGSS::operator==(const BaseObject &object) const {
    Octi2PlayerGameSearchState *other = (Octi2PlayerGameSearchState *)
        (const Octi2PlayerGameSearchState *) &object;
    Octi2PlayerGameSearchState *self = (Octi2PlayerGameSearchState *)
        (const Octi2PlayerGameSearchState *) this;

    other->syncState();
    self->syncState();

    boolean check1;
    check1 =
        (_playerToMove == other->_playerToMove) &&
        (_freePods[0] == other->_freePods[0]) &&
        (_freePods[1] == other->_freePods[1]) &&
        (_freeProngs[0] == other->_freeProngs[0]) &&
        (_freeProngs[1] == other->_freeProngs[1]) &&
        (_freeSuperProngs[0] == other->_freeSuperProngs[0]) &&
        (_freeSuperProngs[1] == other->_freeSuperProngs[1]) &&
        (_capturedPods[0] == other->_capturedPods[0]) &&
        (_capturedPods[1] == other->_capturedPods[1]);
  
    if(!check1) {
        return FALSE;
    }

    Ptr<SlistIterator<ValidSquare> > iterator = 
        (SlistIterator<ValidSquare> *) other->_validSquares.getIterator();
    const ValidSquare *current;
    const Octi2PlayerSquare *square;
    int x, y;
    for(iterator->first(); iterator->isNotNull(); iterator->next()) {
        current = iterator->current();
        x = current->loc.x;
        y = current->loc.y;
        square = current->square.ptr();
        if(_squares[x][y] == NULL || !_squares[x][y]->hasSameMaterialsAs(*square)) {
            return FALSE;
        }
    }

    return TRUE;
}

BaseObject *O2PGSS::clone() const {
    return new Octi2PlayerGameSearchState(this);
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
