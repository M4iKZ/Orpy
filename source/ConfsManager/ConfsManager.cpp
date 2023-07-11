
#include "ConfsManager.hpp"

namespace Orpy
{
	std::shared_ptr<IConfs> _confs = nullptr;

	void loadConfs()
	{
		_confs = std::make_shared<ConfsManager>();
	}

	ConfsManager::ConfsManager()
	{
		_guard = new fileGuard<ConfsManager>(this, fs::current_path().string() + "/Configs/sites");

		debug("Confs Loaded ... ");
	}

	ConfsManager::~ConfsManager()
	{
		_confs.reset();

		debug("Confs lib unloaded ...");
	}

	void ConfsManager::New(fs::path file)
	{
		Load(file);

		debug(file.stem().string() + " added ... ");
	}

	void ConfsManager::Edited(fs::path file)
	{
		Load(file);

		debug(file.stem().string() + " updated ... ");
	}

	void ConfsManager::Deleted(std::string site)
	{
		std::unique_lock<std::shared_mutex> lock(_mutex);

		debug("try to delete " + site + " ... ");

		auto it = _sites.find(site);
		if (it != _sites.end())
		{
			_sites.erase(it);
			debug(site + " deleted ... ");
		}
	}

	void ConfsManager::Load(fs::path path)
	{
		std::unique_lock<std::shared_mutex> lock(_mutex);
		loadJsonConfig(path, _sites);
	}

	bool ConfsManager::Get(std::string host, SITEData& site)
	{
		std::unique_lock<std::shared_mutex> lock(_mutex);

		auto it = _sites.find(host);
		if (it == _sites.end())
			return false;

		site = it->second;

		return true;
	}
}