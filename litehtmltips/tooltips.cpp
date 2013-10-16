#define ISOLATION_AWARE_ENABLED		1
#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>
#include <shlwapi.h>
#include "tooltips.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <Uxtheme.h>
#include <vsstyle.h>

//defines
#define LITEHTML_TOOLTIPS_WINDOW	L"LITEHTML_TOOLTIPS_WINDOW"
#define TIMER_SHOW_TIP		1
#define TIMER_HIDE_TIP		2


litehtml::tooltips::tooltips(HINSTANCE hInst, litehtml::context* html_context)
{
	registerClass(hInst);
	m_hWnd				= NULL;
	m_html_context		= html_context;
	m_max_width			= 300;
	m_show_time			= 500;
	m_hide_time			= 0;
	m_callback			= NULL;
	m_style				= tips_style_square;
	m_over_tool			= 0;
	m_disabled			= false;
	m_alpha				= 255;
	m_cr				= NULL;
	m_surface			= NULL;
	m_last_shown_tool	= 0;
	m_cached_tool		= 0;
	m_mouse_hover_on	= false;

	init_def_font();
}

litehtml::tooltips::~tooltips(void)
{
	DestroyWindow(m_hWnd);
	RemoveWindowSubclass(m_hWndParent, SubclassProc, (DWORD_PTR) this);
	if(m_surface) cairo_surface_destroy(m_surface);
	if(m_cr) cairo_destroy(m_cr);
}

void litehtml::tooltips::make_url( LPCWSTR url, LPCWSTR basepath, std::wstring& out )
{
	out = url;
}

CTxDIB* litehtml::tooltips::get_image( LPCWSTR url, bool redraw_on_ready )
{
	if(m_callback)
	{
		return m_callback->ttcb_get_image(m_show_tool, url);
	}
	return NULL;
}

void litehtml::tooltips::set_caption( const wchar_t* caption )
{

}

void litehtml::tooltips::set_base_url( const wchar_t* base_url )
{

}

void litehtml::tooltips::link( litehtml::document* doc, litehtml::element::ptr el )
{

}

void litehtml::tooltips::import_css( std::wstring& text, const std::wstring& url, std::wstring& baseurl, const litehtml::string_vector& media )
{

}

void litehtml::tooltips::on_anchor_click( const wchar_t* url, litehtml::element::ptr el )
{

}

void litehtml::tooltips::set_cursor( const wchar_t* cursor )
{

}

LRESULT CALLBACK litehtml::tooltips::WndProc( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
	tooltips* pThis = NULL;
	if(IsWindow(hWnd))
	{
		pThis = (tooltips*)GetProp(hWnd, TEXT("tooltips_this"));
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
				pThis = (tooltips*)(lpcs->lpCreateParams);
				SetProp(hWnd, TEXT("tooltips_this"), (HANDLE) pThis);
				pThis->m_hWnd = hWnd;
			}
			break;
		case WM_DESTROY:
			{
				LRESULT ret = pThis->OnMessage(hWnd, uMessage, wParam, lParam);
				RemoveProp(hWnd, TEXT("tooltips_this"));
				pThis->m_hWnd = NULL;
				return ret;
			}
			break;
		}
		return pThis->OnMessage(hWnd, uMessage, wParam, lParam);
	}
	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

void litehtml::tooltips::registerClass( HINSTANCE hInstance )
{
	m_hInst = hInstance;

	WNDCLASSEX wc;
	if(!GetClassInfoEx(hInstance, LITEHTML_TOOLTIPS_WINDOW, &wc))
	{
		ZeroMemory(&wc, sizeof(wc));
		wc.cbSize			= sizeof(wc);
		wc.lpfnWndProc		= (WNDPROC) tooltips::WndProc;
		wc.hInstance		= hInstance;
		wc.cbClsExtra     = 0;
		wc.cbWndExtra     = 0;
		wc.hIcon          = NULL;
		wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground  = (HBRUSH) (COLOR_WINDOW + 1);
		wc.lpszMenuName   = NULL;
		wc.lpszClassName	= LITEHTML_TOOLTIPS_WINDOW;

		RegisterClassEx(&wc);
	}
}

LRESULT litehtml::tooltips::OnMessage( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
	switch(uMessage)
	{
	case WM_TIMER:
		switch(wParam)
		{
		case TIMER_SHOW_TIP:
			stop_timers();
			if(m_over_tool)
			{
				show(m_over_tool);
			}
			break;
		case TIMER_HIDE_TIP:
			stop_timers();
			hide();
			break;
		}
		break;
	}
	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

void litehtml::tooltips::add_tool( unsigned int id, const wchar_t* text, HWND ctl, LPCRECT rc_tool, UINT options )
{
	m_tools[id] = tool(text, ctl, rc_tool, options);
}

void litehtml::tooltips::clear()
{
	m_tools.clear();
}

void litehtml::tooltips::get_client_rect( litehtml::position& client )
{
	client.x		= 0;
	client.y		= 0;
	client.width	= 300;
	client.height	= 300;
}

void litehtml::tooltips::create( HWND parent )
{
	m_hWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_TRANSPARENT, LITEHTML_TOOLTIPS_WINDOW, L"", WS_POPUP, 0, 0, 0, 0, NULL, NULL, m_hInst, (LPVOID) this);
	if(parent)
	{
		m_hWndParent = parent;
		SetWindowSubclass(parent, SubclassProc, (UINT_PTR) this, (DWORD_PTR) this);
	}
}

void litehtml::tooltips::show( unsigned int id, int top, bool is_update, bool re_render )
{
	tool::map::iterator ti = m_tools.find(id);
	if(ti != m_tools.end())
	{
		if(!is_update && m_cached_tool != id)
		{
			clear_images();
		}
		m_show_tool			= id;
		m_last_shown_tool	= id;
		if(!re_render && m_html || !m_html)
		{
			if(m_html)
			{
				m_html = NULL;
			}
			if(ti->second.options & tool_opt_ask_text)
			{
				if(m_callback)
				{
					std::wstring text;
					m_callback->ttcb_get_text(ti->first, text);
					if(!text.empty())
					{
						m_html = litehtml::document::createFromString(text.c_str(), this, m_html_context);
					}
				} else
				{
					return;
				}
			} else
			{
				if(!ti->second.text.empty())
				{
					m_html = litehtml::document::createFromString(ti->second.text.c_str(), this, m_html_context);
				}
			}
		}

		if(!m_html)
		{
			return;
		}
		m_cached_tool = id;

		int w = m_html->render(m_max_width);
		if(w < m_max_width)
		{
			m_html->render(w);
		}

		calc_layout(&ti->second, &m_layout);
		create_dib(m_layout.width, m_layout.height);

		m_top = top;
		if(m_top)
		{
			scroll(0);
		}

		draw_window();
		ShowWindow(m_hWnd, SW_SHOWNA);

		if(m_hide_time)
		{
			SetTimer(m_hWnd, TIMER_HIDE_TIP, m_hide_time, NULL);
		}
	}
}

void litehtml::tooltips::hide()
{
	if(m_show_tool)
	{
		m_show_tool = 0;
		ShowWindow(m_hWnd, SW_HIDE);
	}
	stop_timers();
}

LRESULT CALLBACK litehtml::tooltips::SubclassProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	tooltips* pThis = (tooltips*) dwRefData;

	switch(uMsg)
	{
	case WM_MOUSEWHEEL:
		if(pThis->can_scroll())
		{
			if(pThis->scroll(-GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA * pThis->m_def_font_size * 5))
			{
				pThis->draw_window(TRUE);
			}
			return 0;
		} else
		{
			pThis->hide();
		}
		break;
	case WM_MOUSELEAVE:
		pThis->m_last_shown_tool = 0;
		pThis->hide();
		pThis->start_hover_tracking(false);
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONDBLCLK:
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCMBUTTONDOWN:
	case WM_NCMBUTTONDBLCLK:
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KILLFOCUS:
	case WM_CLOSE:
		pThis->hide();
		break;
	case WM_MOUSEHOVER:
		{
			pThis->m_mouse_hover_on = false;
			if(!pThis->m_disabled)
			{
				unsigned int over_id = pThis->find_tool(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				if(over_id)
				{
					pThis->show(over_id);
				}
			}
		}
		break;
	case WM_MOUSEMOVE:
		if(!pThis->m_disabled)
		{
			unsigned int over_id = pThis->find_tool(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			if(over_id != pThis->m_last_shown_tool)
			{
				pThis->m_last_shown_tool = 0;
			}

			if(over_id != pThis->m_show_tool)
			{
				pThis->hide();
			}

			if(!pThis->m_show_tool && !pThis->m_last_shown_tool)
			{
				pThis->start_hover_tracking(true);
			}
/*

			pThis->m_mouse_pos.x = GET_X_LPARAM(lParam);
			pThis->m_mouse_pos.y = GET_Y_LPARAM(lParam);
			unsigned int over_id = pThis->find_tool(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

			if(!pThis->m_show_tool)
			{
				pThis->stop_timers();
				pThis->m_over_tool = over_id;
				if(pThis->m_over_tool && pThis->m_last_shown_tool != over_id)
				{
					SetTimer(pThis->m_hWnd, TIMER_SHOW_TIP, pThis->m_show_time, NULL);
				}
			} else
			{
				if(pThis->m_show_tool != over_id)
				{
					pThis->hide();
				}
				if(over_id)
				{
					if(over_id != pThis->m_over_tool)
					{
						pThis->m_over_tool = over_id;
						if(pThis->m_over_tool)
						{
							pThis->stop_timers();
							SetTimer(pThis->m_hWnd, TIMER_SHOW_TIP, pThis->m_show_time, NULL);
						}
					}
				}
			}
*/
		}
		break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void litehtml::tooltips::draw_background(tip_layout* layout)
{
	m_bg_cache.draw(m_cr, layout, m_hWnd, m_alpha);

	if(m_html->height() > layout->content_height)
	{
		double sb_ratio = (double) layout->content_height / (double) m_html->height();
		double sb_height = layout->content_height * sb_ratio;
		double sb_top = layout->content_y + m_top * sb_ratio;

		rounded_rect(m_cr, layout->content_x + layout->content_width + 3, (int) sb_top, 4, (int) sb_height, 2, 0);
		cairo_set_source_rgb(m_cr, (double) GetRValue(m_bg_cache.m_clr_border) / 255.0, (double) GetGValue(m_bg_cache.m_clr_border) / 255.0, (double) GetBValue(m_bg_cache.m_clr_border) / 255.0);
		cairo_fill(m_cr);
	}
}

int litehtml::tooltips::tip_width()
{
	int width = m_html->width();
	switch(m_style)
	{
	case tips_style_square:
		width += 8;
		break;
	case tips_style_rounded:
		width += 16;
		break;
	case tips_style_baloon:
		width += 16;
		break;
	}
	return width;
}

int litehtml::tooltips::tip_height()
{
	int height = m_html->height();
	switch(m_style)
	{
	case tips_style_square:
		height += 4;
		break;
	case tips_style_rounded:
		height += 24;
		break;
	case tips_style_baloon:
		height += 24;
		break;
	}
	return height;
}

void litehtml::tooltips::content_point( LPPOINT pt )
{
	pt->x = 0;
	pt->y = 0;
	switch(m_style)
	{
	case tips_style_square:
		pt->x = 4;
		pt->y = 2;
		break;
	case tips_style_rounded:
		pt->x = 12;
		pt->y = 12;
		break;
	case tips_style_baloon:
		pt->x = 12;
		pt->y = 12;
		break;
	}
}

void litehtml::tooltips::rounded_rect( cairo_t* cr, int x, int y, int width, int height, int radius, int line_width )
{
	double xx	= x;
	double yy	= y;
	double w	= width;
	double h	= height;
	double r	= radius;

	if(line_width != 0)
	{
		xx += line_width / 2.0;
		yy += line_width / 2.0;
		w  -= line_width;
		h  -= line_width;
	}

	cairo_new_path(cr);

	cairo_move_to	(cr, xx + r, yy);
	cairo_arc		(cr, xx + w - r,	yy + r,		r, M_PI + M_PI / 2, M_PI * 2        );
	cairo_arc		(cr, xx + w - r,	yy + h - r,	r, 0,               M_PI / 2        );
	cairo_arc		(cr, xx + r,		yy + h - r,	r, M_PI/2,          M_PI            );
	cairo_arc		(cr, xx + r,		yy + r,		r, M_PI,            270 * M_PI / 180);
	cairo_close_path(cr);
}

void litehtml::tooltips::stop_timers()
{
	KillTimer(m_hWnd, TIMER_SHOW_TIP);
	KillTimer(m_hWnd, TIMER_HIDE_TIP);
}

unsigned int litehtml::tooltips::find_tool( int x, int y )
{
	POINT pt = {x, y};
	for(tool::map::iterator t = m_tools.begin(); t != m_tools.end(); t++)
	{
		if(PtInRect(&t->second.rc_tool, pt))
		{
			return t->first;
		}
	}
	return 0;
}

void litehtml::tooltips::calc_layout( tool* t, tip_layout* layout )
{
	layout->style			= m_style;
	layout->width			= m_html->width();
	layout->height			= m_html->height();
	layout->content_width	= m_html->width();
	layout->content_height	= m_html->height();

	switch(m_style)
	{
	case tips_style_square:
		layout->width		+= 8;
		layout->height		+= 4;
		layout->content_x	= 4;
		layout->content_y	= 2;
		break;
	case tips_style_rounded:
		layout->width		+= 16;
		layout->height		+= 16;
		layout->content_x	= 8;
		layout->content_y	= 8;
		break;
	case tips_style_baloon:
		layout->width	+= 24;
		if(layout->width < 32)	layout->width  = 32;
		layout->height	+= 24;
		if(layout->height < 32)	layout->height = 32;
		break;
	}

	RECT rcDesktop;
	GetDesktopRect(&rcDesktop, m_hWndParent);

	if(layout->height > rcDesktop.bottom - rcDesktop.top)
	{
		layout->width += 7;
	}

	RECT rc_tool = t->rc_tool;
	MapWindowPoints(m_hWndParent, NULL, (LPPOINT) &rc_tool, 2);

	calc_position(t->options & tool_opt_align_mask, &rc_tool, layout);

	if(layout->y + layout->height > rcDesktop.bottom)
	{
		int margin = layout->height - m_html->height();
		layout->height = rcDesktop.bottom - layout->y;
		layout->content_height = layout->height - margin;
	} 

	switch(layout->align)
	{
	case tool_opt_align_top:
		layout->anchor_x = rc_tool.left + (rc_tool.right - rc_tool.left) / 2;
		layout->anchor_y = rc_tool.top;
		if(layout->y + layout->height > rc_tool.top)
		{
			layout->content_height	-= (layout->y + layout->height) - rc_tool.top;
			layout->height			-= layout->y + layout->height - rc_tool.top;
			layout->y				=  rc_tool.top - layout->height;
		}
		break;
	case tool_opt_align_bottom:
		layout->anchor_x = rc_tool.left + (rc_tool.right - rc_tool.left) / 2;
		layout->anchor_y = rc_tool.bottom;
		if(rc_tool.bottom > layout->y)
		{
			layout->content_height	-= rc_tool.bottom - layout->y;
			layout->height			-= rc_tool.bottom - layout->y;
			layout->y				=  rc_tool.bottom;
		}
		break;
	case tool_opt_align_left:
		layout->anchor_y = rc_tool.top + (rc_tool.bottom - rc_tool.top) / 2;
		layout->anchor_x = rc_tool.left;
		break;
	case tool_opt_align_right:
		layout->anchor_y = rc_tool.top + (rc_tool.bottom - rc_tool.top) / 2;
		layout->anchor_x = rc_tool.right;
		break;
	}

	if(m_style == tips_style_baloon)
	{
		switch(layout->align)
		{
		case tool_opt_align_top:
			layout->content_x	= 8;
			layout->content_y	= 8;
			if(layout->width - 8 >= 32)
			{
				layout->width -= 8;
			} else
			{
				layout->width = 32;
			}
			break;
		case tool_opt_align_left:
			layout->content_x	= 8;
			layout->content_y	= 8;
			if(layout->height - 8 >= 32)
			{
				layout->height -= 8;
			} else
			{
				layout->height = 32;
			}
			break;
		case tool_opt_align_bottom:
			layout->content_x	= 8;
			layout->content_y	= 16;
			if(layout->width - 8 >= 32)
			{
				layout->width -= 8;
			} else
			{
				layout->width = 32;
			}
			break;
		case tool_opt_align_right:
			layout->content_x	= 16;
			layout->content_y	= 8;
			if(layout->height - 8 >= 32)
			{
				layout->height -= 8;
			} else
			{
				layout->height = 32;
			}
			break;
		}
	}
	// add 5px shadow
	layout->width	+= 5;
	layout->height	+= 5;
}

void litehtml::tooltips::GetDesktopRect(RECT* rcDsk, HWND hWnd)
{
	int nMonitors = GetSystemMetrics(80);
	if(!nMonitors) nMonitors = 1;
	if(nMonitors == 1)
	{
oneMonitor:
		HWND dskWnd = GetDesktopWindow();
		GetClientRect(dskWnd, rcDsk);
	} else
	{
		HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		if(!hMonitor)
		{
			goto oneMonitor;
		}
		MONITORINFO mInf;
		mInf.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(hMonitor, &mInf);
		*rcDsk = mInf.rcMonitor;
	}
}

void litehtml::tooltips::calc_position( UINT align, LPRECT rc_tool, tip_layout* layout, bool second )
{
	RECT rcDesktop;
	GetDesktopRect(&rcDesktop, m_hWndParent);

	int shift = 0;
	if(m_style != tips_style_baloon)
	{
		shift = 8;
	}

	switch(align)
	{
	case tool_opt_align_top:
		layout->align = tool_opt_align_top;
		layout->x = rc_tool->left + (rc_tool->right - rc_tool->left) / 2 - layout->width /2;
		layout->y = rc_tool->top - layout->height - shift;
		if(layout->x + layout->width > rcDesktop.right)
		{
			layout->x = rcDesktop.right - layout->width;
		}
		if(layout->x < rcDesktop.left)
		{
			layout->x = rcDesktop.left;
		}
		if(layout->y < rcDesktop.top)
		{
			if(!second)
			{
				if(rcDesktop.bottom - rc_tool->bottom >= layout->height + shift)
				{
					calc_position(tool_opt_align_bottom, rc_tool, layout, true);
				} else
				{
					if(rc_tool->left - rcDesktop.left > rcDesktop.right - rc_tool->right)
					{
						calc_position(tool_opt_align_left, rc_tool, layout, true);
					} else
					{
						calc_position(tool_opt_align_right, rc_tool, layout, true);
					}
				}
			} else
			{
				layout->y = rcDesktop.top;
			}
		}
		break;
	case tool_opt_align_bottom:
		layout->align = tool_opt_align_bottom;
		layout->x = rc_tool->left + (rc_tool->right - rc_tool->left) / 2 - layout->width /2;
		layout->y = rc_tool->bottom + shift;
		if(layout->x + layout->width > rcDesktop.right)
		{
			layout->x = rcDesktop.right - layout->width;
		}
		if(layout->x < rcDesktop.left)
		{
			layout->x = rcDesktop.left;
		}
		if(layout->y + layout->height > rcDesktop.bottom)
		{
			if(!second)
			{
				if(rc_tool->top - rcDesktop.top >= layout->height + shift)
				{
					calc_position(tool_opt_align_top, rc_tool, layout, true);
				} else
				{
					if(rc_tool->left - rcDesktop.left > rcDesktop.right - rc_tool->right)
					{
						calc_position(tool_opt_align_left, rc_tool, layout, true);
					} else
					{
						calc_position(tool_opt_align_right, rc_tool, layout, true);
					}
				}
			} else
			{
				layout->y = rcDesktop.top;
			}
		}
		break;
	case tool_opt_align_left:
		layout->align = tool_opt_align_left;
		layout->y = rc_tool->top + (rc_tool->bottom - rc_tool->top) / 2 - layout->height /2;
		layout->x = rc_tool->left - layout->width - shift;
		if(layout->y + layout->height > rcDesktop.bottom)
		{
			layout->y = rcDesktop.bottom - layout->height;
		}
		if(layout->y < rcDesktop.top)
		{
			layout->y = rcDesktop.top;
		}
		if(layout->x < rcDesktop.left)
		{
			if(!second)
			{
				if(rcDesktop.right - rc_tool->right >= layout->width + shift)
				{
					calc_position(tool_opt_align_right, rc_tool, layout, true);
				} else
				{
					if(rc_tool->top - rcDesktop.top > rcDesktop.bottom - rc_tool->bottom)
					{
						calc_position(tool_opt_align_top, rc_tool, layout, true);
					} else
					{
						calc_position(tool_opt_align_bottom, rc_tool, layout, true);
					}
				}
			} else
			{
				layout->x = rcDesktop.left;
			}
		}
		break;
	case tool_opt_align_right:
		layout->align = tool_opt_align_right;
		layout->y = rc_tool->top + (rc_tool->bottom - rc_tool->top) / 2 - layout->height /2;
		layout->x = rc_tool->right + shift;
		if(layout->y + layout->height > rcDesktop.bottom)
		{
			layout->y = rcDesktop.bottom - layout->height;
		}
		if(layout->y < rcDesktop.top)
		{
			layout->y = rcDesktop.top;
		}
		if(layout->x + layout->width > rcDesktop.right)
		{
			if(!second)
			{
				if(rc_tool->left - rcDesktop.left >= layout->width + shift)
				{
					calc_position(tool_opt_align_left, rc_tool, layout, true);
				} else
				{
					if(rc_tool->top - rcDesktop.top > rcDesktop.bottom - rc_tool->bottom)
					{
						calc_position(tool_opt_align_top, rc_tool, layout, true);
					} else
					{
						calc_position(tool_opt_align_bottom, rc_tool, layout, true);
					}
				}
			} else
			{
				layout->x = rcDesktop.left;
			}
		}
		break;
	}
}

void litehtml::tooltips::baloon( cairo_t* cr, int x, int y, int width, int height, int ax, int ay, UINT align, int radius, int line_width )
{
	double xx	= x;
	double yy	= y;
	double w	= width;
	double h	= height;
	double r	= radius;
	double axx	= ax;
	double ayy	= ay;

	if(line_width != 0)
	{
		xx += line_width / 2.0;
		yy += line_width / 2.0;
		w  -= line_width;
		h  -= line_width;
	}

	cairo_new_path(cr);

	switch(align)
	{
	case tool_opt_align_top:
		h -= 8;
		if(axx - 8 < xx + 8)		axx = xx + 16;
		if(axx + 8 > xx + w - 8)	axx = xx + w - 16;
		cairo_move_to	(cr, xx + r, yy);
		cairo_arc		(cr, xx + w - r,	yy + r,		r, M_PI + M_PI / 2, M_PI * 2        );
		cairo_arc		(cr, xx + w - r,	yy + h - r,	r, 0,               M_PI / 2        );
		cairo_line_to	(cr, axx + 8.0, yy + h);
		cairo_line_to	(cr, axx, ayy);
		cairo_line_to	(cr, axx - 8.0, yy + h);
		cairo_arc		(cr, xx + r,		yy + h - r,	r, M_PI/2,          M_PI            );
		cairo_arc		(cr, xx + r,		yy + r,		r, M_PI,            270 * M_PI / 180);
		break;
	case tool_opt_align_bottom:
		h -= 8;
		yy += 8;
		if(axx - 8 < xx + 8)		axx = xx + 16;
		if(axx + 8 > xx + w - 8)	axx = xx + w - 16;
		cairo_move_to	(cr, xx + r, yy);
		cairo_line_to	(cr, axx - 8.0, yy);
		cairo_line_to	(cr, axx, ayy);
		cairo_line_to	(cr, axx + 8.0, yy);
		cairo_arc		(cr, xx + w - r,	yy + r,		r, M_PI + M_PI / 2, M_PI * 2        );
		cairo_arc		(cr, xx + w - r,	yy + h - r,	r, 0,               M_PI / 2        );
		cairo_arc		(cr, xx + r,		yy + h - r,	r, M_PI/2,          M_PI            );
		cairo_arc		(cr, xx + r,		yy + r,		r, M_PI,            270 * M_PI / 180);
		break;
	case tool_opt_align_left:
		w -= 8;
		if(ayy - 8 < yy + 8)		ayy = yy + 16;
		if(ayy + 8 > yy + h - 8)	ayy = yy + h - 16;
		cairo_move_to	(cr, xx + r, yy);
		cairo_arc		(cr, xx + w - r,	yy + r,		r, M_PI + M_PI / 2, M_PI * 2        );
		cairo_line_to	(cr, xx + w, ayy - 8);
		cairo_line_to	(cr, axx, ayy);
		cairo_line_to	(cr, xx + w, ayy + 8);
		cairo_arc		(cr, xx + w - r,	yy + h - r,	r, 0,               M_PI / 2        );
		cairo_arc		(cr, xx + r,		yy + h - r,	r, M_PI/2,          M_PI            );
		cairo_arc		(cr, xx + r,		yy + r,		r, M_PI,            270 * M_PI / 180);
		break;
	case tool_opt_align_right:
		w -= 8;
		xx += 8;
		if(ayy - 8 < yy + 8)		ayy = yy + 16;
		if(ayy + 8 > yy + h - 8)	ayy = yy + h - 16;
		cairo_move_to	(cr, xx + r, yy);
		cairo_arc		(cr, xx + w - r,	yy + r,		r, M_PI + M_PI / 2, M_PI * 2        );
		cairo_arc		(cr, xx + w - r,	yy + h - r,	r, 0,               M_PI / 2        );
		cairo_arc		(cr, xx + r,		yy + h - r,	r, M_PI/2,          M_PI            );
		cairo_line_to	(cr, xx, ayy + 8);
		cairo_line_to	(cr, axx, ayy);
		cairo_line_to	(cr, xx, ayy - 8);
		cairo_arc		(cr, xx + r,		yy + r,		r, M_PI,            270 * M_PI / 180);
		break;
	default:
		cairo_move_to	(cr, xx + r, yy);
		cairo_arc		(cr, xx + w - r,	yy + r,		r, M_PI + M_PI / 2, M_PI * 2        );
		cairo_arc		(cr, xx + w - r,	yy + h - r,	r, 0,               M_PI / 2        );
		cairo_arc		(cr, xx + r,		yy + h - r,	r, M_PI/2,          M_PI            );
		cairo_arc		(cr, xx + r,		yy + r,		r, M_PI,            270 * M_PI / 180);
	}

	cairo_close_path(cr);
}

int litehtml::tooltips::get_default_font_size()
{
	return m_def_font_size;
}

const wchar_t* litehtml::tooltips::get_default_font_name()
{
	return m_def_font_name.c_str();
}

void litehtml::tooltips::init_def_font()
{
	LOGFONT lf;
	GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), (LPVOID) &lf);

	m_def_font_name = lf.lfFaceName;

	HDC hdc = GetDC(NULL);
	HFONT old_font = (HFONT) SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
	TEXTMETRIC tm;
	GetTextMetrics(hdc, &tm);
	SelectObject(hdc, old_font);
	ReleaseDC(NULL, hdc);

	m_def_font_size = tm.tmHeight;
}

void litehtml::tooltips::disable( bool val )
{
	m_disabled = val;
	if(val)
	{
		hide();
	}
}

void litehtml::tooltips::update( unsigned int id, bool re_render )
{
	if(id == m_show_tool && IsWindowVisible(m_hWnd))
	{
		show(id, m_top, true, re_render);
	}
}

void litehtml::tooltips::create_dib( int width, int height )
{
	if(!m_cr || !m_dib.hdc() || m_dib.width() != width || m_dib.height() != height)
	{
		if(m_surface) cairo_surface_destroy(m_surface);
		if(m_cr) cairo_destroy(m_cr);

		m_dib.clear();
		m_dib.create(width, height, true);

		m_surface	= cairo_image_surface_create_for_data((unsigned char*) m_dib.bits(), CAIRO_FORMAT_ARGB32, m_dib.width(), m_dib.height(), m_dib.width() * 4);
		m_cr		= cairo_create(m_surface);
	} else
	{
		m_dib.clear();
	}

}

void litehtml::tooltips::draw_window(BOOL clr)
{
	if(clr)
	{
		cairo_save(m_cr);

		cairo_set_operator(m_cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba(m_cr, 0, 0, 0, 0);
		cairo_paint(m_cr);

		cairo_restore(m_cr);
	}

	draw_background(&m_layout);

	litehtml::position clip(m_layout.content_x, m_layout.content_y, m_layout.content_width, m_layout.content_height);
	cairo_save(m_cr);
		cairo_rectangle(m_cr, clip.x, clip.y, clip.width, clip.height);
		cairo_clip(m_cr);
		m_html->draw((litehtml::uint_ptr) m_cr, m_layout.content_x, m_layout.content_y - m_top, &clip);
	cairo_restore(m_cr);

	POINT ptDst;
	POINT ptSrc	= {0, 0};
	SIZE size;

	BLENDFUNCTION bf;
	bf.BlendOp				= AC_SRC_OVER;
	bf.BlendFlags			= 0;
	bf.AlphaFormat			= AC_SRC_ALPHA;
	bf.SourceConstantAlpha	= 255;

	ptDst.x = m_layout.x;
	ptDst.y = m_layout.y;

	size.cx	= m_dib.width();
	size.cy	= m_dib.height();

	UpdateLayeredWindow(m_hWnd, NULL, &ptDst, &size, m_dib.hdc(), &ptSrc, 0, &bf, ULW_ALPHA);
}

BOOL litehtml::tooltips::scroll( int dx )
{
	if(IsWindow(m_hWnd) && IsWindowVisible(m_hWnd))
	{
		int new_top = m_top + dx;
		if(new_top < 0) new_top = 0;
		if(new_top > m_html->height() - m_layout.content_height)
		{
			new_top = m_html->height() - m_layout.content_height;
		}
		if(new_top != m_top)
		{
			m_top = new_top;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL litehtml::tooltips::can_scroll()
{
	if(IsWindow(m_hWnd) && IsWindowVisible(m_hWnd))
	{
		if(m_html->height() > m_layout.content_height)
		{
			return TRUE;
		}
	}
	return FALSE;
}

void litehtml::tooltips::set_def_font( const wchar_t* font_name, int font_size )
{
	if(!font_name)
	{
		init_def_font();
	} else
	{
		m_def_font_name = font_name;
		m_def_font_size	= font_size;
	}
}

void litehtml::tooltips::start_hover_tracking( bool start )
{
	if(start)
	{
		if(!m_mouse_hover_on)
		{
			TRACKMOUSEEVENT tm;
			tm.cbSize		= sizeof(TRACKMOUSEEVENT);
			tm.dwFlags		= TME_HOVER;
			tm.hwndTrack	= m_hWndParent;
			tm.dwHoverTime	= m_show_time;
			TrackMouseEvent(&tm);
			m_mouse_hover_on = true;
		}
	} else
	{
		if(m_mouse_hover_on)
		{
			TRACKMOUSEEVENT tm;
			tm.cbSize		= sizeof(TRACKMOUSEEVENT);
			tm.dwFlags		= TME_HOVER | TME_CANCEL;
			tm.hwndTrack	= m_hWndParent;
			tm.dwHoverTime	= m_show_time;
			TrackMouseEvent(&tm);
			m_mouse_hover_on = false;
		}
	}
}

bool litehtml::tooltips_bg_cache::need_redraw( tip_layout* layout )
{
	if(!m_surface) return true;

	if(	layout->width		!= m_layout.width		||
		layout->height		!= m_layout.height		||
		layout->anchor_x	!= m_layout.anchor_x	||
		layout->anchor_y	!= m_layout.anchor_y	||
		layout->style		!= m_layout.style		||
		layout->align		!= m_layout.align)
	{
		return true;
	}
	return false;
}

void litehtml::tooltips_bg_cache::draw( cairo_t* cr, tip_layout* layout, HWND hWnd, BYTE alpha )
{
	if(need_redraw(layout))
	{
		m_layout = *layout;
		if(m_surface)
		{
			cairo_surface_destroy(m_surface);
			m_surface = NULL;
		}
		if(m_dib.width() != m_layout.width || m_dib.height() != m_layout.height)
		{
			m_dib.create(m_layout.width, m_layout.height);
		} else
		{
			m_dib.clear();
		}
		m_surface = cairo_image_surface_create_for_data((unsigned char*) m_dib.bits(), CAIRO_FORMAT_ARGB32, m_dib.width(), m_dib.height(), m_dib.width() * 4);

		// begin draw background

		cairo_t* m_cr = cairo_create(m_surface);

		COLORREF clr_bg	= GetSysColor(COLOR_INFOBK);
		m_clr_border	= GetSysColor(COLOR_INFOTEXT);

		switch(m_layout.style)
		{
		case tips_style_rounded:
			tooltips::rounded_rect(m_cr, 0, 0, m_dib.width() - 5, m_dib.height() - 5, 8, 1);
			break;
		case tips_style_baloon:
			tooltips::baloon(m_cr, 0, 0, m_dib.width() - 5, m_dib.height() - 5, layout->anchor_x - layout->x, layout->anchor_y - layout->y, layout->align, 8, 1);
			break;
		default:
			cairo_rectangle(m_cr, 0.5, 0.5, m_dib.width() - 1 - 5, m_dib.height() - 1 - 5);
			break;
		}

		BOOL stdDraw = TRUE;

		HTHEME ttTheme = OpenThemeData(hWnd, VSCLASS_TOOLTIP);
		if(ttTheme) 
		{
			simpledib::dib dib_bg;
			dib_bg.create(m_dib.width(), m_dib.height(), true);
			RECT rcDraw = {0, 0, m_dib.width(), m_dib.height()};

			if(DrawThemeBackground(ttTheme, dib_bg, TTP_STANDARD, 0, &rcDraw, &rcDraw) == S_OK)
			{
				cairo_surface_t* bg_sf = cairo_image_surface_create_for_data((unsigned char*) dib_bg.bits(), CAIRO_FORMAT_ARGB32, dib_bg.width(), dib_bg.height(), dib_bg.width() * 4);

				cairo_save(m_cr);
				cairo_clip_preserve(m_cr);
				cairo_set_source_surface(m_cr, bg_sf, 0, 0);
				cairo_paint(m_cr);
				cairo_restore(m_cr);

				int border_idx = dib_bg.width() / 2;

				m_clr_border = RGB(dib_bg.bits()[border_idx].rgbRed, dib_bg.bits()[border_idx].rgbGreen, dib_bg.bits()[border_idx].rgbBlue);

				cairo_surface_destroy(bg_sf);

				stdDraw = FALSE;
			}
			CloseThemeData(ttTheme);
		}

		if(stdDraw)
		{
			cairo_set_source_rgb(m_cr, (double) GetRValue(clr_bg) / 255.0, (double) GetGValue(clr_bg) / 255.0, (double) GetBValue(clr_bg) / 255.0);
			cairo_fill_preserve(m_cr);
		}
		cairo_set_line_width(m_cr, 1);
		cairo_set_source_rgb(m_cr, (double) GetRValue(m_clr_border) / 255.0, (double) GetGValue(m_clr_border) / 255.0, (double) GetBValue(m_clr_border) / 255.0);
		cairo_stroke(m_cr);

		int shadow_width	= 10;
		int shadow_height	= 10;
		switch(m_layout.style)
		{
		case tips_style_rounded:
			shadow_width	= 15;
			shadow_height	= 15;
			break;
		case tips_style_baloon:
			shadow_width	= 15;
			shadow_height	= 15;
			if(layout->align == tool_opt_align_top)
			{
				shadow_height += 8;
			}
			if(layout->align == tool_opt_align_left)
			{
				shadow_width += 8;
			}
			break;
		}

		CTxDIB img;
		img._copy(m_dib.bits(), m_dib.width(), m_dib.height(), TRUE);

		RGBQUAD* pixels = img.getBits();
		int sz = img.getWidth() * img.getHeight();

		for(int i=0; i < sz; i++)
		{
			pixels[i].rgbRed		= 0;
			pixels[i].rgbGreen		= 0;
			pixels[i].rgbBlue		= 0;
		}

		img.resample(img.getWidth() - 5, img.getHeight() - 5);
		cairo_surface_t* img_sf = cairo_image_surface_create_for_data((unsigned char*) img.getBits(), CAIRO_FORMAT_ARGB32, img.getWidth(), img.getHeight(), img.getWidth() * 4);

		// draw shadow at the right side
		{
			simpledib::dib shadow;
			shadow.create(shadow_width, m_dib.height(), true);

			cairo_surface_t*	surface_shadow	= cairo_image_surface_create_for_data((unsigned char*) shadow.bits(), CAIRO_FORMAT_ARGB32, shadow.width(), shadow.height(), shadow.width() * 4);
			cairo_t*			cr_shadow		= cairo_create(surface_shadow);

			cairo_set_source_surface(cr_shadow, img_sf, -m_dib.width() + shadow_width + 5, 5);
			cairo_paint(cr_shadow);
			fastbluralpha(shadow.bits(), shadow.width(), shadow.height(), 5);

			cairo_set_operator(m_cr, CAIRO_OPERATOR_DEST_OVER);

			cairo_save(m_cr);

			cairo_rectangle(m_cr, m_dib.width() - shadow_width, 0, shadow_width, m_dib.height());
			cairo_clip(m_cr);
			cairo_set_source_surface(m_cr, surface_shadow, m_dib.width() - shadow.width(), 0);
			cairo_paint(m_cr);

			cairo_restore(m_cr);

			cairo_destroy(cr_shadow);
			cairo_surface_destroy(surface_shadow);
		}

		// draw shadow at the bottom side
		{
			simpledib::dib shadow;
			shadow.create(m_dib.width(), shadow_height, true);

			cairo_surface_t*	surface_shadow	= cairo_image_surface_create_for_data((unsigned char*) shadow.bits(), CAIRO_FORMAT_ARGB32, shadow.width(), shadow.height(), shadow.width() * 4);
			cairo_t*			cr_shadow		= cairo_create(surface_shadow);

			cairo_set_source_surface(cr_shadow, img_sf, 5, -m_dib.height() + shadow_height + 5);
			cairo_paint(cr_shadow);
			fastbluralpha(shadow.bits(), shadow.width(), shadow.height(), 5);

			cairo_set_operator(m_cr, CAIRO_OPERATOR_DEST_OVER);

			cairo_save(m_cr);

			cairo_rectangle(m_cr, 0, 0, m_dib.width() - shadow_width, m_dib.height());
			cairo_clip(m_cr);
			cairo_set_source_surface(m_cr, surface_shadow, 0, m_dib.height() - shadow.height());
			cairo_paint(m_cr);

			cairo_restore(m_cr);

			cairo_destroy(cr_shadow);
			cairo_surface_destroy(surface_shadow);
		}

		cairo_surface_destroy(img_sf);

		cairo_destroy(m_cr);
	}

	if(m_surface)
	{
		cairo_save(cr);

		cairo_set_source_surface(cr, m_surface, 0, 0);
		if(alpha == 255)
		{
			cairo_paint(cr);
		} else
		{
			cairo_paint_with_alpha(cr, (double) alpha / 255.0);
		}

		cairo_restore(cr);
	}
}

void litehtml::tooltips_bg_cache::fastbluralpha(LPRGBQUAD pixels, int width, int height, int radius)
{
	if (radius < 1) {
		return;
	}

	LPRGBQUAD pix = pixels;
	int w   = width;
	int h   = height;
	int wm  = w - 1;
	int hm  = h - 1;
	int wh  = w * h;
	int div = radius + radius + 1;

	int *a = new int[wh];
	int asum, x, y, i, yp, yi, yw;
	int *vmin = new int[max(w,h)];

	int divsum = (div+1) >> 1;
	divsum *= divsum;
	int *dv = new int[256 * divsum];
	for (i=0; i < 256 * divsum; ++i) 
	{
		dv[i] = (i/divsum);
	}

	yw = yi = 0;

	int* stack = new int[div];


	int stackpointer;
	int stackstart;
	int *sir;
	int rbs;
	int r1 = radius + 1;
	int aoutsum;
	int ainsum;

	for (y = 0; y < h; ++y)
	{
		ainsum	= 0;
		aoutsum = 0;
		asum	= 0;
		for(i =- radius; i <= radius; ++i) 
		{
			sir = stack + i + radius;
			*sir = pix[yi + min(wm, max(i, 0))].rgbReserved;

			rbs = r1 - abs(i);
			asum += (*sir) * rbs;

			if (i > 0)
			{
				ainsum += *sir;
			} else 
			{
				aoutsum += *sir;
			}
		}
		stackpointer = radius;

		for (x=0; x < w; ++x) 
		{
			a[yi] = dv[asum];

			asum -= aoutsum;

			stackstart = stackpointer - radius + div;
			sir = stack + (stackstart % div);

			aoutsum -= *sir;

			if (y == 0) 
			{
				vmin[x] = min(x + radius + 1, wm);
			}

			*sir = pix[yw + vmin[x]].rgbReserved;

			ainsum	+= *sir;
			asum	+= ainsum;

			stackpointer = (stackpointer + 1) % div;
			sir = stack + (stackpointer % div);

			aoutsum += *sir;

			ainsum -= *sir;

			++yi;
		}
		yw += w;
	}
	for (x=0; x < w; ++x)
	{
		ainsum = aoutsum = asum = 0;
		yp =- radius * w;

		for(i=-radius; i <= radius; ++i) 
		{
			yi = max(0, yp) + x;

			sir = stack + i + radius;

			*sir = a[yi];

			rbs = r1 - abs(i);

			asum += a[yi] * rbs;

			if (i > 0) 
			{
				ainsum += *sir;
			} else 
			{
				aoutsum += *sir;
			}

			if (i < hm)
			{
				yp += w;
			}
		}

		yi = x;
		stackpointer = radius;

		for (y=0; y < h; ++y)
		{
			pix[yi].rgbReserved	= dv[asum];

			asum -= aoutsum;

			stackstart = stackpointer - radius + div;
			sir = stack + (stackstart % div);

			aoutsum -= *sir;

			if (x==0)
			{
				vmin[y] = min(y + r1, hm) * w;
			}
			int p = x + vmin[y];

			*sir = a[p];

			ainsum += *sir;

			asum += ainsum;

			stackpointer = (stackpointer + 1) % div;
			sir = stack + stackpointer;

			aoutsum += *sir;
			ainsum -= *sir;

			yi += w;
		}
	}
	delete [] a;
	delete [] vmin;
	delete [] dv;
	delete stack;
}
