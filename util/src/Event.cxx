/*************************************************************************
 * $Id: Event.cxx,v 1.5 2000/11/14 06:17:40 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/14 06:17:40 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created:  11/17/99 by Charles Chiou
 * Comments: Utilities for handling call-back events.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include "Event.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(EventObject, BaseObject, 1);
IMPLEMENT_RUNTIME_DISCOVERABLE(EventListenerList, BaseObject);

void EventListenerList::addEventListener(EventListener *listener) {
  if(listener == NULL) {
    return;
  }

  int size = _listeners.size();
  for(int i = 0; i < size; i++) {
    if(getListenerAt(i) == listener) {
      return;
    }
  }

  _listeners.append(Void(listener));
}

void EventListenerList::removeEventListener(EventListener *listener) {
  if(listener == NULL) {
    return;
  }

  _listeners.removeElement(Void(listener));
}

__END_NAMESPACE(SELFSOFT);
