#include "html.h"
#include "render_item.h"
#include "document.h"

int litehtml::render_item_block::place_float(const std::shared_ptr<render_item> &el, int top, const containing_block_context &self_size)
{
    int line_top	= get_cleared_top(el, top);
    int line_left	= 0;
    int line_right	= self_size.render_width;
    get_line_left_right(line_top, self_size.render_width, line_left, line_right);

    int ret_width = 0;

    if (el->src_el()->css().get_float() == float_left)
    {
        el->render(line_left, line_top, self_size.new_width(line_right));
        if(el->right() > line_right)
        {
            int new_top = find_next_line_top(el->top(), el->width(), self_size.render_width);
            el->pos().x = get_line_left(new_top) + el->content_offset_left();
            el->pos().y = new_top + el->content_offset_top();
        }
        add_float(el, 0, 0, self_size.context_idx);
		fix_line_width(float_left, self_size);
		ret_width = el->right();
    } else if (el->src_el()->css().get_float() == float_right)
    {
        el->render(0, line_top, self_size.new_width(line_right));

        if(line_left + el->width() > line_right)
        {
            int new_top = find_next_line_top(el->top(), el->width(), self_size.render_width);
            el->pos().x = get_line_right(new_top, self_size.render_width) - el->width() + el->content_offset_left();
            el->pos().y = new_top + el->content_offset_top();
        } else
        {
            el->pos().x = line_right - el->width() + el->content_offset_left();
        }
        add_float(el, 0, 0, self_size.context_idx);
		fix_line_width(float_right, self_size);
		line_left	= 0;
		line_right	= self_size.render_width;
		get_line_left_right(line_top, self_size.render_width, line_left, line_right);

		ret_width = ret_width + (self_size.render_width - line_right);
    }
    return ret_width;
}

int litehtml::render_item_block::get_floats_height(element_float el_float) const
{
    if(src_el()->is_floats_holder())
    {
        int h = 0;

        for(const auto& fb : m_floats_left)
        {
            bool process = false;
            switch(el_float)
            {
                case float_none:
                    process = true;
                    break;
                case float_left:
                    if (fb.clear_floats == clear_left || fb.clear_floats == clear_both)
                    {
                        process = true;
                    }
                    break;
                case float_right:
                    if (fb.clear_floats == clear_right || fb.clear_floats == clear_both)
                    {
                        process = true;
                    }
                    break;
            }
            if(process)
            {
                if(el_float == float_none)
                {
                    h = std::max(h, fb.pos.bottom());
                } else
                {
                    h = std::max(h, fb.pos.top());
                }
            }
        }


        for(const auto& fb : m_floats_right)
        {
            int process = false;
            switch(el_float)
            {
                case float_none:
                    process = true;
                    break;
                case float_left:
                    if (fb.clear_floats == clear_left || fb.clear_floats == clear_both)
                    {
                        process = true;
                    }
                    break;
                case float_right:
                    if (fb.clear_floats == clear_right || fb.clear_floats == clear_both)
                    {
                        process = true;
                    }
                    break;
            }
            if(process)
            {
                if(el_float == float_none)
                {
                    h = std::max(h, fb.pos.bottom());
                } else
                {
                    h = std::max(h, fb.pos.top());
                }
            }
        }

        return h;
    }
    auto el_parent = parent();
    if (el_parent)
    {
        int h = el_parent->get_floats_height(el_float);
        return h - m_pos.y;
    }
    return 0;
}

int litehtml::render_item_block::get_left_floats_height() const
{
    if(src_el()->is_floats_holder())
    {
        int h = 0;
        if(!m_floats_left.empty())
        {
            for (const auto& fb : m_floats_left)
            {
                h = std::max(h, fb.pos.bottom());
            }
        }
        return h;
    }
    auto el_parent = parent();
    if (el_parent)
    {
        int h = el_parent->get_left_floats_height();
        return h - m_pos.y;
    }
    return 0;
}

int litehtml::render_item_block::get_right_floats_height() const
{
    if(src_el()->is_floats_holder())
    {
        int h = 0;
        if(!m_floats_right.empty())
        {
            for(const auto& fb : m_floats_right)
            {
                h = std::max(h, fb.pos.bottom());
            }
        }
        return h;
    }
    auto el_parent = parent();
    if (el_parent)
    {
        int h = el_parent->get_right_floats_height();
        return h - m_pos.y;
    }
    return 0;
}

int litehtml::render_item_block::get_line_left( int y )
{
    if(src_el()->is_floats_holder())
    {
        if(m_cache_line_left.is_valid && m_cache_line_left.hash == y)
        {
            return m_cache_line_left.val;
        }

        int w = 0;
        for(const auto& fb : m_floats_left)
        {
            if (y >= fb.pos.top() && y < fb.pos.bottom())
            {
                w = std::max(w, fb.pos.right());
                if (w < fb.pos.right())
                {
                    break;
                }
            }
        }
        m_cache_line_left.set_value(y, w);
        return w;
    }
    auto el_parent = parent();
    if (el_parent)
    {
        int w = el_parent->get_line_left(y + m_pos.y);
        if (w < 0)
        {
            w = 0;
        }
        return w - (w ? m_pos.x : 0);
    }
    return 0;
}

int litehtml::render_item_block::get_line_right( int y, int def_right )
{
    if(src_el()->is_floats_holder())
    {
        if(m_cache_line_right.is_valid && m_cache_line_right.hash == y)
        {
            if(m_cache_line_right.is_default)
            {
                return def_right;
            } else
            {
                return std::min(m_cache_line_right.val, def_right);
            }
        }

        int w = def_right;
        m_cache_line_right.is_default = true;
        for(const auto& fb : m_floats_right)
        {
            if(y >= fb.pos.top() && y < fb.pos.bottom())
            {
                w = std::min(w, fb.pos.left());
                m_cache_line_right.is_default = false;
                if(w > fb.pos.left())
                {
                    break;
                }
            }
        }
        m_cache_line_right.set_value(y, w);
        return w;
    }
    auto el_parent = parent();
    if (el_parent)
    {
        int w = el_parent->get_line_right(y + m_pos.y, def_right + m_pos.x);
        return w - m_pos.x;
    }
    return 0;
}


void litehtml::render_item_block::get_line_left_right( int y, int def_right, int& ln_left, int& ln_right )
{
    if(src_el()->is_floats_holder())
    {
        ln_left		= get_line_left(y);
        ln_right	= get_line_right(y, def_right);
    } else
    {
        auto el_parent = parent();
        if (el_parent)
        {
            el_parent->get_line_left_right(y + m_pos.y, def_right + m_pos.x, ln_left, ln_right);
        }
        ln_right -= m_pos.x;

        if(ln_left < 0)
        {
            ln_left = 0;
        } else if (ln_left > 0)
        {
            ln_left -= m_pos.x;
            if (ln_left < 0)
            {
                ln_left = 0;
            }
        }
    }
}

void litehtml::render_item_block::clear_floats(int context)
{
	if(src_el()->is_floats_holder())
	{
		auto iter = m_floats_left.begin();
		while(iter != m_floats_left.end())
		{
			if(iter->context >= context)
			{
				iter = m_floats_left.erase(iter);
				m_cache_line_left.invalidate();
			} else
			{
				iter++;
			}
		}

		iter = m_floats_right.begin();
		while(iter != m_floats_right.end())
		{
			if(iter->context >= context)
			{
				iter = m_floats_right.erase(iter);
				m_cache_line_right.invalidate();
			} else
			{
				iter++;
			}
		}
	} else
	{
		auto el_parent = parent();
		if (el_parent)
		{
			el_parent->clear_floats(context);
		}
	}
}

void litehtml::render_item_block::add_float(const std::shared_ptr<render_item> &el, int x, int y, int context)
{
    if(src_el()->is_floats_holder())
    {
        floated_box fb;
        fb.pos.x		= el->left() + x;
        fb.pos.y		= el->top() + y;
        fb.pos.width	= el->width();
        fb.pos.height	= el->height();
        fb.float_side	= el->src_el()->css().get_float();
        fb.clear_floats	= el->src_el()->css().get_clear();
        fb.el			= el;
		fb.context		= context;

        if(fb.float_side == float_left)
        {
            if(m_floats_left.empty())
            {
                m_floats_left.push_back(fb);
            } else
            {
                bool inserted = false;
                for(auto i = m_floats_left.begin(); i != m_floats_left.end(); i++)
                {
                    if(fb.pos.right() > i->pos.right())
                    {
                        m_floats_left.insert(i, std::move(fb));
                        inserted = true;
                        break;
                    }
                }
                if(!inserted)
                {
                    m_floats_left.push_back(std::move(fb));
                }
            }
            m_cache_line_left.invalidate();
        } else if(fb.float_side == float_right)
        {
            if(m_floats_right.empty())
            {
                m_floats_right.push_back(std::move(fb));
            } else
            {
                bool inserted = false;
                for(auto i = m_floats_right.begin(); i != m_floats_right.end(); i++)
                {
                    if(fb.pos.left() < i->pos.left())
                    {
                        m_floats_right.insert(i, std::move(fb));
                        inserted = true;
                        break;
                    }
                }
                if(!inserted)
                {
                    m_floats_right.push_back(fb);
                }
            }
            m_cache_line_right.invalidate();
        }
    } else
    {
        auto el_parent = parent();
        if (el_parent)
        {
            el_parent->add_float(el, x + m_pos.x, y + m_pos.y, context);
        }
    }
}

int litehtml::render_item_block::get_cleared_top(const std::shared_ptr<render_item> &el, int line_top) const
{
    switch(el->src_el()->css().get_clear())
    {
        case clear_left:
        {
            int fh = get_left_floats_height();
            if(fh && fh > line_top)
            {
                line_top = fh;
            }
        }
            break;
        case clear_right:
        {
            int fh = get_right_floats_height();
            if(fh && fh > line_top)
            {
                line_top = fh;
            }
        }
            break;
        case clear_both:
        {
            int fh = get_floats_height(float_none);
            if(fh && fh > line_top)
            {
                line_top = fh;
            }
        }
            break;
        default:
            if(el->src_el()->css().get_float() != float_none)
            {
                int fh = get_floats_height(el->src_el()->css().get_float());
                if(fh && fh > line_top)
                {
                    line_top = fh;
                }
            }
            break;
    }
    return line_top;
}

int litehtml::render_item_block::find_next_line_top( int top, int width, int def_right )
{
    if(src_el()->is_floats_holder())
    {
        int new_top = top;
        int_vector points;

        for(const auto& fb : m_floats_left)
        {
            if(fb.pos.top() >= top)
            {
                if(find(points.begin(), points.end(), fb.pos.top()) == points.end())
                {
                    points.push_back(fb.pos.top());
                }
            }
            if (fb.pos.bottom() >= top)
            {
                if (find(points.begin(), points.end(), fb.pos.bottom()) == points.end())
                {
                    points.push_back(fb.pos.bottom());
                }
            }
        }

        for (const auto& fb : m_floats_right)
        {
            if (fb.pos.top() >= top)
            {
                if (find(points.begin(), points.end(), fb.pos.top()) == points.end())
                {
                    points.push_back(fb.pos.top());
                }
            }
            if (fb.pos.bottom() >= top)
            {
                if (find(points.begin(), points.end(), fb.pos.bottom()) == points.end())
                {
                    points.push_back(fb.pos.bottom());
                }
            }
        }

        if(!points.empty())
        {
            sort(points.begin(), points.end(), std::less<int>( ));
            new_top = points.back();

            for(auto pt : points)
            {
                int pos_left	= 0;
                int pos_right	= def_right;
                get_line_left_right(pt, def_right, pos_left, pos_right);

                if(pos_right - pos_left >= width)
                {
                    new_top = pt;
                    break;
                }
            }
        }
        return new_top;
    }
    auto el_parent = parent();
    if (el_parent)
    {
        int new_top = el_parent->find_next_line_top(top + m_pos.y, width, def_right + m_pos.x);
        return new_top - m_pos.y;
    }
    return 0;
}

void litehtml::render_item_block::update_floats(int dy, const std::shared_ptr<render_item> &_parent)
{
    if(src_el()->is_floats_holder())
    {
        bool reset_cache = false;
        for(auto fb = m_floats_left.rbegin(); fb != m_floats_left.rend(); fb++)
        {
            if(fb->el->src_el()->is_ancestor(_parent->src_el()))
            {
                reset_cache	= true;
                fb->pos.y	+= dy;
            }
        }
        if(reset_cache)
        {
            m_cache_line_left.invalidate();
        }
        reset_cache = false;
        for(auto fb = m_floats_right.rbegin(); fb != m_floats_right.rend(); fb++)
        {
            if(fb->el->src_el()->is_ancestor(_parent->src_el()))
            {
                reset_cache	= true;
                fb->pos.y	+= dy;
            }
        }
        if(reset_cache)
        {
            m_cache_line_right.invalidate();
        }
    } else
    {
        auto el_parent = parent();
        if (el_parent)
        {
            el_parent->update_floats(dy, _parent);
        }
    }
}

std::shared_ptr<litehtml::render_item> litehtml::render_item_block::init()
{
    {
        css_selector sel;
        sel.parse(".inline_rating");
        if(src_el()->select(sel))
        {
            int i = 0;
            i++;
        }
    }
    std::shared_ptr<render_item> ret;

    // Initialize indexes for list items
    if(src_el()->css().get_display() == display_list_item && src_el()->css().get_list_style_type() >= list_style_type_armenian)
    {
        if (auto p = src_el()->parent())
        {
            int val = atoi(p->get_attr("start", "1"));
            for (int i = 0; i < (int)p->get_children_count(); i++)
            {
                auto child = p->get_child(i);
                if (child == src_el())
                {
                    src_el()->set_attr("list_index", std::to_string(val).c_str());
                    break;
                }
                else if (child->css().get_display() == display_list_item)
                    val++;
            }
        }
    }
    // Split inline blocks with box blocks inside
    auto iter = m_children.begin();
    while (iter != m_children.end())
    {
        const auto& el = *iter;
        if(el->src_el()->css().get_display() == display_inline && !el->children().empty())
        {
            auto split_el = el->split_inlines();
            if(std::get<0>(split_el))
            {
                iter = m_children.erase(iter);
                iter = m_children.insert(iter, std::get<2>(split_el));
                iter = m_children.insert(iter, std::get<1>(split_el));
                iter = m_children.insert(iter, std::get<0>(split_el));

                std::get<0>(split_el)->parent(shared_from_this());
                std::get<1>(split_el)->parent(shared_from_this());
                std::get<2>(split_el)->parent(shared_from_this());
                continue;
            }
        }
        ++iter;
    }

    bool has_block_level = false;
	bool has_inlines = false;
	bool has_floats = false;
    for (const auto& el : m_children)
    {
		if(!el->src_el()->is_float())
		{
			if (el->src_el()->is_block_box())
			{
				has_block_level = true;
			} else if (el->src_el()->is_inline_box())
			{
				has_inlines = true;
			}
		}
        if(has_block_level && has_inlines)
            break;
    }
    if(has_block_level)
    {
        ret = std::make_shared<render_item_block_context>(src_el());
        ret->parent(parent());

        auto doc = src_el()->get_document();
        decltype(m_children) new_children;
        decltype(m_children) inlines;
        bool not_ws_added = false;
        for (const auto& el : m_children)
        {
            if(el->src_el()->is_inline_box())
            {
                inlines.push_back(el);
                if(!el->src_el()->is_white_space())
                    not_ws_added = true;
            } else
            {
                if(not_ws_added)
                {
                    auto anon_el = std::make_shared<html_tag>(src_el());
                    auto anon_ri = std::make_shared<render_item_block>(anon_el);
                    for(const auto& inl : inlines)
                    {
                        anon_ri->add_child(inl);
                    }

                    not_ws_added = false;
                    new_children.push_back(anon_ri);
                    anon_ri->parent(ret);
                }
                new_children.push_back(el);
                el->parent(ret);
                inlines.clear();
            }
        }
        if(!inlines.empty() && not_ws_added)
        {
            auto anon_el = std::make_shared<html_tag>(src_el());
            auto anon_ri = std::make_shared<render_item_block>(anon_el);
            for(const auto& inl : inlines)
            {
                anon_ri->add_child(inl);
            }

            new_children.push_back(anon_ri);
            anon_ri->parent(ret);
        }
        ret->children() = new_children;
    }

    if(!ret)
    {
        ret = std::make_shared<render_item_inline_context>(src_el());
        ret->parent(parent());
        ret->children() = children();
        for (const auto &el: ret->children())
        {
            el->parent(ret);
        }
    }

    ret->src_el()->add_render(ret);

    for(auto& el : ret->children())
    {
        el = el->init();
    }

    return ret;
}

int litehtml::render_item_block::render(int x, int y, const containing_block_context &containing_block_size, bool second_pass)
{
    int ret_width = 0;
    calc_outlines(containing_block_size.width);

    m_pos.clear();
    m_pos.move_to(x, y);

    m_pos.x += content_offset_left();
    m_pos.y += content_offset_top();

    m_floats_left.clear();
    m_floats_right.clear();
    m_cache_line_left.invalidate();
    m_cache_line_right.invalidate();

	containing_block_context self_size = calculate_containing_block_context(containing_block_size);

    //*****************************************
    // Render content
    //*****************************************
    ret_width = _render_content(x, y, second_pass, ret_width, self_size);
    //*****************************************

	bool requires_rerender = false;		// when true, the second pass for content rendering is required

	// Set block width
	if(self_size.width.type == containing_block_context::cbc_value_type_absolute)
	{
		ret_width = m_pos.width = self_size.render_width;
	} else if(self_size.width.type == containing_block_context::cbc_value_type_percentage)
	{
		m_pos.width = self_size.render_width;
	} else
	{
		if(src_el()->is_inline_box() ||
			src_el()->css().get_float() != float_none ||
			src_el()->css().get_display() == display_table_cell ||
			src_el()->css().get_display() == display_table_caption ||
			src_el()->css().get_position() > element_position_relative)
		{
			m_pos.width = ret_width;
			if(ret_width < self_size.render_width && !second_pass)
			{
				// We have to render content again with new max_width
				requires_rerender = true;
			}
		} else
		{
			m_pos.width = self_size.render_width;
		}
	}

	// Fix width with min-width attribute
	if(self_size.min_width.type != containing_block_context::cbc_value_type_none)
	{
		if(m_pos.width < self_size.min_width)
		{
			m_pos.width = self_size.min_width;
			requires_rerender = true;
		}
	}

	// Fix width with max-width attribute
	if(self_size.max_width.type != containing_block_context::cbc_value_type_none)
	{
		if(m_pos.width > self_size.max_width)
		{
			m_pos.width = self_size.max_width;
			requires_rerender = true;
		}
	}

	// re-render content with new width if required
	if (requires_rerender && !second_pass && !is_root())
	{
		if(src_el()->is_floats_holder())
		{
			m_floats_left.clear();
			m_floats_right.clear();
		} else
		{
			clear_floats(self_size.context_idx);
		}
		m_cache_line_left.invalidate();
		m_cache_line_right.invalidate();

		_render_content(x, y, true, ret_width, self_size.new_width(m_pos.width));
	}

	// Set block height
	if (self_size.height.type != containing_block_context::cbc_value_type_auto)
	{
		m_pos.height = self_size.height;
		if(src_el()->css().get_box_sizing() == box_sizing_border_box)
		{
			m_pos.height -= box_sizing_height();
		}
	} else if (src_el()->is_floats_holder())
    {
		// add the floats' height to the block height
        int floats_height = get_floats_height();
        if (floats_height > m_pos.height)
        {
            m_pos.height = floats_height;
        }
    }

	// Fix height with min-height attribute
	if(self_size.min_height.type != containing_block_context::cbc_value_type_none)
	{
		if(m_pos.height < self_size.min_height)
		{
			m_pos.height = self_size.min_height;
		}
	}

	// Fix width with max-width attribute
	if(self_size.max_height.type != containing_block_context::cbc_value_type_none)
	{
		if(m_pos.height > self_size.max_height)
		{
			m_pos.height = self_size.max_height;
		}
	}

    // calculate the final position
    m_pos.move_to(x, y);
    m_pos.x += content_offset_left();
    m_pos.y += content_offset_top();

    if (src_el()->css().get_display() == display_list_item)
    {
        string list_image = src_el()->css().get_list_style_image();
        if (!list_image.empty())
        {
            size sz;
            string list_image_baseurl = src_el()->css().get_list_style_image_baseurl();
            src_el()->get_document()->container()->get_image_size(list_image.c_str(), list_image_baseurl.c_str(), sz);
            if (m_pos.height < sz.height)
            {
				m_pos.height = sz.height;
            }
        }

    }

    if (src_el()->is_floats_holder() && !second_pass)
    {
        for (const auto& fb : m_floats_left)
        {
            fb.el->apply_relative_shift(containing_block_size);
        }
    }
    return ret_width + content_offset_width();
}
