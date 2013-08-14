#include "html.h"
#include "el_style.h"
#include "document.h"


litehtml::el_style::el_style( litehtml::document* doc ) : litehtml::element(doc)
{

}

litehtml::el_style::~el_style()
{

}

void litehtml::el_style::parse_attributes()
{
	m_doc->add_stylesheet(m_text.c_str(), 0);
}

bool litehtml::el_style::appendChild( litehtml::element* el )
{
	el->get_text(m_text);
	return true;
}

const litehtml::tchar_t* litehtml::el_style::get_tagName() const
{
	return _t("style");
}
