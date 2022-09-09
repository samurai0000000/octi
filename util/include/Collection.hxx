/*************************************************************************
 * $Id: Collection.hxx,v 1.4 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created:  9/11/99 by Charles Chiou
 * Comments: Collection Interface Classes.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _COLLECTION_HXX_
#define _COLLECTION_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

/**************************************************************************/
/* Iterators                                                              */
/**************************************************************************/

/**
 * The Iterator is the base interface class to be used in iterating through
 * data in different collections.
 */
template <class T>
class Iterator : public BaseObject {

public:

    virtual boolean isNotNull() const = 0;
    virtual boolean isNextNotNull() const = 0;

    virtual const T *first() = 0;
    virtual const T *current() = 0;
    virtual const T *next() = 0;

};

/**
 * The ListIterator is used to iterate through elements in a list.
 */
template <class T>
class ListIterator : public Iterator<T> {

public:

    virtual boolean isNotNull() const = 0;
    virtual boolean isNextNotNull() const = 0;

    virtual const T *first() = 0;
    virtual const T *current() = 0;
    virtual const T *next() = 0;
    virtual const T *last() = 0;

};

/**
 * The MapKeyIterator is used to iterate through all keys in a map.
 */
template <class Key, class Value>
class MapKeyIterator : public Iterator<Key> {

public:

    virtual boolean isNotNull() const = 0;
    virtual boolean isNextNotNull() const = 0;

    virtual const Key *first() = 0;
    virtual const Key *current() = 0;
    virtual const Key *next() = 0;
    virtual const Key *last() = 0;

};

/**
 * The MapKeyIterator is used to iterate through all values in a map.
 */
template <class Key, class Value>
class MapValueIterator : public Iterator<Value> {

public:

    virtual boolean isNotNull() const = 0;
    virtual boolean isNextNotNull() const = 0;

    virtual const Value *first() = 0;
    virtual const Value *current() = 0;
    virtual const Value *next() = 0;
    virtual const Value *last() = 0;

};

/**************************************************************************/
/* Interface classes                                                      */
/**************************************************************************/

/**
 * A list is a sequential collection of data.
 */
template <class T>
class AbstractList : public BaseObject {

public:

    // Adding new elements to list
    virtual boolean append(const T &datum) = 0;
    virtual boolean append(const T *data, int len);
    virtual boolean append(AbstractList<T> &data);
    virtual boolean prepend(const T &datum) = 0;
    virtual boolean prepend(const T *data, int len);
    virtual boolean prepend(AbstractList<T> &data);
    virtual boolean insert(const T &datum, int pos) = 0;
    virtual boolean insert(const T *data, int len, int pos);
    virtual boolean insert(AbstractList<T> &data, int pos);

    // Removing elements from list
    virtual boolean remove(int pos);
    virtual boolean remove(int pos1, int pos2) = 0;
    virtual boolean removeElement(const T &datum, int occurence = 1) = 0;
    virtual boolean removeFirst();
    virtual boolean removeLast();

    // Accessor functions
    virtual const T &get(int pos) const = 0;
    virtual T &get(int pos) = 0;
    virtual const T &operator[](int pos) const;
    virtual T &operator[](int pos);
    virtual void set(const T &datum, int pos) = 0;

    // Queries
    virtual int firstIndexOf(const T &datum) const = 0;
    virtual int lastIndexOf(const T &datum) const = 0;
    virtual int size() const = 0;

    virtual ListIterator<T> *getIterator() const = 0;
    virtual void clear() = 0;

};

/**
 * A Map is a collection of <Key, Value> data pairs.
 */
template <class Key, class Value>
class AbstractMap : public BaseObject {

public:

    virtual boolean put(const Key &key, const Value &value) = 0;
    virtual const Value *get(const Key &key) const = 0;
    virtual boolean remove(const Key &key) = 0;

    virtual boolean containsKey(const Key &key) const = 0;
    virtual boolean containsValue(const Value &value) const = 0;

    virtual MapKeyIterator<Key, Value> *getKeyIterator() const = 0;
    virtual MapValueIterator<Key, Value> *getValueIterator() const = 0;

    virtual unsigned long size() const = 0;

    virtual void clear() = 0;

};

// Inline functions below

template <class T> inline
boolean AbstractList<T>::append(const T *data, int len) {
    for(int i = 0; i < len; i++) {
        append(data[i]);
    }

    return TRUE;
}

template <class T> inline
boolean AbstractList<T>::append(AbstractList<T> &data) {
    int dlen = data.size();
    int i;

    if(&data != this) {
        for(i = 0; i < dlen; i++) {
            append(data[i]);
        }
    } else {
        Ptr<ListIterator<T> > iterator = getIterator();
        T *buffer = new T[dlen];
        for(i = 0; iterator->isNextNotNull(); i++) {
            buffer[i] = *iterator->next();
        }
        for(i = 0; i < dlen; i++) {
            append(buffer[i]);
        }
        delete [] buffer;
    }

    return TRUE;
}

template <class T> inline
boolean AbstractList<T>::prepend(const T *data, int len) {
    for(int i = len - 1; i >= 0; i--) {
        prepend(data[i]);
    }

    return TRUE;
}

template <class T> inline
boolean AbstractList<T>::prepend(AbstractList<T> &data) {
    int dlen = data.size();
    int i;
    if(&data != this) {
        for(i = dlen - 1; i >= 0; i--) {
            prepend(data[i]);
        }
    } else {
        Ptr<ListIterator<T> > iterator = getIterator();
        T *buffer = new T[dlen];
        for(i = 0; iterator->isNextNotNull(); i++) {
            buffer[i] = *iterator->next();
        }
        for(i = dlen - 1; i >= 0; i--) {
            prepend(buffer[i]);
        }
        delete [] buffer;
    }

    return TRUE;
}

template <class T> inline
boolean AbstractList<T>::insert(const T *data, int len,
                                int pos) {
    for(int i = len - 1; i >= 0; i--) {
        insert(data[i], pos);
    }

    return TRUE;
}

template <class T> inline
boolean AbstractList<T>::insert(AbstractList<T> &data, int pos) {
    int dlen = data.size();
    int i;
    if(&data != this) {
        for(i = dlen - 1; i >= 0; i--) {
            insert(data[i], pos);
        }
    } else {
        Ptr<ListIterator<T> > iterator = getIterator();
        T *buffer = new T[dlen];
        for(i = 0; iterator->isNextNotNull(); i++) {
            buffer[i] = *iterator->next();
        }
        for(i = dlen - 1; i >= 0; i--) {
            insert(buffer[i], pos);
        }
        delete [] buffer;
    }

    return TRUE;
}

template <class T> inline
boolean AbstractList<T>::remove(int pos) {
    return remove(pos, pos);
}

template <class T> inline
boolean AbstractList<T>::removeFirst() {
    return remove(0);
}

template <class T> inline
boolean AbstractList<T>::removeLast() {
    return remove(size() - 1);
}

template <class T> inline
const T &AbstractList<T>::operator[](int pos) const {
    return get(pos);
}

template <class T> inline
T &AbstractList<T>::operator[](int pos) {
    return get(pos);
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
