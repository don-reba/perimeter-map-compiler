#include "stdafx.h"
#include "pmc wnd.h"

//-------------------------
// PMCWindow implementation
//-------------------------

PMCWindow::PMCWindow()
	:ErrorHandler(hwnd_)
{
	cursor_ = LoadCursor(NULL, IDC_ARROW);
	old_cursor_ = NULL;
}

RECT PMCWindow::GetRect() const
{
	return window_rect_;
}

void PMCWindow::ToggleWaitCursor(bool on)
{
	if (on)
	{
		if (NULL != old_cursor_)
			return;
		old_cursor_ = cursor_;
		cursor_ = LoadCursor(NULL, IDC_APPSTARTING);
	}
	else
	{
		if (NULL == old_cursor_)
			return;
		cursor_ = old_cursor_;
		old_cursor_ = NULL;
	}
}

void PMCWindow::OnMove(Msg<WM_MOVE> &msg)
{
	GetWindowRect(hwnd_, &window_rect_);
	msg.handled_ = true;
}

void PMCWindow::OnSetCursor(Msg<WM_SETCURSOR> &msg)
{
	SetCursor(cursor_);
	msg.handled_ = true;
}

void PMCWindow::OnSize(Msg<WM_SIZE> &msg)
{
	GetWindowRect(hwnd_, &window_rect_);
	msg.handled_ = true;
}

void PMCWindow::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		&PMCWindow::OnMove,
		&PMCWindow::OnSetCursor,
		&PMCWindow::OnSize
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}

void PMCWindow::SetRect(const RECT &rect)
{
	window_rect_ = rect;
}