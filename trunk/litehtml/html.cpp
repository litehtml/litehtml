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

int litehtml::value_in_list( const wchar_t* val, const wchar_t* strings, const wchar_t* delim )
{
	int idx = value_index(val, strings, -1, delim);
	if(idx >= 0)
	{
		return true;
	}
	return false;
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
	tokenize(text, tokens, L"", L" \t>+~:");

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
	case L':':
		m_combinator	= combinator_pseudo;
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
