#pragma once
#include "txwnd.h"
#include <tooltips.h>

#define TTT_TEST_WND	L"TTT_TEST_WND"

class CMainWnd :	public CTxWnd,
					public litehtml::tooltips_callback
{
	HINSTANCE			m_hInst;
	litehtml::tooltips	m_tips;
	litehtml::context	m_html_context;
	int					m_counter;
public:
	CMainWnd(HINSTANCE hInst);
	virtual ~CMainWnd(void);

	virtual void	preRegisterClass( WNDCLASSEX* wcex );
	virtual LRESULT OnMessage( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam );

	virtual CTxDIB* ttcb_get_image(unsigned int id, LPCWSTR url);
	virtual void ttcb_get_text(unsigned int id, std::wstring& text);

	BOOL	create();
};
