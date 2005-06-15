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

#include "panel wnd.h"

//---------------------------
// PanelWindow implementation
//---------------------------

PanelWindow::PanelWindow()
	:is_visible_             (false)
	,visibility_event_       (NULL)
	,visibility_notification_(NULL)
{}

bool PanelWindow::IsVisible() const
{
	return is_visible_;
}

// PRE: event must be allocated on the heap
void PanelWindow::SetVisibilityEvent(PanelWindow::ToggleVisibility *visibility_event)
{
	_ASSERTE(NULL == visibility_event_);
	visibility_event_ = visibility_event;
}

// PRE: notification must be allocated on the heap
void PanelWindow::SetVisibilityNotification(PanelWindow::ToggleVisibility *visibility_notification)
{
	if (NULL != visibility_notification_)
		delete visibility_notification_;
	visibility_notification_ = visibility_notification;
}

void PanelWindow::OnCommand(Msg<WM_COMMAND> &msg)
{
	switch (msg.CtrlId())
	{
	case IDCANCEL:
		ShowWindow(hwnd_, SW_HIDE);
	}
	msg.handled_ = true;
}

void PanelWindow::OnShowWindow(Msg<WM_SHOWWINDOW> &msg)
{
	is_visible_ = msg.IsShown();
	if (NULL != visibility_event_)
		(*visibility_event_)(is_visible_);
	if (NULL != visibility_notification_ && is_visible_)
	{
		(*visibility_notification_)(is_visible_);
		delete visibility_notification_;
		visibility_notification_ = NULL;
	}
	msg.handled_ = true;
}

void PanelWindow::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		&PanelWindow::OnCommand,
		&PanelWindow::OnShowWindow
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}