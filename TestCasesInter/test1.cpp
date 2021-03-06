#include "test1.h"
#include <stdio.h>
#include <sstream>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

bool start = true;
bool tryTCP = true;
bool connection = true;
bool manager = true;
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

int main () {
    start &= pthread_cancel(marrthrHandle[1]);
    start &= mClientSocket.closeConnection(SOCK_STREAM);
    if (start){
        manager &= mReceivedMessagesTCP.init();
        if(manager){
        //Create BEGIN package
            //The values given are taken from median videogame rate characteristic
            Q4SSDPParams    Proposal_params; 
            Proposal_params.session_id = 1;
            Proposal_params.qosLevelUp = 0;
            Proposal_params.qosLevelDown = 0;
            Proposal_params.size_packet = 1000;
            Proposal_params.q4SSDPAlertingMode = Q4SSDPALERTINGMODE_Q4SAWARENETWORK;
            Proposal_params.alertPause = 2000;
            Proposal_params.recoveryPause = 2000;
            Proposal_params.latency = 100;
            Proposal_params.jitterUp = 10;
            Proposal_params.jitterDown = 10;
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
            //The port depends on the implementation
            Q4SMessage  message;
            message.initRequest(Q4SMTYPE_BEGIN, "127.0.0.1", "56001", false, 0, false, 0, false, 0, false, NULL, true, &Proposal_params);
            //Create a connection with the server
            
            connection &= mClientSocket.openConnection( SOCK_STREAM );

            //Create a thread to manage the TCP data
            thread_error = pthread_create( &marrthrHandle[1], NULL, manageTcpReceivedData ,NULL);

            if (connection){
                //Send the begin message
                tryTCP &= mClientSocket.sendTcpData(message.getMessageCChar());
                    
                    if ( tryTCP ) 
                    {
                        //Wait for an answer
                        int i = 0;
                        while(mReceivedMessagesTCP.size()==0){
                            i++;
                            sleep(1);
                            if (i>=100){
                                std::cout<<"Failure, server did not send a message or it didn't reach"<<std::endl;
                                pthread_cancel(marrthrHandle[1]);
                                mClientSocket.closeConnection(SOCK_STREAM);
                                return 1;
                            }
                        }
                        //Check if the message received is a 200 OK or not
                        std::string message;
                        mReceivedMessagesTCP.readFirst( message );            
                        tryTCP = Q4SMessageTools_is200OKMessage(message,false, 0, 0);
                        if (tryTCP){
                            std::cout<<"Success"<<std::endl;
                            pthread_cancel(marrthrHandle[1]);
                            mClientSocket.closeConnection(SOCK_STREAM);
                            return 0;
                        } else {
                            std::cout<<message<<std::endl;
                            std::cout<<"Failure, did not receive 200 OK message"<<std::endl;
                            pthread_cancel(marrthrHandle[1]);
                            mClientSocket.closeConnection(SOCK_STREAM);
                            return 1;
                        }
                    } else { 
                        std::cout<<"Failure, could not send TCP message"<<std::endl;
                        pthread_cancel(marrthrHandle[1]);
                        mClientSocket.closeConnection(SOCK_STREAM);
                        return 1;
                    }
            } else {
                std::cout<<"Failure, could not establish TCP session"<<std::endl;
                pthread_cancel(marrthrHandle[1]);
                mClientSocket.closeConnection(SOCK_STREAM);
                return 1;
            }
        } else {
            std::cout<<"Failure, could not create TCP messages manager"<<std::endl;
            pthread_cancel(marrthrHandle[1]);
            mClientSocket.closeConnection(SOCK_STREAM);
            return 1;
        }
    } else {
        std::cout<<"Failure, test could not start"<<std::endl;
        return 1;
    }
}