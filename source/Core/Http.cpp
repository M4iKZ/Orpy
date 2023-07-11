
#include "Http.hpp"

namespace Orpy
{
	IHttp* allHttp()
	{
		return new Http();
	}

#ifdef _WIN32
	Http::Http() : Sockets("Sockets.dll", "Sockets", this, _host.c_str(), _port)
#else
	Http::Http() : Sockets("libSockets.so", "Sockets", this, _host.c_str(), _port)
#endif
	{
		debug("Open a socket on " + _host + " with port " + std::to_string(_port));

		if (Sockets->start())
		{
			loadConfs();
						
			debug("Core lib loaded ...");

			debug("Enter [quit] to stop the server");
			std::string command;
			while (std::cin >> command, command != "quit"); //Add Command Manager and Log Manager?

			debug("'quit' command entered. Stopping the web server ...");
		}
	}

	Http::~Http()
	{
		debug("Core lib unloaded ...");
	}

	void Http::managePOST(HTTPData* data, HttpRequest* request)
	{
		if (request->isMultipart)
			elaborateMultipart(data->buffer, request);
		else
			getPOST(data->buffer, request);

		if (request->response.location == "")
			request->response.location = request->URI;

		request->response.status = 303; //REDIRECT FOR POST	

		//MANAGE HERE POST
	}

	void Http::generatePage(HttpRequest* request)
	{
		std::ifstream file(fs::current_path().string() + "/Data/" + request->Conf.path + "/style/main.style");
		request->response.content.assign((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
		file.close();

		if (request->response.content_type == "")
			request->response.content_type = "html";

		getType(request, request->response.content_type);
	}

	void Http::prepareFile(HttpRequest* request, HTTPData* response)
	{
		std::filesystem::path file_path(request->URL);
		std::string file_extension = file_path.extension().string().substr(1);
		if (getType(request, file_extension))
		{
			// Stored File
			std::filesystem::file_time_type ftime = std::filesystem::last_write_time(request->filePath);
			std::time_t fileUnixtime = std::chrono::system_clock::to_time_t(std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()));

			std::tm fileTime = *std::gmtime(&fileUnixtime);
			std::ostringstream filess;
			filess << std::put_time(&fileTime, "%a, %d %b %Y %T GMT");

			response->isFile = true;
			request->response.fileLastEdit = filess.str();

			// Cached File
			// Define the format of the input date and time string
			std::string format = "%a, %d %b %Y %H:%M:%S %Z";

			// Create an input string stream from the date string
			std::istringstream iss(request->fileModifiedSince);

			// Create tm structure to store parsed values
			std::tm timeStruct = {};

			// Parse the date string using input stream and put it into tm structure
			iss >> std::get_time(&timeStruct, format.c_str());

			// Set the time zone offset to zero (GMT)
			timeStruct.tm_hour += 1; // Adjust hour if necessary
			std::time_t cachedUnixtime = std::mktime(&timeStruct);

			if (cachedUnixtime < fileUnixtime)
			{
				response->phase = 3; //SEND FILE

				response->fileName = request->filePath;
				response->fileSize = std::filesystem::file_size(response->fileName);

				request->response.isFile = response->isFile;

				request->response.content_type = file_extension;
				request->response.content_length = response->fileSize;
			}
			else
				request->response.status = 304;
		}
		else
			request->response.status = 404;
	}

	void Http::elaborateData(HTTPData* data)
	{
		HttpRequest* request = new HttpRequest();
		parser(request, data->buffer);

		if (request->clientIP == "")
			request->clientIP = data->IP;

		if (!request->error)
		{
			check_URL(request);
						
			if (request->isPOST)
				managePOST(data, request);

			if (request->response.status == 200 && !request->error && !request->done)
			{
				if (!request->isFile)
					generatePage(request);

				if (request->isFile)
					//prepareFile(request, response);
					prepareFile(request, data);
			}
		}

		data->buffer.clear();

		request->response.getBuffer(data->buffer);
		data->length = request->response.bufferSize;

		delete request;
	}
}