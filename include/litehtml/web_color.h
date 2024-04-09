#ifndef LH_WEB_COLOR_H
#define LH_WEB_COLOR_H

namespace litehtml
{
	struct def_color
	{
		const char*	name;
		const char* rgb;
	};

	extern def_color g_def_colors[];

	class document_container;

	struct web_color
	{
		byte red   = 0;
		byte green = 0;
		byte blue  = 0;
		byte alpha = 255;
		bool is_current_color = false;

		static const web_color transparent;
		static const web_color black;
		static const web_color white;
		static const web_color current_color;

		web_color(byte r, byte g, byte b, byte a = 255)
		{
			red		= r;
			green	= g;
			blue	= b;
			alpha	= a;
		}

		web_color() = default;

		web_color(bool is_current_color) : is_current_color(is_current_color) {}


		bool operator==(web_color color) const { return red == color.red && green == color.green && blue == color.blue && alpha == color.alpha; }
		bool operator!=(web_color color) const { return !(*this == color); }

		string to_string() const;
		static web_color	from_string(const string& str, document_container* callback);
		bool from_token(const css_token& token, document_container* container);
		bool parse_hash_token(const css_token& token);
		bool parse_function_token(const css_token& token);
		bool parse_rgb_func(const css_token& tok);
		bool parse_hsl_func(const css_token& tok);
		bool parse_ident_token(const css_token& token, document_container* container);
		static string		resolve_name(const string& name, document_container* callback);
		static bool			is_color(const string& str, document_container* callback);
		web_color darken(double fraction) const
		{
			int v_red = (int) red;
			int v_blue = (int) blue;
			int v_green = (int) green;
			v_red = (int) std::max(v_red - (v_red * fraction), 0.0);
			v_blue = (int) std::max(v_blue - (v_blue * fraction), 0.0);
			v_green = (int) std::max(v_green - (v_green * fraction), 0.0);
			return {(byte) v_red, (byte) v_green, (byte) v_blue, alpha};
		}
	};

	typedef std::vector<web_color>	web_color_vector;
}

#endif  // LH_WEB_COLOR_H
