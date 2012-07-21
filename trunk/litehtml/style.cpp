#include "html.h"
#include "style.h"
#include "tokenizer.h"
#include <functional>
#include <algorithm>
#include <locale>

litehtml::style::style()
{
}

litehtml::style::style( const style& val )
{
	m_properties = val.m_properties;
}

litehtml::style::~style()
{

}

void litehtml::style::parse( const wchar_t* txt, const wchar_t* baseurl )
{
	std::vector<std::wstring> properties;
	tokenize(txt, properties, L";");

	for(std::vector<std::wstring>::const_iterator i = properties.begin(); i != properties.end(); i++)
	{
		parse_property(*i, baseurl);
	}
}

void litehtml::style::parse_property( const std::wstring& txt, const wchar_t* baseurl )
{
	std::wstring::size_type pos = txt.find_first_of(L":");
	if(pos != std::wstring::npos)
	{
		std::wstring name	= txt.substr(0, pos);
		std::wstring val	= txt.substr(pos + 1);

		trim(name);
		trim(val);

		if(!name.empty() && !val.empty())
		{
			add_property(name.c_str(), val.c_str(), baseurl);
		}
	}
}

void litehtml::style::combine( const litehtml::style& src )
{
	for(string_map::const_iterator i = src.m_properties.begin(); i != src.m_properties.end(); i++)
	{
		m_properties[i->first] = i->second;
	}
}

void litehtml::style::add_property( const wchar_t* name, const wchar_t* val, const wchar_t* baseurl )
{
	if(!name || !val)
	{
		return;
	}

	// Add baseurl for background image 
	if(	!wcscmp(name, L"background-image"))
	{
		m_properties[name] = val;
		if(baseurl)
		{
			m_properties[L"background-image-baseurl"] = baseurl;
		}
	} else

	// Parse border spacing properties 
	if(	!wcscmp(name, L"border-spacing"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ");
		if(tokens.size() == 1)
		{
			add_property(L"-litehtml-border-spacing-x", tokens[0].c_str(), baseurl);
			add_property(L"-litehtml-border-spacing-y", tokens[0].c_str(), baseurl);
		} else if(tokens.size() == 2)
		{
			add_property(L"-litehtml-border-spacing-x", tokens[0].c_str(), baseurl);
			add_property(L"-litehtml-border-spacing-y", tokens[1].c_str(), baseurl);
		}
	} else

	// Parse borders shorthand properties 

	if(	!wcscmp(name, L"border"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ", L"", L"()");
		int idx;
		std::wstring str;
		for(string_vector::const_iterator tok = tokens.begin(); tok != tokens.end(); tok++)
		{
			idx = value_index(tok->c_str(), border_style_strings, -1);
			if(idx >= 0)
			{
				add_property(L"border-left-style", tok->c_str(), baseurl);
				add_property(L"border-right-style", tok->c_str(), baseurl);
				add_property(L"border-top-style", tok->c_str(), baseurl);
				add_property(L"border-bottom-style", tok->c_str(), baseurl);
			} else
			{
				if(web_color::is_color(tok->c_str()))
				{
					add_property(L"border-left-color", tok->c_str(), baseurl);
					add_property(L"border-right-color", tok->c_str(), baseurl);
					add_property(L"border-top-color", tok->c_str(), baseurl);
					add_property(L"border-bottom-color", tok->c_str(), baseurl);
				} else
				{
					add_property(L"border-left-width", tok->c_str(), baseurl);
					add_property(L"border-right-width", tok->c_str(), baseurl);
					add_property(L"border-top-width", tok->c_str(), baseurl);
					add_property(L"border-bottom-width", tok->c_str(), baseurl);
				}
			}
		}
	} else if(	!wcscmp(name, L"border-left")	||
		!wcscmp(name, L"border-right")	||
		!wcscmp(name, L"border-top")	||
		!wcscmp(name, L"border-bottom") )
	{
		string_vector tokens;
		tokenize(val, tokens, L" ", L"", L"()");
		int idx;
		std::wstring str;
		for(string_vector::const_iterator tok = tokens.begin(); tok != tokens.end(); tok++)
		{
			idx = value_index(tok->c_str(), border_style_strings, -1);
			if(idx >= 0)
			{
				str = name;
				str += L"-style";
				add_property(str.c_str(), tok->c_str(), baseurl);
			} else
			{
				if(web_color::is_color(tok->c_str()))
				{
					str = name;
					str += L"-color";
					add_property(str.c_str(), tok->c_str(), baseurl);
				} else
				{
					str = name;
					str += L"-width";
					add_property(str.c_str(), tok->c_str(), baseurl);
				}
			}
		}
	} else 

	// Parse border radius shorthand properties 
	if(!wcscmp(name, L"border-bottom-left-radius"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ");
		if(tokens.size() >= 2)
		{
			add_property(L"border-bottom-left-radius-x", tokens[0].c_str(), baseurl);
			add_property(L"border-bottom-left-radius-y", tokens[1].c_str(), baseurl);
		} else if(tokens.size() == 1)
		{
			add_property(L"border-bottom-left-radius-x", tokens[0].c_str(), baseurl);
			add_property(L"border-bottom-left-radius-y", tokens[0].c_str(), baseurl);
		}

	} else if(!wcscmp(name, L"border-bottom-right-radius"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ");
		if(tokens.size() >= 2)
		{
			add_property(L"border-bottom-right-radius-x", tokens[0].c_str(), baseurl);
			add_property(L"border-bottom-right-radius-y", tokens[1].c_str(), baseurl);
		} else if(tokens.size() == 1)
		{
			add_property(L"border-bottom-right-radius-x", tokens[0].c_str(), baseurl);
			add_property(L"border-bottom-right-radius-y", tokens[0].c_str(), baseurl);
		}

	} else if(!wcscmp(name, L"border-top-right-radius"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ");
		if(tokens.size() >= 2)
		{
			add_property(L"border-top-right-radius-x", tokens[0].c_str(), baseurl);
			add_property(L"border-top-right-radius-y", tokens[1].c_str(), baseurl);
		} else if(tokens.size() == 1)
		{
			add_property(L"border-top-right-radius-x", tokens[0].c_str(), baseurl);
			add_property(L"border-top-right-radius-y", tokens[0].c_str(), baseurl);
		}

	} else if(!wcscmp(name, L"border-top-left-radius"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ");
		if(tokens.size() >= 2)
		{
			add_property(L"border-top-left-radius-x", tokens[0].c_str(), baseurl);
			add_property(L"border-top-left-radius-y", tokens[1].c_str(), baseurl);
		} else if(tokens.size() == 1)
		{
			add_property(L"border-top-left-radius-x", tokens[0].c_str(), baseurl);
			add_property(L"border-top-left-radius-y", tokens[0].c_str(), baseurl);
		}

	} else if(!wcscmp(name, L"list-style"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ", L"", L"()");
		for(string_vector::iterator tok = tokens.begin(); tok != tokens.end(); tok++)
		{
			int idx = value_index(tok->c_str(), list_style_type_strings, -1);
			if(idx >= 0)
			{
				m_properties[L"list-style-type"] = val;
			} else
			{
				idx = value_index(tok->c_str(), list_style_position_strings, -1);
				if(idx >= 0)
				{
					m_properties[L"list-style-position"] = val;
				} else if(!wcsncmp(val, L"url", 3))
				{
					m_properties[L"list-style-image"] = val;
				}
			}
		}
	} else if(!wcscmp(name, L"background"))
	{
		parse_short_background(val, baseurl);

	} else if(!wcscmp(name, L"margin") || !wcscmp(name, L"padding"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ");
		if(tokens.size() >= 4)
		{
			m_properties[std::wstring(name) + L"-top"]		= tokens[0];
			m_properties[std::wstring(name) + L"-right"]	= tokens[1];
			m_properties[std::wstring(name) + L"-bottom"]	= tokens[2];
			m_properties[std::wstring(name) + L"-left"]		= tokens[3];
		} else if(tokens.size() == 3)
		{
			m_properties[std::wstring(name) + L"-top"]		= tokens[0];
			m_properties[std::wstring(name) + L"-right"]	= tokens[1];
			m_properties[std::wstring(name) + L"-left"]		= tokens[1];
			m_properties[std::wstring(name) + L"-bottom"]	= tokens[2];
		} else if(tokens.size() == 2)
		{
			m_properties[std::wstring(name) + L"-top"]		= tokens[0];
			m_properties[std::wstring(name) + L"-bottom"]	= tokens[0];
			m_properties[std::wstring(name) + L"-right"]	= tokens[1];
			m_properties[std::wstring(name) + L"-left"]		= tokens[1];
		} else if(tokens.size() == 1)
		{
			m_properties[std::wstring(name) + L"-top"]		= tokens[0];
			m_properties[std::wstring(name) + L"-bottom"]	= tokens[0];
			m_properties[std::wstring(name) + L"-right"]	= tokens[0];
			m_properties[std::wstring(name) + L"-left"]		= tokens[0];
		}
	} else if(	!wcscmp(name, L"border-left") || !wcscmp(name, L"border-right") ||
				!wcscmp(name, L"border-top")  || !wcscmp(name, L"border-bottom"))
	{
		parse_short_border(name, val);
	} else if(!wcscmp(name, L"border"))
	{
		parse_short_border(L"border-left", val);
		parse_short_border(L"border-top", val);
		parse_short_border(L"border-right", val);
		parse_short_border(L"border-bottom", val);
	} else if(	!wcscmp(name, L"border-width") ||
				!wcscmp(name, L"border-style")  ||
				!wcscmp(name, L"border-color") )
	{
		string_vector nametokens;
		tokenize(name, nametokens, L"-");

		string_vector tokens;
		tokenize(val, tokens, L" ");
		if(tokens.size() >= 4)
		{
			m_properties[nametokens[0] + L"-top-"		+ nametokens[1]]	= tokens[0];
			m_properties[nametokens[0] + L"-right-"		+ nametokens[1]]	= tokens[1];
			m_properties[nametokens[0] + L"-bottom-"	+ nametokens[1]]	= tokens[2];
			m_properties[nametokens[0] + L"-left-"		+ nametokens[1]]	= tokens[3];
		} else if(tokens.size() == 3)
		{
			m_properties[nametokens[0] + L"-top-"		+ nametokens[1]]	= tokens[0];
			m_properties[nametokens[0] + L"-right-"		+ nametokens[1]]	= tokens[1];
			m_properties[nametokens[0] + L"-left-"		+ nametokens[1]]	= tokens[1];
			m_properties[nametokens[0] + L"-bottom-"	+ nametokens[1]]	= tokens[2];
		} else if(tokens.size() == 2)
		{
			m_properties[nametokens[0] + L"-top-"		+ nametokens[1]]	= tokens[0];
			m_properties[nametokens[0] + L"-bottom-"	+ nametokens[1]]	= tokens[0];
			m_properties[nametokens[0] + L"-right-"		+ nametokens[1]]	= tokens[1];
			m_properties[nametokens[0] + L"-left-"		+ nametokens[1]]	= tokens[1];
		} else if(tokens.size() == 1)
		{
			m_properties[nametokens[0] + L"-top-"		+ nametokens[1]]	= tokens[0];
			m_properties[nametokens[0] + L"-bottom-"	+ nametokens[1]]	= tokens[0];
			m_properties[nametokens[0] + L"-right-"		+ nametokens[1]]	= tokens[0];
			m_properties[nametokens[0] + L"-left-"		+ nametokens[1]]	= tokens[0];
		}
	} else if(!wcscmp(name, L"font"))
	{
		parse_short_font(val);
	} else 
	{
		m_properties[name] = val;
	}
}

void litehtml::style::parse_short_border( const std::wstring& prefix, const std::wstring& val )
{
	string_vector tokens;
	tokenize(val, tokens, L" ", L"", L"()");
	if(tokens.size() >= 3)
	{
		m_properties[prefix + L"-width"]	= tokens[0];
		m_properties[prefix + L"-style"]	= tokens[1];
		m_properties[prefix + L"-color"]	= tokens[2];
	} else if(tokens.size() == 2)
	{
		if(iswdigit(tokens[0][0]) || value_index(val.c_str(), border_width_strings) >= 0)
		{
			m_properties[prefix + L"-width"]	= tokens[0];
			m_properties[prefix + L"-style"]	= tokens[1];
		} else
		{
			m_properties[prefix + L"-style"]	= tokens[0];
			m_properties[prefix + L"-color"]	= tokens[1];
		}
	}
}

void litehtml::style::parse_short_background( const std::wstring& val, const wchar_t* baseurl )
{
	string_vector tokens;
	tokenize(val, tokens, L" ", L"", L"()");
	bool origin_found = false;
	for(string_vector::iterator tok = tokens.begin(); tok != tokens.end(); tok++)
	{
		if(web_color::is_color(tok->c_str()))
		{
			m_properties[L"background-color"] = *tok;
		} else if(tok->substr(0, 3) == L"url")
		{
			m_properties[L"background-image"] = *tok;
			if(baseurl)
			{
				m_properties[L"background-image-baseurl"] = baseurl;
			}

		} else if( value_in_list(tok->c_str(), background_repeat_strings) )
		{
			m_properties[L"background-repeat"] = *tok;
		} else if( value_in_list(tok->c_str(), background_attachment_strings) )
		{
			m_properties[L"background-attachment"] = *tok;
		} else if( value_in_list(tok->c_str(), background_box_strings) )
		{
			if(!origin_found)
			{
				m_properties[L"background-origin"] = *tok;
				origin_found = true;
			} else
			{
				m_properties[L"background-clip"] = *tok;
			}
		} else if(	value_in_list(tok->c_str(), L"left;right;top;bottom;center") ||
					iswdigit((*tok)[0]) ||
					(*tok)[0] == L'-'	||
					(*tok)[0] == L'.'	||
					(*tok)[0] == L'+')
		{
			if(m_properties.find(L"background-position") != m_properties.end())
			{
				m_properties[L"background-position"] = m_properties[L"background-position"] + L" " + *tok;
			} else
			{
				m_properties[L"background-position"] = *tok;
			}
		}
	}
}

void litehtml::style::parse_short_font( const std::wstring& val )
{
	string_vector tokens;
	tokenize(val, tokens, L" ", L"", L"\"");

	int idx = 0;
	bool was_normal = false;
	bool is_family = false;
	for(string_vector::iterator tok = tokens.begin(); tok != tokens.end(); tok++)
	{
		idx = value_index(tok->c_str(), font_style_strings);
		if(!is_family)
		{
			if(idx >= 0)
			{
				if(idx == 0 && !was_normal)
				{
					m_properties[L"font-weight"]	= *tok;
					m_properties[L"font-variant"]	= *tok;
					m_properties[L"font-style"]		= *tok;
				} else
				{
					m_properties[L"font-style"] = *tok;
				}
			} else
			{
				if(value_in_list(tok->c_str(), font_weight_strings))
				{
					m_properties[L"font-weight"] = *tok;
				} else
				{
					if(value_in_list(tok->c_str(), font_variant_strings))
					{
						m_properties[L"font-variant"] = *tok;
					} else if( iswdigit((*tok)[0]) )
					{
						string_vector szlh;
						tokenize(*tok, szlh, L"/");

						if(szlh.size() == 1)
						{
							m_properties[L"font-size"]		= szlh[0];
						} else	if(szlh.size() >= 2)
						{
							m_properties[L"font-size"]		= szlh[0];
							m_properties[L"line-height"]	= szlh[1];
						}
					} else
					{
						is_family = true;
						m_properties[L"font-family"] += *tok;
					}
				}
			}
		} else
		{
			m_properties[L"font-family"] += *tok;
		}
	}
}

