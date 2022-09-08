/*************************************************************************
 * $Id: Adler32.hxx,v 1.2 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created:  8/28/99 by Charles Chiou
 * Comments: The checksum algorithm used in ZLIB.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _ADLER32_HXX_
#define _ADLER32_HXX_

#include "Checksum.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI Adler32 : public Checksum {

    DECLARE_RUNTIME_SERIALIZABLE(Adler32);

public:

    void update(byte b);
    void update(byte *b, int len);
  
    void reset();
    word32 value() const;
    word32 length() const;

private:

    static word32 s_updateBytes(word32 adler, byte *buf, int len);

};

// Inline functions

inline void Adler32::reset() {
    _value = 0;
    _length = 0;
}

inline word32 Adler32::value() const {
    return _value;
}

inline word32 Adler32::length() const {
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
