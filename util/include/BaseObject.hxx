/*************************************************************************
 * $Id: BaseObject.hxx,v 1.9 2001/08/21 05:14:24 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/21 05:14:24 $
 * $Locker:  $
 * $Revision: 1.9 $
 *
 * Created:  9/4/99 by Charles Chiou
 * Comments: This is the base object that all other SELFSOFT class objects
 *           inherit from.
 *
 * NOTES:
 *           Comparator functions:
 *           The two functions operator== and operator< are the basic
 *           comparator operations that should be overriden by deriving
 *           classes. The rest of the the comparison operators
 *           { operator!=, operator>=, operator<=, operator> } are inlined
 *           functions that uses the logic provided by the basic comparators
 *           mentioned above.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _BASEOBJECT_HXX_
#define _BASEOBJECT_HXX_

#include "utilapi.h"
#include "PtrReference.hxx"
#include "BaseRuntimeClass.hxx"
#include "ThreadException.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class Monitor;

class UTILAPI BaseObject : public PtrReference {

public:

    BaseObject();
    BaseObject(const BaseObject &object);
    virtual ~BaseObject();

    // Object comparison operators
    virtual boolean operator==(const BaseObject &object) const;
    virtual boolean operator!=(const BaseObject &object) const;
    virtual boolean operator>=(const BaseObject &object) const;
    virtual boolean operator<=(const BaseObject &object) const;
    virtual boolean operator>(const BaseObject &object) const;
    virtual boolean operator<(const BaseObject &object) const;
  
    // Class identifications and representations
    virtual operator const char *() const;
    virtual const char *toString() const;
    virtual const BaseRuntimeClass *getRuntimeClass() const;
    boolean isKindOf(const BaseRuntimeClass *rclass) const;
    boolean isKindOf(const char *className) const;
    boolean isInstanceOf(const BaseRuntimeClass *rclass) const;
    boolean isInstanceOf(const char *className) const;
    boolean isSerializable(const BaseRuntimeClass *rclass) const;

    // Hashing function for the object
    virtual unsigned long hashCode() const;

    // Thread synchronization
    void wait() throw(InterruptedException);
    void wait(long timeout) throw(InterruptedException);
    void notify() throw(IllegalMonitorStateException);
    void notifyAll() throw(IllegalMonitorStateException);

    // Object cloning
    virtual BaseObject *clone() const;

protected:

    static const BaseRuntimeClass *_getParentClass();
    static const BaseRuntimeClass *CS_classBaseObject;

private:

    void ensureMonitorInit();

private:

    friend class Synchronized;

    Monitor *_monitor;

};

// Inline functions below

inline boolean BaseObject::operator!=(const BaseObject &object) const {
    return !(*this == object);
}

inline boolean BaseObject::operator>=(const BaseObject &object) const {
    return !(*this < object);
}

inline boolean BaseObject::operator<=(const BaseObject &object) const {
    return (*this == object || *this < object);
}

inline boolean BaseObject::operator>(const BaseObject &object) const {
    return !(*this <= object);
}

inline BaseObject::operator const char *() const {
    return toString();
}

inline unsigned long BaseObject::hashCode() const {
    return (unsigned long) (void *) this;
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
