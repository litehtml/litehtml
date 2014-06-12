#include "globals.h"
#include "HtmlViewWnd.h"
#include "..\litehtml\tokenizer.h"
#include <WindowsX.h>
#include <algorithm>
#include <strsafe.h>

CHTMLViewWnd::CHTMLViewWnd(HINSTANCE hInst, litehtml::context* ctx)
{
	m_hInst			= hInst;
	m_hWnd			= NULL;
	m_top			= 0;
	m_left			= 0;
	m_max_top		= 0;
	m_max_left		= 0;
	m_context		= ctx;
	m_page			= NULL;
	m_page_next		= NULL;

	InitializeCriticalSection(&m_sync);

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
		wc.hCursor        = NULL;
		wc.hbrBackground  = NULL;
		wc.lpszMenuName   = NULL;
		wc.lpszClassName  = HTMLVIEWWND_CLASS;

		RegisterClass(&wc);
	}
}

CHTMLViewWnd::~CHTMLViewWnd(void)
{
	DeleteCriticalSection(&m_sync);
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
		case WM_PAGE_LOADED:
			pThis->OnPageReady();
			return 0;
		case WM_IMAGE_LOADED:
			if(wParam)
			{
				pThis->redraw(NULL, FALSE);
			} else
			{
				pThis->render();
			}
			break;
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
			RemoveProp(hWnd, TEXT("htmlview_this"));
			pThis->OnDestroy();
			delete pThis;
			return 0;
		case WM_VSCROLL:
			pThis->OnVScroll(HIWORD(wParam), LOWORD(wParam));
			return 0;
		case WM_HSCROLL:
			pThis->OnHScroll(HIWORD(wParam), LOWORD(wParam));
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

void CHTMLViewWnd::OnPaint( simpledib::dib* dib, LPRECT rcDraw )
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

	lock();

	web_page* page = get_page(false);

	if(page)
	{

		litehtml::position clip(rcDraw->left, rcDraw->top, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top);
		page->m_doc->draw((litehtml::uint_ptr) cr, -m_left, -m_top, &clip);

		page->release();
	}

	unlock();

	cairo_destroy(cr);
	cairo_surface_destroy(surface);
}

void CHTMLViewWnd::OnSize( int width, int height )
{
	render();
}

void CHTMLViewWnd::OnDestroy()
{

}

void CHTMLViewWnd::create( int x, int y, int width, int height, HWND parent )
{
	m_hWnd = CreateWindow(HTMLVIEWWND_CLASS, L"htmlview", WS_CHILD | WS_VISIBLE, x, y, width, height, parent, NULL, m_hInst, (LPVOID) this);
}

void CHTMLViewWnd::open( LPCWSTR url, bool reload )
{
	std::wstring hash;
	std::wstring s_url = url;

	std::wstring::size_type hash_pos = s_url.find_first_of(L'#');
	if(hash_pos != std::wstring::npos)
	{
		hash = s_url.substr(hash_pos + 1);
		s_url.erase(hash_pos);
	}

	bool open_hash_only = false;

	lock();

	if(m_page)
	{
		if(m_page->m_url == s_url && !reload)
		{
			open_hash_only = true;
		} else
		{
			m_page->m_http.stop();
		}
	}
	if(!open_hash_only)
	{
		if(m_page_next)
		{
			m_page_next->m_http.stop();
			m_page_next->release();
		}
		m_page_next = new web_page(this);
		m_page_next->m_hash	= hash;
		m_page_next->load(s_url.c_str());
	}
	
	unlock();

	if(open_hash_only)
	{
		show_hash(hash);
		update_scroll();
		redraw(NULL, FALSE);
		update_history();
	}
}

void CHTMLViewWnd::render(BOOL calc_time, BOOL do_redraw)
{
	if(!m_hWnd)
	{
		return;
	}

	web_page* page = get_page();

	if(page)
	{
		RECT rcClient;
		GetClientRect(m_hWnd, &rcClient);

		int width	= rcClient.right - rcClient.left;
		int height	= rcClient.bottom - rcClient.top;

		if(calc_time)
		{
			DWORD tic1 = GetTickCount();
			page->m_doc->render(width);
			DWORD tic2 = GetTickCount();
			WCHAR msg[255];
			StringCchPrintf(msg, 255, L"Render time: %d msec", tic2 - tic1);
			MessageBox(m_hWnd, msg, L"litebrowser", MB_ICONINFORMATION | MB_OK);
		} else
		{
			page->m_doc->render(width);
		}

		m_max_top = page->m_doc->height() - height;
		if(m_max_top < 0) m_max_top = 0;

		m_max_left = page->m_doc->width() - width;
		if(m_max_left < 0) m_max_left = 0;

		if(do_redraw)
		{
			update_scroll();
			redraw(NULL, FALSE);
		}

		page->release();
	}
}

void CHTMLViewWnd::redraw(LPRECT rcDraw, BOOL update)
{
	if(m_hWnd)
	{
		InvalidateRect(m_hWnd, rcDraw, TRUE);
		if(update)
		{
			UpdateWindow(m_hWnd);
		}
	}
}

void CHTMLViewWnd::update_scroll()
{
	if(!is_valid_page())
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

	if(m_max_left > 0)
	{
		ShowScrollBar(m_hWnd, SB_HORZ, TRUE);

		RECT rcClient;
		GetClientRect(m_hWnd, &rcClient);

		SCROLLINFO si;
		si.cbSize	= sizeof(SCROLLINFO);
		si.fMask	= SIF_ALL;
		si.nMin		= 0;
		si.nMax		= m_max_left + (rcClient.right - rcClient.left);
		si.nPos		= m_left;
		si.nPage	= rcClient.right - rcClient.left;
		SetScrollInfo(m_hWnd, SB_HORZ, &si, TRUE);
	} else
	{
		ShowScrollBar(m_hWnd, SB_HORZ, FALSE);
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

void CHTMLViewWnd::OnHScroll( int pos, int flags )
{
	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);

	int lineWidth	= 16;
	int pageWidth	= rcClient.right - rcClient.left - lineWidth;

	int newLeft = m_left;

	switch(flags)
	{
	case SB_LINERIGHT:
		newLeft = m_left + lineWidth;
		if(newLeft > m_max_left)
		{
			newLeft = m_max_left;
		}
		break;
	case SB_PAGERIGHT:
		newLeft = m_left + pageWidth;
		if(newLeft > m_max_left)
		{
			newLeft = m_max_left;
		}
		break;
	case SB_LINELEFT:
		newLeft = m_left - lineWidth;
		if(newLeft < 0)
		{
			newLeft = 0;
		}
		break;
	case SB_PAGELEFT:
		newLeft = m_left - pageWidth;
		if(newLeft < 0)
		{
			newLeft = 0;
		}
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		newLeft  = pos;
		if(newLeft < 0)
		{
			newLeft = 0;
		}
		if(newLeft > m_max_left)
		{
			newLeft = m_max_left;
		}
		break;
	}

	if(newLeft != m_left)
	{
		ScrollWindowEx(m_hWnd, m_left - newLeft, 0, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
		m_left  = newLeft;
		SetScrollPos(m_hWnd, SB_HORZ, m_left, TRUE);
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
	web_page* page = get_page();

	if(page)
	{
		open(page->m_url.c_str(), true);

		page->release();
	}
}

void CHTMLViewWnd::set_caption()
{
	web_page* page = get_page();

	if(!page)
	{
		SetWindowText(GetParent(m_hWnd), L"litebrowser");
	} else
	{
		SetWindowText(GetParent(m_hWnd), page->m_caption.c_str());
		page->release();
	}
}

void CHTMLViewWnd::OnMouseMove( int x, int y )
{
	web_page* page = get_page();
	if(page)
	{
		litehtml::position::vector redraw_boxes;
		if(page->m_doc->on_mouse_over(x + m_left, y + m_top, redraw_boxes))
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
				redraw(&rcRedraw, FALSE);
			}
			UpdateWindow(m_hWnd);
			update_cursor();
		}
		page->release();
	}
}

void CHTMLViewWnd::OnMouseLeave()
{
	web_page* page = get_page();

	if(page)
	{
		litehtml::position::vector redraw_boxes;
		if(page->m_doc->on_mouse_leave(redraw_boxes))
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
				redraw(&rcRedraw, FALSE);
			}
			UpdateWindow(m_hWnd);
		}

		page->release();
	}
}

void CHTMLViewWnd::OnLButtonDown( int x, int y )
{
	web_page* page = get_page();

	if(page)
	{
		litehtml::position::vector redraw_boxes;
		if(page->m_doc->on_lbutton_down(x + m_left, y + m_top, redraw_boxes))
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
				redraw(&rcRedraw, FALSE);
			}
			UpdateWindow(m_hWnd);
		}

		page->release();
	}
}

void CHTMLViewWnd::OnLButtonUp( int x, int y )
{
	web_page* page = get_page();

	if(page)
	{
		litehtml::position::vector redraw_boxes;
		if(page->m_doc->on_lbutton_up(x + m_left, y + m_top, redraw_boxes))
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
				redraw(&rcRedraw, FALSE);
			}
			UpdateWindow(m_hWnd);
		}

		page->release();
	}
}

void CHTMLViewWnd::back()
{
	std::wstring url;
	if(m_history.back(url))
	{
		open(url.c_str(), false);
	}
}

void CHTMLViewWnd::forward()
{
	std::wstring url;
	if(m_history.forward(url))
	{
		open(url.c_str(), false);
	}
}

void CHTMLViewWnd::update_cursor()
{
	LPCWSTR defArrow = m_page_next ? IDC_APPSTARTING : IDC_ARROW;

	web_page* page = get_page();

	if(!page)
	{
		SetCursor(LoadCursor(NULL, defArrow));
	} else
	{
		if(page->m_cursor == L"pointer")
		{
			SetCursor(LoadCursor(NULL, IDC_HAND));
		} else
		{
			SetCursor(LoadCursor(NULL, defArrow));
		}
		page->release();
	}
}

void CHTMLViewWnd::get_client_rect( litehtml::position& client )
{
	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);
	client.x		= rcClient.left;
	client.y		= rcClient.top;
	client.width	= rcClient.right - rcClient.left;
	client.height	= rcClient.bottom - rcClient.top;
}

bool CHTMLViewWnd::is_valid_page(bool with_lock)
{
	bool ret_val = true;

	if(with_lock)
	{
		lock();
	}

	if(!m_page || m_page && !m_page->m_doc)
	{
		ret_val = false;
	}

	if(with_lock)
	{
		unlock();
	}

	return ret_val;
}

web_page* CHTMLViewWnd::get_page(bool with_lock)
{
	web_page* ret_val = NULL;
	if(with_lock)
	{
		lock();
	}
	if(is_valid_page(false))
	{
		ret_val = m_page;
		ret_val->add_ref();
	}
	if(with_lock)
	{
		unlock();
	}

	return ret_val;
}

void CHTMLViewWnd::OnPageReady()
{
	lock();
	web_page* page = m_page_next;
	unlock();

	std::wstring hash;

	bool is_ok = false;

	lock();

	if(m_page_next)
	{
		if(m_page)
		{
			m_page->release();
		}
		m_page = m_page_next;
		m_page_next = NULL;
		is_ok = true;
		hash = m_page->m_hash;
	}

	unlock();

	if(is_ok)
	{
		render(FALSE, FALSE);
		m_top	= 0;
		m_left	= 0;
		show_hash(hash);
		update_scroll();
		redraw(NULL, FALSE);
		set_caption();
		update_history();
	}
}

void CHTMLViewWnd::show_hash(std::wstring& hash)
{
	web_page* page = get_page();
	if(page)
	{
		if(!hash.empty())
		{
			WCHAR selector[255];
			StringCchPrintf(selector, 255, L"#%s", hash.c_str());
			element::ptr el = page->m_doc->root()->select_one(selector);
			if(!el)
			{
				StringCchPrintf(selector, 255, L"[name=%s]", hash.c_str());
				el = page->m_doc->root()->select_one(selector);
			}
			if(el)
			{
				litehtml::position pos = el->get_placement();
				m_top = pos.y;
			}
		} else
		{
			m_top = 0;
		}
		if(page->m_hash != hash)
		{
			page->m_hash = hash;
		}
		page->release();
	}
}

void CHTMLViewWnd::update_history()
{
	web_page* page = get_page();

	if(page)
	{
		std::wstring url;
		page->get_url(url);
		
		m_history.url_opened(url);

		page->release();
	}
}
