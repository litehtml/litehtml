#include "html.h"
#include "css_length.h"

void litehtml::css_length::fromString( const tstring& str, const std::vector<tstring> & predefs, int defValue )
{
	// TODO: Make support for calc
	if( str.length() >= 4 && t_strncmp( str.c_str(), _t("calc"), 4 ) == 0 )
	{
		m_is_predefined = true;
		m_predef		= 0;
		return;
	}

	int predef = value_index(str, predefs);
	if(predef >= 0)
	{
		m_is_predefined = true;
		m_predef		= predef;
	} else
	{
		m_is_predefined = false;

		char * end_character;
		float value = (float) t_strtod(str.c_str(), &end_character);

		if( end_character != str.c_str() )
		{
			m_value = value;
			m_units	= (css_units) value_index( end_character, css_units_strings, css_units_none);
		} else
		{
			// not a number so it is predefined
			m_is_predefined = true;
			m_predef = defValue;
		}
	}
}
