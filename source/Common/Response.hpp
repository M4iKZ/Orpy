#pragma once

#include <string>
#include <sstream>
#include <ctime>
#include <chrono>
#include <iomanip>

#include "HTTPProtocol.hpp"

namespace Orpy
{
	struct Cookie
	{
		std::string name;
		std::string value;
		std::string path = "/";
		std::string domain;
		time_t expires;
		bool secure = false;
		bool http_only = false;
	};

	struct HttpResponse
	{
		//Status
		int status = 200;

		std::string content_type = ContentTypeMapping.at("txt");
		std::string content = "";
		size_t content_length = 0; 

		size_t bufferSize;

		std::vector<Cookie> cookies;

		std::string location = "";

		bool compressed = false;

		// Generates the response buffer from the fields of the struct
		void getBuffer(std::vector<char>& buffer)
		{
			std::string response = "HTTP/1.1 " + HttpResponses.at(status) + "\r\n";

			if (status == 301 || status == 302 || status == 303 || status == 307)
			{
				response += "Location: " + location + "\r\n";
			}
			else
			{
				response += "Content-Type: " + content_type + "\r\n";

				if (compressed)
					response += "Content-Encoding: br\r\n";

				if (content_type == "application/octet-stream")
					response += "Content-Disposition: attachment; filename=10GB.bin\r\n"; //WIP
				
				response += "Content-Length: " + std::to_string(content_length) + "\r\n";

				for (const auto& cookie : cookies)
				{
					std::string timestamp;
					if (cookie.expires)
					{
						// Convert the calendar time to a time point
						std::chrono::system_clock::time_point time_point = std::chrono::system_clock::from_time_t(cookie.expires);

						// Format the time point as a string
						std::stringstream ss;
						ss << std::put_time(std::gmtime(&cookie.expires), "%a, %d-%b-%Y %H:%M:%S GMT");
						timestamp = ss.str();
					}
					else
					{
						timestamp = "Session";
					}

					response += "Set-Cookie: " + cookie.name + "=" + cookie.value + "; Expires=" + timestamp + "; Path=" + cookie.path +
						(cookie.secure ? "; Secure" : "") + (cookie.http_only ? "; HttpOnly" : "")
						+ "\r\n";
				}
			}

			response += "\r\n";

			// Calculate the total length of the buffer + 4 for end header and 2 for every end line			
			bufferSize = response.length() + content.length();

			buffer.insert(buffer.begin(), response.begin(), response.end());
			
			if (content != "")
				buffer.insert(buffer.end(), content.begin(), content.end());			
		}
	};
}