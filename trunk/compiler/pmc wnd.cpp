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

void PMCWindow::OnDestroy(Msg<WM_DESTROY> &msg)
{
	hwnd_ = NULL;
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
		&PMCWindow::OnDestroy,
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