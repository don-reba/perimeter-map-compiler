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

#include "app data.h"
#include "preference wnd.h"
#include "preview wnd.h"
#include "project manager.h"
#include "resource.h"
#include <commctrl.h>

//----------------------------
// additional message crackers
//----------------------------

#define UD_SetRange(hwndCtl, nLower, nUpper) ((void)SNDMSG((hwndCtl), UDM_SETRANGE, 0L, MAKELPARAM((nLower), (nUpper))))
#define UD_SetPos(hwndCtl, nPos) ((void)SNDMSG((hwndCtl), UDM_SETPOS, 0L, MAKELPARAM((nPos), 0)))
#define UD_GetPos(hwndCtl, nPos) ((int)SNDMSG((hwndCtl), UDM_GETPOS, 0L, 0L))
#define Button_SetIcon(hwndCtl, icon_id) ((void)SNDMSG((hwndCtl), BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(icon_id), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR)))

//-----------------------------
// PreferenceWnd implementation
//-----------------------------

PreferenceWnd::PreferenceWnd(PreviewWnd &preview_wnd, ProjectManager &project_manager)
	:preview_wnd_    (preview_wnd)
	,project_manager_(project_manager)
{
	zero_plast_colour_       = GetSysColor(COLOR_BTNFACE);
	zero_plast_colour_brush_ = CreateSolidBrush(zero_plast_colour_);
	for (int i(0); i != 16; ++i)
		custom_colors_[i] = RGB(255, 255, 255);
}

PreferenceWnd::~PreferenceWnd()
{
	DeleteObject(zero_plast_colour_brush_);
}

void PreferenceWnd::Create(HWND hWndParent)
{
	// create the window
	hwnd_ = CreateDialogParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_PREFERENCES),
		hWndParent,
		DlgProc<PreferenceWnd>,
		ri_cast<LPARAM>(this));
	if (NULL == hwnd_)
		MacroDisplayError(_T("The Preferences window could not be created."));
}

void PreferenceWnd::OnCommand(Msg<WM_COMMAND> &msg)
{
	switch (msg.CtrlId())
	{
	case IDOK:
		Apply();
		DestroyWindow(hwnd_);
		break;
	case IDCANCEL:
		DestroyWindow(hwnd_);
		break;
	case ID_APPLY_NOW:
		Apply();
		break;
	case IDC_ZERO_PLAST_COLOUR_BTN:
		{
			CHOOSECOLOR cc;
			ZeroMemory(&cc, sizeof(cc));
			cc.lStructSize = sizeof(cc);
			cc.hwndOwner = hwnd_;
			cc.lpCustColors = custom_colors_;
			cc.rgbResult = zero_plast_colour_;
			cc.Flags = CC_FULLOPEN | CC_RGBINIT;
			if (ChooseColor(&cc))
			{
				zero_plast_colour_ = cc.rgbResult;
				DeleteObject(zero_plast_colour_brush_);
				zero_plast_colour_brush_ = CreateSolidBrush(zero_plast_colour_);
				InvalidateRect(GetDlgItem(hwnd_, IDC_ZERO_PLAST_COLOUR), NULL, true);
			}
		} break;
	}
	msg.result_  = FALSE;
	msg.handled_ = TRUE;
}

void PreferenceWnd::OnColorStatic(Msg<WM_CTLCOLORSTATIC> &msg)
{
	if (msg.ChildHwnd() == GetDlgItem(hwnd_, IDC_ZERO_PLAST_COLOUR))
	{
		msg.SetResult(zero_plast_colour_brush_);
		msg.handled_ = true;
	}
}

void PreferenceWnd::OnInitDialog(Msg<WM_INITDIALOG> &msg)
{
	// initialise controls
	const size_t buffer_size(256);
	TCHAR buffer[buffer_size];
	// threshold
	{
		_stprintf(buffer, "%.2f", MacroAppData(ID_THRESHOLD));
		SetDlgItemText(hwnd_, IDC_LOD, buffer);
	}
	// opacity
	{
		HWND zero_plast_opacity_spin(GetDlgItem(hwnd_, IDC_ZERO_PLAST_OPACITY_SPIN));
		UD_SetRange(GetDlgItem(hwnd_, IDC_ZERO_PLAST_OPACITY_SPIN), 255, 0);
		UD_SetPos(zero_plast_opacity_spin, MacroAppData(ID_ZERO_LAYER_OPACITY));
		Button_SetIcon(GetDlgItem(hwnd_, IDC_ZERO_PLAST_COLOUR_BTN), IDI_TRIANGLE);
	}
	// colour
	{
		zero_plast_colour_ = (MacroAppData(ID_ZERO_LAYER_COLOUR));
		DeleteObject(zero_plast_colour_brush_);
		zero_plast_colour_brush_ = CreateSolidBrush(zero_plast_colour_);
		InvalidateRect(GetDlgItem(hwnd_, IDC_ZERO_PLAST_COLOUR), NULL, TRUE);
	}
	// texture_quality
	{
		CheckRadioButton(
			hwnd_,
			IDC_TEXTURE_COLOUR_SPEED,
			IDC_TEXTURE_COLOUR_QUALITY,
			MacroAppData(ID_FAST_TEXTURE_QUANTIZATION) ? IDC_TEXTURE_COLOUR_SPEED : IDC_TEXTURE_COLOUR_QUALITY);
	}
	// lighting
	{
		CheckDlgButton(hwnd_, IDC_LIGHTING, MacroAppData(ID_ENABLE_LIGHTING) ? TRUE : FALSE);
	}
	msg.result_  = TRUE;
	msg.handled_ = true;
}

void PreferenceWnd::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		&PreferenceWnd::OnCommand,
		&PreferenceWnd::OnColorStatic,
		&PreferenceWnd::OnInitDialog
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}

void PreferenceWnd::Apply()
{
	// get control info
	bool  enable_lighting(true);
	uint  opacity(0x28);
	bool  texture_colour_quality(false);
	float threshold(3.0f);
	// enable_lighting
	{
		enable_lighting = BST_CHECKED == IsDlgButtonChecked(hwnd_, IDC_LIGHTING);
	}
	// opacity
	{
		opacity = GetDlgItemInt(hwnd_, IDC_ZERO_PLAST_OPACITY, NULL, FALSE);
		opacity = __min(255, opacity);
	}
	// texture_colour_quality
	{
		texture_colour_quality = BST_CHECKED == IsDlgButtonChecked(hwnd_, IDC_TEXTURE_COLOUR_QUALITY);
	}
	// threshold
	{
		HWND ctrl(GetDlgItem(hwnd_, IDC_LOD));
		const size_t buffer_size(GetWindowTextLength(ctrl) + 1);
		TCHAR *buffer(new TCHAR[buffer_size]);
		GetWindowText(ctrl, buffer, buffer_size);
		_stscanf(buffer, "%f", &threshold);
		delete [] buffer;
		threshold = __max(0, __min(8.0f, threshold));
	}
	// see which actions will have to be taken, depending on which properties changed
	bool reload_heightmap(false);
	bool reload_texture  (false);
	bool update_preview  (false);
	if (
		MacroAppData(ID_ZERO_LAYER_COLOUR)  != zero_plast_colour_ ||
		MacroAppData(ID_ZERO_LAYER_OPACITY) != opacity)
	{
		update_preview = true;
	}
	if (MacroAppData(ID_ENABLE_LIGHTING) != enable_lighting)
	{
		reload_heightmap = true;
		reload_texture   = true;
	}
	if (MacroAppData(ID_THRESHOLD) != threshold)
	{
		reload_heightmap = true;
	}
	// commit settings
	MacroAppData(ID_ENABLE_LIGHTING)    = enable_lighting;
	MacroAppData(ID_THRESHOLD)          = threshold;
	MacroAppData(ID_ZERO_LAYER_COLOUR)  = zero_plast_colour_;
	MacroAppData(ID_ZERO_LAYER_OPACITY) = opacity;
	// take the necessary actions
	if (update_preview)
		preview_wnd_.UpdateSettings();
	if (reload_heightmap || reload_texture)
	{
		project_manager_.UpdateSettings();
		project_manager_.ReloadFiles(reload_heightmap, reload_texture);
	}
}