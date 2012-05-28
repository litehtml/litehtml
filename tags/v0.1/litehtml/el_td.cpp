#include "html.h"
#include "el_td.h"


litehtml::el_td::el_td( litehtml::document* doc ) : element(doc)
{

}

litehtml::el_td::~el_td()
{

}

void litehtml::el_td::parse_styles(bool is_reparse)
{
	const wchar_t* str_width = get_attr(L"width");
	if(str_width)
	{
		m_style.add_property(L"width", str_width, 0);
	}

	element::parse_styles(is_reparse);
}
