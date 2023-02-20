#include "Http.hpp"

namespace Orpy
{
	IHttp* allHttp()
	{
		return new Http();
	}
			
#ifdef _WIN32
	Http::Http() : Sockets("Sockets.dll", "Sockets", this, _host.c_str(), _port), Creator("Creator.dll", "Creator", this)
#else
	Http::Http() : Sockets("libSockets.so", "Sockets", this, _host.c_str(), _port), Creator("libCreator.so", "Creator", this)
#endif
	{ 
		debug("Open a socket on " + _host + " with port " + std::to_string(_port));

		if(Sockets->start())
		{			
			loadConfs();
						
			debug("Core lib loaded ...");

			debug("Enter [quit] to stop the server");
			std::string command;
			while (std::cin >> command, command != "quit"); //Add Command Manager and Log Manager?

			debug("'quit' command entered. Stopping the web server ...");
		}
	}

	Http::~Http() 
	{ 
		debug("Core lib unloaded ..."); 
	}

	void Http::elaborateData(HTTPData* data, HTTPData* response)
	{	
		Creator->workOnData(data, response);
	}

	void Http::receivePOST(HTTPData* data, int size)
	{
		Sockets->receiveAll(data, size);
	}
}