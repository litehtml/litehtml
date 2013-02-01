#pragma once
#include "gdiplus_container.h"

#define TOOLBARWND_CLASS	L"TOOLBAR_WINDOW"

class CBrowserWnd;

class CToolbarWnd : public gdiplus_container
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

	virtual void				make_url(LPCWSTR url, LPCWSTR basepath, std::wstring& out);
	virtual litehtml::uint_ptr	get_image(LPCWSTR url);

	// litehtml::document_container members
	virtual	void	set_caption(const wchar_t* caption);
	virtual	void	set_base_url(const wchar_t* base_url);
	virtual	void	link(litehtml::document* doc, litehtml::element::ptr el);
	virtual void	import_css(std::wstring& text, const std::wstring& url, std::wstring& baseurl, const litehtml::string_vector& media);
	virtual	void	on_anchor_click(const wchar_t* url, litehtml::element::ptr el);
	virtual	void	set_cursor(const wchar_t* cursor);

protected:
	virtual void	OnCreate();
	virtual void	OnPaint(HDC hdc, LPCRECT rcClip);
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
