#pragma once

#include <iostream>

#include "IHttp.hpp"
#include "ICreator.hpp"

//#include "Compressor.hpp"
#include "Parser.hpp"
#include "Router.hpp"
#include "Template.hpp"
//#include "Extensions.hpp"
//#include "Localization.hpp"

namespace Orpy
{	
	class Creator : public ICreator
	{
	private:
		void managePOST(HTTPData*, HttpRequest*);		
		void generatePage(HttpRequest*);		
		void prepareFile(HttpRequest*, HTTPData*);
		
	protected:
		IHttp* _http;
	public:
		Creator(IHttp*);
		~Creator() = default;

		void workOnData(HTTPData*, HTTPData*) override;
	};
}