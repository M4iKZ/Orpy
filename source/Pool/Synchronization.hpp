#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

namespace Orpy
{
    template<typename T>
    class ThreadPool
    {
    public:
        void push(std::unique_ptr<T> data) 
        {
            {
                std::lock_guard<std::mutex> lock(mut);
                q.push(std::move(data));
            }

            con.notify_one();
        }

        std::unique_ptr<T> pop() 
        {
            {
                std::unique_lock<std::mutex> lock(mut);
                con.wait(lock, [this]() { return !q.empty() || !isRunning; });

                if (!isRunning)
                    return nullptr;

                std::unique_ptr<T> data = std::move(q.front());
                q.pop();

                return data;
            }
        }

        bool empty() const 
        {
            std::lock_guard<std::mutex> lock(mut);
            return q.empty();
        }

        void stop() 
        {
            std::lock_guard<std::mutex> lock(mut);
            {
                isRunning = false;
            }
            
            con.notify_all();
        }

    private:
        std::queue<std::unique_ptr<T>> q;
        std::mutex mut;
        std::condition_variable con;
        bool isRunning = true;
    };
}