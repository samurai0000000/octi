/*************************************************************************
 * $Id: HashTable.hxx,v 1.8 2001/08/09 03:11:08 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 03:11:08 $
 * $Locker:  $
 * $Revision: 1.8 $
 *
 * Created: 1/30/99 by Charles Chiou
 * Comments: Hashtable implemented using chainining.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _HASHTABLE_HXX_
#define _HASHTABLE_HXX_

#include "utilapi.h"
#include "debug.h"
#include "BaseObject.hxx"
#include "String.hxx"
#include "Slist.hxx"
#include "Pair.hxx"

#define DEFAULT_BUCKET_SIZE 97  // A prime number

__BEGIN_NAMESPACE(SELFSOFT);

/************************************************************************/
/* Here are some predefined hash functions.                             */
/************************************************************************/

extern UTILAPI unsigned long hash_int16(const int16 &data);
extern UTILAPI unsigned long hash_word16(const word16 &data);
extern UTILAPI unsigned long hash_int32(const int32 &data);
extern UTILAPI unsigned long hash_word32(const word32 &data);
#ifdef __HAS_WORD64__
extern UTILAPI unsigned long hash_int64(const int64 &data);
extern UTILAPI unsigned long hash_word64(const word64 &data);
#endif
extern UTILAPI unsigned long hash_string(const char *string);
extern UTILAPI unsigned long hashString(const String &string);

template <class Key, class Value> class HashTable;
template <class Key, class Value> class HashTableKeyIterator;
template <class Key, class Value> class HashTableValueIterator;

template <class Key, class Value>
class HashElement : public Pair<Key, Value> {

public:

    HashElement(const Key &key, const Value &value);

    const Key &getKey() const;
    const Value &getValue() const;

    boolean hasSameKey(const Key &key) const;
    boolean hasSameValue(const Value &value) const;

    Key _key;
    Value _value;

};


extern UTILAPI const unsigned long G_HashTablePrimeList[];
extern UTILAPI const unsigned long G_HashTablePrimeListLength;


/**
 * The HashTable class uses pre-allocated buckets to store hashed data and
 * uses linear-chaining when there are collisions.
 *
 * Bucket[0]
 * Bucket[1] -> <Key, Value> -> <Key, Value> -> ... -> <Key, Value>
 * ...
 *
 * Bucket[n] -> <Key, Value> -> <Key, Value> -> ... -> <Key, Value>
 *
 * The setBucketSize() method allows the bucket size to be changed. As a
 * result of this call, all items are rehashed.
 *
 */
template <class Key, class Value>
class HashTable : public AbstractMap<Key, Value> {

public:

    HashTable(unsigned long (*hashfunc)(const Key &key),
              unsigned long bucketSize = DEFAULT_BUCKET_SIZE);
    ~HashTable();

    void setBucketSize(unsigned long bukcetSize = DEFAULT_BUCKET_SIZE);
    unsigned long getBucketSize() const;

    boolean put(const Key &key, const Value &value);
    const Value *get(const Key &key) const;
    boolean remove(const Key &key);

    boolean containsKey(const Key &key) const;
    boolean containsValue(const Value &value) const;

    MapKeyIterator<Key, Value> *getKeyIterator() const;
    MapValueIterator<Key, Value> *getValueIterator() const;

    unsigned long size() const;

    void clear();

protected:

    friend class HashTableKeyIterator<Key, Value>;
    friend class HashTableValueIterator<Key, Value>;

    typedef HashElement<Key, Value> HashElementType;
    typedef Slist<Ptr<HashElementType> > HashTableListType;

    unsigned long (*_hashfunc)(const Key &);  // The hash function
    HashTableListType *_bucket;               // The buckets
    HashTableListType _seqList;               // Sequential list for iteration
    unsigned long _bucketSize;  // The bucket size
    unsigned long _size;        // The number of entries

};


template <class Key, class Value>
class HashTableKeyIterator : public MapKeyIterator<Key, Value> {

public:

    HashTableKeyIterator();
    HashTableKeyIterator(const HashTable<Key, Value> &hashTable);

    HashTableKeyIterator<Key, Value> &operator=(const HashTable<Key, Value> &hashTable);

    boolean isNotNull() const;
    boolean isNextNotNull() const;

    const Key *first();
    const Key *current();
    const Key *next();
    const Key *last();

private:

    Ptr<SlistIterator<Ptr< HashElement<Key, Value> > > > it;

};


template <class Key, class Value>
class HashTableValueIterator : public MapValueIterator<Key, Value> {

public:

    HashTableValueIterator();
    HashTableValueIterator(const HashTable<Key, Value> &hashTable);

    HashTableValueIterator<Key, Value> &operator=(const HashTable<Key, Value> &hashTable);

    boolean isNotNull() const;
    boolean isNextNotNull() const;

    const Value *first();
    const Value *current();
    const Value *next();
    const Value *last();

private:

    Ptr<SlistIterator<Ptr< HashElement<Key, Value> > > > it;

};


// Inline functions

template <class Key, class Value>
inline HashElement<Key, Value>::HashElement(const Key &key, const Value &value) {
    _key = key;
    _value = value;
}

template <class Key, class Value>
inline const Key &HashElement<Key, Value>::getKey() const {
    return _key;
}

template <class Key, class Value>
inline const Value &HashElement<Key, Value>::getValue() const {
    return _value;
}

template <class Key, class Value>
inline boolean HashElement<Key, Value>::hasSameKey(const Key &key) const {
    return _key == key;
}

template <class Key, class Value>
inline boolean HashElement<Key, Value>::hasSameValue(const Value &value) const {

    return _value == value;
}

template <class Key, class Value>
inline HashTable<Key, Value>::HashTable(unsigned long (*hashfunc)(const Key &key),
                                        unsigned long bucketSize) {
    ASSERT(hashfunc != NULL);
    ASSERT(bucketSize > 0);

    _hashfunc = hashfunc;
    _bucket = NULL;
    setBucketSize(bucketSize);
}

template <class Key, class Value>
inline HashTable<Key, Value>::~HashTable() {
    if(_bucket != NULL) {
        delete [] _bucket;
    }
}

template <class Key, class Value>
inline void HashTable<Key, Value>::setBucketSize(unsigned long bucketSize) {
    // Find the closest bucket size in the prime number list
    unsigned int p;
    for(p = 0; p < G_HashTablePrimeListLength && G_HashTablePrimeList[p] < bucketSize; p++);
    bucketSize = G_HashTablePrimeList[p];

    if(_bucketSize != bucketSize) {
        _bucketSize = bucketSize;
        HashTableListType *newBucket =
            new Slist<Ptr<HashElementType> >[_bucketSize];

        if(_bucket == NULL) {
            _bucket = newBucket;
        } else {
            // Rehash to the new bucket
            Ptr<SlistIterator<Ptr<HashElementType> > > it =
                (SlistIterator<Ptr<HashElementType> > *) _seqList.getIterator();
            while(it->isNextNotNull()) {
                Ptr<HashElementType> node = *(Ptr<HashElementType> *) it->next();
                unsigned long bucketLoc = _hashfunc(node->getKey()) % _bucketSize;
                HashTableListType *list = &newBucket[bucketLoc];
                ASSERT(list != NULL);

                list->append(node);                   // Chain to the bucket list
            }

            delete [] _bucket;
            _bucket = newBucket;
        }
    }
}

template <class Key, class Value>
inline boolean HashTable<Key, Value>::put(const Key &key, const Value &value) {
    unsigned long bucketLoc = _hashfunc(key) % _bucketSize;
    HashTableListType *list = &_bucket[bucketLoc];
    ASSERT(list != NULL);

    // Search to to see if the key exists.
    Ptr<SlistIterator<Ptr<HashElementType> > > it =
        (SlistIterator<Ptr<HashElementType> > *) list->getIterator();
    while(it->isNextNotNull()) {
        Ptr<HashElementType> node = *(Ptr<HashElementType> *) it->next();

        if(node->hasSameKey(key)) {
            // The key already exists
            // Replace with the new value
            node->_value = value;
            return TRUE;
        }
    }

    Ptr<HashElementType> newNode = new HashElementType(key, value);
    list->append(newNode);      // Chain to the bucket list
    _seqList.append(newNode);  // Add to the sequential list
    _size++;

    return FALSE;
}

template <class Key, class Value>
inline const Value *HashTable<Key, Value>::get(const Key &key) const {
    unsigned long bucketLoc = _hashfunc(key) % _bucketSize;
    HashTableListType *list = &_bucket[bucketLoc];
    ASSERT(list != NULL);

    // Search to to see if the key exists.
    Ptr<SlistIterator<Ptr<HashElementType> > > it =
        (SlistIterator<Ptr<HashElementType> > *) list->getIterator();
    while(it->isNextNotNull()) {
        Ptr<HashElementType> node = *(Ptr<HashElementType> *) it->next();

        if(node->hasSameKey(key)) {
            return &node->_value;
        }
    }

    return NULL;
}

template <class Key, class Value>
inline boolean HashTable<Key, Value>::remove(const Key &key) {
    unsigned long bucketLoc = _hashfunc(key) %_bucketSize;
    HashTableListType *list = &_bucket[bucketLoc];
    ASSERT(list != NULL);

    // Search to to see if the key exists.
    Ptr<SlistIterator<Ptr<HashElementType> > > it =
        (SlistIterator<Ptr<HashElementType> > *) list->getIterator();
    while(it->isNextNotNull()) {
        Ptr<HashElementType> node = *(Ptr<HashElementType> *) it->next();

        if(node->hasSameKey(key)) {
            boolean rc;
            rc = list->removeElement(node);
            ASSERT(rc == TRUE);
            rc = _seqList.removeElement(node);
            ASSERT(rc == TRUE);
            _size--;
            return TRUE;
        }
    }

    return FALSE;
}

template <class Key, class Value>
inline boolean HashTable<Key, Value>::containsKey(const Key &key) const {
    unsigned long bucketLoc = _hashfunc(key) %_bucketSize;
    HashTableListType *list = &_bucket[bucketLoc];
    ASSERT(list != NULL);

    // Search to to see if the key exists.
    Ptr<SlistIterator<Ptr<HashElementType> > > it =
        (SlistIterator<Ptr<HashElementType> > *) list->getIterator();
    while(it->isNextNotNull()) {
        Ptr<HashElementType> node = *(Ptr<HashElementType> *) it->next();

        if(node->hasSameKey(key)) {
            return TRUE;
        }
    }

    return FALSE;
}

template <class Key, class Value>
inline boolean HashTable<Key, Value>::containsValue(const Value &value) const {
    Ptr<SlistIterator<Ptr<HashElementType> > > it =
        (SlistIterator<Ptr<HashElementType> > *) _seqList.getIterator();
    while(it->isNextNotNull()) {
        Ptr<HashElementType> node = *(Ptr<HashElementType> *) it->next();

        if(node->hasSameValue(value)) {
            return TRUE;
        }
    }

    return FALSE;
}

template <class Key, class Value>
inline void HashTable<Key, Value>::clear() {
    for(unsigned int i = 0; i < _bucketSize; i++) {
        _bucket[i].clear();
    }

    _seqList.clear();
    _size = 0;
}

template <class Key, class Value>
inline unsigned long HashTable<Key, Value>::getBucketSize() const {
    return _bucketSize;
}

template <class Key, class Value>
inline unsigned long HashTable<Key, Value>::size() const {
    return _size;
};

template <class Key, class Value>
inline MapKeyIterator<Key, Value> *HashTable<Key, Value>::getKeyIterator() const {
    return new HashTableKeyIterator<Key, Value>(*this);
}

template <class Key, class Value>
inline MapValueIterator<Key, Value> *HashTable<Key, Value>::getValueIterator() const {
    return new HashTableValueIterator<Key, Value>(*this);
}

template <class Key, class Value>
inline HashTableKeyIterator<Key, Value>::HashTableKeyIterator() {
    it = NULL;
}

template <class Key, class Value>
inline HashTableKeyIterator<Key, Value>::HashTableKeyIterator(const HashTable<Key, Value> &hashTable) {
    it = (SlistIterator<Ptr< HashElement<Key, Value> > > *) hashTable._seqList.getIterator();
}

template <class Key, class Value>
inline HashTableKeyIterator<Key, Value> &HashTableKeyIterator<Key, Value>::operator=(const HashTable<Key, Value> &hashTable) {
    it = (SlistIterator<Ptr< HashElement<Key, Value> > > *) hashTable._seqList.getIterator();
    return *this;
}

template <class Key, class Value>
inline boolean HashTableKeyIterator<Key, Value>::isNotNull() const {
    return it->isNotNull();
}

template <class Key, class Value>
inline boolean HashTableKeyIterator<Key, Value>::isNextNotNull() const {
    return it->isNextNotNull();
}

template <class Key, class Value>
inline const Key *HashTableKeyIterator<Key, Value>::first() {
    return &(*(Ptr< HashElement<Key, Value> > *) it->first())->getKey();
}

template <class Key, class Value>
inline const Key *HashTableKeyIterator<Key, Value>::current() {
    return &(*(Ptr< HashElement<Key, Value> > *) it->current())->getKey();
}

template <class Key, class Value>
inline const Key *HashTableKeyIterator<Key, Value>::next() {
    return &(*(Ptr< HashElement<Key, Value> > *) it->next())->getKey();
}

template <class Key, class Value>
inline const Key *HashTableKeyIterator<Key, Value>::last() {
    return &(*(Ptr< HashElement<Key, Value> > *) it->last())->getKey();
}

template <class Key, class Value>
inline HashTableValueIterator<Key, Value>::HashTableValueIterator() {
    it = NULL;
}

template <class Key, class Value>
inline HashTableValueIterator<Key, Value>::HashTableValueIterator(const HashTable<Key, Value> &hashTable) {
    it = (SlistIterator<Ptr< HashElement<Key, Value> > > *) hashTable._seqList.getIterator();
}

template <class Key, class Value>
HashTableValueIterator<Key, Value> &HashTableValueIterator<Key, Value>::operator=(const HashTable<Key, Value> &hashTable) {
    it = (SlistIterator<Ptr< HashElement<Key, Value> > > *) it._seqList.getIterator();
    return *this;
}

template <class Key, class Value>
inline boolean HashTableValueIterator<Key, Value>::isNotNull() const {
    return it->isNotNull();
}

template <class Key, class Value>
inline boolean HashTableValueIterator<Key, Value>::isNextNotNull() const {
    return it->isNextNotNull();
}

template <class Key, class Value>
inline const Value *HashTableValueIterator<Key, Value>::first() {
    return &(*(Ptr< HashElement<Key, Value> > *) it->first())->getValue();
}

template <class Key, class Value>
inline const Value *HashTableValueIterator<Key, Value>::current() {
    return &(*(Ptr< HashElement<Key, Value> > *) it->current())->getValue();
}

template <class Key, class Value>
inline const Value *HashTableValueIterator<Key, Value>::next() {
    return &(*(Ptr< HashElement<Key, Value> > *) it->next())->getValue();
}

template <class Key, class Value>
inline const Value *HashTableValueIterator<Key, Value>::last() {
    return &(*(Ptr< HashElement<Key, Value> > *) it->last())->getValue();
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
