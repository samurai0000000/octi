/*************************************************************************
 * $Id: Octi2PlayerGameSearchState.hxx,v 1.5 2000/12/10 19:41:31 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/12/10 19:41:31 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created: 10/1/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _OCTI2PLAYERGAMESEARCHSTATE_HXX_
#define _OCTI2PLAYERGAMESEARCHSTATE_HXX_

#include "ByteUtil.hxx"
#include "BaseObject.hxx"
#include "String.hxx"
#include "Slist.hxx"
#include "AISearchAlgorithm.hxx"
#include "OctiDefs.hxx"
#include "OctiPod.hxx"
#include "OctiSquare.hxx"
#include "OctiSquareImpl.hxx"
#include "OctiBoard.hxx"
#include "OctiGame.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class OCTIAPI Octi2PlayerGameSearchState;

/**
 * Move/Jump/Capture bitmap.
 */
class OCTIAPI MJCMap : public Root {

public:

    MJCMap();
    ~MJCMap();

    void reset();
    boolean tryMarkOD(int x, int y);
    boolean tryMarkJC(int x, int y);
    boolean tryUnmarkOD(int x, int y);
    boolean tryUnmarkJC(int x, int y);

private:

    static const word32 s_NEG_OD_COLUMN[];
    static const word32 s_MASK_OD_COLUMN[];
    static const word32 s_NEG_JC_COLUMN[];
    static const word32 s_MASK_JC_COLUMN[];

    int _rows[9];

};

/**
 * A data structure as a container for a list of states that form
 * can be used to produce composite-move states.
 *
 * head - The head of the list
 * tail - The tail of the list
 * pod - The pod that all the states in the list share.
 */
typedef struct _CompositeMove {
    Octi2PlayerGameSearchState *head;
    Octi2PlayerGameSearchState *tail;
    OctiPod pod;
} CompositeMove;

/**
 * A data structure for mobility calculation
 *
 */
typedef struct _PodMobilityEval {
    int numProngs;
    int reachableSquares;
    int capturableEnemyPods;
    int capturableEnemyProngs;
} PodMobilityEval;

/**
 * A data structure for specifying the grid location.
 */
typedef struct _Location {
    int x;
    int y;

    _Location &operator=(const _Location &loc);
    boolean operator==(const _Location &loc) const;
} Location;

/**
 * A data sturucture used to link directly into the list of squares
 * that contain material. It is faster using this list than checking
 * the 9 x 9 map and eliminating null-pointered entries.
 */
typedef struct _ValidSquare {
    Location loc;
    Ptr<Octi2PlayerSquare> square;

    _ValidSquare &operator=(const _ValidSquare &sq);
    boolean operator==(const _ValidSquare &sq) const;
} ValidSquare;

/**
 * A data structure specifying an AddPod op transition.
 */
typedef struct _AddPodTransition {
    Location loc;

    _AddPodTransition &operator=(const _AddPodTransition &trans);
    boolean operator==(const _AddPodTransition &trans) const;
} AddPodTransition;

/**
 * A data structure specifying an AddProng op transition.
 */
typedef struct _AddProngTransition {
    OctiPod pod;
    Location loc;
    OctiDir addDir;
  
    _AddProngTransition &operator=(const _AddProngTransition &trans);
    boolean operator==(const _AddProngTransition &trans) const;
} AddProngTransition;

/**
 * A data structure specifying a Reposition op transition.
 */
typedef struct _RepositionTransition {
    OctiPod pod;
    Location loc;
    OctiDir rmDir;
    OctiDir addDir;

    _RepositionTransition &operator=(const _RepositionTransition &trans);
    boolean operator==(const _RepositionTransition &trans) const;
} RepositionTransition;

/**
 * A data structure specifying a MovePod op transition.
 */
typedef struct _MovePodTransition {
    OctiPod pod;
    Location src;
    Location dest;
  
    _MovePodTransition &operator=(const _MovePodTransition &trans);
    boolean operator==(const _MovePodTransition &trans) const;
} MovePodTransition;

/**
 * A data structure specifying a Capture op transition.
 */
typedef struct _CaptureTransition {
    Location loc;
  
    _CaptureTransition &operator=(const _CaptureTransition &trans);
    boolean operator==(const _CaptureTransition &trans) const;
} CaptureTransition;

enum TransitionType {
    TRANS_UNDEF            = 0,
    TRANS_ADD_POD          = 1,
    TRANS_ADD_PRONG        = 2,
    TRANS_REPOSITION       = 3,
    TRANS_MOVE_POD         = 4,
    TRANS_CAPTURE          = 5
};

/**
 * A data structure for holding transition ops.
 */
class OCTIAPI TransitionOp : public PtrReference {
  
public:
  
    TransitionOp();
    TransitionOp(const TransitionOp &trans);
    TransitionOp(const AddPodTransition &addPodTrans);
    TransitionOp(const AddProngTransition &addProngTrans);
    TransitionOp(const RepositionTransition &repositionTrans);
    TransitionOp(const MovePodTransition &movePodTrans);
    TransitionOp(const CaptureTransition &captureTrans);
    ~TransitionOp();
  
    boolean operator==(const TransitionOp &trans) const;
    TransitionOp &operator=(const TransitionOp &trans);
  
    TransitionType getTransitionType() const;
    const AddPodTransition *getAddPodTransition() const;
    const AddProngTransition *getAddProngTransition() const;
    const RepositionTransition *getRepositionTransition() const;
    const MovePodTransition *getMovePodTransition() const;
    const CaptureTransition *getCaptureTransition() const;

private:
   
    TransitionType _type;
    union {
        AddPodTransition *addPodTrans;
        AddProngTransition *addProngTrans;
        RepositionTransition *repositionTrans;
        MovePodTransition *movePodTrans;
        CaptureTransition *captureTrans;
    } _u;

};

/**
**************************************************************************
* The search state for a 2 player Octi game.
**************************************************************************
*/
class OCTIAPI Octi2PlayerGameSearchState : public SearchState {

    DECLARE_RUNTIME_DISCOVERABLE(Octi2PlayerGameSearchState);

private:

    Octi2PlayerGameSearchState();
    Octi2PlayerGameSearchState(const Octi2PlayerGameSearchState *state);
    Octi2PlayerGameSearchState(Octi2PlayerGameSearchState *parent,
                               const char *moveDescription);
    Octi2PlayerGameSearchState(Octi2PlayerGameSearchState *parent,
                               Octi2PlayerGameSearchState *copy,
                               const char *moveDescription);
    Octi2PlayerGameSearchState(Octi2PlayerGameSearchState *parent,
                               Octi2PlayerGameSearchState *states[],
                               int statesCount,
                               const char *moveDescription) throw(Exception);

public:

    Octi2PlayerGameSearchState(OctiBoard *board);
    ~Octi2PlayerGameSearchState();

    // Override SearchState
    unsigned long hashCode() const;
    word32 hashChecksum() const;

    // Override SearchState
    const char *getDescription() const;
    float evaluate() const;
    boolean isTerminal() const;  
    void printState(ostream &out) const;

    boolean operator==(const BaseObject &object) const;
    boolean operator!=(const BaseObject &object) const;
    BaseObject *clone() const;


protected:

    // Implement SearchState
    void expandState();

private:

    // Move enumerations
    void enumerateAddPodPlys();
    void enumerateAddProngPlys(int x, int y, OctiPod pod);
    void enumerateMovePodPlys(int x, int y, OctiPod pod);
    void enumerateJumpPodPlys(int x, int y, OctiPod pod);
    void recursiveEnumerateJumpPodPlys(int srcX, int srcY,
                                       int x, int y,
                                       OctiPod pod,
                                       const char *prevJumpStr,
                                       MJCMap *map,
                                       Octi2PlayerGameSearchState *pS);
    void enumerateCompositePlys(int x, int y, CompositeMove *cm, int cmCount);

private:

    // Operators for changing internal state
    void appendTransitionOp(const TransitionOp &op);
    void addPodTransitionOp(int x, int y);
    void addProngTransitionOp(int x, int y, OctiPod pod, OctiDir dir);
    void repositionTransitionOp(int x, int y, OctiPod pod,
                                OctiDir addDir, OctiDir rmDir);
    void movePodTransitionOp(int x0, int y0, OctiPod pod, int x1, int y1);
    void captureTransitionOp(int x, int y);

    void syncState();

    void addPod(int x, int y);
    void addProng(int x, int y, OctiPod pod, OctiDir dir);
    void repositionProng(int x, int y, OctiPod pod,
                         OctiDir rmDir, OctiDir addDir);
    void movePod(int x0, int y0, OctiPod pod, int x1, int y1);
    void capture(int x, int y);

private:

    // State transition
    void copyInternalStatesFromParent();
    void appendValidSquare(int x, int y);
    void removeValidSquare(int x, int y);
    void appendAddPodState(Octi2PlayerGameSearchState *s);
    void appendAddProngState(Octi2PlayerGameSearchState *s);
    void appendMoveJumpCaptureState(Octi2PlayerGameSearchState *s);
    void appendCompositeState(Octi2PlayerGameSearchState *s);
    void orderListAndUpdateSuccessors();

private:

    // State evaluation
    static boolean isEnemySquareOf(int x, int y, OctiPlayer player);
    static float getDistance(int x0, int y0, int x1, int y1);
    void evaluateJumpCapture(int x, int y, OctiPodType type, OctiPod pod,
                             PodMobilityEval *mob, MJCMap *map) const;

private:

    static const char *s_STR_LOCS[9][9];
    static const char s_CHAR_DIRS[];
    static const char *s_STR_ADDPOD_MOVES[];

private:

    String _moveDescription;
    OctiPlayer _playerToMove;

    Ptr<Octi2PlayerSquare> _squares[9][9];
    Slist<ValidSquare> _validSquares;

    int _freePods[2];
    int _freeProngs[2];
    int _freeSuperProngs[2];
    int _capturedPods[2];

    boolean _evaled;
    float _eval;
    boolean _terminalTested;
    float _terminalValue;
  
    // Transition opeators
    Slist<TransitionOp> _trans;

    // Temporary variables for state enumeration
    Octi2PlayerGameSearchState *_headAddPodStates;
    Octi2PlayerGameSearchState *_tailAddPodStates;
    Octi2PlayerGameSearchState *_headAddProngStates;
    Octi2PlayerGameSearchState *_tailAddProngStates;
    Octi2PlayerGameSearchState *_headMoveJumpCaptureStates;
    Octi2PlayerGameSearchState *_tailMoveJumpCaptureStates;
    Octi2PlayerGameSearchState *_headCompositeStates;
    Octi2PlayerGameSearchState *_tailCompositeStates;

};

// Inline functions

inline MJCMap::MJCMap() {
    reset();
}

inline MJCMap::~MJCMap() {

}

inline void MJCMap::reset() {
    _rows[0] = 0x0;
    _rows[1] = 0x0;
    _rows[2] = 0x0;
    _rows[3] = 0x0;
    _rows[4] = 0x0;
    _rows[5] = 0x0;
    _rows[6] = 0x0;
    _rows[7] = 0x0;
    _rows[8] = 0x0;
}

inline boolean MJCMap::tryMarkOD(int x, int y) {
    if((_rows[x] & s_MASK_OD_COLUMN[y]) == 0x0) {
        _rows[x] |= s_MASK_OD_COLUMN[y];
        return TRUE;
    }
  
    return FALSE;
}

inline boolean MJCMap::tryMarkJC(int x, int y) {
    if((_rows[x] & s_MASK_JC_COLUMN[y]) == 0x0) {
        _rows[x] |= s_MASK_JC_COLUMN[y];
        return TRUE;
    }
  
    return FALSE;
}

inline boolean MJCMap::tryUnmarkOD(int x, int y) {
    _rows[x] &= s_NEG_OD_COLUMN[y];
    return TRUE;
}

inline boolean MJCMap::tryUnmarkJC(int x, int y) {
    _rows[x] &= s_NEG_JC_COLUMN[y];
    return TRUE;
}

inline _Location &_Location::operator=(const _Location &loc) {
    x = loc.x;
    y = loc.y;
    return *this;
}

inline boolean _Location::operator==(const _Location &loc) const {
    return x == loc.x && y == loc.y;
}

inline _ValidSquare &_ValidSquare::operator=(const _ValidSquare &sq) {
    loc = sq.loc;
    square = sq.square;
    return *this;
}

inline boolean _ValidSquare::operator==(const _ValidSquare &sq) const {
    return loc == sq.loc;
}

inline _AddPodTransition &_AddPodTransition::operator=(const _AddPodTransition &trans) {
    loc = trans.loc;
    return *this;
}

inline boolean _AddPodTransition::operator==(const _AddPodTransition &trans) const {
    return loc == trans.loc;
}

inline _AddProngTransition &_AddProngTransition::operator=(const _AddProngTransition &trans) {
    pod = trans.pod;
    loc = trans.loc;
    addDir = trans.addDir;
    return *this;
}

inline boolean _AddProngTransition::operator==(const _AddProngTransition &trans) const {
    return pod == trans.pod && loc == trans.loc && addDir == trans.addDir;
}

inline _RepositionTransition &_RepositionTransition::operator=(const _RepositionTransition &trans) {
    pod = trans.pod;
    loc = trans.loc;
    rmDir = trans.rmDir;
    addDir = trans.addDir;
    return *this;
}

inline boolean _RepositionTransition::operator==(const _RepositionTransition &trans) const {
    return pod == trans.pod && loc == trans.loc &&
        rmDir == trans.rmDir && addDir == trans.addDir;
}

inline _MovePodTransition &_MovePodTransition::operator=(const _MovePodTransition &trans) {
    pod = trans.pod;
    src = trans.src;
    dest = trans.dest;
    return *this;
}

inline boolean _MovePodTransition::operator==(const _MovePodTransition &trans) const {
    return pod == trans.pod && src == trans.src && dest == trans.dest;
}

inline _CaptureTransition &_CaptureTransition::operator=(const _CaptureTransition &trans) {
    loc = trans.loc;
    return *this;
}

inline boolean _CaptureTransition::operator==(const _CaptureTransition &trans) const {
    return loc == trans.loc;
}

inline TransitionOp::TransitionOp() {
    _type = TRANS_UNDEF;
    _u.addPodTrans = NULL;
    _u.addProngTrans = NULL;
    _u.repositionTrans = NULL;
    _u.movePodTrans = NULL;
    _u.captureTrans = NULL;
}

inline TransitionOp::TransitionOp(const AddPodTransition &addPodTrans) {
    _type = TRANS_ADD_POD;
    _u.addPodTrans = new AddPodTransition();
    *_u.addPodTrans = addPodTrans;
}

inline TransitionOp::TransitionOp(const AddProngTransition &addProngTrans) {
    _type = TRANS_ADD_PRONG;
    _u.addProngTrans = new AddProngTransition();
    *_u.addProngTrans = addProngTrans;
}

inline TransitionOp::TransitionOp(const RepositionTransition &repositionTrans) {
    _type = TRANS_REPOSITION;
    _u.repositionTrans = new RepositionTransition();
    *_u.repositionTrans = repositionTrans;
}

inline TransitionOp::TransitionOp(const MovePodTransition &movePodTrans) {
    _type = TRANS_MOVE_POD;
    _u.movePodTrans = new MovePodTransition();
    *_u.movePodTrans = movePodTrans;
}

inline TransitionOp::TransitionOp(const CaptureTransition &captureTrans) {
    _type = TRANS_CAPTURE;
    _u.captureTrans = new CaptureTransition();
    *_u.captureTrans = captureTrans;
}

inline TransitionOp::TransitionOp(const TransitionOp &trans) {
    _type = trans._type;
    _u.addPodTrans = NULL;
    _u.addProngTrans = NULL;
    _u.repositionTrans = NULL;
    _u.movePodTrans = NULL;
    _u.captureTrans = NULL;
}

inline TransitionOp::~TransitionOp() {
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
}

inline boolean TransitionOp::operator==(const TransitionOp &trans) const {
    if(_type != trans._type) {
        return FALSE;
    }

    switch(_type) {
    case TRANS_ADD_POD:
        return *_u.addPodTrans == *trans._u.addPodTrans;
    case TRANS_ADD_PRONG:
        return *_u.addProngTrans == *trans._u.addProngTrans;
    case TRANS_REPOSITION:
        return *_u.repositionTrans == *trans._u.repositionTrans;
    case TRANS_MOVE_POD:
        return *_u.movePodTrans == *trans._u.movePodTrans;
    case TRANS_CAPTURE:
        return *_u.captureTrans == *trans._u.captureTrans;
    default:
        return FALSE;
    }
}

inline TransitionType TransitionOp::getTransitionType() const {
    return _type;
}

inline const AddPodTransition *TransitionOp::getAddPodTransition() const {
    return _u.addPodTrans;
}

inline const AddProngTransition *TransitionOp::getAddProngTransition() const {
    return _u.addProngTrans;
}

inline const RepositionTransition *TransitionOp::getRepositionTransition() const {
    return _u.repositionTrans;
}

inline const MovePodTransition *TransitionOp::getMovePodTransition() const {
    return _u.movePodTrans;
}

inline const CaptureTransition *TransitionOp::getCaptureTransition() const {
    return _u.captureTrans;
}

inline Octi2PlayerGameSearchState::Octi2PlayerGameSearchState() {
  
}

inline Octi2PlayerGameSearchState::~Octi2PlayerGameSearchState() {

}

inline const char *Octi2PlayerGameSearchState::getDescription() const {
    return _moveDescription;
}

inline boolean Octi2PlayerGameSearchState::operator!=(const BaseObject &object) const {
    return !operator==(object);
}

__END_NAMESPACE(SELFSOFT);

#endif

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
