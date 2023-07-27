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
        std::vector<std::string> _path;

        std::mutex _mutex;
        std::condition_variable _condition;

        bool _isRunning = false;

        std::map<fs::path, fs::file_time_type> last_modified;

        std::thread _guard;

    public:

        fileGuard(C* c, std::string path = "") : _class(c)
        {
            if(!path.empty())
            {
                if (!fs::exists(path) || !fs::is_directory(path))
                {
                    std::cout << "Error: " << path << " is not a valid directory" << std::endl;
                    return;
                }

                std::unique_lock<std::mutex> lock(_mutex);
                {
                    _path.push_back(path);
                }
            }

            _isRunning = true;

            _guard = std::thread(&fileGuard<C>::guardFolder, this);
        }

        ~fileGuard()
        {
            std::unique_lock<std::mutex> lock(_mutex);
            {
                _isRunning = false;
            }

            _condition.notify_all();

            if (_guard.joinable())
                _guard.join();
        }

        void addPath(std::string path)
        {
            if (!fs::exists(path) || !fs::is_directory(path))
            {
                std::cout << "Error: " << path << " is not a valid directory" << std::endl;
                return;
            }

            std::unique_lock<std::mutex> lock(_mutex);
            {                
                _path.push_back(path);
            }
        }

        void removePath(std::string path)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            {
                _path.erase(std::remove(_path.begin(), _path.end(), path), _path.end());
            }
        }

    private:

        void ciclePath(std::string p)
        {
            for (const auto& entry : fs::recursive_directory_iterator(p))
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
        }

        void checkDelete()
        {
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
        }

        void guardFolder()
        {
            while (_isRunning)
            {
                checkDelete();

                for(auto& p : _path)
                    ciclePath(p);

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    };
}