CC=g++
HEADERS_COMMON= ../Socket/ConfigFile.h ../Socket/Q4SClientConfigFile.h ../Socket/Q4SClientSocket.h ../Socket/Q4SServerConfigFile.h ../Socket/Q4SServerSocket.h ../Socket/Q4SSocket.h ../PackageCreator/Q4SMessageInfo.h ../PackageCreator/Q4SMessageTools.h ../PackageCreator/Q4SSDP.h ../PackageCreator/Q4SSDPParams.h ../PackageCreator/Q4SStructs.h  ../PackageTester/Q4SMessageInfo.h ../PackageTester/Q4SMessageManager.h
FILE_COMMON= ../Socket/ConfigFile.cpp ../Socket/Q4SClientConfigFile.cpp ../Socket/Q4SClientSocket.cpp ../Socket/Q4SServerConfigFile.cpp ../Socket/Q4SServerSocket.cpp ../Socket/Q4SSocket.cpp ../PackageCreator/Q4SMessage.cpp ../PackageCreator/Q4SMessageTools.cpp ../PackageCreator/Q4SSDP.cpp ../PackageCreator/Q4SSDPParams.cpp ../PackageCreator/Q4SStructs.cpp ../PackageTester/Q4SMessageManager.cpp 
DEBUG_FLAGS= -DDEBUG
FLAGS= -pthread -std=c++11
NUMBERSINTER=1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
all:
	$(foreach i, $(NUMBERSINTER), $(CC) -O3 $(HEADERS_COMMON) test$(i).h $(FILE_COMMON) test$(i).cpp $(DEBUG_FLAGS) $(FLAGS) -o test$(i) ;)	
clean:
	$(foreach i, $(NUMBERSINTER), rm -rf test$i *~* ;)


