#ifndef LH_HTML_TAG_H
#define LH_HTML_TAG_H

#include "element.h"
#include "style.h"
#include "background.h"
#include "css_margins.h"
#include "borders.h"
#include "css_selector.h"
#include "stylesheet.h"
#include "line_box.h"
#include "table.h"

namespace litehtml
{

	class html_tag : public element
	{
		friend class elements_iterator;
		friend class el_table;
		friend class table_grid;
		friend class line_box;
	public:
		typedef std::shared_ptr<litehtml::html_tag>	ptr;
	protected:
		string_vector			m_class_values;
		tstring					m_tag;
		litehtml::style			m_style;
		string_map				m_attrs;
		string_vector			m_pseudo_classes;

		void			select_all(const css_selector& selector, elements_vector& res) override;

	public:
		explicit html_tag(const std::shared_ptr<litehtml::document>& doc);

		bool				appendChild(const element::ptr &el) override;
		bool				removeChild(const element::ptr &el) override;
		void				clearRecursive() override;
		const tchar_t*		get_tagName() const override;
		void				set_tagName(const tchar_t* tag) override;
		void				set_data(const tchar_t* data) override;
		size_t				get_children_count() const override;
		element::ptr		get_child(int idx) const override;

		void				set_attr(const tchar_t* name, const tchar_t* val) override;
		const tchar_t*		get_attr(const tchar_t* name, const tchar_t* def = nullptr) const override;
		void				apply_stylesheet(const litehtml::css& stylesheet) override;
		void				refresh_styles() override;

		bool				is_white_space() const override;
		bool				is_body() const override;
		bool				is_break() const override;

        bool				on_mouse_over() override;
		bool				on_mouse_leave() override;
		bool				on_lbutton_down() override;
		bool				on_lbutton_up() override;
		void				on_click() override;
		const tchar_t*		get_cursor() override;
		bool				set_pseudo_class(const tchar_t* pclass, bool add) override;
		bool				set_class(const tchar_t* pclass, bool add) override;
		bool				is_replaced() const override;
		void				parse_styles(bool is_reparse = false) override;
		void                draw(uint_ptr hdc, int x, int y, const position *clip, const std::shared_ptr<render_item> &ri) override;
		void                draw_background(uint_ptr hdc, int x, int y, const position *clip,
                                    const std::shared_ptr<render_item> &ri) override;

		const tchar_t*		get_style_property(const tchar_t* name, bool inherited, const tchar_t* def = nullptr) const override;

		elements_vector&	children();

		int					select(const css_selector& selector, bool apply_pseudo = true) override;
		int					select(const css_element_selector& selector, bool apply_pseudo = true) override;

		elements_vector		select_all(const tstring& selector) override;
		elements_vector		select_all(const css_selector& selector) override;

		element::ptr		select_one(const tstring& selector) override;
		element::ptr		select_one(const css_selector& selector) override;

		element::ptr		find_ancestor(const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = nullptr) override;
		element::ptr		find_adjacent_sibling(const element::ptr& el, const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = nullptr) override;
		element::ptr		find_sibling(const element::ptr& el, const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = nullptr) override;
		void				get_text(tstring& text) override;
		void				parse_attributes() override;

		void				get_content_size(size& sz, int max_width) override;
		bool				is_floats_holder() const override;
		void				add_style(const tstring& style, const tstring& baseurl) override;

		bool				is_nth_child(const element::ptr& el, int num, int off, bool of_type) const override;
		bool				is_nth_last_child(const element::ptr& el, int num, int off, bool of_type) const override;
		bool				is_only_child(const element::ptr& el, bool of_type) const override;
		const background*	get_background(bool own_only = false) override;

        tstring             dump_get_name() override;

	protected:
		void				init_background_paint( position pos, background_paint &bg_paint, const background* bg, const std::shared_ptr<render_item> &ri );
		void				draw_list_marker( uint_ptr hdc, const position &pos );
		tstring				get_list_marker_text(int index);
		static void			parse_nth_child_params( const tstring& param, int &num, int &off );
		litehtml::element::ptr  get_element_before(const tstring& style, const tstring& baseurl, bool create);
		litehtml::element::ptr  get_element_after(const tstring& style, const tstring& baseurl, bool create);
    };

	/************************************************************************/
	/*                        Inline Functions                              */
	/************************************************************************/

	inline elements_vector& litehtml::html_tag::children()
	{
		return m_children;
	}
}

#endif  // LH_HTML_TAG_H
