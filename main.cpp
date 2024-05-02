#include <iostream>
#include <unistd.h>
#include <cstring>
#include "wsconnectionmanager.h"


void startSendData(WsConnectionManager& connManager)
{
    int intervalBetweenPacketsSend = 100000; //50ms
    for (size_t i = 1; i < 1000000; i++)
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
    connManager.StartSending("ws://host.docker.internal:5678");
    startSendData(connManager);
}





// #include <string>
// #include <cstdlib>
// #include <cstring>
// #include <iostream>
// #include <curl/curl.h>


// static int wait_on_socket(curl_socket_t sockfd, int for_recv, long timeout_ms)
// {
//   struct timeval tv;
//   fd_set infd, outfd, errfd;
//   int res;

//   tv.tv_sec = timeout_ms / 1000;
//   tv.tv_usec = (int)(timeout_ms % 1000) * 1000;

//   FD_ZERO(&infd);
//   FD_ZERO(&outfd);
//   FD_ZERO(&errfd);

// /* Avoid this warning with pre-2020 Cygwin/MSYS releases:
//  * warning: conversion to 'long unsigned int' from 'curl_socket_t' {aka 'int'}
//  * may change the sign of the result [-Wsign-conversion]
//  */
// #if defined(__GNUC__) && defined(__CYGWIN__)
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wsign-conversion"
// #endif
//   FD_SET(sockfd, &errfd); /* always check for error */

//   if(for_recv) {
//     FD_SET(sockfd, &infd);
//   }
//   else {
//     FD_SET(sockfd, &outfd);
//   }
// #if defined(__GNUC__) && defined(__CYGWIN__)
// #pragma GCC diagnostic pop
// #endif

//   /* select() returns the number of signalled sockets or -1 */
//   res = select((int)sockfd + 1, &infd, &outfd, &errfd, &tv);
//   return res;
// }


// int main()
// {
//     CURL* m_curl = curl_easy_init();
//     curl_easy_setopt(m_curl, CURLOPT_URL, "ws://host.docker.internal:5678");
//     curl_easy_setopt(m_curl, CURLOPT_CONNECT_ONLY, 2L); /* websocket style */
//     curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYSTATUS, 0);
//     curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0);
//     curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0);
//     curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 2L);
//     auto res = curl_easy_perform(m_curl);

//     int sockfd = 0;
//     res = curl_easy_getinfo(m_curl, CURLINFO_ACTIVESOCKET, &sockfd);
//     if(res != CURLE_OK)
//     {
//         printf("%s %d\n", curl_easy_strerror(res), __LINE__);
//         return res;
//     }


//     int i = 77;
//     std::string s = std::to_string(i);
//     char *data = const_cast<char*>(s.c_str());
//     auto length = strlen(data);
//     size_t sent;
//     res = curl_ws_send(m_curl, data, length, &sent, 0, CURLWS_BINARY);
//     if(res != CURLE_OK)
//     {
//         printf("%s %d\n", curl_easy_strerror(res), __LINE__);
//         return res;
//     }
//     size_t rlen;
//     const struct curl_ws_frame *meta;
//     //We expect 4 byte int(counter) as answer
//     char buffer[20] = {0};
//     res = curl_ws_recv(m_curl, buffer, sizeof(buffer), &rlen, &meta);
//     if(res == CURLE_OK)
//     {

//         auto len = strlen(buffer);
//         buffer[len - 2] = 0; //ignore last ok


//         //Read data
//         char *str;
//         long index = strtol(buffer, &str, 10);
//         if(index > 0)
//         {
//             //As we definitely know that packet was received, remove it from cache
//             std::cout << "Definitely sent " << index << " packet" << std::endl;
//         }
//     }
//     else
//     {
//         printf("%s %d\n", curl_easy_strerror(res), __LINE__);

//         if(res == CURLE_AGAIN)
//         {
//             int wait_res = wait_on_socket(sockfd, 1, 60000L);
//             printf("wait_res is %d\n", wait_res);
//             if(wait_res == 1)
//             {
//                 char buffer[20] = {0};
//                 res = curl_ws_recv(m_curl, buffer, sizeof(buffer), &rlen, &meta);
//                 int tt = 0;
//             }
//         }

//         return res;
//     }




//     return res;
// }