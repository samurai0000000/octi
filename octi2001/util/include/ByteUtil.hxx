/*************************************************************************
 * $Id: ByteUtil.hxx,v 1.9 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.9 $
 *
 * Created:  2/1/99 by Charles Chiou
 * Comments: This file contains utility functions that operates on bytes.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _BYTEUTIL_HXX_
#define _BYTEUTIL_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

#define GETBYTE(x, y) (((byte *) &(x)) [y])

/**************************************************************************/
/* The following functions reversed the byte order for a given data type. */
/**************************************************************************/

// word16:
UTILAPI word16 reverseEndian(word16 w16);
UTILAPI void reverseEndian(word16 *w16, int len);
// word32:
UTILAPI word32 reverseEndian(word32 w32);
UTILAPI void reverseEndian(word32 *w32, int len);
#ifdef __HAS_WORD64__
// word64:
UTILAPI word64 reverseEndian(word64 w64);
UTILAPI void reverseEndian(word64 *w64, int len);
#endif /* __HAS_WORD64__ */
// int16:
UTILAPI int16 reverseEndian(int16 i16);
UTILAPI void reverseEndian(int16 *w16, int len);
// int32:
UTILAPI int32 reverseEndian(int32 i32);
UTILAPI void reverseEndian(int32 *i32, int len);
#ifdef __HAS_WORD64__
// int64:
UTILAPI int64 reverseEndian(int64 i64);
UTILAPI void reverseEndian(int64 *i64, int len);
#endif /* __HAS_WORD64__ */

/* The following functions are operations that mutate a given data type. */
// Rotate to the left.
template <class T> T rotate_left(const T t, int b);
template <class T> void rotate_left(T *t, int len, int b);  // Use with caution
// Rotate to the right.
template <class T> T rotate_right(const T t, int b);
template <class T> void rotate_right(T *t, int len, int b); // Use with caution

/**************************************************************************/
/* Classes that operate on bytes.                                         */
/**************************************************************************/

/**
 * This is an interface class for all "byte stream to text encoding scheme"
 * implementation.
 */
class UTILAPI ByteEncoder : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(ByteEncoder);

public:

    virtual ~ByteEncoder() {};

    virtual void update(byte b) = 0;
    virtual void update(const byte *b, int len) = 0;
    virtual void reset() = 0;
    virtual const char *getEncoded() = 0;

};

/**
 * This is an interface class for all "text encoding scheme to byte stream"
 * implementations.
 */
class UTILAPI ByteDecoder : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(ByteDecoder);

public:

    virtual ~ByteDecoder() {};

    virtual void update(char c) = 0;
    virtual void update(const char *s, int len) = 0;
    virtual void reset() = 0;
    virtual const byte *getDecoded() = 0;
    virtual int getDecodedLength() const = 0;

};


// The inline functions are below here.

inline word16 reverseEndian(word16 w16) {
    return w16 << 8 | w16 >> 8;
}

inline void reverseEndian(word16 *w16, int len) {
    int x;
    for(x = 0; x < len; x++) {
        w16[x] = reverseEndian(w16[x]);
    }
}

inline word32 reverseEndian(word32 w32) {
    return
        w32 >> 24               |
        (w32 >> 8) & 0x0000ff00 |
        (w32 << 8) & 0x00ff0000 |
        w32 << 24;
}

inline void reverseEndian(word32 *w32, int len) {
    int x;
    for(x = 0; x < len; x++) {
        w32[x] = reverseEndian(w32[x]);
    }
}

#ifdef __HAS_WORD64__

inline word64 reverseEndian(word64 w64) {
    return
        w64  >> 56                                  |
        (w64 >> 40) & WORD64STR(0x000000000000ff00) |
        (w64 >> 24) & WORD64STR(0x0000000000ff0000) |
        (w64 >>  8) & WORD64STR(0x00000000ff000000) |
        (w64 <<  8) & WORD64STR(0x000000ff00000000) |
        (w64 << 24) & WORD64STR(0x0000ff0000000000) |
        (w64 << 40) & WORD64STR(0x00ff000000000000) |
        w64  << 56;
}

inline void reverseEndian(word64 *w64, int len) {
    int x;
    for(x = 0; x < len; x++) {
        w64[x] = reverseEndian(w64[x]);
    }
}

#endif  /* __HAS_WORD64__ */

inline int16 reverseEndian(int16 i16) {
    return reverseEndian((word16) i16);
}

inline void reverseEndian(int16 *i16, int len) {
    int x;
    for(x = 0; x < len; x++) {
        i16[x] = reverseEndian(i16[x]);
    }
}

inline int32 reverseEndian(int32 i32) {
    return reverseEndian((word32) i32);
}

inline void reverseEndian(int32 *i32, int len) {
    int x;
    for(x = 0; x < len; x++) {
        i32[x] = reverseEndian(i32[x]);
    }
}

#ifdef __HAS_WORD64__

inline int64 reverseEndian(int64 i64) {
    return reverseEndian(i64);
}

inline void reverseEndian(int64 *i64, int len) {
    int x;
    for(x = 0; x < len; x++) {
        i64[x] = reverseEndian(i64[x]);
    }
}

#endif  /* __HAS_WORD64__ */

template <class T> inline T rotate_left(const T t, int b) {
    return ((t << b) | (t >> (sizeof(T) * 8 - b)));
}

template <class T> inline void rotate_left(T *t, int len, int b) {
    T *buf = new T[len];
    memcpy(buf, &t[b],  len - b);
    memcpy(&buf[len - b], t, b);
    memcpy(t, buf, len);
    delete [] buf;
}

template <class T> inline T rotate_right(const T t, int b) {
    return ((t >> b) | (t << (sizeof(T) * 8 - b)));
}

template <class T> inline void rotate_right(T *t, int len, int b) {
    T *buf = new T[len];
    memcpy(&buf[b], t,  len - b);
    memcpy(buf, &t[len - b], b);
    memcpy(t, buf, len);
    delete [] buf;
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
