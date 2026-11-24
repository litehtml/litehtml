#ifndef LITEHTML_EL_BUTTON_H
#define LITEHTML_EL_BUTTON_H

#include "controls.h"
#include "document.h"
#include "html_tag.h"
#include <optional>

namespace litehtml
{
    enum class button_type
    {
        button,
        reset,
        submit
    };

    class el_button : public html_tag
    {
      public:
        explicit el_button(const document::ptr& doc);

        bool         is_replaced() const override;
        bool         can_be_focused() const override;
        control_type get_control_type() const override;

        void parse_attributes() override;
        void on_click() override;
        void draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip,
                  const std::shared_ptr<render_item>& ri) override;
        void get_content_size(size& sz, pixel_t max_width) override;

        bool                      is_submit_control() const;
        std::optional<form_field> create_form_field();

        std::shared_ptr<render_item> create_render_item(const std::shared_ptr<render_item>& parent_ri) override;

        std::string dump_get_name() override;

      private:
        // A button with child elements renders its content as a normal inline-block container;
        // a button with only text keeps the replaced control rendering of its centered label.
        bool has_content_children() const;

        button_type m_type = button_type::submit;
        std::string m_name;
        std::string m_value;
        std::string m_label;
    };
} // namespace litehtml

#endif // LITEHTML_EL_BUTTON_H
