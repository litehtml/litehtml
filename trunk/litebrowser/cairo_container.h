#pragma once
#include "dib.h"

class cairo_dev
{
	cairo_surface_t*	m_surface;
	cairo_t*			m_cr;
public:
	cairo_dev(simpledib::dib* dib);
	~cairo_dev();
	operator cairo_t*()	{	return m_cr;	}
	void set_color(litehtml::web_color color)
	{
		cairo_set_source_rgba(m_cr, color.red / 255.0, color.green / 255.0, color.blue / 255.0, color.alpha / 255.0);
	}
	void draw_image(CTxDIB* bmp, int x, int y, int cx, int cy);
};

class cairo_fnt
{
	cairo_font_face_t*		m_font_face;
	int						m_size;
	BOOL					m_bUnderline;
	BOOL					m_bStrikeOut;
public:
	cairo_fnt(LPCWSTR facename, int size, int weight, BOOL italic, BOOL strikeout, BOOL underline);
	~cairo_fnt();

	BOOL	is_strikeout()			{ return m_bStrikeOut;	}
	BOOL	is_underline()			{ return m_bUnderline;	}
	int		size()					{ return m_size;		}

	cairo_font_face_t* fnt()		{ return m_font_face;	}
};

class utf8_str
{
	LPSTR m_str;
public:
	utf8_str(LPCWSTR str);
	~utf8_str();

	operator LPCSTR()	{ return m_str; }
};

class cairo_container :	public litehtml::document_container
{
	typedef std::map<std::wstring, CTxDIB*>	images_map;

protected:

	images_map					m_images;
	litehtml::position::vector	m_clips;
public:
	cairo_container(void);
	virtual ~cairo_container(void);

	virtual litehtml::uint_ptr	create_font(const wchar_t* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration);
	virtual void				delete_font(litehtml::uint_ptr hFont);
	virtual int					line_height(litehtml::uint_ptr hdc, litehtml::uint_ptr hFont);
	virtual int					text_width(litehtml::uint_ptr hdc, const wchar_t* text, litehtml::uint_ptr hFont);
	virtual void				draw_text(litehtml::uint_ptr hdc, const wchar_t* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos);
	virtual void				fill_rect(litehtml::uint_ptr hdc, const litehtml::position& pos, const litehtml::web_color color, const litehtml::css_border_radius& radius);
	virtual litehtml::uint_ptr	get_temp_dc();
	virtual void				release_temp_dc(litehtml::uint_ptr hdc);
	virtual int					pt_to_px(int pt);
	virtual int					get_default_font_size();
	virtual int					get_text_base_line(litehtml::uint_ptr hdc, litehtml::uint_ptr hFont);
	virtual void				draw_list_marker(litehtml::uint_ptr hdc, litehtml::list_style_type marker_type, int x, int y, int height, const litehtml::web_color& color);
	virtual void				load_image(const wchar_t* src, const wchar_t* baseurl);
	virtual void				get_image_size(const wchar_t* src, const wchar_t* baseurl, litehtml::size& sz);
	virtual void				draw_image(litehtml::uint_ptr hdc, const wchar_t* src, const wchar_t* baseurl, const litehtml::position& pos);
	virtual void				draw_background(litehtml::uint_ptr hdc, 
												const wchar_t* image, 
												const wchar_t* baseurl, 
												const litehtml::position& draw_pos,
												const litehtml::css_position& bg_pos,
												litehtml::background_repeat repeat, 
												litehtml::background_attachment attachment);
	virtual void				draw_borders(litehtml::uint_ptr hdc, const litehtml::css_borders& borders, const litehtml::position& draw_pos);

	virtual	wchar_t				toupper(const wchar_t c);
	virtual	wchar_t				tolower(const wchar_t c);
	virtual void				set_clip(const litehtml::position& pos, bool valid_x, bool valid_y);
	virtual void				del_clip();

	virtual void				make_url( LPCWSTR url, LPCWSTR basepath, std::wstring& out ) = 0;
	virtual CTxDIB*				get_image(LPCWSTR url) = 0;
	virtual void				get_client_rect(litehtml::position& client) = 0;

protected:
	virtual void				draw_ellipse(simpledib::dib* dib, int x, int y, int width, int height, const litehtml::web_color& color, int line_width);
	virtual void				fill_ellipse(simpledib::dib* dib, int x, int y, int width, int height, const litehtml::web_color& color);

private:
	simpledib::dib*				get_dib(litehtml::uint_ptr hdc)	{ return (simpledib::dib*) hdc;				}
	void						apply_clip(cairo_t* cr);
	void						clear_images();
	void						add_path_arc(cairo_t* cr, double x, double y, double rx, double ry, double a1, double a2, bool neg);
};
