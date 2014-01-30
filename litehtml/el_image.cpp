#include "html.h"
#include "el_image.h"
#include "document.h"

litehtml::el_image::el_image( litehtml::document* doc ) : html_tag(doc)
{
	m_display = display_inline_block;
}

litehtml::el_image::~el_image( void )
{

}

void litehtml::el_image::get_content_size( size& sz, int max_width )
{
	m_doc->container()->get_image_size(m_src.c_str(), 0, sz);
}

int litehtml::el_image::line_height() const
{
	return height();
}

bool litehtml::el_image::is_replaced() const
{
	return true;
}

int litehtml::el_image::render( int x, int y, int max_width )
{
	int parent_width = max_width;

	// restore margins after collapse
	m_margins.top		= m_doc->cvt_units(m_css_margins.top,		m_font_size);
	m_margins.bottom	= m_doc->cvt_units(m_css_margins.bottom,	m_font_size);

	m_pos.move_to(x, y);
	if(m_el_position == element_position_relative)
	{
		m_pos.x += m_css_left.calc_percent(parent_width);
	}

	litehtml::size sz;
	m_doc->container()->get_image_size(m_src.c_str(), 0, sz);

	m_pos.width		= sz.width;
	m_pos.height	= sz.height;

	if(m_css_height.is_predefined() && m_css_width.is_predefined())
	{
		m_pos.height	= sz.height;
		m_pos.width		= sz.width;

		// check for max-height
		if(!m_css_max_width.is_predefined())
		{
			int max_width = m_doc->cvt_units(m_css_max_width, m_font_size, parent_width);
			if(m_pos.width > max_width)
			{
				m_pos.width = max_width;
			}
			if(sz.width)
			{
				m_pos.height = (int) ((float) m_pos.width * (float) sz.height / (float)sz.width);
			} else
			{
				m_pos.height = sz.height;
			}
		}

		// check for max-height
		if(!m_css_max_height.is_predefined())
		{
			int max_height = m_doc->cvt_units(m_css_max_height, m_font_size);
			if(m_pos.height > max_height)
			{
				m_pos.height = max_height;
			}
			if(sz.height)
			{
				m_pos.width = (int) (m_pos.height * (float)sz.width / (float)sz.height);
			} else
			{
				m_pos.width = sz.width;
			}
		}
	} else if(!m_css_height.is_predefined() && m_css_width.is_predefined())
	{
		m_pos.height = (int) m_css_height.val();

		// check for max-height
		if(!m_css_max_height.is_predefined())
		{
			int max_height = m_doc->cvt_units(m_css_max_height, m_font_size);
			if(m_pos.height > max_height)
			{
				m_pos.height = max_height;
			}
		}

		if(sz.height)
		{
			m_pos.width = (int) (m_pos.height * (float)sz.width / (float)sz.height);
		} else
		{
			m_pos.width = sz.width;
		}
	} else if(m_css_height.is_predefined() && !m_css_width.is_predefined())
	{
		m_pos.width = (int) m_css_width.calc_percent(parent_width);

		// check for max-width
		if(!m_css_max_width.is_predefined())
		{
			int max_width = m_doc->cvt_units(m_css_max_width, m_font_size, parent_width);
			if(m_pos.width > max_width)
			{
				m_pos.width = max_width;
			}
		}

		if(sz.width)
		{
			m_pos.height = (int) ((float) m_pos.width * (float) sz.height / (float)sz.width);
		} else
		{
			m_pos.height = sz.height;
		}
	} else
	{
		m_pos.width		= (int) m_css_width.calc_percent(parent_width);
		m_pos.height	= (int) m_css_height.val();

		// check for max-height
		if(!m_css_max_height.is_predefined())
		{
			int max_height = m_doc->cvt_units(m_css_max_height, m_font_size);
			if(m_pos.height > max_height)
			{
				m_pos.height = max_height;
			}
		}

		// check for max-height
		if(!m_css_max_width.is_predefined())
		{
			int max_width = m_doc->cvt_units(m_css_max_width, m_font_size, parent_width);
			if(m_pos.width > max_width)
			{
				m_pos.width = max_width;
			}
		}
	}

	calc_outlines(parent_width);

	m_pos.x	+= content_margins_left();
	m_pos.y += content_margins_top();

	return m_pos.width + content_margins_left() + content_margins_right();
}

void litehtml::el_image::parse_attributes()
{
	m_src = get_attr(_t("src"), _t(""));

	const tchar_t* attr_height = get_attr(_t("height"));
	if(attr_height)
	{
		m_style.add_property(_t("height"), attr_height, 0, false);
	}
	const tchar_t* attr_width = get_attr(_t("width"));
	if(attr_width)
	{
		m_style.add_property(_t("width"), attr_width, 0, false);
	}
}

void litehtml::el_image::draw( uint_ptr hdc, int x, int y, const position* clip )
{
	position pos = m_pos;
	pos.x	+= x;
	pos.y	+= y;

	draw_background(hdc, x, y, clip);

	if(pos.does_intersect(clip))
	{
		background_paint bg;
		bg.image				= m_src;
		bg.clip_box				= pos;
		bg.origin_box			= pos;
		bg.border_box			= pos;
		bg.border_box			+= m_padding;
		bg.border_box			+= m_borders;
		bg.repeat				= background_repeat_no_repeat;
		bg.image_size.width		= pos.width;
		bg.image_size.height	= pos.height;
		bg.border_radius		= m_css_borders.radius;
		bg.position_x			= pos.x;
		bg.position_y			= pos.y;
		m_doc->container()->draw_background(hdc, bg);
	}
}

void litehtml::el_image::parse_styles( bool is_reparse /*= false*/ )
{
	html_tag::parse_styles(is_reparse);

	if(!m_src.empty())
	{
		if(!m_css_height.is_predefined() && !m_css_width.is_predefined())
		{
			m_doc->container()->load_image(m_src.c_str(), 0, true);
		} else
		{
			m_doc->container()->load_image(m_src.c_str(), 0, false);
		}
	}
}
