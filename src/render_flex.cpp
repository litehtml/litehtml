#include "types.h"
#include "document.h"
#include "render_flex.h"
#include "html_tag.h"
#include <optional>

namespace
{
    using litehtml::operator""_px;

    bool may_have_gap(const litehtml::css_length& gap)
    {
        return gap.calc_percent(0_px) != 0_px || gap.has_nonzero_percentage();
    }

    litehtml::pixel_t static_main_position(litehtml::flex_justify_content justify, litehtml::pixel_t free_space,
                                           bool reverse, bool is_row)
    {
        using namespace litehtml;
        switch(justify)
        {
        case flex_justify_content_center:
        case flex_justify_content_space_around:
        case flex_justify_content_space_evenly:
            return free_space / 2_px;
        case flex_justify_content_end:
            return free_space;
        case flex_justify_content_right:
            return is_row ? free_space : 0_px;
        case flex_justify_content_flex_end:
            return reverse ? 0_px : free_space;
        case flex_justify_content_start:
        case flex_justify_content_left:
            return 0_px;
        default:
            return reverse ? free_space : 0_px;
        }
    }

    litehtml::pixel_t static_cross_position(litehtml::flex_align_items align, litehtml::pixel_t free_space,
                                            bool reverse)
    {
        using namespace litehtml;
        switch(align & 0xFF)
        {
        case flex_align_items_center:
            return free_space / 2_px;
        case flex_align_items_end:
        case flex_align_items_self_end:
            return free_space;
        case flex_align_items_flex_end:
            return reverse ? 0_px : free_space;
        case flex_align_items_normal:
        case flex_align_items_stretch:
        case flex_align_items_flex_start:
            return reverse ? free_space : 0_px;
        default:
            return 0_px;
        }
    }
} // namespace

litehtml::pixel_t litehtml::render_item_flex::_render_content(pixel_t x, pixel_t y, bool second_pass,
                                                              const containing_block_context& containing_block_size,
                                                              formatting_context*             fmt_ctx)
{
    containing_block_context self_size = containing_block_size;
    if(self_size.height.type != containing_block_context::cbc_value_type_auto)
    {
        // Flex items resolve percentages against the container's content box.
        self_size.height.value = std::max(0_px, self_size.height.value - box_sizing_height());
        if(src_el()->get_document()->mode() == quirks_mode)
        {
            self_size.quirks_height = self_size.height;
        }
    }

    bool    is_row_direction    = true;
    bool    reverse             = false;
    pixel_t container_main_size = self_size.render_width;

    switch(css().get_flex_direction())
    {
    case flex_direction_column:
        is_row_direction = false;
        reverse          = false;
        break;
    case flex_direction_column_reverse:
        is_row_direction = false;
        reverse          = true;
        break;
    case flex_direction_row:
        is_row_direction = true;
        reverse          = false;
        break;
    case flex_direction_row_reverse:
        is_row_direction = true;
        reverse          = true;
        break;
    }

    bool single_line   = css().get_flex_wrap() == flex_wrap_nowrap;
    bool fit_container = false;

    const pixel_t column_gap = css().get_column_gap().calc_percent(self_size.render_width);
    const pixel_t row_gap    = css().get_row_gap().calc_percent(
        self_size.height.type == containing_block_context::cbc_value_type_auto ? 0_px : self_size.height.value);
    const pixel_t main_gap          = is_row_direction ? column_gap : row_gap;
    const pixel_t cross_gap         = is_row_direction ? row_gap : column_gap;

    if(!is_row_direction)
    {
        if(self_size.height.type != containing_block_context::cbc_value_type_auto)
        {
            container_main_size = self_size.height.value;
        } else
        {
            // Direction columns, height is auto - always in single line
            container_main_size = 0;
            single_line         = true;
            fit_container       = true;
        }
        if(self_size.min_height.type != containing_block_context::cbc_value_type_auto &&
           self_size.min_height.value > container_main_size)
        {
            container_main_size = self_size.min_height;
        }
        if(self_size.max_height.type != containing_block_context::cbc_value_type_auto &&
           self_size.max_height.value > container_main_size)
        {
            container_main_size = self_size.max_height;
            single_line         = false;
        }
    }

    /////////////////////////////////////////////////////////////////
    /// Split flex items to lines
    /////////////////////////////////////////////////////////////////
    m_lines = get_lines(self_size, fmt_ctx, is_row_direction, container_main_size, main_gap, single_line, second_pass);

    pixel_t sum_cross_size = 0_px;
    pixel_t sum_main_size  = 0_px;
    pixel_t ret_width      = 0_px;

    /////////////////////////////////////////////////////////////////
    /// Resolving Flexible Lengths
    /// REF: https://www.w3.org/TR/css-flexbox-1/#resolve-flexible-lengths
    /////////////////////////////////////////////////////////////////
    for(auto& ln : m_lines)
    {
        if(is_row_direction)
        {
            ret_width += ln.base_size + ln.gap_size();
        }
        if(reverse)
        {
            ln.items.reverse();
        }
        ln.init(container_main_size, fit_container, is_row_direction, self_size, fmt_ctx);
        sum_cross_size += ln.cross_size;
        sum_main_size   = std::max(sum_main_size, ln.main_size + ln.gap_size());
    }
    if(m_lines.size() > 1)
    {
        sum_cross_size += cross_gap * static_cast<int>(m_lines.size() - 1);
    }

    pixel_t free_cross_size = 0_px;
    bool    is_wrap_reverse = css().get_flex_wrap() == flex_wrap_wrap_reverse;
    if(fit_container && container_main_size == 0_px)
    {
        container_main_size = sum_main_size;
    }

    /////////////////////////////////////////////////////////////////
    /// Calculate free cross size
    /////////////////////////////////////////////////////////////////
    if(is_row_direction)
    {
        if(self_size.height.type != containing_block_context::cbc_value_type_auto)
        {
            pixel_t height  = self_size.height.value;
            free_cross_size = height - sum_cross_size;
        }
    } else
    {
        free_cross_size = self_size.render_width.value - sum_cross_size;
        ret_width       = sum_cross_size;
    }

    /////////////////////////////////////////////////////////////////
    /// Fix align-content property
    /////////////////////////////////////////////////////////////////
    flex_align_content align_content = css().get_flex_align_content();
    if(align_content == flex_align_content_space_between)
    {
        // If the leftover free-space is negative or there is only a single flex line in the flex
        // container, this value is identical to flex-start.
        if(m_lines.size() <= 1 || free_cross_size < 0_px)
        {
            align_content = flex_align_content_flex_start;
        }
    }
    if(align_content == flex_align_content_space_around)
    {
        // If the leftover free-space is negative or there is only a single flex line in the flex
        // container, this value is identical to flex-start.
        if(m_lines.size() <= 1 || free_cross_size < 0_px)
        {
            align_content = flex_align_content_center;
        }
    }

    /////////////////////////////////////////////////////////////////
    /// Distribute free cross size for align-content: stretch
    /////////////////////////////////////////////////////////////////
    if(!m_lines.empty() && css().get_flex_align_content() == flex_align_content_stretch && free_cross_size > 0_px)
    {
        pixel_t add = free_cross_size / pixel_t(static_cast<int>(m_lines.size()));
        if(add > 0_px)
        {
            for(auto& ln : m_lines)
            {
                ln.cross_size   += add;
                free_cross_size -= add;
            }
        }
        if(!m_lines.empty())
        {
            while(free_cross_size > 0_px)
            {
                pixel_t distributeStep = 1_px;
                for(auto& ln : m_lines)
                {
                    ln.cross_size   += distributeStep;
                    free_cross_size -= distributeStep;
                }
            }
        }
    }

    /// Reverse lines for flex-wrap: wrap-reverse
    if(css().get_flex_wrap() == flex_wrap_wrap_reverse)
    {
        m_lines.reverse();
    }

    /////////////////////////////////////////////////////////////////
    /// Align flex lines
    /////////////////////////////////////////////////////////////////
    pixel_t line_pos        = 0_px;
    pixel_t add_before_line = 0_px;
    pixel_t add_after_line  = 0_px;
    switch(align_content)
    {
    case flex_align_content_flex_start:
        if(is_wrap_reverse)
        {
            line_pos = free_cross_size;
        }
        break;
    case flex_align_content_flex_end:
        if(!is_wrap_reverse)
        {
            line_pos = free_cross_size;
        }
        break;
    case flex_align_content_end:
        line_pos = free_cross_size;
        break;
    case flex_align_content_center:
        line_pos = free_cross_size / 2_px;
        break;
    case flex_align_content_space_between:
        add_after_line = free_cross_size / pixel_t(static_cast<int>(m_lines.size() - 1));
        break;
    case flex_align_content_space_around:
        add_before_line = add_after_line = free_cross_size / pixel_t(static_cast<int>(m_lines.size() * 2));
        break;
    case flex_align_content_space_evenly:
        if(free_cross_size > 0_px)
        {
            add_before_line = free_cross_size / pixel_t(static_cast<int>(m_lines.size() + 1));
            add_after_line  = 0_px;
        } else
        {
            line_pos = free_cross_size / 2_px;
        }
        break;
    default:
        if(is_wrap_reverse)
        {
            line_pos = free_cross_size;
        }
        break;
    }
    bool first_line = true;
    for(auto& ln : m_lines)
    {
        if(!first_line)
        {
            line_pos += cross_gap;
        }
        first_line      = false;
        line_pos       += add_before_line;
        ln.cross_start  = line_pos;
        line_pos       += ln.cross_size + add_after_line;
    }

    /// Fix justify-content property
    flex_justify_content justify_content = css().get_flex_justify_content();
    if((justify_content == flex_justify_content_right || justify_content == flex_justify_content_left) &&
       !is_row_direction)
    {
        justify_content = flex_justify_content_start;
    }

    /////////////////////////////////////////////////////////////////
    /// Align flex items in flex lines
    /////////////////////////////////////////////////////////////////
    for(auto& ln : m_lines)
    {
        pixel_t height =
            ln.calculate_items_position(container_main_size, justify_content, is_row_direction, self_size, fmt_ctx);
        m_pos.height = std::max(m_pos.height, height);
    }

    {
        // Out-of-flow children align to the container, not to an in-flow flex line.
        pixel_t content_height = self_size.height.type == containing_block_context::cbc_value_type_auto
                                     ? m_pos.height
                                     : self_size.height.value;
        if(self_size.min_height.type != containing_block_context::cbc_value_type_none)
        {
            content_height = std::max(content_height, self_size.min_height.value);
        }
        if(self_size.max_height.type != containing_block_context::cbc_value_type_none)
        {
            content_height = std::min(content_height, self_size.max_height.value);
        }

        for(const auto& el : m_children)
        {
            if(!is_one_of(el->css().get_position(), element_position_absolute, element_position_fixed))
            {
                continue;
            }
            auto align = el->css().get_flex_align_self();
            if(align == flex_align_items_auto)
            {
                align = css().get_flex_align_items();
            }
            pixel_t main_free  = (is_row_direction ? self_size.render_width.value : content_height) -
                                 (is_row_direction ? el->width() : el->height());
            pixel_t cross_free = (is_row_direction ? content_height : self_size.render_width.value) -
                                 (is_row_direction ? el->height() : el->width());
            pixel_t main_pos   = static_main_position(justify_content, main_free, reverse, is_row_direction);
            pixel_t cross_pos  = static_cross_position(align, cross_free, is_wrap_reverse);
            el->pos().x        = (is_row_direction ? main_pos : cross_pos) + el->content_offset_left();
            el->pos().y        = (is_row_direction ? cross_pos : main_pos) + el->content_offset_top();
        }
    }

    // calculate the final position
    m_pos.move_to(x, y);
    m_pos.x += content_offset_left();
    m_pos.y += content_offset_top();

    m_pos.width = self_size.render_width;

    return ret_width;
}

std::list<litehtml::flex_line> litehtml::render_item_flex::get_lines(
    const litehtml::containing_block_context& self_size, formatting_context* fmt_ctx, bool is_row_direction,
    pixel_t container_main_size, pixel_t main_gap, bool single_line, bool second_pass)
{
    bool reverse_main;
    bool reverse_cross = css().get_flex_wrap() == flex_wrap_wrap_reverse;

    if(is_row_direction)
    {
        reverse_main = css().get_flex_direction() == flex_direction_row_reverse;
    } else
    {
        reverse_main = css().get_flex_direction() == flex_direction_column_reverse;
    }

    std::list<flex_line>                  lines;
    flex_line                             line(reverse_main, reverse_cross, main_gap);
    std::list<std::shared_ptr<flex_item>> items;
    int                                   src_order     = 0;
    bool                                  sort_required = false;
    def_value<int>                        prev_order(0);

    for(auto& el : m_children)
    {
        if(is_one_of(el->css().get_position(), element_position_absolute, element_position_fixed))
        {
            if(!second_pass)
            {
                el->render(0_px, 0_px, self_size, fmt_ctx);
            }
            continue;
        }
        std::shared_ptr<flex_item> item = nullptr;
        if(is_row_direction)
        {
            item = std::make_shared<flex_item_row_direction>(el);
        } else
        {
            item = std::make_shared<flex_item_column_direction>(el);
        }
        item->init(self_size, css().get_flex_align_items(), fmt_ctx);
        item->src_order = src_order++;

        if(prev_order.is_default())
        {
            prev_order = item->order;
        } else if(!sort_required && item->order != prev_order)
        {
            sort_required = true;
        }

        items.emplace_back(item);
    }

    if(sort_required)
    {
        items.sort([](const std::shared_ptr<flex_item>& item1, const std::shared_ptr<flex_item>& item2) {
            if(item1->order < item2->order)
            {
                return true;
            }
            if(item1->order == item2->order)
            {
                return item1->src_order < item2->src_order;
            }
            return false;
        });
    }

    // Add flex items to lines
    for(auto& item : items)
    {
        if(!line.items.empty() && !single_line &&
           line.main_size + line.gap_size() + main_gap + item->main_size > container_main_size)
        {
            lines.emplace_back(line);
            line = flex_line(reverse_main, reverse_cross, main_gap);
        }
        line.base_size += item->base_size;
        line.main_size += item->main_size;
        if(!item->auto_margin_main_start.is_default())
        {
            line.num_auto_margin_main_start++;
        }
        if(!item->auto_margin_main_end.is_default())
        {
            line.num_auto_margin_main_end++;
        }
        line.items.push_back(item);
    }
    // Add the last line to the lines list
    if(!line.items.empty())
    {
        lines.emplace_back(line);
    }
    return lines;
}

std::shared_ptr<litehtml::render_item> litehtml::render_item_flex::init()
{
    auto                 doc = src_el()->get_document();
    decltype(m_children) new_children;
    decltype(m_children) inlines;

    auto convert_inlines = [&]() {
        if(!inlines.empty())
        {
            // Find last not space
            auto not_space =
                std::find_if(inlines.rbegin(), inlines.rend(),
                             [&](const std::shared_ptr<render_item>& el) { return !el->src_el()->is_space(); });
            if(not_space != inlines.rend())
            {
                // Erase all spaces at the end
                inlines.erase((not_space.base()), inlines.end());
            }

            auto anon_el = std::make_shared<html_tag>(src_el());
            auto anon_ri = std::make_shared<render_item_block>(anon_el);
            for(const auto& inl : inlines)
            {
                anon_ri->add_child(inl);
            }
            anon_ri->parent(shared_from_this());

            new_children.push_back(anon_ri->init());
            inlines.clear();
        }
    };

    for(const auto& el : m_children)
    {
        if(el->src_el()->css().get_display() == display_inline_text)
        {
            if(!inlines.empty())
            {
                inlines.push_back(el);
            } else
            {
                if(!el->src_el()->is_white_space())
                {
                    inlines.push_back(el);
                }
            }
        } else
        {
            convert_inlines();
            if(el->src_el()->is_block_box())
            {
                // Add block boxes as is
                el->parent(shared_from_this());
                new_children.push_back(el->init());
            } else
            {
                // Wrap inlines with anonymous block box
                auto anon_el = std::make_shared<html_tag>(el->src_el());
                auto anon_ri = std::make_shared<render_item_block>(anon_el);
                anon_ri->add_child(el->init());
                anon_ri->parent(shared_from_this());
                new_children.push_back(anon_ri->init());
            }
        }
    }
    convert_inlines();
    children() = new_children;

    return shared_from_this();
}

litehtml::pixel_t litehtml::render_item_flex::get_first_baseline()
{
    if(css().get_flex_direction() == flex_direction_row || css().get_flex_direction() == flex_direction_row_reverse)
    {
        if(!m_lines.empty())
        {
            const auto& first_line = m_lines.front();
            if(first_line.first_baseline.type() != baseline::baseline_type_none)
            {
                return first_line.cross_start + first_line.first_baseline.get_offset_from_top(first_line.cross_size) +
                       content_offset_top();
            }
            if(first_line.last_baseline.type() != baseline::baseline_type_none)
            {
                return first_line.cross_start + first_line.last_baseline.get_offset_from_top(first_line.cross_size) +
                       content_offset_top();
            }
        }
    }
    if(!m_lines.empty())
    {
        if(!m_lines.front().items.empty())
        {
            return m_lines.front().items.front()->el->get_first_baseline() + content_offset_top();
        }
    }
    return height();
}

litehtml::pixel_t litehtml::render_item_flex::get_last_baseline()
{
    if(css().get_flex_direction() == flex_direction_row || css().get_flex_direction() == flex_direction_row_reverse)
    {
        if(!m_lines.empty())
        {
            const auto& first_line = m_lines.front();
            if(first_line.last_baseline.type() != baseline::baseline_type_none)
            {
                return first_line.cross_start + first_line.last_baseline.get_offset_from_top(first_line.cross_size) +
                       content_offset_top();
            }
            if(first_line.first_baseline.type() != baseline::baseline_type_none)
            {
                return first_line.cross_start + first_line.first_baseline.get_offset_from_top(first_line.cross_size) +
                       content_offset_top();
            }
        }
    }
    if(!m_lines.empty())
    {
        if(!m_lines.front().items.empty())
        {
            return m_lines.front().items.front()->el->get_last_baseline() + content_offset_top();
        }
    }
    return height();
}

void litehtml::render_item_flex::calc_intrinsic_size()
{
    bool is_row_direction = is_one_of(css().get_flex_direction(), flex_direction_row, flex_direction_row_reverse);
    const auto& main_gap  = is_row_direction ? css().get_column_gap() : css().get_row_gap();
    bool   wraps = may_have_gap(main_gap) && css().get_flex_wrap() != flex_wrap_nowrap &&
                   (is_row_direction || !css().get_height().is_predefined() || !css().get_max_height().is_predefined());
    size_t item_count = 0;

    for(const auto& el : m_children)
    {
        if(is_one_of(el->css().get_position(), element_position_absolute, element_position_fixed))
        {
            continue;
        }
        ++item_count;
        auto child_min = el->get_intrinsic_min_size();
        auto child_max = el->get_intrinsic_max_size();

        const auto& width           = el->css().get_width();
        const bool  intrinsic_width = width.is_intrinsic_sizing() || el->css().get_min_width().is_intrinsic_sizing() ||
                                      el->css().get_max_width().is_intrinsic_sizing();
        const auto& basis           = el->css().get_flex_basis();
        const bool  fixed_basis =
            intrinsic_width && is_row_direction && !basis.is_predefined() && !basis.has_percentage();
        if(fixed_basis)
        {
            child_min.width = child_max.width = basis.calc_percent(0_px) + el->render_offset_width();
        } else if(width.is_intrinsic_sizing() && width.predef() != sizing_fit_content && !width.has_percentage())
        {
            child_min.width = child_max.width = el->resolve_intrinsic_width(width, 0_px) + el->render_offset_width();
        } else if(!width.is_predefined())
        {
            child_min.width = child_max.width =
                width.calc_percent(0_px) + (intrinsic_width ? el->render_offset_width() : el->content_offset_width());
        }
        if(intrinsic_width)
        {
            const auto known_limit = [&](const css_length& value) -> std::optional<pixel_t> {
                if(value.is_intrinsic_sizing())
                {
                    if(value.predef() == sizing_fit_content || value.has_percentage())
                    {
                        return std::nullopt;
                    }
                    return el->resolve_intrinsic_width(value, 0_px) + el->render_offset_width();
                }
                if(!value.is_predefined() && !value.has_percentage())
                {
                    return value.calc_percent(0_px) + el->render_offset_width();
                }
                return std::nullopt;
            };
            if(const auto maximum = known_limit(el->css().get_max_width()))
            {
                child_min.width = std::min(child_min.width, *maximum);
                child_max.width = std::min(child_max.width, *maximum);
            }
            if(const auto minimum = known_limit(el->css().get_min_width()))
            {
                child_min.width = std::max(child_min.width, *minimum);
                child_max.width = std::max(child_max.width, *minimum);
            } else if(fixed_basis && el->css().get_min_width().is_auto_or_none())
            {
                child_min.width = std::max(child_min.width, el->get_intrinsic_min_size().width);
                child_max.width = std::max(child_max.width, el->get_intrinsic_min_size().width);
            }
        }
        if(!el->css().get_height().is_predefined())
        {
            child_min.height = child_max.height = el->css().get_height().calc_percent(0) + el->content_offset_height();
        }

        if(is_row_direction)
        {
            // Row direction: items are laid out horizontally
            // Width accumulates (sum of all items)
            if(wraps)
            {
                m_intrinsic_min_size.width = std::max(m_intrinsic_min_size.width, child_min.width);
            } else
            {
                m_intrinsic_min_size.width += child_min.width;
            }
            m_intrinsic_max_size.width += child_max.width;
            // Height is the maximum of all items
            m_intrinsic_min_size.height = std::max(m_intrinsic_min_size.height, child_min.height);
            m_intrinsic_max_size.height = std::max(m_intrinsic_max_size.height, child_max.height);
        } else
        {
            // Column direction: items are laid out vertically
            // Width is the maximum of all items
            m_intrinsic_min_size.width = std::max(m_intrinsic_min_size.width, child_min.width);
            m_intrinsic_max_size.width = std::max(m_intrinsic_max_size.width, child_max.width);
            // Height accumulates (sum of all items)
            if(wraps)
            {
                m_intrinsic_min_size.height = std::max(m_intrinsic_min_size.height, child_min.height);
            } else
            {
                m_intrinsic_min_size.height += child_min.height;
            }
            m_intrinsic_max_size.height += child_max.height;
        }
    }

    if(item_count > 1)
    {
        const auto total_gap = main_gap.calc_percent(0_px) * static_cast<int>(item_count - 1);
        if(is_row_direction)
        {
            m_intrinsic_max_size.width += total_gap;
            if(!wraps)
            {
                m_intrinsic_min_size.width += total_gap;
            }
        } else
        {
            m_intrinsic_max_size.height += total_gap;
            if(!wraps)
            {
                m_intrinsic_min_size.height += total_gap;
            }
        }
    }

    // Add the container's own offsets (padding, borders, margins)
    m_intrinsic_min_size.width  += content_offset_width();
    m_intrinsic_max_size.width  += content_offset_width();
    m_intrinsic_min_size.height += content_offset_height();
    m_intrinsic_max_size.height += content_offset_height();
}
