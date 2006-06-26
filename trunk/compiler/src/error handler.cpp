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


#include "stdafx.h"
#include "error handler.h"
#include <fstream>

//-------------------------------
// implementation of ErrorHandler
//-------------------------------

const char * const log_name("error log.txt");

ErrorHandler::ErrorHandler(HWND hwnd)
	:error_hwnd_(hwnd)
{}

void ErrorHandler::DisplayError(
	const tstring &cause,
	const tstring &function,
	int line)
{
	using std::endl;
	// convert the line number to a string
	TCHAR line_str[16];
	itoa(line, line_str, 10);
	// log the error
	std::ofstream err_log(log_name, std::ios_base::app);
	err_log << function << _T(" ") << line_str << endl << cause << endl << endl;
	// display a message
	tstring message;
	message += _T("Error in ");
	message += function;
	message += _T(" at line ");
	message += line_str;
	message += ".\n";
	message += cause;
	_RPT0(_CRT_ERROR, message.c_str());
	MessageBox(error_hwnd_, message.c_str(), _T("Error"), MB_OK | MB_ICONERROR);
}