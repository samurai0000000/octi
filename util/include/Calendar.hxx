/*************************************************************************
 * $Id: Calendar.hxx,v 1.3 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created:  11/15/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _CALENDAR_HXX_
#define _CALENDAR_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"
#include "Date.hxx"
#include "TimeZone.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

enum CAL_FIELD {
    ERA                    = 0,
    YEAR                   = 1,
    MONTH                  = 2,
    WEEK_OF_YEAR           = 3,
    DATE                   = 4,
    DAY_OF_MONTH           = 5,
    DAY_OF_YEAR            = 6,
    DAY_OF_WEEK            = 7,
    DAY_OF_WEEK_IN_MONTH   = 8,
    AM_PM                  = 9,
    HOUR                   = 10,
    HOUR_OF_DAY            = 11,
    MINUTE                 = 12,
    SECOND                 = 13,
    MILLISECOND            = 14,
    ZONE_OFFSET            = 15,
    DST_OFFSET             = 16
};

/* The number of fields would equal to the cardinal number of the last element
   in the CAL_FIELD enumeration plus 1. */
#define NUM_FIELDS DST_OFFSET + 1

enum CAL_DAY {
    AM = 0,
    PM = 1
};

enum CAL_WEEK {
    SUNDAY     = 1,
    MONDAY     = 2,
    TUESDAY    = 3,
    WEDNESDAY  = 4,
    THURSDAY   = 5,
    FRIDAY     = 6,
    SATURDAY   = 7
};

enum CAL_MONTH {
    JANUARY    =  0,
    FEBRUARY   =  1,
    MARCH      =  2,
    APRIL      =  3,
    MAY        =  4,
    JUNE       =  5,
    JULY       =  6,
    AUGUST     =  7,
    SEPTEMBER  =  8,
    OCTOBER    =  9,
    NOVEMBER   = 10,
    DECEMBER   = 11
};

class UTILAPI Calendar : public BaseObject {

    DECLARE_RUNTIME_SERIALIZABLE(Calendar);

public:

    // Note: getInstance() returns a new instance of Calendar. The object must
    // be explicitly freed when no longer in use.
    static Calendar *getInstance();
    static Calendar *getInstance(const TimeZone &zone);

    Date getTime() const;
    void setTime(const Date &date);

    int get(CAL_FIELD field) const;
    void set(CAL_FIELD field, int value);
    void set(int year, int month, int date,
             int hour = 0, int minute = 0, int second = 0);
    void clear();
    void clear(CAL_FIELD field);
    boolean isSet(CAL_FIELD field) const;

    boolean after(const Calendar &date) const;
    boolean before(const Calendar &date) const;
    boolean operator>(const Calendar &date) const;
    boolean operator<(const Calendar &date) const;
    boolean operator>=(const Calendar &date) const;
    boolean operator<=(const Calendar &date) const;
    boolean operator==(const Calendar &date) const;
    boolean operator!=(const Calendar &date) const;

    void add(CAL_FIELD field, int amount);
    void roll(CAL_FIELD field, boolean up);
    void roll(CAL_FIELD field, int amount);

    void setTimeZone(const TimeZone &zone);
    TimeZone getTimeZone() const;

    void setLenient(boolean lenient);
    boolean isLenient() const;

    void setFirstDayOfWeek(int value);
    int getFirstDayOfWeek() const;
    void setMinimalDaysInFirstWeek(int value);
    int getMinimalDaysInFirstWeek() const;

    int getMinimum(CAL_FIELD field) const;
    int getMaximum(CAL_FIELD field) const;
    int getGreatestMinimum(CAL_FIELD field) const;
    int getGreatestMaximum(CAL_FIELD field) const;
    int getLeastMinimum(CAL_FIELD field) const;
    int getLeastMaximum(CAL_FIELD field) const;
    int getActualMinimum(CAL_FIELD field) const;
    int getActualMaximum(CAL_FIELD field) const;
  
    const char *toString() const;

protected:

    Calendar();
    Calendar(const TimeZone &zone);

    void computeTime() const;
    void computeFields() const;
  
    long getTimeInMillis() const;
    void setTimeInMillis(long millis);

    int internalGet(CAL_FIELD field) const;
    void complete();

    int _fields[NUM_FIELDS];
    boolean _isSet[NUM_FIELDS];
    long _time;
    boolean _isTimeSet;
    boolean _areFieldsSet;

};

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
