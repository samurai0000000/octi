/*************************************************************************
 * $Id: Base64.cxx,v 1.5 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created:  1/25/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include "Error.hxx"
#include "Base64.hxx"

#define EXPAND_BUFFER_INCRE_SIZE 64

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(Base64FormatException, Exception, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(Base64Encoder, ByteEncoder, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(Base64Decoder, ByteDecoder, 1);

static const char s_BASE64_MAP[] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',   
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/'
};

void Base64Encoder::update(byte b) {
  _buffer[++_bufferPointer] = b;

  if(_padded) {
    // If padded, then remove the last four characters from the encoding
    // before proceeding.
    _encoded.remove(_encoded.length() - 4, _encoded.length() - 1);
    _padded = FALSE;
  }

  if(_bufferPointer == 2) {
    _encoded += s_BASE64_MAP[(_buffer[0] & 0xfc) >> 2];
    if(_lineBreak > 0 && ++_linePointer > _lineBreak) {
      _encoded += LINE_SEPARATOR;
      _linePointer = 0;
    }
    _encoded += s_BASE64_MAP[(_buffer[0] & 0x03) << 4 |
			    (_buffer[1] & 0xf0) >> 4];
    if(_lineBreak > 0 && ++_linePointer > _lineBreak) {
      _encoded += LINE_SEPARATOR;
      _linePointer = 0;
    }
    _encoded += s_BASE64_MAP[(_buffer[1] & 0x0f) << 2 |
			    (_buffer[2] & 0xc0) >> 6];
    if(_lineBreak > 0 && ++_linePointer > _lineBreak) {
      _encoded += LINE_SEPARATOR;
      _linePointer = 0;
    }
    _encoded += s_BASE64_MAP[(_buffer[2] & 0x3f)];
    if(_lineBreak > 0 && ++_linePointer > _lineBreak) {
      _encoded += LINE_SEPARATOR;
      _linePointer = 0;
    }
    
    _bufferPointer = -1;
  }
}

const char *Base64Encoder::getEncoded() {
  char pad[5] = {'=', '=', '=', '=', '\0'};

  if(!_padded) {
    if(_bufferPointer == 0) {
      pad[0] = s_BASE64_MAP[(_buffer[0] & 0xfc) >> 2];
      pad[1] = s_BASE64_MAP[(_buffer[0] & 0x03) << 4];
    } else if(_bufferPointer == 1) {
      pad[0] = s_BASE64_MAP[(_buffer[0] & 0xfc) >> 2];
      pad[1] = s_BASE64_MAP[(_buffer[0] & 0x03) << 4 |
			   (_buffer[1] & 0xf0) >> 4];
      pad[2] = s_BASE64_MAP[(_buffer[1] & 0x0f) << 2];
    }
    
    _encoded += pad;
    _padded = TRUE;
  }

  return _encoded;
}

/**
 * Converts a Base64 digit into a 6-bit value.
 *
 * @param c The character digit.
 * @return The decoded 6-bit value.
 * @throw NumberFormatException If c is not a valid Base64 digit.
 */
inline byte conv(int c) throw(Base64FormatException) {
  if ((c >= 'A') && (c <= 'Z')) {
    return (byte) (c - 'A');
  } else if ((c >= 'a') && (c <= 'z')) {
    return (byte) (c - 'a' + 0x1a);
  } else if ((c >= '0') && (c <= '9')) {
    return (byte) (c - '0' + 0x34);
  } else {
    switch(c) {
    case '+':
      return (byte) 0x3e;
    case '/':
      return (byte) 0x3f;
    case '=':
      return (byte) 0x0;
    default:
      throw Base64FormatException("Invalid base64 encoding");
    }  
  }
}


void Base64Decoder::update(char c) throw(Base64FormatException) {
  if(isspace(c)) {
    return;  // Ignore white spaces.
  }

  if(_isFinal) {
    throw Base64FormatException("New decodings must be reset before update");
  } else if(_padded) {
    if(c != '=' || _base64CharsIndex == 3) {
      throw Base64FormatException("Illegal padding");
    }
    _base64Chars[_base64CharsIndex] = c;
    _base64CharsIndex++;
  } else if(c == '=') {
    if(_base64CharsIndex > 3) {
      throw Base64FormatException("Illegal padding");
    }
    _padded = TRUE;
    _base64Chars[_base64CharsIndex] = c;
    _base64CharsIndex++;
  } else if(_base64CharsIndex < 3) {
    _base64Chars[_base64CharsIndex] = c;
    _base64CharsIndex++;
  } else {
    _base64Chars[3] = c;
    _base64CharsIndex = 0;

    byte b1 = conv(_base64Chars[0]);
    byte b2 = conv(_base64Chars[1]);
    byte b3 = conv(_base64Chars[2]);
    byte b4 = conv(_base64Chars[3]);

    byte x1 = (byte) ((b1 & 0x3f) << 2 | rotate_right(b2 & 0x30, 4));
    byte x2 = (byte) ((b2 & 0x0f) << 4 | rotate_right(b3 & 0x3c, 2));
    byte x3 = (byte) ((b3 & 0x03) << 6 | (b4 & 0x3f));

    _decoded.append(x1);
    _decoded.append(x2);
    _decoded.append(x3);
  }
}

const byte *Base64Decoder::getDecoded() throw(Base64FormatException) {
  if(!_isFinal) {  // If the final decoding has not been computed
    if(!_padded || _base64CharsIndex != 4) {
      throw Base64FormatException("Illegal padding");
    }
    
    int padding = 0;
    for(int p = 3; p >= 0 && _base64Chars[p] == '='; p--) {
      padding++;
    }
    
    if(padding > 2) {
      throw Base64FormatException("Illegal padding");
    }

    byte b1 = conv(_base64Chars[0]);
    byte b2 = conv(_base64Chars[1]);
    byte b3 = conv(_base64Chars[2]);
    byte b4 = conv(_base64Chars[3]);

    byte x1 = (byte) ((b1 & 0x3f) << 2 | rotate_right(b2 & 0x30, 4));
    byte x2 = (byte) ((b2 & 0x0f) << 4 | rotate_right(b3 & 0x3c, 2));
    byte x3 = (byte) ((b3 & 0x03) << 6 | (b4 & 0x3f));

    _decoded.append(x1);
    _decoded.append(x2);
    _decoded.append(x3);

    _isFinal = TRUE;
  }

  return _decoded.getData();
}

__END_NAMESPACE(SELFSOFT);
