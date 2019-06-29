#include "../PackageCreator/Q4SMessage.h"
#include "../Socket/Q4SClientSocket.h"
#include "../PackageTester/Q4SMessageManager.h"
#include "../PackageCreator/Q4SMessageTools.h"
#include "../PackageCreator/Q4SMessage.h"
#include "../PackageTester/Q4SMathUtils.h"
#include <vector>
#include <signal.h>
#include <unistd.h>

Q4SClientSocket mClientSocket;
Q4SMessageManager   mReceivedMessagesTCP;
Q4SMessageManager   mReceivedMessagesUDP;
void* manageTcpReceivedData( void* useless  );