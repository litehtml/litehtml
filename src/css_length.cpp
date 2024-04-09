#include "html.h"
#include "css_length.h"

bool litehtml::css_length::from_token(const css_token& token, int options, const string& keywords)
{
	if ((options & f_positive) && is_one_of(token.type, NUMBER, DIMENSION, PERCENTAGE) && token.n.number < 0)
		return false;

	if (token.type == IDENT)
	{
		int idx = value_index(lowcase(token.name), keywords);
		if (idx == -1) return false;
		m_predef = idx;
		m_is_predefined = true;
		return true;
	}
	else if (token.type == DIMENSION)
	{
		if (!(options & f_length)) return false;

		int idx = value_index(lowcase(token.unit), css_units_strings);
		// note: 1none and 1\% are invalid
		if (idx == -1 || idx == css_units_none || idx == css_units_percentage)
			return false;
		
		m_value = token.n.number;
		m_units = (css_units)idx;
		m_is_predefined = false;
		return true;
	}
	else if (token.type == PERCENTAGE)
	{
		if (!(options & f_percentage)) return false;

		m_value = token.n.number;
		m_units = css_units_percentage;
		m_is_predefined = false;
		return true;
	}
	else if (token.type == NUMBER)
	{
		// if token is a nonzero number and neither f_number nor f_integer is specified in the options
		if (!(options & (f_number | f_integer)) && token.n.number != 0)
			return false;
		// if token is a zero number and neither of f_number, f_integer or f_length are specified in the options
		if (!(options & (f_number | f_integer | f_length)) && token.n.number == 0)
			return false;
		if ((options & f_integer) && token.n.number_type != css_number_integer)
			return false;
		
		m_value = token.n.number;
		m_units = css_units_none;
		m_is_predefined = false;
		return true;
	}
	return false;
}

void litehtml::css_length::fromString( const string& str, const string& predefs, int defValue )
{
	// TODO: Make support for calc
	if(str.substr(0, 4) == "calc")
	{
		m_is_predefined = true;
		m_predef		= defValue;
		return;
	}

	int predef = value_index(str, predefs, -1);
	if(predef >= 0)
	{
		m_is_predefined = true;
		m_predef		= predef;
	} else
	{
		m_is_predefined = false;

		string num;
		string un;
		bool is_unit = false;
		for(char chr : str)
		{
			if(!is_unit)
			{
				if(t_isdigit(chr) || chr == '.' || chr == '+' || chr == '-')
				{
					num += chr;
				} else
				{
					is_unit = true;
				}
			}
			if(is_unit)
			{
				un += chr;
			}
		}
		if(!num.empty())
		{
			m_value = t_strtof(num);
			m_units	= (css_units) value_index(un, css_units_strings, css_units_none);
		} else
		{
			// not a number so it is predefined
			m_is_predefined = true;
			m_predef = defValue;
		}
	}
}

litehtml::css_length litehtml::css_length::from_string(const string& str, const string& predefs, int defValue)
{
	css_length len;
	len.fromString(str, predefs, defValue);
	return len;
}

litehtml::string litehtml::css_length::to_string() const
{
    if(m_is_predefined)
    {
        return "def(" + std::to_string(m_predef) + ")";
    }
    return std::to_string(m_value) + "{" + index_value(m_units, css_units_strings) + "}";
}

litehtml::css_length litehtml::css_length::predef_value(int val)
{
	css_length len;
	len.predef(val);
	return len;
}
