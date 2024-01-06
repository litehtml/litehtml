#include "html.h"
#include "types.h"
#include "render_flex.h"

namespace litehtml
{
	class flex_justify_content_spread
	{
		flex_justify_content m_type;
		int m_num_items;
		int m_free_space;
		bool m_row_direction;
		bool m_reverse;
	public:
		flex_justify_content_spread(flex_justify_content type, int num_items, int free_space, bool row_direction, bool reverse) :
			m_type(type), m_num_items(num_items), m_free_space(0), m_row_direction(row_direction), m_reverse(reverse)
		{
			set_free_space(free_space);
		}

		void set_free_space(int free_space)
		{
			m_free_space = free_space;
			switch (m_type)
			{

				case flex_justify_content_space_between:
					// If the leftover free-space is negative or there is only a single flex item on the line, this
					// value is identical to flex-start.
					if(m_num_items == 1 || m_free_space < 0)  m_type = flex_justify_content_flex_start;
					break;
				case flex_justify_content_space_around:
				case flex_justify_content_space_evenly:
					// If the leftover free-space is negative or there is only a single flex item on the line, this
					// value is identical to center
					if(m_num_items == 1 || m_free_space < 0)  m_type = flex_justify_content_center;
					break;
				case flex_justify_content_right:
				case flex_justify_content_left:
					if(!m_row_direction)
					{
						m_type = flex_justify_content_start;
					}
					break;
				default:
					break;
			}
		}

		int start()
		{
			switch (m_type)
			{
				case flex_justify_content_right:
					if(!m_reverse)
					{
						return m_free_space;
					}
					return 0;
				case flex_justify_content_start:
				case flex_justify_content_left:
					if(m_reverse)
					{
						return m_free_space;
					}
					return 0;
				case flex_justify_content_flex_end:
				case flex_justify_content_end:
					return m_free_space;
				case flex_justify_content_center:
					return m_free_space / 2;
				case flex_justify_content_space_between:
				case flex_justify_content_space_around:
				default:
					// using flex-start by default
					return 0;
			}
		}

		int before_item()
		{
			switch (m_type)
			{
				case flex_justify_content_space_evenly:
					return m_free_space / (m_num_items + 1);
				case flex_justify_content_space_between:
					return 0;
				case flex_justify_content_space_around:
					return m_free_space / (m_num_items * 2);
				default:
					return 0;
			}
		}

		int after_item()
		{
			switch (m_type)
			{
				case flex_justify_content_space_between:
					return m_free_space / (m_num_items - 1);
				case flex_justify_content_space_around:
					return m_free_space / (m_num_items * 2);
				default:
					return 0;
			}
		}
	};

	class flex_align_content_spread
	{
		flex_align_content m_type;
		int m_num_lines;
		int m_free_space;
		flex_wrap m_wrap;
	public:
		flex_align_content_spread(flex_align_content type, flex_wrap wrap, int num_lines, int free_space) :
				m_type(type), m_num_lines(num_lines), m_free_space(0), m_wrap(wrap)
		{
			if(m_wrap == flex_wrap_nowrap)
			{
				m_type = flex_align_content_stretch;
			}
			set_free_space(free_space);
		}

		void set_free_space(int free_space)
		{
			m_free_space = free_space;
			switch (m_type)
			{

				case flex_align_content_space_between:
					// If the leftover free-space is negative or there is only a single flex line in the flex
					// container, this value is identical to flex-start.
					if(m_num_lines == 1 || m_free_space < 0) m_type = flex_align_content_flex_start;
					break;
				case flex_align_content_space_around:
					// If the leftover free-space is negative this value is identical to center.
					if(m_num_lines == 1 || m_free_space < 0) m_type = flex_align_content_center;
					break;
				default:
					break;
			}
		}

		int start()
		{
			switch (m_type)
			{
				case flex_align_content_flex_end:
				case flex_align_content_end:
					return m_free_space;
				case flex_align_content_center:
					return m_free_space / 2;
				case flex_align_content_stretch:
				case flex_align_content_space_between:
				case flex_align_content_space_around:
				default:
					// using stretch by default
					return 0;
			}
		}

		int add_line_size()
		{
			switch (m_type)
			{

				case flex_align_content_flex_start:
				case flex_align_content_flex_end:
				case flex_align_content_start:
				case flex_align_content_end:
				case flex_align_content_center:
				case flex_align_content_space_between:
				case flex_align_content_space_around:
					return 0;
				case flex_align_content_stretch:
				default:
					return m_free_space / m_num_lines;
			}
		}

		int before_line()
		{
			switch (m_type)
			{
				case flex_align_content_space_between:
					return 0;
				case flex_align_content_space_around:
					return m_free_space / (m_num_lines * 2);
				default:
					return 0;
			}
		}

		int after_line()
		{
			switch (m_type)
			{
				case flex_align_content_space_between:
					return m_free_space / (m_num_lines - 1);
				case flex_align_content_space_around:
					return m_free_space / (m_num_lines * 2);
				default:
					return 0;
			}
		}
	};
}

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
			break;
		case flex_direction_column_reverse:
			is_row_direction = false;
			reverse = true;
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

	bool single_line = css().get_flex_wrap() == flex_wrap_nowrap;
	bool fit_container = false;

	if(!is_row_direction)
	{
		if(self_size.height.type != containing_block_context::cbc_value_type_auto)
		{
			container_main_size = self_size.height;
			if (css().get_box_sizing() == box_sizing_border_box)
			{
				container_main_size -= box_sizing_height();
			}
		} else
		{
			// Direction columns, height is auto - always in single line
			container_main_size = 0;
			single_line = true;
			fit_container = true;
		}
		if(self_size.min_height.type != containing_block_context::cbc_value_type_auto && self_size.min_height > container_main_size)
		{
			container_main_size = self_size.min_height;
		}
	}

	// Split flex items to lines
	std::list<flex_line> lines = get_lines(self_size, fmt_ctx, is_row_direction, container_main_size, single_line);

	// Resolving Flexible Lengths
	// REF: https://www.w3.org/TR/css-flexbox-1/#resolve-flexible-lengths

	int el_y = 0;
	int el_x = 0;
	int sum_cross_size = 0;
	int sum_main_size = 0;
	int ret_width = 0;
	for(auto& ln : lines)
	{
		ln.cross_size = 0;
		ln.main_size = 0;

		if(is_row_direction)
		{
			ret_width += ln.base_size;
		}

		if(!fit_container)
		{
			ln.distribute_free_space(container_main_size);
		}

		if(is_row_direction)
		{
			// render items into new size and find line cross_size
			for (auto &item: ln.items)
			{
				item.el->render(el_x,
								el_y,
								self_size.new_width(item.main_size - item.el->content_offset_width(), containing_block_context::size_mode_exact_width), fmt_ctx, false);
				ln.main_size += item.el->width();
				ln.cross_size = std::max(ln.cross_size, item.el->height());
				el_x += item.el->width();
			}
			sum_cross_size += ln.cross_size;
			el_x = 0;
		} else
		{
			for (auto &item: ln.items)
			{
				int el_ret_width = item.el->render(el_x,
								el_y,
								self_size, fmt_ctx, false);
				item.el->render(el_x,
								el_y,
								self_size.new_width_height(el_ret_width - item.el->content_offset_width(),
														   item.main_size - item.el->content_offset_height(),
														   containing_block_context::size_mode_exact_width |
														   containing_block_context::size_mode_exact_height),
														   fmt_ctx, false);
				ln.main_size += item.el->height();
				ln.cross_size = std::max(ln.cross_size, item.el->width());
				el_y += item.el->height();
			}
			sum_cross_size += ln.cross_size;
			el_y = 0;
		}
		sum_main_size = std::max(sum_main_size, ln.main_size);
	}

	int free_cross_size = 0;
	int cross_end = 0;
	if(container_main_size == 0)
	{
		container_main_size = sum_main_size;
	}
	if (is_row_direction)
	{
		if (self_size.height.type != containing_block_context::cbc_value_type_auto)
		{
			int height = self_size.height;
			if (src_el()->css().get_box_sizing() == box_sizing_border_box)
			{
				height -= box_sizing_height();
			}
			free_cross_size = height - sum_cross_size;
			cross_end = std::max(sum_cross_size, height);
		} else
		{
			cross_end = sum_cross_size;
		}
	} else
	{
		free_cross_size = self_size.render_width - sum_cross_size;
		ret_width = sum_cross_size;
		cross_end = std::max(sum_cross_size, (int) self_size.render_width);
	}

	// Find line cross size and align items
	el_x = el_y = 0;
	bool is_wrap_reverse = css().get_flex_wrap() == flex_wrap_wrap_reverse;

	flex_align_content_spread lines_spread(css().get_flex_align_content(), css().get_flex_wrap(), (int) lines.size(), free_cross_size);

	if(is_wrap_reverse)
	{
		if(is_row_direction)
		{
			el_y = cross_end - lines_spread.start();
		} else
		{
			el_x = cross_end - lines_spread.start();
		}
	} else
	{
		if(is_row_direction)
		{
			el_y = lines_spread.start();
		} else
		{
			el_x = lines_spread.start();
		}
	}

	for(auto& ln : lines)
	{
		int free_main_size = container_main_size - ln.main_size;
		// distribute auto margins
		if(free_main_size > 0 && (ln.num_auto_margin_start || ln.num_auto_margin_end))
		{
			int add = (int) (free_main_size / (ln.items.size() * 2));
			for (auto &item: ln.items)
			{
				if(!item.auto_margin_start.is_default())
				{
					item.auto_margin_start = add;
					item.main_size += add;
					ln.main_size += add;
					free_main_size -= add;
				}
				if(!item.auto_margin_end.is_default())
				{
					item.auto_margin_end = add;
					item.main_size += add;
					ln.main_size += add;
					free_main_size -= add;
				}
			}
			while (free_main_size > 0)
			{
				for (auto &item: ln.items)
				{
					if(!item.auto_margin_start.is_default())
					{
						item.auto_margin_start = item.auto_margin_start + 1;
						free_main_size--;
						if(!free_main_size) break;
					}
					if(!item.auto_margin_end.is_default())
					{
						item.auto_margin_end = item.auto_margin_end + 1;
						free_main_size--;
						if(!free_main_size) break;
					}
				}
			}
		}

		ln.cross_size += lines_spread.add_line_size();

		flex_justify_content_spread content_spread(css().get_flex_justify_content(),
												   (int) ln.items.size(),
												   free_main_size, is_row_direction, reverse);
		if(is_row_direction)
		{
			if(is_wrap_reverse)
			{
				el_y -= ln.cross_size - lines_spread.before_line();
			} else
			{
				el_y += lines_spread.before_line();
			}
			if(reverse)
			{
				el_x = container_main_size - content_spread.start();
			} else
			{
				el_x = content_spread.start();
			}
			for (auto &item: ln.items)
			{
				// apply auto margins to item
				if(!item.auto_margin_start.is_default())
				{
					item.el->get_margins().left = item.auto_margin_start;
					item.el->pos().x += item.auto_margin_start;
				}
				if(!item.auto_margin_end.is_default()) item.el->get_margins().right = item.auto_margin_end;
				if(!reverse)
				{
					// justify content [before_item]
					el_x += content_spread.before_item();
					item.el->pos().x = el_x + item.el->content_offset_left();
					el_x += item.el->width();
				} else
				{
					// justify content [before_item]
					el_x -= content_spread.before_item();
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
						if(item.el->css().get_height().is_predefined())
						{
							// TODO: must be rendered into the specified height
							item.el->pos().height = ln.cross_size - item.el->content_offset_height();
						} else if(is_wrap_reverse)
						{
							item.el->pos().y = el_y + ln.cross_size - item.el->height() + item.el->content_offset_top();
						}
						break;
				}
				m_pos.height = std::max(m_pos.height, item.el->bottom());
				// justify content [after_item]
				if(!reverse)
				{
					el_x += content_spread.after_item();
				} else
				{
					el_x -= content_spread.after_item();
				}
			}
			if(!is_wrap_reverse)
			{
				el_y += ln.cross_size + lines_spread.after_line();
			} else
			{
				el_y -= lines_spread.after_line();
			}
		} else
		{
			if(!reverse)
			{
				el_y = content_spread.start();
			} else
			{
				if(self_size.height.type == containing_block_context::cbc_value_type_auto)
				{
					content_spread.set_free_space(0);
					el_y = ln.main_size;
				} else
				{
					content_spread.set_free_space(self_size.height - ln.main_size);
					el_y = self_size.height;
				}
				el_y -= content_spread.start();
			}
			if(is_wrap_reverse)
			{
				el_x -= ln.cross_size - lines_spread.before_line();
			} else
			{
				el_x += lines_spread.before_line();
			}
			for (auto &item: ln.items)
			{
				// apply auto margins to item
				if(!item.auto_margin_start.is_default())
				{
					item.el->get_margins().top = item.auto_margin_start;
					item.el->pos().y += item.auto_margin_start;
				}
				if(!item.auto_margin_end.is_default()) item.el->get_margins().bottom = item.auto_margin_end;

				if(!reverse)
				{
					// justify content [before_item]
					el_y += content_spread.before_item();

					item.el->pos().y = el_y + item.el->content_offset_top();
					el_y += item.el->height();
				} else
				{
					// justify content [before_item]
					el_y -= content_spread.before_item();

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
						if(!item.el->css().get_width().is_predefined())
						{
							item.el->render(el_x,
											item.el->pos().y - item.el->content_offset_top(),
											self_size.new_width_height(ln.cross_size,
																	   item.main_size - item.el->content_offset_height(),
																	   containing_block_context::size_mode_exact_height),
											fmt_ctx, false);
						} else
						{
							item.el->render(el_x,
											item.el->pos().y - item.el->content_offset_top(),
											self_size.new_width_height(ln.cross_size - item.el->content_offset_width(),
																	   item.main_size - item.el->content_offset_height(),
																	   containing_block_context::size_mode_exact_width |
																	   containing_block_context::size_mode_exact_height),
											fmt_ctx, false);
						}
						// apply auto margins to item after rendering
						if(!item.auto_margin_start.is_default())
						{
							item.el->get_margins().top = item.auto_margin_start;
							item.el->pos().y += item.auto_margin_start;
						}
						if(!item.auto_margin_end.is_default()) item.el->get_margins().bottom = item.auto_margin_end;

						if(!item.el->css().get_width().is_predefined() && is_wrap_reverse)
						{
							item.el->pos().x = el_x + ln.cross_size - item.el->width() + item.el->content_offset_left();
						}
						break;
				}
				m_pos.height = std::max(m_pos.height, item.el->bottom());
				// justify content [after_item]
				if(!reverse)
				{
					el_y += content_spread.after_item();
				} else
				{
					el_y -= content_spread.after_item();
				}
			}
			if(!is_wrap_reverse)
			{
				el_x += ln.cross_size + lines_spread.after_line();
			} else
			{
				el_x -= lines_spread.after_line();
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
							if(!item.max_size.is_default() && item.main_size >= item.max_size)
							{
								total_clamped++;
								item.main_size = item.max_size;
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
							if(!item.max_size.is_default() && item.main_size >= item.max_size)
							{
								total_clamped++;
								item.main_size = item.max_size;
								item.frozen = true;
							}
						}
					}
				}
				if (total_clamped == 0) processed = false;
			}
		}
		// Distribute remaining after algorithm space
		int sum_main_size = 0;
		for(auto &item : items)
		{
			sum_main_size += item.main_size;
		}
		int free_space = container_main_size - sum_main_size;
		if(free_space > 0)
		{
			for(auto &item : items)
			{
				if(free_space == 0) break;
				item.main_size++;
				free_space--;
			}
		}
	}
}

std::list<litehtml::render_item_flex::flex_line> litehtml::render_item_flex::get_lines(const litehtml::containing_block_context &self_size,
																	 litehtml::formatting_context *fmt_ctx,
																	 bool is_row_direction, int container_main_size,
																	 bool single_line)
{
	std::list<flex_line> lines;
	flex_line line;
	std::list<flex_item> items;
	int src_order = 0;
	bool sort_required = false;
	def_value<int> prev_order(0);

	for( auto& el : m_children)
	{
		flex_item item(el);

		item.grow = (int) (item.el->css().get_flex_grow() * 1000.0);
		// Negative numbers are invalid.
		// https://www.w3.org/TR/css-flexbox-1/#valdef-flex-grow-number
		if(item.grow < 0) item.grow = 0;

		item.shrink = (int) (item.el->css().get_flex_shrink() * 1000.0);
		// Negative numbers are invalid.
		// https://www.w3.org/TR/css-flexbox-1/#valdef-flex-shrink-number
		if(item.shrink < 0) item.shrink = 1000.0;

		item.el->calc_outlines(self_size.render_width);
		item.order = item.el->css().get_order();
		item.src_order = src_order++;

		if(prev_order.is_default())
		{
			prev_order = item.order;
		} else if(!sort_required && item.order != prev_order)
		{
			sort_required = true;
		}

		if (is_row_direction)
		{
			if(item.el->css().get_margins().left.is_predefined())
			{
				item.auto_margin_start = 0;
			}
			if(item.el->css().get_margins().right.is_predefined())
			{
				item.auto_margin_end = 0;
			}
			if (item.el->css().get_min_width().is_predefined())
			{
				item.min_size = el->render(0, 0,
										   self_size.new_width(el->content_offset_width(),
															   containing_block_context::size_mode_content), fmt_ctx);
			} else
			{
				item.min_size = item.el->css().get_min_width().calc_percent(self_size.render_width) +
								el->content_offset_width();
			}
			if (!item.el->css().get_max_width().is_predefined())
			{
				item.max_size = item.el->css().get_max_width().calc_percent(self_size.render_width) +
								el->content_offset_width();
			}
			bool flex_basis_predefined = item.el->css().get_flex_basis().is_predefined();
			int predef = flex_basis_auto;
			if(flex_basis_predefined)
			{
				predef = item.el->css().get_flex_basis().predef();
			} else
			{
				if(item.el->css().get_flex_basis().val() < 0)
				{
					flex_basis_predefined = true;
				}
			}

			if (flex_basis_predefined)
			{
				switch (predef)
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
					default:
						item.base_size = 0;
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
			if(item.el->css().get_margins().top.is_predefined())
			{
				item.auto_margin_start = 0;
			}
			if(item.el->css().get_margins().bottom.is_predefined())
			{
				item.auto_margin_end = 0;
			}
			if (item.el->css().get_min_height().is_predefined())
			{
				el->render(0, 0, self_size.new_width(self_size.render_width, containing_block_context::size_mode_content), fmt_ctx);
				item.min_size = el->height();
			} else
			{
				item.min_size = item.el->css().get_min_height().calc_percent(self_size.height) +
								el->content_offset_height();
			}
			if (!item.el->css().get_max_height().is_predefined())
			{
				item.max_size = item.el->css().get_max_height().calc_percent(self_size.height) +
								el->content_offset_width();
			}

			bool flex_basis_predefined = item.el->css().get_flex_basis().is_predefined();
			int predef = flex_basis_auto;
			if(flex_basis_predefined)
			{
				predef = item.el->css().get_flex_basis().predef();
			} else
			{
				if(item.el->css().get_flex_basis().val() < 0)
				{
					flex_basis_predefined = true;
				}
			}

			if (flex_basis_predefined)
			{
				switch (predef)
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
					default:
						item.base_size = 0;
				}
			} else
			{
				item.base_size = item.el->css().get_flex_basis().calc_percent(self_size.height) +
								 item.el->content_offset_height();
			}
		}

		if (el->css().get_flex_align_self() == flex_align_items_auto)
		{
			item.align = css().get_flex_align_items();
		} else
		{
			item.align = el->css().get_flex_align_self();
		}
		item.main_size = item.base_size;
		item.scaled_flex_shrink_factor = item.base_size * item.shrink;
		item.frozen = false;

		items.push_back(item);
	}

	if(sort_required)
	{
		items.sort();
	}

	// Add flex items to lines
	for(auto& item : items)
	{
		if(!line.items.empty() && !single_line && line.base_size + item.base_size > container_main_size)
		{
			lines.push_back(line);
			line.clear();
		}
		line.base_size += item.base_size;
		line.total_grow += item.grow;
		line.total_shrink += item.shrink;
		if(!item.auto_margin_start.is_default()) line.num_auto_margin_start++;
		if(!item.auto_margin_end.is_default()) line.num_auto_margin_end++;
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
