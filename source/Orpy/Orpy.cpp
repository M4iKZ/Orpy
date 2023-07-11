#include <iostream>

#include "libHelper.hpp"
#include "IHttp.hpp"

using namespace Orpy;

int main()
{	
#ifdef _WIN32
		SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

		std::wstring Core = std::filesystem::current_path().wstring() + L"/Core";
		std::wstring Configs = std::filesystem::current_path().wstring() + L"/Configs";
		std::wstring Tools = std::filesystem::current_path().wstring() + L"/Tools";

		AddDllDirectory(Core.c_str());
		AddDllDirectory(Configs.c_str());
		AddDllDirectory(Tools.c_str());

		DynamicLibrary<IHttp> core("Core.dll", "Http");
#else
		DynamicLibrary<IHttp> core("libCore.so", "Http");
#endif
}
