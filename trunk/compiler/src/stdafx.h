//-----------------------------------------------------------------------------
// Perimeter Map Compiler
// Copyright (c) 2005, Don Reba
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// • Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer. 
// • Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution. 
// • Neither the name of Don Reba nor the names of its contributors may be used
//   to endorse or promote products derived from this software without specific
//   prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------


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
typedef std::basic_stringstream <TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR> > tstringstream;
typedef std::basic_istringstream<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR> > tistringstream;
typedef std::basic_ostringstream<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR> > tostringstream;
#include <vector>
using std::vector;
// third party libraries
#include <bzip2-1.0.2\bzlib.h>
#include "..\FreeImage\FreeImagePlus.h"
#include <jasper\jasper.h>
#include <tinyxml\tinyxml.h>
// project-wide utilites
#include "util.h"
#include "foreach.h"
// useful typedefs and definitions
#define ri_cast reinterpret_cast
typedef unsigned int   uint;
typedef unsigned long  ulong;
typedef unsigned short ushort;
// extremely annoying definitions
#undef min
#undef max
