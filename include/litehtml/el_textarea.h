#ifndef LITEHTML_EL_TEXTAREA_H
#define LITEHTML_EL_TEXTAREA_H

#include "controls.h"
#include "document.h"
#include "html_tag.h"
#include <optional>

namespace litehtml
{
    class el_textarea : public html_tag
    {
      public:
        explicit el_textarea(const document::ptr& doc);

        bool         is_replaced() const override;
        bool         can_be_focused() const override;
        control_type get_control_type() const override;
        control_data get_control_data() const override;
        bool         update_focused(const control_data& data) override;

        void parse_attributes() override;
        void draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip,
                  const std::shared_ptr<render_item>& ri) override;
        void get_content_size(size& sz, pixel_t max_width) override;

        void                      reset();
        bool                      has_name() const;
        std::string               current_value();
        std::optional<form_field> create_form_field();

        std::shared_ptr<render_item> create_render_item(const std::shared_ptr<render_item>& parent_ri) override;

        std::string dump_get_name() override;

      private:
        text_control_data m_text;
        std::string       m_initial_text;
        std::string       m_name;
        size_t            m_first_line = 0;

        void notify_change();
        void draw_text_control(uint_ptr hdc, const position& pos);
        void fill_rect(uint_ptr hdc, const position& pos, const web_color& color);
    };
} // namespace litehtml

#endif // LITEHTML_EL_TEXTAREA_H
