#include "html.h"
#include "el_div.h"


litehtml::el_div::el_div(const std::shared_ptr<document>& doc) : html_tag(doc)
{

}

void litehtml::el_div::parse_attributes()
{
	const char* str = get_attr("align");
	if(str)
	{
		m_style.add_property(_text_align_, str, 0, false, this);
	}
	html_tag::parse_attributes();
}
