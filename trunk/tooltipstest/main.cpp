#include "globals.h"
#include "MainWnd.h"
#include "..\containers\cairo\cairo_font.h"

CRITICAL_SECTION cairo_font::m_sync;

int APIENTRY _tWinMain(HINSTANCE hInstance,
					   HINSTANCE hPrevInstance,
					   LPTSTR    lpCmdLine,
					   int       nCmdShow)
{
	CoInitialize(NULL);
	InitCommonControls();

	InitializeCriticalSectionAndSpinCount(&cairo_font::m_sync, 1000);

	{
		CMainWnd wnd(hInstance);
		wnd.create();

		MSG msg;

		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}

