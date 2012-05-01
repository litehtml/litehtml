#pragma once

#include <string>
#include <ctype.h>
#include <vector>
#include <map>
#include <Windows.h>
#include "types.h"
#include "background.h"

namespace litehtml
{
	// call back interface to draw text, images and other elements
	class painter
	{
	public:
		virtual uint_ptr	create_font(const wchar_t* faceName, int size, int weight, font_style italic, unsigned int decoration) = 0;
		virtual void		delete_font(uint_ptr hFont) = 0;
		virtual int			line_height(uint_ptr hdc, uint_ptr hFont) = 0;
		virtual int			text_width(uint_ptr hdc, const wchar_t* text, uint_ptr hFont) = 0;
		virtual void		draw_text(uint_ptr hdc, const wchar_t* text, uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) = 0;
		virtual void		fill_rect(uint_ptr hdc, const litehtml::position& pos, litehtml::web_color color) = 0;
		virtual uint_ptr	get_temp_dc() = 0;
		virtual void		release_temp_dc(uint_ptr hdc) = 0;
		virtual int			pt_to_px(int pt) = 0;
		virtual int			get_text_base_line(uint_ptr hdc, uint_ptr hFont) = 0;
		virtual void		draw_list_marker(uint_ptr hdc, list_style_type marker_type, int x, int y, int height, const web_color& color) = 0;
		virtual void		load_image(const wchar_t* src) = 0;
		virtual void		get_image_size(const wchar_t* src, litehtml::size& sz) = 0;
		virtual void		draw_image(uint_ptr hdc, const wchar_t* src, const litehtml::position& pos) = 0;
		virtual void		draw_background(uint_ptr hdc, 
											const wchar_t* image, 
											const wchar_t* baseurl, 
											const litehtml::position& draw_pos,
											const litehtml::css_position& bg_pos,
											litehtml::background_repeat repeat, 
											litehtml::background_attachment attachment) = 0;
	};

	void trim(std::wstring &s);
	int value_index(const wchar_t* val, const wchar_t* strings, int defValue = -1, const wchar_t* delim = L";");
}