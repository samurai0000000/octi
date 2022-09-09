/*************************************************************************
 * $Id: OctiSquare.cxx,v 1.9 2001/08/24 03:00:35 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:00:35 $
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

#define _OCTIAPI_IMPL_

#include "OctiDefs.hxx"
#include "OctiSquare.hxx"
#include "OctiSquareImpl.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_DISCOVERABLE(OctiSquareBaseImpl, BaseObject);
IMPLEMENT_RUNTIME_SERIALIZABLE(Octi2PlayerSquare, OctiSquareBaseImpl, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(Octi4PlayerSquare, OctiSquareBaseImpl, 1);

void OctiSquareBaseImpl::addPod(OctiPod pod) {
    if(_numPods < 7) {
        _pods[_numPods] = new OctiPod(pod);
        _numPods++;
    }
}

void OctiSquareBaseImpl::removePod(OctiPod pod) {
    int i;
    for(i = 0; i < 7 && _pods[i] != NULL; i++) {
        if(_pods[i]->hasSameConfig(pod)) {
            delete _pods[i];
            _numPods--;
            for(; i < 7 - 1; i++) {
                _pods[i] = _pods[i + 1];
            }
            _pods[6] = NULL;
            return;
        }
    }
}

boolean OctiSquareBaseImpl::containsPod(OctiPod pod) const {
    int i;

    for(i = 0; i < _numPods; i++) {
        if(_pods[i]->hasSameConfig(pod)) {
            return TRUE;
        }
    }

    return FALSE;
}

boolean OctiSquareBaseImpl::isEmptyWithout(const Slist<OctiPod> &ghostPods) const {
    int size;

    Ptr<SlistIterator<OctiPod> > iterator =
        (SlistIterator<OctiPod> *) ghostPods.getIterator();
    for(iterator->first(); iterator->isNotNull(); iterator->next()) {
        if(containsPod(*iterator->current())) {
            size--;
        }
    }

    return size == 0;
}

boolean OctiSquareBaseImpl::hasSameMaterialsAs(const OctiSquare &sq) const {
    const OctiSquareBaseImpl *sqb = (const OctiSquareBaseImpl *) &sq;

    if(_numPods != sqb->_numPods) {
        return FALSE;
    }

    int i, j, match;
    OctiPod tmp[7];

    for(i = 0; i < _numPods; i++) {
        tmp[i] = *sqb->_pods[i];
    }

    match = 0;
    for(i = 0; i < _numPods; i++) {
        for(j = 0; j < _numPods; j++) {
            if(*_pods[i] == tmp[j]) {
                tmp[j] = OctiPod::EMPTY_POD;
                match++;
                break;
            }
        }
    }

    return match == _numPods;
}

boolean Octi2PlayerSquare::isCapturedBy(OctiPlayer player) const {
    if(_numPods == 0 || _type == NEUTRAL_SQUARE) {
        return FALSE;
    }

    return ((int) _pods[0]->getType() == (int) player);
}

boolean Octi2PlayerSquare::isCaptured() const {
    if(_numPods == 0 || _type == NEUTRAL_SQUARE) {
        return FALSE;
    }

    return ((int) _pods[0]->getType()) != ((int) _type);
}

boolean Octi2PlayerSquare::hasEnemyOf(OctiPlayer player) const {
    OctiPodType podType;

    if(_numPods > 0) {
        podType =  _pods[0]->getType();
        switch(player) {
        case PLAYER_A:
            return podType == B_POD;
        case PLAYER_B:
            return podType == A_POD;
        }
    }

    return FALSE;
}

boolean Octi2PlayerSquare::isEnemyOf(OctiPlayer player) const {
    if(_type != NEUTRAL_SQUARE) {
        switch(player) {
        case PLAYER_A:
            return _type == B_HOME_SQUARE;
        case PLAYER_B:
            return _type == A_HOME_SQUARE;
        }
    }

    return FALSE;
}

boolean Octi4PlayerSquare::isCapturedBy(OctiPlayer player) const {
    if(_numPods == 0 || _type == NEUTRAL_SQUARE) {
        return FALSE;
    }

    return ((int) _pods[0]->getType() == (int) player);
}

boolean Octi4PlayerSquare::isCaptured() const {
    if(_numPods > 0 && _type != NEUTRAL_SQUARE) {
        switch(_pods[0]->getType()) {
        case A_POD:
        case C_POD:
            return ((int) _type != (int) B_POD) && ((int) _type != (int) D_POD);
        case B_POD:
        case D_POD:
            return ((int) _type != (int) A_POD) && ((int) _type != (int) C_POD);
        }
    }

    return FALSE;
}

boolean Octi4PlayerSquare::hasEnemyOf(OctiPlayer player) const {
    OctiPodType podType;

    if(_numPods > 0) {
        podType = _pods[0]->getType();
        switch(player) {
        case PLAYER_A:
            return podType == B_POD || podType == D_POD;
        case PLAYER_B:
            return podType == A_POD || podType == C_POD;
        case PLAYER_C:
            return podType == B_POD || podType == D_POD;
        case PLAYER_D:
            return podType == A_POD || podType == C_POD;
        }
    }

    return FALSE;
}

boolean Octi4PlayerSquare::isEnemyOf(OctiPlayer player) const {
    if(_type != NEUTRAL_SQUARE) {
        switch(player) {
        case PLAYER_A:
        case PLAYER_C:
            return _type == B_HOME_SQUARE || _type == D_HOME_SQUARE;
        case PLAYER_B:
        case PLAYER_D:
            return _type == A_HOME_SQUARE || _type == C_HOME_SQUARE;
        }
    }

    return FALSE;
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
