#pragma once

#include <sstream>

#include "Common/ICore.hpp"

#include "Common/common.hpp"
#include "Parser/parser.hpp"
#include "Router/router.hpp"

namespace Orpy
{
	class Core : public ICore
	{
	protected:
		std::string _host = "localhost";
		int _port = 8888;

	private:
		
		void managePOST(std::unique_ptr<http::Data>&);
		void generatePage(std::unique_ptr<http::Data>&);
		void prepareFile(std::unique_ptr<http::Data>&);

		void getBuffer(std::unique_ptr<http::Data>& data);

	public:
		Core();
		~Core();

		void elaborateData(std::unique_ptr<http::Data>&) override;
	};
}