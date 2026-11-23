#include "el_input.h"
#include "control_text_layout.h"
#include "document.h"
#include "document_container.h"
#include "el_form.h"
#include "html.h"
#include "html_microsyntaxes.h"
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

        pixel_t text_line_height(const html_tag& el)
        {
            const auto line_height = el.css().line_height().computed_value;
            if(line_height > 0_px)
            {
                return line_height;
            }
            const auto height = el.css().get_font_metrics().height;
            return height > 0_px ? height : pixel_t(16);
        }
    } // namespace

    el_input::el_input(const document::ptr& doc) :
        html_tag(doc)
    {
        m_css->set_display(display_inline_block);
    }

    void el_input::parse_type(const std::string& type)
    {
        const auto name = lowcase(type);
        if(name == "button")
        {
            m_type = input_type::button;
        } else if(name == "checkbox")
        {
            m_type = input_type::checkbox;
        } else if(name == "hidden")
        {
            m_type = input_type::hidden;
        } else if(name == "image")
        {
            m_type = input_type::image;
        } else if(name == "number")
        {
            m_type = input_type::number;
        } else if(name == "password")
        {
            m_type = input_type::password;
        } else if(name == "radio")
        {
            m_type = input_type::radio;
        } else if(name == "reset")
        {
            m_type = input_type::reset;
        } else if(name == "search")
        {
            m_type = input_type::search;
        } else if(name == "submit")
        {
            m_type = input_type::submit;
        } else
        {
            m_type = input_type::text;
        }
    }

    void el_input::parse_attributes()
    {
        html_tag::parse_attributes();

        m_name = get_attr("name", "");
        parse_type(get_attr("type", "text"));

        switch(m_type)
        {
        case input_type::text:
        case input_type::number:
        case input_type::password:
        case input_type::search:
            m_value         = get_attr("value", "");
            m_initial_value = m_value;
            m_text.text     = m_value;
            m_text.caret    = m_text.text.size();
            {
                int size = 20;
                if(const char* attr = get_attr("size"))
                {
                    html_parse_non_negative_integer(attr, size);
                }
                if(size > 0)
                {
                    css_token tok(DIMENSION, static_cast<float>(size), css_number_integer, "ch");
                    add_attr_property(_width_, {tok});
                }
            }
            break;
        case input_type::checkbox:
        case input_type::radio:
            m_value         = get_attr("value", "on");
            m_initial_value = m_value;
            m_checked       = get_attr("checked") != nullptr;
            break;
        case input_type::hidden:
        case input_type::button:
        case input_type::submit:
        case input_type::reset:
            m_value         = get_attr("value", "");
            m_initial_value = m_value;
            break;
        case input_type::image:
            m_src = get_attr("src", "");
            m_alt = get_attr("alt", "");
            break;
        }
    }

    bool el_input::is_replaced() const
    {
        return true;
    }

    bool el_input::can_be_focused() const
    {
        return !is_inert() && !is_disabled() && m_type != input_type::hidden;
    }

    bool el_input::is_checked() const
    {
        if(m_checked.has_value())
        {
            return *m_checked;
        }
        return get_attr("checked") != nullptr;
    }

    control_type el_input::get_control_type() const
    {
        switch(m_type)
        {
        case input_type::button:
            return control_type::button;
        case input_type::checkbox:
            return control_type::checkbox;
        case input_type::hidden:
            return control_type::none;
        case input_type::image:
            return control_type::image;
        case input_type::number:
            return control_type::number;
        case input_type::password:
            return control_type::password;
        case input_type::radio:
            return control_type::radio;
        case input_type::reset:
            return control_type::reset;
        case input_type::search:
            return control_type::search;
        case input_type::submit:
            return control_type::submit;
        case input_type::text:
            return control_type::text;
        }
        return control_type::none;
    }

    control_data el_input::get_control_data() const
    {
        switch(m_type)
        {
        case input_type::checkbox:
        case input_type::radio:
            return is_checked();
        case input_type::text:
        case input_type::number:
        case input_type::password:
        case input_type::search:
            return m_text;
        default:
            return {};
        }
    }

    void el_input::set_checked(bool checked)
    {
        m_checked = checked;
        set_pseudo_class(_checked_, checked);
    }

    bool el_input::select_radio()
    {
        bool         changed = false;
        element::ptr scope   = find_form_ancestor(shared_from_this());
        if(!scope)
        {
            scope = get_document()->root();
        }

        const auto uncheck = [&](const auto& self, const element::ptr& parent) -> void {
            for(const auto& child : parent->children())
            {
                if(auto* input = dynamic_cast<el_input*>(child.get()))
                {
                    if(child.get() != this && input->m_type == input_type::radio && input->m_name == m_name &&
                       input->is_checked())
                    {
                        input->set_checked(false);
                        changed = true;
                    }
                }
                self(self, child);
            }
        };

        if(scope)
        {
            uncheck(uncheck, scope);
        }
        set_checked(true);
        return changed;
    }

    void el_input::notify_change()
    {
        if(auto* container = get_document()->container())
        {
            container->on_change(shared_from_this(), get_attr("id"), get_control_data());
        }
    }

    bool el_input::update_focused(const control_data& data)
    {
        if(const auto checked = std::get_if<bool>(&data))
        {
            if(m_type == input_type::checkbox)
            {
                if(*checked != is_checked())
                {
                    set_checked(*checked);
                    notify_change();
                    return true;
                }
            } else if(m_type == input_type::radio)
            {
                if(*checked && !is_checked())
                {
                    const bool styles_changed = select_radio();
                    notify_change();
                    return styles_changed;
                }
            }
            return false;
        }

        if(const auto text = std::get_if<text_control_data>(&data))
        {
            if(!is_text_control())
            {
                return false;
            }
            const bool value_changed = text->text != m_text.text;
            m_text                   = *text;
            m_value                  = m_text.text;
            if(value_changed)
            {
                notify_change();
            }
        }
        return false;
    }

    void el_input::on_click()
    {
        if(is_inert() || is_disabled())
        {
            return;
        }

        switch(m_type)
        {
        case input_type::checkbox:
            set_checked(!is_checked());
            notify_change();
            break;
        case input_type::radio:
            if(!is_checked())
            {
                select_radio();
                notify_change();
            }
            break;
        case input_type::submit:
        case input_type::image:
            if(auto form = find_form_ancestor(shared_from_this()))
            {
                get_document()->submit_form(form, shared_from_this());
            }
            break;
        case input_type::reset:
            if(auto form = find_form_ancestor(shared_from_this()))
            {
                if(auto form_el = std::dynamic_pointer_cast<el_form>(form))
                {
                    form_el->reset();
                }
            }
            break;
        default:
            break;
        }

        html_tag::on_click();
    }

    void el_input::reset()
    {
        if(is_text_control())
        {
            m_value                = m_initial_value;
            m_text.text            = m_initial_value;
            m_text.caret           = m_initial_value.size();
            m_text.selection_start = m_text.selection_end = m_text.caret;
            m_text_scroll                                 = 0_px;
        } else if(m_type == input_type::checkbox || m_type == input_type::radio)
        {
            set_checked(get_attr("checked") != nullptr);
        }
    }

    bool el_input::is_submit_control() const
    {
        return m_type == input_type::submit || m_type == input_type::image;
    }

    bool el_input::is_text_control() const
    {
        return m_type == input_type::text || m_type == input_type::number || m_type == input_type::password ||
               m_type == input_type::search;
    }

    bool el_input::has_name() const
    {
        return !m_name.empty();
    }

    const std::string& el_input::name() const
    {
        return m_name;
    }

    const std::string& el_input::src() const
    {
        return m_src;
    }

    input_type el_input::type() const
    {
        return m_type;
    }

    std::string el_input::label() const
    {
        if(m_type == input_type::submit)
        {
            return m_value.empty() ? "Submit" : m_value;
        }
        if(m_type == input_type::reset)
        {
            return m_value.empty() ? "Reset" : m_value;
        }
        if(m_type == input_type::image)
        {
            return m_alt.empty() ? "Submit" : m_alt;
        }
        return m_value;
    }

    std::string el_input::current_value()
    {
        if(is_text_control())
        {
            text_control_data data = m_text;
            if(auto* container = get_document()->container())
            {
                container->get_text_data(shared_from_this(), data);
            }
            m_text  = data;
            m_value = m_text.text;
        }
        return m_value;
    }

    std::optional<form_field> el_input::create_form_field(bool submitted)
    {
        if(!has_name())
        {
            return std::nullopt;
        }

        switch(m_type)
        {
        case input_type::hidden:
        case input_type::text:
        case input_type::number:
        case input_type::password:
        case input_type::search:
            return litehtml::form_field{m_name, current_value()};
        case input_type::checkbox:
        case input_type::radio:
            if(is_checked())
            {
                return litehtml::form_field{m_name, m_value};
            }
            return std::nullopt;
        case input_type::submit:
        case input_type::image:
            if(submitted)
            {
                return litehtml::form_field{m_name, label()};
            }
            return std::nullopt;
        default:
            return std::nullopt;
        }
    }

    void el_input::compute_styles(bool recursive)
    {
        html_tag::compute_styles(recursive);

        if(m_type == input_type::image && !m_src.empty())
        {
            const bool redraw_on_ready = !css().get_width().is_predefined() && !css().get_height().is_predefined();
            get_document()->container()->load_image(m_src.c_str(), nullptr, redraw_on_ready);
        }
    }

    void el_input::get_content_size(size& sz, pixel_t /*max_width*/)
    {
        sz.width  = 0_px;
        sz.height = 0_px;

        const auto fm = css().get_font_metrics();

        switch(m_type)
        {
        case input_type::hidden:
            break;
        case input_type::checkbox:
        case input_type::radio:
            sz.width  = 13_px;
            sz.height = 13_px;
            break;
        case input_type::image:
            get_document()->container()->get_image_size(m_src.c_str(), nullptr, sz);
            if(sz.width == 0_px || sz.height == 0_px)
            {
                const auto font = css().get_font();
                const auto text = label();
                sz.width  = text.empty() ? pixel_t(0) : get_document()->container()->text_width(text.c_str(), font);
                sz.height = fm.height;
            }
            break;
        case input_type::text:
        case input_type::number:
        case input_type::password:
        case input_type::search:
            {
                int chars = 20;
                if(const char* attr = get_attr("size"))
                {
                    html_parse_non_negative_integer(attr, chars);
                }
                const auto ch_width = fm.ch_width > 0_px ? fm.ch_width : pixel_t(8);
                sz.width            = pixel_t(chars) * ch_width;
                sz.height           = fm.height;
            }
            break;
        default:
            {
                const auto font = css().get_font();
                const auto text = label();
                if(font && !text.empty())
                {
                    sz.width = get_document()->container()->text_width(text.c_str(), font) + 20_px;
                } else
                {
                    sz.width = 20_px;
                }
                sz.height = fm.height + 4_px;
            }
            break;
        }
    }

    void el_input::fill_rect(uint_ptr hdc, const position& pos, const web_color& color)
    {
        background_layer layer;
        layer.border_box = pos;
        layer.clip_box   = pos;
        layer.origin_box = pos;
        layer.repeat     = background_repeat_no_repeat;
        get_document()->container()->draw_solid_fill(hdc, layer, color);
    }

    void el_input::draw_text_control(uint_ptr hdc, const position& pos)
    {
        if(pos.width <= 0_px || pos.height <= 0_px)
        {
            return;
        }

        text_control_data data = m_text;
        if(auto* container = get_document()->container())
        {
            container->get_text_data(shared_from_this(), data);
        }
        m_text  = data;
        m_value = m_text.text;

        auto container = get_document()->container();
        auto font      = css().get_font();
        auto color     = css().get_color();
        auto line_h    = text_line_height(*this);
        auto text      = m_type == input_type::password ? control_text::masked_text(data.text) : data.text;
        auto caret     = data.caret;
        if(m_type == input_type::password)
        {
            // Each masked codepoint takes three bytes.
            size_t codepoints = 0;
            for(size_t p = 0; p < std::min(caret, data.text.size()); p = control_text::next_offset(data.text, p))
            {
                codepoints++;
            }
            caret = codepoints * 3;
        }

        const auto caret_x  = control_text::measure(*get_document(), font, text, 0, caret);
        const auto caret_px = std::max(0_px, pos.width - 1_px);
        if(caret_x < m_text_scroll)
        {
            m_text_scroll = caret_x;
        } else if(caret_x > m_text_scroll + caret_px)
        {
            m_text_scroll = caret_x - caret_px;
        }
        m_text_scroll = std::max(0_px, m_text_scroll);

        container->set_clip(pos, border_radiuses{});
        const pixel_t text_y = pos.y + std::max(0_px, (pos.height - line_h) / 2_px);

        size_t selection_start = std::min(data.selection_start, data.selection_end);
        size_t selection_end   = std::max(data.selection_start, data.selection_end);
        if(m_type == input_type::password)
        {
            auto mask_offset = [&](size_t offset) {
                size_t codepoints = 0;
                for(size_t p = 0; p < std::min(offset, data.text.size()); p = control_text::next_offset(data.text, p))
                {
                    codepoints++;
                }
                return codepoints * 3;
            };
            selection_start = mask_offset(selection_start);
            selection_end   = mask_offset(selection_end);
        }

        if(selection_start != selection_end)
        {
            auto x1 = control_text::measure(*get_document(), font, text, 0, selection_start);
            auto x2 = control_text::measure(*get_document(), font, text, 0, selection_end);
            if(x2 > x1)
            {
                position sel(pos.x + x1 - m_text_scroll, text_y, x2 - x1, line_h);
                fill_rect(hdc, sel, web_color(0, 120, 215, 96));
            }
        }

        if(!text.empty())
        {
            const auto tw = control_text::measure(*get_document(), font, text, 0, text.size());
            position   text_pos(pos.x - m_text_scroll, text_y, tw, line_h);
            container->draw_text(hdc, text.c_str(), font, color, text_pos);
        }

        const bool focused = get_document()->get_focused_element().get() == this;
        if(focused && data.caret_visible)
        {
            position caret(pos.x + caret_x - m_text_scroll, text_y, 1_px, line_h);
            fill_rect(hdc, caret, color);
        }

        container->del_clip();
    }

    void el_input::draw_button_label(uint_ptr hdc, const position& pos)
    {
        const auto text = label();
        if(text.empty())
        {
            return;
        }
        auto     container = get_document()->container();
        auto     font      = css().get_font();
        auto     fm        = css().get_font_metrics();
        auto     tw        = container->text_width(text.c_str(), font);
        auto     th        = fm.height > 0_px ? fm.height : pos.height;
        position text_pos(pos.x + std::max(0_px, (pos.width - tw) / 2_px),
                          pos.y + std::max(0_px, (pos.height - th) / 2_px), tw, th);
        container->set_clip(pos, border_radiuses{});
        container->draw_text(hdc, text.c_str(), font, css().get_color(), text_pos);
        container->del_clip();
    }

    void el_input::draw_checkbox(uint_ptr hdc, const position& pos)
    {
        if(!is_checked())
        {
            return;
        }
        auto     container = get_document()->container();
        auto     font      = css().get_font();
        auto     fm        = css().get_font_metrics();
        auto     text      = "\xE2\x9C\x93"; // U+2713 CHECK MARK
        auto     tw        = container->text_width(text, font);
        auto     th        = fm.height > 0_px ? fm.height : pos.height;
        position text_pos(pos.x + std::max(0_px, (pos.width - tw) / 2_px),
                          pos.y + std::max(0_px, (pos.height - th) / 2_px), tw, th);
        container->set_clip(pos, border_radiuses{});
        container->draw_text(hdc, text, font, web_color(255, 255, 255), text_pos);
        container->del_clip();
    }

    void el_input::draw_radio(uint_ptr hdc, const position& pos)
    {
        if(!is_checked() || pos.width <= 0_px || pos.height <= 0_px)
        {
            return;
        }
        const auto diameter = std::min(pos.width, pos.height) / 2_px;
        const auto radius   = diameter / 2_px;
        position dot(pos.x + (pos.width - diameter) / 2_px, pos.y + (pos.height - diameter) / 2_px, diameter, diameter);
        background_layer layer;
        layer.border_box               = dot;
        layer.clip_box                 = dot;
        layer.origin_box               = dot;
        layer.repeat                   = background_repeat_no_repeat;
        layer.border_radius.top_left_x = layer.border_radius.top_left_y = radius;
        layer.border_radius.top_right_x = layer.border_radius.top_right_y = radius;
        layer.border_radius.bottom_right_x = layer.border_radius.bottom_right_y = radius;
        layer.border_radius.bottom_left_x = layer.border_radius.bottom_left_y = radius;
        get_document()->container()->draw_solid_fill(hdc, layer, web_color(11, 94, 215));
    }

    void el_input::draw_image(uint_ptr hdc, const position& pos, const std::shared_ptr<render_item>& ri)
    {
        if(pos.width <= 0_px || pos.height <= 0_px)
        {
            return;
        }
        background_layer layer;
        layer.clip_box    = pos;
        layer.origin_box  = pos;
        layer.border_box  = pos;
        layer.border_box += ri->get_paddings();
        layer.border_box += ri->get_borders();
        layer.repeat      = background_repeat_no_repeat;
        layer.border_radius = css().get_borders().radius.calc_percents(layer.border_box.width, layer.border_box.height);
        get_document()->container()->draw_image(hdc, layer, m_src, {});
    }

    void el_input::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip,
                        const std::shared_ptr<render_item>& ri)
    {
        html_tag::draw(hdc, x, y, clip, ri);

        position pos  = ri->pos();
        pos.x        += x;
        pos.y        += y;
        pos.round();

        if(!pos.does_intersect(clip))
        {
            return;
        }

        switch(m_type)
        {
        case input_type::hidden:
            break;
        case input_type::checkbox:
            draw_checkbox(hdc, pos);
            break;
        case input_type::radio:
            draw_radio(hdc, pos);
            break;
        case input_type::image:
            draw_image(hdc, pos, ri);
            break;
        case input_type::button:
        case input_type::submit:
        case input_type::reset:
            draw_button_label(hdc, pos);
            break;
        default:
            draw_text_control(hdc, pos);
            break;
        }
    }

    std::shared_ptr<render_item> el_input::create_render_item(const std::shared_ptr<render_item>& parent_ri)
    {
        if(css().get_display() == display_none)
        {
            return nullptr;
        }
        auto ret = std::make_shared<render_item_control>(shared_from_this());
        ret->parent(parent_ri);
        return ret;
    }

    std::string el_input::dump_get_name()
    {
        return "input";
    }
} // namespace litehtml
