#include "html.h"
#include "el_table.h"
#include "document.h"
#include "table.h"
#include "iterators.h"


litehtml::el_table::el_table( litehtml::document* doc ) : element(doc)
{

}


litehtml::el_table::~el_table()
{

}

int litehtml::el_table::render( uint_ptr hdc, int x, int y, int max_width )
{
	int parent_width = max_width;

	m_pos.move_to(x, y);

	m_pos.x	+= content_margins_left();
	m_pos.y += content_margins_top();

	int ret_width = 0;

	int block_width = m_css_width.calc_percent(parent_width);

	if(block_width)
	{
		ret_width = max_width = block_width;
	} else
	{
		if(max_width)
		{
			max_width -= content_margins_left() + content_margins_right();
		}
	}

	css_length css_border_spacing_x;
	css_length css_border_spacing_y;

	css_border_spacing_x.fromString(get_style_property(L"-litehtml-border-spacing-x", true, L"0px"));
	css_border_spacing_y.fromString(get_style_property(L"-litehtml-border-spacing-y", true, L"0px"));

	int fntsz = get_font_size();
	int border_spacing_x = m_doc->cvt_units(css_border_spacing_x, fntsz);
	int border_spacing_y = m_doc->cvt_units(css_border_spacing_y, fntsz);

	table_grid grid;

	elements_iterator row_iter(this, &go_inside_table(), &table_rows_selector());

	element* row = row_iter.next(false);
	while(row)
	{
		grid.begin_row();

		elements_iterator cell_iter(row, &go_inside_table(), &table_cells_selector());
		element* cell = cell_iter.next();
		while(cell)
		{
			grid.add_cell(cell);

			cell = cell_iter.next(false);
		}
		row = row_iter.next(false);
	}
	grid.finish();

	// Calculate the minimum content width (MCW) of each cell: the formatted content may span any number of lines but may not overflow the cell box. 
	// If the specified 'width' (W) of the cell is greater than MCW, W is the minimum cell width. A value of 'auto' means that MCW is the minimum 
	// cell width.
	// 
	// Also, calculate the "maximum" cell width of each cell: formatting the content without breaking lines other than where explicit line breaks occur.

	for(int row = 0; row < grid.rows_count(); row++)
	{
		for(int col = 0; col < grid.cols_count(); col++)
		{
			table_cell* cell = grid.cell(col, row);
			if(cell && cell->el)
			{
				// calculate minimum content width
				cell->min_width = cell->el->render(hdc, 0, 0, 1);
				// calculate maximum content width
				cell->max_width = cell->el->render(hdc, 0, 0, max_width);
			}
		}
	}

	// For each column, determine a maximum and minimum column width from the cells that span only that column. 
	// The minimum is that required by the cell with the largest minimum cell width (or the column 'width', whichever is larger). 
	// The maximum is that required by the cell with the largest maximum cell width (or the column 'width', whichever is larger).

	for(int col = 0; col < grid.cols_count(); col++)
	{
		grid.column(col).max_width = 0;
		grid.column(col).min_width = 0;
		for(int row = 0; row < grid.rows_count(); row++)
		{
			if(grid.cell(col, row)->colspan <= 1)
			{
				grid.column(col).max_width = max(grid.column(col).max_width, grid.cell(col, row)->max_width);
				grid.column(col).min_width = max(grid.column(col).min_width, grid.cell(col, row)->min_width);
			}
		}
	}

	// For each cell that spans more than one column, increase the minimum widths of the columns it spans so that together, 
	// they are at least as wide as the cell. Do the same for the maximum widths. 
	// If possible, widen all spanned columns by approximately the same amount.

	for(int col = 0; col < grid.cols_count(); col++)
	{
		for(int row = 0; row < grid.rows_count(); row++)
		{
			if(grid.cell(col, row)->colspan > 1)
			{
				int max_total_width = grid.column(col).max_width;
				int min_total_width = grid.column(col).min_width;
				for(int col2 = col + 1; col2 < col + grid.cell(col, row)->colspan; col2++)
				{
					max_total_width += grid.column(col2).max_width;
					min_total_width += grid.column(col2).min_width;
				}
				if(min_total_width < grid.cell(col, row)->min_width)
				{
					grid.distribute_min_width(grid.cell(col, row)->min_width - min_total_width, col, col + grid.cell(col, row)->colspan - 1);
				}
				if(max_total_width < grid.cell(col, row)->max_width)
				{
					grid.distribute_max_width(grid.cell(col, row)->max_width - max_total_width, col, col + grid.cell(col, row)->colspan - 1);
				}
			}
		}
	}

	// If the 'table' or 'inline-table' element's 'width' property has a computed value (W) other than 'auto', the used width is the 
	// greater of W, CAPMIN, and the minimum width required by all the columns plus cell spacing or borders (MIN). 
	// If the used width is greater than MIN, the extra width should be distributed over the columns.
	//
	// If the 'table' or 'inline-table' element has 'width: auto', the used width is the greater of the table's containing block width, 
	// CAPMIN, and MIN. However, if either CAPMIN or the maximum width required by the columns plus cell spacing or borders (MAX) is 
	// less than that of the containing block, use max(MAX, CAPMIN).

	int min_table_width = border_spacing_x * (grid.cols_count() + 1); // MIN
	int max_table_width = border_spacing_x * (grid.cols_count() + 1); // MAX

	for(int col = 0; col < grid.cols_count(); col++)
	{
		min_table_width += grid.column(col).min_width;
		max_table_width += grid.column(col).max_width;
	}

	int table_width = 0;

	if(!m_css_width.is_predefined())
	{
		table_width = max(min_table_width, max_width);
		if(table_width > min_table_width)
		{
			for(int col2 = 0; col2 < grid.cols_count(); col2++)
			{
				grid.column(col2).width	= grid.column(col2).min_width;
			}
			grid.distribute_width(table_width - min_table_width, 0, grid.cols_count() - 1/*, &table_column_accessor_width()*/);
		} else
		{
			for(int col2 = 0; col2 < grid.cols_count(); col2++)
			{
				grid.column(col2).width	= grid.column(col2).min_width;
			}
		}
	} else
	{
		if(max_table_width < max_width)
		{
			table_width = max_table_width;
			for(int col2 = 0; col2 < grid.cols_count(); col2++)
			{
				grid.column(col2).width	= grid.column(col2).max_width;
			}
		} else if(min_table_width >= max_width)
		{
			table_width = min_table_width;
			for(int col2 = 0; col2 < grid.cols_count(); col2++)
			{
				grid.column(col2).width	= grid.column(col2).min_width;
			}
		} else
		{
			table_width = max_width;
			for(int col2 = 0; col2 < grid.cols_count(); col2++)
			{
				grid.column(col2).width	= grid.column(col2).min_width;
			}
			grid.distribute_width(table_width - min_table_width, 0, grid.cols_count() - 1/*, &table_column_accessor_width()*/);
		}
	}

	// now we have the columns widths and the table width

	table_width = grid.set_table_width(table_width, border_spacing_x);

	// render cells with computed width
	int top		= border_spacing_y;
	bool row_span_found = false;

	for(int row = 0; row < grid.rows_count(); row++)
	{
		int left	= border_spacing_x;
		int max_height = 0;
		for(int col = 0; col < grid.cols_count(); col++)
		{
			table_cell* cell = grid.cell(col, row);
			int cell_width = (cell->colspan - 1) * border_spacing_x;
			if(cell->el)
			{
				for (int col2 = col; col2 < col + cell->colspan; col2++)
				{
					cell_width += grid.column(col2).width;
				}
				cell->el->render(hdc, left, top, cell_width);
				cell->el->m_pos.width = cell_width - cell->el->content_margins_left() - cell->el->content_margins_right();
				if(cell->rowspan <= 1)
				{
					max_height = max(max_height, cell->el->height());
				} else
				{
					row_span_found = true;
				}
			} else
			{
				cell_width += grid.column(col).width;
			}
			left += grid.column(col).width + border_spacing_x;
		}
		grid.row(row).height = max_height;
		
		for(int col = 0; col < grid.cols_count(); col++)
		{
			table_cell* cell = grid.cell(col, row);
			if(cell->el && cell->rowspan <= 1)
			{
				cell->el->m_pos.height = max_height - cell->el->content_margins_top() - cell->el->content_margins_bottom();
			}
		}
		top += max_height + border_spacing_y;
	}

	if(row_span_found)
	{
		bool row_height_changed = false;

		for(int row = 0; row < grid.rows_count(); row++)
		{
			for(int col = 0; col < grid.cols_count(); col++)
			{
				table_cell* cell = grid.cell(col, row);
				if(cell->el)
				{
					if(cell->rowspan > 1)
					{
						int cell_height = (cell->rowspan - 1) * border_spacing_y;
						for (int row2 = row; row2 < row + cell->rowspan && row2 < grid.rows_count(); row2++)
						{
							cell_height += grid.row(row2).height;
						}
						if(cell_height >= cell->el->height())
						{
							cell->el->m_pos.height = cell_height - cell->el->content_margins_top() - cell->el->content_margins_bottom();
						} else
						{
							grid.row(min(row + cell->rowspan - 1, grid.rows_count() - 1)).height += cell->el->height() - cell_height;
							row_height_changed = true;
						}
					}
				}
			}
		}
		if(row_height_changed)
		{
			top		= border_spacing_y;
			for(int row = 0; row < grid.rows_count(); row++)
			{
				for(int col = 0; col < grid.cols_count(); col++)
				{
					table_cell* cell = grid.cell(col, row);
					if(cell->el)
					{
						int cell_height = (cell->rowspan - 1) * border_spacing_y;
						for (int row2 = row; row2 < row + cell->rowspan && row2 < grid.rows_count(); row2++)
						{
							cell_height += grid.row(row2).height;
						}
						cell->el->m_pos.height  = cell_height - cell->el->content_margins_top() - cell->el->content_margins_bottom();
						cell->el->m_pos.y		= top + cell->el->content_margins_top();
					}
				}
				top += grid.row(row).height + border_spacing_y;
			}
		}
	}

	m_pos.width		= table_width;
	m_pos.height	= top;

	return table_width;
}

bool litehtml::el_table::appendChild( litehtml::element* el )
{
	if(!el)	return false;
	if(el->m_tag == L"tbody" || el->m_tag == L"thead" || el->m_tag == L"tfoot")
	{
		return element::appendChild(el);
	}
	return false;
}

void litehtml::el_table::parse_styles(bool is_reparse)
{
	const wchar_t* str = get_attr(L"width");
	if(str)
	{
		m_style.add_property(L"width", str, 0);
	}

	str = get_attr(L"align");
	if(str)
	{
		int align = value_index(str, L"left;center;right");
		switch(align)
		{
		case 1:
			m_style.add_property(L"margin-left", L"auto", 0);
			m_style.add_property(L"margin-right", L"auto", 0);
			break;
		case 2:
			m_style.add_property(L"margin-left", L"auto", 0);
			m_style.add_property(L"margin-right", L"0", 0);
			break;
		}
		m_style.add_property(L"width", str, 0);
	}

	element::parse_styles(is_reparse);
}
