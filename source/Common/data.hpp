#pragma once

#include <vector>
#include <string>
#include <unordered_map>

namespace Orpy
{
	namespace site
	{
		struct Settings
		{
			std::string path = "";
			
			std::string redirect = "";
			
			std::vector<std::string> supported_langs;

			std::vector<std::string> urls;
			std::vector<std::string> positions;
						
			std::string cookieDomain = "";

			bool allowFiles = true;
			bool enableMemo = false;

			std::unordered_map<std::string, std::string> mimetypes;
		};
	}
}