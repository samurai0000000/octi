/*************************************************************************
 * $Id: Base64.hxx,v 1.6 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.6 $
 *
 * Created:
 * Comments:
 *     An utility class for the Base64 encoding scheme described in RFC 1521.
 *
 *     The method Base64Decode::getDecoded() may throw a Base64FormatException
 *     if the paddings at the end of the input was incorrect.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _BASE64_HXX_
#define _BASE64_HXX_

#include "debug.h"
#include "Exception.hxx"
#include "ByteUtil.hxx"
#include "BufferedArray.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

/**
 * This exception is thrown when error is encountered in decoding a stram
 * of Base64-encoded characters.
 */
class UTILAPI Base64FormatException : public Exception {

    DECLARE_RUNTIME_SERIALIZABLE(Base64FormatException);

public:

    Base64FormatException();
    Base64FormatException(const char *msg);

};

/**
 * This class encodes byte streams into Base64 encoded text.
 *
 * NOTE: In using getEncoded(), make sure that the output buffer allocated
 *       is sufficient to hold the text to be written to. If the parameter
 *       len is negative, then the function assums that the buffer is
 *       sufficiently large and will write to the buffer as much text as
 *       present in the encoded stream.
 */
class UTILAPI Base64Encoder : public ByteEncoder {

    DECLARE_RUNTIME_SERIALIZABLE(Base64Encoder);

public:

    Base64Encoder();
    ~Base64Encoder();

    void update(byte b);
    void update(const byte *b, int len);
    void reset();

    void setLineBreakAt(int lineBreak);
    int getLineBreakAt() const;

    const char *getEncoded();

private:

    int _lineBreak;
    int _linePointer;
    String _encoded;
    byte _buffer[3];
    int _bufferPointer;
    boolean _padded;

};

/**
 * This class decodes Base64-encoded streams back into a normal byte stream.
 */
class UTILAPI Base64Decoder : public ByteDecoder {

    DECLARE_RUNTIME_SERIALIZABLE(Base64Decoder);

public:

    Base64Decoder();
    ~Base64Decoder();

    void update(char c) throw(Base64FormatException);
    void update(const char *s, int len) throw(Base64FormatException);
    void reset();
    const byte *getDecoded() throw(Base64FormatException);
    int getDecodedLength() const;

private:

    char _base64Chars[4];
    int _base64CharsIndex;
    BufferedArray<byte> _decoded;
    boolean _padded;
    boolean _isFinal;

};

// Inline functions for Base64FormatException class are below.

inline void Base64Encoder::update(const byte *b, int len) {
    ASSERT(b != NULL);
    ASSERT(len >= 0);
    for(int i = 0; i < len; i++) {
        update(b[i]);
    }
}

inline Base64FormatException::Base64FormatException() : Exception() {
}

inline Base64FormatException::Base64FormatException(const char *msg)
    : Exception(msg) {
}

inline Base64Encoder::Base64Encoder() {
    _lineBreak = 0;
    reset();
}

inline Base64Encoder::~Base64Encoder() {

}

inline void Base64Encoder::reset() {
    memset(_buffer, 0, 3);
    _bufferPointer = -1;
    _linePointer = 0;
    _encoded = (char *) NULL;
    _padded = FALSE;
}

inline void Base64Encoder::setLineBreakAt(int lineBreak) {
    ASSERT(lineBreak > 0);
    _lineBreak = lineBreak;
}

inline int Base64Encoder::getLineBreakAt() const {
    return _lineBreak;
}

inline Base64Decoder::Base64Decoder() {
    reset();
}

inline Base64Decoder::~Base64Decoder() {

}

inline void Base64Decoder::reset() {
    _base64CharsIndex = 0;
    _decoded.clear();
    _padded = FALSE;
    _isFinal = FALSE;
}

inline void Base64Decoder::update(const char *s, int len)
    throw(Base64FormatException) {
    ASSERT(s != NULL);
    ASSERT(len >= 0);
    for(int i = 0; i < len; i++) {
        update(s[i]);
    }
}

inline int Base64Decoder::getDecodedLength() const {
    return _decoded.size();
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
