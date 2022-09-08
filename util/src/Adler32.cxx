/*************************************************************************
 * $Id: Adler32.cxx,v 1.5 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created:  8/28/99 by Charles Chiou
 * Comments: Adapted implementation of the adler32 algorithm from ZLIB.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

/* adler32.c -- compute the Adler-32 checksum of a data stream
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#define _UTILAPI_IMPL_

#include "debug.h"
#include "Adler32.hxx"

#define ADLER_BASE 65521L /* largest prime smaller than 65536 */
#define ADLER_NMAX 5552
/* ADLER_NMAX is the largest n such that:                     */
/* 255n(n+1)/2 + (n+1)(ADLER_BASE-1) <= 2^32-1                */

#define ADLER_DO1(buf,i)  { s1 += buf[i]; s2 += s1; }
#define ADLER_DO2(buf,i)  ADLER_DO1(buf, i); ADLER_DO1(buf, i+1);
#define ADLER_DO4(buf,i)  ADLER_DO2(buf, i); ADLER_DO2(buf, i+2);
#define ADLER_DO8(buf,i)  ADLER_DO4(buf, i); ADLER_DO4(buf, i+4);
#define ADLER_DO16(buf)   ADLER_DO8(buf, 0); ADLER_DO8(buf, 8);

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(Adler32, Checksum, 1);

void Adler32::update(byte b) {
    byte buf[1] = { b };
    _value = s_updateBytes(_value, buf, 1);
}

void Adler32::update(byte *b, int len) {
    ASSERT(b != NULL);
    _value = s_updateBytes(_value, b, len);
}

word32 Adler32::s_updateBytes(word32 adler, byte *buf, int len) {
    word32 s1 = adler & 0xffff;
    word32 s2 = (adler >> 16) & 0xffff;
    unsigned int k;
    unsigned int l = len;
  
    if(buf == 0) {
        return 0x1L;
    }
  
    while(l > 0) {
        k = l < ADLER_NMAX ? len : ADLER_NMAX;
        l -= k;
        while (k >= 16) {
            ADLER_DO16(buf);
            buf += 16;
            k -= 16;
        }
        if(k != 0) {
            do {
                s1 += *buf++;
                s2 += s1;
            } while(--k);
        }
    
        s1 %= ADLER_BASE;
        s2 %= ADLER_BASE;
    }

    return (s2 << 16) | s1;
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
