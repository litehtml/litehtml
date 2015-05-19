#pragma once
#include <windows.h>
#include <mlang.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <mlang.h>
#include <vector>
#include <cairo.h>
#include <cairo-win32.h>
#include <litehtml.h>
#include <dib.h>
#include <txdib.h>

#ifdef LITEHTML_UTF8
#define t_make_url	make_url_utf8
#else
#define t_make_url	make_url
#endif

struct cairo_clip_box
{
	typedef std::vector<cairo_clip_box> vector;
	litehtml::position	box;
	litehtml::border_radiuses radius;

	cairo_clip_box(const litehtml::position& vBox, litehtml::border_radiuses vRad)
	{
		box = vBox;
		radius = vRad;
	}

	cairo_clip_box(const cairo_clip_box& val)
	{
		box = val.box;
		radius = val.radius;
	}
	cairo_clip_box& operator=(const cairo_clip_box& val)
	{
		box = val.box;
		radius = val.radius;
		return *this;
	}
};

class cairo_container :	public litehtml::document_container
{
public:
	typedef std::shared_ptr<CTxDIB>				image_ptr;
	typedef std::map<std::wstring, image_ptr>	images_map;

protected:
	cairo_surface_t*			m_temp_surface;
	cairo_t*					m_temp_cr;
	images_map					m_images;
	cairo_clip_box::vector		m_clips;
	IMLangFontLink2*			m_font_link;
	CRITICAL_SECTION			m_img_sync;
public:
	cairo_container(void);
	virtual ~cairo_container(void);

	virtual litehtml::uint_ptr			create_font(const litehtml::tchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm);
	virtual void						delete_font(litehtml::uint_ptr hFont);
	virtual int							text_width(const litehtml::tchar_t* text, litehtml::uint_ptr hFont);
	virtual void						draw_text(litehtml::uint_ptr hdc, const litehtml::tchar_t* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos);

	virtual int							pt_to_px(int pt);
	virtual int							get_default_font_size() const;
	virtual const litehtml::tchar_t*	get_default_font_name() const;
	virtual void						draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker);
	virtual void						load_image(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, bool redraw_on_ready);
	virtual void						get_image_size(const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, litehtml::size& sz);
	virtual void						draw_image(litehtml::uint_ptr hdc, const litehtml::tchar_t* src, const litehtml::tchar_t* baseurl, const litehtml::position& pos);
	virtual void						draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint& bg);
	virtual void						draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root);

	virtual	void						transform_text(litehtml::tstring& text, litehtml::text_transform tt);
	virtual void						set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius, bool valid_x, bool valid_y);
	virtual void						del_clip();
	virtual litehtml::element*			create_element(const litehtml::tchar_t* tag_name, const litehtml::string_map& attributes, litehtml::document* doc);
	virtual void						get_media_features(litehtml::media_features& media);

	virtual void						make_url( LPCWSTR url, LPCWSTR basepath, std::wstring& out ) = 0;
	virtual image_ptr					get_image(LPCWSTR url, bool redraw_on_ready) = 0;
	virtual void						get_client_rect(litehtml::position& client) = 0;
	void								clear_images();
	void								add_image(std::wstring& url, image_ptr& img);
	void								remove_image(std::wstring& url);
	void								make_url_utf8( const char* url, const char* basepath, std::wstring& out );

protected:
	virtual void						draw_ellipse(cairo_t* cr, int x, int y, int width, int height, const litehtml::web_color& color, double line_width);
	virtual void						fill_ellipse(cairo_t* cr, int x, int y, int width, int height, const litehtml::web_color& color);
	virtual void						rounded_rectangle( cairo_t* cr, const litehtml::position &pos, const litehtml::border_radiuses &radius );

	void								set_color(cairo_t* cr, litehtml::web_color color)	{ cairo_set_source_rgba(cr, color.red / 255.0, color.green / 255.0, color.blue / 255.0, color.alpha / 255.0); }
private:
	simpledib::dib*						get_dib(litehtml::uint_ptr hdc)	{ return (simpledib::dib*) hdc;				}
	void								apply_clip(cairo_t* cr);
	bool								add_path_arc(cairo_t* cr, double x, double y, double rx, double ry, double a1, double a2, bool neg);

	void								draw_txdib(cairo_t* cr, CTxDIB* bmp, int x, int y, int cx, int cy);
	void								lock_images_cache();
	void								unlock_images_cache();
};
