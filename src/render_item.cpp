#include "html.h"
#include "render_item.h"
#include "document.h"
#include <typeinfo>
#include <utf8_strings.h>

litehtml::render_item::render_item(std::shared_ptr<element>  _src_el) :
        m_element(std::move(_src_el)),
        m_skip(false),
        m_box(nullptr)
{
    document::ptr doc = src_el()->get_document();
    auto fnt_size = src_el()->css().get_font_size();

    m_margins.left		= doc->to_pixels(src_el()->css().get_margins().left,         fnt_size);
    m_margins.right		= doc->to_pixels(src_el()->css().get_margins().right,        fnt_size);
    m_margins.top		= doc->to_pixels(src_el()->css().get_margins().top,          fnt_size);
    m_margins.bottom	= doc->to_pixels(src_el()->css().get_margins().bottom,       fnt_size);

    m_padding.left		= doc->to_pixels(src_el()->css().get_padding().left,         fnt_size);
    m_padding.right		= doc->to_pixels(src_el()->css().get_padding().right,        fnt_size);
    m_padding.top		= doc->to_pixels(src_el()->css().get_padding().top,          fnt_size);
    m_padding.bottom	= doc->to_pixels(src_el()->css().get_padding().bottom,       fnt_size);

    m_borders.left		= doc->to_pixels(src_el()->css().get_borders().left.width,   fnt_size);
    m_borders.right		= doc->to_pixels(src_el()->css().get_borders().right.width,  fnt_size);
    m_borders.top		= doc->to_pixels(src_el()->css().get_borders().top.width,    fnt_size);
    m_borders.bottom	= doc->to_pixels(src_el()->css().get_borders().bottom.width, fnt_size);
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

void litehtml::render_item::calc_auto_margins(int parent_width)
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
        }
        else if (src_el()->css().get_margins().left.is_predefined() && !src_el()->css().get_margins().right.is_predefined())
        {
            int el_width = m_pos.width + m_borders.left + m_borders.right + m_padding.left + m_padding.right + m_margins.right;
            m_margins.left = parent_width - el_width;
            if (m_margins.left < 0) m_margins.left = 0;
        }
        else if (!src_el()->css().get_margins().left.is_predefined() && src_el()->css().get_margins().right.is_predefined())
        {
            int el_width = m_pos.width + m_borders.left + m_borders.right + m_padding.left + m_padding.right + m_margins.left;
            m_margins.right = parent_width - el_width;
            if (m_margins.right < 0) m_margins.right = 0;
        }
    }
}

void litehtml::render_item::apply_relative_shift(int parent_width)
{
    if (src_el()->css().get_position() == element_position_relative)
    {
        css_offsets offsets = src_el()->css().get_offsets();
        if (!offsets.left.is_predefined())
        {
            m_pos.x += offsets.left.calc_percent(parent_width);
        }
        else if (!offsets.right.is_predefined())
        {
            m_pos.x -= offsets.right.calc_percent(parent_width);
        }
        if (!offsets.top.is_predefined())
        {
            int h = 0;

            if (offsets.top.units() == css_units_percentage)
            {
                auto el_parent = parent();
                if (el_parent)
                {
                    el_parent->get_predefined_height(h);
                }
            }

            m_pos.y += offsets.top.calc_percent(h);
        }
        else if (!offsets.bottom.is_predefined())
        {
            int h = 0;

            if (offsets.top.units() == css_units_percentage)
            {
                auto el_parent = parent();
                if (el_parent)
                {
                    el_parent->get_predefined_height(h);
                }
            }

            m_pos.y -= offsets.bottom.calc_percent(h);
        }
    }
}

bool litehtml::render_item::get_predefined_height(int& p_height) const
{
    css_length h = src_el()->css().get_height();
    if(h.is_predefined())
    {
        p_height = m_pos.height;
        return false;
    }
    if(h.units() == css_units_percentage)
    {
        auto el_parent = parent();
        if (!el_parent)
        {
            position client_pos;
            src_el()->get_document()->container()->get_client_rect(client_pos);
            p_height = h.calc_percent(client_pos.height);
            return true;
        } else
        {
            int ph = 0;
            if (el_parent->get_predefined_height(ph))
            {
                p_height = h.calc_percent(ph);
                if (src_el()->is_body())
                {
                    p_height -= content_margins_height();
                }
                return true;
            } else
            {
                p_height = m_pos.height;
                return false;
            }
        }
    }
    p_height = src_el()->get_document()->to_pixels(h, src_el()->css().get_font_size());
    return true;
}

int litehtml::render_item::get_inline_shift_left()
{
    int ret = 0;
    auto el_parent = parent();
    if (el_parent)
    {
        if (el_parent->src_el()->css().get_display() == display_inline)
        {
            style_display disp = src_el()->css().get_display();

            if (disp == display_inline_text || disp == display_inline_block)
            {
                auto el = shared_from_this();
                while (el_parent && el_parent->src_el()->css().get_display() == display_inline)
                {
                    if (el_parent->is_first_child_inline(el))
                    {
                        ret += el_parent->padding_left() + el_parent->border_left() + el_parent->margin_left();
                    } else
                        break;
                    el = el_parent;
                    el_parent = el_parent->parent();
                }
            }
        }
    }

    return ret;
}

int litehtml::render_item::get_inline_shift_right()
{
    int ret = 0;
    auto el_parent = parent();
    if (el_parent)
    {
        if (el_parent->src_el()->css().get_display() == display_inline)
        {
            style_display disp = src_el()->css().get_display();

            if (disp == display_inline_text || disp == display_inline_block)
            {
                auto el = shared_from_this();
                while (el_parent && el_parent->src_el()->css().get_display() == display_inline)
                {
                    if (el_parent->is_last_child_inline(el))
                    {
                        ret += el_parent->padding_right() + el_parent->border_right() + el_parent->margin_right();
                    } else
                        break;
                    el = el_parent;
                    el_parent = el_parent->parent();
                }
            }
        }
    }

    return ret;
}

bool litehtml::render_item::is_first_child_inline(const std::shared_ptr<render_item>& el) const
{
    if(!m_children.empty())
    {
        for (const auto& this_el : m_children)
        {
            if (!this_el->src_el()->is_white_space())
            {
                if (el == this_el)
                {
                    return true;
                }
                if (this_el->src_el()->css().get_display() == display_inline)
                {
                    if (this_el->have_inline_child())
                    {
                        return false;
                    }
                } else
                {
                    return false;
                }
            }
        }
    }
    return false;
}

bool litehtml::render_item::is_last_child_inline(const std::shared_ptr<render_item>& el) const
{
    if(!m_children.empty())
    {
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it)
        {
            const auto& this_el = *it;
            if (!this_el->src_el()->is_white_space())
            {
                if (el == this_el)
                {
                    return true;
                }
                if (this_el->src_el()->css().get_display() == display_inline)
                {
                    if (this_el->have_inline_child())
                    {
                        return false;
                    }
                } else
                {
                    return false;
                }
            }
        }
    }
    return false;
}

bool litehtml::render_item::have_inline_child() const
{
    if(!m_children.empty())
    {
        for(const auto& el : m_children)
        {
            if(!el->src_el()->is_white_space())
            {
                return true;
            }
        }
    }
    return false;
}

int litehtml::render_item::calc_width(int defVal) const
{
    css_length w = src_el()->css().get_width();
    if(w.is_predefined() || src_el()->css().get_display() == display_table_cell)
    {
        return defVal;
    }
    if(w.units() == css_units_percentage)
    {
        auto el_parent = parent();
        if (!el_parent)
        {
            position client_pos;
            src_el()->get_document()->container()->get_client_rect(client_pos);
            return w.calc_percent(client_pos.width) - content_margins_width();
        } else
        {
            int pw = el_parent->calc_width(defVal);
            if (src_el()->is_body())
            {
                pw -= content_margins_width();
            }
            return w.calc_percent(pw);
        }
    }
    return 	src_el()->get_document()->to_pixels(w, src_el()->css().get_font_size());
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
            int parent_height	= 0;
            int parent_width	= 0;
            int client_x		= 0;
            int client_y		= 0;
            if(el_position == element_position_fixed)
            {
                parent_height	= wnd_position.height;
                parent_width	= wnd_position.width;
                client_x		= wnd_position.left();
                client_y		= wnd_position.top();
            } else
            {
                auto el_parent = el->parent();
                if(el_parent)
                {
                    parent_height	= el_parent->height();
                    parent_width	= el_parent->width();
                }
            }

            css_length	css_left	= el->src_el()->css().get_offsets().left;
            css_length	css_right	= el->src_el()->css().get_offsets().right;
            css_length	css_top		= el->src_el()->css().get_offsets().top;
            css_length	css_bottom	= el->src_el()->css().get_offsets().bottom;

            bool need_render = false;

            css_length el_w = el->src_el()->css().get_width();
            css_length el_h = el->src_el()->css().get_height();

            int new_width = -1;
            int new_height = -1;
            if(el_w.units() == css_units_percentage && parent_width)
            {
                new_width = el_w.calc_percent(parent_width);
                if(el->m_pos.width != new_width)
                {
                    need_render = true;
                    el->m_pos.width = new_width;
                }
            }

            if(el_h.units() == css_units_percentage && parent_height)
            {
                new_height = el_h.calc_percent(parent_height);
                if(el->m_pos.height != new_height)
                {
                    need_render = true;
                    el->m_pos.height = new_height;
                }
            }

            bool cvt_x = false;
            bool cvt_y = false;

            if(el_position == element_position_fixed)
            {
                if(!css_left.is_predefined() || !css_right.is_predefined())
                {
                    if(!css_left.is_predefined() && css_right.is_predefined())
                    {
                        el->m_pos.x = css_left.calc_percent(parent_width) + el->content_margins_left();
                    } else if(css_left.is_predefined() && !css_right.is_predefined())
                    {
                        el->m_pos.x = parent_width - css_right.calc_percent(parent_width) - el->m_pos.width - el->content_margins_right();
                    } else
                    {
                        el->m_pos.x		= css_left.calc_percent(parent_width) + el->content_margins_left();
                        el->m_pos.width	= parent_width - css_left.calc_percent(parent_width) - css_right.calc_percent(parent_width) - (el->content_margins_left() + el->content_margins_right());
                        need_render = true;
                    }
                }

                if(!css_top.is_predefined() || !css_bottom.is_predefined())
                {
                    if(!css_top.is_predefined() && css_bottom.is_predefined())
                    {
                        el->m_pos.y = css_top.calc_percent(parent_height) + el->content_margins_top();
                    } else if(css_top.is_predefined() && !css_bottom.is_predefined())
                    {
                        el->m_pos.y = parent_height - css_bottom.calc_percent(parent_height) - el->m_pos.height - el->content_margins_bottom();
                    } else
                    {
                        el->m_pos.y			= css_top.calc_percent(parent_height) + el->content_margins_top();
                        el->m_pos.height	= parent_height - css_top.calc_percent(parent_height) - css_bottom.calc_percent(parent_height) - (el->content_margins_top() + el->content_margins_bottom());
                        need_render = true;
                    }
                }
            } else
            {
                if(!css_left.is_predefined() || !css_right.is_predefined())
                {
                    if(!css_left.is_predefined() && css_right.is_predefined())
                    {
                        el->m_pos.x = css_left.calc_percent(parent_width) + el->content_margins_left() - m_padding.left;
                    } else if(css_left.is_predefined() && !css_right.is_predefined())
                    {
                        el->m_pos.x = m_pos.width + m_padding.right - css_right.calc_percent(parent_width) - el->m_pos.width - el->content_margins_right();
                    } else
                    {
                        el->m_pos.x		= css_left.calc_percent(parent_width) + el->content_margins_left() - m_padding.left;
                        el->m_pos.width	= m_pos.width + m_padding.left + m_padding.right - css_left.calc_percent(parent_width) - css_right.calc_percent(parent_width) - (el->content_margins_left() + el->content_margins_right());
                        if (new_width != -1)
                        {
                            el->m_pos.x += (el->m_pos.width - new_width) / 2;
                            el->m_pos.width = new_width;
                        }
                        need_render = true;
                    }
                    cvt_x = true;
                }

                if(!css_top.is_predefined() || !css_bottom.is_predefined())
                {
                    if(!css_top.is_predefined() && css_bottom.is_predefined())
                    {
                        el->m_pos.y = css_top.calc_percent(parent_height) + el->content_margins_top() - m_padding.top;
                    } else if(css_top.is_predefined() && !css_bottom.is_predefined())
                    {
                        el->m_pos.y = m_pos.height + m_padding.bottom - css_bottom.calc_percent(parent_height) - el->m_pos.height - el->content_margins_bottom();
                    } else
                    {
                        el->m_pos.y			= css_top.calc_percent(parent_height) + el->content_margins_top() - m_padding.top;
                        el->m_pos.height	= m_pos.height + m_padding.top + m_padding.bottom - css_top.calc_percent(parent_height) - css_bottom.calc_percent(parent_height) - (el->content_margins_top() + el->content_margins_bottom());
                        if (new_height != -1)
                        {
                            el->m_pos.y += (el->m_pos.height - new_height) / 2;
                            el->m_pos.height = new_height;
                        }
                        need_render = true;
                    }
                    cvt_y = true;
                }
            }

            if(cvt_x || cvt_y)
            {
                int offset_x = 0;
                int offset_y = 0;
                auto cur_el = el->parent();
                auto this_el = shared_from_this();
                while(cur_el && cur_el != this_el)
                {
                    offset_x += cur_el->m_pos.x;
                    offset_y += cur_el->m_pos.y;
                    cur_el = cur_el->parent();
                }
                if(cvt_x)	el->m_pos.x -= offset_x;
                if(cvt_y)	el->m_pos.y -= offset_y;
            }

            if(need_render)
            {
                position pos = el->m_pos;
                el->_render(el->left(), el->top(), el->width(), true);
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
    if (src_el()->css().get_position() != element_position_static || (!have_parent()))
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

void litehtml::render_item::calc_document_size( litehtml::size& sz, int x /*= 0*/, int y /*= 0*/ )
{
    if(is_visible() && src_el()->css().get_position() != element_position_fixed)
    {
        sz.width	= std::max(sz.width,	x + right());
        sz.height	= std::max(sz.height,	y + bottom());

        if(src_el()->css().get_overflow() == overflow_visible)
        {
            for(auto& el : m_children)
            {
                el->calc_document_size(sz, x + m_pos.x, y + m_pos.y);
            }
        }

        // root element (<html>) have to cover entire window
        if(!have_parent())
        {
            position client_pos;
            src_el()->get_document()->container()->get_client_rect(client_pos);
            m_pos.height = std::max(sz.height, client_pos.height) - content_margins_top() - content_margins_bottom();
            m_pos.width	 = std::max(sz.width, client_pos.width) - content_margins_left() - content_margins_right();
        }
    }
}

void litehtml::render_item::get_inline_boxes( position::vector& boxes )
{
    if(src_el()->css().get_display() == display_table_row)
    {
        position pos;
        for(auto& el : m_children)
        {
            if(el->src_el()->css().get_display() == display_table_cell)
            {
                pos.x		= el->left() + el->margin_left();
                pos.y		= el->top() - m_padding.top - m_borders.top;

                pos.width	= el->right() - pos.x - el->margin_right() - el->margin_left();
                pos.height	= el->height() + m_padding.top + m_padding.bottom + m_borders.top + m_borders.bottom;

                boxes.push_back(pos);
            }
        }
    } else
    {

        litehtml::line_box *old_box = nullptr;
        position pos;
        for (auto &el: m_children)
        {
            if (!el->skip())
            {
                if (el->m_box)
                {
                    if (el->m_box != old_box)
                    {
                        if (old_box)
                        {
                            if (boxes.empty())
                            {
                                pos.x -= m_padding.left + m_borders.left;
                                pos.width += m_padding.left + m_borders.left;
                            }
                            boxes.push_back(pos);
                        }
                        old_box = el->m_box;
                        pos.x = el->left() + el->margin_left();
                        pos.y = el->top() - m_padding.top - m_borders.top;
                        pos.width = 0;
                        pos.height = 0;
                    }
                    pos.width = el->right() - pos.x - el->margin_right() - el->margin_left();
                    pos.height = std::max(pos.height, el->height() + m_padding.top + m_padding.bottom + m_borders.top +
                                                      m_borders.bottom);
                } else if (el->src_el()->css().get_display() == display_inline)
                {
                    position::vector sub_boxes;
                    el->get_inline_boxes(sub_boxes);
                    if (!sub_boxes.empty())
                    {
                        sub_boxes.rbegin()->width += el->margin_right();
                        if (boxes.empty())
                        {
                            if (m_padding.left + m_borders.left > 0)
                            {
                                position padding_box = (*sub_boxes.begin());
                                padding_box.x -= m_padding.left + m_borders.left + el->margin_left();
                                padding_box.width = m_padding.left + m_borders.left + el->margin_left();
                                boxes.push_back(padding_box);
                            }
                        }

                        sub_boxes.rbegin()->width += el->margin_right();

                        boxes.insert(boxes.end(), sub_boxes.begin(), sub_boxes.end());
                    }
                }
            }
        }
        if (pos.width || pos.height)
        {
            if (boxes.empty())
            {
                pos.x -= m_padding.left + m_borders.left;
                pos.width += m_padding.left + m_borders.left;
            }
            boxes.push_back(pos);
        }
        if (!boxes.empty())
        {
            if (m_padding.right + m_borders.right > 0)
            {
                boxes.back().width += m_padding.right + m_borders.right;
            }
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

            doc->container()->set_clip(pos, bdr_radius, true, true);
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
                    if (!el->src_el()->is_inline_box() && el->src_el()->css().get_float() == float_none && !el->src_el()->is_positioned())
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
                    if (el->src_el()->is_inline_box() && el->src_el()->css().get_float() == float_none && !el->src_el()->is_positioned())
                    {
                        el->src_el()->draw(hdc, pos.x, pos.y, clip, el);
                        if (el->src_el()->css().get_display() == display_inline_block)
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

    for(auto i = m_children.begin(); i != m_children.end() && !ret; std::advance(i, 1))
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
                    if(!el->src_el()->is_inline_box() && el->src_el()->css().get_float() == float_none && !el->src_el()->is_positioned())
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
                    if(el->src_el()->is_inline_box() && el->src_el()->css().get_float() == float_none && !el->src_el()->is_positioned())
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
                           el->src_el()->css().get_display() != display_inline_block)
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

    for(const auto& zindex : z_indexes)
    {
        if(zindex.first > 0)
        {
            ret = get_child_by_point(x, y, client_x, client_y, draw_positioned, zindex.first);
            break;
        }
    }
    if(ret) return ret;

    for(const auto& z_index : z_indexes)
    {
        if(z_index.first == 0)
        {
            ret = get_child_by_point(x, y, client_x, client_y, draw_positioned, z_index.first);
            break;
        }
    }
    if(ret) return ret;

    ret = get_child_by_point(x, y, client_x, client_y, draw_inlines, 0);
    if(ret) return ret;

    ret = get_child_by_point(x, y, client_x, client_y, draw_floats, 0);
    if(ret) return ret;

    ret = get_child_by_point(x, y, client_x, client_y, draw_block, 0);
    if(ret) return ret;


    for(const auto& z_index : z_indexes)
    {
        if(z_index.first < 0)
        {
            ret = get_child_by_point(x, y, client_x, client_y, draw_positioned, z_index.first);
            break;
        }
    }
    if(ret) return ret;

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
        position::vector boxes;
        get_inline_boxes(boxes);
        for(auto & box : boxes)
        {
            redraw_boxes.push_back(box);
        }
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
    cout.begin_node(src_el()->dump_get_name() + _t("{") + (const tstring) litehtml_from_utf8(typeid(*this).name()) + _t("}"));

    auto attrs = src_el()->dump_get_attrs();
    if(!attrs.empty())
    {
        cout.begin_attrs_group(_t("attributes"));
        for (const auto &attr: attrs)
        {
            cout.add_attr(std::get<0>(attr), std::get<1>(attr));
        }
        cout.end_attrs_group();
    }

    if(!m_children.empty())
    {
        cout.begin_attrs_group(_t("children"));
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
