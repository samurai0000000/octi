/*************************************************************************
 * $Id: Timer.hxx,v 1.3 2000/12/03 00:11:35 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/12/03 00:11:35 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created: 11/7/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _TIMER_HXX_
#define _TIMER_HXX_

#include "Thread.hxx"
#include "Event.hxx"

#define DEFAULT_DELAY 1000ul   // 1 second default delay

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI TimerTick;
class UTILAPI TimerEvent;
class UTILAPI TimerListener;
class UTILAPI Timer;

/**
 * Event object generated by timer.
 */
class UTILAPI TimerEvent : public EventObject {

  DECLARE_RUNTIME_SERIALIZABLE(TimerEvent);

public:

  TimerEvent();
  TimerEvent(const BaseObject *source);

};

/**
 * Interface class for listening to time events.
 */
class UTILAPI TimerListener : public EventListener {

public:

  virtual void timeTick(TimerEvent *e) = 0;

};

/**
 * This is a thread for generating timer events.
 */
class UTILAPI TimerTick : public Thread {

  DECLARE_RUNTIME_DISCOVERABLE(TimerTick);

public:

  TimerTick(Timer *timer);
  ~TimerTick();
  
  void run();
  void stopTick();

private:

  friend class Timer;

  Timer *_timer;
  boolean _running;

};

/**
 * The Timer class that is of most interest to other applications.
 * USE this class for generating timed events!
 */
class UTILAPI Timer : public BaseObject {

  DECLARE_RUNTIME_SERIALIZABLE(Timer);

public:

  Timer(unsigned long delay = DEFAULT_DELAY);
  ~Timer();

  void addTimerListener(TimerListener *listener);
  void removeTimerListener(TimerListener *listener);
  
  void setDelay(unsigned long delay);
  unsigned long getDelay() const;
  void setRepeat(boolean repeat);
  boolean isRepeat() const;

  boolean isRunning() const;

  void start();
  void stop();
  void restart();

private:

  friend class TimerTick;

  void fireTimeTick();
  void tick(BaseObject *source);

private:

  Ptr<TimerTick> _tick;
  EventListenerList _listeners;
  unsigned long _delay;
  boolean _isRepeat;

};

// Inline functions

inline TimerEvent::TimerEvent() : EventObject() {

}

inline TimerEvent::TimerEvent(const BaseObject *source) : EventObject(source)  {

}

inline TimerTick::TimerTick(Timer *timer) : Thread("TIMER") {
  _timer = timer;
  _running = false;
}

inline TimerTick::~TimerTick() {

}

inline Timer::Timer(unsigned long delay) {
  _delay = delay;
}

inline Timer::~Timer() {

}

inline void Timer::addTimerListener(TimerListener *listener) {
  synchronized(_listeners, {
    _listeners.addEventListener(listener);
  });
}

inline void Timer::removeTimerListener(TimerListener *listener) {
  synchronized(_listeners, {
    _listeners.removeEventListener(listener);
  });
}

inline void Timer::setDelay(unsigned long delay) {
  _delay = delay;
}

inline unsigned long Timer::getDelay() const {
  return _delay;
}

inline void Timer::setRepeat(boolean repeat) {
  _isRepeat = repeat;
}

inline boolean Timer::isRepeat() const {
  return _isRepeat;
}

inline boolean Timer::isRunning() const {
  return _tick != NULL;
}

__END_NAMESPACE(SELFSOFT);

#endif
