
#include <iostream>

#include "Common/ISockets.hpp"
#include "Common/IConf.hpp"
#include "Common/ICore.hpp"

namespace Orpy
{
	void start()
	{
		setSockets("localhost", 8888);
		if (_sock->start())
		{			
			setConf();
			setCore();

			debug("Enter [quit] to stop the server");
			std::string command;
			while (std::cin >> command, command != "quit");

			debug("'quit' command entered. Stopping the web server ...");		
		}
	}
}

int main()
{	
	Orpy::start();
}
