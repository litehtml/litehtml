#include "globals.h"
#include "ToolbarWnd.h"
#include "memdc.h"

using namespace Gdiplus;

CToolbarWnd::CToolbarWnd( HINSTANCE hInst )
{
	m_hInst		= hInst;
	m_hWnd		= NULL;

	WNDCLASS wc;
	if(!GetClassInfo(m_hInst, TOOLBARWND_CLASS, &wc))
	{
		ZeroMemory(&wc, sizeof(wc));
		wc.style          = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc    = (WNDPROC)CToolbarWnd::WndProc;
		wc.cbClsExtra     = 0;
		wc.cbWndExtra     = 0;
		wc.hInstance      = m_hInst;
		wc.hIcon          = NULL;
		wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground  = (HBRUSH) (COLOR_WINDOW + 1);
		wc.lpszMenuName   = NULL;
		wc.lpszClassName  = TOOLBARWND_CLASS;

		RegisterClass(&wc);
	}
}
CToolbarWnd::~CToolbarWnd(void)
{
}

LRESULT CALLBACK CToolbarWnd::WndProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
	CToolbarWnd* pThis = NULL;
	if(IsWindow(hWnd))
	{
		pThis = (CToolbarWnd*)GetProp(hWnd, TEXT("toolbar_this"));
		if(pThis && pThis->m_hWnd != hWnd)
		{
			pThis = NULL;
		}
	}

	if(pThis || uMessage == WM_CREATE)
	{
		switch (uMessage)
		{
		case WM_ERASEBKGND:
			return TRUE;
		case WM_CREATE:
			{
				LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
				pThis = (CToolbarWnd*)(lpcs->lpCreateParams);
				SetProp(hWnd, TEXT("toolbar_this"), (HANDLE) pThis);
				pThis->m_hWnd = hWnd;
				pThis->OnCreate();
			}
			break;
		case WM_PAINT:
			{
				RECT rcClient;
				GetClientRect(pThis->m_hWnd, &rcClient);

				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);

				CMemDC dc;
				HDC memdc = dc.beginPaint(hdc, &ps.rcPaint);

				FillRect(memdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW + 1));

				pThis->OnPaint(memdc, &ps.rcPaint);

				dc.endPaint();

				EndPaint(hWnd, &ps);
			}
			return 0;
		case WM_SIZE:
			pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_DESTROY:
			RemoveProp(hWnd, TEXT("toolbar_this"));
			pThis->OnDestroy();
			delete pThis;
			return 0;
		}
	}

	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

void CToolbarWnd::OnCreate()
{

}

void CToolbarWnd::OnPaint( HDC hdc, LPCRECT rcClip )
{
	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);
	Graphics graphics(hdc);
	LinearGradientBrush* brush = NULL;
	Rect rc(	rcClient.left, rcClient.top, 
				rcClient.right - rcClient.left, 
				rcClient.bottom - rcClient.left);

	brush = new LinearGradientBrush(rc,
		Color(250, 250, 250),
		Color(223, 223, 223),
		LinearGradientModeVertical);

	graphics.FillRectangle(brush, rcClient.left, rcClient.top, 
		rcClient.right - rcClient.left, 
		rcClient.bottom - rcClient.top);

	Pen pen( Color(182, 186, 192) );

	graphics.DrawLine(&pen, rcClient.left, rcClient.bottom - 1, rcClient.right, rcClient.bottom - 1);

	delete brush;
}

void CToolbarWnd::OnSize( int width, int height )
{

}

void CToolbarWnd::OnDestroy()
{

}

void CToolbarWnd::create( int x, int y, int width, int height, HWND parent )
{
	m_hWnd = CreateWindow(TOOLBARWND_CLASS, L"toolbar", WS_CHILD | WS_VISIBLE, x, y, width, height, parent, NULL, m_hInst, (LPVOID) this);
}
