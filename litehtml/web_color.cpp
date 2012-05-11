#include "html.h"
#include "web_color.h"
#include "tokenizer.h"

litehtml::def_color litehtml::g_def_colors[] = 
{
	{L"transparent",L"rgba(0, 0, 0, 0)"},
	{L"AliceBlue",L"#F0F8FF"},
	{L"AntiqueWhite",L"#FAEBD7"},
	{L"Aqua",L"#00FFFF"},
	{L"Aquamarine",L"#7FFFD4"},
	{L"Azure",L"#F0FFFF"},
	{L"Beige",L"#F5F5DC"},
	{L"Bisque",L"#FFE4C4"},
	{L"Black",L"#000000"},
	{L"BlanchedAlmond",L"#FFEBCD"},
	{L"Blue",L"#0000FF"},
	{L"BlueViolet",L"#8A2BE2"},
	{L"Brown",L"#A52A2A"},
	{L"BurlyWood",L"#DEB887"},
	{L"CadetBlue",L"#5F9EA0"},
	{L"Chartreuse",L"#7FFF00"},
	{L"Chocolate",L"#D2691E"},
	{L"Coral",L"#FF7F50"},
	{L"CornflowerBlue",L"#6495ED"},
	{L"Cornsilk",L"#FFF8DC"},
	{L"Crimson",L"#DC143C"},
	{L"Cyan",L"#00FFFF"},
	{L"DarkBlue",L"#00008B"},
	{L"DarkCyan",L"#008B8B"},
	{L"DarkGoldenRod",L"#B8860B"},
	{L"DarkGray",L"#A9A9A9"},
	{L"DarkGrey",L"#A9A9A9"},
	{L"DarkGreen",L"#006400"},
	{L"DarkKhaki",L"#BDB76B"},
	{L"DarkMagenta",L"#8B008B"},
	{L"DarkOliveGreen",L"#556B2F"},
	{L"Darkorange",L"#FF8C00"},
	{L"DarkOrchid",L"#9932CC"},
	{L"DarkRed",L"#8B0000"},
	{L"DarkSalmon",L"#E9967A"},
	{L"DarkSeaGreen",L"#8FBC8F"},
	{L"DarkSlateBlue",L"#483D8B"},
	{L"DarkSlateGray",L"#2F4F4F"},
	{L"DarkSlateGrey",L"#2F4F4F"},
	{L"DarkTurquoise",L"#00CED1"},
	{L"DarkViolet",L"#9400D3"},
	{L"DeepPink",L"#FF1493"},
	{L"DeepSkyBlue",L"#00BFFF"},
	{L"DimGray",L"#696969"},
	{L"DimGrey",L"#696969"},
	{L"DodgerBlue",L"#1E90FF"},
	{L"FireBrick",L"#B22222"},
	{L"FloralWhite",L"#FFFAF0"},
	{L"ForestGreen",L"#228B22"},
	{L"Fuchsia",L"#FF00FF"},
	{L"Gainsboro",L"#DCDCDC"},
	{L"GhostWhite",L"#F8F8FF"},
	{L"Gold",L"#FFD700"},
	{L"GoldenRod",L"#DAA520"},
	{L"Gray",L"#808080"},
	{L"Grey",L"#808080"},
	{L"Green",L"#008000"},
	{L"GreenYellow",L"#ADFF2F"},
	{L"HoneyDew",L"#F0FFF0"},
	{L"HotPink",L"#FF69B4"},
	{L"Ivory",L"#FFFFF0"},
	{L"Khaki",L"#F0E68C"},
	{L"Lavender",L"#E6E6FA"},
	{L"LavenderBlush",L"#FFF0F5"},
	{L"LawnGreen",L"#7CFC00"},
	{L"LemonChiffon",L"#FFFACD"},
	{L"LightBlue",L"#ADD8E6"},
	{L"LightCoral",L"#F08080"},
	{L"LightCyan",L"#E0FFFF"},
	{L"LightGoldenRodYellow",L"#FAFAD2"},
	{L"LightGray",L"#D3D3D3"},
	{L"LightGrey",L"#D3D3D3"},
	{L"LightGreen",L"#90EE90"},
	{L"LightPink",L"#FFB6C1"},
	{L"LightSalmon",L"#FFA07A"},
	{L"LightSeaGreen",L"#20B2AA"},
	{L"LightSkyBlue",L"#87CEFA"},
	{L"LightSlateGray",L"#778899"},
	{L"LightSlateGrey",L"#778899"},
	{L"LightSteelBlue",L"#B0C4DE"},
	{L"LightYellow",L"#FFFFE0"},
	{L"Lime",L"#00FF00"},
	{L"LimeGreen",L"#32CD32"},
	{L"Linen",L"#FAF0E6"},
	{L"Magenta",L"#FF00FF"},
	{L"Maroon",L"#800000"},
	{L"MediumAquaMarine",L"#66CDAA"},
	{L"MediumBlue",L"#0000CD"},
	{L"MediumOrchid",L"#BA55D3"},
	{L"MediumPurple",L"#9370D8"},
	{L"MediumSeaGreen",L"#3CB371"},
	{L"MediumSlateBlue",L"#7B68EE"},
	{L"MediumSpringGreen",L"#00FA9A"},
	{L"MediumTurquoise",L"#48D1CC"},
	{L"MediumVioletRed",L"#C71585"},
	{L"MidnightBlue",L"#191970"},
	{L"MintCream",L"#F5FFFA"},
	{L"MistyRose",L"#FFE4E1"},
	{L"Moccasin",L"#FFE4B5"},
	{L"NavajoWhite",L"#FFDEAD"},
	{L"Navy",L"#000080"},
	{L"OldLace",L"#FDF5E6"},
	{L"Olive",L"#808000"},
	{L"OliveDrab",L"#6B8E23"},
	{L"Orange",L"#FFA500"},
	{L"OrangeRed",L"#FF4500"},
	{L"Orchid",L"#DA70D6"},
	{L"PaleGoldenRod",L"#EEE8AA"},
	{L"PaleGreen",L"#98FB98"},
	{L"PaleTurquoise",L"#AFEEEE"},
	{L"PaleVioletRed",L"#D87093"},
	{L"PapayaWhip",L"#FFEFD5"},
	{L"PeachPuff",L"#FFDAB9"},
	{L"Peru",L"#CD853F"},
	{L"Pink",L"#FFC0CB"},
	{L"Plum",L"#DDA0DD"},
	{L"PowderBlue",L"#B0E0E6"},
	{L"Purple",L"#800080"},
	{L"Red",L"#FF0000"},
	{L"RosyBrown",L"#BC8F8F"},
	{L"RoyalBlue",L"#4169E1"},
	{L"SaddleBrown",L"#8B4513"},
	{L"Salmon",L"#FA8072"},
	{L"SandyBrown",L"#F4A460"},
	{L"SeaGreen",L"#2E8B57"},
	{L"SeaShell",L"#FFF5EE"},
	{L"Sienna",L"#A0522D"},
	{L"Silver",L"#C0C0C0"},
	{L"SkyBlue",L"#87CEEB"},
	{L"SlateBlue",L"#6A5ACD"},
	{L"SlateGray",L"#708090"},
	{L"SlateGrey",L"#708090"},
	{L"Snow",L"#FFFAFA"},
	{L"SpringGreen",L"#00FF7F"},
	{L"SteelBlue",L"#4682B4"},
	{L"Tan",L"#D2B48C"},
	{L"Teal",L"#008080"},
	{L"Thistle",L"#D8BFD8"},
	{L"Tomato",L"#FF6347"},
	{L"Turquoise",L"#40E0D0"},
	{L"Violet",L"#EE82EE"},
	{L"Wheat",L"#F5DEB3"},
	{L"White",L"#FFFFFF"},
	{L"WhiteSmoke",L"#F5F5F5"},
	{L"Yellow",L"#FFFF00"},
	{L"YellowGreen",L"#9ACD32"},
	{0,0}
};


litehtml::web_color litehtml::web_color::from_string( const wchar_t* str )
{
	if(!str)
	{
		return web_color(0, 0, 0);
	}
	if(str[0] == L'#')
	{
		std::wstring red		= L"";
		std::wstring green		= L"";
		std::wstring blue		= L"";
		if(wcslen(str + 1) == 3)
		{
			red		+= str[1];
			red		+= str[1];
			green	+= str[2];
			green	+= str[2];
			blue	+= str[3];
			blue	+= str[3];
		} else if(wcslen(str + 1) == 6)
		{
			red		+= str[1];
			red		+= str[2];
			green	+= str[3];
			green	+= str[4];
			blue	+= str[5];
			blue	+= str[6];
		}
		wchar_t* sss = 0;
		web_color clr;
		clr.red		= (byte) wcstol(red.c_str(),	&sss, 16);
		clr.green	= (byte) wcstol(green.c_str(),	&sss, 16);
		clr.blue	= (byte) wcstol(blue.c_str(),	&sss, 16);
		return clr;
	} else if(!wcsncmp(str, L"rgb", 3))
	{
		std::wstring s = str;

		std::wstring::size_type pos = s.find_first_of(L"(");
		if(pos != std::wstring::npos)
		{
			s.erase(s.begin(), s.begin() + pos + 1);
		}
		pos = s.find_last_of(L")");
		if(pos != std::wstring::npos)
		{
			s.erase(s.begin() + pos, s.end());
		}

		std::vector<std::wstring> tokens;
		tokenize(s, tokens, L", \t");

		web_color clr;

		if(tokens.size() >= 1)	clr.red		= (byte) _wtoi(tokens[0].c_str());
		if(tokens.size() >= 2)	clr.green	= (byte) _wtoi(tokens[1].c_str());
		if(tokens.size() >= 3)	clr.blue	= (byte) _wtoi(tokens[2].c_str());
		if(tokens.size() >= 4)	clr.alpha	= (byte) _wtoi(tokens[3].c_str());

		return clr;
	} else
	{
		const wchar_t* rgb = resolve_name(str);
		if(rgb)
		{
			return from_string(rgb);
		}
	}
	return web_color(0, 0, 0);
}

const wchar_t* litehtml::web_color::resolve_name( const wchar_t* name )
{
	for(int i=0; g_def_colors[i].name; i++)
	{
		if(!_wcsicmp(name, g_def_colors[i].name))
		{
			return g_def_colors[i].rgb;
		}
	}
	return 0;
}

bool litehtml::web_color::is_color( const wchar_t* str )
{
	if(!_wcsnicmp(str, L"rgb", 3) || str[0] == L'#')
	{
		return true;
	}
	if(resolve_name(str))
	{
		return true;
	}
	return false;
}
