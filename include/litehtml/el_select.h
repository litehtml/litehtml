#ifndef LITEHTML_EL_SELECT_H
#define LITEHTML_EL_SELECT_H

#include "controls.h"
#include "document.h"
#include "html_tag.h"
#include <vector>

namespace litehtml
{
    class el_select : public html_tag
    {
      public:
        explicit el_select(const document::ptr& doc);

        bool         is_replaced() const override;
        bool         can_be_focused() const override;
        control_type get_control_type() const override;
        control_data get_control_data() const override;
        bool         update_focused(const control_data& data) override;

        void parse_attributes() override;
        void draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip,
                  const std::shared_ptr<render_item>& ri) override;
        void get_content_size(size& sz, pixel_t max_width) override;
        void on_click() override;

        void                    reset();
        std::vector<form_field> form_fields();
        select_control_data     select_data() const;

        std::shared_ptr<render_item> create_render_item(const std::shared_ptr<render_item>& parent_ri) override;

        std::string dump_get_name() override;

      private:
        bool m_multiple = false;
        int  m_size     = 1;

        void        normalize_selection();
        std::string selected_text() const;
        void        notify_change();
    };
} // namespace litehtml

#endif // LITEHTML_EL_SELECT_H
