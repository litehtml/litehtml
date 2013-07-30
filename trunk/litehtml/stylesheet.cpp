#include "html.h"
#include "stylesheet.h"
#include "tokenizer.h"
#include <algorithm>


void litehtml::css::parse_stylesheet( const tchar_t* str, const tchar_t* baseurl, document_container* doc )
{
	tstring text = str;

	// remove comments
	tstring::size_type c_start = text.find(_t("/*"));
	while(c_start != tstring::npos)
	{
		tstring::size_type c_end = text.find(_t("*/"), c_start + 2);
		text.erase(c_start, c_end - c_start + 2);
		c_start = text.find(_t("/*"));
	}

	tstring::size_type pos = text.find_first_not_of(_t(" \n\r\t"));
	while(pos != tstring::npos)
	{
		while(text[pos] == _t('@'))
		{
			tstring::size_type sPos = pos;
			pos = text.find(_t(";"), pos);

			if(text.substr(sPos, 7) == _t("@import"))
			{
				sPos += 7;
				tstring iStr;
				if(pos == tstring::npos)
				{
					iStr = text.substr(sPos);
				} else
				{
					iStr = text.substr(sPos, pos - sPos);
				}
				trim(iStr);
				string_vector tokens;
				tokenize(iStr, tokens, _t(","), _t(""), _t("()\""));
				if(!tokens.empty())
				{
					tstring url;
					parse_css_url(tokens.front(), url);
					if(url.empty())
					{
						url = tokens.front();
					}
					tokens.erase(tokens.begin());
					if(doc)
					{
						tstring css_text;
						tstring css_baseurl;
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

			if(pos == tstring::npos)
			{
				break;
			}
			pos++;
		}

		if(pos == tstring::npos)
		{
			break;
		}

		tstring::size_type style_start = text.find(_t("{"), pos);
		tstring::size_type style_end	= text.find(_t("}"), pos);
		if(style_start != tstring::npos && style_end != tstring::npos)
		{
			style::ptr st = new style;
			st->add(text.substr(style_start + 1, style_end - style_start - 1).c_str(), baseurl);

			parse_selectors(text.substr(pos, style_start - pos), st);

			pos = style_end + 1;
		} else
		{
			pos = tstring::npos;
		}

		if(pos != tstring::npos)
		{
			pos = text.find_first_not_of(_t(" \n\r\t"), pos);
		}
	}
}

void litehtml::css::parse_css_url( const tstring& str, tstring& url )
{
	url = _t("");
	size_t pos1 = str.find(_t('('));
	size_t pos2 = str.find(_t(')'));
	if(pos1 != tstring::npos && pos2 != tstring::npos)
	{
		url = str.substr(pos1 + 1, pos2 - pos1 - 1);
		if(url.length())
		{
			if(url[0] == _t('\'') || url[0] == _t('"'))
			{
				url.erase(0, 1);
			}
		}
		if(url.length())
		{
			if(url[url.length() - 1] == _t('\'') || url[url.length() - 1] == _t('"'))
			{
				url.erase(url.length() - 1, 1);
			}
		}
	}
}

void litehtml::css::parse_selectors( const tstring& txt, litehtml::style::ptr styles )
{
	tstring selector = txt;
	trim(selector);
	string_vector tokens;
	tokenize(selector, tokens, _t(","));

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
