/*************************************************************************
 * $Id: Slist.hxx,v 1.9 2001/08/09 03:11:08 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 03:11:08 $
 * $Locker:  $
 * $Revision: 1.9 $
 *
 * Created:  9/4/99 by Charles Chiou
 * Comments: A singly-linked list.
 *           Avoid calling the lastIndexOf method since there is no effective
 *           implementation in a singly-linked list for backward traversal.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _SLIST_HXX_
#define _SLIST_HXX_

#include "debug.h"
#include "BaseObject.hxx"
#include "String.hxx"
#include "Collection.hxx"

__BEGIN_NAMESPACE(SELFSOFT);


template <class T> class Slist;
template <class T> class SlistIterator;


class UTILAPI SlistException : public Exception {

  DECLARE_RUNTIME_SERIALIZABLE(SlistException);

public:

  SlistException();
  SlistException(const char *msg);

};

template <class T>
class SlistNode : public BaseObject {

private:

  friend class Slist<T>;
  friend class SlistIterator<T>;

  T _data;
  SlistNode<T> *_next;

};


template <class T>
class Slist : public AbstractList<T> {
  
public:

  Slist();
  Slist(const Slist<T> &list);
  ~Slist();

  Slist<T> &operator=(const Slist<T> &list);

  boolean append(const T &datum);
  boolean append(const T *data, int len);
  boolean append(const Slist<T> &list);
  boolean prepend(const T &datum);
  boolean prepend(const Slist<T> &list);
  boolean prepend(const T *data, int len);
  boolean insert(const T &datum, int pos);
  boolean insert(const T *data, int len, int pos);
  boolean insert(Slist<T> &data, int pos);

  boolean remove(int pos);
  boolean remove(int pos1, int pos2);
  boolean removeElement(const T &datum, int occurence = 1);
  boolean removeFirst();
  boolean removeLast();

  const T &get(int pos) const;
  T &get(int pos);
  void set(const T &datum, int pos);

  int firstIndexOf(const T &datum) const;
  int lastIndexOf(const T &datum) const;  // Avoid using this call
  int size() const;
  boolean isEmpty() const;

  ListIterator<T> *getIterator() const;
  void clear();

private:

  friend class SlistIterator<T>;

  SlistNode<T> *_first, *_last;
  int _size;

};


template <class T>
class SlistIterator : public ListIterator<T> {

public:

  SlistIterator();
  SlistIterator(const Slist<T> &list);
  
  SlistIterator<T> &operator=(const Slist<T> &list);

  boolean isNotNull() const;
  boolean isNextNotNull() const;

  const T *first();
  const T *current();
  const T *next();
  const T *last();

private:  

  const Slist<T> *_list;
  SlistNode<T> *_current, *_next;

};


// Inline functions below

template <class T> inline Slist<T>::Slist() {
  _first = _last = NULL;
  _size = 0;
}

template <class T> inline Slist<T>::Slist(const Slist<T> &list) {
  _first = _last = NULL;
  _size = 0;
  operator=(list);
}

template <class T> inline Slist<T>::~Slist() {
  clear();
}

template <class T> inline Slist<T> &Slist<T>::operator=(const Slist<T> &list) {
  if(&list == this) {
    return *this;
  }

  clear();
  SlistNode<T> *node = list._first;
    
  if(node != NULL) {
    _first = _last = new SlistNode<T>();
    _size++;
    _last->_data = node->_data;
    _last->_next = NULL;
    
    while((node = node->_next) != NULL) {
      _last->_next = new SlistNode<T>();
      _size++;
      _last = _last->_next;
      _last->_data = node->_data;
    }
    _last->_next = NULL;
  }

  return *this;
}

template <class T> inline boolean Slist<T>::append(const T &datum) {
  SlistNode<T> *node = new SlistNode<T>();
  _size++;
  node->_data = datum;
  node->_next = NULL;

  if(_first == NULL) {
    _first = _last = node;
  } else {
    _last->_next = node;
    _last = node;
  }

  return TRUE;
}

template <class T> inline boolean Slist<T>::append(const T *data, int len) {
  ASSERT(data != NULL);
  SlistNode<T> *node = NULL, *newfirst = NULL;

  for(int i = 0; i < len; i++) {
    if(newfirst == NULL) {
      newfirst = node = new SlistNode<T>();
      _size++;
      newfirst->_data = data[i];
    } else {
      node->_next = new SlistNode<T>();
      _size++;
      node = node->_next;
      node->_data = data[i];
    }
  }

  if(newfirst == NULL) {
    return FALSE;
  }

  node->_next = NULL;

  if(_first == NULL) {
    _first = newfirst;
    _last = node;
  } else {
    _last->_next = newfirst;
    _last = node;
  }

  return TRUE;
}

template <class T> inline boolean Slist<T>::append(const Slist<T> &list) {
  SlistNode<T> *scan, *node = NULL, *newfirst = NULL;

  for(scan = list._first; scan != NULL; scan = scan->_next) {
    if(newfirst == NULL) {
      newfirst = node = new SlistNode<T>();
      _size++;
      newfirst->_data = scan->_data;
    } else {
      node->_next = new SlistNode<T>();
      _size++;
      node = node->_next;
      node->_data = scan->_data;
    }
  }

  if(newfirst == NULL) {
    return FALSE;
  }

  node->_next = NULL;

  if(_first == NULL) {
    _first = newfirst;
    _last = node;
  } else {
    _last->_next = newfirst;
    _last = node;
  }

  return TRUE;
}

template <class T> inline boolean Slist<T>::prepend(const T &datum) {
  SlistNode<T> *node = new SlistNode<T>();
  _size++;

  node->_data = datum;
  node->_next = _first;

  if(_last == NULL) {
    _first = _last = node;
  } else {
    _first = node;
  }

  return TRUE;
}

template <class T> inline boolean Slist<T>::prepend(const T *data, int len) {
  ASSERT(data != NULL);
  SlistNode<T> *node = NULL, *newfirst = NULL;

  for(int i = 0; i < len; i++) {
    if(newfirst == NULL) {
      newfirst = node = new SlistNode<T>();
      _size++;
      newfirst->_data = data[i];
    } else {
      node->_next = new SlistNode<T>();
      _size++;
      node = node->_next;
      node->_data = data[i];
    }
  }

  if(newfirst == NULL) {
    return FALSE;
  }

  if(_last == NULL) {
    _first = newfirst;
    _last = node;
  } else {
    node->_next = _first;
    _first = newfirst;
  }

  return TRUE;
}

template <class T> inline boolean Slist<T>::prepend(const Slist<T> &list) {
  SlistNode<T> *scan, *node = NULL, *newfirst = NULL;

  for(scan = list._first; scan != NULL; scan = scan->_next) {
    if(newfirst == NULL) {
      newfirst = node = new SlistNode<T>();
      _size++;
      newfirst->_data = scan->_data;
    } else {
      node->_next = new SlistNode<T>();
      _size++;
      node = node->_next;
      node->_data = scan->_data;
    }
  }

  if(newfirst == NULL) {
    return FALSE;
  }

  if(_last == NULL) {
    _first = newfirst;
    _last = node;
  } else {
    node->_next = _first;
    _first = newfirst;
  }

  return TRUE;
}

template <class T> inline boolean Slist<T>::insert(const T &datum, int pos) {
  SlistNode<T> *node = NULL;

  if(pos == 0) {
    node = new SlistNode<T>();
    _size++;

    node->_data = datum;
    node->_next = _first;

    if(_last == NULL) {
      _first = _last = node;
    } else {
      _first = node;
    }
  } else {
    SlistNode<T> *current, *previous;
    current = previous = _first;
    
    for(int i = 0; i < pos; i++) {
      if(current == NULL) {
	throw SlistException("pos out of bounds");
      }
      previous = current;
      current = current->_next;
    }
    
    node = new SlistNode<T>();
    _size++;
    
    node->_data = datum;
    node->_next = current;
    previous->_next = node;
  }

  return TRUE;
}

template <class T> inline boolean Slist<T>::insert(const T *data, int len,
						   int pos) {
  ASSERT(data != NULL);
  SlistNode<T> *node = NULL, *newfirst = NULL;
  int i;

  if(pos == 0) {
    for(i = 0; i < len; i++) {
      if(newfirst == NULL) {
	newfirst = node = new SlistNode<T>();
	_size++;
	newfirst->_data = data[i];
      } else {
	node->_next = new SlistNode<T>();
	_size++;
	node = node->_next;
	node->_data = data[i];
      }
    }

    if(_last == NULL) {
      node->_next = NULL;
      _first = newfirst;
      _last = node;
    } else {
      node->_next = _first;
      _first = newfirst;
    }
  } else {
    SlistNode<T> *current, *previous;
    current = previous = _first;
    
    for(i = 0; i < pos; i++) {
      if(current == NULL) {
	throw SlistException("pos out of bounds");
      }
      previous = current;
      current = current->_next;
    }
    
    for(i = 0; i < len; i++) {
      if(newfirst == NULL) {
	newfirst = node = new SlistNode<T>();
	_size++;
	newfirst->_data = data[i];
      } else {
	node->_next = new SlistNode<T>();
	_size++;
	node = node->_next;
	node->_data = data[i];
      }
    }
    
    node->_next = current;
    previous->_next = newfirst;
  }

  return TRUE;
}

template <class T> inline boolean Slist<T>::insert(Slist<T> &data, int pos) {
  SlistNode<T> *scan, *node = NULL, *newfirst = NULL;
  int i;

  if(pos == 0) {
    for(scan = list._first; scan != NULL; scan = scan->_next) {
      if(newfirst == NULL) {
	newfirst = node = new SlistNode<T>();
	_size++;
	newfirst->_data = scan->_data;
      } else {
	node->_next = new SlistNode<T>();
	_size++;
	node = node->_next;
	node->_data = scan->_data;
      }
    }

    if(_last == NULL) {
      node->_next = NULL;
      _first = newfirst;
      _last = node;
    } else {
      node->_next = _first;
      _first = newfirst;
    }
  } else {
    SlistNode<T> *current, *previous;
    current = previous = _first;
    
    for(i = 0; i < pos; i++) {
      if(current == NULL) {
	throw SlistException("pos out of bounds");
      }
      previous = current;
      current = current->_next;
    }
    
    for(scan = list._first; scan != NULL; scan = scan->_next) {
      if(newfirst == NULL) {
	newfirst = node = new SlistNode<T>();
	_size++;
	newfirst->_data = scan->_data;
      } else {
	node->_next = new SlistNode<T>();
	_size++;
	node = node->_next;
	node->_data = scan->_data;
      }
    }
    
    node->_next = current;
    previous->_next = newfirst;
  }

  return TRUE;
}

template <class T> inline boolean Slist<T>::remove(int pos) {
  return remove(pos, pos);
}

template <class T> inline boolean Slist<T>::remove(int pos1, int pos2) {
  if(pos1 > pos2) {
    throw SlistException("pos out of bounds");
  }

  SlistNode<T> *preRemove, *endRemove, *tmp;
  int i;
  preRemove = _first;

  if(pos1 > 0) {
    for(i = 0; i < pos1 - 1; i++) {
      if(preRemove == NULL) {
	throw SlistException("pos out of bounds");
      }
      preRemove = preRemove->_next;
    }
    endRemove = preRemove->_next;
  } else {
    endRemove = _first;
  }
    
  for(i = pos1; i <= pos2; i++) {
    if(endRemove == NULL) {
      throw SlistException("pos out of bounds");
    }

    if(_last == endRemove) {
      _last = preRemove;
    }

    tmp = endRemove->_next;
    delete endRemove;
    _size--;
    endRemove = tmp;
  }

  if(pos1 == 0) {
    _first = endRemove;
  } else {
    preRemove->_next = endRemove;
  }

  return TRUE;
}

template <class T> inline boolean Slist<T>::removeElement(const T &datum,
							  int occurence) {
  SlistNode<T> *node = _first, *prev = _first;
  for(int o = 0; node != NULL; node = node->_next) {
    if(node->_data == datum && ++o >= occurence) {
      if(_first == _last && node == _first) {
	_first = _last = NULL;
      } else if(_first == node) {
	_first = node->_next;
      } else if(_last == node) {
	prev->_next = NULL;
	_last = prev;
      } else {
	prev->_next = node->_next;
      }

      delete node;
      _size--;
      return TRUE;
    }

    prev = node;
  }

  return FALSE;
}

template <class T> inline boolean Slist<T>::removeFirst() {
  if(_first == NULL) {
    return FALSE;
  }

  if(_first == _last) {
    delete _first;
    _size--;
    _first = _last = NULL;
  } else {
    SlistNode<T> *node = _first;
    _first = _first->_next;
    delete node;
    _size--;
  }

  return TRUE;
}

template <class T> inline boolean Slist<T>::removeLast() {
  if(_last == NULL) {
    return FALSE;
  }

  if(_first == _last) {
    delete _last;
    _size--;
    _first = _last = NULL;
  } else {
    SlistNode<T> *node = _first;
    while(node->_next != _last) {
      node = node->_next;
    }
    node->_next = NULL;
    delete _last;
    _size--;
    _last = node;
  }

  return TRUE;
}

template <class T> inline const T &Slist<T>::get(int pos) const {
  SlistNode<T> *node = _first;
  for(int i = 0; i < pos; i++) {
    if(node == NULL) {
      throw SlistException("pos out of bounds");
    }
    node = node->_next;
  }

  if(node == NULL) {
    throw SlistException("pos out of bounds");
  }

  return node->_data;
}

template <class T> inline T &Slist<T>::get(int pos) {
  SlistNode<T> *node = _first;
  for(int i = 0; i < pos; i++) {
    if(node == NULL) {
      throw SlistException("pos out of bounds");
    }
    node = node->_next;
  }

  if(node == NULL) {
    throw SlistException("pos out of bounds");
  }

  return node->_data;
}

template <class T> inline void Slist<T>::set(const T &datum, int pos) {
  SlistNode<T> *node = _first;
  for(int i = 0; i < pos; i++) {
    if(node == NULL) {
      throw SlistException("pos out of bounds");
    }

    node = node->_next;
  }

  if(node == NULL) {
    throw SlistException("pos out of bounds");
  }

  node->_data = datum;
}

template <class T> inline int Slist<T>::firstIndexOf(const T &datum) const {
  SlistNode<T> *node = _first;
  int index = 0;
  for(index = 0; node != NULL; index++) {
    if(node->_data == datum) {
      return index;
    }

    node = node->_next;
  }

  return -1;
}

template <class T> inline int Slist<T>::lastIndexOf(const T &datum) const {
  for(int index = size() - 1; index >= 0; index--) {
    SlistNode<T> *node = _first;
    for(int i = 0; i < index && node != NULL; i++) {
      node = node->_next;
    }

    if(node != NULL && node->_data == datum) {
      return index;
    }
  }
  
  return -1;
}

template <class T> inline int Slist<T>::size() const {
  return _size;

  /*
    int len = 0;
  
    for(SlistNode<T> *node = _first; node != NULL; node = node->_next) {
    len++;
    }

    return len; 
  */
}

template <class T> inline boolean Slist<T>::isEmpty() const {
  return _first == NULL;
}

template <class T> inline ListIterator<T> *Slist<T>::getIterator() const {
  return new SlistIterator<T>(*this);
}

template <class T> inline void Slist<T>::clear() {
  while(_first != NULL) {
    _last = _first;
    _first = _first->_next;
    delete _last;
    _size--;
  }
  _last = NULL;
}

template <class T> inline SlistIterator<T>::SlistIterator() {
  _list = NULL;
  _current = NULL;
  _next = NULL;
}

template <class T> inline SlistIterator<T>::SlistIterator(const Slist<T> &list) {
  _list = &list;
  _current = NULL;
  _next = _list->_first;
}

template <class T> inline SlistIterator<T> &SlistIterator<T>::operator=(const Slist<T> &list) {
  _list = &list;
  _current = NULL;
  _next = _list->_first;
  return *this;
}

template <class T> inline boolean SlistIterator<T>::isNotNull() const {
  return _current != NULL;
}

template <class T> inline boolean SlistIterator<T>::isNextNotNull() const {
  return _next != NULL;
}

template <class T> inline const T *SlistIterator<T>::first() {
  _current = _list->_first;
  _next = (_current == NULL) ? NULL : _current->_next;
  return (_current == NULL) ? NULL : &_current->_data;
}

template <class T> inline const T *SlistIterator<T>::current() {
  return (_current == NULL) ? NULL : &_current->_data;
}

template <class T> inline const T *SlistIterator<T>::next() {
  _current = _next;
  if(_next != NULL) {
    _next = _next->_next;
  }
  return (_current == NULL) ? NULL : &_current->_data;
}

template <class T> inline const T *SlistIterator<T>::last() {
  _current = _list->_last;
  _next = NULL;

  return (_current == NULL) ? NULL : &_current->_data;
}

inline SlistException::SlistException() : Exception() {

}

inline SlistException::SlistException(const char *msg) : Exception(msg) {

}

__END_NAMESPACE(SELFSOFT);

#endif
