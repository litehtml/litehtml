#include "html.h"
#include "el_div.h"


litehtml::el_div::el_div( litehtml::document* doc ) : element(doc)
{

}

litehtml::el_div::~el_div()
{

}

void litehtml::el_div::finish()
{
	const tchar_t* str = get_attr(_t("align"));
	if(str)
	{
		m_style.add_property(_t("text-align"), str, 0, false);
	}
	element::finish();
}
