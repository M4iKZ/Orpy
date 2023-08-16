#pragma once

#include <iostream>
#include <mutex>

#include "Common/data.hpp"
#include "Common/http.hpp"

namespace Orpy
{
	const size_t ReadSize = 1024 * 4 * 16;
	const size_t MaxFileSize = 1024 * 1024;
	const size_t MaxUploadSize = ReadSize * 64;	
}