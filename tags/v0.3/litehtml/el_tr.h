#pragma once
#include "element.h"

namespace litehtml
{
	class el_tr : public element
	{
	public:
		el_tr(litehtml::document* doc);
		virtual ~el_tr();

		virtual void	finish();
		virtual void	get_inline_boxes(position::vector& boxes);
	};
}