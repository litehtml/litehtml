#include <filesystem>
#include <fontconfig/fontconfig.h>

#include "render2png.h"
#include "fonts.h"

int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		fprintf(stderr, "Usage: %s <html_file> <png_file>", argv[0]);
		return -1;
	}

	auto font_options = prepare_fonts_for_testing();

	html2png::converter converter(800, 600, 96, "serif", font_options);
	auto ret = converter.to_png(argv[1], argv[2]);

	if(font_options)
	{
		cairo_font_options_destroy(font_options);
	}

	if(ret)
	{
		printf("OK. Saved to: %s\n", argv[2]);
		return 0;
	}
	printf("ERROR\n");

	return 1;
}