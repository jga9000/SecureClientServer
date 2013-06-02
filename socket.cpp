#include <stdio.h>
#include <iostream>
#include <sstream>
#include "Socket.h"

using namespace std;

void Socket::start() {
    WSADATA info;
    if (WSAStartup(MAKEWORD(2,0), &info)) {
        throw "Could not start WSA";
    }
}

void Socket::end() {
    WSACleanup();
}

Socket::Socket() : s_(0) {
    start();
    s_ = socket(AF_INET,SOCK_STREAM,0);

    if (s_ == INVALID_SOCKET) {
        throw "INVALID_SOCKET";
    }
}

Socket::Socket(SOCKET s) : s_(s) {
    start();
};

Socket::~Socket() {
    close();
    end();
}

void Socket::close() {
    closesocket(s_);
}

int Socket::handleError( int err )
{
    if( err != 0)
    {
        int nError = WSAGetLastError();
        if(nError != WSAEINPROGRESS && nError !=0 )
        {
            cout << "Winsock error code: " << nError << endl;
            cout << "Server disconnected!" << endl;
            // Shutdown our socket
            shutdown(s_, SD_SEND);

            // Close our socket entirely
            closesocket(s_);
            return err;
        }
    }
    return 0;
}

void Socket::initCtlSocket(u_long arg) {
    // 0 for blocking mode
    u_long arg_ret = arg;
    handleError( ioctlsocket(s_, FIONBIO, &arg_ret) );
}

u_long Socket::receiveBytes( char* ret_buf, u_long max_recv) {
    // Nonblocking mode = disabled
    int rv = recv (s_, ret_buf, max_recv, 0);
    if (rv < 0)
    {
        cout << "Winsock recv error: " << rv << endl;
    }
    else if( rv > 0 ) {
        cout << "received " << rv << " bytes" << endl;
    }
    else if( rv == 0) {
        cout << "received 0 bytes" << endl;
    }
    return rv;
}

void Socket::sendBytes(const std::string& s) {
    send(s_, s.c_str(), s.length(), 0);
}

void Socket::sendBytes(const char& s, u_int length) {
    send(s_, &s, length, 0);
}

SocketClient::SocketClient(const std::string& host, int port) : Socket() {

    std::string error = "";

    hostent *he;
    if ((he = gethostbyname(host.c_str())) == 0) {
        error = strerror(errno);
        throw error;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((in_addr *)he->h_addr);
    memset(&(addr.sin_zero), 0, 8);

    int err = connect(s_, (sockaddr *) &addr, sizeof(sockaddr));

    if ( err != 0 ) {
        error = strerror(WSAGetLastError());
        cout << "SocketClient, connect error:" << error << endl;
        Sleep( 5000 );
        throw error;
    }
    initCtlSocket(BlockingSocket);
}

void SocketClient::sendRequest( char *request,
                                u_int length,
                                int blocking_mode )
{
    //initCtlSocket(blocking_mode);
    stringstream stream;
    stream.write(request, length);
    sendBytes( stream.str() );
}

char* SocketClient::receiveResponse( u_int responseMax, int blocking_mode )
{
    //initCtlSocket(1);

    char* server_resp = new char [responseMax];
    while( 1 )
    {
        u_long arg = 0; // How many bytes avail
        int ret = ioctlsocket(s_, FIONREAD, &arg);
        if( ret < 0)
        {
            if( handleError( ret ) < 0) {
                delete server_resp;
                throw ret;
                break;
            }
        }

        if (arg == 0 )
        {
            if( handleError( ret ) < 0) {
                delete server_resp;
                throw ret;
                break;
            }
        }
        else{
            if (arg > responseMax) arg = responseMax;

            int resp = (u_int)receiveBytes( server_resp, responseMax );
            if( ret < 0 && handleError( resp ) ) {
                delete server_resp;
                throw resp;
            }
            else if(resp >= 0 ){
                cout << "SocketClient, received " << resp << " bytes" << endl;
                break;
            }
        }
    }
    return server_resp;
}
