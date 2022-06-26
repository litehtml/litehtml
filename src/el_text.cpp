#include "html.h"
#include "el_text.h"
#include "document.h"
#include "render_item.h"

litehtml::el_text::el_text(const tchar_t* text, const std::shared_ptr<litehtml::document>& doc) : element(doc)
{
	if(text)
	{
		m_text = text;
	}
	m_use_transformed	= false;
	m_draw_spaces		= true;
    css_w().set_display(display_inline_text);
}

void litehtml::el_text::get_content_size( size& sz, int max_width )
{
	sz = m_size;
}

void litehtml::el_text::get_text( tstring& text )
{
	text += m_text;
}

const litehtml::tchar_t* litehtml::el_text::get_style_property( const tchar_t* name, bool inherited, const tchar_t* def /*= 0*/ ) const
{
	if(inherited)
	{
		element::ptr el_parent = parent();
		if (el_parent)
		{
			return el_parent->get_style_property(name, inherited, def);
		}
	}
	return def;
}

void litehtml::el_text::parse_styles(bool is_reparse)
{
    element::ptr el_parent = parent();
    if (el_parent)
    {
        css_w().set_line_height(el_parent->css().get_line_height());
        css_w().set_font(el_parent->css().get_font());
        css_w().set_font_metrics(el_parent->css().get_font_metrics());
        css_w().set_white_space(el_parent->css().get_white_space());
    }
    css_w().set_display(display_inline_text);
    css_w().set_float(float_none);

	if(m_css.get_text_transform() != text_transform_none)
	{
		m_transformed_text	= m_text;
		m_use_transformed = true;
		get_document()->container()->transform_text(m_transformed_text, m_css.get_text_transform());
	}

    element::ptr p = parent();
    while(p && p->css().get_display() == display_inline)
    {
        if(p->css().get_position() == element_position_relative)
        {
            css_w().set_offsets(p->css().get_offsets());
            css_w().set_position(element_position_relative);
            break;
        }
        p = p->parent();
    }
    if(p)
    {
        css_w().set_position(element_position_static);
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
	uint_ptr font = 0;
	if (el_parent)
	{
		font = el_parent->css().get_font();
        fm = el_parent->css().get_font_metrics();
	}
	if(is_break())
	{
		m_size.height	= 0;
		m_size.width	= 0;
	} else
	{
		m_size.height	= fm.height;
		m_size.width	= get_document()->container()->text_width(m_use_transformed ? m_transformed_text.c_str() : m_text.c_str(), font);
	}
	m_draw_spaces = fm.draw_spaces;
}

void litehtml::el_text::draw(uint_ptr hdc, int x, int y, const position *clip, const std::shared_ptr<render_item> &ri)
{
	if(is_white_space() && !m_draw_spaces)
	{
		return;
	}

	position pos = ri->pos();
	pos.x	+= x;
	pos.y	+= y;

	if(pos.does_intersect(clip))
	{
		element::ptr el_parent = parent();
		if (el_parent)
		{
			document::ptr doc = get_document();

			uint_ptr font = el_parent->css().get_font();
			litehtml::web_color color = el_parent->get_color(_t("color"), true, doc->get_def_color());
			doc->container()->draw_text(hdc, m_use_transformed ? m_transformed_text.c_str() : m_text.c_str(), font, color, pos);
		}
	}
}

litehtml::tstring litehtml::el_text::dump_get_name()
{
    return _t("text: \"") + get_escaped_string(m_text) + _t("\"");
}

std::vector<std::tuple<litehtml::tstring, litehtml::tstring>> litehtml::el_text::dump_get_attrs()
{
    return std::vector<std::tuple<litehtml::tstring, litehtml::tstring>>();
}
