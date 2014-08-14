#pragma once
#include "..\containers\cairo\cairo_container.h"
#include "dib.h"

#define TOOLBARWND_CLASS	L"TOOLBAR_WINDOW"

class CBrowserWnd;

class CToolbarWnd : public cairo_container
{
	HWND					m_hWnd;
	HINSTANCE				m_hInst;
	litehtml::context		m_context;
	litehtml::document::ptr	m_doc;
	CBrowserWnd*			m_parent;
public:
	CToolbarWnd(HINSTANCE hInst, CBrowserWnd* parent);
	virtual ~CToolbarWnd(void);

	void create(int x, int y, int width, HWND parent);
	HWND wnd()	{ return m_hWnd; }
	int height()
	{
		if(m_doc)
		{
			return m_doc->height();
		}
		return 0;
	}
	int set_width(int width);

	// cairo_container members
	virtual void			make_url(LPCWSTR url, LPCWSTR basepath, std::wstring& out);
	virtual CTxDIB*			get_image(LPCWSTR url, bool redraw_on_ready);

	// litehtml::document_container members
	virtual	void	set_caption(const wchar_t* caption);
	virtual	void	set_base_url(const wchar_t* base_url);
	virtual	void	link(litehtml::document* doc, litehtml::element::ptr el);
	virtual void	import_css(std::wstring& text, const std::wstring& url, std::wstring& baseurl);
	virtual	void	on_anchor_click(const wchar_t* url, litehtml::element::ptr el);
	virtual	void	set_cursor(const wchar_t* cursor);

protected:
	virtual void	OnCreate();
	virtual void	OnPaint(simpledib::dib* dib, LPRECT rcDraw);
	virtual void	OnSize(int width, int height);
	virtual void	OnDestroy();
	virtual void	OnMouseMove(int x, int y);
	virtual void	OnLButtonDown(int x, int y);
	virtual void	OnLButtonUp(int x, int y);
	virtual void	OnMouseLeave();

	virtual void	get_client_rect(litehtml::position& client);

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
};
