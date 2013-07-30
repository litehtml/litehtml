#include "html.h"
#include "css_selector.h"
#include "tokenizer.h"

void litehtml::css_element_selector::parse( const tstring& txt )
{
	tstring::size_type el_end = txt.find_first_of(_t(".#[:"));
	m_tag = txt.substr(0, el_end);
	while(el_end != tstring::npos)
	{
		if(txt[el_end] == _t('.'))
		{
			css_attribute_selector attribute;

			tstring::size_type pos = txt.find_first_of(_t(".#[:"), el_end + 1);
			attribute.val		= txt.substr(el_end + 1, pos - el_end - 1);
			attribute.condition	= select_equal;
			m_attrs[_t("class")] = attribute;
			el_end = pos;
		} else if(txt[el_end] == _t(':'))
		{
			css_attribute_selector attribute;

			tstring::size_type pos = txt.find_first_of(_t(".#[:"), el_end + 1);
			attribute.val		= txt.substr(el_end + 1, pos - el_end - 1);
			attribute.condition	= select_pseudo_class;
			m_attrs[_t("pseudo")] = attribute;
			el_end = pos;
		} else if(txt[el_end] == _t('#'))
		{
			css_attribute_selector attribute;

			tstring::size_type pos = txt.find_first_of(_t(".#[:"), el_end + 1);
			attribute.val		= txt.substr(el_end + 1, pos - el_end - 1);
			attribute.condition	= select_equal;
			m_attrs[_t("id")] = attribute;
			el_end = pos;
		} else if(txt[el_end] == _t('['))
		{
			css_attribute_selector attribute;

			tstring::size_type pos = txt.find_first_of(_t("]~=|$*^"), el_end + 1);
			tstring attr = txt.substr(el_end + 1, pos - el_end - 1);
			trim(attr);
			if(pos != tstring::npos)
			{
				if(txt[pos] == _t(']'))
				{
					attribute.condition = select_exists;
				} else if(txt[pos] == _t('='))
				{
					attribute.condition = select_equal;
					pos++;
				} else if(txt.substr(pos, 2) == _t("~="))
				{
					attribute.condition = select_contain_str;
					pos += 2;
				} else if(txt.substr(pos, 2) == _t("|="))
				{
					attribute.condition = select_start_str;
					pos += 2;
				} else if(txt.substr(pos, 2) == _t("^="))
				{
					attribute.condition = select_start_str;
					pos += 2;
				} else if(txt.substr(pos, 2) == _t("$="))
				{
					attribute.condition = select_end_str;
					pos += 2;
				} else if(txt.substr(pos, 2) == _t("*="))
				{
					attribute.condition = select_contain_str;
					pos += 2;
				} else
				{
					attribute.condition = select_exists;
					pos += 1;
				}
				pos = txt.find_first_not_of(_t(" \t"), pos);
				if(pos != tstring::npos)
				{
					if(txt[pos] == _t('"'))
					{
						tstring::size_type pos2 = txt.find_first_of(_t("\""), pos + 1);
						attribute.val = txt.substr(pos + 1, pos2 == tstring::npos ? pos2 : (pos2 - pos - 2));
						pos = pos2 == tstring::npos ? pos2 : (pos2 + 1);
					} else if(txt[pos] == _t(']'))
					{
						pos ++;
					} else
					{
						tstring::size_type pos2 = txt.find_first_of(_t("]"), pos + 1);
						attribute.val = txt.substr(pos, pos2 == tstring::npos ? pos2 : (pos2 - pos));
						pos = pos2 == tstring::npos ? pos2 : (pos2 + 1);
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
		el_end = txt.find_first_of(_t(".#[:"), el_end);
	}
}


bool litehtml::css_selector::parse( const tstring& text )
{
	if(text.empty())
	{
		return false;
	}
	string_vector tokens;
	tokenize(text, tokens, _t(""), _t(" \t>+~"));

	if(tokens.empty())
	{
		return false;
	}

	tstring left;
	tstring right = tokens.back();
	tchar_t combinator = 0;

	tokens.pop_back();
	while(!tokens.empty() && (tokens.back() == _t(" ") || tokens.back() == _t("\t") || tokens.back() == _t("+") || tokens.back() == _t("~") || tokens.back() == _t(">")))
	{
		if(combinator == _t(' ') || combinator == 0)
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
	case _t('>'):
		m_combinator	= combinator_child;
		break;
	case _t('+'):
		m_combinator	= combinator_adjacent_sibling;
		break;
	case _t('~'):
		m_combinator	= combinator_adjacent_sibling;
		break;
	default:
		m_combinator	= combinator_descendant;
		break;
	}

/*	if(m_left)
	{
		delete m_left;
	}*/
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

void litehtml::css_selector::calc_specificity()
{
	if(!m_right.m_tag.empty() && m_right.m_tag != _t("*"))
	{
		m_specificity.d = 1;
	}
	for(css_attribute_selector::map::iterator i = m_right.m_attrs.begin(); i != m_right.m_attrs.end(); i++)
	{
		if(i->first == _t("id"))
		{
			m_specificity.b++;
		} else
		{
			m_specificity.c++;
		}	
	}
	if(m_left)
	{
		m_left->calc_specificity();
		m_specificity += m_left->m_specificity;
	}
}
