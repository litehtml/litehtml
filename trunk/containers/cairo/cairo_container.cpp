#include "cairo_container.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "cairo_font.h"

cairo_container::cairo_container(void)
{
	m_temp_surface	= cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 2, 2);
	m_temp_cr		= cairo_create(m_temp_surface);
	m_font_link		= NULL;
	CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_ALL, IID_IMLangFontLink2, (void**) &m_font_link);
	InitializeCriticalSection(&m_img_sync);
}

cairo_container::~cairo_container(void)
{
	clear_images();
	if(m_font_link)
	{
		m_font_link->Release();
	}
	cairo_surface_destroy(m_temp_surface);
	cairo_destroy(m_temp_cr);
	DeleteCriticalSection(&m_img_sync);
}

litehtml::uint_ptr cairo_container::create_font( const litehtml::tchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm )
{
	litehtml::tstring fnt_name = _t("sans-serif");

	litehtml::string_vector fonts;
	litehtml::tokenize(faceName, fonts, L",");
	if(!fonts.empty())
	{
		fnt_name = fonts[0];
		litehtml::trim(fnt_name);
	}

	cairo_font* fnt = new cairo_font(	m_font_link,
										fnt_name.c_str(), 
										size, 
										weight, 
										(italic == litehtml::fontStyleItalic) ? TRUE : FALSE,
										(decoration & litehtml::font_decoration_linethrough) ? TRUE : FALSE,
										(decoration & litehtml::font_decoration_underline) ? TRUE : FALSE);

	if(fm)
	{
		cairo_save(m_temp_cr);

		cairo_font_metrics cfm;
		fnt->get_metrics(m_temp_cr, &cfm);

		fm->ascent		= cfm.ascent;
		fm->descent		= cfm.descent;
		fm->height		= cfm.height;
		fm->x_height	= cfm.x_height;
		if(italic == litehtml::fontStyleItalic || decoration)
		{
			fm->draw_spaces = true;
		} else
		{
			fm->draw_spaces = false;
		}

		cairo_restore(m_temp_cr);
	}

	return (litehtml::uint_ptr) fnt;
}

void cairo_container::delete_font( litehtml::uint_ptr hFont )
{
	cairo_font* fnt = (cairo_font*) hFont;
	if(fnt)
	{
		delete fnt;
	}
}

int cairo_container::text_width( const litehtml::tchar_t* text, litehtml::uint_ptr hFont )
{
	cairo_font* fnt = (cairo_font*) hFont;
	
	cairo_save(m_temp_cr);
	int ret = fnt->text_width(m_temp_cr, text);
	cairo_restore(m_temp_cr);
	return ret;
}

void cairo_container::draw_text( litehtml::uint_ptr hdc, const litehtml::tchar_t* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos )
{
	cairo_font* fnt = (cairo_font*) hFont;
	cairo_t* cr		= (cairo_t*) hdc;
	cairo_save(cr);

	apply_clip(cr);

	cairo_font_metrics cfm;
	fnt->get_metrics(cr, &cfm);

	int x = pos.left();
	int y = pos.bottom()	- cfm.descent;

	set_color(cr, color);
	fnt->show_text(cr, x, y, text);

	cairo_restore(cr);
}

int cairo_container::pt_to_px( int pt )
{
	HDC dc = GetDC(NULL);
	int ret = MulDiv(pt, GetDeviceCaps(dc, LOGPIXELSY), 72);
	ReleaseDC(NULL, dc);
	return ret;
}

int cairo_container::get_default_font_size()
{
	return 16;
}

void cairo_container::draw_list_marker( litehtml::uint_ptr hdc, const litehtml::list_marker& marker )
{
	if(!marker.image.empty())
	{
		litehtml::tstring url;
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
		unlock_images_cache();
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
		}
	}
}

void cairo_container::load_image( const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, bool redraw_on_ready )
{
	litehtml::tstring url;
	make_url(src, baseurl, url);
	lock_images_cache();
	if(m_images.find(url.c_str()) == m_images.end())
	{
		unlock_images_cache();
		CTxDIB* img = get_image(url.c_str(), redraw_on_ready);
		lock_images_cache();
		m_images[url] = img;
		unlock_images_cache();
	} else
	{
		unlock_images_cache();
	}

}

void cairo_container::get_image_size( const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz )
{
	litehtml::tstring url;
	make_url(src, baseurl, url);

	sz.width	= 0;
	sz.height	= 0;

	lock_images_cache();
	images_map::iterator img = m_images.find(url.c_str());
	if(img != m_images.end())
	{
		if(img->second)
		{
			sz.width	= img->second->getWidth();
			sz.height	= img->second->getHeight();
		}
	}
	unlock_images_cache();
}

void cairo_container::draw_image( litehtml::uint_ptr hdc, const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, const litehtml::position& pos )
{
	cairo_t* cr = (cairo_t*) hdc;
	cairo_save(cr);
	apply_clip(cr);

	litehtml::tstring url;
	make_url(src, baseurl, url);
	lock_images_cache();
	images_map::iterator img = m_images.find(url.c_str());
	if(img != m_images.end())
	{
		if(img->second)
		{
			draw_txdib(cr, img->second, pos.x, pos.y, pos.width, pos.height);
		}
	}
	unlock_images_cache();
	cairo_restore(cr);
}

void cairo_container::draw_background( litehtml::uint_ptr hdc, const litehtml::background_paint& bg )
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

	lock_images_cache();
	images_map::iterator img_i = m_images.find(url.c_str());
	if(img_i != m_images.end() && img_i->second)
	{
		CTxDIB* bgbmp = img_i->second;
		
		CTxDIB* new_img = 0;
		if(bg.image_size.width != bgbmp->getWidth() || bg.image_size.height != bgbmp->getHeight())
		{
			new_img = new CTxDIB;
			bgbmp->resample(bg.image_size.width, bg.image_size.height, new_img);
			bgbmp = new_img;
		}


		cairo_surface_t* img = cairo_image_surface_create_for_data((unsigned char*) bgbmp->getBits(), CAIRO_FORMAT_ARGB32, bgbmp->getWidth(), bgbmp->getHeight(), bgbmp->getWidth() * 4);
		cairo_pattern_t *pattern = cairo_pattern_create_for_surface(img);
		cairo_matrix_t flib_m;
		cairo_matrix_init(&flib_m, 1, 0, 0, -1, 0, 0);
		cairo_matrix_translate(&flib_m, -bg.position_x, -bg.position_y);
		cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
		cairo_pattern_set_matrix (pattern, &flib_m);

		switch(bg.repeat)
		{
		case litehtml::background_repeat_no_repeat:
			draw_txdib(cr, bgbmp, bg.position_x, bg.position_y, bgbmp->getWidth(), bgbmp->getHeight());
			break;

		case litehtml::background_repeat_repeat_x:
			cairo_set_source(cr, pattern);
			cairo_rectangle(cr, bg.clip_box.left(), bg.position_y, bg.clip_box.width, bgbmp->getHeight());
			cairo_fill(cr);
			break;

		case litehtml::background_repeat_repeat_y:
			cairo_set_source(cr, pattern);
			cairo_rectangle(cr, bg.position_x, bg.clip_box.top(), bgbmp->getWidth(), bg.clip_box.height);
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

		if(new_img)
		{
			delete new_img;
		}
	}
	unlock_images_cache();
	cairo_restore(cr);
}

void cairo_container::add_path_arc(cairo_t* cr, double x, double y, double rx, double ry, double a1, double a2, bool neg)
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

void cairo_container::draw_borders( litehtml::uint_ptr hdc, const litehtml::css_borders& borders, const litehtml::position& draw_pos )
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

litehtml::tchar_t cairo_container::toupper( const litehtml::tchar_t c )
{
	return (litehtml::tchar_t) CharUpper((LPWSTR) c);
}

litehtml::tchar_t cairo_container::tolower( const litehtml::tchar_t c )
{
	return (litehtml::tchar_t) CharLower((LPWSTR) c);
}

void cairo_container::set_clip( const litehtml::position& pos, bool valid_x, bool valid_y )
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

void cairo_container::del_clip()
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

void cairo_container::apply_clip( cairo_t* cr )
{
	for(litehtml::position::vector::iterator iter = m_clips.begin(); iter != m_clips.end(); iter++)
	{
		cairo_rectangle(cr, iter->x, iter->y, iter->width, iter->height);
		cairo_clip(cr);
	}
}

void cairo_container::draw_ellipse( cairo_t* cr, int x, int y, int width, int height, const litehtml::web_color& color, double line_width )
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

void cairo_container::fill_ellipse( cairo_t* cr, int x, int y, int width, int height, const litehtml::web_color& color )
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

void cairo_container::clear_images()
{
	lock_images_cache();
	for(images_map::iterator i = m_images.begin(); i != m_images.end(); i++)
	{
		if(i->second)
		{
			delete i->second;
		}
	}
	m_images.clear();
	unlock_images_cache();
}

const litehtml::tchar_t* cairo_container::get_default_font_name()
{
	return L"Times New Roman";
}

void cairo_container::draw_txdib( cairo_t* cr, CTxDIB* bmp, int x, int y, int cx, int cy )
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

void cairo_container::rounded_rectangle( cairo_t* cr, const litehtml::position &pos, const litehtml::css_border_radius &radius )
{
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
}

void cairo_container::add_image( litehtml::tstring& url, CTxDIB* img )
{
	lock_images_cache();
	images_map::iterator i = m_images.find(url);
	if(i != m_images.end())
	{
		if(i->second)
		{
			delete i->second;
		}
		i->second = img;
	}
	unlock_images_cache();
}

void cairo_container::lock_images_cache()
{
	EnterCriticalSection(&m_img_sync);
}

void cairo_container::unlock_images_cache()
{
	LeaveCriticalSection(&m_img_sync);
}

bool cairo_container::is_media_valid( const litehtml::tstring& media )
{
	litehtml::string_vector medias;
	litehtml::tokenize(media, medias, L",");
	for(litehtml::string_vector::iterator i = medias.begin(); i != medias.end(); i++)
	{
		litehtml::trim((*i));
		if(i->substr(0, 3) == L"all" || i->substr(0, 6) == L"screen")
		{
			return true;
		}
	}
	return false;
}

litehtml::element* cairo_container::create_element( const litehtml::tchar_t* tag_name )
{
	return 0;
}
