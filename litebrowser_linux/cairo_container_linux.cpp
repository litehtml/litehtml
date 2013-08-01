#include "cairo_container_linux.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>
#include <cairo.h>
#include <cairo-ft.h>
#include <gdk/gdk.h>

cairo_container_linux::cairo_container_linux(void)
{
	m_temp_surface	= cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 2, 2);
	m_temp_cr		= cairo_create(m_temp_surface);
}

cairo_container_linux::~cairo_container_linux(void)
{
	clear_images();
	cairo_surface_destroy(m_temp_surface);
	cairo_destroy(m_temp_cr);
}

litehtml::uint_ptr cairo_container_linux::create_font( const litehtml::tchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm )
{
	litehtml::string_vector fonts;
	litehtml::tokenize(faceName, fonts, ",");
	litehtml::trim(fonts[0]);

	cairo_font_face_t* fnt = 0;

	FcPattern *pattern = FcPatternCreate();
	bool found = false;
	for(litehtml::string_vector::iterator i = fonts.begin(); i != fonts.end(); i++)
	{
		if(FcPatternAddString(pattern, FC_FAMILY, (unsigned char *) i->c_str()))
		{
			found = true;
			break;
		}
	}
	if(found)
	{
		if(italic == litehtml::fontStyleItalic )
		{
			FcPatternAddInteger (pattern, FC_SLANT, FC_SLANT_ITALIC);
		} else
		{
			FcPatternAddInteger (pattern, FC_SLANT, FC_SLANT_ROMAN);
		}
		FcPatternAddInteger (pattern, FC_WEIGHT, weight);

		fnt = cairo_ft_font_face_create_for_pattern(pattern);
	}

	FcPatternDestroy(pattern);

	cairo_font* ret = 0;

	if(fm && fnt)
	{
		cairo_save(m_temp_cr);

		cairo_set_font_face(m_temp_cr, fnt);
		cairo_set_font_size(m_temp_cr, size);
		cairo_font_extents_t ext;
		cairo_font_extents(m_temp_cr, &ext);

		cairo_text_extents_t tex;
		cairo_text_extents(m_temp_cr, "x", &tex);

		fm->ascent		= (int) ext.ascent;
		fm->descent		= (int) ext.descent;
		fm->height		= (int) (ext.ascent + ext.descent);
		fm->x_height	= (int) tex.height;

		cairo_restore(m_temp_cr);

		ret = new cairo_font;
		ret->font	= fnt;
		ret->size	= size;
	}

	return (litehtml::uint_ptr) ret;
}

void cairo_container_linux::delete_font( litehtml::uint_ptr hFont )
{
	cairo_font* fnt = (cairo_font*) hFont;
	if(fnt)
	{
		cairo_font_face_destroy(fnt->font);
		delete fnt;
	}
}

int cairo_container_linux::text_width( const litehtml::tchar_t* text, litehtml::uint_ptr hFont )
{
	cairo_font* fnt = (cairo_font*) hFont;

	cairo_save(m_temp_cr);

	cairo_set_font_size(m_temp_cr, fnt->size);
	cairo_set_font_face(m_temp_cr, fnt->font);
	cairo_text_extents_t ext;
	cairo_text_extents(m_temp_cr, text, &ext);

	cairo_restore(m_temp_cr);

	return (int) ext.x_advance;
}

void cairo_container_linux::draw_text( litehtml::uint_ptr hdc, const litehtml::tchar_t* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos )
{
	cairo_font* fnt = (cairo_font*) hFont;
	cairo_t* cr		= (cairo_t*) hdc;
	cairo_save(cr);

	apply_clip(cr);

	cairo_set_font_face(cr, fnt->font);
	cairo_set_font_size(cr, fnt->size);
	cairo_font_extents_t ext;
	cairo_font_extents(cr, &ext);

	int x = pos.left();
	int y = pos.bottom()	- ext.descent;

	set_color(cr, color);

	cairo_move_to(cr, x, y);
	cairo_show_text(cr, text);

	int tw = 0;

	if(fnt->underline || fnt->strikeout)
	{
		tw = text_width(text, hFont);
	}

	if(fnt->underline)
	{
		cairo_set_line_width(cr, 1);
		cairo_move_to(cr, x, y + 1.5);
		cairo_line_to(cr, x + tw, y + 1.5);
		cairo_stroke(cr);
	}
	if(fnt->strikeout)
	{
		cairo_text_extents_t tex;
		cairo_text_extents(cr, "x", &tex);

		int ln_y = y - tex.height / 2.0;

		cairo_set_line_width(cr, 1);
		cairo_move_to(cr, x, (double) ln_y - 0.5);
		cairo_line_to(cr, x + tw, (double) ln_y - 0.5);
		cairo_stroke(cr);
	}

	cairo_restore(cr);
}

void cairo_container_linux::fill_rect( litehtml::uint_ptr hdc, const litehtml::position& pos, const litehtml::web_color color, const litehtml::css_border_radius& radius )
{
	if(hdc)
	{
		cairo_t* cr = (cairo_t*) hdc;
		cairo_save(cr);
		apply_clip(cr);

		cairo_new_path(cr);

		if(radius.top_left_x.val())
		{
			cairo_arc(cr, pos.left() + radius.top_left_x.val(), pos.top() + radius.top_left_x.val(), radius.top_left_x.val(), M_PI, M_PI * 3.0 / 2.0);
		} else
		{
			cairo_move_to(cr, pos.left(), pos.top());
		}

		cairo_line_to(cr, pos.right() - radius.top_right_x.val(), pos.top());

		if(radius.top_right_x.val())
		{
			cairo_arc(cr, pos.right() - radius.top_right_x.val(), pos.top() + radius.top_right_x.val(), radius.top_right_x.val(), M_PI * 3.0 / 2.0, 2.0 * M_PI);
		}

		cairo_line_to(cr, pos.right(), pos.bottom() - radius.bottom_right_x.val());

		if(radius.bottom_right_x.val())
		{
			cairo_arc(cr, pos.right() - radius.bottom_right_x.val(), pos.bottom() - radius.bottom_right_x.val(), radius.bottom_right_x.val(), 0, M_PI / 2.0);
		}

		cairo_line_to(cr, pos.left() - radius.bottom_left_x.val(), pos.bottom());

		if(radius.bottom_left_x.val())
		{
			cairo_arc(cr, pos.left() + radius.bottom_left_x.val(), pos.bottom() - radius.bottom_left_x.val(), radius.bottom_left_x.val(), M_PI / 2.0, M_PI);
		}

		set_color(cr, color);
		cairo_fill(cr);
		cairo_restore(cr);
	}
}

int cairo_container_linux::pt_to_px( int pt )
{
	GdkScreen* screen = gdk_screen_get_default();
	double dpi = gdk_screen_get_resolution(screen);

	return (int) ((double) pt * dpi / 72.0);
}

int cairo_container_linux::get_default_font_size()
{
	return 16;
}

void cairo_container_linux::draw_list_marker( litehtml::uint_ptr hdc, litehtml::list_style_type marker_type, int x, int y, int height, const litehtml::web_color& color )
{
	int top_margin = height / 3;

	int draw_x		= x;
	int draw_y		= y + top_margin;
	int draw_width	= height - top_margin * 2;
	int draw_height	= height - top_margin * 2;

	switch(marker_type)
	{
	case litehtml::list_style_type_circle:
		{
			draw_ellipse((cairo_t*) hdc, draw_x, draw_y, draw_width, draw_height, color, 1);
		}
		break;
	case litehtml::list_style_type_disc:
		{
			fill_ellipse((cairo_t*) hdc, draw_x, draw_y, draw_width, draw_height, color);
		}
		break;
	case litehtml::list_style_type_square:
		{
			fill_rect(hdc, litehtml::position(draw_x, draw_y, draw_width, draw_height), color, litehtml::css_border_radius());
		}
		break;
	}
}

void cairo_container_linux::load_image( const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl )
{
/*	litehtml::tstring url;
	make_url(src, baseurl, url);
	if(m_images.find(url.c_str()) == m_images.end())
	{
		CTxDIB* img = get_image(url.c_str());
		if(img)
		{
			m_images[url.c_str()] = img;
		}
	}
*/
}

void cairo_container_linux::get_image_size( const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz )
{
/*	litehtml::tstring url;
	make_url(src, baseurl, url);

	images_map::iterator img = m_images.find(url.c_str());
	if(img != m_images.end())
	{
		sz.width	= img->second->getWidth();
		sz.height	= img->second->getHeight();
	}
*/
	sz.width	= 0;
	sz.height	= 0;
}

void cairo_container_linux::draw_image( litehtml::uint_ptr hdc, const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, const litehtml::position& pos )
{
/*	cairo_t* cr = (cairo_t*) hdc;
	cairo_save(cr);
	apply_clip(cr);

	litehtml::tstring url;
	make_url(src, baseurl, url);
	images_map::iterator img = m_images.find(url.c_str());
	if(img != m_images.end())
	{
		draw_txdib(cr, img->second, pos.x, pos.y, pos.width, pos.height);
	}
	cairo_restore(cr);
*/
}

void cairo_container_linux::draw_background( litehtml::uint_ptr hdc, const litehtml::tchar_t* image, const litehtml::tchar_t* baseurl, const litehtml::position& draw_pos, const litehtml::css_position& bg_pos, litehtml::background_repeat repeat, litehtml::background_attachment attachment )
{
/*	cairo_t* cr = (cairo_t*) hdc;
	cairo_save(cr);
	apply_clip(cr);

	cairo_rectangle(cr, draw_pos.x, draw_pos.y, draw_pos.width, draw_pos.height);
	cairo_clip(cr);

	litehtml::tstring url;
	make_url(image, baseurl, url);

	images_map::iterator img_i = m_images.find(url.c_str());
	if(img_i != m_images.end())
	{
		CTxDIB* bgbmp = img_i->second;

		litehtml::size img_sz;
		img_sz.width	= bgbmp->getWidth();
		img_sz.height	= bgbmp->getHeight();

		int bg_x = draw_pos.x;
		int bg_y = draw_pos.y;


		if(bg_pos.x.units() != litehtml::css_units_percentage)
		{
			bg_x += (int) bg_pos.x.val();
		} else
		{
			bg_x += (int) ((float) (draw_pos.width - img_sz.width) * bg_pos.x.val() / 100.0);
		}

		if(bg_pos.y.units() != litehtml::css_units_percentage)
		{
			bg_y += (int) bg_pos.y.val();
		} else
		{
			bg_y += (int) ( (float) (draw_pos.height - img_sz.height) * bg_pos.y.val() / 100.0);
		}

		cairo_surface_t* img = cairo_image_surface_create_for_data((unsigned char*) bgbmp->getBits(), CAIRO_FORMAT_ARGB32, bgbmp->getWidth(), bgbmp->getHeight(), bgbmp->getWidth() * 4);
		cairo_pattern_t *pattern = cairo_pattern_create_for_surface(img);
		cairo_matrix_t flib_m;
		cairo_matrix_init(&flib_m, 1, 0, 0, -1, 0, 0);
		cairo_matrix_translate(&flib_m, -bg_x, -bg_y);
		cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
		cairo_pattern_set_matrix (pattern, &flib_m);

		switch(repeat)
		{
		case litehtml::background_repeat_no_repeat:
			draw_txdib(cr, bgbmp, bg_x, bg_y, bgbmp->getWidth(), bgbmp->getHeight());
			break;

		case litehtml::background_repeat_repeat_x:
			cairo_set_source(cr, pattern);
			cairo_rectangle(cr, draw_pos.left(), bg_y, draw_pos.width, bgbmp->getHeight());
			cairo_fill(cr);
			break;

		case litehtml::background_repeat_repeat_y:
			cairo_set_source(cr, pattern);
			cairo_rectangle(cr, bg_x, draw_pos.top(), bgbmp->getWidth(), draw_pos.height);
			cairo_fill(cr);
			break;

		case litehtml::background_repeat_repeat:
			cairo_set_source(cr, pattern);
			cairo_rectangle(cr, draw_pos.left(), draw_pos.top(), draw_pos.width, draw_pos.height);
			cairo_fill(cr);
			break;
		}

		cairo_pattern_destroy(pattern);
		cairo_surface_destroy(img);
	}
	cairo_restore(cr);
*/
}

void cairo_container_linux::add_path_arc(cairo_t* cr, double x, double y, double rx, double ry, double a1, double a2, bool neg)
{
	if(rx > 0 && ry > 0)
	{

		cairo_save(cr);

		cairo_translate(cr, x, y);
		cairo_scale(cr, 1, ry / rx);
		cairo_translate(cr, -x, -y);

		if(neg)
		{
			cairo_arc_negative(cr, x, y, rx, a1, a2);
		} else
		{
			cairo_arc(cr, x, y, rx, a1, a2);
		}

		cairo_restore(cr);
	} else
	{
		cairo_move_to(cr, x, y);
	}
}

void cairo_container_linux::draw_borders( litehtml::uint_ptr hdc, const litehtml::css_borders& borders, const litehtml::position& draw_pos )
{
	cairo_t* cr = (cairo_t*) hdc;
	cairo_save(cr);
	apply_clip(cr);

	cairo_new_path(cr);

	int bdr_top		= 0;
	int bdr_bottom	= 0;
	int bdr_left	= 0;
	int bdr_right	= 0;

	if(borders.top.width.val() != 0 && borders.top.style > litehtml::border_style_hidden)
	{
		bdr_top = (int) borders.top.width.val();
	}
	if(borders.bottom.width.val() != 0 && borders.bottom.style > litehtml::border_style_hidden)
	{
		bdr_bottom = (int) borders.bottom.width.val();
	}
	if(borders.left.width.val() != 0 && borders.left.style > litehtml::border_style_hidden)
	{
		bdr_left = (int) borders.left.width.val();
	}
	if(borders.right.width.val() != 0 && borders.right.style > litehtml::border_style_hidden)
	{
		bdr_right = (int) borders.right.width.val();
	}

	// draw right border
	if(bdr_right)
	{
		set_color(cr, borders.right.color);

		double r_top	= borders.radius.top_right_x.val();
		double r_bottom	= borders.radius.bottom_right_x.val();

		if(r_top)
		{
			double end_angle	= 2 * M_PI;
			double start_angle	= end_angle - M_PI / 2.0  / ((double) bdr_top / (double) bdr_right + 1);

			add_path_arc(cr,
				draw_pos.right() - r_top,
				draw_pos.top() + r_top,
				r_top - bdr_right,
				r_top - bdr_right + (bdr_right - bdr_top),
				end_angle,
				start_angle, true);

			add_path_arc(cr,
				draw_pos.right() - r_top,
				draw_pos.top() + r_top,
				r_top,
				r_top,
				start_angle,
				end_angle, false);
		} else
		{
			cairo_move_to(cr, draw_pos.right() - bdr_right, draw_pos.top() + bdr_top);
			cairo_line_to(cr, draw_pos.right(), draw_pos.top());
		}

		if(r_bottom)
		{
			cairo_line_to(cr, draw_pos.right(),	draw_pos.bottom() - r_bottom);

			double start_angle	= 0;
			double end_angle	= start_angle + M_PI / 2.0  / ((double) bdr_bottom / (double) bdr_right + 1);

			add_path_arc(cr,
				draw_pos.right() - r_bottom,
				draw_pos.bottom() - r_bottom,
				r_bottom,
				r_bottom,
				start_angle,
				end_angle, false);

			add_path_arc(cr,
				draw_pos.right() - r_bottom,
				draw_pos.bottom() - r_bottom,
				r_bottom - bdr_right,
				r_bottom - bdr_right + (bdr_right - bdr_bottom),
				end_angle,
				start_angle, true);
		} else
		{
			cairo_line_to(cr, draw_pos.right(),	draw_pos.bottom());
			cairo_line_to(cr, draw_pos.right() - bdr_right,	draw_pos.bottom() - bdr_bottom);
		}

		cairo_fill(cr);
	}

	// draw bottom border
	if(bdr_bottom)
	{
		set_color(cr, borders.bottom.color);

		double r_left	= borders.radius.bottom_left_x.val();
		double r_right	= borders.radius.bottom_right_x.val();

		if(r_left)
		{
			double start_angle	= M_PI / 2.0;
			double end_angle	= start_angle + M_PI / 2.0  / ((double) bdr_left / (double) bdr_bottom + 1);

			add_path_arc(cr,
				draw_pos.left() + r_left,
				draw_pos.bottom() - r_left,
				r_left - bdr_bottom + (bdr_bottom - bdr_left),
				r_left - bdr_bottom,
				start_angle,
				end_angle, false);

			add_path_arc(cr,
				draw_pos.left() + r_left,
				draw_pos.bottom() - r_left,
				r_left,
				r_left,
				end_angle,
				start_angle, true);
		} else
		{
			cairo_move_to(cr, draw_pos.left(), draw_pos.bottom());
			cairo_line_to(cr, draw_pos.left() + bdr_left, draw_pos.bottom() - bdr_bottom);
		}

		if(r_right)
		{
			cairo_line_to(cr, draw_pos.right() - r_right,	draw_pos.bottom());

			double end_angle	= M_PI / 2.0;
			double start_angle	= end_angle - M_PI / 2.0  / ((double) bdr_right / (double) bdr_bottom + 1);

			add_path_arc(cr,
				draw_pos.right() - r_right,
				draw_pos.bottom() - r_right,
				r_right,
				r_right,
				end_angle,
				start_angle, true);

			add_path_arc(cr,
				draw_pos.right() - r_right,
				draw_pos.bottom() - r_right,
				r_right - bdr_bottom + (bdr_bottom - bdr_right),
				r_right - bdr_bottom,
				start_angle,
				end_angle, false);
		} else
		{
			cairo_line_to(cr, draw_pos.right() - bdr_right,	draw_pos.bottom() - bdr_bottom);
			cairo_line_to(cr, draw_pos.right(),	draw_pos.bottom());
		}

		cairo_fill(cr);
	}

	// draw top border
	if(bdr_top)
	{
		set_color(cr, borders.top.color);

		double r_left	= borders.radius.top_left_x.val();
		double r_right	= borders.radius.top_right_x.val();

		if(r_left)
		{
			double end_angle	= M_PI * 3.0 / 2.0;
			double start_angle	= end_angle - M_PI / 2.0  / ((double) bdr_left / (double) bdr_top + 1);

			add_path_arc(cr,
				draw_pos.left() + r_left,
				draw_pos.top() + r_left,
				r_left,
				r_left,
				end_angle,
				start_angle, true);

			add_path_arc(cr,
				draw_pos.left() + r_left,
				draw_pos.top() + r_left,
				r_left - bdr_top + (bdr_top - bdr_left),
				r_left - bdr_top,
				start_angle,
				end_angle, false);
		} else
		{
			cairo_move_to(cr, draw_pos.left(), draw_pos.top());
			cairo_line_to(cr, draw_pos.left() + bdr_left, draw_pos.top() + bdr_top);
		}

		if(r_right)
		{
			cairo_line_to(cr, draw_pos.right() - r_right,	draw_pos.top() + bdr_top);

			double start_angle	= M_PI * 3.0 / 2.0;
			double end_angle	= start_angle + M_PI / 2.0  / ((double) bdr_right / (double) bdr_top + 1);

			add_path_arc(cr,
				draw_pos.right() - r_right,
				draw_pos.top() + r_right,
				r_right - bdr_top + (bdr_top - bdr_right),
				r_right - bdr_top,
				start_angle,
				end_angle, false);

			add_path_arc(cr,
				draw_pos.right() - r_right,
				draw_pos.top() + r_right,
				r_right,
				r_right,
				end_angle,
				start_angle, true);
		} else
		{
			cairo_line_to(cr, draw_pos.right() - bdr_right,	draw_pos.top() + bdr_top);
			cairo_line_to(cr, draw_pos.right(),	draw_pos.top());
		}

		cairo_fill(cr);
	}

	// draw left border
	if(bdr_left)
	{
		set_color(cr, borders.left.color);

		double r_top	= borders.radius.top_left_x.val();
		double r_bottom	= borders.radius.bottom_left_x.val();

		if(r_top)
		{
			double start_angle	= M_PI;
			double end_angle	= start_angle + M_PI / 2.0  / ((double) bdr_top / (double) bdr_left + 1);

			add_path_arc(cr,
				draw_pos.left() + r_top,
				draw_pos.top() + r_top,
				r_top - bdr_left,
				r_top - bdr_left + (bdr_left - bdr_top),
				start_angle,
				end_angle, false);

			add_path_arc(cr,
				draw_pos.left() + r_top,
				draw_pos.top() + r_top,
				r_top,
				r_top,
				end_angle,
				start_angle, true);
		} else
		{
			cairo_move_to(cr, draw_pos.left() + bdr_left, draw_pos.top() + bdr_top);
			cairo_line_to(cr, draw_pos.left(), draw_pos.top());
		}

		if(r_bottom)
		{
			cairo_line_to(cr, draw_pos.left(),	draw_pos.bottom() - r_bottom);

			double end_angle	= M_PI;
			double start_angle	= end_angle - M_PI / 2.0  / ((double) bdr_bottom / (double) bdr_left + 1);

			add_path_arc(cr,
				draw_pos.left() + r_bottom,
				draw_pos.bottom() - r_bottom,
				r_bottom,
				r_bottom,
				end_angle,
				start_angle, true);

			add_path_arc(cr,
				draw_pos.left() + r_bottom,
				draw_pos.bottom() - r_bottom,
				r_bottom - bdr_left,
				r_bottom - bdr_left + (bdr_left - bdr_bottom),
				start_angle,
				end_angle, false);
		} else
		{
			cairo_line_to(cr, draw_pos.left(),	draw_pos.bottom());
			cairo_line_to(cr, draw_pos.left() + bdr_left,	draw_pos.bottom() - bdr_bottom);
		}

		cairo_fill(cr);
	}
	cairo_restore(cr);
}

litehtml::tchar_t cairo_container_linux::toupper( const litehtml::tchar_t c )
{
	return (litehtml::tchar_t) c;
}

litehtml::tchar_t cairo_container_linux::tolower( const litehtml::tchar_t c )
{
	return (litehtml::tchar_t) c;
}

void cairo_container_linux::set_clip( const litehtml::position& pos, bool valid_x, bool valid_y )
{
	litehtml::position clip_pos = pos;
	litehtml::position client_pos;
	get_client_rect(client_pos);
	if(!valid_x)
	{
		clip_pos.x		= client_pos.x;
		clip_pos.width	= client_pos.width;
	}
	if(!valid_y)
	{
		clip_pos.y		= client_pos.y;
		clip_pos.height	= client_pos.height;
	}
	m_clips.push_back(clip_pos);
}

void cairo_container_linux::del_clip()
{
	if(!m_clips.empty())
	{
		m_clips.pop_back();
		if(!m_clips.empty())
		{
			litehtml::position clip_pos = m_clips.back();
		}
	}
}

void cairo_container_linux::apply_clip( cairo_t* cr )
{
	for(litehtml::position::vector::iterator iter = m_clips.begin(); iter != m_clips.end(); iter++)
	{
		cairo_rectangle(cr, iter->x, iter->y, iter->width, iter->height);
		cairo_clip(cr);
	}
}

void cairo_container_linux::draw_ellipse( cairo_t* cr, int x, int y, int width, int height, const litehtml::web_color& color, int line_width )
{
	if(!cr) return;
	cairo_save(cr);

	apply_clip(cr);

	cairo_new_path(cr);

	cairo_translate (cr, x + width / 2.0, y + height / 2.0);
	cairo_scale (cr, width / 2.0, height / 2.0);
	cairo_arc (cr, 0, 0, 1, 0, 2 * M_PI);

	set_color(cr, color);
	cairo_set_line_width(cr, line_width);
	cairo_stroke(cr);

	cairo_restore(cr);
}

void cairo_container_linux::fill_ellipse( cairo_t* cr, int x, int y, int width, int height, const litehtml::web_color& color )
{
	if(!cr) return;
	cairo_save(cr);

	apply_clip(cr);

	cairo_new_path(cr);

	cairo_translate (cr, x + width / 2.0, y + height / 2.0);
	cairo_scale (cr, width / 2.0, height / 2.0);
	cairo_arc (cr, 0, 0, 1, 0, 2 * M_PI);

	set_color(cr, color);
	cairo_fill(cr);

	cairo_restore(cr);
}

void cairo_container_linux::clear_images()
{
/*	for(images_map::iterator i = m_images.begin(); i != m_images.end(); i++)
	{
		if(i->second)
		{
			delete i->second;
		}
	}
	m_images.clear();
*/
}

const litehtml::tchar_t* cairo_container_linux::get_default_font_name()
{
	return "Times New Roman";
}

/*void cairo_container_linux::draw_txdib( cairo_t* cr, CTxDIB* bmp, int x, int y, int cx, int cy )
{
	cairo_save(cr);

	cairo_matrix_t flib_m;
	cairo_matrix_init(&flib_m, 1, 0, 0, -1, 0, 0);

	cairo_surface_t* img = NULL;

	CTxDIB rbmp;

	if(cx != bmp->getWidth() || cy != bmp->getHeight())
	{
		bmp->resample(cx, cy, &rbmp);
		img = cairo_image_surface_create_for_data((unsigned char*) rbmp.getBits(), CAIRO_FORMAT_ARGB32, rbmp.getWidth(), rbmp.getHeight(), rbmp.getWidth() * 4);
		cairo_matrix_translate(&flib_m, 0, -rbmp.getHeight());
		cairo_matrix_translate(&flib_m, x, -y);
	} else
	{
		img = cairo_image_surface_create_for_data((unsigned char*) bmp->getBits(), CAIRO_FORMAT_ARGB32, bmp->getWidth(), bmp->getHeight(), bmp->getWidth() * 4);
		cairo_matrix_translate(&flib_m, 0, -bmp->getHeight());
		cairo_matrix_translate(&flib_m, x, -y);
	}

	cairo_transform(cr, &flib_m);
	cairo_set_source_surface(cr, img, 0, 0);
	cairo_paint(cr);

	cairo_restore(cr);
	cairo_surface_destroy(img);
}
*/
