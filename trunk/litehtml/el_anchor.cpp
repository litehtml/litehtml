#include "html.h"
#include "el_anchor.h"

litehtml::el_anchor::el_anchor( litehtml::document* doc ) : element(doc)
{
	m_pseudo_classes.push_back(L"link");
}

litehtml::el_anchor::~el_anchor()
{

}
