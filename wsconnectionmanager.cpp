#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <future>
#include <thread>
#include <chrono>
#include "wsconnectionmanager.h"

static int wait_on_socket(curl_socket_t sockfd, int for_recv, long timeout_ms)
{
  struct timeval tv;
  fd_set infd, outfd, errfd;
  int res;

  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (int)(timeout_ms % 1000) * 1000;

  FD_ZERO(&infd);
  FD_ZERO(&outfd);
  FD_ZERO(&errfd);

/* Avoid this warning with pre-2020 Cygwin/MSYS releases:
 * warning: conversion to 'long unsigned int' from 'curl_socket_t' {aka 'int'}
 * may change the sign of the result [-Wsign-conversion]
 */
#if defined(__GNUC__) && defined(__CYGWIN__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
  FD_SET(sockfd, &errfd); /* always check for error */

  if(for_recv) {
    FD_SET(sockfd, &infd);
  }
  else {
    FD_SET(sockfd, &outfd);
  }
#if defined(__GNUC__) && defined(__CYGWIN__)
#pragma GCC diagnostic pop
#endif

  /* select() returns the number of signalled sockets or -1 */
  res = select((int)sockfd + 1, &infd, &outfd, &errfd, &tv);
  return res;
}

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
                cleanupConnection(connected);
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
                    int counter = dt._cnt;
                    res = handlePacketSendReceive(sockfd, data, length, counter, connected);
                }
            }
        }
    });
}

CURLcode WsConnectionManager::handlePacketSendReceive(int sockfd, 
    char* data, 
    int length, 
    int counter,
    bool& connected)
{
    auto res = sendData(data, length, counter);
    if(res == CURLE_OK)
    {
        printf("sendData: %s %d ok!\n", data, counter);
        bool timeoutDetected;
        res = receiveData(sockfd, timeoutDetected);
        if(timeoutDetected || CURLE_OK != res)
            cleanupConnection(connected);
        int a = 0;
    }
    else
        cleanupConnection(connected);
    return res;
}

void WsConnectionManager::cleanupConnection(bool& connected)
{
    connected = false;
    curl_easy_cleanup(m_curl);
    usleep(2000000);
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
    size_t sent;
    CURLcode res = curl_ws_send(m_curl, data, length, &sent, 0, CURLWS_BINARY);
    return res;
}

CURLcode WsConnectionManager::receiveData(int sockfd, bool& timeOutDetectedOnReceive)
{
    char buffer[20] = {0};
    CURLcode res;
    timeOutDetectedOnReceive = false; 
    do {
        size_t rlen;
        const struct curl_ws_frame *meta;
        res = curl_ws_recv(m_curl, buffer, sizeof(buffer), &rlen, &meta);

        int for_receive = 1;
        if(res == CURLE_AGAIN && !wait_on_socket(sockfd, for_receive, 60000L)) {
          timeOutDetectedOnReceive = true;
          break;
        }
    } while(res == CURLE_AGAIN);
    
    // while(CURLE_AGAIN == res)
    // {
    //     auto start = std::chrono::system_clock::now();
    //     auto end = std::chrono::system_clock::now();
    //     int readySockets = wait_on_socket(sockfd, 1, 60000L);
    //     std::chrono::duration<double> diff = end - start;
    //     printf("CURLE_AGAIN detected for index %d readySockets: %d timeout: %f microsecs\n", dt._cnt, readySockets, diff.count()*1'000'000);
    // }




    if(res == CURLE_OK)
    {
        //We expect 4 byte int(counter) as answer
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