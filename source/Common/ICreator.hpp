#pragma once

namespace Orpy
{
	class ICreator
	{
	public:
		virtual ~ICreator() = default;

		virtual void workOnData(HTTPData*, HTTPData*) = 0;
	};

	extern "C"
	{
		ICreator* allCreator(IHttp*);
	}
}