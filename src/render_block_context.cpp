#include "html.h"
#include "render_item.h"
#include "document.h"

int litehtml::render_item_block_context::_render_content(int x, int y, int max_width, bool second_pass, int ret_width)
{
    element_position el_position;

    int child_top = 0;
    int last_margin = 0;
    bool is_first = true;
    for (const auto& el : m_children)
    {
        // we don't need to process absolute and fixed positioned element on the second pass
        if (second_pass)
        {
            el_position = el->src_el()->css().get_position();
            if ((el_position == element_position_absolute || el_position == element_position_fixed)) continue;
        }

        if(el->src_el()->css().get_float() != float_none)
        {
            int rw = place_float(el, child_top, max_width);
            if (rw > ret_width)
            {
                ret_width = rw;
            }
        } else if(el->src_el()->css().get_display() != display_none)
        {
            if(el->src_el()->css().get_position() == element_position_absolute || el->src_el()->css().get_position() == element_position_fixed)
            {
                el->render(0, child_top, max_width);
                el->pos().x	+= el->content_offset_left();
                el->pos().y	+= el->content_offset_top();
            } else
            {
                child_top = get_cleared_top(el, child_top);
                int child_x  = 0;
                int child_width = max_width;

                el->calc_outlines(max_width);

                // Collapse top margin
                if(is_first && collapse_top_margin())
                {
                    child_top -= el->get_margins().top;
                    if(el->get_margins().top > get_margins().top)
                    {
                        m_margins.top = el->get_margins().top;
                    }
                } else
                {
                    if(last_margin > el->get_margins().top)
                    {
                        child_top -= el->get_margins().top;
                    } else
                    {
                        child_top -= last_margin;
                    }
                }

                if(el->src_el()->is_replaced() || el->src_el()->is_floats_holder())
                {
                    int ln_left = 0;
                    int ln_right = child_width;
                    get_line_left_right(child_top, child_width, ln_left, ln_right);
                    child_x = ln_left;
                    child_width = ln_right - ln_left;

                    auto el_parent = el->parent();
                    el->pos().width = el->src_el()->css().get_width().calc_percent(child_width);
                    el->pos().height = el->src_el()->css().get_height().calc_percent(el_parent ? el_parent->pos().height : 0);
                }

                int rw = el->render(child_x, child_top, child_width);
                if (rw > ret_width)
                {
                    ret_width = rw;
                }
                child_top += el->height();
                last_margin = el->get_margins().bottom;
                is_first = false;

                if (el->src_el()->css().get_position() == element_position_relative)
                {
                    el->apply_relative_shift(max_width);
                }
            }
        }
    }

    int block_height = 0;
    if (get_predefined_height(block_height))
    {
        m_pos.height = block_height;
    } else
    {
        m_pos.height = child_top;
        if(collapse_bottom_margin())
        {
            m_pos.height -= last_margin;
            if(m_margins.bottom < last_margin)
            {
                m_margins.bottom = last_margin;
            }
        }
    }

    return ret_width;
}
