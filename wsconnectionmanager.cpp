#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include "wsconnectionmanager.h"

WsConnectionManager::WsConnectionManager(const char* wsUri)
{
    m_wsUri = wsUri;
    m_connectToServerRequired.store(true);
}

WsConnectionManager::~WsConnectionManager()
{
}

// fprintf(stdout, "sent: %ld diff: %ld res: %d c: %d\n",
//         sent, length - sent, res, counter);

CURLcode WsConnectionManager::connect()
{
    m_curl = curl_easy_init();
    curl_easy_setopt(m_curl, CURLOPT_URL, m_wsUri);
    curl_easy_setopt(m_curl, CURLOPT_CONNECT_ONLY, 2L); /* websocket style */
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYSTATUS, 0);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0);
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 2L);
    auto res = curl_easy_perform(m_curl);
    return res;
}

void WsConnectionManager::handleError(CURLcode code)
{
    std::cout << "sendPacket status: " << curl_easy_strerror(code) << std::endl;
    curl_easy_cleanup(m_curl);
    startReconnectTimer();
}

void WsConnectionManager::cacheDataLocally(char *data, int length)
{
    DataForSend dt;
    dt.data = data;
    dt.length = length;
    m_waitingAnswerBuffer.insert({m_sendCounter, dt});
}

void WsConnectionManager::removeDataFromCache(int index)
{
    auto iter = m_waitingAnswerBuffer.find(index);
    if(iter != m_waitingAnswerBuffer.end())
        m_waitingAnswerBuffer.erase(iter);
}

void WsConnectionManager::SendData(char *data, int length)
{
    //Always save data locally first
    cacheDataLocally(data, length);

    auto resConnect = connectToServerIfNeeded();
    if(resConnect == true)
    {
        auto res = sendData(data, length);
        if(res != CURLE_OK)
        {
            if(res != CURLE_AGAIN)
                handleError(res);
        }
    }
}

bool WsConnectionManager::connectToServerIfNeeded()
{
    if (m_connectToServerRequired.load() == true)
    {
        m_connectToServerRequired.store(false);
        auto res = connect();
        if (res != CURLE_OK)
        {
            handleError(res);
            return false;
        }
    }
    return true;
}

void WsConnectionManager::startReconnectTimer()
{
    if (!m_reconnectTimer)
        m_reconnectTimer = make_unique<Timer>();
    m_reconnectTimer->start(chrono::milliseconds(1000), [this]() { 
                                m_connectToServerRequired.store(true); 
                            });
}

CURLcode WsConnectionManager::sendData(char *data, int length)
{
    size_t sent;
    CURLcode res = curl_ws_send(m_curl, data, length, &sent, 0, CURLWS_BINARY);
    if(res != CURLE_OK)
        return res;
    size_t rlen;
    const struct curl_ws_frame *meta;
    //We expect 4 byte int(counter) as answer
    char buffer[20] = {0};
    res = curl_ws_recv(m_curl, buffer, sizeof(buffer), &rlen, &meta);

    if(res == CURLE_OK)
    {


        auto len = strlen(buffer);
        buffer[len - 2] = 0; //ignore last ok


        //Read data
        char *str;
        long index = strtol(buffer, &str, 10);

        if(index > 0)
        {
            //As we definitely know that packet was received, remove it from cache
            std::cout << "We received index: " << index << std::endl;
            removeDataFromCache(index);
        }
    }

    return res;
}
