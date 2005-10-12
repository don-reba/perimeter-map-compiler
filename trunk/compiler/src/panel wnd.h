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

#include "pmc wnd.h"

class PanelHolderWindow;

//------------------------
// panel window definition
//------------------------

class PanelWindow : public PMCWindow
{
// nested types
public:
	// functor called by OnShowWindow
	struct ToggleVisibility {
		virtual void operator() (bool on) = 0;
		virtual ~ToggleVisibility() {};
	};
// creation/destruction
public:
	PanelWindow();
// interface
public:
	bool IsVisible() const;
	void SetVisibilityEvent(ToggleVisibility *visibility_event);
	void SetVisibilityNotification(ToggleVisibility *visibility_notification);
// message handlers
protected:
	void OnCommand   (Msg<WM_COMMAND>    &msg);
	void OnShowWindow(Msg<WM_SHOWWINDOW> &msg);
// message pump
protected:
	void ProcessMessage(WndMsg &msg);
// data
private:
	ToggleVisibility *visibility_event_;
	ToggleVisibility *visibility_notification_;
	bool is_visible_;
};