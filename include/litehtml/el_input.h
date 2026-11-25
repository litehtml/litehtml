#ifndef LITEHTML_EL_INPUT_H
#define LITEHTML_EL_INPUT_H

#include "controls.h"
#include "document.h"
#include "html_tag.h"
#include <memory>
#include <optional>

namespace litehtml
{
    enum class input_type
    {
        button,
        checkbox,
        hidden,
        image,
        number,
        password,
        radio,
        reset,
        search,
        submit,
        text
    };

    class el_input : public html_tag
    {
      public:
        explicit el_input(const document::ptr& doc);

        bool         is_replaced() const override;
        bool         can_be_focused() const override;
        bool         is_checked() const override;
        bool         is_placeholder_shown() const override;
        control_type get_control_type() const override;
        control_data get_control_data() const override;
        bool         update_focused(const control_data& data) override;

        void parse_attributes() override;
        void compute_styles(bool recursive = true) override;
        void refresh_styles(bool recursive = true) override;
        void draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip,
                  const std::shared_ptr<render_item>& ri) override;
        void get_content_size(size& sz, pixel_t max_width) override;
        void on_click() override;
        void reset();

        std::shared_ptr<render_item> create_render_item(const std::shared_ptr<render_item>& parent_ri) override;

        input_type                type() const;
        bool                      is_submit_control() const;
        bool                      is_text_control() const;
        bool                      has_name() const;
        const std::string&        name() const;
        const std::string&        src() const;
        std::string               current_value();
        std::optional<form_field> create_form_field(bool submitted);

        std::string dump_get_name() override;

      private:
        input_type          m_type = input_type::text;
        std::optional<bool> m_checked;
        std::string         m_name;
        std::string         m_value;
        std::string         m_initial_value;
        std::string         m_src;
        std::string         m_alt;
        std::string         m_placeholder;
        text_control_data   m_text;
        pixel_t             m_text_scroll = 0_px;

        style                     m_placeholder_style;
        std::unique_ptr<html_tag> m_placeholder_el;

        void        parse_type(const std::string& type);
        void        set_checked(bool checked);
        bool        select_radio();
        std::string label() const;
        void        notify_change();
        void        apply_placeholder_style(const style& style) override;
        void        compute_placeholder_style();
        void        draw_text_control(uint_ptr hdc, const position& pos);
        void        draw_button_label(uint_ptr hdc, const position& pos);
        void        draw_checkbox(uint_ptr hdc, const position& pos);
        void        draw_radio(uint_ptr hdc, const position& pos);
        void        draw_image(uint_ptr hdc, const position& pos, const std::shared_ptr<render_item>& ri);
        void        fill_rect(uint_ptr hdc, const position& pos, const web_color& color);
    };
} // namespace litehtml

#endif // LITEHTML_EL_INPUT_H
