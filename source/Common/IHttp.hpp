#pragma once

#include "Common.hpp"

namespace Orpy
{	
	class IHttp
	{
	public:
		virtual ~IHttp() = default;

		virtual void elaborateData(HTTPData*, HTTPData*) = 0;		
		virtual void receivePOST(HTTPData*, int) = 0;		
	};

	extern "C" 
	{
		IHttp* allHttp();
	}
}