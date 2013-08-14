#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_para : public html_tag
	{
	public:
		el_para(litehtml::document* doc);
		virtual ~el_para();

		virtual void	parse_attributes();

	};
}