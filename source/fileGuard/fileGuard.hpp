#pragma once

#ifdef _WIN32
#ifdef _EXP_FGUARD
#define MY_EXPORT __declspec(dllexport)
#endif 
#define MY_EXPORT __declspec(dllimport)
#else
#define MY_EXPORT
#endif

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <filesystem>
#include <map>
#include <chrono>
#include <functional>
#include <string>

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

        std::thread _guard;

    public:

        fileGuard(C* c, std::string path);
        ~fileGuard();
        
    private:
        void guardFolder();
    };
    
    template<typename C>
    extern MY_EXPORT fileGuard<C>* initFGuard(C* c, std::string path);
}