#include "html.h"
#include "types.h"
#include "render_flex.h"

namespace litehtml
{
	struct flex_item
	{
		std::shared_ptr<render_item> el;
		int base_size;
		int min_size;
		int max_size;
		int main_size;
		int grow;
		int shrink;
		int scaled_flex_shrink_factor;
		bool frozen;
		flex_align_items align;

		explicit flex_item(std::shared_ptr<render_item> &_el) :
				el(_el),
				align(flex_align_items_auto),
				grow(0),
				base_size(0),
				shrink(0),
				min_size(0),
				frozen(false),
				main_size(0),
				max_size(0),
				scaled_flex_shrink_factor(0)
		{}
	};

	struct flex_line
	{
		std::list<flex_item> items;
		int top;
		int cross_size;
		int base_size;
		int total_grow;
		int total_shrink;

		flex_line() :
				cross_size(0),
				top(0),
				total_grow(0),
				base_size(0),
				total_shrink(0)
		{}

		void clear()
		{
			items.clear();
			top = cross_size = base_size = total_shrink = total_grow = 0;
		}
	};
}

int litehtml::render_item_flex::_render_content(int x, int y, bool second_pass, const containing_block_context &self_size, formatting_context* fmt_ctx)
{
	std::list<flex_line> lines;
	flex_line line;
	for( auto& el : m_children)
	{
		flex_item item(el);
		item.grow = (int) (item.el->css().get_flex_grow() * 1000.0);
		item.shrink = (int) (item.el->css().get_flex_shrink() * 1000.0);
		item.el->calc_outlines(self_size.render_width);
		if(item.el->css().get_min_width().is_predefined())
		{
			item.min_size = el->render(0, 0, self_size.new_width(el->content_offset_width()), fmt_ctx);
		} else
		{
			item.min_size = item.el->css().get_min_width().calc_percent(self_size.render_width) + el->content_offset_width();
		}
		if(item.el->css().get_max_width().is_predefined())
		{
			item.max_size = self_size.render_width;
		} else
		{
			item.max_size = item.el->css().get_max_width().calc_percent(self_size.render_width) + el->content_offset_width();
		}
		if(item.el->css().get_flex_basis().is_predefined())
		{
			switch (item.el->css().get_flex_basis().predef())
			{
				case flex_basis_auto:
					if(!item.el->css().get_width().is_predefined())
					{
						item.base_size = item.el->css().get_width().calc_percent(self_size.render_width) + item.el->content_offset_width();
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
			item.base_size = item.el->css().get_flex_basis().calc_percent(self_size.render_width) + item.el->content_offset_width();
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
		if(!line.items.empty() && css().get_flex_wrap() != flex_wrap_nowrap && line.base_size + item.base_size > self_size.render_width)
		{
			lines.push_back(line);
			line.clear();
		}
		line.base_size += item.base_size;
		line.total_grow += item.grow;
		line.total_shrink += item.shrink;
		if(item.base_size > item.min_size)
		{
			item.frozen = false;
		} else
		{
			item.frozen = true;
		}
		line.items.push_back(item);
	}
	// Add the last line to the lines list
	if(!line.items.empty())
	{
		lines.push_back(line);
	}

	// Resolving Flexible Lengths
	// REF: https://www.w3.org/TR/css-flexbox-1/#resolve-flexible-lengths

	int el_y = 0;
	int ret_width = 0;
	for(auto& ln : lines)
	{
		ln.top = el_y;
		ln.cross_size = 0;
		int el_x = 0;

		ret_width += ln.base_size;

		// Determine the used flex factor. Sum the outer hypothetical main sizes of all items on the line.
		// If the sum is less than the flex container’s inner main size, use the flex grow factor for the
		// rest of this algorithm; otherwise, use the flex shrink factor.
		int initial_free_space = self_size.render_width - ln.base_size;
		bool grow;
		int total_flex_factor;
		if(initial_free_space < 0)
		{
			grow = false;
			total_flex_factor = ln.total_shrink;
		} else
		{
			grow = true;
			total_flex_factor = ln.total_grow;
		}

		if(total_flex_factor > 0)
		{
			bool processed = true;
			while (processed)
			{
				int sum_scaled_flex_shrink_factor = 0;
				int sum_flex_factors = 0;
				int remaining_free_space = self_size.render_width;
				int total_not_frozen = 0;
				for (auto &item: ln.items)
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

				remaining_free_space = std::abs(remaining_free_space);
				// c. Distribute free space proportional to the flex factors.
				// If the remaining free space is zero
				//    Do nothing.
				if (remaining_free_space)
				{
					int total_clamped = 0;
					for (auto &item: ln.items)
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
								if (item.main_size >= self_size.render_width)
								{
									total_clamped++;
									item.main_size = self_size.render_width;
									item.frozen = true;
								}
							}
						}
					}
					if (total_clamped == 0) processed = false;
				}
			}
		}

		// render items into new width
		for(auto& item : ln.items)
		{
			item.el->render(el_x,
							el_y,
							self_size.new_width(item.main_size), fmt_ctx, false);
			ln.cross_size = std::max(ln.cross_size, item.el->height());
			el_x += item.el->width();
		}
		el_y += ln.cross_size;
	}
	for(auto& ln : lines)
	{
		for(auto& item : ln.items)
		{
			switch (item.align)
			{
				case flex_align_items_flex_end:
					item.el->pos().y = ln.top + ln.cross_size - item.el->height() + item.el->content_offset_top();
					break;
				case flex_align_items_center:
					item.el->pos().y = ln.top + ln.cross_size / 2 - item.el->height() /2 + item.el->content_offset_top();
					break;
				case flex_align_items_flex_start:
					item.el->pos().y = ln.top + item.el->content_offset_top();
					break;
				default:
					item.el->pos().y = ln.top + item.el->content_offset_top();
					item.el->pos().height = ln.cross_size - item.el->content_offset_height();
					break;
			}
		}
	}

	// calculate the final position
	m_pos.move_to(x, y);
	m_pos.x += content_offset_left();
	m_pos.y += content_offset_top();
	m_pos.height = el_y;

	return ret_width + content_offset_width();
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
