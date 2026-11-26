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

        void    parse_attributes() override;
        void    draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip, const clipping* clip_chain,
                     const std::shared_ptr<render_item>& ri) override;
        void    get_content_size(size& sz, pixel_t max_width) override;
        void    on_click() override;
        bool    on_lbutton_down_at(pixel_t x, pixel_t y) override;
        bool    can_scroll_content(pixel_t dy) const override;
        pixel_t scroll_content(pixel_t dy) override;

        void                    reset();
        std::vector<form_field> form_fields();
        select_control_data     select_data() const;

        std::shared_ptr<render_item> create_render_item(const std::shared_ptr<render_item>& parent_ri) override;

        // Keyboard cursor of a listbox: the option index (document order) that receives navigation and Space
        // toggles. -1 when no option is active. The active option is drawn with an outline when it is not selected.
        void set_active_option(int option_index);
        int  active_option() const;
        // Scrolls the listbox so the row of the given option is fully visible.
        void scroll_option_into_view(int option_index);

        std::string dump_get_name() override;

      private:
        struct listbox_row
        {
            std::string label;
            int         option_index = -1;
            bool        group_label  = false;
            bool        grouped      = false;
            bool        disabled     = false;
            bool        selected     = false;
        };

        bool m_multiple = false;
        int  m_size     = 1;
        // Vertical scroll offset of a listbox, in pixels.
        pixel_t m_scroll_pos    = 0_px;
        int     m_active_option = -1;

        bool                     is_listbox() const;
        pixel_t                  max_scroll() const;
        std::vector<listbox_row> listbox_rows() const;
        void                     draw_listbox(uint_ptr hdc, const position& pos, const clipping* clip_chain);
        void                     draw_active_outline(uint_ptr hdc, const position& row_pos, const clipping* clip_chain);
        void fill_rect(uint_ptr hdc, const position& pos, const web_color& color, const clipping* clip_chain);
        bool apply_listbox_selection(int option_index);

        void        normalize_selection();
        std::string selected_text() const;
        void        notify_change();
    };
} // namespace litehtml

#endif // LITEHTML_EL_SELECT_H
