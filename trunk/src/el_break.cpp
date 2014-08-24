#include "html.h"
#include "el_break.h"

litehtml::el_break::el_break( litehtml::document* doc ) : html_tag(doc)
{

}

litehtml::el_break::~el_break()
{

}

bool litehtml::el_break::is_break() const
{
	return true;
}

void litehtml::el_break::parse_attributes()
{
	const tchar_t* attr_clear = get_attr(_t("clear"));
	if(attr_clear)
	{
		m_style.add_property(_t("clear"), attr_clear, 0, false);
	}

	html_tag::parse_attributes();
}
