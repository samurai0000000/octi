/*************************************************************************
 * $Id: BufferedArray.hxx,v 1.8 2001/08/09 03:11:08 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 03:11:08 $
 * $Locker:  $
 * $Revision: 1.8 $
 *
 * Created:  1/25/99 by Charles Chiou
 * Comments:
 *     This class is used for storing an array that is mutable in size, but
 *     is generally expected to grow. This is a data holder class and does
 *     not provide much array manipulation functions as normally expected
 *     from arrays. There is no mechanism for pattern matching for data
 *     stored in this object.
 * 
 *     If the template class T is not a primitive data type, then there is a
 *     penalty (can be heavy if the object is large!) for invoking the class
 *     object constructor everytime there is an assignment.
 *     To avoid it, use the Ptr class or store only pointer to the objects.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _BUFFEREDARRAY_HXX_
#define _BUFFEREDARRAY_HXX_

#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "BaseObject.hxx"
#include "ArrayException.hxx"
#include "String.hxx"
#include "Collection.hxx"

#define DEFAULT_BUFFERED_ARRAY_SIZE 16

__BEGIN_NAMESPACE(SELFSOFT);

template <class T> class BufferedArrayIterator;

template <class T>
class BufferedArray : public AbstractList<T> {

public:

  BufferedArray(int size = DEFAULT_BUFFERED_ARRAY_SIZE);
  BufferedArray(const T *data, int len);
  BufferedArray(const BufferedArray<T> &array);
  ~BufferedArray();

  BufferedArray<T> &operator=(const BufferedArray<T> &array);

  boolean append(const T &datum);
  boolean append(const T *data, int len);
  boolean append(const BufferedArray<T> &data);
  boolean prepend(const T &datum);
  boolean prepend(const T *data, int len);
  boolean prepend(const BufferedArray<T> &data);
  boolean insert(const T &datum, int pos);
  boolean insert(const T *data, int len, int pos);
  boolean insert(const BufferedArray<T> &data, int pos);

  boolean remove(int pos);
  boolean remove(int pos1, int pos2);
  boolean removeElement(const T &datum, int occurence = 1);
  boolean removeFirst();
  boolean removeLast();

  const T &get(int pos) const;
  T &get(int pos);
  const T &operator[](int pos) const;
  T &operator[](int pos);
  void set(const T &datum, int pos);

  int firstIndexOf(const T &datum) const;
  int lastIndexOf(const T &datum) const;
  int size() const;

  ListIterator<T> *getIterator() const;
  void clear();
  
  int capacity() const;
  void doubleCapacity();
  void expandCapacity(int size);
  void ensureCapacity(int size);
  void syncCapacity();

  operator const T *() const;
  operator T *();
  const T *getData() const;
  const T *getSyncData();

protected:

  friend class BufferedArrayIterator<T>;

  T *_data;
  int _length;
  int _capacity;
  size_t _sizeT;

private:
  
  void checkBounds(int pos) const throw(ArrayException);
  void checkBounds(int pos1, int pos2) const throw(ArrayException);

};


template <class T>
class BufferedArrayIterator : public ListIterator<T> {

public:

  BufferedArrayIterator();
  BufferedArrayIterator(const BufferedArray<T> &array);

  BufferedArrayIterator &operator=(const BufferedArray<T> &array);

  boolean isNotNull() const;
  boolean isNextNotNull() const;

  const T *first();
  const T *current();
  const T *prev();
  const T *next();
  const T *last();

protected:

  const BufferedArray<T> *_array;
  int _pos;

};


// Inline functions below

template <class T> inline
BufferedArray<T>::BufferedArray(int size) {
  _sizeT = sizeof(T);
  _capacity = size;
  _data = (T *) malloc(_sizeT * _capacity);
  _length = 0;
}

template <class T> inline
BufferedArray<T>::BufferedArray(const T *data, int len) {
  ASSERT(data != NULL);
  _sizeT = sizeof(T);
  _capacity = len;
  _data = (T *) malloc(_sizeT * _capacity);
  memcpy(_data, data, len * _sizeT);
  _length = _capacity;
}

template <class T> inline
BufferedArray<T>::BufferedArray(const BufferedArray<T> &array) {
  _sizeT = sizeof(T);
  _capacity = 0;
  _data = NULL;
  _length = 0;
  operator=(array);
}

template <class T> inline
BufferedArray<T>::~BufferedArray() {
  free(_data);
}

template <class T> inline
BufferedArray<T> &BufferedArray<T>::operator=(const BufferedArray<T> &array) {
  if(&array != this) {
    clear();
    ensureCapacity(array._length);
    for(int i = 0; i < array._length; i++) {
      _data[i] = array._data[i];
    }
    _length = array._length;
  }

  return *this;
}

template <class T> inline
int BufferedArray<T>::size() const {
  return _length;
}

template <class T> inline
int BufferedArray<T>::capacity() const {
  return _capacity;
}

template <class T> inline
void BufferedArray<T>::doubleCapacity() {
  expandCapacity(_capacity * 2);
}

template <class T> inline
void BufferedArray<T>::expandCapacity(int size) {
  if(size <= 0) {
    return;
  }

  _capacity += size;
  _data = (T *) realloc(_data, _sizeT * _capacity);
}

template <class T> inline
void BufferedArray<T>::clear() {
  memset(_data, 0, _length * _sizeT);
  _length = 0;
}

template <class T> inline
void BufferedArray<T>::ensureCapacity(int size) {
  if(size <= 0 || size <= _capacity) {
    return;
  }

  _capacity = size;
  _data = (T *) realloc(_data, _sizeT * _capacity);
}

template <class T> inline
void BufferedArray<T>::syncCapacity() {
  if(_length == 0) {
    free(_data);
    _data = 0;
  } else if(_capacity > _length) {
    _data = (T *) realloc(_data, _sizeT * _length);
    _capacity = _length;
  }
}

template <class T> inline
BufferedArray<T>::operator const T *() const {
  return _data;
}

template <class T> inline
BufferedArray<T>::operator T*() {
  return _data;
}

template <class T> inline
const T *BufferedArray<T>::getData() const {
  return _data;
}

template <class T> inline
const T *BufferedArray<T>::getSyncData() {
  syncCapacity();
  return _data;
}

template <class T> inline
boolean BufferedArray<T>::append(const T &datum) {
  ensureCapacity(_length + 1);
  _data[_length] = datum;
  _length++;
  return TRUE;
}

template <class T> inline
boolean BufferedArray<T>::append(const T *data, int len) {
  ASSERT(data != NULL);
  ensureCapacity(_length + len);
  memcpy(&_data[_length], data, len * _sizeT);
  _length += len;
  return TRUE;
}

template <class T> inline
boolean BufferedArray<T>::append(const BufferedArray<T> &data) {
  ensureCapacity(_length + data._length);
  memcpy(&_data[_length], data._data, data._length * _sizeT);
  _length += data._length;
  return TRUE;
}

template <class T> inline
boolean BufferedArray<T>::prepend(const T &datum) {
  ensureCapacity(_length + 1);
  memmove(&_data[1], _data, _length * _sizeT);
  _data[0] = datum;
  _length++;
  return TRUE;
}

template <class T> inline
boolean BufferedArray<T>::prepend(const T *data, int len) {
  ASSERT(data != NULL);
  ensureCapacity(_length + len);
  memmove(&_data[len], _data, _length * _sizeT);
  memcpy(_data, data, len * _sizeT);
  _length += len;
  return TRUE;
}

template <class T> inline
boolean BufferedArray<T>::prepend(const BufferedArray<T> &data) {
  ensureCapacity(_length + data._length);
  memmove(&_data[data._length], _data, _length * _sizeT);
  memcpy(_data, data._data, data._length * _sizeT);
  _length += data._length;
  return TRUE;
}

template <class T> inline
boolean BufferedArray<T>::insert(const T &datum, int pos) {
  if(pos == _length) {
    return append(datum);
  } else {
    checkBounds(pos);
    ensureCapacity(_length + 1);
    memmove(&_data[pos + 1], &_data[pos], (_length - pos) * _sizeT);
    _data[pos] = datum;
    _length++;
  }

  return TRUE;
}

template <class T> inline
boolean BufferedArray<T>::insert(const T *data, int len, int pos) {
  ASSERT(data != NULL);
  if(pos == _length) {
    return append(data, len);
  } else {
    checkBounds(pos);
    
    if(data != NULL) {
      ensureCapacity(_length + len);
      memmove(&_data[pos + len], &_data[pos], (_length - pos) * _sizeT);
      memcpy(&_data[pos], data, len * _sizeT);
      _length += len;
    } else {
      return FALSE;
    }
  }

  return TRUE;
}

template <class T> inline
boolean BufferedArray<T>::insert(const BufferedArray<T> &data, int pos) {
  if(pos == _length) {
    return append(data);
  } else {
    checkBounds(pos);
    
    if(data._length != 0) {
      ensureCapacity(_length + data._length);
      memmove(&_data[pos + data._length], &_data[pos],
	      (_length - pos) * _sizeT);
      memcpy(&_data[pos], data._data, data._length * _sizeT);
      _length += data._length;
    } else {
      return FALSE;
    }
  }
  
  return TRUE;
}

template <class T> inline
boolean BufferedArray<T>::remove(int pos) {
  _length--;
  if(pos != _length) {
    memmove(&_data[pos], &_data[pos + 1], (_length - pos) * _sizeT);
  } else {
    return FALSE;
  }

  return TRUE;
}

template <class T> inline
boolean BufferedArray<T>::remove(int pos1, int pos2) {
  checkBounds(pos1, pos2);
  if(_length - pos2 > 0) {
    memmove(&_data[pos1], &_data[pos2 + 1], (_length - pos2) * _sizeT);
    _length -= (pos2 - pos1 + 1);
  } else {
    return FALSE;
  }

  return TRUE;
}

template <class T> inline
boolean BufferedArray<T>::removeElement(const T &datum, int occurence) {
  int o = 0;
  for(int i = 0; i < _length; i++) {
    if(_data[i] == datum && ++o >= occurence) {
      return remove(i);
    }
  }

  return FALSE;
}

template <class T> inline
boolean BufferedArray<T>::removeFirst() {
  if(_length > 0) {
    memmove(_data, &_data[1], (--_length) * _sizeT);
  } else {
    return FALSE;
  }

  return TRUE;
}

template <class T> inline
boolean BufferedArray<T>::removeLast() {
  if(_length > 0) {
    _length--;
  } else {
    return FALSE;
  }

  return TRUE;
}

template <class T> inline
const T &BufferedArray<T>::get(int pos) const {
  checkBounds(pos);
  return _data[pos];
}

template <class T> inline
T &BufferedArray<T>::get(int pos) {
  checkBounds(pos);
  return _data[pos];
}

template <class T> inline
const T &BufferedArray<T>::operator[](int pos) const {
  checkBounds(pos);
  return _data[pos];
}

template <class T> inline
T &BufferedArray<T>::operator[](int pos) {
  checkBounds(pos);
  return _data[pos];
}

template <class T> inline
void BufferedArray<T>::set(const T &datum, int pos) {
  checkBounds(pos);
  _data[pos] = datum;
}

template <class T> inline
int BufferedArray<T>::firstIndexOf(const T &datum) const {
  for(int i = 0; i < _length; i++) {
    if(datum == _data[i]) {
      return i;
    }
  }

  return -1;
}

template <class T> inline
int BufferedArray<T>::lastIndexOf(const T &datum) const {
  for(int i = _length - 1; i >= 0; i--) {
    if(datum == _data[i]) {
      return i;
    }
  }

  return -1;
}

template <class T> inline
void BufferedArray<T>::checkBounds(int pos) const throw(ArrayException) {
  if(pos < 0 || pos >= _length) {
    throw ArrayException("pos out of bounds");
  }
}

template <class T> inline
void BufferedArray<T>::checkBounds(int pos1, int pos2) const throw(ArrayException) {
  if(pos1 < 0) {
    throw ArrayException("pos1 out of bounds");
  } else if(pos2 >= _length) {
    throw ArrayException("pos2 out of bounds");
  } else if(pos1 > pos2) {
    throw ArrayException("pos1 > pos2");
  }
}


template <class T> inline
ListIterator<T> *BufferedArray<T>::getIterator() const {
  return new BufferedArrayIterator<T>(*this);
}

template <class T> inline
BufferedArrayIterator<T>::BufferedArrayIterator() {
  _array = NULL;
  _pos = -1;
}

template <class T> inline
BufferedArrayIterator<T>::BufferedArrayIterator(const BufferedArray<T> &array) {
  _array = &array;
  _pos = -1;
}

template <class T> inline
BufferedArrayIterator<T> &BufferedArrayIterator<T>::operator=(const BufferedArray<T> &array) {
  _array = &array;
  _pos = -1;
  return *this;
}

template <class T> inline
boolean BufferedArrayIterator<T>::isNotNull() const {
  return (_pos >= 0 && _pos < _array->_length);
}

template <class T> inline
boolean BufferedArrayIterator<T>::isNextNotNull() const {
  return ((_pos + 1 >= 0) && (_pos + 1 < _array->_length));
}

template <class T> inline
const T *BufferedArrayIterator<T>::first() {
  if(_array->_length < 1) {
    return NULL;
  }
  _pos = 0;
  return &(_array->_data[_pos]);
}

template <class T> inline
const T *BufferedArrayIterator<T>::current() {
  return &(_array->_data[_pos]);
}

template <class T> inline
const T *BufferedArrayIterator<T>::prev() {
  if(_array->_length < 1 || _pos - 1 < 0) {
    return NULL;
  }

  _pos--;
  return &(_array->_data[_pos]);
}

template <class T> inline
const T *BufferedArrayIterator<T>::next() {
  if(_array->_length < 1 || _pos + 1 >= _array->_length) {
    return NULL;
  }

  _pos++;
  return &(_array->_data[_pos]);
}

template <class T> inline
const T *BufferedArrayIterator<T>::last() {
  if(_array->_length < 1) {
    return NULL;
  }

  _pos = _array->_length - 1;
  return &(_array->_data[_pos]);
}

__END_NAMESPACE(SELFSOFT);

#endif
