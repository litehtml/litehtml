#include "html.h"
#include "el_td.h"


litehtml::el_td::el_td(const std::shared_ptr<litehtml::document>& doc) : html_tag(doc)
{

}

void litehtml::el_td::parse_attributes()
{
	const tchar_t* str = get_attr(_t("width"));
	if(str)
	{
		m_style.add_property(_t("width"), str, nullptr, false, this);
	}
	str = get_attr(_t("background"));
	if(str)
	{
		tstring url = _t("url('");
		url += str;
		url += _t("')");
		m_style.add_property(_t("background-image"), url.c_str(), nullptr, false, this);
	}
	str = get_attr(_t("align"));
	if(str)
	{
		m_style.add_property(_t("text-align"), str, nullptr, false, this);
	}

	str = get_attr(_t("bgcolor"));
	if (str)
	{
		m_style.add_property(_t("background-color"), str, nullptr, false, this);
	}

	str = get_attr(_t("valign"));
	if(str)
	{
		m_style.add_property(_t("vertical-align"), str, nullptr, false, this);
	}
	html_tag::parse_attributes();
}

litehtml::element::ptr litehtml::el_td::clone(const element::ptr& cloned_el)
{
    auto ret = std::dynamic_pointer_cast<litehtml::el_td>(cloned_el);
    if(!ret)
    {
        ret = std::make_shared<el_td>(get_document());
        html_tag::clone(ret);
    }

    return cloned_el ? nullptr : ret;
}
