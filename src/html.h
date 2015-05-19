#pragma once

#include <stdlib.h>
#include <string>
#include <ctype.h>
#include <vector>
#include <map>
#include <cstring>
#include <algorithm>
#include "os_types.h"
#include "types.h"
#include "object.h"
#include "background.h"
#include "borders.h"
#include "html_tag.h"
#include "web_color.h"
#include "media_query.h"

namespace litehtml
{
	struct list_marker
	{
		tstring			image;
		const tchar_t*	baseurl;
		list_style_type	marker_type;
		web_color		color;
		position		pos;
	};

	// call back interface to draw text, images and other elements
	class document_container
	{
	public:
		virtual uint_ptr			create_font(const tchar_t* faceName, int size, int weight, font_style italic, unsigned int decoration, litehtml::font_metrics* fm) = 0;
		virtual void				delete_font(uint_ptr hFont) = 0;
		virtual int					text_width(const tchar_t* text, uint_ptr hFont) = 0;
		virtual void				draw_text(uint_ptr hdc, const tchar_t* text, uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) = 0;
		virtual int					pt_to_px(int pt) = 0;
		virtual int					get_default_font_size() const = 0;
		virtual const tchar_t*		get_default_font_name() const = 0;
		virtual void				draw_list_marker(uint_ptr hdc, const litehtml::list_marker& marker) = 0;
		virtual void				load_image(const tchar_t* src, const tchar_t* baseurl, bool redraw_on_ready) = 0;
		virtual void				get_image_size(const tchar_t* src, const tchar_t* baseurl, litehtml::size& sz) = 0;
		virtual void				draw_background(uint_ptr hdc, const litehtml::background_paint& bg) = 0;
		virtual void				draw_borders(uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) = 0;

		virtual	void				set_caption(const tchar_t* caption)		= 0;
		virtual	void				set_base_url(const tchar_t* base_url)	= 0;
		virtual void				link(litehtml::document* doc, litehtml::element::ptr el) = 0;
		virtual void				on_anchor_click(const tchar_t* url, litehtml::element::ptr el) = 0;
		virtual	void				set_cursor(const tchar_t* cursor)	= 0;
		virtual	void				transform_text(litehtml::tstring& text, litehtml::text_transform tt) = 0;
		virtual void				import_css(tstring& text, const tstring& url, tstring& baseurl) = 0;
		virtual void				set_clip(const litehtml::position& pos, const border_radiuses& bdr_radius, bool valid_x, bool valid_y) = 0;
		virtual void				del_clip() = 0;
		virtual void				get_client_rect(litehtml::position& client) = 0;
		virtual litehtml::element*	create_element(const tchar_t* tag_name, const string_map& attributes, litehtml::document* doc) = 0;
		virtual void				get_media_features(litehtml::media_features& media) = 0;
	};

	void trim(tstring &s);
	void lcase(tstring &s);
	int	 value_index(const tstring& val, const tstring& strings, int defValue = -1, tchar_t delim = _t(';'));
	bool value_in_list(const tstring& val, const tstring& strings, tchar_t delim = _t(';'));
	tstring::size_type find_close_bracket(const tstring &s, tstring::size_type off, tchar_t open_b = _t('('), tchar_t close_b = _t(')'));
	void split_string(const tstring& str, string_vector& tokens, const tstring& delims, const tstring& delims_preserve = _t(""), const tstring& quote = _t("\""));

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
