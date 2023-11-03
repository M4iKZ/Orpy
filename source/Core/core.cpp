
#include "core.hpp"

namespace Orpy
{	
	std::shared_ptr<ICore> _core = nullptr;

	void setCore()
	{
		_core = std::make_shared<Core>();
	}

	Core::Core()
	{
		debug("Open a socket on " + _host + " with port " + std::to_string(_port));		
	}

	Core::~Core()
	{		
		debug("Core unloaded ...");
	}

	void Core::managePOST(std::unique_ptr<http::Data>& data)
	{
		if ((data->buffer.size() - data->position) < data->request.contentLength)
		{
			if (!_sock->receivePOST(data))
			{
				data->response.status = 500;
				return;
			}
		}

		if (data->request.isMultipart)
			elaborateMultipart(data);
		else
			getPOST(data);

		if (data->response.location == "")
			data->response.location = data->request.URI;

		data->response.status = 303; //REDIRECT FOR POST	

		//MANAGE HERE POST
	}

	void Core::generatePage(std::unique_ptr<http::Data>& data)
	{
		std::string path = std::filesystem::current_path().string() + "/Data/" + data->Conf.path + "style/main.style";
		std::ifstream file(path, std::ios::binary);
		if (file)
		{
			std::string line;
			while (std::getline(file, line)) 
			{
				line.erase(std::remove(line.begin(), line.end(), '\t'), line.end());
				line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
					
				data->response.content += line;
			}

			file.close();
		}
		
		if (data->response.content_type.empty())
			data->response.content_type = "html";

		getType(data);
	}

	void Core::prepareFile(std::unique_ptr<http::Data>& data)
	{
		if (!data->request.fileModifiedSince.empty())
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
			if (cachedUnixtime >= fileUnixtime)
			{
				data->response.status = 304;
				return;
			}
		}

		data->response.fileName = data->request.filePath;

		if (data->response.content.empty())
		{
			data->phase = 2; // SEND FILE

			data->response.fileSize = std::filesystem::file_size(data->response.fileName);
		}

		data->response.content_length = data->response.fileSize;
	}
	
	// Generates the response buffer from the fields in the struct
	void Core::getBuffer(std::unique_ptr<http::Data>& data)
	{
		std::ostringstream response;

		// HTTP Status Line
		response << "HTTP/1.1 " << http::Responses.at(data->response.status) << "\r\n";

		// Server information
		response << "Server: Orpy by M4iKZ\r\n";

		int status = data->response.status;
		if (status == 301 || status == 302 || status == 303 || status == 307)
		{
			// Redirection response
			response << "Location: " << data->response.location << "\r\n";
		}
		else
		{
			// Regular response
			response << "Content-Type: " << data->response.content_type << "\r\n";

			if (!data->response.attachmentName.empty())
			{
				// Download file instead of showing it
				response << "Content-Disposition: attachment; filename=" << data->response.attachmentName << "\r\n";
			}

			if (!data->response.content.empty() && data->response.content_length == 0)
				data->response.content_length = data->response.content.size();

			response << "Content-Length: " << data->response.content_length << "\r\n";

			if (data->response.isFile)
			{
				response << "Last-Modified: " << data->response.fileLastEdit << "\r\n";
				response << "Cache-Control: must-revalidate\r\n";
			}
		}

		response << "\r\n";

		// Convert the response string to a vector of characters
		data->buffer = response.str();
		
		if (!data->response.content.empty())
			data->buffer += data->response.content;

		// Set the total length of the buffer		
		data->length = data->buffer.size();
	}

	void Core::elaborateData(std::unique_ptr<http::Data>& data)
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
		getBuffer(data);
	}
}