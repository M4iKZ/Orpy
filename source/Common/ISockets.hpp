#pragma once

#include <memory>

#include "Common/common.hpp"

namespace Orpy
{
	class ISockets
	{
	public:
		virtual ~ISockets() = default;

		virtual bool start(bool = false) = 0;		
		virtual bool receivePOST(std::unique_ptr<http::Data>&) = 0;
	};

	void setSockets(const std::string&, const int&);

	extern std::shared_ptr<ISockets> _sock;
}
