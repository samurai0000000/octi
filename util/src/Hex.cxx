/*************************************************************************
 * $Id: Hex.cxx,v 1.5 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created:  2/9/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include <string.h>
#include "Error.hxx"
#include "Hex.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(HexFormatException, Exception, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(HexEncoder, ByteEncoder, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(HexDecoder, ByteDecoder, 1);

#define EXPAND_BUFFER_INCRE_SIZE 64

static const char s_HEX_MAP[] = {
  '0', '1', '2', '3',
  '4', '5', '6', '7',
  '8', '9', 'a', 'b',
  'c', 'd', 'e', 'f'
};

void HexEncoder::update(byte b) {
  char c;
  c = s_HEX_MAP[b >> 4];

  _encoded += _upperCase ? toupper(c) : c;
  if(_lineBreak > 0 && ++_linePointer > _lineBreak) {
    _encoded += LINE_SEPARATOR;
    _linePointer = 0;
  }

  c = s_HEX_MAP[b & 0x0f];
  _encoded += _upperCase ? toupper(c) : c;
  if(_lineBreak > 0 && ++_linePointer > _lineBreak) {
    _encoded += LINE_SEPARATOR;
    _linePointer = 0;
  }
}

inline byte getHalfByte(char c) throw(HexFormatException) {
  int i = tolower(c);
  if(i >= '0' && i <= '9') {
    return (byte) (i - '0');
  } else if(i >= 'a' && i <= 'f') {
    return (byte) (i - 'a' + 10);
  } else {
    throw HexFormatException("Invalid hex encoding");
  }
}

void HexDecoder::update(char c) throw(HexFormatException) {
  if(isspace(c)) {
    return;  // Ignore white-spaces.
  }

  if(_hexCharsIndex == 0) {
    _hexChars[0] = c;
    _hexCharsIndex = 1;
  } else {
    _hexChars[1] = c;
    _hexCharsIndex = 0;

    byte b = (getHalfByte(_hexChars[0]) << 4) | getHalfByte(_hexChars[1]);
    _decoded.append(b);
  }
}

__END_NAMESPACE(SELFSOFT);
