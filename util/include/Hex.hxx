/*************************************************************************
 * $Id: Hex.hxx,v 1.6 2000/04/27 05:37:02 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/04/27 05:37:02 $
 * $Locker:  $
 * $Revision: 1.6 $
 *
 * Created:  2/9/99 by Charles Chiou
 * Comments: Hex encoder and decoder to and from binary streams.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _HEX_HXX_
#define _HEX_HXX_

#include "debug.h"
#include "Exception.hxx"
#include "ByteUtil.hxx"
#include "String.hxx"
#include "BufferedArray.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

/**
 * This exception is thrown when error is encountered in decoding a hex-
 * encoded stream.
 */
class UTILAPI HexFormatException : public Exception {

  DECLARE_RUNTIME_SERIALIZABLE(HexFormatException);
  
public:
  
  HexFormatException();
  HexFormatException(const char *msg);

};

/**
 * This class encodes byte streams into hexadecimal format [012346789abcdef].
 * The numerals 'a' to 'f' are by default lower case and the encoder can be
 * set to generate upper case characters using setUpperCase(TRUE);
 *
 */
class UTILAPI HexEncoder : public ByteEncoder {

  DECLARE_RUNTIME_SERIALIZABLE(HexEncoder);
  
public:
  
  HexEncoder();
  ~HexEncoder();
  
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


class UTILAPI HexDecoder : public ByteDecoder {

  DECLARE_RUNTIME_SERIALIZABLE(HexDecoder);
  
public:

  HexDecoder();
  ~HexDecoder();

  void update(char c) throw(HexFormatException);
  void update(const char *s, int len) throw(HexFormatException);
  void reset();
  const byte *getDecoded() throw(HexFormatException);
  int getDecodedLength() const;

private:

  char _hexChars[2];
  int _hexCharsIndex;
  BufferedArray<byte> _decoded;

};

// Inline functions below

inline HexFormatException::HexFormatException() : Exception() {

}

inline HexFormatException::HexFormatException(const char *msg)
  : Exception(msg) {

}

inline HexEncoder::HexEncoder() {
  _upperCase = FALSE;
  _lineBreak = 0;
  reset();
}

inline HexEncoder::~HexEncoder() {

}

inline void HexEncoder::update(const byte *b, int len) {
  ASSERT(b != NULL);
  ASSERT(len >= 0);
  for(int i = 0; i < len; i++) {
    update(b[i]);
  }
}

inline void HexEncoder::setUpperCase(const boolean toUpperCase) {
  _upperCase = (toUpperCase == TRUE);
}

inline boolean HexEncoder::isUpperCase() const {
  return _upperCase == TRUE;
}

inline void HexEncoder::reset() {
  _linePointer = 0;
  _encoded = (char *) NULL;
}

inline void HexEncoder::setLineBreakAt(int lineBreak) {
  ASSERT(lineBreak > 0);
  _lineBreak = lineBreak;
}

inline int HexEncoder::getLineBreakAt() const {
  return _lineBreak;
}

inline const char *HexEncoder::getEncoded() {
  return (const char *) _encoded;
}

inline HexDecoder::HexDecoder() {
  reset();
}

inline HexDecoder::~HexDecoder() {

}

inline void HexDecoder::reset() {
  _hexCharsIndex = 0;
  _decoded.clear();
}

inline void HexDecoder::update(const char *s, int len)
  throw(HexFormatException) {
  ASSERT(s != NULL);
  ASSERT(len >= 0);
  for(int i = 0; i < len; i++) {
    update(s[i]);
  }
}

inline const byte *HexDecoder::getDecoded() throw(HexFormatException) {
  if(_hexCharsIndex != 0) {
    throw HexFormatException("Uneven number of hex string");
  }

  return _decoded.getData();
}

inline int HexDecoder::getDecodedLength() const {
  return _decoded.size();
}

__END_NAMESPACE(SELFSOFT);

#endif
