#pragma once

#include "style.h"

namespace litehtml
{
	extern style_sheet::vector master_stylesheet;
	void	parse_stylesheet(const wchar_t* str, style_sheet::vector& styles, const wchar_t* baseurl);
	void	load_master_stylesheet(const wchar_t* str);
	void	parse_css_url(const std::wstring& str, std::wstring& url);
}
