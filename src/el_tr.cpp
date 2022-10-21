#include "html.h"
#include "el_tr.h"


litehtml::el_tr::el_tr(const std::shared_ptr<document>& doc) : html_tag(doc)
{

}

void litehtml::el_tr::parse_attributes()
{
	const char* str = get_attr("align");
	if(str)
	{
		m_style.add_property("text-align", str, nullptr, false, this);
	}
	str = get_attr("valign");
	if(str)
	{
		m_style.add_property("vertical-align", str, nullptr, false, this);
	}
	str = get_attr("bgcolor");
	if (str)
	{
		m_style.add_property("background-color", str, nullptr, false, this);
	}
	html_tag::parse_attributes();
}
