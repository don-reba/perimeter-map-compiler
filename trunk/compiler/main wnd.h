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

#include "panel wnd.h"
#include "pmc wnd.h"
#include "preference wnd.h"
#include "project manager.h"

class InfoWnd;
class PreviewWnd;
class StatWnd;

//---------------------
// some custom messages
//---------------------

const uint WM_USR_TOGGLE_BUSY (WM_APP + 32);

inline void PostToggleBusy(HWND hwnd, uint task_count) {
	PostMessage(hwnd, WM_USR_TOGGLE_BUSY, task_count, 0L);
}

template <>
struct Msg<WM_USR_TOGGLE_BUSY> : Msg_
{
	Msg(WndMsg &msg) : Msg_(msg) {}
	uint TaskCount() const {
		return wprm_;
	}
};

//-----------------------------------
// main application window definition
//-----------------------------------

class MainWnd : public PMCWindow
{
// constants
public:
	static const UINT panel_count = 3;
// types
public:
	struct PanelInfo
	{
		PanelInfo(PanelWindow *panel, WORD image_id, LPCTSTR tip)
			:panel_(panel), image_id_(image_id), tip_(tip) {}
		PanelWindow *panel_;
		WORD         image_id_;
		LPCTSTR      tip_;
	};
private:
	struct TasksLeft : ProjectManager::TasksLeft
	{
		TasksLeft(HWND &hwnd);
		virtual void operator() (uint task_count);
	private:
		HWND &hwnd_;
	};
	struct TogglePanelVisibility : PanelWindow::ToggleVisibility
	{
		TogglePanelVisibility(HWND button);
		~TogglePanelVisibility();
		virtual void operator() (bool on);
		HWND button_;
	};
	struct PanelData
	{
		PanelData() : button_hwnd_(NULL), image_(NULL) {}
		HWND         button_hwnd_;
		HBITMAP      image_;
		WORD         image_id_;
		PanelWindow *panel_;
		WORD         panel_id_;
		TogglePanelVisibility *toggle_panel_visibility_;
	};
	enum MenuState
	{
		MS_EMPTY,
		MS_PROJECT,
		MS_SHRUB
	};
// construction/destruction
public:
	MainWnd(
		ProjectManager &project_manager,
		InfoWnd        &info_wnd,
		PreviewWnd     &preview_wnd,
		StatWnd        &stat_wnd);
// interface
public:
	void AddPanelWnds(PanelInfo (&panels)[panel_count]);
	bool Create(POINT position);
// message handlers
private:
	// window
	void OnDestroy   (Msg<WM_DESTROY>         &msg);
	void OnCommand   (Msg<WM_COMMAND>         &msg);
	void OnCreate    (Msg<WM_CREATE>          &msg);
	void OnToggleBusy(Msg<WM_USR_TOGGLE_BUSY> &msg);
	// command
	void OnAbout        (Msg<WM_COMMAND> &msg);
	void OnExit         (Msg<WM_COMMAND> &msg);
	void OnInstallShrub (Msg<WM_COMMAND> &msg);
	void OnManageMaps   (Msg<WM_COMMAND> &msg);
	void OnNewProject   (Msg<WM_COMMAND> &msg);
	void OnOpenProject  (Msg<WM_COMMAND> &msg);
	void OnPackShrub    (Msg<WM_COMMAND> &msg);
	void OnPreferences  (Msg<WM_COMMAND> &msg);
	void OnSaveThumbnail(Msg<WM_COMMAND> &msg);
	void OnUpackShrub   (Msg<WM_COMMAND> &msg);
// internal function
protected:
	virtual void ProcessMessage(WndMsg &msg);
private:
	static VOID CALLBACK ToolTipCleanupCallback(HWND hwnd, UINT msg_id, DWORD data, LRESULT result);
private:
	bool    AddPanelWnd(PanelWindow *panel, WORD image_id, WORD panel_index, RECT &button_rect, LPCTSTR tip);
	void    AddToolTip(HWND hwnd, LPCTSTR text);
	void    ChangeToolTipText(HWND hwnd, LPCTSTR text);
	HBITMAP CreateButtonImage(WORD image_id);
	tstring GetFilePathDlg(LPCTSTR title, LPCTSTR filter);
	tstring GetFolderPathDlg(LPCTSTR title);
	void    SetMenuState(MenuState state);
	void    ToggleBusyIcon(bool busy, LPCTSTR message);
//data
private:
	HWND            busy_on_icon_;
	HWND            busy_off_icon_;
	InfoWnd        &info_wnd_;
	PanelData       panels_[panel_count];
	PreferenceWnd   preference_wnd_;
	ProjectManager &project_manager_;
	PreviewWnd     &preview_wnd_;
	StatWnd        &stat_wnd_;
	HWND            state_shrub_icon_;
	HWND            state_project_icon_;
	TasksLeft       tasks_left_;
	HWND            tool_tip_;
};