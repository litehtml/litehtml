#pragma once
#include "object.h"
#include "style.h"
#include "types.h"

namespace litehtml
{
	class element;

	class document : public object
	{
	public:
		typedef object_ptr<document>	ptr;
	private:
		element*			m_root;
		painter*			m_painter;
		fonts_map			m_fonts;
		style_sheet::vector	m_styles;
		std::wstring		m_font_name;
		int					m_font_size;
		litehtml::web_color		m_def_color;
	public:
		document(litehtml::painter* objPainter);
		virtual ~document();

		litehtml::painter*	get_painter()	{ return m_painter; }
		uint_ptr		get_font(const wchar_t* name, const wchar_t* size, const wchar_t* weight, const wchar_t* style, const wchar_t* decoration);
		void			render(uint_ptr hdc, int max_width);
		void			draw(uint_ptr hdc, int x, int y, position* clip);
		web_color		get_def_color()	{ return m_def_color; }
		int				cvt_units(const wchar_t* str, int fontSize, bool* is_percent = 0) const;
		int				cvt_units(css_length& val, int fontSize) const;
		int				cvt_font_size( const wchar_t* size ) const;
		int				width() const;
		int				height() const;
		void			add_stylesheet(const wchar_t* str, const wchar_t* baseurl);

		static litehtml::document::ptr createFromString(const wchar_t* str, litehtml::painter* objPainter, const wchar_t* stylesheet, const wchar_t* cssbaseurl);
	
	private:
		//void			load_default_styles();
		litehtml::element*	add_root();
		litehtml::element*	add_body();
		uint_ptr		add_font(const wchar_t* name, const wchar_t* size, const wchar_t* weight, const wchar_t* style, const wchar_t* decoration);
	};
}