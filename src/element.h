#pragma once
#include "object.h"
#include "stylesheet.h"
#include "css_offsets.h"
#include "css_margins.h"

namespace litehtml
{
	class box;

	class element : public object
	{
		friend class block_box;
		friend class line_box;
		friend class html_tag;
		friend class el_table;
		friend class document;
	public:
		typedef litehtml::object_ptr<litehtml::element>		ptr;
	protected:
		litehtml::element*			m_parent;
		litehtml::document*			m_doc;
		litehtml::box*				m_box;
		elements_vector				m_children;
		position					m_pos;
		margins						m_margins;
		margins						m_padding;
		margins						m_borders;
		bool						m_skip;
		pointer_events				m_pointer_events;
		litehtml::style				m_style;
		float						m_opacity;
		bool						m_dirty_style;
	public:
		element(litehtml::document* doc);
		virtual ~element();

		// returns refer to m_pos member;
		position&					get_position();
		litehtml::document *		document();
		const litehtml::document *	document() 					const;

		int							left()						const;
		int							right()						const;
		int							top()						const;
		int							bottom()					const;
		int							height()					const;
		int							width()						const;
		int							box_height()				const;
		int							box_width()					const;

		int							content_margins_top()		const;
		int							content_margins_bottom()	const;
		int							content_margins_left()		const;
		int							content_margins_right()		const;
		int							content_margins_width()		const;
		int							content_margins_height()	const;

		int							margin_top()				const;
		int							margin_bottom()				const;
		int							margin_left()				const;
		int							margin_right()				const;
		margins						get_margins()				const;

		int							padding_top()				const;
		int							padding_bottom()			const;
		int							padding_left()				const;
		int							padding_right()				const;
		margins						get_paddings()				const;

		int							border_top()				const;
		int							border_bottom()				const;
		int							border_left()				const;
		int							border_right()				const;
		margins						get_borders()				const;

		bool						in_normal_flow()			const;
		litehtml::web_color			get_color(const string_hash & prop_name, bool inherited, const litehtml::web_color& def_color = litehtml::web_color());
		bool						is_inline_box()				const;
		position					get_placement()				const;
		bool						collapse_top_margin()		const;
		bool						collapse_bottom_margin()	const;
		bool						is_positioned()				const;
		litehtml::pointer_events	get_pointer_events() 		const;

		bool						skip();
		void						skip(bool val);
		element*					parent() const;
		void						parent(element* par);
		bool						is_visible() const;
		float                       get_opacity() const;
		int							calc_width(int defVal) const;
		int							get_inline_shift_left();
		int							get_inline_shift_right();
		void						apply_relative_shift(int parent_width);

		bool						set_inner_html(const tchar_t* text);

		virtual element::ptr		select_one(const tstring& selector);
		virtual element::ptr		select_one(const css_selector& selector);

		virtual int					render(int x, int y, int max_width, bool second_pass = false);
		virtual int					render_inline(element* container, int max_width);
		virtual int					place_element(element* el, int max_width);
		virtual void				calc_outlines( int parent_width );
		virtual void				calc_auto_margins(int parent_width);
		virtual void				apply_vertical_align();
		virtual bool				fetch_positioned();
		virtual void				render_positioned(render_type rt = render_all);

		virtual bool				appendChild(litehtml::element* el);
		virtual bool				addChildAfter(litehtml::element* new_child, litehtml::element * existing_child);

		virtual const tchar_t*		get_tagName() const;
		virtual void				set_tagName(const tchar_t* tag);
		virtual void				set_data(const tchar_t* data);
		virtual element_float		get_float() const;
		virtual vertical_align		get_vertical_align() const;
		virtual element_clear		get_clear() const;
		virtual size_t				get_children_count() const;
		virtual element::ptr		get_child(int idx) const;
		elements_vector &           children();
		virtual size_t				get_index() const;
		virtual size_t				get_index( const tstring & selector ) const;
		virtual overflow			get_overflow() const;

		virtual css_margins			get_css_margins() const; // :TODO: Make this a const ref when these css accesors are moved to html_tag.h
		virtual css_length			get_css_left() const;
		virtual css_length			get_css_right() const;
		virtual css_length			get_css_top() const;
		virtual css_length			get_css_bottom() const;
		virtual css_offsets			get_css_offsets() const;
		virtual css_length			get_css_width() const;
		virtual void				set_css_width(css_length& w);
		virtual css_length			get_css_height() const;

		virtual void				set_attr(const tchar_t* name, const tchar_t* val);
		virtual const tchar_t*		get_attr(const tchar_t* name, const tchar_t* def = 0) const;
		virtual void				apply_stylesheet(const litehtml::css& stylesheet);
		virtual void				refresh_styles();
		virtual bool				is_white_space();
		virtual bool				is_body() const;
		virtual bool				is_break() const;
		virtual int					get_base_line();
		virtual bool				on_mouse_over();
		virtual bool				on_mouse_leave();
		virtual bool				on_lbutton_down();
		virtual bool				on_lbutton_up();
		virtual void				on_click();
		virtual bool				find_styles_changes(position::vector& redraw_boxes, int x, int y);
		virtual const tchar_t*		get_cursor();
		virtual void				init_font();
		virtual bool				is_point_inside(int x, int y);
		virtual bool				set_pseudo_class(const tchar_t* pclass, bool add);
		virtual bool				set_class(const tchar_t* pclass, bool add);
		virtual bool				has_class(const tchar_t* pclass) const;
		virtual bool				is_replaced() const;
		virtual int					line_height() const;
		virtual white_space			get_white_space() const;
		virtual style_display		get_display() const;
		virtual visibility			get_visibility() const;
		virtual element_position	get_element_position(css_offsets* offsets = 0) const;
		virtual void				get_inline_boxes(position::vector& boxes);
		virtual void				parse_styles(bool is_reparse = false);
		virtual void				draw(uint_ptr hdc, int x, int y, const position* clip);
		virtual void				draw_background( uint_ptr hdc, int x, int y, const position* clip );
		const tchar_t*		        get_style_property(const string_hash & name, bool inherited, const tchar_t* def = 0) const;
		virtual uint_ptr			get_font(font_metrics* fm = 0);
		virtual int					get_font_size() const;
		virtual void				get_text(tstring& text);
		virtual void				parse_attributes();
		virtual int					select(const css_selector& selector, bool apply_pseudo = true);
		virtual int					select(const css_element_selector& selector, bool apply_pseudo = true);
		virtual element*			find_ancestor(const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = 0);
		bool						is_ancestor(element* el) const;
		virtual element*			find_adjacent_sibling(element* el, const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = 0);
		virtual element*			find_sibling(element* el, const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = 0);
		virtual bool				is_first_child_inline(const element* el);
		virtual bool				is_last_child_inline(const element* el);
		virtual bool				have_inline_child();
		virtual void				get_content_size(size& sz, int max_width);
		virtual void				init();
		virtual void				finalize();
		virtual bool				is_floats_holder() const;
		virtual int					get_floats_height(element_float el_float = float_none) const;
		virtual int					get_left_floats_height() const;
		virtual int					get_right_floats_height() const;
		virtual int					get_line_left(int y);
		virtual int					get_line_right(int y, int def_right);
		virtual void				get_line_left_right(int y, int def_right, int& ln_left, int& ln_right);
		virtual void				add_float(element* el, int x, int y);
		virtual void				update_floats(int dy, element* parent);
		virtual void				add_positioned(element* el);
		virtual int					find_next_line_top(int top, int width, int def_right);
		virtual int					get_zindex() const;
		virtual void				draw_stacking_context(uint_ptr hdc, int x, int y, const position* clip, bool with_positioned);
		virtual void				draw_children( uint_ptr hdc, int x, int y, const position* clip, draw_flag flag, int zindex );
		virtual bool				is_nth_child(element* el, int num, int off, bool of_type);
		virtual bool				is_nth_last_child(element* el, int num, int off, bool of_type);
		virtual bool				is_only_child(element* el, bool of_type);
		virtual bool				get_predefined_height(int& p_height) const;
		virtual void				calc_document_size(litehtml::size& sz, int x = 0, int y = 0);
		virtual void				get_redraw_box(litehtml::position& pos, int x = 0, int y = 0);
		virtual void				add_user_style(litehtml::style::ptr st);
		virtual void				add_style(litehtml::style::ptr st);
		virtual element*			get_element_by_point(int x, int y, int client_x, int client_y);
		virtual element*			get_child_by_point(int x, int y, int client_x, int client_y, draw_flag flag, int zindex);
		virtual background*			get_background(bool own_only = false);
	};

	//////////////////////////////////////////////////////////////////////////
	//							INLINE FUNCTIONS							//
	//////////////////////////////////////////////////////////////////////////

	inline litehtml::elements_vector & litehtml::element::children()
	{
		return m_children;
	}

	inline litehtml::document* litehtml::element::document()
	{
		return m_doc;
	}

	inline const litehtml::document * litehtml::element::document() const
	{
		return m_doc;
	}

	inline int litehtml::element::right() const
	{
		return left() + width();
	}

	inline int litehtml::element::left() const
	{
		return m_pos.left() - margin_left() - m_padding.left - m_borders.left;
	}

	inline int litehtml::element::top() const
	{
		return m_pos.top() - margin_top() - m_padding.top - m_borders.top;
	}

	inline int litehtml::element::bottom() const
	{
		return top() + height();
	}

	inline int litehtml::element::height() const
	{
		return margin_top() + margin_bottom() + box_height() + m_borders.height();
	}

	inline int litehtml::element::width() const
	{
		return margin_left() + margin_right() + box_width() + m_borders.width();
	}

	inline int litehtml::element::box_height() const
	{
		return m_pos.height + m_padding.height();
	}

	inline int litehtml::element::box_width() const
	{
		return m_pos.width + m_padding.width();
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

	inline int litehtml::element::content_margins_width() const
	{
		return content_margins_left() + content_margins_right();
	}

	inline int litehtml::element::content_margins_height() const
	{
		return content_margins_top() + content_margins_bottom();
	}

	inline litehtml::margins litehtml::element::get_paddings()	const
	{
		return m_padding;
	}

	inline litehtml::margins litehtml::element::get_borders()	const
	{
		return m_borders;
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

	inline bool litehtml::element::in_normal_flow() const
	{
		if(get_element_position() != element_position_absolute && get_display() != display_none)
		{
			return true;
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

	inline bool litehtml::element::skip()
	{
		return m_skip;
	}

	inline void litehtml::element::skip(bool val)
	{
		m_skip = val;
	}

	inline element* litehtml::element::parent() const
	{
		return m_parent;
	}

	inline void litehtml::element::parent(element* par)
	{
		m_parent = par;
		m_dirty_style = true;
	}

	inline int litehtml::element::margin_top() const
	{
		return m_margins.top;
	}

	inline int litehtml::element::margin_bottom() const
	{
		return m_margins.bottom;
	}

	inline int litehtml::element::margin_left() const
	{
		return m_margins.left;
	}

	inline int litehtml::element::margin_right() const
	{
		return m_margins.right;
	}

	inline litehtml::margins litehtml::element::get_margins() const
	{
		margins ret;
		ret.left	= margin_left();
		ret.right	= margin_right();
		ret.top		= margin_top();
		ret.bottom	= margin_bottom();

		return ret;
	}

	inline bool litehtml::element::is_positioned()	const
	{
		return (get_element_position() > element_position_static);
	}

	inline litehtml::pointer_events litehtml::element::get_pointer_events()	const
	{
		return m_pointer_events;
	}

	inline bool litehtml::element::is_visible() const
	{
		return !(m_skip || get_display() == display_none || get_visibility() != visibility_visible);
	}

	inline float litehtml::element::get_opacity() const
	{
		return m_opacity;
	}

	inline position& litehtml::element::get_position()
	{
		return m_pos;
	}

}
