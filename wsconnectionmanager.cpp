#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <future>
#include <thread>
#include <chrono>
#include "wsconnectionmanager.h"

WsConnectionManager::WsConnectionManager()
{
}

WsConnectionManager::~WsConnectionManager()
{
}

void WsConnectionManager::StartSending(const char* wssUri)
{
    new std::thread([=] {
        curl_global_init(CURL_GLOBAL_ALL);
        bool connected = false;
        while(true)
        {
            auto res = connect(wssUri);
            if (res != CURLE_OK)
            {
                // std::cout << "Connect failed. Packet count: " << m_sendingPacketsBuffer.size() << "\n";
                curl_easy_cleanup(m_curl);
                usleep(2000000);
                continue;
            }
            connected = true;
            int sockfd = 0;
            res = curl_easy_getinfo(m_curl, CURLINFO_ACTIVESOCKET, &sockfd);
            
            while(connected == true)
            {
                DataForSend dt;
                auto status = m_sendingPacketsBuffer.take(dt);
                char* data = dt.data; 
                int length = dt.length;
                cacheDataLocally(data, length);
                if(status == code_machina::BlockingCollectionStatus::Ok)
                {
                    res = sendData(data, length, dt._cnt);
                    if(res != CURLE_OK)
                    {
                        if(res != CURLE_AGAIN)
                        {
                            connected = false;
                            curl_easy_cleanup(m_curl);
                            usleep(2000000);
                        }
                        else
                        {
                            // while(CURLE_AGAIN == res)
                            // {
                                printf("CURLE_AGAIN detected for index %d\n", dt._cnt);
                                // auto res = waitForAnswer(10000000, sockfd);
                                // std::cout << "waited for " << res << " microseconds" << std::endl;
                            // }
                        }
                    }
                }
            }
        }
    });
}

// fprintf(stdout, "sent: %ld diff: %ld res: %d c: %d\n",
//         sent, length - sent, res, counter);

CURLcode WsConnectionManager::connect(const char* wssUri)
{
    m_curl = curl_easy_init();
    curl_easy_setopt(m_curl, CURLOPT_URL, wssUri);
    curl_easy_setopt(m_curl, CURLOPT_CONNECT_ONLY, 2L); /* websocket style */
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYSTATUS, 0);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0);
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 2L);
    auto res = curl_easy_perform(m_curl);
    return res;
}

void WsConnectionManager::cacheDataLocally(char *data, int length)
{
    DataForSend dt{ data = data, length = length};
    m_waitingAnswerBuffer.insert({m_sendCounter, dt});
    m_sendCounter++;
}

void WsConnectionManager::removeDataFromCache(int index)
{
    auto iter = m_waitingAnswerBuffer.find(index);
    if(iter != m_waitingAnswerBuffer.end())
        m_waitingAnswerBuffer.erase(iter);
}

void WsConnectionManager::SendData(char *data, int length, int sendDounter)
{
    DataForSend dt; 
    dt.data = data;
    dt.length = length;
    dt._cnt = sendDounter;
    m_sendingPacketsBuffer.add(dt);
}

CURLcode WsConnectionManager::sendData(char *data, int length, int counter)
{
    printf("Sending: %s %d\n", data, counter);
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
            std::cout << "Definitely sent " << index << " packet" << std::endl;
            removeDataFromCache(index);
        }
    }

    return res;
}

double WsConnectionManager::waitForAnswer(int maxWait, int sockFd)
{
    fd_set readfds;
    /* Extract the socket from the curl handle - we need it for waiting. */
    FD_ZERO(&readfds); // Initialize the set
    FD_SET(sockFd, &readfds); // Add sockfd to the set
    // Set timeout for select (if desired)
    struct timeval timeout;
    timeout.tv_sec = 0; 
    timeout.tv_usec = maxWait; // micro seconds

    auto start = std::chrono::system_clock::now();
    int ready = select(sockFd + 1, &readfds, NULL, NULL, &timeout);
    //printf("CURLE_AGAIN detected for socket %d ready: %d\n", sockFd, ready);
    // usleep(5000);
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = end - start;
    // std::cout << "received content on " << sockFd << " after " << diff.count() *1000000 << "\n";

    return diff.count() *1000000;
}
