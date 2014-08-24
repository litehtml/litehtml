#include "html.h"
#include "el_base.h"
#include "document.h"

litehtml::el_base::el_base( litehtml::document* doc ) : html_tag(doc)
{
	
}

litehtml::el_base::~el_base()
{

}

void litehtml::el_base::parse_attributes()
{
	m_doc->container()->set_base_url(get_attr(_t("href")));
}
