#include "html.h"
#include "document.h"
#include "el_space.h"

litehtml::el_space::el_space( const wchar_t* text, litehtml::document* doc ) : el_text(text, doc)
{
}

litehtml::el_space::~el_space()
{

}

bool litehtml::el_space::is_white_space()
{
	if(	m_white_space == white_space_normal || 
		m_white_space == white_space_nowrap ||
		m_white_space == white_space_pre_line )
	{
		return true;
	}
	return false;
}

bool litehtml::el_space::is_break() const
{
	if(	m_white_space == white_space_pre ||
		m_white_space == white_space_pre_line ||
		m_white_space == white_space_pre_wrap)
	{
		if(m_text == L"\n")
		{
			return true;
		}
	}
	return false;
}
