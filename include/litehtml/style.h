#ifndef LH_STYLE_H
#define LH_STYLE_H
#include <variant>

namespace litehtml
{
	struct invalid {}; // indicates "not found" condition in style::get_property
	struct inherit {}; // "inherit" was specified as the value of this property
	using property_value_base = std::variant<
		invalid,
		inherit,
		int,
		int_vector,
		css_length,
		length_vector,
		float,
		web_color,
		std::vector<background_image>,
		string,
		string_vector,
		size_vector
	>;

	struct property_value : property_value_base
	{
		bool m_important = false;
		bool m_has_var   = false; // string; parsing is delayed because of var()

		property_value() {}
		template<class T> property_value(const T& val, bool important, bool has_var = false) 
			: property_value_base(val), m_important(important), m_has_var(has_var) {}

		template<class T> bool is() const { return std::holds_alternative<T>(*this); }
		template<class T> const T& get() const { return std::get<T>(*this); }
	};

	class html_tag;
	typedef std::map<string_id, property_value>	props_map;

	class style
	{
	public:
		typedef std::shared_ptr<style>		ptr;
		typedef std::vector<style::ptr>		vector;
	private:
		props_map							m_properties;
		static std::map<string_id, string>	m_valid_values;
	public:
		void add(const string& txt, const string& baseurl = "", document_container* container = nullptr)
		{
			parse(txt, baseurl, container);
		}

		void add_property(string_id name, const string& val, const string& baseurl = "", bool important = false, document_container* container = nullptr);

		const property_value& get_property(string_id name) const;

		void combine(const style& src);
		void clear()
		{
			m_properties.clear();
		}

		void subst_vars(const html_tag* el);

	private:
		void inherit_property(string_id name, bool important);
		void parse_property(const string& txt, const string& baseurl, document_container* container);
		void parse(const string& txt, const string& baseurl, document_container* container);
		void parse_background(const string& val, const string& baseurl, bool important, document_container* container);
		bool parse_one_background(const string& val, document_container* container, background& bg);
		void parse_background_image(const string& val, document_container* container, const string& baseurl, bool important);
		// parse comma-separated list of keywords
		void parse_keyword_comma_list(string_id name, const string& val, bool important);
		void parse_background_position(const string& val, bool important);
		bool parse_one_background_position(const string& val, css_length& x, css_length& y);
		void parse_background_size(const string& val, bool important);
		bool parse_one_background_size(const string& val, css_size& size);
		void parse_font(const string& val, bool important);
		void parse_flex(const string& val, bool important);
		void parse_align_self(string_id name, const string& val, bool important);
		static css_length parse_border_width(const string& str);
		static void parse_two_lengths(const string& str, css_length len[2]);
		static int parse_four_lengths(const string& str, css_length len[4]);
		static void subst_vars_(string& str, const html_tag* el);

		void add_parsed_property(string_id name, const property_value& propval);
		void remove_property(string_id name, bool important);
	};
}

#endif  // LH_STYLE_H
