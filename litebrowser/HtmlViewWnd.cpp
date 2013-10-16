#include "globals.h"
#include "HtmlViewWnd.h"
#include "..\litehtml\tokenizer.h"
#include "downloader.h"
#include <WindowsX.h>
#include <algorithm>
#include <strsafe.h>

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

	m_hImageAdded = CreateEvent(NULL, FALSE, FALSE, NULL);
	InitializeCriticalSection(&m_images_queue_sync);

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
	DeleteCriticalSection(&m_images_queue_sync);
	CTxThread::Stop();
	CloseHandle(m_hImageAdded);
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
		m_doc->draw((litehtml::uint_ptr) cr, -m_left, -m_top, &clip);

		cairo_destroy(cr);
		cairo_surface_destroy(surface);
	}
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
	CTxThread::Run();
}

void CHTMLViewWnd::open( LPCWSTR path, bool reload )
{
	if(!m_doc_path.empty())
	{
		m_history_back.push_back(m_doc_path);
	}
	std::wstring hash;
	std::wstring s_path = path;

	make_url(path, NULL, s_path);

	std::wstring::size_type hash_pos = s_path.find_first_of(L'#');
	if(hash_pos != std::wstring::npos)
	{
		hash = s_path.substr(hash_pos + 1);
		s_path.erase(hash_pos);
	}

	if(s_path != m_doc_path || reload)
	{
		make_url(path, NULL, m_doc_path);

		m_doc		= NULL;
		m_base_path = m_doc_path;

		LPWSTR html_text = load_text_file(m_doc_path.c_str(), true);
		if(html_text)
		{
			m_doc = litehtml::document::createFromString(html_text, this, m_context);
			delete html_text;
		}
		render(FALSE, FALSE);
	}

	m_top	= 0;
	m_left	= 0;

	if(!hash.empty())
	{
		if(m_doc)
		{
			WCHAR selector[255];
			StringCchPrintf(selector, 255, L"#%s", hash.c_str());
			element::ptr el = m_doc->root()->select_one(selector);
			if(!el)
			{
				StringCchPrintf(selector, 255, L"[name=%s]", hash.c_str());
				el = m_doc->root()->select_one(selector);
			}
			if(el)
			{
				litehtml::position pos = el->get_placement();
				m_top = pos.y;
			}
		}
	}

	update_scroll();
	redraw(NULL, FALSE);
}

void CHTMLViewWnd::render(BOOL calc_time, BOOL do_redraw)
{
	if(!m_hWnd || !m_doc)
	{
		return;
	}

	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);

	int width	= rcClient.right - rcClient.left;
	int height	= rcClient.bottom - rcClient.top;

	if(calc_time)
	{
		DWORD tic1 = GetTickCount();
		m_doc->render(width);
		DWORD tic2 = GetTickCount();
		WCHAR msg[255];
		StringCchPrintf(msg, 255, L"Render time: %d msec", tic2 - tic1);
		MessageBox(m_hWnd, msg, L"litebrowser", MB_ICONINFORMATION | MB_OK);
	} else
	{
		m_doc->render(width);
	}

	m_max_top = m_doc->height() - height;
	if(m_max_top < 0) m_max_top = 0;

	m_max_left = m_doc->width() - width;
	if(m_max_left < 0) m_max_left = 0;

	if(do_redraw)
	{
		update_scroll();
		redraw(NULL, FALSE);
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
	open(m_doc_path.c_str(), true);
	redraw(NULL, TRUE);
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
		if(media && (wcsstr(media, L"screen") || wcsstr(media, L"all")))
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
				redraw(&rcRedraw, FALSE);
			}
			UpdateWindow(m_hWnd);
			update_cursor();
		}
	}
}

CTxDIB* CHTMLViewWnd::get_image( LPCWSTR url, bool redraw_on_ready )
{
	lock_images_queue();
	m_images_queue.push_back(image_queue_item(url, redraw_on_ready));
	unlock_images_queue();

	SetEvent(m_hImageAdded);

	return NULL;
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
				redraw(&rcRedraw, FALSE);
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
				redraw(&rcRedraw, FALSE);
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
				redraw(&rcRedraw, FALSE);
			}
			UpdateWindow(m_hWnd);
		}
		if(!m_anchor.empty())
		{
			open(m_anchor.c_str(), false);
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
		open(url.c_str(), false);
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
		open(url.c_str(), false);
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

void CHTMLViewWnd::import_css( std::wstring& text, const std::wstring& url, std::wstring& baseurl, const string_vector& media )
{
	if(media.empty() || std::find(media.begin(), media.end(), std::wstring(L"all")) != media.end() || std::find(media.begin(), media.end(), std::wstring(L"screen")) != media.end())
	{
		std::wstring css_url;
		make_url(url.c_str(), baseurl.c_str(), css_url);
		LPWSTR css = load_text_file(css_url.c_str());
		if(css)
		{
			baseurl = css_url;
			text = css;
			delete css;
		}
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

DWORD CHTMLViewWnd::ThreadProc()
{
	bool update = false;
	while(CTxThread::WaitForStop2(m_hImageAdded, INFINITE) == WAIT_OBJECT_0 + 1)
	{
		update = false;
		bool re_render = false;
		while(!m_images_queue.empty())
		{
			litehtml::tstring url;
			bool redraw_on_ready;

			lock_images_queue();

				redraw_on_ready = m_images_queue[0].redraw_on_ready;
				url				= m_images_queue[0].url;
				m_images_queue.erase(m_images_queue.begin());

			unlock_images_queue();

			CTxDIB* img = NULL;

			CRemotedFile rf;

			HANDLE hFile = rf.Open(url.c_str());
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

					img = new CTxDIB;
					if(!img->load((LPBYTE) data, (DWORD) memSize))
					{
						delete img;
						img = NULL;
					}

					UnmapViewOfFile(data);
					CloseHandle(hMapping);
				}
			}
			if(img)
			{
				cairo_container::add_image(url, img);
				if(redraw_on_ready)
				{
					PostMessage(m_hWnd, WM_IMAGE_LOADED, (WPARAM) 1, 0);
				} else
				{
					re_render = true;
				}
			}
		}
		if(re_render)
		{
			PostMessage(m_hWnd, WM_IMAGE_LOADED, (WPARAM) 0, 0);
		}
	}

	return 0;
}
