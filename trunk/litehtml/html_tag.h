#pragma once

#include "element.h"
#include "object.h"
#include "style.h"
#include "background.h"
#include "css_margins.h"
#include "borders.h"
#include "css_selector.h"
#include "stylesheet.h"
#include "box.h"

namespace litehtml
{
	class html_tag : public element
	{
		friend class elements_iterator;
		friend class el_table;
		friend class table_grid;
		friend class block_box;
		friend class line_box;
	public:
		typedef litehtml::object_ptr<litehtml::html_tag>	ptr;
	protected:
		elements_vector			m_children;
		box::vector				m_boxes;
		tstring					m_id;
		tstring					m_class;
		tstring					m_tag;
		litehtml::style			m_style;
		string_map				m_attrs;
		vertical_align			m_vertical_align;
		text_align				m_text_align;
		style_display			m_display;
		list_style_type			m_list_style_type;
		list_style_position		m_list_style_position;
		white_space				m_white_space;
		element_float			m_float;
		element_clear			m_clear;
		floated_box::vector		m_floats_left;
		floated_box::vector		m_floats_right;
		elements_vector			m_positioned;
		background				m_bg;
		element_position		m_el_position;
		int						m_line_height;
		bool					m_lh_predefined;
		string_vector			m_pseudo_classes;
		used_selector::vector	m_used_styles;		
		
		uint_ptr				m_font;
		int						m_font_size;
		font_metrics			m_font_metrics;

		css_margins				m_css_margins;
		css_margins				m_css_padding;
		css_borders				m_css_borders;
		css_length				m_css_width;
		css_length				m_css_height;
		css_length				m_css_min_width;
		css_length				m_css_min_height;
		css_length				m_css_max_width;
		css_length				m_css_max_height;
		css_length				m_css_left;
		css_length				m_css_right;
		css_length				m_css_top;
		css_length				m_css_bottom;

		overflow				m_overflow;
		visibility				m_visibility;
		int						m_z_index;

		int_int_cache			m_cahe_line_left;
		int_int_cache			m_cahe_line_right;
	public:
		html_tag(litehtml::document* doc);
		virtual ~html_tag();

		/* render functions */

		virtual int					render(int x, int y, int max_width);

		virtual int					render_inline(element* container, int max_width);
		virtual int					place_element(element* el, int max_width);
		virtual bool				fetch_positioned();
		virtual void				render_absolutes();

		int							new_box( element* el, int max_width );

		int							get_cleared_top( element* el, int line_top );
		int							finish_last_box(bool end_of_render = false);

		virtual bool				appendChild(litehtml::element* el);
		virtual const tchar_t*		get_tagName() const;
		virtual void				set_tagName(const tchar_t* tag);
		virtual void				set_data(const tchar_t* data);
		virtual element_float		get_float() const;
		virtual vertical_align		get_vertical_align() const;
		virtual css_length			get_css_left() const;
		virtual css_length			get_css_right() const;
		virtual css_length			get_css_top() const;
		virtual css_length			get_css_bottom() const;
		virtual css_length			get_css_width() const;
		virtual void				set_css_width(css_length& w);
		virtual css_length			get_css_height() const;
		virtual element_clear		get_clear() const;
		virtual size_t				get_children_count() const;
		virtual element::ptr		get_child(int idx) const;
		virtual element_position	get_element_position() const;
		virtual overflow			get_overflow() const;

		virtual void				set_attr(const tchar_t* name, const tchar_t* val);
		virtual const tchar_t*		get_attr(const tchar_t* name, const tchar_t* def = 0);
		virtual void				apply_stylesheet(const litehtml::css& stylesheet);

		virtual bool				is_white_space();
		virtual bool				is_body() const;
		virtual bool				is_break() const;
		virtual int					get_base_line();
		virtual bool				on_mouse_over(int x, int y);
		virtual bool				on_mouse_leave();
		virtual bool				on_lbutton_down(int x, int y);
		virtual bool				on_lbutton_up(int x, int y);
		virtual void				on_click(int x, int y);
		virtual bool				find_styles_changes(position::vector& redraw_boxes, int x, int y);
		virtual const tchar_t*		get_cursor();
		virtual void				init_font();
		virtual bool				set_pseudo_class(const tchar_t* pclass, bool add);
		virtual bool				is_replaced() const;
		virtual int					line_height() const;
		virtual white_space			get_white_space() const;
		virtual style_display		get_display() const;
		virtual visibility			get_visibility() const;
		virtual void				parse_styles(bool is_reparse = false);
		virtual void				draw(uint_ptr hdc, int x, int y, const position* clip);
		virtual void				draw_background( uint_ptr hdc, int x, int y, const position* clip );

		virtual const tchar_t*		get_style_property(const tchar_t* name, bool inherited, const tchar_t* def = 0);
		virtual uint_ptr			get_font(font_metrics* fm = 0);
		virtual int					get_font_size() const;

		elements_vector&			children();
		virtual void				calc_outlines( int parent_width );

		virtual int					select(const css_selector& selector, bool apply_pseudo = true);
		virtual int					select(const css_element_selector& selector, bool apply_pseudo = true);

		virtual element::ptr		select_one(const tstring& selector);
		virtual element::ptr		select_one(const css_selector& selector);

		virtual element*			find_ancestor(const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = 0);
		virtual element*			find_adjacent_sibling(element* el, const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = 0);
		virtual element*			find_sibling(element* el, const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = 0);
		virtual void				get_text(tstring& text);
		virtual void				parse_attributes();

		virtual bool				is_first_child(const element* el);
		virtual bool				is_last_child(const element* el);
		virtual void				get_content_size(size& sz, int max_width);
		virtual void				init();
		virtual void				get_inline_boxes(position::vector& boxes);
		virtual bool				is_floats_holder() const;
		virtual int					get_floats_height(element_float el_float = float_none) const;
		virtual int					get_left_floats_height() const;
		virtual int					get_right_floats_height() const;
		virtual int					get_line_left(int y);
		virtual int					get_line_right(int y, int def_right);
		virtual void				get_line_left_right(int y, int def_right, int& ln_left, int& ln_right);
		virtual void				add_float(element* el, int x, int y);
		virtual void				update_floats(int dy, element* parent);
		virtual void				add_absolute(element* el);
		virtual int					find_next_line_top(int top, int width, int def_right);
		virtual void				apply_vertical_align();
		virtual void				draw_children( uint_ptr hdc, int x, int y, const position* clip, draw_flag flag, int zindex );
		virtual int					get_zindex() const;
		virtual void				draw_stacking_context(uint_ptr hdc, int x, int y, const position* clip, bool with_positioned);
		virtual void				calc_document_size(litehtml::size& sz, int x = 0, int y = 0);
		virtual void				add_style(litehtml::style::ptr st);

		virtual bool				is_nth_child(element* el, int num, int off, bool of_type);
		virtual bool				is_nth_last_child(element* el, int num, int off, bool of_type);
		virtual bool				is_only_child(element* el, bool of_type);

	protected:
		void						fix_line_width(int max_width, element_float flt);
		void						parse_background();
		void						init_background_paint( position pos, background_paint &bg_paint );
		void						draw_list_marker( uint_ptr hdc, const position &pos );
		void						parse_nth_child_params( tstring param, int &num, int &off );
		void						remove_before_after();
		litehtml::element*			get_element_before();
		litehtml::element*			get_element_after();

	private:
		bool	m_second_pass;
	};

	/************************************************************************/
	/*                        Inline Functions                              */
	/************************************************************************/

	inline elements_vector& litehtml::html_tag::children()
	{
		return m_children;
	}
}

