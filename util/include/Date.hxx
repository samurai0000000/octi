/*************************************************************************
 * $Id: Date.hxx,v 1.6 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.6 $
 *
 * Created:  11/15/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _DATE_HXX_
#define _DATE_HXX_

#include <time.h>
#include "utilapi.h"
#include "BaseObject.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI Date : public BaseObject {

  DECLARE_RUNTIME_SERIALIZABLE(Date);

public:

  Date();
  Date(long time);

  long getTime() const;
  void setTime(long time);
  void captureCurrentTime();

  boolean after(const Date &date) const;
  boolean before(const Date &date) const;
  boolean operator>(const Date &date) const;
  boolean operator<(const Date &date) const;
  boolean operator>=(const Date &date) const;
  boolean operator<=(const Date &date) const;
  boolean operator==(const Date &date) const;
  boolean operator!=(const Date &date) const;

  static unsigned long currentTimeMillis();

private:

  long _time;  // Since January 1, 1970, 00:00:00 GMT.

};

// Inline functions

inline Date::Date() {
  captureCurrentTime();
}

inline Date::Date(long time) {
  _time = time;
}

inline long Date::getTime() const {
  return _time;
}

inline void Date::setTime(long time) {
  _time = time;
}

inline boolean Date::after(const Date &date) const {
  return _time > date._time;
}

inline boolean Date::before(const Date &date) const {
  return _time < date._time;
}

inline boolean Date::operator>(const Date &date) const {
  return _time > date._time;
}

inline boolean Date::operator<(const Date &date) const {
  return _time < date._time;
}

inline boolean Date::operator>=(const Date &date) const {
  return _time >= date._time;
}

inline boolean Date::operator<=(const Date &date) const {
  return _time <= date._time;
}

inline boolean Date::operator==(const Date &date) const {
  return _time == date._time;
}

inline boolean Date::operator!=(const Date &date) const {
  return _time != date._time;
}

__END_NAMESPACE(SELFSOFT);

#endif
