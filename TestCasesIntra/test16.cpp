#include "test16.h"
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
    //Create manager
    manager &= mReceivedMessagesTCP.init( );
    //Create socket to receive BEGIN
    mServerSocket.startAlertSender();
    if(manager){
      //Start listening for connection requests
      connection &= openConnectionListening(); 
      if(connection){
        //Wait until a message is received
        while(mReceivedMessagesTCP.size()==0){
          sleep(1);
        }
        std::string message;
        //Reads the message
        mReceivedMessagesTCP.readFirst( message );
        //Check if the message received is a BEGIN
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
            } else{
              std::cout<<message<<std::endl;
              std::cout<<"Failure, the message received was not a BEGIN"<<std::endl;
              pthread_cancel(marrthrHandle[0]);
              pthread_cancel(marrthrDataHandle[0]);
              pthread_cancel(marrthrListenHandle[0]);
              mServerSocket.closeConnection( SOCK_STREAM );
              return 1;
            }
        }
      }  else {
          std::cout<<"Could not start listening"<<std::endl;
          pthread_cancel(marrthrHandle[0]);
          pthread_cancel(marrthrDataHandle[0]);
          pthread_cancel(marrthrListenHandle[0]);
          mServerSocket.closeConnection( SOCK_STREAM );
      }
    } else {
        std::cout<<"Could not create TCP messages manager"<<std::endl;
        pthread_cancel(marrthrHandle[0]);
        pthread_cancel(marrthrDataHandle[0]);
        pthread_cancel(marrthrListenHandle[0]);
        mServerSocket.closeConnection( SOCK_STREAM );
    }
  } else {
        std::cout<<"Test could not start"<<std::endl;
        return 1;
  }  
}