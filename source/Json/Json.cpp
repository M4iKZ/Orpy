
#include <iostream>
#include <fstream>
#include <sstream>

#include "Json.hpp"
#include "nlohmann/Json.hpp"

using json = nlohmann::json;

namespace Orpy
{
	void conf_from_json(const json j, SITEData& site, fs::path file)
	{
		site.redirect = j.value("redirect", "");
		if (site.redirect != "")
			return;

		site.path = j.at("path").get<std::string>();

		std::vector<std::string> langs = {};
		site.langs = j.value("lang", langs);

		std::vector<std::string> positions = {};
		site.positions = j.value("positions", positions);

		site.allowFiles = j.value("allowFiles", true);
				
		std::unordered_map<std::string, std::string> mimetypes = {};
		site.mimetypes = j.value("mimetypes", mimetypes);

		std::vector<std::string> urls = {};
		site.urls = j.value("urls", urls);
	}

	void loadJsonConfig(fs::path file_path, std::unordered_map<std::string, SITEData>& archive)
	{
		std::ifstream json_file(file_path, std::ios::binary);

		try
		{
			json jsonData = json::parse(json_file);

			SITEData site;
			conf_from_json(jsonData, site, file_path);

			archive[file_path.stem().string()] = site;
		}
		catch (json::parse_error& e)
		{
			std::cerr << "Error parsing config file " << file_path.stem().string() << " with error : " << e.what() << std::endl;
			// Discard the file and continue running the program
		}

		json_file.close();
	}
}