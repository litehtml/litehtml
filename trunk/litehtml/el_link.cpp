#include "html.h"
#include "el_link.h"
#include "document.h"


litehtml::el_link::el_link( litehtml::document* doc ) : litehtml::html_tag(doc)
{

}

litehtml::el_link::~el_link()
{

}

void litehtml::el_link::parse_attributes()
{
	m_doc->container()->link(m_doc, this);
}
