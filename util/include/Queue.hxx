/*************************************************************************
 * $Id: Queue.hxx,v 1.8 2001/09/10 07:06:38 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/09/10 07:06:38 $
 * $Locker:  $
 * $Revision: 1.8 $
 *
 * Created:  8/25/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _QUEUE_HXX_
#define _QUEUE_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"
#include "String.hxx"

#define DEFAULT_QUEUE_SIZE 256

__BEGIN_NAMESPACE(SELFSOFT);

template <class T>
class Queue : public BaseObject {

public:

  Queue(int maxQueueSize = DEFAULT_QUEUE_SIZE);
  Queue(const Queue<T> &queue);
  ~Queue();

  // In append and remove operations, non-zero return values specify that
  // the queue is full (for append) or empty (for remove).
  int append(const T &item);
  int remove(T &item);

  void clear();

  boolean isFull() const;
  boolean isEmpty() const;

  int size() const;
  int capacity() const;

  Queue<T> &operator=(const Queue<T> &queue);

private:

  int _front;
  int _rear;
  T *_queue;
  int _maxQueueSize;

};

// Inline functions below

template <class T> inline Queue<T>::Queue(int maxQueueSize) {
  _maxQueueSize = maxQueueSize + 1;
  _queue = new T[_maxQueueSize];
  _front = _rear = 0;
}

template <class T> inline Queue<T>::Queue(const Queue<T> &queue) {
  _maxQueueSize = 0;
  _queue = NULL;
  _front = _rear = 0;
  operator=(queue);
}

template <class T> inline Queue<T>::~Queue() {
  delete [] _queue;
}

template <class T> inline int Queue<T>::append(const T &item) {
  int k = (_rear + 1) % _maxQueueSize;

  if(_front == k) {
    return -1;
  } else {
    _queue[_rear = k] = item;
  }

  return 0;
}

template <class T> inline int Queue<T>::remove(T &item) {
  if(isEmpty()) {
    return -1;
  }

  item = _queue[++_front %= _maxQueueSize];
  return 0;
}

template <class T> inline void Queue<T>::clear() {
  _front = _rear = 0;
}

template <class T> inline boolean Queue<T>::isFull() const {
  return (_front == ((_rear + 1) % _maxQueueSize));
}

template <class T> inline boolean Queue<T>::isEmpty() const {
  return (_front == _rear);
}

template <class T> inline int Queue<T>::size() const {
  if(_front > _rear) {
    return _rear + 1 + _maxQueueSize - _front;
  }

  return _rear - _front;
}

template <class T> inline int Queue<T>::capacity() const {
  return _maxQueueSize - 1;
}

template <class T> Queue<T> &Queue<T>::operator=(const Queue<T> &queue) {
  _front = queue._front;
  _rear = queue._rear;
  if(_maxQueueSize != queue._maxQueueSize) {
    _maxQueueSize = queue._maxQueueSize;
    if(_queue) {
      delete [] _queue;
    }
    _queue = new T[_maxQueueSize];
  }

  for(int i = 0; i < _maxQueueSize; i++) {
    _queue[i] = queue._queue[i];
  }

  return *this;
}

__END_NAMESPACE(SELFSOFT);

#endif
