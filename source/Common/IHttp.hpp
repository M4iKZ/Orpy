#pragma once

#include "Common.hpp"

namespace Orpy
{	
	class IHttp
	{
	public:
		virtual ~IHttp() = default;
			
		virtual void elaborateData(std::unique_ptr<HTTPData>&) = 0;
	};

	extern "C" 
	{
		IHttp* allHttp();
	}
}