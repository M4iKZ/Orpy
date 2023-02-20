
#include <iostream>
#include <fstream>

#include "Json.hpp"
#include "nlohmann/Json.hpp"

using json = nlohmann::json;

namespace Orpy
{
	void conf_from_json(const json j, SITEData& site)
	{
		site.path = j.at("path").get<std::string>();
		
		std::vector<std::string> langs;
		site.langs = j.value("lang", langs);
				
		std::vector<std::string> urls;
		site.urls = j.value("urls", urls);		
		
		site.allowFiles = j.value("allowFiles", false);
		site.DB = j.value("DB", "");
	}

	void loadJsonConfig(fs::path json_file_path, std::unordered_map<std::string, SITEData>& archive)
	{
		std::ifstream json_file(json_file_path);

		try
		{
			json jsonData = json::parse(json_file);

			SITEData site;			
			conf_from_json(jsonData, site);

			archive[json_file_path.stem().string()] = site;
		}
		catch (json::parse_error& e)
		{
			std::cerr << "Error parsing config file " << json_file_path.stem().string() << " with error : " << e.what() << std::endl;
			// Discard the file and continue running the program
		}

		json_file.close();
	}
}