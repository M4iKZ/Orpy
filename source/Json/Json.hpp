
#include <unordered_map>
#include <filesystem>

#include "Common/data.hpp"

namespace Orpy
{
	namespace json
	{
		void loadConfig(const std::filesystem::path&, std::unordered_map <std::string, site::Settings>&);
		void saveConfig(const std::string&, const std::string&, const site::Settings&);
	}
}
