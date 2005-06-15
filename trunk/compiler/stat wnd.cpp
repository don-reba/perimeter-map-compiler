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

void StatWnd::SetAverageHeight(uint height)
{
	SetDlgItemInt(hwnd_, IDC_AVE_HEIGHT, height, false);
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
