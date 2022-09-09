/*************************************************************************
 * $Id: Event.hxx,v 1.3 2000/11/14 06:15:33 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/14 06:15:33 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created:  11/17/99 by Charles Chiou
 * Comments: Utilities for handling call-back events.
 *           When an external event happens, the source object broadcasts
 *           the event to all the handlers in the order which they were
 *           registered (added) to the EventHandlerList, which in turn
 *           process the event and returns.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _EVENT_HXX_
#define _EVENT_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"
#include "BufferedArray.hxx"
#include "Thread.hxx"
#include "Void.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

/**
 * The event to be sent to the event handler.
 */
class UTILAPI EventObject : public BaseObject {

    DECLARE_RUNTIME_SERIALIZABLE(EventObject);

public:

    EventObject();
    EventObject(const BaseObject *source);

    const BaseObject *getSource() const;

protected:

    const BaseObject *_source;

};

/**
 * Base interface class for all event listeners.
 */
class UTILAPI EventListener {

};

/**
 * A utility class for storing event listeners.
 */
class UTILAPI EventListenerList : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(EventListenerList);

public:

    EventListenerList();
    ~EventListenerList();

    void addEventListener(EventListener *listener);
    void removeEventListener(EventListener *listener);

    int getNumListeners() const;
    EventListener *getListenerAt(int index);

private:

    BufferedArray<Void> _listeners;

};

// Inline functions

inline EventObject::EventObject() {
    _source = NULL;
}

inline EventObject::EventObject(const BaseObject *source) {
    _source = source;
}

inline const BaseObject *EventObject::getSource() const {
    return _source;
}


inline EventListenerList::EventListenerList() {

}

inline EventListenerList::~EventListenerList() {

}

inline int EventListenerList::getNumListeners() const {
    return _listeners.size();
}


inline EventListener *EventListenerList::getListenerAt(int index) {
    Void v = _listeners[index];
    EventListener *listener = (EventListener *) v.getPointer();
    return listener;
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
