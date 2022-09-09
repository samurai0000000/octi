/*************************************************************************
 * $Id: Inet.hxx,v 1.2 2000/12/03 00:04:19 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/12/03 00:04:19 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created:  11/11/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _INET_HXX_
#define _INET_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI InetAddress : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(InetAddress);

public:

    InetAddress();
    ~InetAddress();

    const char *getHostName() const;
    const char *getHostAddress() const;

private:

};

// Inline functions

inline InetAddress::InetAddress() {

}

inline InetAddress::~InetAddress() {

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
