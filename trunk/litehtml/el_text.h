#pragma once

#include "html_tag.h"

namespace litehtml
{
	class el_text : public element
	{
	protected:
		tstring			m_text;
		tstring			m_transformed_text;
		size			m_size;
		text_transform	m_text_transform;
	public:
		el_text(const tchar_t* text, litehtml::document* doc);
		virtual ~el_text();

		virtual void				apply_stylesheet(const litehtml::css& stylesheet);
		virtual void				get_text(tstring& text);
		virtual const tchar_t*		get_style_property(const tchar_t* name, bool inherited, const tchar_t* def = 0);
		virtual void				parse_styles(bool is_reparse);
		virtual int					get_base_line();
		virtual void				draw(uint_ptr hdc, int x, int y, const position* clip);
		virtual int					line_height() const;
		virtual uint_ptr			get_font(font_metrics* fm = 0);
		virtual style_display		get_display() const;
		virtual white_space			get_white_space() const;

	protected:
		virtual void				get_content_size(size& sz, int max_width);
		virtual void				draw_content(uint_ptr hdc, const litehtml::position& pos);
	};
}