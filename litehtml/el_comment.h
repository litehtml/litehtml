#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_comment : public html_tag
	{
		tstring	m_text;
	public:
		el_comment(litehtml::document* doc);
		virtual ~el_comment();

		virtual void				get_text(tstring& text);
		virtual void				set_data(const tchar_t* data);

		virtual void				apply_stylesheet(const litehtml::css& stylesheet);
		virtual void				parse_styles(bool is_reparse);
		virtual int					get_base_line();
	};
}
