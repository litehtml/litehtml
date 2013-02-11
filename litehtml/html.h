#pragma once

#include <string>
#include <ctype.h>
#include <vector>
#include <map>
#include <Windows.h>
#include "types.h"
#include "background.h"
#include "borders.h"
#include "element.h"
#include "web_color.h"
#include "object.h"

namespace litehtml
{
	// call back interface to draw text, images and other elements
	class document_container
	{
	public:
		virtual uint_ptr	create_font(const wchar_t* faceName, int size, int weight, font_style italic, unsigned int decoration) = 0;
		virtual void		delete_font(uint_ptr hFont) = 0;
		virtual int			line_height(uint_ptr hdc, uint_ptr hFont) = 0;
		virtual int			text_width(uint_ptr hdc, const wchar_t* text, uint_ptr hFont) = 0;
		virtual void		draw_text(uint_ptr hdc, const wchar_t* text, uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) = 0;
		virtual void		fill_rect(uint_ptr hdc, const litehtml::position& pos, const litehtml::web_color color, const litehtml::css_border_radius& radius) = 0;
		virtual uint_ptr	get_temp_dc() = 0;
		virtual void		release_temp_dc(uint_ptr hdc) = 0;
		virtual int			pt_to_px(int pt) = 0;
		virtual int			get_default_font_size() = 0;
		virtual int			get_text_base_line(uint_ptr hdc, uint_ptr hFont) = 0;
		virtual void		draw_list_marker(uint_ptr hdc, list_style_type marker_type, int x, int y, int height, const web_color& color) = 0;
		virtual void		load_image(const wchar_t* src, const wchar_t* baseurl) = 0;
		virtual void		get_image_size(const wchar_t* src, const wchar_t* baseurl, litehtml::size& sz) = 0;
		virtual void		draw_image(uint_ptr hdc, const wchar_t* src, const wchar_t* baseurl, const litehtml::position& pos) = 0;
		virtual void		draw_background(uint_ptr hdc, 
											const wchar_t* image, 
											const wchar_t* baseurl, 
											const litehtml::position& draw_pos,
											const litehtml::css_position& bg_pos,
											litehtml::background_repeat repeat, 
											litehtml::background_attachment attachment) = 0;
		virtual void		draw_borders(uint_ptr hdc, const css_borders& borders, const litehtml::position& draw_pos) = 0;

		virtual	void		set_caption(const wchar_t* caption)		= 0;
		virtual	void		set_base_url(const wchar_t* base_url)	= 0;
		virtual	void		link(litehtml::document* doc, litehtml::element::ptr el)	= 0;
		virtual	void		on_anchor_click(const wchar_t* url, litehtml::element::ptr el)	= 0;
		virtual	void		set_cursor(const wchar_t* cursor)	= 0;
		virtual	wchar_t		toupper(const wchar_t c) = 0;
		virtual	wchar_t		tolower(const wchar_t c) = 0;
		virtual void		import_css(std::wstring& text, const std::wstring& url, std::wstring& baseurl, const string_vector& media) = 0;
		virtual void		set_clip(const litehtml::position& pos, bool valid_x, bool valid_y) = 0;
		virtual void		del_clip() = 0;
	};

	void trim(std::wstring &s);
	int value_index(const wchar_t* val, const wchar_t* strings, int defValue = -1, const wchar_t* delim = L";");
	int value_in_list(const wchar_t* val, const wchar_t* strings, const wchar_t* delim = L";");

	inline int round_f(float val)
	{
		int int_val = (int) val;
		if(val - int_val >= 0.5)
		{
			int_val++;
		}
		return int_val;
	}

	inline int round_d(double val)
	{
		int int_val = (int) val;
		if(val - int_val >= 0.5)
		{
			int_val++;
		}
		return int_val;
	}
}
