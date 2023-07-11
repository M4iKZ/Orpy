#pragma once

#include <iostream>

#include "libHelper.hpp"

#include "IHttp.hpp"
#include "ISockets.hpp"

#include "IConfs.hpp"
#include "Parser.hpp"
#include "Router.hpp"

namespace Orpy
{
	class Http : public IHttp
	{
	protected:
		std::string _host = "localhost";
		int _port = 8888;

		DynamicLibrary<ISockets, IHttp*, const char*, int&> Sockets;		

	private:
		void managePOST(HTTPData*, HttpRequest*);
		void generatePage(HttpRequest*);
		void prepareFile(HttpRequest*, HTTPData*);

	public:
		Http();
		~Http();
				
		void elaborateData(HTTPData*) override;		
	};
}