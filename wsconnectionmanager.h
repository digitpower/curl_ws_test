#pragma once
#include <curl/curl.h>
#include <map>
#include <thread>
#include <atomic>
#include "packetscache.h"
#include "audiopacket.h"
#include "BlockingCollection.h"

class WsConnectionManager
{
public:
    enum class SendResult
    {
        SEND_OK = 0,
        SEND_ERROR = 1,
        GIVEUP_RECONNECT_ATTEMPTS = 2,
        RECONNECTED = 3
    };
public:
    WsConnectionManager();
    ~WsConnectionManager();
    void StartSending(const char* wssUri);
    void SendData(char *data, int length, int sendDounter);
private:
    CURLcode connect(const char* wssUri);
    CURLcode sendData(char *data, int length, int counter);
    CURLcode receiveData(int sockfd, bool& timeOutDetectedOnReceive);

    void cleanupConnection(bool& connected);
    void closeGracefully() {}
private:
    CURL* m_curl = nullptr;
    long m_sockfd = 0;

    CURLcode handlePacketSendReceive(int sockfd, 
        DataForSend& dt,
        bool& connected);
    PacketsCache m_sendingPacketsBuffer;
};