#pragma once

namespace litehtml
{
	struct table_row
	{
		typedef std::vector<table_row>	vector;

		int height;

		table_row()
		{
			height		= 0;
		}

		table_row(int h)
		{
			height	= h;
		}

		table_row(const table_row& val)
		{
			height	= val.height;
		}
	};

	struct table_column
	{
		typedef std::vector<table_column>	vector;
		
		int			min_width;
		int			max_width;
		int			width;
		css_length	css_width;

		table_column()
		{
			min_width	= 0;
			max_width	= 0;
			width		= 0;
			css_width.predef(0);
		}

		table_column(int min_w, int max_w)
		{
			max_width	= max_w;
			min_width	= min_w;
			width		= 0;
			css_width.predef(0);
		}

		table_column(const table_column& val)
		{
			max_width	= val.max_width;
			min_width	= val.min_width;
			width		= val.width;
			css_width	= val.css_width;
		}
	};

	class table_column_accessor
	{
	public:
		virtual int& get(table_column& col) = 0;
	};

	class table_column_accessor_max_width : public table_column_accessor
	{
	public:
		virtual int& get(table_column& col);
	};

	class table_column_accessor_min_width : public table_column_accessor
	{
	public:
		virtual int& get(table_column& col);
	};

	class table_column_accessor_width : public table_column_accessor
	{
	public:
		virtual int& get(table_column& col);
	};

	struct table_cell
	{
		element*		el;
		int				colspan;
		int				rowspan;
		int				min_width;
		int				min_height;
		int				max_width;
		int				max_height;
		int				width;
		int				height;

		table_cell()
		{
			min_width	= 0;
			min_height	= 0;
			max_width	= 0;
			max_height	= 0;
			width		= 0;
			height		= 0;
			colspan		= 1;
			rowspan		= 1;
			el			= 0;
		}

		table_cell(const table_cell& val)
		{
			el			= val.el;
			colspan		= val.colspan;
			rowspan		= val.rowspan;
			width		= val.width;
			height		= val.height;
			min_width	= val.min_width;
			min_height	= val.min_height;
			max_width	= val.max_width;
			max_height	= val.max_height;
		}
	};

	class table_grid
	{
	public:
		typedef std::vector< std::vector<table_cell> >	rows;
	private:
		int						m_rows_count;
		int						m_cols_count;
		rows					m_cells;
		table_column::vector	m_columns;
		table_row::vector		m_rows;
	public:

		table_grid()
		{
			m_rows_count	= 0;
			m_cols_count	= 0;
		}

		void			begin_row();
		void			add_cell(element* el);
		bool			is_rowspanned(int r, int c);
		void			finish();
		table_cell*		cell(int t_col, int t_row);
		table_column&	column(int c)	{ return m_columns[c];	}
		table_row&		row(int r)		{ return m_rows[r];		}

		int				rows_count()	{ return m_rows_count;	}
		int				cols_count()	{ return m_cols_count;	}

		void			distribute_max_width(int width, int start, int end);
		void			distribute_min_width(int width, int start, int end);
		void			distribute_width(int width, int start, int end);
		void			distribute_width(int width, int start, int end, table_column_accessor* acc);
		int				set_table_width(int width, int bs_x);
	};
}