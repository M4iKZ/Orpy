#include "Creator.hpp"

#include <filesystem>

namespace Orpy
{
	ICreator* allCreator(IHttp* ptr)
	{
		return new Creator(ptr);
	}

	Creator::Creator(IHttp* ptr) : _http(ptr) { std::cout << "Creator Lib loaded ... "; }

	void Creator::managePOST(HTTPData* data, HttpRequest* request)	
	{
		if (request->contentLength > (data->buffer.size() - request->position))
			_http->receivePOST(data, request->contentLength);

		if (request->isMultipart)
		{
			elaborateMultipart(data->buffer, request);
		}
		else
		{
			getPOST(data->buffer, request);
		}

		//DO

		if (request->response.location == "")
			request->response.location = request->URI;

		request->response.status = 303; //REDIRECT FOR POST	
	}

	void Creator::generatePage(HttpRequest* request)
	{
		setPage();

		request->response.content = R"(<body style="background-color:#333; color:#f3f3f3; ">
			Hello my friend that joined me from )" + request->Host + R"( looking for )" + request->URL + R"( with )" + std::to_string(request->nCommands) + R"( commands. 
			Your language: )" + request->lang + R"(
			</body>)";		

		request->response.content_length = request->response.content.length();

		std::string type = "html";
		getType(type);

		request->response.content_type = type;
	}

	void Creator::prepareFile(HttpRequest* request, HTTPData* response)	
	{
		std::filesystem::path file_path(request->URL);
		std::string file_extension = file_path.extension().string().substr(1);
		if (getType(file_extension))
		{
			response->phase = 3;

			response->isFile = true;
			response->fileName = request->filePath;
			response->fileSize = std::filesystem::file_size(response->fileName);

			request->response.content_type = file_extension;
			request->response.content_length = response->fileSize;
		}
		else
		{
			request->response.status = 404;
		}
	}

	void Creator::workOnData(HTTPData* data, HTTPData* response)
	{	
		HttpRequest* request = new HttpRequest();
		parser(request, data->buffer);

		if (!request->error)
		{
			check_URL(request);
						
			if (request->isFile)
				prepareFile(request, response);
			else if (request->isPOST)
				managePOST(data, request);
			else if(!request->error)
				generatePage(request);			
		}
					
		request->response.getBuffer(response->buffer);
		response->length = request->response.bufferSize;
				
		delete request; 
	}
}