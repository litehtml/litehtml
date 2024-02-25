#include "container_cairo.h"
#include "cairo_borders.h"
#include <cmath>

#ifndef M_PI
#       define M_PI    3.14159265358979323846
#endif


int container_cairo::pt_to_px(int pt ) const
{
	double dpi = get_screen_dpi();

	return (int) ((double) pt * dpi / 72.0);
}

int container_cairo::get_default_font_size() const
{
	return pt_to_px(12);
}

void container_cairo::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker )
{
	if(!marker.image.empty())
	{
		/*litehtml::string url;
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
				draw_ellipse((cairo_t*) hdc, marker.pos.x, marker.pos.y, marker.pos.width, marker.pos.height, marker.color, 1);
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
				auto* cr = (cairo_t*) hdc;
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

void container_cairo::get_image_size(const char* src, const char* baseurl, litehtml::size& sz )
{
	litehtml::string url;
	make_url(src, baseurl, url);

	auto img = get_image(url);
	if(img)
	{
		sz.width = cairo_image_surface_get_width(img);
		sz.height = cairo_image_surface_get_height(img);
		cairo_surface_destroy(img);
	} else
	{
		sz.width	= 0;
		sz.height	= 0;
	}
}

void container_cairo::draw_background(litehtml::uint_ptr hdc, const std::vector<litehtml::background_paint>& bgvec )
{
	auto* cr = (cairo_t*) hdc;
	cairo_save(cr);
	apply_clip(cr);

	const auto& bg = bgvec.back();

	rounded_rectangle(cr, bg.border_box, bg.border_radius);
	cairo_clip(cr);

	cairo_rectangle(cr, bg.clip_box.x, bg.clip_box.y, bg.clip_box.width, bg.clip_box.height);
	cairo_clip(cr);

	if(bg.color.alpha)
	{
		set_color(cr, bg.color);
		cairo_paint(cr);
	}

	for (int i = (int)bgvec.size() - 1; i >= 0; i--)
	{
		const auto& bg = bgvec[i];

		if(bg.image_size.height == 0 || bg.image_size.width == 0) continue;

		cairo_rectangle(cr, bg.clip_box.x, bg.clip_box.y, bg.clip_box.width, bg.clip_box.height);
		cairo_clip(cr);

		std::string url;
		make_url(bg.image.c_str(), bg.baseurl.c_str(), url);

		auto bgbmp = get_image(url);
		if(bgbmp)
		{
			if(bg.image_size.width != cairo_image_surface_get_width(bgbmp) || bg.image_size.height != cairo_image_surface_get_height(bgbmp))
			{
				auto new_img = scale_surface(bgbmp, bg.image_size.width, bg.image_size.height);
				cairo_surface_destroy(bgbmp);
				bgbmp = new_img;
			}

			cairo_pattern_t *pattern = cairo_pattern_create_for_surface(bgbmp);
			cairo_matrix_t flib_m;
			cairo_matrix_init_identity(&flib_m);
			cairo_matrix_translate(&flib_m, -bg.position_x, -bg.position_y);
			cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
			cairo_pattern_set_matrix (pattern, &flib_m);

			switch(bg.repeat)
			{
			case litehtml::background_repeat_no_repeat:
				draw_pixbuf(cr, bgbmp, bg.position_x, bg.position_y, cairo_image_surface_get_width(bgbmp), cairo_image_surface_get_height(bgbmp));
				break;

			case litehtml::background_repeat_repeat_x:
				cairo_set_source(cr, pattern);
				cairo_rectangle(cr, bg.clip_box.left(), bg.position_y, bg.clip_box.width, cairo_image_surface_get_height(bgbmp));
				cairo_fill(cr);
				break;

			case litehtml::background_repeat_repeat_y:
				cairo_set_source(cr, pattern);
				cairo_rectangle(cr, bg.position_x, bg.clip_box.top(), cairo_image_surface_get_width(bgbmp), bg.clip_box.height);
				cairo_fill(cr);
				break;

			case litehtml::background_repeat_repeat:
				cairo_set_source(cr, pattern);
				cairo_rectangle(cr, bg.clip_box.left(), bg.clip_box.top(), bg.clip_box.width, bg.clip_box.height);
				cairo_fill(cr);
				break;
			}

			cairo_pattern_destroy(pattern);
			cairo_surface_destroy(bgbmp);
		}
	}

	cairo_restore(cr);
}

void container_cairo::make_url(const char* url, const char* /*basepath*/, litehtml::string& out)
{
	out = url;
}

void container_cairo::add_path_arc(cairo_t* cr, double x, double y, double rx, double ry, double a1, double a2, bool neg)
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

void container_cairo::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool /*root*/)
{
	auto* cr = (cairo_t*) hdc;
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
		cairo_matrix_t save_matrix;
		cairo_get_matrix(cr, &save_matrix);
		cairo_translate(cr, draw_pos.left(), draw_pos.top());
		cairo_rotate(cr, M_PI);
		cairo_translate(cr, -draw_pos.left(), -draw_pos.top());

		cairo::border border(cr, draw_pos.left() - draw_pos.width, draw_pos.top() - draw_pos.height, draw_pos.top());
		border.real_side = cairo::border::right_side;
		border.color = borders.right.color;
		border.style = borders.right.style;
		border.border_width = bdr_right;
		border.top_border_width = bdr_bottom;
		border.bottom_border_width = bdr_top;
		border.radius_top_x = borders.radius.bottom_right_x;
		border.radius_top_y = borders.radius.bottom_right_y;
		border.radius_bottom_x = borders.radius.top_right_x;
		border.radius_bottom_y = borders.radius.top_right_y;
		border.draw_border();

		cairo_set_matrix(cr, &save_matrix);
	}

	// draw bottom border
	if(bdr_bottom)
	{
		cairo_matrix_t save_matrix;
		cairo_get_matrix(cr, &save_matrix);
		cairo_translate(cr, draw_pos.left(), draw_pos.top());
		cairo_rotate(cr, - M_PI / 2.0);
		cairo_translate(cr, -draw_pos.left(), -draw_pos.top());

		cairo::border border(cr, draw_pos.left() - draw_pos.height, draw_pos.top(), draw_pos.top() + draw_pos.width);
		border.real_side = cairo::border::bottom_side;
		border.color = borders.bottom.color;
		border.style = borders.bottom.style;
		border.border_width = bdr_bottom;
		border.top_border_width = bdr_left;
		border.bottom_border_width = bdr_right;
		border.radius_top_x = borders.radius.bottom_left_x;
		border.radius_top_y = borders.radius.bottom_left_y;
		border.radius_bottom_x = borders.radius.bottom_right_x;
		border.radius_bottom_y = borders.radius.bottom_right_y;
		border.draw_border();

		cairo_set_matrix(cr, &save_matrix);
	}

	// draw top border
	if(bdr_top)
	{
		cairo_matrix_t save_matrix;
		cairo_get_matrix(cr, &save_matrix);
		cairo_translate(cr, draw_pos.left(), draw_pos.top());
		cairo_rotate(cr, M_PI / 2.0);
		cairo_translate(cr, -draw_pos.left(), -draw_pos.top());

		cairo::border border(cr, draw_pos.left(), draw_pos.top() - draw_pos.width, draw_pos.top());
		border.real_side = cairo::border::top_side;
		border.color = borders.top.color;
		border.style = borders.top.style;
		border.border_width = bdr_top;
		border.top_border_width = bdr_right;
		border.bottom_border_width = bdr_left;
		border.radius_top_x = borders.radius.top_right_x;
		border.radius_top_y = borders.radius.top_right_y;
		border.radius_bottom_x = borders.radius.top_left_x;
		border.radius_bottom_y = borders.radius.top_left_y;
		border.draw_border();

		cairo_set_matrix(cr, &save_matrix);
	}

	// draw left border
	if(bdr_left)
	{
		cairo::border border(cr, draw_pos.left(), draw_pos.top(), draw_pos.bottom());
		border.real_side = cairo::border::left_side;
		border.color = borders.left.color;
		border.style = borders.left.style;
		border.border_width = bdr_left;
		border.top_border_width = bdr_top;
		border.bottom_border_width = bdr_bottom;
		border.radius_top_x = borders.radius.top_left_x;
		border.radius_top_y = borders.radius.top_left_y;
		border.radius_bottom_x = borders.radius.bottom_left_x;
		border.radius_bottom_y = borders.radius.bottom_left_y;
		border.draw_border();
	}
	cairo_restore(cr);
}

void container_cairo::transform_text(litehtml::string& /*text*/, litehtml::text_transform /*tt*/)
{

}

void container_cairo::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius )
{
	m_clips.emplace_back(pos, bdr_radius);
}

void container_cairo::del_clip()
{
	if(!m_clips.empty())
	{
		m_clips.pop_back();
	}
}

void container_cairo::apply_clip(cairo_t* cr )
{
	for(const auto& clip_box : m_clips)
	{
		rounded_rectangle(cr, clip_box.box, clip_box.radius);
		cairo_clip(cr);
	}
}

void container_cairo::draw_ellipse(cairo_t* cr, int x, int y, int width, int height, const litehtml::web_color& color, int line_width )
{
	if(!cr || !width || !height) return;
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

void container_cairo::fill_ellipse(cairo_t* cr, int x, int y, int width, int height, const litehtml::web_color& color )
{
	if(!cr || !width || !height) return;
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

void container_cairo::clear_images()
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

const char* container_cairo::get_default_font_name() const
{
	return "Times New Roman";
}

std::shared_ptr<litehtml::element>	container_cairo::create_element(const char */*tag_name*/,
																	  const litehtml::string_map &/*attributes*/,
																	  const std::shared_ptr<litehtml::document> &/*doc*/)
{
	return nullptr;
}

void container_cairo::rounded_rectangle(cairo_t* cr, const litehtml::position &pos, const litehtml::border_radiuses &radius )
{
	cairo_new_path(cr);
	if(radius.top_left_x && radius.top_left_y)
	{
		add_path_arc(cr,
			 pos.left() + radius.top_left_x,
			 pos.top() + radius.top_left_y,
			 radius.top_left_x,
			 radius.top_left_y,
			 M_PI,
			 M_PI * 3.0 / 2.0, false);
	} else
	{
		cairo_move_to(cr, pos.left(), pos.top());
	}

	cairo_line_to(cr, pos.right() - radius.top_right_x, pos.top());

	if(radius.top_right_x && radius.top_right_y)
	{
		add_path_arc(cr,
			 pos.right() - radius.top_right_x,
			 pos.top() + radius.top_right_y,
			 radius.top_right_x,
			 radius.top_right_y,
			 M_PI * 3.0 / 2.0,
			 2.0 * M_PI, false);
	}

	cairo_line_to(cr, pos.right(), pos.bottom() - radius.bottom_right_x);

	if(radius.bottom_right_x && radius.bottom_right_y)
	{
		add_path_arc(cr,
			 pos.right() - radius.bottom_right_x,
			 pos.bottom() - radius.bottom_right_y,
			 radius.bottom_right_x,
			 radius.bottom_right_y,
			 0,
			 M_PI / 2.0, false);
	}

	cairo_line_to(cr, pos.left() - radius.bottom_left_x, pos.bottom());

	if(radius.bottom_left_x && radius.bottom_left_y)
	{
		add_path_arc(cr,
			 pos.left() + radius.bottom_left_x,
			 pos.bottom() - radius.bottom_left_y,
			 radius.bottom_left_x,
			 radius.bottom_left_y,
			 M_PI / 2.0,
			 M_PI, false);
	}
}

cairo_surface_t* container_cairo::scale_surface(cairo_surface_t* surface, int width, int height)
{
	int s_width = cairo_image_surface_get_width(surface);
	int s_height = cairo_image_surface_get_height(surface);
	cairo_surface_t *result = cairo_surface_create_similar(surface, cairo_surface_get_content(surface), width, height);
	cairo_t *cr = cairo_create(result);
	cairo_scale(cr, (double) width / (double) s_width, (double) height / (double) s_height);
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	cairo_destroy(cr);
	return result;
}

void container_cairo::draw_pixbuf(cairo_t* cr, cairo_surface_t* bmp, int x, int y, int cx, int cy)
{
	cairo_save(cr);

	{
		cairo_matrix_t flip_m;
		cairo_matrix_init(&flip_m, 1, 0, 0, -1, 0, 0);

		if(cx != cairo_image_surface_get_width(bmp) || cy != cairo_image_surface_get_height(bmp))
		{
			auto bmp_scaled = scale_surface(bmp, cx, cy);
			cairo_set_source_surface(cr, bmp_scaled, x, y);
			cairo_paint(cr);
			cairo_surface_destroy(bmp_scaled);
		} else
		{
			cairo_set_source_surface(cr, bmp, x, y);
			cairo_paint(cr);
		}
	}

	cairo_restore(cr);
}

void container_cairo::get_media_features(litehtml::media_features& media) const
{
	litehtml::position client;
    get_client_rect(client);
	media.type			= litehtml::media_type_screen;
	media.width			= client.width;
	media.height		= client.height;
	media.device_width	= get_screen_width();
	media.device_height	= get_screen_height();
	media.color			= 8;
	media.monochrome	= 0;
	media.color_index	= 256;
	media.resolution	= 96;
}

void container_cairo::get_language(litehtml::string& language, litehtml::string& culture) const
{
	language = "en";
	culture = "";
}

void container_cairo::link(const std::shared_ptr<litehtml::document> &/*ptr*/, const litehtml::element::ptr& /*el*/)
{

}
