#pragma once

#include <mutex>
#include <condition_variable>

#include "Pool.hpp"

namespace Orpy
{
    struct ThreadSynchronization
    {
    protected:
        
        std::condition_variable condition;
        std::mutex mutex;

        Pool* pool = new Pool();

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

        void* waitCondition(int id)
        {            
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [this]() { return !pool->empty() || !isRunning; });

            if (!pool->empty())
            {
                auto ptr = pool->get();
                pool->pop();

                return ptr;
            }           

            return nullptr;
        }
                
        void push(void* ptr)
        {
            {
                std::lock_guard<std::mutex> lock(mutex);

                pool->push(ptr);
            }

            condition.notify_one();
        }
    };
}