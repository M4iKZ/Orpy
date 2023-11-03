#pragma once

#include <filesystem>
#include <algorithm>
#include <vector>
#include <variant>

#include "Common/http.hpp"

namespace Orpy
{
	void check_URL(std::unique_ptr<http::Data>&);

	bool getType(std::unique_ptr<http::Data>&);
}