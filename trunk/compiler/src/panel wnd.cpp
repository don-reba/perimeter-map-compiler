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
// • Neither the name of Don Reba nor the names of his contributors may be used
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
	:is_visible_(false)
{}

bool PanelWindow::IsVisible() const
{
	return is_visible_;
}


void PanelWindow::OnCommand(Msg<WM_COMMAND> &msg)
{
	switch (msg.CtrlId())
	{
	case IDCANCEL:
		ShowWindow(hwnd_, SW_HIDE);
		break;
	}
	msg.handled_ = true;
}

void PanelWindow::OnShowWindow(Msg<WM_SHOWWINDOW> &msg)
{
	is_visible_ = msg.IsShown();
	if (is_visible_)
		foreach (const on_show_t::delegate_t &delegate, on_show_)
			delegate();
	else
		foreach (const on_hide_t::delegate_t &delegate, on_hide_)
			delegate();
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
