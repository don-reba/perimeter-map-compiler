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

#include "about wnd.h"
#include "../resource.h"
#include "resource management.h"
#include "task common.h"

#include "gaussian blur.ipp"

#include <cmath>
#include <fstream>

//---------------------
// About implementation
//---------------------

About::About()
	:painting_(false)
	,current_bk_colour_(0)
	,lightmap_lookup_(ComputeLightmapLookup())
{
	// set initial mouse coordinate records
	cursor_pos_.x = cursor_pos_.y = 0;
	// initialise the background colours
	bk_colours_[0] = 0x00007FFF;
	bk_colours_[1] = 0x00FFFFFF;
	bk_colours_[2] = 0x00A56E3A;
	// initialise the painting brush probabilites
	for (size_t r(0); r != brush_size_; ++r)
		for (size_t c(0); c != brush_size_; ++c)
		{
			const float x(r - brush_size_ / 2.0f);
			const float y(c - brush_size_ / 2.0f);
			float radius = sqrt(x*x + y*y);
			brush_[r][c] = radius / brush_size_ * 2;
		}
}

About::~About()
{
	delete [] lightmap_lookup_;
}

INT_PTR About::DoModal(HWND parent_wnd)
{
	HINSTANCE hinstance(GetModuleHandle(NULL));
	// register window class
	WNDCLASSEX window_class = { sizeof(window_class) };
	{
		window_class.lpfnWndProc   = WndProc<About>;
		window_class.hInstance     = hinstance;
		window_class.hIcon         = LoadIcon(hinstance, (LPCTSTR)IDI_COMPILER);
		window_class.hCursor       = LoadCursor(NULL, IDC_ARROW);
		window_class.lpszClassName = _T("EnhancedAboutDialog");
		window_class.hIconSm       = LoadIcon(hinstance, (LPCTSTR)IDI_COMPILER);
		RegisterClassEx(&window_class);
	}
	// create window
	CreateWindow(
		window_class.lpszClassName,
		_T("About Perimeter Map Compiler"),
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		parent_wnd,
		NULL,
		hinstance,
		this);
	if (NULL == hwnd_)
		return IDCANCEL;
	// disable the parent
	bool parent_was_enabled(EnableWindow(parent_wnd, FALSE) == FALSE);
	// enter the message loop
	MSG msg = {};
	quitting_ = false;
	while (!quitting_ && GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	if (parent_was_enabled)
	{
		EnableWindow(parent_wnd, TRUE);
		SetFocus(parent_wnd);
	}
	DestroyWindow(hwnd_);
	if (WM_QUIT == msg.message)
		PostQuitMessage(static_cast<int>(msg.wParam));
	return IDOK;
}

void About::OnCaptureChanged(Msg<WM_CAPTURECHANGED> &msg)
{
	painting_    = false;
	msg.result_  = FALSE;
	msg.handled_ = true;
}

void About::OnClose(Msg<WM_CLOSE> &msg)
{
	Destroy();
}

void About::OnCreate(Msg<WM_CREATE> &msg)
{
	msg.result_  = FALSE;
	msg.handled_ = true;
	// load evmp
	uint width, height;
	{
		const size_t evmp_alloc(0x200000); // 2MB should be enough
		vector<BYTE> evmp(evmp_alloc);
		if (!RsrcMgmt::UncompressResource(IDR_EVMP, &evmp[0], evmp_alloc))
		{
			MacroDisplayError(_T("EVMP could not be loaded."));
			return;
		}
		width  = *ri_cast<uint*>(&evmp[0]);
		height = *ri_cast<uint*>(&evmp[4]);
		bmp_size_.cx = width;
		bmp_size_.cy = height;
		map_size_.cx = width + 1;
		map_size_.cy = height;
		map_.resize(map_size_.cx * map_size_.cy);
		CopyMemory(&map_[0], &evmp[8], map_.size() * sizeof(MapPixel));
		// initialize the temporary buffer
		temp_dim_.cx = bmp_size_.cx + 2 * border_;
		temp_dim_.cy = map_size_.cy + 2 * border_;
		temp_buffer_.resize(temp_dim_.cx * temp_dim_.cy);
	}
	// initialise the background bitmap
	{
		RECT client_rect;
		GetClientRect(hwnd_, &client_rect);
		HDC client_dc(GetDC(hwnd_));
		bk_dc_ = CreateCompatibleDC(client_dc);
		bk_bmp_ = CreateCompatibleBitmap(
			client_dc,
			client_rect.right - client_rect.left,
			client_rect.bottom - client_rect.top);
		ReleaseDC(hwnd_, client_dc);
		SelectObject(bk_dc_, bk_bmp_);
		RenderLines(0, map_size_.cy);
	}
	// resize window (centre on the same monitor as the parent)
	{
		RECT rect = { 0, 0, width, height };
		AdjustWindowRect(&rect, GetWindowLong(hwnd_, GWL_STYLE), FALSE);
		HMONITOR monitor(MonitorFromWindow(GetParent(hwnd_), MONITOR_DEFAULTTONEAREST));
		MONITORINFO info = { sizeof(info) };
		if (!GetMonitorInfo(monitor, &info))
		{
			MacroDisplayError(_T("Monitor information could not be querried."));
			return;
		}
		SIZE win_size = { rect.right - rect.left, rect.bottom - rect.top };
		RECT &mon_rect(info.rcMonitor);
		SetWindowPos(
			hwnd_,
			NULL,
			mon_rect.left + ((mon_rect.right  - mon_rect.left) - win_size.cx) / 2,
			mon_rect.top  + ((mon_rect.bottom - mon_rect.top)  - win_size.cy) / 2,
			win_size.cx,
			win_size.cy,
			SWP_SHOWWINDOW | SWP_NOZORDER);
	}
	// set timers
	SetTimer(hwnd_, TMR_CHANGE_BK,   10000, NULL);
	SetTimer(hwnd_, TMR_DRAW_STROKE, 128,   NULL);
	msg.result_  = TRUE;
	msg.handled_ = true;
}

void About::OnDestroy(Msg<WM_DESTROY> &msg)
{
	// destroy the background dc
	DeleteDC(bk_dc_);
	DeleteObject(bk_bmp_);
}

void About::OnEraseBkgnd(Msg<WM_ERASEBKGND> &msg)
{
	// draw background
	RECT client_rect;
	GetClientRect(hwnd_, &client_rect);
	BitBlt(
		msg.DC(),
		0,
		0,
		client_rect.right - client_rect.left,
		client_rect.bottom - client_rect.top,
		bk_dc_,
		0,
		0,
		SRCCOPY);
	msg.handled_ = true;
}

void About::OnKeyDown(Msg<WM_KEYDOWN> &msg)
{
	switch (msg.VKey())
	{
	case VK_RETURN:
	case VK_ESCAPE:
	case VK_SPACE:
		Destroy();
		break;
	}
}

void About::OnLButtonDown(Msg<WM_LBUTTONDOWN> &msg)
{
	cursor_pos_.x = msg.Position().x;
	cursor_pos_.y = msg.Position().y;
	if (
		cursor_pos_.x > 0 && cursor_pos_.x < bmp_size_.cx &&
		cursor_pos_.y > 0 && cursor_pos_.y < bmp_size_.cy &&
		!(map_[cursor_pos_.y * map_size_.cx + cursor_pos_.x].flags & MapPixel::F_OK))
		Destroy();
	SetCapture(hwnd_);
	painting_     = true;
	msg.result_   = FALSE;
	msg.handled_  = true;
}

void About::OnLButtonUp(Msg<WM_LBUTTONUP> &msg)
{
	painting_ = false;
	ReleaseCapture();
	msg.result_  = FALSE;
	msg.handled_ = true;
}

void About::OnMouseMove(Msg<WM_MOUSEMOVE> &msg)
{
	cursor_pos_.x = msg.Position().x;
	cursor_pos_.y = msg.Position().y;
	msg.result_   = FALSE;
	msg.handled_  = true;
}

void About::OnSetCursor(Msg<WM_SETCURSOR> &msg)
{
	if (msg.Hwnd() != hwnd_)
		return;
	// set cursor to hand in client area, but to normal elsewhere
	RECT client_rect;
	POINT cursor_pos;
	GetClientRect(hwnd_, &client_rect);
	ClientToScreen(hwnd_, &client_rect);
	GetCursorPos(&cursor_pos);
	if (
		TRUE == PtInRect(&client_rect, cursor_pos)        &&
		cursor_pos_.x > 0 && cursor_pos_.x < bmp_size_.cx &&
		cursor_pos_.y > 0 && cursor_pos_.y < bmp_size_.cy &&
		map_[cursor_pos_.y * map_size_.cx + cursor_pos_.x].flags & MapPixel::F_OK)
	{
		SetCursor(LoadCursor(NULL, IDC_HAND));
	}
	else
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	msg.result_  = TRUE;
	msg.handled_ = true;
}

void About::OnTimer(Msg<WM_TIMER> &msg)
{
	switch (msg.TimerId())
	{
	case TMR_CHANGE_BK:   ChangeBackground(); break;
	case TMR_DRAW_STROKE: DrawStroke();       break;
	};
}

void About::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		&About::OnCaptureChanged,
		&About::OnClose,
		&About::OnCreate,
		&About::OnDestroy,
		&About::OnEraseBkgnd,
		&About::OnKeyDown,
		&About::OnLButtonDown,
		&About::OnLButtonUp,
		&About::OnMouseMove,
		&About::OnSetCursor,
		&About::OnTimer
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}

void About::ChangeBackground()
{
	current_bk_colour_ = (current_bk_colour_ + 1) % num_bk_colours_;
}

const BYTE *About::ComputeLightmapLookup()
{
	BYTE *lightmap_lookup(new BYTE[0x1FF]);
	float normal_dy(1.0f);
	float light_dx (1.0f / static_cast<float>(sqrt(2.0f)));
	float light_dy (1.0f / static_cast<float>(sqrt(2.0f)));
	for (int i(0); i != 0x1FF; ++i)
	{
		float normal_dx(-static_cast<float>(i - 0xFF));
		// calculate and store the cosine of the angle between the vectors
		float normal_l(sqrt(normal_dx * normal_dx + normal_dy * normal_dy));
		float cos_a((normal_dx * light_dx + normal_dy * light_dy) / normal_l);
		lightmap_lookup[i] = static_cast<BYTE>(0xFF * abs(cos_a));
	}
	return lightmap_lookup;
}

void About::DrawStroke()
{
	if (!painting_)
		return;
	// return if the brush does not touch the canvas
	if (
		cursor_pos_.x + static_cast<int>(brush_size_/2) <  0            ||
		cursor_pos_.y + static_cast<int>(brush_size_/2) <  0            ||
		cursor_pos_.x - static_cast<int>(brush_size_/2) >= bmp_size_.cx ||
		cursor_pos_.y - static_cast<int>(brush_size_/2) >= bmp_size_.cy)
		return;
	// seed random number generator
	srand(clock());
	// calculate the border around the painting region
	_ASSERTE(0 == brush_size_ % 2);
	const int radius(brush_size_ / 2);
	RECT offset;
	offset.left   = cursor_pos_.x - radius;
	offset.top    = cursor_pos_.y - radius;
	offset.right  = map_size_.cx - offset.left - brush_size_;
	offset.bottom = map_size_.cy - offset.top  - brush_size_;
	// draw a stroke
	MapType::iterator map_iter(map_.begin() + map_size_.cx * offset.top);
	for (int r(0); r != brush_size_; ++r)
	{
		if (0 <= offset.top + r && offset.top + r < bmp_size_.cy)
		{
			map_iter += offset.left;
			for (int c(0); c != brush_size_; ++c)
			{
				if (0 <= offset.left + c && offset.left + c < bmp_size_.cx)
				{
					float p(static_cast<float>(rand()));
					p /= RAND_MAX;
					if (p > brush_[r][c] && map_iter->flags & MapPixel::F_SOFT)
						if (current_bk_colour_ < 3)
							map_iter->top_texture = bk_colours_[current_bk_colour_];
						else
							map_iter->heightmap = static_cast<uint>(__max(0, static_cast<int>(map_iter->heightmap) - 3));
				}
				++map_iter;
			}
			map_iter += offset.right;
		}
		else
			map_iter += map_size_.cx;
	}
	{
		uint overlap(0);
		if (offset.top < 0)
			overlap += -offset.top;
		if (offset.top + static_cast<int>(brush_size_) >= bmp_size_.cy)
			overlap += offset.top + brush_size_ - bmp_size_.cy + 1;
		const uint first_line(__max(0, offset.top));
		const uint line_count(brush_size_ - overlap);
		RenderLines(first_line, line_count);
	}
	InvalidateRect(hwnd_, NULL, TRUE);
}

void About::Destroy()
{
	quitting_ = true;
}

void About::RenderLines(uint first_line, uint line_count)
{
	// calculate adjusted first_line and line_count, for borders
	const uint first_line_e(__max(0, static_cast<int>(first_line) - border_));
	const uint temp_offset (border_ + first_line_e - first_line);
	const uint line_count_e(__min(
		line_count + border_ * 2 - temp_offset,
		line_count - (first_line_e + line_count - bmp_size_.cy)));
	// calculate the lightmap
	{
		TempType::iterator      temp_iter(temp_buffer_.begin() + temp_dim_.cx * temp_offset);
		MapType::const_iterator map_iter (map_.begin() + map_size_.cx * first_line_e);
		MapType::const_iterator row_iter;
		MapType::const_iterator peak_x;
		uint                    peak_y;
		for (uint y(0); y != line_count_e; ++y)
		{
			row_iter = map_iter + bmp_size_.cx; // end of the row
			peak_x  = row_iter;                // location of the nearest peak
			peak_y  = row_iter->heightmap;     // height of the nearest peak
			temp_iter += bmp_size_.cx + border_;    // move to the end of the row
			while (row_iter != map_iter)
			{
				--temp_iter;
				--row_iter;
				_ASSERTE(temp_iter - temp_buffer_.begin() < static_cast<ptrdiff_t>(temp_buffer_.size()));
				const uint dx(&*peak_x - &*row_iter); // distance to the peak
				const uint y (row_iter->heightmap);   // current height
				// if the point is unshadowed, dot surface slope with the sun, othewise set to zero
				// the sun vector is presumed to be (1,1)
				if (peak_y < y || peak_y - y < dx * 2) // shadows are shortened twofold
				{
					// carry out the projection table lookup
					const int dy(row_iter[1].heightmap - static_cast<int>(y));
					*temp_iter = lightmap_lookup_[dy + 0xFF];
					// shift the highest point to this one
					peak_y = y;
					peak_x = row_iter;
				}
				else
					*temp_iter = 0x00; // shadow lightness
			}
			temp_iter += temp_dim_.cx - border_;
			map_iter  += map_size_.cx;
		}
	}
	// blur the buffer
	GaussianBlur<BYTE, ushort>(&temp_buffer_[0], temp_dim_);
	// merge the buffer with the bitmap
	{
		TempType::const_iterator temp_iter(temp_buffer_.begin() + temp_dim_.cx * border_);
		MapType::const_iterator  map_iter (map_.begin() + map_size_.cx * first_line);
		for (uint y(first_line); y != first_line + line_count; ++y)
		{
			temp_iter += border_;
			for (LONG x(0); x != bmp_size_.cx; ++x)
			{
				// determine the texture colour
				const COLORREF colour(map_iter->heightmap ? map_iter->top_texture : map_iter->bottom_texture);
				// compute the lighting ratio * 0x100
				const uint factor(0x101); // precision (odd to avoid bit shifts)
				const uint i_light(((*temp_iter + 0x80) * factor) / 0xFF);
				SetPixelV(bk_dc_, x, y, RGB(
					__min(0xFF, (GetRValue(colour) * i_light) / factor),
					__min(0xFF, (GetGValue(colour) * i_light) / factor),
					__min(0xFF, (GetBValue(colour) * i_light) / factor)));
				++temp_iter;
				++map_iter;
			}
			temp_iter += border_;
			++map_iter;
		}
	}
}
