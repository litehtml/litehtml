#include "container_cairo_pango.h"

container_cairo_pango::container_cairo_pango()
{
	m_temp_surface	= cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 2, 2);
	m_temp_cr		= cairo_create(m_temp_surface);
}

container_cairo_pango::~container_cairo_pango()
{
	clear_images();
	cairo_surface_destroy(m_temp_surface);
	cairo_destroy(m_temp_cr);
}

litehtml::uint_ptr container_cairo_pango::create_font(const char *faceName, int size, int weight, litehtml::font_style italic,
													  unsigned int decoration, litehtml::font_metrics *fm)
{
	PangoFontDescription *desc = pango_font_description_from_string (faceName);
	pango_font_description_set_absolute_size(desc, size * PANGO_SCALE);
	if(italic == litehtml::font_style_italic)
	{
		pango_font_description_set_style(desc, PANGO_STYLE_ITALIC);
	} else
	{
		pango_font_description_set_style(desc, PANGO_STYLE_NORMAL);
	}
	PangoWeight fnt_weight;
	if(weight >= 0 && weight < 150)			fnt_weight = PANGO_WEIGHT_THIN;
	else if(weight >= 150 && weight < 250)	fnt_weight = PANGO_WEIGHT_ULTRALIGHT;
	else if(weight >= 250 && weight < 350)	fnt_weight = PANGO_WEIGHT_LIGHT;
	else if(weight >= 350 && weight < 450)	fnt_weight = PANGO_WEIGHT_NORMAL;
	else if(weight >= 450 && weight < 550)	fnt_weight = PANGO_WEIGHT_MEDIUM;
	else if(weight >= 550 && weight < 650)	fnt_weight = PANGO_WEIGHT_SEMIBOLD;
	else if(weight >= 650 && weight < 750)	fnt_weight = PANGO_WEIGHT_BOLD;
	else if(weight >= 750 && weight < 850)	fnt_weight = PANGO_WEIGHT_ULTRABOLD;
	else fnt_weight = PANGO_WEIGHT_HEAVY;

	pango_font_description_set_weight(desc, fnt_weight);

	cairo_font* ret = nullptr;

	if(fm)
	{
		cairo_save(m_temp_cr);
		PangoLayout *layout = pango_cairo_create_layout(m_temp_cr);
		PangoContext *context = pango_layout_get_context(layout);
		PangoLanguage *language = pango_language_get_default();
		pango_layout_set_font_description(layout, desc);
		PangoFontMetrics *metrics = pango_context_get_metrics(context, desc, language);

		fm->ascent = PANGO_PIXELS((double)pango_font_metrics_get_ascent(metrics));
		fm->descent = PANGO_PIXELS((double)pango_font_metrics_get_descent(metrics));
		fm->height = fm->ascent + fm->descent;
		fm->x_height = fm->height;

		pango_layout_set_text(layout, "x", 1);

		int x_width, x_height;
		pango_layout_get_pixel_size(layout, &x_width, &x_height);

		fm->x_height	= x_height;

		cairo_restore(m_temp_cr);

		g_object_unref(layout);
		pango_font_metrics_unref(metrics);

		ret = new cairo_font;
		ret->font		= desc;
		ret->size		= size;
		ret->strikeout 	= (decoration & litehtml::font_decoration_linethrough) != 0;
		ret->underline	= (decoration & litehtml::font_decoration_underline) != 0;
		ret->ascent     = fm->ascent;
		ret->descent    = fm->descent;

		ret->underline_thickness = pango_font_metrics_get_underline_thickness(metrics);
		ret->underline_position = -pango_font_metrics_get_underline_position(metrics);
		pango_quantize_line_geometry(&ret->underline_thickness, &ret->underline_position);
		ret->underline_thickness = PANGO_PIXELS(ret->underline_thickness);
		ret->underline_position = -1;//PANGO_PIXELS(ret->underline_position);

		ret->strikethrough_thickness = pango_font_metrics_get_strikethrough_thickness(metrics);
		ret->strikethrough_position = pango_font_metrics_get_strikethrough_position(metrics);
		pango_quantize_line_geometry(&ret->strikethrough_thickness, &ret->strikethrough_position);
		ret->strikethrough_thickness = PANGO_PIXELS(ret->strikethrough_thickness);
		ret->strikethrough_position = PANGO_PIXELS(ret->strikethrough_position);
	}

	return (litehtml::uint_ptr) ret;
}

void container_cairo_pango::delete_font(litehtml::uint_ptr hFont)
{
	auto* fnt = (cairo_font*) hFont;
	if(fnt)
	{
		pango_font_description_free(fnt->font);
		delete fnt;
	}
}

int container_cairo_pango::text_width(const char *text, litehtml::uint_ptr hFont)
{
	auto* fnt = (cairo_font*) hFont;

	cairo_save(m_temp_cr);

	PangoLayout *layout = pango_cairo_create_layout(m_temp_cr);
	pango_layout_set_font_description(layout, fnt->font);

	pango_layout_set_text(layout, text, -1);
	pango_cairo_update_layout (m_temp_cr, layout);

	int x_width, x_height;
	pango_layout_get_pixel_size(layout, &x_width, &x_height);

	cairo_restore(m_temp_cr);

	g_object_unref(layout);

	return (int) x_width;
}

void container_cairo_pango::draw_text(litehtml::uint_ptr hdc, const char *text, litehtml::uint_ptr hFont,
									  litehtml::web_color color, const litehtml::position &pos)
{
	auto* fnt = (cairo_font*) hFont;
	auto* cr = (cairo_t*) hdc;
	cairo_save(cr);

	apply_clip(cr);

	set_color(cr, color);

	PangoLayout *layout = pango_cairo_create_layout(cr);
	pango_layout_set_font_description (layout, fnt->font);
	pango_layout_set_text (layout, text, -1);

	int baseline = PANGO_PIXELS(pango_layout_get_baseline(layout));

	PangoRectangle ink_rect, logical_rect;
	pango_layout_get_pixel_extents(layout, &ink_rect, &logical_rect);

	int text_baseline = pos.height - fnt->descent;

	int x = pos.left() + logical_rect.x;
	int y = pos.top() + logical_rect.y + text_baseline - baseline;

	cairo_move_to(cr, x, y);
	pango_cairo_update_layout (cr, layout);
	pango_cairo_show_layout (cr, layout);

	int tw = 0;

	if(fnt->underline || fnt->strikeout)
	{
		tw = text_width(text, hFont);
	}

	if(fnt->underline)
	{
		cairo_set_line_width(cr, fnt->underline_thickness);
		cairo_move_to(cr, x, pos.top() + text_baseline - fnt->underline_position + 0.5);
		cairo_line_to(cr, x + tw, pos.top() + text_baseline - fnt->underline_position + 0.5);
		cairo_stroke(cr);
	}
	if(fnt->strikeout)
	{
		cairo_set_line_width(cr, fnt->strikethrough_thickness);
		cairo_move_to(cr, x, pos.top() + text_baseline - fnt->strikethrough_position - 0.5);
		cairo_line_to(cr, x + tw, pos.top() + text_baseline - fnt->strikethrough_position - 0.5);
		cairo_stroke(cr);
	}

	cairo_restore(cr);

	g_object_unref(layout);
}

