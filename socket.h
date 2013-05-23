#ifndef SOCKET_H
#define SOCKET_H

#include <WinSock2.h>
#include <string>

enum TypeSocket {BlockingSocket, NonBlockingSocket};

class Socket {
public:

    virtual ~Socket();
    Socket(const Socket&);
    Socket& operator = (Socket&);

    void initCtlSocket();
    u_long receiveBytes( char*, u_long );

    void close();

    void sendBytes( const std::string& );
    void sendBytes( const char&, u_int length );

protected:

    Socket(SOCKET s);
    Socket();

    SOCKET s_;

    int* refCounter_;

private:
    static void start();
    static void end();
    static int  nofSockets_;
};

class SocketClient : public Socket {
public:
    SocketClient( const std::string& host, int port );

    void sendRequest( char *request, u_int length );
    char* receiveResponse( u_int responseSize );
};

#endif
