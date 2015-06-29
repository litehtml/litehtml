#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_anchor : public html_tag
	{
	public:
		el_anchor(std::shared_ptr<litehtml::document>& doc);
		virtual ~el_anchor();

		virtual void	on_click();
		virtual void	apply_stylesheet(const litehtml::css& stylesheet);
	};
}