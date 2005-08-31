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


//--------------------------------
// The application "About" dialog.
// Styled after Vangers.
//--------------------------------

#pragma once

#include "pmc wnd.h"

#include <commctrl.h>

//------------------
// About declaration
//------------------

class About : public PMCWindow
{
private:
	struct MapPixel
	{
		enum Flag { F_SOFT = 1, F_OK = 2 };
		uint top_texture    : 24;
		uint flags          : 8;
		uint bottom_texture : 24;
		uint heightmap      : 8;
	};
	typedef vector<MapPixel> MapType;
	enum Timer {
		TMR_CHANGE_BK,
		TMR_DRAW_STROKE
	};
	typedef vector<BYTE> TempType;
public:
	About();
	~About();
// interface
public:
	INT_PTR DoModal(HWND parent_wnd_);
// message handlers
private:
	void OnCaptureChanged(Msg<WM_CAPTURECHANGED> &msg);
	void OnClose         (Msg<WM_CLOSE>          &msg);
	void OnCreate        (Msg<WM_CREATE>         &msg);
	void OnDestroy       (Msg<WM_DESTROY>        &msg);
	void OnEraseBkgnd    (Msg<WM_ERASEBKGND>     &msg);
	void OnKeyDown       (Msg<WM_KEYDOWN>        &msg);
	void OnLButtonDown   (Msg<WM_LBUTTONDOWN>    &msg);
	void OnLButtonUp     (Msg<WM_LBUTTONUP>      &msg);
	void OnMouseMove     (Msg<WM_MOUSEMOVE>      &msg);
	void OnSetCursor     (Msg<WM_SETCURSOR>      &msg);
	void OnTimer         (Msg<WM_TIMER>          &msg);
// internal function
protected:
	void ProcessMessage(WndMsg &msg);
private:
	void ChangeBackground();
	void Destroy();
	void DrawStroke();
	void RenderLines(uint first_line, uint line_count);
// data
private:
	static const size_t num_bk_colours_ = 6;     // number of background colours to cycle through
	static const size_t brush_size_     = 64;    // diameter of the brush in pixels
	static const int    border_         = 3;     // hidden border around the image, for blurring
	HBITMAP    bk_bmp_;                          // bitmap to back the background DC
	COLORREF   bk_colours_[num_bk_colours_];     // background colours to cycle through
	HDC        bk_dc_;                           // background DC
	SIZE       bmp_size_;                        // dimensions of the map, excluding padding
	float      brush_[brush_size_][brush_size_]; // probability that a pixel under the brush will be painted
	size_t     current_bk_colour_;               // index of the current stroke colour
	POINT      cursor_pos_;                      // position of the cursor for painting
	MapType    map_;                             // the array containing map data
	HWND       parent_hwnd_;
	SIZE       map_size_;                        // dimensions of the map, including padding
	bool       parent_was_enabled_;              // whether the parent has to be reenabled at exit
	bool       painting_;                        // determines whether DrawStroke should act or not
	bool       quitting_;                        // flag signifying the dialog's coming demise
	TempType   temp_buffer_;
	SIZE       temp_dim_;
};
