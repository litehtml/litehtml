#pragma once
#include "html_tag.h"
#include "table.h"

namespace litehtml
{
	struct col_info
	{
		int		width;
		bool	is_auto;
	};


	class el_table : public html_tag
	{
		table_grid		m_grid;
		css_length		m_css_border_spacing_x;
		css_length		m_css_border_spacing_y;
		int				m_border_spacing_x;
		int				m_border_spacing_y;
		border_collapse	m_border_collapse;
	public:
		el_table(litehtml::document* doc);
		virtual ~el_table();

		virtual int		render(int x, int y, int max_width);
		virtual bool	appendChild(litehtml::element* el);
		virtual void	parse_styles(bool is_reparse = false);
		virtual void	draw_children( uint_ptr hdc, int x, int y, const position* clip, draw_flag flag, int zindex );
		virtual void	parse_attributes();

	protected:
		virtual void	init();
	};
}