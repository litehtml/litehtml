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

		lcase(name);

		if(!name.empty() && !val.empty())
		{
			string_vector vals;
			tokenize(val, vals, L"!");
			if(vals.size() == 1)
			{
				add_property(name.c_str(), val.c_str(), baseurl, false);
			} else if(vals.size() > 1)
			{
				trim(vals[0]);
				lcase(vals[1]);
				if(vals[1] == L"important")
				{
					add_property(name.c_str(), vals[0].c_str(), baseurl, true);
				} else
				{
					add_property(name.c_str(), vals[0].c_str(), baseurl, false);
				}
			}
		}
	}
}

void litehtml::style::combine( const litehtml::style& src )
{
	for(props_map::const_iterator i = src.m_properties.begin(); i != src.m_properties.end(); i++)
	{
		add_parsed_property(i->first.c_str(), i->second.m_value.c_str(), i->second.m_important);
	}
}

void litehtml::style::add_property( const wchar_t* name, const wchar_t* val, const wchar_t* baseurl, bool important )
{
	if(!name || !val)
	{
		return;
	}

	// Add baseurl for background image 
	if(	!wcscmp(name, L"background-image"))
	{
		add_parsed_property(name, val, important);
		if(baseurl)
		{
			add_parsed_property(L"background-image-baseurl", baseurl, important);
		}
	} else

	// Parse border spacing properties 
	if(	!wcscmp(name, L"border-spacing"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ");
		if(tokens.size() == 1)
		{
			add_property(L"-litehtml-border-spacing-x", tokens[0].c_str(), baseurl, important);
			add_property(L"-litehtml-border-spacing-y", tokens[0].c_str(), baseurl, important);
		} else if(tokens.size() == 2)
		{
			add_property(L"-litehtml-border-spacing-x", tokens[0].c_str(), baseurl, important);
			add_property(L"-litehtml-border-spacing-y", tokens[1].c_str(), baseurl, important);
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
				add_property(L"border-left-style", tok->c_str(), baseurl, important);
				add_property(L"border-right-style", tok->c_str(), baseurl, important);
				add_property(L"border-top-style", tok->c_str(), baseurl, important);
				add_property(L"border-bottom-style", tok->c_str(), baseurl, important);
			} else
			{
				if(web_color::is_color(tok->c_str()))
				{
					add_property(L"border-left-color", tok->c_str(), baseurl, important);
					add_property(L"border-right-color", tok->c_str(), baseurl, important);
					add_property(L"border-top-color", tok->c_str(), baseurl, important);
					add_property(L"border-bottom-color", tok->c_str(), baseurl, important);
				} else
				{
					add_property(L"border-left-width", tok->c_str(), baseurl, important);
					add_property(L"border-right-width", tok->c_str(), baseurl, important);
					add_property(L"border-top-width", tok->c_str(), baseurl, important);
					add_property(L"border-bottom-width", tok->c_str(), baseurl, important);
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
				add_property(str.c_str(), tok->c_str(), baseurl, important);
			} else
			{
				if(web_color::is_color(tok->c_str()))
				{
					str = name;
					str += L"-color";
					add_property(str.c_str(), tok->c_str(), baseurl, important);
				} else
				{
					str = name;
					str += L"-width";
					add_property(str.c_str(), tok->c_str(), baseurl, important);
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
			add_property(L"border-bottom-left-radius-x", tokens[0].c_str(), baseurl, important);
			add_property(L"border-bottom-left-radius-y", tokens[1].c_str(), baseurl, important);
		} else if(tokens.size() == 1)
		{
			add_property(L"border-bottom-left-radius-x", tokens[0].c_str(), baseurl, important);
			add_property(L"border-bottom-left-radius-y", tokens[0].c_str(), baseurl, important);
		}

	} else if(!wcscmp(name, L"border-bottom-right-radius"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ");
		if(tokens.size() >= 2)
		{
			add_property(L"border-bottom-right-radius-x", tokens[0].c_str(), baseurl, important);
			add_property(L"border-bottom-right-radius-y", tokens[1].c_str(), baseurl, important);
		} else if(tokens.size() == 1)
		{
			add_property(L"border-bottom-right-radius-x", tokens[0].c_str(), baseurl, important);
			add_property(L"border-bottom-right-radius-y", tokens[0].c_str(), baseurl, important);
		}

	} else if(!wcscmp(name, L"border-top-right-radius"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ");
		if(tokens.size() >= 2)
		{
			add_property(L"border-top-right-radius-x", tokens[0].c_str(), baseurl, important);
			add_property(L"border-top-right-radius-y", tokens[1].c_str(), baseurl, important);
		} else if(tokens.size() == 1)
		{
			add_property(L"border-top-right-radius-x", tokens[0].c_str(), baseurl, important);
			add_property(L"border-top-right-radius-y", tokens[0].c_str(), baseurl, important);
		}

	} else if(!wcscmp(name, L"border-top-left-radius"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ");
		if(tokens.size() >= 2)
		{
			add_property(L"border-top-left-radius-x", tokens[0].c_str(), baseurl, important);
			add_property(L"border-top-left-radius-y", tokens[1].c_str(), baseurl, important);
		} else if(tokens.size() == 1)
		{
			add_property(L"border-top-left-radius-x", tokens[0].c_str(), baseurl, important);
			add_property(L"border-top-left-radius-y", tokens[0].c_str(), baseurl, important);
		}

	} else 

	// Parse border-radius shorthand properties 
	if(!wcscmp(name, L"border-radius"))
	{
		string_vector tokens;
		tokenize(val, tokens, L"/");
		if(tokens.size() == 1)
		{
			add_property(L"border-radius-x", tokens[0].c_str(), baseurl, important);
			add_property(L"border-radius-y", tokens[0].c_str(), baseurl, important);
		} else if(tokens.size() >= 2)
		{
			add_property(L"border-radius-x", tokens[0].c_str(), baseurl, important);
			add_property(L"border-radius-y", tokens[1].c_str(), baseurl, important);
		}
	} else if(!wcscmp(name, L"border-radius-x"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ");
		if(tokens.size() == 1)
		{
			add_property(L"border-top-left-radius-x",		tokens[0].c_str(), baseurl, important);
			add_property(L"border-top-right-radius-x",		tokens[0].c_str(), baseurl, important);
			add_property(L"border-bottom-right-radius-x",	tokens[0].c_str(), baseurl, important);
			add_property(L"border-bottom-left-radius-x",	tokens[0].c_str(), baseurl, important);
		} else if(tokens.size() == 2)
		{
			add_property(L"border-top-left-radius-x",		tokens[0].c_str(), baseurl, important);
			add_property(L"border-top-right-radius-x",		tokens[1].c_str(), baseurl, important);
			add_property(L"border-bottom-right-radius-x",	tokens[0].c_str(), baseurl, important);
			add_property(L"border-bottom-left-radius-x",	tokens[1].c_str(), baseurl, important);
		} else if(tokens.size() == 3)
		{
			add_property(L"border-top-left-radius-x",		tokens[0].c_str(), baseurl, important);
			add_property(L"border-top-right-radius-x",		tokens[1].c_str(), baseurl, important);
			add_property(L"border-bottom-right-radius-x",	tokens[2].c_str(), baseurl, important);
			add_property(L"border-bottom-left-radius-x",	tokens[1].c_str(), baseurl, important);
		} else if(tokens.size() == 4)
		{
			add_property(L"border-top-left-radius-x",		tokens[0].c_str(), baseurl, important);
			add_property(L"border-top-right-radius-x",		tokens[1].c_str(), baseurl, important);
			add_property(L"border-bottom-right-radius-x",	tokens[2].c_str(), baseurl, important);
			add_property(L"border-bottom-left-radius-x",	tokens[3].c_str(), baseurl, important);
		}
	} else if(!wcscmp(name, L"border-radius-y"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ");
		if(tokens.size() == 1)
		{
			add_property(L"border-top-left-radius-y",		tokens[0].c_str(), baseurl, important);
			add_property(L"border-top-right-radius-y",		tokens[0].c_str(), baseurl, important);
			add_property(L"border-bottom-right-radius-y",	tokens[0].c_str(), baseurl, important);
			add_property(L"border-bottom-left-radius-y",	tokens[0].c_str(), baseurl, important);
		} else if(tokens.size() == 2)
		{
			add_property(L"border-top-left-radius-y",		tokens[0].c_str(), baseurl, important);
			add_property(L"border-top-right-radius-y",		tokens[1].c_str(), baseurl, important);
			add_property(L"border-bottom-right-radius-y",	tokens[0].c_str(), baseurl, important);
			add_property(L"border-bottom-left-radius-y",	tokens[1].c_str(), baseurl, important);
		} else if(tokens.size() == 3)
		{
			add_property(L"border-top-left-radius-y",		tokens[0].c_str(), baseurl, important);
			add_property(L"border-top-right-radius-y",		tokens[1].c_str(), baseurl, important);
			add_property(L"border-bottom-right-radius-y",	tokens[2].c_str(), baseurl, important);
			add_property(L"border-bottom-left-radius-y",	tokens[1].c_str(), baseurl, important);
		} else if(tokens.size() == 4)
		{
			add_property(L"border-top-left-radius-y",		tokens[0].c_str(), baseurl, important);
			add_property(L"border-top-right-radius-y",		tokens[1].c_str(), baseurl, important);
			add_property(L"border-bottom-right-radius-y",	tokens[2].c_str(), baseurl, important);
			add_property(L"border-bottom-left-radius-y",	tokens[3].c_str(), baseurl, important);
		}
	}
	

	// Parse list-style shorthand properties 
	if(!wcscmp(name, L"list-style"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ", L"", L"()");
		for(string_vector::iterator tok = tokens.begin(); tok != tokens.end(); tok++)
		{
			int idx = value_index(tok->c_str(), list_style_type_strings, -1);
			if(idx >= 0)
			{
				add_parsed_property(L"list-style-type", val, important);
			} else
			{
				idx = value_index(tok->c_str(), list_style_position_strings, -1);
				if(idx >= 0)
				{
					add_parsed_property(L"list-style-position", val, important);
				} else if(!wcsncmp(val, L"url", 3))
				{
					add_parsed_property(L"list-style-image", val, important);
				}
			}
		}
	} else 
		
	// Parse background shorthand properties 
	if(!wcscmp(name, L"background"))
	{
		parse_short_background(val, baseurl, important);

	} else 
		
	// Parse margin and padding shorthand properties 
	if(!wcscmp(name, L"margin") || !wcscmp(name, L"padding"))
	{
		string_vector tokens;
		tokenize(val, tokens, L" ");
		if(tokens.size() >= 4)
		{
			add_parsed_property(std::wstring(name) + L"-top",		tokens[0], important);
			add_parsed_property(std::wstring(name) + L"-right",		tokens[1], important);
			add_parsed_property(std::wstring(name) + L"-bottom",	tokens[2], important);
			add_parsed_property(std::wstring(name) + L"-left",		tokens[3], important);
		} else if(tokens.size() == 3)
		{
			add_parsed_property(std::wstring(name) + L"-top",		tokens[0], important);
			add_parsed_property(std::wstring(name) + L"-right",		tokens[1], important);
			add_parsed_property(std::wstring(name) + L"-left",		tokens[1], important);
			add_parsed_property(std::wstring(name) + L"-bottom",	tokens[2], important);
		} else if(tokens.size() == 2)
		{
			add_parsed_property(std::wstring(name) + L"-top",		tokens[0], important);
			add_parsed_property(std::wstring(name) + L"-bottom",	tokens[0], important);
			add_parsed_property(std::wstring(name) + L"-right",		tokens[1], important);
			add_parsed_property(std::wstring(name) + L"-left",		tokens[1], important);
		} else if(tokens.size() == 1)
		{
			add_parsed_property(std::wstring(name) + L"-top",		tokens[0], important);
			add_parsed_property(std::wstring(name) + L"-bottom",	tokens[0], important);
			add_parsed_property(std::wstring(name) + L"-right",		tokens[0], important);
			add_parsed_property(std::wstring(name) + L"-left",		tokens[0], important);
		}
	} else 
		
		
	// Parse border-* shorthand properties 
	if(	!wcscmp(name, L"border-left") || 
		!wcscmp(name, L"border-right") ||
		!wcscmp(name, L"border-top")  || 
		!wcscmp(name, L"border-bottom"))
	{
		parse_short_border(name, val, important);
	} else 
		
	// Parse border-width/style/color shorthand properties 
	if(	!wcscmp(name, L"border-width") ||
		!wcscmp(name, L"border-style")  ||
		!wcscmp(name, L"border-color") )
	{
		string_vector nametokens;
		tokenize(name, nametokens, L"-");

		string_vector tokens;
		tokenize(val, tokens, L" ");
		if(tokens.size() >= 4)
		{
			add_parsed_property(nametokens[0] + L"-top-"	+ nametokens[1],	tokens[0], important);
			add_parsed_property(nametokens[0] + L"-right-"	+ nametokens[1],	tokens[1], important);
			add_parsed_property(nametokens[0] + L"-bottom-"	+ nametokens[1],	tokens[2], important);
			add_parsed_property(nametokens[0] + L"-left-"	+ nametokens[1],	tokens[3], important);
		} else if(tokens.size() == 3)
		{
			add_parsed_property(nametokens[0] + L"-top-"	+ nametokens[1],	tokens[0], important);
			add_parsed_property(nametokens[0] + L"-right-"	+ nametokens[1],	tokens[1], important);
			add_parsed_property(nametokens[0] + L"-left-"	+ nametokens[1],	tokens[1], important);
			add_parsed_property(nametokens[0] + L"-bottom-"	+ nametokens[1],	tokens[2], important);
		} else if(tokens.size() == 2)
		{
			add_parsed_property(nametokens[0] + L"-top-"	+ nametokens[1],	tokens[0], important);
			add_parsed_property(nametokens[0] + L"-bottom-"	+ nametokens[1],	tokens[0], important);
			add_parsed_property(nametokens[0] + L"-right-"	+ nametokens[1],	tokens[1], important);
			add_parsed_property(nametokens[0] + L"-left-"	+ nametokens[1],	tokens[1], important);
		} else if(tokens.size() == 1)
		{
			add_parsed_property(nametokens[0] + L"-top-"	+ nametokens[1],	tokens[0], important);
			add_parsed_property(nametokens[0] + L"-bottom-"	+ nametokens[1],	tokens[0], important);
			add_parsed_property(nametokens[0] + L"-right-"	+ nametokens[1],	tokens[0], important);
			add_parsed_property(nametokens[0] + L"-left-"	+ nametokens[1],	tokens[0], important);
		}
	} else 
		
	// Parse font shorthand properties 
	if(!wcscmp(name, L"font"))
	{
		parse_short_font(val, important);
	} else 
	{
		add_parsed_property(name, val, important);
	}
}

void litehtml::style::parse_short_border( const std::wstring& prefix, const std::wstring& val, bool important )
{
	string_vector tokens;
	tokenize(val, tokens, L" ", L"", L"()");
	if(tokens.size() >= 3)
	{
		add_parsed_property(prefix + L"-width",	tokens[0], important);
		add_parsed_property(prefix + L"-style",	tokens[1], important);
		add_parsed_property(prefix + L"-color",	tokens[2], important);
	} else if(tokens.size() == 2)
	{
		if(iswdigit(tokens[0][0]) || value_index(val.c_str(), border_width_strings) >= 0)
		{
			add_parsed_property(prefix + L"-width",	tokens[0], important);
			add_parsed_property(prefix + L"-style",	tokens[1], important);
		} else
		{
			add_parsed_property(prefix + L"-style",	tokens[0], important);
			add_parsed_property(prefix + L"-color",	tokens[1], important);
		}
	}
}

void litehtml::style::parse_short_background( const std::wstring& val, const wchar_t* baseurl, bool important )
{
	string_vector tokens;
	tokenize(val, tokens, L" ", L"", L"()");
	bool origin_found = false;
	for(string_vector::iterator tok = tokens.begin(); tok != tokens.end(); tok++)
	{
		if(web_color::is_color(tok->c_str()))
		{
			add_parsed_property(L"background-color", *tok, important);
		} else if(tok->substr(0, 3) == L"url")
		{
			add_parsed_property(L"background-image", *tok, important);
			if(baseurl)
			{
				add_parsed_property(L"background-image-baseurl", baseurl, important);
			}

		} else if( value_in_list(tok->c_str(), background_repeat_strings) )
		{
			add_parsed_property(L"background-repeat", *tok, important);
		} else if( value_in_list(tok->c_str(), background_attachment_strings) )
		{
			add_parsed_property(L"background-attachment", *tok, important);
		} else if( value_in_list(tok->c_str(), background_box_strings) )
		{
			if(!origin_found)
			{
				add_parsed_property(L"background-origin", *tok, important);
				origin_found = true;
			} else
			{
				add_parsed_property(L"background-clip",*tok, important);
			}
		} else if(	value_in_list(tok->c_str(), L"left;right;top;bottom;center") ||
					iswdigit((*tok)[0]) ||
					(*tok)[0] == L'-'	||
					(*tok)[0] == L'.'	||
					(*tok)[0] == L'+')
		{
			if(m_properties.find(L"background-position") != m_properties.end())
			{
				m_properties[L"background-position"].m_value = m_properties[L"background-position"].m_value + L" " + *tok;
			} else
			{
				add_parsed_property(L"background-position", *tok, important);
			}
		}
	}
}

void litehtml::style::parse_short_font( const std::wstring& val, bool important )
{
	string_vector tokens;
	tokenize(val, tokens, L" ", L"", L"\"");

	int idx = 0;
	bool was_normal = false;
	bool is_family = false;
	std::wstring font_family;
	for(string_vector::iterator tok = tokens.begin(); tok != tokens.end(); tok++)
	{
		idx = value_index(tok->c_str(), font_style_strings);
		if(!is_family)
		{
			if(idx >= 0)
			{
				if(idx == 0 && !was_normal)
				{
					add_parsed_property(L"font-weight",		*tok, important);
					add_parsed_property(L"font-variant",	*tok, important);
					add_parsed_property(L"font-style",		*tok, important);
				} else
				{
					add_parsed_property(L"font-style",		*tok, important);
				}
			} else
			{
				if(value_in_list(tok->c_str(), font_weight_strings))
				{
					add_parsed_property(L"font-weight",		*tok, important);
				} else
				{
					if(value_in_list(tok->c_str(), font_variant_strings))
					{
						add_parsed_property(L"font-variant",	*tok, important);
					} else if( iswdigit((*tok)[0]) )
					{
						string_vector szlh;
						tokenize(*tok, szlh, L"/");

						if(szlh.size() == 1)
						{
							add_parsed_property(L"font-size",	szlh[0], important);
						} else	if(szlh.size() >= 2)
						{
							add_parsed_property(L"font-size",	szlh[0], important);
							add_parsed_property(L"line-height",	szlh[1], important);
						}
					} else
					{
						is_family = true;
						font_family += *tok;
					}
				}
			}
		} else
		{
			font_family += *tok;
		}
	}
	add_parsed_property(L"font-family", font_family, important);
}

void litehtml::style::add_parsed_property( const std::wstring& name, const std::wstring& val, bool important )
{
	props_map::iterator prop = m_properties.find(name);
	if(prop != m_properties.end())
	{
		if(!prop->second.m_important || important && prop->second.m_important)
		{
			prop->second.m_value		= val;
			prop->second.m_important	= important;
		}
	} else
	{
		m_properties[name] = property_value(val.c_str(), important);
	}
}

