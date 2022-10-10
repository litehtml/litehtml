#include "line_box.h"
#include "element.h"
#include "render_item.h"
#include <algorithm>


int litehtml::line_box::height() const
{
    return m_height;
}

int litehtml::line_box::width() const
{
    return m_width;
}

void litehtml::line_box::add_element(const std::shared_ptr<render_item> &el)
{
    el->skip(false);
    el->set_line_box(nullptr);
    bool add	= true;
    if( (m_items.empty() && el->src_el()->is_white_space()) || el->src_el()->is_break() )
    {
        el->skip(true);
    } else if(el->src_el()->is_white_space())
    {
        if (have_last_space())
        {
            add = false;
            el->skip(true);
        }
    }

    if(add)
    {
        el->set_line_box(this);
        m_items.push_back(el);

        if(!el->skip())
        {
            int el_shift_left	= el->get_inline_shift_left();
            int el_shift_right	= el->get_inline_shift_right();

            el->pos().x	= m_box_left + m_width + el_shift_left + el->content_margins_left();
            el->pos().y	= m_box_top + el->content_margins_top();
            m_width		+= el->width() + el_shift_left + el_shift_right;
        }
    }
}

void litehtml::line_box::finish(bool last_box)
{
    // remove empty elements at the end of line
    while (!m_items.empty() && (m_items.back()->src_el()->is_white_space() || m_items.back()->src_el()->is_break()))
    {
        m_width -= m_items.back()->width();
        m_items.pop_back();
    }

    if( is_empty() || (!is_empty() && last_box && is_break_only()) )
    {
        m_height = 0;
        return;
    }

    int base_line	= m_font_metrics.base_line();
    int line_height = m_line_height;
    int spc_x = 0;

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
        case text_align_justify:
            if (m_width < (m_box_right - m_box_left))
            {
                add_x = 0;
                spc_x = (m_box_right - m_box_left) - m_width;
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

    m_height = 0;
    // find line box baseline and line-height
    for(const auto& el : m_items)
    {
        if(el->src_el()->css().get_display() == display_inline_text)
        {
            font_metrics fm = el->src_el()->css().get_font_metrics();
            base_line	= std::max(base_line,	fm.base_line());
            line_height = std::max(line_height, el->src_el()->css().get_line_height());
            m_height = std::max(m_height, fm.height);
        }
        if (spc_x && counter)
        {
            cixx += offj;
            if ((counter+1) == int(m_items.size()))
                cixx += 0.99f;
            el->pos().x += int(cixx);
        }
        counter++;
        if((m_text_align == text_align_right || spc_x) && counter == int(m_items.size()))
        {
            // Forcible justify the last element to the right side for text align right and justify;
            el->pos().x = m_box_right - el->pos().width;
        } else
        {
            el->pos().x += add_x;
        }
    }

    if(m_height)
    {
        base_line += (line_height - m_height) / 2;
    }

    m_height = line_height;

    int y1	= 0;
    int y2	= m_height;

    for (const auto& el : m_items)
    {
        if(el->src_el()->css().get_display() == display_inline_text)
        {
            el->pos().y = m_height - base_line - el->src_el()->css().get_font_metrics().ascent;
        } else
        {
            switch(el->src_el()->css().get_vertical_align())
            {
                case va_super:
                case va_sub:
                case va_baseline:
                    el->pos().y = m_height - base_line - el->height() + el->get_base_line() + el->content_margins_top();
                    break;
                case va_top:
                    el->pos().y = y1 + el->content_margins_top();
                    break;
                case va_text_top:
                    el->pos().y = m_height - base_line - m_font_metrics.ascent + el->content_margins_top();
                    break;
                case va_middle:
                    el->pos().y = m_height - base_line - m_font_metrics.x_height / 2 - el->height() / 2 + el->content_margins_top();
                    break;
                case va_bottom:
                    el->pos().y = y2 - el->height() + el->content_margins_top();
                    break;
                case va_text_bottom:
                    el->pos().y = m_height - base_line + m_font_metrics.descent - el->height() + el->content_margins_top();
                    break;
            }
            y1 = std::min(y1, el->top());
            y2 = std::max(y2, el->bottom());
        }
    }

    for (const auto& el : m_items)
    {
        el->pos().y -= y1;
        el->pos().y += m_box_top;
        if(el->src_el()->css().get_display() != display_inline_text)
        {
            switch(el->src_el()->css().get_vertical_align())
            {
                case va_top:
                    el->pos().y = m_box_top + el->content_margins_top();
                    break;
                case va_bottom:
                    el->pos().y = m_box_top + (y2 - y1) - el->height() + el->content_margins_top();
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

        el->apply_relative_shift(m_box_right - m_box_left);
    }
    m_height = y2 - y1;
    m_baseline = (base_line - y1) - (m_height - line_height);
}

bool litehtml::line_box::can_hold(const std::shared_ptr<render_item> &el, white_space ws) const
{
    if(!el->src_el()->is_inline_box()) return false;

    if(el->src_el()->is_break())
    {
        return false;
    }

    if(ws == white_space_nowrap || ws == white_space_pre || (ws == white_space_pre_wrap && el->src_el()->is_space()))
    {
        return true;
    }

    if(m_box_left + m_width + el->width() + el->get_inline_shift_left() + el->get_inline_shift_right() > m_box_right)
    {
        return false;
    }

    return true;
}

bool litehtml::line_box::have_last_space()  const
{
    if(m_items.empty())
    {
        return false;
    }
    return m_items.back()->src_el()->is_white_space() || m_items.back()->src_el()->is_break();
}

bool litehtml::line_box::is_empty() const
{
    if(m_items.empty()) return true;
    for (const auto& el : m_items)
    {
        if(!el->skip() || el->src_el()->is_break())
        {
            return false;
        }
    }
    return true;
}

int litehtml::line_box::baseline() const
{
    return m_baseline;
}

void litehtml::line_box::get_elements( std::vector< std::shared_ptr<render_item> >& els )
{
    els.insert(els.begin(), m_items.begin(), m_items.end());
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
    m_box_top += shift;
    for (auto& el : m_items)
    {
        el->pos().y += shift;
    }
}

bool litehtml::line_box::is_break_only() const
{
    if(m_items.empty()) return true;

    if(m_items.front()->src_el()->is_break())
    {
        for (auto& el : m_items)
        {
            if(!el->skip())
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

void litehtml::line_box::new_width( int left, int right, std::vector< std::shared_ptr<render_item> >& els )
{
    int add = left - m_box_left;
    if(add)
    {
        m_box_left	= left;
        m_box_right	= right;
        m_width = 0;
        auto remove_begin = m_items.end();
        for (auto i = m_items.begin() + 1; i != m_items.end(); i++)
        {
            auto el = (*i);

            if(!el->skip())
            {
                if(m_box_left + m_width + el->width() + el->get_inline_shift_right() + el->get_inline_shift_left() > m_box_right)
                {
                    remove_begin = i;
                    break;
                } else
                {
                    el->pos().x += add;
                    m_width += el->width() + el->get_inline_shift_right() + el->get_inline_shift_left();
                }
            }
        }
        if(remove_begin != m_items.end())
        {
            els.insert(els.begin(), remove_begin, m_items.end());
            m_items.erase(remove_begin, m_items.end());

            for(const auto& el : els)
            {
                el->set_line_box(nullptr);
            }
        }
    }
}

