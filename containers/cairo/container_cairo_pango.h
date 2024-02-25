#ifndef LITEBROWSER_CONTAINER_CAIRO_PANGO_H
#define LITEBROWSER_CONTAINER_CAIRO_PANGO_H

#include <litehtml.h>
#include "container_cairo.h"
#include <cairo.h>
#include <pango/pangocairo.h>
#include <pango/pango-font.h>

struct cairo_font
{
	PangoFontDescription* font;
	int size;
	bool underline;
	bool strikeout;
	int ascent;
	int descent;
	int underline_thickness;
	int underline_position;
	int strikethrough_thickness;
	int strikethrough_position;
};

class container_cairo_pango : public container_cairo
{
	cairo_surface_t*			m_temp_surface;
	cairo_t*					m_temp_cr;
public:
	container_cairo_pango();
	~container_cairo_pango() override;
	litehtml::uint_ptr create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) override;
	void delete_font(litehtml::uint_ptr hFont) override;
	int text_width(const char* text, litehtml::uint_ptr hFont) override;
	void draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) override;
};

#endif //LITEBROWSER_CONTAINER_CAIRO_PANGO_H
