#ifndef LITEHTML_RENDER_CONTROL_H
#define LITEHTML_RENDER_CONTROL_H

#include "render_image.h"

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
