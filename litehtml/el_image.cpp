#include "html.h"
#include "el_image.h"
#include "document.h"

litehtml::el_image::el_image( litehtml::document* doc ) : element(doc)
{

}

litehtml::el_image::~el_image( void )
{

}

void litehtml::el_image::get_content_size( size& sz, int max_width )
{
	m_doc->container()->get_image_size(m_src.c_str(), 0, sz);
}

void litehtml::el_image::draw_content( uint_ptr hdc, const litehtml::position& pos )
{
	m_doc->container()->draw_image(hdc, m_src.c_str(), 0, pos);
}

void litehtml::el_image::parse_styles(bool is_reparse)
{
	element::parse_styles(is_reparse);
	m_src = get_attr(_t("src"), _t(""));
	m_doc->container()->load_image(m_src.c_str(), NULL);

	if(!m_css_height.val())
	{
		m_css_height.fromString(get_attr(_t("height"), _t("auto")), _t("auto"));
	}
	if(!m_css_width.val())
	{
		m_css_width.fromString(get_attr(_t("width"), _t("auto")), _t("auto"));
	}
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
	m_doc->container()->load_image(m_src.c_str(), NULL);

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
	} else if(!m_css_height.is_predefined() && m_css_width.is_predefined())
	{
		m_pos.height = (int) m_css_height.val();
		if(sz.height)
		{
			m_pos.width = (int) (m_css_height.val() * (float)sz.width / (float)sz.height);
		} else
		{
			m_pos.width = sz.width;
		}
	} else if(m_css_height.is_predefined() && !m_css_width.is_predefined())
	{
		m_pos.width = (int) m_css_width.val();
		if(sz.width)
		{
			m_pos.height = (int) ((float) m_css_width.val() * (float) sz.height / (float)sz.width);
		} else
		{
			m_pos.height = sz.height;
		}
	}

	if(!m_css_max_width.is_predefined())
	{
		int max_width = m_doc->cvt_units(m_css_max_width, m_font_size, parent_width);
		if(m_pos.width > max_width)
		{
			m_pos.width = max_width;
		}
	}
	if(!m_css_max_height.is_predefined())
	{
		int max_height = m_doc->cvt_units(m_css_max_height, m_font_size);
		if(m_pos.height > max_height)
		{
			m_pos.height = max_height;
		}
	}

	calc_outlines(parent_width);

	m_pos.x	+= content_margins_left();
	m_pos.y += content_margins_top();

	return m_pos.width + content_margins_left() + content_margins_right();
}
