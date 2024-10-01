#ifndef LH_DOCUMENT_H
#define LH_DOCUMENT_H

#include "style.h"
#include "types.h"
#include "master_css.h"
#include "encodings.h"
typedef struct GumboInternalOutput GumboOutput;

namespace litehtml
{
	struct css_text
	{
		typedef std::vector<css_text>	vector;

		string	text;
		string	baseurl;
		string	media;
		
		css_text() = default;

		css_text(const char* txt, const char* url, const char* media_str)
		{
			text	= txt ? txt : "";
			baseurl	= url ? url : "";
			media	= media_str ? media_str : "";
		}
	};

	class dumper
	{
	public:
		virtual ~dumper() {}
		virtual void begin_node(const string& descr) = 0;
		virtual void end_node() = 0;
		virtual void begin_attrs_group(const string& descr) = 0;
		virtual void end_attrs_group() = 0;
		virtual void add_attr(const string& name, const string& value) = 0;
	};

	class html_tag;
	class render_item;

	class document : public std::enable_shared_from_this<document>
	{
	public:
		typedef std::shared_ptr<document>	ptr;
		typedef std::weak_ptr<document>		weak_ptr;
	private:
		std::shared_ptr<element>			m_root;
		std::shared_ptr<render_item>		m_root_render;
		document_container*					m_container;
		fonts_map							m_fonts;
		css_text::vector					m_css;
		litehtml::css						m_styles;
		litehtml::web_color					m_def_color;
		litehtml::css						m_master_css;
		litehtml::css						m_user_css;
		litehtml::size						m_size;
		litehtml::size						m_content_size;
		position::vector					m_fixed_boxes;
		element::ptr						m_over_element;
		std::list<shared_ptr<render_item>>	m_tabular_elements;
		media_query_list_list::vector		m_media_lists;
		media_features						m_media;
		string								m_lang;
		string								m_culture;
		string								m_text;
		document_mode						m_mode = no_quirks_mode;
	public:
		document(document_container* objContainer);
		virtual ~document();

		document_container*				container()	{ return m_container; }
		document_mode					mode() const { return m_mode; }
		uint_ptr						get_font(const char* name, int size, const char* weight, const char* style, const char* decoration, font_metrics* fm);
		int								render(int max_width, render_type rt = render_all);
		void							draw(uint_ptr hdc, int x, int y, const position* clip);
		web_color						get_def_color()	{ return m_def_color; }
		void 							cvt_units(css_length& val, const font_metrics& metrics, int size) const;
		int								to_pixels(const css_length& val, const font_metrics& metrics, int size) const;
		int								width() const;
		int								height() const;
		int								content_width() const;
		int								content_height() const;
		void							add_stylesheet(const char* str, const char* baseurl, const char* media);
		bool							on_mouse_over(int x, int y, int client_x, int client_y, position::vector& redraw_boxes);
		bool							on_lbutton_down(int x, int y, int client_x, int client_y, position::vector& redraw_boxes);
		bool							on_lbutton_up(int x, int y, int client_x, int client_y, position::vector& redraw_boxes);
		bool							on_mouse_leave(position::vector& redraw_boxes);
		element::ptr					create_element(const char* tag_name, const string_map& attributes);
		element::ptr					root();
		std::shared_ptr<render_item>	root_render();
		void							get_fixed_boxes(position::vector& fixed_boxes);
		void							add_fixed_box(const position& pos);
		void							add_media_list(media_query_list_list::ptr list);
		bool							media_changed();
		bool							lang_changed();
		bool							match_lang(const string& lang);
		void							add_tabular(const std::shared_ptr<render_item>& el);
		element::const_ptr				get_over_element() const { return m_over_element; }

		void							append_children_from_string(element& parent, const char* str);
		void							dump(dumper& cout);

		// see doc/document_createFromString.txt
		static document::ptr  createFromString(
			const estring&       str,
			document_container*  container,
			const string&        master_styles = litehtml::master_css,
			const string&        user_styles = "");
	
	private:
		uint_ptr	add_font(const char* name, int size, const char* weight, const char* style, const char* decoration, font_metrics* fm);

		GumboOutput* parse_html(estring str);
		void create_node(void* gnode, elements_list& elements, bool parseTextNode);
		bool update_media_lists(const media_features& features);
		void fix_tables_layout();
		void fix_table_children(const std::shared_ptr<render_item>& el_ptr, style_display disp, const char* disp_str);
		void fix_table_parent(const std::shared_ptr<render_item> & el_ptr, style_display disp, const char* disp_str);
	};

	inline element::ptr document::root()
	{
		return m_root;
	}

	inline std::shared_ptr<render_item> document::root_render()
	{
		return m_root_render;
	}

	inline void document::add_tabular(const std::shared_ptr<render_item>& el)
	{
		m_tabular_elements.push_back(el);
	}
	inline bool document::match_lang(const string& lang)
	{
		return lang == m_lang || lang == m_culture;
	}
}

#endif  // LH_DOCUMENT_H
