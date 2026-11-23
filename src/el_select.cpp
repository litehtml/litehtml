#include "el_select.h"
#include "document.h"
#include "document_container.h"
#include "el_optgroup.h"
#include "el_option.h"
#include "html_microsyntaxes.h"
#include "render_control.h"
#include <algorithm>

namespace litehtml
{
    namespace
    {
        pixel_t select_line_height(const html_tag& el)
        {
            const auto line_height = el.css().line_height().computed_value;
            if(line_height > 0_px)
            {
                return line_height;
            }
            const auto height = el.css().get_font_metrics().height;
            return height > 0_px ? height : pixel_t(16);
        }

        void collect_options(const element::ptr& parent, std::vector<el_option*>& options)
        {
            for(const auto& child : parent->children())
            {
                if(auto* option = dynamic_cast<el_option*>(child.get()))
                {
                    options.push_back(option);
                }
                collect_options(child, options);
            }
        }

    } // namespace

    el_select::el_select(const document::ptr& doc) :
        html_tag(doc)
    {
        m_css->set_display(display_inline_block);
    }

    void el_select::parse_attributes()
    {
        html_tag::parse_attributes();

        m_multiple = get_attr("multiple") != nullptr;
        m_size     = 1;
        if(const char* attr = get_attr("size"))
        {
            html_parse_non_negative_integer(attr, m_size);
        }
        if(m_size > 1)
        {
            css_token tok(DIMENSION, static_cast<float>(m_size) * 1.2f, css_number_number, "em");
            add_attr_property(_height_, {tok});
        }

        normalize_selection();
    }

    bool el_select::is_replaced() const
    {
        return true;
    }

    bool el_select::can_be_focused() const
    {
        return !is_inert() && !is_disabled();
    }

    control_type el_select::get_control_type() const
    {
        return control_type::select;
    }

    select_control_data el_select::select_data() const
    {
        select_control_data data;
        data.multiple = m_multiple;
        data.size     = m_size;

        int                index       = 0;
        bool               in_optgroup = false;
        select_group_data* group       = nullptr;

        const auto add_option = [&](const element::ptr& child) {
            auto* option = dynamic_cast<el_option*>(child.get());
            if(!option)
            {
                return;
            }
            select_option_data option_data;
            option_data.value    = option->value();
            option_data.label    = option->label_text();
            option_data.index    = index++;
            option_data.disabled = option->is_disabled();
            option_data.selected = option->selected();
            group->options.push_back(option_data);
        };

        for(const auto& child : m_children)
        {
            if(child->tag() == _optgroup_)
            {
                select_group_data optgroup;
                if(auto* optgroup_el = dynamic_cast<el_optgroup*>(child.get()))
                {
                    optgroup.label = optgroup_el->label_text();
                } else
                {
                    optgroup.label = child->get_attr("label", "");
                }
                optgroup.disabled = child->get_attr("disabled") != nullptr;
                data.groups.push_back(optgroup);
                group       = &data.groups.back();
                in_optgroup = true;
                for(const auto& option : child->children())
                {
                    if(option->tag() == _option_)
                    {
                        add_option(option);
                    }
                }
            } else if(child->tag() == _option_)
            {
                if(!group || in_optgroup)
                {
                    data.groups.emplace_back();
                    group       = &data.groups.back();
                    in_optgroup = false;
                }
                add_option(child);
            }
        }

        return data;
    }

    control_data el_select::get_control_data() const
    {
        return select_data();
    }

    void el_select::normalize_selection()
    {
        std::vector<el_option*> options;
        collect_options(shared_from_this(), options);

        const bool has_selected =
            std::any_of(options.begin(), options.end(), [](const el_option* option) { return option->selected(); });
        if(has_selected || m_multiple)
        {
            return;
        }

        for(auto* option : options)
        {
            if(!option->is_disabled())
            {
                option->set_selected(true);
                return;
            }
        }
    }

    bool el_select::update_focused(const control_data& data)
    {
        const auto* select_data = std::get_if<select_control_data>(&data);
        if(!select_data)
        {
            return false;
        }

        std::vector<el_option*> options;
        collect_options(shared_from_this(), options);

        bool changed = false;
        for(const auto& group : select_data->groups)
        {
            for(const auto& option_data : group.options)
            {
                if(option_data.index < 0 || static_cast<size_t>(option_data.index) >= options.size())
                {
                    continue;
                }
                auto* option = options[static_cast<size_t>(option_data.index)];
                if(option_data.selected && option->is_disabled())
                {
                    continue;
                }
                if(option->selected() != option_data.selected)
                {
                    option->set_selected(option_data.selected);
                    changed = true;
                }
            }
        }

        if(changed)
        {
            notify_change();
        }
        return changed;
    }

    void el_select::notify_change()
    {
        if(auto* container = get_document()->container())
        {
            container->on_change(shared_from_this(), get_attr("id"), select_data());
        }
    }

    void el_select::on_click()
    {
        if(is_inert() || is_disabled())
        {
            return;
        }
        if(auto* container = get_document()->container())
        {
            container->show_select_popup(shared_from_this(), get_attr("id"), select_data());
        }
        html_tag::on_click();
    }

    std::string el_select::selected_text() const
    {
        std::string text;
        for(const auto& group : select_data().groups)
        {
            for(const auto& option : group.options)
            {
                if(!option.selected)
                {
                    continue;
                }
                if(!text.empty())
                {
                    text += ", ";
                }
                text += option.label;
            }
        }
        return text;
    }

    std::vector<form_field> el_select::form_fields()
    {
        std::vector<form_field> fields;
        const char*             name = get_attr("name");
        if(!name || !*name)
        {
            return fields;
        }
        for(const auto& group : select_data().groups)
        {
            for(const auto& option : group.options)
            {
                if(option.selected && !option.disabled)
                {
                    fields.push_back({name, option.value});
                }
            }
        }
        return fields;
    }

    void el_select::reset()
    {
        std::vector<el_option*> options;
        collect_options(shared_from_this(), options);
        for(auto* option : options)
        {
            option->set_selected(option->get_attr("selected") != nullptr);
        }
        normalize_selection();
    }

    void el_select::get_content_size(size& sz, pixel_t /*max_width*/)
    {
        auto       font      = css().get_font();
        const auto fm        = css().get_font_metrics();
        const auto line_h    = select_line_height(*this);
        pixel_t    max_width = 0_px;

        if(font)
        {
            for(const auto& group : select_data().groups)
            {
                for(const auto& option : group.options)
                {
                    max_width =
                        std::max(max_width, get_document()->container()->text_width(option.label.c_str(), font));
                }
            }
        }
        if(max_width <= 0_px)
        {
            max_width = (fm.ch_width > 0_px ? fm.ch_width : pixel_t(8)) * 10_px;
        }
        sz.width  = max_width + line_h + 8_px;
        sz.height = line_h;
    }

    void el_select::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip,
                         const std::shared_ptr<render_item>& ri)
    {
        html_tag::draw(hdc, x, y, clip, ri);

        position pos  = ri->pos();
        pos.x        += x;
        pos.y        += y;
        pos.round();

        if(!pos.does_intersect(clip) || pos.width <= 0_px || pos.height <= 0_px)
        {
            return;
        }

        auto       container = get_document()->container();
        auto       font      = css().get_font();
        auto       fm        = css().get_font_metrics();
        const auto line_h    = select_line_height(*this);
        const auto arrow_w   = line_h > 0_px ? line_h : pixel_t(12);
        const auto text      = selected_text();
        position   text_pos(pos.x, pos.y + std::max(0_px, (pos.height - line_h) / 2_px),
                            std::max(0_px, pos.width - arrow_w - 2_px), line_h);

        container->set_clip(pos, border_radiuses{});
        if(!text.empty())
        {
            container->draw_text(hdc, text.c_str(), font, css().get_color(), text_pos);
        }

        const auto arrow_text = "\xE2\x96\xBE"; // U+25BE BLACK DOWN-POINTING SMALL TRIANGLE
        const auto arrow_px   = container->text_width(arrow_text, font);
        position   arrow_pos(pos.right() - arrow_w - 2_px + std::max(0_px, (arrow_w - arrow_px) / 2_px),
                             pos.y + std::max(0_px, (pos.height - fm.height) / 2_px), arrow_px, fm.height);
        container->draw_text(hdc, arrow_text, font, css().get_color(), arrow_pos);
        container->del_clip();
    }

    std::shared_ptr<render_item> el_select::create_render_item(const std::shared_ptr<render_item>& parent_ri)
    {
        if(css().get_display() == display_none)
        {
            return nullptr;
        }
        auto ret = std::make_shared<render_item_control>(shared_from_this());
        ret->parent(parent_ri);
        return ret;
    }

    std::string el_select::dump_get_name()
    {
        return "select";
    }
} // namespace litehtml
