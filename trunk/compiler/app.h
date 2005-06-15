#pragma once

#include "error handler.h"
#include "main wnd.h"
#include "preview wnd.h"
#include "project manager.h"
#include "stat wnd.h"

//----------------------------------
// main application class definition
//----------------------------------
class App : ErrorHandler
{
// construction/destruction
public:
	App();
// interface
public:
	bool Initialize(HINSTANCE instance);
	int  Run();
// internal function
private:
	void    Destroy();
	tstring MakeIniFileName();
// data
private:
	// application
	HINSTANCE instance_;
	// windows
	InfoWnd    info_wnd_;
	MainWnd    main_wnd_;
	PreviewWnd preview_wnd_;
	StatWnd    stat_wnd_;
	// other
	ProjectManager project_manager_;
};
