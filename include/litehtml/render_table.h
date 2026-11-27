#ifndef LITEHTML_RENDER_TABLE_H
#define LITEHTML_RENDER_TABLE_H

#include "render_item.h"
#include <tuple>

namespace litehtml
{
    class render_item_table_part : public render_item
    {
      protected:
        // Cell areas this part's background is painted over. CSS 2.1 17.5.1 paints the column,
        // column group, row and row group backgrounds behind the cells, in the area of the cells.
        position::vector m_paint_boxes;

        void calc_intrinsic_size() override {}

      public:
        explicit render_item_table_part(std::shared_ptr<element> src_el) :
            render_item(std::move(src_el))
        {
        }

        std::shared_ptr<render_item> clone() override
        {
            return std::make_shared<render_item_table_part>(src_el());
        }
        position::vector& paint_boxes()
        {
            return m_paint_boxes;
        }
        bool for_inline_boxes(
            const std::function<bool(const position& box, bool first, bool last)>& process) const override;
    };

    class render_item_table : public render_item
    {
      protected:
        // A table part together with the columns its background is painted over
        struct part_paint_area
        {
            std::shared_ptr<render_item_table_part> el;
            int                                     first_column;
            int                                     column_count;
        };

        // data for table rendering
        std::unique_ptr<table_grid> m_grid;
        pixel_t                     m_border_spacing_x;
        pixel_t                     m_border_spacing_y;
        // Column groups are painted below the columns, so they are kept apart.
        std::vector<part_paint_area> m_column_group_areas;
        std::vector<part_paint_area> m_column_areas;

        std::tuple<pixel_t, pixel_t> calc_table_spacings() const;
        void draw_part_background(uint_ptr hdc, const part_paint_area& area, const position& pos, const position* clip,
                                  const clipping* cur_clipping);
        void draw_row_group_backgrounds(uint_ptr hdc, const position& pos, const position* clip,
                                        const clipping* cur_clipping);
        // The borders of the columns and their groups, one entry per column.
        void collect_column_borders(std::vector<borders>& column_borders,
                                    std::vector<borders>& column_group_borders) const;

        void    calc_intrinsic_size() override;
        pixel_t _render(pixel_t x, pixel_t y, const containing_block_context& containing_block_size,
                        formatting_context* fmt_ctx, bool second_pass) override;

      public:
        explicit render_item_table(std::shared_ptr<element> src_el);

        std::shared_ptr<render_item> clone() override
        {
            return std::make_shared<render_item_table>(src_el());
        }
        void draw_children(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip, draw_flag flag, int zindex,
                           const render_item* stacking_context = nullptr, const clipping* cur_clipping = nullptr,
                           const clipping* abs_clipping = nullptr) override;
        std::shared_ptr<element> get_child_by_point(
            pixel_t x, pixel_t y, pixel_t client_x, pixel_t client_y, pixel_t doc_x, pixel_t doc_y, draw_flag flag,
            int zindex, const std::function<bool(const std::shared_ptr<render_item>&)>& check,
            const render_item* stacking_context = nullptr) override;
        pixel_t                      get_draw_vertical_offset() override;
        pixel_t                      get_draw_vertical_bottom_offset() override;
        bool                         paints_collapsed_borders() const override;
        std::shared_ptr<render_item> init() override;
    };

    class render_item_table_row : public render_item
    {
      protected:
        void calc_intrinsic_size() override {}

      public:
        explicit render_item_table_row(std::shared_ptr<element> src_el) :
            render_item(std::move(src_el))
        {
        }

        std::shared_ptr<render_item> clone() override
        {
            return std::make_shared<render_item_table_row>(src_el());
        }
        bool for_inline_boxes([[maybe_unused]] const std::function<bool(const position& box, bool first, bool last)>&
                                  process) const override;
        bool is_point_inside(pixel_t x, pixel_t y) const override;
    };
} // namespace litehtml

#endif // LITEHTML_RENDER_TABLE_H
