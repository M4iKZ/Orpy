#pragma once

#include <filesystem>
#include <shared_mutex>
#include <unordered_map>

#include "Common/common.hpp"
#include "Common/IConf.hpp"

#include "fileGuard/fileGuard.hpp"
#include "Json/json.hpp"

namespace Orpy
{
	class Conf : public IConf
	{
	private:
		std::shared_mutex _mutex;

		std::unique_ptr<fileGuard<Conf>> _guard;

		std::unordered_map<std::string, site::Settings> _confs;

		std::filesystem::path confFolder;
		std::filesystem::path dataFolder;

		void Load(std::filesystem::path);
		void printHelp(const std::string&);
	public:
		Conf();
		~Conf();

		void New(std::filesystem::path) override;
		void Edited(std::filesystem::path) override;
		void Deleted(std::string) override;

		bool Get(std::string, site::Settings&) override;

		int Count() override;

		void Add() override;
	};
}