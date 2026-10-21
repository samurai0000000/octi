/*************************************************************************
 * $Id: OctiSquareImpl.hxx,v 1.6 2001/08/24 03:00:33 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:00:33 $
 * $Locker:  $
 * $Revision: 1.6 $
 *
 * Created: 11/13/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#include "BaseObject.hxx"
#include "OctiPod.hxx"
#include "OctiSquare.hxx"

#ifndef _OCTISQUAREIMPL_HXX_
#define _OCTISQUAREIMPL_HXX_

__BEGIN_NAMESPACE(SELFSOFT);

class OctiSquareBaseImpl;

/**
 * Base implementation class of an OCTI square.
 */
class OCTIAPI OctiSquareBaseImpl : public OctiSquare, public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(OctiSquareBaseImpl);

protected:

    OctiSquareBaseImpl();
    OctiSquareBaseImpl(int x, int y);
    OctiSquareBaseImpl(OctiSquareType type, int x, int y);
    OctiSquareBaseImpl(const OctiSquareBaseImpl &square);
    ~OctiSquareBaseImpl();

public:

    void clear();

    void addPod(OctiPod pod);
    void removePod(OctiPod pod);
    boolean containsPod(OctiPod pod) const;

    int getNumPods() const;
    OctiPod *getPodAt(int index) const;
    boolean isEmpty() const;
    boolean isEmptyWithout(const Slist<OctiPod> &ghostPods) const;
    OctiSquareType getType() const;

    virtual boolean isCaptured() const = 0;
    virtual boolean isCapturedBy(OctiPlayer player) const = 0;
    virtual boolean hasEnemyOf(OctiPlayer player) const = 0;
    virtual boolean isEnemyOf(OctiPlayer player) const = 0;

    boolean hasSameMaterialsAs(const OctiSquare &sq) const;

    int getLocId() const;
    void getLocXY(int &x, int &y) const;
    int getLocX() const;
    int getLocY() const;

protected:

    OctiSquareType _type;
    int _x;
    int _y;
    int _numPods;
    OctiPod *_pods[7];

};


/**
 * Implementation class of a square used in OCTI game played under
 * 2-player mode.
 */
class OCTIAPI Octi2PlayerSquare : public OctiSquareBaseImpl {

    DECLARE_RUNTIME_SERIALIZABLE(Octi2PlayerSquare);

private:

    Octi2PlayerSquare();

public:

    Octi2PlayerSquare(int x, int y);
    Octi2PlayerSquare(OctiSquareType type, int x, int y);
    Octi2PlayerSquare(const Octi2PlayerSquare &square);
    ~Octi2PlayerSquare();

    boolean isCaptured() const;
    boolean isCapturedBy(OctiPlayer player) const;
    boolean hasEnemyOf(OctiPlayer player) const;
    boolean isEnemyOf(OctiPlayer player) const;

};


/**
 * Implementation class of a square used in OCTI game played under
 * 4-player mode.
 */
class OCTIAPI Octi4PlayerSquare : public OctiSquareBaseImpl {

    DECLARE_RUNTIME_SERIALIZABLE(Octi4PlayerSquare);

private:

    Octi4PlayerSquare();

public:

    Octi4PlayerSquare(int x, int y);
    Octi4PlayerSquare(OctiSquareType type, int x, int y);
    Octi4PlayerSquare(const Octi4PlayerSquare &square);
    ~Octi4PlayerSquare();

    boolean isCaptured() const;
    boolean isCapturedBy(OctiPlayer player) const;
    boolean hasEnemyOf(OctiPlayer player) const;
    boolean isEnemyOf(OctiPlayer player) const;

};

// Inline functions

inline OctiSquareBaseImpl::OctiSquareBaseImpl() {
    _type = NEUTRAL_SQUARE;
    _x = 0;
    _y = 0;
    _pods[0] = NULL;
    _pods[1] = NULL;
    _pods[2] = NULL;
    _pods[3] = NULL;
    _pods[4] = NULL;
    _pods[5] = NULL;
    _pods[6] = NULL;
    _numPods = 0;
}

inline OctiSquareBaseImpl::OctiSquareBaseImpl(int x, int y) {
    _type = NEUTRAL_SQUARE;
    _x = x;
    _y = y;
    _pods[0] = NULL;
    _pods[1] = NULL;
    _pods[2] = NULL;
    _pods[3] = NULL;
    _pods[4] = NULL;
    _pods[5] = NULL;
    _pods[6] = NULL;
    _numPods = 0;
}

inline OctiSquareBaseImpl::OctiSquareBaseImpl(OctiSquareType type, int x, int y) {
    _type = type;
    _x = x;
    _y = y;
    _pods[0] = NULL;
    _pods[1] = NULL;
    _pods[2] = NULL;
    _pods[3] = NULL;
    _pods[4] = NULL;
    _pods[5] = NULL;
    _pods[6] = NULL;
    _numPods = 0;
}

inline OctiSquareBaseImpl::OctiSquareBaseImpl(const OctiSquareBaseImpl &square) {
    int i;

    _type = square._type;
    _x = square._x;
    _y = square._y;
    _numPods = square._numPods;
    _pods[0] = NULL;
    _pods[1] = NULL;
    _pods[2] = NULL;
    _pods[3] = NULL;
    _pods[4] = NULL;
    _pods[5] = NULL;
    _pods[6] = NULL;
    for(i = 0; i < 7; i++) {
        if(square._pods[i] != NULL) {
            _pods[i] = new OctiPod(*square._pods[i]);
        }
    }
}

inline OctiSquareBaseImpl::~OctiSquareBaseImpl() {
    int i;

    for(i = 0; i < 7; i++) {
        if(_pods[i] != NULL) {
            delete _pods[i];
        }
    }
}

inline void OctiSquareBaseImpl::clear() {
    int i;

    for(i = 0; i < 7; i++) {
        delete _pods[i];
        _pods[i] = NULL;
    }

    _numPods = 0;
}

inline int OctiSquareBaseImpl::getNumPods() const {
    return _numPods;
}

inline OctiPod *OctiSquareBaseImpl::getPodAt(int index) const {
    return _pods[index];
}

inline boolean OctiSquareBaseImpl::isEmpty() const {
    return _numPods == 0;
}

inline OctiSquareType OctiSquareBaseImpl::getType() const {
    return _type;
}

inline int OctiSquareBaseImpl::getLocId() const {
    return _x * 9 + _y;
}

inline void OctiSquareBaseImpl::getLocXY(int &x, int &y) const {
    x = _x;
    y = _y;
}

inline int OctiSquareBaseImpl::getLocX() const {
    return _x;
}

inline int OctiSquareBaseImpl::getLocY() const {
    return _y;
}

inline Octi2PlayerSquare::Octi2PlayerSquare() : OctiSquareBaseImpl() {

}

inline Octi2PlayerSquare::Octi2PlayerSquare(int x, int y) : OctiSquareBaseImpl(x, y) {

}

inline Octi2PlayerSquare::Octi2PlayerSquare(OctiSquareType type, int x, int y) : OctiSquareBaseImpl(type, x, y) {
}

inline Octi2PlayerSquare::Octi2PlayerSquare(const Octi2PlayerSquare &square) : OctiSquareBaseImpl(square) {

}

inline Octi2PlayerSquare::~Octi2PlayerSquare() {

}

inline Octi4PlayerSquare::Octi4PlayerSquare() : OctiSquareBaseImpl() {

}

inline Octi4PlayerSquare::Octi4PlayerSquare(int x, int y) : OctiSquareBaseImpl(x, y) {

}

inline Octi4PlayerSquare::Octi4PlayerSquare(OctiSquareType type, int x, int y) : OctiSquareBaseImpl(type, x, y) {

}

inline Octi4PlayerSquare::Octi4PlayerSquare(const Octi4PlayerSquare &square) : OctiSquareBaseImpl(square) {

}

inline Octi4PlayerSquare::~Octi4PlayerSquare() {

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
