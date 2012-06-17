#include "globals.h"
#include "HtmlViewWnd.h"
#include "memdc.h"
#include "..\litehtml\tokenizer.h"
#include "downloader.h"
#include <WindowsX.h>

using namespace Gdiplus;

CHTMLViewWnd::CHTMLViewWnd(HINSTANCE hInst, litehtml::context* ctx)
{
	m_hInst		= hInst;
	m_hWnd		= NULL;
	m_doc		= NULL;
	m_top		= 0;
	m_left		= 0;
	m_max_top	= 0;
	m_max_left	= 0;
	m_context	= ctx;
	m_cursor	= L"auto";

	WNDCLASS wc;
	if(!GetClassInfo(m_hInst, HTMLVIEWWND_CLASS, &wc))
	{
		ZeroMemory(&wc, sizeof(wc));
		wc.style          = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc    = (WNDPROC)CHTMLViewWnd::WndProc;
		wc.cbClsExtra     = 0;
		wc.cbWndExtra     = 0;
		wc.hInstance      = m_hInst;
		wc.hIcon          = NULL;
		wc.hCursor        = NULL/*LoadCursor(NULL, IDC_ARROW)*/;
		wc.hbrBackground  = NULL;
		wc.lpszMenuName   = NULL;
		wc.lpszClassName  = HTMLVIEWWND_CLASS;

		RegisterClass(&wc);
	}
}

CHTMLViewWnd::~CHTMLViewWnd(void)
{
	clear_images();
}

LRESULT CALLBACK CHTMLViewWnd::WndProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
	CHTMLViewWnd* pThis = NULL;
	if(IsWindow(hWnd))
	{
		pThis = (CHTMLViewWnd*)GetProp(hWnd, TEXT("htmlview_this"));
		if(pThis && pThis->m_hWnd != hWnd)
		{
			pThis = NULL;
		}
	}

	if(pThis || uMessage == WM_CREATE)
	{
		switch (uMessage)
		{
		case WM_SETCURSOR:
			pThis->update_cursor();
			break;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_CREATE:
			{
				LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
				pThis = (CHTMLViewWnd*)(lpcs->lpCreateParams);
				SetProp(hWnd, TEXT("htmlview_this"), (HANDLE) pThis);
				pThis->m_hWnd = hWnd;
				pThis->OnCreate();
			}
			break;
		case WM_PAINT:
			{

/*
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);

				FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW + 1));

				pThis->OnPaint(hdc, &ps.rcPaint);

				EndPaint(hWnd, &ps);
*/

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
			RemoveProp(hWnd, TEXT("htmlview_this"));
			pThis->OnDestroy();
			delete pThis;
			return 0;
		case WM_VSCROLL:
			pThis->OnVScroll(HIWORD(wParam), LOWORD(wParam));
			return 0;
		case WM_MOUSEWHEEL:
			pThis->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
			return 0;
		case WM_KEYDOWN:
			pThis->OnKeyDown((UINT) wParam);
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

void CHTMLViewWnd::OnCreate()
{

}

void CHTMLViewWnd::OnPaint( HDC hdc, LPCRECT rcClip )
{
	if(m_doc)
	{
		if(rcClip)
		{
			litehtml::position clip(rcClip->left, rcClip->top, rcClip->right - rcClip->left, rcClip->bottom - rcClip->top);
			m_doc->draw((litehtml::uint_ptr) hdc, -m_left, -m_top, &clip);
		} else
		{
			m_doc->draw((litehtml::uint_ptr) hdc, -m_left, -m_top, 0);
		}
	}
}

void CHTMLViewWnd::OnSize( int width, int height )
{
	render();
	update_scroll();
}

void CHTMLViewWnd::OnDestroy()
{

}

void CHTMLViewWnd::create( int x, int y, int width, int height, HWND parent )
{
	m_hWnd = CreateWindow(HTMLVIEWWND_CLASS, L"htmlview", WS_CHILD | WS_VISIBLE, x, y, width, height, parent, NULL, m_hInst, (LPVOID) this);
}

void CHTMLViewWnd::open( LPCWSTR path )
{
	if(!m_doc_path.empty())
	{
		m_history_back.push_back(m_doc_path);
	}
	make_url(path, NULL, m_doc_path);

	m_doc		= NULL;
	m_base_path = m_doc_path;

	LPWSTR html_text = load_text_file(m_doc_path.c_str(), true);
	if(html_text)
	{
		m_doc = litehtml::document::createFromString(html_text, this, m_context);
		delete html_text;
	}


	m_top	= 0;
	m_left	= 0;
	render();
	update_scroll();
}

void CHTMLViewWnd::render()
{
	if(!m_hWnd || !m_doc)
	{
		return;
	}

	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);
	HDC hdc = GetDC(m_hWnd);
	m_doc->render((litehtml::uint_ptr) hdc, rcClient.right - rcClient.left);
	ReleaseDC(m_hWnd, hdc);

	m_max_top = m_doc->height() - (rcClient.bottom - rcClient.top);
	if(m_max_top < 0) m_max_top = 0;

	m_max_left = m_doc->width() - (rcClient.right - rcClient.left);
	if(m_max_left < 0) m_max_left = 0;

	redraw();
}

void CHTMLViewWnd::redraw()
{
	if(m_hWnd)
	{
		InvalidateRect(m_hWnd, NULL, TRUE);
	}
}

void CHTMLViewWnd::update_scroll()
{
	if(!m_doc)
	{
		ShowScrollBar(m_hWnd, SB_BOTH, FALSE);
		return;
	}

	if(m_max_top > 0)
	{
		ShowScrollBar(m_hWnd, SB_VERT, TRUE);

		RECT rcClient;
		GetClientRect(m_hWnd, &rcClient);

		SCROLLINFO si;
		si.cbSize	= sizeof(SCROLLINFO);
		si.fMask	= SIF_ALL;
		si.nMin		= 0;
		si.nMax		= m_max_top + (rcClient.bottom - rcClient.top);
		si.nPos		= m_top;
		si.nPage	= rcClient.bottom - rcClient.top;
		SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);
	} else
	{
		ShowScrollBar(m_hWnd, SB_VERT, FALSE);
	}
}

void CHTMLViewWnd::OnVScroll( int pos, int flags )
{
	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);

	int lineHeight	= 16;
	int pageHeight	= rcClient.bottom - rcClient.top - lineHeight;

	int newTop = m_top;

	switch(flags)
	{
	case SB_LINEDOWN:
		newTop = m_top + lineHeight;
		if(newTop > m_max_top)
		{
			newTop = m_max_top;
		}
		break;
	case SB_PAGEDOWN:
		newTop = m_top + pageHeight;
		if(newTop > m_max_top)
		{
			newTop = m_max_top;
		}
		break;
	case SB_LINEUP:
		newTop = m_top - lineHeight;
		if(newTop < 0)
		{
			newTop = 0;
		}
		break;
	case SB_PAGEUP:
		newTop = m_top - pageHeight;
		if(newTop < 0)
		{
			newTop = 0;
		}
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		newTop  = pos;
		if(newTop < 0)
		{
			newTop = 0;
		}
		if(newTop > m_max_top)
		{
			newTop = m_max_top;
		}
		break;
	}

	if(newTop != m_top)
	{
		ScrollWindowEx(m_hWnd, 0, m_top - newTop, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
		m_top  = newTop;
		SetScrollPos(m_hWnd, SB_VERT, m_top, TRUE);
		UpdateWindow(m_hWnd);
	}
}

void CHTMLViewWnd::OnMouseWheel( int delta )
{
	int lineHeight	= 16;

	int newTop  = m_top - delta / WHEEL_DELTA * lineHeight * 3;

	if(newTop < 0)
	{
		newTop = 0;
	}
	if(newTop > m_max_top)
	{
		newTop = m_max_top;
	}

	if(newTop != m_top)
	{
		ScrollWindowEx(m_hWnd, 0, m_top - newTop, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
		m_top  = newTop;
		SetScrollPos(m_hWnd, SB_VERT, m_top, TRUE);
		UpdateWindow(m_hWnd);
	}
}

void CHTMLViewWnd::OnKeyDown( UINT vKey )
{
	switch(vKey)
	{
	case VK_F5:
		refresh();
		break;
	}
}

void CHTMLViewWnd::refresh()
{
	open(m_doc_path.c_str());
	InvalidateRect(m_hWnd, NULL, TRUE);
	UpdateWindow(m_hWnd);
}

void CHTMLViewWnd::set_caption( const wchar_t* caption )
{
	if(caption)
	{
		SetWindowText(GetParent(m_hWnd), caption);
	}
}

void CHTMLViewWnd::link( litehtml::document* doc, litehtml::element::ptr el )
{
	const wchar_t* rel = el->get_attr(L"rel");
	if(rel && !wcscmp(rel, L"stylesheet"))
	{
		const wchar_t* media = el->get_attr(L"media", L"screen");
		if(media && wcsstr(media, L"screen"))
		{
			const wchar_t* href = el->get_attr(L"href");
			if(href)
			{
				std::wstring url;
				make_url(href, NULL, url);
				LPWSTR css = load_text_file(url.c_str());
				if(css)
				{
					doc->add_stylesheet(css, url.c_str());
					delete css;
				}
			}
		}
	}
}

void CHTMLViewWnd::make_url( LPCWSTR url, LPCWSTR basepath, std::wstring& out )
{
	if(PathIsRelative(url) && !PathIsURL(url))
	{
		WCHAR abs_url[512];
		DWORD dl = 512;
		if(basepath && basepath[0])
		{
			UrlCombine(basepath, url, abs_url, &dl, 0);
		} else
		{
			UrlCombine(m_base_path.c_str(), url, abs_url, &dl, 0);
		}
		out = abs_url;
	} else
	{
		if(PathIsURL(url))
		{
			out = url;
		} else
		{
			WCHAR abs_url[512];
			DWORD dl = 512;
			UrlCreateFromPath(url, abs_url, &dl, 0);
			out = abs_url;
		}
	}
	if(out.substr(0, 8) == L"file:///")
	{
		out.erase(5, 1);
	}
}

void CHTMLViewWnd::set_base_url( const wchar_t* base_url )
{
	if(base_url)
	{
		if(PathIsRelative(base_url) && !PathIsURL(base_url))
		{
			make_url(base_url, NULL, m_base_path);
		} else
		{
			m_base_path = base_url;
		}
	} else
	{
		m_base_path = m_doc_path;
	}
}

void CHTMLViewWnd::OnMouseMove( int x, int y )
{
	if(m_doc)
	{
		litehtml::position::vector redraw_boxes;
		if(m_doc->on_mouse_over(x + m_left, y + m_top, redraw_boxes))
		{
			for(litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
			{
				box->x -= m_left;
				box->y -= m_top;
				RECT rcRedraw;
				rcRedraw.left	= box->left();
				rcRedraw.right	= box->right();
				rcRedraw.top	= box->top();
				rcRedraw.bottom	= box->bottom();
				InvalidateRect(m_hWnd, &rcRedraw, TRUE);
			}
			UpdateWindow(m_hWnd);
			update_cursor();
		}
	}
}

Gdiplus::Bitmap* CHTMLViewWnd::get_image( LPCWSTR url )
{
	Gdiplus::Bitmap* img = NULL;

	CRemotedFile rf;

	HANDLE hFile = rf.Open(url);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		DWORD szHigh;
		DWORD szLow = GetFileSize(hFile, &szHigh);
		HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, szHigh, szLow, NULL);
		if(hMapping)
		{
			SIZE_T memSize;
			if(szHigh)
			{
				memSize = MAXDWORD;
			} else
			{
				memSize = szLow;
			}
			LPVOID data = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, memSize);

			HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, memSize);
			LPVOID hgData = GlobalLock(hGlobal);
			CopyMemory(hgData, data, memSize);
			GlobalUnlock(hGlobal);

			UnmapViewOfFile(data);
			CloseHandle(hMapping);

			IStream* pStream = NULL;
			if (::CreateStreamOnHGlobal(hGlobal, TRUE, &pStream) == S_OK)
			{
				img = Gdiplus::Bitmap::FromStream(pStream);
				pStream->Release();
				if(img)
				{ 
					if (img->GetLastStatus() != Gdiplus::Ok)
					{
						delete img;
						img = NULL;
					}
				}
			}
		}
	}

	return img;
}

void CHTMLViewWnd::on_anchor_click( const wchar_t* url, litehtml::element::ptr el )
{
	make_url(url, NULL, m_anchor);
}

void CHTMLViewWnd::OnMouseLeave()
{
	if(m_doc)
	{
		litehtml::position::vector redraw_boxes;
		if(m_doc->on_mouse_leave(redraw_boxes))
		{
			for(litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
			{
				box->x -= m_left;
				box->y -= m_top;
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

void CHTMLViewWnd::OnLButtonDown( int x, int y )
{
	if(m_doc)
	{
		litehtml::position::vector redraw_boxes;
		if(m_doc->on_lbutton_down(x + m_left, y + m_top, redraw_boxes))
		{
			for(litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
			{
				box->x -= m_left;
				box->y -= m_top;
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

void CHTMLViewWnd::OnLButtonUp( int x, int y )
{
	if(m_doc)
	{
		m_anchor = L"";
		litehtml::position::vector redraw_boxes;
		if(m_doc->on_lbutton_up(x + m_left, y + m_top, redraw_boxes))
		{
			for(litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++)
			{
				box->x -= m_left;
				box->y -= m_top;
				RECT rcRedraw;
				rcRedraw.left	= box->left();
				rcRedraw.right	= box->right();
				rcRedraw.top	= box->top();
				rcRedraw.bottom	= box->bottom();
				InvalidateRect(m_hWnd, &rcRedraw, TRUE);
			}
			UpdateWindow(m_hWnd);
		}
		if(!m_anchor.empty())
		{
			open(m_anchor.c_str());
		}
	}
}

void CHTMLViewWnd::back()
{
	if(!m_history_back.empty())
	{
		if(!m_doc_path.empty())
		{
			m_history_forward.push_back(m_doc_path);
		}
		std::wstring url = m_history_back.back();
		m_history_back.pop_back();
		m_doc_path = L"";
		open(url.c_str());
	}
}

void CHTMLViewWnd::forward()
{
	if(!m_history_forward.empty())
	{
		if(!m_doc_path.empty())
		{
			m_history_back.push_back(m_doc_path);
		}
		std::wstring url = m_history_forward.back();
		m_history_forward.pop_back();
		m_doc_path = L"";
		open(url.c_str());
	}
}

void CHTMLViewWnd::set_cursor( const wchar_t* cursor )
{
	m_cursor = cursor;
	if(m_cursor != L"auto")
	{
		int i = 0;
		i++;
	}
}

void CHTMLViewWnd::update_cursor()
{
	if(m_cursor == L"pointer")
	{
		SetCursor(LoadCursor(NULL, IDC_HAND));
	} else
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	}
}
