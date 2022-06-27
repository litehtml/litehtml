#ifndef LH_ELEMENT_H
#define LH_ELEMENT_H

#include <memory>
#include <tuple>
#include <list>
#include "stylesheet.h"
#include "css_offsets.h"
#include "css_margins.h"
#include "css_properties.h"

namespace litehtml
{
	class line_box;
    class dumper;
    class render_item;

	class element : public std::enable_shared_from_this<element>
	{
		friend class line_box;
		friend class html_tag;
		friend class el_table;
		friend class document;
	public:
		typedef std::shared_ptr<litehtml::element>			ptr;
		typedef std::shared_ptr<const litehtml::element>	const_ptr;
		typedef std::weak_ptr<litehtml::element>			weak_ptr;
	protected:
		std::weak_ptr<element>		            m_parent;
		std::weak_ptr<litehtml::document>	    m_doc;
        elements_vector				            m_children;
        css_properties                          m_css;
        std::list<std::weak_ptr<render_item>>   m_renders;
        used_selector::vector	                m_used_styles;

        virtual void select_all(const css_selector& selector, elements_vector& res);
        element::ptr _add_before_after(int type, const tstring& style, const tstring& baseurl);
	public:
		explicit element(const std::shared_ptr<litehtml::document>& doc);
        virtual ~element() = default;

        const css_properties&       css() const;
        css_properties&             css_w();

		bool						in_normal_flow()			const;
		litehtml::web_color			get_color(const tchar_t* prop_name, bool inherited, const litehtml::web_color& def_color = litehtml::web_color());
		bool						is_inline_box()				const;
        bool                        is_block_box()              const;
		position					get_placement()				const;
		bool						is_positioned()				const;
        bool						is_float()				    const;

		bool						have_parent() const;
		element::ptr				parent() const;
		void						parent(const element::ptr& par);
		// returns true for elements inside a table (but outside cells) that don't participate in table rendering
		bool						is_table_skip() const;

		std::shared_ptr<document>	get_document() const;

		virtual elements_vector		select_all(const tstring& selector);
		virtual elements_vector		select_all(const css_selector& selector);

		virtual element::ptr		select_one(const tstring& selector);
		virtual element::ptr		select_one(const css_selector& selector);

		virtual bool				appendChild(const ptr &el);
		virtual bool				removeChild(const ptr &el);
		virtual void				clearRecursive();

		virtual const tchar_t*		get_tagName() const;
		virtual void				set_tagName(const tchar_t* tag);
		virtual void				set_data(const tchar_t* data);
		virtual size_t				get_children_count() const;
		virtual element::ptr		get_child(int idx) const;

		virtual void				set_attr(const tchar_t* name, const tchar_t* val);
		virtual const tchar_t*		get_attr(const tchar_t* name, const tchar_t* def = nullptr) const;
		virtual void				apply_stylesheet(const litehtml::css& stylesheet);
		virtual void				refresh_styles();
		virtual bool				is_white_space() const;
        virtual bool                is_space() const;
		virtual bool				is_comment() const;
		virtual bool				is_body() const;
        virtual bool				is_break() const;
        virtual bool				is_text() const;

        virtual bool				on_mouse_over();
		virtual bool				on_mouse_leave();
		virtual bool				on_lbutton_down();
		virtual bool				on_lbutton_up();
		virtual void				on_click();
		virtual const tchar_t*		get_cursor();
		virtual bool				set_pseudo_class(const tchar_t* pclass, bool add);
		virtual bool				set_class(const tchar_t* pclass, bool add);
		virtual bool				is_replaced() const;
		virtual void				parse_styles(bool is_reparse = false);
		virtual void                draw(uint_ptr hdc, int x, int y, const position *clip, const std::shared_ptr<render_item>& ri);
		virtual void                draw_background(uint_ptr hdc, int x, int y, const position *clip, const std::shared_ptr<render_item> &ri);
		virtual const tchar_t*		get_style_property(const tchar_t* name, bool inherited, const tchar_t* def = nullptr) const;
		virtual void				get_text(tstring& text);
		virtual void				parse_attributes();
		virtual int					select(const css_selector& selector, bool apply_pseudo = true);
		virtual int					select(const css_element_selector& selector, bool apply_pseudo = true);
		virtual element::ptr		find_ancestor(const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = nullptr);
		virtual bool				is_ancestor(const ptr &el) const;
		virtual element::ptr		find_adjacent_sibling(const element::ptr& el, const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = nullptr);
		virtual element::ptr		find_sibling(const element::ptr& el, const css_selector& selector, bool apply_pseudo = true, bool* is_pseudo = nullptr);
		virtual void				get_content_size(size& sz, int max_width);
		virtual bool				is_floats_holder() const;
		virtual void				update_floats(int dy, const ptr &parent);
		virtual bool				is_nth_child(const element::ptr& el, int num, int off, bool of_type) const;
		virtual bool				is_nth_last_child(const element::ptr& el, int num, int off, bool of_type) const;
		virtual bool				is_only_child(const element::ptr& el, bool of_type) const;
		virtual void				add_style(const tstring& style, const tstring& baseurl);
		virtual const background*	get_background(bool own_only = false);

        virtual tstring             dump_get_name();
        virtual std::vector<std::tuple<tstring, tstring>> dump_get_attrs();
        void                        dump(litehtml::dumper& cout);

        std::tuple<element::ptr, element::ptr, element::ptr> split_inlines();
        virtual std::shared_ptr<render_item> create_render_item(const std::shared_ptr<render_item>& parent_ri);
        bool requires_styles_update();
        void add_render(const std::shared_ptr<render_item>& ri);
        bool find_styles_changes( position::vector& redraw_boxes);
        element::ptr add_pseudo_before(const tstring& style, const tstring& baseurl)
        {
            return _add_before_after(0, style, baseurl);
        }
        element::ptr add_pseudo_after(const tstring& style, const tstring& baseurl)
        {
            return _add_before_after(1, style, baseurl);
        }
	};

	//////////////////////////////////////////////////////////////////////////
	//							INLINE FUNCTIONS							//
	//////////////////////////////////////////////////////////////////////////

	inline bool litehtml::element::in_normal_flow() const
	{
		if(css().get_position() != element_position_absolute && css().get_display() != display_none)
		{
			return true;
		}
		return false;
	}

	inline bool litehtml::element::have_parent() const
	{
		return !m_parent.expired();
	}

	inline element::ptr litehtml::element::parent() const
	{
		return m_parent.lock();
	}

	inline void litehtml::element::parent(const element::ptr& par)
	{
		m_parent = par;
	}

	inline bool litehtml::element::is_positioned()	const
	{
		return (css().get_position() > element_position_static);
	}

    inline bool litehtml::element::is_float()	const
    {
        return (css().get_float() != float_none);
    }

	inline std::shared_ptr<document> element::get_document() const
	{
		return m_doc.lock();
	}

    inline const css_properties& element::css() const
    {
        return m_css;
    }

    inline css_properties& element::css_w()
    {
        return m_css;
    }

    inline bool element::is_block_box() const
    {
        if(css().get_display() == display_block ||
           css().get_display() == display_flex ||
           css().get_display() == display_table ||
           css().get_display() == display_list_item ||
           css().get_display() == display_flex)
        {
            return true;
        }
        return false;
    }
}

#endif  // LH_ELEMENT_H
