#pragma once

#include <filesystem>

#include "Request.hpp"

namespace Orpy
{
	namespace Tools
	{
		void setPOSTAlert(HttpRequest* request, std::string str, std::string type = "error")
		{
			std::string alert = "";
			if (type == "info")
				alert = "?info=" + str;
			else
				alert = "?error=" + str;

			request->response.location = request->URI + alert;
		}

		std::string getConfPath(HttpRequest* request)
		{
			return std::filesystem::current_path().string() + "/Data/" + request->Conf.path;
		}
	}
}