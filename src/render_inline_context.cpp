#include "html.h"
#include "render_item.h"
#include "document.h"
#include "iterators.h"

int litehtml::render_item_inline_context::_render(int x, int y, int max_width, bool second_pass)
{
    int parent_width = max_width;

    calc_outlines(parent_width);

    m_pos.clear();
    m_pos.move_to(x, y);

    m_pos.x += content_margins_left();
    m_pos.y += content_margins_top();

    int ret_width = 0;

    def_value<int>	block_width(0);

    if (src_el()->css().get_display() != display_table_cell && !src_el()->css().get_width().is_predefined())
    {
        int w = calc_width(parent_width);

        if (src_el()->css().get_box_sizing() == box_sizing_border_box)
        {
            w -= m_padding.width() + m_borders.width();
        }
        max_width = w;
        if(src_el()->css().get_width().units() != css_units_percentage)
        {
            block_width = ret_width = w;
        } else
        {
            auto par = parent();
            if(!par || par && !par->src_el()->css().get_width().is_predefined())
            {
                block_width = ret_width = w;
            }
        }
    }
    else
    {
        if (max_width)
        {
            max_width -= content_margins_left() + content_margins_right();
        }
    }

    // check for max-width (on the first pass only)
    if (!src_el()->css().get_max_width().is_predefined() && !second_pass)
    {
        int mw = src_el()->get_document()->to_pixels(src_el()->css().get_max_width(), src_el()->css().get_font_size(), parent_width);
        if (src_el()->css().get_box_sizing() == box_sizing_border_box)
        {
            mw -= m_padding.left + m_borders.left + m_padding.right + m_borders.right;
        }
        if (max_width > mw)
        {
            max_width = mw;
        }
    }

    m_floats_left.clear();
    m_floats_right.clear();
    m_line_boxes.clear();
    m_cache_line_left.invalidate();
    m_cache_line_right.invalidate();

    element_position el_position;

    int block_height = 0;

    m_pos.height = 0;

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

    if (block_width.is_default() && (src_el()->is_inline_box() || src_el()->css().get_float() != float_none))
    {
        m_pos.width = ret_width;
    }
    else
    {
        m_pos.width = max_width;
    }
    calc_auto_margins(parent_width);

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

    // add the floats height to the block height
    if (src_el()->is_floats_holder())
    {
        int floats_height = get_floats_height();
        if (floats_height > m_pos.height)
        {
            m_pos.height = floats_height;
        }
    }

    // calculate the final position

    m_pos.move_to(x, y);
    m_pos.x += content_margins_left();
    m_pos.y += content_margins_top();

    if (get_predefined_height(block_height))
    {
        m_pos.height = block_height;
    }

    int min_height = 0;
    if (!src_el()->css().get_min_height().is_predefined() && src_el()->css().get_min_height().units() == css_units_percentage)
    {
        auto el_parent = parent();
        if (el_parent)
        {
            if (el_parent->get_predefined_height(block_height))
            {
                min_height = src_el()->css().get_min_height().calc_percent(block_height);
            }
        }
    }
    else
    {
        min_height = (int)src_el()->css().get_min_height().val();
    }
    if (min_height != 0 && src_el()->css().get_box_sizing() == box_sizing_border_box)
    {
        min_height -= m_padding.top + m_borders.top + m_padding.bottom + m_borders.bottom;
        if (min_height < 0) min_height = 0;
    }

    if (src_el()->css().get_display() == display_list_item)
    {
        const tchar_t* list_image = src_el()->get_style_property(_t("list-style-image"), true, nullptr);
        if (list_image)
        {
            tstring url;
            css::parse_css_url(list_image, url);

            size sz;
            const tchar_t* list_image_baseurl = src_el()->get_style_property(_t("list-style-image-baseurl"), true, nullptr);
            src_el()->get_document()->container()->get_image_size(url.c_str(), list_image_baseurl, sz);
            if (min_height < sz.height)
            {
                min_height = sz.height;
            }
        }

    }

    if (min_height > m_pos.height)
    {
        m_pos.height = min_height;
    }

    int min_width = src_el()->css().get_min_width().calc_percent(parent_width);

    if (min_width != 0 && src_el()->css().get_box_sizing() == box_sizing_border_box)
    {
        min_width -= m_padding.left + m_borders.left + m_padding.right + m_borders.right;
        if (min_width < 0) min_width = 0;
    }

    if (min_width != 0)
    {
        if (min_width > m_pos.width)
        {
            m_pos.width = min_width;
        }
        if (min_width > ret_width)
        {
            ret_width = min_width;
        }
    }

    ret_width += content_margins_left() + content_margins_right();

    // re-render with new width
    if (ret_width < max_width && !second_pass && have_parent())
    {
        if (src_el()->css().get_display() == display_inline_block ||
                (src_el()->css().get_width().is_predefined() &&
                    (src_el()->css().get_float() != float_none ||
                        src_el()->css().get_display() == display_table ||
                        src_el()->css().get_position() == element_position_absolute ||
                        src_el()->css().get_position() == element_position_fixed
                    )))
        {
            _render(x, y, ret_width, true);
            m_pos.width = ret_width - (content_margins_left() + content_margins_right());
        }
    }

    if (src_el()->is_floats_holder() && !second_pass)
    {
        for (const auto& fb : m_floats_left)
        {
            fb.el->apply_relative_shift(fb.el->parent()->calc_width(m_pos.width));
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
