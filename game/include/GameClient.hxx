/*************************************************************************
 * $Id: GameClient.hxx,v 1.1 2000/11/18 10:25:33 samurai Exp $
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

#ifndef _GAMECLIENT_HXX_
#define _GAMECLIENT_HXX_

#include "BaseObject.hxx"
#include "Socket.hxx"
#include "gameapi.h"

__BEGIN_NAMESPACE(SELFSOFT);

class GAMEAPI GameClient : public BaseObject {

  DECLARE_RUNTIME_DISCOVERABLE(GameClient);

public:

  GameClient();
  ~GameClient();

private:

};

// Inline functions

inline GameClient::GameClient() {

}

inline GameClient::~GameClient() {

}

__END_NAMESPACE(SELFSOFT);

#endif
