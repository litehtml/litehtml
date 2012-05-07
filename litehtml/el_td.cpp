#include "html.h"
#include "el_td.h"


litehtml::el_td::el_td( litehtml::document* doc ) : element(doc)
{

}

litehtml::el_td::~el_td()
{

}

void litehtml::el_td::parse_styles()
{
	css_length width;
	const wchar_t* str_width = get_attr(L"width", L"auto");
	width.fromString(str_width, L"auto");
	if(!width.is_predefined())
	{
		m_style.add_property(L"width", str_width, 0);
	}

	element::parse_styles();
}
