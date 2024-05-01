#include <iostream>
#include <unistd.h>
#include <cstring>
#include "wsconnectionmanager.h"


void startSendData(WsConnectionManager& connManager)
{
    int intervalBetweenPacketsSend = 1000000; //50ms
    for (size_t i = 0; i < 1000000; i++)
    {
        std::string s = std::to_string(i);
        char *pchar = const_cast<char*>(s.c_str());
        auto len = strlen(pchar);


        //Send data
        char* dd = new char[len];
        strncpy(dd, pchar, len);
        connManager.SendData(dd, len, i);

        usleep(intervalBetweenPacketsSend);
    }
}

int main(int argc, char **argv)
{
    WsConnectionManager connManager;
    connManager.StartSending("ws://host.docker.internal:3000");
    startSendData(connManager);
}
