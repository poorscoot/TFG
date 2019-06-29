#include "test4.h"
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
bool alertM = false;
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

                // mandar respuesta del ping
                char reasonPhrase[ 256 ];
                Q4SMessage message200;
                sprintf( reasonPhrase, "Q4S/1.0 200 OK\r\nSequence-Number:%d\r\nTimestamp:%" PRIu64 "\r\nContent-Length:0\r\n\r\n", pingNumber,receivedTimeStamp );
                ok &= mClientSocket.sendUdpData( reasonPhrase );
            }
            mReceivedMessagesUDP.addMessage(message, actualTimeStamp);

        }
    }
}

//Function that manages Stage 0
//Modified to send less PINGs than agreed upon
//Taken from the hpcn-uam implementation
bool sendRegularPings(std::vector<uint64_t> &arrSentPingTimestamps, unsigned long pingsToSend, unsigned long timeBetweenPings)
{
    bool ok = true;

    Q4SMessage message;
    uint64_t timeStamp = 0;
    int pingNumber = 0;
    int pingNumberToSend = pingsToSend;
    int time_error ;
    struct timeval time_s;
    for ( pingNumber = 0; pingNumber < pingNumberToSend; pingNumber=pingNumber+10)
    {
        // Store the timestamp
        time_error = gettimeofday(&time_s, NULL); 
        timeStamp =  time_s.tv_sec*1000 + time_s.tv_usec/1000;
        
        arrSentPingTimestamps.push_back( timeStamp );

        // Prepare message and send
        message.initPing("127.0.0.1", "56000", pingNumber, timeStamp);
        ok &= mClientSocket.sendUdpData( message.getMessageCChar() );

        // Wait the established time between pings

        
        usleep(timeBetweenPings*1000*2 );
    }
   

    return ok;
}


//Taken from the hpcn-uam implementation
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
            actualPingLatency = 300;
            // Latency store
            arrPingLatencies.push_back( actualPingLatency );
        }
    }
    // Latency calculation
    latency = EMathUtils_median( arrPingLatencies );
}

//Taken from the hpcn-uam implementation
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
        }
    }
    jitter = EMathUtils_mean( arrPingJitters );

    if (calculatePacketLoss)
    {
        packetLoss = ((float)(pingMaxCount-indexReceived+packetLossCount)/ (float)(pingsSent)) * 100.f;
    }
}

//Taken from the hpcn-uam implementation
void calculateJitterStage0(Q4SMessageManager &mReceivedMessages, float &jitter, unsigned long timeBetweenPings, unsigned long pingsSent, bool showMeasureInfo)
{
    float packetLoss = 0.f;
    calculateJitter(mReceivedMessages, jitter, timeBetweenPings, pingsSent, false, packetLoss, showMeasureInfo);
}

//Taken from the hpcn-uam implementation
bool interchangeMeasurementProcedure(Q4SMeasurementValues &downMeasurements, Q4SMeasurementResult results)
{
    bool ok = true;
    while(mReceivedMessagesUDP.size()>0)
    {
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

//Taken from the hpcn-uam implementation
bool checkStage0(float maxLatency, float maxJitter, Q4SMeasurementResult &results)
{
    bool ok = true;

    if ( results.values.latency > maxLatency )
    {
        results.latencyAlert = true;
        ok = false;
    }

    if ( results.values.jitter > maxJitter)
    {
        results.jitterAlert = true;
        ok = false;
    }

    return ok;
}

//Taken from the hpcn-uam implementation
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
            
            //Create a TCP and a UDP connection with the server
            connection &= mClientSocket.openConnection( SOCK_STREAM );
            connection &= mClientSocket.openConnection( SOCK_DGRAM );

            //Create threads to manage the TCP and UDP data
            thread_error = pthread_create( &marrthrHandle[0], NULL, manageUdpReceivedData, NULL);
            thread_error = pthread_create( &marrthrHandle[1], NULL, manageTcpReceivedData, NULL);

            if (connection){
                //Send the BEGIN
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
                        std::string messageR;
                        mReceivedMessagesTCP.readFirst( messageR );            
                        tryTCP = Q4SMessageTools_is200OKMessage(messageR,false, 0, 0);
                        if (tryTCP){
                            //Create a READY 0 request
                            message.initRequest(Q4SMTYPE_READY, "127.0.0.1", "56001", false, 0, false, 0, true, 0);
                            while(mReceivedMessagesUDP.size()>0)
                            {
                                mReceivedMessagesUDP.eraseMessages();
                            }
                            //Send the request
                            tryStage0 &= mClientSocket.sendTcpData( message.getMessageCChar() );
                            if(tryStage0){
                                Q4SMeasurementResult results;
                                Q4SMeasurementResult downResults;
                                //Wait for an answer
                                i = 0;
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
                                mReceivedMessagesTCP.readFirst( messageR );
                                stage0 &= Q4SMessageTools_is200OKMessage(messageR, false, 0, 0); 
                                if (stage0){
                                    //Measure Stage 0
                                    Q4SSDP_parse(messageR, params);
                                    std::vector<uint64_t> arrSentPingTimestamps;
                                    Q4SMeasurementValues downMeasurements;
                                    char data2save[200]={};
                                    unsigned long pingsToSend = params.procedure.windowSizeLatencyCalcDownlink;
                                    pings &= sendRegularPings(arrSentPingTimestamps, pingsToSend, params.procedure.negotiationTimeBetweenPingsUplink);
                                    if (pings){
                                        usleep(params.procedure.negotiationTimeBetweenPingsUplink *1000);
                                        // Calculate Latency
                                        calculateLatency(mReceivedMessagesUDP, arrSentPingTimestamps, results.values.latency, pingsToSend, false);
                                        float packetLoss = 0.f;
                                        calculateJitterStage0(mReceivedMessagesUDP, results.values.jitter,params.procedure.negotiationTimeBetweenPingsUplink, pingsToSend, false);
                                        //Send measurements that do not comply, modify to fit the proposed params or the ones given by the server
                                        results.values.latency= 1000;
                                        results.values.jitter= 100;
                                        results.values.packetLoss= 0; 
                                        results.values.bandwidth= 0;
                                        interchange &= interchangeMeasurementProcedure(downMeasurements, results);
                                        if (interchange){
                                            //Wait for an ALERT message to reach
                                            i=0;
                                            while(true){
                                                while(mReceivedMessagesTCP.size()==0){
                                                    sleep(1);
                                                    i++;
                                                    if(i>=100){
                                                        std::cout<<"Failure, did not receive an ALERT"<<std::endl;
                                                        pthread_cancel(marrthrHandle[0]);
                                                        pthread_cancel(marrthrHandle[1]);
                                                        mClientSocket.closeConnection(SOCK_STREAM);
                                                        mClientSocket.closeConnection( SOCK_DGRAM );
                                                        return 1;
                                                    }
                                                }
                                                mReceivedMessagesTCP.readFirst( messageR );
                                                if (!alertM){
                                                    std::string extracted;
                                                    std::string::size_type initialPosition;
                                                    std::istringstream messageStream (messageR);
                                                    std::getline(messageStream, extracted);
                                                    initialPosition = extracted.find("ALERT");
                                                    if (initialPosition != std::string::npos){
                                                        alertM = true;
                                                    } else{
                                                        alertM = false;
                                                    }
                                                }
                                                if(alertM){
                                                    std::cout<<"Success"<<std::endl;
                                                    pthread_cancel(marrthrHandle[0]);
                                                    pthread_cancel(marrthrHandle[1]);
                                                    mClientSocket.closeConnection(SOCK_STREAM);
                                                    mClientSocket.closeConnection( SOCK_DGRAM );
                                                    return 0;
                                                }
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
