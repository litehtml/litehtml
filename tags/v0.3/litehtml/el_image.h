#pragma once

#include "element.h"

namespace litehtml
{

	class el_image : public element
	{
		tstring	m_src;
	public:
		el_image(litehtml::document* doc);
		virtual ~el_image(void);

		virtual void	parse_styles(bool is_reparse = false);
		virtual int		line_height() const;
		virtual bool	is_replaced() const;
		virtual int		render(int x, int y, int max_width);
		virtual void	finish();
	protected:
		virtual void	get_content_size(size& sz, int max_width);
		virtual void	draw_content(uint_ptr hdc, const litehtml::position& pos);
	};
}
