
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
}