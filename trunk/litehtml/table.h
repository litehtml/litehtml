#pragma once

namespace litehtml
{
	struct table_cell
	{
		element*		el;
		int				colspan;
		int				rowspan;

		table_cell()
		{
			colspan = 0;
			rowspan	= 0;
			el		= 0;
		}

		table_cell(const table_cell& val)
		{
			el		= val.el;
			colspan	= val.colspan;
			rowspan	= val.rowspan;
		}
	};

	class table_grid
	{
	public:
		typedef std::vector<table_cell>		row;
		typedef std::vector<row>			rows;

		rows		m_rows;
		int_vector	m_rows_height;
		int_vector	m_cols_width;

		void begin_row();
		void add_cell(element* el);
		void end_row();
		bool is_rowspanned(int r, int c);
	};
}