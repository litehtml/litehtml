#include "html.h"
#include "element.h"
#include "line.h"


void litehtml::line::operator+=( element* el )
{
	m_items.push_back(el);
	if(el->m_float == float_none)
	{
		if(!el->is_white_space() || !m_last_white_space)
		{
			m_height			= max(el->height(), m_height);
			m_padding_bottom	= max(m_padding_bottom, el->m_padding.bottom + el->m_borders.bottom);
			m_padding_top		= max(m_padding_top, el->m_padding.top + el->m_borders.top);
			m_top_margin		= max(el->margin_top(), m_top_margin);
			m_bottom_margin		= max(el->margin_bottom(), m_bottom_margin);
			m_left += el->width();
			if(el->m_display == display_block)
			{
				m_is_block	= true;
				m_clear		= el->m_clear;
			} else
			{
				m_is_block = false;
			}
			m_last_white_space = el->is_white_space();
			el->m_skip = false;
		} else
		{
			el->m_skip = true;
		}
	}
}

void litehtml::line::set_top( int top, element* parent )
{
	m_top	= top;
	int parent_base_line = parent ? parent->get_base_line() : 0;

	int add = 0;
	if(m_min_height > m_height)
	{
		add = (m_min_height - m_height) / 2;
	}

	for(elements_vector::iterator i = m_items.begin(); i != m_items.end(); i++)
	{
		object_ptr<element> el = (*i);
		if(el->m_float == float_none)
		{
			switch(el->m_vertical_align)
			{
			case va_baseline:
				if(!is_block())
				{
					el->m_pos.y = top + m_height - m_bottom_margin - parent_base_line - m_padding_bottom - el->m_pos.height + el->get_base_line() - m_top_margin;
					el->m_pos.y += add;
				} else
				{
					el->m_pos.y = top + el->content_margins_top();
				}
				break;
			default:
				el->m_pos.y = top + el->content_margins_top();
				break;
			}
		} else
		{
			el->m_pos.y = m_top + el->content_margins_top();
		}
	}
}

void litehtml::line::get_elements( elements_vector& els )
{
	els.insert(els.begin(), m_items.begin(), m_items.end());
}

bool litehtml::line::finish(text_align align)
{
	m_height			= 0;
	m_padding_bottom	= 0;
	m_top_margin		= 0;
	m_bottom_margin		= 0;

	int add = 0;

	if(!is_block() && !empty() && align != text_align_left)
	{
		switch(align)
		{
		case text_align_right:
			add = line_right() - m_items.back()->right();
			break;
		case text_align_center:
			add = (line_right() - m_items.back()->right()) / 2;
			break;
		}
		if(add < 0) add = 0;
	}

	bool ret = false;
	for(elements_vector::reverse_iterator i = m_items.rbegin(); i!= m_items.rend(); i++)
	{
		object_ptr<element> el = (*i);

		el->m_pos.x += add;

		if(el->is_white_space() && !ret)
		{
			el->m_skip = true;
		} else
		{
			ret = true;
			m_height			= max(el->height(), m_height);
			m_padding_bottom	= max(m_padding_bottom, el->m_padding.bottom + el->m_borders.bottom);
			m_padding_top		= max(m_padding_top, el->m_padding.top + el->m_borders.top);
			m_top_margin		= max(el->margin_top(), m_top_margin);
			m_bottom_margin		= max(el->margin_bottom(), m_bottom_margin);
		}
	}
	return ret;
}

void litehtml::line::add_top( int add )
{
	m_top	+= add;

	for(elements_vector::iterator i = m_items.begin(); i != m_items.end(); i++)
	{
		object_ptr<element> el = (*i);
		el->m_pos.y += add;
	}
}

void litehtml::line::init( int left, int right, int top, int line_height )
{
	m_line_left		= left;
	m_left			= left;
	m_line_right	= right;
	m_min_height	= line_height;
	m_top			= top;
}

bool litehtml::line::have_room_for( element* el )
{
	if(m_left + el->width() > m_line_right)
	{
		return false;
	}
	return true;
}
