#include "el_button.h"
#include "document.h"
#include "document_container.h"
#include "el_form.h"
#include "html.h"
#include "render_control.h"
#include <algorithm>

namespace litehtml
{
    namespace
    {
        element::ptr find_form_ancestor(const element::ptr& el)
        {
            for(auto par = el ? el->parent() : nullptr; par; par = par->parent())
            {
                if(par->tag() == _form_)
                {
                    return par;
                }
            }
            return nullptr;
        }
    } // namespace

    el_button::el_button(const document::ptr& doc) :
        html_tag(doc)
    {
        m_css->set_display(display_inline_block);
    }

    void el_button::parse_attributes()
    {
        html_tag::parse_attributes();

        m_name  = get_attr("name", "");
        m_value = get_attr("value", "");

        const auto type = lowcase(get_attr("type", "submit"));
        if(type == "reset")
        {
            m_type = button_type::reset;
        } else if(type == "button")
        {
            m_type = button_type::button;
        } else
        {
            m_type = button_type::submit;
        }

        get_text(m_label);
        if(m_label.empty() && m_children.empty())
        {
            if(m_type == button_type::submit)
            {
                m_label = "Submit";
            } else if(m_type == button_type::reset)
            {
                m_label = "Reset";
            }
        }
    }

    bool el_button::has_content_children() const
    {
        return std::any_of(m_children.begin(), m_children.end(),
                           [](const element::ptr& child) { return !child->is_text(); });
    }

    bool el_button::is_replaced() const
    {
        return !has_content_children();
    }

    bool el_button::can_be_focused() const
    {
        return !is_inert() && !is_disabled();
    }

    control_type el_button::get_control_type() const
    {
        switch(m_type)
        {
        case button_type::reset:
            return control_type::reset;
        case button_type::submit:
            return control_type::submit;
        default:
            return control_type::button;
        }
    }

    bool el_button::is_submit_control() const
    {
        return m_type == button_type::submit;
    }

    std::optional<form_field> el_button::create_form_field()
    {
        if(m_name.empty())
        {
            return std::nullopt;
        }
        return litehtml::form_field{m_name, m_value};
    }

    void el_button::on_click()
    {
        if(is_inert() || is_disabled())
        {
            return;
        }
        if(auto form = find_form_ancestor(shared_from_this()))
        {
            if(m_type == button_type::submit)
            {
                get_document()->submit_form(form, shared_from_this());
            } else if(m_type == button_type::reset)
            {
                if(auto form_el = std::dynamic_pointer_cast<el_form>(form))
                {
                    form_el->reset();
                }
            }
        }
        html_tag::on_click();
    }

    void el_button::get_content_size(size& sz, pixel_t /*max_width*/)
    {
        auto font = css().get_font();
        auto fm   = css().get_font_metrics();
        auto tw   = font ? get_document()->container()->text_width(m_label.c_str(), font) : 0_px;
        // The label is the whole content: the paddings and the borders come from the CSS box.
        sz.width  = tw;
        sz.height = fm.height + 4_px;
    }

    void el_button::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip,
                         const std::shared_ptr<render_item>& ri)
    {
        html_tag::draw(hdc, x, y, clip, ri);

        // With child elements the content is laid out and drawn by the render items.
        if(!is_replaced())
        {
            return;
        }

        position pos  = ri->pos();
        pos.x        += x;
        pos.y        += y;
        pos.round();

        if(!pos.does_intersect(clip) || m_label.empty())
        {
            return;
        }

        auto     container = get_document()->container();
        auto     font      = css().get_font();
        auto     fm        = css().get_font_metrics();
        auto     tw        = container->text_width(m_label.c_str(), font);
        auto     th        = fm.height > 0_px ? fm.height : pos.height;
        position text_pos(pos.x + std::max(0_px, (pos.width - tw) / 2_px),
                          pos.y + std::max(0_px, (pos.height - th) / 2_px), tw, th);
        container->set_clip(pos, border_radiuses{});
        container->draw_text(hdc, m_label.c_str(), font, css().get_color(), text_pos);
        container->del_clip();
    }

    std::shared_ptr<render_item> el_button::create_render_item(const std::shared_ptr<render_item>& parent_ri)
    {
        if(css().get_display() == display_none)
        {
            return nullptr;
        }
        if(!is_replaced())
        {
            return element::create_render_item(parent_ri);
        }
        auto ret = std::make_shared<render_item_control>(shared_from_this());
        ret->parent(parent_ri);
        return ret;
    }

    std::string el_button::dump_get_name()
    {
        return "button";
    }
} // namespace litehtml
