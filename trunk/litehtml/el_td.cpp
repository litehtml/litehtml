#include "html.h"
#include "el_td.h"


litehtml::el_td::el_td( litehtml::document* doc ) : html_tag(doc)
{

}

litehtml::el_td::~el_td()
{

}

void litehtml::el_td::parse_styles(bool is_reparse)
{
	const tchar_t* str = get_attr(_t("width"));
	if(str)
	{
		m_style.add_property(_t("width"), str, 0, false);
	}
	str = get_attr(_t("background"));
	if(str)
	{
		tstring url = _t("url('");
		url += str;
		url += _t("')");
		m_style.add_property(_t("background-image"), url.c_str(), 0, false);
	}
	str = get_attr(_t("align"));
	if(str)
	{
		m_style.add_property(_t("text-align"), str, 0, false);
	}

	str = get_attr(_t("valign"));
	if(str)
	{
		m_style.add_property(_t("vertical-align"), str, 0, false);
	}
	html_tag::parse_styles(is_reparse);
}

