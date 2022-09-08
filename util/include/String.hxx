/*************************************************************************
 * $Id: String.hxx,v 1.11 2000/11/15 06:42:07 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/15 06:42:07 $
 * $Locker:  $
 * $Revision: 1.11 $
 *
 * Created:  1/18/99 by Charles Chiou
 * Comments:
 *
 * A generic string class.
 *
 * The following methods may throw a StringException:
 *   String insert(char, int) const;
 *   String insert(const char *, int) const;
 *   String insert(const String &, int) const;
 *   const char &operator[](int) const;
 *   char &operator[](int);
 *   const char &charAt(int) const;
 *   char &charAt(int);
 *   int indexOf(char, int) const;
 *   int lastIndexOf(char, int) const;
 *   int contains(char, int) const;
 *   int contains(const char *, int) const;
 *   int contains(const String &, int) const;
 *   String after(int) const;
 *   String substring(int, int) const;
 *   String operator()(int, int) const;
 *
 * Notes:
 * When comparing String(NULL) to NULL,
 *     String(NULL) == NULL -> TRUE
 *     String(NULL) != NULL -> FALSE
 *     String(NULL) >  NULL -> FALSE
 *     String(NULL) >= NULL -> FALSE
 *     String(NULL) <  NULL -> FALSE
 *     String(NULL) <= NULL -> FALSE
 * and that,
 *     String(NULL).startsWith(NULL) -> TRUE
 *     String(NULL).endsWith(NULL) -> TRUE
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _STRING_HXX_
#define _STRING_HXX_

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <iostream>
#include "utilapi.h"
#include "BaseObject.hxx"
#include "Exception.hxx"

#define INITIAL_CAPACITY    512  // The default initial string size.
#define VSNPRINTF_CAPACITY 1024  // The maximum characters allowed with variable arg constructor that uses vsnprintf to initialize the string

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI StringException : public Exception {

    DECLARE_RUNTIME_SERIALIZABLE(StringException);

public:

    StringException();
    StringException(const char *msg);

};

class UTILAPI String : public BaseObject {

    DECLARE_RUNTIME_SERIALIZABLE(String);

public:

    // Constructors and destructor.
    String();
    String(int size);
    String(char c);
    String(const char *s);
    String(const String &string);
    ~String();

    operator const char *() const;

    // Assignment.
    String &format(char *format, ...);
    String &operator=(char c);
    String &operator=(const char *s);
    String &operator=(const String &string);
    String &operator+=(char c);
    String &operator+=(const char *s);
    String &operator+=(const String &string);

    // Stream operations.
    friend istream &operator>>(istream &in, String &string);
    friend ostream &operator<<(ostream &out, const String &string);

    // Comparison.
    boolean operator==(const char *s) const;
    boolean operator==(const String &string) const;
    boolean operator!=(const char *s) const;
    boolean operator!=(const String &string) const;
    boolean operator>(const char *s) const;
    boolean operator>(const String &string) const;
    boolean operator>=(const char *s) const;
    boolean operator>=(const String &string) const;
    boolean operator<(const char *s) const;
    boolean operator<(const String &string) const;
    boolean operator<=(const char *s) const;
    boolean operator<=(const String &string) const;
    boolean compareIgnoreCase(const char *s) const;
    boolean compareIgnoreCase(const String &string) const;

    // Inmutable append
    String operator+(char c) const;
    String operator+(const char *s) const;
    String operator+(const String &string) const;
    friend String operator+(char c, const String &string);
    friend String operator+(const char *s, const String &string);

    // Addition and deletion operations.
    void append(char c);
    void append(const char *);
    void append(const String &string);
    void prepend(char c);
    void prepend(const char *s);
    void prepend(const String &string);
    void insert(char c, int pos);
    void insert(const char *s, int pos);
    void insert(const String &string, int pos);
    void remove(int pos);
    void remove(int pos1, int pos2);
    void delFirst(char c);
    void delFirst(char c, int pos);
    void delFirst(const char *s);
    void delFirst(const char *s, int pos);
    void delFirst(const String &string);
    void delFirst(const String &string, int pos);
    void delLast(char c);
    void delLast(char c, int pos);
    void delLast(const char *s);
    void delLast(const char *s, int pos);
    void delLast(const String &string);
    void delLast(const String &string, int pos);

    // Indexing.
    const char &operator[](int pos) const;
    char &operator[](int pos);
    const char &charAt(int pos) const;
    char &charAt(int pos);
    int indexOf(char c) const;
    int indexOf(char c, int pos) const;
    int indexOf(const char *s) const;
    int indexOf(const char *s, int pos) const;
    int indexOf(const String &string) const;
    int indexOf(const String &string, int pos) const;
    int lastIndexOf(char c) const;
    int lastIndexOf(char c, int pos) const;
    int lastIndexOf(const char *s) const;
    int lastIndexOf(const char *s, int pos) const;
    int lastIndexOf(const String &string) const;
    int lastIndexOf(const String &string, int pos) const;

    // Matching.
    boolean startsWith(char c) const;
    boolean startsWith(const char *s) const;
    boolean startsWith(const String &string) const;
    boolean endsWith(char c) const;
    boolean endsWith(const char *s) const;
    boolean endsWith(const String &string) const;
    boolean contains(char c) const;
    boolean contains(char c, int pos) const;
    boolean contains(const char *s) const;
    boolean contains(const char *s, int pos) const;
    boolean contains(const String &string) const;
    boolean contains(const String &string, int pos) const;

    // Extraction.
    String before(int pos) const;
    String after(int pos) const;
    String substring(int pos) const;
    String substring(int pos1, int pos2) const;
    String operator()(int pos1, int pos2) const;

    // Other functions.
    int length() const;
    String getTrimmed() const;
    String getLowerCaseForm() const;
    String getUpperCaseForm() const;
    String getReversed() const;
    String getReplaced(char oldc, char newc) const;
    void trim();
    void toLowerCase();
    void toUpperCase();
    void reverse();
    void replace(char oldc, char newc);

    // Overriding BaseObject functions
    unsigned long hashCode() const;
    const char *toString() const;

protected:

    void init(int size);
    int capacity() const;
    void doubleCapacity();
    void expandCapacity(int size);
    void ensureCapacity(int size);
    void syncCapacity();

    char *_string;
    int _length;
    int _capacity;

private:

    void checkBounds(int pos) const throw(StringException);
    void checkBounds(int pos1, int pos2) const throw (StringException);

};

// Inline functions for StringException class are below here.

inline StringException::StringException() : Exception() {

}

inline StringException::StringException(const char *msg) : Exception(msg) {

}

// Inline functions for String class are below here.

inline void String::init(int size) {
    if(size <= 0) {
        _capacity = INITIAL_CAPACITY;
    } else {
        _capacity = size;
    }
    _string = (char *) malloc(sizeof(char) * _capacity);
}

inline int String::capacity() const {
    return _capacity;
}

inline void String::expandCapacity(int size) {
    if(size <= 0) {
        return;
    }

    _capacity += size;
    _string = (char *) realloc(_string, sizeof(char) * _capacity);
}

inline void String::doubleCapacity() {
    expandCapacity(_capacity * 2);
}

inline void String::ensureCapacity(int size) {
    if(size <= 0 || size <= _capacity) {
        return;
    }

    _capacity = size;
    _string = (char *) realloc(_string, sizeof(char) * _capacity);
}

inline void String::syncCapacity() {
    if(_capacity > _length + 1) {
        _string = (char *) realloc(_string, sizeof(char) * _length + 1);
        _capacity = _length + 1;
    }
}

inline void String::checkBounds(int pos) const throw(StringException) {
    if(pos < 0 || pos >= _length) {
        throw StringException("pos out of bounds");
    }
}

inline void String::checkBounds(int pos1, int pos2) const
    throw (StringException) {
    if(pos1 < 0) {
        throw StringException("pos1 out of bounds");
    } else if(pos2 >= _length) {
        throw StringException("pos2 out of bounds");
    } else if(pos1 > pos2) {
        throw StringException("pos1 > pos2");
    }
}

inline String::operator const char *() const {
    return _string;
}

inline boolean String::operator==(const char *s) const {
    return (s == NULL) ? (_string[0] == '\0') : (strcmp(_string, s) == 0);
}

inline boolean String::operator==(const String &string) const {
    return (strcmp(_string, string._string) == 0);
}

inline boolean String::operator!=(const char *s) const {
    return (s == NULL) ? (_string[0] != '\0') : (strcmp(_string, s) != 0);
}

inline boolean String::operator!=(const String &string) const {
    return (strcmp(_string, string._string) != 0);
}


inline boolean String::operator>(const char *s) const {
    return (s == NULL) ? (_string[0] != '\0') : (strcmp(_string, s) > 0);
}

inline boolean String::operator>(const String &string) const {
    return (strcmp(_string, string._string) > 0);
}

inline boolean String::operator>=(const char *s) const {
    return (s == NULL) ? (_string[0] != '\0') : (strcmp(_string, s) >= 0);
}

inline boolean String::operator>=(const String &string) const {
    return (strcmp(_string, string._string) >= 0);
}

inline boolean String::operator<(const char *s) const {
    return (s == NULL) ? (_string[0] != '\0') : (strcmp(_string, s) < 0);
}

inline boolean String::operator<(const String &string) const {
    return (strcmp(_string, string._string) < 0);
}

inline boolean String::operator<=(const char *s) const {
    return (s == NULL) ? (_string[0] != '\0') : (strcmp(_string, s) <= 0);
}

inline boolean String::operator<=(const String &string) const {
    return (strcmp(_string, string._string) <= 0);
}

inline boolean String::compareIgnoreCase(const char *s) const {
#ifdef _WIN32
    return (s == NULL) ? (_string[0] != '\0') : (strcmpi(_string, s) == 0);
#else
    return (s == NULL) ? (_string[0] != '\0') : (strcasecmp(_string, s) == 0);
#endif
}

inline boolean String::compareIgnoreCase(const String &string) const {
#ifdef _WIN32
    return (strcmpi(_string, string._string) == 0);
#else
    return (strcasecmp(_string, string._string) == 0);
#endif
}

inline String String::operator+(char c) const {
    String ret(_string);
    return ret += c;
}

inline String String::operator+(const char *s) const {
    String ret(_string);
    return ret += s;
}

inline String String::operator+(const String &string) const {
    String ret(_string);
    return ret += string;
}

inline String operator+(char c, const String &string) {
    String ret(c);
    return ret += string;
}

inline String operator+(const char *s, const String &string) {
    String ret(s);
    return ret += string;
}

inline void String::append(char c) {
    operator+=(c);
}

inline void String::append(const char *s) {
    operator+=(s);
}

inline void String::append(const String &string) {
    operator+=(string);
}

inline void String::delFirst(char c) {
    int idx;
    if((idx = indexOf(c)) >= 0) {
        remove(idx);
    }
}

inline void String::delFirst(char c, int pos) {
    int idx;
    if((idx = indexOf(c, pos)) >= 0) {
        remove(idx);
    }
}

inline void String::delFirst(const char *s) {
    int idx;
    if(s != NULL && (idx = indexOf(s)) >= 0) {
        remove(idx, idx + strlen(s) - 1);
    }
}

inline void String::delFirst(const char *s, int pos) {
    int idx;
    if(s != NULL && (idx = indexOf(s, pos)) >= 0) {
        remove(idx, idx + strlen(s) - 1);
    }
}

inline void String::delFirst(const String &string) {
    int idx;
    if(string._string[0] != '\0' && (idx = indexOf(string._string)) >= 0) {
        remove(idx, idx + string._length - 1);
    }
}

inline void String::delFirst(const String &string, int pos) {
    int idx;
    if(string._string[0] != '\0' && (idx = indexOf(string._string, pos)) >= 0) {
        remove(idx, idx + string._length - 1);
    }
}

inline void String::delLast(char c) {
    int idx;
    if((idx = lastIndexOf(c)) >= 0) {
        remove(idx);
    }
}

inline void String::delLast(char c, int pos) {
    int idx;
    if((idx = lastIndexOf(c, pos)) >= 0) {
        remove(idx);
    }
}

inline void String::delLast(const char *s) {
    int idx;
    if(s != NULL && (idx = lastIndexOf(s)) >= 0) {
        remove(idx, idx + strlen(s) - 1);
    }
}

inline void String::delLast(const char *s, int pos) {
    int idx;
    if(s != NULL && (idx = lastIndexOf(s, pos)) >= 0) {
        remove(idx, idx + strlen(s) - 1);
    }
}

inline void String::delLast(const String &string) {
    int idx;
    if(string._string[0] != '\0' && (idx = lastIndexOf(string._string)) >= 0) {
        remove(idx, idx + string._length - 1);
    }
}

inline void String::delLast(const String &string, int pos) {
    int idx;
    if(string._string[0] != '\0' && (idx = lastIndexOf(string._string, pos)) >= 0) {
        remove(idx, idx + string._length);
    }
}

inline const char &String::operator[](int pos) const {
    checkBounds(pos);
    return _string[pos];
}

inline char &String::operator[](int pos) {
    checkBounds(pos);
    return _string[pos];
}

inline const char &String::charAt(int pos) const {
    checkBounds(pos);
    return _string[pos];
}

inline char &String::charAt(int pos){
    checkBounds(pos);
    return _string[pos];
}


inline boolean String::startsWith(char c) const {
    return (_length != 0) && (_string[0] == c);
}

inline boolean String::startsWith(const char *s) const {
    if(s == NULL) {
        return _string[0] == '\0';
    }

    return strncmp(_string, s, strlen(s)) == 0;
}

inline boolean String::startsWith(const String &string) const {
    if(string._string[0] == '\0') {
        return _string[0] == '\0';
    }

    return strncmp(_string, string._string, string._length) == 0;
}

inline boolean String::endsWith(char c) const {
    return (_length != 0) && (_string[_length - 1] == c);
}

inline boolean String::endsWith(const char *s) const {
    if(s == NULL) {
        return _string[0] == '\0';
    }

    int len;
    if((len = strlen(s)) > _length) {
        return FALSE;
    }
    return strncmp(&_string[_length - len], s, len) == 0;
}

inline boolean String::endsWith(const String &string) const {
    if(string._string[0] == '\0') {
        return _string[0] == '\0';
    }

    int len;
    if((len = string._length) > _length) {
        return FALSE;
    }
    return strncmp(&_string[_length - len], string._string, len) == 0;
}

inline boolean String::contains(char c) const {
    return strchr(_string, c) != NULL;
}

inline boolean String::contains(char c, int pos) const {
    checkBounds(pos);
    return strchr(&_string[pos], c) != NULL;
}

inline boolean String::contains(const char *s) const {
    return strstr(_string, s) != NULL;
}

inline boolean String::contains(const char *s, int pos) const {
    checkBounds(pos);
    return strstr(&_string[pos], s) != NULL;
}

inline boolean String::contains(const String &string) const {
    return strstr(_string, string._string) != NULL;
}

inline boolean String::contains(const String &string, int pos) const {
    checkBounds(pos);
    return strstr(&_string[pos], string._string) != NULL;
}

inline String String::operator()(int pos1, int pos2) const {
    return substring(pos1, pos2);
}

inline int String::length() const {
    return _length;
}

inline void String::trim() {
    *this = getTrimmed();
}

inline void String::toLowerCase() {
    *this = getLowerCaseForm();
}

inline void String::toUpperCase() {
    *this = getUpperCaseForm();
}

inline void String::reverse() {
    *this = getReversed();
}

inline void String::replace(char oldc, char newc) {
    *this = getReplaced(oldc, newc);
}

inline const char *String::toString() const {
    return _string;
}

inline unsigned long String::hashCode() const {
    if(_string == NULL) {
        return 0;
    }

    unsigned long h = 0;
    for(unsigned long i = 0; i < (unsigned long)_length; i++) {
        h += (_string[i] +  h);
    }

    return h;
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
