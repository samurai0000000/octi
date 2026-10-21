/*************************************************************************
 * $Id: OctiSquare.hxx,v 1.6 2001/08/24 03:00:33 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:00:33 $
 * $Locker:  $
 * $Revision: 1.6 $
 *
 * Created: 10/1/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _OCTISQUARE_HXX_
#define _OCTISQUARE_HXX_

#include "OctiPod.hxx"
#include "Slist.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

/**
 * Interface to the representation of an OCTI square.
 * It is the base class of Octi2PlayerSquare and Octi4PlayerSquare.
 *
 * This class is used to do more than a mere container class for pods.
 * In addition to being a container of pods inside a square, various
 * operators are available as utility methods for a game-logic controller
 * class.
 */
class OCTIAPI OctiSquare : public Root {

public:

    virtual void clear() = 0;
    virtual void addPod(OctiPod pod) = 0;
    virtual void removePod(OctiPod pod) = 0;
    virtual boolean containsPod(OctiPod pod) const = 0;

    virtual int getNumPods() const = 0;
    virtual OctiPod *getPodAt(int index) const = 0;
    virtual boolean isEmpty() const = 0;
    virtual boolean isEmptyWithout(const Slist<OctiPod> &ghostPods) const = 0;
    virtual OctiSquareType getType() const = 0;

    virtual boolean isCaptured() const = 0;
    virtual boolean isCapturedBy(OctiPlayer player) const = 0;
    virtual boolean hasEnemyOf(OctiPlayer player) const = 0;
    virtual boolean isEnemyOf(OctiPlayer player) const = 0;

    virtual boolean hasSameMaterialsAs(const OctiSquare &sq) const = 0;

    // Get the location of this square on the OCTI board
    virtual int getLocId() const = 0;
    virtual void getLocXY(int &x, int &y) const = 0;
    virtual int getLocX() const = 0;
    virtual int getLocY() const = 0;

};

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
