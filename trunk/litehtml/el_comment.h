#pragma once
#include "element.h"

namespace litehtml
{
	class el_comment : public element
	{
		std::wstring	m_text;
	public:
		el_comment(litehtml::document* doc);
		virtual ~el_comment();

		virtual void get_text(std::wstring& text);
		virtual void set_data(const wchar_t* data);
	};
}
