#include "html.h"
#include "el_font.h"


litehtml::el_font::el_font(const std::shared_ptr<document>& doc) : html_tag(doc)
{

}

void litehtml::el_font::parse_attributes()
{
	const char* str = get_attr("color");
	if(str)
	{
		m_style.add_property(_color_, str, nullptr, false, this);
	}

	str = get_attr("face");
	if(str)
	{
		m_style.add_property(_font_family_, str, nullptr, false, this);
	}

	str = get_attr("size");
	if(str)
	{
		int sz = atoi(str);
		if(sz <= 1)
		{
			m_style.add_property(_font_size_, "x-small", nullptr, false, this);
		} else if(sz >= 6)
		{
			m_style.add_property(_font_size_, "xx-large", nullptr, false, this);
		} else
		{
			switch(sz)
			{
			case 2:
				m_style.add_property(_font_size_, "small", nullptr, false, this);
				break;
			case 3:
				m_style.add_property(_font_size_, "medium", nullptr, false, this);
				break;
			case 4:
				m_style.add_property(_font_size_, "large", nullptr, false, this);
				break;
			case 5:
				m_style.add_property(_font_size_, "x-large", nullptr, false, this);
				break;
			}
		}
	}

	html_tag::parse_attributes();
}
