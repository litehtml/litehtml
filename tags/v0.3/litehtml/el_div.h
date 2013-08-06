#pragma once
#include "element.h"

namespace litehtml
{
	class el_div : public element
	{
	public:
		el_div(litehtml::document* doc);
		virtual ~el_div();

		virtual void finish();
	};
}