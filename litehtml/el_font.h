#pragma once
#include "element.h"

namespace litehtml
{
	class el_font : public element
	{
	public:
		el_font(litehtml::document* doc);
		virtual ~el_font();

		virtual void parse_styles(bool is_reparse);
	};
}