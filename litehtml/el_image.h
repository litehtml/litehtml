#pragma once

#include "html_tag.h"

namespace litehtml
{

	class el_image : public html_tag
	{
		tstring	m_src;
	public:
		el_image(litehtml::document* doc);
		virtual ~el_image(void);

		virtual int		line_height() const;
		virtual bool	is_replaced() const;
		virtual int		render(int x, int y, int max_width);
		virtual void	finish();
	protected:
		virtual void	get_content_size(size& sz, int max_width);
		virtual void	draw_content(uint_ptr hdc, const litehtml::position& pos);
	};
}
