#pragma once
#include "element.h"

namespace litehtml
{
	class el_anchor : public element
	{
	public:
		el_anchor(litehtml::document* doc);
		virtual ~el_anchor();

		virtual void on_click(int x, int y);
	};
}