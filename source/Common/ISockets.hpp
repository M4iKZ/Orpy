#pragma once

#include "IHttp.hpp"

namespace Orpy
{
	class ISockets
	{
	public:
		virtual ~ISockets() = default;

		virtual bool start(bool = false) = 0;
		virtual void close(int, bool = false) = 0;										
	};
	
	extern "C"
	{
		ISockets* allSockets(IHttp*, const char*, int&);
	}
}