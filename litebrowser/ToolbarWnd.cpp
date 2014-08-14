#include "globals.h"
#include "ToolbarWnd.h"
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
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);

				simpledib::dib dib;

				dib.beginPaint(hdc, &ps.rcPaint);
				pThis->OnPaint(&dib, &ps.rcPaint);
				dib.endPaint();

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

void CToolbarWnd::OnPaint( simpledib::dib* dib, LPRECT rcDraw )
{
	if(m_doc)
	{
		cairo_surface_t* surface = cairo_image_surface_create_for_data((unsigned char*) dib->bits(), CAIRO_FORMAT_ARGB32, dib->width(), dib->height(), dib->width() * 4);
		cairo_t* cr = cairo_create(surface);

		POINT pt;
		GetWindowOrgEx(dib->hdc(), &pt);
		if(pt.x != 0 || pt.y != 0)
		{
			cairo_translate(cr, -pt.x, -pt.y);
		}
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_paint(cr);

		litehtml::position clip(rcDraw->left, rcDraw->top, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top);
		m_doc->draw((litehtml::uint_ptr) cr, 0, 0, &clip);

		cairo_destroy(cr);
		cairo_surface_destroy(surface);
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

CTxDIB* CToolbarWnd::get_image( LPCWSTR url, bool redraw_on_ready )
{
	CTxDIB* img = new CTxDIB;
	if(!img->load(FindResource(m_hInst, url, RT_HTML), m_hInst))
	{
		delete img;
		img = NULL;
	}

	return img;
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
		if(m_doc->on_mouse_over(x, y, x, y, redraw_boxes))
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
		if(m_doc->on_lbutton_down(x, y, x, y, redraw_boxes))
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
		if(m_doc->on_lbutton_up(x, y, x, y, redraw_boxes))
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

struct
{
	LPCWSTR	name;
	LPCWSTR	url;
} g_bookmarks[] = 
{
	{L"DMOZ",					L"http://www.dmoz.org/"},
	{L"litehtml project",		L"https://code.google.com/p/litehtml/"},
	{L"litehtml website",		L"http://www.litehtml.com/"},
	{L"True Launch Bar",		L"http://www.truelaunchbar.com/"},
	{L"Tordex",					L"http://www.tordex.com/"},
	{L"True Paste",				L"http://www.truepaste.com/"},
	{L"Text Accelerator",		L"http://www.textaccelerator.com/"},
	{L"Wiki: Web Browser",		L"http://en.wikipedia.org/wiki/Web_browser"},
	{L"Wiki: Obama",			L"http://en.wikipedia.org/wiki/Obama"},

	{NULL,						NULL},
};

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
	} else if(!wcscmp(url, L"bookmarks"))
	{
		litehtml::position pos = el->get_placement();
		POINT pt;
		pt.x	= pos.right();
		pt.y	= pos.bottom();
		MapWindowPoints(m_hWnd, NULL, &pt, 1);

		HMENU hMenu = CreatePopupMenu();

		for(int i = 0; g_bookmarks[i].url; i++)
		{
			InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, i + 1, g_bookmarks[i].name);
		}

		int ret = TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
		DestroyMenu(hMenu);

		if(ret)
		{
			m_parent->open(g_bookmarks[ret - 1].url);
		}
	} else if(!wcscmp(url, L"settings"))
	{
		litehtml::position pos = el->get_placement();
		POINT pt;
		pt.x	= pos.right();
		pt.y	= pos.bottom();
		MapWindowPoints(m_hWnd, NULL, &pt, 1);

		HMENU hMenu = CreatePopupMenu();

		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING,	1, L"Calculate Render Time");
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, L"");
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING,	2, L"Exit");

		int ret = TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
		DestroyMenu(hMenu);

		switch(ret)
		{
		case 2:
			PostQuitMessage(0);
			break;
		case 1:
			m_parent->calc_time();
			break;
		}
	}
}

void CToolbarWnd::set_cursor( const wchar_t* cursor )
{

}

void CToolbarWnd::import_css( std::wstring& text, const std::wstring& url, std::wstring& baseurl )
{

}

void CToolbarWnd::get_client_rect( litehtml::position& client )
{
	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);
	client.x		= rcClient.left;
	client.y		= rcClient.top;
	client.width	= rcClient.right - rcClient.left;
	client.height	= rcClient.bottom - rcClient.top;
}
