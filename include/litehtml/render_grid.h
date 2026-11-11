#ifndef LITEHTML_RENDER_GRID_H
#define LITEHTML_RENDER_GRID_H

#include "render_flex.h"

namespace litehtml
{
    class render_item_grid : public render_item_flex
    {
        struct inherited_axis
        {
            std::vector<pixel_t> sizes;
            grid_track_list      names;
            pixel_t              gap = 0_px;
        };
        std::optional<inherited_axis> m_inherited_columns;
        std::optional<inherited_axis> m_inherited_rows;
        std::vector<grid_area>        m_placed_areas;

        void    calc_intrinsic_size() override;
        pixel_t _render_content(pixel_t x, pixel_t y, bool second_pass, const containing_block_context& self_size,
                                formatting_context* fmt_ctx) override;

      public:
        explicit render_item_grid(std::shared_ptr<element> src_el) :
            render_item_flex(std::move(src_el))
        {
        }
        std::shared_ptr<render_item> init() override
        {
            auto result = render_item_flex::init();
            src_el()->add_render(result);
            return result;
        }
        std::shared_ptr<render_item> clone() override
        {
            return std::make_shared<render_item_grid>(src_el());
        }
    };
} // namespace litehtml

#endif // LITEHTML_RENDER_GRID_H
