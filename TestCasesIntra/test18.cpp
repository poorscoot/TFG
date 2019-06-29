#include "test18.h"
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
    //Close connections to prevent failure (Not implemented, crashes)
    if(start){
        manager &= mReceivedMessagesTCP.init( );
        //Create socket to receive BEGIN
        mServerSocket.startAlertSender();
        if(manager){
            //Start listening for connection requests
            connection &= openConnectionListening(); 
            if(connection){
                //Waits until you receive a message
                while(mReceivedMessagesTCP.size()==0){
                    sleep(1);
                }
                std::string message;
                mReceivedMessagesTCP.readFirst( message );
                //Checks if message received is BEGIN
                    std::string extracted;
                    std::string::size_type initialPosition;
                    std::istringstream messageStream (message);
                    std::getline(messageStream, extracted);
                    initialPosition = extracted.find("BEGIN");
                    if (initialPosition != std::string::npos){
                        int i = 0;
                        while(mReceivedMessagesTCP.size()==0){
                            i++;
                            sleep(1);
                            if ( i>=100){
                                std::cout<<"Failure, either BEGIN message was not resent or didn't reach"<<std::endl;
                                pthread_cancel(marrthrHandle[0]);
                                pthread_cancel(marrthrDataHandle[0]);
                                pthread_cancel(marrthrListenHandle[0]);
                                mServerSocket.closeConnection( SOCK_STREAM );
                                return 1;
                            }
                        }
                        std::string message;
                        mReceivedMessagesTCP.readFirst( message );
                        //Waits for another message and checks if the message received is BEGIN
                        if (ok){
                            std::string extracted;
                            std::string::size_type initialPosition;
                            std::istringstream messageStream (message);
                            std::getline(messageStream, extracted);
                            initialPosition = extracted.find("BEGIN");
                            if (initialPosition != std::string::npos){
                                std::cout<<"Success"<<std::endl;
                                pthread_cancel(marrthrHandle[0]);
                                pthread_cancel(marrthrDataHandle[0]);
                                pthread_cancel(marrthrListenHandle[0]);
                                mServerSocket.closeConnection( SOCK_STREAM );
                                return 0;
                            } else {
                                std::cout<<message<<std::endl;
                                std::cout<<"Failure, did not receive a BEGIN message"<<std::endl;
                                pthread_cancel(marrthrHandle[0]);
                                pthread_cancel(marrthrDataHandle[0]);
                                pthread_cancel(marrthrListenHandle[0]);
                                mServerSocket.closeConnection( SOCK_STREAM );
                                return 1;
                            }
                    } else {
                        std::cout<<message<<std::endl;
                        std::cout<<"Failure, did not receive a BEGIN message"<<std::endl;
                        pthread_cancel(marrthrHandle[0]);
                        pthread_cancel(marrthrDataHandle[0]);
                        pthread_cancel(marrthrListenHandle[0]);
                        mServerSocket.closeConnection( SOCK_STREAM );
                        return 1;
                    }
                } else {
                    std::cout<<"Failure, something went wrong"<<std::endl;
                    pthread_cancel(marrthrHandle[0]);
                    pthread_cancel(marrthrDataHandle[0]);
                    pthread_cancel(marrthrListenHandle[0]);
                    mServerSocket.closeConnection( SOCK_STREAM );
                    return 1;
                } 
            } else {
                std::cout<<"Failure, could not open connection to listen"<<std::endl;
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
