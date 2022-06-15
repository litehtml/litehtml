#include "html.h"
#include "el_font.h"


litehtml::el_font::el_font(const std::shared_ptr<litehtml::document>& doc) : html_tag(doc)
{

}

void litehtml::el_font::parse_attributes()
{
	const tchar_t* str = get_attr(_t("color"));
	if(str)
	{
		m_style.add_property(_t("color"), str, nullptr, false, this);
	}

	str = get_attr(_t("face"));
	if(str)
	{
		m_style.add_property(_t("font-face"), str, nullptr, false, this);
	}

	str = get_attr(_t("size"));
	if(str)
	{
		int sz = t_atoi(str);
		if(sz <= 1)
		{
			m_style.add_property(_t("font-size"), _t("x-small"), nullptr, false, this);
		} else if(sz >= 6)
		{
			m_style.add_property(_t("font-size"), _t("xx-large"), nullptr, false, this);
		} else
		{
			switch(sz)
			{
			case 2:
				m_style.add_property(_t("font-size"), _t("small"), nullptr, false, this);
				break;
			case 3:
				m_style.add_property(_t("font-size"), _t("medium"), nullptr, false, this);
				break;
			case 4:
				m_style.add_property(_t("font-size"), _t("large"), nullptr, false, this);
				break;
			case 5:
				m_style.add_property(_t("font-size"), _t("x-large"), nullptr, false, this);
				break;
			}
		}
	}

	html_tag::parse_attributes();
}

litehtml::element::ptr litehtml::el_font::clone(const element::ptr& cloned_el)
{
    auto ret = std::dynamic_pointer_cast<litehtml::el_font>(cloned_el);
    if(!ret)
    {
        ret = std::make_shared<el_font>(get_document());
        html_tag::clone(ret);
    }

    return cloned_el ? nullptr : ret;
}
