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


#include "StdAfx.h"

#include "main wnd.h"
#include "project data.h"
#include "project manager.h"
#include "project settings wnd.h"
#include "project tasks.h"
#include "resource.h"

//----------------------------------
// ProjectSettingsWnd implementation
//----------------------------------

ProjectSettingsWnd::ProjectSettingsWnd(ProjectManager &project_manager, MainWnd &main_wnd)
	:main_wnd_       (main_wnd)
	,project_manager_(project_manager)
{}

UINT_PTR ProjectSettingsWnd::DoModal(HWND parent_hwnd)
{
	return DialogBoxParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_PROJECT_SETTINGS),
		parent_hwnd,
		DlgProc<ProjectSettingsWnd>,
		ri_cast<LPARAM>(this));
}

void ProjectSettingsWnd::OnCommand (Msg<WM_COMMAND> &msg)
{
	switch (msg.CtrlId())
	{
	case IDOK:
		Apply();
	case IDCANCEL:
		EndDialog(hwnd_, 0);
		return;
	}
	msg.result_  = FALSE;
	msg.handled_ = true;
}

void ProjectSettingsWnd::OnInitDialog(Msg<WM_INITDIALOG> &msg)
{
	SetDlgItemText(hwnd_, IDC_MAP_NAME, MacroProjectData(ID_MAP_NAME).c_str());
	CheckDlgButton(hwnd_, IDC_CUSTOM_HARDNESS,   MacroProjectData(ID_CUSTOM_HARDNESS)   ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd_, IDC_CUSTOM_ZERO_LAYER, MacroProjectData(ID_CUSTOM_ZERO_LAYER) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd_, IDC_CUSTOM_SURFACE,    MacroProjectData(ID_CUSTOM_SURFACE)    ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd_, IDC_CUSTOM_SKY,        MacroProjectData(ID_CUSTOM_SKY)        ? BST_CHECKED : BST_UNCHECKED);
	msg.result_  = TRUE;
	msg.handled_ = true;
}

void ProjectSettingsWnd::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		&ProjectSettingsWnd::OnCommand,
		&ProjectSettingsWnd::OnInitDialog
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}

void ProjectSettingsWnd::Apply()
{
	// map name
	{
		const size_t map_name_length(20);
		TCHAR map_name[map_name_length];
		GetDlgItemText(hwnd_, IDC_MAP_NAME, map_name, map_name_length);
		SetDlgItemText(hwnd_, IDC_MAP_NAME, map_name);
		if (MacroProjectData(ID_MAP_NAME) != map_name)
		{
			MacroProjectData(ID_MAP_NAME) = map_name;
			project_manager_.UpdateSettings();
			SetWindowText(main_wnd_.hwnd_, map_name);
		}
	}
	// custom resources
	MacroProjectData(ID_CUSTOM_HARDNESS)   = (BST_CHECKED == IsDlgButtonChecked(hwnd_, IDC_CUSTOM_HARDNESS));
	MacroProjectData(ID_CUSTOM_ZERO_LAYER) = (BST_CHECKED == IsDlgButtonChecked(hwnd_, IDC_CUSTOM_ZERO_LAYER));
	MacroProjectData(ID_CUSTOM_SURFACE)    = (BST_CHECKED == IsDlgButtonChecked(hwnd_, IDC_CUSTOM_SURFACE));
	MacroProjectData(ID_CUSTOM_SKY)        = (BST_CHECKED == IsDlgButtonChecked(hwnd_, IDC_CUSTOM_SKY));
	if (MacroProjectData(ID_CUSTOM_HARDNESS))
		project_manager_.CreateResouce(RS_HARDNESS);
	if (MacroProjectData(ID_CUSTOM_ZERO_LAYER))
		project_manager_.CreateResouce(RS_ZERO_LAYER);
	if (MacroProjectData(ID_CUSTOM_SURFACE))
		project_manager_.CreateResouce(RS_SURFACE);
	if (MacroProjectData(ID_CUSTOM_SKY))
		project_manager_.CreateResouce(RS_SKY);
}
