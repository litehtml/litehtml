#include "html.h"
#include "el_base.h"
#include "document.h"

litehtml::el_base::el_base( litehtml::document* doc ) : element(doc)
{
	
}

litehtml::el_base::~el_base()
{

}

void litehtml::el_base::finish()
{
	m_doc->container()->set_base_url(get_attr(L"href"));
}
