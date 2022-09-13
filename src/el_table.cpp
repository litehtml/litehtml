#include "html.h"
#include "el_table.h"
#include "document.h"
#include "iterators.h"


litehtml::el_table::el_table(const std::shared_ptr<litehtml::document>& doc) : html_tag(doc)
{
}


bool litehtml::el_table::appendChild(const litehtml::element::ptr& el)
{
	if(!el)	return false;
	if( !t_strcmp(el->get_tagName(), _t("tbody")) || 
		!t_strcmp(el->get_tagName(), _t("thead")) || 
		!t_strcmp(el->get_tagName(), _t("tfoot")) ||
		!t_strcmp(el->get_tagName(), _t("caption")))
	{
		return html_tag::appendChild(el);
	}
	return false;
}

void litehtml::el_table::parse_attributes()
{
	const tchar_t* str = get_attr(_t("width"));
	if(str)
	{
		m_style.add_property(_t("width"), str, nullptr, false, this);
	}

	str = get_attr(_t("align"));
	if(str)
	{
		int align = value_index(str, _t("left;center;right"));
		switch(align)
		{
		case 1:
			m_style.add_property(_t("margin-left"), _t("auto"), nullptr, false, this);
			m_style.add_property(_t("margin-right"), _t("auto"), nullptr, false, this);
			break;
		case 2:
			m_style.add_property(_t("margin-left"), _t("auto"), nullptr, false, this);
			m_style.add_property(_t("margin-right"), _t("0"), nullptr, false, this);
			break;
		}
	}

	str = get_attr(_t("cellspacing"));
	if(str)
	{
		tstring val = str;
		val += _t(" ");
		val += str;
		m_style.add_property(_t("border-spacing"), val.c_str(), nullptr, false, this);
	}
	
	str = get_attr(_t("border"));
	if(str)
	{
		m_style.add_property(_t("border-width"), str, nullptr, false, this);
	}

	str = get_attr(_t("bgcolor"));
	if (str)
	{
		m_style.add_property(_t("background-color"), str, nullptr, false, this);
	}

	html_tag::parse_attributes();
}
