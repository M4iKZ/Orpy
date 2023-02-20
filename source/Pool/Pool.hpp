#pragma once

#include <iostream>
#include <queue>

namespace Orpy
{
	struct Pool
	{
		//phases: 0 request : 1 response
		void push(void* ptr)
		{
			q.push(ptr);
		}

		bool empty()
		{
			return q.empty();
		}

		void* get()
		{
			return q.front();
		}

		void pop()
		{
			q.pop();
		}

	protected:
		std::queue<void*> q;
	};
}