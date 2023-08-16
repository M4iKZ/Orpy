#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>

#include "Common/data.hpp"
#include "Tools/tools.hpp"

namespace Orpy
{
	namespace http
	{
		const std::vector<std::string> supportedType =
		{
			"application/x-www-form-urlencoded",
			"multipart/form-data",
			"application/json",
			"application/xml",
			"application/pdf",
			"application/octet-stream"
		};

		const std::unordered_map<int, std::string> Responses =
		{
			{100, "100 Continue"},
			{101, "101 Switching Protocols"},
			{102, "102 Processing"},
			{200, "200 OK"},
			{201, "201 Created"},
			{202, "202 Accepted"},
			{203, "203 Non-Authoritative Information"},
			{204, "204 No Content"},
			{205, "205 Reset Content"},
			{206, "206 Partial Content"},
			{207, "207 Multi-Status"},
			{208, "208 Already Reported"},
			{226, "226 IM Used"},
			{300, "300 Multiple Choices"},
			{301, "301 Moved Permanently"},
			{302, "302 Found"},
			{303, "303 See Other"},
			{304, "304 Not Modified"},
			{305, "305 Use Proxy"},
			{306, "306 Switch Proxy"},
			{307, "307 Temporary Redirect"},
			{308, "308 Permanent Redirect"},
			{400, "400 Bad Request"},
			{401, "401 Unauthorized"},
			{402, "402 Payment Required"},
			{403, "403 Forbidden"},
			{404, "404 Not Found"},
			{405, "405 Method Not Allowed"},
			{406, "406 Not Acceptable"},
			{407, "407 Proxy Authentication Required"},
			{408, "408 Request Timeout"},
			{409, "409 Conflict"},
			{410, "410 Gone"},
			{411, "411 Length Required"},
			{412, "412 Precondition Failed"},
			{413, "413 Payload Too Large"},
			{414, "414 URI Too Long"},
			{415, "415 Unsupported Media Type"},
			{416, "416 Range Not Satisfiable"},
			{417, "417 Expectation Failed"},
			{418, "418 I'm a teapot"},
			{421, "421 Misdirected Request"},
			{422, "422 Unprocessable Entity"},
			{423, "423 Locked"},
			{424, "424 Failed Dependency"},
			{426, "426 Upgrade Required"},
			{428, "428 Precondition Required"},
			{429, "429 Too Many Requests"},
			{431, "431 Request Header Fields Too Large"},
			{451, "451 Unavailable For Legal Reasons"},
			{444, "444 No Response"},
			{500, "500 Internal Server Error"},
			{501, "501 Not Implemented"},
			{502, "502 Bad Gateway"},
			{503, "503 Service Unavailable"},
			{504, "504 Gateway Timeout"},
			{505, "505 HTTP Version Not Supported"},
			{506, "506 Variant Also Negotiates"},
			{507, "507 Insufficient Storage"},
			{508, "508 Loop Detected"},
			{510, "510 Not Extended"},
			{511, "511 Network Authentication Required"}
		};

		const std::unordered_map<std::string, std::string> ContentType =
		{
			{"html", "text/html"},
			{"txt", "text/plain"},
			{"css", "text/css"},
			{"scss", "text/x-scss"},
			{"sass", "text/x-sass"},
			{"map", "application/json"},
			{"js", "application/javascript"},

			{"jpeg", "image/jpeg"},
			{"jpg", "image/jpeg"},
			{"png", "image/png"},
			{"gif", "image/gif"},
			{"svg", "image/svg+xml"},
			{"ico", "image/x-icon"},
			{"webp", "image/webp"},

			{"woff", "font/woff"},
			{"woff2", "font/woff2"},

			{"json", "application/json"},
			{"xml", "application/xml"},

			{"zip", "application/zip"}
		};

		namespace Request
		{
			struct MultiPart
			{
				std::string Disposition = "";
				std::string name = "";

				bool isFile = false;
				std::string filename = "";
				std::string type = "";
				std::vector<char> data;
			};

			struct Data
			{
				std::string method = "";

				// User-Agent / Bot
				std::string useragent = "";
				bool isBot = false;

				// URL
				std::string URI = "";
				std::string URL = "";
				std::vector<std::string> commands;
				int nCommands = 0;

				// Host
				std::string Host = "";

				// Referer
				std::string Referer = "";

				// Origin
				std::string Origin = "";

				// Supported Encoding
				// not implemented yet
				bool br = false;

				// Content-Length
				int contentLength = 0;
				std::string contentType = ""; // basic form

				// Cookies
				std::unordered_map<std::string, std::string> cookies;

				// GET VARs
				bool hasGET = false;
				std::unordered_map<std::string, std::string> GET;

				// POST
				bool isPOST = false;
				std::unordered_map<std::string, std::string> POST;
				bool done = false;

				bool isMultipart = false;
				std::string boundary = "";
				std::unordered_map<std::string, MultiPart> MULTIPART;

				// is FILE?
				bool isFile = false;
				std::string filePath = "";
				std::string fileModifiedSince = "";
			};
		}

		struct Response
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

			int memoPos = 0;
		};

		struct Data
		{
			Data(int p = 0, int clientfd = -1, std::string k = "", std::string ip = "") 
				: phase(p), fd(clientfd), key(k), IP(ip), length(0), buffer("")
			{}

			int fd;
			int phase;

			std::string key;

			std::string IP = "";

			size_t length = 0;			

			//std::vector<char> buffer;
			std::string buffer;
			int position = 0;

			// Conf
			site::Settings Conf;

			// Request
			Request::Data request;

			// Response
			Response response;

			time_t startTime = setTime();

			// Error
			bool error = false;
		};		
	}
}