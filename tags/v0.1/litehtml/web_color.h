#pragma once

namespace litehtml
{
	struct def_color
	{
		const wchar_t*	name;
		const wchar_t*	rgb;
	};

	extern def_color g_def_colors[];

	struct web_color
	{
		byte    blue;
		byte    green;
		byte    red;
		byte    alpha;

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

		void operator=(const web_color& val)
		{
			blue	= val.blue;
			green	= val.green;
			red		= val.red;
			alpha	= val.alpha;
		}
		static web_color		from_string(const wchar_t* str);
		static const wchar_t*	resolve_name(const wchar_t* name);
		static bool				is_color(const wchar_t* str);
	};
}