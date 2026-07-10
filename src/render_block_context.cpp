#include "render_block_context.h"

#include "document.h"
#include "pixel_type.h"
#include "types.h"
#include <algorithm>

litehtml::pixel_t litehtml::render_item_block_context::_render_content(pixel_t /*x*/, pixel_t /*y*/, bool second_pass,
                                                                       const containing_block_context& self_size,
                                                                       formatting_context*             fmt_ctx)
{
    std::shared_ptr<render_item> last_margin_el;

    pixel_t ret_width   = 0_px;
    pixel_t child_top   = 0_px;
    pixel_t last_margin = 0_px;
    bool    is_first    = true;
    for(const auto& el : m_children)
    {
        // we don't need to process absolute and fixed positioned element on the second pass
        if(second_pass)
        {
            auto el_position = el->src_el()->css().get_position();
            if((el_position == element_position_absolute || el_position == element_position_fixed))
            {
                continue;
            }
        }

        if(el->src_el()->css().get_float() != float_none)
        {
            auto rw   = place_float(el, child_top, self_size, fmt_ctx);
            ret_width = std::max(ret_width, rw);
        } else if(el->src_el()->css().get_display() != display_none)
        {
            if(el->src_el()->css().get_position() == element_position_absolute ||
               el->src_el()->css().get_position() == element_position_fixed)
            {
                el->render(0_px, child_top, self_size, fmt_ctx);
            } else
            {
                child_top           = fmt_ctx->get_cleared_top(el, child_top);
                pixel_t child_left  = 0_px;
                pixel_t child_width = self_size.render_width;
                pixel_t top_margin  = m_margins.top;

                el->calc_outlines(self_size.width);

                // Collapse top margin
                if(is_first && collapse_top_margin())
                {
                    if(el->get_margins().top > 0_px)
                    {
                        child_top -= el->get_margins().top;
                        if(el->get_margins().top > get_margins().top)
                        {
                            top_margin = el->get_margins().top;
                        }
                    }
                } else
                {
                    if(el->get_margins().top > 0_px)
                    {
                        if(last_margin > el->get_margins().top)
                        {
                            child_top -= el->get_margins().top;
                        } else
                        {
                            child_top -= last_margin;
                        }
                    }
                }

                pixel_t rw = 0_px;

                // Check if we need to move block to the next line
                if(el->src_el()->is_replaced() || el->src_el()->is_block_formatting_context() ||
                   el->src_el()->css().get_display() == display_table)
                {
                    pixel_t el_width = el->get_intrinsic_min_size().width;
                    if(!el->css().get_width().is_predefined())
                    {
                        el_width =
                            el->css().get_width().calc_percent(self_size.render_width) + el->render_offset_width();
                    }

                    formatting_context::el_position el_pos;
                    el_pos.el_pos.x        = child_left;
                    el_pos.el_pos.y        = child_top;
                    el_pos.el_pos.width    = el_width;
                    el_pos.el_pos.height   = el->get_intrinsic_min_size().height;
                    el_pos.container_width = self_size.render_width;
                    el_pos.el_margins      = el->get_margins();
                    auto new_pos           = fmt_ctx->place_to_left(el_pos);
                    if(new_pos.found)
                    {
                        child_top   = new_pos.top;
                        el->pos().x = new_pos.left + el->content_offset_left();
                        el->pos().y = child_top + el->content_offset_top();
                        //  Rollback top margin collapse if the block was moved to the new line
                        if(is_first && new_pos.new_line && collapse_top_margin())
                        {
                            top_margin = m_margins.top;
                        }
                    }
                    rw  = el->render(new_pos.left, new_pos.top, self_size.new_width(new_pos.width), fmt_ctx);
                    rw += self_size.render_width.value - new_pos.width;
                } else
                {
                    rw = el->render(child_left, child_top, self_size.new_width(child_width), fmt_ctx);
                }

                pixel_t auto_margin = el->calc_auto_margins(child_width);
                if(auto_margin != 0_px)
                {
                    el->pos().x += auto_margin;
                }
                ret_width       = std::max(ret_width, rw);
                m_margins.top   = top_margin;
                child_top      += el->height();
                last_margin     = el->get_margins().bottom;
                last_margin_el  = el;
                is_first        = false;

                if(el->src_el()->css().get_position() == element_position_relative)
                {
                    el->apply_relative_shift(self_size);
                }
            }
        }
    }

    m_pos.width = self_size.render_width;
    if(self_size.height.type != containing_block_context::cbc_value_type_auto && self_size.height.value > 0_px)
    {
        m_pos.height = self_size.height;
    } else
    {
        m_pos.height = child_top;
        if(collapse_bottom_margin())
        {
            m_pos.height     -= last_margin;
            m_margins.bottom  = std::max(m_margins.bottom, last_margin);
            if(last_margin_el)
            {
                last_margin_el->get_margins().bottom = 0_px;
            }
        }
    }

    return ret_width;
}

litehtml::pixel_t litehtml::render_item_block_context::get_first_baseline()
{
    if(m_children.empty())
    {
        return height() - margin_bottom();
    }
    const auto& item = m_children.front();
    return content_offset_top() + item->top() + item->get_first_baseline();
}

litehtml::pixel_t litehtml::render_item_block_context::get_last_baseline()
{
    if(m_children.empty())
    {
        return height() - margin_bottom();
    }
    const auto& item = m_children.back();
    return content_offset_top() + item->top() + item->get_last_baseline();
}

void litehtml::render_item_block_context::calc_intrinsic_size()
{
    pixel_t float_width_min = 0_px;
    pixel_t float_width_max = 0_px;
    bool    has_float       = false;
    for(const auto& el : m_children)
    {
        // Skip absolute and fixed positioned elements, they don't affect the intrinsic size of the block formatting
        // context
        if(is_one_of(el->css().get_position(), element_position_absolute, element_position_fixed))
        {
            continue;
        }

        if(el->css().get_float() == float_left || (el->css().get_float() == float_right && has_float))
        {
            float_width_min += el->get_intrinsic_min_size().width;
            float_width_max += el->get_intrinsic_max_size().width;
            has_float        = true;
            continue;
        }

        if(has_float && (el->src_el()->is_replaced() || el->src_el()->is_block_formatting_context() ||
                         el->src_el()->css().get_display() == display_table))
        {
            float_width_min += el->get_intrinsic_min_size().width;
            float_width_max += el->get_intrinsic_max_size().width;
            if(has_float)
            {
                m_intrinsic_min_size.width = std::max(m_intrinsic_min_size.width, float_width_min);
                m_intrinsic_max_size.width = std::max(m_intrinsic_max_size.width, float_width_max);
                float_width_min            = 0_px;
                float_width_max            = 0_px;
                has_float                  = false;
            }
        } else
        {
            if(has_float)
            {
                m_intrinsic_min_size.width = std::max(m_intrinsic_min_size.width, float_width_min);
                m_intrinsic_max_size.width = std::max(m_intrinsic_max_size.width, float_width_max);
                float_width_min            = 0_px;
                float_width_max            = 0_px;
                has_float                  = false;
            }

            m_intrinsic_min_size.width   = std::max(m_intrinsic_min_size.width, el->get_intrinsic_min_size().width);
            m_intrinsic_min_size.height += el->get_intrinsic_min_size().height;
            m_intrinsic_max_size.width   = std::max(m_intrinsic_max_size.width, el->get_intrinsic_max_size().width);
            m_intrinsic_max_size.height += el->get_intrinsic_max_size().height;
        }
    }
    if(has_float)
    {
        m_intrinsic_min_size.width = std::max(m_intrinsic_min_size.width, float_width_min);
        m_intrinsic_max_size.width = std::max(m_intrinsic_max_size.width, float_width_max);
    }
    m_intrinsic_min_size.width  += content_offset_width();
    m_intrinsic_max_size.width  += content_offset_width();
    m_intrinsic_min_size.height += content_offset_height();
    m_intrinsic_max_size.height += content_offset_height();
}
