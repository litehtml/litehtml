#pragma once
#include "element.h"

namespace litehtml
{
	class el_body : public element
	{
	public:
		el_body(litehtml::document* doc);
		virtual ~el_body();

		virtual bool is_body();
	};
}