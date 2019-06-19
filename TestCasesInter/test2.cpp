#include "test2.h"
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

//Prototype, divide in sub-functions & debug;
//Compares de Q4S Params between a message and the given params
bool compare (std::string message, Q4SSDPParams& params){

    bool ok = true;
    Q4SSDPParams p;
    p.session_id=params.session_id;
    
    //Compare qos values
    std::string pattern = QOSLEVEL_PATTERN;
    std::string firstParamText;
    std::string secondParamText;
    ok &= Q4SSDP_parseTwoElementsLine(message, pattern, "/", firstParamText, secondParamText);
    if (ok)
    {
        int up = std::stoi(firstParamText);
        int down = std::stoi(secondParamText);
        p.qosLevelUp=up;
        p.qosLevelDown=down;
        ok &= (up==params.qosLevelUp);
        ok &= (down==params.qosLevelDown);
    } else {return ok;}
    
    //Compare alerting mode
    pattern = ALERTINGMODE_PATTERN;
    std::string paramText;
    ok &= Q4SSDP_parseOneElementLine (message, pattern, paramText);
    if (ok)
    {
        Q4SSDPAlertingMode alertingMode = Q4SSDP_alertingMode_getFromText(paramText);
        p.q4SSDPAlertingMode=alertingMode;
        ok &= (alertingMode==params.q4SSDPAlertingMode);
    } else {return ok;}

    //Compare alert pause value
    pattern = ALERTPAUSE_PATTERN;
    paramText;
    ok &= Q4SSDP_parseOneElementLine (message, pattern, paramText); 
    if (ok)
    {
        unsigned long appAlertPause = std::stol(paramText);
        p.alertPause=appAlertPause;
        ok &= (appAlertPause==params.alertPause);
    } else {return ok;}
    
    //Compare recovery pause values
    pattern = RECOVERYPAUSE_PATTERN;
    paramText;
    ok &= Q4SSDP_parseOneElementLine (message, pattern, paramText); 
    if (ok)
    {
        unsigned long recoveryPause = std::stol(paramText);
        p.recoveryPause=recoveryPause;
        ok &= (recoveryPause==params.recoveryPause);
    } else {return ok;}
    
    //Compare latency values
    pattern = APPLATENCY_PATTERN;
    paramText;
    ok &= Q4SSDP_parseOneElementLine (message, pattern, paramText); 
    if (ok)
    {
        float latency = std::stof(paramText);
        std::cout<<latency<<std::endl;
        p.latency=latency;
        ok &= ((latency-params.recoveryPause)<=0.1);
    } else {return ok;}
    
    //Compare jitter values
    pattern = APPJITTER_PATTERN;
    firstParamText;
    secondParamText;
    ok &= Q4SSDP_parseTwoElementsLine(message, pattern, "/", firstParamText, secondParamText);
    if (ok)
    {
        float up = std::stof(firstParamText);
        float down = std::stof(secondParamText);
        p.jitterUp=up;
        p.jitterDown=down;
        ok &= ((up-params.jitterUp)<=0.1);
        ok &= ((down-params.jitterDown)<=0.1);
    } else {return ok;}
    
    //Compare BW values
    pattern = APPBANDWIDTH_PATTERN;
    firstParamText;
    secondParamText;
    ok &= Q4SSDP_parseTwoElementsLine(message, pattern, "/", firstParamText, secondParamText);
    if (ok)
    {
        unsigned long up = std::stol(firstParamText);
        unsigned long down = std::stol(secondParamText);
        p.bandWidthUp=up;
        p.bandWidthDown=down;
        ok &= (up==params.bandWidthUp);
        ok &= (down==params.bandWidthDown);
    } else {return ok;}
    
    //Compare PL values
    pattern = APPPACKETLOSS_PATTERN;
    firstParamText;
    secondParamText;
    ok &= Q4SSDP_parseTwoElementsLine(message, pattern, "/", firstParamText, secondParamText);
    if (ok)
    {
        float up = std::stof(firstParamText);
        float down = std::stof(secondParamText);
        p.packetLossUp=up;
        p.packetLossDown=down;
        ok &= ((up-params.packetLossUp)<=0.1);
        ok &= ((down-params.packetLossDown)<=0.1);
    } else {return ok;}
    
    //Compare procedure pattern
    std::string::size_type initialNegotiationTimeBetweenPingsUplinkPosition;
    std::string::size_type betweenNegotiationTimeBetweenPingsPosition;
    std::string::size_type betweenNegotiationContinuityPosition;
    std::string::size_type betweenContinuityTimeBetweenPingsPosition;
    std::string::size_type betweenContinuityBandwidthPosition;
    std::string::size_type betweenBandwidthWindowSizeLatencyCalcUplinkPosition;
    std::string::size_type betweenWindowSizeLatencysPosition;
    std::string::size_type betweenWindowSizeLatencysWindowSizePacketLossesPosition;
    std::string::size_type betweenWindowSizePacketLossesPosition;
    std::string::size_type finalPosition;
    pattern = PROCEDURE_PATTERN;
    if (ok)
    {   
        initialNegotiationTimeBetweenPingsUplinkPosition = message.find(pattern) + pattern.length();
        if (initialNegotiationTimeBetweenPingsUplinkPosition == std::string::npos){
            ok=false;
            return ok;
        }
        betweenNegotiationTimeBetweenPingsPosition = message.find("/", initialNegotiationTimeBetweenPingsUplinkPosition+1);
        if (betweenNegotiationTimeBetweenPingsPosition == std::string::npos){
            ok=false;
            return ok;
        }
        betweenNegotiationContinuityPosition = message.find(",", betweenNegotiationTimeBetweenPingsPosition+1);
        if (betweenNegotiationContinuityPosition == std::string::npos){
            ok=false;
            return ok;
        }
        betweenContinuityTimeBetweenPingsPosition = message.find("/", betweenNegotiationContinuityPosition+1);
        if (betweenContinuityTimeBetweenPingsPosition == std::string::npos){
            ok=false;
            return ok;
        }
        betweenContinuityBandwidthPosition = message.find(",", betweenContinuityTimeBetweenPingsPosition+1);
        if (betweenContinuityBandwidthPosition == std::string::npos){
            ok=false;
            return ok;
        }
        betweenBandwidthWindowSizeLatencyCalcUplinkPosition = message.find(",", betweenContinuityBandwidthPosition+1);
        if (betweenBandwidthWindowSizeLatencyCalcUplinkPosition == std::string::npos){
            ok=false;
            return ok;
        }
        betweenWindowSizeLatencysPosition = message.find("/", betweenBandwidthWindowSizeLatencyCalcUplinkPosition+1);
        if (betweenWindowSizeLatencysPosition == std::string::npos){
            ok=false;
            return ok;
        }
        betweenWindowSizeLatencysWindowSizePacketLossesPosition = message.find(",", betweenWindowSizeLatencysPosition+1);
        if (betweenWindowSizeLatencysWindowSizePacketLossesPosition == std::string::npos){
            ok=false;
            return ok;
        }
        betweenWindowSizePacketLossesPosition = message.find("/", betweenWindowSizeLatencysWindowSizePacketLossesPosition+1);
        if (betweenWindowSizePacketLossesPosition == std::string::npos){
            ok=false;
            return ok;
        }
        finalPosition = message.find(PROCEDURE_CLOSE_PATTERN, betweenWindowSizePacketLossesPosition);
        if (finalPosition == std::string::npos){
            ok=false;
            return ok;
        }
        if(ok){
            p.procedure.negotiationTimeBetweenPingsUplink = std::stol(message.substr(initialNegotiationTimeBetweenPingsUplinkPosition, betweenNegotiationTimeBetweenPingsPosition - initialNegotiationTimeBetweenPingsUplinkPosition));;
            p.procedure.negotiationTimeBetweenPingsDownlink = std::stol(message.substr(betweenNegotiationTimeBetweenPingsPosition + 1, betweenNegotiationContinuityPosition - (betweenNegotiationTimeBetweenPingsPosition + 1 )));
            p.procedure.continuityTimeBetweenPingsUplink = std::stol(message.substr(betweenNegotiationContinuityPosition + 1, betweenContinuityTimeBetweenPingsPosition - (betweenNegotiationContinuityPosition + 1 )));
            p.procedure.continuityTimeBetweenPingsDownlink = std::stol(message.substr(betweenContinuityTimeBetweenPingsPosition + 1, betweenContinuityBandwidthPosition - (betweenContinuityTimeBetweenPingsPosition + 1 )));
            p.procedure.bandwidthTime = std::stol(message.substr(betweenContinuityBandwidthPosition + 1, betweenBandwidthWindowSizeLatencyCalcUplinkPosition - (betweenContinuityBandwidthPosition + 1 )));
            p.procedure.windowSizeLatencyCalcUplink= std::stol(message.substr(betweenBandwidthWindowSizeLatencyCalcUplinkPosition + 1, betweenWindowSizeLatencysPosition - (betweenBandwidthWindowSizeLatencyCalcUplinkPosition + 1 )));
            p.procedure.windowSizeLatencyCalcDownlink= std::stol(message.substr(betweenWindowSizeLatencysPosition + 1, betweenWindowSizeLatencysWindowSizePacketLossesPosition - (betweenWindowSizeLatencysPosition + 1 )));
            p.procedure.windowSizePacketLossCalcUplink= std::stol(message.substr(betweenWindowSizeLatencysWindowSizePacketLossesPosition + 1, betweenWindowSizePacketLossesPosition - (betweenWindowSizeLatencysWindowSizePacketLossesPosition + 1 )));
            p.procedure.windowSizePacketLossCalcDownlink= std::stol(message.substr(betweenWindowSizePacketLossesPosition + 1, finalPosition - (betweenWindowSizePacketLossesPosition + 1 )));
        }
    } else {return ok;}

    //Compare packet lenght
    pattern ="a=max-content-length:";
    paramText;
    ok &= Q4SSDP_parseOneElementLine (message, pattern, paramText); 
    if (ok)
    {   
        int packet_length = std::stoi(paramText);
        p.size_packet=packet_length;
        ok &= (packet_length==params.size_packet);
    } else {return ok;}
    return ok;
    
}

int main () {
    manager &= mReceivedMessagesTCP.init( );
    
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
        message.initRequest(Q4SMTYPE_BEGIN, "127.0.0.1", "27015", false, 0, false, 0, false, 0, false, NULL, true, &Proposal_params);
    //Send BEGIN package
        //Create TCP port
        //Create TCP connection to the same port as protocol
        //Send Begin
        
        connection &= mClientSocket.openConnection( SOCK_STREAM );

        // launch received data managing threads.
        thread_error = pthread_create( &marrthrHandle[1], NULL, manageTcpReceivedData ,NULL);

        if (connection){
            tryTCP &= mClientSocket.sendTcpData(message.getMessageCChar());

            //Receive ACK to BEGIN
                //Wait until socket recieves answer
                if ( tryTCP ) 
                {
                    sleep(10);
                    std::string message;
                    mReceivedMessagesTCP.readFirst( message );            
                    tryTCP = Q4SMessageTools_is200OKMessage(message,false, 0, 0);
                    if (tryTCP){
                        ok &= compare(message, Proposal_params);
                        if (ok){
                            std::cout<<"Success"<<std::endl;
                            return 0;
                        } else {
                            std::cout<<"Failure"<<std::endl;
                            return 1;
                        }
                    } else {
                        std::cout<<"Failure"<<std::endl;
                        return 1;
                    }
                }
        }
    }

}