/*************************************************************************
 * $Id: Socket.hxx,v 1.1 2000/11/14 06:19:28 samurai Exp $
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

#ifndef _SOCKET_HXX_
#define _SOCKET_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"
#include "String.hxx"
#include "File.hxx"
#include "Inet.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI Socket : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(Socket);

protected:

    Socket() throw(IOException);

    void open();

public:

    Socket(InetAddress &addr, int port) throw(IOException);
    Socket(InetAddress &addr, int port, InetAddress &localAddr, int localPort)
        throw(IOException);
    Socket(const char *host, int port) throw(IOException);
    Socket(const char *host, int port, const char *localAddr, int localPort)
        throw(IOException);
    ~Socket();

private:

    String _hostName;
    int _port;
    int _sock;

};

class UTILAPI ServerSocket : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(ServerSocket);

public:

    ServerSocket();
    ~ServerSocket();

private:

};

// Inline functions

inline Socket::Socket() throw(IOException) {

}

inline Socket::Socket(InetAddress &addr, int port) throw(IOException) {

}

inline Socket::Socket(InetAddress &addr, int port, InetAddress &localAddr, int localPort) throw(IOException) {

}

inline Socket::Socket(const char *host, int port) throw(IOException) {

}

inline Socket::Socket(const char *host, int port, const char *localAddr, int localPort) throw(IOException) {

}

inline Socket::~Socket() {

}

inline ServerSocket::ServerSocket() {

}

inline ServerSocket::~ServerSocket() {

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
