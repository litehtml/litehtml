#include "html.h"
#include "el_font.h"


litehtml::el_font::el_font( litehtml::document* doc ) : element(doc)
{

}

litehtml::el_font::~el_font()
{

}

void litehtml::el_font::parse_styles(bool is_reparse)
{
	const wchar_t* str = get_attr(L"color");
	if(str)
	{
		m_style.add_property(L"color", str, 0, false);
	}

	str = get_attr(L"face");
	if(str)
	{
		m_style.add_property(L"font-face", str, 0, false);
	}

	str = get_attr(L"size");
	if(str)
	{
		int sz = _wtoi(str);
		if(sz <= 1)
		{
			m_style.add_property(L"font-size", L"x-small", 0, false);
		} else if(sz >= 6)
		{
			m_style.add_property(L"font-size", L"xx-large", 0, false);
		} else
		{
			switch(sz)
			{
			case 2:
				m_style.add_property(L"font-size", L"small", 0, false);
				break;
			case 3:
				m_style.add_property(L"font-size", L"medium", 0, false);
				break;
			case 4:
				m_style.add_property(L"font-size", L"large", 0, false);
				break;
			case 5:
				m_style.add_property(L"font-size", L"x-large", 0, false);
				break;
			}
		}
	}

	element::parse_styles(is_reparse);
}
