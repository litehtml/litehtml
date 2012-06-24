#pragma once

#include "element.h"

namespace litehtml
{

	class el_image : public element
	{
		std::wstring	m_src;
	public:
		el_image(litehtml::document* doc);
		virtual ~el_image(void);

		virtual void	parse_styles(bool is_reparse = false);

	protected:
		virtual void	get_content_size(size& sz, int max_width);
		virtual void	draw_content(uint_ptr hdc, const litehtml::position& pos);
	};
}
