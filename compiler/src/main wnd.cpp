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

#include "about wnd.h"
#include "main wnd.h"
#include "map manager.h"
#include "project create wnd.h"
#include "project settings wnd.h"
#include "resource.h"
#include "version detector.h"

#include <map>
#include <shellapi.h>
#include <shlobj.h>
#include <sstream>
#include <windowsx.h>

//-----------------------
// MainWnd implementation
//-----------------------

namespace MainWndMetrics
{
	// |--------------------------------|
	// |             frame              |
	// |--------------------------------|
	// | pre | btn | intro | btn | post |
	// |--------------------------------|
	// |             frame              |
	// |--------------------------------|
	const SIZE state = { 32, 32 };             // state indicator size
	const SIZE busy  = { 32, 32 };             // "busy" indicator size
	const SIZE btn   = { 36, 36 };             // button size
	const LONG frame(4);                       // empty space around the edges
	const LONG pre  (frame + state.cx + 16);   // space before the button area
	const LONG intro(4);                       // space between buttons
	const LONG post (16 + busy.cx + frame);    // space after the button area
	const SIZE client = {
		pre + btn.cx * MainWnd::panel_count + intro * (MainWnd::panel_count - 1) + post,
		frame + __max(state.cy, __max(busy.cy, btn.cy) + frame)
	};                                         // size of the whole client area
	const DWORD btn_style   (WS_CHILD | BS_AUTOCHECKBOX | BS_PUSHLIKE | BS_CENTER | BS_BITMAP);
	const DWORD window_style(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
};

MainWnd::MainWnd(
	ProjectManager &project_manager,
	InfoWnd        &info_wnd,
	PreviewWnd     &preview_wnd,
	StatWnd        &stat_wnd)
	:info_wnd_       (info_wnd)
	,preference_wnd_ (preview_wnd, project_manager)
	,preview_wnd_    (preview_wnd)
	,project_manager_(project_manager)
	,stat_wnd_       (stat_wnd)
	,tasks_left_     (hwnd_)
{}

void MainWnd::AddPanelWnds(MainWnd::PanelInfo (&panels)[MainWnd::panel_count])
{
#ifdef _DEBUG
	{
		// make sure the function is not called more than once
		static bool dbg_first_call(true);
		_ASSERTE(dbg_first_call);
		dbg_first_call = false;
	}
#endif
	// move the "busy" indicators
	MoveWindow(
		busy_on_icon_,
		MainWndMetrics::frame + MainWndMetrics::pre + MainWndMetrics::btn.cx * panel_count + MainWndMetrics::post - 32,
		MainWndMetrics::frame,
		32,
		32,
		FALSE);
	MoveWindow(
		busy_off_icon_,
		MainWndMetrics::frame + MainWndMetrics::pre + MainWndMetrics::btn.cx * panel_count + MainWndMetrics::post - 32,
		MainWndMetrics::frame,
		32,
		32,
		FALSE);
	// add panels
	RECT button_rect;
	POINT position = { MainWndMetrics::pre, MainWndMetrics::frame };
	for (WORD i(0); i != panel_count; ++i)
	{
		button_rect.left   = position.x;
		button_rect.top    = position.y;
		button_rect.right  = button_rect.left + MainWndMetrics::btn.cx;
		button_rect.bottom = button_rect.top  + MainWndMetrics::btn.cy;
		AddPanelWnd(
			panels[i].panel_,
			panels[i].image_id_,
			i,
			button_rect,
			panels[i].creation_rect_,
			panels[i].tip_);
		position.x += MainWndMetrics::btn.cx + MainWndMetrics::intro;
	}
}

bool MainWnd::Create(POINT position)
{
#ifdef _DEBUG
	{
		// make sure the function is not called more than once
		static bool dbg_first_call(true);
		_ASSERTE(dbg_first_call);
		dbg_first_call = false;
	}
#endif
	// get instance handle
	HINSTANCE hinstance(GetModuleHandle(NULL));
	// calculate the window rectangle based on the client area size
	RECT window_rect;
	{
		// set initial rectangle size
		window_rect.left   = 0;
		window_rect.top    = 0;
		window_rect.right  = MainWndMetrics::client.cx;
		window_rect.bottom = MainWndMetrics::client.cy;
		// adjust the rectangle appropriately
		AdjustWindowRect(&window_rect, MainWndMetrics::window_style, TRUE);
		window_rect.right  = position.x + (window_rect.right - window_rect.left);
		window_rect.left   = position.x;
		window_rect.bottom = position.y + (window_rect.bottom - window_rect.top);
		window_rect.top    = position.y;
	}
	// register the window class
	WNDCLASSEX window_class;
	{
		ZeroMemory(&window_class, sizeof(window_class));
		window_class.cbSize        = sizeof(window_class);
		window_class.style         = CS_HREDRAW | CS_VREDRAW;
		window_class.lpfnWndProc   = WndProc<MainWnd>;
		window_class.cbClsExtra    = 0;
		window_class.cbWndExtra    = 0;
		window_class.hInstance     = hinstance;
		window_class.hIcon         = LoadIcon(hinstance, (LPCTSTR)IDI_COMPILER);
		window_class.hCursor       = LoadCursor(NULL, IDC_ARROW);
		window_class.hbrBackground = (HBRUSH)(COLOR_BTNSHADOW);
		window_class.lpszMenuName  = (LPCTSTR)IDC_COMPILER;
		window_class.lpszClassName = _T("PmcMainWnd");
		window_class.hIconSm       = LoadIcon(hinstance, (LPCTSTR)IDI_COMPILER);
		if (0 == RegisterClassEx(&window_class))
		{
			MacroDisplayError(_T("The main window could not be registered."));
			return false;
		}
	}
	// create the main window
	CreateWindow(
		window_class.lpszClassName,
		_T("Perimeter Map Compiler"),
		MainWndMetrics::window_style,
		window_rect.left,
		window_rect.top,
		window_rect.right - window_rect.left,
		window_rect.bottom - window_rect.top,
		NULL,
		NULL,
		hinstance,
		this);
	if (NULL == hwnd_)
	{
		MacroDisplayError(_T("The main window could not be created."));
		return false;
	}
	// create the tooltip control
	tool_tip_ = CreateWindowEx(
		WS_EX_TOPMOST,
		TOOLTIPS_CLASS,
		NULL,
		WS_POPUP | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		hwnd_,
		NULL,
		hinstance,
		NULL);
	// create state_shrub_icon_
	state_shrub_icon_ = CreateWindow(
		_T("STATIC"),
		NULL,
		WS_CHILD | SS_ICON,
		MainWndMetrics::frame,
		MainWndMetrics::frame,
		MainWndMetrics::state.cx,
		MainWndMetrics::state.cy,
		hwnd_,
		NULL,
		hinstance,
		NULL);
	if (NULL == state_shrub_icon_)
	{
		MacroDisplayError(_T("state_shrub_icon_ could not be created."));
		return false;
	}
	Static_SetIcon(state_shrub_icon_, static_cast<HICON>(LoadImage(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDI_SHRUB_ACTIVE),
		IMAGE_ICON,
		MainWndMetrics::state.cx,
		MainWndMetrics::state.cy,
		LR_DEFAULTCOLOR)));
	// create state_project_icon_
	state_project_icon_ = CreateWindow(
		_T("STATIC"),
		NULL,
		WS_CHILD | SS_ICON | WS_VISIBLE,
		MainWndMetrics::frame,
		MainWndMetrics::frame,
		MainWndMetrics::state.cx,
		MainWndMetrics::state.cy,
		hwnd_,
		NULL,
		hinstance,
		NULL);
	if (NULL == state_project_icon_)
	{
		MacroDisplayError(_T("state_project_icon_ could not be created."));
		return false;
	}
	Static_SetIcon(state_project_icon_, static_cast<HICON>(LoadImage(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDI_SHRUB_INACTIVE),
		IMAGE_ICON,
		MainWndMetrics::state.cx,
		MainWndMetrics::state.cy,
		LR_DEFAULTCOLOR)));
	// create the "busy" indicators
	busy_on_icon_ = CreateWindow(
		_T("STATIC"),
		NULL,
		WS_CHILD | WS_VISIBLE | SS_ICON | SS_NOTIFY,
		MainWndMetrics::frame + MainWndMetrics::pre + MainWndMetrics::post - 32,
		MainWndMetrics::frame,
		MainWndMetrics::state.cx,
		MainWndMetrics::state.cy,
		hwnd_,
		NULL,
		hinstance,
		NULL);
	if (NULL == state_shrub_icon_)
	{
		MacroDisplayError(_T("state_shrub_icon_ could not be created."));
		return false;
	}
	AddToolTip(busy_on_icon_, _T("busy"));
	Static_SetIcon(busy_on_icon_, static_cast<HICON>(LoadImage(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDI_RED_LIGHT),
		IMAGE_ICON,
		MainWndMetrics::state.cx,
		MainWndMetrics::state.cy,
		LR_DEFAULTCOLOR)));
	busy_off_icon_ = CreateWindow(
		_T("STATIC"),
		NULL,
		WS_CHILD | SS_ICON | WS_VISIBLE,
		MainWndMetrics::frame + MainWndMetrics::pre + MainWndMetrics::post - 32,
		MainWndMetrics::frame,
		MainWndMetrics::state.cx,
		MainWndMetrics::state.cy,
		hwnd_,
		NULL,
		hinstance,
		NULL);
	if (NULL == state_shrub_icon_)
	{
		MacroDisplayError(_T("state_shrub_icon_ could not be created."));
		return false;
	}
	EnableWindow(busy_off_icon_, TRUE);
	Static_SetIcon(busy_off_icon_, static_cast<HICON>(LoadImage(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDI_GREEN_LIGHT),
		IMAGE_ICON,
		MainWndMetrics::state.cx,
		MainWndMetrics::state.cy,
		LR_DEFAULTCOLOR)));
	project_manager_.tasks_left_ = &tasks_left_;
	SetMenuState(MS_EMPTY);
	return true;
}

void MainWnd::OpenProject(LPCTSTR path)
{
	project_manager_.OpenProject(path, hwnd_);
	SetMenuState(MS_PROJECT);
	ToggleStateIcon(MS_PROJECT);
}

void MainWnd::ShowPanel(int panel_id, bool show)
{
	ShowPanel(panels_[panel_id], show);
}

void MainWnd::ShowPanel(PanelData &panel, bool show)
{
	// delayed creation
	if (show && !panel.panel_->hwnd_)
	{
		panel.created_ = panel.panel_->Create(hwnd_, panel.creation_rect_);
		// disable button if creation fails
		if (!panel.created_)
			EnableWindow(panel.button_hwnd_, FALSE);
	}
	if (panel.created_)
	{
		// show the window
		ShowWindow(panel.panel_->hwnd_, show ? SW_SHOW : SW_HIDE);
		SetFocus(hwnd_);
	}
}

void MainWnd::UnpackShrub(LPCTSTR path)
{
	if (project_manager_.UnpackShrub(path, hwnd_))
	{
		SetMenuState(MS_SHRUB);
		ToggleStateIcon(MS_SHRUB);
	}
}

void MainWnd::OnEnabled(Msg<WM_ENABLE> &msg)
{
	BOOL is_enabled(msg.IsEnabled() ? TRUE : FALSE);
	if (0 != info_wnd_.hwnd_)       EnableWindow(info_wnd_.hwnd_,       is_enabled);
	if (0 != preference_wnd_.hwnd_) EnableWindow(preference_wnd_.hwnd_, is_enabled);
	if (0 != preview_wnd_.hwnd_)    EnableWindow(preview_wnd_.hwnd_,    is_enabled);
	if (0 != stat_wnd_.hwnd_)       EnableWindow(stat_wnd_.hwnd_,       is_enabled);
}

void MainWnd::OnProjectOpen(Msg<WM_USR_PROJECT_OPEN> &msg)
{
	project_manager_.OnProjectOpen(hwnd_);
	msg.handled_ = true;
}

void MainWnd::OnProjectUnpacked(Msg<WM_USR_PROJECT_UNPACKED> &msg)
{
	project_manager_.OnProjectUnpacked(hwnd_);
	msg.handled_ = true;
}

void MainWnd::OnSysColorChange(Msg<WM_SYSCOLORCHANGE> &msg)
{
	for (size_t i(0); i != panel_count; ++i)
	{
		PanelData &panel(panels_[i]);
		if (!panel.created_)
			continue;
		HBITMAP image(CreateButtonImage(panel.image_id_));
		SendMessage(panel.button_hwnd_, BM_SETIMAGE, IMAGE_BITMAP, ri_cast<LPARAM>(image));
		DeleteObject(panel.image_);
		panel.image_ = image;
	}
}

void MainWnd::OnToggleBusy(Msg<WM_USR_TOGGLE_BUSY> &msg)
{
	if (0 == msg.TaskCount())
	{
		ToggleWaitCursor(false);
		ToggleBusyIcon(false, NULL);
	}
	else
	{
		typedef std::basic_stringstream<TCHAR> tstringstream;
		tstringstream message;
		message << msg.TaskCount() << _T(" task");
		if (msg.TaskCount() > 1)
			message << _T("s");
		message << _T(" left");
		ToggleWaitCursor(true);
		ToggleBusyIcon(true, message.str().c_str());
	}
	msg.handled_ = true;
}

void MainWnd::ToggleBusyIcon(bool busy, LPCTSTR message)
{
	if (busy)
	{
		ShowWindowAsync(busy_on_icon_,  SW_SHOW);
		ShowWindowAsync(busy_off_icon_, SW_HIDE);
		ChangeToolTipText(busy_on_icon_, message);
	}
	else
	{
		ShowWindowAsync(busy_off_icon_, SW_SHOW);
		ShowWindowAsync(busy_on_icon_,  SW_HIDE);
	}
}

void MainWnd::ToggleStateIcon(MainWnd::MenuState state)
{
	if (MS_SHRUB == state)
	{
		ShowWindowAsync(state_shrub_icon_,  SW_SHOW);
		ShowWindowAsync(state_project_icon_, SW_HIDE);
	}
	else
	{
		ShowWindowAsync(state_project_icon_,  SW_SHOW);
		ShowWindowAsync(state_shrub_icon_, SW_HIDE);
	}
}

void MainWnd::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		&MainWnd::OnCommand,
		&MainWnd::OnCreate,
		&MainWnd::OnDestroy,
		&MainWnd::OnEnabled,
		&MainWnd::OnProjectOpen,
		&MainWnd::OnProjectUnpacked,
		&MainWnd::OnResourceCreated,
		&MainWnd::OnSysColorChange,
		&MainWnd::OnToggleBusy
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}

void MainWnd::OnResourceCreated(Msg<WM_USR_RESOURCE_CREATED> &msg)
{
	project_manager_.OnResourceCreated(msg.Id());
}

void MainWnd::OnCommand(Msg<WM_COMMAND> &msg)
{
	switch (msg.CtrlId())
	{
	case IDM_ABOUT:               OnAbout          (msg); break;
	case IDM_EXIT:                OnExit           (msg); break;
	case ID_FILE_INSTALLSHRUB:    OnInstallShrub   (msg); break;
	case ID_FILE_MANAGEMAPS:      OnManageMaps     (msg); break;
	case ID_FILE_NEWPROJECT:      OnNewProject     (msg); break;
	case ID_FILE_OPENPROJECT:     OnOpenProject    (msg); break;
	case ID_FILE_PACKSHRUB:       OnPackShrub      (msg); break;
	case ID_FILE_PROJECTSETTINGS: OnProjectSettings(msg); break;
	case ID_FILE_UNPACKSHRUB:     OnUpackShrub     (msg); break;
	case ID_HELP_EN:              OnHelpEn         (msg); break;
	case ID_HELP_RU:              OnHelpRu         (msg); break;
	case ID_TOOLS_IMPORTSCRIPT:   OnImportScript   (msg); break;
	case ID_TOOLS_PREFERENCES:    OnPreferences    (msg); break;
	case ID_TOOLS_SAVETHUMBNAIL:  OnSaveThumbnail  (msg); break;
	default:
		// check for panel button messages
		for(PanelData *panel(panels_); panel != panels_ + panel_count; ++panel)
		{
			if (msg.CtrlId() == panel->panel_id_)
			{
				DWORD check(Button_GetCheck(panel->button_hwnd_));
				ShowPanel(*panel, check == BST_CHECKED);
			}
		}
	}
}

void MainWnd::OnCreate(Msg<WM_CREATE> &msg)
{
	msg.result_ = TRUE;
	msg.handled_ = true;
}

void MainWnd::OnDestroy(Msg<WM_DESTROY> &msg)
{
	// destroy buttons
	for(PanelData *i(panels_); i != panels_ + panel_count; ++i)
	{
		if (NULL != i->button_hwnd_)
			DestroyWindow(i->button_hwnd_);
		if (NULL != i->image_)
			DeleteObject(i->image_);
	}
	// destroy state indicator windows
	if (NULL != state_project_icon_)
		DestroyWindow(state_project_icon_);
	if (NULL != state_shrub_icon_)
		DestroyWindow(state_shrub_icon_);
	// quit the main thread
	PostQuitMessage(0);
	msg.result_  = FALSE;
	msg.handled_ = true;
}

void MainWnd::OnAbout(Msg<WM_COMMAND> &msg)
{
	About about;
	about.DoModal(hwnd_);
}

void MainWnd::OnExit(Msg<WM_COMMAND> &msg)
{
	DestroyWindow(hwnd_);
}

void MainWnd::OnHelpEn(Msg<WM_COMMAND> &msg)
{
	ShellExecute(hwnd_, _T("open"), _T("help (english).chm"), NULL, NULL, SW_MAXIMIZE);
}

void MainWnd::OnHelpRu(Msg<WM_COMMAND> &msg)
{
	ShellExecute(hwnd_, _T("open"), _T("help (russian).chm"), NULL, NULL, SW_MAXIMIZE);
}

void MainWnd::OnImportScript(Msg<WM_COMMAND> &msg)
{
	tstring script_path(GetFilePathDlg(
		_T("Import Script"),
		_T("Perimeter Script (*.spg)\0*.spg\0All Files\0*\0")));
	if (!script_path.empty())
		project_manager_.ImportScript(script_path.c_str(), hwnd_);
}

void MainWnd::OnInstallShrub(Msg<WM_COMMAND> &msg)
{
	VersionDetector detector;
	if (!detector.ShowSelectDialog(hwnd_))
		return;
	project_manager_.InstallMap(detector.GetPath().c_str(), detector.GetVersion());
}

void MainWnd::OnManageMaps(Msg<WM_COMMAND> &msg)
{
	VersionDetector detector;
	if (!detector.ShowSelectDialog(hwnd_))
		return;
	MapManager map_manager;
	map_manager.DoModal(hwnd_, detector.GetPath().c_str());
}

void MainWnd::OnNewProject(Msg<WM_COMMAND> &msg)
{
	msg.result_  = TRUE;
	msg.handled_ = true;
	// get project information
	CreateProjectDlg create_project_dlg;
	if (IDOK != create_project_dlg.DoModal(hwnd_))
		return;
	// get the project folder path
	tstring folder_path(GetFolderPathDlg(_T("Choose a folder for the new project's files.")));
	if (folder_path.empty())
		return;
	// create the project
	project_manager_.CreateProject(
		folder_path.c_str(),
		create_project_dlg.map_name_.c_str(),
		create_project_dlg.map_size_,
		hwnd_);
	SetMenuState(MS_PROJECT);
	ToggleStateIcon(MS_PROJECT);
}

void MainWnd::OnOpenProject(Msg<WM_COMMAND> &msg)
{
	msg.result_  = TRUE;
	tstring pmproj_path(GetFilePathDlg(_T("Open Project"), _T("Map Project Files (*.pmproj)\0*.pmproj\0")));
	if (pmproj_path.empty())
		return;
	OpenProject(pmproj_path.c_str());
}

void MainWnd::OnPackShrub(Msg<WM_COMMAND> &msg)
{
	project_manager_.PackShrub();
}

void MainWnd::OnPreferences(Msg<WM_COMMAND> &msg)
{
	if (NULL == preference_wnd_.hwnd_)
		preference_wnd_.Create(hwnd_);
	else
		SetForegroundWindow(preference_wnd_.hwnd_);
}

void MainWnd::OnProjectSettings(Msg<WM_COMMAND> &msg)
{
	ProjectSettingsWnd project_settings_wnd(project_manager_, *this);
	project_settings_wnd.DoModal(hwnd_);
}

void MainWnd::OnSaveThumbnail(Msg<WM_COMMAND> &msg)
{
	project_manager_.SaveThumbnail();
}

void MainWnd::OnUpackShrub(Msg<WM_COMMAND> &msg)
{
	msg.result_  = TRUE;
	tstring shrub_path(GetFilePathDlg(_T("Unpack Shrub"), _T("Shrub Files (*.shrub)\0*.shrub\0")));
	if (shrub_path.empty())
		return;
	UnpackShrub(shrub_path.c_str());
}

VOID CALLBACK MainWnd::ToolTipCleanupCallback(HWND hwnd, UINT msg_id, DWORD data, LRESULT result)
{
	delete [] ri_cast<TCHAR*>(data);
}

bool MainWnd::AddPanelWnd(
	PanelWindow *panel,
	WORD         image_id,
	WORD         panel_index,
	RECT        &button_rect,
	RECT        &creation_rect,
	LPCTSTR      tip)
{
	WORD panel_id = ID_PANEL_WND_0 + panel_index;
	// create the corresponding button
	HWND button(CreateWindow(
		_T("BUTTON"),
		_T(""),
		WS_VISIBLE | MainWndMetrics::btn_style,
		button_rect.left,
		button_rect.top,
		button_rect.right - button_rect.left,
		button_rect.bottom - button_rect.top,
		hwnd_,
		ri_cast<HMENU>(panel_id),
		GetModuleHandle(NULL),
		this));
	if (NULL == button)
	{
		MacroDisplayError(_T("panel button could not be created"));
		return false;
	}
	RECT client;
	GetClientRect(button, &client);
	// set the button's icon
	HBITMAP image(CreateButtonImage(image_id));
	SendMessage(button, BM_SETIMAGE, IMAGE_BITMAP, ri_cast<LPARAM>(image));
	AddToolTip(button, tip);
	// add the PanelData structure
	{
		panels_[panel_index].button_hwnd_   = button;
		panels_[panel_index].creation_rect_ = creation_rect;
		panels_[panel_index].image_         = image;
		panels_[panel_index].image_id_      = image_id;
		panels_[panel_index].panel_         = panel;
		panels_[panel_index].panel_id_      = panel_id;
	}
	// subscribe to the panel visibility events
	panel->on_show_ += PanelWindow::on_show_t::delegate_t(&panels_[panel_index], &PanelData::on);
	panel->on_hide_ += PanelWindow::on_hide_t::delegate_t(&panels_[panel_index], &PanelData::off);
	// wrap up
	++panel_id;
	return true;
}

void MainWnd::AddToolTip(HWND hwnd, LPCTSTR text)
{
	TOOLINFO info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize   = sizeof(info);
	info.uFlags   = TTF_IDISHWND | TTF_SUBCLASS;
	info.uId      = ri_cast<UINT_PTR>(hwnd);
	info.hwnd     = hwnd_;
	info.hinst    = GetModuleHandle(NULL);
	info.lpszText = new TCHAR[_tcslen(text) + 1]; // deleted in the callback
	_tcscpy(info.lpszText, text);
	SendMessageCallback(
		tool_tip_,
		TTM_ADDTOOL,
		0,
		ri_cast<LPARAM>(&info),
		ToolTipCleanupCallback,
		ri_cast<ULONG_PTR>(info.lpszText));
}

void MainWnd::ChangeToolTipText(HWND hwnd, LPCTSTR text)
{
	TOOLINFO info;
	ZeroMemory(&info, sizeof(info));
	info.cbSize   = sizeof(info);
	info.uFlags   = TTF_IDISHWND | TTF_SUBCLASS;
	info.uId      = ri_cast<UINT_PTR>(hwnd);
	info.hwnd     = GetParent(hwnd);
	info.hinst    = GetModuleHandle(NULL);
	info.lpszText = new TCHAR[_tcslen(text) + 1]; // deleted in the callback
	_tcscpy(info.lpszText, text);
	SendMessageCallback(
		tool_tip_,
		TTM_UPDATETIPTEXT,
		0,
		ri_cast<LPARAM>(&info),
		ToolTipCleanupCallback,
		ri_cast<ULONG_PTR>(info.lpszText));
}

HBITMAP MainWnd::CreateButtonImage(WORD image_id)
{
	// initialize
	COLORREF back_clr(GetSysColor(COLOR_3DFACE));
	COLORREF fore_clr(GetSysColor(COLOR_BTNTEXT));
	HINSTANCE hinst(GetModuleHandle(NULL));
	HBITMAP bmp(LoadBitmap(hinst, MAKEINTRESOURCE(image_id)));
	HDC dc(CreateCompatibleDC(NULL));
	HGDIOBJ old_bitmap(SelectObject(dc, bmp));
	// check for errors
	if (NULL == bmp)
	{
		MacroDisplayError("Bitmap resource could not be loaded.");
		return NULL;
	}
	if (NULL == dc)
	{
		MacroDisplayError("Compatible DC could not be created.");
		return NULL;
	}
	if (NULL == old_bitmap)
	{
		MacroDisplayError("Bitmap could not be drawn on.");
		return NULL;
	}
	// colorize the bitmap using the appropriate system colors
	// this is not a very effective way of accessing a bitmap
	//  and should be modified shall performance ever matter
	for (int x(0); x != 32; ++x)
		for (int y(0); y != 32; ++y)
		{
			COLORREF clr(GetPixel(dc, x, y));
			float val(static_cast<BYTE>(clr) / 255.0f);
			BYTE r(static_cast<BYTE>(GetRValue(back_clr) * val + GetRValue(fore_clr) * (1 - val)));
			BYTE g(static_cast<BYTE>(GetGValue(back_clr) * val + GetGValue(fore_clr) * (1 - val)));
			BYTE b(static_cast<BYTE>(GetBValue(back_clr) * val + GetBValue(fore_clr) * (1 - val)));
			SetPixelV(dc, x, y, RGB(r, g, b));
		}
	// wrap up
	SelectObject(dc, old_bitmap);
	DeleteDC(dc);
	return bmp;
}

tstring MainWnd::GetFilePathDlg(LPCTSTR title, LPCTSTR filter)
{
	TCHAR strFileName[MAX_PATH] = { 0 };
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner   = hwnd_;
	ofn.lpstrFile   = strFileName;
	ofn.nMaxFile    = sizeof(strFileName);
	ofn.lpstrTitle  = title;
	ofn.lpstrFilter = filter;
	ofn.Flags =
		OFN_LONGNAMES     |
		OFN_FILEMUSTEXIST |
		OFN_READONLY      |
		OFN_NOCHANGEDIR   |
		OFN_HIDEREADONLY;
	if (false != GetOpenFileName(&ofn))
		return strFileName;
	return "";
}

tstring MainWnd::GetFolderPathDlg(LPCTSTR title)
{
	TCHAR strFolderName[MAX_PATH] = { 0 };
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(bi));
	bi.hwndOwner     = hwnd_;
	bi.pszDisplayName = strFolderName;
	bi.lpszTitle      = title;
	bi.ulFlags        = BIF_USENEWUI | BIF_VALIDATE;
	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	//  get path
	if (NULL != pidl && SHGetPathFromIDList(pidl, strFolderName))
		return strFolderName;
	return "";
}

void MainWnd::SetMenuState(MainWnd::MenuState state)
{
	// minimum
	typedef std::map<uint, bool> ItemsType;
	ItemsType items;
	items[IDM_ABOUT]               = true;
	items[IDM_EXIT]                = true;
	items[ID_FILE_INSTALLSHRUB]    = false;
	items[ID_FILE_MANAGEMAPS]      = true;
	items[ID_FILE_NEWPROJECT]      = true;
	items[ID_FILE_OPENPROJECT]     = true;
	items[ID_FILE_PACKSHRUB]       = false;
	items[ID_FILE_PROJECTSETTINGS] = false;
	items[ID_FILE_UNPACKSHRUB]     = true;
	items[ID_TOOLS_IMPORTSCRIPT]   = false;
	items[ID_TOOLS_PREFERENCES]    = true;
	items[ID_TOOLS_SAVETHUMBNAIL]  = false;
	// optionally enabled items
	vector<uint> on_items;
	switch (state)
	{
	case MS_EMPTY:
		break;
	case MS_PROJECT:
		on_items.push_back(ID_FILE_INSTALLSHRUB);
		on_items.push_back(ID_FILE_PACKSHRUB);
		on_items.push_back(ID_FILE_PROJECTSETTINGS);
		on_items.push_back(ID_TOOLS_SAVETHUMBNAIL);
		on_items.push_back(ID_TOOLS_IMPORTSCRIPT);
		break;
	case MS_SHRUB:
		on_items.push_back(ID_FILE_INSTALLSHRUB);
		on_items.push_back(ID_TOOLS_SAVETHUMBNAIL);
		break;
	}
	// add the optional items
	foreach (uint &on_item, on_items)
		items[on_item] = true;
	// toggle menus
	HMENU menu(GetMenu(hwnd_));
	foreach (ItemsType::value_type &item, items)
		EnableMenuItem(menu, item.first, item.second ? MF_ENABLED : MF_GRAYED);
}

//----------------------------------
// MainWnd::PanelData implementation
//----------------------------------

MainWnd::PanelData::PanelData()
	:button_hwnd_(NULL)
	,image_      (NULL)
	,created_    (false)
{}

void MainWnd::PanelData::on()
{
	Button_SetCheck(button_hwnd_, BST_CHECKED);
}

void MainWnd::PanelData::off()
{
	Button_SetCheck(button_hwnd_, BST_UNCHECKED);
}

//----------------------------------
// MainWnd::TasksLeft implementation
//----------------------------------

MainWnd::TasksLeft::TasksLeft(HWND &hwnd)
	:hwnd_(hwnd)
{}

void MainWnd::TasksLeft::operator() (uint task_count)
{
	PostToggleBusy(hwnd_, task_count);
}
