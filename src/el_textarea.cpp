#include "el_textarea.h"
#include "control_text_layout.h"
#include "document.h"
#include "document_container.h"
#include "html_microsyntaxes.h"
#include "render_control.h"
#include <algorithm>

namespace litehtml
{
    namespace
    {
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

    el_textarea::el_textarea(const document::ptr& doc) :
        html_tag(doc)
    {
        m_css->set_display(display_inline_block);
    }

    void el_textarea::parse_attributes()
    {
        html_tag::parse_attributes();

        m_name        = get_attr("name", "");
        m_placeholder = get_attr("placeholder", "");

        std::string text;
        get_text(text);
        if(!text.empty() && text[0] == '\n')
        {
            text.erase(0, 1);
        }
        m_initial_text = text;
        m_text.text    = text;
        m_text.caret   = text.size();

        int cols = 20;
        if(const char* attr = get_attr("cols"))
        {
            html_parse_non_negative_integer(attr, cols);
        }
        if(cols > 0)
        {
            css_token tok(DIMENSION, static_cast<float>(cols), css_number_integer, "ch");
            add_attr_property(_width_, {tok});
        }

        int rows = 2;
        if(const char* attr = get_attr("rows"))
        {
            html_parse_non_negative_integer(attr, rows);
        }
        if(rows > 0)
        {
            css_token tok(DIMENSION, static_cast<float>(rows) * 1.6f + 0.5f, css_number_number, "em");
            add_attr_property(_height_, {tok});
        }
    }

    bool el_textarea::is_replaced() const
    {
        return true;
    }

    bool el_textarea::can_be_focused() const
    {
        return !is_inert() && !is_disabled();
    }

    control_type el_textarea::get_control_type() const
    {
        return control_type::textarea;
    }

    bool el_textarea::is_placeholder_shown() const
    {
        return !m_placeholder.empty() && m_text.text.empty();
    }

    control_data el_textarea::get_control_data() const
    {
        return m_text;
    }

    bool el_textarea::update_focused(const control_data& data)
    {
        if(const auto text = std::get_if<text_control_data>(&data))
        {
            const bool placeholder_shown = is_placeholder_shown();
            const bool value_changed     = text->text != m_text.text;
            m_text                       = *text;
            if(value_changed)
            {
                notify_change();
            }
            return placeholder_shown != is_placeholder_shown();
        }
        return false;
    }

    void el_textarea::compute_styles(bool recursive)
    {
        html_tag::compute_styles(recursive);
        compute_placeholder_style();
    }

    void el_textarea::refresh_styles(bool recursive)
    {
        m_placeholder_style.clear();
        html_tag::refresh_styles(recursive);
    }

    void el_textarea::apply_placeholder_style(const style& style)
    {
        m_placeholder_style.combine(style);
    }

    void el_textarea::compute_placeholder_style()
    {
        if(m_placeholder.empty())
        {
            m_placeholder_el.reset();
            return;
        }

        auto placeholder = std::make_unique<html_tag>(get_document());
        placeholder->parent(shared_from_this());
        if(!m_placeholder_style.empty())
        {
            placeholder->add_style(m_placeholder_style);
        }
        placeholder->compute_styles(false);
        m_placeholder_el = std::move(placeholder);
    }

    void el_textarea::notify_change()
    {
        if(auto* container = get_document()->container())
        {
            container->on_change(shared_from_this(), get_attr("id"), get_control_data());
        }
    }

    void el_textarea::reset()
    {
        m_text.text            = m_initial_text;
        m_text.caret           = m_initial_text.size();
        m_text.selection_start = m_text.selection_end = m_text.caret;
        m_first_line                                  = 0;
    }

    bool el_textarea::has_name() const
    {
        return !m_name.empty();
    }

    std::string el_textarea::current_value()
    {
        text_control_data data = m_text;
        if(auto* container = get_document()->container())
        {
            container->get_text_data(shared_from_this(), data);
        }
        m_text = data;
        return m_text.text;
    }

    std::optional<form_field> el_textarea::create_form_field()
    {
        if(!has_name())
        {
            return std::nullopt;
        }
        return litehtml::form_field{m_name, current_value()};
    }

    void el_textarea::get_content_size(size& sz, pixel_t /*max_width*/)
    {
        const auto fm       = css().get_font_metrics();
        const auto ch_width = fm.ch_width > 0_px ? fm.ch_width : pixel_t(8);
        const auto line_h   = text_line_height(*this);
        int        cols     = 20;
        int        rows     = 2;
        if(const char* attr = get_attr("cols"))
        {
            html_parse_non_negative_integer(attr, cols);
        }
        if(const char* attr = get_attr("rows"))
        {
            html_parse_non_negative_integer(attr, rows);
        }
        sz.width  = pixel_t(cols) * ch_width;
        sz.height = pixel_t(rows) * line_h;
    }

    void el_textarea::fill_rect(uint_ptr hdc, const position& pos, const web_color& color)
    {
        background_layer layer;
        layer.border_box = pos;
        layer.clip_box   = pos;
        layer.origin_box = pos;
        layer.repeat     = background_repeat_no_repeat;
        get_document()->container()->draw_solid_fill(hdc, layer, color);
    }

    void el_textarea::draw_text_control(uint_ptr hdc, const position& pos)
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
        m_text = data;

        auto container = get_document()->container();
        auto font      = css().get_font();
        auto color     = css().get_color();
        auto line_h    = text_line_height(*this);

        const bool show_placeholder = data.text.empty() && !m_placeholder.empty();
        if(show_placeholder && m_placeholder_el)
        {
            if(const auto placeholder_font = m_placeholder_el->css().get_font())
            {
                font = placeholder_font;
            }
            color = m_placeholder_el->css().get_color();
        }

        auto lines = control_text::layout_lines(*get_document(), font, data.text, pos.width);
        auto caret = control_text::caret_position(*get_document(), font, lines, data.text, data.caret);

        const auto visible_lines =
            std::max(static_cast<size_t>(1), static_cast<size_t>(static_cast<int>(pos.height / line_h)));
        if(caret.line < m_first_line)
        {
            m_first_line = caret.line;
        } else if(caret.line >= m_first_line + visible_lines)
        {
            m_first_line = caret.line - visible_lines + 1;
        }
        if(m_first_line >= lines.size())
        {
            m_first_line = lines.empty() ? 0 : lines.size() - 1;
        }

        container->set_clip(pos, border_radiuses{});

        if(show_placeholder)
        {
            const auto placeholder_lines = control_text::layout_lines(*get_document(), font, m_placeholder, pos.width);
            for(size_t i = 0; i < placeholder_lines.size(); i++)
            {
                const auto y = pos.y + pixel_t(static_cast<int>(i)) * line_h;
                if(y >= pos.bottom())
                {
                    break;
                }
                const auto& line = placeholder_lines[i];
                if(line.end > line.start)
                {
                    const auto text = m_placeholder.substr(line.start, line.end - line.start);
                    position   text_pos(pos.x, y, line.width, line_h);
                    container->draw_text(hdc, text.c_str(), font, color, text_pos);
                }
            }
        }

        size_t selection_start = std::min(data.selection_start, data.selection_end);
        size_t selection_end   = std::max(data.selection_start, data.selection_end);

        for(size_t i = m_first_line; i < lines.size(); i++)
        {
            const auto y = pos.y + pixel_t(static_cast<int>(i - m_first_line)) * line_h;
            if(y >= pos.bottom())
            {
                break;
            }
            const auto& line = lines[i];

            if(selection_start != selection_end && selection_end > line.start && selection_start < line.end)
            {
                const auto sel_start = std::max(selection_start, line.start);
                const auto sel_end   = std::min(selection_end, line.end);
                const auto x1        = control_text::measure(*get_document(), font, data.text, line.start, sel_start);
                const auto x2        = control_text::measure(*get_document(), font, data.text, line.start, sel_end);
                if(x2 > x1)
                {
                    position sel(pos.x + x1, y, x2 - x1, line_h);
                    fill_rect(hdc, sel, web_color(0, 120, 215, 96));
                }
                if(sel_end == line.end && selection_end > line.end)
                {
                    position sel(pos.x + x2, y, 4_px, line_h);
                    fill_rect(hdc, sel, web_color(0, 120, 215, 96));
                }
            }

            if(line.end > line.start)
            {
                const auto text = data.text.substr(line.start, line.end - line.start);
                position   text_pos(pos.x, y, line.width, line_h);
                container->draw_text(hdc, text.c_str(), font, color, text_pos);
            }
        }

        const bool focused = get_document()->get_focused_element().get() == this;
        if(focused && data.caret_visible && caret.line >= m_first_line)
        {
            const auto y = pos.y + pixel_t(static_cast<int>(caret.line - m_first_line)) * line_h;
            if(y < pos.bottom())
            {
                position caret_pos(pos.x + caret.x, y, 1_px, line_h);
                fill_rect(hdc, caret_pos, color);
            }
        }

        container->del_clip();
    }

    void el_textarea::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip,
                           const std::shared_ptr<render_item>& ri)
    {
        html_tag::draw(hdc, x, y, clip, ri);

        position pos  = ri->pos();
        pos.x        += x;
        pos.y        += y;
        pos.round();

        if(pos.does_intersect(clip))
        {
            draw_text_control(hdc, pos);
        }
    }

    std::shared_ptr<render_item> el_textarea::create_render_item(const std::shared_ptr<render_item>& parent_ri)
    {
        if(css().get_display() == display_none)
        {
            return nullptr;
        }
        auto ret = std::make_shared<render_item_control>(shared_from_this());
        ret->parent(parent_ri);
        return ret;
    }

    std::string el_textarea::dump_get_name()
    {
        return "textarea";
    }
} // namespace litehtml
