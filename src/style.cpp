#include "html.h"
#include "style.h"
#ifndef WINCE
#include <locale>
#endif

litehtml::string_map litehtml::style::m_valid_values =
{
	{ "white-space", white_space_strings }
};

litehtml::style::style( const style& val )
{
	m_properties = val.m_properties;
}

void litehtml::style::parse( const char* txt, const char* baseurl, const element* el )
{
	std::vector<string> properties;
	split_string(txt, properties, ";", "", "\"'");

	for(const auto & property : properties)
	{
		parse_property(property, baseurl, el);
	}
}

void litehtml::style::parse_property( const string& txt, const char* baseurl, const element* el )
{
	string::size_type pos = txt.find_first_of(':');
	if(pos != string::npos)
	{
		string name = txt.substr(0, pos);
		string val	= txt.substr(pos + 1);

		trim(name); lcase(name);
		trim(val);

		if(!name.empty() && !val.empty())
		{
			string_vector vals;
			split_string(val, vals, "!");
			if(vals.size() == 1)
			{
				add_property(name.c_str(), val.c_str(), baseurl, false, el);
			} else if(vals.size() > 1)
			{
				trim(vals[0]);
				lcase(vals[1]);
				add_property(name.c_str(), vals[0].c_str(), baseurl, vals[1] == "important", el);
			}
		}
	}
}

void litehtml::style::combine( const litehtml::style& src )
{
	for(const auto& property : src.m_properties)
	{
		add_parsed_property(property.first, property.second.m_value, property.second.m_important);
	}
}

void litehtml::style::subst_vars( string& str, const element* el )
{
	if (!el) return;

	while (1)
	{
		auto start = str.find("var(");
		if (start == -1) break;
		if (start > 0 && isalnum(str[start - 1])) break;
		auto end = str.find(")", start + 4);
		if (end == -1) break;
		auto name = str.substr(start + 4, end - start - 4);
		trim(name);
		auto val = el->get_style_property(name.c_str(), true);
		if (!val) break;
		str.replace(start, end - start + 1, val);
	}
}

void litehtml::style::add_property( const char* name, const char* _val, const char* baseurl, bool important, const element* el )
{
	if(!name || !_val)
	{
		return;
	}

	string val = _val;
	subst_vars(val, el);

	// Add baseurl for background image 
	if(	!strcmp(name, "background-image"))
	{
		add_parsed_property(name, val, important);
		if(baseurl)
		{
			add_parsed_property("background-image-baseurl", baseurl, important);
		}
	} else

	// Parse border spacing properties 
	if(	!strcmp(name, "border-spacing"))
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if(tokens.size() == 1)
		{
			add_parsed_property("-litehtml-border-spacing-x", tokens[0], important);
			add_parsed_property("-litehtml-border-spacing-y", tokens[0], important);
		} else if(tokens.size() == 2)
		{
			add_parsed_property("-litehtml-border-spacing-x", tokens[0], important);
			add_parsed_property("-litehtml-border-spacing-y", tokens[1], important);
		}
	} else

	// Parse borders shorthand properties 

	if(	!strcmp(name, "border"))
	{
		string_vector tokens;
		split_string(val, tokens, " ", "", "(");
		int idx;
		string str;
		for(const auto& token : tokens)
		{
			idx = value_index(token, border_style_strings, -1);
			if(idx >= 0)
			{
				add_property("border-left-style", token.c_str(), baseurl, important, el);
				add_property("border-right-style", token.c_str(), baseurl, important, el);
				add_property("border-top-style", token.c_str(), baseurl, important, el);
				add_property("border-bottom-style", token.c_str(), baseurl, important, el);
			} else
			{
				if (t_isdigit(token[0]) || token[0] == '.' ||
					value_in_list(token, "thin;medium;thick"))
				{
					add_property("border-left-width", token.c_str(), baseurl, important, el);
					add_property("border-right-width", token.c_str(), baseurl, important, el);
					add_property("border-top-width", token.c_str(), baseurl, important, el);
					add_property("border-bottom-width", token.c_str(), baseurl, important, el);
				} 
				else
				{
					add_property("border-left-color", token.c_str(), baseurl, important, el);
					add_property("border-right-color", token.c_str(), baseurl, important, el);
					add_property("border-top-color", token.c_str(), baseurl, important, el);
					add_property("border-bottom-color", token.c_str(), baseurl, important, el);
				}
			}
		}
	} else if(	!strcmp(name, "border-left")	||
		!strcmp(name, "border-right")	||
		!strcmp(name, "border-top")	||
		!strcmp(name, "border-bottom") )
	{
		string_vector tokens;
		split_string(val, tokens, " ", "", "(");
		int idx;
		string str;
		for(const auto& token : tokens)
		{
			idx = value_index(token, border_style_strings, -1);
			if(idx >= 0)
			{
				str = name;
				str += "-style";
				add_property(str.c_str(), token.c_str(), baseurl, important, el);
			} else
			{
				if(web_color::is_color(token.c_str()))
				{
					str = name;
					str += "-color";
					add_property(str.c_str(), token.c_str(), baseurl, important, el);
				} else
				{
					str = name;
					str += "-width";
					add_property(str.c_str(), token.c_str(), baseurl, important, el);
				}
			}
		}
	} else 

	// Parse border radius shorthand properties 
	if(!strcmp(name, "border-bottom-left-radius"))
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if(tokens.size() >= 2)
		{
			add_property("border-bottom-left-radius-x", tokens[0].c_str(), baseurl, important, el);
			add_property("border-bottom-left-radius-y", tokens[1].c_str(), baseurl, important, el);
		} else if(tokens.size() == 1)
		{
			add_property("border-bottom-left-radius-x", tokens[0].c_str(), baseurl, important, el);
			add_property("border-bottom-left-radius-y", tokens[0].c_str(), baseurl, important, el);
		}

	} else if(!strcmp(name, "border-bottom-right-radius"))
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if(tokens.size() >= 2)
		{
			add_property("border-bottom-right-radius-x", tokens[0].c_str(), baseurl, important, el);
			add_property("border-bottom-right-radius-y", tokens[1].c_str(), baseurl, important, el);
		} else if(tokens.size() == 1)
		{
			add_property("border-bottom-right-radius-x", tokens[0].c_str(), baseurl, important, el);
			add_property("border-bottom-right-radius-y", tokens[0].c_str(), baseurl, important, el);
		}

	} else if(!strcmp(name, "border-top-right-radius"))
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if(tokens.size() >= 2)
		{
			add_property("border-top-right-radius-x", tokens[0].c_str(), baseurl, important, el);
			add_property("border-top-right-radius-y", tokens[1].c_str(), baseurl, important, el);
		} else if(tokens.size() == 1)
		{
			add_property("border-top-right-radius-x", tokens[0].c_str(), baseurl, important, el);
			add_property("border-top-right-radius-y", tokens[0].c_str(), baseurl, important, el);
		}

	} else if(!strcmp(name, "border-top-left-radius"))
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if(tokens.size() >= 2)
		{
			add_property("border-top-left-radius-x", tokens[0].c_str(), baseurl, important, el);
			add_property("border-top-left-radius-y", tokens[1].c_str(), baseurl, important, el);
		} else if(tokens.size() == 1)
		{
			add_property("border-top-left-radius-x", tokens[0].c_str(), baseurl, important, el);
			add_property("border-top-left-radius-y", tokens[0].c_str(), baseurl, important, el);
		}

	} else 

	// Parse border-radius shorthand properties 
	if(!strcmp(name, "border-radius"))
	{
		string_vector tokens;
		split_string(val, tokens, "/");
		if(tokens.size() == 1)
		{
			add_property("border-radius-x", tokens[0].c_str(), baseurl, important, el);
			add_property("border-radius-y", tokens[0].c_str(), baseurl, important, el);
		} else if(tokens.size() >= 2)
		{
			add_property("border-radius-x", tokens[0].c_str(), baseurl, important, el);
			add_property("border-radius-y", tokens[1].c_str(), baseurl, important, el);
		}
	} else if(!strcmp(name, "border-radius-x"))
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if(tokens.size() == 1)
		{
			add_property("border-top-left-radius-x",		tokens[0].c_str(), baseurl, important, el);
			add_property("border-top-right-radius-x",		tokens[0].c_str(), baseurl, important, el);
			add_property("border-bottom-right-radius-x",	tokens[0].c_str(), baseurl, important, el);
			add_property("border-bottom-left-radius-x",	tokens[0].c_str(), baseurl, important, el);
		} else if(tokens.size() == 2)
		{
			add_property("border-top-left-radius-x",		tokens[0].c_str(), baseurl, important, el);
			add_property("border-top-right-radius-x",		tokens[1].c_str(), baseurl, important, el);
			add_property("border-bottom-right-radius-x",	tokens[0].c_str(), baseurl, important, el);
			add_property("border-bottom-left-radius-x",	tokens[1].c_str(), baseurl, important, el);
		} else if(tokens.size() == 3)
		{
			add_property("border-top-left-radius-x",		tokens[0].c_str(), baseurl, important, el);
			add_property("border-top-right-radius-x",		tokens[1].c_str(), baseurl, important, el);
			add_property("border-bottom-right-radius-x",	tokens[2].c_str(), baseurl, important, el);
			add_property("border-bottom-left-radius-x",	tokens[1].c_str(), baseurl, important, el);
		} else if(tokens.size() == 4)
		{
			add_property("border-top-left-radius-x",		tokens[0].c_str(), baseurl, important, el);
			add_property("border-top-right-radius-x",		tokens[1].c_str(), baseurl, important, el);
			add_property("border-bottom-right-radius-x",	tokens[2].c_str(), baseurl, important, el);
			add_property("border-bottom-left-radius-x",	tokens[3].c_str(), baseurl, important, el);
		}
	} else if(!strcmp(name, "border-radius-y"))
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if(tokens.size() == 1)
		{
			add_property("border-top-left-radius-y",		tokens[0].c_str(), baseurl, important, el);
			add_property("border-top-right-radius-y",		tokens[0].c_str(), baseurl, important, el);
			add_property("border-bottom-right-radius-y",	tokens[0].c_str(), baseurl, important, el);
			add_property("border-bottom-left-radius-y",	tokens[0].c_str(), baseurl, important, el);
		} else if(tokens.size() == 2)
		{
			add_property("border-top-left-radius-y",		tokens[0].c_str(), baseurl, important, el);
			add_property("border-top-right-radius-y",		tokens[1].c_str(), baseurl, important, el);
			add_property("border-bottom-right-radius-y",	tokens[0].c_str(), baseurl, important, el);
			add_property("border-bottom-left-radius-y",	tokens[1].c_str(), baseurl, important, el);
		} else if(tokens.size() == 3)
		{
			add_property("border-top-left-radius-y",		tokens[0].c_str(), baseurl, important, el);
			add_property("border-top-right-radius-y",		tokens[1].c_str(), baseurl, important, el);
			add_property("border-bottom-right-radius-y",	tokens[2].c_str(), baseurl, important, el);
			add_property("border-bottom-left-radius-y",	tokens[1].c_str(), baseurl, important, el);
		} else if(tokens.size() == 4)
		{
			add_property("border-top-left-radius-y",		tokens[0].c_str(), baseurl, important, el);
			add_property("border-top-right-radius-y",		tokens[1].c_str(), baseurl, important, el);
			add_property("border-bottom-right-radius-y",	tokens[2].c_str(), baseurl, important, el);
			add_property("border-bottom-left-radius-y",	tokens[3].c_str(), baseurl, important, el);
		}
	}
	else

	// Parse list-style shorthand properties 
	if(!strcmp(name, "list-style"))
	{
		add_parsed_property("list-style-type",			"disc",		important);
		add_parsed_property("list-style-position",		"outside",	important);
		add_parsed_property("list-style-image",			"",			important);
		add_parsed_property("list-style-image-baseurl",	"",			important);

		string_vector tokens;
		split_string(val, tokens, " ", "", "(");
		for(const auto& token : tokens)
		{
			int idx = value_index(token, list_style_type_strings, -1);
			if(idx >= 0)
			{
				add_parsed_property("list-style-type", token, important);
			} else
			{
				idx = value_index(token, list_style_position_strings, -1);
				if(idx >= 0)
				{
					add_parsed_property("list-style-position", token, important);
				} else if(!strncmp(val.c_str(), "url", 3))
				{
					add_parsed_property("list-style-image", token, important);
					if(baseurl)
					{
						add_parsed_property("list-style-image-baseurl", baseurl, important);
					}
				}
			}
		}
	} else 

	// Add baseurl for background image 
	if(	!strcmp(name, "list-style-image"))
	{
		add_parsed_property(name, val, important);
		if(baseurl)
		{
			add_parsed_property("list-style-image-baseurl", baseurl, important);
		}
	} else
		
	// Parse background shorthand properties 
	if(!strcmp(name, "background"))
	{
		parse_short_background(val, baseurl, important);

	} else 
		
	// Parse margin and padding shorthand properties 
	if(!strcmp(name, "margin") || !strcmp(name, "padding"))
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if(tokens.size() >= 4)
		{
			add_parsed_property(string(name) + "-top",		tokens[0], important);
			add_parsed_property(string(name) + "-right",		tokens[1], important);
			add_parsed_property(string(name) + "-bottom",	tokens[2], important);
			add_parsed_property(string(name) + "-left",		tokens[3], important);
		} else if(tokens.size() == 3)
		{
			add_parsed_property(string(name) + "-top",		tokens[0], important);
			add_parsed_property(string(name) + "-right",		tokens[1], important);
			add_parsed_property(string(name) + "-left",		tokens[1], important);
			add_parsed_property(string(name) + "-bottom",	tokens[2], important);
		} else if(tokens.size() == 2)
		{
			add_parsed_property(string(name) + "-top",		tokens[0], important);
			add_parsed_property(string(name) + "-bottom",	tokens[0], important);
			add_parsed_property(string(name) + "-right",		tokens[1], important);
			add_parsed_property(string(name) + "-left",		tokens[1], important);
		} else if(tokens.size() == 1)
		{
			add_parsed_property(string(name) + "-top",		tokens[0], important);
			add_parsed_property(string(name) + "-bottom",	tokens[0], important);
			add_parsed_property(string(name) + "-right",		tokens[0], important);
			add_parsed_property(string(name) + "-left",		tokens[0], important);
		}
	} else 
		
		
	// Parse border-* shorthand properties 
	if(	!strcmp(name, "border-left") || 
		!strcmp(name, "border-right") ||
		!strcmp(name, "border-top")  || 
		!strcmp(name, "border-bottom"))
	{
		parse_short_border(name, val, important);
	} else 
		
	// Parse border-width/style/color shorthand properties 
	if(	!strcmp(name, "border-width") ||
		!strcmp(name, "border-style")  ||
		!strcmp(name, "border-color") )
	{
		string_vector nametokens;
		split_string(name, nametokens, "-");

		string_vector tokens;
		split_string(val, tokens, " ");
		if(tokens.size() >= 4)
		{
			add_parsed_property(nametokens[0] + "-top-"		+ nametokens[1],	tokens[0], important);
			add_parsed_property(nametokens[0] + "-right-"	+ nametokens[1],	tokens[1], important);
			add_parsed_property(nametokens[0] + "-bottom-"	+ nametokens[1],	tokens[2], important);
			add_parsed_property(nametokens[0] + "-left-"	+ nametokens[1],	tokens[3], important);
		} else if(tokens.size() == 3)
		{
			add_parsed_property(nametokens[0] + "-top-"		+ nametokens[1],	tokens[0], important);
			add_parsed_property(nametokens[0] + "-right-"	+ nametokens[1],	tokens[1], important);
			add_parsed_property(nametokens[0] + "-left-"	+ nametokens[1],	tokens[1], important);
			add_parsed_property(nametokens[0] + "-bottom-"	+ nametokens[1],	tokens[2], important);
		} else if(tokens.size() == 2)
		{
			add_parsed_property(nametokens[0] + "-top-"		+ nametokens[1],	tokens[0], important);
			add_parsed_property(nametokens[0] + "-bottom-"	+ nametokens[1],	tokens[0], important);
			add_parsed_property(nametokens[0] + "-right-"	+ nametokens[1],	tokens[1], important);
			add_parsed_property(nametokens[0] + "-left-"	+ nametokens[1],	tokens[1], important);
		} else if(tokens.size() == 1)
		{
			add_parsed_property(nametokens[0] + "-top-"		+ nametokens[1],	tokens[0], important);
			add_parsed_property(nametokens[0] + "-bottom-"	+ nametokens[1],	tokens[0], important);
			add_parsed_property(nametokens[0] + "-right-"	+ nametokens[1],	tokens[0], important);
			add_parsed_property(nametokens[0] + "-left-"	+ nametokens[1],	tokens[0], important);
		}
	} else 
		
	// Parse font shorthand properties 
	if(!strcmp(name, "font"))
	{
		parse_short_font(val, important);
	} else

    // Parse flex-flow shorthand properties
    if(!strcmp(name, "flex-flow"))
    {
        string_vector tokens;
        split_string(val, tokens, " ");
        for(const auto& tok : tokens)
        {
            if(value_in_list(tok, flex_direction_strings))
            {
                add_parsed_property("flex-direction", tok, important);
            } else if(value_in_list(tok, flex_wrap_strings))
            {
                add_parsed_property("flex-wrap", tok, important);
            }
        }
    } else

    // Parse flex-flow shorthand properties
    if(!strcmp(name, "flex"))
    {
        auto is_number = [](const string& val)
            {
                for(auto ch : val)
                {
                    if((ch < '0' || ch > '9') && ch != '.')
                    {
                        return false;
                    }
                }
                return true;
            };
        if(val == "initial")
        {
            // 0 1 auto
            add_parsed_property("flex-grow", "0", important);
            add_parsed_property("flex-shrink", "1", important);
            add_parsed_property("flex-basis", "auto", important);
        } else if(val == "auto")
        {
            // 1 1 auto
            add_parsed_property("flex-grow", "1", important);
            add_parsed_property("flex-shrink", "1", important);
            add_parsed_property("flex-basis", "auto", important);
        } else if(val == "none")
        {
            // 0 0 auto
            add_parsed_property("flex-grow", "0", important);
            add_parsed_property("flex-shrink", "0", important);
            add_parsed_property("flex-basis", "auto", important);
        }
        string_vector tokens;
        split_string(val, tokens, " ");
        if(tokens.size() == 3)
        {
            add_parsed_property("flex-grow", tokens[0], important);
            add_parsed_property("flex-shrink", tokens[1], important);
            add_parsed_property("flex-basis", tokens[2], important);
        } else if(tokens.size() == 2)
        {
            if(is_number(tokens[0]))
            {
                add_parsed_property("flex-grow", tokens[0], important);
            } else
            {
                if (is_number(tokens[1]))
                {
                    add_parsed_property("flex-shrink", tokens[0], important);
                } else
                {
                    add_parsed_property("flex-base", tokens[0], important);
                }
            }
        } else if(tokens.size() == 1)
        {
            if (is_number(tokens[0]))
            {
                add_parsed_property("flex-grow", tokens[0], important);
                auto v = (float) t_strtod(tokens[0].c_str(), nullptr);
                if(v >= 1)
                {
                    add_parsed_property("flex-shrink", "1", important);
                    add_parsed_property("flex-basis", "0", important);
                }
            } else
            {
                add_parsed_property("flex-base", tokens[0], important);
            }
        }
    } else
    {
        add_parsed_property(name, val, important);
	}
}

void litehtml::style::parse_short_border( const string& prefix, const string& val, bool important )
{
	string_vector tokens;
	split_string(val, tokens, " ", "", "(");
	if(tokens.size() >= 3)
	{
		add_parsed_property(prefix + "-width",	tokens[0], important);
		add_parsed_property(prefix + "-style",	tokens[1], important);
		add_parsed_property(prefix + "-color",	tokens[2], important);
	} else if(tokens.size() == 2)
	{
		if(iswdigit(tokens[0][0]) || value_index(val, border_width_strings) >= 0)
		{
			add_parsed_property(prefix + "-width",	tokens[0], important);
			add_parsed_property(prefix + "-style",	tokens[1], important);
		} else
		{
			add_parsed_property(prefix + "-style",	tokens[0], important);
			add_parsed_property(prefix + "-color",	tokens[1], important);
		}
	}
}

void litehtml::style::parse_short_background( const string& val, const char* baseurl, bool important )
{
	add_parsed_property("background-color",			"transparent",	important);
	add_parsed_property("background-image",			"",				important);
	add_parsed_property("background-image-baseurl", "",				important);
	add_parsed_property("background-repeat",		"repeat",		important);
	add_parsed_property("background-origin",		"padding-box",	important);
	add_parsed_property("background-clip",			"border-box",	important);
	add_parsed_property("background-attachment",	"scroll",		important);

	if(val == "none")
	{
		return;
	}

	string_vector tokens;
	split_string(val, tokens, " ", "", "(");
	bool origin_found = false;
	for(const auto& token : tokens)
	{
		if(token.substr(0, 3) == "url")
		{
			add_parsed_property("background-image", token, important);
			if(baseurl)
			{
				add_parsed_property("background-image-baseurl", baseurl, important);
			}

		} else if( value_in_list(token, background_repeat_strings) )
		{
			add_parsed_property("background-repeat", token, important);
		} else if( value_in_list(token, background_attachment_strings) )
		{
			add_parsed_property("background-attachment", token, important);
		} else if( value_in_list(token, background_box_strings) )
		{
			if(!origin_found)
			{
				add_parsed_property("background-origin", token, important);
				origin_found = true;
			} else
			{
				add_parsed_property("background-clip", token, important);
			}
		} else if(	value_in_list(token, "left;right;top;bottom;center") ||
					iswdigit(token[0]) ||
					token[0] == '-'	||
					token[0] == '.'	||
					token[0] == '+')
		{
			if(m_properties.find("background-position") != m_properties.end())
			{
				m_properties["background-position"].m_value = m_properties["background-position"].m_value + " " + token;
			} else
			{
				add_parsed_property("background-position", token, important);
			}
		} else if (web_color::is_color(token.c_str()))
		{
			add_parsed_property("background-color", token, important);
		}
	}
}

void litehtml::style::parse_short_font( const string& val, bool important )
{
	add_parsed_property("font-style",	"normal",	important);
	add_parsed_property("font-variant",	"normal",	important);
	add_parsed_property("font-weight",	"normal",	important);
	add_parsed_property("font-size",	"medium",	important);
	add_parsed_property("line-height",	"normal",	important);

	string_vector tokens;
	split_string(val, tokens, " ", "", "\"");

	int idx;
	bool is_family = false;
	string font_family;
	for(const auto& token : tokens)
	{
		idx = value_index(token, font_style_strings);
		if(!is_family)
		{
			if(idx >= 0)
			{
				if(idx == 0)
				{
					add_parsed_property("font-weight", token, important);
					add_parsed_property("font-variant", token, important);
					add_parsed_property("font-style", token, important);
				} else
				{
					add_parsed_property("font-style", token, important);
				}
			} else
			{
				if(value_in_list(token, font_weight_strings))
				{
					add_parsed_property("font-weight", token, important);
				} else
				{
					if(value_in_list(token, font_variant_strings))
					{
						add_parsed_property("font-variant", token, important);
					} else if( iswdigit(token[0]) )
					{
						string_vector szlh;
						split_string(token, szlh, "/");

						if(szlh.size() == 1)
						{
							add_parsed_property("font-size",	szlh[0], important);
						} else	if(szlh.size() >= 2)
						{
							add_parsed_property("font-size",	szlh[0], important);
							add_parsed_property("line-height",	szlh[1], important);
						}
					} else
					{
						is_family = true;
						font_family += token;
					}
				}
			}
		} else
		{
			font_family += token;
		}
	}
	add_parsed_property("font-family", font_family, important);
}

void litehtml::style::add_parsed_property( const string& name, const string& val, bool important )
{
	bool is_valid = true;
	auto vals = m_valid_values.find(name);
	if (vals != m_valid_values.end())
	{
		if (!value_in_list(val, vals->second))
		{
			is_valid = false;
		}
	}

	if (is_valid)
	{
		auto prop = m_properties.find(name);
		if (prop != m_properties.end())
		{
			if (!prop->second.m_important || (important && prop->second.m_important))
			{
				prop->second.m_value = val;
				prop->second.m_important = important;
			}
		}
		else
		{
			m_properties[name] = property_value(val.c_str(), important);
		}
	}
}

void litehtml::style::remove_property( const string& name, bool important )
{
	auto prop = m_properties.find(name);
	if(prop != m_properties.end())
	{
		if( !prop->second.m_important || (important && prop->second.m_important) )
		{
			m_properties.erase(prop);
		}
	}
}
