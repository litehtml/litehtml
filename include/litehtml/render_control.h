#ifndef LITEHTML_RENDER_CONTROL_H
#define LITEHTML_RENDER_CONTROL_H

#include "render_image.h"
#include <algorithm>

namespace litehtml
{
    // Replaced render item used by form controls. It shares the intrinsic-size and CSS box
    // handling of render_item_image, but has no image-specific drawing of its own.
    class render_item_control : public render_item_image
    {
      public:
        explicit render_item_control(std::shared_ptr<element> src_el) :
            render_item_image(std::move(src_el))
        {
        }

        std::shared_ptr<render_item> clone() override
        {
            return std::make_shared<render_item_control>(src_el());
        }

        pixel_t v_scroll(pixel_t dy) override
        {
            return src_el()->scroll_content(dy);
        }

        bool is_v_scrollable(const pixel_t dy) const override
        {
            return src_el()->can_scroll_content(dy);
        }

        // The label of a button without child boxes is painted vertically centred in the button's
        // box (see el_button::draw), so the baseline of such a button is the baseline of its label
        // and not the bottom of its box. The markers and values of other controls are not text
        // centred that way and keep the replaced-element baseline.
        pixel_t get_first_baseline() override
        {
            const auto fm = css().get_font_metrics();
            if(src_el()->tag() != _button_ || fm.height <= 0_px)
            {
                return height() - margin_bottom();
            }
            return margin_top() + std::max(0_px, (m_pos.height - fm.height) / 2_px) + fm.ascent;
        }

        pixel_t get_last_baseline() override
        {
            return get_first_baseline();
        }

      protected:
        pixel_t _render(pixel_t x, pixel_t y, const containing_block_context& containing_block_size,
                        formatting_context* fmt_ctx, bool second_pass) override
        {
            // render_item_image uses the element height as the line height, which is right for
            // images but would collapse multi-line controls into one line.
            auto&         line_height = src_el()->css().line_height().computed_value;
            const pixel_t saved       = line_height;
            const pixel_t result      = render_item_image::_render(x, y, containing_block_size, fmt_ctx, second_pass);
            line_height               = saved;
            return result;
        }
    };
} // namespace litehtml

#endif // LITEHTML_RENDER_CONTROL_H
