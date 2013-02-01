#pragma once

#define ISOLATION_AWARE_ENABLED		1

#define GDIPVER	0x0110

#include "targetver.h"
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include "..\include\litehtml.h"
#include <Wininet.h>

#define TLB_USE_TXDIB
#define TLB_USE_CAIRO
#define TLB_NO_TLBPDK
#define TLB_USE_HTTPREADER

#include <tlbpdklib.h>
