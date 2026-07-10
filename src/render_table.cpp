#include "render_table.h"
#include "document.h"
#include "iterators.h"
#include "pixel_type.h"

litehtml::render_item_table::render_item_table(std::shared_ptr<element> _src_el) :
    render_item(std::move(_src_el)),
    m_border_spacing_x(0),
    m_border_spacing_y(0)
{
}

std::tuple<litehtml::pixel_t, litehtml::pixel_t> litehtml::render_item_table::calc_table_spacings() const
{
    pixel_t table_width_spacing = 0_px;
    if(src_el()->css().get_border_collapse() == border_collapse_separate)
    {
        table_width_spacing = m_border_spacing_x * pixel_t(m_grid->cols_count() + 1);
    } else
    {
        table_width_spacing = 0_px;

        if(m_grid->cols_count())
        {
            table_width_spacing -= std::min(border_left(), m_grid->column(0).border_left);
            table_width_spacing -= std::min(border_right(), m_grid->column(m_grid->cols_count() - 1).border_right);
        }

        for(int col = 1; col < m_grid->cols_count(); col++)
        {
            table_width_spacing -= std::min(m_grid->column(col).border_left, m_grid->column(col - 1).border_right);
        }
    }

    // Calculate vertical table spacing
    pixel_t table_height_spacing = 0_px;
    if(src_el()->css().get_border_collapse() == border_collapse_separate)
    {
        table_height_spacing = m_border_spacing_y * pixel_t(m_grid->rows_count() + 1);
    } else
    {
        table_height_spacing = 0_px;

        if(m_grid->rows_count())
        {
            table_height_spacing -= std::min(border_top(), m_grid->row(0).border_top);
            table_height_spacing -= std::min(border_bottom(), m_grid->row(m_grid->rows_count() - 1).border_bottom);
        }

        for(int row = 1; row < m_grid->rows_count(); row++)
        {
            table_height_spacing -= std::min(m_grid->row(row).border_top, m_grid->row(row - 1).border_bottom);
        }
    }

    return {table_width_spacing, table_height_spacing};
}

litehtml::pixel_t litehtml::render_item_table::_render(pixel_t x, pixel_t y,
                                                       const containing_block_context& containing_block_size,
                                                       formatting_context*             fmt_ctx, bool /*second_pass*/)
{
    if(!m_grid)
    {
        return 0_px;
    }

    containing_block_context self_size = calculate_containing_block_context(containing_block_size);

    // Calculate table spacing
    auto [table_width_spacing, table_height_spacing] = calc_table_spacings();

    // If the 'table' or 'inline-table' element's 'width' property has a computed value (W) other than 'auto', the used
    // width is the greater of W, CAPMIN, and the minimum width required by all the columns plus cell spacing or borders
    // (MIN). If the used width is greater than MIN, the extra width should be distributed over the columns.
    //
    // If the 'table' or 'inline-table' element has 'width: auto', the used width is the greater of the table's
    // containing block width, CAPMIN, and MIN. However, if either CAPMIN or the maximum width required by the columns
    // plus cell spacing or borders (MAX) is less than that of the containing block, use max(MAX, CAPMIN).

    pixel_t table_width     = 0_px;
    pixel_t min_table_width = 0_px;
    pixel_t max_table_width = 0_px;

    if(self_size.width.type == containing_block_context::cbc_value_type_absolute)
    {
        table_width = m_grid->calc_table_width(self_size.render_width.value - table_width_spacing, false,
                                               min_table_width, max_table_width);
    } else
    {
        table_width = m_grid->calc_table_width(self_size.render_width.value - table_width_spacing,
                                               self_size.width.type == containing_block_context::cbc_value_type_auto,
                                               min_table_width, max_table_width);
    }

    min_table_width += table_width_spacing;
    max_table_width += table_width_spacing;
    table_width     += table_width_spacing;
    m_grid->calc_horizontal_positions(m_borders, src_el()->css().get_border_collapse(), m_border_spacing_x);

    bool row_span_found = false;

    // render cells with computed width
    for(int row = 0; row < m_grid->rows_count(); row++)
    {
        m_grid->row(row).height = 0;
        for(int col = 0; col < m_grid->cols_count(); col++)
        {
            table_cell* cell = m_grid->cell(col, row);
            if(cell->el)
            {
                int span_col = col + cell->colspan - 1;
                if(span_col >= m_grid->cols_count())
                {
                    span_col = m_grid->cols_count() - 1;
                }
                pixel_t cell_width = m_grid->column(span_col).right - m_grid->column(col).left;

                cell->el->render(m_grid->column(col).left, 0_px, self_size.new_width(cell_width), fmt_ctx, true);
                cell->el->pos().width = cell_width - cell->el->content_offset_width();

                if(cell->rowspan <= 1)
                {
                    m_grid->row(row).height = std::max(m_grid->row(row).height, cell->el->height());
                } else
                {
                    row_span_found = true;
                }
            }
        }
    }

    if(row_span_found)
    {
        for(int col = 0; col < m_grid->cols_count(); col++)
        {
            for(int row = 0; row < m_grid->rows_count(); row++)
            {
                table_cell* cell = m_grid->cell(col, row);
                if(cell->el)
                {
                    int span_row = row + cell->rowspan - 1;
                    if(span_row >= m_grid->rows_count())
                    {
                        span_row = m_grid->rows_count() - 1;
                    }
                    if(span_row != row)
                    {
                        pixel_t h = 0_px;
                        for(int i = row; i <= span_row; i++)
                        {
                            h += m_grid->row(i).height;
                        }
                        if(h < cell->el->height())
                        {
                            m_grid->row(span_row).height += cell->el->height() - h;
                        }
                    }
                }
            }
        }
    }

    // calculate block height
    pixel_t block_height = 0_px;
    if(self_size.height.type != containing_block_context::cbc_value_type_auto && self_size.height.value > 0_px)
    {
        block_height = self_size.height.value - (m_padding.height() + m_borders.height());
    }

    // calculate minimum height from m_css.get_min_height()
    pixel_t min_height = 0_px;
    if(!src_el()->css().get_min_height().is_predefined() &&
       src_el()->css().get_min_height().units() == css_units_percentage)
    {
        min_height = src_el()->css().get_min_height().calc_percent(containing_block_size.height);
    } else
    {
        min_height = static_cast<pixel_t>(src_el()->css().get_min_height().val());
    }

    pixel_t minimum_table_height = std::max(block_height, min_height);

    m_grid->calc_rows_height(minimum_table_height - table_height_spacing, m_border_spacing_y);
    m_grid->calc_vertical_positions(m_borders, src_el()->css().get_border_collapse(), m_border_spacing_y);

    pixel_t table_height = 0_px;

    // place cells vertically
    for(int col = 0; col < m_grid->cols_count(); col++)
    {
        for(int row = 0; row < m_grid->rows_count(); row++)
        {
            table_cell* cell = m_grid->cell(col, row);
            if(cell->el)
            {
                int span_row = row + cell->rowspan - 1;
                if(span_row >= m_grid->rows_count())
                {
                    span_row = m_grid->rows_count() - 1;
                }
                cell->el->pos().y      = m_grid->row(row).top + cell->el->content_offset_top();
                cell->el->pos().height = m_grid->row(span_row).bottom - m_grid->row(row).top -
                                         cell->el->content_offset_top() - cell->el->content_offset_bottom();
                table_height           = std::max(table_height, m_grid->row(span_row).bottom);
                cell->el->apply_vertical_align();
            }
        }
    }

    if(src_el()->css().get_border_collapse() == border_collapse_collapse)
    {
        if(m_grid->rows_count())
        {
            table_height -= std::min(border_bottom(), m_grid->row(m_grid->rows_count() - 1).border_bottom);
        }
    } else
    {
        table_height += m_border_spacing_y;
    }

    // Render table captions
    // Table border doesn't round the caption, so we have to start caption in the border position
    pixel_t top_captions = -border_top();

    for(auto& caption : m_grid->captions())
    {
        if(caption->css().get_caption_side() == caption_side_top)
        {
            caption->render(-border_left(), top_captions,
                            self_size.new_width(table_width + border_left() + border_right()), fmt_ctx);
            top_captions += caption->height();
        }
    }

    if(top_captions != 0_px)
    {
        // Add border height to get the top of cells
        top_captions += border_top();

        // Save caption height for draw_background
        m_grid->top_captions_height(top_captions);

        // Move table cells to the bottom side
        for(int row = 0; row < m_grid->rows_count(); row++)
        {
            m_grid->row(row).el_row->pos().y += top_captions;
            for(int col = 0; col < m_grid->cols_count(); col++)
            {
                table_cell* cell = m_grid->cell(col, row);
                if(cell->el)
                {
                    cell->el->pos().y += top_captions;
                }
            }
        }
    }

    pixel_t bottom_captions = 0_px;

    for(auto& caption : m_grid->captions())
    {
        if(caption->css().get_caption_side() == caption_side_bottom)
        {
            caption->render(-border_left(), table_height + top_captions + bottom_captions,
                            self_size.new_width(table_width + border_left() + border_right()), fmt_ctx);
            bottom_captions += caption->height();
        }
    }

    m_pos.move_to(x + content_offset_left(), y + content_offset_top());
    m_pos.width  = table_width;
    m_pos.height = table_height + top_captions + bottom_captions;

    if(self_size.width.type == containing_block_context::cbc_value_type_absolute)
    {
        return table_width + content_offset_width();
    }
    return std::min(max_table_width + content_offset_width(), table_width + content_offset_width());
}

void litehtml::render_item_table::calc_intrinsic_size()
{
    if(!m_grid)
    {
        return;
    }

    auto [table_width_spacing, table_height_spacing] = calc_table_spacings();

    pixel_t table_width = 0_px;

    if(!css().get_width().is_predefined() && css().get_width().units() != css_units_percentage)
    {
        table_width = m_grid->calc_table_width({}, false, m_intrinsic_min_size.width, m_intrinsic_max_size.width);
    } else
    {
        table_width = m_grid->calc_table_width({}, css().get_width().is_predefined(), m_intrinsic_min_size.width,
                                               m_intrinsic_max_size.width);
    }

    for(int row = 0; row < m_grid->rows_count(); row++)
    {
        pixel_t min_row_height = 0_px;
        pixel_t max_row_height = 0_px;
        for(int col = 0; col < m_grid->cols_count(); col++)
        {
            table_cell* cell = m_grid->cell(col, row);
            if(cell && cell->el)
            {
                min_row_height = std::max(min_row_height, cell->el->get_intrinsic_min_size().height);
                max_row_height = std::max(max_row_height, cell->el->get_intrinsic_max_size().height);
            }
        }
        m_intrinsic_min_size.height += min_row_height;
        m_intrinsic_max_size.height += max_row_height;
    }

    m_intrinsic_min_size.width  += table_width_spacing + content_offset_width();
    m_intrinsic_max_size.width  += table_width_spacing + content_offset_width();
    m_intrinsic_min_size.height += table_height_spacing + content_offset_height();
    m_intrinsic_max_size.height += table_height_spacing + content_offset_height();
}

std::shared_ptr<litehtml::render_item> litehtml::render_item_table::init()
{
    // Initialize Grid
    m_grid = std::make_unique<table_grid>();

    go_inside_table      table_selector;
    table_rows_selector  row_selector;
    table_cells_selector cell_selector;

    elements_iterator row_iter(false, &table_selector, &row_selector);

    row_iter.process(shared_from_this(), [&](std::shared_ptr<render_item>& el, iterator_item_type /*item_type*/) {
        m_grid->begin_row(el);

        elements_iterator cell_iter(true, &table_selector, &cell_selector);
        cell_iter.process(el, [&](std::shared_ptr<render_item>& el, iterator_item_type item_type) {
            if(item_type != iterator_item_type_end_parent)
            {
                el = el->init();
                m_grid->add_cell(el);
            }
        });
    });

    for(auto& el : m_children)
    {
        if(el->src_el()->css().get_display() == display_table_caption)
        {
            el = el->init();
            m_grid->captions().push_back(el);
        }
    }

    m_grid->finish();

    if(src_el()->css().get_border_collapse() == border_collapse_separate)
    {
        auto          fm   = css().get_font_metrics();
        document::ptr doc  = src_el()->get_document();
        m_border_spacing_x = doc->to_pixels(src_el()->css().get_border_spacing_x(), fm, 0_px);
        m_border_spacing_y = doc->to_pixels(src_el()->css().get_border_spacing_y(), fm, 0_px);
    } else
    {
        m_border_spacing_x = 0_px;
        m_border_spacing_y = 0_px;
    }

    src_el()->add_render(shared_from_this());

    return shared_from_this();
}

void litehtml::render_item_table::draw_children(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip,
                                                draw_flag flag, int zindex)
{
    if(!m_grid)
    {
        return;
    }

    position pos  = m_pos;
    pos.x        += x;
    pos.y        += y;
    for(auto& caption : m_grid->captions())
    {
        if(flag == draw_block)
        {
            caption->src_el()->draw(hdc, pos.x, pos.y, clip, caption);
        }
        caption->draw_children(hdc, pos.x, pos.y, clip, flag, zindex);
    }
    for(int row = 0; row < m_grid->rows_count(); row++)
    {
        if(flag == draw_block)
        {
            m_grid->row(row).el_row->src_el()->draw_background(hdc, pos.x, pos.y, clip, m_grid->row(row).el_row);
        }
        for(int col = 0; col < m_grid->cols_count(); col++)
        {
            table_cell* cell = m_grid->cell(col, row);
            if(cell->el)
            {
                if(flag == draw_block)
                {
                    cell->el->src_el()->draw(hdc, pos.x, pos.y, clip, cell->el);
                }
                cell->el->draw_children(hdc, pos.x, pos.y, clip, flag, zindex);
            }
        }
    }
}

litehtml::pixel_t litehtml::render_item_table::get_draw_vertical_offset()
{
    if(m_grid)
    {
        return m_grid->top_captions_height();
    }
    return 0_px;
}

bool litehtml::render_item_table_row::for_inline_boxes(
    const std::function<bool(const position& box, bool first, bool last)>& process) const
{
    std::optional<position> prev_pos;
    bool                    first = true;
    for(const auto& el : m_children)
    {
        if(el->src_el()->css().get_display() == display_table_cell)
        {
            if(prev_pos.has_value())
            {
                if(!process(prev_pos.value(), first, false))
                {
                    return true;
                }
                if(first)
                {
                    first = false;
                }
            }
            position pos;
            pos.x = el->left() + el->margin_left();
            pos.y = el->top() - m_padding.top - m_borders.top;

            pos.width  = el->right() - pos.x - el->margin_right() - el->margin_left();
            pos.height = el->height() + m_padding.top + m_padding.bottom + m_borders.top + m_borders.bottom;

            prev_pos = pos;
        }
    }
    if(prev_pos.has_value())
    {
        process(prev_pos.value(), first, true);
    }
    return true;
}

bool litehtml::render_item_table_row::is_point_inside(pixel_t x, pixel_t y) const
{
    position pos;
    for(const auto& el : m_children)
    {
        if(el->src_el()->css().get_display() == display_table_cell)
        {
            pos.x = el->left() + el->margin_left();
            pos.y = el->top() - m_padding.top - m_borders.top;

            pos.width  = el->right() - pos.x - el->margin_right() - el->margin_left();
            pos.height = el->height() + m_padding.top + m_padding.bottom + m_borders.top + m_borders.bottom;

            if(pos.is_point_inside(x, y))
            {
                return true;
            }
        }
    }
    return false;
}
