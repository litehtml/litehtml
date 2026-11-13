#include "render_image.h"
#include "document.h"
#include "types.h"
#include <algorithm>
#include <optional>

litehtml::pixel_t litehtml::render_item_image::_render(pixel_t x, pixel_t y,
                                                       const containing_block_context& containing_block_size,
                                                       formatting_context* /*fmt_ctx*/, bool /*second_pass*/)
{
    pixel_t                  parent_width = containing_block_size.width;
    containing_block_context self_size    = calculate_containing_block_context(containing_block_size);

    calc_outlines(parent_width);

    m_pos.move_to(x, y);

    document::ptr doc = src_el()->get_document();

    litehtml::size sz;
    src_el()->get_content_size(sz, containing_block_size.width);

    m_pos.width  = sz.width;
    m_pos.height = sz.height;

    src_el()->css().line_height().computed_value = height();

    const auto& width                  = css().get_width();
    const auto& height                 = css().get_height();
    const auto& min_width              = css().get_min_width();
    const auto& max_width              = css().get_max_width();
    const auto& min_height             = css().get_min_height();
    const auto& max_height             = css().get_max_height();
    const bool  intrinsic_width_limit  = min_width.is_intrinsic_sizing() || max_width.is_intrinsic_sizing();
    const bool  intrinsic_height_limit = min_height.is_intrinsic_sizing() || max_height.is_intrinsic_sizing();
    if(width.is_intrinsic_sizing() || height.is_intrinsic_sizing() || intrinsic_width_limit || intrinsic_height_limit)
    {
        const bool exact_width =
            (containing_block_size.size_mode & containing_block_context::size_mode_exact_width) != 0;
        const bool exact_height =
            (containing_block_size.size_mode & containing_block_context::size_mode_exact_height) != 0;
        const bool definite_width = exact_width || !width.is_predefined();
        const bool definite_height =
            exact_height ||
            (!height.is_predefined() && self_size.height.type != containing_block_context::cbc_value_type_auto);
        const pixel_t preferred_width =
            std::max(0_px, (exact_width ? containing_block_size.width.value : width.calc_percent(parent_width)) -
                               box_sizing_width());
        const pixel_t preferred_height = std::max(0_px, self_size.height.value - box_sizing_height());
        const auto&   height_base      = containing_block_size.height_for_percentages(doc->mode());
        const bool    height_base_definite =
            height_base.type != containing_block_context::cbc_value_type_auto ||
            is_one_of(css().get_position(), element_position_absolute, element_position_fixed);
        const auto numeric_bound = [&](const css_length& value, pixel_t base, pixel_t box,
                                       bool definite) -> std::optional<pixel_t> {
            if(value.is_predefined() || (value.has_percentage() && !definite))
            {
                return std::nullopt;
            }
            return std::max(0_px, doc->to_pixels(value, css().get_font_metrics(), base) - box);
        };
        const auto numeric_min_width = numeric_bound(min_width, parent_width, box_sizing_width(), true);
        const auto numeric_max_width = numeric_bound(max_width, parent_width, box_sizing_width(), true);
        const auto numeric_min_height =
            numeric_bound(min_height, height_base.value, box_sizing_height(), height_base_definite);
        const auto numeric_max_height =
            numeric_bound(max_height, height_base.value, box_sizing_height(), height_base_definite);
        const auto clamp = [](pixel_t value, const std::optional<pixel_t>& minimum,
                              const std::optional<pixel_t>& maximum) {
            if(maximum)
            {
                value = std::min(value, *maximum);
            }
            if(minimum)
            {
                value = std::max(value, *minimum);
            }
            return value;
        };
        const auto width_from_height = [&](pixel_t value) {
            return sz.height != 0_px ? value * sz.width / sz.height : sz.width;
        };
        const auto height_from_width = [&](pixel_t value) {
            return sz.width != 0_px ? value * sz.height / sz.width : sz.height;
        };

        const pixel_t reference_width =
            clamp(definite_width ? preferred_width : sz.width,
                  min_width.is_intrinsic_sizing() ? std::optional<pixel_t>(sz.width) : numeric_min_width,
                  max_width.is_intrinsic_sizing() ? std::optional<pixel_t>(sz.width) : numeric_max_width);
        const pixel_t reference_height =
            clamp(definite_height ? preferred_height : sz.height,
                  min_height.is_intrinsic_sizing() ? std::optional<pixel_t>(sz.height) : numeric_min_height,
                  max_height.is_intrinsic_sizing() ? std::optional<pixel_t>(sz.height) : numeric_max_height);
        const auto intrinsic_width  = width_from_height(reference_height);
        const auto intrinsic_height = height_from_width(reference_width);
        m_intrinsic_content_height  = intrinsic_height;
        const std::optional<pixel_t> used_min_width =
            min_width.is_intrinsic_sizing() ? std::optional<pixel_t>(intrinsic_width) : numeric_min_width;
        const std::optional<pixel_t> used_max_width =
            max_width.is_intrinsic_sizing() ? std::optional<pixel_t>(intrinsic_width) : numeric_max_width;
        const std::optional<pixel_t> used_min_height =
            min_height.is_intrinsic_sizing() ? std::optional<pixel_t>(intrinsic_height) : numeric_min_height;
        const std::optional<pixel_t> used_max_height =
            max_height.is_intrinsic_sizing() ? std::optional<pixel_t>(intrinsic_height) : numeric_max_height;

        if(definite_width && definite_height)
        {
            m_pos.width  = clamp(preferred_width, used_min_width, used_max_width);
            m_pos.height = clamp(preferred_height, used_min_height, used_max_height);
        } else if(definite_width)
        {
            m_pos.width  = clamp(preferred_width, used_min_width, used_max_width);
            m_pos.height = clamp(height_from_width(m_pos.width), used_min_height, used_max_height);
        } else if(definite_height)
        {
            m_pos.height = clamp(preferred_height, used_min_height, used_max_height);
            m_pos.width  = clamp(width_from_height(m_pos.height), used_min_width, used_max_width);
        } else
        {
            m_pos.width  = clamp(sz.width, used_min_width, used_max_width);
            m_pos.height = height_from_width(m_pos.width);
            if(m_pos.width == sz.width)
            {
                m_pos.height = clamp(sz.height, used_min_height, used_max_height);
                m_pos.width  = width_from_height(m_pos.height);
            }
            m_pos.width  = clamp(m_pos.width, used_min_width, used_max_width);
            m_pos.height = clamp(m_pos.height, used_min_height, used_max_height);
        }

        m_pos.x += content_offset_left();
        m_pos.y += content_offset_top();
        return m_pos.width + content_offset_width();
    }

    const bool height_is_auto = src_el()->css().get_height().is_predefined() ||
                                self_size.height.type == containing_block_context::cbc_value_type_auto;

    if(height_is_auto && src_el()->css().get_width().is_predefined())
    {
        m_pos.height = sz.height;
        m_pos.width  = sz.width;

        // check for max-width
        if(!src_el()->css().get_max_width().is_predefined())
        {
            pixel_t max_width = doc->to_pixels(css().get_max_width(), css().get_font_metrics(), parent_width);
            if(m_pos.width > max_width)
            {
                m_pos.width = max_width;
            }
            if(sz.width != 0_px)
            {
                m_pos.height = m_pos.width * sz.height / sz.width;
            } else
            {
                m_pos.height = sz.height;
            }
        }

        // check for max-height
        if(!src_el()->css().get_max_height().is_predefined())
        {
            pixel_t max_height = calc_max_height(sz.height, containing_block_size.height);
            if(m_pos.height > max_height)
            {
                m_pos.height = max_height;
            }
            if(sz.height != 0_px)
            {
                m_pos.width = m_pos.height * sz.width / sz.height;
            } else
            {
                m_pos.width = sz.width;
            }
        }
    } else if(!height_is_auto && src_el()->css().get_width().is_predefined())
    {
        m_pos.height = self_size.height;

        // check for max-height
        if(!src_el()->css().get_max_height().is_predefined())
        {
            pixel_t max_height = calc_max_height(sz.height, containing_block_size.height);
            if(m_pos.height > max_height)
            {
                m_pos.height = max_height;
            }
        }

        if(sz.height != 0_px)
        {
            m_pos.width = m_pos.height * sz.width / sz.height;
        } else
        {
            m_pos.width = sz.width;
        }
    } else if(height_is_auto && !src_el()->css().get_width().is_predefined())
    {
        m_pos.width = src_el()->css().get_width().calc_percent(parent_width);

        // check for max-width
        if(!src_el()->css().get_max_width().is_predefined())
        {
            pixel_t max_width = doc->to_pixels(css().get_max_width(), css().get_font_metrics(), parent_width);
            if(m_pos.width > max_width)
            {
                m_pos.width = max_width;
            }
        }

        if(sz.width != 0_px)
        {
            m_pos.height = m_pos.width * sz.height / sz.width;
        } else
        {
            m_pos.height = sz.height;
        }
    } else
    {
        m_pos.width  = src_el()->css().get_width().calc_percent(parent_width);
        m_pos.height = self_size.height;

        // check for max-height
        if(!src_el()->css().get_max_height().is_predefined())
        {
            pixel_t max_height = calc_max_height(sz.height, containing_block_size.height);
            if(m_pos.height > max_height)
            {
                m_pos.height = max_height;
            }
        }

        // check for max-height
        if(!src_el()->css().get_max_width().is_predefined())
        {
            pixel_t max_width = doc->to_pixels(css().get_max_width(), css().get_font_metrics(), parent_width);
            if(m_pos.width > max_width)
            {
                m_pos.width = max_width;
            }
        }
    }

    m_pos.x += content_offset_left();
    m_pos.y += content_offset_top();

    return m_pos.width + content_offset_width();
}

litehtml::pixel_t litehtml::render_item_image::calc_max_height(pixel_t image_height, pixel_t containing_block_height)
{
    document::ptr doc = src_el()->get_document();
    return doc->to_pixels(css().get_max_height(), css().get_font_metrics(),
                          containing_block_height == 0_px ? image_height : containing_block_height);
}

void litehtml::render_item_image::calc_intrinsic_size()
{
    litehtml::size sz;

    src_el()->get_content_size(sz, 0_px);

    pixel_t     intrinsic_width = sz.width;
    const auto& width           = css().get_width();
    const auto& height          = css().get_height();
    if((width.is_intrinsic_sizing() || css().get_min_width().is_intrinsic_sizing() ||
        css().get_max_width().is_intrinsic_sizing()) &&
       sz.height != 0_px && !height.is_predefined() && !height.has_percentage())
    {
        const auto doc   = src_el()->get_document();
        const auto bound = [&](const css_length& value) -> std::optional<pixel_t> {
            if(value.is_intrinsic_sizing())
            {
                return sz.height;
            }
            if(value.is_predefined() || value.has_percentage())
            {
                return std::nullopt;
            }
            return std::max(0_px, doc->to_pixels(value, css().get_font_metrics(), 0_px) - box_sizing_height());
        };
        auto intrinsic_height =
            std::max(0_px, doc->to_pixels(height, css().get_font_metrics(), 0_px) - box_sizing_height());
        if(const auto maximum = bound(css().get_max_height()))
        {
            intrinsic_height = std::min(intrinsic_height, *maximum);
        }
        if(const auto minimum = bound(css().get_min_height()))
        {
            intrinsic_height = std::max(intrinsic_height, *minimum);
        }
        intrinsic_width = intrinsic_height * sz.width / sz.height;
    }

    m_intrinsic_min_size.width = m_intrinsic_max_size.width = intrinsic_width;
    // Compressible replaced element: a percentage width contributes 0 to the min-content size
    if(!css().get_width().is_predefined() && css().get_width().has_percentage())
    {
        m_intrinsic_min_size.width = 0_px;
    }
    m_intrinsic_min_size.height = m_intrinsic_max_size.height = sz.height;
}
