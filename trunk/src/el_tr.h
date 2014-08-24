#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_tr : public html_tag
	{
	public:
		el_tr(litehtml::document* doc);
		virtual ~el_tr();

		virtual void	parse_attributes();
		virtual void	get_inline_boxes(position::vector& boxes);
	};
}