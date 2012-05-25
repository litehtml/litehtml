#include "html.h"
#include "stylesheet.h"

litehtml::style_sheet::vector litehtml::master_stylesheet;

void litehtml::parse_stylesheet( const wchar_t* str, style_sheet::vector& styles, const wchar_t* baseurl )
{
	std::wstring text = str;

	// remove comments
	std::wstring::size_type c_start = text.find(L"/*");
	while(c_start != std::wstring::npos)
	{
		std::wstring::size_type c_end = text.find(L"*/", c_start + 2);
		text.erase(c_start, c_end - c_start + 2);
		c_start = text.find(L"/*");
	}

	std::wstring::size_type pos = text.find_first_not_of(L" \n\r\t");
	while(pos != std::wstring::npos)
	{
		while(text[pos] == L'@')
		{
			pos = text.find(L";", pos);
			if(pos == std::wstring::npos)
			{
				break;
			}
			pos++;
		}

		if(pos == std::wstring::npos)
		{
			break;
		}

		std::wstring::size_type style_start = text.find(L"{", pos);
		std::wstring::size_type style_end	= text.find(L"}", pos);
		if(style_start != std::wstring::npos && style_end != std::wstring::npos)
		{
			style_sheet::ptr st = new style_sheet;
			st->add_selector(text.substr(pos, style_start - pos));

			st->m_style.add(text.substr(style_start + 1, style_end - style_start - 1).c_str(), baseurl);
			styles.push_back(st);

			pos = style_end + 1;
		} else
		{
			pos = std::wstring::npos;
		}

		if(pos != std::wstring::npos)
		{
			pos = text.find_first_not_of(L" \n\r\t", pos);
		}
	}
}

void litehtml::load_master_stylesheet( const wchar_t* str )
{
	parse_stylesheet(str, master_stylesheet, NULL);
}

void litehtml::parse_css_url( const std::wstring& str, std::wstring& url )
{
	url = L"";
	size_t pos1 = str.find(L'(');
	size_t pos2 = str.find(L')');
	if(pos1 != std::wstring::npos && pos2 != std::wstring::npos)
	{
		url = str.substr(pos1 + 1, pos2 - pos1 - 1);
		if(url.length())
		{
			if(url[0] == L'\'' || url[0] == L'"')
			{
				url.erase(0, 1);
			}
		}
		if(url.length())
		{
			if(url[url.length() - 1] == L'\'' || url[url.length() - 1] == L'"')
			{
				url.erase(url.length() - 1, 1);
			}
		}
	}
}
