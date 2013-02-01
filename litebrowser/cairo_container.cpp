#include "globals.h"
#include "cairo_container.h"
#include "..\litehtml\tokenizer.h"
#define _USE_MATH_DEFINES
#include <math.h>

cairo_container::cairo_container(void)
{
	m_hTheme = OpenThemeData(NULL, TEXT("WINDOW"));
	m_hClipRgn = NULL;
}

cairo_container::~cairo_container(void)
{
	if(m_hClipRgn)
	{
		DeleteObject(m_hClipRgn);
	}
	if(m_hTheme)
	{
		CloseThemeData(m_hTheme);
	}
	clear_images();
}

litehtml::uint_ptr cairo_container::create_font( const wchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration )
{
	litehtml::string_vector fonts;
	tokenize(faceName, fonts, L",");
	litehtml::trim(fonts[0]);

	cairo_fnt* fnt = new cairo_fnt(	fonts[0].c_str(), 
									size, 
									weight, 
									(italic == litehtml::fontStyleItalic) ? TRUE : FALSE,
									(decoration & litehtml::font_decoration_linethrough) ? TRUE : FALSE,
									(decoration & litehtml::font_decoration_underline) ? TRUE : FALSE);

	return (litehtml::uint_ptr) fnt;
}

void cairo_container::delete_font( litehtml::uint_ptr hFont )
{
	cairo_fnt* fnt = (cairo_fnt*) hFont;
	if(fnt)
	{
		delete fnt;
	}
}

int cairo_container::line_height( litehtml::uint_ptr hdc, litehtml::uint_ptr hFont )
{
	cairo_fnt* fnt = (cairo_fnt*) hFont;
	cairo_dev cr(get_dib(hdc));
	
	cairo_set_font_face(cr, fnt->fnt());
	cairo_set_font_size(cr, fnt->size());
	cairo_font_extents_t ext;
	cairo_font_extents(cr, &ext);
	
	return (int) ext.height;
}

int cairo_container::text_width( litehtml::uint_ptr hdc, const wchar_t* text, litehtml::uint_ptr hFont )
{
	cairo_fnt* fnt = (cairo_fnt*) hFont;
	cairo_dev cr(get_dib(hdc));

	cairo_set_font_face(cr, fnt->fnt());
	cairo_set_font_size(cr, fnt->size());
	cairo_text_extents_t ext;
	cairo_text_extents(cr, utf8_str(text), &ext);

	return (int) ext.x_advance;
}

void cairo_container::draw_text( litehtml::uint_ptr hdc, const wchar_t* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos )
{
	cairo_fnt* fnt = (cairo_fnt*) hFont;
	cairo_dev cr(get_dib(hdc));
	apply_clip(cr);

	utf8_str str(text);

	cairo_set_font_face(cr, fnt->fnt());
	cairo_set_font_size(cr, fnt->size());

	cairo_text_extents_t te;
	cairo_font_extents_t fe;
	
	cairo_text_extents(cr, str, &te);
	cairo_font_extents(cr, &fe);

	double x = pos.left();
	double y = pos.bottom()	- fe.descent;

	cr.set_color(color);
	cairo_move_to (cr, x, y);
	cairo_show_text(cr, str);
	
	if(fnt->is_underline())
	{
		cairo_set_line_width(cr, 1);
		cairo_move_to(cr, x, y + 1.5);
		cairo_line_to(cr, pos.right(), y + 1.5);
		cairo_stroke(cr);
	}
}

void cairo_container::fill_rect( litehtml::uint_ptr hdc, const litehtml::position& pos, const litehtml::web_color color, const litehtml::css_border_radius& radius )
{
	if(hdc)
	{
		cairo_dev cr( get_dib(hdc) );
		apply_clip(cr);

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

		cr.set_color(color);
		cairo_fill(cr);
	}
}

litehtml::uint_ptr cairo_container::get_temp_dc()
{
	simpledib::dib* ret = new simpledib::dib;
	ret->create(1, 1, true);
	return (litehtml::uint_ptr) ret;
}

void cairo_container::release_temp_dc( litehtml::uint_ptr hdc )
{
	if(hdc)
	{
		delete get_dib(hdc);
	}
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

int cairo_container::get_text_base_line( litehtml::uint_ptr hdc, litehtml::uint_ptr hFont )
{
	simpledib::dib* dib = get_dib(hdc);
	if(!dib)
	{
		dib = new simpledib::dib;
		dib->create(1, 1, true);
	}

	cairo_fnt* fnt = (cairo_fnt*) hFont;
	cairo_dev cr(dib);

	cairo_set_font_face(cr, fnt->fnt());
	cairo_set_font_size(cr, fnt->size());
	cairo_font_extents_t ext;
	cairo_font_extents(cr, &ext);

	if(!hdc)
	{
		delete dib;
	}

	return (int) ext.descent;
}

void cairo_container::draw_list_marker( litehtml::uint_ptr hdc, litehtml::list_style_type marker_type, int x, int y, int height, const litehtml::web_color& color )
{
	apply_clip(get_hdc(hdc));

	int top_margin = height / 3;

	int draw_x		= x;
	int draw_y		= y + top_margin;
	int draw_width	= height - top_margin * 2;
	int draw_height	= height - top_margin * 2;

	switch(marker_type)
	{
	case litehtml::list_style_type_circle:
		{
			draw_ellipse(get_dib(hdc), draw_x, draw_y, draw_width, draw_height, color, 1);
		}
		break;
	case litehtml::list_style_type_disc:
		{
			fill_ellipse(get_dib(hdc), draw_x, draw_y, draw_width, draw_height, color);
		}
		break;
	case litehtml::list_style_type_square:
		{
			fill_rect(hdc, litehtml::position(draw_x, draw_y, draw_width, draw_height), color, litehtml::css_border_radius());
		}
		break;
	}
	release_clip(get_hdc(hdc));
}

void cairo_container::load_image( const wchar_t* src, const wchar_t* baseurl )
{
	std::wstring url;
	make_url(src, baseurl, url);
	if(m_images.find(url.c_str()) == m_images.end())
	{
		CTxDIB* img = get_image(url.c_str());
		if(img)
		{ 
			m_images[url.c_str()] = img;
		}
	}
}

void cairo_container::get_image_size( const wchar_t* src, const wchar_t* baseurl, litehtml::size& sz )
{
	std::wstring url;
	make_url(src, baseurl, url);

	images_map::iterator img = m_images.find(url.c_str());
	if(img != m_images.end())
	{
		sz.width	= img->second->getWidth();
		sz.height	= img->second->getHeight();
	}
}

void cairo_container::draw_image( litehtml::uint_ptr hdc, const wchar_t* src, const wchar_t* baseurl, const litehtml::position& pos )
{
	apply_clip(get_hdc(hdc));

	std::wstring url;
	make_url(src, baseurl, url);
	images_map::iterator img = m_images.find(url.c_str());
	if(img != m_images.end())
	{
		img->second->draw(get_hdc(hdc), pos.x, pos.y, pos.width, pos.height);
	}

	release_clip(get_hdc(hdc));
}

void cairo_container::draw_background( litehtml::uint_ptr hdc, const wchar_t* image, const wchar_t* baseurl, const litehtml::position& draw_pos, const litehtml::css_position& bg_pos, litehtml::background_repeat repeat, litehtml::background_attachment attachment )
{
	apply_clip(get_hdc(hdc));

	std::wstring url;
	make_url(image, baseurl, url);

	images_map::iterator img = m_images.find(url.c_str());
	if(img != m_images.end())
	{
		CTxDIB* bgbmp = img->second;

		litehtml::size img_sz;
		img_sz.width = bgbmp->getWidth();
		img_sz.height	= bgbmp->getHeight();

		litehtml::position pos = draw_pos;

		if(bg_pos.x.units() != litehtml::css_units_percentage)
		{
			pos.x += (int) bg_pos.x.val();
		} else
		{
			pos.x += (int) ((float) (draw_pos.width - img_sz.width) * bg_pos.x.val() / 100.0);
		}

		if(bg_pos.y.units() != litehtml::css_units_percentage)
		{
			pos.y += (int) bg_pos.y.val();
		} else
		{
			pos.y += (int) ( (float) (draw_pos.height - img_sz.height) * bg_pos.y.val() / 100.0);
		}

		int img_width	= bgbmp->getWidth();
		int img_height	= bgbmp->getHeight();

		HRGN oldClip = CreateRectRgn(0, 0, 0, 0);
		if(GetClipRgn(get_hdc(hdc), oldClip) != 1)
		{
			DeleteObject(oldClip);
			oldClip = NULL;
		}

		POINT pt_wnd_org;
		GetWindowOrgEx(get_hdc(hdc), &pt_wnd_org);

		HRGN clipRgn = CreateRectRgn(draw_pos.left() - pt_wnd_org.x, draw_pos.top() - pt_wnd_org.y, draw_pos.right() - pt_wnd_org.x, draw_pos.bottom() - pt_wnd_org.y);
		ExtSelectClipRgn(get_hdc(hdc), clipRgn, RGN_AND);

		switch(repeat)
		{
		case litehtml::background_repeat_no_repeat:
			{
				bgbmp->draw(get_hdc(hdc), pos.x, pos.y);
			}
			break;
		case litehtml::background_repeat_repeat_x:
			{
				for(int x = pos.left(); x < pos.right(); x += bgbmp->getWidth())
				{
					bgbmp->draw(get_hdc(hdc), x, pos.top());
				}

				for(int x = pos.left() - bgbmp->getWidth(); x + (int) bgbmp->getWidth() > draw_pos.left(); x -= bgbmp->getWidth())
				{
					bgbmp->draw(get_hdc(hdc), x, pos.top());
				}
			}
			break;
		case litehtml::background_repeat_repeat_y:
			{
				for(int y = pos.top(); y < pos.bottom(); y += bgbmp->getHeight())
				{
					bgbmp->draw(get_hdc(hdc), pos.left(), y);
				}

				for(int y = pos.top() - bgbmp->getHeight(); y + (int) bgbmp->getHeight() > draw_pos.top(); y -= bgbmp->getHeight())
				{
					bgbmp->draw(get_hdc(hdc), pos.left(), y);
				}
			}
			break;
		case litehtml::background_repeat_repeat:
			{
				if(bgbmp->getHeight() >= 0)
				{
					for(int x = pos.left(); x < pos.right(); x += bgbmp->getWidth())
					{
						for(int y = pos.top(); y < pos.bottom(); y += bgbmp->getHeight())
						{
							bgbmp->draw(get_hdc(hdc), x, y);
						}
					}
				}
			}
			break;
		}

		SelectClipRgn(get_hdc(hdc), oldClip);
		DeleteObject(clipRgn);
		if(oldClip)
		{
			DeleteObject(oldClip);
		}
	}

	release_clip(get_hdc(hdc));
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
	cairo_dev cr(get_dib(hdc));
	apply_clip(cr);

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
		cr.set_color(borders.right.color);

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
		cr.set_color(borders.bottom.color);

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
		cr.set_color(borders.top.color);

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
		cr.set_color(borders.left.color);

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
}

wchar_t cairo_container::toupper( const wchar_t c )
{
	return (wchar_t) CharUpper((LPWSTR) c);
}

wchar_t cairo_container::tolower( const wchar_t c )
{
	return (wchar_t) CharLower((LPWSTR) c);
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

void cairo_container::apply_clip(HDC hdc)
{
	if(m_hClipRgn)
	{
		DeleteObject(m_hClipRgn);
		m_hClipRgn = NULL;
	}

	if(!m_clips.empty())
	{
		POINT ptView = {0, 0};
		GetWindowOrgEx(hdc, &ptView);

		litehtml::position clip_pos = m_clips.back();
		m_hClipRgn = CreateRectRgn(clip_pos.left() - ptView.x, clip_pos.top() - ptView.y, clip_pos.right() - ptView.x, clip_pos.bottom() - ptView.y);
		SelectClipRgn(hdc, m_hClipRgn);
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

void cairo_container::release_clip(HDC hdc)
{
	SelectClipRgn(hdc, NULL);

	if(m_hClipRgn)
	{
		DeleteObject(m_hClipRgn);
		m_hClipRgn = NULL;
	}
}

void cairo_container::draw_ellipse( simpledib::dib* dib, int x, int y, int width, int height, const litehtml::web_color& color, int line_width )
{
	if(!dib) return;

	cairo_dev cr(dib);
	apply_clip(cr);

	cairo_translate (cr, x + width / 2.0, y + height / 2.0);
	cairo_scale (cr, width / 2.0, height / 2.0);
	cairo_arc (cr, 0, 0, 1, 0, 2 * M_PI);

	cr.set_color(color);
	cairo_set_line_width(cr, line_width);
	cairo_stroke(cr);
}

void cairo_container::fill_ellipse( simpledib::dib* dib, int x, int y, int width, int height, const litehtml::web_color& color )
{
	if(!dib) return;

	cairo_dev cr(dib);
	apply_clip(cr);

	cairo_translate (cr, x + width / 2.0, y + height / 2.0);
	cairo_scale (cr, width / 2.0, height / 2.0);
	cairo_arc (cr, 0, 0, 1, 0, 2 * M_PI);

	cr.set_color(color);
	cairo_fill(cr);
}

void cairo_container::clear_images()
{
	for(images_map::iterator i = m_images.begin(); i != m_images.end(); i++)
	{
		if(i->second)
		{
			delete i->second;
		}
	}
	m_images.clear();
}

//////////////////////////////////////////////////////////////////////////

cairo_dev::cairo_dev( simpledib::dib* dib )
{
	m_surface	= cairo_image_surface_create_for_data((unsigned char*) dib->bits(), CAIRO_FORMAT_ARGB32, dib->width(), dib->height(), dib->width() * 4);
	m_cr		= cairo_create(m_surface);

	POINT pt;
	GetWindowOrgEx(dib->hdc(), &pt);
	if(pt.x != 0 || pt.y != 0)
	{
		cairo_translate(m_cr, -pt.x, -pt.y);
	}
}

cairo_dev::~cairo_dev()
{
	cairo_destroy(m_cr);
	cairo_surface_destroy(m_surface);
}

//////////////////////////////////////////////////////////////////////////

cairo_fnt::cairo_fnt( LPCWSTR facename, int size, int weight, BOOL italic, BOOL strikeout, BOOL underline )
{
	m_bStrikeOut	= strikeout;
	m_bUnderline	= underline;
	m_size			= size;

	LOGFONT lf;
	ZeroMemory(&lf, sizeof(lf));
	wcscpy_s(lf.lfFaceName, LF_FACESIZE, facename);

	lf.lfHeight			= 100;
	lf.lfWeight			= weight;
	lf.lfItalic			= italic;
	lf.lfCharSet		= DEFAULT_CHARSET;
	lf.lfOutPrecision	= OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision	= CLIP_DEFAULT_PRECIS;
	lf.lfQuality		= DEFAULT_QUALITY;
	lf.lfStrikeOut		= strikeout;
	lf.lfUnderline		= underline;

	m_font_face = cairo_win32_font_face_create_for_logfontw(&lf);
}

cairo_fnt::~cairo_fnt()
{
	cairo_font_face_destroy(m_font_face);
}

//////////////////////////////////////////////////////////////////////////

utf8_str::utf8_str( LPCWSTR str )
{
	int sz = WideCharToMultiByte(CP_UTF8, 0, str, -1, m_str, 0, NULL, NULL) + 1;
	m_str = new CHAR[sz];
	WideCharToMultiByte(CP_UTF8, 0, str, -1, m_str, sz, NULL, NULL);
}

utf8_str::~utf8_str()
{
	if(m_str) delete m_str;
}
