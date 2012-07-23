#pragma once
#include "..\containers\win32_container.h"

#define HTMLVIEWWND_CLASS	L"HTMLVIEW_WINDOW"

using namespace litehtml;

class CHTMLViewWnd : public litehtml::win32_container
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
	litehtml::context*		m_context;
	std::wstring			m_anchor;
	litehtml::string_vector	m_history_back;
	litehtml::string_vector	m_history_forward;
	std::wstring			m_cursor;
public:
	CHTMLViewWnd(HINSTANCE	hInst, litehtml::context* ctx);
	virtual ~CHTMLViewWnd(void);

	void	create(int x, int y, int width, int height, HWND parent);
	void	open(LPCWSTR path);
	HWND	wnd()	{ return m_hWnd;	}
	void	refresh();
	void	back();
	void	forward();

	// litehtml::document_container members
	virtual	void		set_caption(const wchar_t* caption);
	virtual	void		set_base_url(const wchar_t* base_url);
	virtual	void		link(litehtml::document* doc, litehtml::element::ptr el);
	virtual void		import_css(std::wstring& text, const std::wstring& url, std::wstring& baseurl, const string_vector& media);
	virtual	void		on_anchor_click(const wchar_t* url, litehtml::element::ptr el);
	virtual	void		set_cursor(const wchar_t* cursor);

protected:
	virtual void OnCreate();
	virtual void OnPaint(HDC hdc, LPCRECT rcClip);
	virtual void OnSize(int width, int height);
	virtual void OnDestroy();
	virtual void OnVScroll(int pos, int flags);
	virtual void OnMouseWheel(int delta);
	virtual void OnKeyDown(UINT vKey);
	virtual void OnMouseMove(int x, int y);
	virtual void OnLButtonDown(int x, int y);
	virtual void OnLButtonUp(int x, int y);
	virtual void OnMouseLeave();
	
	void					render();
	void					redraw();
	void					update_scroll();
	void					update_cursor();
	
	virtual void			make_url(LPCWSTR url, LPCWSTR basepath, std::wstring& out);
	virtual Gdiplus::Bitmap*	get_image(LPCWSTR url);
	void						get_client_rect(litehtml::position& client);

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
};
