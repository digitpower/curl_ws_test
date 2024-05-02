#include "packetscache.h"


void PacketsCache::append(DataForSend& packet)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cache.push_back(packet);
    cv.notify_one();
}

DataForSend& PacketsCache::front()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    cv.wait(lock, [this]{ return m_cache.size() != 0; });
    return m_cache.front();
}

void PacketsCache::removeFront()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    cv.wait(lock, [this]{ return m_cache.size() != 0; });
    m_cache.pop_front();
}

size_t PacketsCache::size()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_cache.size();
}