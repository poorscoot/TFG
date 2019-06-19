#include "../PackageCreator/Q4SMessage.h"
#include "../Socket/Q4SClientSocket.h"
#include "../PackageTester/Q4SMessageManager.h"
#include "../PackageCreator/Q4SMessageTools.h"
#include "../PackageCreator/Q4SMessage.h"
#include "../PackageCreator/Q4SSDP.h"
#include "../PackageCreator/Q4SSDPParams.h"
#include <vector>
#include <signal.h>
#include <unistd.h>

Q4SClientSocket mClientSocket;
Q4SMessageManager   mReceivedMessagesTCP;
void* manageTcpReceivedData( void* useless );
bool compare (std::string message, Q4SSDPParams& Params);
