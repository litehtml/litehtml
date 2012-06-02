#include "html.h"
#include "el_comment.h"


litehtml::el_comment::el_comment( litehtml::document* doc ) : element(doc)
{
	m_skip = true;
}
litehtml::el_comment::~el_comment()
{

}

void litehtml::el_comment::get_text( std::wstring& text )
{
	text += m_text;
}

void litehtml::el_comment::set_data( const wchar_t* data )
{
	if(data)
	{
		m_text = data;
	}
}

void litehtml::el_comment::apply_stylesheet( const litehtml::style_sheet& style )
{

}

void litehtml::el_comment::parse_styles( bool is_reparse )
{
	m_display = display_none;
}

int litehtml::el_comment::get_base_line()
{
	return 0;
}