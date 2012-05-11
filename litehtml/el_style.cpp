#include "html.h"
#include "el_style.h"
#include "document.h"


litehtml::el_style::el_style( litehtml::document* doc ) : litehtml::element(doc)
{

}

litehtml::el_style::~el_style()
{

}

void litehtml::el_style::finish()
{
	std::wstring text;
	get_text(text);
	m_doc->add_stylesheet(text.c_str(), NULL);
}
