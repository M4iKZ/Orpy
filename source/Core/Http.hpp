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
		void managePOST(std::unique_ptr<HTTPData>&);
		void generatePage(std::unique_ptr<HTTPData>&);
		void prepareFile(std::unique_ptr<HTTPData>&);
		int getBuffer(HttpResponse*, std::vector<char>&);

	public:
		Http();
		~Http();
				
		void elaborateData(std::unique_ptr<HTTPData>&) override;
	};
}