/*************************************************************************
 * $Id: Checksum.hxx,v 1.3 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created:  1/24/99 by Charles Chiou
 * Comments:
 *     This is an interface class for a data cheksum.
 *
 *     update(byte) takes a byte as the parameter.
 *     update(byte *, int) takes an
 *     array of bytes and length as the parameters.
 *
 *     reset() resets the state of the Checksum object.
 *     value returns the computed checksum so far.
 *
 *     Note: the type 'byte' should b an 'octect', meaning that only the
 *     lower 8 bits are used to calculate the checksum, and 'word32' should
 *     be a 32-bit number.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _CHECKSUM_HXX
#define _CHECKSUM_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI Checksum : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(Checksum);

public:

    Checksum();

    virtual void update(byte b) = 0;
    virtual void update(byte *b, int len) = 0;

    virtual void reset();
    virtual word32 value() const = 0;
    virtual word32 length() const = 0;

protected:

    word32 _value;
    word32 _length;

};

// Inline functions

inline Checksum::Checksum() {
    _value = 0;
    _length = 0;
}

inline void Checksum::reset() {
    _value = 0;
    _length = 0;
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
