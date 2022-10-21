#include "html.h"
#include "context.h"
#include "stylesheet.h"


void litehtml::context::load_master_stylesheet( const char* str )
{
	media_query_list::ptr media;

	m_master_css.parse_stylesheet(str, nullptr, std::shared_ptr<document>(), media_query_list::ptr());
	m_master_css.sort_selectors();
}
