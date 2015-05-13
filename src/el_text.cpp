#include "html.h"
#include "el_text.h"
#include "document.h"

litehtml::el_text::el_text( const tchar_t* text, litehtml::document* doc ) : element(doc)
{
	if(text)
	{
		m_text = text;
	}
	m_text_transform	= text_transform_none;
	m_use_transformed	= false;
	m_draw_spaces		= true;
}

litehtml::el_text::~el_text()
{

}

void litehtml::el_text::get_content_size( size& sz, int max_width )
{
	sz = m_size;
}

void litehtml::el_text::get_text( tstring& text )
{
	text += m_text;
}

void litehtml::el_text::set_text( const tstring& text )
{
    m_text = text;
    parse_styles( true );
}

const litehtml::tchar_t* litehtml::el_text::get_style_property( const tchar_t* name, bool inherited, const tchar_t* def /*= 0*/ )
{
	if(inherited)
	{
		return m_parent->get_style_property(name, inherited, def);
	}
	return def;
}

void litehtml::el_text::parse_styles(bool is_reparse)
{
	m_text_transform	= (text_transform)	value_index(get_style_property(_t("text-transform"), true,	_t("none")),	text_transform_strings,	text_transform_none);
	if(m_text_transform != text_transform_none)
	{
		m_transformed_text	= m_text;
		m_use_transformed = true;
		m_doc->container()->transform_text(m_transformed_text, m_text_transform);
	}

	if(is_white_space())
	{
		m_transformed_text = _t(" ");
		m_use_transformed = true;
	} else
	{
		if(m_text == _t("\t"))
		{
			m_transformed_text = _t("    ");
			m_use_transformed = true;
		}
		if(m_text == _t("\n") || m_text == _t("\r"))
		{
			m_transformed_text = _t("");
			m_use_transformed = true;
		}
	}

	font_metrics fm;
	uint_ptr font	= m_parent->get_font(&fm);
	if(is_break())
	{
		m_size.height	= 0;
		m_size.width	= 0;
	} else
	{
		m_size.height	= fm.height;
		m_size.width	= m_doc->container()->text_width(m_use_transformed ? m_transformed_text.c_str() : m_text.c_str(), font);
	}
	m_draw_spaces = fm.draw_spaces;
}

int litehtml::el_text::get_base_line()
{
	return m_parent->get_base_line();
}

void litehtml::el_text::draw( uint_ptr hdc, int x, int y, const position* clip )
{
	if(is_white_space() && !m_draw_spaces)
	{
		return;
	}

	position pos = m_pos;
	pos.x	+= x;
	pos.y	+= y;

	if(pos.does_intersect(clip))
	{
		uint_ptr font = m_parent->get_font();
		litehtml::web_color color = m_parent->get_color(_t("color"), true, m_doc->get_def_color());
		m_doc->container()->draw_text(hdc, m_use_transformed ? m_transformed_text.c_str() : m_text.c_str(), font, color, pos);
	}
}

int litehtml::el_text::line_height() const
{
	return m_parent->line_height();
}

litehtml::uint_ptr litehtml::el_text::get_font( font_metrics* fm /*= 0*/ )
{
	return m_parent->get_font(fm);
}

litehtml::style_display litehtml::el_text::get_display() const
{
	return display_inline_text;
}

litehtml::white_space litehtml::el_text::get_white_space() const
{
	if(m_parent) return m_parent->get_white_space();
	return white_space_normal;
}

litehtml::element_position litehtml::el_text::get_element_position(css_offsets* offsets) const
{
	element* p = parent();
	while(p && p->get_display() == display_inline)
	{
		if(p->get_element_position() == element_position_relative)
		{
			if(offsets)
			{
				*offsets = p->get_css_offsets();
			}
			return element_position_relative;
		}
		p = p->parent();
	}
	return element_position_static;
}

litehtml::css_offsets litehtml::el_text::get_css_offsets() const
{
	element* p = parent();
	while(p && p->get_display() == display_inline)
	{
		if(p->get_element_position() == element_position_relative)
		{
			return p->get_css_offsets();
		}
		p = p->parent();
	}
	return css_offsets();
}
