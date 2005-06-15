#pragma once

//---------------------------------------------
// base class for display and logging of errors
//---------------------------------------------
class ErrorHandler
{
public:
	ErrorHandler(HWND &hwnd);
public:
	void DisplayError(
		const tstring &cause,
		const tstring &function,
		int line);
protected:
	HWND &error_hwnd_;
};

#define MacroDisplayError(cause) \
	DisplayError(cause, __FUNCTION__, __LINE__)