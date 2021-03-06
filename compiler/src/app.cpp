//-----------------------------------------------------------------------------
// Perimeter Map Compiler
// Copyright (c) 2005, Don Reba
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// ? Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer. 
// ? Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution. 
// ? Neither the name of Don Reba nor the names of his contributors may be used
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

#include "app.h"
#include "app data.h"
#include "resource.h"

#include <shlobj.h>

//--------
// WinMain
//--------

int APIENTRY _tWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	//_CrtSetDbgFlag(
	//	_CRTDBG_ALLOC_MEM_DF
	//	| _CRTDBG_CHECK_ALWAYS_DF
	//	| _CRTDBG_LEAK_CHECK_DF);
	App app;
	if (!app.Initialize(hInstance, lpCmdLine))
		return 1;
	return app.Run();
}

//--------------------------------------
// main application class implementation
//--------------------------------------

App::App()
	:ErrorHandler(main_wnd_.hwnd_)
	,project_manager_(main_wnd_, info_wnd_, preview_wnd_, stat_wnd_)
	,main_wnd_(project_manager_, info_wnd_, preview_wnd_, stat_wnd_)
	,info_wnd_(preview_wnd_, &project_manager_.zero_level_changed_)
{}

bool App::Initialize(HINSTANCE instance, LPCTSTR cmd_line)
{
	instance_ = instance;
	InitCommonControls();
	if (0 != jas_init())	
		MacroDisplayError("JasPer initialization failed.");
	FreeImage_Initialise(TRUE);
	// load application data
	SSAppData::Load(MakeIniFileName().c_str());
	SSAppData::Output();
	// initialize the main window
	{
		POINT pos =
		{
			MacroAppData(ID_MAIN_WND_RECT).left,
			MacroAppData(ID_MAIN_WND_RECT).top
		};
		if (!main_wnd_.Create(pos))
			return false;
	}
	// add the panels to the main window
	{
		using MainWnd::PanelInfo;
		PanelInfo panels[MainWnd::panel_count] =
		{
			PanelInfo(&preview_wnd_, IDB_PREVIEW, _T("Toggle Preview Panel"),     MacroAppData(ID_PREVIEW_WND_RECT)),
			PanelInfo(&stat_wnd_,    IDB_STATS,   _T("Toggle Statistics Panel"),  MacroAppData(ID_STAT_WND_RECT)),
			PanelInfo(&info_wnd_,    IDB_INFO,    _T("Toggle Map Details Panel"), MacroAppData(ID_INFO_WND_RECT))
		};
		main_wnd_.AddPanelWnds(panels);
	}
	// initialize the project manager
	if (!project_manager_.Initialize())
		return false;
	// toggle panels' visibility
	main_wnd_.ShowPanel(0, MacroAppData(ID_PREVIEW_WND_VISIBLE));
	main_wnd_.ShowPanel(1, MacroAppData(ID_STAT_WND_VISIBLE));
	main_wnd_.ShowPanel(2, MacroAppData(ID_INFO_WND_VISIBLE));
	// show the main window
	ShowWindow(main_wnd_.hwnd_, SW_SHOW);
	SetForegroundWindow(main_wnd_.hwnd_);
	// parse the command line, and carry out appropriate actions
	{
		vector<TCHAR> path_vector(MAX_PATH);
		TCHAR *path(&path_vector[0]);
		// strip quotes, if they are present
		{
			const size_t cmd_line_length(_tcslen(cmd_line));
			if (cmd_line[0] == _T('"') && cmd_line[cmd_line_length - 1] == _T('"'))
			{
				_tcscpy(path, cmd_line + 1);
				path[cmd_line_length - 2] = _T('\0');
			}
			else
				_tcscpy(path, cmd_line);
		}
		if (PathFileExists(path))
		{
			if (0 == _tcscmp(_T(".pmproj"), PathFindExtension(path)))
				main_wnd_.OpenProject(path);
		}
	}
	return true;
}

int App::Run()
{
	MSG msg;
	HACCEL hAccelTable(LoadAccelerators(instance_, (LPCTSTR)IDC_COMPILER));
	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	Destroy();
	return static_cast<int>(msg.wParam);
}

void App::Destroy()
{
	// save application data
	MacroAppData(ID_INFO_WND_VISIBLE)    = info_wnd_   .IsVisible();
	MacroAppData(ID_PREVIEW_WND_VISIBLE) = preview_wnd_.IsVisible();
	MacroAppData(ID_STAT_WND_VISIBLE)    = stat_wnd_   .IsVisible();
	if (info_wnd_.IsRectValid())
		MacroAppData(ID_INFO_WND_RECT) = info_wnd_.GetRect();
	if (preview_wnd_.IsRectValid())
		MacroAppData(ID_PREVIEW_WND_RECT) = preview_wnd_.GetRect();
	if (stat_wnd_.IsRectValid())
		MacroAppData(ID_STAT_WND_RECT) = stat_wnd_.GetRect();
	if (main_wnd_.IsRectValid())
		MacroAppData(ID_MAIN_WND_RECT) = main_wnd_.GetRect();
	SSAppData::Save(MakeIniFileName().c_str());
	FreeImage_DeInitialise();
	jas_image_clearfmts();
}

tstring App::MakeIniFileName()
{
	// the ini has the same name as the executable,
	//  but with an "ini" extension
	TCHAR ini_path[MAX_PATH];
	GetModuleFileName(NULL, ini_path, MAX_PATH);
	PathRemoveExtension(ini_path);
	PathAddExtension(ini_path, ".ini");
	return ini_path;
}
