/*************************************************************************
 * $Id: GameSession.hxx,v 1.1 2000/11/18 10:25:33 samurai Exp $
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

#ifndef _GAMESESSION_HXX_
#define _GAMESESSION_HXX_

#include "Event.hxx"
#include "Socket.hxx"
#include "GameExceptions.hxx"
#include "GameClient.hxx"
#include "GameServer.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

enum GameSessionType {
  GT_UNKNOWN,
  GT_PEER,
  GT_SERVER
};

enum GameSessionState {
  GS_INACTIVE,
  GS_WAIT_REMOTE,
  GS_CONNECT_REMOTE,
  GS_AUTHENTICATE,
  GS_IDLE,
};

class GAMEAPI GameSessionListener : public EventListener {

public:

  virtual void chatMessage() = 0;
  virtual void tableMessage() = 0;
  virtual void gameMessage() = 0;

};

class GAMEAPI GameSession : public BaseObject {

  DECLARE_RUNTIME_DISCOVERABLE(GameSession);

public:

  GameSession();
  ~GameSession();

  GameSessionState getSessionState() const;
  GameSessionType getSessionType() const;

  // Connection operators
  void listen(int port) throw(GameSessionException);
  void connect(const char *host, int port) throw(GameSessionException);
  void disconnect() throw(GameSessionException);

  // Messaging operators
  void say(const char *msg) throw(GameSessionException);
  void whisper(const char *msg) throw(GameSessionException);
  void invite() throw(GameSessionException);
  void boot() throw(GameSessionException);
  void ignore() throw(GameSessionException);
  void who() throw(GameSessionException);
  void tables() throw(GameSessionException);
  void join() throw(GameSessionException);

  // Game control operators
  void sit() throw(GameSessionException);
  void stand() throw(GameSessionException);
  void move() throw(GameSessionException);
  void resign() throw(GameSessionException);
  void call() throw(GameSessionException);

private:

  void keepalive();

private:

  GameSessionState _state;
  GameSessionType _type;

  union {
    GameClient *client;
    GameServer *server;
  } connection;

};

// Inline functions

inline GameSession::GameSession() {
  _state = GS_INACTIVE;
  _type = GT_UNKNOWN;
}

inline GameSession::~GameSession() {
  
}

inline GameSessionState GameSession::getSessionState() const {
  return _state;
}

inline GameSessionType GameSession::getSessionType() const {
  return _type;
}

__END_NAMESPACE(SELFSOFT);

#endif
