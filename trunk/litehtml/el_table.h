#pragma once
#include "element.h"

namespace litehtml
{
	struct col_info
	{
		int		width;
		bool	is_auto;
	};


	class el_table : public element
	{
	public:
		el_table(litehtml::document* doc);
		virtual ~el_table();

		virtual int		render(uint_ptr hdc, int x, int y, int max_width);
		virtual bool	appendChild(litehtml::element* el);
	};
}