#pragma once

#ifdef _WIN32
#ifdef _EXP_CONFS
#define MY_EXPORT __declspec(dllexport)
#endif 
#define MY_EXPORT __declspec(dllimport)
#else
#define MY_EXPORT
#endif

#include <filesystem>

#include "Common.hpp"

namespace fs = std::filesystem;

namespace Orpy
{
	class IConfs
	{
	public:
		~IConfs() = default;

		virtual void New(fs::path) = 0;
		virtual void Edited(fs::path) = 0;
		virtual void Deleted(std::string) = 0;

		virtual bool Get(std::string, SITEData&) = 0;
	};

	extern MY_EXPORT std::shared_ptr<IConfs> _confs;
	
	void loadConfs();
}