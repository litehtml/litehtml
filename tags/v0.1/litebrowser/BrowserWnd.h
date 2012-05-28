#pragma once

#define BROWSERWND_CLASS	L"BROWSER_WINDOW"

class CHTMLViewWnd;
class CToolbarWnd;

class CBrowserWnd
{
	HWND				m_hWnd;
	HINSTANCE			m_hInst;
	CHTMLViewWnd*		m_view;
	CToolbarWnd*		m_toolbar;
	litehtml::context	m_browser_context;
public:
	CBrowserWnd(HINSTANCE hInst);
	virtual ~CBrowserWnd(void);

	void create();
	void open(LPCWSTR path);

	void back();
	void forward();
	void reload();

protected:
	virtual void OnCreate();
	virtual void OnSize(int width, int height);
	virtual void OnDestroy();

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
};
