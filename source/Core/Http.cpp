
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

	void Http::managePOST(std::unique_ptr<HTTPData>& data)
	{
		if (data->request.isMultipart)
			elaborateMultipart(data);
		else
			getPOST(data);

		if (data->response.location == "")
			data->response.location = data->request.URI;

		data->response.status = 303; //REDIRECT FOR POST	

		//MANAGE HERE POST
	}

	void Http::generatePage(std::unique_ptr<HTTPData>& data)
	{
		std::ifstream file(fs::current_path().string() + "/Data/" + data->Conf.path + "/style/main.style", std::ios::binary);
		if (file)
		{
			// Get the file size
			file.seekg(0, std::ios::end);
			std::streampos fileSize = file.tellg();
			file.seekg(0, std::ios::beg);

			// Read the entire file into the content string
			data->response.content.resize(fileSize);
			file.read(&data->response.content[0], fileSize);

			file.close();
		}

		if (data->response.content_type.empty())
			data->response.content_type = "html";

		getType(data);
	}

	void Http::prepareFile(std::unique_ptr<HTTPData>& data)
	{
		std::filesystem::path file_path(data->request.URL);
		data->response.content_type = file_path.extension().string().substr(1);		
		if (getType(data))
		{
			// Stored File
			std::filesystem::file_time_type ftime = std::filesystem::last_write_time(data->request.filePath);
			std::time_t fileUnixtime = std::chrono::system_clock::to_time_t(std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()));

			std::tm fileTime = *std::gmtime(&fileUnixtime);
			std::ostringstream filess;
			filess << std::put_time(&fileTime, "%a, %d %b %Y %T GMT");

			data->response.isFile = true;
			data->response.fileLastEdit = filess.str();

			// Cached File
			// Define the format of the input date and time string
			std::string format = "%a, %d %b %Y %H:%M:%S %Z";

			// Create an input string stream from the date string
			std::istringstream iss(data->request.fileModifiedSince);

			// Create tm structure to store parsed values
			std::tm timeStruct = {};

			// Parse the date string using input stream and put it into tm structure
			iss >> std::get_time(&timeStruct, format.c_str());

			// Set the time zone offset to zero (GMT)
			timeStruct.tm_hour += 1; // Adjust hour if necessary
			std::time_t cachedUnixtime = std::mktime(&timeStruct);

			if (cachedUnixtime < fileUnixtime)
			{
				data->phase = 3; //SEND FILE

				data->response.fileName = data->request.filePath;
				data->response.fileSize = std::filesystem::file_size(data->response.fileName);
				
				data->response.content_length = data->response.fileSize;				
			}
			else
				data->response.status = 304;
		}
		else
			data->response.status = 404;
	}

	// Generates the response buffer from the fields in the struct
	int Http::getBuffer(HttpResponse* data, std::vector<char>& buffer)
	{
		std::ostringstream response;

		// HTTP Status Line
		response << "HTTP/1.1 " << HttpResponses.at(data->status) << "\r\n";

		// Server information
		response << "Server: Orpy by M4iKZ\r\n";

		if (data->status == 301 || data->status == 302 || data->status == 303 || data->status == 307)
		{
			// Redirection response
			response << "Location: " << data->location << "\r\n";
		}
		else
		{
			// Regular response
			response << "Content-Type: " << data->content_type << "\r\n";

			if (!data->attachmentName.empty())
			{
				// Download file instead of showing it
				response << "Content-Disposition: attachment; filename=" << data->attachmentName << "\r\n";
			}

			if (!data->content.empty() && data->content_length == 0)
				data->content_length = data->content.size();

			response << "Content-Length: " << data->content_length << "\r\n";

			if (data->isFile)
			{
				response << "Last-Modified: " << data->fileLastEdit << "\r\n";
				response << "Cache-Control: must-revalidate\r\n";
			}
		}

		response << "\r\n";

		// Convert the response string to a vector of characters
		std::string responseStr = response.str();
		buffer.insert(buffer.begin(), responseStr.begin(), responseStr.end());

		if (!data->content.empty())
			buffer.insert(buffer.end(), data->content.begin(), data->content.end());

		// Calculate the total length of the buffer		
		return buffer.size();
	}

	void Http::elaborateData(std::unique_ptr<HTTPData>& data)
	{
		parser(data);

		if (!data->error)
		{
			check_URL(data);
						
			if (data->request.isPOST)
				managePOST(data);

			if (data->response.status == 200 && !data->error && !data->request.done)
			{
				if (!data->request.isFile)
					generatePage(data);

				if (data->request.isFile)
					prepareFile(data);
			}
		}

		data->buffer.clear();
		data->length = getBuffer(&data->response, data->buffer);
	}
}