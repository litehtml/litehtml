#include "html.h"
#include "line_box.h"
#include "element.h"
#include "render_item.h"
#include <algorithm>

//////////////////////////////////////////////////////////////////////////////////////////

void litehtml::line_box_item::place_to(int x, int y)
{
	m_element->pos().x = x + m_element->content_margins_left();
	m_element->pos().y = y + m_element->content_margins_top();
}

litehtml::position& litehtml::line_box_item::pos()
{
	return m_element->pos();
}


int litehtml::line_box_item::width() const
{
	return m_element->width();
}

int litehtml::line_box_item::top() const
{
	return m_element->top();
}

int litehtml::line_box_item::bottom() const
{
	return m_element->bottom();
}

int litehtml::line_box_item::right() const
{
	return m_element->right();
}

//////////////////////////////////////////////////////////////////////////////////////////

litehtml::lbi_start::lbi_start(const std::shared_ptr<render_item>& element) : line_box_item(element)
{
	m_pos.height = m_element->src_el()->css().get_line_height();
	m_pos.width = m_element->content_margins_left();
}

void litehtml::lbi_start::place_to(int x, int y)
{
	m_pos.x = x + m_element->content_margins_left();
	m_pos.y = y + m_element->content_margins_top();
}

int litehtml::lbi_start::width() const
{
	return m_pos.width;
}

int litehtml::lbi_start::top() const
{
	return m_pos.y - m_element->content_margins_top();
}

int litehtml::lbi_start::bottom() const
{
	return m_pos.y + m_element->pos().height + m_element->content_margins_top() + m_element->content_margins_bottom();
}

int litehtml::lbi_start::right() const
{
	return m_pos.x;
}

//////////////////////////////////////////////////////////////////////////////////////////

litehtml::lbi_end::lbi_end(const std::shared_ptr<render_item>& element) : lbi_start(element)
{
	m_pos.height = m_element->src_el()->css().get_line_height();
	m_pos.width = m_element->content_margins_right();
}

void litehtml::lbi_end::place_to(int x, int y)
{
	m_pos.x = x;
	m_pos.y = y;
}

int litehtml::lbi_end::right() const
{
	return m_pos.x + m_pos.width;
}

//////////////////////////////////////////////////////////////////////////////////////////

void litehtml::line_box::add_item(std::unique_ptr<line_box_item> item)
{
    item->get_el()->skip(false);
    bool add	= true;
	switch (item->get_type())
	{
		case line_box_item::type_text_part:
			if(item->get_el()->src_el()->is_white_space())
			{
				add = !m_items.empty() && !have_last_space();
			}
			break;
		case line_box_item::type_inline_start:
		case line_box_item::type_inline_end:
			add = true;
			break;
	}
	if(add)
	{
		item->place_to(m_left + m_width, m_top);
		m_width += item->width();
		if(item->get_el()->src_el()->css().get_display() == display_inline_text)
		{
			m_baseline = std::max(m_baseline, item->get_el()->css().get_font_metrics().base_line());
			m_line_height = std::max(m_line_height, item->get_el()->css().get_line_height());
			m_height = std::max(m_height, item->get_el()->css().get_font_metrics().height);
		}
		m_items.emplace_back(std::move(item));
	} else
	{
		item->get_el()->skip(true);
	}
}

std::list< std::unique_ptr<litehtml::line_box_item> > litehtml::line_box::finish(bool last_box)
{
	std::list< std::unique_ptr<line_box_item> > ret_items;

	if(!last_box)
	{
		while(!m_items.empty())
		{
			if (m_items.back()->get_type() == line_box_item::type_text_part)
			{
				// remove empty elements at the end of line
				if (m_items.back()->get_el()->src_el()->is_break() ||
					m_items.back()->get_el()->src_el()->is_white_space())
				{
					m_width -= m_items.back()->width();
					m_items.back()->get_el()->skip(true);
					m_items.pop_back();
				} else
				{
					break;
				}
			} else if (m_items.back()->get_type() == line_box_item::type_inline_start)
			{
				m_width -= m_items.back()->width();
				ret_items.emplace_back(std::move(m_items.back()));
				m_items.pop_back();
			} else
			{
				break;
			}
		}
	} else
	{
		// remove trailing spaces
		auto iter = m_items.rbegin();
		while(iter != m_items.rend())
		{
			if ((*iter)->get_type() == line_box_item::type_text_part)
			{
				if((*iter)->get_el()->src_el()->is_white_space())
				{
					(*iter)->get_el()->skip(true);
					m_width -= (*iter)->width();
					iter = decltype(iter) (m_items.erase( std::next(iter).base() ));
				} else
				{
					break;
				}
			} else
			{
				iter++;
			}
		}
	}

    if( is_empty() || (!is_empty() && last_box && is_break_only()) )
    {
        m_height = m_default_line_height;
		m_baseline = m_font_metrics.base_line();
        return ret_items;
    }

	m_min_width = m_items.back()->right();

    int spc_x = 0;

    int add_x = 0;
    switch(m_text_align)
    {
        case text_align_right:
            if(m_width < (m_right - m_left))
            {
                add_x = (m_right - m_left) - m_width;
            }
            break;
        case text_align_center:
            if(m_width < (m_right - m_left))
            {
                add_x = ((m_right - m_left) - m_width) / 2;
            }
            break;
        case text_align_justify:
            if (m_width < (m_right - m_left))
            {
                add_x = 0;
                spc_x = (m_right - m_left) - m_width;
                if (spc_x > m_width/4)
                    spc_x = 0;
            }
            break;
        default:
            add_x = 0;
    }

    int counter = 0;
    float offj  = float(spc_x) / std::max(1.f, float(m_items.size())-1.f);
    float cixx  = 0.0f;

    if(m_height)
    {
        m_baseline += (m_line_height - m_height) / 2;
    }

    m_height = m_line_height;

    int y1	= 0;
    int y2	= m_height;

    for (const auto& el : m_items)
    {
		// for text_align_justify
		if (spc_x && counter)
		{
			cixx += offj;
			if ((counter + 1) == int(m_items.size()))
				cixx += 0.99f;
			el->pos().x += int(cixx);
		}
		counter++;
		if((m_text_align == text_align_right || spc_x) && counter == int(m_items.size()))
		{
			// Forcible justify the last element to the right side for text align right and justify;
			el->pos().x = m_right - el->pos().width;
		} else if(add_x)
		{
			el->pos().x += add_x;
		}

        if(el->get_el()->src_el()->css().get_display() == display_inline_text || el->get_el()->src_el()->css().get_display() == display_inline)
        {
            el->pos().y = m_height - m_baseline - el->get_el()->css().get_font_metrics().ascent;
        } else
        {
            switch(el->get_el()->css().get_vertical_align())
            {
                case va_super:
                case va_sub:
                case va_baseline:
                    el->pos().y = m_height - m_baseline - el->get_el()->height() + el->get_el()->get_base_line() + el->get_el()->content_margins_top();
                    break;
                case va_top:
                    el->pos().y = y1 + el->get_el()->content_margins_top();
                    break;
                case va_text_top:
                    el->pos().y = m_height - m_baseline - m_font_metrics.ascent + el->get_el()->content_margins_top();
                    break;
                case va_middle:
                    el->pos().y = m_height - m_baseline - m_font_metrics.x_height / 2 - el->get_el()->height() / 2 + el->get_el()->content_margins_top();
                    break;
                case va_bottom:
                    el->pos().y = y2 - el->get_el()->height() + el->get_el()->content_margins_top();
                    break;
                case va_text_bottom:
                    el->pos().y = m_height - m_baseline + m_font_metrics.descent - el->get_el()->height() + el->get_el()->content_margins_top();
                    break;
            }
            y1 = std::min(y1, el->top());
            y2 = std::max(y2, el->bottom());
        }
    }

    for (const auto& el : m_items)
    {
        el->pos().y -= y1;
        el->pos().y += m_top;
        if(el->get_el()->css().get_display() != display_inline_text && el->get_el()->css().get_display() != display_inline)
        {
            switch(el->get_el()->css().get_vertical_align())
            {
                case va_top:
                    el->pos().y = m_top + el->get_el()->content_margins_top();
                    break;
                case va_bottom:
                    el->pos().y = m_top + (y2 - y1) - el->get_el()->height() + el->get_el()->content_margins_top();
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

        el->get_el()->apply_relative_shift(m_right - m_left);
    }
    m_height = y2 - y1;
    m_baseline -= y1;

	return std::move(ret_items);
}

std::shared_ptr<litehtml::render_item> litehtml::line_box::get_first_text_part() const
{
	for(const auto & item : m_items)
	{
		if(item->get_type() == line_box_item::type_text_part)
		{
			return item->get_el();
		}
	}
	return nullptr;
}


std::shared_ptr<litehtml::render_item> litehtml::line_box::get_last_text_part() const
{
	for(auto iter = m_items.rbegin(); iter != m_items.rend(); iter++)
	{
		if((*iter)->get_type() == line_box_item::type_text_part)
		{
			return (*iter)->get_el();
		}
	}
	return nullptr;
}


bool litehtml::line_box::can_hold(const std::unique_ptr<line_box_item>& item, white_space ws) const
{
    if(!item->get_el()->src_el()->is_inline_box()) return false;

	if(item->get_type() == line_box_item::type_text_part)
	{
		auto last_el = get_last_text_part();

		// force new line if the last placed element was line break
		if (last_el && last_el->src_el()->is_break())
		{
			return false;
		}

		// line break should stay in current line box
		if (item->get_el()->src_el()->is_break())
		{
			return true;
		}

		if (ws == white_space_nowrap || ws == white_space_pre ||
			(ws == white_space_pre_wrap && item->get_el()->src_el()->is_space()))
		{
			return true;
		}

		if (m_left + m_width + item->width() > m_right)
		{
			return false;
		}
	}

    return true;
}

bool litehtml::line_box::have_last_space()  const
{
	auto last_el = get_last_text_part();
	if(last_el)
	{
		return last_el->src_el()->is_white_space() || last_el->src_el()->is_break();
	}
	return false;
}

bool litehtml::line_box::is_empty() const
{
    if(m_items.empty()) return true;
    for (const auto& el : m_items)
    {
		if(el->get_type() == line_box_item::type_text_part)
		{
			if (!el->get_el()->skip() || el->get_el()->src_el()->is_break())
			{
				return false;
			}
		}
    }
    return true;
}

int litehtml::line_box::baseline() const
{
    return m_baseline;
}

int litehtml::line_box::top_margin() const
{
    return 0;
}

int litehtml::line_box::bottom_margin() const
{
    return 0;
}

void litehtml::line_box::y_shift( int shift )
{
	m_top += shift;
    for (auto& el : m_items)
    {
        el->pos().y += shift;
    }
}

bool litehtml::line_box::is_break_only() const
{
    if(m_items.empty()) return false;

	bool break_found = false;

	for (auto iter = m_items.rbegin(); iter != m_items.rend(); iter++)
	{
		if((*iter)->get_type() == line_box_item::type_text_part)
		{
			if((*iter)->get_el()->src_el()->is_break())
			{
				break_found = true;
			} else if(!(*iter)->get_el()->skip())
			{
				return false;
			}
		}
	}
	return break_found;
}

std::list< std::unique_ptr<litehtml::line_box_item> > litehtml::line_box::new_width( int left, int right)
{
	std::list< std::unique_ptr<line_box_item> > ret_items;
    int add = left - m_left;
    if(add)
    {
		m_left	= left;
		m_right	= right;
        m_width = 0;
        auto remove_begin = m_items.end();
		auto i = m_items.begin();
		i++;
		while (i != m_items.end())
        {
            if(!(*i)->get_el()->skip())
            {
                if(m_left + m_width + (*i)->width() > m_right)
                {
                    remove_begin = i;
                    break;
                } else
                {
					(*i)->pos().x += add;
                    m_width += (*i)->get_el()->width();
                }
            }
			i++;
        }
        if(remove_begin != m_items.end())
        {
			while(remove_begin != m_items.end())
			{
				ret_items.emplace_back(std::move(*remove_begin));
			}
            m_items.erase(remove_begin, m_items.end());
        }
    }
	return ret_items;
}

