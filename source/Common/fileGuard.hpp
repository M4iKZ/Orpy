#pragma once

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <filesystem>
#include <map>
#include <chrono>
#include <functional>

#include <thread>

#ifndef _WIN32
#include <pthread.h>
#endif

namespace fs = std::filesystem;

namespace Orpy
{
    template<typename C>
    class fileGuard
    {

    protected:
        C* _class;
        std::string _path;

        std::mutex _mutex;
        std::condition_variable _condition;

        bool _isRunning = false;

        std::map<fs::path, fs::file_time_type> last_modified;

#ifdef _WIN32
        std::thread _guard;
#else
        pthread_t _guard;

        static void* guardHelper(void* context)
        {
            reinterpret_cast<fileGuard<C>*>(context)->guardFolder();
            return nullptr;
        }
#endif
    public:

        fileGuard(C* c, std::string path) : _class(c), _path(path)
        {
            if (!fs::exists(_path) || !fs::is_directory(_path))
            {
                std::cout << "Error: " << _path << " is not a valid directory" << std::endl;
                return;
            }

            _isRunning = true;

#ifdef _WIN32
            _guard = std::thread(&fileGuard<C>::guardFolder, this);
#else
            pthread_create(&_guard, nullptr, &fileGuard<C>::guardHelper, this);
#endif
        }

        ~fileGuard()
        {
            {
                std::unique_lock<std::mutex> lock(_mutex);

                _isRunning = false;
            }

            _condition.notify_all();

#ifdef _WIN32
            if (_guard.joinable())
                _guard.join();
#else
            pthread_join(_guard, NULL);
#endif
        }

        void guardFolder()
        {
            while (_isRunning)
            {
                for (const auto& entry : fs::recursive_directory_iterator(_path))
                {
                    if (fs::is_regular_file(entry))
                    {
                        auto path = entry.path();
                        auto last_write_time = fs::last_write_time(path);
                        if (last_modified.find(path) == last_modified.end())
                        {
                            last_modified[path] = last_write_time;
                            _class->New(path);
                        }
                        else if (last_modified[path] != last_write_time)
                        {
                            last_modified[path] = last_write_time;
                            _class->Edited(path);
                        }
                    }
                }

                for (auto it = last_modified.begin(); it != last_modified.end();)
                {
                    if (!fs::exists(it->first))
                    {
                        _class->Deleted(it->first.stem().string());
                        it = last_modified.erase(it);                        
                    }
                    else
                        ++it;
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    };
}