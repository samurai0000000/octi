/*************************************************************************
 * $Id: Stack.hxx,v 1.7 2001/09/10 07:06:38 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/09/10 07:06:38 $
 * $Locker:  $
 * $Revision: 1.7 $
 *
 * Created:  8/25/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _STACK_HXX_
#define _STACK_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"
#include "String.hxx"

#define DEFAUL_STACK_SIZE 256

__BEGIN_NAMESPACE(SELFSOFT);

template <class T>
class Stack : public BaseObject {

public:

  Stack(int maxStackSize = DEFAUL_STACK_SIZE);
  Stack(const Stack &stack);
  ~Stack();

  // In push and pop operations, non-zero return values specify that
  // the stack is full (for push) or empty (for pop).
  int push(const T &item);
  int pop(T &item);

  void clear();
  
  boolean isFull() const;
  boolean isEmpty() const;

  int size() const;
  int capacity() const;

  Stack<T> &operator=(const Stack<T> &stack);

private:

  int _top;
  T *_stack;
  int _maxStackSize;

};

// Inline functions below

template <class T> inline Stack<T>::Stack(int maxStackSize) {
  _maxStackSize = maxStackSize;
  _stack = new T[_maxStackSize];
  _top = -1;
}

template <class T> inline Stack<T>::Stack(const Stack<T> &stack) {
  _maxStackSize = 0;
  _stack = NULL;
  _top = -1;
  operator=(stack);
}


template <class T> inline Stack<T>::~Stack() {
  delete [] _stack;
}

template <class T> inline int Stack<T>::push(const T &item) {
  if(isFull()) {
    return -1;
  }

  _stack[++_top] = item;
  return 0;
}

template <class T> inline int Stack<T>::pop(T &item) {
  if(isEmpty()) {
    return -1;
  }
  
  item = _stack[_top--];
  return 0;
}

template <class T> inline void Stack<T>::clear() {
  _top = -1;
}

template <class T> inline boolean Stack<T>::isFull() const {
  return (_top == _maxStackSize - 1);
}

template <class T> inline boolean Stack<T>::isEmpty() const {
  return (_top == -1);
}

template <class T> inline int Stack<T>::size() const {
  return _top + 1;
}

template <class T> inline int Stack<T>::capacity() const {
  return _maxStackSize;
}

template <class T> inline Stack<T> & Stack<T>::operator=(const Stack<T> &stack) {
  _top = stack._top;

  if(_maxStackSize != stack._maxStackSize) {
    _maxStackSize = stack._maxStackSize;
    if(_stack) {
      delete [] _stack;
    }
    _stack = new T[_maxStackSize];
  }

  for(int i = 0; i < _maxStackSize; i++) {
    _stack[i] = stack._stack[i];
  }

  return *this;
}

__END_NAMESPACE(SELFSOFT);

#endif
