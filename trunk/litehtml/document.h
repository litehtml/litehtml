#pragma once
#include "object.h"
#include "style.h"
#include "types.h"
#include "xh_scanner.h"
#include "context.h"

namespace litehtml
{

	struct str_istream: public litehtml::instream
	{
		const tchar_t* p;
		const tchar_t* end;

		str_istream(const tchar_t* src): p(src), end(src + t_strlen(src)) {}
		virtual tchar_t get_char() { return p < end? *p++: 0; }
	};

	struct css_text
	{
		typedef std::vector<css_text>	vector;

		tstring	text;
		tstring	baseurl;
		
		css_text()
		{
		}

		css_text(const tchar_t* txt, const tchar_t* url)
		{
			text	= txt ? txt : _t("");
			baseurl	= url ? url : _t("");
		}

		css_text(const css_text& val)
		{
			text	= val.text;
			baseurl	= val.baseurl;
		}
	};

	struct tags_parse_data
	{
		const litehtml::tchar_t*	tag;
		const litehtml::tchar_t*	parents;
		const litehtml::tchar_t*	stop_parent;
	};

	class html_tag;

	class document : public object
	{
	public:
		typedef object_ptr<document>	ptr;
	private:
		element::ptr						m_root;
		document_container*					m_container;
		fonts_map							m_fonts;
		css_text::vector					m_css;
		litehtml::css						m_styles;
		litehtml::web_color					m_def_color;
		litehtml::context*					m_context;
		litehtml::size						m_size;
		static litehtml::tags_parse_data 	m_tags_table[];
		elements_vector						m_parse_stack;
	public:
		document(litehtml::document_container* objContainer, litehtml::context* ctx);
		virtual ~document();

		litehtml::document_container*	container()	{ return m_container; }
		uint_ptr						get_font(const tchar_t* name, int size, const tchar_t* weight, const tchar_t* style, const tchar_t* decoration, font_metrics* fm);
		int								render(int max_width);
		void							draw(uint_ptr hdc, int x, int y, const position* clip);
		web_color						get_def_color()	{ return m_def_color; }
		int								cvt_units(const tchar_t* str, int fontSize, bool* is_percent = 0) const;
		int								cvt_units(css_length& val, int fontSize, int size = 0) const;
		int								width() const;
		int								height() const;
		void							add_stylesheet(const tchar_t* str, const tchar_t* baseurl);
		bool							on_mouse_over(int x, int y, position::vector& redraw_boxes);
		bool							on_lbutton_down(int x, int y, position::vector& redraw_boxes);
		bool							on_lbutton_up(int x, int y, position::vector& redraw_boxes);
		bool							on_mouse_leave(position::vector& redraw_boxes);
		litehtml::element::ptr			create_element(const tchar_t* tag_name);
		element::ptr					root();

		static litehtml::document::ptr createFromString(const tchar_t* str, litehtml::document_container* objPainter, litehtml::context* ctx, litehtml::css* user_styles = 0);
	
	private:
		//void			load_default_styles();
		litehtml::element*	add_root();
		litehtml::element*	add_body();
		litehtml::uint_ptr	add_font(const tchar_t* name, int size, const tchar_t* weight, const tchar_t* style, const tchar_t* decoration, font_metrics* fm);

		void begin_parse();

		void parse_tag_start(const tchar_t* tag_name);
		void parse_tag_end(const tchar_t* tag_name);
		void parse_attribute(const tchar_t* attr_name, const tchar_t* attr_value);
		void parse_word(const tchar_t* val);
		void parse_space(const tchar_t* val);
		void parse_comment_start();
		void parse_comment_end();
		void parse_data(const tchar_t* val);
		void parse_push_element(element::ptr el);
		bool parse_pop_element();
		bool parse_pop_element(const tchar_t* tag, const tchar_t* stop_tags = _t(""));
		void parse_pop_empty_element();
		void parse_pop_to_parent(const tchar_t* parents, const tchar_t* stop_parent);
	};

	inline element::ptr document::root()
	{
		return m_root;
	}

}
