/*************************************************************************
 * $Id: Timer.cxx,v 1.2 2000/11/18 10:23:45 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/18 10:23:45 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created: 11/7/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include "Date.hxx"
#include "Timer.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_DISCOVERABLE(TimerTick, BaseObject);
IMPLEMENT_RUNTIME_SERIALIZABLE(TimerEvent, BaseObject, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(Timer, BaseObject, 1);

void TimerTick::run() {
  unsigned long delay = _timer->getDelay();
  unsigned long currentTime = Date::currentTimeMillis();
  unsigned long nextTime;

  _running = true;

  while(_running) {
    try {
      nextTime = currentTime + delay;
      unsigned long waitTime = nextTime - Date::currentTimeMillis();
      currentTime = nextTime;
      if(waitTime > 0) {
	Thread::sleep(waitTime);
      } else {
	// The target listener cannot keep up with the ticks.
	// Coalesce the firings.
	Thread::sleep(1);
	do {
	  currentTime += delay;
	} while(currentTime < Date::currentTimeMillis());
      }
      _timer->tick(this);
    } catch(InterruptedException e) {
    }
  }
}

void TimerTick::stopTick() {
  synchronized(*this, {
    _running = false;
    interrupt();
  });
}

void Timer::start() {
  if(_tick == NULL) {
    _tick = new TimerTick(this);
    _tick->start();
  }
}

void Timer::stop() {
  if(_tick != NULL) {
    _tick->stopTick();
    _tick = NULL;
  }
}

void Timer::restart() {
  stop();
  start();
}

void Timer::fireTimeTick() {
  TimerEvent e(this);
  for(int i = _listeners.getNumListeners() - 1; i >= 0; i--) {
    TimerListener *listener = (TimerListener *) _listeners.getListenerAt(i);
    listener->timeTick(&e);
  }
}

void Timer::tick(BaseObject *source) {
  fireTimeTick();
}

__END_NAMESPACE(SELFSOFT);
