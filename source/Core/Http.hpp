#pragma once

#include <iostream>

#include "libHelper.hpp"

#include "IHttp.hpp"
#include "ISockets.hpp"
#include "ICreator.hpp"

#include "IConfs.hpp"

namespace Orpy
{
	class Http : public IHttp
	{
	protected:
		
		std::string _host = "localhost";
		int _port = 8888;

		DynamicLibrary<ISockets, IHttp*, const char*, int&> Sockets;
		DynamicLibrary<ICreator, IHttp*> Creator;

	public:

		Http();
		~Http();

		void elaborateData(HTTPData*, HTTPData*) override;		
		void receivePOST(HTTPData*, int) override;		
	};
}