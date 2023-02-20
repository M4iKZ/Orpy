
#include <unordered_map>
#include <filesystem>

#include "Common.hpp"

namespace fs = std::filesystem;

namespace Orpy
{
	void loadJsonConfig(fs::path, std::unordered_map <std::string, SITEData>&);
}
