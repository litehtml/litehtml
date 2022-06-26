#include "html.h"
#include "el_tr.h"


litehtml::el_tr::el_tr(const std::shared_ptr<litehtml::document>& doc) : html_tag(doc)
{

}

void litehtml::el_tr::parse_attributes()
{
	const tchar_t* str = get_attr(_t("align"));
	if(str)
	{
		m_style.add_property(_t("text-align"), str, nullptr, false, this);
	}
	str = get_attr(_t("valign"));
	if(str)
	{
		m_style.add_property(_t("vertical-align"), str, nullptr, false, this);
	}
	str = get_attr(_t("bgcolor"));
	if (str)
	{
		m_style.add_property(_t("background-color"), str, nullptr, false, this);
	}
	html_tag::parse_attributes();
}
