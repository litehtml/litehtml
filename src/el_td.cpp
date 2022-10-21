#include "html.h"
#include "el_td.h"


litehtml::el_td::el_td(const std::shared_ptr<document>& doc) : html_tag(doc)
{

}

void litehtml::el_td::parse_attributes()
{
	const char* str = get_attr("width");
	if(str)
	{
		m_style.add_property("width", str, nullptr, false, this);
	}
	str = get_attr("background");
	if(str)
	{
		string url = "url('";
		url += str;
		url += "')";
		m_style.add_property("background-image", url.c_str(), nullptr, false, this);
	}
	str = get_attr("align");
	if(str)
	{
		m_style.add_property("text-align", str, nullptr, false, this);
	}

	str = get_attr("bgcolor");
	if (str)
	{
		m_style.add_property("background-color", str, nullptr, false, this);
	}

	str = get_attr("valign");
	if(str)
	{
		m_style.add_property("vertical-align", str, nullptr, false, this);
	}
	html_tag::parse_attributes();
}
