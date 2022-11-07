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
		typedef std::shared_ptr<html_tag>	ptr;
	protected:
		string_vector			m_class_values;
		string					m_tag;
		litehtml::style			m_style;
		string_map				m_attrs;
		string_vector			m_pseudo_classes;

		void			select_all(const css_selector& selector, elements_vector& res) override;

	public:
		explicit html_tag(const std::shared_ptr<litehtml::document>& doc);

		bool				appendChild(const element::ptr &el) override;
		bool				removeChild(const element::ptr &el) override;
		void				clearRecursive() override;
		const char*			get_tagName() const override;
		void				set_tagName(const char* tag) override;
		void				set_data(const char* data) override;
		size_t				get_children_count() const override;
		element::ptr		get_child(int idx) const override;

		void				set_attr(const char* name, const char* val) override;
		const char*			get_attr(const char* name, const char* def = nullptr) const override;
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
		const char*			get_cursor() override;
		bool				set_pseudo_class(const char* pclass, bool add) override;
		bool				set_class(const char* pclass, bool add) override;
		bool				is_replaced() const override;
		void				parse_styles(bool is_reparse = false) override;
		void                draw(uint_ptr hdc, int x, int y, const position *clip, const std::shared_ptr<render_item> &ri) override;
		void                draw_background(uint_ptr hdc, int x, int y, const position *clip,
                                    const std::shared_ptr<render_item> &ri) override;

		const char*			get_style_property(string_id name, bool inherited, const char* def = nullptr) const override;

		elements_vector&	children();

		int					select(const css_selector& selector, bool apply_pseudo = true) override;
		int					select(const css_element_selector& selector, bool apply_pseudo = true) override;

		elements_vector		select_all(const string& selector) override;
		elements_vector		select_all(const css_selector& selector) override;

		element::ptr		select_one(const string& selector) override;
		element::ptr		select_one(const css_selector& selector) override;

		element::ptr		find_ancestor(const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = nullptr) override;
		element::ptr		find_adjacent_sibling(const element::ptr& el, const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = nullptr) override;
		element::ptr		find_sibling(const element::ptr& el, const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = nullptr) override;
		void				get_text(string& text) override;
		void				parse_attributes() override;

		void				get_content_size(size& sz, int max_width) override;
		bool				is_floats_holder() const override;
		void				add_style(const string& style, const string& baseurl) override;

		bool				is_nth_child(const element::ptr& el, int num, int off, bool of_type) const override;
		bool				is_nth_last_child(const element::ptr& el, int num, int off, bool of_type) const override;
		bool				is_only_child(const element::ptr& el, bool of_type) const override;
		const background*	get_background(bool own_only = false) override;

        string             dump_get_name() override;

	protected:
		void				init_background_paint( position pos, background_paint &bg_paint, const background* bg, const std::shared_ptr<render_item> &ri );
		void				draw_list_marker( uint_ptr hdc, const position &pos );
		string				get_list_marker_text(int index);
		static void			parse_nth_child_params( const string& param, int &num, int &off );
		element::ptr		get_element_before(const string& style, const string& baseurl, bool create);
		element::ptr		get_element_after(const string& style, const string& baseurl, bool create);
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
