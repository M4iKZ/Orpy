
#include <iostream>

#include "Common/ISockets.hpp"
#include "Common/IConf.hpp"
#include "Common/ICore.hpp"

namespace Orpy
{
	void start()
	{
		setConf();

		setSockets("localhost", 8888);
		if (_sock->start())
		{			
			setCore();

			if (!_conf->Count())
				_conf->Add();
			
			debug("Enter [quit] to stop the server");

			std::string command;
			while (std::cin >> command, command != "quit")
			{
				if (command == "add")
					_conf->Add();
			}

			debug("'quit' command entered. Stopping the web server ...");		
		}
	}
}

int main()
{	
	Orpy::start();
}
