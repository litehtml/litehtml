#include "html.h"
#include "render_item.h"
#include "document.h"
#include "iterators.h"

int litehtml::render_item_inline_context::_render_content(int x, int y, int max_width, bool second_pass, int ret_width)
{
    m_line_boxes.clear();
	m_inlines.clear();

    int block_height = 0;

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
    elements_iterator inlines_iter(true, &go_inside_inlines_selector, &select_inlines);

    inlines_iter.process(shared_from_this(), [&](const std::shared_ptr<render_item>& el, iterator_item_type item_type)
        {
			switch (item_type)
			{
				case iterator_item_type_child:
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
								} else
								{
									was_space = true;
								}
							} else
							{
								// skip all spaces after line break
								was_space = el->src_el()->is_break();
							}
						}
						// place element into rendering flow
						int rw = place_inline(std::unique_ptr<line_box_item>(new line_box_item(el)), max_width);
						if(rw > ret_width)
						{
							ret_width = rw;
						}
					}
					break;

				case iterator_item_type_start_parent:
					{
						el->clear_inline_boxes();
						m_inlines.emplace_back(el);
						int rw = place_inline(std::unique_ptr<lbi_start>(new lbi_start(el)), max_width);
						if (rw > ret_width)
						{
							ret_width = rw;
						}
					}
					break;

				case iterator_item_type_end_parent:
				{
					int rw = place_inline(std::unique_ptr<lbi_end>(new lbi_end(el)), max_width);
					if (rw > ret_width)
					{
						ret_width = rw;
					}
				}
					break;
			}
        });

    finish_last_box(true);

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

    return ret_width;
}

int litehtml::render_item_inline_context::fix_line_width( int max_width, element_float flt )
{
    int ret_width = 0;
    if(!m_line_boxes.empty())
    {
		auto el_front = m_line_boxes.back()->get_first_text_part();

        std::vector<std::shared_ptr<render_item>> els;
        bool was_cleared = false;
        if(el_front && el_front->src_el()->css().get_clear() != clear_none)
        {
            if(el_front->src_el()->css().get_clear() == clear_both)
            {
                was_cleared = true;
            } else
            {
                if(	(flt == float_left	&& el_front->src_el()->css().get_clear() == clear_left) ||
                       (flt == float_right	&& el_front->src_el()->css().get_clear() == clear_right) )
                {
                    was_cleared = true;
                }
            }
        }

        if(!was_cleared)
        {
			std::list<std::unique_ptr<line_box_item> > items = std::move(m_line_boxes.back()->items());
            m_line_boxes.pop_back();

            for(auto& item : items)
            {
                int rw = place_inline(std::move(item), max_width);
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

            if(m_line_boxes.size() == 1)
            {
                if (src_el()->css().get_list_style_type() != list_style_type_none && src_el()->css().get_list_style_position() == list_style_position_inside)
                {
                    int sz_font = src_el()->css().get_font_size();
                    line_left += sz_font;
                }

                if (src_el()->css().get_text_indent().val() != 0)
                {
                    line_left += src_el()->css().get_text_indent().calc_percent(max_width);
                }
            
            }

            auto items = m_line_boxes.back()->new_width(line_left, line_right);
            for(auto& item : items)
            {
                int rw = place_inline(std::move(item), max_width);
                if(rw > ret_width)
                {
                    ret_width = rw;
                }
            }
        }
    }

    return ret_width;
}

std::list<std::unique_ptr<litehtml::line_box_item> > litehtml::render_item_inline_context::finish_last_box(bool end_of_render)
{
	std::list<std::unique_ptr<line_box_item> > ret;

    if(!m_line_boxes.empty())
    {
		ret = m_line_boxes.back()->finish(end_of_render);

        if(m_line_boxes.back()->is_empty() && end_of_render)
        {
			// remove the last empty line
            m_line_boxes.pop_back();
        } else
		{
			// set start box into the line start for all inlines
			for (auto &inline_item: m_inlines)
			{
				inline_item.start_box.x = m_line_boxes.back()->left();
				inline_item.start_box.height = inline_item.element->src_el()->css().get_font_metrics().height;
				inline_item.start_box.y = m_line_boxes.back()->top() +
										  baseline_align(m_line_boxes.back()->height(), m_line_boxes.back()->baseline(),
														 inline_item.element->src_el()->css().get_font_metrics().height,
														 inline_item.element->src_el()->css().get_font_metrics().base_line());
			}

			for (const auto &item: m_line_boxes.back()->items())
			{
				if (item->get_type() == line_box_item::type_inline_start)
				{
					auto el_it = std::find_if(m_inlines.begin(), m_inlines.end(), [&](const inlines_item &a)
						{ return a.element == item->get_el(); });
					if (el_it != m_inlines.end())
					{
						// set real position
						el_it->start_box.x = item->pos().x - item->pos().width;
						el_it->start_box.height = el_it->element->src_el()->css().get_font_metrics().height;
						el_it->start_box.y = m_line_boxes.back()->top() +
											 baseline_align(m_line_boxes.back()->height(),
															m_line_boxes.back()->baseline(),
															el_it->element->src_el()->css().get_font_metrics().height,
															el_it->element->src_el()->css().get_font_metrics().base_line());
					}
				} else if (item->get_type() == line_box_item::type_inline_end)
				{
					auto el_it = std::find_if(m_inlines.begin(), m_inlines.end(), [&](const inlines_item &a)
						{ return a.element == item->get_el(); });
					if (el_it != m_inlines.end())
					{
						// set real position
						int end_box_x = item->pos().x + item->pos().width;

						// calculate box
						position pos;
						pos.x = el_it->start_box.x;
						pos.y = el_it->start_box.y - el_it->element->padding_top() - el_it->element->border_top();
						pos.height =
								el_it->start_box.height + el_it->element->padding_top() + el_it->element->border_top() +
								el_it->element->padding_bottom() + el_it->element->border_bottom();
						pos.width = end_box_x - el_it->start_box.x;
						el_it->boxes.push_back(pos);

						// add boxes to the render item
						el_it->element->set_inline_boxes(el_it->boxes);

						// remove from inlines
						m_inlines.erase(el_it);
					}
				}
			}

			// make boxes for all inlines in list
			for (auto &inline_item: m_inlines)
			{
				// Find start item in the removed elements to ignore them
				auto el_it = std::find_if(ret.begin(), ret.end(), [&](const std::unique_ptr<line_box_item> &a)
					{ return a->get_el() == inline_item.element && a->get_type() == line_box_item::type_inline_start; });

				if(el_it == ret.end())
				{
					// set real position
					int end_box_x = m_line_boxes.back()->right();

					// calculate box
					position pos;
					pos.x = inline_item.start_box.x;
					pos.y = inline_item.start_box.y - inline_item.element->padding_top() -
							inline_item.element->border_top();
					pos.height = inline_item.start_box.height + inline_item.element->padding_top() +
								 inline_item.element->border_top() +
								 inline_item.element->padding_bottom() + inline_item.element->border_bottom();
					pos.width = end_box_x - inline_item.start_box.x;
					inline_item.boxes.push_back(pos);
				}
			}
		}
    }
    return ret;
}

int litehtml::render_item_inline_context::new_box(const std::unique_ptr<line_box_item>& el, int max_width, line_context& line_ctx)
{
	auto items = finish_last_box();
	int line_top = 0;
	if(!m_line_boxes.empty())
	{
		line_top = m_line_boxes.back()->bottom();
	}
    line_ctx.top = get_cleared_top(el->get_el(), line_top);

    line_ctx.left = 0;
    line_ctx.right = max_width;
    line_ctx.fix_top();
    get_line_left_right(line_ctx.top, max_width, line_ctx.left, line_ctx.right);

    if(el->get_el()->src_el()->is_inline_box() || el->get_el()->src_el()->is_floats_holder())
    {
        if (el->get_el()->width() > line_ctx.right - line_ctx.left)
        {
            line_ctx.top = find_next_line_top(line_ctx.top, el->get_el()->width(), max_width);
            line_ctx.left = 0;
            line_ctx.right = max_width;
            line_ctx.fix_top();
            get_line_left_right(line_ctx.top, max_width, line_ctx.left, line_ctx.right);
        }
    }

    int first_line_margin = 0;
    int text_indent = 0;
    if(m_line_boxes.empty())
    {
        if(src_el()->css().get_list_style_type() != list_style_type_none && src_el()->css().get_list_style_position() == list_style_position_inside)
        {
            int sz_font = src_el()->css().get_font_size();
            first_line_margin = sz_font;
        }
        if(src_el()->css().get_text_indent().val() != 0)
        {
            text_indent = src_el()->css().get_text_indent().calc_percent(max_width);
        }
    }

    m_line_boxes.emplace_back(std::unique_ptr<line_box>(new line_box(
			line_ctx.top,
			line_ctx.left + first_line_margin + text_indent, line_ctx.right,
			el->get_el()->css().get_line_height(),
			el->get_el()->css().get_font_metrics(),
			css().get_text_align())));
	for(auto& it : items)
	{
		m_line_boxes.back()->add_item(std::move(it));
	}

    return line_ctx.top;
}

int litehtml::render_item_inline_context::place_inline(std::unique_ptr<line_box_item> item, int max_width)
{
    if(item->get_el()->src_el()->css().get_display() == display_none) return 0;

    if(item->get_el()->src_el()->is_float())
    {
        int line_top = 0;
        if(!m_line_boxes.empty())
        {
            line_top = m_line_boxes.back()->top();
        }
        return place_float(item->get_el(), line_top, max_width);
    }

    int ret_width = 0;

    line_context line_ctx = {0};
    line_ctx.top = 0;
    if (!m_line_boxes.empty())
    {
        line_ctx.top = m_line_boxes.back().get()->top();
    }
    line_ctx.left = 0;
    line_ctx.right = max_width;
    line_ctx.fix_top();
    get_line_left_right(line_ctx.top, max_width, line_ctx.left, line_ctx.right);

	if(item->get_type() == line_box_item::type_text_part)
	{
		switch (item->get_el()->src_el()->css().get_display())
		{
			case display_inline_block:
			case display_inline_table:
				ret_width = item->get_el()->render(line_ctx.left, line_ctx.top, line_ctx.right);
				break;
			case display_inline_text:
			{
				litehtml::size sz;
				item->get_el()->src_el()->get_content_size(sz, line_ctx.right);
				item->get_el()->pos() = sz;
			}
				break;
			default:
				ret_width = 0;
				break;
		}
	}

    bool add_box = true;
    if(!m_line_boxes.empty())
    {
        if(m_line_boxes.back()->can_hold(item, src_el()->css().get_white_space()))
        {
            add_box = false;
        }
    }
    if(add_box)
    {
        new_box(item, max_width, line_ctx);
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

    if(!item->get_el()->src_el()->is_inline_box())
    {
        if(m_line_boxes.size() == 1)
        {
            if(collapse_top_margin())
            {
                int shift = item->get_el()->margin_top();
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

            if(prev_margin > item->get_el()->margin_top())
            {
                shift = item->get_el()->margin_top();
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

	auto el = item->get_el();
	m_line_boxes.back()->add_item(std::move(item));

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
