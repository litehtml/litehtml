#include "html.h"
#include "el_div.h"


litehtml::el_div::el_div( litehtml::document* doc ) : element(doc)
{

}

litehtml::el_div::~el_div()
{

}

void litehtml::el_div::parse_styles(bool is_reparse)
{
	const wchar_t* str = get_attr(L"align");
	if(str)
	{
		m_style.add_property(L"text-align", str, 0, false);
	}

	element::parse_styles(is_reparse);
}
