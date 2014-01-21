#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_anchor : public html_tag
	{
	public:
		el_anchor(litehtml::document* doc);
		virtual ~el_anchor();

		virtual void	on_click(int x, int y);
		virtual void	apply_stylesheet(const litehtml::css& stylesheet);
	};
}