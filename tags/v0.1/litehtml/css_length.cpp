#include "html.h"
#include "css_length.h"

void litehtml::css_length::fromString( const std::wstring& str, const std::wstring& predefs )
{
	int predef = value_index(str.c_str(), predefs.c_str(), -1);
	if(predef >= 0)
	{
		m_is_predefined = true;
		m_predef		= predef;
	} else
	{
		m_is_predefined = false;

		std::wstring num;
		std::wstring un;
		bool is_unit = false;
		for(std::wstring::const_iterator chr = str.begin(); chr != str.end(); chr++)
		{
			if(!is_unit)
			{
				if(iswdigit(*chr) || *chr == L'.' || *chr == L'+' || *chr == L'-')
				{
					num += *chr;
				} else
				{
					is_unit = true;
				}
			}
			if(is_unit)
			{
				un += *chr;
			}
		}
		m_value = (float) _wtof(num.c_str());
		m_units	= (css_units) value_index(un.c_str(), css_units_strings, css_units_none);
	}
}
