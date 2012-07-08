#include "html.h"
#include "document.h"
#include "el_space.h"

litehtml::el_space::el_space( litehtml::document* doc ) : el_text(L" ", doc)
{
}

litehtml::el_space::~el_space()
{

}

bool litehtml::el_space::is_white_space()
{
	return true;
}
