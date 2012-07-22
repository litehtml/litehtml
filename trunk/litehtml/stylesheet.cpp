#include "html.h"
#include "stylesheet.h"
#include "tokenizer.h"
#include <algorithm>


void litehtml::css::parse_stylesheet( const wchar_t* str, const wchar_t* baseurl, document_container* doc )
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
			std::wstring::size_type sPos = pos;
			pos = text.find(L";", pos);

			if(text.substr(sPos, 7) == L"@import")
			{
				sPos += 7;
				std::wstring iStr;
				if(pos == std::wstring::npos)
				{
					iStr = text.substr(sPos);
				} else
				{
					iStr = text.substr(sPos, pos - sPos);
				}
				trim(iStr);
				string_vector tokens;
				tokenize(iStr, tokens, L",", L"", L"()\"");
				if(!tokens.empty())
				{
					std::wstring url;
					parse_css_url(tokens.front(), url);
					if(url.empty())
					{
						url = tokens.front();
					}
					tokens.erase(tokens.begin());
					if(doc)
					{
						std::wstring css_text;
						std::wstring css_baseurl;
						if(baseurl)
						{
							css_baseurl = baseurl;
						}
						doc->import_css(css_text, url, css_baseurl, tokens);
						if(!css_text.empty())
						{
							parse_stylesheet(css_text.c_str(), css_baseurl.c_str(), doc);
						}
					}
				}
			}

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
			style::ptr st = new style;
			st->add(text.substr(style_start + 1, style_end - style_start - 1).c_str(), baseurl);

			parse_selectors(text.substr(pos, style_start - pos), st);

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

void litehtml::css::parse_css_url( const std::wstring& str, std::wstring& url )
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

void litehtml::css::parse_selectors( const std::wstring& txt, litehtml::style::ptr styles )
{
	std::wstring selector = txt;
	trim(selector);
	string_vector tokens;
	tokenize(selector, tokens, L",");

	for(string_vector::iterator tok = tokens.begin(); tok != tokens.end(); tok++)
	{
		css_selector::ptr selector = new css_selector;
		selector->m_style = styles;
		trim(*tok);
		selector->parse(*tok);
		selector->calc_specificity();
		add_selector(selector);
	}
}

void litehtml::css::sort_selectors()
{
	sort(m_selectors.begin(), m_selectors.end(), std::less<css_selector::ptr>( ));
}
