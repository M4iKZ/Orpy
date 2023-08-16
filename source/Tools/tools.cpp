
#include <iostream>
#include <mutex>

#include "tools.hpp"

namespace Orpy
{
	time_t setTime(const time_t& time) 
	{
		return std::time(nullptr) + time;
	}

	std::mutex _mtx;
	void debug(std::string msg)
	{
		{
			std::unique_lock<std::mutex> lock(_mtx);
			std::cout << msg << std::endl;
		}
	}
}