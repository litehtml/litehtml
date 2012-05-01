#include "html.h"
#include "el_text.h"
#include "document.h"

litehtml::el_text::el_text( const wchar_t* text, litehtml::document* doc ) : element(doc)
{
	if(text)
	{
		m_text = text;
	}
	set_attr(L"style", L"display:inline");
}

litehtml::el_text::~el_text()
{

}

void litehtml::el_text::get_content_size( uint_ptr hdc, size& sz, int max_width )
{
	uint_ptr font = m_parent->get_font();
	sz.height	= m_doc->get_painter()->line_height(hdc, font);
	sz.width	= m_doc->get_painter()->text_width(hdc, m_text.c_str(), font);
}

void litehtml::el_text::draw_content( uint_ptr hdc, const litehtml::position& pos )
{
	uint_ptr font = m_parent->get_font();
	litehtml::web_color color = m_parent->get_color(L"color", true, m_doc->get_def_color());

	m_doc->get_painter()->draw_text(hdc, m_text.c_str(), font, color, pos);
}

