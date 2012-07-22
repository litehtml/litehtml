#include "globals.h"
#include "ToolbarWnd.h"
#include "memdc.h"
#include <WindowsX.h>
#include "BrowserWnd.h"

using namespace Gdiplus;

CToolbarWnd::CToolbarWnd( HINSTANCE hInst, CBrowserWnd* parent )
{
	m_parent	= parent;
	m_hInst		= hInst;
	m_hWnd		= NULL;

	WNDCLASS wc;
	if(!GetClassInfo(m_hInst, TOOLBARWND_CLASS, &wc))
	{
		ZeroMemory(&wc, sizeof(wc));
		wc.style          = CS_HREDRAW | CS_VREDRAW;
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

	m_context.load_master_stylesheet(L"html,div,body { display: block; } head,style { display: none; }");
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
		case WM_MOUSEMOVE:
			{
				TRACKMOUSEEVENT tme;
				ZeroMemory(&tme, sizeof(TRACKMOUSEEVENT));
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags		= TME_QUERY;
				tme.hwndTrack	= hWnd;
				TrackMouseEvent(&tme);
				if(!(tme.dwFlags & TME_LEAVE))
				{
					tme.dwFlags		= TME_LEAVE;
					tme.hwndTrack	= hWnd;
					TrackMouseEvent(&tme);
				}
				pThis->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			}
			return 0;
		case WM_MOUSELEAVE:
			pThis->OnMouseLeave();
			return 0;
		case WM_LBUTTONDOWN:
			pThis->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		case WM_LBUTTONUP:
			pThis->OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
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
	if(m_doc)
	{
		if(rcClip)
		{
			litehtml::position clip(rcClip->left, rcClip->top, rcClip->right - rcClip->left, rcClip->bottom - rcClip->top);
			m_doc->draw((litehtml::uint_ptr) hdc, 0, 0, &clip);
		} else
		{
			m_doc->draw((litehtml::uint_ptr) hdc, 0, 0, 0);
		}
	}
}

void CToolbarWnd::OnSize( int width, int height )
{

}

void CToolbarWnd::OnDestroy()
{

}

void CToolbarWnd::create( int x, int y, int width, HWND parent )
{
	LPWSTR html = NULL;

	HRSRC hResource = ::FindResource(m_hInst, L"toolbar.html", RT_HTML);
	if(hResource)
	{
		DWORD imageSize = ::SizeofResource(m_hInst, hResource);
		if(imageSize)
		{
			LPCSTR pResourceData = (LPCSTR) ::LockResource(::LoadResource(m_hInst, hResource));
			if(pResourceData)
			{
				html = new WCHAR[imageSize * 3];
				int ret = MultiByteToWideChar(CP_UTF8, 0, pResourceData, imageSize, html, imageSize * 3);
				html[ret] = 0;
			}
		}
	}

	m_doc = litehtml::document::createFromString(html, this, &m_context);
	delete html;
	m_doc->render(width);

	m_hWnd = CreateWindow(TOOLBARWND_CLASS, L"toolbar", WS_CHILD | WS_VISIBLE, x, y, width, m_doc->height(), parent, NULL, m_hInst, (LPVOID) this);
}

void CToolbarWnd::make_url( LPCWSTR url, LPCWSTR basepath, std::wstring& out )
{
	out = url;
}

Gdiplus::Bitmap* CToolbarWnd::get_image( LPCWSTR url )
{
	Gdiplus::Bitmap* bmp = NULL;

	HRSRC hResource = ::FindResource(m_hInst, url, RT_HTML);
	if(hResource)
	{
		DWORD imageSize = ::SizeofResource(m_hInst, hResource);
		if(imageSize)
		{
			const void* pResourceData = ::LockResource(::LoadResource(m_hInst, hResource));
			if(pResourceData)
			{
				HGLOBAL buffer  = ::GlobalAlloc(GMEM_MOVEABLE, imageSize);
				if(buffer)
				{
					void* pBuffer = ::GlobalLock(buffer);
					if (pBuffer)
					{
						CopyMemory(pBuffer, pResourceData, imageSize);

						IStream* pStream = NULL;
						if (::CreateStreamOnHGlobal(buffer, FALSE, &pStream) == S_OK)
						{
							bmp = Gdiplus::Bitmap::FromStream(pStream);
							pStream->Release();
							if (bmp)
							{ 
								if (bmp->GetLastStatus() != Gdiplus::Ok)
								{
									delete bmp;
									bmp = NULL;
								}
							}
						}
						::GlobalUnlock(buffer);
					}
					::GlobalFree(buffer);
				}
			}
		}
	}

	return bmp;
}

void CToolbarWnd::set_caption( const wchar_t* caption )
{

}

void CToolbarWnd::set_base_url( const wchar_t* base_url )
{

}

void CToolbarWnd::link( litehtml::document* doc, litehtml::element::ptr el )
{

}

int CToolbarWnd::set_width( int width )
{
	if(m_doc)
	{
		m_doc->render(width);

		return m_doc->height();
	}
	return 0;
}

void CToolbarWnd::OnMouseMove( int x, int y )
{
	if(m_doc)
	{
		litehtml::position::vector redraw_boxes;
		if(m_doc->on_mouse_over(x, y, redraw_boxes))
		{
			for(litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
			{
				RECT rcRedraw;
				rcRedraw.left	= box->left();
				rcRedraw.right	= box->right();
				rcRedraw.top	= box->top();
				rcRedraw.bottom	= box->bottom();
				InvalidateRect(m_hWnd, &rcRedraw, TRUE);
			}
			UpdateWindow(m_hWnd);
		}
	}
}

void CToolbarWnd::OnMouseLeave()
{
	if(m_doc)
	{
		litehtml::position::vector redraw_boxes;
		if(m_doc->on_mouse_leave(redraw_boxes))
		{
			for(litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
			{
				RECT rcRedraw;
				rcRedraw.left	= box->left();
				rcRedraw.right	= box->right();
				rcRedraw.top	= box->top();
				rcRedraw.bottom	= box->bottom();
				InvalidateRect(m_hWnd, &rcRedraw, TRUE);
			}
			UpdateWindow(m_hWnd);
		}
	}
}

void CToolbarWnd::OnLButtonDown( int x, int y )
{
	if(m_doc)
	{
		litehtml::position::vector redraw_boxes;
		if(m_doc->on_lbutton_down(x, y, redraw_boxes))
		{
			for(litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
			{
				RECT rcRedraw;
				rcRedraw.left	= box->left();
				rcRedraw.right	= box->right();
				rcRedraw.top	= box->top();
				rcRedraw.bottom	= box->bottom();
				InvalidateRect(m_hWnd, &rcRedraw, TRUE);
			}
			UpdateWindow(m_hWnd);
		}
	}
}

void CToolbarWnd::OnLButtonUp( int x, int y )
{
	if(m_doc)
	{
		litehtml::position::vector redraw_boxes;
		if(m_doc->on_lbutton_up(x, y, redraw_boxes))
		{
			for(litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
			{
				RECT rcRedraw;
				rcRedraw.left	= box->left();
				rcRedraw.right	= box->right();
				rcRedraw.top	= box->top();
				rcRedraw.bottom	= box->bottom();
				InvalidateRect(m_hWnd, &rcRedraw, TRUE);
			}
			UpdateWindow(m_hWnd);
		}
	}
}

void CToolbarWnd::on_anchor_click( const wchar_t* url, litehtml::element::ptr el )
{
	if(!wcscmp(url, L"back"))
	{
		m_parent->back();
	} else if(!wcscmp(url, L"forward"))
	{
		m_parent->forward();
	} else if(!wcscmp(url, L"reload"))
	{
		m_parent->reload();
	}
}

void CToolbarWnd::set_cursor( const wchar_t* cursor )
{

}

void CToolbarWnd::import_css( std::wstring& text, const std::wstring& url, std::wstring& baseurl, const litehtml::string_vector& media )
{

}