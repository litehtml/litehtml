#ifndef LH_WEB_COLOR_H
#define LH_WEB_COLOR_H

namespace litehtml
{
	struct def_color
	{
		const char*	name;
		const char*	rgb;
	};

	extern def_color g_def_colors[];

	class document_container;

	struct web_color
	{
		byte	blue;
		byte	green;
		byte	red;
		byte	alpha;

		web_color(byte r, byte g, byte b, byte a = 255)
		{
			blue	= b;
			green	= g;
			red		= r;
			alpha	= a;
		}

		web_color()
		{
			blue	= 0;
			green	= 0;
			red		= 0;
			alpha	= 0xFF;
		}

		web_color(const web_color& val)
		{
			blue	= val.blue;
			green	= val.green;
			red		= val.red;
			alpha	= val.alpha;
		}

		web_color& operator=(const web_color& val)
		{
			blue	= val.blue;
			green	= val.green;
			red		= val.red;
			alpha	= val.alpha;
			return *this;
		}
		string to_string();
		static web_color	from_string(const char* str, document_container* callback);
		static string		resolve_name(const char* name, document_container* callback);
		static bool			is_color(const char* str);
	};
}

#endif  // LH_WEB_COLOR_H
