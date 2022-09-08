/*************************************************************************
 * $Id: String.cxx,v 1.9 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.9 $
 *
 * Created:  1/19/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include <stdio.h>
#include "Error.hxx"
#include "String.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(StringException, Exception, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(String, BaseObject, 1);

String::String() {
    init(INITIAL_CAPACITY);
    _string[0] = '\0';
    _length = 0;
}

String::String(int size) {
    init(size);
    _string[0] = '\0';
    _length = 0;
}

String::String(char c) {
    init(INITIAL_CAPACITY);
    _string[0] = c;
    _string[1] = '\0';
    _length = 1;
}

String::String(const char *s) {
    if(s == NULL) {
        init(INITIAL_CAPACITY);
        _string[0] = '\0';
        _length = 0;
    } else {
        _length = strlen(s);
        init(_length + 1);
        memcpy(_string, s, _length);
        _string[_length] = '\0';
    }
}

String::String(const String &string) {
    _length = string.length();
    init(_length + 1);
    memcpy(_string, string._string, _length);
    _string[_length] = '\0';
}

String::~String() {
    free(_string);
}

String &String::format(char *format, ...) {
    va_list args;
    va_start(args, format);
    ensureCapacity(VSNPRINTF_CAPACITY);
#ifdef _WIN32
    int rc = _vsnprintf(_string, _capacity, format, args);
#else
    int rc = vsnprintf(_string, _capacity, format, args);
#endif
    if(rc == -1) {
        Error::warning("In String::format: buffer too small, result truncated");
    }
    va_end(args);

    return *this;
}

String &String::operator=(char c) {
    _length = 1;
    ensureCapacity(_length + 1);
    _string[0] = c;
    _string[1] = '\0';

    return *this;
}

String &String::operator=(const char *s) {
    if(s == NULL) {
        _string[0] = '\0';
        _length = 0;
    } else {
        _length = strlen(s);
        ensureCapacity(_length + 1);
    
        memcpy(_string, s, _length);
        _string[_length] = '\0';
    }
  
    return *this;
}

String &String::operator=(const String &string) {
    if(&string == this) {
        return *this;
    }

    _length = string._length;
    ensureCapacity(_length + 1);

    memcpy(_string, string._string, _length);
    _string[_length] = '\0';

    return *this;
}

String &String::operator+=(char c) {
    _length++;
    ensureCapacity(_length + 1);

    _string[_length - 1] = c;
    _string[_length] = '\0';

    return *this;
}

String &String::operator+=(const char *s) {
    if(s != NULL) {
        int copylength = strlen(s);
        int newlength = _length + copylength;
        ensureCapacity(newlength + 1);
    
        memcpy(_string + _length, s, copylength);
        _string[_length = newlength] = '\0';
    }

    return *this;
}

String &String::operator+=(const String &string) {
    int copylength = string._length;
    int newlength = _length + copylength;
    ensureCapacity(newlength + 1);

    memcpy(_string + _length, string._string, copylength);
    _string[_length = newlength] = '\0';

    return *this;
}

istream &operator>>(istream &in, String &string) {
    char buf[256];

    in >> buf;
    string = buf;

    return in;
}

ostream &operator<<(ostream &out, const String &string) {
    return out << string._string;
}

void String::prepend(char c) {
    _length++;
    ensureCapacity(_length + 1);

    memmove(&_string[1], _string, _length + 1);
    _string[0] = c;
}

void String::prepend(const char *s) {
    if(s != NULL) {
        int copylength = strlen(s);
        int newlength = _length + copylength;
        ensureCapacity(newlength + 1);
    
        memmove(&_string[copylength], _string, _length + 1);
        memcpy(_string, s, copylength);
        _length = newlength;
    }
}

void String::prepend(const String &string) {
    int copylength = string._length;
    int newlength = _length + copylength;
    ensureCapacity(newlength + 1);
  
    memmove(&_string[copylength], _string, _length + 1);
    memcpy(_string, string._string, copylength);
    _length = newlength;
}

void String::insert(char c, int pos) {
    if(pos == _length) {
        append(c);
        return;
    }
    checkBounds(pos);


    ensureCapacity(_length + 1 + 1);
    memmove(&_string[pos + 1], &_string[pos], _length - pos + 1);
    _string[pos] = c;
    _length++;
}

void String::insert(const char *s, int pos) {
    if(pos == _length) {
        append(s);
        return;
    }
    checkBounds(pos);

    int copylength = strlen(s);
    if(s != NULL) {
        ensureCapacity(_length + copylength + 1);
        memmove(&_string[pos + copylength], &_string[pos], _length - pos + 1);
        memcpy(&_string[pos], s, copylength);
        _length += copylength;
    }
}

void String::insert(const String &string, int pos) {
    if(pos == _length) {
        append(string);
    }
    checkBounds(pos);

    int copylength = string._length;
    if(copylength != 0) {
        ensureCapacity(_length + copylength + 1);
        if(&string == this) {
            char *buffer = new char[copylength];
            memcpy(buffer, _string, copylength);
            memmove(&_string[pos + copylength], &_string[pos], _length - pos + 1);
            memcpy(&_string[pos], buffer, copylength);
            delete [] buffer;
        } else {
            memmove(&_string[pos + copylength], &_string[pos], _length - pos + 1);
            memcpy(&_string[pos], string._string, copylength);
        }
        _length += copylength;
    }
}

void String::remove(int pos) {
    checkBounds(pos);
    memmove(&_string[pos], &_string[pos + 1], _length - pos + 1);
    _length--;
    _string[_length] = '\0';
}

void String::remove(int pos1, int pos2) {
    checkBounds(pos1, pos2);
    memmove(&_string[pos1], &_string[pos2 + 1], _length - pos2 + 1);
    _length -= (pos2 - pos1 + 1);
    _string[_length] = '\0';
}

int String::indexOf(char c) const {
    int idx;
    for(idx = 0; idx < _length && _string[idx] != c; idx++);

    return (idx >= _length || _string[idx] != c) ? -1 : idx;
}

int String::indexOf(char c, int pos) const {
    checkBounds(pos);
    int idx;
    for(idx = pos; idx < _length && _string[idx] != c; idx++);
  
    return (idx >= _length || _string[idx] != c) ? -1 : idx;
}

int String::indexOf(const char *s) const {
    if(s == NULL) {
        return -1;
    }
  
    char *pidx = strstr(_string, s);
    return pidx == NULL ? -1 : pidx - _string;
}

int String::indexOf(const char *s, int pos) const {
    checkBounds(pos);
  
    if(s == NULL) {
        return -1;
    }
  
    char *pidx = strstr(_string + pos, s);
    return pidx == NULL ? -1 : pidx - _string;
}

int String::indexOf(const String &string) const {
    if(string._string[0] == '\0') {
        return -1;
    }
  
    char *pidx = strstr(_string, string._string);
    return pidx == NULL ? -1 : pidx - _string;
}

int String::indexOf(const String &string, int pos) const {
    checkBounds(pos);
  
    if(string._string[0] == '\0') {
        return -1;
    }
  
    char *pidx = strstr(_string + pos, string._string);
    return pidx == NULL ? -1 : pidx - _string;
}

int String::lastIndexOf(char c) const {
    int idx;
    for(idx = _length - 1; idx >= 0 && _string[idx] != c; idx--);
  
    return (idx < 0 || _string[idx] != c) ? -1 : idx;
}

int String::lastIndexOf(char c, int pos) const {
    checkBounds(pos);
    int idx;
    for(idx = pos; idx >= 0 && _string[idx] != c; idx--);
  
    return (idx < 0 || _string[idx] != c) ? -1 : idx;
}

int String::lastIndexOf(const char *s) const {
    if(s == NULL) {
        return -1;
    }
  
    char *pidx;
    for(int i = _length - 1; i >= 0; i--) {
        if((pidx = strstr(&_string[i], s)) != NULL) {
            return pidx - _string;
        }
    }

    return -1;
}

int String::lastIndexOf(const char *s, int pos) const {
    checkBounds(pos);

    char *pidx;
    for(int i = pos - strlen(s); i >= 0; i--) {
        if((pidx = strstr(&_string[i], s)) != NULL &&
           pidx < &_string[pos]) {
            return pidx - _string;
        }
    }

    return -1;
}

int String::lastIndexOf(const String &string) const {
    char *pidx;
    for(int i = _length - 1; i >= 0; i--) {
        if((pidx = strstr(&_string[i], string._string)) != NULL) {
            return pidx - _string;
        }
    }

    return -1;
}

int String::lastIndexOf(const String &string, int pos) const {
    checkBounds(pos);

    char *pidx;
    for(int i = pos - string._length; i >= 0; i--) {
        if((pidx = strstr(&_string[i], string._string)) != NULL &&
           pidx < &_string[pos]) {
            return pidx - _string;
        }
    }

    return -1;
}

String String::before(int pos) const {
    checkBounds(pos);
    String ret(pos + 1);
    memcpy(ret._string, _string, pos);
    ret._string[pos] = '\0';
    ret._length = pos;

    return ret;
}

String String::after(int pos) const {
    checkBounds(pos);
    String ret(_length - pos + 1);
    memcpy(ret._string, _string + pos + 1, _length - pos);
    ret._string[_length - pos] = '\0';
    ret._length = _length - pos;

    return ret;
}

String String::substring(int pos) const {
    checkBounds(pos);
    String ret(pos + 1 + 1);
    memcpy(ret._string, _string, pos + 1);
    ret._string[pos + 1] = '\0';
    ret._length = pos + 1;

    return ret;
}

String String::substring(int pos1, int pos2) const {
    checkBounds(pos1, pos2);
    String ret(pos2 - pos1 + 1 + 1);
    memcpy(ret._string, _string + pos1, pos2 - pos1 + 1);
    ret._string[pos2 - pos1 + 1] = '\0';
    ret._length = pos2 - pos1 + 1;

    return ret;
}

String String::getTrimmed() const {
    int start, end;

    for(start = 0; (start < _length) && (_string[start] <= ' '); start++);
    for(end = _length - 1; (end > start) && (_string[end] <= ' '); end--);

    return ((start > 0) || (end < _length)) ? 
        substring(start, end) : String(_string);
}

String String::getLowerCaseForm() const {
    String ret(_length + 1);

    int i;
    for(i = 0; i < _length + 1; i++) {
        if (isalpha(_string[i])) {
            ret._string[i] = tolower(_string[i]);
        } else {
            ret._string[i] = _string[i];
        }
    }
    ret._length = _length;

    return ret;
}

String String::getUpperCaseForm() const {
    String ret(_length + 1);

    int i;
    for(i = 0; i < _length + 1; i++) {
        if (isalpha(_string[i])) {
            ret._string[i] = toupper(_string[i]);
        } else {
            ret._string[i] = _string[i];
        }
    }
    ret._length = _length;

    return String(ret);
}

String String::getReversed() const {
    String ret(_length + 1);

    int i;
    for(i = 0; i < _length; i++) {
        ret._string[i] = _string[_length - 1 - i];
    }

    ret._string[_length] = '\0';
    ret._length = _length;

    return ret;
}

String String::getReplaced(char oldc, char newc) const {
    if(oldc == 0) {
        return String(_string);
    }

    int i = 0;
    String ret(_length + 1);
    for(i = 0; i < _length; i++) {
        ret._string[i] = (_string[i] == oldc) ? newc : _string[i];
    }
    ret._string[_length] = '\0';
    ret._length = _length;

    return ret;
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
