#include "html.h"
#include "el_table.h"
#include "document.h"
#include "iterators.h"


litehtml::el_table::el_table(const std::shared_ptr<document>& doc) : html_tag(doc)
{
}


bool litehtml::el_table::appendChild(const element::ptr& el)
{
	if(!el)	return false;
	if( !strcmp(el->get_tagName(), "tbody") || 
		!strcmp(el->get_tagName(), "thead") || 
		!strcmp(el->get_tagName(), "tfoot") ||
		!strcmp(el->get_tagName(), "caption"))
	{
		return html_tag::appendChild(el);
	}
	return false;
}

void litehtml::el_table::parse_attributes()
{
	const char* str = get_attr("width");
	if(str)
	{
		m_style.add_property("width", str, nullptr, false, this);
	}

	str = get_attr("align");
	if(str)
	{
		int align = value_index(str, "left;center;right");
		switch(align)
		{
		case 1:
			m_style.add_property("margin-left", "auto", nullptr, false, this);
			m_style.add_property("margin-right", "auto", nullptr, false, this);
			break;
		case 2:
			m_style.add_property("margin-left", "auto", nullptr, false, this);
			m_style.add_property("margin-right", "0", nullptr, false, this);
			break;
		}
	}

	str = get_attr("cellspacing");
	if(str)
	{
		string val = str;
		val += " ";
		val += str;
		m_style.add_property("border-spacing", val.c_str(), nullptr, false, this);
	}
	
	str = get_attr("border");
	if(str)
	{
		m_style.add_property("border-width", str, nullptr, false, this);
	}

	str = get_attr("bgcolor");
	if (str)
	{
		m_style.add_property("background-color", str, nullptr, false, this);
	}

	html_tag::parse_attributes();
}
