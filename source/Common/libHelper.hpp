#pragma once

#include <iostream>
#include <string>
#include <functional>

#include <filesystem> //on linux I need to find the correct directory

#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

template <typename T, typename... Args>
class DynamicLibrary 
{
public:        
#ifdef _WIN32
    DynamicLibrary(LPCTSTR library_path, std::string lib, Args&&... args) : _lib(lib)
    {
        library_handle_ = LoadLibrary(library_path);
#else
    DynamicLibrary(const std::string & library_path, std::string lib, Args&&... args) : _lib(lib)
    {
        library_handle_ = dlopen(library_path.c_str(), RTLD_LAZY);
#endif
        try 
        {
            if (!is_loaded())
            {
                const std::string error = lib + " library not loaded!";
                throw std::runtime_error(error);
            }

            auto all = get_function<T* (Args...)>("all" + _lib);
            if(all)
                object_ = all(std::forward<Args>(args)...);
            else
            {
                const std::string error = lib + " all not found!";
                throw std::runtime_error(error);
            }
        }
        catch (const std::runtime_error& e) 
        {
            // handle the exception
            std::cerr << e.what() << std::endl;
        }
        
    }

    ~DynamicLibrary() 
    {   
        close();
    }

    T* operator->() 
    {
        return object_;
    }

private:
    void close()
    {
        if (is_loaded())
            delete object_;

#ifdef _WIN32
        FreeLibrary(library_handle_);
#else
        dlclose(library_handle_);
#endif     
    }

    bool is_loaded() const 
    {
        return library_handle_ != nullptr;
    }

    template <typename F>
    F* get_function(const std::string& name) 
    {
#ifdef _WIN32
        return reinterpret_cast<F*>(GetProcAddress(library_handle_, name.c_str()));
#else
        return reinterpret_cast<F*>(dlsym(library_handle_, name.c_str()));
#endif
    }

#ifdef _WIN32
    HMODULE library_handle_;
#else
    void* library_handle_;
#endif

    T* object_;    
    std::string _lib;
};
