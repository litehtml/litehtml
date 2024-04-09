#ifndef LH_STYLE_H
#define LH_STYLE_H
#include "css_tokenizer.h"

namespace litehtml
{

	enum property_type
	{
		prop_type_invalid, // indicates "not found" condition in style::get_property
		prop_type_inherit, // "inherit" was specified as the value of this property

		prop_type_enum_item,
		prop_type_enum_item_vector,
		prop_type_length,
		prop_type_length_vector,
		prop_type_number,
		prop_type_color,
		prop_type_bg_image,
		prop_type_string,
		prop_type_string_vector,
		prop_type_size_vector,
		prop_type_custom, // css_token_vector

		prop_type_var, // css_token_vector, needs further parsing because of var()
	};

	class property_value
	{
	public:
		property_type	m_type;
		bool			m_important;

		union {
			int 			m_enum_item;
			int_vector		m_enum_item_vector;
			css_length		m_length;
			length_vector	m_length_vector;
			float			m_number;
			web_color		m_color;
			std::vector<image> m_bg_images;
			string			m_string;
			string_vector	m_string_vector;
			size_vector		m_size_vector;
			css_token_vector m_token_vector;
		};

		property_value()
			: m_type(prop_type_invalid)
		{
		}
		property_value(bool important, property_type type)
			: m_type(type), m_important(important)
		{
		}
		property_value(const css_token_vector& tokens, bool important, property_type type)
			: m_type(type), m_important(important), m_token_vector(tokens)
		{
		}
		property_value(const string& str, bool important)
			: m_type(prop_type_string), m_important(important), m_string(str)
		{
		}
		property_value(const string_vector& vec, bool important)
			: m_type(prop_type_string_vector), m_important(important), m_string_vector(vec)
		{
		}
		property_value(const css_length& length, bool important)
			: m_type(prop_type_length), m_important(important), m_length(length)
		{
		}
		property_value(const length_vector& vec, bool important)
			: m_type(prop_type_length_vector), m_important(important), m_length_vector(vec)
		{
		}
		property_value(float number, bool important)
			: m_type(prop_type_number), m_important(important), m_number(number)
		{
		}
		property_value(int enum_item, bool important)
			: m_type(prop_type_enum_item), m_important(important), m_enum_item(enum_item)
		{
		}
		property_value(const int_vector& vec, bool important)
			: m_type(prop_type_enum_item_vector), m_important(important), m_enum_item_vector(vec)
		{
		}
		property_value(web_color color, bool important)
			: m_type(prop_type_color), m_important(important), m_color(color)
		{
		}
		property_value(const std::vector<image>& images, bool important)
			: m_type(prop_type_bg_image), m_important(important), m_bg_images(images)
		{
		}
		property_value(const size_vector& vec, bool important)
			: m_type(prop_type_size_vector), m_important(important), m_size_vector(vec)
		{
		}
		~property_value()
		{
			switch (m_type)
			{
			case prop_type_string:
				m_string.~string();
				break;
			case prop_type_var:
				m_token_vector.~css_token_vector();
				break;
			case prop_type_string_vector:
				m_string_vector.~string_vector();
				break;
			case prop_type_length:
				m_length.~css_length();
				break;
			case prop_type_length_vector:
				m_length_vector.~length_vector();
				break;
			case prop_type_enum_item_vector:
				m_enum_item_vector.~int_vector();
				break;
			case prop_type_color:
				m_color.~web_color();
				break;
			case prop_type_bg_image:
				m_bg_images.~vector<image>();
				break;
			case prop_type_size_vector:
				m_size_vector.~size_vector();
				break;
			default:
				break;
			}
		}
		property_value& operator=(const property_value& val)
		{
			this->~property_value();

			switch (val.m_type)
			{
			case prop_type_invalid:
				new(this) property_value();
				break;
			case prop_type_inherit:
				new(this) property_value(val.m_important, val.m_type);
				break;
			case prop_type_string:
				new(this) property_value(val.m_string, val.m_important);
				break;
			case prop_type_var:
			case prop_type_custom:
				new(this) property_value(val.m_token_vector, val.m_important, val.m_type);
				break;
			case prop_type_string_vector:
				new(this) property_value(val.m_string_vector, val.m_important);
				break;
			case prop_type_enum_item:
				new(this) property_value(val.m_enum_item, val.m_important);
				break;
			case prop_type_enum_item_vector:
				new(this) property_value(val.m_enum_item_vector, val.m_important);
				break;
			case prop_type_length:
				new(this) property_value(val.m_length, val.m_important);
				break;
			case prop_type_length_vector:
				new(this) property_value(val.m_length_vector, val.m_important);
				break;
			case prop_type_number:
				new(this) property_value(val.m_number, val.m_important);
				break;
			case prop_type_color:
				new(this) property_value(val.m_color, val.m_important);
				break;
			case prop_type_bg_image:
				new(this) property_value(val.m_bg_images, val.m_important);
				break;
			case prop_type_size_vector:
				new(this) property_value(val.m_size_vector, val.m_important);
				break;
			}

			return *this;
		}
	};

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

		void subst_vars(const element* el);

	private:
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
		
		void parse_flex_flow(const css_token_vector& tokens, bool important);
		void parse_flex(const css_token_vector& tokens, bool important);
		void parse_align_self(string_id name, const css_token_vector& tokens, bool important);
		
		void add_parsed_property(string_id name, const property_value& propval);
		void add_length_property(string_id name, css_token val, string keywords, int options, bool important);
		template<class T> void add_four_properties(string_id top_name, T val[4], int n, bool important);
		void remove_property(string_id name, bool important);
	};
	
	bool parse_bg_image(const css_token& token, image& bg_image, document_container* container);
	bool parse_url(const css_token& token, string& url);
	bool parse_bg_position_size(const css_token_vector& tokens, int& index, css_length& x, css_length& y, css_size& size);
	bool parse_bg_size(const css_token_vector& tokens, int& index, css_size& size);
	bool parse_bg_position(const css_token_vector& tokens, int& index, css_length& x, css_length& y, bool convert_keywords_to_percents);
	template<typename Enum>
	bool parse_keyword(const css_token& token, Enum& val, string keywords, int first_keyword_value = 0);
	bool parse_two_lengths(const css_token_vector& tokens, css_length len[2], int options);
	template<class T, class... Args>
	int parse_1234_values(const css_token_vector& tokens, T result[4], bool (*func)(const css_token&, T&, Args...), Args... args);
	int parse_1234_lengths(const css_token_vector& tokens, css_length len[4], int options, string keywords = "");
	bool parse_color(const css_token& tok, web_color& color, document_container* container);
	bool parse_length(const css_token& tok, css_length& length, int options, string keywords = "");
	bool parse_border_width(const css_token& tok, css_length& width);

	bool parse_angle(const css_token& tok, float& angle, bool percents_allowed = false);
	bool parse_linear_gradient_direction(const css_token_vector& tokens, int& index, float& angle, int& side);
	bool parse_linear_gradient_direction_and_interpolation(const css_token_vector& tokens, gradient& gradient);
	bool parse_color_interpolation_method(const css_token_vector& tokens, int& index, color_space_t& color_space, hue_interpolation_t& hue_interpolation);
	bool parse_gradient_position(const css_token_vector& tokens, int& index, gradient& gradient);
	bool parse_radial_gradient_shape_size_position_interpolation(const css_token_vector& tokens, gradient& result);
	bool parse_conic_gradient_angle_position_interpolation(const css_token_vector& tokens, gradient& gradient);
	template<class T>
	bool parse_color_stop_list(const vector<css_token_vector>& list, gradient& grad, document_container* container);
	bool parse_gradient(const css_token& token, gradient& gradient, document_container* container);

} // namespace litehtml

#endif  // LH_STYLE_H
