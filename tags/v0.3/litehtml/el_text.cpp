#include "html.h"
#include "el_text.h"
#include "document.h"

litehtml::el_text::el_text( const tchar_t* text, litehtml::document* doc ) : element(doc)
{
	if(text)
	{
		m_text = text;
	}
	m_display			= display_inline_text;
	m_text_transform	= text_transform_none;
}

litehtml::el_text::~el_text()
{

}

void litehtml::el_text::get_content_size( size& sz, int max_width )
{
	sz = m_size;
}

void litehtml::el_text::draw_content( uint_ptr hdc, const litehtml::position& pos )
{
	uint_ptr font = m_parent->get_font();
	litehtml::web_color color = m_parent->get_color(_t("color"), true, m_doc->get_def_color());
	m_doc->container()->draw_text(hdc, m_transformed_text.c_str(), font, color, pos);
}

void litehtml::el_text::apply_stylesheet( const litehtml::css& stylesheet )
{

}

void litehtml::el_text::get_text( tstring& text )
{
	text += m_text;
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
	m_white_space		= m_parent->get_white_space();
	m_transformed_text	= m_text;
	if(m_text_transform != text_transform_none)
	{
		switch(m_text_transform)
		{
		case text_transform_capitalize:
			if(!m_transformed_text.empty())
			{
				m_transformed_text[0] = m_doc->container()->toupper(m_transformed_text[0]);
			}
			break;
		case text_transform_uppercase:
			m_transformed_text = _t("");
			for(int i=0; i < m_text.length(); i++)
			{
				m_transformed_text += m_doc->container()->toupper(m_text[i]);
			}
			break;
		case text_transform_lowercase:
			m_transformed_text = _t("");
			for(int i=0; i < m_text.length(); i++)
			{
				m_transformed_text += m_doc->container()->tolower(m_text[i]);
			}
			break;
		}
	}

	if(is_white_space())
	{
		m_transformed_text = _t(" ");
	} else
	{
		if(m_text == _t("\t"))
		{
			m_transformed_text = _t("    ");
		}
		if(m_text == _t("\n") || m_text == _t("\r"))
		{
			m_transformed_text = _t("");
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
		m_size.width	= m_doc->container()->text_width(m_transformed_text.c_str(), font);
	}

	if(m_parent->get_display() == display_inline)
	{
		if(m_parent->is_first_child(this))
		{
			m_margins.left	= m_parent->margin_left();
			m_padding.left	= m_parent->padding_left() + m_parent->border_left();
		}
		if(m_parent->is_last_child(this))
		{
			m_margins.right = m_parent->margin_right();
			m_padding.right	= m_parent->padding_right() + m_parent->border_right();
		}
	}
}

int litehtml::el_text::get_base_line()
{
	return m_parent->get_base_line();
}

void litehtml::el_text::draw( uint_ptr hdc, int x, int y, const position* clip )
{
	position pos = m_pos;
	pos.x	+= x;
	pos.y	+= y;

	if(pos.does_intersect(clip))
	{
		draw_content(hdc, pos);
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