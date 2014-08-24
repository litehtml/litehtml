#include "globals.h"
#include "litebrowser.h"
#include "BrowserWnd.h"
#include "..\containers\cairo\cairo_font.h"

#pragma comment( lib, "gdiplus.lib" )
#pragma comment( lib, "shlwapi.lib" )

using namespace Gdiplus;

CRITICAL_SECTION cairo_font::m_sync;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	CoInitialize(NULL);
	InitCommonControls();

	InitializeCriticalSectionAndSpinCount(&cairo_font::m_sync, 1000);

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	{
		CBrowserWnd wnd(hInstance);

		wnd.create();
		if(lpCmdLine && lpCmdLine[0])
		{
			wnd.open(lpCmdLine);
		} else
		{
			wnd.open(L"http://www.dmoz.org/");
		}

		MSG msg;

		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	GdiplusShutdown(gdiplusToken);

	return 0;
}

