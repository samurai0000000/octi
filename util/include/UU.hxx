/*************************************************************************
 * $Id: UU.hxx,v 1.2 2000/11/14 06:15:03 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/14 06:15:03 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created:  3/29/00 by Charles Chiou
 * Comments: Berkeley uu character encoder and decoder.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _UU_HXX_
#define _UU_HXX_

#include "debug.h"
#include "Exception.hxx"
#include "ByteUtil.hxx"
#include "String.hxx"
#include "BufferedArray.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

/**
 * This exception is thrown when error is encountered in decoding uuencoded
 * stream.
 */
class UTILAPI UUEncodeFormatException : public Exception {

    DECLARE_RUNTIME_SERIALIZABLE(UUEncodeFormatException);

public:

    UUEncodeFormatException();
    UUEncodeFormatException(const char *msg);

};

/**
 *
 */
class UTILAPI UUEncoder : public ByteEncoder {

    DECLARE_RUNTIME_SERIALIZABLE(UUEncoder);

public:

    UUEncoder();
    ~UUEncoder();

    void update(byte b);
    void update(const byte *b, int len);
    void reset();

    void setUpperCase(const boolean toUpperCase);
    boolean isUpperCase() const;
    void setLineBreakAt(int lineBreak);
    int getLineBreakAt() const;

    const char *getEncoded();

private:

    boolean _upperCase;
    int _lineBreak;
    int _linePointer;
    String _encoded;

};


class UTILAPI UUDecoder : public ByteDecoder {

    DECLARE_RUNTIME_SERIALIZABLE(UUDecoder);

public:

    UUDecoder();
    ~UUDecoder();

    void update(char c) throw(UUEncodeFormatException);
    void update(const char *s, int len) throw(UUEncodeFormatException);
    void reset();
    const byte *getDecoded() throw(UUEncodeFormatException);
    int getDecodedLength() const;

private:

    char _hexChars[2];
    int _hexCharsIndex;
    BufferedArray<byte> _decoded;

};

// Inline functions

inline UUEncodeFormatException::UUEncodeFormatException() : Exception(){

}

inline UUEncodeFormatException::UUEncodeFormatException(const char *msg)
    : Exception(msg) {

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
