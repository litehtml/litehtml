#include "html.h"
#include "context.h"
#include "stylesheet.h"


void litehtml::context::load_master_stylesheet( const tchar_t* str )
{
	m_master_css.parse_stylesheet(str, 0, 0);
	m_master_css.sort_selectors();
}
