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

#include "resource.h"
#include "stat wnd.h"

//--------------------------------
// statistics panel implementation
//--------------------------------

bool StatWnd::Create(HWND parent_wnd, const RECT &window_rect)
{
	hwnd_ = CreateDialogParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_STAT_DLG),
		parent_wnd,
		DlgProc<StatWnd>,
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
	return true;
}

void StatWnd::SetAverageHeight(float height)
{
	const size_t buffer_size(32);
	vector<TCHAR> v_buffer(buffer_size);
	TCHAR *buffer(&v_buffer[0]);
	_sntprintf(buffer, buffer_size - 1, "%.1f", height);
	SetDlgItemText(hwnd_,IDC_AVE_HEIGHT, buffer);
}

void StatWnd::SetAverageColour(COLORREF colour)
{
	DeleteObject(average_colour_);
	average_colour_ = CreateSolidBrush(colour);
	InvalidateRect(GetDlgItem(hwnd_, IDC_AVE_COLOUR), NULL, TRUE);
}

void StatWnd::OnInitDialog(Msg<WM_INITDIALOG> &msg)
{
	average_colour_ = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
}

void StatWnd::OnColorStatic(Msg<WM_CTLCOLORSTATIC> &msg)
{
	if (msg.ChildHwnd() == GetDlgItem(hwnd_, IDC_AVE_COLOUR))
	{
		msg.result_  = ri_cast<LRESULT>(average_colour_);
		msg.handled_ = true;
	}
}

void StatWnd::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		&StatWnd::OnColorStatic,
		&StatWnd::OnInitDialog
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}
