/*************************************************************************
 * $Id: TimeTest.cxx,v 1.5 2000/11/14 06:17:56 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/14 06:17:56 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created:  12/18/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#include "Date.hxx"
#include "Calendar.hxx"
#include "TimeZone.hxx"

__USING_NAMESPACE(SELFSOFT);

void doDateTest() {
    Date date;

    cout << "Number of seconds: " << date.getTime() << endl;
}

void doCalendarTest() {

}

void doTimeZoneTest() {

}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
