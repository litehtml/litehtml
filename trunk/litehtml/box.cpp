#include "html.h"
#include "box.h"
#include "html_tag.h"


litehtml::box_type litehtml::block_box::get_type()
{
	return box_block;
}

int litehtml::block_box::height()
{
	return m_element->height();
}

int litehtml::block_box::width()
{
	return m_element->width();
}

void litehtml::block_box::add_element( element* el)
{
	m_element = el;
	el->m_box = this;
}

void litehtml::block_box::finish(bool last_box)
{
	if(!m_element) return;


	//m_element->m_pos.x = m_element->content_margins_left();
	//m_element->m_pos.y = m_box_top + m_element->content_margins_top();
}

bool litehtml::block_box::can_hold( element* el, white_space ws )
{
	if(m_element || el->is_inline_box())
	{
		return false;
	}
	return true;
}

bool litehtml::block_box::is_empty()
{
	if(m_element)
	{
		return false;
	}
	return true;
}

int litehtml::block_box::baseline()
{
	if(m_element)
	{
		return m_element->get_base_line();
	}
	return 0;
}

void litehtml::block_box::get_elements( elements_vector& els )
{
	els.push_back(m_element);
}

int litehtml::block_box::top_margin()
{
	if(m_element && m_element->collapse_top_margin())
	{
		return m_element->m_margins.top;
	}
	return 0;
}

int litehtml::block_box::bottom_margin()
{
	if(m_element && m_element->collapse_bottom_margin())
	{
		return m_element->m_margins.bottom;
	}
	return 0;
}

void litehtml::block_box::y_shift( int shift )
{
	m_box_top += shift;
	if(m_element)
	{
		m_element->m_pos.y += shift;
	}
}

void litehtml::block_box::new_width( int left, int right, elements_vector& els )
{

}
//////////////////////////////////////////////////////////////////////////

litehtml::box_type litehtml::line_box::get_type()
{
	return box_line;
}

int litehtml::line_box::height()
{
	return m_height;
}

int litehtml::line_box::width()
{
	return m_width;
}

void litehtml::line_box::add_element( element* el )
{
	el->m_skip	= false;
	el->m_box	= 0;
	bool add	= true;
	if( (m_items.empty() && el->is_white_space()) || el->is_break() )
	{
		el->m_skip = true;
	} else if(el->is_white_space())
	{
		element* ws = get_last_space();
		if(ws)
		{
			add = false;
			el->m_skip = true;
		}
	}

	if(add)
	{
		el->m_box = this;
		m_items.push_back(el);

		if(!el->m_skip)
		{
			el->m_pos.x	= m_box_left + m_width + el->content_margins_left();
			el->m_pos.y	= m_box_top;
			m_width		+= el->width();
		}
	}
}

void litehtml::line_box::finish(bool last_box)
{
	if( is_empty() || (!is_empty() && last_box && is_break_only()) )
	{
		m_height = 0;
		return;
	}

	for(std::vector<element*>::reverse_iterator i = m_items.rbegin(); i != m_items.rend(); i++)
	{
		if((*i)->is_white_space() || (*i)->is_break())
		{
			if(!(*i)->m_skip)
			{
				(*i)->m_skip = true;
				m_width -= (*i)->width();
			}
		} else
		{
			break;
		}
	}

	int base_line	= m_font_metrics.base_line();
	int line_height = m_line_height;

	int add_x = 0;
	switch(m_text_align)
	{
	case text_align_right:
		if(m_width < (m_box_right - m_box_left))
		{
			add_x = (m_box_right - m_box_left) - m_width;
		}
		break;
	case text_align_center:
		if(m_width < (m_box_right - m_box_left))
		{
			add_x = ((m_box_right - m_box_left) - m_width) / 2;
		}
		break;
	default:
		add_x = 0;
	}

	m_height = 0;
	// find line box baseline and line-height
	for(size_t i = 0; i < m_items.size(); i++)
	{
		if(m_items[i]->get_display() == display_inline_text)
		{
			font_metrics fm;
			m_items[i]->get_font(&fm);
			base_line	= std::max(base_line,	fm.base_line());
			line_height	= std::max(line_height,	m_items[i]->line_height());
			m_height = std::max(m_height, fm.height);
		}
		m_items[i]->m_pos.x += add_x;
	}

	base_line += (line_height - m_height) / 2;

	m_height = line_height;

	int y1	= 0;
	int y2	= m_height;

	for(size_t i = 0; i < m_items.size(); i++)
	{
		if(m_items[i]->get_display() == display_inline_text)
		{
			font_metrics fm;
			m_items[i]->get_font(&fm);
			m_items[i]->m_pos.y = m_height - base_line - fm.ascent;
		} else
		{
			switch(m_items[i]->get_vertical_align())
			{
			case va_super:
			case va_sub:
			case va_baseline:
				m_items[i]->m_pos.y = m_height - base_line - m_items[i]->height() + m_items[i]->get_base_line() + m_items[i]->content_margins_top();
				break;
			case va_top:
				m_items[i]->m_pos.y = y1 + m_items[i]->content_margins_top();
				break;
			case va_text_top:
				m_items[i]->m_pos.y = m_height - base_line - m_font_metrics.ascent + m_items[i]->content_margins_top();
				break;
			case va_middle:
				m_items[i]->m_pos.y = m_height - base_line - m_font_metrics.x_height / 2 - m_items[i]->height() / 2 + m_items[i]->content_margins_top();
				break;
			case va_bottom:
				m_items[i]->m_pos.y = y2 - m_items[i]->height() + m_items[i]->content_margins_top();
				break;
			case va_text_bottom:
				m_items[i]->m_pos.y = m_height - base_line + m_font_metrics.descent - m_items[i]->height() + m_items[i]->content_margins_top();
				break;
			}
			y1 = std::min(y1, m_items[i]->top());
			y2 = std::max(y2, m_items[i]->bottom());
		}
	}

	for(size_t i = 0; i < m_items.size(); i++)
	{
		m_items[i]->m_pos.y -= y1;
		m_items[i]->m_pos.y += m_box_top;
		if(m_items[i]->get_display() != display_inline_text)
		{
			switch(m_items[i]->get_vertical_align())
			{
			case va_top:
				m_items[i]->m_pos.y = m_box_top + m_items[i]->content_margins_top();
				break;
			case va_bottom:
				m_items[i]->m_pos.y = m_box_top + (y2 - y1) - m_items[i]->height() + m_items[i]->content_margins_top();
				break;
			case va_baseline:
				//TODO: process vertical align "baseline"
				break;
			case va_middle:
				//TODO: process vertical align "middle"
				break;
			case va_sub:
				//TODO: process vertical align "sub"
				break;
			case va_super:
				//TODO: process vertical align "super"
				break;
			case va_text_bottom:
				//TODO: process vertical align "text-bottom"
				break;
			case va_text_top:
				//TODO: process vertical align "text-top"
				break;
			}
		}
	}
	m_height = y2 - y1;
	m_baseline = (base_line - y1) - (m_height - line_height);
}

bool litehtml::line_box::can_hold( element* el, white_space ws )
{
	if(!el->is_inline_box()) return false;

	if(el->is_break())
	{
		return false;
	}

	if(ws == white_space_nowrap || ws == white_space_pre)
	{
		return true;
	}

	if(m_box_left + m_width + el->width() > m_box_right)
	{
		return false;
	}

	return true;
}

litehtml::element* litehtml::line_box::get_last_space()
{
	element* ret = 0;
	for(std::vector<element*>::reverse_iterator i = m_items.rbegin(); i != m_items.rend() && !ret; i++)
	{
		if((*i)->is_white_space() || (*i)->is_break())
		{
			ret = (*i);
		} else
		{
			break;
		}
	}
	return ret;
}

bool litehtml::line_box::is_empty()
{
	if(m_items.empty()) return true;
	for(std::vector<element*>::reverse_iterator i = m_items.rbegin(); i != m_items.rend(); i++)
	{
		if(!(*i)->m_skip || (*i)->is_break())
		{
			return false;
		}
	}
	return true;
}

int litehtml::line_box::baseline()
{
	return m_baseline;
}

void litehtml::line_box::get_elements( elements_vector& els )
{
	els.insert(els.begin(), m_items.begin(), m_items.end());
}

int litehtml::line_box::top_margin()
{
	return 0;
}

int litehtml::line_box::bottom_margin()
{
	return 0;
}

void litehtml::line_box::y_shift( int shift )
{
	m_box_top += shift;
	for(std::vector<element*>::iterator i = m_items.begin(); i != m_items.end(); i++)
	{
		(*i)->m_pos.y += shift;
	}
}

bool litehtml::line_box::is_break_only()
{
	if(m_items.empty()) return true;

	if(m_items.front()->is_break())
	{
		for(std::vector<element*>::iterator i = m_items.begin() + 1; i != m_items.end(); i++)
		{
			if(!(*i)->m_skip)
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

void litehtml::line_box::new_width( int left, int right, elements_vector& els )
{
	int add = left - m_box_left;
	if(add)
	{
		m_box_left	= left;
		m_box_right	= right;
		m_width = 0;
		std::vector<element*>::iterator remove_begin = m_items.end();
		for(std::vector<element*>::iterator i = m_items.begin() + 1; i != m_items.end(); i++)
		{
			if(!(*i)->m_skip)
			{
				if(m_box_left + m_width + (*i)->width() > m_box_right)
				{
					remove_begin = i;
					break;
				} else
				{
					(*i)->m_pos.x += add;
					m_width += (*i)->width();
				}
			}
		}
		if(remove_begin != m_items.end())
		{
			els.insert(els.begin(), remove_begin, m_items.end());
			m_items.erase(remove_begin, m_items.end());
		}
	}
}
