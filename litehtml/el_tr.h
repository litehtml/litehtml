#pragma once
#include "element.h"

namespace litehtml
{
	class el_tr : public element
	{
	public:
		el_tr(litehtml::document* doc);
		virtual ~el_tr();

		virtual void	parse_styles(bool is_reparse);
		virtual void	get_inline_boxes(position::vector& boxes);
	};
}