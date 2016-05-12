#include "html.h"
#include "web_color.h"
#include "string_hash.h"
#include <string.h>

std::map< litehtml::string_hash, const litehtml::tchar_t *> litehtml::g_def_colors =
{
	{litehtml::string_hash(_t("transparent")),_t("rgba(0, 0, 0, 0)")},
	{litehtml::string_hash(_t("aliceblue")),_t("#F0F8FF")},
	{litehtml::string_hash(_t("antiquewhite")),_t("#FAEBD7")},
	{litehtml::string_hash(_t("aqua")),_t("#00FFFF")},
	{litehtml::string_hash(_t("aquamarine")),_t("#7FFFD4")},
	{litehtml::string_hash(_t("azure")),_t("#F0FFFF")},
	{litehtml::string_hash(_t("beige")),_t("#F5F5DC")},
	{litehtml::string_hash(_t("bisque")),_t("#FFE4C4")},
	{litehtml::string_hash(_t("black")),_t("#000000")},
	{litehtml::string_hash(_t("blanchedalmond")),_t("#FFEBCD")},
	{litehtml::string_hash(_t("blue")),_t("#0000FF")},
	{litehtml::string_hash(_t("blueviolet")),_t("#8A2BE2")},
	{litehtml::string_hash(_t("brown")),_t("#A52A2A")},
	{litehtml::string_hash(_t("burlywood")),_t("#DEB887")},
	{litehtml::string_hash(_t("cadetblue")),_t("#5F9EA0")},
	{litehtml::string_hash(_t("chartreuse")),_t("#7FFF00")},
	{litehtml::string_hash(_t("chocolate")),_t("#D2691E")},
	{litehtml::string_hash(_t("coral")),_t("#FF7F50")},
	{litehtml::string_hash(_t("cornflowerblue")),_t("#6495ED")},
	{litehtml::string_hash(_t("cornsilk")),_t("#FFF8DC")},
	{litehtml::string_hash(_t("crimson")),_t("#DC143C")},
	{litehtml::string_hash(_t("cyan")),_t("#00FFFF")},
	{litehtml::string_hash(_t("darkblue")),_t("#00008B")},
	{litehtml::string_hash(_t("darkcyan")),_t("#008B8B")},
	{litehtml::string_hash(_t("darkgoldenrod")),_t("#B8860B")},
	{litehtml::string_hash(_t("darkgray")),_t("#A9A9A9")},
	{litehtml::string_hash(_t("darkgrey")),_t("#A9A9A9")},
	{litehtml::string_hash(_t("darkgreen")),_t("#006400")},
	{litehtml::string_hash(_t("darkkhaki")),_t("#BDB76B")},
	{litehtml::string_hash(_t("darkmagenta")),_t("#8B008B")},
	{litehtml::string_hash(_t("darkolivegreen")),_t("#556B2F")},
	{litehtml::string_hash(_t("darkorange")),_t("#FF8C00")},
	{litehtml::string_hash(_t("darkorchid")),_t("#9932CC")},
	{litehtml::string_hash(_t("darkred")),_t("#8B0000")},
	{litehtml::string_hash(_t("darksalmon")),_t("#E9967A")},
	{litehtml::string_hash(_t("darkseagreen")),_t("#8FBC8F")},
	{litehtml::string_hash(_t("darkslateblue")),_t("#483D8B")},
	{litehtml::string_hash(_t("darkslategray")),_t("#2F4F4F")},
	{litehtml::string_hash(_t("darkslategrey")),_t("#2F4F4F")},
	{litehtml::string_hash(_t("darkturquoise")),_t("#00CED1")},
	{litehtml::string_hash(_t("darkviolet")),_t("#9400D3")},
	{litehtml::string_hash(_t("deeppink")),_t("#FF1493")},
	{litehtml::string_hash(_t("deepskyblue")),_t("#00BFFF")},
	{litehtml::string_hash(_t("dimgray")),_t("#696969")},
	{litehtml::string_hash(_t("dimgrey")),_t("#696969")},
	{litehtml::string_hash(_t("dodgerblue")),_t("#1E90FF")},
	{litehtml::string_hash(_t("firebrick")),_t("#B22222")},
	{litehtml::string_hash(_t("floralwhite")),_t("#FFFAF0")},
	{litehtml::string_hash(_t("forestgreen")),_t("#228B22")},
	{litehtml::string_hash(_t("fuchsia")),_t("#FF00FF")},
	{litehtml::string_hash(_t("gainsboro")),_t("#DCDCDC")},
	{litehtml::string_hash(_t("ghostwhite")),_t("#F8F8FF")},
	{litehtml::string_hash(_t("gold")),_t("#FFD700")},
	{litehtml::string_hash(_t("goldenrod")),_t("#DAA520")},
	{litehtml::string_hash(_t("gray")),_t("#808080")},
	{litehtml::string_hash(_t("grey")),_t("#808080")},
	{litehtml::string_hash(_t("green")),_t("#008000")},
	{litehtml::string_hash(_t("greenyellow")),_t("#ADFF2F")},
	{litehtml::string_hash(_t("honeydew")),_t("#F0FFF0")},
	{litehtml::string_hash(_t("hotpink")),_t("#FF69B4")},
	{litehtml::string_hash(_t("ivory")),_t("#FFFFF0")},
	{litehtml::string_hash(_t("khaki")),_t("#F0E68C")},
	{litehtml::string_hash(_t("lavender")),_t("#E6E6FA")},
	{litehtml::string_hash(_t("lavenderblush")),_t("#FFF0F5")},
	{litehtml::string_hash(_t("lawngreen")),_t("#7CFC00")},
	{litehtml::string_hash(_t("lemonchiffon")),_t("#FFFACD")},
	{litehtml::string_hash(_t("lightblue")),_t("#ADD8E6")},
	{litehtml::string_hash(_t("lightcoral")),_t("#F08080")},
	{litehtml::string_hash(_t("lightcyan")),_t("#E0FFFF")},
	{litehtml::string_hash(_t("lightgoldenrodyellow")),_t("#FAFAD2")},
	{litehtml::string_hash(_t("lightgray")),_t("#D3D3D3")},
	{litehtml::string_hash(_t("lightgrey")),_t("#D3D3D3")},
	{litehtml::string_hash(_t("lightgreen")),_t("#90EE90")},
	{litehtml::string_hash(_t("lightpink")),_t("#FFB6C1")},
	{litehtml::string_hash(_t("lightsalmon")),_t("#FFA07A")},
	{litehtml::string_hash(_t("lightseagreen")),_t("#20B2AA")},
	{litehtml::string_hash(_t("lightskyblue")),_t("#87CEFA")},
	{litehtml::string_hash(_t("lightslategray")),_t("#778899")},
	{litehtml::string_hash(_t("lightslategrey")),_t("#778899")},
	{litehtml::string_hash(_t("lightsteelblue")),_t("#B0C4DE")},
	{litehtml::string_hash(_t("lightyellow")),_t("#FFFFE0")},
	{litehtml::string_hash(_t("lime")),_t("#00FF00")},
	{litehtml::string_hash(_t("limegreen")),_t("#32CD32")},
	{litehtml::string_hash(_t("linen")),_t("#FAF0E6")},
	{litehtml::string_hash(_t("magenta")),_t("#FF00FF")},
	{litehtml::string_hash(_t("maroon")),_t("#800000")},
	{litehtml::string_hash(_t("mediumaquamarine")),_t("#66CDAA")},
	{litehtml::string_hash(_t("mediumblue")),_t("#0000CD")},
	{litehtml::string_hash(_t("mediumorchid")),_t("#BA55D3")},
	{litehtml::string_hash(_t("mediumpurple")),_t("#9370D8")},
	{litehtml::string_hash(_t("mediumseagreen")),_t("#3CB371")},
	{litehtml::string_hash(_t("mediumslateblue")),_t("#7B68EE")},
	{litehtml::string_hash(_t("mediumspringgreen")),_t("#00FA9A")},
	{litehtml::string_hash(_t("mediumturquoise")),_t("#48D1CC")},
	{litehtml::string_hash(_t("mediumvioletred")),_t("#C71585")},
	{litehtml::string_hash(_t("midnightblue")),_t("#191970")},
	{litehtml::string_hash(_t("mintcream")),_t("#F5FFFA")},
	{litehtml::string_hash(_t("mistyrose")),_t("#FFE4E1")},
	{litehtml::string_hash(_t("moccasin")),_t("#FFE4B5")},
	{litehtml::string_hash(_t("navajowhite")),_t("#FFDEAD")},
	{litehtml::string_hash(_t("navy")),_t("#000080")},
	{litehtml::string_hash(_t("oldlace")),_t("#FDF5E6")},
	{litehtml::string_hash(_t("olive")),_t("#808000")},
	{litehtml::string_hash(_t("olivedrab")),_t("#6B8E23")},
	{litehtml::string_hash(_t("orange")),_t("#FFA500")},
	{litehtml::string_hash(_t("orangered")),_t("#FF4500")},
	{litehtml::string_hash(_t("orchid")),_t("#DA70D6")},
	{litehtml::string_hash(_t("palegoldenrod")),_t("#EEE8AA")},
	{litehtml::string_hash(_t("palegreen")),_t("#98FB98")},
	{litehtml::string_hash(_t("paleturquoise")),_t("#AFEEEE")},
	{litehtml::string_hash(_t("palevioletred")),_t("#D87093")},
	{litehtml::string_hash(_t("papayawhip")),_t("#FFEFD5")},
	{litehtml::string_hash(_t("peachpuff")),_t("#FFDAB9")},
	{litehtml::string_hash(_t("peru")),_t("#CD853F")},
	{litehtml::string_hash(_t("pink")),_t("#FFC0CB")},
	{litehtml::string_hash(_t("plum")),_t("#DDA0DD")},
	{litehtml::string_hash(_t("powderblue")),_t("#B0E0E6")},
	{litehtml::string_hash(_t("purple")),_t("#800080")},
	{litehtml::string_hash(_t("red")),_t("#FF0000")},
	{litehtml::string_hash(_t("rosybrown")),_t("#BC8F8F")},
	{litehtml::string_hash(_t("royalblue")),_t("#4169E1")},
	{litehtml::string_hash(_t("saddlebrown")),_t("#8B4513")},
	{litehtml::string_hash(_t("salmon")),_t("#FA8072")},
	{litehtml::string_hash(_t("sandybrown")),_t("#F4A460")},
	{litehtml::string_hash(_t("seagreen")),_t("#2E8B57")},
	{litehtml::string_hash(_t("seashell")),_t("#FFF5EE")},
	{litehtml::string_hash(_t("sienna")),_t("#A0522D")},
	{litehtml::string_hash(_t("silver")),_t("#C0C0C0")},
	{litehtml::string_hash(_t("skyblue")),_t("#87CEEB")},
	{litehtml::string_hash(_t("slateblue")),_t("#6A5ACD")},
	{litehtml::string_hash(_t("slategray")),_t("#708090")},
	{litehtml::string_hash(_t("slategrey")),_t("#708090")},
	{litehtml::string_hash(_t("snow")),_t("#FFFAFA")},
	{litehtml::string_hash(_t("springgreen")),_t("#00FF7F")},
	{litehtml::string_hash(_t("steelblue")),_t("#4682B4")},
	{litehtml::string_hash(_t("tan")),_t("#D2B48C")},
	{litehtml::string_hash(_t("teal")),_t("#008080")},
	{litehtml::string_hash(_t("thistle")),_t("#D8BFD8")},
	{litehtml::string_hash(_t("tomato")),_t("#FF6347")},
	{litehtml::string_hash(_t("turquoise")),_t("#40E0D0")},
	{litehtml::string_hash(_t("violet")),_t("#EE82EE")},
	{litehtml::string_hash(_t("wheat")),_t("#F5DEB3")},
	{litehtml::string_hash(_t("white")),_t("#FFFFFF")},
	{litehtml::string_hash(_t("whitesmoke")),_t("#F5F5F5")},
	{litehtml::string_hash(_t("yellow")),_t("#FFFF00")},
	{litehtml::string_hash(_t("yellowgreen")),_t("#9ACD32")}
};


litehtml::web_color litehtml::web_color::from_string( const tchar_t* str )
{
	if(!str)
	{
		return web_color(0, 0, 0);
	}
	if(str[0] == _t('#'))
	{
		tstring red		= _t("");
		tstring green		= _t("");
		tstring blue		= _t("");
		if(t_strlen(str + 1) == 3)
		{
			red		+= str[1];
			red		+= str[1];
			green	+= str[2];
			green	+= str[2];
			blue	+= str[3];
			blue	+= str[3];
		} else if(t_strlen(str + 1) == 6)
		{
			red		+= str[1];
			red		+= str[2];
			green	+= str[3];
			green	+= str[4];
			blue	+= str[5];
			blue	+= str[6];
		}
		tchar_t* sss = 0;
		web_color clr;
		clr.red		= (byte) t_strtol(red.c_str(),	&sss, 16);
		clr.green	= (byte) t_strtol(green.c_str(),	&sss, 16);
		clr.blue	= (byte) t_strtol(blue.c_str(),	&sss, 16);
		return clr;
	} else if(!t_strncmp(str, _t("rgb"), 3))
	{
		tstring s = str;

		tstring::size_type pos = s.find_first_of(_t("("));
		if(pos != tstring::npos)
		{
			s.erase(s.begin(), s.begin() + pos + 1);
		}
		pos = s.find_last_of(_t(")"));
		if(pos != tstring::npos)
		{
			s.erase(s.begin() + pos, s.end());
		}

		std::vector<tstring> tokens;
		split_string(s, tokens, _t(", \t"));

		web_color clr;

		if(tokens.size() >= 1)	clr.red		= (byte) t_atoi(tokens[0].c_str());
		if(tokens.size() >= 2)	clr.green	= (byte) t_atoi(tokens[1].c_str());
		if(tokens.size() >= 3)	clr.blue	= (byte) t_atoi(tokens[2].c_str());
		if(tokens.size() >= 4)	clr.alpha	= (byte) (t_strtod(tokens[3].c_str(), 0) * 255.0);

		return clr;
	} else
	{
		const tchar_t* rgb = resolve_name(str);
		if(rgb)
		{
			return from_string(rgb);
		}
	}
	return web_color(0, 0, 0);
}

const litehtml::tchar_t* litehtml::web_color::resolve_name( const tchar_t* name )
{
	auto
		result = g_def_colors.find(string_hash::normalizeFromString(name) );

	if ( result != g_def_colors.end() )
	{
		return result->second;
	}
	return 0;
}

bool litehtml::web_color::is_color( const tchar_t* str )
{
	if(!t_strncasecmp(str, _t("rgb"), 3) || str[0] == _t('#'))
	{
		return true;
	}
	if(resolve_name(str))
	{
		return true;
	}
	return false;
}
