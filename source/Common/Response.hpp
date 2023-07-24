#pragma once

#include <string>
#include <sstream>
#include <ctime>
#include <chrono>
#include <iomanip>

namespace Orpy
{	
	struct HttpResponse
	{
		//Status
		int status = 200;

		std::string content_type = "";
		std::string content = "";
		size_t content_length = 0; 

		size_t bufferSize = 0;
				
		std::string location = "";

		// not implemented yet
		bool compressed = false;

		// FILE
		bool isFile = false;
		std::string fileLastEdit = "";
		
		std::string fileName = "";
		std::string attachmentName = "";
		size_t fileSize = 0;
		
		size_t headerSize = 0;
		bool sentHeader = false;
		
		int sent = 0;
		std::streampos cursor = std::streampos();
	};
}