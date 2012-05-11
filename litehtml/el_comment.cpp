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
