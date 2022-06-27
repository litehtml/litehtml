#include "html.h"
#include "render_item.h"
#include "document.h"

int litehtml::render_item_block_context::_render(int x, int y, int max_width, bool second_pass)
{
    int parent_width = max_width;

    calc_outlines(parent_width);

    m_pos.clear();
    m_pos.move_to(x, y);

    m_pos.x += content_margins_left();
    m_pos.y += content_margins_top();

    int ret_width = 0;

    def_value<int>	block_width(0);

    if (m_element->css().get_display() != display_table_cell && !m_element->css().get_width().is_predefined())
    {
        int w = calc_width(parent_width);

        if (m_element->css().get_box_sizing() == box_sizing_border_box)
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
    if (!m_element->css().get_max_width().is_predefined() && !second_pass)
    {
        int mw = m_element->get_document()->to_pixels(m_element->css().get_max_width(), m_element->css().get_font_size(), parent_width);
        if (m_element->css().get_box_sizing() == box_sizing_border_box)
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
    m_cache_line_left.invalidate();
    m_cache_line_right.invalidate();

    element_position el_position;

    int block_height = 0;

    m_pos.height = 0;

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
                el->pos().x	+= el->content_margins_left();
                el->pos().y	+= el->content_margins_top();
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

    m_pos.width = max_width;
    calc_auto_margins(parent_width);

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
    if (!m_element->css().get_min_height().is_predefined() && m_element->css().get_min_height().units() == css_units_percentage)
    {
        auto el_parent = parent();
        if (el_parent)
        {
            if (el_parent->get_predefined_height(block_height))
            {
                min_height = m_element->css().get_min_height().calc_percent(block_height);
            }
        }
    }
    else
    {
        min_height = (int)m_element->css().get_min_height().val();
    }
    if (min_height != 0 && m_element->css().get_box_sizing() == box_sizing_border_box)
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
            m_element->get_document()->container()->get_image_size(url.c_str(), list_image_baseurl, sz);
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

    if (min_width != 0 && m_element->css().get_box_sizing() == box_sizing_border_box)
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
        if (m_element->css().get_display() == display_inline_block ||
            (src_el()->css().get_width().is_predefined() &&
             (m_element->css().get_float() != float_none ||
              m_element->css().get_display() == display_table ||
              m_element->css().get_position() == element_position_absolute ||
              m_element->css().get_position() == element_position_fixed
             ))
                )
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
