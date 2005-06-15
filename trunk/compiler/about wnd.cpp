#include "StdAfx.h"

#include "about wnd.h"
#include "resource.h"

#include <cmath>

//---------------------
// About implementation
//---------------------

About::About(void)
	:painting_(false)
	,current_bk_colour_(1)
{
	// initialise the background colours
	bk_colours_[0] = GetSysColor(COLOR_BTNFACE);
	bk_colours_[1] = 0x00007FFF;
	bk_colours_[2] = 0x00FFFFFF;
	bk_colours_[3] = 0x00A56E3A;
	// initialise the hollow control brush
	ctl_brush_ = ri_cast<HBRUSH>(GetStockObject(HOLLOW_BRUSH));
	// initialise image list for drawing the icons
	icon_list_ = ImageList_Create(64, 64, ILC_COLOR24 | ILC_MASK, 1, 1);
	HICON icon(ri_cast<HICON>(LoadImage(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDI_SHRUB_ACTIVE),
		IMAGE_ICON,
		0,
		0,
		LR_DEFAULTCOLOR)));
	ImageList_AddIcon(icon_list_, icon);
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

About::~About(void)
{
	ImageList_Destroy(icon_list_);
	DeleteObject(ctl_brush_);
}

INT_PTR About::DoModal(HWND parent_wnd)
{
	return DialogBoxParam(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_ABOUTBOX),
		parent_wnd,
		DlgProc<About>,
		ri_cast<LPARAM>(this));
}

void About::OnCaptureChanged(Msg<WM_CAPTURECHANGED> &msg)
{
	painting_    = false;
	msg.result_  = FALSE;
	msg.handled_ = true;
}

void About::OnCommand(Msg<WM_COMMAND> &msg)
{
	switch (msg.CtrlId())
	{
	case IDOK:
	case IDCANCEL:
		EndDialog(hwnd_, 0);
		return;
	}
	msg.result_  = FALSE;
	msg.handled_ = true;
}

void About::OnCtlColorStatic(Msg<WM_CTLCOLORSTATIC> &msg)
{
	SetBkMode(msg.DC(), TRANSPARENT);
	msg.result_  = ri_cast<LRESULT>(ctl_brush_);
	msg.handled_ = true;
}

void About::OnDestroy(Msg<WM_DESTROY> &msg)
{
	// destroy the background dc
	DeleteDC(bk_dc_);
	DeleteObject(bk_bmp_);
	msg.result_  = FALSE;
	msg.handled_ = true;
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
	// draw icon
	ImageList_Draw(icon_list_, 0, msg.DC(), 12, 12, ILD_NORMAL);
	msg.result_  = TRUE;
	msg.handled_ = true;
}

void About::OnInitDialog(Msg<WM_INITDIALOG> &msg)
{
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
		HBRUSH brush(CreateSolidBrush(bk_colours_[0]));
		FillRect(bk_dc_, &client_rect, brush);
		DeleteObject(brush);
	}
	// set timers
	SetTimer(hwnd_, ri_cast<UINT_PTR>(this),     12000, ChangeBackground);
	SetTimer(hwnd_, ri_cast<UINT_PTR>(this) + 1, 128,   DrawStroke);
	msg.result_  = TRUE;
	msg.handled_ = true;
}

void About::OnLButtonDown(Msg<WM_LBUTTONDOWN> &msg)
{
	SetCapture(hwnd_);
	cursor_pos_.x = msg.Position().x;
	cursor_pos_.y = msg.Position().y;
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
	{
		POINT corner;
		// top left
		corner.x = client_rect.left;
		corner.y = client_rect.top;
		ClientToScreen(hwnd_, &corner);
		client_rect.left = corner.x;
		client_rect.top  = corner.y;
		// bottom right
		corner.x = client_rect.right;
		corner.y = client_rect.bottom;
		ClientToScreen(hwnd_, &corner);
		client_rect.right  = corner.x;
		client_rect.bottom = corner.y;
	}
	GetCursorPos(&cursor_pos);
	if (TRUE == PtInRect(&client_rect, cursor_pos))
		SetCursor(LoadCursor(NULL, IDC_HAND));
	else
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	msg.result_  = TRUE;
	msg.handled_ = true;
}

void About::ProcessMessage(WndMsg &msg)
{
	static Handler mmp[] =
	{
		OnCaptureChanged,
		OnCommand,
		OnCtlColorStatic,
		OnDestroy,
		OnEraseBkgnd,
		OnInitDialog,
		OnLButtonDown,
		OnLButtonUp,
		OnMouseMove,
		OnSetCursor
	};
	if (!Handler::Call(mmp, this, msg))
		__super::ProcessMessage(msg);
}

VOID CALLBACK About::ChangeBackground(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	About* obj(ri_cast<About*>(idEvent));
	obj->current_bk_colour_ = (obj->current_bk_colour_ + 1) % obj->num_bk_colours_;
}

VOID CALLBACK About::DrawStroke(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	About* obj(ri_cast<About*>(idEvent - 1));
	if (!obj->painting_)
		return;
	// seed random number generator
	srand(clock());
	// draw a stroke
	for (size_t r(0); r != brush_size_; ++r)
		for (size_t c(0); c != brush_size_; ++c)
		{
			float p(static_cast<float>(rand()));
			p /= RAND_MAX;
			if (p > obj->brush_[r][c])
			{
				const size_t x(r - brush_size_ / 2);
				const size_t y(c - brush_size_ / 2);
				SetPixelV(
					obj->bk_dc_,
					obj->cursor_pos_.x + x,
					obj->cursor_pos_.y + y,
					obj->bk_colours_[obj->current_bk_colour_]);
			}
		}
	InvalidateRect(hWnd, NULL, TRUE);
}