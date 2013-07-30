#include "html.h"
#include "el_para.h"
#include "document.h"

litehtml::el_para::el_para( litehtml::document* doc ) : litehtml::element(doc)
{
}

litehtml::el_para::~el_para()
{

}

void litehtml::el_para::parse_styles( bool is_reparse /*= false*/ )
{
	const tchar_t* str = get_attr(_t("align"));
	if(str)
	{
		m_style.add_property(_t("text-align"), str, 0, false);
	}

	element::parse_styles(is_reparse);
}
