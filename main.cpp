#include <iostream>
#include <unistd.h>
#include <cstring>
#include "wsconnectionmanager.h"


void startSendData(WsConnectionManager& connManager)
{
    int intervalBetweenPacketsSend = 500000; //50ms
    for (size_t i = 0; i < 1000000; i++)
    {
        std::string s = std::to_string(i);
        char *pchar = const_cast<char*>(s.c_str());
        auto len = strlen(pchar);


        //Send data
        connManager.SendData(pchar, len);

        usleep(intervalBetweenPacketsSend);
    }
}

int main(int argc, char **argv)
{
    WsConnectionManager connManager("ws://host.docker.internal:3000");
    startSendData(connManager);
}
