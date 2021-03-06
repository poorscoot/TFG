#include "test3.h"
#include <stdio.h>
#include <sstream>
//#include <math.h>
//#include <cmath>
#include <cstdlib>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

bool start = true;
bool tryTCP = true;
bool connection = true;
bool manager = true;
bool tryStage0 = true;
bool stage0 = true;
bool pings = true;
bool interchange = true;
bool stage0ok = true;
bool tryStage1 = true;
bool stage1 = true;
int thread_error;
pthread_t   marrthrHandle[ 2 ];
Q4SSDPParams params; 

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



bool sendRegularPings(std::vector<uint64_t> &arrSentPingTimestamps, unsigned long pingsToSend, unsigned long timeBetweenPings)
{
    bool ok = true;

    Q4SMessage message;
    uint64_t timeStamp = 0;
    int pingNumber = 0;
    int pingNumberToSend = pingsToSend;
    int time_error ;
    struct timeval time_s;
    for ( pingNumber = 0; pingNumber < pingNumberToSend; pingNumber++ )
    {
        //std::cout<<"ping"<<pingNumber<<std::endl;
        // Store the timestamp
        time_error = gettimeofday(&time_s, NULL); 
        timeStamp =  time_s.tv_sec*1000 + time_s.tv_usec/1000;
        //timeStamp = (unsigned long) 20; 
        //timeStamp = ETime_getTime( );
        
        arrSentPingTimestamps.push_back( timeStamp );

        // Prepare message and send
        message.initPing("127.0.0.1", "56000", pingNumber, timeStamp);
        ok &= mClientSocket.sendUdpData( message.getMessageCChar() );

        // Wait the established time between pings

        
        usleep(timeBetweenPings*1000 );
    }
   

    return ok;
}

void calculateLatency(Q4SMessageManager &mReceivedMessages, std::vector<uint64_t> &arrSentPingTimestamps, float &latency, unsigned long pingsSent, bool showMeasureInfo)
{
    Q4SMessageInfo messageInfo;
    std::vector<float> arrPingLatencies;
    float actualPingLatency;
    int pingIndex = 0;
    int pingMaxCount = pingsSent;
    int sequenceNumberPing; 
    uint64_t TimeStampPing; 
    // Prepare for Latency calculation
    for( pingIndex = 0; pingIndex < pingMaxCount && mReceivedMessages.size()>0; pingIndex++ )
    {
        if( mReceivedMessages.read200OKMessage( messageInfo, true, &TimeStampPing, &sequenceNumberPing ) == true )
        {
            // Actual ping latency calculation
            //printf("TIEMPO 200 0k: %"PRIu64"\nTIEMPO PING: %"PRIu64"\n",messageInfo.timeStamp, TimeStampPing);

            
            actualPingLatency = (messageInfo.timeStamp - TimeStampPing)/ 2.0f;
            //printf("TIEMPO 200 0k: %lu\nTIEMPO PING: %lu\n",messageInfo.timeStamp, arrSentPingTimestamps[ pingIndex ]);
            // Latency store
            arrPingLatencies.push_back( actualPingLatency );

            #if SHOW_INFO2
            
                printf( "PING %d actual ping latency: %.3f\n", pingIndex, actualPingLatency );
            #endif
        }
        else
        {
            #if SHOW_INFO2
                printf( "PING RESPONSE %d message lost\n", pingIndex);
            #endif
        }
    }

    // Latency calculation
    latency = EMathUtils_median( arrPingLatencies );
}

void calculateJitter(Q4SMessageManager &mReceivedMessages, float &jitter, unsigned long timeBetweenPings, unsigned long pingsSent, bool calculatePacketLoss, float &packetLoss, bool showMeasureInfo) 
{
    int pingIndex = 0;
    int pingMaxCount = pingsSent;
    Q4SMessageInfo messageInfo;
    std::vector<uint64_t> arrReceivedPingTimestamps;
    std::vector<unsigned long> arrPingJitters={};
    bool firstPing = true; 
    uint64_t actualPingTimeWithPrevious;
    unsigned long packetLossCount = 0;
    int indexReceived= 0; 


    for( pingIndex = 0; pingIndex < pingMaxCount && mReceivedMessages.size()>0; pingIndex++ )
    {
        if( mReceivedMessages.readPingMessage( pingIndex, messageInfo, true ) == true )
        {
            arrReceivedPingTimestamps.push_back( messageInfo.timeStamp );
            if( !firstPing)
            {
                unsigned long delAbs = arrReceivedPingTimestamps[ indexReceived ] - arrReceivedPingTimestamps[ indexReceived - 1 ];
                actualPingTimeWithPrevious = /*std::abs( delAbs ) */delAbs;
                arrPingJitters.push_back( (unsigned long)abs((double)actualPingTimeWithPrevious - (double)timeBetweenPings) );
                #if SHOW_INFO2
                    printf( "PING %d ET: %" PRIu64 "\n", pingIndex, actualPingTimeWithPrevious );
                #endif
            }
            firstPing= false; 
            indexReceived++; 
        }
        else
        {
            if (calculatePacketLoss)
            {
                packetLossCount++;
            }
            if (!firstPing)
            {
                uint64_t timeOfPacketLoss = arrReceivedPingTimestamps[ indexReceived - 1 ] + timeBetweenPings;
                arrReceivedPingTimestamps.push_back(timeOfPacketLoss);
                indexReceived++; 

            }
            #if SHOW_INFO2
                printf( "PING %d message lost\n", pingIndex);
            #endif
        }
    }
    jitter = EMathUtils_mean( arrPingJitters );

    if (calculatePacketLoss)
    {
        packetLoss = ((float)(pingMaxCount-indexReceived+packetLossCount)/ (float)(pingsSent)) * 100.f;
    }

    #if SHOW_INFO2
        printf( "Time With previous ping mean: %.3f\n", jitter );
    #endif
}

void calculateJitterStage0(Q4SMessageManager &mReceivedMessages, float &jitter, unsigned long timeBetweenPings, unsigned long pingsSent, bool showMeasureInfo)
{
    float packetLoss = 0.f;
    calculateJitter(mReceivedMessages, jitter, timeBetweenPings, pingsSent, false, packetLoss, showMeasureInfo);
}

bool interchangeMeasurementProcedure(Q4SMeasurementValues &downMeasurements, Q4SMeasurementResult results)
{
    bool ok = true;
    while(mReceivedMessagesUDP.size()>0)
    {
        //printf("BORRANDO MENSAJES\n");
        mReceivedMessagesUDP.eraseMessages();
    }
    if ( ok ) 
    {
        // Send Info Ping with sequenceNumber 0
        Q4SMessage infoPingMessage;
        ok &= infoPingMessage.initPing("127.0.0.1", "56001", 0, 0, true, &results.values);
        ok &= mClientSocket.sendTcpData( infoPingMessage.getMessageCChar() );
    }
    
    if (ok)
    {
        // Wait to recive the measurements Ping
        Q4SMessageInfo  messageInfo={};
        ok= false; 
        while(!ok)
        {

            ok = mReceivedMessagesTCP.readPingMessage( 0, messageInfo, true );
            if (ok)
            {
                ok &= Q4SMeasurementValues_parse(messageInfo.message, downMeasurements);
                if (!ok)
                {
                    printf( "ERROR:Interchange Read measurements fail\n");
                }
            }
          
        }
    }

    return ok;
}


bool checkStage0(float maxLatency, float maxJitter, Q4SMeasurementResult &results)
{
    bool ok = true;

    if ( results.values.latency > maxLatency )
    {
        results.latencyAlert = true;
        #if SHOW_INFO
            printf( "Lantecy limits not reached: %.3f\n", maxLatency);
        #endif
        ok = false;
    }

    if ( results.values.jitter > maxJitter)
    {
        results.jitterAlert = true;
        #if SHOW_INFO
            printf( "Jitter limits not reached: %.3f\n", maxJitter);
        #endif
        ok = false;
    }

    return ok;
}

bool checkStage0(float maxLatencyUp, float maxJitterUp, float maxLatencyDown, float maxJitterDown, Q4SMeasurementResult &upResults, Q4SMeasurementResult &downResults)
{
    bool ok = true;

    ok &= checkStage0(maxLatencyUp, maxJitterUp, upResults);

    ok &= checkStage0(maxLatencyDown, maxJitterDown, downResults);

    return ok;
}

int main () {
    start &= pthread_cancel(marrthrHandle[0]);
    start &= pthread_cancel(marrthrHandle[1]);
    start &= mClientSocket.closeConnection(SOCK_STREAM);
    start &= mClientSocket.closeConnection( SOCK_DGRAM );
    if (start){
        manager &= mReceivedMessagesTCP.init();
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
            //The IP and port values are taken from the implementations
            Q4SMessage  message;
            message.initRequest(Q4SMTYPE_BEGIN, "127.0.0.1", "56001", false, 0, false, 0, false, 0, false, NULL, true, &Proposal_params);
        //Send BEGIN package
            //Create TCP port
            //Create TCP connection to the same port as protocol
            //Send Begin
            
            connection &= mClientSocket.openConnection( SOCK_STREAM );
            connection &= mClientSocket.openConnection( SOCK_DGRAM );

            // launch received data managing threads.
            thread_error = pthread_create( &marrthrHandle[0], NULL, manageUdpReceivedData, NULL);
            thread_error = pthread_create( &marrthrHandle[1], NULL, manageTcpReceivedData, NULL);

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
                            message.initRequest(Q4SMTYPE_READY, "127.0.0.1", "56001", false, 0, false, 0, true, 0);
                            while(mReceivedMessagesUDP.size()>0)
                            {
                                mReceivedMessagesUDP.eraseMessages();
                            }
                            tryStage0 &= mClientSocket.sendTcpData( message.getMessageCChar() );
                            if(tryStage0){
                                Q4SMeasurementResult results;
                                Q4SMeasurementResult downResults;
                                mReceivedMessagesTCP.readFirst( messageR );
                                stage0 &= Q4SMessageTools_is200OKMessage(messageR, false, 0, 0); 
                                if (stage0){
                                    Q4SSDP_parse(messageR, params);
                                    std::vector<uint64_t> arrSentPingTimestamps;
                                    Q4SMeasurementValues downMeasurements;
                                    char data2save[200]={};
                                    unsigned long pingsToSend = params.procedure.windowSizeLatencyCalcDownlink;
                                    pings &= sendRegularPings(arrSentPingTimestamps, pingsToSend, params.procedure.negotiationTimeBetweenPingsUplink);
                                    usleep(50*1000*pingsToSend);
                                    if (pings){
                                        usleep(params.procedure.negotiationTimeBetweenPingsUplink *1000);
                                        // Calculate Latency
                                        calculateLatency(mReceivedMessagesUDP, arrSentPingTimestamps, results.values.latency, pingsToSend, false);
                                        float packetLoss = 0.f;
                                        calculateJitterStage0(mReceivedMessagesUDP, results.values.jitter,params.procedure.negotiationTimeBetweenPingsUplink, pingsToSend, false);
                                        results.values.packetLoss= 0; 
                                        results.values.bandwidth= 0; 
                                        interchange &= interchangeMeasurementProcedure(downMeasurements, results);
                                        if (interchange){
                                            downResults.values = downMeasurements;
                                            stage0ok &= checkStage0(params.latency, params.jitterUp, params.latency, params.jitterDown, results, downResults);
                                            if(stage0ok){
                                                message.initRequest(Q4SMTYPE_READY, "127.0.0.1", "56001", false, 0, false, 0, true, 1);
                                                while(mReceivedMessagesUDP.size()>0)
                                                {
                                                    mReceivedMessagesUDP.eraseMessages();
                                                }
                                                tryStage1 &= mClientSocket.sendTcpData( message.getMessageCChar() );
                                                if(tryStage1){
                                                    while(mReceivedMessagesTCP.size()==0){
                                                        sleep(1);
                                                    }
                                                    mReceivedMessagesTCP.readFirst( messageR );
                                                    stage1 &= Q4SMessageTools_is200OKMessage(messageR, false, 0, 0); 
                                                    if (stage1){
                                                        

                                                    } else {
                                                        std::cout<<messageR<<std::endl;
                                                        std::cout<<"Did not receive 200 OK after requesting Stage 1"<<std::endl;
                                                        pthread_cancel(marrthrHandle[0]);
                                                        pthread_cancel(marrthrHandle[1]);
                                                        mClientSocket.closeConnection(SOCK_STREAM);
                                                        mClientSocket.closeConnection( SOCK_DGRAM );
                                                        return 1;
                                                    }
                                                } else {
                                                    std::cout<<"Could not send Stage 1"<<std::endl;
                                                    pthread_cancel(marrthrHandle[0]);
                                                    pthread_cancel(marrthrHandle[1]);
                                                    mClientSocket.closeConnection(SOCK_STREAM);
                                                    mClientSocket.closeConnection( SOCK_DGRAM );
                                                    return 1;
                                                }
                                            } else {
                                                std::cout<<"Stage 0 did not meet requirements"<<std::endl;
                                                pthread_cancel(marrthrHandle[0]);
                                                pthread_cancel(marrthrHandle[1]);
                                                mClientSocket.closeConnection(SOCK_STREAM);
                                                mClientSocket.closeConnection( SOCK_DGRAM );
                                                return 1;
                                            }
                                        } else {
                                            std::cout<<"Could not interchange measures"<<std::endl;
                                            pthread_cancel(marrthrHandle[0]);
                                            pthread_cancel(marrthrHandle[1]);
                                            mClientSocket.closeConnection(SOCK_STREAM);
                                            mClientSocket.closeConnection( SOCK_DGRAM );
                                            return 1;
                                        }
                                    } else {
                                        std::cout<<"Coud not measure Stage 0"<<std::endl;
                                        pthread_cancel(marrthrHandle[0]);
                                        pthread_cancel(marrthrHandle[1]);
                                        mClientSocket.closeConnection(SOCK_STREAM);
                                        mClientSocket.closeConnection( SOCK_DGRAM );
                                        return 1;
                                    }
                                } else {
                                    std::cout<<"Could not start Stage 0"<<std::endl;
                                    pthread_cancel(marrthrHandle[0]);
                                    pthread_cancel(marrthrHandle[1]);
                                    mClientSocket.closeConnection(SOCK_STREAM);
                                    mClientSocket.closeConnection( SOCK_DGRAM );
                                    return 1;
                                }

                            } else {
                                std::cout<<"Could not send Ready 0"<<std::endl;
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

/* 
std::cout<<"Success"<<std::endl;
                                                        pthread_cancel(marrthrHandle[0]);
                                                        pthread_cancel(marrthrHandle[1]);
                                                        mClientSocket.closeConnection(SOCK_STREAM);
                                                        mClientSocket.closeConnection( SOCK_DGRAM );
                                                        return 0;
                                                        */