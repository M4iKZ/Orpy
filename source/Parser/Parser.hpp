#pragma once

#include <vector>

#include "Common.hpp"

#include "Request.hpp"

namespace Orpy
{
	void getPOST(std::unique_ptr<HTTPData>&);
	void elaborateMultipart(std::unique_ptr<HTTPData>&);

	bool getType(std::unique_ptr<HTTPData>&);

	void parser(std::unique_ptr<HTTPData>&);
}