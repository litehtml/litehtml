#pragma once

#include "object.h"
#include "style.h"
#include "line.h"
#include "background.h"
#include "css_margins.h"
#include "borders.h"

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
		elements_vector			m_inlines;
		line::vector			m_lines;
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
		used_styles::vector		m_style_sheets;		

		css_margins				m_css_margins;
		css_margins				m_css_padding;
		css_borders				m_css_borders;
		css_length				m_css_width;
		css_length				m_css_height;
		css_length				m_css_left;
		css_length				m_css_right;
		css_length				m_css_top;
		css_length				m_css_bottom;

	public:
		element(litehtml::document* doc);
		virtual ~element();

		virtual bool				appendChild(litehtml::element* el);
		virtual element::ptr		parentElement() const;
		virtual const wchar_t*		get_tagName() const;
		virtual void				set_tagName(const wchar_t* tag);
		virtual void				set_data(const wchar_t* data);

		virtual void				set_attr(const wchar_t* name, const wchar_t* val);
		virtual const wchar_t*		get_attr(const wchar_t* name, const wchar_t* def = 0);
		virtual void				apply_stylesheet(const litehtml::style_sheet::ptr style);
		virtual bool				is_white_space();
		virtual bool				is_body();
		virtual int					get_base_line();
		virtual background			get_background();
		virtual bool				on_mouse_over(int x, int y);
		virtual bool				on_mouse_leave();
		virtual bool				on_lbutton_down(int x, int y);
		virtual bool				on_lbutton_up(int x, int y);
		virtual void				on_click(int x, int y);
		virtual bool				find_styles_changes(position::vector& redraw_boxes, int x, int y);
		virtual const wchar_t*		get_cursor();

		style_display				get_display() const;
		elements_vector&			children();
		
		bool						select(const wchar_t* selectors);
		virtual int					render(uint_ptr hdc, int x, int y, int max_width);

		void						calc_outlines( int parent_width );
		virtual void				parse_styles(bool is_reparse = false);
		void						draw(uint_ptr hdc, int x, int y, position* clip);

		int							left()		const;
		int							right()		const;
		int							top()		const;
		int							bottom()	const;
		int							height()	const;
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

		virtual const wchar_t*		get_style_property(const wchar_t* name, bool inherited, const wchar_t* def = 0);

		uint_ptr					get_font();
		virtual int					get_font_size();
		litehtml::web_color			get_color(const wchar_t* prop_name, bool inherited, const litehtml::web_color& def_color = litehtml::web_color());
		int							select(const css_selector& selector, bool apply_pseudo = true);
		int							select(const css_element_selector& selector, bool apply_pseudo = true);
		int							select(const litehtml::style_sheet::ptr style, bool apply_pseudo = true);
		element*					find_ancestor(const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = 0);
		void						get_abs_position(position& pos, const element* root);
		virtual void				get_text(std::wstring& text);
		virtual void				finish();

	protected:
		virtual void				get_content_size(uint_ptr hdc, size& sz, int max_width);
		virtual void				draw_content(uint_ptr hdc, const litehtml::position& pos);
		virtual void				clear_inlines();
		virtual void				find_inlines();
		void						get_inline_boxes(position::vector& boxes);

	private:
		bool						select_one(const std::wstring& selector);
		int							add_line(line::ptr& ln, int max_width);
		int							get_floats_height() const;
		int							get_left_floats_height() const;
		int							get_right_floats_height() const;
		int							get_line_left(int y) const;
		int							get_line_right(int y, int def_right) const;
		void						fix_line_width(line::ptr& ln, int max_width);
		void						init_line(line::ptr& ln, int top, int def_right, element_clear el_clear = clear_none);
		void						add_float(element* el);
		void						add_absolute(element* el);
		bool						is_floats_holder() const;
		int							place_inline(element* el, line::ptr& ln, int max_width);
		int							find_next_line_top(int top, int width);
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
		if(m_display == display_inline_block || m_display == display_table_cell || !m_parent || m_tag == L"body" || m_float != float_none)
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

	inline void litehtml::element::set_tagName( const wchar_t* tag )
	{
		m_tag = tag;
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

}

