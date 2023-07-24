#pragma once

#include <iostream>
#include <fstream>
#include <ctime>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

#include "HTTPProtocol.hpp"
#include "Request.hpp"
#include "Response.hpp"

namespace Orpy
{
	std::mutex _mutex;
	
	const size_t ReadSize = 1024 * 4 * 16;
	const size_t MaxFileSize = 1024 * 1024;
	const size_t MaxUploadSize = ReadSize * 64;

	time_t setTime(time_t time = 10) //10 seconds should be enough?				
	{
		return std::time(nullptr) + time;
	}

	struct SITEData
	{
		std::string redirect = "";

		std::string path = "";
		std::vector<std::string> langs;

		std::vector<std::string> positions;

		std::unordered_map<std::string, std::string> mimetypes = {};
		std::vector<std::string> urls = {};

		bool allowFiles = false;

		std::string cookieDomain = "";
	};

	struct HTTPData
	{
		HTTPData(int p = 0, int clientfd = -1, std::string k = "", std::string ip = "") : fd(clientfd), key(k), phase(p), IP(ip), length(0), buffer({}) {}

		int fd = 0;
		int phase = 0;

		std::string key = "";

		size_t length = 0;

		std::string IP = "";
		
		std::vector<char> buffer;
		int position = 0;

		// Conf
		SITEData Conf;

		// Request
		HttpRequest request;

		// Response
		HttpResponse response;
				
		time_t startTime = setTime();

		// Error
		bool error = false;
	};
		
	void debug(std::string msg)
	{
		{
			std::unique_lock<std::mutex> lock(_mutex);

			std::cout << msg << std::endl;
		}
	}
}