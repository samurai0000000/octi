/*************************************************************************
 * $Id: PtrReference.hxx,v 1.7 2001/08/24 03:02:37 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:02:37 $
 * $Locker:  $
 * $Revision: 1.7 $
 *
 * Created:  1/19/99 by Charles Chiou
 * Comments:
 *     Pointer and reference counter classes.
 *
 *     In usage: Ptr<T>, T must be derived from PtrReference.
 *     The PtrReference class is a reference counter that by default destroys
 *     "this" pointer when there is zero references held on this object.
 *     The PtrReference class is useful when the increaseReference() and
 *     decreaseReference() are used accordingingly to keep track of other
 *     variables that hold a reference on this object.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _PTRREFERENCE_HXX_
#define _PTRREFERENCE_HXX_

#include "utilapi.h"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI PtrReference : public Root {

public:

    PtrReference();
    void increaseReference() const;
    void decreaseReference() const;

protected:

    virtual ~PtrReference();

    int numReferences() const;

    // Implement the zeroReferences method to override the default
    // behaviour ("delete this").
    virtual void zeroReferences();

private:

    int _numReferences;

};

// The Ptr class is useful for making sure that there is no memory leak
// on locally allocated data structures.

template <class T>
class UTILAPI Ptr : public Root {

public:

    Ptr();
    Ptr(const T *ptr);
    Ptr(const Ptr<T> &ptr);
    ~Ptr();

    Ptr<T> &operator=(const T *ptr);
    Ptr<T> &operator=(const Ptr<T> &ptr);
    const T *operator->() const;
    T *operator->();
    const T *ptr() const;
    T *ptr();

    operator const T *() const;
    operator T *();

private:

    const T *_ptr;

};

// Inline functions for PtrReference class are below here.

inline PtrReference::PtrReference() {
    _numReferences = 0;
}

inline PtrReference::~PtrReference() {

}

inline void PtrReference::increaseReference() const {
    PtrReference *self = (PtrReference *) this;
    self->_numReferences++;
}

inline void PtrReference::decreaseReference() const {
    PtrReference *self = (PtrReference *) this;
    if(--self->_numReferences == 0) {
        self->zeroReferences();
    }
}

inline int PtrReference::numReferences() const {
    return _numReferences;
}

inline void PtrReference::zeroReferences() {
    delete this;
}

// Inline functions for Ptr class are below here.

template <class T> inline Ptr<T>::Ptr() {
    _ptr = NULL;
}

template <class T> inline Ptr<T>::Ptr(const T *ptr) {
    if((_ptr = ptr) != NULL) {
        ptr->increaseReference();
    }
}

template <class T> inline Ptr<T>::Ptr(const Ptr<T> &ptr) {
    if((_ptr = ptr._ptr) != NULL) {
        ptr->increaseReference();
    }
}

template <class T> inline Ptr<T>::~Ptr() {
    if(_ptr != NULL) {
        _ptr->decreaseReference();
    }
}

template <class T> inline Ptr<T> &Ptr<T>::operator=(const T *ptr) {
    const PtrReference *old = _ptr;
    _ptr = ptr;
    if(_ptr != old) {
        if(_ptr != NULL) {
            _ptr->increaseReference();
        }
        if(old != NULL){
            old->decreaseReference();
        }
    }
    return *this;
}

template <class T> inline Ptr<T> &Ptr<T>::operator=(const Ptr<T> &ptr) {
    const PtrReference *old = _ptr;
    _ptr = ptr._ptr;
    if(_ptr != old) {
        if(_ptr != NULL) {
            _ptr->increaseReference();
        }
        if(old != NULL){
            old->decreaseReference();
        }
    }
    return *this;
}

template <class T> inline const T *Ptr<T>::operator->() const {
    return _ptr;
}

template <class T> inline T *Ptr<T>::operator->() {
    return (T *) _ptr;
}

template <class T> inline const T *Ptr<T>::ptr() const {
    return _ptr;
}

template <class T> inline T *Ptr<T>::ptr() {
    return (T *) _ptr;
}

template <class T> inline Ptr<T>::operator const T *() const {
    return _ptr;
}

template <class T> inline Ptr<T>::operator T *() {
    return (T *) _ptr;
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
