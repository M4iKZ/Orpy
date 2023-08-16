
#include "router.hpp"

namespace Orpy
{
	void check_if_file_accessible(std::unique_ptr<http::Data>& data)
	{
		std::string url = data->request.URL.substr(1);
		if (url == "" || url[0] == '/')
			return;

		std::string path = std::filesystem::current_path().string() + "/Data/" + data->Conf.path + url;
		if (std::filesystem::exists(path))
		{
			data->request.isFile = true;
			data->request.filePath = path;
		}
	}

	void check_url_from_conf(std::unique_ptr<http::Data>& data)
	{
		if (std::find(data->Conf.urls.begin(), data->Conf.urls.end(), data->request.commands.at(0)) != data->Conf.urls.end())
		{
			check_if_file_accessible(data);
			return;
		}
		else if (data->Conf.allowFiles)
		{
			check_if_file_accessible(data);

			if (data->request.isFile)
				return;
		}

		data->error = true;
		data->response.status = 404;
	}

	void check_URL(std::unique_ptr<http::Data>& data)
	{
		if ((data->Conf.urls.size() > 0 && data->request.nCommands > 0) || data->request.nCommands == 0)
			check_url_from_conf(data);
		else
		{
			data->error = true;
			data->response.status = 404;
		}
	}
}