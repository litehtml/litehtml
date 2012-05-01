#pragma once

namespace litehtml
{
	struct attr_color
	{
		unsigned char    rgbBlue;
		unsigned char    rgbGreen;
		unsigned char    rgbRed;
		unsigned char    rgbAlpha;
		attr_color()
		{
			rgbAlpha	= 255;
			rgbBlue		= 0;
			rgbGreen	= 0;
			rgbRed		= 0;
		}
	};

	struct attr_border
	{
		style_border	border;
		int				width;
		attr_color		color;

		attr_border()
		{
			border	= borderNone;
			width	= 0;
		}
	};

/*
	struct attr_font
	{
		std::wstring	family;
		font_size		size;
		int				customSize;
		font_style		fontStyle;
		font_weight		fontWeight;

		attr_font()
		{
			size		= fontSize_nope;
			customSize	= 0;
			fontStyle	= fontStyleNope;
			fontWeight	= fontWeightNope;
		}

		void operator=(const attr_font& val)
		{
			family		= val.family;
			size		= val.size;
			customSize	= val.customSize;
			fontStyle	= val.fontStyle;
			fontWeight	= val.fontWeight;
		}
	};
*/
}