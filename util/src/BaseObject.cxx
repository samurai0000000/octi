/*************************************************************************
 * $Id: BaseObject.cxx,v 1.9 2001/08/21 05:14:30 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/21 05:14:30 $
 * $Locker:  $
 * $Revision: 1.9 $
 *
 * Created:  9/4/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include "Error.hxx"
#include "Mutex.hxx"
#include "Condition.hxx"
#include "Monitor.hxx"
#include "BaseObject.hxx"
#include "String.hxx"
#include "ThreadException.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

const BaseRuntimeClass *BaseObject::CS_classBaseObject =
    new BaseRuntimeClass("BaseObject", sizeof(class BaseObject), 
                         NULL, NULL);

BaseObject::BaseObject() {
    _monitor = NULL;
}

BaseObject::BaseObject(const BaseObject &object) {
    _monitor = NULL;
}

BaseObject::~BaseObject() {
    if(_monitor) {
        delete _monitor;
    }
}

boolean BaseObject::operator==(const BaseObject &object) const {
    return (this == &object);
}

boolean BaseObject::operator<(const BaseObject &object) const {
    return (this < &object);
}

BaseObject *BaseObject::clone() const {
    Error::warning("clone() not implemented");
    return 0;
}

const char *BaseObject::toString() const {
    String s;
    s.format("(%s *) 0x%08x", this->getRuntimeClass()->getClassName(), (void *) this);
    return s;
}

const BaseRuntimeClass *BaseObject::getRuntimeClass() const {
    return BASERUNTIME_CLASS(BaseObject);
}

const BaseRuntimeClass *BaseObject::_getParentClass() {
    return NULL;
}

boolean BaseObject::isKindOf(const BaseRuntimeClass *rclass) const {
    if(rclass == NULL) {
        return FALSE;
    }

    for(const BaseRuntimeClass *target = getRuntimeClass(); target;
        target = target->getParentClass()) {
        if(target == rclass) {
            return TRUE;
        }
    }

    return FALSE;
}

boolean BaseObject::isKindOf(const char *className) const {
    if(className == NULL) {
        return FALSE;
    }

    for(const BaseRuntimeClass *target = getRuntimeClass(); target;
        target = target->getParentClass()) {
        if(strcmp(target->getClassName(), className) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

boolean BaseObject::isInstanceOf(const BaseRuntimeClass *rclass) const {
    return rclass != NULL && getRuntimeClass() == rclass;
}

boolean BaseObject::isInstanceOf(const char *className) const {
    if(className == NULL) {
        return FALSE;
    }

    return (strcmp(getRuntimeClass()->getClassName(), className) == 0);
}

boolean BaseObject::isSerializable(const BaseRuntimeClass *rclass) const {
    return FALSE;
}

void BaseObject::wait() throw(InterruptedException) {
    ensureMonitorInit();
    _monitor->wait();
}

void BaseObject::wait(long timeout) throw(InterruptedException) {
    ensureMonitorInit();
    _monitor->wait(timeout);
}

void BaseObject::notify() throw(IllegalMonitorStateException) {
    ensureMonitorInit();
    _monitor->notify();
}

void BaseObject::notifyAll() throw(IllegalMonitorStateException) {
    ensureMonitorInit();
    _monitor->notifyAll();
}

void BaseObject::ensureMonitorInit() {
    if(_monitor == NULL) {
        _monitor = new Monitor();
    }
}

__END_NAMESPACE(SELFSOFT);

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
