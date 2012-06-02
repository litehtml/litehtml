#include "html.h"
#include "document.h"
#include "el_space.h"

litehtml::el_space::el_space( litehtml::document* doc ) : element(doc)
{
	set_attr(L"style", L"display:inline");
}

litehtml::el_space::~el_space()
{

}

void litehtml::el_space::get_content_size( uint_ptr hdc, size& sz, int max_width )
{
	sz = m_size;
}

bool litehtml::el_space::is_white_space()
{
	return true;
}

void litehtml::el_space::draw_content( uint_ptr hdc, const litehtml::position& pos )
{
	uint_ptr font = m_parent->get_font();
	litehtml::web_color color = m_parent->get_color(L"color", true, m_doc->get_def_color());

	m_doc->container()->draw_text(hdc, L" ", font, color, pos);
}

void litehtml::el_space::apply_stylesheet( const litehtml::style_sheet& style )
{

}

void litehtml::el_space::get_text( std::wstring& text )
{
	text += L" ";
}

const wchar_t* litehtml::el_space::get_style_property( const wchar_t* name, bool inherited, const wchar_t* def /*= 0*/ )
{
	if(inherited)
	{
		return m_parent->get_style_property(name, inherited, def);
	}
	return def;
}

void litehtml::el_space::parse_styles(bool is_reparse)
{
	uint_ptr hdc = m_doc->container()->get_temp_dc();
	uint_ptr font = m_parent->get_font();
	m_size.height	= m_doc->container()->line_height(hdc, font);
	m_size.width	= m_doc->container()->text_width(hdc, L" ", font);
	m_doc->container()->release_temp_dc(hdc);
}

int litehtml::el_space::get_base_line()
{
	return m_parent->get_base_line();
}
