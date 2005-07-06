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

#include "info wnd.h"
#include "preview wnd.h"
#include "project data.h"
#include "resource.h"

#include <commctrl.h>

//----------------------------
// additional message crackers
//----------------------------

#define UD_SetRange(hwndCtl, nLower, nUpper) ((void)SNDMSG((hwndCtl), UDM_SETRANGE, 0L, MAKELPARAM((nLower), (nUpper))))
#define UD_SetPos(hwndCtl, nPos) ((void)SNDMSG((hwndCtl), UDM_SETPOS, 0L, MAKELPARAM((nPos), 0)))
#define UD_GetPos(hwndCtl, nPos) ((int)SNDMSG((hwndCtl), UDM_GETPOS, 0L, 0L))
#define Button_SetIcon(hwndCtl, icon_id) ((void)SNDMSG((hwndCtl), BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(icon_id), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR)))

//-----------------------
// InfoWnd implementation
//-----------------------

InfoWnd::InfoWnd(PreviewWnd &preview_wnd)
	:preview_wnd_(preview_wnd)
{}

bool InfoWnd::Create(HWND parent_wnd, const RECT &window_rect, bool enabled)
{
	// create the window
	hwnd_ = CreateDialogParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_INFO_DLG),
		parent_wnd,
		DlgProc<InfoWnd>,
		ri_cast<LPARAM>(this));
	if (NULL == hwnd_)
	{
		MacroDisplayError(_T("The preview window could not be created."));
		return false;
	}
	SetRect(window_rect);
	SetWindowPos(
		hwnd_,
		NULL,
		window_rect.left,
		window_rect.top,
		0,
		0,
		SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
	EnableWindow(hwnd_, enabled ? TRUE : FALSE);
	return true;
}

void InfoWnd::Update(bool read_only)
{
	if (read_only)
		EnableControls(false);
	// fog colour
	DeleteObject(fog_colour_);
	fog_colour_ = CreateSolidBrush(MacroProjectData(ID_FOG_COLOUR));
	// fog distance
	SetDlgItemInt(hwnd_, IDC_FOG_START, MacroProjectData(ID_FOG_START), FALSE);
	SetDlgItemInt(hwnd_, IDC_FOG_END,   MacroProjectData(ID_FOG_END),   FALSE);
	// zero plast
	UD_SetRange(GetDlgItem(hwnd_, IDC_ZERO_PLAST_SPIN), 255, 0);
	UD_SetPos(GetDlgItem(hwnd_, IDC_ZERO_PLAST_SPIN), MacroProjectData(ID_ZERO_LEVEL));
	// positions
	UD_SetRange(GetDlgItem(hwnd_, IDC_SP1_X_SPIN), exp2(MacroProjectData(ID_POWER_X)) - 1, 0);
	UD_SetRange(GetDlgItem(hwnd_, IDC_SP1_Y_SPIN), exp2(MacroProjectData(ID_POWER_Y)) - 1, 0);
	UD_SetRange(GetDlgItem(hwnd_, IDC_SP2_X_SPIN), exp2(MacroProjectData(ID_POWER_X)) - 1, 0);
	UD_SetRange(GetDlgItem(hwnd_, IDC_SP2_Y_SPIN), exp2(MacroProjectData(ID_POWER_Y)) - 1, 0);
	UD_SetRange(GetDlgItem(hwnd_, IDC_SP3_X_SPIN), exp2(MacroProjectData(ID_POWER_X)) - 1, 0);
	UD_SetRange(GetDlgItem(hwnd_, IDC_SP3_Y_SPIN), exp2(MacroProjectData(ID_POWER_Y)) - 1, 0);
	UD_SetRange(GetDlgItem(hwnd_, IDC_SP4_X_SPIN), exp2(MacroProjectData(ID_POWER_X)) - 1, 0);
	UD_SetRange(GetDlgItem(hwnd_, IDC_SP4_Y_SPIN), exp2(MacroProjectData(ID_POWER_Y)) - 1, 0);
	UD_SetRange(GetDlgItem(hwnd_, IDC_SP0_X_SPIN), exp2(MacroProjectData(ID_POWER_X)) - 1, 0);
	UD_SetRange(GetDlgItem(hwnd_, IDC_SP0_Y_SPIN), exp2(MacroProjectData(ID_POWER_Y)) - 1, 0);
	UD_SetPos(GetDlgItem(hwnd_, IDC_SP1_X_SPIN), MacroProjectData(ID_SP_1).x);
	UD_SetPos(GetDlgItem(hwnd_, IDC_SP1_Y_SPIN), MacroProjectData(ID_SP_1).y);
	UD_SetPos(GetDlgItem(hwnd_, IDC_SP2_X_SPIN), MacroProjectData(ID_SP_2).x);
	UD_SetPos(GetDlgItem(hwnd_, IDC_SP2_Y_SPIN), MacroProjectData(ID_SP_2).y);
	UD_SetPos(GetDlgItem(hwnd_, IDC_SP3_X_SPIN), MacroProjectData(ID_SP_3).x);
	UD_SetPos(GetDlgItem(hwnd_, IDC_SP3_Y_SPIN), MacroProjectData(ID_SP_3).y);
	UD_SetPos(GetDlgItem(hwnd_, IDC_SP4_X_SPIN), MacroProjectData(ID_SP_4).x);
	UD_SetPos(GetDlgItem(hwnd_, IDC_SP4_Y_SPIN), MacroProjectData(ID_SP_4).y);
	UD_SetPos(GetDlgItem(hwnd_, IDC_SP0_X_SPIN), MacroProjectData(ID_SP_0).x);
	UD_SetPos(GetDlgItem(hwnd_, IDC_SP0_Y_SPIN), MacroProjectData(ID_SP_0).y);
	if (!read_only)
		EnableControls(true);
}

void InfoWnd::OnColorStatic(Msg<WM_CTLCOLORSTATIC> &msg)
{
	if (msg.ChildHwnd() == GetDlgItem(hwnd_, IDC_FOG_COLOUR))
	{
		msg.SetResult(fog_colour_);
		msg.handled_ = true;
	}
}

#define HANDLE_SP_CHANGE(num, s_sfx, c_sfx)                                 \
	case IDC_SP##num##_##c_sfx:                                              \
		if (EN_CHANGE == msg.CodeNotify())                                    \
		{                                                                     \
		const UINT max_val(exp2(MacroProjectData(ID_POWER_##c_sfx)) - 1);     \
			UINT val(GetDlgItemInt(hwnd_, IDC_SP##num##_##c_sfx, NULL, TRUE)); \
			val = __min(val, max_val);                                         \
			MacroProjectData(ID_SP_##num).##s_sfx = val;                       \
			preview_wnd_.ProjectDataChanged(ProjectData::ID_SP_##num);         \
		} break

void InfoWnd::OnCommand(Msg<WM_COMMAND> &msg)
{
	switch (msg.CtrlId())
	{
		HANDLE_SP_CHANGE(0, x, X);
		HANDLE_SP_CHANGE(0, y, Y);
		HANDLE_SP_CHANGE(1, x, X);
		HANDLE_SP_CHANGE(1, y, Y);
		HANDLE_SP_CHANGE(2, x, X);
		HANDLE_SP_CHANGE(2, y, Y);
		HANDLE_SP_CHANGE(3, x, X);
		HANDLE_SP_CHANGE(3, y, Y);
		HANDLE_SP_CHANGE(4, x, X);
		HANDLE_SP_CHANGE(4, y, Y);
	case IDCANCEL:
		ShowWindow(hwnd_, SW_HIDE);
		break;
	case IDC_ZERO_PLAST:
		if (EN_CHANGE == msg.CodeNotify())
		{
			const UINT max_val(255);
			UINT val(GetDlgItemInt(hwnd_, IDC_ZERO_PLAST, NULL, TRUE));
			val = __min(val, max_val);
			MacroProjectData(ID_ZERO_LEVEL) = val;
			preview_wnd_.ProjectDataChanged(ProjectData::ID_ZERO_LEVEL);
		} break;
	case IDC_FOG_START:
		if (EN_CHANGE == msg.CodeNotify())
		{
			UINT val(GetDlgItemInt(hwnd_, IDC_FOG_START, NULL, FALSE));
			MacroProjectData(ID_FOG_START) = val;
			preview_wnd_.ProjectDataChanged(ProjectData::ID_FOG_START);
		} break;
	case IDC_FOG_END:
		if (EN_CHANGE == msg.CodeNotify())
		{
			unsigned int val(GetDlgItemInt(hwnd_, IDC_FOG_END, NULL, FALSE));
			MacroProjectData(ID_FOG_END) = val;
			preview_wnd_.ProjectDataChanged(ProjectData::ID_FOG_END);
		} break;
	case IDC_FOG_COLOUR_BTN:
		{
			CHOOSECOLOR cc;
			ZeroMemory(&cc, sizeof(cc));
			cc.lStructSize  = sizeof(cc);
			cc.hwndOwner    = hwnd_;
			cc.lpCustColors = custom_colours_;
			cc.rgbResult    = MacroProjectData(ID_FOG_COLOUR);
			cc.Flags        = CC_FULLOPEN | CC_RGBINIT;
			if (ChooseColor(&cc))
			{
				MacroProjectData(ID_FOG_COLOUR) = cc.rgbResult;
				DeleteObject(fog_colour_);
				fog_colour_ = CreateSolidBrush(MacroProjectData(ID_FOG_COLOUR));
				InvalidateRect(GetDlgItem(hwnd_, IDC_FOG_COLOUR), NULL, true);
				preview_wnd_.ProjectDataChanged(ProjectData::ID_FOG_COLOUR);
			}
		} break;
	}
}

void InfoWnd::OnInitDialog(Msg<WM_INITDIALOG> &msg)
{
	HWND item;
	// set spin controls
	{
		const size_t count(10);
		const int units[count] = {
			IDC_SP0_X_SPIN,
			IDC_SP0_Y_SPIN,
			IDC_SP1_X_SPIN,
			IDC_SP1_Y_SPIN,
			IDC_SP2_X_SPIN,
			IDC_SP2_Y_SPIN,
			IDC_SP3_X_SPIN,
			IDC_SP3_Y_SPIN,
			IDC_SP4_X_SPIN,
			IDC_SP4_Y_SPIN
		};
		for (size_t i(0); i != count; ++i)
		{
			item = GetDlgItem(hwnd_, units[i]);
			UD_SetRange(item, 0, 0);
			UD_SetPos(item, 0);
		}
	}
	item = GetDlgItem(hwnd_, IDC_ZERO_PLAST_SPIN);
	UD_SetRange(item, 255, 0);
	UD_SetPos(item, 0);
	// set colours
	fog_colour_ = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	for (int i(0); i != 16; ++i)
		custom_colours_[i] = RGB(255, 255, 255);
	// set fog distance values
	SetDlgItemInt(hwnd_, IDC_FOG_START, 0u, FALSE);
	SetDlgItemInt(hwnd_, IDC_FOG_END,   0u, FALSE);
	// set colour button icon
	Button_SetIcon(GetDlgItem(hwnd_, IDC_FOG_COLOUR_BTN), IDI_TRIANGLE);
	// disable all controls
	EnableControls(false);
	// wrap up
	msg.result_ = TRUE;
	msg.handled_ = true;
}

void InfoWnd::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		&InfoWnd::OnColorStatic,
		&InfoWnd::OnCommand,
		&InfoWnd::OnInitDialog
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}

BOOL CALLBACK InfoWnd::EnumChildrenProc(HWND hwnd, LPARAM lprm)
{
	ri_cast<vector<HWND>*>(lprm)->push_back(hwnd);
	return TRUE;
}

void InfoWnd::EnableControls(bool on)
{
	vector<HWND> children;
	EnumChildWindows(hwnd_, EnumChildrenProc, ri_cast<LPARAM>(&children));
	foreach(HWND &hwnd, children)
		EnableWindow(hwnd, on ? TRUE : FALSE);
}
