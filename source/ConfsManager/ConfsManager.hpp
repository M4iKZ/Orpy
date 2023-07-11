#pragma once

#include <shared_mutex>

#include "IConfs.hpp"
#include "Json.hpp"
#include "fileGuard.hpp"

namespace fs = std::filesystem;

namespace Orpy
{
	class ConfsManager : public IConfs
	{
	private:
		std::shared_mutex _mutex;

		fileGuard<ConfsManager>* _guard;

		std::unordered_map<std::string, SITEData> _sites;

		void Load(fs::path);

	public:
		ConfsManager();
		~ConfsManager();

		void New(fs::path) override;
		void Edited(fs::path) override;
		void Deleted(std::string) override;

		bool Get(std::string, SITEData&) override;
	};
}