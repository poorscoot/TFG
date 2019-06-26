#include "test5.h"
#include <stdio.h>
#include <sstream>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

bool tryTCP = true;
bool ok = true;
bool connection = true;
bool manager = true;
bool start = true;
bool stage1 = true;
bool tryStage1 = true;
int thread_error;
pthread_t   marrthrHandle[ 2 ];


void* manageTcpReceivedData( void* useless ){
    bool                ok = true;
    char                buffer[ 65536 ];
    memset(buffer, 0, sizeof(buffer));

    while (ok) {    
        memset(buffer, 0, sizeof(buffer));
        ok &= mClientSocket.receiveTcpData( buffer, sizeof( buffer ) );
        if( ok ) {
            std::string message = buffer;
            mReceivedMessagesTCP.addMessage ( message );
        }
    }

}

void* manageUdpReceivedData( void* useless )
{
    bool            ok = true;
    char            udpBuffer[ 2048 ]={};
    int             time_error;
    int             pingNumber; 
    uint64_t        actualTimeStamp;
    uint64_t        receivedTimeStamp;
    struct          timeval time_s;
    int contadorpaquetes= 0; 
    std::string message;
    memset(udpBuffer, 0, sizeof(udpBuffer));
    while ( ok )
    {
        memset(udpBuffer, 0, sizeof(udpBuffer));

        ok &= mClientSocket.receiveUdpData( udpBuffer, sizeof( udpBuffer ) );
        contadorpaquetes ++; 
        if( ok )
        {
            time_error = gettimeofday(&time_s, NULL); 
            actualTimeStamp =  time_s.tv_sec*1000 + time_s.tv_usec/1000;
            message = udpBuffer;
           

            pingNumber = 0;

            // Comprobar que es un ping
            if ( Q4SMessageTools_isPingMessage(udpBuffer, &pingNumber, &receivedTimeStamp) )
            {

                #if SHOW_INFO2                
                    printf( "Received Ping, number:%d, timeStamp: %" PRIu64 "\n", pingNumber, receivedTimeStamp);
                #endif

                // mandar respuesta del ping
                char reasonPhrase[ 256 ];
                #if SHOW_INFO2
                    printf( "Ping responsed %d\n", pingNumber);
                #endif
                Q4SMessage message200;
                sprintf( reasonPhrase, "Q4S/1.0 200 OK\r\nSequence-Number:%d\r\nTimestamp:%" PRIu64 "\r\nContent-Length:0\r\n\r\n", pingNumber,receivedTimeStamp );
                ok &= mClientSocket.sendUdpData( reasonPhrase );

                #if SHOW_INFO2
                    printf( "Received Udp: <%s>\n", udpBuffer );
                #endif
            }
            mReceivedMessagesUDP.addMessage(message, actualTimeStamp);

        }
    }
}

int main () {
    start &= pthread_cancel(marrthrHandle[0]);
    start &= pthread_cancel(marrthrHandle[1]);
    start &= mClientSocket.closeConnection(SOCK_STREAM);
    start &= mClientSocket.closeConnection( SOCK_DGRAM );
    if (start){
        manager &= mReceivedMessagesTCP.init( );
        manager &= mReceivedMessagesUDP.init();
        if(manager){
        //Create BEGIN package
            //Call packageCreator, create Begin
            //The values given are taken from median videogame rate characteristic
            
            Q4SSDPParams    Proposal_params;
            Proposal_params.session_id = 1;
            Proposal_params.qosLevelUp = 0;
            Proposal_params.qosLevelDown = 0;
            Proposal_params.size_packet = 1000;
            Proposal_params.q4SSDPAlertingMode = Q4SSDPALERTINGMODE_Q4SAWARENETWORK;
            Proposal_params.alertPause = 2000;
            Proposal_params.recoveryPause = 2000;
            Proposal_params.latency = 0;
            Proposal_params.jitterUp = 0;
            Proposal_params.jitterDown = 0;
            Proposal_params.bandWidthUp = 1000;
            Proposal_params.bandWidthDown = 3000;
            Proposal_params.packetLossUp = 2;
            Proposal_params.packetLossDown = 2;
            Proposal_params.procedure.negotiationTimeBetweenPingsUplink = 50;
            Proposal_params.procedure.negotiationTimeBetweenPingsDownlink = 50;
            Proposal_params.procedure.continuityTimeBetweenPingsUplink = 50;
            Proposal_params.procedure.continuityTimeBetweenPingsDownlink = 50;
            Proposal_params.procedure.bandwidthTime = 1000;
            Proposal_params.procedure.windowSizeLatencyCalcUplink = 30;
            Proposal_params.procedure.windowSizeLatencyCalcDownlink = 30;
            Proposal_params.procedure.windowSizePacketLossCalcUplink = 30;
            Proposal_params.procedure.windowSizePacketLossCalcDownlink = 30;
            //The IP and port values are taken from the implementations
            Q4SMessage  message;
            message.initRequest(Q4SMTYPE_BEGIN, "127.0.0.1", "56001", false, 0, false, 0, false, 0, false, NULL, true, &Proposal_params);
        //Send BEGIN package
            //Create TCP port
            //Create TCP connection to the same port as protocol
            //Send Begin
            
            connection & mClientSocket.openConnection( SOCK_STREAM );
            connection &= mClientSocket.openConnection( SOCK_DGRAM );

            // launch received data managing threads.
            thread_error = pthread_create( &marrthrHandle[0], NULL, manageUdpReceivedData, NULL);
            thread_error = pthread_create( &marrthrHandle[1], NULL, manageTcpReceivedData ,NULL);

            if (connection){
                tryTCP &= mClientSocket.sendTcpData(message.getMessageCChar());

                //Receive ACK to BEGIN
                    //Wait until socket recieves answer
                    if ( tryTCP ) 
                    {
                        while(mReceivedMessagesTCP.size()==0){
                                sleep(1);
                        }
                        std::string messageR;
                        mReceivedMessagesTCP.readFirst( messageR );            
                        tryTCP = Q4SMessageTools_is200OKMessage(messageR,false, 0, 0);
                        if (tryTCP){
                            message.initRequest(Q4SMTYPE_READY, "127.0.0.1", "56001", false, 0, false, 0, true, 1);
                            tryStage1 &= mClientSocket.sendTcpData( message.getMessageCChar() );
                            if(tryStage1){
                                Q4SMeasurementResult results;
                                Q4SMeasurementResult downResults;
                                mReceivedMessagesTCP.readFirst( messageR );
                                stage1 &= Q4SMessageTools_is200OKMessage(messageR, false, 0, 0); 
                                if (stage1){
                                    
                                } else {
                                    std::cout<<messageR<<std::endl;
                                    std::cout<<"Failure, the server did not accept to pass to Stage 1"<<std::endl;
                                    pthread_cancel(marrthrHandle[0]);
                                    pthread_cancel(marrthrHandle[1]);
                                    mClientSocket.closeConnection(SOCK_STREAM);
                                    mClientSocket.closeConnection( SOCK_DGRAM );
                                    return 1;
                                }
                            } else {
                                std::cout<<"Could not send READY 1"<<std::endl;
                                pthread_cancel(marrthrHandle[0]);
                                pthread_cancel(marrthrHandle[1]);
                                mClientSocket.closeConnection(SOCK_STREAM);
                                mClientSocket.closeConnection( SOCK_DGRAM );
                                return 1;
                            }
                        } else {
                            std::cout<<"Did not receive 200 OK message"<<std::endl;
                            pthread_cancel(marrthrHandle[0]);
                            pthread_cancel(marrthrHandle[1]);
                            mClientSocket.closeConnection(SOCK_STREAM);
                            mClientSocket.closeConnection( SOCK_DGRAM );
                            return 1;
                        }
                    } else { 
                        std::cout<<"Could not send TCP message"<<std::endl;
                        pthread_cancel(marrthrHandle[0]);
                        pthread_cancel(marrthrHandle[1]);
                        mClientSocket.closeConnection(SOCK_STREAM);
                        mClientSocket.closeConnection( SOCK_DGRAM );
                        return 1;
                    }
            } else {
                std::cout<<"Could not establish TCP session"<<std::endl;
                pthread_cancel(marrthrHandle[0]);
                pthread_cancel(marrthrHandle[1]);
                mClientSocket.closeConnection(SOCK_STREAM);
                mClientSocket.closeConnection( SOCK_DGRAM );
                return 1;
            }
        } else {
            std::cout<<"Could not create TCP messages manager"<<std::endl;
            pthread_cancel(marrthrHandle[0]);
            pthread_cancel(marrthrHandle[1]);
            mClientSocket.closeConnection(SOCK_STREAM);
            mClientSocket.closeConnection( SOCK_DGRAM );
            return 1;
        }
    } else {
        std::cout<<"Test could not start"<<std::endl;
        return 1;
    }
}
std::cout<<"Success"<<std::endl;
                                    pthread_cancel(marrthrHandle[0]);
                                    pthread_cancel(marrthrHandle[1]);
                                    mClientSocket.closeConnection(SOCK_STREAM);
                                    mClientSocket.closeConnection( SOCK_DGRAM );
                                    return 0;