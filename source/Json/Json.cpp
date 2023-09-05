
#include <iostream>
#include <fstream>
#include <sstream>

#include "nlohmann/Json.hpp"
#include "json.hpp"

namespace Orpy
{
	namespace json
	{
		void conf_from_json(const nlohmann::json& j, site::Settings& site, const std::filesystem::path& file)
		{
			site.redirect = j.value("redirect", "");
			if (!site.redirect.empty())
				return;

			site.path = j.at("path").get<std::string>();

			std::vector<std::string> langs = { "en" };
			site.supported_langs = j.value("lang", langs);

			std::vector<std::string> urls = {};
			site.urls = j.value("urls", urls);
			std::vector<std::string> positions = {};
			site.positions = j.value("positions", positions);

			std::unordered_map<std::string, std::string> mimetypes = {};
			site.mimetypes = j.value("mimetypes", mimetypes);

			site.allowFiles = j.value("allowFiles", true);
			site.enableMemo = j.value("memo", false);
		}

		void loadConfig(const std::filesystem::path& file_path, std::unordered_map<std::string, site::Settings>& archive)
		{
			std::ifstream json_file(file_path, std::ios::binary);

			try
			{
				nlohmann::json jsonData = nlohmann::json::parse(json_file);

				site::Settings site;
				conf_from_json(jsonData, site, file_path);

				archive[file_path.stem().string()] = site;
			}
			catch (nlohmann::json::parse_error& e)
			{
				// PUT INTO LOGGER!
				std::cerr << "Error parsing config file " << file_path.stem().string() << " with error : " << e.what() << std::endl;				
			}

			json_file.close();
		}

		void saveConfig(const std::string& filePath, const std::string& fileName, const site::Settings& site)
		{
			try
			{
				nlohmann::json j;

				if (!site.redirect.empty())
				{
					j["redirect"] = site.redirect;
					return;
				}

				if (site.path.empty())
					return;

				j["path"] = site.path;
								
				if (site.supported_langs.size() > 0)
				{
					nlohmann::json langsArray = nlohmann::json::array();
					for (const std::string& lang : site.supported_langs)
						langsArray.push_back(lang);

					j["langs"] = langsArray;
				}
				
				if (site.urls.size() > 0)
				{
					nlohmann::json urlsArray = nlohmann::json::array();

					for (const std::string& url : site.urls)
						urlsArray.push_back(url);

					j["urls"] = urlsArray;
				}

				if(!site.allowFiles)
					j["allowFiles"] = site.allowFiles;

				std::ofstream confFile(filePath + "/" + fileName + ".json");
				confFile << j.dump(3);
				confFile.close();
			}
			catch (nlohmann::json::parse_error& e)
			{
				// PUT INTO LOGGER!
				std::cerr << "Error save config file " << fileName << " with error : " << e.what() << std::endl;
			}
		}
	}
}