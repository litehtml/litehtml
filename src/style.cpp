#include "html.h"
#include "style.h"
#ifndef WINCE
#include <locale>
#endif

std::map<litehtml::string_id, litehtml::string> litehtml::style::m_valid_values =
{
	{ _white_space_, white_space_strings }
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
				add_property(_id(name), val.c_str(), baseurl, false, el);
			} else if(vals.size() > 1)
			{
				trim(vals[0]);
				lcase(vals[1]);
				add_property(_id(name), vals[0].c_str(), baseurl, vals[1] == "important", el);
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
		auto val = el->get_style_property(_id(name), true);
		if (!val) break;
		str.replace(start, end - start + 1, val);
	}
}

void litehtml::style::add_property( string_id name, const char* _val, const char* baseurl, bool important, const element* el )
{
	if(!_val) return;
	string val = _val;
	subst_vars(val, el);

	switch (name)
	{
	// Add baseurl for background image 
	case _background_image_:
		add_parsed_property(name, val, important);
		if (baseurl)
		{
			add_parsed_property(_background_image_baseurl_, baseurl, important);
		}
		break;

	// Parse border spacing properties 
	case _border_spacing_:
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if (tokens.size() == 1)
		{
			add_parsed_property(__litehtml_border_spacing_x_, tokens[0], important);
			add_parsed_property(__litehtml_border_spacing_y_, tokens[0], important);
		}
		else if (tokens.size() == 2)
		{
			add_parsed_property(__litehtml_border_spacing_x_, tokens[0], important);
			add_parsed_property(__litehtml_border_spacing_y_, tokens[1], important);
		}
		break;
	}

	// Parse borders shorthand properties 
	case _border_:
	{
		string_vector tokens;
		split_string(val, tokens, " ", "", "(");
		int idx;
		string str;
		for (const auto& token : tokens)
		{
			idx = value_index(token, border_style_strings, -1);
			if (idx >= 0)
			{
				add_parsed_property(_border_left_style_, token.c_str(), important);
				add_parsed_property(_border_right_style_, token.c_str(), important);
				add_parsed_property(_border_top_style_, token.c_str(), important);
				add_parsed_property(_border_bottom_style_, token.c_str(), important);
			}
			else
			{
				if (t_isdigit(token[0]) || token[0] == '.' ||
					value_in_list(token, "thin;medium;thick"))
				{
					add_parsed_property(_border_left_width_, token.c_str(), important);
					add_parsed_property(_border_right_width_, token.c_str(), important);
					add_parsed_property(_border_top_width_, token.c_str(), important);
					add_parsed_property(_border_bottom_width_, token.c_str(), important);
				}
				else
				{
					add_parsed_property(_border_left_color_, token.c_str(), important);
					add_parsed_property(_border_right_color_, token.c_str(), important);
					add_parsed_property(_border_top_color_, token.c_str(), important);
					add_parsed_property(_border_bottom_color_, token.c_str(), important);
				}
			}
		}
		break;
	}
	case _border_left_:
	case _border_right_:
	case _border_top_:
	case _border_bottom_:
	{
		string_vector tokens;
		split_string(val, tokens, " ", "", "(");
		string str;
		for (const auto& token : tokens)
		{
			int idx = value_index(token, border_style_strings, -1);
			if (idx >= 0)
			{
				str = _s(name) + "-style";
				add_parsed_property(_id(str), token.c_str(), important);
			}
			else
			{
				if (web_color::is_color(token.c_str()))
				{
					str = _s(name) + "-color";
					add_parsed_property(_id(str), token.c_str(), important);
				}
				else
				{
					str = _s(name) + "-width";
					add_parsed_property(_id(str), token.c_str(), important);
				}
			}
		}
		break;
	}

	// Parse border radius shorthand properties 
	case _border_bottom_left_radius_:
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if (tokens.size() >= 2)
		{
			add_parsed_property(_border_bottom_left_radius_x_, tokens[0].c_str(), important);
			add_parsed_property(_border_bottom_left_radius_y_, tokens[1].c_str(), important);
		}
		else if (tokens.size() == 1)
		{
			add_parsed_property(_border_bottom_left_radius_x_, tokens[0].c_str(), important);
			add_parsed_property(_border_bottom_left_radius_y_, tokens[0].c_str(), important);
		}
		break;
	}
	case _border_bottom_right_radius_:
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if (tokens.size() >= 2)
		{
			add_parsed_property(_border_bottom_right_radius_x_, tokens[0].c_str(), important);
			add_parsed_property(_border_bottom_right_radius_y_, tokens[1].c_str(), important);
		}
		else if (tokens.size() == 1)
		{
			add_parsed_property(_border_bottom_right_radius_x_, tokens[0].c_str(), important);
			add_parsed_property(_border_bottom_right_radius_y_, tokens[0].c_str(), important);
		}
		break;
	}
	case _border_top_right_radius_:
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if (tokens.size() >= 2)
		{
			add_parsed_property(_border_top_right_radius_x_, tokens[0].c_str(), important);
			add_parsed_property(_border_top_right_radius_y_, tokens[1].c_str(), important);
		}
		else if (tokens.size() == 1)
		{
			add_parsed_property(_border_top_right_radius_x_, tokens[0].c_str(), important);
			add_parsed_property(_border_top_right_radius_y_, tokens[0].c_str(), important);
		}
		break;
	}
	case _border_top_left_radius_:
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if (tokens.size() >= 2)
		{
			add_parsed_property(_border_top_left_radius_x_, tokens[0].c_str(), important);
			add_parsed_property(_border_top_left_radius_y_, tokens[1].c_str(), important);
		}
		else if (tokens.size() == 1)
		{
			add_parsed_property(_border_top_left_radius_x_, tokens[0].c_str(), important);
			add_parsed_property(_border_top_left_radius_y_, tokens[0].c_str(), important);
		}
		break;
	}

	// Parse border-radius shorthand properties 
	case _border_radius_:
	{
		string_vector tokens;
		split_string(val, tokens, "/");
		if (tokens.size() == 1)
		{
			add_property(_border_radius_x_, tokens[0].c_str(), baseurl, important, el);
			add_property(_border_radius_y_, tokens[0].c_str(), baseurl, important, el);
		}
		else if (tokens.size() >= 2)
		{
			add_property(_border_radius_x_, tokens[0].c_str(), baseurl, important, el);
			add_property(_border_radius_y_, tokens[1].c_str(), baseurl, important, el);
		}
		break;
	}
	case _border_radius_x_:
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if (tokens.size() == 1)
		{
			add_parsed_property(_border_top_left_radius_x_,		tokens[0].c_str(), important);
			add_parsed_property(_border_top_right_radius_x_,	tokens[0].c_str(), important);
			add_parsed_property(_border_bottom_right_radius_x_,	tokens[0].c_str(), important);
			add_parsed_property(_border_bottom_left_radius_x_,	tokens[0].c_str(), important);
		}
		else if (tokens.size() == 2)
		{
			add_parsed_property(_border_top_left_radius_x_,		tokens[0].c_str(), important);
			add_parsed_property(_border_top_right_radius_x_,	tokens[1].c_str(), important);
			add_parsed_property(_border_bottom_right_radius_x_,	tokens[0].c_str(), important);
			add_parsed_property(_border_bottom_left_radius_x_,	tokens[1].c_str(), important);
		}
		else if (tokens.size() == 3)
		{
			add_parsed_property(_border_top_left_radius_x_,		tokens[0].c_str(), important);
			add_parsed_property(_border_top_right_radius_x_,	tokens[1].c_str(), important);
			add_parsed_property(_border_bottom_right_radius_x_,	tokens[2].c_str(), important);
			add_parsed_property(_border_bottom_left_radius_x_,	tokens[1].c_str(), important);
		}
		else if (tokens.size() == 4)
		{
			add_parsed_property(_border_top_left_radius_x_,		tokens[0].c_str(), important);
			add_parsed_property(_border_top_right_radius_x_,	tokens[1].c_str(), important);
			add_parsed_property(_border_bottom_right_radius_x_,	tokens[2].c_str(), important);
			add_parsed_property(_border_bottom_left_radius_x_,	tokens[3].c_str(), important);
		}
		break;
	}
	case _border_radius_y_:
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if (tokens.size() == 1)
		{
			add_parsed_property(_border_top_left_radius_y_,		tokens[0].c_str(), important);
			add_parsed_property(_border_top_right_radius_y_,	tokens[0].c_str(), important);
			add_parsed_property(_border_bottom_right_radius_y_,	tokens[0].c_str(), important);
			add_parsed_property(_border_bottom_left_radius_y_,	tokens[0].c_str(), important);
		}
		else if (tokens.size() == 2)
		{
			add_parsed_property(_border_top_left_radius_y_,		tokens[0].c_str(), important);
			add_parsed_property(_border_top_right_radius_y_,	tokens[1].c_str(), important);
			add_parsed_property(_border_bottom_right_radius_y_,	tokens[0].c_str(), important);
			add_parsed_property(_border_bottom_left_radius_y_,	tokens[1].c_str(), important);
		}
		else if (tokens.size() == 3)
		{
			add_parsed_property(_border_top_left_radius_y_,		tokens[0].c_str(), important);
			add_parsed_property(_border_top_right_radius_y_,	tokens[1].c_str(), important);
			add_parsed_property(_border_bottom_right_radius_y_,	tokens[2].c_str(), important);
			add_parsed_property(_border_bottom_left_radius_y_,	tokens[1].c_str(), important);
		}
		else if (tokens.size() == 4)
		{
			add_parsed_property(_border_top_left_radius_y_,		tokens[0].c_str(), important);
			add_parsed_property(_border_top_right_radius_y_,	tokens[1].c_str(), important);
			add_parsed_property(_border_bottom_right_radius_y_,	tokens[2].c_str(), important);
			add_parsed_property(_border_bottom_left_radius_y_,	tokens[3].c_str(), important);
		}
		break;
	}

	// Parse list-style shorthand properties 
	case _list_style_:
	{
		add_parsed_property(_list_style_type_,			"disc",		important);
		add_parsed_property(_list_style_position_,		"outside",	important);
		add_parsed_property(_list_style_image_,			"",			important);
		add_parsed_property(_list_style_image_baseurl_,	"",			important);

		string_vector tokens;
		split_string(val, tokens, " ", "", "(");
		for (const auto& token : tokens)
		{
			int idx = value_index(token, list_style_type_strings, -1);
			if (idx >= 0)
			{
				add_parsed_property(_list_style_type_, token, important);
			}
			else
			{
				idx = value_index(token, list_style_position_strings, -1);
				if (idx >= 0)
				{
					add_parsed_property(_list_style_position_, token, important);
				}
				else if (!strncmp(val.c_str(), "url", 3))
				{
					add_parsed_property(_list_style_image_, token, important);
					if (baseurl)
					{
						add_parsed_property(_list_style_image_baseurl_, baseurl, important);
					}
				}
			}
		}
		break;
	}

	case _list_style_image_:
		add_parsed_property(name, val, important);
		if (baseurl)
		{
			add_parsed_property(_list_style_image_baseurl_, baseurl, important);
		}
		break;

	// Parse background shorthand properties 
	case _background_:
		parse_short_background(val, baseurl, important);
		break;

	// Parse margin and padding shorthand properties 
	case _margin_:
	case _padding_:
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		if (tokens.size() >= 4)
		{
			add_parsed_property(_id(_s(name) + "-top"),		tokens[0], important);
			add_parsed_property(_id(_s(name) + "-right"),	tokens[1], important);
			add_parsed_property(_id(_s(name) + "-bottom"),	tokens[2], important);
			add_parsed_property(_id(_s(name) + "-left"),	tokens[3], important);
		}
		else if (tokens.size() == 3)
		{
			add_parsed_property(_id(_s(name) + "-top"),		tokens[0], important);
			add_parsed_property(_id(_s(name) + "-right"),	tokens[1], important);
			add_parsed_property(_id(_s(name) + "-left"),	tokens[1], important);
			add_parsed_property(_id(_s(name) + "-bottom"),	tokens[2], important);
		}
		else if (tokens.size() == 2)
		{
			add_parsed_property(_id(_s(name) + "-top"),		tokens[0], important);
			add_parsed_property(_id(_s(name) + "-bottom"),	tokens[0], important);
			add_parsed_property(_id(_s(name) + "-right"),	tokens[1], important);
			add_parsed_property(_id(_s(name) + "-left"),	tokens[1], important);
		}
		else if (tokens.size() == 1)
		{
			add_parsed_property(_id(_s(name) + "-top"),		tokens[0], important);
			add_parsed_property(_id(_s(name) + "-bottom"),	tokens[0], important);
			add_parsed_property(_id(_s(name) + "-right"),	tokens[0], important);
			add_parsed_property(_id(_s(name) + "-left"),	tokens[0], important);
		}
		break;
	}

	// Parse border-width/style/color shorthand properties 
	case _border_width_:
	case _border_style_:
	case _border_color_:
	{
		string_vector nametokens;
		split_string(_s(name), nametokens, "-");

		string_vector tokens;
		split_string(val, tokens, " ");
		if (tokens.size() >= 4)
		{
			add_parsed_property(_id(nametokens[0] + "-top-"		+ nametokens[1]), tokens[0], important);
			add_parsed_property(_id(nametokens[0] + "-right-"	+ nametokens[1]), tokens[1], important);
			add_parsed_property(_id(nametokens[0] + "-bottom-"	+ nametokens[1]), tokens[2], important);
			add_parsed_property(_id(nametokens[0] + "-left-"	+ nametokens[1]), tokens[3], important);
		}
		else if (tokens.size() == 3)
		{
			add_parsed_property(_id(nametokens[0] + "-top-"		+ nametokens[1]), tokens[0], important);
			add_parsed_property(_id(nametokens[0] + "-right-"	+ nametokens[1]), tokens[1], important);
			add_parsed_property(_id(nametokens[0] + "-left-"	+ nametokens[1]), tokens[1], important);
			add_parsed_property(_id(nametokens[0] + "-bottom-"	+ nametokens[1]), tokens[2], important);
		}
		else if (tokens.size() == 2)
		{
			add_parsed_property(_id(nametokens[0] + "-top-"		+ nametokens[1]), tokens[0], important);
			add_parsed_property(_id(nametokens[0] + "-bottom-"	+ nametokens[1]), tokens[0], important);
			add_parsed_property(_id(nametokens[0] + "-right-"	+ nametokens[1]), tokens[1], important);
			add_parsed_property(_id(nametokens[0] + "-left-"	+ nametokens[1]), tokens[1], important);
		}
		else if (tokens.size() == 1)
		{
			add_parsed_property(_id(nametokens[0] + "-top-"		+ nametokens[1]), tokens[0], important);
			add_parsed_property(_id(nametokens[0] + "-bottom-"	+ nametokens[1]), tokens[0], important);
			add_parsed_property(_id(nametokens[0] + "-right-"	+ nametokens[1]), tokens[0], important);
			add_parsed_property(_id(nametokens[0] + "-left-"	+ nametokens[1]), tokens[0], important);
		}
		break;
	}

	// Parse font shorthand properties 
	case _font_:
		parse_short_font(val, important);
		break;

	// Parse flex-flow shorthand properties
	case _flex_flow_:
	{
		string_vector tokens;
		split_string(val, tokens, " ");
		for (const auto& tok : tokens)
		{
			if (value_in_list(tok, flex_direction_strings))
			{
				add_parsed_property(_flex_direction_, tok, important);
			}
			else if (value_in_list(tok, flex_wrap_strings))
			{
				add_parsed_property(_flex_wrap_, tok, important);
			}
		}
		break;
	}

	// Parse flex-flow shorthand properties
	case _flex_:
	{
		auto is_number = [](const string& val)
		{
			for (auto ch : val)
			{
				if ((ch < '0' || ch > '9') && ch != '.')
				{
					return false;
				}
			}
			return true;
		};
		if (val == "initial")
		{
			// 0 1 auto
			add_parsed_property(_flex_grow_, "0", important);
			add_parsed_property(_flex_shrink_, "1", important);
			add_parsed_property(_flex_basis_, "auto", important);
		}
		else if (val == "auto")
		{
			// 1 1 auto
			add_parsed_property(_flex_grow_, "1", important);
			add_parsed_property(_flex_shrink_, "1", important);
			add_parsed_property(_flex_basis_, "auto", important);
		}
		else if (val == "none")
		{
			// 0 0 auto
			add_parsed_property(_flex_grow_, "0", important);
			add_parsed_property(_flex_shrink_, "0", important);
			add_parsed_property(_flex_basis_, "auto", important);
		}
		string_vector tokens;
		split_string(val, tokens, " ");
		if (tokens.size() == 3)
		{
			add_parsed_property(_flex_grow_, tokens[0], important);
			add_parsed_property(_flex_shrink_, tokens[1], important);
			add_parsed_property(_flex_basis_, tokens[2], important);
		}
		else if (tokens.size() == 2)
		{
			if (is_number(tokens[0]))
			{
				add_parsed_property(_flex_grow_, tokens[0], important);
			}
			else
			{
				if (is_number(tokens[1]))
				{
					add_parsed_property(_flex_shrink_, tokens[0], important);
				}
				else
				{
					add_parsed_property(_flex_basis_, tokens[0], important);
				}
			}
		}
		else if (tokens.size() == 1)
		{
			if (is_number(tokens[0]))
			{
				add_parsed_property(_flex_grow_, tokens[0], important);
				auto v = (float)t_strtod(tokens[0].c_str(), nullptr);
				if (v >= 1)
				{
					add_parsed_property(_flex_shrink_, "1", important);
					add_parsed_property(_flex_basis_, "0", important);
				}
			}
			else
			{
				add_parsed_property(_flex_basis_, tokens[0], important);
			}
		}
		break;
	}

	default:
		add_parsed_property(name, val, important);
	}
}

void litehtml::style::parse_short_background( const string& val, const char* baseurl, bool important )
{
	add_parsed_property(_background_color_,			"transparent",	important);
	add_parsed_property(_background_image_,			"",				important);
	add_parsed_property(_background_image_baseurl_, "",				important);
	add_parsed_property(_background_repeat_,		"repeat",		important);
	add_parsed_property(_background_origin_,		"padding-box",	important);
	add_parsed_property(_background_clip_,			"border-box",	important);
	add_parsed_property(_background_attachment_,	"scroll",		important);

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
			add_parsed_property(_background_image_, token, important);
			if(baseurl)
			{
				add_parsed_property(_background_image_baseurl_, baseurl, important);
			}

		} else if( value_in_list(token, background_repeat_strings) )
		{
			add_parsed_property(_background_repeat_, token, important);
		} else if( value_in_list(token, background_attachment_strings) )
		{
			add_parsed_property(_background_attachment_, token, important);
		} else if( value_in_list(token, background_box_strings) )
		{
			if(!origin_found)
			{
				add_parsed_property(_background_origin_, token, important);
				origin_found = true;
			} else
			{
				add_parsed_property(_background_clip_, token, important);
			}
		} else if(	value_in_list(token, "left;right;top;bottom;center") ||
					iswdigit(token[0]) ||
					token[0] == '-'	||
					token[0] == '.'	||
					token[0] == '+')
		{
			if(m_properties.count(_background_position_))
			{
				m_properties[_background_position_].m_value += " " + token;
			} else
			{
				add_parsed_property(_background_position_, token, important);
			}
		} else if (web_color::is_color(token.c_str()))
		{
			add_parsed_property(_background_color_, token, important);
		}
	}
}

void litehtml::style::parse_short_font( const string& val, bool important )
{
	add_parsed_property(_font_style_,	"normal",	important);
	add_parsed_property(_font_variant_,	"normal",	important);
	add_parsed_property(_font_weight_,	"normal",	important);
	add_parsed_property(_font_size_,	"medium",	important);
	add_parsed_property(_line_height_,	"normal",	important);

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
					add_parsed_property(_font_weight_, token, important);
					add_parsed_property(_font_variant_, token, important);
					add_parsed_property(_font_style_, token, important);
				} else
				{
					add_parsed_property(_font_style_, token, important);
				}
			} else
			{
				if(value_in_list(token, font_weight_strings))
				{
					add_parsed_property(_font_weight_, token, important);
				} else
				{
					if(value_in_list(token, font_variant_strings))
					{
						add_parsed_property(_font_variant_, token, important);
					} else if( iswdigit(token[0]) )
					{
						string_vector szlh;
						split_string(token, szlh, "/");

						if(szlh.size() == 1)
						{
							add_parsed_property(_font_size_,	szlh[0], important);
						} else	if(szlh.size() >= 2)
						{
							add_parsed_property(_font_size_,	szlh[0], important);
							add_parsed_property(_line_height_,	szlh[1], important);
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
	add_parsed_property(_font_family_, font_family, important);
}

void litehtml::style::add_parsed_property( string_id name, const string& val, bool important )
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

void litehtml::style::remove_property( string_id name, bool important )
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
