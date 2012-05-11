#pragma once

#define HTMLVIEWWND_CLASS	L"HTMLVIEW_WINDOW"

using namespace litehtml;

typedef std::map<std::wstring, Gdiplus::Image*>	images_map;

class CHTMLViewWnd : public litehtml::document_container
{
	HWND					m_hWnd;
	HINSTANCE				m_hInst;
	litehtml::document::ptr	m_doc;
	int						m_top;
	int						m_left;
	int						m_max_top;
	int						m_max_left;
	images_map				m_images;
	std::wstring			m_base_path;
	std::wstring			m_doc_path;
public:
	CHTMLViewWnd(HINSTANCE	hInst);
	virtual ~CHTMLViewWnd(void);

	void	create(int x, int y, int width, int height, HWND parent);
	void	open(LPCWSTR path);
	HWND	wnd()	{ return m_hWnd;	}
	void	refresh();

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

	virtual	void		set_caption(const wchar_t* caption);
	virtual	void		set_base_url(const wchar_t* base_url);
	virtual	void		link(litehtml::document* doc, litehtml::element::ptr el);
	virtual int			get_default_font_size();

protected:
	virtual void OnCreate();
	virtual void OnPaint(HDC hdc, LPCRECT rcClip);
	virtual void OnSize(int width, int height);
	virtual void OnDestroy();
	virtual void OnVScroll(int pos, int flags);
	virtual void OnMouseWheel(int delta);
	virtual void OnKeyDown(UINT vKey);
	
	void	render();
	void	redraw();
	void	update_scroll();
	void	clear_images();
	LPWSTR	load_text_file(LPCWSTR path);
	void	make_url(LPCWSTR url, LPCWSTR basepath, std::wstring& out);

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
};
