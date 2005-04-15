#pragma once

#include <commctrl.h>

class CAbout
{
public:
	CAbout(void);
	~CAbout(void);
public:
	// interface
	INT_PTR DoModal(HINSTANCE hInstance, HWND hParentWnd);
protected:
	// window functions
	static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static VOID    CALLBACK ChangeBackground(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	static VOID    CALLBACK DrawStroke(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	// message handlers
	BOOL   OnCaptureChanged(HWND hWnd, HWND capture_reciever);
	BOOL   OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify);
	HBRUSH OnCtlColorStatic(HWND hWnd, HDC dc, HWND child_wnd, int type);
	BOOL   OnDestroy(HWND hWnd);
	BOOL   OnEraseBkgnd(HWND hWnd, HDC dc);
	BOOL   OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);
	BOOL   OnLButtonDown(HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
	BOOL   OnLButtonUp(HWND hWnd, int x, int y, UINT keyFlags);
	BOOL   OnMouseMove(HWND hWnd, int x, int y, UINT codeHitTest);
	BOOL   OnSetCursor(HWND hWnd, HWND hWndCursor, UINT codeHitTest, UINT msg);
	BOOL   OnWindowPosChanging(HWND hWnd, WINDOWPOS *wpos);
protected:
	// data
	static const size_t num_bk_colours = 4;   // number of background colours to cycle through
	static const size_t brush_size = 64;      // diameter of the brush
	COLORREF   bk_colours[num_bk_colours];    // background colours to cycle through
	float      brush[brush_size][brush_size]; // probability that a pixel under the brush will be painted
	size_t     current_bk_colour;             // index of the current stroke colour
	HBRUSH     ctl_brush;                     // a hollow brush for static controls
	HIMAGELIST icon_list;                     // a list for drawing the shrub icon
	HBITMAP    bk_bmp;                        // bitmap to back the background DC
	HDC        bk_dc;                         // background DC
	bool       painting;                      // determines wheter DrawStroke should act or not
	POINT      cursor_pos;                    // position of the cursor for painting
};
