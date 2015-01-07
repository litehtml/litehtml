#pragma once
#include "html_tag.h"

namespace litehtml
{
	struct col_info
	{
		int		width;
		bool	is_auto;
	};


	class el_table : public html_tag
	{
	public:
		el_table(litehtml::document* doc);
		virtual ~el_table();

		virtual bool	appendChild(litehtml::element* el);
		virtual void	parse_styles(bool is_reparse = false);
		virtual void	parse_attributes();
	};
}