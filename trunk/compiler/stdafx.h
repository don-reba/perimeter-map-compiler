// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#pragma warning(disable: 4100)
#pragma warning(disable: 4702)
#pragma warning(disable: 4355)


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINDOWS 0x0401    // require Win98 or NT 4 at minimum
// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <Commdlg.h>
#include <shlwapi.h>
// C RunTime Header Files
#include <stdlib.h>	
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <crtdbg.h>
#include <time.h>
// some necessary STL
#include <string>
using std::string;
#include <vector>
using std::vector;
// project-wide utilites
#include "util.h"


// additional message crackers
/* void Cls_OnCaptureChanged(HWND hwnd, HWND hwndCaptureReciever) */
#define HANDLE_WM_CAPTURECHANGED(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HWND)(lParam)))


#define ri_cast reinterpret_cast

// TODO: reference additional headers your program requires here
