#pragma once

#pragma warning(disable: 4100)
#pragma warning(disable: 4702)
#pragma warning(disable: 4355)
#pragma warning(disable: 4512)

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
typedef std::basic_string<char>    string;
typedef std::basic_string<wchar_t> wstring;
typedef std::basic_string<TCHAR>   tstring;
typedef std::basic_stringstream<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR> > tstringstream;
#include <vector>
using std::vector;
// third party libraries
#include <bzip2-1.0.2\bzlib.h>
#include <FreeImage\Wrapper\FreeImagePlus\FreeImagePlus.h>
#include <jasper\jasper.h>
#include <tinyxml\tinyxml.h>
// project-wide utilites
#include "util.h"
#include "foreach.h"
// useful typedefs and definitions
#define ri_cast reinterpret_cast
typedef unsigned int uint;
typedef unsigned long ulong;
// extremely annoying definitions
#undef min
#undef max
