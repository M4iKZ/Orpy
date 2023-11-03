
#include "router.hpp"

namespace Orpy
{
	bool getType(std::unique_ptr<http::Data>& data)
	{
		//CUSTOM SITE MIME TYPES
		if (!data->Conf.mimetypes.empty())
		{
			auto conf = data->Conf.mimetypes.find(data->response.content_type);
			if (conf != data->Conf.mimetypes.end())
			{
				data->response.content_type = conf->second;
				return true;
			}
		}

		//ORPY DEFAULT MIME TYPES
		auto it = http::ContentType.find(data->response.content_type);
		if (it == http::ContentType.end())
			return false;

		data->response.content_type = it->second;

		return true;
	}

	void check_if_file_accessible(std::unique_ptr<http::Data>& data)
	{
		std::string url = data->request.URL.substr(1);
		if (url == "" || url[0] == '/')
			return;

		std::filesystem::path file_path(data->request.URL);
		data->response.content_type = file_path.extension().string();
		if (!data->response.content_type.empty())
			data->response.content_type = data->response.content_type.substr(1);

		if (getType(data))
		{
			std::string path = std::filesystem::current_path().string() + "/Data/" + data->Conf.path + url;

			if (!std::filesystem::exists(path))
			{
				data->response.content_type.clear();
				return;
			}

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
		check_url_from_conf(data);		
	}
}