#pragma once

/*
#ifdef _WIN32
#ifdef _EXP_CONFS
#define MY_EXPORT __declspec(dllexport)
#endif 
#define MY_EXPORT __declspec(dllimport)
#else
#define MY_EXPORT
#endif
//*/

#include <shared_mutex>

#include "IConfs.hpp"
//#include "Common.hpp"
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
		//*/

		/*
		void New(fs::path);
		void Edited(fs::path);
		void Deleted(std::string);

		bool Get(std::string, SITEData&);
		//*/
	};

	//extern MY_EXPORT std::shared_ptr<ConfsManager> _confs;
	
	//void loadConfs();
}