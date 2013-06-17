#include "globals.h"
#include "TxWnd.h"

CTxWnd::CTxWnd()
{
	m_hWnd	= NULL;
	setClass(TXWND_CLASS);
}

CTxWnd::~CTxWnd(void)
{
	destroy();
}

LRESULT CALLBACK CTxWnd::WndProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
	CTxWnd* pThis = NULL;
	if(IsWindow(hWnd))
	{
		pThis = (CTxWnd*)GetProp(hWnd, TEXT("CTxWnd_this"));
		if(pThis && pThis->m_hWnd != hWnd)
		{
			pThis = NULL;
		}
	}
	if(pThis || uMessage == WM_CREATE)
	{
		switch (uMessage)
		{
		case WM_CREATE:
			{
				LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
				pThis = (CTxWnd*)(lpcs->lpCreateParams);
				SetProp(hWnd, TEXT("CTxWnd_this"), (HANDLE) pThis);
				pThis->m_hWnd = hWnd;
			}
			break;
		case WM_DESTROY:
			{
				LRESULT ret = pThis->OnMessage(hWnd, uMessage, wParam, lParam);
				RemoveProp(hWnd, TEXT("CTxWnd_this"));
				pThis->m_hWnd = NULL;
				return ret;
			}
			break;
		}
		return pThis->OnMessage(hWnd, uMessage, wParam, lParam);
	}
	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

void CTxWnd::registerClass(HINSTANCE hInstance)
{
	WNDCLASSEX wc;
	if(!GetClassInfoEx(hInstance, m_class, &wc))
	{
		ZeroMemory(&wc, sizeof(wc));
		wc.cbSize			= sizeof(wc);
		wc.lpfnWndProc		= (WNDPROC)CTxWnd::WndProc;
		wc.hInstance		= hInstance;
		wc.lpszClassName	= m_class;

		preRegisterClass(&wc);

		ATOM ret = RegisterClassEx(&wc);
		int i=0;
		i++;
	}
}

void CTxWnd::preRegisterClass( WNDCLASSEX* wcex )
{

}

LRESULT CTxWnd::OnMessage( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

BOOL CTxWnd::create( DWORD dwExStyle, LPCTSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance )
{
	registerClass(hInstance);
	m_hWnd = CreateWindowEx(
		dwExStyle, 
		m_class,
		lpWindowName, 
		dwStyle, 
		x, y, 
		nWidth, nHeight, 
		hWndParent, 
		hMenu, 
		hInstance,
		this);
	if(m_hWnd)
	{
		return TRUE;
	}
	return FALSE;
}

void CTxWnd::destroy()
{
	if(m_hWnd)
	{
		DestroyWindow(m_hWnd);
		m_hWnd = NULL;
	}
}
