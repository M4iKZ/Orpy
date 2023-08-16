
#include "fileGuard.hpp"

namespace Orpy
{
    template<typename C>
    fileGuard<C>* initFGuard(C* c, std::string path)
    {
        return new fileGuard<C>(C* c, std::string path);
    }

    template<typename C>
    fileGuard<C>::fileGuard(C* c, std::string path) : _class(c), _path(path)
    {
        if (!fs::exists(_path) || !fs::is_directory(_path))
        {
            std::cout << "Error: " << _path << " is not a valid directory" << std::endl;
            return;
        }

        _isRunning = true;

        _guard = std::thread(&fileGuard<C>::guardFolder, this);
    }

    template<typename C>
    fileGuard<C>::~fileGuard()
    {
        {
            std::unique_lock<std::mutex> lock(_mutex);

            _isRunning = false;
        }

        _condition.notify_all();

        if (_guard.joinable())
            _guard.join();
    }

    template<typename C>
    void fileGuard<C>::guardFolder()
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
}