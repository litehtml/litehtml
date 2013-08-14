#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_break : public html_tag
	{
	public:
		el_break(litehtml::document* doc);
		virtual ~el_break();

		virtual bool	is_break() const;
		virtual void	parse_attributes();
	};
}
