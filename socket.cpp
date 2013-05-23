#include <stdio.h>
#include <iostream>
#include <sstream>
#include "Socket.h"

using namespace std;

int Socket::nofSockets_= 0;

void Socket::start() {
    if (!nofSockets_) {
        WSADATA info;
        if (WSAStartup(MAKEWORD(2,0), &info)) {
        throw "Could not start WSA";
        }
    }
    ++nofSockets_;
}

void Socket::end() {
    WSACleanup();
}

Socket::Socket() : s_(0) {
    start();
    // UDP: use SOCK_DGRAM instead of SOCK_STREAM
    s_ = socket(AF_INET,SOCK_STREAM,0);

    if (s_ == INVALID_SOCKET) {
        throw "INVALID_SOCKET";
    }

    refCounter_ = new int(1);
}

Socket::Socket(SOCKET s) : s_(s) {
    start();
    refCounter_ = new int(1);
};

Socket::~Socket() {
    if (! --(*refCounter_)) {
        close();
        delete refCounter_;
    }

    --nofSockets_;
    if (!nofSockets_) end();
}

Socket::Socket(const Socket& o) {
    refCounter_=o.refCounter_;
    (*refCounter_)++;
    s_ = o.s_;

    nofSockets_++;
}

Socket& Socket::operator=(Socket& o) {
  (*o.refCounter_)++;

    refCounter_ = o.refCounter_;
    s_ =o.s_;

    nofSockets_++;

    return *this;
}

void Socket::close() {
    closesocket(s_);
}

void Socket::initCtlSocket() {
    u_long arg = 0;
    if (ioctlsocket(s_, FIONREAD, &arg) != 0)
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
        }
    }
}

u_long Socket::receiveBytes( char* ret_buf, u_long max_recv) {
    // Nonblocking mode = disabled
    // arg = amount of data

    // TODO: catch any throw error here
    int rv = recv (s_, ret_buf, max_recv, 0);
    if (rv < 0)
    {
        cout << "Winsock recv error: " << rv << endl;
    }
    else if( rv > 0 )
    {
        cout << "received " << rv << " bytes" << endl;
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
}

void SocketClient::sendRequest( char *request,
                                u_int length )
{
    stringstream stream;
    stream.write(request, length);
    sendBytes( stream.str() );
}

char* SocketClient::receiveResponse( u_int responseSize )
{
    initCtlSocket();

    char* server_resp = new char [responseSize];
    u_int resp_size( 0 );

    while( resp_size < responseSize )
    {
        if( resp_size < responseSize )
        {
            char* ptr = server_resp+resp_size;

            int response = (u_int)receiveBytes( ptr, responseSize-resp_size );
            if(response > 0){
                resp_size += response;
            }
            else if(response < 0){
                delete server_resp;
                throw response;
            }
            else{
                cout << "Received 0 bytes" << endl;
                throw "Aborting";
            }

        }
        else
        {
            cout << "resp size exceed max:" << resp_size << endl;
            delete server_resp;
            throw "Aborting";
        }
    }
    return server_resp;
}
