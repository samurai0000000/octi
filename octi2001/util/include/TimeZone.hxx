/*************************************************************************
 * $Id: TimeZone.hxx,v 1.2 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created:  11/15/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _TIMEZONE_HXX_
#define _TIMEZONE_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"
#include "Slist.hxx"
#include "Date.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

enum TZ_FORMAT {
    SHORT = 0,
    LONG  = 1
};

class UTILAPI TimeZone : public BaseObject {

    DECLARE_RUNTIME_SERIALIZABLE(TimeZone);

public:

    TimeZone();

    // Note: getTimeZone() and getDefault() creates a new instance of TimeZone
    // which must be explicitly freed when no longer in use.
    static TimeZone *getTimeZone(const String &Id);
    static TimeZone *getDefault();

    int getOffset(int era, int year, int month,
                  int day, int dayOfWeek, int milliseconds) const;
    void setRawOffset(int offsetMillis);
    int getRawOffset() const;

    String getId() const;
    void setID(const String &Id);

    String getDisplayName(boolean daylight = FALSE, TZ_FORMAT = SHORT) const;
    boolean useDaylightTime() const;
    boolean inDaylightTime(const Date &date) const;

    Slist<String> getAvailableIds(int rawOffset = 0) const;

    boolean hasSameRules(const TimeZone &other) const;

protected:

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
