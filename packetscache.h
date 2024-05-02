#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>
#include "audiopacket.h"

class PacketsCache
{
public:
    void append(DataForSend& packet);
    DataForSend& front();
    void removeFront();
    size_t size();
private:
    std::deque<DataForSend> m_cache;
    std::mutex m_mutex;
    std::condition_variable cv;

};