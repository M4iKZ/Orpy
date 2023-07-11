#pragma once

#include <vector>

#include "Common.hpp"

#include "Request.hpp"

namespace Orpy
{
	void getPOST(std::vector<char>&, HttpRequest*);	
	void elaborateMultipart(std::vector<char>&, HttpRequest*);	

	bool getType(HttpRequest*, std::string&);

	void parser(HttpRequest*, std::vector<char>&);	
}