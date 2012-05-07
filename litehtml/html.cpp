#include "html.h"
#include "types.h"
#include "tokenizer.h"
#include "element.h"

void litehtml::trim(std::wstring &s) 
{
	std::wstring::size_type pos = s.find_first_not_of(L" \n\r\t");
	if(pos != std::wstring::npos)
	{
		s.erase(s.begin(), s.begin() + pos);
	}
	pos = s.find_last_not_of(L" \n\r\t");
	if(pos != std::wstring::npos)
	{
		s.erase(s.begin() + pos + 1, s.end());
	}
}

int litehtml::value_index( const wchar_t* val, const wchar_t* strings, int defValue, const wchar_t* delim )
{
	if(!val || !strings || !delim)
	{
		return defValue;
	}
	string_vector tokens;
	tokenize(strings, tokens, delim);
	for(size_t i = 0; i < tokens.size(); i++)
	{
		if(tokens[i] == val)
		{
			return (int) i;
		}
	}
	return defValue;
}


litehtml::web_color litehtml::web_color::from_string( const wchar_t* str )
{
	if(!str)
	{
		return web_color(0, 0, 0);
	}
	if(str[0] == L'#')
	{
		std::wstring red		= L"";
		std::wstring green		= L"";
		std::wstring blue		= L"";
		if(wcslen(str + 1) == 3)
		{
			red		+= str[1];
			red		+= str[1];
			green	+= str[2];
			green	+= str[2];
			blue	+= str[3];
			blue	+= str[3];
		} else if(wcslen(str + 1) == 6)
		{
			red		+= str[1];
			red		+= str[2];
			green	+= str[3];
			green	+= str[4];
			blue	+= str[5];
			blue	+= str[6];
		}
		wchar_t* sss = 0;
		web_color clr;
		clr.red		= (byte) wcstol(red.c_str(),	&sss, 16);
		clr.green	= (byte) wcstol(green.c_str(),	&sss, 16);
		clr.blue	= (byte) wcstol(blue.c_str(),	&sss, 16);
		return clr;
	} else if(!wcsncmp(str, L"rgb", 3))
	{
		std::wstring s = str;

		std::wstring::size_type pos = s.find_first_of(L"(");
		if(pos != std::wstring::npos)
		{
			s.erase(s.begin(), s.begin() + pos + 1);
		}
		pos = s.find_last_of(L")");
		if(pos != std::wstring::npos)
		{
			s.erase(s.begin() + pos, s.end());
		}

		std::vector<std::wstring> tokens;
		tokenize(s, tokens, L", \t");

		web_color clr;

		if(tokens.size() >= 1)	clr.red		= (byte) _wtoi(tokens[0].c_str());
		if(tokens.size() >= 2)	clr.green	= (byte) _wtoi(tokens[1].c_str());
		if(tokens.size() >= 3)	clr.blue	= (byte) _wtoi(tokens[2].c_str());
		if(tokens.size() >= 4)	clr.alpha	= (byte) _wtoi(tokens[3].c_str());

		return clr;
	}
	return web_color(0, 0, 0);
}


void litehtml::inline_pos::place_element( position& pos, style_display display )
{
	switch(display)
	{
	case display_block:
		{
			x			= block.left();
			y			+= line_height;
			pos.move_to(x, y);
			line_height	= pos.height;
		}
		break;
	case display_inline:
		{
			if(x + pos.width > block.right())
			{
				x			= block.left();
				y			+= line_height;
				line_height	= pos.height;
			} else
			{
				line_height = max(line_height, pos.height);
			}
			pos.move_to(x, y);
			x += pos.width;
		}
		break;
	}
}

void litehtml::inline_pos::next_place( position& pos, style_display display )
{
	switch(display)
	{
	case display_block:
		pos.move_to(block.left(), y + line_height);
		break;
	case display_inline:
		{
			if(x + pos.width > block.right())
			{
				pos.move_to(block.left(), y + line_height);
			} else
			{
				pos.move_to(x, y);
			}
		}
		break;
	}
}

void litehtml::css_element_selector::parse( const std::wstring& txt )
{
	std::wstring::size_type el_end = txt.find_first_of(L".#[:");
	m_tag = txt.substr(0, el_end);
	while(el_end != std::wstring::npos)
	{
		if(txt[el_end] == L'.')
		{
			css_attribute_selector attribute;

			std::wstring::size_type pos = txt.find_first_of(L".#[:", el_end + 1);
			attribute.val		= txt.substr(el_end + 1, pos - el_end - 1);
			attribute.condition	= select_equal;
			m_attrs[L"class"] = attribute;
			el_end = pos;
		} else if(txt[el_end] == L'#')
		{
			css_attribute_selector attribute;

			std::wstring::size_type pos = txt.find_first_of(L".#[:", el_end + 1);
			attribute.val		= txt.substr(el_end + 1, pos - el_end - 1);
			attribute.condition	= select_equal;
			m_attrs[L"id"] = attribute;
			el_end = pos;
		} else if(txt[el_end] == L'[')
		{
			css_attribute_selector attribute;

			std::wstring::size_type pos = txt.find_first_of(L"]~=|$*^", el_end + 1);
			std::wstring attr = txt.substr(el_end + 1, pos - el_end - 1);
			trim(attr);
			if(pos != std::wstring::npos)
			{
				if(txt[pos] == L']')
				{
					attribute.condition = select_exists;
				} else if(txt[pos] == L'=')
				{
					attribute.condition = select_equal;
					pos++;
				} else if(txt.substr(pos, 2) == L"~=")
				{
					attribute.condition = select_contain_str;
					pos += 2;
				} else if(txt.substr(pos, 2) == L"|=")
				{
					attribute.condition = select_start_str;
					pos += 2;
				} else if(txt.substr(pos, 2) == L"^=")
				{
					attribute.condition = select_start_str;
					pos += 2;
				} else if(txt.substr(pos, 2) == L"$=")
				{
					attribute.condition = select_end_str;
					pos += 2;
				} else if(txt.substr(pos, 2) == L"*=")
				{
					attribute.condition = select_contain_str;
					pos += 2;
				} else
				{
					attribute.condition = select_exists;
					pos += 1;
				}
				pos = txt.find_first_not_of(L" \t", pos);
				if(pos != std::wstring::npos)
				{
					if(txt[pos] == L'"')
					{
						std::wstring::size_type pos2 = txt.find_first_of(L"\"", pos + 1);
						attribute.val = txt.substr(pos + 1, pos2 == std::wstring::npos ? pos2 : (pos2 - pos - 2));
						pos = pos2 == std::wstring::npos ? pos2 : (pos2 + 1);
					} else if(txt[pos] == L']')
					{
						pos ++;
					} else
					{
						std::wstring::size_type pos2 = txt.find_first_of(L"]", pos + 1);
						attribute.val = txt.substr(pos, pos2 == std::wstring::npos ? pos2 : (pos2 - pos));
						pos = pos2 == std::wstring::npos ? pos2 : (pos2 + 1);
					}
				}
			} else
			{
				attribute.condition = select_exists;
			}
			m_attrs[attr] = attribute;
			el_end = pos;
		} else
		{
			el_end++;
		}
		el_end = txt.find_first_of(L".#[:", el_end);
	}
}


bool litehtml::css_selector::parse( const std::wstring& text )
{
	if(text.empty())
	{
		return false;
	}
	string_vector tokens;
	tokenize(text, tokens, L"", L" \t>+~");

	if(tokens.empty())
	{
		return false;
	}

	std::wstring left;
	std::wstring right = tokens.back();
	wchar_t combinator = 0;

	tokens.pop_back();
	while(!tokens.empty() && (tokens.back() == L" " || tokens.back() == L"\t" || tokens.back() == L"+" || tokens.back() == L"~"))
	{
		if(combinator == L' ' || combinator == 0)
		{
			combinator = tokens.back()[0];
		}
		tokens.pop_back();
	}

	for(string_vector::const_iterator i = tokens.begin(); i != tokens.end(); i++)
	{
		left += *i;
	}

	trim(left);
	trim(right);

	if(right.empty())
	{
		return false;
	}

	m_right.parse(right);

	switch(combinator)
	{
	case L'>':
		m_combinator	= combinator_child;
		break;
	case L'+':
		m_combinator	= combinator_adjacent_sibling;
		break;
	case L'~':
		m_combinator	= combinator_adjacent_sibling;
		break;
	default:
		m_combinator	= combinator_descendant;
	}

	if(m_left)
	{
		delete m_left;
	}
	m_left = 0;

	if(!left.empty())
	{
		m_left = new css_selector;
		if(!m_left->parse(left))
		{
			return false;
		}
	}

	return true;
}
