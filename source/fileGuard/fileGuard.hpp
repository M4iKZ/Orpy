#pragma once

#include <iostream>
#include <shared_mutex>
#include <condition_variable>
#include <filesystem>
#include <map>
#include <chrono>
#include <functional>
#include <string>
#include <vector>
#include <atomic>

#include <thread>

#ifndef _WIN32
#include <pthread.h>
#endif

namespace Orpy
{
    template<typename C>
    class fileGuard
    {

    protected:
        C* _class;
        std::vector<std::string> _paths;

        std::shared_mutex _mutex;
        std::condition_variable _condition;

        std::atomic<bool> _isRunning;

        std::map<std::filesystem::path, std::filesystem::file_time_type> last_modified;

        std::thread _worker;

    public:

        fileGuard(C* c, const std::string path = "") : _class(c)
        {
            if(!path.empty())
            {
                if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path))
                {
                    std::cout << "Error: " << path << " is not a valid directory..." << std::endl;
                    return;
                }

                std::unique_lock<std::shared_mutex> lock(_mutex);
                {
                    _paths.push_back(path);
                }
            }

            _isRunning.store(true);

            _worker = std::thread(&fileGuard<C>::guardFolder, this);
        }

        ~fileGuard()
        {
            std::unique_lock<std::shared_mutex> lock(_mutex);
            {
                _isRunning.store(false);
            }

            _condition.notify_all();

            if (_worker.joinable())
                _worker.join();
        }

        void addPath(const std::string& path)
        {
            if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path))
            {
                std::cout << "Error: " << path << " is not a valid directory" << std::endl;
                return;
            }

            std::unique_lock<std::shared_mutex> lock(_mutex);
            {                
                _paths.push_back(path);
            }
        }

        void removePath(const std::string& path)
        {
            std::lock_guard<std::shared_mutex> lock(_mutex);
            {
                _paths.erase(std::remove(_paths.begin(), _paths.end(), path), _paths.end());
            }
        }

    private:

        bool ciclePath(const std::string& p)
        {
            if (std::filesystem::exists(p))
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(p))
                {
                    if (std::filesystem::is_regular_file(entry))
                    {
                        auto path = entry.path();
                        auto last_write_time = std::filesystem::last_write_time(path);
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

                return true;
            }

            return false;
        }

        void checkDelete()
        {
            for (auto it = last_modified.begin(); it != last_modified.end();)
            {
                if (!std::filesystem::exists(it->first))
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
            while (_isRunning.load())
            {
                checkDelete();

                for (auto& p : _paths)
                    if (!ciclePath(p))
                        continue;

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    };
}