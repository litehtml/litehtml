#pragma once
#include <GdiPlus.h>

namespace litehtml
{
	class win32_container : public document_container
	{
	public:
		typedef std::map<std::wstring, Gdiplus::Bitmap*>	images_map;
	
	private:
		
		images_map	m_images;

	public:
		// litehtml::document_container members
		virtual uint_ptr	create_font(const wchar_t* faceName, int size, int weight, font_style italic, unsigned int decoration);
		virtual void		delete_font(uint_ptr hFont);
		virtual int			line_height(uint_ptr hdc, uint_ptr hFont);
		virtual int			get_text_base_line(uint_ptr hdc, uint_ptr hFont);
		virtual int			text_width(uint_ptr hdc, const wchar_t* text, uint_ptr hFont);
		virtual void		draw_text(uint_ptr hdc, const wchar_t* text, uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos);
		virtual void		fill_rect(uint_ptr hdc, const litehtml::position& pos, litehtml::web_color color);
		virtual uint_ptr	get_temp_dc();
		virtual void		release_temp_dc(uint_ptr hdc);
		virtual int			pt_to_px(int pt);
		virtual void		draw_list_marker(uint_ptr hdc, list_style_type marker_type, int x, int y, int height, const web_color& color);
		virtual void		load_image(const wchar_t* src, const wchar_t* baseurl);
		virtual void		get_image_size(const wchar_t* src, const wchar_t* baseurl, litehtml::size& sz);
		virtual void		draw_image(uint_ptr hdc, const wchar_t* src, const wchar_t* baseurl, const litehtml::position& pos);
		virtual void		draw_background(uint_ptr hdc, 
											const wchar_t* image, 
											const wchar_t* baseurl, 
											const litehtml::position& draw_pos,
											const litehtml::css_position& bg_pos,
											litehtml::background_repeat repeat, 
											litehtml::background_attachment attachment);
		virtual void		draw_borders(uint_ptr hdc, const css_borders& borders, const litehtml::position& draw_pos);

		virtual int			get_default_font_size();
		virtual	wchar_t		toupper(const wchar_t c);
		virtual	wchar_t		tolower(const wchar_t c);

	protected:
		void					clear_images();
		virtual void			make_url( LPCWSTR url, LPCWSTR basepath, std::wstring& out ) = 0;
		virtual Gdiplus::Bitmap*	get_image(LPCWSTR url) = 0;
	};
}