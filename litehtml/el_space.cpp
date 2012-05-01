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
	uint_ptr font = m_parent->get_font();
	sz.height	= m_doc->get_painter()->line_height(hdc, font);
	sz.width	= m_doc->get_painter()->text_width(hdc, L" ", font);
}

bool litehtml::el_space::is_white_space()
{
	return true;
}

void litehtml::el_space::draw_content( uint_ptr hdc, const litehtml::position& pos )
{
	uint_ptr font = m_parent->get_font();
	litehtml::web_color color = m_parent->get_color(L"color", true, m_doc->get_def_color());

	m_doc->get_painter()->draw_text(hdc, L" ", font, color, pos);
}
