/*************************************************************************
 * $Id: Packet.hxx,v 1.1 2000/11/14 06:19:28 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/14 06:19:28 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created: 11/11/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _PACKET_HXX_
#define _PACKET_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI Packet : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(Packet);

public:

    Packet();
    ~Packet();

private:

};


// Inline functions

inline Packet::Packet() {

}

inline Packet::~Packet() {

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
