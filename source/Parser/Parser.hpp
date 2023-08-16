#pragma once

#include <string>
#include <vector>
#include <sstream>

#include "Conf/conf.hpp"
#include "Common/http.hpp"

namespace Orpy
{
	void getPOST(std::unique_ptr<http::Data>&);
	void elaborateMultipart(std::unique_ptr<http::Data>&);

	bool getType(std::unique_ptr<http::Data>&);

	void parser(std::unique_ptr<http::Data>&);
}