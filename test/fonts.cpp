#include <filesystem>
#include <fontconfig/fontconfig.h>
#include <cairo.h>
#include "fonts.h"

#include <algorithm>

namespace fs = std::filesystem;

static const char* g_fc_config = R"(<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "urn:fontconfig:fonts.dtd">
<fontconfig>
	<alias>
		<family>serif</family>
		<prefer>
			<family>Noto Serif</family>
		</prefer>
	</alias>
	<alias>
		<family>sans-serif</family>
		<prefer>
			<family>Noto Sans</family>
		</prefer>
	</alias>
	<alias>
		<family>fantasy</family>
		<prefer>
			<family>Noto Sans</family>
		</prefer>
	</alias>
	<alias>
		<family>cursive</family>
		<prefer>
			<family>Noto Sans</family>
		</prefer>
	</alias>
	<alias>
		<family>monospace</family>
		<prefer>
			<family>Noto Sans Mono</family>
		</prefer>
	</alias>
</fontconfig>
)";

cairo_font_options_t* prepare_fonts_for_testing()
{
	FcConfig* fc = FcConfigCreate();
	if(FcConfigParseAndLoadFromMemory(fc, (FcChar8*) g_fc_config, FcTrue) != FcTrue)
	{
		printf("ERROR: FcConfigParseAndLoadFromMemory failed\n");
	}

	if(FcConfigSetCurrent(fc) != FcTrue)
	{
		printf("ERROR: FcConfigSetCurrent failed\n");
	}

	for(auto const& dir_entry : std::filesystem::directory_iterator{fs::path(__FILE__).parent_path() / "fonts"})
	{
		auto etx = dir_entry.path().extension().string();
		std::transform(etx.begin(), etx.end(), etx.begin(), [](unsigned char ch) { return std::tolower(ch);});
		if(dir_entry.is_regular_file() && etx == ".ttf")
		{
			FcConfigAppFontAddFile(FcConfigGetCurrent(), (const FcChar8*) dir_entry.path().c_str());
		}
	}

	auto font_options = cairo_font_options_create();
	cairo_font_options_set_antialias(font_options, CAIRO_ANTIALIAS_GRAY);
	cairo_font_options_set_hint_style(font_options, CAIRO_HINT_STYLE_NONE);
	cairo_font_options_set_subpixel_order(font_options, CAIRO_SUBPIXEL_ORDER_RGB);
	return font_options;
}
