#include "html.h"
#include "el_anchor.h"
#include "document.h"

litehtml::el_anchor::el_anchor( litehtml::document* doc ) : html_tag(doc)
{
}

litehtml::el_anchor::~el_anchor()
{

}

void litehtml::el_anchor::on_click( int x, int y )
{
	const tchar_t* href = get_attr(_t("href"));

	if(href)
	{
		m_doc->container()->on_anchor_click(href, this);
	}
}

void litehtml::el_anchor::apply_stylesheet( const litehtml::css& stylesheet )
{
	if( get_attr(_t("href")) )
	{
		m_pseudo_classes.push_back(_t("link"));
	}
	html_tag::apply_stylesheet(stylesheet);
}
