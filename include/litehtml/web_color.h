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

		static const web_color transparent;
		static const web_color black;

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

		string to_string() const;
		static web_color	from_string(const string& str, document_container* callback);
		static string		resolve_name(const string& name, document_container* callback);
		static bool			is_color(const string& str);
	};
}

#endif  // LH_WEB_COLOR_H
