#pragma once

#include <string>
#include <sstream>
#include <ctime>
#include <chrono>
#include <iomanip>

#include "HTTPProtocol.hpp"

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

		bool compressed = false;

		bool isFile = false;
		std::string fileLastEdit = "";
		std::string fileName = "";

		// Generates the response buffer from the fields in the struct
		void getBuffer(std::vector<char>& buffer)
		{
			std::string response = "HTTP/1.1 " + HttpResponses.at(status) + "\r\n";

			response += "Server: Orpy by M4iKZ\r\n";

			if (status == 301 || status == 302 || status == 303 || status == 307)
				response += "Location: " + location + "\r\n";			
			else
			{
				response += "Content-Type: " + content_type + "\r\n";

				if (fileName != "") // download file instead to show it
					response += "Content-Disposition: attachment; filename=" + fileName + "\r\n";
				
				if (content != "" && content_length == 0)
					content_length = content.size();

				response += "Content-Length: " + std::to_string(content_length) + "\r\n";

				if (isFile)
				{
					response += "Last-Modified: " + fileLastEdit + "\r\n";
					response += "Cache-Control: must-revalidate\r\n";
				}
			}

			response += "\r\n";

			// Calculate the total length of the buffer		
			bufferSize = response.length() + content.length();

			buffer.insert(buffer.begin(), response.begin(), response.end());
			
			if (content != "")
				buffer.insert(buffer.end(), content.begin(), content.end());
		}
	};
}