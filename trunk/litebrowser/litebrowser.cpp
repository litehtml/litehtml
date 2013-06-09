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
	CoInitialize(NULL);
	InitCommonControls();

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

