#pragma once

#include <memory>

namespace Orpy
{
	class ISockets
	{
	public:
		virtual ~ISockets() = default;

		virtual bool start(bool = false) = 0;									
	};

	void setSockets(const std::string&, const int&);

	extern std::shared_ptr<ISockets> _sock;
}
