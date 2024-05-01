#pragma once
#include <iostream>
#include <thread>
#include <chrono>
#include <functional>

using namespace std;

class Timer
{
    thread* th;
    bool running = false;

public:
    typedef std::function<void(void)> Timeout;
    void start(const std::chrono::milliseconds &interval,
               const Timeout &timeout)
    {
        std::cout << "th.start" << std::endl;
        running = true;

        th = new thread([=]()
        {
            while (running == true) {
                for(int i = 0;i < 50; i++)
                {
                    this_thread::sleep_for(interval / 20);
                    if(running == false)
                        break;
                }
                if(running == false)
                    break;
                timeout();
            }
            int a = 0;
        });
    }

    void stop()
    {
        std::cout << "th.stop" << std::endl;
        running = false;
        if(th->joinable())
            th->join();
    }
    Timer()
    {
        std::cout << "Timer::Timer" << std::endl;
    }
    ~Timer()
    {
        std::cout << "Timer::~Timer" << std::endl;
        if(th->joinable())
        {
            std::cout << "th.joinable" << std::endl;
            th->join();
        }
    }
};
