#ifndef LH_STYLE_H
#define LH_STYLE_H

#include "background.h"
#include "css_length.h"
#include "css_position.h"
#include "css_tokenizer.h"
#include "gradient.h"
#include "web_color.h"

namespace litehtml
{
	struct invalid {}; // indicates "not found" condition in style::get_property
	struct inherit {}; // "inherit" was specified as the value of this property

	struct property_value : variant<
		invalid,
		inherit,
		int,
		int_vector,
		css_length,
		length_vector,
		float,
		web_color,
		vector<image>,
		string,
		string_vector,
		size_vector,
		css_token_vector
	>
	{
		bool m_important = false;
		bool m_has_var   = false; // css_token_vector, parsing is delayed because of var()

		property_value() {}
		template<class T> property_value(const T& val, bool important, bool has_var = false)
			: base(val), m_important(important), m_has_var(has_var) {}
	};

	class html_tag;
	typedef std::map<string_id, property_value>	props_map;

	// represents a style block, eg. "color: black; display: inline"
	class style
	{
	public:
		typedef std::shared_ptr<style>		ptr;
		typedef std::vector<style::ptr>		vector;
	private:
		props_map							m_properties;
		static std::map<string_id, string>	m_valid_values;
	public:
		void add(const css_token_vector& tokens, const string& baseurl = "", document_container* container = nullptr);
		void add(const string& txt,              const string& baseurl = "", document_container* container = nullptr);

		void add_property(string_id name, const css_token_vector& tokens, const string& baseurl = "", bool important = false, document_container* container = nullptr);
		void add_property(string_id name, const string& val,              const string& baseurl = "", bool important = false, document_container* container = nullptr);

		const property_value& get_property(string_id name) const;

		void combine(const style& src);
		void clear()
		{
			m_properties.clear();
		}

		void subst_vars(const html_tag* el);

	private:
		void inherit_property(string_id name, bool important);

		void parse_background(const css_token_vector& tokens, const string& baseurl, bool important, document_container* container);
		bool parse_bg_layer(const css_token_vector& tokens, document_container* container, background& bg, bool final_layer);
		// parse the value of background-image property, which is comma-separated list of <bg-image>s
		void parse_background_image(const css_token_vector& tokens, const string& baseurl, bool important, document_container* container);

		// parse comma-separated list of keywords
		void parse_keyword_comma_list(string_id name, const css_token_vector& tokens, bool important);
		void parse_background_position(const css_token_vector& tokens, bool important);
		void parse_background_size(const css_token_vector& tokens, bool important);

		void parse_border(const css_token_vector& tokens, bool important, document_container* container);
		void parse_border_side(string_id name, const css_token_vector& tokens, bool important, document_container* container);
		void parse_border_radius(const css_token_vector& tokens, bool important);

		bool parse_list_style_image(const css_token& tok, string& url);
		void parse_list_style(const css_token_vector& tokens, string baseurl, bool important);

		void parse_font(css_token_vector tokens, bool important);
		void parse_text_decoration(const css_token_vector& tokens, bool important, document_container* container);
		bool parse_text_decoration_color(const css_token& token, bool important, document_container* container);
		void parse_text_decoration_line(const css_token_vector& tokens, bool important);

		void parse_text_emphasis(const css_token_vector& tokens, bool important, document_container* container);
		bool parse_text_emphasis_color(const css_token& token, bool important, document_container* container);
		void parse_text_emphasis_position(const css_token_vector& tokens, bool important);

		void parse_flex_flow(const css_token_vector& tokens, bool important);
		void parse_flex(const css_token_vector& tokens, bool important);
		void parse_align_self(string_id name, const css_token_vector& tokens, bool important);

		void add_parsed_property(string_id name, const property_value& propval);
		void add_length_property(string_id name, css_token val, string keywords, int options, bool important);
		template<class T> void add_four_properties(string_id top_name, T val[4], int n, bool important);
		void remove_property(string_id name, bool important);
	};

	bool parse_url(const css_token& token, string& url);
	bool parse_length(const css_token& tok, css_length& length, int options, string keywords = "");
	bool parse_angle(const css_token& tok, float& angle, bool percents_allowed = false);
	bool parse_bg_position(const css_token_vector& tokens, int& index, css_length& x, css_length& y, bool convert_keywords_to_percents);

	template<typename Enum>
	bool parse_keyword(const css_token& tok, Enum& val, string keywords, int first_keyword_value = 0)
	{
		int	value_index(const string& val, const string& strings, int defValue = -1, char delim = ';');
		int idx = value_index(tok.ident(), keywords);
		if (idx == -1) return false;
		val = (Enum)(first_keyword_value + idx);
		return true;
	}

} // namespace litehtml

#endif  // LH_STYLE_H
