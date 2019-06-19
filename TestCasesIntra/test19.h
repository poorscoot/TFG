#include "../PackageCreator/Q4SMessage.h"
#include "../Socket/Q4SServerSocket.h"
#include "../PackageTester/Q4SMessageManager.h"
#include "../PackageCreator/Q4SMessageTools.h"
#include "../PackageCreator/Q4SMessage.h"
#include <vector>
#include <signal.h>
#include <unistd.h>

Q4SServerSocket mServerSocket;
Q4SMessageManager   mReceivedMessagesTCP;
void* manageTcpReceivedData( void* useless  );