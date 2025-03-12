#ifndef LITEHTML_FONT_DESCRIPTION
#define LITEHTML_FONT_DESCRIPTION

#include <string>
#include "types.h"
#include "css_length.h"
#include "web_color.h"

namespace litehtml
{
	struct font_description
	{
		std::string				family;	// Font Family
		int						size = 0;	// Font size
		font_style				style = font_style_normal;	// Font stype, see the enum font_style
		int						weight;	// Font weight.
		int						decoration_line = text_decoration_line_none;	// Decoration line. A bitset of flags of the enum text_decoration_line
		css_length				decoration_thickness;	// Decoration line thickness in pixels. See predefined values in enumtext_decoration_thickness
		text_decoration_style	decoration_style = text_decoration_style_solid;	// Decoration line style. See enum text_decoration_style
		web_color				decoration_color = web_color::current_color;	// Decoration line color

		std::string	hash() const
		{
			std::string out;
			out += family;
			out += ":sz=" + std::to_string(size);
			out += ":st=" + std::to_string(style);
			out += ":w=" + std::to_string(weight);
			out += ":dl=" + std::to_string(decoration_line);
			out += ":dt=" + decoration_thickness.to_string();
			out += ":ds=" + std::to_string(decoration_style);
			out += ":dc=" + decoration_color.to_string();

			return out;
		}
	};
}

#endif