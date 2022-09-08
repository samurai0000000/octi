/*************************************************************************
 * $Id: GameServer.hxx,v 1.1 2000/11/18 10:25:33 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/18 10:25:33 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created: 11/17/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _GAMESERVER_HXX_
#define _GAMESERVER_HXX_

#include "BaseObject.hxx"
#include "Socket.hxx"
#include "gameapi.h"

__BEGIN_NAMESPACE(SELFSOFT);

class GAMEAPI GameServer : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(GameServer);

public:

    GameServer();
    ~GameServer();

private:

};

// Inline functions

inline GameServer::GameServer() {

}

inline GameServer::~GameServer() {

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
