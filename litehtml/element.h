#pragma once

#include "object.h"
#include "style.h"
#include "line.h"
#include "background.h"
#include "css_margins.h"
#include "borders.h"
#include "css_selector.h"
#include "stylesheet.h"

namespace litehtml
{
	class element : public object
	{
		friend class elements_iterator;
		friend class line;
		friend class el_table;
		friend class table_grid;
	public:
		typedef litehtml::object_ptr<litehtml::element>	ptr;
	protected:
		litehtml::element*		m_parent;
		litehtml::document*		m_doc;
		elements_vector			m_children;
		std::wstring			m_id;
		std::wstring			m_class;
		std::wstring			m_tag;
		litehtml::style			m_style;
		string_map				m_attrs;
		position				m_pos;
		margins					m_margins;
		margins					m_padding;
		margins					m_borders;
		vertical_align			m_vertical_align;
		text_align				m_text_align;
		style_display			m_display;
		list_style_type			m_list_style_type;
		list_style_position		m_list_style_position;
		white_space				m_white_space;
		element_float			m_float;
		element_clear			m_clear;
		elements_vector			m_floats;
		elements_vector			m_absolutes;
		bool					m_skip;
		background				m_bg;
		element_position		m_el_position;
		int						m_line_height;
		litehtml::line*			m_line;
		string_vector			m_pseudo_classes;
		used_selector::vector	m_used_styles;		
		
		uint_ptr				m_font;
		int						m_font_size;
		int						m_base_line;

		css_margins				m_css_margins;
		css_margins				m_css_padding;
		css_borders				m_css_borders;
		css_length				m_css_width;
		css_length				m_css_height;
		css_length				m_css_min_width;
		css_length				m_css_min_height;
		css_length				m_css_left;
		css_length				m_css_right;
		css_length				m_css_top;
		css_length				m_css_bottom;

		overflow				m_overflow;

		/* rendered lines */
		line::vector			m_lines;

	public:
		element(litehtml::document* doc);
		virtual ~element();

		/* render functions */

		virtual int					render(int x, int y, int max_width);

		int							place_element( element* el, int max_width );
		virtual int					render_inline(litehtml::element* container, int max_width);
		int							add_line(int max_width, element_clear clr = clear_none, int el_width = 0, bool finish_prev = true);
		void						finish_line(int max_width);
		line::ptr					first_line() const;
		line::ptr					last_line() const;

		virtual bool				appendChild(litehtml::element* el);
		virtual element::ptr		parentElement() const;
		virtual const wchar_t*		get_tagName() const;
		virtual void				set_tagName(const wchar_t* tag);
		virtual void				set_data(const wchar_t* data);

		virtual void				set_attr(const wchar_t* name, const wchar_t* val);
		virtual const wchar_t*		get_attr(const wchar_t* name, const wchar_t* def = 0);
		virtual void				apply_stylesheet(const litehtml::css& stylesheet);
		virtual bool				is_white_space();
		virtual bool				is_body() const;
		virtual bool				is_break() const;
		virtual int					get_base_line();
		virtual background			get_background();
		virtual bool				on_mouse_over(int x, int y);
		virtual bool				on_mouse_leave();
		virtual bool				on_lbutton_down(int x, int y);
		virtual bool				on_lbutton_up(int x, int y);
		virtual void				on_click(int x, int y);
		virtual bool				find_styles_changes(position::vector& redraw_boxes, int x, int y);
		virtual const wchar_t*		get_cursor();
		virtual void				init_font();
		virtual bool				is_point_inside(int x, int y);
		virtual bool				set_pseudo_class(const wchar_t* pclass, bool add);
		virtual bool				in_normal_flow() const;

		white_space					get_white_space() const;
		style_display				get_display() const;
		elements_vector&			children();
		
		bool						select(const wchar_t* selectors);

		void						calc_outlines( int parent_width );
		virtual void				parse_styles(bool is_reparse = false);
		virtual void				draw(uint_ptr hdc, int x, int y, const position* clip);

		virtual void				draw_background( uint_ptr hdc, int x, int y, const position* clip );
		int							left()		const;
		int							right()		const;
		int							top()		const;
		int							bottom()	const;
		int							height()	const;
		int							height_raw() const;
		int							width()		const;

		int							content_margins_top()		const;
		int							content_margins_bottom()	const;
		int							content_margins_left()		const;
		int							content_margins_right()		const;

		margins						content_margins()		const;

		int							margin_top()		const;
		int							margin_bottom()		const;
		int							margin_left()		const;
		int							margin_right()		const;
		margins						get_margins()		const;

		int							padding_top()		const;
		int							padding_bottom()	const;
		int							padding_left()		const;
		int							padding_right()		const;
		margins						get_paddings()		const;

		int							border_top()		const;
		int							border_bottom()		const;
		int							border_left()		const;
		int							border_right()		const;
		margins						get_borders()		const;
		css_borders					get_css_borders()	const;

		virtual const wchar_t*		get_style_property(const wchar_t* name, bool inherited, const wchar_t* def = 0);

		uint_ptr					get_font();
		virtual int					get_font_size();
		litehtml::web_color			get_color(const wchar_t* prop_name, bool inherited, const litehtml::web_color& def_color = litehtml::web_color());
		int							select(const css_selector& selector, bool apply_pseudo = true);
		int							select(const css_element_selector& selector, bool apply_pseudo = true);
		element*					find_ancestor(const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = 0);
		void						get_abs_position(position& pos, const element* root);
		virtual void				get_text(std::wstring& text);
		virtual void				finish();

		bool						is_first_child(const element* el);
		bool						is_last_child(const element* el);

	protected:
		virtual void				get_content_size(size& sz, int max_width);
		virtual void				draw_content(uint_ptr hdc, const litehtml::position& pos);
		virtual void				init();
		void						get_inline_boxes(position::vector& boxes);

	private:
		bool						select_one(const std::wstring& selector);
		int							get_floats_height() const;
		int							get_left_floats_height() const;
		int							get_right_floats_height() const;
		int							get_line_left(int y) const;
		int							get_line_right(int y, int def_right) const;
		void						fix_line_width(int max_width);
		void						init_line(line::ptr& ln, int top, int def_right, element_clear el_clear = clear_none);
		void						add_float(element* el);
		void						add_absolute(element* el);
		bool						is_floats_holder() const;
		int							place_inline(element* el, int max_width);
		int							find_next_line_top(int top, int width, int def_right);
		void						parse_background();

	private:
		bool	m_second_pass;
	};

	/************************************************************************/
	/*                        Inline Functions                              */
	/************************************************************************/

	inline int litehtml::element::right() const
	{
		return left() + width();
	}

	inline int litehtml::element::left() const
	{
		return m_pos.left() - margin_left() - m_padding.left - m_borders.left;
	}

	inline bool	element::is_floats_holder() const
	{
		if(	m_display == display_inline_block || 
			m_display == display_table_cell || 
			!m_parent || 
			is_body() || 
			m_float != float_none ||
			m_el_position == element_position_absolute ||
			m_overflow > overflow_visible)
		{
			return true;
		}
		return false;
	}

	inline litehtml::style_display litehtml::element::get_display() const
	{
		return m_display;
	}


	inline elements_vector& litehtml::element::children()
	{
		return m_children;
	}

	inline int litehtml::element::top() const
	{
		return m_pos.top() - margin_top() - m_padding.top - m_borders.top;
	}

	inline int litehtml::element::bottom() const
	{
		return top() + width();
	}

	inline int litehtml::element::height() const
	{
		return m_pos.height + margin_top() + margin_bottom() + m_padding.height() + m_borders.height();
	}

	inline int litehtml::element::height_raw() const
	{
		return m_pos.height + (m_margins.top > 0 ? m_margins.top : 0) + (m_margins.bottom > 0 ? m_margins.bottom : 0) + m_padding.height() + m_borders.height();
	}

	inline int litehtml::element::width() const
	{
		return m_pos.width + margin_left() + margin_right() + m_padding.width() + m_borders.height();
	}

	inline int litehtml::element::content_margins_top() const
	{
		return margin_top() + m_padding.top + m_borders.top;
	}

	inline int litehtml::element::content_margins_bottom() const
	{
		return margin_bottom() + m_padding.bottom + m_borders.bottom;
	}

	inline int litehtml::element::content_margins_left() const
	{
		return margin_left() + m_padding.left + m_borders.left;
	}

	inline int litehtml::element::content_margins_right() const
	{
		return margin_right() + m_padding.right + m_borders.right;
	}

	inline litehtml::margins litehtml::element::content_margins() const
	{
		margins ret;
		ret.left	= content_margins_left();
		ret.right	= content_margins_right();
		ret.top		= content_margins_top();
		ret.bottom	= content_margins_bottom();
		return ret;
	}

	inline litehtml::margins litehtml::element::get_paddings()	const
	{
		return m_padding;
	}

	inline litehtml::margins litehtml::element::get_borders()	const
	{
		return m_borders;
	}

	inline litehtml::css_borders litehtml::element::get_css_borders() const
	{
		return m_css_borders;
	}

	inline int litehtml::element::padding_top() const
	{
		return m_padding.top;
	}

	inline int litehtml::element::padding_bottom() const
	{
		return m_padding.bottom;
	}

	inline int litehtml::element::padding_left() const
	{
		return m_padding.left;
	}

	inline int litehtml::element::padding_right() const
	{
		return m_padding.right;
	}

	inline bool litehtml::element::is_first_child( const element* el )
	{
		if(!m_children.empty())
		{
			if(el == m_children.front())
			{
				return true;
			}
		}
		return false;
	}

	inline bool litehtml::element::is_last_child( const element* el )
	{
		if(!m_children.empty())
		{
			if(el == m_children.back())
			{
				return true;
			}
		}
		return false;
	}

	inline int litehtml::element::border_top() const
	{
		return m_borders.top;
	}

	inline int litehtml::element::border_bottom() const
	{
		return m_borders.bottom;
	}

	inline int litehtml::element::border_left() const
	{
		return m_borders.left;
	}

	inline int litehtml::element::border_right() const
	{
		return m_borders.right;
	}

	inline white_space litehtml::element::get_white_space() const
	{
		return m_white_space;
	}
}

