#ifndef LH_RENDER_ITEM_H
#define LH_RENDER_ITEM_H

#include <memory>
#include <utility>
#include <list>
#include <tuple>
#include "types.h"
#include "line_box.h"
#include "table.h"

namespace litehtml
{
    class element;

    class render_item : public std::enable_shared_from_this<render_item>
    {
    protected:
        std::shared_ptr<element>                    m_element;
        std::weak_ptr<render_item>                  m_parent;
        std::list<std::shared_ptr<render_item>>     m_children;
        margins						                m_margins;
        margins						                m_padding;
        margins						                m_borders;
        position					                m_pos;
        line_box*				                    m_box;
        bool                                        m_skip;
        std::vector<std::shared_ptr<render_item>>   m_positioned;

        virtual int _render(int x, int y, int max_width, bool second_pass) { return 0; }

    public:
        explicit render_item(std::shared_ptr<element>  src_el);

        virtual ~render_item() {}

        std::list<std::shared_ptr<render_item>>& children()
        {
            return m_children;
        }

        void set_line_box(line_box* box)
        {
            m_box = box;
        }

        line_box* get_line_box()
        {
            return m_box;
        }

        position& pos()
        {
            return m_pos;
        }

        bool skip() const
        {
            return m_skip;
        }

        void skip(bool val)
        {
            m_skip = val;
        }

        int right() const
        {
            return left() + width();
        }

        int left() const
        {
            return m_pos.left() - m_margins.left - m_padding.left - m_borders.left;
        }

        int top() const
        {
            return m_pos.top() - m_margins.top - m_padding.top - m_borders.top;
        }

        int bottom() const
        {
            return top() + height();
        }

        int height() const
        {
            return m_pos.height + m_margins.top + m_margins.bottom + m_padding.height() + m_borders.height();
        }

        int width() const
        {
            return m_pos.width + m_margins.left + m_margins.right + m_padding.width() + m_borders.width();
        }

        int padding_top() const
        {
            return m_padding.top;
        }

        int padding_bottom() const
        {
            return m_padding.bottom;
        }

        int padding_left() const
        {
            return m_padding.left;
        }

        int padding_right() const
        {
            return m_padding.right;
        }

        int border_top() const
        {
            return m_borders.top;
        }

        int border_bottom() const
        {
            return m_borders.bottom;
        }

        int border_left() const
        {
            return m_borders.left;
        }

        int border_right() const
        {
            return m_borders.right;
        }

        int margin_top() const
        {
            return m_margins.top;
        }

        int margin_bottom() const
        {
            return m_margins.bottom;
        }

        int margin_left() const
        {
            return m_margins.left;
        }

        int margin_right() const
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

        margins& get_borders()
        {
            return m_borders;
        }

        int content_margins_top() const
        {
            return m_margins.top + m_padding.top + m_borders.top;
        }

        inline int content_margins_bottom() const
        {
            return m_margins.bottom + m_padding.bottom + m_borders.bottom;
        }

        int content_margins_left() const
        {
            return m_margins.left + m_padding.left + m_borders.left;
        }

        int content_margins_right() const
        {
            return m_margins.right + m_padding.right + m_borders.right;
        }

        int content_margins_width() const
        {
            return content_margins_left() + content_margins_right();
        }

        int content_margins_height() const
        {
            return content_margins_top() + content_margins_bottom();
        }

        void parent(const std::shared_ptr<render_item>& par)
        {
            m_parent = par;
        }

        const std::shared_ptr<element>& src_el() const
        {
            return m_element;
        }

        void add_child(const std::shared_ptr<render_item>& ri)
        {
            m_children.push_back(ri);
            ri->parent(shared_from_this());
        }

        int render(int x, int y, int max_width)
        {
            return _render(x, y, max_width, false);
        }

        bool have_parent() const
        {
            return !m_parent.expired();
        }

        bool collapse_top_margin() const
        {
            return !m_borders.top &&
                   !m_padding.top &&
                   m_element->in_normal_flow() &&
                   m_element->css().get_float() == float_none &&
                   m_margins.top >= 0 &&
                   have_parent();
        }

        bool collapse_bottom_margin() const
        {
            return !m_borders.bottom &&
                   !m_padding.bottom &&
                   m_element->in_normal_flow() &&
                   m_element->css().get_float() == float_none &&
                   m_margins.bottom >= 0 &&
                   have_parent();
        }

        bool is_visible() const
        {
            return !(m_skip || src_el()->css().get_display() == display_none || src_el()->css().get_visibility() != visibility_visible);
        }

        int calc_width(int defVal) const;
        bool get_predefined_height(int& p_height) const;
        void apply_relative_shift(int parent_width);
        void calc_outlines( int parent_width );
        void calc_auto_margins(int parent_width);
        int get_inline_shift_left();
        int get_inline_shift_right();
        bool is_first_child_inline(const std::shared_ptr<render_item>& el) const;
        bool is_last_child_inline(const std::shared_ptr<render_item>& el) const;
        bool have_inline_child() const;

        virtual std::shared_ptr<render_item> init();
        virtual void apply_vertical_align() {}
        virtual int get_base_line() { return 0; }
        virtual std::shared_ptr<render_item> clone()
        {
            return std::make_shared<render_item>(src_el());
        }
        std::tuple<
                std::shared_ptr<litehtml::render_item>,
                std::shared_ptr<litehtml::render_item>,
                std::shared_ptr<litehtml::render_item>
                > split_inlines();
        bool fetch_positioned();
        void render_positioned(render_type rt = render_all);
        void add_positioned(const std::shared_ptr<litehtml::render_item> &el);
        void get_redraw_box(litehtml::position& pos, int x = 0, int y = 0);
        void calc_document_size( litehtml::size& sz, int x = 0, int y = 0 );
        void get_inline_boxes( position::vector& boxes );
        void draw_stacking_context( uint_ptr hdc, int x, int y, const position* clip, bool with_positioned );
        virtual void draw_children( uint_ptr hdc, int x, int y, const position* clip, draw_flag flag, int zindex );
        virtual int get_draw_vertical_offset() { return 0; }
        virtual std::shared_ptr<element> get_child_by_point(int x, int y, int client_x, int client_y, draw_flag flag, int zindex);
        std::shared_ptr<element> get_element_by_point(int x, int y, int client_x, int client_y);
        bool is_point_inside( int x, int y );
        void dump(litehtml::dumper& cout);
        position get_placement() const;
        /**
         * Returns the boxes of rendering element. All coordinates are absolute
         *
         * @param redraw_boxes [out] resulting rendering boxes
         * @return
         */
        void get_rendering_boxes( position::vector& redraw_boxes);
    };

    class render_item_block : public render_item
    {
    protected:
        floated_box::vector m_floats_left;
        floated_box::vector m_floats_right;
        int_int_cache m_cache_line_left;
        int_int_cache m_cache_line_right;

        int _render(int x, int y, int max_width, bool second_pass) override;
        virtual int _render_content(int x, int y, int max_width, bool second_pass, int ret_width) {return ret_width;}

        int place_float(const std::shared_ptr<render_item> &el, int top, int max_width);
        int get_floats_height(element_float el_float = float_none) const;
        int get_left_floats_height() const;
        int get_right_floats_height() const;
        int get_line_left( int y );
        int get_line_right( int y, int def_right );
        void get_line_left_right( int y, int def_right, int& ln_left, int& ln_right );
        void add_float(const std::shared_ptr<render_item> &el, int x, int y);
        int get_cleared_top(const std::shared_ptr<render_item> &el, int line_top) const;
        int find_next_line_top( int top, int width, int def_right );
        virtual int fix_line_width( int max_width, element_float flt ) { return 0; }
        void update_floats(int dy, const std::shared_ptr<render_item> &_parent);
    public:
        explicit render_item_block(std::shared_ptr<element>  src_el) : render_item(std::move(src_el))
        {}

        std::shared_ptr<render_item> clone() override
        {
            return std::make_shared<render_item_block>(src_el());
        }
        std::shared_ptr<render_item> init() override;
    };

    /**
     * In a block formatting context, boxes are laid out one after the other, vertically, beginning at the top of a
     * containing block.
     * https://www.w3.org/TR/CSS22/visuren.html#block-formatting
     */
    class render_item_block_context : public render_item_block
    {
    protected:
        int _render_content(int x, int y, int max_width, bool second_pass, int ret_width) override;

    public:
        explicit render_item_block_context(std::shared_ptr<element>  src_el) : render_item_block(std::move(src_el))
        {}

        std::shared_ptr<render_item> clone() override
        {
            return std::make_shared<render_item_block_context>(src_el());
        }
    };

    /**
     * An inline formatting context is established by a block container box that contains no block-level boxes.
     * https://www.w3.org/TR/CSS22/visuren.html#inline-formatting
     */
    class render_item_inline_context : public render_item_block
    {
    protected:
        std::vector<std::unique_ptr<litehtml::line_box>> m_line_boxes;

        int _render_content(int x, int y, int max_width, bool second_pass, int ret_width) override;
        int fix_line_width( int max_width, element_float flt ) override;

        int finish_last_box(bool end_of_render = false);
        int place_inline(const std::shared_ptr<render_item> &el, int max_width);
        int new_box(const std::shared_ptr<render_item> &el, int max_width, line_context& line_ctx);
        void apply_vertical_align() override;
    public:
        explicit render_item_inline_context(std::shared_ptr<element>  src_el) : render_item_block(std::move(src_el))
        {}

        std::shared_ptr<render_item> clone() override
        {
            return std::make_shared<render_item_inline_context>(src_el());
        }

        int get_base_line() override;
    };

    class render_item_table : public render_item
    {
    protected:
        // data for table rendering
        std::unique_ptr<table_grid>	m_grid;
        int						    m_border_spacing_x;
        int						    m_border_spacing_y;

        int _render(int x, int y, int max_width, bool second_pass) override;

    public:
        explicit render_item_table(std::shared_ptr<element>  src_el);

        std::shared_ptr<render_item> clone() override
        {
            return std::make_shared<render_item_table>(src_el());
        }
        void draw_children(uint_ptr hdc, int x, int y, const position* clip, draw_flag flag, int zindex) override;
        int get_draw_vertical_offset() override;
        std::shared_ptr<render_item> init() override;
    };

    class render_item_table_part : public render_item
    {
    public:
        explicit render_item_table_part(std::shared_ptr<element>  src_el) : render_item(std::move(src_el))
        {}

        int _render(int x, int y, int max_width, bool second_pass) override
        {return 0;}
        std::shared_ptr<render_item> clone() override
        {
            return std::make_shared<render_item_table_part>(src_el());
        }
    };

    class render_item_inline : public render_item
    {
    protected:
        int _render(int x, int y, int max_width, bool second_pass) override;

    public:
        explicit render_item_inline(std::shared_ptr<element>  src_el) : render_item(std::move(src_el))
        {}

        std::shared_ptr<render_item> clone() override
        {
            return std::make_shared<render_item_inline>(src_el());
        }
    };

    class render_item_image : public render_item
    {
    protected:
        int _render(int x, int y, int max_width, bool second_pass) override;
        int calc_max_height(int image_height);

    public:
        explicit render_item_image(std::shared_ptr<element>  src_el) : render_item(std::move(src_el))
        {}

        std::shared_ptr<render_item> clone() override
        {
            return std::make_shared<render_item_image>(src_el());
        }
    };

    class render_item_flex : public render_item_block
    {
        struct flex_item
        {
            std::shared_ptr<render_item> el;
            int base_size;
            int main_size;
            int min_width;
            int max_width;
            int line;

            explicit flex_item(std::shared_ptr<render_item>  _el) :
                el(std::move(_el)),
                min_width(0),
                max_width(0),
                line(0),
                base_size(0),
                main_size(0)
            {}
        };
    protected:
        std::list<std::unique_ptr<flex_item>>   m_flex_items;

        int _render_content(int x, int y, int max_width, bool second_pass, int ret_width) override;

    public:
        explicit render_item_flex(std::shared_ptr<element>  src_el) : render_item_block(std::move(src_el))
        {}

        std::shared_ptr<render_item> clone() override
        {
            return std::make_shared<render_item_flex>(src_el());
        }
        void draw_children(uint_ptr hdc, int x, int y, const position* clip, draw_flag flag, int zindex) override;
        std::shared_ptr<render_item> init() override;
    };

}

#endif //LH_RENDER_ITEM_H
