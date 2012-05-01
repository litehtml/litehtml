#pragma once

#define TOOLBARWND_CLASS	L"TOOLBAR_WINDOW"

class CToolbarWnd
{
	HWND		m_hWnd;
	HINSTANCE	m_hInst;
public:
	CToolbarWnd(HINSTANCE hInst);
	virtual ~CToolbarWnd(void);

	void create(int x, int y, int width, int height, HWND parent);
	HWND wnd()	{ return m_hWnd; }

protected:
	virtual void OnCreate();
	virtual void OnPaint(HDC hdc, LPCRECT rcClip);
	virtual void OnSize(int width, int height);
	virtual void OnDestroy();

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
};
