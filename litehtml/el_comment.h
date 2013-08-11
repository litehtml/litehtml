#pragma once
#include "html_tag.h"

namespace litehtml
{
	class el_comment : public element
	{
		tstring	m_text;
	public:
		el_comment(litehtml::document* doc);
		virtual ~el_comment();

		virtual void	get_text(tstring& text);
		virtual void	set_data(const tchar_t* data);
	};
}
