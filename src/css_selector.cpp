#include "html.h"
#include "css_selector.h"
#include "document.h"

void litehtml::css_element_selector::parse( const string& txt )
{
	string::size_type el_end = txt.find_first_of(".#[:");
	m_tag = txt.substr(0, el_end);
	litehtml::lcase(m_tag);
	m_attrs.clear();
	while(el_end != string::npos)
	{
		if(txt[el_end] == '.')
		{
			css_attribute_selector attribute;

			string::size_type pos = txt.find_first_of(".#[:", el_end + 1);
			attribute.val		= txt.substr(el_end + 1, pos - el_end - 1);
			split_string( attribute.val, attribute.class_val, " " );
			attribute.condition	= select_equal;
			attribute.attribute	= "class";
			m_attrs.push_back(attribute);
			el_end = pos;
		} else if(txt[el_end] == ':')
		{
			css_attribute_selector attribute;

			if(txt[el_end + 1] == ':')
			{
				string::size_type pos = txt.find_first_of(".#[:", el_end + 2);
				attribute.val		= txt.substr(el_end + 2, pos - el_end - 2);
				attribute.condition	= select_pseudo_element;
				litehtml::lcase(attribute.val);
				attribute.attribute	= "pseudo-el";
				m_attrs.push_back(attribute);
				el_end = pos;
			} else
			{
				string::size_type pos = txt.find_first_of(".#[:(", el_end + 1);
				if(pos != string::npos && txt.at(pos) == '(')
				{
					pos = find_close_bracket(txt, pos);
					if(pos != string::npos)
					{
						pos++;
					}
				}
				if(pos != string::npos)
				{
					attribute.val		= txt.substr(el_end + 1, pos - el_end - 1);
				} else
				{
					attribute.val		= txt.substr(el_end + 1);
				}
				litehtml::lcase(attribute.val);
				if(attribute.val == "after" || attribute.val == "before")
				{
					attribute.condition	= select_pseudo_element;
				} else
				{
					attribute.condition	= select_pseudo_class;
				}
				attribute.attribute	= "pseudo";
				m_attrs.push_back(attribute);
				el_end = pos;
			}
		} else if(txt[el_end] == '#')
		{
			css_attribute_selector attribute;

			string::size_type pos = txt.find_first_of(".#[:", el_end + 1);
			attribute.val		= txt.substr(el_end + 1, pos - el_end - 1);
			attribute.condition	= select_equal;
			attribute.attribute	= "id";
			m_attrs.push_back(attribute);
			el_end = pos;
		} else if(txt[el_end] == '[')
		{
			css_attribute_selector attribute;

			string::size_type pos = txt.find_first_of("]~=|$*^", el_end + 1);
			string attr = txt.substr(el_end + 1, pos - el_end - 1);
			trim(attr);
			litehtml::lcase(attr);
			if(pos != string::npos)
			{
				if(txt[pos] == ']')
				{
					attribute.condition = select_exists;
				} else if(txt[pos] == '=')
				{
					attribute.condition = select_equal;
					pos++;
				} else if(txt.substr(pos, 2) == "~=")
				{
					attribute.condition = select_contain_str;
					pos += 2;
				} else if(txt.substr(pos, 2) == "|=")
				{
					attribute.condition = select_start_str;
					pos += 2;
				} else if(txt.substr(pos, 2) == "^=")
				{
					attribute.condition = select_start_str;
					pos += 2;
				} else if(txt.substr(pos, 2) == "$=")
				{
					attribute.condition = select_end_str;
					pos += 2;
				} else if(txt.substr(pos, 2) == "*=")
				{
					attribute.condition = select_contain_str;
					pos += 2;
				} else
				{
					attribute.condition = select_exists;
					pos += 1;
				}
				pos = txt.find_first_not_of(" \t", pos);
				if(pos != string::npos)
				{
					if(txt[pos] == '"')
					{
						string::size_type pos2 = txt.find_first_of('\"', pos + 1);
						attribute.val = txt.substr(pos + 1, pos2 == string::npos ? pos2 : (pos2 - pos - 1));
						pos = pos2 == string::npos ? pos2 : (pos2 + 1);
					} else if(txt[pos] == ']')
					{
						pos ++;
					} else
					{
						string::size_type pos2 = txt.find_first_of(']', pos + 1);
						attribute.val = txt.substr(pos, pos2 == string::npos ? pos2 : (pos2 - pos));
						trim(attribute.val);
						pos = pos2 == string::npos ? pos2 : (pos2 + 1);
					}
				}
			} else
			{
				attribute.condition = select_exists;
			}
			attribute.attribute	= attr;
			m_attrs.push_back(attribute);
			el_end = pos;
		} else
		{
			el_end++;
		}
		el_end = txt.find_first_of(".#[:", el_end);
	}
}


bool litehtml::css_selector::parse( const string& text )
{
	if(text.empty())
	{
		return false;
	}
	string_vector tokens;
	split_string(text, tokens, "", " \t>+~", "([");

	if(tokens.empty())
	{
		return false;
	}

	string left;
	string right = tokens.back();
	char combinator = 0;

	tokens.pop_back();
	while(!tokens.empty() && (tokens.back() == " " || tokens.back() == "\t" || tokens.back() == "+" || tokens.back() == "~" || tokens.back() == ">"))
	{
		if(combinator == ' ' || combinator == 0)
		{
			combinator = tokens.back()[0];
		}
		tokens.pop_back();
	}

	for(const auto & token : tokens)
	{
		left += token;
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
	case '>':
		m_combinator	= combinator_child;
		break;
	case '+':
		m_combinator	= combinator_adjacent_sibling;
		break;
	case '~':
		m_combinator	= combinator_general_sibling;
		break;
	default:
		m_combinator	= combinator_descendant;
		break;
	}

	m_left = nullptr;

	if(!left.empty())
	{
		m_left = std::make_shared<css_selector>(media_query_list::ptr(nullptr), "");
		if(!m_left->parse(left))
		{
			return false;
		}
	}

	return true;
}

void litehtml::css_selector::calc_specificity()
{
	if(!m_right.m_tag.empty() && m_right.m_tag != "*")
	{
		m_specificity.d = 1;
	}
	for(const auto& attr : m_right.m_attrs)
	{
		if(attr.attribute == "id")
		{
			m_specificity.b++;
		} else
		{
			if(attr.attribute == "class")
			{
				m_specificity.c += (int) attr.class_val.size();
			} else
			{
				m_specificity.c++;
			}
		}	
	}
	if(m_left)
	{
		m_left->calc_specificity();
		m_specificity += m_left->m_specificity;
	}
}

void litehtml::css_selector::add_media_to_doc( document* doc ) const
{
	if(m_media_query && doc)
	{
		doc->add_media_list(m_media_query);
	}
}

