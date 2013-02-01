#pragma once

#include "types.h"

namespace litehtml
{
	class line : public object
	{
	public:
		typedef object_ptr<line>		ptr;
		typedef std::vector<line::ptr>	vector;

	private:
		elements_vector	m_items;
		int				m_height;
		int				m_top_margin;
		int				m_bottom_margin;
		bool			m_is_block;
		bool			m_is_break;
		int				m_left;
		int				m_top;
		int				m_padding_bottom;
		int				m_padding_top;
		element_clear	m_clear;
		int				m_min_height;

		int				m_line_left;
		int				m_line_right;
	public:
		line(void)
		{
			m_is_break			= false;
			m_line_left			= 0;
			m_line_right		= 0;
			m_min_height		= 0;
			m_padding_bottom	= 0;
			m_padding_top		= 0;
			m_height			= 0;
			m_top_margin		= 0;
			m_bottom_margin		= 0;
			m_is_block			= false;
			m_left				= 0;
			m_top				= 0;
			m_clear				= clear_none;
		}

		line(const line& val)
		{
			m_line_left			= val.m_line_left;
			m_line_right		= val.m_line_right;
			m_min_height		= val.m_min_height;
			m_items				= val.m_items;
			m_height			= val.m_height;
			m_top_margin		= val.m_top_margin;
			m_bottom_margin		= val.m_bottom_margin;
			m_is_block			= val.m_is_block;
			m_is_break			= val.m_is_break;
			m_left				= val.m_left;
			m_top				= val.m_top;
			m_padding_bottom	= val.m_padding_bottom;
			m_padding_top		= val.m_padding_top;
			m_clear				= val.m_clear;
		}

		~line(void)
		{
			clear();
		}

		void clear()
		{
			m_items.clear();
			m_height			= 0;
			m_top_margin		= 0;
			m_bottom_margin		= 0;
			m_is_block			= false;
			m_is_break			= false;
			m_left				= 0;
			m_top				= 0;
			m_padding_bottom	= 0;
			m_padding_top		= 0;
			m_clear				= clear_none;
			m_min_height		= 0;
			m_line_left			= 0;
			m_line_right		= 0;
		}

		void	add_element(element* el);

		element* get_last_space( );
		void	set_top(int top, element* parent);
		void	add_top(int add);

		int		get_height() const			{	return m_height;			}
		void	set_height(int h)			{	m_height = h;				}
		int		get_left() const			{	return m_left;				}
		void	init(int left, int right, int top, int line_height);
		int		get_top() const				{	return m_top;				}
		int		get_margin_top() const		{	return m_top_margin;		}
		int		get_margin_bottom() const	{	return m_bottom_margin;		}
		bool	is_block() const			{	return m_is_block;			}
		bool	is_break() const			{	return m_is_break;			}
		bool	empty()	const;
		void	get_elements(elements_vector& els);
		bool	finish(text_align align);
		bool	collapse_top_margin()		{	return (m_padding_top == 0);	}
		bool	collapse_bottom_margin()	{	return (m_padding_bottom == 0);	}
		bool	have_room_for(element* el);
		int		line_right()				{	return m_line_right;			}
		int		line_left()					{	return m_line_left;				}
		void	set_line_height(int val)	{	m_min_height = val;				}
		
		element_clear get_clear_floats() const { return m_clear; }
	};
}
