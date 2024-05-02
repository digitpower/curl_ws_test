#pragma once
#include <curl/curl.h>
#include <map>
#include <thread>
#include <atomic>
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
    void cacheDataLocally(char *data, int length);
    void removeDataFromCache(int index);
    CURLcode connect(const char* wssUri);
    CURLcode sendData(char *data, int length, int counter);
    void closeGracefully() {}
    double waitForAnswer(int maxWait, int sockFd);
private:
    CURL* m_curl = nullptr;
    long m_sockfd = 0;

    struct DataForSend {
        char* data;
        int length;
        int _cnt;
    };
    std::map<u_int64_t, DataForSend> m_waitingAnswerBuffer;
    int m_sendCounter = 0;
    code_machina::BlockingCollection<DataForSend> m_sendingPacketsBuffer;
};