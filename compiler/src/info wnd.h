//-----------------------------------------------------------------------------
// Perimeter Map Compiler
// Copyright (c) 2005, Don Reba
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// ? Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer. 
// ? Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution. 
// ? Neither the name of Don Reba nor the names of his contributors may be used
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

#include "panel wnd.h"

#pragma warning(push, 1)
#include <HTMLLayout/htmlayout.h>
#pragma warning(pop)

class PreviewWnd;

//---------------------------------
// map information panel definition
//---------------------------------

class InfoWnd : public PanelWindow
{
// nested classes
public:
	struct ZeroLevelChanged {
		virtual void operator() () = 0;
	};
	typedef vector<POINT> Locations;
// construction
public:
	InfoWnd(PreviewWnd &preview_wnd, ZeroLevelChanged *zero_layer_changed);
// interface
public:
	bool Create(HWND parent_wnd, const RECT &window_rect);
	void Update();
	void SetReadOnly(bool read_only);
// message processing
private:
	void OnCommand(Msg<WM_COMMAND>        &msg);
	void OnNotify (Msg<WM_NOTIFY>         &msg);
	void OnSize   (Msg<WM_SIZE>           &msg);
	void OnTimer  (Msg<WM_TIMER>          &msg);
protected:
	void ProcessMessage(WndMsg &msg);
// internal function
private:
	void AddLocation(tstring name, uint x, uint y);
	SIZE CalculateMinWindowSize() const;
	SIZE CalculateWindowSize() const;
	void PositionChildren();
	// html manipulation
	bool SetHtmlText(const char * id, int value, htmlayout::dom::element root);
	bool SetColorBox(const char * id, COLORREF color, htmlayout::dom::element root);
// data
private:
	Locations         locations_;
	PreviewWnd       &preview_wnd_;
	bool              read_only_;
	uint              zero_level_changes_ignored_;
	ZeroLevelChanged *zero_level_changed_;
	// layout
	HWND layout_;
};
