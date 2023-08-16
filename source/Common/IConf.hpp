#pragma once

#include <filesystem>

#include "Common/data.hpp"

namespace Orpy
{
	class IConf
	{
	public:
		~IConf() = default;

		virtual void New(std::filesystem::path) = 0;
		virtual void Edited(std::filesystem::path) = 0;
		virtual void Deleted(std::string) = 0;

		virtual bool Get(std::string, site::Settings&) = 0;
	};

	void setConf();

	extern std::shared_ptr<IConf> _conf;
}