
#include <filesystem>
#include <algorithm>
#include <vector>
#include <variant>

#include "Common.hpp"
#include "Request.hpp"

namespace Orpy
{
	void check_if_file_accessible(HttpRequest* request)
	{
		std::string url = request->URL.substr(1);
		if (url == "")
			return;

		std::string path = std::filesystem::current_path().string() + "/Data/" + request->Conf.path + url;
		if (std::filesystem::exists(path))
		{
			request->isFile = true;
			request->filePath = path;
		}
	}

	void check_url_from_conf(HttpRequest* request)
	{
		if (std::find(request->Conf.urls.begin(), request->Conf.urls.end(), request->commands.at(0)) != request->Conf.urls.end())
		{
			check_if_file_accessible(request);
			return;
		}
		else if (request->Conf.allowFiles)
		{
			check_if_file_accessible(request);

			if (request->isFile)
				return;
		}

		request->error = true;
		request->response.status = 404;
	}

	void checker(HttpRequest* request)
	{
		if ((request->Conf.urls.size() > 0 && request->nCommands > 0) || request->nCommands == 0)
			check_url_from_conf(request);
		else
		{
			request->error = true;
			request->response.status = 404;
		}
	}

	void check_URL(HttpRequest* request)
	{
		checker(request);

		if (request->isFile)
			return;
	}
}