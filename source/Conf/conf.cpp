
#include "conf.hpp"

namespace Orpy
{
	std::shared_ptr<IConf> _conf = nullptr;

	void setConf()
	{
		_conf = std::make_shared<Conf>();
	}

	Conf::Conf()
	{
		confFolder =  std::filesystem::current_path() / "Conf/sites";		
		dataFolder =  std::filesystem::current_path() / "Data";		

		try
		{
			if (!std::filesystem::exists(confFolder))
				std::filesystem::create_directories(confFolder);

			if (!std::filesystem::exists(dataFolder))
				std::filesystem::create_directories(dataFolder);
		}	
		catch (const std::exception& e) 
		{
			std::cerr << "Error: " << e.what() << std::endl;
		}

		_guard = std::make_unique<fileGuard<Conf>>(this, std::filesystem::current_path().string() + "/Conf/sites");
		
		debug("Confs Loaded ... ");
	}

	Conf::~Conf()
	{
		_guard.reset();

		debug("Confs unloaded ...");
	}

	void Conf::New(std::filesystem::path file)
	{
		Load(file);

		debug(file.stem().string() + " added ... ");
	}

	void Conf::Edited(std::filesystem::path file)
	{
		Load(file);

		debug(file.stem().string() + " updated ... ");
	}

	void Conf::Deleted(std::string site)
	{
		{
			std::unique_lock<std::shared_mutex> lock(_mutex);

			auto it = _confs.find(site);
			if (it != _confs.end())
			{
				_confs.erase(it);
				debug(site + " deleted ... ");
			}
		}
	}

	void Conf::Load(std::filesystem::path path)
	{
		{
			std::unique_lock<std::shared_mutex> lock(_mutex);
			json::loadConfig(path, _confs);
		}
	}

	bool Conf::Get(std::string host, site::Settings& site)
	{
		{
			std::unique_lock<std::shared_mutex> lock(_mutex);

			auto it = _confs.find(host);
			if (it == _confs.end())
				return false;

			site = it->second;

			return true;
		}
	}

	int Conf::Count()
	{	
		std::unique_lock<std::shared_mutex> lock(_mutex);
		return (int)_confs.size();		
	}

	void Conf::printHelp(const std::string& part)
	{
		if (part == "redirect")
			debug("insert the redirect URL with http/https, press enter to jump to next field");
		else if (part == "path")
			debug("insert the Data path where domain files are stored");
		else if (part == "urls")
			debug("insert the main url supported");
		else if (part == "langs")
			debug("insert the main language supported (default en)");
		else if (part == "allowfiles")
			debug("enable the serving of supported MIME types for files (t, true, f, false | default true)");
	}

	void Conf::Add()
	{
		std::string domain;
		site::Settings site;
		std::string command;

		std::vector<std::string> parts = { "redirect", "path", "urls", "langs", "allowfiles" };

		debug("Enter domain name without schema and port, this will be used as config file name (ex. localhost)");
		while (std::cin >> command)
		{
			if (command == "")
				continue;

			domain = command;
			break;
		}

		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		debug("Enter [!end] to quit the creation...");
		for (int i = 0; i < parts.size();)
		{
			printHelp(parts[i]);

			std::getline(std::cin, command);
			
			if (command == "!end")
				break;
			
			if (parts[i] == "redirect")
			{
				if (!command.empty())
				{
					site.redirect = command;
					break;
				}
			}
			else if (parts[i] == "path")
			{
				if (command.empty())
				{
					debug("path cannot be empty");
					continue;
				}

				if (command.back() != '/')
					command += '/';	

				site.path = command;
			}
			else if (parts[i] == "langs")
			{
				if (!command.empty())
				{
					site.supported_langs.push_back(command);
				}
			}
			else if (parts[i] == "urls")			
				site.urls.push_back(command);
			else if (parts[i] == "allowfiles")			
				if (command == "f" || command == "false")
					site.allowFiles = false;			

			++i;
		}
		
		if (command != "!end")
		{
			if (!std::filesystem::exists(dataFolder / site.path))
			{
				std::filesystem::create_directories(dataFolder / site.path);
				std::filesystem::create_directories(dataFolder / site.path / "css");
				std::filesystem::create_directories(dataFolder / site.path / "js");
				std::filesystem::create_directories(dataFolder / site.path / "lang");
				std::filesystem::create_directories(dataFolder / site.path / "style");

				std::ofstream styleFile(dataFolder / site.path / "style" / "main.style");
				styleFile << "Hello World";
				styleFile.close();
			}

			json::saveConfig(confFolder.string(), domain, site);
		}
	}
}