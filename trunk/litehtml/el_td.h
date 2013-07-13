#pragma once
#include "element.h"

namespace litehtml
{
	class el_td : public element
	{
	public:
		el_td(litehtml::document* doc);
		virtual ~el_td();

		virtual void			parse_styles(bool is_reparse);
		virtual const wchar_t*	get_style_property(const wchar_t* name, bool inherited, const wchar_t* def = 0);
	};
}