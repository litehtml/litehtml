#ifndef LITEHTML_RENDER_ITEM_H
#define LITEHTML_RENDER_ITEM_H

#include <memory>
#include <list>
#include <optional>
#include <tuple>
#include "html.h"
#include "pixel_type.h"
#include "types.h"
#include "line_box.h"
#include "table.h"
#include "formatting_context.h"
#include "element.h"
#include "scroll_view.h"

namespace litehtml
{
    class element;
    class clipping;

    using split_inlines_result =
        std::tuple<std::shared_ptr<render_item>, std::shared_ptr<render_item>, std::shared_ptr<render_item>>;

    class render_item : public std::enable_shared_from_this<render_item>
    {
        friend class document;

      protected:
        std::shared_ptr<element>                  m_element;
        std::weak_ptr<render_item>                m_parent;
        std::list<std::shared_ptr<render_item>>   m_children;
        std::vector<std::shared_ptr<render_item>> m_stacking_contexts;
        std::weak_ptr<render_item>                m_stacking_context;
        std::shared_ptr<scroll_view>              m_scroll_view;

        margins m_margins;
        margins m_padding;
        margins m_borders;
        // The borders the box is laid out with; the collapsed ones while the table renders a cell.
        bool                   m_borders_overridden = false;
        std::optional<borders> m_paint_borders;
        position               m_pos;
        // Arguments of the last render(): the item can be laid out again in place without the
        // parent once its own box is known to be definite.
        containing_block_context m_cb_context;
        position                 m_render_origin;
        bool                     m_outer_floats = false;
        size                     m_intrinsic_min_size;
        size                     m_intrinsic_max_size;
        pixel_t                  m_intrinsic_content_min_width = 0_px;
        pixel_t                  m_intrinsic_content_max_width = 0_px;
        pixel_t                  m_intrinsic_content_height    = 0_px;
        position                 m_bounds; // bounding box of the element
        bool                     m_skip                         = false;
        bool                     m_height_dependent             = false;
        uint64_t                 m_height_dependency_generation = 0;

        // Results of recent layouts of a subtree with its own formatting context, valid during one
        // document render. A hit restores the item's own box; when the hit is not the layout the
        // descendants currently hold, the item is marked stale and laid out again by ensure_layout()
        // before anything reads its descendants.
        struct layout_entry
        {
            uint64_t                 generation = 0;
            uint64_t                 last_use   = 0;
            containing_block_context size;
            bool                     second_pass = false;
            position                 pos; // relative to the render() origin
            litehtml::margins        margin_box;
            litehtml::margins        padding_box;
            litehtml::margins        border_box;
            pixel_t                  result = 0_px;
        };
        static constexpr int layout_entries_count = 2;
        struct layout_cache
        {
            layout_entry entries[layout_entries_count];
            int          current = -1; // entry matching the descendants' layout
            int          used    = -1; // entry the parent got from the last render()
            uint64_t     tick    = 0;
        };
        // Allocated on first use, so items that never take the cached path stay small.
        std::unique_ptr<layout_cache> m_layout_cache;
        bool                          m_layout_stale = false;
        // Set on stale items and their ancestors, so ensure_layout_tree() visits only those branches.
        bool m_stale_subtree = false;

        void    update_height_dependency(uint64_t generation);
        void    mark_stale_subtree();
        pixel_t execute_render(pixel_t x, pixel_t y, const containing_block_context& containing_block_size,
                               bool second_pass, uint64_t generation, int slot);
        void render_positioned_children(render_item&                        containing_block,
                                        const std::shared_ptr<render_item>& stacking_parent, const position& viewport,
                                        render_type rt);
        void render_positioned_element(const std::shared_ptr<render_item>& el, const position& viewport);

        containing_block_context calculate_containing_block_context(const containing_block_context& cb_context) const;
        void                     calc_cb_length(const css_length& len, pixel_t percent_base,
                                                containing_block_context::typed_pixel& out_value) const;
        void                     calc_cb_width(const css_length& len, pixel_t percent_base,
                                               containing_block_context::typed_pixel& out_value) const;
        virtual void             calc_intrinsic_size() = 0;
        virtual std::optional<position> positioned_grid_area(const std::shared_ptr<render_item>& /*el*/,
                                                             const position& /*padding_box*/) const
        {
            return std::nullopt;
        }
        virtual pixel_t _render(pixel_t /*x*/, pixel_t /*y*/, const containing_block_context& /*containing_block_size*/,
                                formatting_context* /*fmt_ctx*/, bool /*second_pass = false*/)
        {
            return 0_px;
        }

      public:
        explicit render_item(std::shared_ptr<element> src_el);

        virtual ~render_item() = default;

        pixel_t get_scroll_left() const
        {
            return m_scroll_view ? m_scroll_view->get_left() : 0_px;
        }

        pixel_t get_scroll_top() const
        {
            return m_scroll_view ? m_scroll_view->get_top() : 0_px;
        }

        void scroll_box(position& box) const
        {
            if(m_scroll_view)
            {
                box.x -= m_scroll_view->get_left();
                box.y -= m_scroll_view->get_top();
            }
        }

        const size& get_intrinsic_max_size() const
        {
            return m_intrinsic_max_size;
        }

        const size& get_intrinsic_min_size() const
        {
            return m_intrinsic_min_size;
        }

        pixel_t get_intrinsic_content_height() const
        {
            return m_intrinsic_content_height;
        }

        // Whether making the item's height definite can change its descendants' layout.
        // Computed on demand once per document render, as only flex layout needs it.
        bool height_dependent(uint64_t generation)
        {
            if(m_height_dependency_generation != generation)
            {
                m_height_dependency_generation = generation;
                update_height_dependency(generation);
            }
            return m_height_dependent;
        }

        // Lays out the descendants again if they don't match the item's last render() result.
        void ensure_layout();
        void ensure_layout_tree();

        // Whether the item's border box is fully determined by its containing block, so a layout
        // change inside it cannot move or resize anything outside of it.
        bool has_definite_size() const;

        const containing_block_context& cb_context() const
        {
            return m_cb_context;
        }

        const position& render_origin() const
        {
            return m_render_origin;
        }

        // The formatting context this item was laid out in had floats, so it cannot be laid out
        // again with a fresh context without changing where its content flows.
        bool has_outer_floats() const
        {
            return m_outer_floats;
        }

        // Called before descendants are changed outside of the item's render().
        void invalidate_layout()
        {
            ensure_layout();
            if(m_layout_cache)
            {
                m_layout_cache->current = -1;
            }
        }

        void    update_intrinsic_size();
        pixel_t resolve_intrinsic_width(const css_length& value, pixel_t available_width) const;

        pixel_t h_scroll(pixel_t dx)
        {
            return m_scroll_view ? m_scroll_view->h_scroll(dx) : 0_px;
        }

        virtual pixel_t v_scroll(pixel_t dy)
        {
            return m_scroll_view ? m_scroll_view->v_scroll(dy) : 0_px;
        }

        bool is_h_scrollable(const pixel_t dx) const
        {
            return m_scroll_view ? m_scroll_view->is_h_scrollable(dx) : false;
        }

        virtual bool is_v_scrollable(const pixel_t dy) const
        {
            return m_scroll_view ? m_scroll_view->is_v_scrollable(dy) : false;
        }

        std::list<std::shared_ptr<render_item>>& children()
        {
            return m_children;
        }

        // Access to the m_pos
        position& pos()
        {
            return m_pos;
        }

        // Calculates the position of the element in the document
        // This position is relative to the x and y arguments
        // Scroll shifts are applied to the position
        position calc_placement(int x = 0, int y = 0) const
        {
            position pos  = m_pos;
            pos.x        += pixel_t(x) - get_scroll_left();
            pos.y        += pixel_t(y) - get_scroll_top();
            return pos;
        }

        bool skip() const
        {
            return m_skip;
        }

        void skip(bool val)
        {
            m_skip = val;
        }

        // Element right (including margins, padding, and borders)
        pixel_t right() const
        {
            return left() + width();
        }

        // Element right (excluding margins, but including padding and borders)
        pixel_t content_right() const
        {
            return content_left() + content_width();
        }

        // Element left (including margins, padding, and borders)
        pixel_t left() const
        {
            return m_pos.left() - m_margins.left - m_padding.left - m_borders.left;
        }

        // Element left (excluding margins, but including padding and borders)
        pixel_t content_left() const
        {
            return m_pos.left() - m_padding.left - m_borders.left;
        }

        // Element top (including margins, padding, and borders)
        pixel_t top() const
        {
            return m_pos.top() - m_margins.top - m_padding.top - m_borders.top;
        }

        // Element top (excluding margins, but including padding and borders)
        pixel_t content_top() const
        {
            return m_pos.top() - m_padding.top - m_borders.top;
        }

        // Element bottom (including margins, padding, and borders)
        pixel_t bottom() const
        {
            return top() + height();
        }

        // Element bottom (excluding margins, but including padding and borders)
        pixel_t content_bottom() const
        {
            return content_top() + content_height();
        }

        // Element height (including margins, padding, and borders)
        pixel_t height() const
        {
            return m_pos.height + m_margins.height() + m_padding.height() + m_borders.height();
        }

        // Element height (excluding margins, but including padding and borders)
        pixel_t content_height() const
        {
            return m_pos.height + m_padding.height() + m_borders.height();
        }

        // Element width (including margins, padding, and borders)
        pixel_t width() const
        {
            return m_pos.width + m_margins.width() + m_padding.width() + m_borders.width();
        }

        // Element width (excluding margins, but including padding and borders)
        pixel_t content_width() const
        {
            return m_pos.width + m_padding.width() + m_borders.width();
        }

        pixel_t padding_top() const
        {
            return m_padding.top;
        }

        pixel_t padding_bottom() const
        {
            return m_padding.bottom;
        }

        pixel_t padding_left() const
        {
            return m_padding.left;
        }

        pixel_t padding_right() const
        {
            return m_padding.right;
        }

        pixel_t border_top() const
        {
            return m_borders.top;
        }

        pixel_t border_bottom() const
        {
            return m_borders.bottom;
        }

        pixel_t border_left() const
        {
            return m_borders.left;
        }

        pixel_t border_right() const
        {
            return m_borders.right;
        }

        pixel_t margin_top() const
        {
            return m_margins.top;
        }

        pixel_t margin_bottom() const
        {
            return m_margins.bottom;
        }

        pixel_t margin_left() const
        {
            return m_margins.left;
        }

        pixel_t margin_right() const
        {
            return m_margins.right;
        }

        std::shared_ptr<render_item> parent() const
        {
            return m_parent.lock();
        }

        margins& get_margins()
        {
            return m_margins;
        }

        margins& get_paddings()
        {
            return m_padding;
        }

        void set_paddings(const margins& val)
        {
            m_padding = val;
        }

        margins& get_borders()
        {
            return m_borders;
        }

        const margins& get_borders() const
        {
            return m_borders;
        }

        // Lays the box out with the collapsed borders of a table cell (CSS 2.1 17.6.2).
        void override_borders(const margins& val)
        {
            m_borders            = val;
            m_borders_overridden = true;
        }

        // Restores the borders the box is laid out with once the cell is laid out.
        void clear_borders_override(const margins& own_borders)
        {
            m_borders            = own_borders;
            m_borders_overridden = false;
        }

        // The widths of the borders painted around the box: the collapsed ones for a table cell,
        // its own otherwise.
        margins paint_border_widths() const
        {
            if(!m_paint_borders)
            {
                return m_borders;
            }
            margins ret;
            ret.left   = m_paint_borders->left.width;
            ret.right  = m_paint_borders->right.width;
            ret.top    = m_paint_borders->top.width;
            ret.bottom = m_paint_borders->bottom.width;
            return ret;
        }

        // The element's borders with the used widths (in pixels) and the computed style and color.
        borders get_styled_borders() const;

        // The collapsed borders the table resolved for this item, painted instead of the element's own ones.
        const std::optional<borders>& paint_borders() const
        {
            return m_paint_borders;
        }

        void set_paint_borders(const borders& val)
        {
            m_paint_borders = val;
        }

        void reset_paint_borders()
        {
            m_paint_borders.reset();
        }

        // True when this item paints the collapsed borders of its grid itself.
        virtual bool paints_collapsed_borders() const
        {
            return false;
        }

        /**
         * Top offset to the element content. Includes paddings, margins and borders.
         */
        pixel_t content_offset_top() const
        {
            return m_margins.top + m_padding.top + m_borders.top;
        }

        /**
         * Bottom offset to the element content. Includes paddings, margins and borders.
         */
        pixel_t content_offset_bottom() const
        {
            return m_margins.bottom + m_padding.bottom + m_borders.bottom;
        }

        /**
         * Left offset to the element content. Includes paddings, margins and borders.
         */
        pixel_t content_offset_left() const
        {
            return m_margins.left + m_padding.left + m_borders.left;
        }

        /**
         * Right offset to the element content. Includes paddings, margins and borders.
         */
        pixel_t content_offset_right() const
        {
            return m_margins.right + m_padding.right + m_borders.right;
        }

        /**
         * Sum of left and right offsets to the element content. Includes paddings, margins and borders.
         */
        pixel_t content_offset_width() const
        {
            return content_offset_left() + content_offset_right();
        }

        /**
         * Sum of top and bottom offsets to the element content. Includes paddings, margins and borders.
         */
        pixel_t content_offset_height() const
        {
            return content_offset_top() + content_offset_bottom();
        }

        pixel_t render_offset_left() const
        {
            if(css().get_box_sizing() == box_sizing_content_box)
            {
                return m_margins.left + m_borders.left + m_padding.left;
            }
            return m_margins.left;
        }

        pixel_t render_offset_right() const
        {
            if(css().get_box_sizing() == box_sizing_content_box)
            {
                return m_margins.right + m_borders.right + m_padding.right;
            }
            return m_margins.right;
        }

        pixel_t render_offset_width() const
        {
            return render_offset_left() + render_offset_right();
        }

        pixel_t render_offset_top() const
        {
            if(css().get_box_sizing() == box_sizing_content_box)
            {
                return m_margins.top + m_borders.top + m_padding.top;
            }
            return m_margins.top;
        }

        pixel_t render_offset_bottom() const
        {
            if(css().get_box_sizing() == box_sizing_content_box)
            {
                return m_margins.bottom + m_borders.bottom + m_padding.bottom;
            }
            return m_margins.bottom;
        }

        pixel_t render_offset_height() const
        {
            return render_offset_top() + render_offset_bottom();
        }

        pixel_t box_sizing_left() const
        {
            if(css().get_box_sizing() == box_sizing_border_box)
            {
                return m_padding.left + m_borders.left;
            }
            return 0_px;
        }

        pixel_t box_sizing_right() const
        {
            if(css().get_box_sizing() == box_sizing_border_box)
            {
                return m_padding.right + m_borders.right;
            }
            return 0_px;
        }

        pixel_t box_sizing_width() const
        {
            return box_sizing_left() + box_sizing_right();
        }

        pixel_t box_sizing_top() const
        {
            if(css().get_box_sizing() == box_sizing_border_box)
            {
                return m_padding.top + m_borders.top;
            }
            return 0_px;
        }

        pixel_t box_sizing_bottom() const
        {
            if(css().get_box_sizing() == box_sizing_border_box)
            {
                return m_padding.bottom + m_borders.bottom;
            }
            return 0_px;
        }

        pixel_t box_sizing_height() const
        {
            return box_sizing_top() + box_sizing_bottom();
        }

        void parent(const std::shared_ptr<render_item>& par)
        {
            m_parent = par;
        }

        const std::shared_ptr<element>& src_el() const
        {
            return m_element;
        }

        const css_properties& css() const
        {
            return m_element->css();
        }

        void add_child(const std::shared_ptr<render_item>& ri)
        {
            m_children.push_back(ri);
            ri->parent(shared_from_this());
        }

        bool is_root() const
        {
            return m_parent.expired();
        }

        bool collapse_top_margin() const
        {
            return m_borders.top == 0_px && m_padding.top == 0_px && m_element->in_normal_flow() &&
                   m_element->css().get_float() == float_none && m_margins.top >= 0_px && !is_flex_item() &&
                   !is_grid_item() && !is_root() &&
                   !is_one_of(css().get_overflow(), overflow_hidden, overflow_scroll, overflow_auto);
        }

        bool collapse_bottom_margin() const
        {
            return m_borders.bottom == 0_px && m_padding.bottom == 0_px && m_element->in_normal_flow() &&
                   m_element->css().get_float() == float_none && m_margins.bottom >= 0_px && !is_grid_item() &&
                   !is_root() && !is_one_of(css().get_overflow(), overflow_hidden, overflow_scroll, overflow_auto);
        }

        bool is_visible() const
        {
            return !m_skip && src_el()->css().get_display() != display_none &&
                   src_el()->css().get_visibility() == visibility_visible;
        }

        bool is_flex_item() const
        {
            auto par = parent();
            return par && (par->css().get_display() == display_inline_flex || par->css().get_display() == display_flex);
        }

        bool is_grid_item() const
        {
            auto par = parent();
            return par && is_one_of(par->css().get_display(), display_grid, display_inline_grid);
        }

        pixel_t render(pixel_t x, pixel_t y, const containing_block_context& containing_block_size,
                       formatting_context* fmt_ctx, bool second_pass = false);
        void    apply_relative_shift(const containing_block_context& containing_block_size);
        // How far position: relative moves this element (0, 0 when it isn't relatively positioned)
        position relative_shift(const containing_block_context& containing_block_size) const;
        void     calc_outlines(pixel_t parent_width);
        pixel_t  calc_auto_margins(pixel_t parent_width); // returns left margin

        virtual std::shared_ptr<render_item> init();
        virtual void                         apply_vertical_align() {}
        /**
         * Get first baseline position. Default position is element bottom without bottom margin.
         * @returns offset of the first baseline from element top
         */
        virtual pixel_t get_first_baseline()
        {
            return height() - margin_bottom();
        }
        /**
         * Get the last baseline position.  The default position is element bottom without bottom margin.
         * @returns offset of the last baseline from element top
         */
        virtual pixel_t get_last_baseline()
        {
            return height() - margin_bottom();
        }

        virtual std::shared_ptr<render_item> clone() = 0;

        split_inlines_result split_inlines();
        // Lay out positioned descendants and rebuild stacking membership in every render mode.
        void render_positioned(render_type rt = render_all);
        // returns element offset related to the containing block
        std::tuple<pixel_t, pixel_t> element_static_offset(const std::shared_ptr<litehtml::render_item>& el);

        // Whether this element establishes a stacking context. See
        // https://drafts.csswg.org/css-position-4/#stacking-contexts
        bool creates_stacking_context() const;
        // Whether this element is a stacked child of a stacking context.
        bool is_stacked_member() const
        {
            return !m_stacking_context.expired();
        }
        // Calculate the bounding box of the element, including its children.
        void calc_bounds();
        void calc_document_size(litehtml::size& sz, pixel_t x = 0_px, pixel_t y = 0_px);

        /**
         * @brief Call func for all inline boxes
         * @param process function to call for boxes. "first "argument is true for the first box, "last" argument is
         * true for the last box. Function should return true to continue.
         * @return returns true if supported, else returns false
         */
        virtual bool for_inline_boxes(
            [[maybe_unused]] const std::function<bool(const position& box, bool first, bool last)>& process) const
        {
            return false;
        }

        virtual void set_inline_boxes(position::vector& /*boxes*/) {};
        virtual void add_inline_box(const position& /*box*/) {};
        virtual void clear_inline_boxes() {};
        void draw_stacking_context(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip, bool with_positioned,
                                   const clipping* cur_clipping = nullptr, const clipping* abs_clipping = nullptr);
        virtual void draw_children(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip, draw_flag flag, int zindex,
                                   const render_item* stacking_context = nullptr,
                                   const clipping* cur_clipping = nullptr, const clipping* abs_clipping = nullptr);
        virtual pixel_t get_draw_vertical_offset()
        {
            return 0_px;
        }
        virtual std::shared_ptr<element> get_child_by_point(
            pixel_t x, pixel_t y, pixel_t client_x, pixel_t client_y, pixel_t doc_x, pixel_t doc_y, draw_flag flag,
            int zindex, const std::function<bool(const std::shared_ptr<render_item>&)>& check,
            const render_item* stacking_context = nullptr);
        std::shared_ptr<element> get_element_by_point(
            pixel_t x, pixel_t y, pixel_t client_x, pixel_t client_y, pixel_t doc_x, pixel_t doc_y,
            const std::function<bool(const std::shared_ptr<render_item>&)>& check);
        virtual bool is_point_inside(pixel_t x, pixel_t y) const;
        // Whether the point, given in document coordinates (client_x/client_y are viewport
        // coordinates used inside position:fixed subtrees), is inside every overflow clip that
        // applies to this element. Absolutely positioned elements are not clipped by static
        // ancestors below their containing block and fixed elements are not clipped at all.
        bool         is_point_visible(pixel_t doc_x, pixel_t doc_y, pixel_t client_x, pixel_t client_y) const;
        void         dump(litehtml::dumper& cout);
        position     get_placement() const;
        virtual void y_shift(pixel_t delta);
        /**
         * Returns the boxes of rendering element. All coordinates are absolute
         *
         * @param redraw_box function to be called on every rendering box
         * @param include_outline include the area painted by the outline (redraw invalidation)
         * @return
         */
        void get_rendering_boxes(const std::function<void(const position&)>& redraw_box,
                                 bool                                        include_outline = true) const;
    };
} // namespace litehtml

#endif // LITEHTML_RENDER_ITEM_H
