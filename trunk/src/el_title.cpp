#include "html.h"
#include "el_title.h"
#include "document.h"

litehtml::el_title::el_title( litehtml::document* doc ) : litehtml::html_tag(doc)
{

}

litehtml::el_title::~el_title()
{

}

void litehtml::el_title::parse_attributes()
{
	tstring text;
	get_text(text);
	m_doc->container()->set_caption(text.c_str());
}
