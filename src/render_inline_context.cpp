#include "html.h"
#include "render_item.h"
#include "document.h"
#include "iterators.h"

int litehtml::render_item_inline_context::_render_content(int x, int y, int max_width, bool second_pass, int ret_width)
{
    m_line_boxes.clear();

    element_position el_position;

    int block_height = 0;

    if (get_predefined_height(block_height))
    {
        m_pos.height = block_height;
    }

    white_space ws = src_el()->css().get_white_space();
    bool skip_spaces = false;
    if (ws == white_space_normal ||
        ws == white_space_nowrap ||
        ws == white_space_pre_line)
    {
        skip_spaces = true;
    }

    bool was_space = false;

    go_inside_inline go_inside_inlines_selector;
    inline_selector select_inlines;
    elements_iterator inlines_iter(false, &go_inside_inlines_selector, &select_inlines);

    inlines_iter.process(shared_from_this(), [&](const std::shared_ptr<render_item>& el)
        {
            // skip spaces to make rendering a bit faster
            if (skip_spaces)
            {
                if (el->src_el()->is_white_space())
                {
                    if (was_space)
                    {
                        el->skip(true);
                        return;
                    }
                    else
                    {
                        was_space = true;
                    }
                }
                else
                {
                    was_space = false;
                }
            }
            // place element into rendering flow
            int rw = place_inline(el, max_width);
            if (rw > ret_width)
            {
                ret_width = rw;
            }
        });

    finish_last_box(true);

    if (!m_line_boxes.empty())
    {
        if (collapse_top_margin())
        {
            int old_top = m_margins.top;
            m_margins.top = std::max(m_line_boxes.front()->top_margin(), m_margins.top);
            if (m_margins.top != old_top)
            {
                update_floats(m_margins.top - old_top, shared_from_this());
            }
        }
        if (collapse_bottom_margin())
        {
            m_margins.bottom = std::max(m_line_boxes.back()->bottom_margin(), m_margins.bottom);
            m_pos.height = m_line_boxes.back()->bottom() - m_line_boxes.back()->bottom_margin();
        }
        else
        {
            m_pos.height = m_line_boxes.back()->bottom();
        }
    }

    return ret_width;
}

int litehtml::render_item_inline_context::fix_line_width( int max_width, element_float flt )
{
    int ret_width = 0;
    if(!m_line_boxes.empty())
    {
        std::vector<std::shared_ptr<render_item>> els;
        m_line_boxes.back()->get_elements(els);
        bool was_cleared = false;
        if(!els.empty() && els.front()->src_el()->css().get_clear() != clear_none)
        {
            if(els.front()->src_el()->css().get_clear() == clear_both)
            {
                was_cleared = true;
            } else
            {
                if(	(flt == float_left	&& els.front()->src_el()->css().get_clear() == clear_left) ||
                       (flt == float_right	&& els.front()->src_el()->css().get_clear() == clear_right) )
                {
                    was_cleared = true;
                }
            }
        }

        if(!was_cleared)
        {
            m_line_boxes.pop_back();

            for(const auto& el : els)
            {
                int rw = place_inline(el, max_width);
                if(rw > ret_width)
                {
                    ret_width = rw;
                }
            }
        } else
        {
            int line_top = 0;
            line_top = m_line_boxes.back()->top();

            int line_left	= 0;
            int line_right	= max_width;
            get_line_left_right(line_top, max_width, line_left, line_right);

            if(m_line_boxes.size() == 1 && src_el()->css().get_list_style_type() != list_style_type_none && src_el()->css().get_list_style_position() == list_style_position_inside)
            {
                int sz_font = src_el()->css().get_font_size();
                line_left += sz_font;
            }

            if(src_el()->css().get_text_indent().val() != 0)
            {
                if(!m_line_boxes.empty())
                {
                    line_left += src_el()->css().get_text_indent().calc_percent(max_width);
                }
            }

            els.clear();
            m_line_boxes.back()->new_width(line_left, line_right, els);
            for(auto& el : els)
            {
                int rw = place_inline(el, max_width);
                if(rw > ret_width)
                {
                    ret_width = rw;
                }
            }
        }
    }

    return ret_width;
}

int litehtml::render_item_inline_context::finish_last_box(bool end_of_render)
{
    int line_top = 0;

    if(!m_line_boxes.empty())
    {
        m_line_boxes.back()->finish(end_of_render);

        if(m_line_boxes.back()->is_empty())
        {
            line_top = m_line_boxes.back()->top();
            m_line_boxes.pop_back();
        }

        if(!m_line_boxes.empty())
        {
            line_top = m_line_boxes.back()->bottom();
        }
    }
    return line_top;
}

int litehtml::render_item_inline_context::new_box(const std::shared_ptr<render_item> &el, int max_width, line_context& line_ctx)
{
    line_ctx.top = get_cleared_top(el, finish_last_box());

    line_ctx.left = 0;
    line_ctx.right = max_width;
    line_ctx.fix_top();
    get_line_left_right(line_ctx.top, max_width, line_ctx.left, line_ctx.right);

    if(el->src_el()->is_inline_box() || el->src_el()->is_floats_holder())
    {
        if (el->width() > line_ctx.right - line_ctx.left)
        {
            line_ctx.top = find_next_line_top(line_ctx.top, el->width(), max_width);
            line_ctx.left = 0;
            line_ctx.right = max_width;
            line_ctx.fix_top();
            get_line_left_right(line_ctx.top, max_width, line_ctx.left, line_ctx.right);
        }
    }

    int first_line_margin = 0;
    if(m_line_boxes.empty() && src_el()->css().get_list_style_type() != list_style_type_none && src_el()->css().get_list_style_position() == list_style_position_inside)
    {
        int sz_font = src_el()->css().get_font_size();
        first_line_margin = sz_font;
    }

    int text_indent = 0;
    if(src_el()->css().get_text_indent().val() != 0)
    {
        if(!m_line_boxes.empty())
        {
            text_indent = src_el()->css().get_text_indent().calc_percent(max_width);
        }
    }

    font_metrics fm = src_el()->css().get_font_metrics();
    m_line_boxes.emplace_back(std::unique_ptr<line_box>(new line_box(line_ctx.top, line_ctx.left + first_line_margin + text_indent, line_ctx.right, src_el()->css().get_line_height(), fm, src_el()->css().get_text_align())));

    return line_ctx.top;
}

int litehtml::render_item_inline_context::place_inline(const std::shared_ptr<render_item> &el, int max_width)
{
    if(el->src_el()->css().get_display() == display_none) return 0;

    if(el->src_el()->is_float())
    {
        int line_top = 0;
        if(!m_line_boxes.empty())
        {
            line_top = m_line_boxes.back()->top();
        }
        return place_float(el, line_top, max_width);
    }

    int ret_width = 0;

    line_context line_ctx = {0};
    line_ctx.top = 0;
    if (!m_line_boxes.empty())
    {
        line_ctx.top = m_line_boxes.back()->top();
    }
    line_ctx.left = 0;
    line_ctx.right = max_width;
    line_ctx.fix_top();
    get_line_left_right(line_ctx.top, max_width, line_ctx.left, line_ctx.right);

    switch(el->src_el()->css().get_display())
    {
        case display_inline_block:
        case display_inline_table:
            ret_width = el->render(line_ctx.left, line_ctx.top, line_ctx.right);
            break;
        case display_inline_text:
        {
            litehtml::size sz;
            el->src_el()->get_content_size(sz, line_ctx.right);
            el->pos() = sz;
        }
            break;
        default:
            ret_width = 0;
            break;
    }

    bool add_box = true;
    if(!m_line_boxes.empty())
    {
        if(m_line_boxes.back()->can_hold(el, src_el()->css().get_white_space()))
        {
            add_box = false;
        }
    }
    if(add_box)
    {
        new_box(el, max_width, line_ctx);
    } else if(!m_line_boxes.empty())
    {
        line_ctx.top = m_line_boxes.back()->top();
    }

    if (line_ctx.top != line_ctx.calculatedTop)
    {
        line_ctx.left = 0;
        line_ctx.right = max_width;
        line_ctx.fix_top();
        get_line_left_right(line_ctx.top, max_width, line_ctx.left, line_ctx.right);
    }

    if(!el->src_el()->is_inline_box())
    {
        if(m_line_boxes.size() == 1)
        {
            if(collapse_top_margin())
            {
                int shift = el->margin_top();
                if(shift >= 0)
                {
                    line_ctx.top -= shift;
                    m_line_boxes.back()->y_shift(-shift);
                }
            }
        } else
        {
            int shift = 0;
            int prev_margin = m_line_boxes[m_line_boxes.size() - 2]->bottom_margin();

            if(prev_margin > el->margin_top())
            {
                shift = el->margin_top();
            } else
            {
                shift = prev_margin;
            }
            if(shift >= 0)
            {
                line_ctx.top -= shift;
                m_line_boxes.back()->y_shift(-shift);
            }
        }
    }

    m_line_boxes.back()->add_element(el);

    if(el->src_el()->is_inline_box() && !el->skip())
    {
        ret_width = el->right() + (max_width - line_ctx.right);
    }

    return ret_width;
}

void litehtml::render_item_inline_context::apply_vertical_align()
{
    if(!m_line_boxes.empty())
    {
        int add = 0;
        int content_height	= m_line_boxes.back()->bottom();

        if(m_pos.height > content_height)
        {
            switch(src_el()->css().get_vertical_align())
            {
                case va_middle:
                    add = (m_pos.height - content_height) / 2;
                    break;
                case va_bottom:
                    add = m_pos.height - content_height;
                    break;
                default:
                    add = 0;
                    break;
            }
        }

        if(add)
        {
            for(auto & box : m_line_boxes)
            {
                box->y_shift(add);
            }
        }
    }
}

int litehtml::render_item_inline_context::get_base_line()
{
    auto el_parent = parent();
    if(el_parent && src_el()->css().get_display() == display_inline_flex)
    {
        return el_parent->get_base_line();
    }
    if(src_el()->is_replaced())
    {
        return 0;
    }
    int bl = 0;
    if(!m_line_boxes.empty())
    {
        bl = m_line_boxes.back()->baseline() + content_margins_bottom();
    }
    return bl;
}
