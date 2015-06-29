#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_title : public html_tag
	{
	public:
		el_title(std::shared_ptr<litehtml::document>& doc);
		virtual ~el_title();

	protected:
		virtual void	parse_attributes();
	};
}
