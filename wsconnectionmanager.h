#pragma once
#include <curl/curl.h>
#include <map>
#include <thread>
#include <atomic>
#include "reconnecttimer.h"


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
    WsConnectionManager(const char* wsUri);
    ~WsConnectionManager();
    void SendData(char *data, int length);
private:
    void cacheDataLocally(char *data, int length);
    void removeDataFromCache(int index);
    bool connectToServerIfNeeded();
    void handleError(CURLcode code);
    CURLcode connect();
    void startReconnectTimer();
    CURLcode sendData(char *data, int length);
    void closeGracefully() {}
private:
    CURL* m_curl = nullptr;
    long m_sockfd = 0;

    struct DataForSend {
        char* data;
        int length;
    };
    std::map<u_int64_t, DataForSend> m_waitingAnswerBuffer;
    int m_sendCounter = 0;
    const char* m_wsUri = nullptr;
    std::unique_ptr<Timer> m_reconnectTimer;
    std::atomic<bool> m_connectToServerRequired = false;
};