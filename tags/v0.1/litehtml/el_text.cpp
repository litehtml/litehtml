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
	sz.height	= m_doc->container()->line_height(hdc, font);
	sz.width	= m_doc->container()->text_width(hdc, m_text.c_str(), font);
}

void litehtml::el_text::draw_content( uint_ptr hdc, const litehtml::position& pos )
{
	uint_ptr font = m_parent->get_font();
	litehtml::web_color color = m_parent->get_color(L"color", true, m_doc->get_def_color());

	m_doc->container()->draw_text(hdc, m_text.c_str(), font, color, pos);
}

void litehtml::el_text::apply_stylesheet( const litehtml::style_sheet& style )
{

}

void litehtml::el_text::get_text( std::wstring& text )
{
	text += m_text;
}

const wchar_t* litehtml::el_text::get_style_property( const wchar_t* name, bool inherited, const wchar_t* def /*= 0*/ )
{
	if(inherited)
	{
		return m_parent->get_style_property(name, inherited, def);
	}
	return def;
}

void litehtml::el_text::parse_styles()
{
}
