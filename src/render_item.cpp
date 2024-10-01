#include "html.h"
#include "render_item.h"
#include "document.h"
#include <typeinfo>
#include <utf8_strings.h>

litehtml::render_item::render_item(std::shared_ptr<element>  _src_el) :
        m_element(std::move(_src_el)),
        m_skip(false)
{
    document::ptr doc = src_el()->get_document();
	auto fm = css().get_font_metrics();

    m_margins.left		= doc->to_pixels(src_el()->css().get_margins().left,         fm, 0);
    m_margins.right		= doc->to_pixels(src_el()->css().get_margins().right,        fm, 0);
    m_margins.top		= doc->to_pixels(src_el()->css().get_margins().top,          fm, 0);
    m_margins.bottom	= doc->to_pixels(src_el()->css().get_margins().bottom,       fm, 0);

    m_padding.left		= doc->to_pixels(src_el()->css().get_padding().left,         fm, 0);
    m_padding.right		= doc->to_pixels(src_el()->css().get_padding().right,        fm, 0);
    m_padding.top		= doc->to_pixels(src_el()->css().get_padding().top,          fm, 0);
    m_padding.bottom	= doc->to_pixels(src_el()->css().get_padding().bottom,       fm, 0);

    m_borders.left		= doc->to_pixels(src_el()->css().get_borders().left.width,   fm, 0);
    m_borders.right		= doc->to_pixels(src_el()->css().get_borders().right.width,  fm, 0);
    m_borders.top		= doc->to_pixels(src_el()->css().get_borders().top.width,    fm, 0);
    m_borders.bottom	= doc->to_pixels(src_el()->css().get_borders().bottom.width, fm, 0);
}

int litehtml::render_item::render(int x, int y, const containing_block_context& containing_block_size, formatting_context* fmt_ctx, bool second_pass)
{
	int ret;

	calc_outlines(containing_block_size.width);

	m_pos.clear();
	m_pos.move_to(x, y);

	int content_left = content_offset_left();
	int content_top = content_offset_top();

	m_pos.x += content_left;
	m_pos.y += content_top;


	if(src_el()->is_block_formatting_context() || ! fmt_ctx)
	{
		formatting_context fmt;
		fmt.push_position(content_left, content_top);
		ret = _render(x, y, containing_block_size, &fmt, second_pass);
		fmt.apply_relative_shift(containing_block_size);
	} else
	{
		fmt_ctx->push_position(x + content_left, y + content_top);
		ret = _render(x, y, containing_block_size, fmt_ctx, second_pass);
		fmt_ctx->pop_position(x + content_left, y + content_top);
	}
	return ret;
}

void litehtml::render_item::calc_outlines( int parent_width )
{
    m_padding.left	= m_element->css().get_padding().left.calc_percent(parent_width);
    m_padding.right	= m_element->css().get_padding().right.calc_percent(parent_width);

    m_borders.left	= m_element->css().get_borders().left.width.calc_percent(parent_width);
    m_borders.right	= m_element->css().get_borders().right.width.calc_percent(parent_width);

    m_margins.left	= m_element->css().get_margins().left.calc_percent(parent_width);
    m_margins.right	= m_element->css().get_margins().right.calc_percent(parent_width);

    m_margins.top		= m_element->css().get_margins().top.calc_percent(parent_width);
    m_margins.bottom	= m_element->css().get_margins().bottom.calc_percent(parent_width);

    m_padding.top		= m_element->css().get_padding().top.calc_percent(parent_width);
    m_padding.bottom	= m_element->css().get_padding().bottom.calc_percent(parent_width);
}

int litehtml::render_item::calc_auto_margins(int parent_width)
{
    if ((src_el()->css().get_display() == display_block || src_el()->css().get_display() == display_table) &&
            src_el()->css().get_position() != element_position_absolute &&
            src_el()->css().get_float() == float_none)
    {
        if (src_el()->css().get_margins().left.is_predefined() && src_el()->css().get_margins().right.is_predefined())
        {
            int el_width = m_pos.width + m_borders.left + m_borders.right + m_padding.left + m_padding.right;
            if (el_width <= parent_width)
            {
                m_margins.left = (parent_width - el_width) / 2;
                m_margins.right = (parent_width - el_width) - m_margins.left;
            }
            else
            {
                m_margins.left = 0;
                m_margins.right = 0;
            }
			return m_margins.left;
        }
        else if (src_el()->css().get_margins().left.is_predefined() && !src_el()->css().get_margins().right.is_predefined())
        {
            int el_width = m_pos.width + m_borders.left + m_borders.right + m_padding.left + m_padding.right + m_margins.right;
            m_margins.left = parent_width - el_width;
            if (m_margins.left < 0) m_margins.left = 0;
			return m_margins.left;
        }
        else if (!src_el()->css().get_margins().left.is_predefined() && src_el()->css().get_margins().right.is_predefined())
        {
            int el_width = m_pos.width + m_borders.left + m_borders.right + m_padding.left + m_padding.right + m_margins.left;
            m_margins.right = parent_width - el_width;
            if (m_margins.right < 0) m_margins.right = 0;
        }
    }
	return 0;
}

void litehtml::render_item::apply_relative_shift(const containing_block_context &containing_block_size)
{
    if (src_el()->css().get_position() == element_position_relative)
    {
        css_offsets offsets = src_el()->css().get_offsets();
        if (!offsets.left.is_predefined())
        {
            m_pos.x += offsets.left.calc_percent(containing_block_size.width);
        }
        else if (!offsets.right.is_predefined())
        {
            m_pos.x -= offsets.right.calc_percent(containing_block_size.width);
        }
        if (!offsets.top.is_predefined())
        {
            m_pos.y += offsets.top.calc_percent(containing_block_size.height);
        }
        else if (!offsets.bottom.is_predefined())
        {
            m_pos.y -= offsets.bottom.calc_percent(containing_block_size.height);
        }
    }
}

std::tuple<
        std::shared_ptr<litehtml::render_item>,
        std::shared_ptr<litehtml::render_item>,
        std::shared_ptr<litehtml::render_item>
        > litehtml::render_item::split_inlines()
{
    std::tuple<
            std::shared_ptr<litehtml::render_item>,
            std::shared_ptr<litehtml::render_item>,
            std::shared_ptr<litehtml::render_item>
    > ret;
    for(const auto& child: m_children)
    {
        if(child->src_el()->is_block_box() && child->src_el()->css().get_float() == float_none)
        {
            std::get<0>(ret) = clone();
            std::get<1>(ret) = child;
            std::get<2>(ret) = clone();

            std::get<1>(ret)->parent(std::get<0>(ret));
            std::get<2>(ret)->parent(std::get<0>(ret));

            bool found = false;
            for(const auto& ch: m_children)
            {
                if(ch == child)
                {
                    found = true;
                    continue;
                }
                if(!found)
                {
                    std::get<0>(ret)->add_child(ch);
                } else
                {
                    std::get<2>(ret)->add_child(ch);
                }
            }
            break;
        }
        if(!child->children().empty())
        {
            auto child_split = child->split_inlines();
            if(std::get<0>(child_split))
            {
                std::get<0>(ret) = clone();
                std::get<1>(ret) = std::get<1>(child_split);
                std::get<2>(ret) = clone();

                std::get<2>(ret)->parent(std::get<0>(ret));

                bool found = false;
                for(const auto& ch: m_children)
                {
                    if(ch == child)
                    {
                        found = true;
                        continue;
                    }
                    if(!found)
                    {
                        std::get<0>(ret)->add_child(ch);
                    } else
                    {
                        std::get<2>(ret)->add_child(ch);
                    }
                }
                std::get<0>(ret)->add_child(std::get<0>(child_split));
                std::get<2>(ret)->add_child(std::get<2>(child_split));
                break;
            }
        }
    }
    return ret;
}

bool litehtml::render_item::fetch_positioned()
{
    bool ret = false;

    m_positioned.clear();

    litehtml::element_position el_pos;

    for(auto& el : m_children)
    {
        el_pos = el->src_el()->css().get_position();
        if (el_pos != element_position_static)
        {
            add_positioned(el);
        }
        if (!ret && (el_pos == element_position_absolute || el_pos == element_position_fixed))
        {
            ret = true;
        }
        if(el->fetch_positioned())
        {
            ret = true;
        }
    }
    return ret;
}

void litehtml::render_item::render_positioned(render_type rt)
{
    position wnd_position;
    src_el()->get_document()->container()->get_client_rect(wnd_position);

    element_position el_position;
    bool process;
    for (auto& el : m_positioned)
    {
        el_position = el->src_el()->css().get_position();

        process = false;
        if(el->src_el()->css().get_display() != display_none)
        {
            if(el_position == element_position_absolute)
            {
                if(rt != render_fixed_only)
                {
                    process = true;
                }
            } else if(el_position == element_position_fixed)
            {
                if(rt != render_no_fixed)
                {
                    process = true;
                }
            }
        }

        if(process)
        {
			containing_block_context containing_block_size;
            if(el_position == element_position_fixed || (is_root() && !src_el()->is_positioned()))
            {
				containing_block_size.height	= wnd_position.height;
				containing_block_size.width		= wnd_position.width;
            } else
            {
				containing_block_size.height	= m_pos.height + m_padding.height();
				containing_block_size.width		= m_pos.width + m_padding.width();
            }

            css_length	css_left	= el->src_el()->css().get_offsets().left;
            css_length	css_right	= el->src_el()->css().get_offsets().right;
            css_length	css_top		= el->src_el()->css().get_offsets().top;
            css_length	css_bottom	= el->src_el()->css().get_offsets().bottom;

            bool need_render = false;

            css_length el_width = el->src_el()->css().get_width();
            css_length el_height = el->src_el()->css().get_height();

			auto fix_height_min_max = [&] (int height)
			{
				auto max_height = el->css().get_max_height();
				auto min_height = el->css().get_max_height();
				if(!max_height.is_predefined())
				{
					int max_height_value = max_height.calc_percent(containing_block_size.height);
					if(height > max_height_value)
					{
						height = max_height_value;
					}
				}
				if(!min_height.is_predefined())
				{
					int min_height_value = min_height.calc_percent(containing_block_size.height);
					if(height < min_height_value)
					{
						height = min_height_value;
					}
				}
				height += el->content_offset_height();
				return height;
			};

			auto fix_width_min_max = [&] (int width)
			{
				auto max_width = el->css().get_max_width();
				auto min_width = el->css().get_min_width();
				if(!max_width.is_predefined())
				{
					int max_width_value = max_width.calc_percent(containing_block_size.width);
					if(width > max_width_value)
					{
						width = max_width_value;
					}
				}
				if(!min_width.is_predefined())
				{
					int min_width_value = min_width.calc_percent(containing_block_size.width);
					if(width < min_width_value)
					{
						width = min_width_value;
					}
				}
				width += el->content_offset_width();
				return width;
			};

			int bottom = 0;
			int top = 0;
			int height = 0;
			auto [el_static_offset_x, el_static_offset_y] = element_static_offset(el);
			int el_static_x = el->m_pos.x + el_static_offset_x;
			int el_static_y = el->m_pos.y + el_static_offset_y;
			// Calculate vertical position
			// https://www.w3.org/TR/CSS22/visudet.html#abs-non-replaced-height
			// 10.6.4 Absolutely positioned, non-replaced elements
			if(css_top.is_predefined() && !css_bottom.is_predefined() && el_height.is_predefined())
			{
				// 1. 'top' and 'height' are 'auto' and 'bottom' is not 'auto', then the height is based on the
				// content per 10.6.7, set 'auto' values for 'margin-top' and 'margin-bottom' to 0, and solve for 'top'
				if(el->css().get_margins().top.is_predefined()) el->m_margins.top = 0;
				if(el->css().get_margins().bottom.is_predefined()) el->m_margins.bottom = 0;
				height = el->height();
				bottom = css_bottom.calc_percent(containing_block_size.height);
				top = containing_block_size.height - height - bottom;
			} else if(css_top.is_predefined() && css_bottom.is_predefined() && !el_height.is_predefined())
			{
				// 2. 'top' and 'bottom' are 'auto' and 'height' is not 'auto', then set 'top' to the static position,
				// set 'auto' values for 'margin-top' and 'margin-bottom' to 0, and solve for 'bottom'
				if(el->css().get_margins().top.is_predefined()) el->m_margins.top = 0;
				if(el->css().get_margins().bottom.is_predefined()) el->m_margins.bottom = 0;
				top = el_static_y - el->content_offset_top();
				height = fix_height_min_max(el_height.calc_percent(containing_block_size.height));
			} else if(!css_top.is_predefined() && css_bottom.is_predefined() && el_height.is_predefined())
			{
				// 3. 'height' and 'bottom' are 'auto' and 'top' is not 'auto', then the height is based on the
				// content per 10.6.7, set 'auto' values for 'margin-top' and 'margin-bottom' to 0,
				// and solve for 'bottom'
				if(el->css().get_margins().top.is_predefined()) el->m_margins.top = 0;
				if(el->css().get_margins().bottom.is_predefined()) el->m_margins.bottom = 0;
				height = el->height();
				top = css_top.calc_percent(containing_block_size.height);
			} else if(css_top.is_predefined() && !css_bottom.is_predefined() && !el_height.is_predefined())
			{
				// 4. 'top' is 'auto', 'height' and 'bottom' are not 'auto', then set 'auto' values for 'margin-top'
				// and 'margin-bottom' to 0, and solve for 'top'
				if(el->css().get_margins().top.is_predefined()) el->m_margins.top = 0;
				if(el->css().get_margins().bottom.is_predefined()) el->m_margins.bottom = 0;
				height = fix_height_min_max(el_height.calc_percent(containing_block_size.height));
				bottom = css_bottom.calc_percent(containing_block_size.height);
				top = containing_block_size.height - height - bottom;
			} else if(!css_top.is_predefined() && !css_bottom.is_predefined() && el_height.is_predefined())
			{
				// 5. 'height' is 'auto', 'top' and 'bottom' are not 'auto', then 'auto' values for 'margin-top' and
				// 'margin-bottom' are set to 0 and solve for 'height'
				if(el->css().get_margins().top.is_predefined()) el->m_margins.top = 0;
				if(el->css().get_margins().bottom.is_predefined()) el->m_margins.bottom = 0;
				bottom = css_bottom.calc_percent(containing_block_size.height);
				top = css_top.calc_percent(containing_block_size.height);
				if(el->src_el()->is_replaced())
				{
					height = el->height() - el->content_offset_height();
					int reminded = (containing_block_size.height - top - bottom) - height - el->content_offset_height();
					if(reminded > 0)
					{
						int divider = 0;
						if (el->css().get_margins().top.is_predefined()) divider++;
						if (el->css().get_margins().bottom.is_predefined()) divider++;
						if (divider != 0)
						{
							if (el->css().get_margins().top.is_predefined()) el->m_margins.top = reminded / divider;
							if (el->css().get_margins().bottom.is_predefined()) el->m_margins.bottom = reminded / divider;
						}
					}
					height += el->content_offset_height();
				} else
				{
					height = containing_block_size.height - top - bottom;
				}
				if(!el->css().get_max_height().is_predefined())
				{
					int max_height = el->css().get_max_height().calc_percent(containing_block_size.height);
					if(height - el->content_offset_height() > max_height)
					{
						int reminded = height - el->content_offset_height() - max_height;
						height = max_height;
						int divider = 0;
						if(el->css().get_margins().top.is_predefined()) divider++;
						if(el->css().get_margins().bottom.is_predefined()) divider++;
						if(divider != 0)
						{
							if(el->css().get_margins().top.is_predefined()) el->m_margins.top = reminded / divider;
							if(el->css().get_margins().bottom.is_predefined()) el->m_margins.bottom = reminded / divider;
						}
						height += el->content_offset_height();
					}
				}
			} else if(!css_top.is_predefined() && css_bottom.is_predefined() && !el_height.is_predefined())
			{
				// 6. 'bottom' is 'auto', 'top' and 'height' are not 'auto', then set 'auto' values for 'margin-top'
				// and 'margin-bottom' to 0 and solve for 'bottom'
				if(el->css().get_margins().top.is_predefined()) el->m_margins.top = 0;
				if(el->css().get_margins().bottom.is_predefined()) el->m_margins.bottom = 0;
				height = fix_height_min_max(el_height.calc_percent(containing_block_size.height));
				top = css_top.calc_percent(containing_block_size.height);
			} else if(css_top.is_predefined() && css_bottom.is_predefined() && el_height.is_predefined())
			{
				// If all three of 'top', 'height', and 'bottom' are auto, set 'top' to the static position and
				// apply rule number three.
				if(el->css().get_margins().top.is_predefined()) el->m_margins.top = 0;
				if(el->css().get_margins().bottom.is_predefined()) el->m_margins.bottom = 0;
				height = el->height();
				top = el_static_y - el->content_offset_top();
			} else
			{
				// If none of the three are 'auto':
				height = fix_height_min_max(el_height.calc_percent(containing_block_size.height));
				top = css_top.calc_percent(containing_block_size.height);
				bottom = css_bottom.calc_percent(containing_block_size.height);
				int remained = containing_block_size.height - height - top - bottom;

				if(el->css().get_margins().top.is_predefined() && el->css().get_margins().bottom.is_predefined())
				{
					// If both 'margin-top' and 'margin-bottom' are 'auto', solve the equation under the extra
					// constraint that the two margins get equal values.
					el->m_margins.top = el->m_margins.bottom = remained / 2;
					height += el->m_margins.top + el->m_margins.bottom;
				} else
				{
					// If one of 'margin-top' or 'margin-bottom' is 'auto', solve the equation for that value.
					if(el->css().get_margins().top.is_predefined())
					{
						el->m_margins.top = remained;
						height += el->m_margins.top;
					}
					if(el->css().get_margins().bottom.is_predefined())
					{
						el->m_margins.bottom = remained;
						height += el->m_margins.bottom;
					}
				}
			}
			el->m_pos.y = top + el->content_offset_top();
			if(el->m_pos.height != height - el->content_offset_height())
			{
				el->m_pos.height = height - el->content_offset_height();
				need_render = true;
			}

			// Calculate horizontal position
			int right = 0;
			int left = 0;
			int width = 0;
			// https://www.w3.org/TR/CSS22/visudet.html#abs-non-replaced-width
			// 10.3.7 Absolutely positioned, non-replaced elements
			if(css_left.is_predefined() && !css_right.is_predefined() && el_width.is_predefined())
			{
				// 1. 'left' and 'width' are 'auto' and 'right' is not 'auto', then the width is shrink-to-fit.
				// Then solve for 'left'
				if(el->css().get_margins().left.is_predefined()) el->m_margins.left = 0;
				if(el->css().get_margins().right.is_predefined()) el->m_margins.right = 0;
				width = el->width();
				right = css_right.calc_percent(containing_block_size.width);
				left = containing_block_size.width - width - right;
			} else if(css_left.is_predefined() && css_right.is_predefined() && !el_width.is_predefined())
			{
				// 2. 'left' and 'right' are 'auto' and 'width' is not 'auto', then if the 'direction' property of
				// the element establishing the static-position containing block is 'ltr' set 'left' to the
				// static position, otherwise set 'right' to the static position. Then solve for 'left'
				// (if 'direction is 'rtl') or 'right' (if 'direction' is 'ltr').
				if(el->css().get_margins().left.is_predefined()) el->m_margins.left = 0;
				if(el->css().get_margins().right.is_predefined()) el->m_margins.right = 0;
				left = el_static_x - el->content_offset_left();
				width = fix_width_min_max(el_width.calc_percent(containing_block_size.width));
			} else if(!css_left.is_predefined() && css_right.is_predefined() && el_width.is_predefined())
			{
				// 3. 'width' and 'right' are 'auto' and 'left' is not 'auto', then the width is shrink-to-fit .
				// Then solve for 'right'
				if(el->css().get_margins().left.is_predefined()) el->m_margins.left = 0;
				if(el->css().get_margins().right.is_predefined()) el->m_margins.right = 0;
				width = el->width();
				left = css_left.calc_percent(containing_block_size.width);
			} else if(css_left.is_predefined() && !css_right.is_predefined() && !el_width.is_predefined())
			{
				// 4. 'left' is 'auto', 'width' and 'right' are not 'auto', then solve for 'left'
				if(el->css().get_margins().left.is_predefined()) el->m_margins.left = 0;
				if(el->css().get_margins().right.is_predefined()) el->m_margins.right = 0;
				right = css_right.calc_percent(containing_block_size.width);
				width = fix_width_min_max(el_width.calc_percent(containing_block_size.width));
				left = containing_block_size.width - right - width;
			} else if(!css_left.is_predefined() && !css_right.is_predefined() && el_width.is_predefined())
			{
				// 5. 'width' is 'auto', 'left' and 'right' are not 'auto', then solve for 'width'
				if(el->css().get_margins().left.is_predefined()) el->m_margins.left = 0;
				if(el->css().get_margins().right.is_predefined()) el->m_margins.right = 0;
				left = css_left.calc_percent(containing_block_size.width);
				right = css_right.calc_percent(containing_block_size.width);
				if(el->src_el()->is_replaced())
				{
					width = el->width() - el->content_offset_width();
					int reminded = (containing_block_size.width - left - right) - width - el->content_offset_width();
					if(reminded)
					{
						int divider = 0;
						if (el->css().get_margins().left.is_predefined()) divider++;
						if (el->css().get_margins().right.is_predefined()) divider++;
						if (divider != 0)
						{
							if (el->css().get_margins().left.is_predefined()) el->m_margins.left = reminded / divider;
							if (el->css().get_margins().right.is_predefined()) el->m_margins.right = reminded / divider;
						}
					}
					width += el->content_offset_width();
				} else
				{
					width = containing_block_size.width - left - right;
				}
				if(!el->css().get_max_width().is_predefined())
				{
					int max_width = el->css().get_max_width().calc_percent(containing_block_size.height);
					if(width - el->content_offset_width() > max_width)
					{
						int reminded = width - el->content_offset_width() - max_width;
						width = max_width;
						int divider = 0;
						if(el->css().get_margins().left.is_predefined()) divider++;
						if(el->css().get_margins().right.is_predefined()) divider++;
						if(divider != 0)
						{
							if(el->css().get_margins().left.is_predefined()) el->m_margins.left = reminded / divider;
							if(el->css().get_margins().right.is_predefined()) el->m_margins.right = reminded / divider;
						}
						width += el->content_offset_width();
					}
				}
			} else if(!css_left.is_predefined() && css_right.is_predefined() && !el_width.is_predefined())
			{
				// 6. 'right' is 'auto', 'left' and 'width' are not 'auto', then solve for 'right'
				if(el->css().get_margins().left.is_predefined()) el->m_margins.left = 0;
				if(el->css().get_margins().right.is_predefined()) el->m_margins.right = 0;
				left = css_left.calc_percent(containing_block_size.width);
				width = fix_width_min_max(el_width.calc_percent(containing_block_size.width));
			} else if(css_left.is_predefined() && css_right.is_predefined() && el_width.is_predefined())
			{
				// If all three of 'left', 'width', and 'right' are 'auto': First set any 'auto' values for
				// 'margin-left' and 'margin-right' to 0. Then, if the 'direction' property of the element
				// establishing the static-position containing block is 'ltr' set 'left' to the static position
				// and apply rule number three below; otherwise, set 'right' to the static position and apply
				// rule number one below.
				if(el->css().get_margins().left.is_predefined()) el->m_margins.left = 0;
				if(el->css().get_margins().right.is_predefined()) el->m_margins.right = 0;
				width = el->width();
				left = el_static_x - el->content_offset_left();
			} else
			{
				// If none of the three is 'auto':
				width = fix_width_min_max(el_width.calc_percent(containing_block_size.width));
				left = css_left.calc_percent(containing_block_size.width);
				right = css_right.calc_percent(containing_block_size.width);
				int remained = containing_block_size.width - width - left - right;

				if(el->css().get_margins().left.is_predefined() && el->css().get_margins().right.is_predefined())
				{
					// If both 'margin-left' and 'margin-right' are 'auto', solve the equation under the extra
					// constraint that the two margins get equal values, unless this would make them negative,
					// in which case when direction of the containing block is 'ltr' ('rtl'), set 'margin-left'
					// ('margin-right') to zero and solve for 'margin-right' ('margin-left').
					el->m_margins.left = el->m_margins.right = remained / 2;
					if(el->m_margins.left < 0)
					{
						el->m_margins.left = 0;
						el->m_margins.right = remained;
					}
					width += el->m_margins.left + el->m_margins.right;
				} else
				{
					// If one of 'margin-left' or 'margin-right' is 'auto', solve the equation
					// for that value. If the values are over-constrained, ignore the value for 'left'
					// (in case the 'direction' property of the containing block is 'rtl') or 'right' (in case
					// 'direction' is 'ltr') and solve for that value.
					if(el->css().get_margins().left.is_predefined())
					{
						el->m_margins.left = remained;
						width += el->m_margins.left;
					}
					if(el->css().get_margins().right.is_predefined())
					{
						el->m_margins.right = remained;
						width += el->m_margins.right;
					}
				}
			}
			el->m_pos.x = left + el->content_offset_left();
			if(el->m_pos.width != width - el->content_offset_width())
			{
				el->m_pos.width = width - el->content_offset_width();
				need_render = true;
			}

            if(el_position != element_position_fixed)
            {
				el->m_pos.x -= el_static_offset_x;
				el->m_pos.y -= el_static_offset_y;
            }

            if(need_render)
            {
                position pos = el->m_pos;
				el->render(el->left(), el->top(), containing_block_size.new_width(el->width()), nullptr, true);
                el->m_pos = pos;
            }

            if(el_position == element_position_fixed)
            {
                position fixed_pos;
                el->get_redraw_box(fixed_pos);
                src_el()->get_document()->add_fixed_box(fixed_pos);
            }
        }

		el->render_positioned();
    }

    if(!m_positioned.empty())
    {
        std::stable_sort(m_positioned.begin(), m_positioned.end(), [](const std::shared_ptr<render_item>& Left, const std::shared_ptr<render_item>& Right)
            {
                return (Left->src_el()->css().get_z_index() < Right->src_el()->css().get_z_index());
            });
    }
}

void litehtml::render_item::add_positioned(const std::shared_ptr<litehtml::render_item> &el)
{
    if (src_el()->css().get_position() != element_position_static || is_root())
    {
        m_positioned.push_back(el);
    } else
    {
        auto el_parent = parent();
        if (el_parent)
        {
            el_parent->add_positioned(el);
        }
    }
}

void litehtml::render_item::get_redraw_box(litehtml::position& pos, int x /*= 0*/, int y /*= 0*/)
{
    if(is_visible())
    {
        int p_left		= std::min(pos.left(),	x + m_pos.left() - m_padding.left - m_borders.left);
        int p_right		= std::max(pos.right(), x + m_pos.right() + m_padding.left + m_borders.left);
        int p_top		= std::min(pos.top(), y + m_pos.top() - m_padding.top - m_borders.top);
        int p_bottom	= std::max(pos.bottom(), y + m_pos.bottom() + m_padding.bottom + m_borders.bottom);

        pos.x = p_left;
        pos.y = p_top;
        pos.width	= p_right - p_left;
        pos.height	= p_bottom - p_top;

        if(src_el()->css().get_overflow() == overflow_visible)
        {
            for(auto& el : m_children)
            {
                if(el->src_el()->css().get_position() != element_position_fixed)
                {
                    el->get_redraw_box(pos, x + m_pos.x, y + m_pos.y);
                }
            }
        }
    }
}

void litehtml::render_item::calc_document_size( litehtml::size& sz, litehtml::size& content_size, int x /*= 0*/, int y /*= 0*/ )
{
	if(css().get_display() != display_inline && css().get_display() != display_table_row)
	{
		if (is_visible() && src_el()->css().get_position() != element_position_fixed)
		{
			sz.width = std::max(sz.width, x + right());
			sz.height = std::max(sz.height, y + bottom());

			if (!src_el()->is_root() && !src_el()->is_body())
			{
				content_size.width = std::max(content_size.width, x + right());
				content_size.height = std::max(content_size.height, y + bottom());
			}

			// All children of tables and blocks with style other than "overflow: visible" are inside element.
			// We can skip calculating size of children
			if (src_el()->css().get_overflow() == overflow_visible && src_el()->css().get_display() != display_table)
			{
				for (auto &el: m_children)
				{
					el->calc_document_size(sz, content_size, x + m_pos.x, y + m_pos.y);
				}
			}

			if (src_el()->is_root() || src_el()->is_body())
			{
				content_size.width = std::max(content_size.width, x + right());
				content_size.height = std::max(content_size.height, y + bottom());
			}
		}
	} else
	{
		position::vector boxes;
		get_inline_boxes(boxes);
		for(auto& box : boxes)
		{
			content_size.width = std::max(content_size.width, x + box.x + box.width);
			content_size.height = std::max(content_size.height, y + box.y + box.height);
		}
	}
}

void litehtml::render_item::draw_stacking_context( uint_ptr hdc, int x, int y, const position* clip, bool with_positioned )
{
    if(!is_visible()) return;

    std::map<int, bool> z_indexes;
    if(with_positioned)
    {
        for(const auto& idx : m_positioned)
        {
            z_indexes[idx->src_el()->css().get_z_index()];
        }

        for(const auto& idx : z_indexes)
        {
            if(idx.first < 0)
            {
                draw_children(hdc, x, y, clip, draw_positioned, idx.first);
            }
        }
    }
    draw_children(hdc, x, y, clip, draw_block, 0);
    draw_children(hdc, x, y, clip, draw_floats, 0);
    draw_children(hdc, x, y, clip, draw_inlines, 0);
    if(with_positioned)
    {
        for(auto& z_index : z_indexes)
        {
            if(z_index.first == 0)
            {
                draw_children(hdc, x, y, clip, draw_positioned, z_index.first);
            }
        }

        for(auto& z_index : z_indexes)
        {
            if(z_index.first > 0)
            {
                draw_children(hdc, x, y, clip, draw_positioned, z_index.first);
            }
        }
    }
}

void litehtml::render_item::draw_children(uint_ptr hdc, int x, int y, const position* clip, draw_flag flag, int zindex)
{
    position pos = m_pos;
    pos.x += x;
    pos.y += y;

    document::ptr doc = src_el()->get_document();

    if (src_el()->css().get_overflow() > overflow_visible)
    {
        // TODO: Process overflow for inline elements
        if(src_el()->css().get_display() != display_inline)
        {
            position border_box = pos;
            border_box += m_padding;
            border_box += m_borders;

            border_radiuses bdr_radius = src_el()->css().get_borders().radius.calc_percents(border_box.width,
                                                                                            border_box.height);

            bdr_radius -= m_borders;
            bdr_radius -= m_padding;

            doc->container()->set_clip(pos, bdr_radius);
        }
    }

    for (const auto& el : m_children)
    {
        if (el->is_visible())
        {
            bool process = true;
            switch (flag)
            {
                case draw_positioned:
                    if (el->src_el()->is_positioned() && el->src_el()->css().get_z_index() == zindex)
                    {
                        if (el->src_el()->css().get_position() == element_position_fixed)
                        {
                            position browser_wnd;
                            doc->container()->get_client_rect(browser_wnd);

                            el->src_el()->draw(hdc, browser_wnd.x, browser_wnd.y, clip, el);
                            el->draw_stacking_context(hdc, browser_wnd.x, browser_wnd.y, clip, true);
                        }
                        else
                        {
                            el->src_el()->draw(hdc, pos.x, pos.y, clip, el);
                            el->draw_stacking_context(hdc, pos.x, pos.y, clip, true);
                        }
                        process = false;
                    }
                    break;
                case draw_block:
                    if (!el->src_el()->is_inline() && el->src_el()->css().get_float() == float_none && !el->src_el()->is_positioned())
                    {
                        el->src_el()->draw(hdc, pos.x, pos.y, clip, el);
                    }
                    break;
                case draw_floats:
                    if (el->src_el()->css().get_float() != float_none && !el->src_el()->is_positioned())
                    {
                        el->src_el()->draw(hdc, pos.x, pos.y, clip, el);
                        el->draw_stacking_context(hdc, pos.x, pos.y, clip, false);
                        process = false;
                    }
                    break;
                case draw_inlines:
                    if (el->src_el()->is_inline() && el->src_el()->css().get_float() == float_none && !el->src_el()->is_positioned())
                    {
                        el->src_el()->draw(hdc, pos.x, pos.y, clip, el);
                        if (el->src_el()->css().get_display() == display_inline_block || el->src_el()->css().get_display() == display_inline_flex)
                        {
                            el->draw_stacking_context(hdc, pos.x, pos.y, clip, false);
                            process = false;
                        }
                    }
                    break;
                default:
                    break;
            }

            if (process)
            {
                if (flag == draw_positioned)
                {
                    if (!el->src_el()->is_positioned())
                    {
                        el->draw_children(hdc, pos.x, pos.y, clip, flag, zindex);
                    }
                }
                else
                {
                    if (el->src_el()->css().get_float() == float_none &&
                        el->src_el()->css().get_display() != display_inline_block &&
                        !el->src_el()->is_positioned())
                    {
                        el->draw_children(hdc, pos.x, pos.y, clip, flag, zindex);
                    }
                }
            }
        }
    }

    if (src_el()->css().get_overflow() > overflow_visible)
    {
        doc->container()->del_clip();
    }
}

std::shared_ptr<litehtml::element>  litehtml::render_item::get_child_by_point(int x, int y, int client_x, int client_y, draw_flag flag, int zindex)
{
    element::ptr ret = nullptr;

    if(src_el()->css().get_overflow() > overflow_visible)
    {
        if(!m_pos.is_point_inside(x, y))
        {
            return ret;
        }
    }

    position el_pos = m_pos;
    el_pos.x	= x - el_pos.x;
    el_pos.y	= y - el_pos.y;

    for(auto i = m_children.rbegin(); i != m_children.rend() && !ret; std::advance(i, 1))
    {
        auto el = (*i);

        if(el->is_visible() && el->src_el()->css().get_display() != display_inline_text)
        {
            switch(flag)
            {
                case draw_positioned:
                    if(el->src_el()->is_positioned() && el->src_el()->css().get_z_index() == zindex)
                    {
                        if(el->src_el()->css().get_position() == element_position_fixed)
                        {
                            ret = el->get_element_by_point(client_x, client_y, client_x, client_y);
                            if(!ret && (*i)->is_point_inside(client_x, client_y))
                            {
                                ret = (*i)->src_el();
                            }
                        } else
                        {
                            ret = el->get_element_by_point(el_pos.x, el_pos.y, client_x, client_y);
                            if(!ret && (*i)->is_point_inside(el_pos.x, el_pos.y))
                            {
                                ret = (*i)->src_el();
                            }
                        }
                        el = nullptr;
                    }
                    break;
                case draw_block:
                    if(!el->src_el()->is_inline() && el->src_el()->css().get_float() == float_none && !el->src_el()->is_positioned())
                    {
                        if(el->is_point_inside(el_pos.x, el_pos.y))
                        {
                            ret = el->src_el();
                        }
                    }
                    break;
                case draw_floats:
                    if(el->src_el()->css().get_float() != float_none && !el->src_el()->is_positioned())
                    {
                        ret = el->get_element_by_point(el_pos.x, el_pos.y, client_x, client_y);

                        if(!ret && (*i)->is_point_inside(el_pos.x, el_pos.y))
                        {
                            ret = (*i)->src_el();
                        }
                        el = nullptr;
                    }
                    break;
                case draw_inlines:
                    if(el->src_el()->is_inline() && el->src_el()->css().get_float() == float_none && !el->src_el()->is_positioned())
                    {
                        if(el->src_el()->css().get_display() == display_inline_block ||
                                el->src_el()->css().get_display() == display_inline_table ||
                                el->src_el()->css().get_display() == display_inline_flex)
                        {
                            ret = el->get_element_by_point(el_pos.x, el_pos.y, client_x, client_y);
                            el = nullptr;
                        }
                        if(!ret && (*i)->is_point_inside(el_pos.x, el_pos.y))
                        {
                            ret = (*i)->src_el();
                        }
                    }
                    break;
                default:
                    break;
            }

            if(el && !el->src_el()->is_positioned())
            {
                if(flag == draw_positioned)
                {
                    element::ptr child = el->get_child_by_point(el_pos.x, el_pos.y, client_x, client_y, flag, zindex);
                    if(child)
                    {
                        ret = child;
                    }
                } else
                {
                    if(	el->src_el()->css().get_float() == float_none &&
                           el->src_el()->css().get_display() != display_inline_block && el->src_el()->css().get_display() != display_inline_flex)
                    {
                        element::ptr child = el->get_child_by_point(el_pos.x, el_pos.y, client_x, client_y, flag, zindex);
                        if(child)
                        {
                            ret = child;
                        }
                    }
                }
            }
        }
    }

    return ret;
}

std::shared_ptr<litehtml::element> litehtml::render_item::get_element_by_point(int x, int y, int client_x, int client_y)
{
    if(!is_visible()) return nullptr;

    element::ptr ret;

    std::map<int, bool> z_indexes;

    for(const auto& i : m_positioned)
    {
        z_indexes[i->src_el()->css().get_z_index()];
    }

    for(auto iter = z_indexes.rbegin(); iter != z_indexes.rend(); iter++)
    {
        if(iter->first > 0)
        {
            ret = get_child_by_point(x, y, client_x, client_y, draw_positioned, iter->first);
			if(ret) return ret;
        }
    }

    for(const auto& z_index : z_indexes)
    {
        if(z_index.first == 0)
        {
            ret = get_child_by_point(x, y, client_x, client_y, draw_positioned, z_index.first);
			if(ret) return ret;
        }
    }

    ret = get_child_by_point(x, y, client_x, client_y, draw_inlines, 0);
    if(ret) return ret;

    ret = get_child_by_point(x, y, client_x, client_y, draw_floats, 0);
    if(ret) return ret;

    ret = get_child_by_point(x, y, client_x, client_y, draw_block, 0);
    if(ret) return ret;


	for(auto iter = z_indexes.rbegin(); iter != z_indexes.rend(); iter++)
	{
        if(iter->first < 0)
        {
            ret = get_child_by_point(x, y, client_x, client_y, draw_positioned, iter->first);
			if(ret) return ret;
        }
    }

    if(src_el()->css().get_position() == element_position_fixed)
    {
        if(is_point_inside(client_x, client_y))
        {
            ret = src_el();
        }
    } else
    {
        if(is_point_inside(x, y))
        {
            ret = src_el();
        }
    }

    return ret;
}

bool litehtml::render_item::is_point_inside( int x, int y )
{
	if(src_el()->css().get_display() != display_inline && src_el()->css().get_display() != display_table_row)
	{
		position pos = m_pos;
		pos += m_padding;
		pos += m_borders;
		if(pos.is_point_inside(x, y))
		{
			return true;
		} else
		{
			return false;
		}
	} else
	{
		position::vector boxes;
		get_inline_boxes(boxes);
		for(auto & box : boxes)
		{
			if(box.is_point_inside(x, y))
			{
				return true;
			}
		}
	}
    return false;
}

void litehtml::render_item::get_rendering_boxes( position::vector& redraw_boxes)
{
    if(src_el()->css().get_display() == display_inline || src_el()->css().get_display() == display_table_row)
    {
        get_inline_boxes(redraw_boxes);
    } else
    {
        position pos = m_pos;
        pos += m_padding;
        pos += m_borders;
        redraw_boxes.push_back(pos);
    }

    if(src_el()->css().get_position() != element_position_fixed)
    {
        auto cur_el = parent();
        while(cur_el)
        {
            for(auto& box : redraw_boxes)
            {
                box.x += cur_el->m_pos.x;
                box.y += cur_el->m_pos.y;
            }
            cur_el = cur_el->parent();
        }
    }
}

void litehtml::render_item::dump(litehtml::dumper& cout)
{
    cout.begin_node(src_el()->dump_get_name() + "{" + string(typeid(*this).name()) + "}");

    auto attrs = src_el()->dump_get_attrs();
    if(!attrs.empty())
    {
        cout.begin_attrs_group("attributes");
        for (const auto &attr: attrs)
        {
            cout.add_attr(std::get<0>(attr), std::get<1>(attr));
        }
        cout.end_attrs_group();
    }

    if(!m_children.empty())
    {
        cout.begin_attrs_group("children");
        for (const auto &el: m_children)
        {
            el->dump(cout);
        }
        cout.end_attrs_group();
    }

    cout.end_node();
}

litehtml::position litehtml::render_item::get_placement() const
{
	litehtml::position pos = m_pos;
	auto cur_el = parent();
	while(cur_el)
	{
		pos.x += cur_el->m_pos.x;
		pos.y += cur_el->m_pos.y;
		cur_el = cur_el->parent();
	}
	return pos;
}

std::shared_ptr<litehtml::render_item> litehtml::render_item::init()
{
    src_el()->add_render(shared_from_this());

    for(auto& el : children())
    {
        el = el->init();
    }

    return shared_from_this();
}

void litehtml::render_item::calc_cb_length(const css_length& len, int percent_base, containing_block_context::typed_int& out_value) const
{
	if (!len.is_predefined())
	{
		if(len.units() == litehtml::css_units_percentage)
		{
			out_value.value = len.calc_percent(percent_base);
			out_value.type = litehtml::containing_block_context::cbc_value_type_percentage;
		} else
		{
			out_value.value = src_el()->get_document()->to_pixels(len, css().get_font_metrics(), 0);
			out_value.type = containing_block_context::cbc_value_type_absolute;
		}
	}
}

litehtml::containing_block_context litehtml::render_item::calculate_containing_block_context(const containing_block_context& cb_context)
{
	containing_block_context ret;
	ret.context_idx = cb_context.context_idx + 1;
	ret.width.value = ret.max_width.value = cb_context.width.value - content_offset_width();
	if(src_el()->css().get_position() != element_position_absolute && src_el()->css().get_position() != element_position_fixed)
	{
		ret.height.value = cb_context.height.value - content_offset_height();
	}

	// Calculate width if css property is not auto
	// We have to use aut value for display_table_cell also.
	if (src_el()->css().get_display() != display_table_cell)
	{
		auto par = parent();
		if(cb_context.size_mode & containing_block_context::size_mode_exact_width)
		{
			ret.width.value = cb_context.width;
			ret.width.type = containing_block_context::cbc_value_type_absolute;
		} else
		{
			auto *width = &css().get_width();
			if(par && (par->css().get_display() == display_flex || par->css().get_display() == display_inline_flex))
			{
				if(!css().get_flex_basis().is_predefined() && css().get_flex_basis().val() >= 0)
				{
					if(par->css().get_flex_direction() == flex_direction_row || par->css().get_flex_direction() == flex_direction_row_reverse)
					{
						ret.width.type = containing_block_context::cbc_value_type_auto;
						ret.width.value = 0;
						width = nullptr;
					}
				}
			}
			if(width)
			{
				calc_cb_length(*width, cb_context.width, ret.width);
			}
		}
		if(cb_context.size_mode & containing_block_context::size_mode_exact_height)
		{
			ret.height.value = cb_context.height;
			ret.height.type = containing_block_context::cbc_value_type_absolute;
		} else
		{
			auto *height = &css().get_height();
			if(par && (par->css().get_display() == display_flex || par->css().get_display() == display_inline_flex))
			{
				if(!css().get_flex_basis().is_predefined() && css().get_flex_basis().val() >= 0)
				{
					if(par->css().get_flex_direction() == flex_direction_column || par->css().get_flex_direction() == flex_direction_column_reverse)
					{
						ret.height.type = containing_block_context::cbc_value_type_auto;
						ret.height.value = 0;
						height = nullptr;
					}
				}
			}
			if(height)
			{
				calc_cb_length(*height, cb_context.height, ret.height);
			}
		}
		if (ret.width.type != containing_block_context::cbc_value_type_auto && (src_el()->css().get_display() == display_table || src_el()->is_root()))
		{
			ret.width.value -= content_offset_width();
		}
		if (ret.height.type != containing_block_context::cbc_value_type_auto && (src_el()->css().get_display() == display_table || src_el()->is_root()))
		{
			ret.height.value -= content_offset_height();
		}
	}
	ret.render_width = ret.width;

	calc_cb_length(src_el()->css().get_min_width(), cb_context.width, ret.min_width);
	calc_cb_length(src_el()->css().get_max_width(), cb_context.width, ret.max_width);

	calc_cb_length(src_el()->css().get_min_height(), cb_context.height, ret.min_height);
	calc_cb_length(src_el()->css().get_max_height(), cb_context.height, ret.max_height);

	// Fix box sizing
	if(ret.width.type != containing_block_context::cbc_value_type_auto)
	{
		ret.render_width = ret.width - box_sizing_width();
	}
	if(ret.min_width.type != containing_block_context::cbc_value_type_none)
	{
		ret.min_width.value -= box_sizing_width();
	}
	if(ret.max_width.type != containing_block_context::cbc_value_type_none)
	{
		ret.max_width.value -= box_sizing_width();
	}
	if(ret.min_height.type != containing_block_context::cbc_value_type_none)
	{
		ret.min_height.value -= box_sizing_height();
	}
	if(ret.max_height.type != containing_block_context::cbc_value_type_none)
	{
		ret.max_height.value -= box_sizing_height();
	}

	return ret;
}

std::tuple<int, int> litehtml::render_item::element_static_offset(const std::shared_ptr<litehtml::render_item>& el)
{
	int offset_x = 0;
	int offset_y = 0;
	auto cur_el = el->parent();
	auto this_el = el->css().get_position() != element_position_fixed ? shared_from_this() : src_el()->get_document()->root_render();
	while(cur_el && cur_el != this_el)
	{
		offset_x += cur_el->m_pos.x;
		offset_y += cur_el->m_pos.y;
		cur_el = cur_el->parent();
	}

	if(el->css().get_position() == element_position_fixed || (is_root() && !src_el()->is_positioned()))
	{
		offset_x += this_el->m_pos.x;
		offset_y += this_el->m_pos.y;
	} else
	{
		offset_x += m_padding.left;
		offset_y += m_padding.top;
	}

	return {offset_x, offset_y};
}
