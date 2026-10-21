/*************************************************************************
 * $Id: OctiMove.hxx,v 1.7 2001/08/24 03:00:33 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:00:33 $
 * $Locker:  $
 * $Revision: 1.7 $
 *
 * Created: 10/1/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _OCTIMOVE_HXX_
#define _OCTIMOVE_HXX_

#include "BaseObject.hxx"
#include "String.hxx"
#include "Slist.hxx"
#include "Dlist.hxx"
#include "GameExceptions.hxx"
#include "OctiPod.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

/**
 * OctiMoveOp is a class for storing an independent opeator in a 'chain'
 * of a singular move.
 *
 * NOTE: A 'move' by a player in OCTI may contain multiple operations
 *       due to the fact that stacked pods may jump/move/capture independently
 *       of each other. This class encapsulates the individual operations in
 *       a move. i.e. A move may hold one or more OctiMoveOp objects.
 */
class OCTIAPI OctiMoveOp : public BaseObject {

    DECLARE_RUNTIME_SERIALIZABLE(OctiMoveOp);

public:

    int x0, y0;                // Original coordinate for pod
    OctiPod pod;               // Pod and its configuration
    int x1, y1;                // Destination coordinate
    boolean capture;           // Capture during move
    OctiDir prongAddDir;       // Addition of prong during move
    OctiDir prongRemoveDir;    // Subtraction of prong during move
    boolean addSuperProng;     // Denotes adding a super prong
    boolean removeSuperProng;  // Denotes subtracting a super prong

    /* This part is filled later by the game control. */
    /* Mainly for undo/redo purposes. */
    Slist<OctiPod> capturedPods;
    OctiPod liberatedPod;
    OctiPlayer liberatedFromPlayer;

public:

    OctiMoveOp();
    OctiMoveOp(const OctiMoveOp &op);
    ~OctiMoveOp();

    boolean operator==(const OctiMoveOp &op) const;
    OctiMoveOp &operator=(const OctiMoveOp &op);

private:

    friend class OctiMove;

    void reset();

};


/**
 * OctiMove is a representation of a 'move' operation in an OCTI game.
 * Equivalently, 'move' is a 'ply' in AI literatures.
 *
 * There are many sources that OctiMove objects may be constructed:
 *   - by copy from another OctiMove object
 *   - from a list of OctiMoveOp operator objects
 *   - raw string, which will be parsed and converted into appropriate
 *     internal data structure.
 *
 * NOTE: The parser is implemented using a FSM (finite-state-machine).
 *       It adheres strictly to the official & standard OCTI notation
 *       published by Professor Don Green.
 *       However, some (but not comprehensive) semantic checking is enforced
 *       in the parser. For example, it is illegal to have different source
 *       squares in a stacked move:
 *             "33a-34,33b-44"    is accepted by the parser
 *             "33a-34,34a-35"    is rejected by the parser
 */
class OCTIAPI OctiMove : public BaseObject {

    DECLARE_RUNTIME_SERIALIZABLE(OctiMove);

public:

    OctiMove();
    OctiMove(const OctiMove &move);
    OctiMove(const Dlist<Ptr<OctiMoveOp> > &ops) throw(IllegalMoveSyntaxException);
    OctiMove(const char *s) throw(IllegalMoveSyntaxException);
    ~OctiMove();

    OctiMove &operator=(const OctiMove &move);

    Dlist<Ptr<OctiMoveOp> > getOps();
    int getSourceX() const;
    int getSourceY() const;
    const char *getNotation() const;

private:

    void nextState();
    int peekMove();

private:

    enum FSMState {
        S_START   =  0,
        S_READX0  =  1,
        S_READY0  =  2,
        S_ADDRMV1 =  3,
        S_RHALF   =  4,
        S_ADDRMV2 =  5,
        S_RCOMP   =  6,
        S_MOVE    =  7,
        S_READX1  =  8,
        S_READY1  =  9,
        S_FINISH  = 10,
        S_ERROR   = 11,
    };

    enum CharType {
        T_NUM = 0,  // Numeric coordinate
        T_PRO = 1,  // Prong designation
        T_ARP = 2,  // Add/remove prong operations
        T_MOV = 3,  // Move pod operation
        T_SUP = 4,  // Super prong designation
        T_CAP = 5,  // Capture operation
        T_COM = 6,  // Comma, multiple moves
        T_WHI = 7,  // White spaces
        T_TER = 8,  // Terminate
        T_ERR = 9   // Error
    };

private:

    String _notation;

    Dlist<Ptr<OctiMoveOp> > _ops;

    FSMState _state;
    int _x;
    int _y;
    const char *_proc_s;
    int _proc_index;
    int _proc_s_length;
    Ptr<OctiMoveOp>  _current;
    boolean _lastAddRemoveIsAdd;

};

// Inline functions

inline OctiMoveOp::OctiMoveOp() {
    reset();
}

inline OctiMoveOp::OctiMoveOp(const OctiMoveOp &op) {
    operator=(op);
}

inline OctiMoveOp::~OctiMoveOp() {

}

inline void OctiMoveOp::reset() {
    x0 = -1;
    y0 = -1;
    x1 = -1;
    y1 = -1;
    pod = OctiPod::EMPTY_POD;
    capture = FALSE;
    prongAddDir = DIR_UNDEF;
    prongRemoveDir = DIR_UNDEF;
    addSuperProng = FALSE;
    removeSuperProng = FALSE;
    capturedPods.clear();
    liberatedPod = OctiPod::EMPTY_POD;
    liberatedFromPlayer = PLAYER_UNDEF;
}

inline boolean OctiMoveOp::operator==(const OctiMoveOp &op) const {
    return
        (x0 == op.x0) &&
        (y0 == op.y0) &&
        (x1 == op.x1) &&
        (y1 == op.y1) &&
        (capture == op.capture) &&
        (prongAddDir == op.prongAddDir) &&
        (prongRemoveDir == op.prongRemoveDir) &&
        (addSuperProng == op.addSuperProng) &&
        (removeSuperProng == op.removeSuperProng) &&
        (capturedPods == capturedPods) &&
        (liberatedFromPlayer == op.liberatedFromPlayer);
}

inline OctiMoveOp &OctiMoveOp::operator=(const OctiMoveOp &op) {
    x0 = op.x0;
    y0 = op.y0;
    pod = op.pod;
    x1 = op.x1;
    y1 = op.y1;
    capture = op.capture;
    prongAddDir = op.prongAddDir;
    prongRemoveDir = op.prongRemoveDir;
    addSuperProng = op.addSuperProng;
    removeSuperProng = op.removeSuperProng;
    capturedPods = op.capturedPods;
    liberatedFromPlayer = op.liberatedFromPlayer;

    return *this;
}

inline OctiMove::OctiMove() {

}

inline OctiMove::OctiMove(const OctiMove &move) {
    operator=(move);
}

inline OctiMove::~OctiMove() {

}

inline OctiMove &OctiMove::operator=(const OctiMove &move) {
    _notation = move._notation;
    _ops.clear();

    _x = move._x;
    _y = move._y;
    Ptr<DlistIterator<Ptr<OctiMoveOp> > > it = (DlistIterator<Ptr<OctiMoveOp> > *) move._ops.getIterator();
    for(it->first(); it->isNotNull(); it->next()) {
        _current = new OctiMoveOp(*it->current()->ptr());
        _ops.append(_current);
    }
    _current = NULL;

    return *this;
}

inline Dlist<Ptr<OctiMoveOp> > OctiMove::getOps() {
    return _ops;
}

inline int OctiMove::getSourceX() const {
    return _x;
}

inline int OctiMove::getSourceY() const {
    return _y;
}

inline const char *OctiMove::getNotation() const {
    return _notation;
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
