#pragma once

#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace Orpy
{
    namespace synchronization
    {
        template<typename T>
        class Pool
        {
        public:
            Pool()
            {
                _isRunning.store(true);
            }

            ~Pool()
            {
                stop();
            }

            void push(std::unique_ptr<T> data)
            {                
                {
                    std::lock_guard<std::mutex> lock(_mutex);
                    q.push(std::move(data));
                }

                _cond.notify_one();
            }

            std::unique_ptr<T> pop()
            {
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _cond.wait(lock, [this]() { return !q.empty() || !_isRunning.load(); });

                    if (!_isRunning.load())
                        return nullptr;

                    std::unique_ptr<T> data = std::move(q.front());
                    q.pop();

                    return data;
                }
            }

            bool empty()
            {
                std::lock_guard<std::mutex> lock(_mutex);
                return q.empty();
            }

            void stop()
            {
                _isRunning.store(false);
                _cond.notify_all();
            }

        private:
            std::queue<std::unique_ptr<T>> q;
            std::mutex _mutex;
            std::condition_variable _cond;
            std::atomic<bool> _isRunning;
        };
    }
}