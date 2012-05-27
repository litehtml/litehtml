#pragma once
#include "style.h"

namespace litehtml
{
	void	parse_stylesheet(const wchar_t* str, style_sheet::vector& styles, const wchar_t* baseurl);
	void	parse_css_url(const std::wstring& str, std::wstring& url);
}
