#include "el_select.h"
#include "document.h"
#include "document_container.h"
#include "el_optgroup.h"
#include "el_option.h"
#include "html_microsyntaxes.h"
#include "render_control.h"
#include <algorithm>
#include <cmath>

namespace litehtml
{
    namespace
    {
        const pixel_t   listbox_option_padding          = 2_px;
        const pixel_t   listbox_optgroup_padding        = 20_px;
        const pixel_t   listbox_row_extra_height        = 1_px;
        const web_color listbox_selection_color         = web_color(206, 206, 206);
        const web_color listbox_selection_focused_color = web_color(25, 103, 210);
        const web_color listbox_disabled_color          = web_color(128, 128, 128);

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

        // Listbox option rows are one pixel taller than the line box (option padding-bottom).
        pixel_t select_row_height(const html_tag& el)
        {
            return select_line_height(el) + listbox_row_extra_height;
        }

        uint_ptr select_bold_font(const html_tag& el)
        {
            auto descr   = el.css().get_font_description();
            descr.weight = 700;
            return el.get_document()->get_font(descr, nullptr);
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

        int size = 0;
        if(const char* attr = get_attr("size"))
        {
            html_parse_non_negative_integer(attr, size);
        }
        m_size = size > 0 ? size : (m_multiple ? 4 : 1);

        if(is_listbox())
        {
            // The listbox is sized by its option rows, not by the one-line dropdown default.
            add_attr_property(_height_, {css_token(IDENT, "auto")});
            // Option rows provide their own inset; the control itself has no padding.
            map_to_pixel_length_property(_padding_, "0");
        }

        normalize_selection();
    }

    bool el_select::is_listbox() const
    {
        return m_multiple || m_size > 1;
    }

    void el_select::set_active_option(int option_index)
    {
        m_active_option = option_index;
    }

    int el_select::active_option() const
    {
        return m_active_option;
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
        // Only a one-line dropdown selects its first option automatically. A listbox starts unselected.
        if(has_selected || is_listbox())
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
        // Listboxes select their rows on mouse down and never request a popup.
        if(!is_listbox())
        {
            if(auto* container = get_document()->container())
            {
                container->show_select_popup(shared_from_this(), get_attr("id"), select_data());
            }
        }
        html_tag::on_click();
    }

    bool el_select::on_lbutton_down_at(pixel_t /*x*/, pixel_t y)
    {
        const bool active = html_tag::on_lbutton_down();
        if(!is_listbox() || is_inert() || is_disabled())
        {
            return active;
        }

        const auto rows  = listbox_rows();
        const auto row_h = select_row_height(*this);
        if(rows.empty() || row_h <= 0_px)
        {
            return active;
        }

        const auto pos = get_placement();
        if(y < pos.y || y >= pos.bottom())
        {
            return active;
        }

        const auto index = static_cast<int>(std::floor((y - pos.y + m_scroll_pos).value() / row_h.value()));
        if(index < 0 || static_cast<size_t>(index) >= rows.size())
        {
            return active;
        }

        const auto& row = rows[static_cast<size_t>(index)];
        if(row.group_label || row.disabled || row.option_index < 0)
        {
            return active;
        }

        m_active_option = row.option_index;
        if(!apply_listbox_selection(row.option_index))
        {
            return active;
        }

        // Option selection is drawn by the control, not by a render item, so request a full redraw.
        get_document()->request_redraw();
        return true;
    }

    bool el_select::apply_listbox_selection(int option_index)
    {
        auto data  = select_data();
        bool found = false;
        for(auto& group : data.groups)
        {
            for(auto& option : group.options)
            {
                if(option.index == option_index)
                {
                    found           = true;
                    option.selected = m_multiple ? !option.selected : true;
                } else if(!m_multiple)
                {
                    option.selected = false;
                }
            }
        }
        return found && update_focused(data);
    }

    bool el_select::can_scroll_content(pixel_t dy) const
    {
        if(!is_listbox() || dy == 0_px)
        {
            return false;
        }
        return dy < 0_px ? m_scroll_pos > 0_px : m_scroll_pos < max_scroll();
    }

    pixel_t el_select::scroll_content(pixel_t dy)
    {
        if(!is_listbox())
        {
            return 0_px;
        }
        const pixel_t old = m_scroll_pos;
        m_scroll_pos      = std::clamp(m_scroll_pos + dy, 0_px, max_scroll());
        return m_scroll_pos - old;
    }

    pixel_t el_select::max_scroll() const
    {
        const auto row_h   = select_row_height(*this);
        const auto total   = pixel_t(static_cast<int>(listbox_rows().size())) * row_h;
        const auto visible = pixel_t(m_size) * row_h;
        return std::max(0_px, total - visible);
    }

    void el_select::scroll_option_into_view(int option_index)
    {
        if(!is_listbox() || option_index < 0)
        {
            return;
        }

        const auto rows    = listbox_rows();
        size_t     row_pos = rows.size();
        for(size_t i = 0; i < rows.size(); ++i)
        {
            if(rows[i].option_index == option_index)
            {
                row_pos = i;
                break;
            }
        }
        if(row_pos >= rows.size())
        {
            return;
        }

        const auto row_h   = select_row_height(*this);
        const auto top     = pixel_t(static_cast<int>(row_pos)) * row_h;
        const auto bottom  = top + row_h;
        const auto visible = pixel_t(m_size) * row_h;

        if(top < m_scroll_pos)
        {
            m_scroll_pos = top;
        } else if(bottom > m_scroll_pos + visible)
        {
            m_scroll_pos = bottom - visible;
        }
        m_scroll_pos = std::clamp(m_scroll_pos, 0_px, max_scroll());
    }

    std::vector<el_select::listbox_row> el_select::listbox_rows() const
    {
        std::vector<listbox_row> rows;
        for(const auto& group : select_data().groups)
        {
            if(!group.label.empty())
            {
                listbox_row row;
                row.label       = group.label;
                row.group_label = true;
                row.disabled    = group.disabled;
                rows.push_back(std::move(row));
            }
            for(const auto& option : group.options)
            {
                listbox_row row;
                row.label        = option.label;
                row.option_index = option.index;
                row.grouped      = !group.label.empty();
                row.disabled     = option.disabled || group.disabled;
                row.selected     = option.selected;
                rows.push_back(std::move(row));
            }
        }
        return rows;
    }

    void el_select::fill_rect(uint_ptr hdc, const position& pos, const web_color& color, const clipping* clip_chain)
    {
        background_layer layer;
        layer.border_box = pos;
        layer.clip_box   = pos;
        layer.origin_box = pos;
        layer.repeat     = background_repeat_no_repeat;
        get_document()->container()->draw_solid_fill(hdc, layer, color, clip_chain);
    }

    void el_select::draw_active_outline(uint_ptr hdc, const position& row_pos, const clipping* clip_chain)
    {
        borders bdr;
        border  side;
        side.width = 2_px;
        side.style = border_style_solid;
        side.color = web_color(16, 16, 16);

        bdr.left              = side;
        bdr.top               = side;
        bdr.right             = side;
        bdr.bottom            = side;
        bdr.radius.top_left_x = bdr.radius.top_left_y = 3_px;
        bdr.radius.top_right_x = bdr.radius.top_right_y = 3_px;
        bdr.radius.bottom_right_x = bdr.radius.bottom_right_y = 3_px;
        bdr.radius.bottom_left_x = bdr.radius.bottom_left_y = 3_px;

        position draw_pos(row_pos.x + 1_px, row_pos.y + 1_px, row_pos.width - 2_px, row_pos.height - 2_px);
        draw_pos.round();
        get_document()->container()->draw_borders(hdc, bdr, draw_pos, false, clip_chain);
    }

    void el_select::draw_listbox(uint_ptr hdc, const position& pos, const clipping* clip_chain)
    {
        auto       container = get_document()->container();
        auto       font      = css().get_font();
        const auto line_h    = select_line_height(*this);
        const auto row_h     = select_row_height(*this);
        const auto rows      = listbox_rows();
        const bool focused   = get_document()->get_focused_element().get() == this;
        const auto text      = css().get_color();

        const auto& selection_bg = focused ? listbox_selection_focused_color : listbox_selection_color;
        const auto& selection_fg = focused ? web_color::white : text;
        const auto  bold_font    = select_bold_font(*this);

        clipping listbox_clipping(pos, border_radiuses{}, clip_chain);

        pixel_t y = pos.y - m_scroll_pos;
        for(const auto& row : rows)
        {
            if(y + row_h <= pos.y)
            {
                y += row_h;
                continue;
            }
            if(y >= pos.bottom())
            {
                break;
            }

            if(row.group_label)
            {
                if(font && !row.label.empty())
                {
                    position text_pos(pos.x + listbox_option_padding, y,
                                      std::max(0_px, pos.width - listbox_option_padding), line_h);
                    container->draw_text(hdc, row.label.c_str(), bold_font ? bold_font : font,
                                         row.disabled ? listbox_disabled_color : text, text_pos, &listbox_clipping);
                }
            } else
            {
                if(row.selected)
                {
                    fill_rect(hdc, position(pos.x, y, pos.width, row_h), selection_bg, &listbox_clipping);
                }
                if(font && !row.label.empty())
                {
                    const auto indent = row.grouped ? listbox_optgroup_padding : listbox_option_padding;
                    position   text_pos(pos.x + indent, y, std::max(0_px, pos.width - indent - listbox_option_padding),
                                        line_h);
                    container->draw_text(hdc, row.label.c_str(), font,
                                         row.selected ? selection_fg : (row.disabled ? listbox_disabled_color : text),
                                         text_pos, &listbox_clipping);
                }
                if(focused && !row.selected && row.option_index == m_active_option)
                {
                    draw_active_outline(hdc, position(pos.x, y, pos.width, row_h), &listbox_clipping);
                }
            }
            y += row_h;
        }
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
        m_scroll_pos    = 0_px;
        m_active_option = -1;
        normalize_selection();
    }

    void el_select::get_content_size(size& sz, pixel_t /*max_width*/)
    {
        auto       font      = css().get_font();
        const auto fm        = css().get_font_metrics();
        const auto line_h    = select_line_height(*this);
        pixel_t    max_width = 0_px;

        if(is_listbox())
        {
            const auto rows = listbox_rows();
            if(font)
            {
                uint_ptr bold_font = 0;
                for(const auto& row : rows)
                {
                    pixel_t width = 0_px;
                    if(row.group_label)
                    {
                        bold_font = bold_font ? bold_font : select_bold_font(*this);
                        width =
                            get_document()->container()->text_width(row.label.c_str(), bold_font ? bold_font : font);
                    } else
                    {
                        width = get_document()->container()->text_width(row.label.c_str(), font) +
                                (row.grouped ? listbox_optgroup_padding : listbox_option_padding);
                    }
                    max_width = std::max(max_width, width);
                }
            }
            if(max_width <= 0_px)
            {
                max_width = (fm.ch_width > 0_px ? fm.ch_width : pixel_t(8)) * 4_px;
            }
            sz.width  = max_width + listbox_option_padding;
            sz.height = pixel_t(m_size) * select_row_height(*this);
            return;
        }

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

    void el_select::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip, const clipping* clip_chain,
                         const std::shared_ptr<render_item>& ri)
    {
        html_tag::draw(hdc, x, y, clip, clip_chain, ri);

        position pos  = ri->pos();
        pos.x        += x;
        pos.y        += y;
        pos.round();

        if(!pos.does_intersect(clip) || pos.width <= 0_px || pos.height <= 0_px)
        {
            return;
        }

        if(is_listbox())
        {
            draw_listbox(hdc, pos, clip_chain);
            return;
        }

        auto       container = get_document()->container();
        auto       font      = css().get_font();
        const auto line_h    = select_line_height(*this);
        const auto arrow_w   = line_h > 0_px ? line_h : pixel_t(12);
        const auto text      = selected_text();
        position   text_pos(pos.x, pos.y + std::max(0_px, (pos.height - line_h) / 2_px),
                            std::max(0_px, pos.width - arrow_w - 2_px), line_h);

        clipping select_clipping(pos, border_radiuses{}, clip_chain);
        if(!text.empty())
        {
            container->draw_text(hdc, text.c_str(), font, css().get_color(), text_pos, &select_clipping);
        }

        // The marker is drawn by the container, so it does not depend on the control font having
        // the arrow glyph.
        const auto marker_w = std::max(6_px, arrow_w / 2_px);
        const auto marker_h = std::max(4_px, arrow_w / 3_px);
        position   marker_pos(pos.right() - arrow_w - 2_px + std::max(0_px, (arrow_w - marker_w) / 2_px),
                              pos.y + std::max(0_px, (pos.height - marker_h) / 2_px), marker_w, marker_h);
        container->draw_combobox_marker(hdc, marker_pos, css().get_color(), &select_clipping);
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
