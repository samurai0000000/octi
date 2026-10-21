/*************************************************************************
 * $Id: Dlist.hxx,v 1.9 2001/08/09 03:11:08 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 03:11:08 $
 * $Locker:  $
 * $Revision: 1.9 $
 *
 * Created:  10/23/99 by Charles Chiou
 * Comments: A doubly-linked list.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _DLIST_HXX_
#define _DLIST_HXX_

#include "debug.h"
#include "BaseObject.hxx"
#include "String.hxx"
#include "Collection.hxx"

__BEGIN_NAMESPACE(SELFSOFT);


template <class T> class Dlist;
template <class T> class DlistIterator;


class DlistException : public Exception {

    DECLARE_RUNTIME_SERIALIZABLE(DlistException);

public:

    DlistException();
    DlistException(const char *msg);

};

template <class T>
class DlistNode : public BaseObject {

private:

    friend class Dlist<T>;
    friend class DlistIterator<T>;

    T _data;
    DlistNode<T> *_next;
    DlistNode<T> *_prev;

};


template <class T>
class Dlist : public AbstractList<T> {

public:

    Dlist();
    Dlist(const Dlist<T> &list);
    ~Dlist();

    Dlist<T> &operator=(const Dlist<T> &list);

    boolean append(const T &datum);
    boolean append(const T *data, int len);
    boolean append(const Dlist<T> &list);
    boolean prepend(const T &datum);
    boolean prepend(const Dlist<T> &list);
    boolean prepend(const T *data, int len);
    boolean insert(const T &datum, int pos);
    boolean insert(const T *data, int len, int pos);
    boolean insert(Dlist<T> &data, int pos);

    boolean remove(int pos);
    boolean remove(int pos1, int pos2);
    boolean removeElement(const T &datum, int occurence = 1);
    boolean removeFirst();
    boolean removeLast();

    const T &get(int pos) const;
    T &get(int pos);
    void set(const T &datum, int pos);

    int firstIndexOf(const T &datum) const;
    int lastIndexOf(const T &datum) const;
    int size() const;
    boolean isEmpty() const;

    ListIterator<T> *getIterator() const;
    void clear();

private:

    friend class DlistIterator<T>;

    DlistNode<T> *_first, *_last;
    int _size;

};


template <class T>
class DlistIterator : public ListIterator<T> {

public:

    DlistIterator();
    DlistIterator(const Dlist<T> &list);

    DlistIterator<T> &operator=(const Dlist<T> &list);

    boolean isNotNull() const;
    boolean isPrevNotNull() const;
    boolean isNextNotNull() const;

    const T *first();
    const T *current();
    const T *prev();
    const T *next();
    const T *last();

private:

    const Dlist<T> *_list;
    DlistNode<T> *_current, *_prev, *_next;

};


// Inline functions below

template <class T> inline Dlist<T>::Dlist() {
    _first = _last = NULL;
    _size = 0;
}

template <class T> inline Dlist<T>::Dlist(const Dlist<T> &list) {
    _first = _last = NULL;
    _size = 0;
    operator=(list);
}

template <class T> inline Dlist<T>::~Dlist() {
    clear();
}

template <class T> inline Dlist<T> &Dlist<T>::operator=(const Dlist<T> &list) {
    if(&list == this) {
        return *this;
    }

    clear();
    DlistNode<T> *node = list._first;

    if(node != NULL) {
        _first = _last = new DlistNode<T>();
        _size++;
        _last->_data = node->_data;
        _last->_prev = _last->_next = NULL;

        while((node = node->_next) != NULL) {
            _last->_next = new DlistNode<T>();
            _size++;
            _last->_next->_prev = _last;
            _last = _last->_next;
            _last->_data = node->_data;
        }
        _last->_next = NULL;
    }

    return *this;
}

template <class T> inline boolean Dlist<T>::append(const T &datum) {
    DlistNode<T> *node = new DlistNode<T>();
    _size++;
    node->_data = datum;
    node->_prev = node->_next = NULL;

    if(_first == NULL) {
        _first = _last = node;
    } else {
        node->_prev = _last;
        _last->_next = node;
        _last = node;
    }

    return TRUE;
}

template <class T> inline boolean Dlist<T>::append(const T *data, int len) {
    ASSERT(data != NULL);
    DlistNode<T> *node = NULL, *newfirst = NULL;

    for(int i = 0; i < len; i++) {
        if(newfirst == NULL) {
            newfirst = node = new DlistNode<T>();
            _size++;
            newfirst->_data = data[i];
            newfirst->_prev = NULL;
        } else {
            node->_next = new DlistNode<T>();
            _size++;
            node->_next->_prev = node;
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
        newfirst->_prev = _last;
        _last->_next = newfirst;
        _last = node;
    }

    return TRUE;
}

template <class T> inline boolean Dlist<T>::append(const Dlist<T> &list) {
    DlistNode<T> *scan, *node = NULL, *newfirst = NULL;

    for(scan = list._first; scan != NULL; scan = scan->_next) {
        if(newfirst == NULL) {
            newfirst = node = new DlistNode<T>();
            _size++;
            newfirst->_data = scan->_data;
            newfirst->_prev = NULL;
        } else {
            node->_next = new DlistNode<T>();
            _size++;
            node->_next->_prev = node;
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
        newfirst->_prev = _last;
        _last->_next = newfirst;
        _last = node;
    }

    return TRUE;
}

template <class T> inline boolean Dlist<T>::prepend(const T &datum) {
    DlistNode<T> *node = new DlistNode<T>();
    _size++;

    node->_data = datum;
    node->_next = _first;
    node->_prev = NULL;
    if(_first != NULL) {
        _first->_prev = node;
    }

    if(_last == NULL) {
        _first = _last = node;
    } else {
        _first = node;
    }

    return TRUE;
}

template <class T> inline boolean Dlist<T>::prepend(const T *data, int len) {
    ASSERT(data != NULL);
    DlistNode<T> *node = NULL, *newfirst = NULL;

    for(int i = 0; i < len; i++) {
        if(newfirst == NULL) {
            newfirst = node = new DlistNode<T>();
            _size++;
            newfirst->_data = data[i];
            newfirst->_prev = NULL;
        } else {
            node->_next = new DlistNode<T>();
            _size++;
            node->_next->_prev = node;
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
        if(_first != NULL) {
            _first->_prev = node;
        }
        _first = newfirst;
    }

    return TRUE;
}

template <class T> inline boolean Dlist<T>::prepend(const Dlist<T> &list) {
    DlistNode<T> *scan, *node = NULL, *newfirst = NULL;

    for(scan = list._first; scan != NULL; scan = scan->_next) {
        if(newfirst == NULL) {
            newfirst = node = new DlistNode<T>();
            _size++;
            newfirst->_data = scan->_data;
            newfirst->_prev = NULL;
        } else {
            node->_next = new DlistNode<T>();
            _size++;
            node->_next->_prev = node;
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
        if(_first != NULL) {
            _first->_prev = node;
        }
        _first = newfirst;
    }

    return TRUE;
}

template <class T> inline boolean Dlist<T>::insert(const T &datum, int pos) {
    DlistNode<T> *node = NULL;

    if(pos == 0) {
        node = new DlistNode<T>();
        _size++;

        node->_data = datum;
        node->_next = _first;
        node->_prev = NULL;
        _first->_prev = node;

        if(_last == NULL) {
            _first = _last = node;
        } else {
            _first = node;
        }
    } else {
        DlistNode<T> *current;
        int i;
        if(pos < _size / 2) {
            current = _first;
            for(i = 0; i < pos; i++) {
                if(current == NULL) {
                    throw DlistException("pos out of bounds");
                }
                current = current->_next;
            }
        } else {
            current = _last;
            for(i = _size - 1; i > pos; i--) {
                if(current == NULL) {
                    throw DlistException("pos out of bounds");
                }
                current = current->_prev;
            }
        }

        node = new DlistNode<T>();
        _size++;

        node->_data = datum;
        node->_next = current;
        node->_prev = current->_prev;
        current->_prev->_next = node;
        current->_prev = node;
    }

    return TRUE;
}

template <class T> inline boolean Dlist<T>::insert(const T *data, int len,
                                                   int pos) {
    ASSERT(data != NULL);
    DlistNode<T> *node = NULL, *newfirst = NULL;
    int i;

    if(pos == 0) {
        for(i = 0; i < len; i++) {
            if(newfirst == NULL) {
                newfirst = node = new DlistNode<T>();
                _size++;
                newfirst->_data = data[i];
                newfirst->_prev = NULL;
            } else {
                node->_next = new DlistNode<T>();
                _size++;
                node->_next->_prev = node;
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
            _first->_prev = node;
            _first = newfirst;
        }
    } else {
        DlistNode<T> *current;
        if(pos < _size / 2) {
            current = _first;
            for(i = 0; i < pos; i++) {
                if(current == NULL) {
                    throw DlistException("pos out of bounds");
                }
                current = current->_next;
            }
        } else {
            current = _last;
            for(i = _size - 1; i > pos; i--) {
                if(current == NULL) {
                    throw DlistException("pos out of bounds");
                }
                current = current->_prev;
            }
        }

        for(i = 0; i < len; i++) {
            if(newfirst == NULL) {
                newfirst = node = new DlistNode<T>();
                _size++;
                newfirst->_data = data[i];
                newfirst->_prev = NULL;
            } else {
                node->_next = new DlistNode<T>();
                _size++;
                node->_next->_prev = node;
                node = node->_next;
                node->_data = data[i];
            }
        }

        node->_next = current;
        newfirst->_prev = current->_prev;
        current->_prev->_next = newfirst;
        current->_prev = node;
    }

    return TRUE;
}

template <class T> inline boolean Dlist<T>::insert(Dlist<T> &list, int pos) {
    DlistNode<T> *scan, *node = NULL, *newfirst = NULL;
    int i;

    if(pos == 0) {
        for(scan = list._first; scan != NULL; scan = scan->_next) {
            if(newfirst == NULL) {
                newfirst = node = new DlistNode<T>();
                _size++;
                newfirst->_data = scan->_data;
                newfirst->_prev = NULL;
            } else {
                node->_next = new DlistNode<T>();
                _size++;
                node->_next->_prev = node;
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
            _first->_prev = node;
            _first = newfirst;
        }
    } else {
        DlistNode<T> *current;
        if(pos < _size / 2) {
            current = _first;
            for(i = 0; i < pos; i++) {
                if(current == NULL) {
                    throw DlistException("pos out of bounds");
                }
                current = current->_next;
            }
        } else {
            current = _last;
            for(i = _size - 1; i > pos; i--) {
                if(current == NULL) {
                    throw DlistException("pos out of bounds");
                }
                current = current->_prev;
            }
        }

        for(scan = list._first; scan != NULL; scan = scan->_next) {
            if(newfirst == NULL) {
                newfirst = node = new DlistNode<T>();
                _size++;
                newfirst->_data = scan->_data;
                newfirst->_prev = NULL;
            } else {
                node->_next = new DlistNode<T>();
                _size++;
                node->_next->_prev = node;
                node = node->_next;
                node->_data = scan->_data;
            }
        }

        node->_next = current;
        newfirst->_prev = current->_prev;
        current->_prev->_next = newfirst;
        current->_prev = node;
    }

    return TRUE;
}

template <class T> inline boolean Dlist<T>::remove(int pos) {
    return remove(pos, pos);
}

template <class T> inline boolean Dlist<T>::remove(int pos1, int pos2) {
    if(pos1 > pos2) {
        throw DlistException("pos out of bounds");
    }

    DlistNode<T> *preRemove, *endRemove, *tmp;
    int i;
    preRemove = _first;

    if(pos1 > 0) {
        for(i = 0; i < pos1 - 1; i++) {
            if(preRemove == NULL) {
                throw DlistException("pos out of bounds");
            }
            preRemove = preRemove->_next;
        }
        endRemove = preRemove->_next;
    } else {
        endRemove = _first;
    }

    for(i = pos1; i <= pos2; i++) {
        if(endRemove == NULL) {
            throw DlistException("pos out of bounds");
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
        _first->_prev = NULL;
    } else {
        preRemove->_next = endRemove;
        endRemove->_prev = preRemove;
    }

    return TRUE;
}

template <class T> inline boolean Dlist<T>::removeElement(const T &datum,
                                                          int occurence) {
    DlistNode<T> *node = _first;
    for(int o = 0; node != NULL; node = node->_next) {
        if(node->_data == datum && ++o >= occurence) {
            if(_first == _last && node == _first) {
                _first = _last = NULL;
            } else if(_first == node) {
                _first = node->_next;
                node->_prev = NULL;
            } else if(_last == node) {
                _last = node->_prev;
                node->_prev->_next = NULL;
            } else {
                node->_prev->_next = node->_next;
                node->_next->_prev = node->_prev;
            }

            delete node;
            _size--;
            return TRUE;
        }
    }

    return FALSE;
}

template <class T> inline boolean Dlist<T>::removeFirst() {
    if(_first == NULL) {
        return FALSE;
    }

    if(_first == _last) {
        delete _first;
        _size--;
        _first = _last = NULL;
    } else {
        DlistNode<T> *node = _first;
        _first = _first->_next;
        _first->_prev = NULL;
        delete node;
        _size--;
    }

    return TRUE;
}

template <class T> inline boolean Dlist<T>::removeLast() {
    if(_last == NULL) {
        return FALSE;
    }

    if(_first == _last) {
        delete _last;
        _size--;
        _first = _last = NULL;
    } else {
        DlistNode<T> *node = _last->_prev;
        node->_next = NULL;
        delete _last;
        _size--;
        _last = node;
    }

    return TRUE;
}

template <class T> inline const T &Dlist<T>::get(int pos) const {
    DlistNode<T> *node;
    int i;
    if(pos < _size / 2) {
        node = _first;
        for(i = 0; i < pos; i++) {
            if(node == NULL) {
                throw DlistException("pos out of bounds");
            }
            node = node->_next;
        }
    } else {
        node = _last;
        for(i = _size - 1; i > pos; i--) {
            if(node == NULL) {
                throw DlistException("pos out of bounds");
            }
            node = node->_prev;
        }
    }

    if(node == NULL) {
        throw DlistException("pos out of bounds");
    }

    return node->_data;
}

template <class T> inline T &Dlist<T>::get(int pos) {
    DlistNode<T> *node;
    int i;
    if(pos < _size / 2) {
        node = _first;
        for(i = 0; i < pos; i++) {
            if(node == NULL) {
                throw DlistException("pos out of bounds");
            }
            node = node->_next;
        }
    } else {
        node = _last;
        for(i = _size - 1; i > pos; i--) {
            if(node == NULL) {
                throw DlistException("pos out of bounds");
            }
            node = node->_prev;
        }
    }

    if(node == NULL) {
        throw DlistException("pos out of bounds");
    }

    return node->_data;
}

template <class T> inline void Dlist<T>::set(const T &datum, int pos) {
    DlistNode<T> *node;
    int i;
    if(pos < _size / 2) {
        node = _first;
        for(i = 0; i < pos; i++) {
            if(node == NULL) {
                throw DlistException("pos out of bounds");
            }
            node = node->_next;
        }
    } else {
        node = _last;
        for(i = _size - 1; i > pos; i--) {
            if(node == NULL) {
                throw DlistException("pos out of bounds");
            }
            node = node->_prev;
        }
    }

    if(node == NULL) {
        throw DlistException("pos out of bounds");
    }

    node->_data = datum;
}

template <class T> inline int Dlist<T>::firstIndexOf(const T &datum) const {
    DlistNode<T> *node = _first;
    int index = 0;
    for(index = 0; node != NULL; index++) {
        if(node->_data == datum) {
            return index;
        }

        node = node->_next;
    }

    return -1;
}

template <class T> inline int Dlist<T>::lastIndexOf(const T &datum) const {
    DlistNode<T> *node = _last;
    for(int index = size() - 1; index >= 0; index--) {
        if(node != NULL && node->_data == datum) {
            return index;
        }

        node = node->_prev;
    }

    return -1;
}

template <class T> inline int Dlist<T>::size() const {
    return _size;

    /*
      int len = 0;

      for(DlistNode<T> *node = _first; node != NULL; node = node->_next) {
      len++;
      }

      return len;
    */
}

template <class T> inline boolean Dlist<T>::isEmpty() const {
    return _first == NULL;
}

template <class T> inline ListIterator<T> *Dlist<T>::getIterator() const {
    return new DlistIterator<T>(*this);
}

template <class T> inline void Dlist<T>::clear() {
    while(_first != NULL) {
        _last = _first;
        _first = _first->_next;
        delete _last;
        _size--;
    }
    _last = NULL;
}

template <class T> inline DlistIterator<T>::DlistIterator() {
    _list = NULL;
    _current = NULL;
    _prev = NULL;
    _next = NULL;
}

template <class T> inline DlistIterator<T>::DlistIterator(const Dlist<T> &list) {
    _list = &list;
    _current = _prev = NULL;
    _next = _list->_first;
}

template <class T> inline DlistIterator<T> &DlistIterator<T>::operator=(const Dlist<T> &list) {
    _list = &list;
    _current = _prev = NULL;
    _next = _list->_first;
    return *this;
}

template <class T> inline boolean DlistIterator<T>::isNotNull() const {
    return _current != NULL;
}

template <class T> inline boolean DlistIterator<T>::isPrevNotNull() const {
    return _prev != NULL;
}

template <class T> inline boolean DlistIterator<T>::isNextNotNull() const {
    return _next != NULL;
}

template <class T> inline const T *DlistIterator<T>::first() {
    _current = _list->_first;
    _prev = NULL;
    _next = (_current == NULL) ? NULL : _current->_next;
    return (_current == NULL) ? NULL : &_current->_data;
}

template <class T> inline const T *DlistIterator<T>::current() {
    return (_current == NULL) ? NULL : &_current->_data;
}

template <class T> inline const T *DlistIterator<T>::prev() {
    _current = _prev;
    if(_prev != NULL) {
        _next = _prev;
        _prev = _prev->_prev;
    }
    return (_current == NULL) ? NULL : &_current->_data;
}

template <class T> inline const T *DlistIterator<T>::next() {
    _current = _next;
    if(_next != NULL) {
        _prev = _next;
        _next = _next->_next;
    }
    return (_current == NULL) ? NULL : &_current->_data;
}

template <class T> inline const T *DlistIterator<T>::last() {
    _current = _list->_last;
    _prev = (_current  == NULL) ? NULL : _current->_prev;
    _next = NULL;

    return (_current == NULL) ? NULL : &_current->_data;
}

inline DlistException::DlistException() : Exception() {

}

inline DlistException::DlistException(const char *msg) : Exception(msg) {

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
