#include "globals.h"
#include "litebrowser.h"
#include "BrowserWnd.h"

#pragma comment( lib, "gdiplus.lib" )
#pragma comment( lib, "shlwapi.lib" )

using namespace Gdiplus;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	InitCommonControls();

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	{
		CBrowserWnd wnd(hInstance);

		wnd.create();
		if(lpCmdLine)
		{
			wnd.open(lpCmdLine);
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

