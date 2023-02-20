#pragma once

#include <iostream>
#include <ctime>
#include <vector>
#include <mutex>
#include <condition_variable>

namespace Orpy
{
	std::mutex _mutex;
	
	const int64_t ReadSize = 65536;

	struct HTTPData
	{
		HTTPData(int p = 0) : fd(-1), phase(p), length(0), cursor(0), buffer() {}
		int fd;
		int phase;
		size_t length;
		size_t cursor;
		std::vector<char> buffer;

		//RESPONSE SEND FILE
		bool isFile = false;
		std::string fileName = "";
		int64_t fileSize = 0;
		size_t headerSize = 0;

		bool isWS = false; //websockets?
		std::time_t startTime = setTime();

		std::time_t setTime()
		{
			return std::time(nullptr) + 30; //30 seconds should be enough?				
		}
	};

	struct SITEData
	{
		std::string path = "";
		std::vector<std::string> langs = {};
		std::vector<std::string> urls = {};
		bool allowFiles = false;
		std::string DB = "";

		bool hasDB() { return DB != ""; }
	};

	void debug(std::string msg)
	{
		{
			std::unique_lock<std::mutex> lock(_mutex);

			std::cout << msg << std::endl;
		}
	}
}