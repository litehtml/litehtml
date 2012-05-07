#include "html.h"
#include "table.h"
#include "element.h"

void litehtml::table_grid::add_cell( element* el )
{
	table_cell cell;
	cell.el = el;
	cell.colspan = _wtoi(el->get_attr(L"colspan", L"1"));
	cell.rowspan = _wtoi(el->get_attr(L"rowspan", L"1"));

	while( is_rowspanned( (int) m_rows.size() - 1, (int) m_rows.back().size() ) )
	{
		table_cell empty_cell;
		m_rows.back().push_back(empty_cell);
	}

	m_rows.back().push_back(cell);
	for(int i = 1; i < cell.colspan; i++)
	{
		table_cell empty_cell;
		m_rows.back().push_back(empty_cell);
	}
	while(m_rows.back().size() > m_cols_width.size())
	{
		m_cols_width.push_back(0);
	}
}


void litehtml::table_grid::begin_row()
{
	row r;
	m_rows.push_back(r);
	m_rows_height.push_back(0);
}


void litehtml::table_grid::end_row()
{
	if(m_rows.size() > 1)
	{
		for(int i = (int) m_rows.back().size(); i < (int) m_rows[m_rows.size() - 2].size(); i++)
		{
			table_cell empty_cell;
			m_rows.back().push_back(empty_cell);
		}
	}
}


bool litehtml::table_grid::is_rowspanned( int r, int c )
{
	for(int i = r - 1; i >= 0; i--)
	{
		if(m_rows[i][c].rowspan > 1)
		{
			if(m_rows[i][c].rowspan >= r - i + 1)
			{
				return true;
			}
		}
	}
	return false;
}

