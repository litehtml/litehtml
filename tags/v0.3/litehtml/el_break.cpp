#include "html.h"
#include "el_break.h"

litehtml::el_break::el_break( litehtml::document* doc ) : element(doc)
{

}

litehtml::el_break::~el_break()
{

}

bool litehtml::el_break::is_break() const
{
	return true;
}
