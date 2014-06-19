#include "html.h"
#include "el_table.h"
#include "document.h"
#include "iterators.h"
#include <algorithm>


litehtml::el_table::el_table( litehtml::document* doc ) : html_tag(doc)
{
	m_border_spacing_x	= 0;
	m_border_spacing_y	= 0;
	m_border_collapse	= border_collapse_separate;
}


litehtml::el_table::~el_table()
{

}

int litehtml::el_table::render( int x, int y, int max_width )
{
	int parent_width = max_width;

	// reset auto margins
	if(m_css_margins.left.is_predefined())
	{
		m_margins.left = 0;
	}
	if(m_css_margins.right.is_predefined())
	{
		m_margins.right = 0;
	}

	m_pos.clear();
	m_pos.move_to(x, y);

	m_pos.x	+= content_margins_left();
	m_pos.y += content_margins_top();

	int block_width = m_css_width.calc_percent(parent_width);

	if(block_width)
	{
		max_width = block_width -= content_margins_left() + content_margins_right();
	} else
	{
		if(max_width)
		{
			max_width -= content_margins_left() + content_margins_right();
		}
	}

	calc_outlines(parent_width);

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


	int table_width = 0;
	int table_width_spacing = 0;
	if(m_border_collapse == border_collapse_separate)
	{
		table_width_spacing = m_border_spacing_x * (m_grid.cols_count() + 1);
	} else
	{
		table_width_spacing = 0;

		if(m_grid.cols_count())
		{
			table_width_spacing -= std::min(border_left(), m_grid.column(0).border_left);
			table_width_spacing -= std::min(border_right(), m_grid.column(m_grid.cols_count() - 1).border_right);
		}

		for(int col = 1; col < m_grid.cols_count(); col++)
		{
			table_width_spacing -= std::min(m_grid.column(col).border_left, m_grid.column(col - 1).border_right);
		}
	}

	if(!m_css_width.is_predefined())
	{
		table_width = m_grid.calc_table_width(block_width - table_width_spacing, false);
	} else
	{
		table_width = m_grid.calc_table_width(max_width - table_width_spacing, true);
	}

	table_width += table_width_spacing;
	m_grid.calc_horizontal_positions(m_borders, m_border_collapse, m_border_spacing_x);
	
	bool row_span_found = false;

	// render cells with computed width
	for(int row = 0; row < m_grid.rows_count(); row++)
	{
		m_grid.row(row).height = 0;
		for(int col = 0; col < m_grid.cols_count(); col++)
		{
			table_cell* cell = m_grid.cell(col, row);
			if(cell->el)
			{
				int span_col = col + cell->colspan - 1;
				if(span_col >= m_grid.cols_count())
				{
					span_col = m_grid.cols_count() - 1;
				}
				int cell_width = m_grid.column(span_col).right - m_grid.column(col).left;
				
				cell->el->render(m_grid.column(col).left, 0, cell_width);
				cell->el->m_pos.width = cell_width - cell->el->content_margins_left() - cell->el->content_margins_right();

				if(cell->rowspan <= 1)
				{
					m_grid.row(row).height = std::max(m_grid.row(row).height, cell->el->height());
				} else
				{
					row_span_found = true;
				}

			}
		}
	}

	if(row_span_found)
	{
		for(int col = 0; col < m_grid.cols_count(); col++)
		{
			for(int row = 0; row < m_grid.rows_count(); row++)
			{
				table_cell* cell = m_grid.cell(col, row);
				if(cell->el)
				{
					int span_row = row + cell->rowspan - 1;
					if(span_row >= m_grid.rows_count())
					{
						span_row = m_grid.rows_count() - 1;
					}
					if(span_row != row)
					{
						int h = 0;
						for(int i = row; i <= span_row; i++)
						{
							h += m_grid.row(i).height;
						}
						if(h < cell->el->height())
						{
							m_grid.row(span_row).height += cell->el->height() - h;
						}
					}
				}
			}
		}
	}

	m_grid.calc_vertical_positions(m_borders, m_border_collapse, m_border_spacing_y);

	int table_height = 0;
	// place cells vertically
	for(int col = 0; col < m_grid.cols_count(); col++)
	{
		for(int row = 0; row < m_grid.rows_count(); row++)
		{
			table_cell* cell = m_grid.cell(col, row);
			if(cell->el)
			{
				int span_row = row + cell->rowspan - 1;
				if(span_row >= m_grid.rows_count())
				{
					span_row = m_grid.rows_count() - 1;
				}
				cell->el->m_pos.y		= m_grid.row(row).top + cell->el->content_margins_top();
				cell->el->m_pos.height	= m_grid.row(span_row).bottom - m_grid.row(row).top - cell->el->content_margins_top() - cell->el->content_margins_bottom();
				table_height = std::max(table_height, m_grid.row(span_row).bottom);
				cell->el->apply_vertical_align();
			}
		}
	}

	if(m_border_collapse == border_collapse_collapse)
	{
		if(m_grid.rows_count())
		{
			table_height -= std::min(border_bottom(), m_grid.row(m_grid.rows_count() - 1).border_bottom);
		}
	} else
	{
		table_height += m_border_spacing_y;
	}

	m_pos.width		= table_width;

	calc_outlines(parent_width);

	m_pos.move_to(x, y);
	m_pos.x			+= content_margins_left();
	m_pos.y			+= content_margins_top();
	m_pos.width		= table_width;
	m_pos.height	= table_height;

	return table_width;
}

bool litehtml::el_table::appendChild( litehtml::element* el )
{
	if(!el)	return false;
	if(!t_strcmp(el->get_tagName(), _t("tbody")) || !t_strcmp(el->get_tagName(), _t("thead")) || !t_strcmp(el->get_tagName(), _t("tfoot")))
	{
		return html_tag::appendChild(el);
	}
	return false;
}

void litehtml::el_table::parse_styles(bool is_reparse)
{
	html_tag::parse_styles(is_reparse);

	m_border_collapse = (border_collapse) value_index(get_style_property(_t("border-collapse"), true, _t("separate")), border_collapse_strings, border_collapse_separate);

	if(m_border_collapse == border_collapse_separate)
	{
		m_css_border_spacing_x.fromString(get_style_property(_t("-litehtml-border-spacing-x"), true, _t("0px")));
		m_css_border_spacing_y.fromString(get_style_property(_t("-litehtml-border-spacing-y"), true, _t("0px")));

		int fntsz = get_font_size();
		m_border_spacing_x = m_doc->cvt_units(m_css_border_spacing_x, fntsz);
		m_border_spacing_y = m_doc->cvt_units(m_css_border_spacing_y, fntsz);
	} else
	{
		m_border_spacing_x	= 0;
		m_border_spacing_y	= 0;
		m_padding.bottom	= 0;
		m_padding.top		= 0;
		m_padding.left		= 0;
		m_padding.right		= 0;
		m_css_padding.bottom.set_value(0, css_units_px);
		m_css_padding.top.set_value(0, css_units_px);
		m_css_padding.left.set_value(0, css_units_px);
		m_css_padding.right.set_value(0, css_units_px);
	}
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

void litehtml::el_table::draw_children( uint_ptr hdc, int x, int y, const position* clip, draw_flag flag, int zindex )
{
	position pos = m_pos;
	pos.x += x;
	pos.y += y;
	for(int row = 0; row < m_grid.rows_count(); row++)
	{
		if(flag == draw_block)
		{
			m_grid.row(row).el_row->draw_background(hdc, pos.x, pos.y, clip);
		}
		for(int col = 0; col < m_grid.cols_count(); col++)
		{
			table_cell* cell = m_grid.cell(col, row);
			if(cell->el)
			{
				if(flag == draw_block)
				{
					cell->el->draw(hdc, pos.x, pos.y, clip);
				}
				cell->el->draw_children(hdc, pos.x, pos.y, clip, flag, zindex);
			}
		}
	}
}

void litehtml::el_table::parse_attributes()
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
	}

	str = get_attr(_t("cellspacing"));
	if(str)
	{
		tstring val = str;
		val += _t(" ");
		val += str;
		m_style.add_property(_t("border-spacing"), val.c_str(), 0, false);
	}
	
	str = get_attr(_t("border"));
	if(str)
	{
		m_style.add_property(_t("border-width"), str, 0, false);
	}

	html_tag::parse_attributes();
}
