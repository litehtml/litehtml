#include "globals.h"
#include "HtmlViewWnd.h"
#include "memdc.h"
#include "..\litehtml\tokenizer.h"

using namespace Gdiplus;

CHTMLViewWnd::CHTMLViewWnd(HINSTANCE hInst)
{
	m_hInst		= hInst;
	m_hWnd		= NULL;
	m_doc		= NULL;
	m_top		= 0;
	m_left		= 0;
	m_max_top	= 0;
	m_max_left	= 0;

	WNDCLASS wc;
	if(!GetClassInfo(m_hInst, HTMLVIEWWND_CLASS, &wc))
	{
		ZeroMemory(&wc, sizeof(wc));
		wc.style          = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc    = (WNDPROC)CHTMLViewWnd::WndProc;
		wc.cbClsExtra     = 0;
		wc.cbWndExtra     = 0;
		wc.hInstance      = m_hInst;
		wc.hIcon          = NULL;
		wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground  = (HBRUSH) (COLOR_WINDOW + 1);
		wc.lpszMenuName   = NULL;
		wc.lpszClassName  = HTMLVIEWWND_CLASS;

		RegisterClass(&wc);
	}

	LPWSTR css_text = load_file(L"D:\\WORK\\drawhtml\\include\\master.css");
	if(css_text)
	{
		litehtml::load_master_stylesheet(css_text);
		delete css_text;
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

litehtml::uint_ptr CHTMLViewWnd::create_font( const wchar_t* faceName, int size, int weight, font_style italic, unsigned int decoration )
{
	litehtml::string_vector fonts;
	tokenize(faceName, fonts, L",");
	litehtml::trim(fonts[0]);

	LOGFONT lf;
	ZeroMemory(&lf, sizeof(lf));
	wcscpy_s(lf.lfFaceName, LF_FACESIZE, fonts[0].c_str());

	lf.lfHeight			= -size;
	lf.lfWeight			= weight;
	lf.lfItalic			= (italic == litehtml::fontStyleItalic) ? TRUE : FALSE;
	lf.lfCharSet		= DEFAULT_CHARSET;
	lf.lfOutPrecision	= OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision	= CLIP_DEFAULT_PRECIS;
	lf.lfQuality		= DEFAULT_QUALITY;
	lf.lfStrikeOut		= (decoration & litehtml::font_decoration_linethrough) ? TRUE : FALSE;
	lf.lfUnderline		= (decoration & litehtml::font_decoration_underline) ? TRUE : FALSE;
	HFONT hFont = CreateFontIndirect(&lf);

	return (uint_ptr) hFont;
}

void CHTMLViewWnd::delete_font( uint_ptr hFont )
{
	DeleteObject((HFONT) hFont);
}

int CHTMLViewWnd::line_height( uint_ptr hdc, uint_ptr hFont )
{
	HFONT oldFont = (HFONT) SelectObject((HDC) hdc, (HFONT) hFont);
	TEXTMETRIC tm;
	GetTextMetrics((HDC) hdc, &tm);
	SelectObject((HDC) hdc, oldFont);
	return (int) tm.tmHeight;
}

int CHTMLViewWnd::text_width( uint_ptr hdc, const wchar_t* text, uint_ptr hFont )
{
	HFONT oldFont = (HFONT) SelectObject((HDC) hdc, (HFONT) hFont);

	SIZE sz = {0, 0};

	GetTextExtentPoint32((HDC) hdc, text, lstrlen(text), &sz);

	SelectObject((HDC) hdc, oldFont);

	return (int) sz.cx;
}

void CHTMLViewWnd::draw_text( uint_ptr hdc, const wchar_t* text, uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos )
{
	HFONT oldFont = (HFONT) SelectObject((HDC) hdc, (HFONT) hFont);

	SetBkMode((HDC) hdc, TRANSPARENT);

	SetTextColor((HDC) hdc, RGB(color.red, color.green, color.blue));

	RECT rcText = { pos.left(), pos.top(), pos.right(), pos.bottom() };
	DrawText((HDC) hdc, text, -1, &rcText, DT_SINGLELINE | DT_NOPREFIX | DT_BOTTOM | DT_NOCLIP);

	SelectObject((HDC) hdc, oldFont);
}

void CHTMLViewWnd::fill_rect( uint_ptr hdc, const litehtml::position& pos, litehtml::web_color color )
{
	HBRUSH br = CreateSolidBrush(RGB(color.red, color.green, color.blue));
	RECT rcFill = { pos.left(), pos.top(), pos.right(), pos.bottom() };
	FillRect((HDC) hdc, &rcFill, br);
	DeleteObject(br);
}

litehtml::uint_ptr CHTMLViewWnd::get_temp_dc()
{
	return (litehtml::uint_ptr) GetDC(NULL);
}

void CHTMLViewWnd::release_temp_dc( uint_ptr hdc )
{
	ReleaseDC(NULL, (HDC) hdc);
}

int CHTMLViewWnd::pt_to_px( int pt )
{
	HDC dc = GetDC(NULL);
	int ret = MulDiv(pt, GetDeviceCaps(dc, LOGPIXELSY), 72);
	ReleaseDC(NULL, dc);
	return ret;
}

int CHTMLViewWnd::get_text_base_line( uint_ptr hdc, uint_ptr hFont )
{
	HDC dc = (HDC) hdc;
	if(!dc)
	{
		dc = GetDC(NULL);
	}
	HFONT oldFont = (HFONT) SelectObject(dc, (HFONT) hFont);
	TEXTMETRIC tm;
	GetTextMetrics(dc, &tm);
	SelectObject(dc, oldFont);
	if(!hdc)
	{
		ReleaseDC(NULL, dc);
	}
	return (int) tm.tmDescent;
}

void CHTMLViewWnd::open( LPCWSTR path )
{
	m_doc = NULL;
	m_base_path = L"";

	m_doc_path = path;

	LPWSTR html_text = load_file(path);
	if(html_text)
	{
		WCHAR css_path[MAX_PATH];
		lstrcpy(css_path, path);
		LPWSTR ext = PathFindExtension(css_path);
		lstrcpy(ext, L".css");

		LPWSTR css_text = load_file(css_path);

		LPWSTR base_path = new WCHAR[lstrlen(path) + 1];
		lstrcpy(base_path, path);
		PathRemoveFileSpec(base_path);
		PathAddBackslash(base_path);
		m_base_path = base_path;
		delete base_path;

		m_doc = litehtml::document::createFromString(html_text, this, css_text, NULL);
		delete html_text;

		if(css_text)
		{
			delete css_text;
		}

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

void CHTMLViewWnd::draw_list_marker( uint_ptr hdc, list_style_type marker_type, int x, int y, int height, const web_color& color )
{
	Graphics graphics((HDC) hdc);
	LinearGradientBrush* brush = NULL;

	int top_margin = height / 3;

	Rect rc(x, y + top_margin, 
		height - top_margin * 2, 
		height - top_margin * 2);

	switch(marker_type)
	{
	case list_style_type_circle:
		{
			graphics.SetCompositingQuality(CompositingQualityHighQuality);
			graphics.SetSmoothingMode(SmoothingModeAntiAlias);
			Pen pen( Color(color.alpha, color.red, color.green, color.blue) );
			graphics.DrawEllipse(&pen, rc.X, rc.Y, rc.Width, rc.Height);
		}
		break;
	case list_style_type_disc:
		{
			graphics.SetCompositingQuality(CompositingQualityHighQuality);
			graphics.SetSmoothingMode(SmoothingModeAntiAlias);
			SolidBrush brush( Color(color.alpha, color.red, color.green, color.blue) );
			graphics.FillEllipse(&brush, rc.X, rc.Y, rc.Width, rc.Height);
		}
		break;
	case list_style_type_square:
		{
			SolidBrush brush( Color(color.alpha, color.red, color.green, color.blue) );
			graphics.FillRectangle(&brush, rc.X, rc.Y, rc.Width, rc.Height);
		}
		break;
	}
}

void CHTMLViewWnd::load_image( const wchar_t* src )
{
	if(m_images.find(src) == m_images.end())
	{
		std::wstring path = m_base_path;
		path += src;
		if(PathFileExists(path.c_str()))
		{
			Image* img = Image::FromFile(path.c_str());
			if(img)
			{
				m_images[src] = img;
			}
		}
	}
}

void CHTMLViewWnd::get_image_size( const wchar_t* src, litehtml::size& sz )
{
	images_map::iterator img = m_images.find(src);
	if(img != m_images.end())
	{
		sz.width	= (int) img->second->GetWidth();
		sz.height	= (int) img->second->GetHeight();
	}
}

void CHTMLViewWnd::draw_image( uint_ptr hdc, const wchar_t* src, const litehtml::position& pos )
{
	images_map::iterator img = m_images.find(src);
	if(img != m_images.end())
	{
		Graphics graphics((HDC) hdc);
		graphics.SetInterpolationMode(InterpolationModeNearestNeighbor);
		graphics.SetPixelOffsetMode(PixelOffsetModeHalf);
		graphics.DrawImage(img->second, pos.x, pos.y, pos.width, pos.height);

		img->second->GetWidth();
	}
}

void CHTMLViewWnd::clear_images()
{
	for(images_map::iterator i = m_images.begin(); i != m_images.end(); i++)
	{
		if(i->second)
		{
			delete i->second;
		}
	}
	m_images.clear();
}

LPWSTR CHTMLViewWnd::load_file( LPCWSTR path )
{
	LPWSTR strW = NULL;

	HANDLE fl = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fl != INVALID_HANDLE_VALUE)
	{
		LPWSTR base_path = new WCHAR[lstrlen(path) + 1];
		lstrcpy(base_path, path);
		PathRemoveFileSpec(base_path);
		PathAddBackslash(base_path);
		m_base_path = base_path;
		delete base_path;

		DWORD size = GetFileSize(fl, NULL);
		LPSTR str = (LPSTR) malloc(size + 1);
		DWORD cbRead = 0;
		ReadFile(fl, str, size, &cbRead, NULL);
		str[cbRead] = 0;

		strW = new WCHAR[cbRead + 1];
		MultiByteToWideChar(CP_UTF8, 0, str, cbRead + 1, strW, cbRead + 1);

		free(str);
		CloseHandle(fl);
	}

	return strW;
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

void CHTMLViewWnd::draw_background( uint_ptr hdc, const wchar_t* image, const wchar_t* baseurl, const litehtml::position& draw_pos, const litehtml::css_position& bg_pos, litehtml::background_repeat repeat, litehtml::background_attachment attachment )
{
	load_image(image);

	images_map::iterator img = m_images.find(image);
	if(img != m_images.end())
	{
		Graphics graphics((HDC) hdc);
		graphics.SetInterpolationMode(InterpolationModeNearestNeighbor);
		graphics.SetPixelOffsetMode(PixelOffsetModeHalf);

		Region reg(Rect(draw_pos.left(), draw_pos.top(), draw_pos.width, draw_pos.height));
		graphics.SetClip(&reg);

		litehtml::position pos = draw_pos;

		if(bg_pos.x.units() != css_units_percentage)
		{
			pos.x += (int) bg_pos.x.val();
		} else
		{
			pos.x += (int) ((float) (draw_pos.width - img->second->GetWidth()) * bg_pos.x.val() / 100.0);
		}

		if(bg_pos.y.units() != css_units_percentage)
		{
			pos.y += (int) bg_pos.y.val();
		} else
		{
			pos.y += (int) ( (float) (draw_pos.width - img->second->GetHeight()) * bg_pos.y.val() / 100.0);
		}

		switch(repeat)
		{
		case background_repeat_no_repeat:
			{
				graphics.DrawImage(img->second, pos.x, pos.y, img->second->GetWidth(), img->second->GetHeight());
			}
			break;
		case background_repeat_repeat_x:
			{
				for(int x = pos.left(); x < pos.right(); x += img->second->GetWidth())
				{
					graphics.DrawImage(img->second, x, pos.top(), img->second->GetWidth(), img->second->GetHeight());
				}
			}
			break;
		case background_repeat_repeat_y:
			{
				for(int y = pos.top(); y < pos.bottom(); y += img->second->GetHeight())
				{
					graphics.DrawImage(img->second, pos.left(), y, img->second->GetWidth(), img->second->GetHeight());
				}
			}
			break;
		case background_repeat_repeat:
			{
				if(img->second->GetHeight() >= 0)
				{
					for(int x = pos.left(); x < pos.right(); x += img->second->GetWidth())
					{
						for(int y = pos.top(); y < pos.bottom(); y += img->second->GetHeight())
						{
							graphics.DrawImage(img->second, x, y, img->second->GetWidth(), img->second->GetHeight());
						}
					}
				}
			}
			break;
		}
	}
}
