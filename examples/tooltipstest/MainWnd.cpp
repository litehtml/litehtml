#include "globals.h"
#include "MainWnd.h"

CMainWnd::CMainWnd(HINSTANCE hInst) : m_tips(hInst, &m_html_context)
{
	m_counter = 0;
	m_hInst	= hInst;
	setClass(TTT_TEST_WND);

	LPWSTR css = NULL;

	HRSRC hResource = ::FindResource(m_hInst, L"master.css", L"CSS");
	if(hResource)
	{
		DWORD imageSize = ::SizeofResource(m_hInst, hResource);
		if(imageSize)
		{
			LPCSTR pResourceData = (LPCSTR) ::LockResource(::LoadResource(m_hInst, hResource));
			if(pResourceData)
			{
				css = new WCHAR[imageSize * 3];
				int ret = MultiByteToWideChar(CP_UTF8, 0, pResourceData, imageSize, css, imageSize * 3);
				css[ret] = 0;
			}
		}
	}
	if(css)
	{
		m_html_context.load_master_stylesheet(css);
		m_html_context.load_master_stylesheet(L"body {margin:0;}");
		delete css;
	}
	m_tips.set_callback(this);
	//m_tips.set_alpha(200);
}

CMainWnd::~CMainWnd(void)
{
}

void CMainWnd::preRegisterClass( WNDCLASSEX* wcex )
{
	wcex->hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex->hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
}

struct
{
	UINT		id;
	RECT		rc;
	UINT		options;
	LPCWSTR		text;
} g_tools[] =
{
	{1,	{10, 10,	110, 110}, litehtml::tool_opt_align_left,	L"<h1><span style=\"color: red\">Lite HTML</span> engine</h1><hr /><p>The <b>Lite HTML</b> engine is created for embedding into <i>applications</i>, to show the HTML code. Lite HTML supports most of the CSS2/CSS3 standards. The engine is written on C++ with STL (<b>MS Visual Studio 2008</b>) and was tested on Windows platform only.</p><h2>Embedding Lite HTML</h2><p>Firstly, the Lite HTML don't have the code for draw anything. You are free to use any draw engine. We've included the win32_container class as example to show how to implement the draw code.</p>" },
	{2,	{10, 120,	110, 220}, litehtml::tool_opt_ask_text | litehtml::tool_opt_align_left,	L"Simple text" },
	{1,	{300, 300,	350, 350}, litehtml::tool_opt_align_top,							L"<h1><span style=\"color: red\">Lite HTML</span> engine</h1><hr /><p>The <b>Lite HTML</b> engine is created for embedding into <i>applications</i>, to show the HTML code. Lite HTML supports most of the CSS2/CSS3 standards. The engine is written on C++ with STL (<b>MS Visual Studio 2008</b>) and was tested on Windows platform only.</p><h2>Embedding Lite HTML</h2><p>Firstly, the Lite HTML don't have the code for draw anything. You are free to use any draw engine. We've included the win32_container class as example to show how to implement the draw code.</p>" },
	{0,	{0, 0, 0, 0},	NULL },
};

LRESULT CMainWnd::OnMessage( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
	switch(uMessage)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			for(int i = 0; g_tools[i].id; i++)
			{
				FillRect(hdc, &g_tools[i].rc, (HBRUSH) (COLOR_HIGHLIGHT + 1));
			}
		}
		return 0;
	case WM_TIMER:
		m_counter++;
		m_tips.update(2, true);
		return 0;
	}
	return CTxWnd::OnMessage(hWnd, uMessage, wParam, lParam);
}

BOOL CMainWnd::create()
{
	BOOL ret = CTxWnd::create(0, L"Light HTML Tooltips", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, m_hInst);

	SetTimer(m_hWnd, 1, 200, NULL);

	ShowWindow(m_hWnd, SW_SHOW);
	UpdateWindow(m_hWnd);

	for(int i = 0; g_tools[i].id; i++)
	{
		m_tips.add_tool(g_tools[i].id, g_tools[i].text, NULL, &g_tools[i].rc, g_tools[i].options);
	}

	m_tips.create(m_hWnd);
	m_tips.set_style(litehtml::tips_style_baloon);

	return ret;
}

void CMainWnd::ttcb_get_text( unsigned int id, std::wstring& text )
{
	if(id == 2)
	{
		WCHAR cnt[255];
		wsprintf(cnt, L"Current count: <b>%d</b>", m_counter);
		text = cnt;
	}
}

CTxDIB* CMainWnd::ttcb_get_image( unsigned int id, LPCWSTR url )
{
	return NULL;
}