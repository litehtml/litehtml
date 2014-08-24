#pragma once

#define TXWND_CLASS		L"TORDEX_WINDOW"

class CTxWnd
{
protected:
	HWND		m_hWnd;
	WCHAR		m_class[256];
public:
	CTxWnd();
	virtual ~CTxWnd(void);

	BOOL create(DWORD dwExStyle, LPCTSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance);
	void destroy();
	void setClass(LPCWSTR className)	{ StringCchCopy(m_class, 256, className); }
	operator HWND()
	{
		return m_hWnd;
	}

protected:
	virtual void	preRegisterClass(WNDCLASSEX* wcex);
	virtual LRESULT	OnMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	void registerClass(HINSTANCE hInstance);
};


template <class T> class TTxWnd : public CTxWnd
{
public:
	typedef private LRESULT (T::*OnMessageFunction)(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	TTxWnd()
	{
		m_pOnMessageFunction	= NULL;
		m_pClass				= NULL;
	}

	void setHandler(T *pClass, OnMessageFunction pFunc)
	{
		m_pClass				= pClass;
		m_pOnMessageFunction	= pFunc;
	}

protected:
	virtual LRESULT	OnMessage(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		if (m_pOnMessageFunction && m_pClass)
		{
			return (m_pClass->*m_pOnMessageFunction)(hWnd, uMessage, wParam, lParam);
		}
		return CTxWnd::OnMessage(hWnd, uMessage, wParam, lParam);
	}

private:
	T *m_pClass;
	OnMessageFunction m_pOnMessageFunction;
};