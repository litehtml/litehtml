#pragma once

namespace litehtml
{
	struct def_color
	{
		const tchar_t*	name;
		const tchar_t*	rgb;
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

		web_color& operator=(const web_color& val)
		{
			blue	= val.blue;
			green	= val.green;
			red		= val.red;
			alpha	= val.alpha;
			return *this;
		}
		static web_color		from_string(const tchar_t* str);
		static const tchar_t*	resolve_name(const tchar_t* name);
		static bool				is_color(const tchar_t* str);
	};
}