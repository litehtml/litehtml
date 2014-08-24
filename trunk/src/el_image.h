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
		virtual void	parse_attributes();
		virtual void	parse_styles(bool is_reparse = false);
		virtual void	draw(uint_ptr hdc, int x, int y, const position* clip);
		virtual void	get_content_size(size& sz, int max_width);
	};
}
