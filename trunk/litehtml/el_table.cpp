#include "html.h"
#include "el_table.h"
#include "document.h"
#include "iterators.h"
#include <algorithm>


litehtml::el_table::el_table( litehtml::document* doc ) : html_tag(doc)
{

}


litehtml::el_table::~el_table()
{

}

int litehtml::el_table::render( int x, int y, int max_width )
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


	// Calculate the minimum content width (MCW) of each cell: the formatted content may span any number of lines but may not overflow the cell box. 
	// If the specified 'width' (W) of the cell is greater than MCW, W is the minimum cell width. A value of 'auto' means that MCW is the minimum 
	// cell width.
	// 
	// Also, calculate the "maximum" cell width of each cell: formatting the content without breaking lines other than where explicit line breaks occur.

	for(int row = 0; row < m_grid.rows_count(); row++)
	{
		for(int col = 0; col < m_grid.cols_count(); col++)
		{
			table_cell* cell = m_grid.cell(col, row);
			if(cell && cell->el)
			{
				// calculate minimum content width
				cell->min_width = cell->el->render(0, 0, 1);
				// calculate maximum content width
				cell->max_width = cell->el->render(0, 0, max_width);
			}
		}
	}

	// For each column, determine a maximum and minimum column width from the cells that span only that column. 
	// The minimum is that required by the cell with the largest minimum cell width (or the column 'width', whichever is larger). 
	// The maximum is that required by the cell with the largest maximum cell width (or the column 'width', whichever is larger).

	for(int col = 0; col < m_grid.cols_count(); col++)
	{
		m_grid.column(col).max_width = 0;
		m_grid.column(col).min_width = 0;
		for(int row = 0; row < m_grid.rows_count(); row++)
		{
			if(m_grid.cell(col, row)->colspan <= 1)
			{
				m_grid.column(col).max_width = std::max(m_grid.column(col).max_width, m_grid.cell(col, row)->max_width);
				m_grid.column(col).min_width = std::max(m_grid.column(col).min_width, m_grid.cell(col, row)->min_width);
			}
		}
	}

	// For each cell that spans more than one column, increase the minimum widths of the columns it spans so that together, 
	// they are at least as wide as the cell. Do the same for the maximum widths. 
	// If possible, widen all spanned columns by approximately the same amount.

	for(int col = 0; col < m_grid.cols_count(); col++)
	{
		for(int row = 0; row < m_grid.rows_count(); row++)
		{
			if(m_grid.cell(col, row)->colspan > 1)
			{
				int max_total_width = m_grid.column(col).max_width;
				int min_total_width = m_grid.column(col).min_width;
				for(int col2 = col + 1; col2 < col + m_grid.cell(col, row)->colspan; col2++)
				{
					max_total_width += m_grid.column(col2).max_width;
					min_total_width += m_grid.column(col2).min_width;
				}
				if(min_total_width < m_grid.cell(col, row)->min_width)
				{
					m_grid.distribute_min_width(m_grid.cell(col, row)->min_width - min_total_width, col, col + m_grid.cell(col, row)->colspan - 1);
				}
				if(max_total_width < m_grid.cell(col, row)->max_width)
				{
					m_grid.distribute_max_width(m_grid.cell(col, row)->max_width - max_total_width, col, col + m_grid.cell(col, row)->colspan - 1);
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

	int min_table_width = m_border_spacing_x * (m_grid.cols_count() + 1); // MIN
	int max_table_width = m_border_spacing_x * (m_grid.cols_count() + 1); // MAX

	for(int col = 0; col < m_grid.cols_count(); col++)
	{
		min_table_width += m_grid.column(col).min_width;
		max_table_width += m_grid.column(col).max_width;
	}

	int table_width = 0;

	if(!m_css_width.is_predefined())
	{
		table_width = std::max(min_table_width, max_width);
		if(table_width > min_table_width)
		{
			for(int col2 = 0; col2 < m_grid.cols_count(); col2++)
			{
				m_grid.column(col2).width	= m_grid.column(col2).min_width;
			}
			m_grid.distribute_width(table_width - min_table_width, 0, m_grid.cols_count() - 1/*, &table_column_accessor_width()*/);
		} else
		{
			for(int col2 = 0; col2 < m_grid.cols_count(); col2++)
			{
				m_grid.column(col2).width	= m_grid.column(col2).min_width;
			}
		}
	} else
	{
		if(max_table_width < max_width)
		{
			table_width = max_table_width;
			for(int col2 = 0; col2 < m_grid.cols_count(); col2++)
			{
				m_grid.column(col2).width	= m_grid.column(col2).max_width;
			}
		} else if(min_table_width >= max_width)
		{
			table_width = min_table_width;
			for(int col2 = 0; col2 < m_grid.cols_count(); col2++)
			{
				m_grid.column(col2).width	= m_grid.column(col2).min_width;
			}
		} else
		{
			table_width = max_width;
			for(int col2 = 0; col2 < m_grid.cols_count(); col2++)
			{
				m_grid.column(col2).width	= m_grid.column(col2).min_width;
			}
			m_grid.distribute_width(table_width - min_table_width, 0, m_grid.cols_count() - 1/*, &table_column_accessor_width()*/);
		}
	}

	// now we have the columns widths and the table width

	table_width = m_grid.set_table_width(table_width, m_border_spacing_x);

	// render cells with computed width
	int top		= m_border_spacing_y;
	bool row_span_found = false;

	for(int row = 0; row < m_grid.rows_count(); row++)
	{
		int left	= m_border_spacing_x;
		int max_height = 0;
		for(int col = 0; col < m_grid.cols_count(); col++)
		{
			table_cell* cell = m_grid.cell(col, row);
			int cell_width = (cell->colspan - 1) * m_border_spacing_x;
			if(cell->el)
			{
				for (int col2 = col; col2 < col + cell->colspan; col2++)
				{
					cell_width += m_grid.column(col2).width;
				}
				cell->el->render(left, top, cell_width);
				cell->el->m_pos.width = cell_width - cell->el->content_margins_left() - cell->el->content_margins_right();
				if(cell->rowspan <= 1)
				{
					max_height = std::max(max_height, cell->el->height());
				} else
				{
					row_span_found = true;
				}
			} else
			{
				cell_width += m_grid.column(col).width;
			}
			left += m_grid.column(col).width + m_border_spacing_x;
		}
		m_grid.row(row).height = max_height;
		
		for(int col = 0; col < m_grid.cols_count(); col++)
		{
			table_cell* cell = m_grid.cell(col, row);
			if(cell->el && cell->rowspan <= 1)
			{
				cell->el->m_pos.height = max_height - cell->el->content_margins_top() - cell->el->content_margins_bottom();
			}
		}
		top += max_height + m_border_spacing_y;
	}

	if(row_span_found)
	{
		bool row_height_changed = false;

		for(int row = 0; row < m_grid.rows_count(); row++)
		{
			for(int col = 0; col < m_grid.cols_count(); col++)
			{
				table_cell* cell = m_grid.cell(col, row);
				if(cell->el)
				{
					if(cell->rowspan > 1)
					{
						int cell_height = (cell->rowspan - 1) * m_border_spacing_y;
						for (int row2 = row; row2 < row + cell->rowspan && row2 < m_grid.rows_count(); row2++)
						{
							cell_height += m_grid.row(row2).height;
						}
						if(cell_height >= cell->el->height())
						{
							cell->el->m_pos.height = cell_height - cell->el->content_margins_top() - cell->el->content_margins_bottom();
						} else
						{
							m_grid.row(std::min(row + cell->rowspan - 1, m_grid.rows_count() - 1)).height += cell->el->height() - cell_height;
							row_height_changed = true;
						}
					}
				}
			}
		}
		if(row_height_changed)
		{
			top		= m_border_spacing_y;
			for(int row = 0; row < m_grid.rows_count(); row++)
			{
				for(int col = 0; col < m_grid.cols_count(); col++)
				{
					table_cell* cell = m_grid.cell(col, row);
					if(cell->el)
					{
						int cell_height = (cell->rowspan - 1) * m_border_spacing_y;
						for (int row2 = row; row2 < row + cell->rowspan && row2 < m_grid.rows_count(); row2++)
						{
							cell_height += m_grid.row(row2).height;
						}
						cell->el->m_pos.height  = cell_height - cell->el->content_margins_top() - cell->el->content_margins_bottom();
						cell->el->m_pos.y		= top + cell->el->content_margins_top();
					}
				}
				top += m_grid.row(row).height + m_border_spacing_y;
			}
		}
	}

	// process the vertical align of the cells

	for(int row = 0; row < m_grid.rows_count(); row++)
	{
		for(int col = 0; col < m_grid.cols_count(); col++)
		{
			table_cell* cell = m_grid.cell(col, row);
			if(cell->el)
			{
				cell->el->apply_vertical_align();
			}
		}
	}

	m_pos.width		= table_width;
	m_pos.height	= top;

	return table_width;//min_table_width;
}

bool litehtml::el_table::appendChild( litehtml::element* el )
{
	if(!el)	return false;
	if(!t_strcasecmp(el->get_tagName(), _t("tbody")) || !t_strcasecmp(el->get_tagName(), _t("thead")) || !t_strcasecmp(el->get_tagName(), _t("tfoot")))
	{
		return html_tag::appendChild(el);
	}
	return false;
}

void litehtml::el_table::parse_styles(bool is_reparse)
{
	html_tag::parse_styles(is_reparse);

	m_css_border_spacing_x.fromString(get_style_property(_t("-litehtml-border-spacing-x"), true, _t("0px")));
	m_css_border_spacing_y.fromString(get_style_property(_t("-litehtml-border-spacing-y"), true, _t("0px")));

	int fntsz = get_font_size();
	m_border_spacing_x = m_doc->cvt_units(m_css_border_spacing_x, fntsz);
	m_border_spacing_y = m_doc->cvt_units(m_css_border_spacing_y, fntsz);

}

void litehtml::el_table::init()
{
	m_grid.clear();

	go_inside_table 		table_selector;
	table_rows_selector		row_selector;
	table_cells_selector	cell_selector;

	elements_iterator row_iter(this, &table_selector, &row_selector);

	element* row = row_iter.next(false);
	while(row)
	{
		m_grid.begin_row(row);

		elements_iterator cell_iter(row, &table_selector, &cell_selector);
		element* cell = cell_iter.next();
		while(cell)
		{
			m_grid.add_cell(cell);

			cell = cell_iter.next(false);
		}
		row = row_iter.next(false);
	}

	m_grid.finish();
}

void litehtml::el_table::draw( uint_ptr hdc, int x, int y, const position* clip )
{
	position pos = m_pos;
	pos.x	+= x;
	pos.y	+= y;

	draw_background(hdc, x, y, clip);

	for(int row = 0; row < m_grid.rows_count(); row++)
	{
		m_grid.row(row).el_row->draw_background(hdc, pos.left(), pos.top(), clip);
		for(int col = 0; col < m_grid.cols_count(); col++)
		{
			table_cell* cell = m_grid.cell(col, row);
			if(cell->el)
			{
				cell->el->draw(hdc, pos.left(), pos.top(), clip);
			}
		}
	}
}

void litehtml::el_table::finish()
{
	const tchar_t* str = get_attr(_t("width"));
	if(str)
	{
		m_style.add_property(_t("width"), str, 0, false);
	}

	str = get_attr(_t("align"));
	if(str)
	{
		int align = value_index(str, _t("left;center;right"));
		switch(align)
		{
		case 1:
			m_style.add_property(_t("margin-left"), _t("auto"), 0, false);
			m_style.add_property(_t("margin-right"), _t("auto"), 0, false);
			break;
		case 2:
			m_style.add_property(_t("margin-left"), _t("auto"), 0, false);
			m_style.add_property(_t("margin-right"), _t("0"), 0, false);
			break;
		}
		m_style.add_property(_t("width"), str, 0, false);
	}

	str = get_attr(_t("cellspacing"));
	if(str)
	{
		tstring val = str;
		val += _t(" ");
		val += str;
		m_style.add_property(_t("border-spacing"), val.c_str(), 0, false);
	}
	html_tag::finish();
}
