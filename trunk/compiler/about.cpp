#include "StdAfx.h"
#include "about.h"
#include "resource.h"
#include <cmath>

CAbout::CAbout(void)
	:painting(false)
	,current_bk_colour(1)
{
	// initialise background colours
	bk_colours[0] = GetSysColor(COLOR_BTNFACE);
	bk_colours[1] = 0x00007FFF;
	bk_colours[2] = 0x00FFFFFF;
	bk_colours[3] = 0x00A56E3A;
	// initialise the hollow control brush
	ctl_brush = ri_cast<HBRUSH>(GetStockObject(HOLLOW_BRUSH));
	// initialise image list for drawing the icons
	icon_list = ImageList_Create(64, 64, ILC_COLOR24 | ILC_MASK, 1, 1);
	HICON icon(ri_cast<HICON>(LoadImage(
		GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDI_SHRUB_ACTIVE),
		IMAGE_ICON,
		0,
		0,
		LR_DEFAULTCOLOR)));
	ImageList_AddIcon(icon_list, icon);
	// initialise the painting brush probabilites
	for (size_t r(0); r != brush_size; ++r)
		for (size_t c(0); c != brush_size; ++c)
		{
			const float x(r - brush_size / 2.0f);
			const float y(c - brush_size / 2.0f);
			float radius = sqrt(x*x + y*y);
			brush[r][c] = radius / brush_size * 2;
		}
}

CAbout::~CAbout(void)
{
	ImageList_Destroy(icon_list);
	DeleteObject(ctl_brush);
}

INT_PTR CAbout::DoModal(HINSTANCE hInstance, HWND hParentWnd)
{
	return DialogBoxParam(
		hInstance,
		MAKEINTRESOURCE(IDD_ABOUTBOX),
		hParentWnd,
		(DLGPROC)DialogProc,
		ri_cast<LPARAM>(this));
}

INT_PTR CALLBACK CAbout::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CAbout *obj(ri_cast<CAbout*>((uMsg != WM_INITDIALOG) ? GetWindowLong(hWnd, DWL_USER) : lParam));
	switch (uMsg)
	{
		HANDLE_MSG(hWnd, WM_CAPTURECHANGED, obj->OnCaptureChanged);
		HANDLE_MSG(hWnd, WM_COMMAND,        obj->OnCommand);
		HANDLE_MSG(hWnd, WM_CTLCOLORSTATIC, obj->OnCtlColorStatic);
		HANDLE_MSG(hWnd, WM_DESTROY,        obj->OnDestroy);
		HANDLE_MSG(hWnd, WM_ERASEBKGND,     obj->OnEraseBkgnd);
		HANDLE_MSG(hWnd, WM_INITDIALOG,     obj->OnInitDialog);
		HANDLE_MSG(hWnd, WM_LBUTTONDOWN,    obj->OnLButtonDown);
		HANDLE_MSG(hWnd, WM_LBUTTONUP,      obj->OnLButtonUp);
		HANDLE_MSG(hWnd, WM_MOUSEMOVE,      obj->OnMouseMove);
		HANDLE_MSG(hWnd, WM_SETCURSOR,      obj->OnSetCursor);
		HANDLE_MSG(hWnd, WM_WINDOWPOSCHANGING, obj->OnWindowPosChanging);
	}
	return FALSE;
}

BOOL CAbout::OnWindowPosChanging(HWND hWnd, WINDOWPOS *wpos)
{
	return TRUE;
}

VOID CALLBACK CAbout::ChangeBackground(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	CAbout* obj(ri_cast<CAbout*>(idEvent));
	obj->current_bk_colour = (obj->current_bk_colour + 1) % obj->num_bk_colours;
}

VOID CALLBACK CAbout::DrawStroke(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	CAbout* obj(ri_cast<CAbout*>(idEvent - 1));
	if (!obj->painting)
		return;
	// seed random number generator
	srand(clock());
	// draw a stroke
	for (size_t r(0); r != brush_size; ++r)
		for (size_t c(0); c != brush_size; ++c)
		{
			float p(static_cast<float>(rand()));
			p /= RAND_MAX;
			if (p > obj->brush[r][c])
			{
				const size_t x(r - brush_size / 2);
				const size_t y(c - brush_size / 2);
				SetPixelV(
					obj->bk_dc,
					obj->cursor_pos.x + x,
					obj->cursor_pos.y + y,
					obj->bk_colours[obj->current_bk_colour]);
			}
		}
	InvalidateRect(hWnd, NULL, TRUE);
}

BOOL CAbout::OnCaptureChanged(HWND hWnd, HWND capture_reciever)
{
	painting = false;
	return FALSE;
}

BOOL CAbout::OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDOK:
	case IDCANCEL:
		EndDialog(hWnd, 0);
		return TRUE;
	}
	return FALSE;
}

// WM_CTLCOLORSTATIC handler
HBRUSH CAbout::OnCtlColorStatic(HWND hWnd, HDC dc, HWND child_wnd, int type)
{
	SetBkMode(dc, TRANSPARENT);
	return ctl_brush;
}

// WM_DESTROY handler
BOOL CAbout::OnDestroy(HWND hWnd)
{
	// destroy the background dc
	DeleteDC(bk_dc);
	DeleteObject(bk_bmp);
	return FALSE;
}

// WM_ERASEBKGND handler
BOOL CAbout::OnEraseBkgnd(HWND hWnd, HDC dc)
{
	// draw background
	RECT client_rect;
	GetClientRect(hWnd, &client_rect);
	BitBlt(
		dc,
		0,
		0,
		client_rect.right - client_rect.left,
		client_rect.bottom - client_rect.top,
		bk_dc,
		0,
		0,
		SRCCOPY);
	// draw icon
	ImageList_Draw(icon_list, 0, dc, 12, 12, ILD_NORMAL);
	return TRUE;
}

// WM_INITDIALOG handler
BOOL CAbout::OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
	SetWindowLong(hWnd, DWL_USER, lParam);
	// initialise the background bitmap
	{
		RECT client_rect;
		GetClientRect(hWnd, &client_rect);
		HDC client_dc(GetDC(hWnd));
		bk_dc = CreateCompatibleDC(client_dc);
		bk_bmp = CreateCompatibleBitmap(
			client_dc,
			client_rect.right - client_rect.left,
			client_rect.bottom - client_rect.top);
		ReleaseDC(hWnd, client_dc);
		SelectObject(bk_dc, bk_bmp);
		HBRUSH brush(CreateSolidBrush(bk_colours[0]));
		FillRect(bk_dc, &client_rect, brush);
		DeleteObject(brush);
	}
	// set timers
	SetTimer(hWnd, ri_cast<UINT_PTR>(this),     12000, ChangeBackground);
	SetTimer(hWnd, ri_cast<UINT_PTR>(this) + 1, 128,   DrawStroke);
	return TRUE;
}

// WM_LBUTTONDOWN handler
BOOL CAbout::OnLButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	SetCapture(hWnd);
	cursor_pos.x = x;
	cursor_pos.y = y;
	painting = true;
	return FALSE;
}

// WM_LBUTTONUP handler
BOOL CAbout::OnLButtonUp(HWND hWnd, int x, int y, UINT keyFlags)
{
	painting = false;
	ReleaseCapture();
	return FALSE;
}

// WM_MOUSEMOVE handler
BOOL CAbout::OnMouseMove(HWND hWnd, int x, int y, UINT codeHitTest)
{
	cursor_pos.x = x;
	cursor_pos.y = y;
	return FALSE;
}

// WM_SETCURSOR handler
BOOL CAbout::OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg)
{
	// set cursor to hand in client area, but to normal elsewhere
	RECT client_rect;
	POINT cursor_pos;
	GetClientRect(hWnd, &client_rect);
	{
		POINT corner;
		// top left
		corner.x = client_rect.left;
		corner.y = client_rect.top;
		ClientToScreen(hWnd, &corner);
		client_rect.left = corner.x;
		client_rect.top  = corner.y;
		// bottom right
		corner.x = client_rect.right;
		corner.y = client_rect.bottom;
		ClientToScreen(hWnd, &corner);
		client_rect.right  = corner.x;
		client_rect.bottom = corner.y;
	}
	GetCursorPos(&cursor_pos);
	if (TRUE == PtInRect(&client_rect, cursor_pos))
		SetCursor(LoadCursor(NULL, IDC_HAND));
	else
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	return TRUE;
}