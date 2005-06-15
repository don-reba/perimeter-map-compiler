#include "stdafx.h"
#include "error handler.h"
#include <fstream>

//-------------------------------
// implementation of ErrorHandler
//-------------------------------

const char * const log_name("error log.txt");

ErrorHandler::ErrorHandler(HWND &hwnd)
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