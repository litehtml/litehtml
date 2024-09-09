#include "render2png.h"
#include "container_cairo_pango.h"
#include <filesystem>
#include <gdk/gdk.h>
#include <cairo.h>
#include <fontconfig/fontconfig.h>
#include <sstream>
#include <fstream>
#include "cairo_images_cache.h"

namespace html2png
{
	class container : public container_cairo_pango
	{
		std::string m_base_path;
		converter* m_converter;
		cairo_images_cache m_images;
	public:
		explicit container(const std::string& base_path, converter* cvt) : m_base_path(base_path), m_converter(cvt)
		{
		}

		void load_image([[maybe_unused]] const char *src, [[maybe_unused]] const char *baseurl, [[maybe_unused]] bool redraw_on_ready) override {}
		void set_caption([[maybe_unused]] const char *caption) override {}

		cairo_surface_t *get_image(const std::string &url) override
		{
			if(url.empty())
				return nullptr;

			cairo_surface_t* image_surface = m_images.get_image(url);

			if(!image_surface)
			{
				auto pixbuf = gdk_pixbuf_new_from_file(url.c_str(), nullptr);

				if (pixbuf)
				{
					if (gdk_pixbuf_get_has_alpha(pixbuf))
					{
						image_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, gdk_pixbuf_get_width(pixbuf),
																   gdk_pixbuf_get_height(pixbuf));
					} else
					{
						image_surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, gdk_pixbuf_get_width(pixbuf),
																   gdk_pixbuf_get_height(pixbuf));
					}
					if(image_surface)
					{
						auto cr = cairo_create(image_surface);
						gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
						cairo_paint(cr);
						cairo_destroy(cr);
						m_images.add_image(url, image_surface);
						// add_image doesn't increase reference. We must do it manually
						image_surface = cairo_surface_reference(image_surface);
					}

					g_object_unref(pixbuf);
				}
			}

			return image_surface;
		}

		double get_screen_dpi() const override { return m_converter->get_dpi(); }
		int get_screen_width() const override { return m_converter->get_screen_width(); }
		int get_screen_height() const override { return m_converter->get_screen_height(); }
		void set_base_url([[maybe_unused]] const char *base_url) override
		{
			if(base_url)
			{
				m_base_path = base_url;
			}
		}
		void on_anchor_click([[maybe_unused]] const char *url, [[maybe_unused]] const litehtml::element::ptr &el) override {}
		void on_mouse_event([[maybe_unused]] const litehtml::element::ptr &el, [[maybe_unused]] litehtml::mouse_event event) override {}
		void set_cursor([[maybe_unused]] const char *cursor) override {}
		void import_css([[maybe_unused]] std::string &text, [[maybe_unused]] const std::string &url, [[maybe_unused]] std::string &baseurl) override
		{
			std::string path;
			make_url(url.c_str(), baseurl.c_str(), path);

			std::stringstream ss;
			std::ifstream(path, std::ios::binary) >> ss.rdbuf();
			text = ss.str();
		}

		void get_client_rect(litehtml::position &client) const override
		{
			client.width = m_converter->get_screen_width();
			client.height = m_converter->get_screen_height();
			client.x = 0;
			client.y = 0;
		}

		const char* get_default_font_name() const override { return m_converter->get_default_font(); }
		cairo_font_options_t* get_font_options() override { return m_converter->get_font_options(); }

		void make_url(const char *url, const char *basepath, std::string &out) override
		{
			if(basepath && *basepath)
			{
				out = std::filesystem::path(basepath) / url;
			} else if(!m_base_path.empty())
			{
				out = std::filesystem::path(m_base_path) / url;
			} else
			{
				out = url;
			}
		}
	};

	converter::converter(int screen_width /*= 800*/, int screen_height /*= 600*/, double dpi /*= 96*/, const char* default_font /*= nullptr*/, cairo_font_options_t* font_options /*= nullptr*/)
	{
		m_screen_width = screen_width;
		m_screen_height = screen_height;
		m_dpi = dpi;
		m_font_options = font_options;
		if(default_font)
		{
			m_default_font = default_font;
		} else
		{
			m_default_font = "serif";
		}
	}

	GdkPixbuf* converter::to_pixbuf(const std::string &html_file)
	{
		std::stringstream ss;
		std::ifstream(html_file, std::ios::binary) >> ss.rdbuf();

		std::string base_path = std::filesystem::path(html_file).parent_path().string();

		int width, height;
		container cont(base_path, this);
		auto doc = litehtml::document::createFromString(ss.str(), &cont);
		doc->render(m_screen_width);

		width = doc->content_width();
		height = doc->content_height();

		auto surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
		if(surface)
		{
			auto cr = cairo_create(surface);

			// Fill background with white color
			cairo_save(cr);
			cairo_rectangle(cr, 0, 0, width, height);
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
			cairo_fill(cr);
			cairo_restore(cr);

			// Draw document
			doc->draw((litehtml::uint_ptr) cr, 0, 0, nullptr);

			cairo_surface_flush(surface);
			cairo_destroy(cr);

			GdkPixbuf* pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, width, height);
			cairo_surface_destroy(surface);
			return pixbuf;
		}

		return nullptr;
	}

	bool converter::to_png(const std::string &html_file, const std::string &png_file)
	{
		auto pixbuf = to_pixbuf(html_file);
		bool ret = false;
		if(pixbuf)
		{
			ret = gdk_pixbuf_save(pixbuf, png_file.c_str(), "png", nullptr, nullptr) == TRUE;
			g_object_unref(pixbuf);
		}
		return ret;
	}

	png_diff_t pngcmp(const GdkPixbuf* pixbuf1, const GdkPixbuf* pixbuf2)
	{
		if(gdk_pixbuf_get_height(pixbuf1) != gdk_pixbuf_get_height(pixbuf2) ||
		   gdk_pixbuf_get_width(pixbuf1) != gdk_pixbuf_get_width(pixbuf2))
		{
			return png_diff_dimensions;
		}

		if(gdk_pixbuf_get_byte_length(pixbuf1) !=  gdk_pixbuf_get_byte_length(pixbuf2))
		{
			return png_diff_data;
		}

		const auto sz = gdk_pixbuf_get_byte_length(pixbuf1);

		if(memcmp(gdk_pixbuf_get_pixels(pixbuf1), gdk_pixbuf_get_pixels(pixbuf2), sz))
		{
			return png_diff_data;
		}

		return png_diff_same;
	}

	png_diff_t pngcmp(const std::string &png_file1, const std::string &png_file2)
	{
		auto pixbuf1 = gdk_pixbuf_new_from_file(png_file1.c_str(), nullptr);
		if(!pixbuf1)
		{
			return png_diff_error;
		}
		auto pixbuf2 = gdk_pixbuf_new_from_file(png_file2.c_str(), nullptr);
		if(!pixbuf2)
		{
			return png_diff_error;
		}

		auto res = pngcmp(pixbuf1, pixbuf2);

		g_object_unref(pixbuf1);
		g_object_unref(pixbuf2);

		return res;
	}

	png_diff_t pngcmp(const GdkPixbuf* pixbuf, const std::string &png_file)
	{
		auto pixbuf2 = gdk_pixbuf_new_from_file(png_file.c_str(), nullptr);
		if(!pixbuf2)
		{
			return png_diff_error;
		}
		auto res = pngcmp(pixbuf, pixbuf2);
		g_object_unref(pixbuf2);
		return res;
	}

}
