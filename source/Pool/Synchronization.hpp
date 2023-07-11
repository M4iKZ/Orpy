#pragma once

#include <iostream>
#include <string>

#include <vector>

#include <mutex>
#include <condition_variable>

namespace Orpy
{
    template<typename T>
    struct ThreadSynchronization
    {
    protected:

        std::condition_variable condition;
        std::mutex mutex;

        std::vector<T> q;

    public:
        bool isRunning = true;

        void notifyClose()
        {
            {
                std::lock_guard<std::mutex> lock(mutex);

                isRunning = false;
            }

            condition.notify_all();
        }

        T waitCondition(int id = 0)
        {
            T ptr;
            {
                std::unique_lock<std::mutex> lock(mutex);
                condition.wait(lock, [this]() { if (q.empty()) q.shrink_to_fit(); return !q.empty() || !isRunning; });

                if (isRunning)
                {
                    ptr = q.front();
                    q.erase(q.begin());
                }
            }

            return ptr;
        }

        void push(T ptr)
        {
            {
                std::lock_guard<std::mutex> lock(mutex);

                q.push_back(ptr);
            }

            condition.notify_one();
        }
    };
}