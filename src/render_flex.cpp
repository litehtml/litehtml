#include "html.h"
#include "types.h"
#include "render_flex.h"

int litehtml::render_item_flex::_render_content(int x, int y, bool second_pass, const containing_block_context &self_size, formatting_context* fmt_ctx)
{
	struct flex_item
	{
		std::shared_ptr<render_item> el;
		int basis;		// flex basis
		int min_width;
		int main_size;
		int grow;
		int shrink;
		int scaled_flex_shrink_factor;
		bool frozen;
		flex_align_items align;
		explicit flex_item(std::shared_ptr<render_item>& _el) :
			el(_el),
			align(flex_align_items_auto),
			grow(0),
			basis(0),
			shrink(0),
			min_width(0),
			frozen(false),
			main_size(0),
			scaled_flex_shrink_factor(0) {}
	};

	struct flex_line
	{
		std::list<flex_item> items;
		int top;
		int height; // line height
		int width;
		int basis;
		int total_grow;
		int total_shrink;
		flex_line() :
			height(0),
			top(0),
			total_grow(0),
			width(0),
			basis(0),
			total_shrink(0){}
		void clear()
		{
			items.clear();
			top = height = width = basis = total_shrink = total_grow = 0;
		}
	};

	std::list<flex_item> items;
	for( auto& el : m_children)
	{
		flex_item item(el);
		item.grow = (int) (item.el->css().get_flex_grow() * 1000.0);
		item.shrink = (int) (item.el->css().get_flex_shrink() * 1000.0);
		item.min_width = el->render(0, 0, self_size.new_width(el->content_offset_width()), fmt_ctx);
		if(item.el->css().get_flex_basis().is_predefined())
		{
			switch (item.el->css().get_flex_basis().predef())
			{
				case flex_basis_auto:
					if(!item.el->css().get_width().is_predefined())
					{
						item.el->calc_outlines(self_size.render_width);
						item.basis = item.el->css().get_width().calc_percent(self_size.render_width) + item.el->content_offset_width();
						break;
					}
				case flex_basis_max_content:
				case flex_basis_fit_content:
					item.basis = el->render(0, 0, self_size, fmt_ctx);
					break;
				case flex_basis_min_content:
					item.basis = item.min_width;
					break;
			}
		} else
		{
			item.el->calc_outlines(self_size.render_width);
			item.basis = item.el->css().get_flex_basis().calc_percent(self_size.render_width) + item.el->content_offset_width();
		}
		if(el->css().get_flex_align_self() == flex_align_items_auto)
		{
			item.align = css().get_flex_align_items();
		} else
		{
			item.align = el->css().get_flex_align_self();
		}
		item.main_size = item.basis;
		item.scaled_flex_shrink_factor = item.basis * item.shrink;
		items.push_back(item);
	}

	std::list<flex_line> lines;
	flex_line line;
	for(auto& item : items)
	{
		if(!line.items.empty() && css().get_flex_wrap() != flex_wrap_nowrap && line.basis + item.basis > self_size.render_width)
		{
			lines.push_back(line);
			line.clear();
		}
		line.basis += item.basis;
		line.total_grow += item.grow;
		line.total_shrink += item.shrink;
		if(item.basis > item.min_width)
		{
			item.frozen = false;
		} else
		{
			item.frozen = true;
		}
		line.items.push_back(item);
	}
	if(!line.items.empty())
	{
		lines.push_back(line);
	}

	int el_y = 0;
	int ret_width = 0;
	for(auto& ln : lines)
	{
		ln.top = el_y;
		ln.height = 0;
		int el_x = 0;

		ret_width += ln.basis;

		// distribute free space to items
		int initial_free_space = self_size.render_width - ln.basis;
		if(initial_free_space < 0)
		{
			if(ln.total_shrink > 0)
			{
				initial_free_space = -initial_free_space;
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
							sum_flex_factors += item.shrink;
							remaining_free_space -= item.basis;
							total_not_frozen++;
						} else
						{
							remaining_free_space -= item.main_size;
						}
					}
					if (!total_not_frozen) break;
					remaining_free_space = -remaining_free_space;
					if (remaining_free_space)
					{
						int total_clamped = 0;
						for (auto &item: ln.items)
						{
							if (!item.frozen)
							{
								// Distribute free space proportional to the flex factors.
								int scaled_flex_shrink_factor = item.basis * item.shrink;
								item.main_size = (int) ((float) item.basis - (float) remaining_free_space *
																			 (float) scaled_flex_shrink_factor /
																			 (float) sum_scaled_flex_shrink_factor);

								if (item.main_size <= item.min_width)
								{
									total_clamped++;
									item.main_size = item.min_width;
									item.frozen = true;
								}
							}
						}
						if (total_clamped == 0) processed = false;
					}
				}
			}
		} else
		{
			if(ln.total_grow > 0)
			{
				// Distribute free space by flex-grow
				for (auto &item: ln.items)
				{
					if (item.grow > 0)
					{
						int add_space = (int) ((float) initial_free_space * (float) item.grow / (float) ln.total_grow);
						item.main_size = item.basis + add_space;
					}
				}
			}
		}

		// render items into new width
		for(auto& item : ln.items)
		{
			item.el->render(el_x,
							el_y,
							self_size.new_width(item.main_size), fmt_ctx, false);
			ln.height = std::max(ln.height, item.el->height());
			el_x += item.el->width();
		}
		el_y += ln.height;
	}
	for(auto& ln : lines)
	{
		for(auto& item : ln.items)
		{
			switch (item.align)
			{
				case flex_align_items_flex_end:
					item.el->pos().y = ln.top + ln.height - item.el->height() + item.el->content_offset_top();
					break;
				case flex_align_items_center:
					item.el->pos().y = ln.top + ln.height / 2 - item.el->height() /2 + item.el->content_offset_top();
					break;
				case flex_align_items_flex_start:
					item.el->pos().y = ln.top + item.el->content_offset_top();
					break;
				default:
					item.el->pos().y = ln.top + item.el->content_offset_top();
					item.el->pos().height = ln.height - item.el->content_offset_height();
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
    for(const auto& el : children())
    {
        m_flex_items.emplace_back(new flex_item(el));
    }

    return shared_from_this();
}
