/*************************************************************************
 * $Id: OctiPod.hxx,v 1.6 2001/08/24 03:00:33 samurai Exp $
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

#ifndef _OCTIPOD_HXX_
#define _OCTIPOD_HXX_

#include "BaseObject.hxx"
#include "String.hxx"
#include "OctiDefs.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

/**
 * OctiPod is a class for representing a pod in an OCTI game.
 *
 */
class OCTIAPI OctiPod : public BaseObject {

    DECLARE_RUNTIME_SERIALIZABLE(OctiPod);

public:

    OctiPod();
    OctiPod(const OctiPod &pod);
    OctiPod(OctiPodType type);
    ~OctiPod();

    virtual void setType(OctiPodType type);
    OctiPodType getType() const;

    virtual void addProng(OctiDir dir, boolean superProng);
    virtual void removeProng(OctiDir dir);

    boolean hasSameConfig(const OctiPod &pod) const;
    OctiProngType prongAt(OctiDir dir) const;

    // NOTE: Allocate buf to hold at least 17 characters
    void getProngConfigString(char *buf) const;
    String getProngConfigString() const;

    virtual boolean operator==(const OctiPod &pod) const;
    virtual OctiPod &operator=(const OctiPod &pod);

    // The weight comparison methods are based on how many
    // prongs each pod has.
    boolean lighterThan(const OctiPod &pod) const;
    boolean heavierThan(const OctiPod &pod) const;

    unsigned long hashCode() const;

public:

    static const OctiPod EMPTY_POD;

private:

    /**
     * _config is a special variable for setting prong direction and
     * normal/super states.
     * For fast comparison operations the bit layouts are:
     * xy0000000000000001  - normal prong in N  or 'a'
     * xy0000000000000010  -  super prong in N  or 'a!'
     * xy0000000000000100  - normal prong in NE or 'b'
     * xy0000000000001000  -  super prong in NE or 'b!'
     * xy0000000000010000  - normal prong in E  or 'c'
     * xy0000000000100000  -  super prong in E  or 'c!'
     * xy0000000001000000  - normal prong in SE or 'd'
     * xy0000000010000000  -  super prong in SE or 'd!'
     * xy0000000100000000  - normal prong in S  or 'e'
     * xy0000001000000000  -  super prong in S  or 'e!'
     * xy0000010000000000  - normal prong in SW or 'f'
     * xy0000100000000000  -  super prong in SW or 'f!'
     * xy0001000000000000  - normal prong in W  or 'g'
     * xy0010000000000000  -  super prong in W  or 'g!'
     * xy0100000000000000  - normal prong in NW or 'h'
     * xy1000000000000000  - normal prong in NW or 'h!'
     *
     * for the MSB bits x, and y:
     *  xy = 00  - player A
     *  xy = 01  - player B
     *  xy = 10  - player C
     *  xy = 11  - player D
     *
     * Unfortunately, to speed up the comparison operator means that we can
     * not fit everything in a 16-bit variable.
     */
    word32 _config;

private:

};

// Inline functions

inline OctiPod::OctiPod() {
    _config = 0x0;
}

inline OctiPod::OctiPod(const OctiPod &pod) {
    _config = pod._config;
}

inline OctiPod::OctiPod(OctiPodType type) {
    _config = (((type - 1) & 0x3) << 16);
}

inline OctiPod::~OctiPod() {

}

inline void OctiPod::setType(OctiPodType type) {
    _config &= 0xffff;
    _config |= (((type - 1) & 0x3) << 16);
}

inline OctiPodType OctiPod::getType() const {
    return (OctiPodType) ((_config >> 16) + 1);
}

inline boolean OctiPod::hasSameConfig(const OctiPod &pod) const {
    return (_config & 0xffff) == (pod._config & 0xffff);
}

inline OctiProngType OctiPod::prongAt(OctiDir dir) const {
    int shift;

    shift = (dir << 1);  // dir * 2
    return (OctiProngType) ((_config & ((0x3) << shift)) >> shift);
}

inline boolean OctiPod::operator==(const OctiPod &pod) const {
    return _config == pod._config;
}

inline OctiPod &OctiPod::operator=(const OctiPod &pod) {
    _config = pod._config;
    return *this;
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
