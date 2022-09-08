/*************************************************************************
 * $Id: CRC32.hxx,v 1.2 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created:  1/24/99 by Charles Chiou
 * Comments: The CRC32 class calculates checksum specified by the POSIX 1003.2
 *           Refer to Checksum.hxx for how to operate on this object.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _CRC32_HXX_
#define _CRC32_HXX_

#include "Checksum.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI CRC32 : public Checksum {

    DECLARE_RUNTIME_SERIALIZABLE(CRC32);

public:

    void update(byte b);
    void update(byte *b, int len);
  
    void reset();
    word32 value() const;
    word32 length() const;

private:

    static word32 _s_CRCTable[];

};

// Inline functions

void CRC32::reset() {
    _value = 0;
    _length = 0;
}

inline word32 CRC32::length() const {
    return _length;
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
