#include "html.h"
#include "context.h"
#include "stylesheet.h"


void litehtml::context::load_master_stylesheet( const wchar_t* str )
{
	parse_stylesheet(str, m_master_css, NULL);
}
