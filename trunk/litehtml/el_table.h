#pragma once
#include "element.h"
#include "table.h"

namespace litehtml
{
	struct col_info
	{
		int		width;
		bool	is_auto;
	};


	class el_table : public element
	{
		table_grid		m_grid;
		css_length		m_css_border_spacing_x;
		css_length		m_css_border_spacing_y;
		int				m_border_spacing_x;
		int				m_border_spacing_y;
	public:
		el_table(litehtml::document* doc);
		virtual ~el_table();

		virtual int		render(int x, int y, int max_width);
		virtual bool	appendChild(litehtml::element* el);
		virtual void	parse_styles(bool is_reparse = false);
		virtual void	draw(uint_ptr hdc, int x, int y, const position* clip);

	protected:
		virtual void	init();
	};
}