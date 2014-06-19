#pragma once

#include "../../include/litehtml.h"
#include <cairo.h>
#include <gtkmm.h>

struct cairo_font
{
	cairo_font_face_t*	font;
	int					size;
	bool				underline;
	bool				strikeout;
};

class container_linux :	public litehtml::document_container
{
	typedef std::map<litehtml::tstring, Glib::RefPtr<Gdk::Pixbuf> >	images_map;

protected:
	cairo_surface_t*			m_temp_surface;
	cairo_t*					m_temp_cr;
	images_map					m_images;
	litehtml::position::vector	m_clips;
public:
	container_linux(void);
	virtual ~container_linux(void);

	virtual litehtml::uint_ptr			create_font(const litehtml::tchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm);
	virtual void						delete_font(litehtml::uint_ptr hFont);
	virtual int						text_width(const litehtml::tchar_t* text, litehtml::uint_ptr hFont);
	virtual void						draw_text(litehtml::uint_ptr hdc, const litehtml::tchar_t* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos);
	virtual void						fill_rect(litehtml::uint_ptr hdc, const litehtml::position& pos, const litehtml::web_color color, const litehtml::css_border_radius& radius);
	virtual int						pt_to_px(int pt);
	virtual int						get_default_font_size();
	virtual const litehtml::tchar_t*	get_default_font_name();
	virtual void 						load_image(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, bool redraw_on_ready);
	virtual void						get_image_size(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz);
	virtual void						draw_image(litehtml::uint_ptr hdc, const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, const litehtml::position& pos);
	virtual void						draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint& bg);
	virtual void						draw_borders(litehtml::uint_ptr hdc, const litehtml::css_borders& borders, const litehtml::position& draw_pos);
	virtual void 						draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker);
	virtual bool						is_media_valid(const litehtml::tstring& media);
	virtual litehtml::element*			create_element(const litehtml::tchar_t* tag_name);

	virtual	litehtml::tchar_t			toupper(const litehtml::tchar_t c);
	virtual	litehtml::tchar_t			tolower(const litehtml::tchar_t c);
	virtual void						set_clip(const litehtml::position& pos, bool valid_x, bool valid_y);
	virtual void						del_clip();

	virtual void						make_url( const litehtml::tchar_t* url, const litehtml::tchar_t* basepath, litehtml::tstring& out );
	virtual Glib::RefPtr<Gdk::Pixbuf>	get_image(const litehtml::tchar_t* url, bool redraw_on_ready) = 0;

	virtual void						get_client_rect(litehtml::position& client) = 0;
	void								clear_images();

protected:
	virtual void						draw_ellipse(cairo_t* cr, int x, int y, int width, int height, const litehtml::web_color& color, int line_width);
	virtual void						fill_ellipse(cairo_t* cr, int x, int y, int width, int height, const litehtml::web_color& color);
	virtual void						rounded_rectangle( cairo_t* cr, const litehtml::position &pos, const litehtml::css_border_radius &radius );

private:
	void								apply_clip(cairo_t* cr);
	void								add_path_arc(cairo_t* cr, double x, double y, double rx, double ry, double a1, double a2, bool neg);
	void								set_color(cairo_t* cr, litehtml::web_color color)	{ cairo_set_source_rgba(cr, color.red / 255.0, color.green / 255.0, color.blue / 255.0, color.alpha / 255.0); }
	void								draw_pixbuf(cairo_t* cr, const Glib::RefPtr<Gdk::Pixbuf>& bmp, int x, int y, int cx, int cy);
	cairo_surface_t*					surface_from_pixbuf(const Glib::RefPtr<Gdk::Pixbuf>& bmp);
};
