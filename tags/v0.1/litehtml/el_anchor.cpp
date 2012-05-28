#include "html.h"
#include "el_anchor.h"
#include "document.h"

litehtml::el_anchor::el_anchor( litehtml::document* doc ) : element(doc)
{
	m_pseudo_classes.push_back(L"link");
}

litehtml::el_anchor::~el_anchor()
{

}

void litehtml::el_anchor::on_click( int x, int y )
{
	m_doc->container()->on_anchor_click(get_attr(L"href", L""), this);
}
