#pragma once
#include "element.h"

namespace litehtml
{
	class el_td : public element
	{
	public:
		el_td(litehtml::document* doc);
		virtual ~el_td();

		virtual void	parse_styles(bool is_reparse);
	};
}