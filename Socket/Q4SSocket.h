#ifndef _Q4SSOCKET_H_
#define _Q4SSOCKET_H_

#include <sys/time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <netinet/tcp.h>

using namespace std;
class Q4SSocket
{
public:

    //Constructor
    Q4SSocket( );
    //Destructor
    ~Q4SSocket( );
    // Init
    bool    init( );
    //Done
    void    done( );
    //Sets the socket, type of socket, IP to connect (TCP or UDP)
    void    setSocket( int socket, int socketType, string* connectToIP = NULL, string* connectToUDPPort = NULL);
    //Sends data ovet the socket
    bool    sendData( const char* sendBuffer, sockaddr_in* pAddrInfo = NULL, bool showInfo = false );
    //Sends BWdataover the socket
    bool    sendBWData( const char* sendBuffer, sockaddr_in* pAddrInfo = NULL);
    //Receives data over the socket
    bool    receiveData( char* receiveBuffer, int receiveBufferSize, sockaddr_in* pAddrInfo, bool showInfo);
    //Closes the socket
    bool    shutDown( );
    //Closes the socket
    bool    disconnect( );
private:
    //Clear
    void    clear( );

    int             mSocket;
    int             mSocketType;
    string*    mSocketUDPConnectToIP;
    string*    mSocketUDPConnectToPort;
    sockaddr_in     mPeerAddrInfo;
    socklen_t       mPeerAddrInfoLen;
    bool            mAlreadyReceived;

    struct timeval timeout;      

};

#endif  // _Q4SSOCKET_H_