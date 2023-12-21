#include "html.h"
#include "types.h"
#include "render_flex.h"

int litehtml::render_item_flex::_render_content(int x, int y, bool second_pass, const containing_block_context &self_size, formatting_context* fmt_ctx)
{
	bool is_row_direction = true;
	bool reverse = false;
	int container_main_size = self_size.render_width;

	switch (css().get_flex_direction())
	{
		case flex_direction_column:
			is_row_direction = false;
			reverse = false;
			container_main_size = self_size.height;
			if (css().get_box_sizing() == box_sizing_border_box)
			{
				container_main_size -= box_sizing_height();
			}
			break;
		case flex_direction_column_reverse:
			is_row_direction = false;
			reverse = true;
			container_main_size = self_size.height;
			if (css().get_box_sizing() == box_sizing_border_box)
			{
				container_main_size -= box_sizing_height();
			}
			break;
		case flex_direction_row:
			is_row_direction = true;
			reverse = false;
			break;
		case flex_direction_row_reverse:
			is_row_direction = true;
			reverse = true;
			break;
	}

	// Split flex items to lines
	std::list<flex_line> lines = get_lines(self_size, fmt_ctx, is_row_direction, container_main_size);

	// Resolving Flexible Lengths
	// REF: https://www.w3.org/TR/css-flexbox-1/#resolve-flexible-lengths

	int el_y = 0;
	int el_x = 0;
	int sum_cross_size = 0;
	int ret_width = 0;
	for(auto& ln : lines)
	{
		ln.cross_size = 0;
		ln.main_size =0;

		if(is_row_direction)
		{
			ret_width += ln.base_size;
		}

		ln.distribute_free_space(container_main_size);

		if(is_row_direction)
		{
			// render items into new size and find line cross_size
			for (auto &item: ln.items)
			{
				ln.main_size += item.main_size;

				item.el->render(el_x,
								el_y,
								self_size.new_width(item.main_size), fmt_ctx, false);
				ln.cross_size = std::max(ln.cross_size, item.el->height());
				el_x += item.el->width();
			}
			sum_cross_size += ln.cross_size;
			el_x = 0;
		} else
		{
			for (auto &item: ln.items)
			{
				ln.main_size += item.main_size;

				int el_ret_width = item.el->render(el_x,
								el_y,
								self_size, fmt_ctx, false);
				item.el->render(el_x,
								el_y,
								self_size.new_width(el_ret_width), fmt_ctx, false);
				// TODO: must be rendered into the specified height
				item.el->pos().height = item.main_size - item.el->content_offset_height();
				ln.cross_size = std::max(ln.cross_size, item.el->width());
				el_y += item.el->height();
			}
			sum_cross_size += ln.cross_size;
			el_y = 0;
		}
	}

	int free_cross_size = 0;
	int add_cross_size = 0;
	if(sum_cross_size)
	{
		if (is_row_direction)
		{
			if (self_size.height.type != containing_block_context::cbc_value_type_auto)
			{
				free_cross_size = self_size.height;
				if (src_el()->css().get_box_sizing() == box_sizing_border_box)
				{
					free_cross_size -= box_sizing_height();
				}
			}
		} else
		{
			free_cross_size = self_size.render_width;
			ret_width = sum_cross_size;
		}
		free_cross_size -= sum_cross_size;
		sum_cross_size += free_cross_size;
		add_cross_size = (int) ((float) free_cross_size / (float) lines.size());
	}

	// Find line cross size and align items
	el_x = el_y = 0;
	bool is_wrap_reverse = css().get_flex_wrap() == flex_wrap_wrap_reverse;

	if(is_wrap_reverse)
	{
		if(is_row_direction)
		{
			el_y = sum_cross_size;
		} else
		{
			el_x = sum_cross_size;
		}
	}

	for(auto& ln : lines)
	{
		if(free_cross_size > 0 && add_cross_size > 0)
		{
			ln.cross_size += add_cross_size;
			free_cross_size -= add_cross_size;
		}
		if(is_row_direction)
		{
			el_x = reverse ? container_main_size : 0;
			if(is_wrap_reverse)
			{
				el_y -= ln.cross_size;
			}
			for (auto &item: ln.items)
			{
				if(!reverse)
				{
					item.el->pos().x = el_x + item.el->content_offset_left();
					el_x += item.el->width();
				} else
				{
					el_x -= item.el->width();
					item.el->pos().x = el_x + item.el->content_offset_left();
				}
				switch (item.align)
				{
					case flex_align_items_flex_end:
					case flex_align_items_end:
						item.el->pos().y = el_y + ln.cross_size - item.el->height() + item.el->content_offset_top();
						break;
					case flex_align_items_center:
						item.el->pos().y = el_y + ln.cross_size / 2 - item.el->height() /2 + item.el->content_offset_top();
						break;
					case flex_align_items_flex_start:
					case flex_align_items_start:
						item.el->pos().y = el_y + item.el->content_offset_top();
						break;
					default:
						item.el->pos().y = el_y + item.el->content_offset_top();
						// TODO: must be rendered into the specified height
						item.el->pos().height = ln.cross_size - item.el->content_offset_height();
						break;
				}
				m_pos.height = std::max(m_pos.height, item.el->bottom());
			}
			if(!is_wrap_reverse)
			{
				el_y += ln.cross_size;
			}
		} else
		{
			if(!reverse)
			{
				el_y = 0;
			} else
			{
				if(self_size.height.type == containing_block_context::cbc_value_type_auto)
				{
					el_y = ln.main_size;
				} else
				{
					el_y = self_size.height;
				}
			}
			if(is_wrap_reverse)
			{
				el_x -= ln.cross_size;
			}
			for (auto &item: ln.items)
			{
				if(!reverse)
				{
					item.el->pos().y = el_y + item.el->content_offset_top();
					el_y += item.el->height();
				} else
				{
					el_y -= item.el->height();
					item.el->pos().y = el_y + item.el->content_offset_top();
				}
				switch (item.align)
				{
					case flex_align_items_flex_end:
					case flex_align_items_end:
						item.el->pos().x = el_x + ln.cross_size - item.el->width() + item.el->content_offset_left();
						break;
					case flex_align_items_center:
						item.el->pos().x = el_x + ln.cross_size / 2 - item.el->width() /2 + item.el->content_offset_left();
						break;
					case flex_align_items_start:
					case flex_align_items_flex_start:
						item.el->pos().x = el_x + item.el->content_offset_left();
						break;
					default:
						item.el->pos().x = el_x + item.el->content_offset_left();
						item.el->render(el_x,
										item.el->pos().y - item.el->content_offset_top(),
										self_size.new_width(ln.cross_size), fmt_ctx, false);
						// TODO: must be rendered into the specified height
						item.el->pos().height = item.main_size - item.el->content_offset_height();
						break;
				}
				m_pos.height = std::max(m_pos.height, item.el->bottom());
			}
			if(!is_wrap_reverse)
			{
				el_x += ln.cross_size;
			}
		}
	}

	// calculate the final position
	m_pos.move_to(x, y);
	m_pos.x += content_offset_left();
	m_pos.y += content_offset_top();

	return ret_width;
}

void
litehtml::render_item_flex::flex_line::distribute_free_space(int container_main_size)
{
	// Determine the used flex factor. Sum the outer hypothetical main sizes of all items on the line.
	// If the sum is less than the flex container’s inner main size, use the flex grow factor for the
	// rest of this algorithm; otherwise, use the flex shrink factor.
	int initial_free_space = container_main_size - base_size;
	bool grow;
	int total_flex_factor;
	if(initial_free_space < 0)
	{
		grow = false;
		total_flex_factor = total_shrink;
	} else
	{
		grow = true;
		total_flex_factor = total_grow;
	}

	if(total_flex_factor > 0)
	{
		bool processed = true;
		while (processed)
		{
			int sum_scaled_flex_shrink_factor = 0;
			int sum_flex_factors = 0;
			int remaining_free_space = container_main_size;
			int total_not_frozen = 0;
			for (auto &item: items)
			{
				if (!item.frozen)
				{
					sum_scaled_flex_shrink_factor += item.scaled_flex_shrink_factor;
					if(grow)
					{
						sum_flex_factors += item.grow;
					} else
					{
						sum_flex_factors += item.shrink;
					}
					remaining_free_space -= item.base_size;
					total_not_frozen++;
				} else
				{
					remaining_free_space -= item.main_size;
				}
			}
			// Check for flexible items. If all the flex items on the line are frozen, free space has
			// been distributed; exit this loop.
			if (!total_not_frozen) break;

			remaining_free_space = abs(remaining_free_space);
			// c. Distribute free space proportional to the flex factors.
			// If the remaining free space is zero
			//    Do nothing.
			if (!remaining_free_space)
			{
				processed = false;
			} else
			{
				int total_clamped = 0;
				for (auto &item: items)
				{
					if (!item.frozen)
					{
						if(!grow)
						{
							// If using the flex shrink factor
							//    For every unfrozen item on the line, multiply its flex shrink factor by its
							//    inner flex base size, and note this as its scaled flex shrink factor. Find
							//    the ratio of the item’s scaled flex shrink factor to the sum of the scaled
							//    flex shrink factors of all unfrozen items on the line. Set the item’s target
							//    main size to its flex base size minus a fraction of the absolute value of the
							//    remaining free space proportional to the ratio.
							int scaled_flex_shrink_factor = item.base_size * item.shrink;
							item.main_size = (int) ((float) item.base_size - (float) remaining_free_space *
																		 (float) scaled_flex_shrink_factor /
																		 (float) sum_scaled_flex_shrink_factor);

							// d. Fix min/max violations. Clamp each non-frozen item’s target main size by its used
							// min and max main sizes and floor its content-box size at zero. If the item’s target
							// main size was made smaller by this, it’s a max violation. If the item’s target main
							// size was made larger by this, it’s a min violation.
							if (item.main_size <= item.min_size)
							{
								total_clamped++;
								item.main_size = item.min_size;
								item.frozen = true;
							}
						} else
						{
							// If using the flex grow factor
							//    Find the ratio of the item’s flex grow factor to the sum of the flex grow
							//    factors of all unfrozen items on the line. Set the item’s target main size to
							//    its flex base size plus a fraction of the remaining free space proportional
							//    to the ratio.
							item.main_size = (int) ((float) item.base_size +
													(float) remaining_free_space * (float) item.grow /
													(float) total_flex_factor);
							// d. Fix min/max violations. Clamp each non-frozen item’s target main size by its used
							// min and max main sizes and floor its content-box size at zero. If the item’s target
							// main size was made smaller by this, it’s a max violation. If the item’s target main
							// size was made larger by this, it’s a min violation.
							if (item.main_size >= container_main_size)
							{
								total_clamped++;
								item.main_size = container_main_size;
								item.frozen = true;
							}
						}
					}
				}
				if (total_clamped == 0) processed = false;
			}
		}
	}
}

std::list<litehtml::render_item_flex::flex_line> litehtml::render_item_flex::get_lines(const litehtml::containing_block_context &self_size,
																	 litehtml::formatting_context *fmt_ctx,
																	 bool is_row_direction, int container_main_size)
{
	std::list<flex_line> lines;
	flex_line line;
	for( auto& el : m_children)
	{
		flex_item item(el);
		item.grow = (int) (item.el->css().get_flex_grow() * 1000.0);
		item.shrink = (int) (item.el->css().get_flex_shrink() * 1000.0);
		item.el->calc_outlines(self_size.render_width);
		if(is_row_direction)
		{
			if (item.el->css().get_min_width().is_predefined())
			{
				item.min_size = el->render(0, 0, self_size.new_width(el->content_offset_width()), fmt_ctx);
			} else
			{
				item.min_size = item.el->css().get_min_width().calc_percent(self_size.render_width) +
								el->content_offset_width();
			}
			if (item.el->css().get_max_width().is_predefined())
			{
				item.max_size = self_size.render_width;
			} else
			{
				item.max_size = item.el->css().get_max_width().calc_percent(self_size.render_width) +
								el->content_offset_width();
			}
			if (item.el->css().get_flex_basis().is_predefined())
			{
				switch (item.el->css().get_flex_basis().predef())
				{
					case flex_basis_auto:
						if (!item.el->css().get_width().is_predefined())
						{
							item.base_size = item.el->css().get_width().calc_percent(self_size.render_width) +
											 item.el->content_offset_width();
							break;
						}
					case flex_basis_max_content:
					case flex_basis_fit_content:
						item.base_size = el->render(0, 0, self_size, fmt_ctx);
						break;
					case flex_basis_min_content:
						item.base_size = item.min_size;
						break;
				}
			} else
			{
				item.base_size = item.el->css().get_flex_basis().calc_percent(self_size.render_width) +
								 item.el->content_offset_width();
				item.base_size = std::max(item.base_size, item.min_size);
			}
		} else
		{
			if (item.el->css().get_min_height().is_predefined())
			{
				el->render(0, 0, self_size.new_width(self_size.render_width), fmt_ctx);
				item.min_size = el->height();
			} else
			{
				item.min_size = item.el->css().get_min_height().calc_percent(self_size.height) +
								el->content_offset_height();
			}
			if (item.el->css().get_max_height().is_predefined())
			{
				item.max_size = self_size.height;
			} else
			{
				item.max_size = item.el->css().get_max_height().calc_percent(self_size.height) +
								el->content_offset_width();
			}

			if (item.el->css().get_flex_basis().is_predefined())
			{
				switch (item.el->css().get_flex_basis().predef())
				{
					case flex_basis_auto:
						if (!item.el->css().get_height().is_predefined())
						{
							item.base_size = item.el->css().get_height().calc_percent(self_size.height) +
											 item.el->content_offset_height();
							break;
						}
					case flex_basis_max_content:
					case flex_basis_fit_content:
						el->render(0, 0, self_size, fmt_ctx);
						item.base_size = el->height();
						break;
					case flex_basis_min_content:
						item.base_size = item.min_size;
						break;
				}
			} else
			{
				item.base_size = item.el->css().get_flex_basis().calc_percent(self_size.height) +
								 item.el->content_offset_height();
			}
		}

		if(el->css().get_flex_align_self() == flex_align_items_auto)
		{
			item.align = css().get_flex_align_items();
		} else
		{
			item.align = el->css().get_flex_align_self();
		}
		item.main_size = item.base_size;
		item.scaled_flex_shrink_factor = item.base_size * item.shrink;

		// Add flex item to line
		if(!line.items.empty() && css().get_flex_wrap() != flex_wrap_nowrap && line.base_size + item.base_size > container_main_size)
		{
			lines.push_back(line);
			line.clear();
		}
		line.base_size += item.base_size;
		line.total_grow += item.grow;
		line.total_shrink += item.shrink;
		//if(item.base_size > item.min_size)
		{
			item.frozen = false;
		} /*else
		{
			item.frozen = true;
		}*/
		line.items.push_back(item);
	}
	// Add the last line to the lines list
	if(!line.items.empty())
	{
		lines.push_back(line);
	}
	return lines;
}

std::shared_ptr<litehtml::render_item> litehtml::render_item_flex::init()
{
    auto doc = src_el()->get_document();
    decltype(m_children) new_children;
    decltype(m_children) inlines;

    auto convert_inlines = [&]()
        {
        if(!inlines.empty())
        {
            // Find last not space
            auto not_space = std::find_if(inlines.rbegin(), inlines.rend(), [&](const std::shared_ptr<render_item>& el)
                {
                return !el->src_el()->is_space();
                });
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

    for (const auto& el : m_children)
    {
        if(el->src_el()->css().get_display() == display_inline_text)
        {
            if(!inlines.empty())
            {
                inlines.push_back(el);
            } else
            {
                if (!el->src_el()->is_white_space())
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
