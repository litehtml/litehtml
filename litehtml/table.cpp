#include "html.h"
#include "table.h"
#include "element.h"

void litehtml::table_grid::add_cell( element* el )
{
	table_cell cell;
	cell.el = el;
	cell.colspan = _wtoi(el->get_attr(L"colspan", L"1"));
	cell.rowspan = _wtoi(el->get_attr(L"rowspan", L"1"));

	while( is_rowspanned( (int) m_cells.size() - 1, (int) m_cells.back().size() ) )
	{
		m_cells.back().push_back(table_cell());
	}

	m_cells.back().push_back(cell);
	for(int i = 1; i < cell.colspan; i++)
	{
		table_cell empty_cell;
		m_cells.back().push_back(empty_cell);
	}
}


void litehtml::table_grid::begin_row()
{
	std::vector<table_cell> r;
	m_cells.push_back(r);
}


bool litehtml::table_grid::is_rowspanned( int r, int c )
{
	for(int row = r - 1; row >= 0; row--)
	{
		if(c < (int) m_cells[row].size())
		{
			if(m_cells[row][c].rowspan > 1)
			{
				if(m_cells[row][c].rowspan >= r - row + 1)
				{
					return true;
				}
			}
		}
	}
	return false;
}

void litehtml::table_grid::finish()
{
	m_rows_count	= (int) m_cells.size();
	m_cols_count	= 0;
	for(int i = 0; i < (int) m_cells.size(); i++)
	{
		m_cols_count = max(m_cols_count, (int) m_cells[i].size());
	}
	for(int i = 0; i < (int) m_cells.size(); i++)
	{
		for(int j = (int) m_cells[i].size(); j < m_cols_count; j++)
		{
			table_cell empty_cell;
			m_cells[i].push_back(empty_cell);
		}
	}

	m_columns.clear();
	for(int i = 0; i < m_cols_count; i++)
	{
		m_columns.push_back(table_column(0, 0));
	}

	m_rows.clear();
	for(int i = 0; i < m_rows_count; i++)
	{
		m_rows.push_back(table_row());
	}

	for(int col = 0; col < m_cols_count; col++)
	{
		for(int row = 0; row < m_rows_count; row++)
		{
			if(cell(col, row)->el)
			{
				if(!cell(col, row)->el->m_css_width.is_predefined())
				{
					m_columns[col].css_width = cell(col, row)->el->m_css_width;
					break;
				}
			}
		}
	}

	for(int col = 0; col < m_cols_count; col++)
	{
		for(int row = 0; row < m_rows_count; row++)
		{
			if(cell(col, row)->el)
			{
				cell(col, row)->el->m_css_width = m_columns[col].css_width;
			}
		}
	}

/*
	// find css width
	for(int c = 0; c < m_cols_count; c++)
	{
		table_cell* cell0 = table_grid::cell(c, 0);
		for(int r = 1; r < m_rows_count; r++)
		{
			table_cell* cell = table_grid::cell(c, r);
			if(cell && cell->el)
			{
				if(!cell->el->m_css_width.predef())
				{
					cell0->css_width = cell->el->m_css_width;
					break;
				}
			}
		}
		if(!cell0->css_width.is_predefined())
		{
			for(int r = 1; r < m_rows_count; r++)
			{
				table_cell* cell = table_grid::cell(c, r);
				if(cell)
				{
					cell->css_width = cell0->css_width;
				}
			}
		}
	}


	// find css height
	for(int r = 0; r < m_rows_count; r++)
	{
		table_cell* cell0 = table_grid::cell(0, r);
		for(int c = 1; c < m_cols_count; c++)
		{
			table_cell* cell = table_grid::cell(c, r);
			if(cell && cell->el)
			{
				if(!cell->el->m_css_height.predef())
				{
					cell0->css_height = cell->el->m_css_height;
					break;
				}
			}
		}
		if(!cell0->css_height.is_predefined())
		{
			for(int c = 1; c < m_cols_count; c++)
			{
				table_cell* cell = table_grid::cell(c, r);
				if(cell)
				{
					cell->css_height = cell0->css_height;
				}
			}
		}
	}
*/
}

litehtml::table_cell* litehtml::table_grid::cell( int t_col, int t_row )
{
	if(t_col >= 0 && t_col < m_cols_count && t_row >= 0 && t_row < m_rows_count)
	{
		return &m_cells[t_row][t_col];
	}
	return 0;
}

void litehtml::table_grid::distribute_max_width( int width, int start, int end )
{
	distribute_width(width, start, end, &table_column_accessor_max_width());
}

void litehtml::table_grid::distribute_min_width( int width, int start, int end )
{
	distribute_width(width, start, end, &table_column_accessor_min_width());
}

void litehtml::table_grid::distribute_width( int width, int start, int end, table_column_accessor* acc )
{
	if(!(start >= 0 && start < m_cols_count && end >= 0 && end < m_cols_count))
	{
		return;
	}

	int cols_width = 0;
	for(int col = start; col <= end; col++)
	{
		cols_width		+= m_columns[col].max_width;
	}

	int add = width / (end - start + 1);
	int added_width = 0;
	for(int col = start; col <= end; col++)
	{
		if(cols_width)
		{
			add = round_f( (float) width * ((float) m_columns[col].max_width / (float) cols_width) );
		}
		added_width += add;
		acc->get(m_columns[col]) += add;
	}
	if(added_width < width)
	{
		acc->get(m_columns[start]) += width - added_width;
	}
}

void litehtml::table_grid::distribute_width( int width, int start, int end )
{
	if(!(start >= 0 && start < m_cols_count && end >= 0 && end < m_cols_count))
	{
		return;
	}

	std::vector<table_column*> distribute_columns;

	for(int step = 0; step < 3; step++)
	{
		distribute_columns.clear();

		switch(step)
		{
		case 0:
			{
				// distribute between the columns with width == auto
				for(int col = start; col <= end; col++)
				{
					if(m_columns[col].css_width.is_predefined())
					{
						distribute_columns.push_back(&m_columns[col]);
					}
				}
			}
			break;
		case 1:
			{
				// distribute between the columns with percents
				for(int col = start; col <= end; col++)
				{
					if(!m_columns[col].css_width.is_predefined() && m_columns[col].css_width.units() == css_units_percentage)
					{
						distribute_columns.push_back(&m_columns[col]);
					}
				}
			}
			break;
		case 2:
			{
				//well distribute between all columns
				for(int col = start; col <= end; col++)
				{
					distribute_columns.push_back(&m_columns[col]);
				}
			}
			break;
		}

		int added_width = 0;

		if(!distribute_columns.empty() || step == 2)
		{
			int cols_width = 0;
			for(std::vector<table_column*>::iterator col = distribute_columns.begin(); col != distribute_columns.end(); col++)
			{
				cols_width += (*col)->max_width - (*col)->min_width;
			}

			if(cols_width)
			{
				int add = width / (int) distribute_columns.size();
				for(std::vector<table_column*>::iterator col = distribute_columns.begin(); col != distribute_columns.end(); col++)
				{
					add = round_f( (float) width * ((float) ((*col)->max_width - (*col)->min_width) / (float) cols_width) );
					if((*col)->width + add >= (*col)->min_width)
					{
						(*col)->width	+= add;
						added_width		+= add;
					} else
					{
						added_width	+= ((*col)->width - (*col)->min_width) * (add / abs(add));
						(*col)->width = (*col)->min_width;
					}
				}
				if(added_width < width && step)
				{
					distribute_columns.front()->width += width - added_width;
					added_width = width;
				}
			} else
			{
				distribute_columns.back()->width += width;
				added_width = width;
			}
		}

		if(added_width == width)
		{
			break;
		} else
		{
			width -= added_width;
		}
	}


/*
	int cols_width = 0;
	int cols_width2 = 0;
	int first_predef_width = -1;
	for(int col = start; col <= end; col++)
	{
		cols_width2 += m_columns[col].max_width;
		if(m_columns[col].css_width.is_predefined() || !m_columns[col].css_width.is_predefined() && m_columns[col].css_width.units() == css_units_percentage)
		{
			if(first_predef_width < 0)
			{
				first_predef_width	= col;
			}
			cols_width	+= m_columns[col].max_width;
		}
	}

	if(first_predef_width < 0)
	{
		cols_width = cols_width2;
	}


	int add = width / (end - start + 1);
	int added_width = 0;
	for(int col = start; col <= end; col++)
	{
		if((m_columns[col].css_width.is_predefined() || !m_columns[col].css_width.is_predefined() && m_columns[col].css_width.units() == css_units_percentage) || first_predef_width < 0)
		{
			if(cols_width)
			{
				add = round_f( (float) width * ((float) m_columns[col].max_width / (float) cols_width) );
			}
			added_width				+= add;
			if(m_columns[col].width + add >= m_columns[col].min_width)
			{
				m_columns[col].width += add;
			} else
			{
				m_columns[col].width = m_columns[col].min_width;
			}
		}
	}
	if(added_width < width)
	{
		if(first_predef_width >= 0)
		{
			m_columns[first_predef_width].width += width - added_width;
		} else
		{
			m_columns[start].width += width - added_width;
		}
	}
*/
}

int litehtml::table_grid::set_table_width( int new_width, int bs_x )
{
	int table_width = bs_x * (m_cols_count + 1);

	for(int col = 0; col < m_cols_count; col++)
	{
		if(!m_columns[col].css_width.is_predefined())
		{
			m_columns[col].width = m_columns[col].css_width.calc_percent(new_width - bs_x * (m_cols_count + 1));
			m_columns[col].width = max(m_columns[col].width, m_columns[col].min_width);
		}
		table_width += m_columns[col].width;
	}

	if(new_width != table_width)
	{
		int width = new_width - table_width;
		distribute_width(new_width - table_width, 0, m_cols_count - 1);
	}

	table_width = bs_x * (m_cols_count + 1);

	for(int col = 0; col < m_cols_count; col++)
	{
		table_width += m_columns[col].width;
	}

	return table_width;
}

//////////////////////////////////////////////////////////////////////////

int& litehtml::table_column_accessor_max_width::get( table_column& col )
{
	return col.max_width;
}

int& litehtml::table_column_accessor_min_width::get( table_column& col )
{
	return col.min_width;
}

int& litehtml::table_column_accessor_width::get( table_column& col )
{
	return col.width;
}
