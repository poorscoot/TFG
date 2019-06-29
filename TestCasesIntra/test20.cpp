#include "test20.h"
#include <stdio.h>
#include <sstream>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

bool start =true;
bool manager =true;
bool connection =true;
bool ok =true;
pthread_t                   marrthrListenHandle[2];
pthread_t                   marrthrDataHandle[2];
pthread_t                   marrthrHandle[2];

void* manageTcpReceivedData( void* useless ){
    bool                ok = true;
    char                buffer[ 65536 ]={};     
    memset(buffer, 0, sizeof(buffer));         
    while( ok ) 
    {
        memset(buffer, 0, sizeof(buffer));
        ok &= mServerSocket.receiveTcpData( 1, buffer, sizeof( buffer ) );
        if( ok )
        {
            //printf("TCP: %s\n", &buffer);
            std::string message = buffer;
            mReceivedMessagesTCP.addMessage ( message );
        }
    }    
    std::string message ="CANCEL\r\n" ;
    mReceivedMessagesTCP.addMessage ( message );

    return NULL;
}

void* manageTcpConnection( void* useless )
{
    bool        ok = true;
    int         newConnId = 1;
    int         thread_error;
    if( ok )
    {
        ok &= mServerSocket.startTcpListening( );
    }

    while( ok )
    {
        ok &= mServerSocket.waitForTcpConnection( newConnId );
        if( ok )
        {
            thread_error = pthread_create( &marrthrDataHandle[0], NULL, manageTcpReceivedData, NULL);
            if (thread_error< 0)
            {
                #if SHOW_INFO
                printf("ERRor marrthrDataHandle\n");
                #endif
            }
        }
    } 
    return NULL;
}

bool openConnectionListening()
{
    bool ok = true;
    int thread_error;
    thread_error = pthread_create( &marrthrListenHandle[0], NULL, manageTcpConnection, NULL);
    if (thread_error< 0)
        {
            printf("ERRor marrthrListenHandle[0]\n");
        }
    return ok;
}

int main(){
  //Close connections to prevent failure
  if(start){
    manager &= mReceivedMessagesTCP.init( );
    //Create socket to receive BEGIN
    //Create thread
    mServerSocket.startAlertSender();
    if(manager){
      connection &= openConnectionListening(); 
      if(connection){
        //Waits until you receive a message
        while(mReceivedMessagesTCP.size()==0){
            sleep(1);
        }
        std::string message;
        mReceivedMessagesTCP.readFirst( message );
        //Check if the message received is a BEGIN or not
        std::string extracted;
        std::string::size_type initialPosition;
        std::istringstream messageStream (message);
        std::getline(messageStream, extracted);
        initialPosition = extracted.find("BEGIN");
        if (initialPosition != std::string::npos){
            //Send a 200 OK with the desired parameters (Ignore the ones proposed)
            Q4SMessage message200;
            Q4SSDPParams params;
            params.qosLevelUp = 0;
            params.qosLevelDown = 0;
            params.size_packet = 1000;
            params.q4SSDPAlertingMode = Q4SSDPALERTINGMODE_Q4SAWARENETWORK;
            params.session_id=1; 
            params.alertPause = 1000;
            params.recoveryPause = 1000;
            params.latency = 100;
            params.jitterUp = 10;
            params.jitterDown = 10;
            params.bandWidthUp = 1000;
            params.bandWidthDown = 3000;
            params.packetLossUp = 2;
            params.packetLossDown = 2;
            params.procedure.negotiationTimeBetweenPingsUplink = 50;
            params.procedure.negotiationTimeBetweenPingsDownlink = 50;
            params.procedure.continuityTimeBetweenPingsUplink = 50;
            params.procedure.continuityTimeBetweenPingsDownlink = 50;
            params.procedure.bandwidthTime = 1000;
            params.procedure.windowSizeLatencyCalcUplink= 30;
            params.procedure.windowSizeLatencyCalcDownlink= 30;
            params.procedure.windowSizePacketLossCalcUplink= 30;
            params.procedure.windowSizePacketLossCalcDownlink= 30;
            ok &= message200.init200OKBeginResponse(params);
            ok &= mServerSocket.sendTcpData( 1, message200.getMessageCChar());
            if(ok){
                int i=0;
                while(mReceivedMessagesTCP.size()==0){
                    i++;
                    sleep(1);
                    if ( i>=100){
                        std::cout<<"Failure, either message was not sent or didn't reach"<<std::endl;
                        pthread_cancel(marrthrHandle[0]);
                        pthread_cancel(marrthrDataHandle[0]);
                        pthread_cancel(marrthrListenHandle[0]);
                        mServerSocket.closeConnection( SOCK_STREAM );
                        return 1;
                    };
                }
                mReceivedMessagesTCP.readFirst( message );
                //Check if the type of the message received is READY
                std::string extracted;
                std::string::size_type initialPosition;
                std::istringstream messageStream (message);
                std::getline(messageStream, extracted);
                initialPosition = extracted.find("READY");
                if (initialPosition != std::string::npos){
                    //Wait for a new message
                    i=0;
                    while(mReceivedMessagesTCP.size()==0){
                        i++;
                        sleep(1);
                        if ( i>=100){
                            std::cout<<"Failure, either READY message was not resent or didn't reach"<<std::endl;
                            pthread_cancel(marrthrHandle[0]);
                            pthread_cancel(marrthrDataHandle[0]);
                            pthread_cancel(marrthrListenHandle[0]);
                            mServerSocket.closeConnection( SOCK_STREAM );
                            return 1;
                        };
                    }
                    mReceivedMessagesTCP.readFirst( message );
                    //Check if the type of the new message is READY
                    std::string extracted;
                    std::string::size_type initialPosition;
                    std::istringstream messageStream (message);
                    std::getline(messageStream, extracted);
                    initialPosition = extracted.find("READY");
                    if (initialPosition != std::string::npos){
                        std::cout<<"Success"<<std::endl;
                        pthread_cancel(marrthrHandle[0]);
                        pthread_cancel(marrthrDataHandle[0]);
                        pthread_cancel(marrthrListenHandle[0]);
                        mServerSocket.closeConnection( SOCK_STREAM );
                        return 0;
                    } else {
                        std::cout<<message<<std::endl;
                        std::cout<<"Failure, the READY message was not sent again"<<std::endl;
                        pthread_cancel(marrthrHandle[0]);
                        pthread_cancel(marrthrDataHandle[0]);
                        pthread_cancel(marrthrListenHandle[0]);
                        mServerSocket.closeConnection( SOCK_STREAM );
                        return 1;
                    }
                } else {
                    std::cout<<message<<std::endl;
                    std::cout<<"Failure, the type of the received message received was not READY"<<std::endl;
                    pthread_cancel(marrthrHandle[0]);
                    pthread_cancel(marrthrDataHandle[0]);
                    pthread_cancel(marrthrListenHandle[0]);
                    mServerSocket.closeConnection( SOCK_STREAM );
                    return 1;
                }
            } else {
                std::cout<<"Failure, the 200 OK message could not be sent"<<std::endl;
                pthread_cancel(marrthrHandle[0]);
                pthread_cancel(marrthrDataHandle[0]);
                pthread_cancel(marrthrListenHandle[0]);
                mServerSocket.closeConnection( SOCK_STREAM );
                return 1;
            }
        } else {
            std::cout<<message<<std::endl;
            std::cout<<"Failure, the type of the received message was not BEGIN"<<std::endl;
            pthread_cancel(marrthrHandle[0]);
            pthread_cancel(marrthrDataHandle[0]);
            pthread_cancel(marrthrListenHandle[0]);
            mServerSocket.closeConnection( SOCK_STREAM );
            return 1;
        }
      } else {
        std::cout<<"Failure, could not open connection and start listening"<<std::endl;
        pthread_cancel(marrthrHandle[0]);
        pthread_cancel(marrthrDataHandle[0]);
        pthread_cancel(marrthrListenHandle[0]);
        mServerSocket.closeConnection( SOCK_STREAM );
        return 1;
      }  
    } else {
      std::cout<<"Failure, could not start manager"<<std::endl;
      pthread_cancel(marrthrHandle[0]);
      pthread_cancel(marrthrDataHandle[0]);
      pthread_cancel(marrthrListenHandle[0]);
      mServerSocket.closeConnection( SOCK_STREAM );
      return 1;
    } 
  } else {
    std::cout<<"Test could not start"<<std::endl;
    return 1;
  }   
}