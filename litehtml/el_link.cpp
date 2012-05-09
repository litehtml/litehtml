#include "html.h"
#include "el_link.h"
#include "document.h"


litehtml::el_link::el_link( litehtml::document* doc ) : litehtml::element(doc)
{

}

litehtml::el_link::~el_link()
{

}

void litehtml::el_link::finish()
{
	m_doc->container()->link(get_attr(L"href"), get_attr(L"type"), get_attr(L"rel"));
}
