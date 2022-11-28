#ifndef LH_STYLE_H
#define LH_STYLE_H

namespace litehtml
{
	enum property_type
	{
		prop_type_invalid, // indicates "not found" condition in style::get_property
		prop_type_inherit, // "inherit" was specified as the value of this property

		prop_type_enum_item,
		prop_type_length,
		prop_type_number,
		prop_type_color,
		prop_type_string,

		prop_type_var, // also string, but needs further parsing because of var()
	};

	class property_value
	{
	public:
		property_type	m_type;

		int 			m_enum_item;
		css_length		m_length;
		float			m_number;
		web_color		m_color;
		string			m_string;

		bool			m_important;

		property_value()
		{
			m_type = prop_type_invalid;
		}
		property_value(bool important, property_type type)
		{
			m_type = type;
			m_important = important;
		}
		property_value(const string& str, bool important, property_type type = prop_type_string)
		{
			m_type = type;
			m_string = str;
			m_important = important;
		}
		property_value(const css_length& length, bool important)
		{
			m_type = prop_type_length;
			m_length = length;
			m_important = important;
		}
		property_value(float number, bool important)
		{
			m_type = prop_type_number;
			m_number = number;
			m_important = important;
		}
		property_value(int enum_item, bool important)
		{
			m_type = prop_type_enum_item;
			m_enum_item = enum_item;
			m_important = important;
		}
		property_value(web_color color, bool important)
		{
			m_type = prop_type_color;
			m_color = color;
			m_important = important;
		}
	};

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

		void subst_vars(const element* el);

	private:
		void parse_property(const string& txt, const string& baseurl, document_container* container);
		void parse(const string& txt, const string& baseurl, document_container* container);
		void parse_background(const string& val, const string& baseurl, bool important, document_container* container);
		void parse_background_position(const string& val, bool important);
		void parse_background_size(const string& val, bool important);
		void parse_font(const string& val, bool important);
		void parse_flex(const string& val, bool important);
		static css_length parse_border_width(const string& str);
		static void parse_two_lengths(const string& str, css_length len[2]);
		static int parse_four_lengths(const string& str, css_length len[4]);
		static void subst_vars_(string& str, const element* el);

		void add_parsed_property(string_id name, const property_value& propval);
		void remove_property(string_id name, bool important);
	};
}

#endif  // LH_STYLE_H
