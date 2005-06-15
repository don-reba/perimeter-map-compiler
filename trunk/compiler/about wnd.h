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
public:
	About(void);
	~About(void);
// interface
public:
	INT_PTR DoModal(HWND parent_wnd_);
// message handlers
private:
	void OnCaptureChanged(Msg<WM_CAPTURECHANGED> &msg);
	void OnCommand       (Msg<WM_COMMAND>        &msg);
	void OnCtlColorStatic(Msg<WM_CTLCOLORSTATIC> &msg);
	void OnDestroy       (Msg<WM_DESTROY>        &msg);
	void OnEraseBkgnd    (Msg<WM_ERASEBKGND>     &msg);
	void OnInitDialog    (Msg<WM_INITDIALOG>     &msg);
	void OnLButtonDown   (Msg<WM_LBUTTONDOWN>    &msg);
	void OnLButtonUp     (Msg<WM_LBUTTONUP>      &msg);
	void OnMouseMove     (Msg<WM_MOUSEMOVE>      &msg);
	void OnSetCursor     (Msg<WM_SETCURSOR>      &msg);
// internal function
protected:
	void ProcessMessage(WndMsg &msg);
private:
	static VOID CALLBACK ChangeBackground(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	static VOID CALLBACK DrawStroke(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
// data
private:
	static const size_t num_bk_colours_ = 4;     // number of background colours to cycle through
	static const size_t brush_size_ = 64;        // diameter of the brush in pixels
	COLORREF   bk_colours_[num_bk_colours_];     // background colours to cycle through
	float      brush_[brush_size_][brush_size_]; // probability that a pixel under the brush will be painted
	size_t     current_bk_colour_;               // index of the current stroke colour
	HBRUSH     ctl_brush_;                       // a hollow brush for static controls
	HIMAGELIST icon_list_;                       // a list for drawing the shrub icon
	HBITMAP    bk_bmp_;                          // bitmap to back the background DC
	HDC        bk_dc_;                           // background DC
	bool       painting_;                        // determines whether DrawStroke should act or not
	POINT      cursor_pos_;                      // position of the cursor for painting
};
