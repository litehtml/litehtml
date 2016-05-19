#include "container_linux.h"
#define _USE_MATH_DEFINES
#include <math.h>

#ifndef M_PI
#       define M_PI    3.14159265358979323846
#endif

container_linux::container_linux(void)
{
	m_temp_surface	= cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 2, 2);
	m_temp_cr		= cairo_create(m_temp_surface);
}

container_linux::~container_linux(void)
{
	clear_images();
	cairo_surface_destroy(m_temp_surface);
	cairo_destroy(m_temp_cr);
}

litehtml::uint_ptr container_linux::create_font( const litehtml::tchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm )
{
	litehtml::string_vector fonts;
	litehtml::split_string(faceName, fonts, ",");
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

		int fc_weight = FC_WEIGHT_NORMAL;
		if(weight >= 0 && weight < 150)			fc_weight = FC_WEIGHT_THIN;
		else if(weight >= 150 && weight < 250)	fc_weight = FC_WEIGHT_EXTRALIGHT;
		else if(weight >= 250 && weight < 350)	fc_weight = FC_WEIGHT_LIGHT;
		else if(weight >= 350 && weight < 450)	fc_weight = FC_WEIGHT_NORMAL;
		else if(weight >= 450 && weight < 550)	fc_weight = FC_WEIGHT_MEDIUM;
		else if(weight >= 550 && weight < 650)	fc_weight = FC_WEIGHT_SEMIBOLD;
		else if(weight >= 650 && weight < 750)	fc_weight = FC_WEIGHT_BOLD;
		else if(weight >= 750 && weight < 850)	fc_weight = FC_WEIGHT_EXTRABOLD;
		else if(weight >= 950)					fc_weight = FC_WEIGHT_BLACK;

		FcPatternAddInteger (pattern, FC_WEIGHT, fc_weight);

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
		ret->font		= fnt;
		ret->size		= size;
		ret->strikeout 	= (decoration & litehtml::font_decoration_linethrough) ? true : false;
		ret->underline	= (decoration & litehtml::font_decoration_underline) ? true : false;

	}

	return (litehtml::uint_ptr) ret;
}

void container_linux::delete_font( litehtml::uint_ptr hFont )
{
	cairo_font* fnt = (cairo_font*) hFont;
	if(fnt)
	{
		cairo_font_face_destroy(fnt->font);
		delete fnt;
	}
}

int container_linux::text_width( const litehtml::tchar_t* text, litehtml::uint_ptr hFont )
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

void container_linux::draw_text( litehtml::uint_ptr hdc, const litehtml::tchar_t* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos )
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

int container_linux::pt_to_px( int pt )
{
	GdkScreen* screen = gdk_screen_get_default();
	double dpi = gdk_screen_get_resolution(screen);

	return (int) ((double) pt * dpi / 72.0);
}

int container_linux::get_default_font_size() const
{
	return 16;
}

void container_linux::draw_list_marker( litehtml::uint_ptr hdc, const litehtml::list_marker& marker )
{
	if(!marker.image.empty())
	{
		/*litehtml::tstring url;
		make_url(marker.image.c_str(), marker.baseurl, url);

		lock_images_cache();
		images_map::iterator img_i = m_images.find(url.c_str());
		if(img_i != m_images.end())
		{
			if(img_i->second)
			{
				draw_txdib((cairo_t*) hdc, img_i->second, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height);
			}
		}
		unlock_images_cache();*/
	} else
	{
		switch(marker.marker_type)
		{
		case litehtml::list_style_type_circle:
			{
				draw_ellipse((cairo_t*) hdc, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height, marker.color, 0.5);
			}
			break;
		case litehtml::list_style_type_disc:
			{
				fill_ellipse((cairo_t*) hdc, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height, marker.color);
			}
			break;
		case litehtml::list_style_type_square:
			if(hdc)
			{
				cairo_t* cr = (cairo_t*) hdc;
				cairo_save(cr);

				cairo_new_path(cr);
				cairo_rectangle(cr, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height);

				set_color(cr, marker.color);
				cairo_fill(cr);
				cairo_restore(cr);
			}
			break;
		default:
			/*do nothing*/
			break;
		}
	}
}

void container_linux::load_image( const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, bool redraw_on_ready )
{
	litehtml::tstring url;
	make_url(src, baseurl, url);
	if(m_images.find(url.c_str()) == m_images.end())
	{
		try
		{
			Glib::RefPtr<Gdk::Pixbuf> img = get_image(url.c_str(), true);
			if(img)
			{
				m_images[url.c_str()] = img;
			}
		} catch(...)
		{
			int iii=0;
			iii++;
		}
	}
}

void container_linux::get_image_size( const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz )
{
	litehtml::tstring url;
	make_url(src, baseurl, url);

	images_map::iterator img = m_images.find(url.c_str());
	if(img != m_images.end())
	{
		sz.width	= img->second->get_width();
		sz.height	= img->second->get_height();
	} else
	{
		sz.width	= 0;
		sz.height	= 0;
	}
}

void container_linux::draw_background( litehtml::uint_ptr hdc, const litehtml::background_paint& bg )
{
	cairo_t* cr = (cairo_t*) hdc;
	cairo_save(cr);
	apply_clip(cr);

	rounded_rectangle(cr, bg.border_box, bg.border_radius);
	cairo_clip(cr);

	cairo_rectangle(cr, bg.clip_box.x, bg.clip_box.y, bg.clip_box.width, bg.clip_box.height);
	cairo_clip(cr);

	if(bg.color.alpha)
	{
		set_color(cr, bg.color);
		cairo_paint(cr);
	}

	litehtml::tstring url;
	make_url(bg.image.c_str(), bg.baseurl.c_str(), url);

	//lock_images_cache();
	images_map::iterator img_i = m_images.find(url.c_str());
	if(img_i != m_images.end() && img_i->second)
	{
		Glib::RefPtr<Gdk::Pixbuf> bgbmp = img_i->second;

		Glib::RefPtr<Gdk::Pixbuf> new_img;
		if(bg.image_size.width != bgbmp->get_width() || bg.image_size.height != bgbmp->get_height())
		{
			new_img = bgbmp->scale_simple(bg.image_size.width, bg.image_size.height, Gdk::INTERP_BILINEAR);
			bgbmp = new_img;
		}

		cairo_surface_t* img = surface_from_pixbuf(bgbmp);
		cairo_pattern_t *pattern = cairo_pattern_create_for_surface(img);
		cairo_matrix_t flib_m;
		cairo_matrix_init_identity(&flib_m);
		cairo_matrix_translate(&flib_m, -bg.position_x, -bg.position_y);
		cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
		cairo_pattern_set_matrix (pattern, &flib_m);

		switch(bg.repeat)
		{
		case litehtml::background_repeat_no_repeat:
			draw_pixbuf(cr, bgbmp, bg.position_x, bg.position_y, bgbmp->get_width(), bgbmp->get_height());
			break;

		case litehtml::background_repeat_repeat_x:
			cairo_set_source(cr, pattern);
			cairo_rectangle(cr, bg.clip_box.left(), bg.position_y, bg.clip_box.width, bgbmp->get_height());
			cairo_fill(cr);
			break;

		case litehtml::background_repeat_repeat_y:
			cairo_set_source(cr, pattern);
			cairo_rectangle(cr, bg.position_x, bg.clip_box.top(), bgbmp->get_width(), bg.clip_box.height);
			cairo_fill(cr);
			break;

		case litehtml::background_repeat_repeat:
			cairo_set_source(cr, pattern);
			cairo_rectangle(cr, bg.clip_box.left(), bg.clip_box.top(), bg.clip_box.width, bg.clip_box.height);
			cairo_fill(cr);
			break;
		}

		cairo_pattern_destroy(pattern);
		cairo_surface_destroy(img);

	}
//	unlock_images_cache();
	cairo_restore(cr);
}

void container_linux::make_url(const litehtml::tchar_t* url,	const litehtml::tchar_t* basepath, litehtml::tstring& out)
{
	out = url;
}

void container_linux::add_path_arc(cairo_t* cr, double x, double y, double rx, double ry, double a1, double a2, bool neg)
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

void container_linux::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root)
{
	cairo_t* cr = (cairo_t*) hdc;
	cairo_save(cr);
	apply_clip(cr);

	cairo_new_path(cr);

	int bdr_top		= 0;
	int bdr_bottom	= 0;
	int bdr_left	= 0;
	int bdr_right	= 0;

	if(borders.top.width != 0 && borders.top.style > litehtml::border_style_hidden)
	{
		bdr_top = (int) borders.top.width;
	}
	if(borders.bottom.width != 0 && borders.bottom.style > litehtml::border_style_hidden)
	{
		bdr_bottom = (int) borders.bottom.width;
	}
	if(borders.left.width != 0 && borders.left.style > litehtml::border_style_hidden)
	{
		bdr_left = (int) borders.left.width;
	}
	if(borders.right.width != 0 && borders.right.style > litehtml::border_style_hidden)
	{
		bdr_right = (int) borders.right.width;
	}

	// draw right border
	if(bdr_right)
	{
		set_color(cr, borders.right.color);

		double r_top	= borders.radius.top_right_x;
		double r_bottom	= borders.radius.bottom_right_x;

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

		double r_left	= borders.radius.bottom_left_x;
		double r_right	= borders.radius.bottom_right_x;

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

		double r_left	= borders.radius.top_left_x;
		double r_right	= borders.radius.top_right_x;

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

		double r_top	= borders.radius.top_left_x;
		double r_bottom	= borders.radius.bottom_left_x;

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

void container_linux::transform_text(litehtml::tstring& text, litehtml::text_transform tt)
{

}

void container_linux::set_clip( const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius, bool valid_x, bool valid_y )
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
	m_clips.emplace_back(clip_pos, bdr_radius);
}

void container_linux::del_clip()
{
	if(!m_clips.empty())
	{
		m_clips.pop_back();
	}
}

void container_linux::apply_clip( cairo_t* cr )
{
	for(const auto& clip_box : m_clips)
	{
		rounded_rectangle(cr, clip_box.box, clip_box.radius);
		cairo_clip(cr);
	}
}

void container_linux::draw_ellipse( cairo_t* cr, int x, int y, int width, int height, const litehtml::web_color& color, int line_width )
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

void container_linux::fill_ellipse( cairo_t* cr, int x, int y, int width, int height, const litehtml::web_color& color )
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

void container_linux::clear_images()
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

const litehtml::tchar_t* container_linux::get_default_font_name() const
{
	return "Times New Roman";
}

std::shared_ptr<litehtml::element>	container_linux::create_element(const litehtml::tchar_t *tag_name,
																	  const litehtml::string_map &attributes,
																	  const std::shared_ptr<litehtml::document> &doc)
{
	return 0;
}

void container_linux::rounded_rectangle( cairo_t* cr, const litehtml::position &pos, const litehtml::border_radiuses &radius )
{
	cairo_new_path(cr);
	if(radius.top_left_x)
	{
		cairo_arc(cr, pos.left() + radius.top_left_x, pos.top() + radius.top_left_x, radius.top_left_x, M_PI, M_PI * 3.0 / 2.0);
	} else
	{
		cairo_move_to(cr, pos.left(), pos.top());
	}

	cairo_line_to(cr, pos.right() - radius.top_right_x, pos.top());

	if(radius.top_right_x)
	{
		cairo_arc(cr, pos.right() - radius.top_right_x, pos.top() + radius.top_right_x, radius.top_right_x, M_PI * 3.0 / 2.0, 2.0 * M_PI);
	}

	cairo_line_to(cr, pos.right(), pos.bottom() - radius.bottom_right_x);

	if(radius.bottom_right_x)
	{
		cairo_arc(cr, pos.right() - radius.bottom_right_x, pos.bottom() - radius.bottom_right_x, radius.bottom_right_x, 0, M_PI / 2.0);
	}

	cairo_line_to(cr, pos.left() - radius.bottom_left_x, pos.bottom());

	if(radius.bottom_left_x)
	{
		cairo_arc(cr, pos.left() + radius.bottom_left_x, pos.bottom() - radius.bottom_left_x, radius.bottom_left_x, M_PI / 2.0, M_PI);
	}
}

void container_linux::draw_pixbuf(cairo_t* cr, const Glib::RefPtr<Gdk::Pixbuf>& bmp, int x,	int y, int cx, int cy)
{
	cairo_save(cr);

	{
		Cairo::RefPtr<Cairo::Context> crobj(new Cairo::Context(cr, false));

		cairo_matrix_t flib_m;
		cairo_matrix_init(&flib_m, 1, 0, 0, -1, 0, 0);

		if(cx != bmp->get_width() || cy != bmp->get_height())
		{
			Glib::RefPtr<Gdk::Pixbuf> new_img = bmp->scale_simple(cx, cy, Gdk::INTERP_BILINEAR);;
			Gdk::Cairo::set_source_pixbuf(crobj, new_img, x, y);
			crobj->paint();
		} else
		{
			Gdk::Cairo::set_source_pixbuf(crobj, bmp, x, y);
			crobj->paint();
		}
	}

	cairo_restore(cr);
}

cairo_surface_t* container_linux::surface_from_pixbuf(const Glib::RefPtr<Gdk::Pixbuf>& bmp)
{
	cairo_surface_t* ret = NULL;

	if(bmp->get_has_alpha())
	{
		ret = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, bmp->get_width(), bmp->get_height());
	} else
	{
		ret = cairo_image_surface_create(CAIRO_FORMAT_RGB24, bmp->get_width(), bmp->get_height());
	}

	Cairo::RefPtr<Cairo::Surface> surface(new Cairo::Surface(ret, false));
	Cairo::RefPtr<Cairo::Context> ctx = Cairo::Context::create(surface);
	Gdk::Cairo::set_source_pixbuf(ctx, bmp, 0.0, 0.0);
	ctx->paint();

	return ret;
}

void container_linux::get_media_features(litehtml::media_features& media) const
{
	litehtml::position client;
    get_client_rect(client);
	media.type			= litehtml::media_type_screen;
	media.width			= client.width;
	media.height		= client.height;
	media.device_width	= Gdk::screen_width();
	media.device_height	= Gdk::screen_height();
	media.color			= 8;
	media.monochrome	= 0;
	media.color_index	= 256;
	media.resolution	= 96;
}

void container_linux::get_language(litehtml::tstring& language, litehtml::tstring& culture) const
{
	language = _t("en");
	culture = _t("");
}

void container_linux::link(const std::shared_ptr<litehtml::document> &ptr, const litehtml::element::ptr& el)
{

}
