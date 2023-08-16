#pragma once

#include <memory>

#include "Common/http.hpp"

namespace Orpy
{	
	class ICore
	{
	public:
		virtual ~ICore() = default;
			
		virtual void elaborateData(std::unique_ptr<http::Data>&) = 0;
	};

	void setCore();	

	extern std::shared_ptr<ICore> _core;
}