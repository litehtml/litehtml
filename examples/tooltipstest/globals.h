#pragma once

#define ISOLATION_AWARE_ENABLED		1

#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <litehtml.h>
#include <strsafe.h>

// include TxDIB project
#include <TxDIB.h>
#pragma comment(lib, "txdib.lib")

// include CAIRO project
#include <cairo.h>
#include <cairo-win32.h>
#pragma comment(lib, "cairo.lib")

// include SIMPLEDIB project
#include <dib.h>
#pragma comment(lib, "simpledib.lib")

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Msimg32.lib")
