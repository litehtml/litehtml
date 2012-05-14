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

	element* row = row_iter.next();
	while(row)
	{
		grid.begin_row();

		elements_iterator cell_iter(row, &go_inside_table(), &table_cells_selector());
		element* cell = cell_iter.next();
		while(cell)
		{
			grid.add_cell(cell);

			cell = cell_iter.next();
		}
		row = row_iter.next();
		grid.end_row();
	}

	// full width render
	int row_idx = 0;
	int col_idx = 0;
	for(table_grid::rows::iterator row = grid.m_rows.begin(); row != grid.m_rows.end(); row++, row_idx++)
	{
		col_idx = 0;
		for(table_grid::row::iterator cell = row->begin(); cell != row->end(); cell++, col_idx++)
		{
			if(cell->el)
			{
				int width;
				if(cell->el->m_css_width.units() != css_units_percentage || cell->el->m_css_width.is_predefined())
				{
					width = cell->el->render(hdc, 0, 0, max_width);
				} else
				{
					width = (int) cell->el->m_css_width.val() + cell->el->content_margins_left() + cell->el->content_margins_right();
				}
				if(cell->colspan == 1)
				{
					grid.m_cols_width[col_idx] = max(grid.m_cols_width[col_idx], width);
				}
			}
		}
	}

	// find the table width
	int table_width = 0;
	int columns_width = 0;
	for(int i=0; i < (int) grid.m_cols_width.size(); i++)
	{
		columns_width += grid.m_cols_width[i];
	}

	table_width = columns_width + border_spacing_x * ((int) grid.m_cols_width.size() + 1);
	if(block_width)
	{
		table_width = block_width;
		columns_width = table_width - border_spacing_x * ((int) grid.m_cols_width.size() + 1);
	} else if(table_width > max_width)
	{
		table_width = max_width;		
		columns_width = table_width - border_spacing_x * ((int) grid.m_cols_width.size() + 1);
	}

	std::vector<col_info> cols;
	col_info inf;
	for(int_vector::iterator col = grid.m_cols_width.begin(); col != grid.m_cols_width.end(); col++)
	{
		inf.width		= 0;
		inf.is_auto	= true;
		cols.push_back(inf);
	}

	// calculate columns width
	row_idx = 0;
	col_idx = 0;
	for(table_grid::rows::iterator row = grid.m_rows.begin(); row != grid.m_rows.end(); row++, row_idx++)
	{
		col_idx = 0;
		for(table_grid::row::iterator cell = row->begin(); cell != row->end(); cell++, col_idx++)
		{
			if(cell->el)
			{
				if(cell->colspan == 1)
				{
					int width = 0;
					if(!cell->el->m_css_width.is_predefined())
					{
						if(cell->el->m_css_width.units() == css_units_percentage)
						{
							width = cell->el->m_css_width.calc_percent(columns_width);
						} else
						{
							width = grid.m_cols_width[col_idx];
						}
						cols[col_idx].is_auto = false;
					} else if(cols[col_idx].is_auto)
					{
						width = columns_width / (int) cols.size();
						cols[col_idx].is_auto = true;
					}
					cols[col_idx].width = max(cols[col_idx].width, width);
				}
			}
		}
	}

	int auto_count = 0;
	int auto_width = 0;
	int fixed_width = 0;
	col_idx = 0;
	for(std::vector<col_info>::iterator col = cols.begin(); col != cols.end(); col++, col_idx++)
	{
		if(col->is_auto)
		{
			auto_count++;
			auto_width += grid.m_cols_width[col_idx];
		} else
		{
			fixed_width += col->width;
		}
	}

	col_idx = 0;
	for(std::vector<col_info>::iterator col = cols.begin(); col != cols.end(); col++, col_idx++)
	{
		if(col->is_auto)
		{
			if(auto_width)
			{
				col->width = (int) ((double) (columns_width - fixed_width) * (double) grid.m_cols_width[col_idx] / (double) auto_width);
			} else
			{
				col->width = 0;
			}
		}
	}


	int top		= border_spacing_y;
	int left	= border_spacing_x;
	// render cells with computed width

	bool rerender = true;
	while(rerender)
	{
		top			= border_spacing_y;
		row_idx		= 0;
		rerender	= false;
		for(table_grid::rows::iterator row = grid.m_rows.begin(); row != grid.m_rows.end() && !rerender; row++, row_idx++)
		{
			col_idx = 0;
			left = border_spacing_x;
			for(table_grid::row::iterator cell = row->begin(); cell != row->end() && !rerender; cell++, col_idx++)
			{
				if(cell->el || cell == row->begin())
				{
					int w = cols[col_idx].width;
					for(int i = 1; i < cell->colspan && i + col_idx < (int) cols.size(); i++)
					{
						w += cols[col_idx + i].width + border_spacing_x;
					}
					if(cell->el)
					{
						int min_width = cell->el->render(hdc, left, top, w);
						if(min_width > w)
						{
							for(int i = 0; i < cell->colspan && i + col_idx < (int) cols.size(); i++)
							{
								if(cols[col_idx + i].is_auto || i == cell->colspan - 1)
								{
									cols[col_idx].width += min_width - w;
									break;
								}
							}

							rerender = true;
						}
						if(cell->rowspan == 1)
						{
							grid.m_rows_height[row_idx] = max(grid.m_rows_height[row_idx], cell->el->height());
						}
					}
				}
				left += cols[col_idx].width + border_spacing_x;
			}
			top += grid.m_rows_height[row_idx] + border_spacing_y;
		}
	}

	// find the final table width

	columns_width = 0;
	for(std::vector<col_info>::iterator col = cols.begin(); col != cols.end(); col++)
	{
		columns_width += col->width;
	}
	table_width = columns_width + border_spacing_x * ((int) grid.m_cols_width.size() + 1);

	// set the rows height

	bool need_second_pass = false;

	row_idx = 0;
	for(table_grid::rows::iterator row = grid.m_rows.begin(); row != grid.m_rows.end(); row++, row_idx++)
	{
		col_idx = 0;
		for(table_grid::row::iterator cell = row->begin(); cell != row->end(); cell++, col_idx++)
		{
			if(cell->el)
			{
				if(cell->rowspan == 1)
				{
					cell->el->m_pos.height = grid.m_rows_height[row_idx] - cell->el->content_margins_top() - cell->el->content_margins_bottom();
				} else
				{
					int h = 0;
					int last_row = row_idx;
					for(int i = 0; i + row_idx < (int) grid.m_rows_height.size() && i < cell->rowspan; i++)
					{
						h += grid.m_rows_height[row_idx + i];
						last_row = row_idx + i;
					}
					if(cell->el->m_pos.height > h)
					{
						need_second_pass = true;
						grid.m_rows_height[last_row] += cell->el->height() - h;
					} else
					{
						cell->el->m_pos.height = h - cell->el->content_margins_top() - cell->el->content_margins_bottom();
					}
				}
			}
		}
	}

	if(need_second_pass)
	{
		// re-pos cells cells with computed width
		row_idx = 0;
		top		= border_spacing_y;
		for(table_grid::rows::iterator row = grid.m_rows.begin(); row != grid.m_rows.end(); row++, row_idx++)
		{
			col_idx = 0;
			for(table_grid::row::iterator cell = row->begin(); cell != row->end(); cell++, col_idx++)
			{
				if(cell->el)
				{
					int h = 0;
					for(int i = 0; i + row_idx < (int) grid.m_rows_height.size() && i < cell->rowspan; i++)
					{
						h += grid.m_rows_height[row_idx + i];
					}
					cell->el->m_pos.height	= h - cell->el->content_margins_top() - cell->el->content_margins_bottom();
					cell->el->m_pos.y		= top + cell->el->content_margins_top();
				}
			}
			top += grid.m_rows_height[row_idx] + border_spacing_y;
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

void litehtml::el_table::parse_styles()
{
	const wchar_t* str = get_attr(L"width");
	if(str)
	{
		m_style.add_property(L"width", str, 0);
	}

	element::parse_styles();
}
